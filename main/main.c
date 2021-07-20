#include <stdio.h>
#include <stdlib.h>  //memory allocation/free functions
#include <string.h>

/*-=-=-=-=-=-=-=-=- Custom headers -=-=-=-=-=-=-=-=-=-=*/
#include "task_list.h" //General handler, header and event bits list

#include "i2c_config.h" //I2C bus configuration (sda, scl pins) header
#include "spi_config.h" //SPI bus configuration (miso, mosi, clk pins) header

#include "spi_mcp356x.h" //Header to control 24 bit ADC
#include "spi_adxl355.h" //Header to control 20 bit accelerometer
#include "i2c_mma8451q.h" //14 bit accelerometer header
#include "i2c_ds3231.h" //real time clock header

#include "timer_conf.h" //Timer configuration 
#include "sd_config.h" //SD functions and configurations

#include "wifi_functions.h" //Wifi functions and configurations
#include "http_functions.h" //Http functions 
#include "sntp_config.h" //to update date and time by internet 


/*-=-=-=-=-=-=-=-=-=-=- FreeRTOS headers -=-=-=-=-=-=-=-=-=-=*/
#include "sdkconfig.h"
#include "esp_log.h"  
#include "driver/gpio.h" //gpio manager header

#include "freertos/queue.h" //queues manager header
#include "freertos/semphr.h" //semaphores manager header
#include "nvs_flash.h"




/*-=-=-=-=-=-=-=-=-=-=- Global variables -=-=-=-=-=-=-=-=-=-=*/
#define DELAY_FOR_INIT 20000 /*General delay in miliseconds (20 seconds) for tasks and resource initialization*/

char * TAG = "MAIN";

//Semaphore handler to notify rtc task when has to look for date and time
SemaphoreHandle_t semaphore_rtc_ds3231 = NULL;

/*-=-=-=-=-=-=-=-=-=-=- Queues -=-=-=-=-=-=-=-=-=-=*/
//2500
#define MAX_NUM_MSG 700  //Maximum number of messages per queue 
#define BYTES_PER_MSG_16_BIT 2  //To store 16 bits of information per message
#define BYTES_PER_MSG_24_BIT 3  //To store 24 bits of information per message


//Queue to save values of 14 BIT accelerometer
xQueueHandle queue_mma8451q;

//Queue to save values of 20 BIT accelerometer
xQueueHandle queue_adxl355;

//Queue to save values of 24 BIT ADC mcp3561
xQueueHandle queue_adc_mcp3561;


//Queue to save EMPTY buffer pointers
xQueueHandle queue_empty_buffers;

//Queue to save FULL buffer pointers
xQueueHandle queue_full_buffers;

//Queue to save DATE and TIME in ascii format YY MM DD HH mm SS 
xQueueHandle queue_date_time;


//Queue to save the buffer that will be stored in SD card
xQueueHandle queue_to_save_in_sd;

//Queue to save the buffer that will be send over WiFi
xQueueHandle queue_to_send_wifi;

//Queue to store filename lists of files in SD card
xQueueHandle queue_filename_list;


/*-=-=-=-=-=-=-=-=-=-=- Buffers =-=-=-=-=-=-=-=-=-=-=*/
/*
General structure

Number of sensors = ADC + MMA8451Q (XYZ) + ADXL355 (XYZ) = 7 sensors 

Maximum allowed buffer_size = 27025 bytes (higher the value causes an error using wifi communication).
Note: this value is an stimate.

Maximum time allowed (assuming 100 Hz sampling rate, 1500 items per sensor and 7 sensors) 
                                       =  (ITEMS_PER_SENSOR)/(SAMPLE RATE)
                                       =  (1500)/(100) = 15 seconds per buffer

                 =============================================================================================================================               
                 ||                                         EXAMPLE BUFFER                                                                  ||        
                 =============================================================================================================================            
                 || CONTROL_BYTES |   SENSOR 1  |  SENSOR 2 | SENSOR 3  | SENSOR 4  |  SENSOR 5  |  SENSOR 6  |  SENSOR 7  ||  STATUS BYTE  ||        
                 ||--------------------------------------------------------------------------------------------------------||---------------|| 
Number of items  ||       1       |    1500     |    1500   |   1500    |   1500    |   1500     |    1500    |    1500    ||       1       ||   Number of items     
Bytes per item   ||      18       |       3     |       3   |      3    |      3    |      2     |       2    |       2    ||       1       ||   Bytes per item
Total bytes      ||      18       | id + 4500   | id + 4500 | id + 4500 | id + 4500 |  id + 3000 |  id + 3000 |  id + 3000 ||       1       ||   Total bytes
                 ||--------------------------------------------------------------------------------------------------------||---------------||
                 || CONTROL_BYTES | ADC 24 BITS | X ADXL355 | Y ADXL355 | Z ADXL355 | X mma8451q | Y mma8451q | Z mma8451q ||  (Not sent)   ||     
                 =============================================================================================================================              

NOTE: Every sensor has an ID that helps the server to identify the sensor name and to convert data. 
(that's why every sensor has an extra byte, +id or +1 byte). 

The last byte (STATUS BYTE) is just for internal microcontroller processes and is not sent. 
If STATUS BYTE = 0 it means that the buffer was filled with current information of the sensors.
If STATUS BYTE = 1 it means that the buffer was filled with SD information (not current information).

TOTAL BYTES that will be sent:   18 + 4501 + 4501 + 4501 + 4501 + 3001 + 3001 + 3001  
                                 = 27025 bytes

TOTAL BYTES (considering STATUS byte)= 27026
                     
                      -------------------------------------------------------------------------------------------------------------------------------------
Where CONTROL_BYTES = | ITEMS_PER_SENSOR (2 bytes) |  NUMBER_OF_SENSORS (1 byte) | SAMPLE_RATE (2 bytes) | ID_STATION (1 byte) |  LOCAL_DATETIME(12 bytes) |                      
                      -------------------------------------------------------------------------------------------------------------------------------------                
      CONTROL_BYTES = 2 + 1 + 2 + 1 + 12 = 18 bytes

Note: LOCAL_DATETIME if not used 12 bytes are equal to 0 (size defined in i2c_ds3231.h file). 
If used this date and time are sampled when the first sensor byte is sampled, the format is:
YY MM DD HH mm SS 



1 megabit = 125 000 bytes, assumming a WiFi speed of 5 megabit/second means 625000 bytes/s
it means...

27025 bytes will be uploaded in 0.0432 seconds...
*/

#define STATUS_BYTE_SD_DATA 1     //The buffer was filled with SD information
#define STATUS_BYTE_SENSOR_DATA 0 //The buffer was filled with sensor's information (current information)



#define CONTROL_BYTES 18 //Number of control bytes
#define ITEMS_PER_SENSOR 1500  //Maximum number of samples (1 item = 1 sample) per sensor   
#define NUMBER_OF_SENSORS 7  //Number of sensors
//SAMPLE_RATE defined in timer_conf.h
#define ID_STATION 'A'  //Station ID 

/*Maximum size of buffer

In this example there are 4 sensors of 24 bit and 3 sensors of 16 bit. For this case,
max_buffer_size = 27025 bytes.

max_buffer_size calculated in buffer_general_calc function
*/
uint16_t max_buffer_size= 0;

//How many bytes considering every sensor ex: 3 bytes sensor 1, 2 bytes sensor 2, this variable will be equal to 5 (calculated in general_calc function)
uint16_t total_bytes_sensors= 0;

/*This program can switch between empty and full buffers. Empty buffers will be filled and Full buffers
  will be sent over WiFi. Once one buffer is sent it will be on the Empty Buffer list again.
*/
#define NUMBER_OF_BUFFERS 3 //Number of buffers that will be considered in the empty buffer list at the begining


/*
ID of each sensor
0 = SM-24 (ADC 24 BITS mcp3561)
1 = ADXL355 X axis 
2 = ADXL355 Y axis
3 = ADXL355 Z axis
4 = mma8451q X axis
5 = mma8451q Y axis
6 = mma8451q Z axis

... define more if neccesary
*/

//declare sensors in the same order that will be sent
const uint8_t    id_sensor[] = {0,1,2,3,4,5,6}; 

//declare resolution in bytes of each sensor in the same order that will be sent
const uint8_t bytes_p_item[] = {3,3,3,3,2,2,2}; 

//declare sensor offset in bytes (for buffer making)
uint16_t offset_buffer_per_sensor[NUMBER_OF_SENSORS]; 




/*======================================================================
 * OFFSET BUFFER CALCULATION FUNCTION
 * 
 * Prepare offset buffers per sensor and calculates the maximum number
 * of bytes of the buffer according the previous definitions and 
 * configurations
 ======================================================================*/
void buffer_general_calc(void){
    //calculates de maximum number of bytes in the buffer 
    max_buffer_size= CONTROL_BYTES+NUMBER_OF_SENSORS;

    offset_buffer_per_sensor[0]=CONTROL_BYTES+1; //Control buffers + ID (Sensor 1)
    
    for(int each_sensor=0;each_sensor<NUMBER_OF_SENSORS-1;each_sensor++){
        //SENSOR N
        offset_buffer_per_sensor[each_sensor+1]=offset_buffer_per_sensor[each_sensor]+ 1 + bytes_p_item[each_sensor]*ITEMS_PER_SENSOR;  
    
        //calculates de maximum number of bytes in the buffer
        max_buffer_size = max_buffer_size + ITEMS_PER_SENSOR*bytes_p_item[each_sensor];
        
        //calculates total bytes considering 1 item and all sensors
        total_bytes_sensors+=bytes_p_item[each_sensor];
    } 
    max_buffer_size = max_buffer_size + ITEMS_PER_SENSOR*bytes_p_item[NUMBER_OF_SENSORS-1];
    total_bytes_sensors+=bytes_p_item[NUMBER_OF_SENSORS-1];

    printf("Buffer calc function: max_buffer_size (without considering STATUS_BYTE) = %d\n",max_buffer_size);
    printf("Buffer calc function: total_bytes_sensors = %d\n",total_bytes_sensors);
}


/*======================================================================
 * RESET BUFFERS FUNCTION
 * 
 * Prepare last labels of the buffer (this functions works just once per reboot).
 * This function adds the ID of the station and sensor, sample rate, etc,
 * values that doesn't change during the program's execution.
 ======================================================================*/
void reset_buffer(char * buffer){
    printf("Reset_buffer: Pointer %p and ITEMS_PER_SENSOR: %d\n",buffer,ITEMS_PER_SENSOR);

    //to add sensor's ID to the empty buffer
    uint16_t each_id=0;

    //ITEMS PER SENSOR (2 bytes)
    //buffer[0] = MSB ITEMS_PER_SENSOR
    buffer[0]=(int8_t)((ITEMS_PER_SENSOR & 0xFF00)>>8); //MBS
    //buffer[0]='I'; //TEST
    //buffer[1] = LSB ITEMS_PER_SENSOR
    buffer[1]=(int8_t)(ITEMS_PER_SENSOR & 0x00FF); //LBS
    //buffer[1]='I'; //TEST
    printf("Reset_buffer: Done (ITEMS_PER_SENSOR=%d)\n",ITEMS_PER_SENSOR);

    //NUMBER OF SENSORS (1 byte)
    //buffer[2] = Number of sensors 
    buffer[2]=(int8_t)(NUMBER_OF_SENSORS);
    //buffer[2]+=48; //TEST 
    printf("Reset_buffer: Done (number_of_sensors=%d)\n",NUMBER_OF_SENSORS);

    //SAMPLE RATE (2 bytes)
    //buffer[3] = MSB 
    buffer[3]=(int8_t)((SAMPLE_RATE & 0xFF00)>>8); //MBS
    //buffer[3]='h'; //TEST
    //buffer[4] = LSB 
    buffer[4]=(int8_t)(SAMPLE_RATE & 0x00FF); //LBS
    //buffer[4]='z'; //TEST
    printf("Reset_buffer: Done (SAMPLE_RATE=%d)\n",SAMPLE_RATE);

    //ID STATION (1 byte)
    //buffer[5] = ID_STATION 
    buffer[5]=(int8_t)(ID_STATION); //8 bits
    printf("Reset_buffer: Done (ID_STATION=%d)\n",ID_STATION);

    //LOCAL_DATETIME (12 bytes), not used, so equal to 0
    buffer[6]=0; //Y
    buffer[7]=0; //Y
    buffer[8]=0; //M
    buffer[9]=0;  //M
    buffer[10]=0; //D
    buffer[11]=0; //D
    buffer[12]=0; //H
    buffer[13]=0; //H
    buffer[14]=0; //m
    buffer[15]=0; //m
    buffer[16]=0; //S
    buffer[17]=0; //S
    printf("Reset_buffer: Done (local_date_time=0)\n");

    //SENSOR  
    for (int8_t i=0;i<NUMBER_OF_SENSORS;i++){
        each_id = offset_buffer_per_sensor[i]-1;
        buffer[each_id]=id_sensor[i];
        //buffer[each_id]+=48; //TEST
        printf("Reset_buffer: Done (buffer[%d]=sensor id %d)\n",each_id,id_sensor[i]);
    }
}



/*======================================================================
 *0  SEND BUFFER TO WIFI TASK
 
 *  This task sends the full buffer to server by WIFI
 =======================================================================*/
void send_buffer_wifi_task(void *pvParameters){
    
    char *current_full_buffer=NULL;

    esp_err_t error_handler=ESP_OK; //to store the return error of post function if everything ok, return value from function will be equal to ESP_OK
    
    unsigned int retry_times=0;
    while (1)
    {
        //Receive the buffer to store in SD card
        xQueueReceive(queue_to_send_wifi,&current_full_buffer,portMAX_DELAY);   
        
        //Wifi busy flag
        xEventGroupWaitBits(flags_hardware_available, FLAG_WIFI_AVAILABLE, true, true, portMAX_DELAY);
        
        //send the buffer to the server
        printf("Send buffer by wifi: sending buffer by wifi\n");

        //send buffer to server by HTTP POST method
        error_handler = http_post_send(current_full_buffer,max_buffer_size, MONGODB_SERVER);
            
        //if no error, empty the current buffer
        if (error_handler==ESP_OK){ 
            //Clear the current buffer
            xQueueSendToBack(queue_empty_buffers, &current_full_buffer,portMAX_DELAY);
            retry_times=0;
        }
        //if error message then try to send buffer again
        else if (retry_times<MAX_RETRY_SEND_WIFI){
            printf("SEND_WIFI: FAIL, sending to the queue again...\n");
            xQueueSendToFront(queue_to_send_wifi, &current_full_buffer,portMAX_DELAY);
            retry_times++;
        }
        //if retry times exceded and SD connected then send the buffer to SD queue
        else if ((xEventGroupGetBits(flags_hardware_available)&FLAG_SD_MOUNTED)!=0){
            printf("SEND_WIFI: FAIL, sending to SD card...\n");
            xQueueSendToBack(queue_to_save_in_sd, &current_full_buffer,portMAX_DELAY);
            retry_times=0;
        }
        //if retry times exceded but NO SD CARD then try to send the buffer again
        else{
            printf("SEND_WIFI: FAIL, No SD card so... sending to the queue again...\n");
            xQueueSendToFront(queue_to_send_wifi, &current_full_buffer,portMAX_DELAY);
        }
        
        //Wifi free Flag
        xEventGroupSetBits(flags_hardware_available, FLAG_WIFI_AVAILABLE);
    }
}


/*======================================================================
 *1  SEND BUFFER TO SD CARD TASK
 
 *  This task selects one full buffer and sends it to SD card 
 =======================================================================*/
void send_buffer_to_SD_task(void *pvParameters){
    
    /*Filename depends of the local datetime, 12 chars: YY MM DD HH mm SS*/
    char filename_datetime[SIZE_CHAR_FOLDER+DATETIME_SIZE_FORMAT] = FOLDER"/"; 

    char *current_full_buffer=NULL;

    //maximum size of the route (number of characters)
    uint8_t max_size_route= SIZE_CHAR_FOLDER;
    
    //file handler
    FILE* file = NULL;

    while (1)
    {
        //Receive the buffer to store in SD card
        xQueueReceive(queue_to_save_in_sd,&current_full_buffer,portMAX_DELAY);   
        
        //SD busy flag
        xEventGroupWaitBits(flags_hardware_available, FLAG_SD_AVAILABLE, true, true, portMAX_DELAY);
        
        //Get the filename
        printf("Send buffer to SD task: creating filename\n");
        for (uint8_t i=0;i<DATETIME_SIZE_FORMAT;i++){
            filename_datetime[max_size_route+i]=current_full_buffer[6+i];    
        }

        printf("GRABAR EN SD TASK: abriendo la ruta %s\n",filename_datetime);
        /* Use POSIX and C standard library functions to work with files.
        Open the file
        */
        file = fopen(filename_datetime,"wb");
        if (file == NULL) {
            ESP_LOGE(TAG, "Failed to open file for writing");
            return;
        }
        
        //parameters: buffer_write, size in bytes of each element, number of elements, file
        printf("GRABAR EN SD TASK: grabando informacion en el archivo\n");
        fwrite(current_full_buffer,max_buffer_size,1,file); // write the buffer in SD card
        
        printf("GRABAR EN SD TASK: cerrando archivo\n");
        fclose(file);
            
        printf("GRABAR EN SD TASK: archivo grabado\n");
        //SD free Flag
        xEventGroupSetBits(flags_hardware_available, FLAG_SD_AVAILABLE|FLAG_FILES_AVAILABLE);
        
        //Clear the current buffer
        xQueueSendToBack(queue_empty_buffers, &current_full_buffer,portMAX_DELAY);
    }
}



/*======================================================================
 *2  SELECT WHERE TO SEND FULL BUFFER (WIFI or SD) TASK
 
 *  This task select full buffers from the full buffer list and selects if 
 *  they will be sent to the server by wifi or stored on the SD card. 
 =======================================================================*/
void full_buffer_selection_go_to_task(void *pvParameters){
    
    //to recieve the current empty buffer and fill it
   char *current_full_buffer=NULL;

    //to save event flags
    EventBits_t hardware_available;
             
    while (1)
    {
        printf("=============================================================\n");
        printf("Items in adc queue = %d/%d\n",uxQueueMessagesWaiting(queue_adc_mcp3561),MAX_NUM_MSG);
        printf("Items in adxl355 queue = %d/%d\n",uxQueueMessagesWaiting(queue_adxl355),MAX_NUM_MSG);
        printf("Items in mma8451q queue = %d/%d\n\n",uxQueueMessagesWaiting(queue_mma8451q),MAX_NUM_MSG);
    
        printf("fill_buffer_with_sensor_task: full buffers queue = %d/%d\n",uxQueueMessagesWaiting(queue_full_buffers),NUMBER_OF_BUFFERS);
        printf("fill_buffer_with_sensor_task: empty buffers queue = %d/%d\n\n",uxQueueMessagesWaiting(queue_empty_buffers),NUMBER_OF_BUFFERS);
        printf("fill_buffer_with_sensor_task: pending files queue = %d/%d\n\n",uxQueueMessagesWaiting(queue_filename_list),MAX_OPEN_FILES*2);
        printf("fill_buffer_with_sensor_task: sd store queue = %d/%d\n",uxQueueMessagesWaiting(queue_to_save_in_sd),NUMBER_OF_BUFFERS);
        printf("fill_buffer_with_sensor_task: wifi send queue = %d/%d\n",uxQueueMessagesWaiting(queue_to_send_wifi),NUMBER_OF_BUFFERS);
        printf("=============================================================\n");

        current_full_buffer=NULL;
        xQueueReceive(queue_full_buffers,&current_full_buffer,500);

        /*Get SD and WiFi status (connected or disconnected)

        Parameters (event group bits, 
                    bit or bits, 
                    clear once excecute: true or false, 
                    wait for all bits are in 1 (true) or just one is neccesary to excecute the order (false)
                    maximum time to wait                
                    )
        */
        hardware_available = xEventGroupWaitBits(flags_hardware_available,FLAG_WIFI_CONNECTED|FLAG_SD_MOUNTED, false, false, 0);
        
        if((hardware_available & FLAG_WIFI_CONNECTED)== 0){
            printf("Selector task: WiFi Disconnected\n");
        }
        else{
            printf("Selector task: WiFi Connected\n");
        }

        if((hardware_available & FLAG_SD_MOUNTED)== 0){
            printf("Selector task: SD Disconnected\n");
        }
        else{
            printf("Selector task: SD Connected\n");
        }
        
        //-------------------- if buffer available ---------------------
        if (current_full_buffer!=NULL){
            if(current_full_buffer[max_buffer_size]==STATUS_BYTE_SENSOR_DATA){
                printf("Selector task: BUFFER filled with SENSOR\n");
            }
            else{
                printf("Selector task: BUFFER filled with SD\n");
            }
            
            //If wifi connected then... send the buffer by WiFi
            if ((hardware_available & FLAG_WIFI_CONNECTED)!= 0)
            {    
                printf("Selection task: WiFi send buffer\n"); 
                xQueueSendToBack(queue_to_send_wifi, &current_full_buffer,portMAX_DELAY);
            }
            /*
            If SD connected and buffer was filled with SENSOR data

            or  if SD connected and WiFi Disconnected then...
            
            store buffer data in SD card
            */
            else if(
            (((hardware_available & FLAG_WIFI_CONNECTED)== 0) || 
            (current_full_buffer[max_buffer_size]==STATUS_BYTE_SENSOR_DATA))
            && ((hardware_available & FLAG_SD_MOUNTED)!= 0)
            )
            {
                printf("Selection task: SD store buffer\n"); 
                xQueueSendToBack(queue_to_save_in_sd, &current_full_buffer,portMAX_DELAY);
            }
            /*If WiFi connected but is busy and buffer was filled with SD information then
            send to the back of the full buffer queue 
            */
            else{
                printf("Selection task: WARNING NEITHER SD NOR WIFI CONNECTED!!!\n\n");     
                xQueueSendToBack(queue_empty_buffers, &current_full_buffer,portMAX_DELAY);
            }
        }
        //-------------------- NO buffer available ---------------------
        else{
            printf("Selector task: No full buffer selected\n");

            //verify queues functionality 
            //if wifi disconnected and there are elements in wifi queue, they're stuck, so empty the queue
            if((hardware_available & FLAG_WIFI_CONNECTED)== 0){
                for (int i=0;i<uxQueueMessagesWaiting(queue_to_send_wifi);i++){
                    printf("Selector task: There stuck items in wifi queue, emptying the queuen\n");
                    xQueueReceive(queue_to_send_wifi,&current_full_buffer,100);
                    if (current_full_buffer!=NULL){
                        xQueueSendToBack(queue_full_buffers, &current_full_buffer,portMAX_DELAY);    
                    }
                }
            }

            //if SD disconnected and there are elements in SD queue, they're stuck, so empty the queue         
            if((hardware_available & FLAG_SD_MOUNTED)== 0){
                for (int i=0;i<uxQueueMessagesWaiting(queue_to_save_in_sd);i++){
                    printf("Selector task: There stuck items in SD queue, emptying the queuen\n");
                    xQueueReceive(queue_to_save_in_sd,&current_full_buffer,100);
                    if (current_full_buffer!=NULL){
                        xQueueSendToBack(queue_full_buffers, &current_full_buffer,portMAX_DELAY);    
                    }
                }
            }
        }
    }
}


/*=================================================================================
 *3 GET NAMES OF PENDING FILES
 * 
 * Check filename list of pending files
 =================================================================================*/
void get_filename_list_task(void *pvParameter)
{
    
    //matrix to store the list of files
    char filename_list[MAX_OPEN_FILES][MAX_FILENAME_SIZE];

    //total read files
    uint8_t total_read_files=0;

    //we assume there are files into SD card
    xEventGroupSetBits(flags_hardware_available, FLAG_FILES_AVAILABLE);
        
    while (1){
        //if SD busy then wait...
        xEventGroupWaitBits(flags_hardware_available, FLAG_SD_AVAILABLE, true, true, portMAX_DELAY);
        
        //get 5 (MAX_OPEN_FILES) filenames
        total_read_files = sd_list_files(filename_list,MAX_OPEN_FILES,uxQueueMessagesWaiting(queue_filename_list));
       
        //SD free flag
        xEventGroupSetBits(flags_hardware_available, FLAG_SD_AVAILABLE);

        //if less than 5 (MAX_OPEN_FILES) filenames, then there are no more files in SD card
        if (total_read_files<MAX_OPEN_FILES){
            printf("FILE LIST: There are no more files in SD, %d/%d read files\n",total_read_files,MAX_OPEN_FILES);
            xEventGroupClearBits(flags_hardware_available, FLAG_FILES_AVAILABLE);
        }

        printf("FILE LIST: List of files: \n");
        //send every read filename to the queue
        for (uint8_t i =0; i<total_read_files; i++){
            printf("FILE LIST: Adding file %.12s to the queue...\n",&filename_list[i][0]);
        
            xQueueSendToBack(queue_filename_list, &filename_list[i][0],portMAX_DELAY); 
        }        

        /*if there are no more files into files wait for them...
          also wait 
        */
        xEventGroupWaitBits(flags_hardware_available, FLAG_FILES_AVAILABLE|FLAG_WIFI_CONNECTED, false, true, portMAX_DELAY);
    }
}


/*======================================================================
 *4  FILL BUFFER WITH SD CARD TASK
 
 *  This task get data from sd and fills an empty buffer. One file is 
 *  equal to one full buffer.
 =======================================================================*/
void fill_buffer_with_sd_task(void *pvParameters){

    /*Filename depends of the local datetime, 12 chars: YY MM DD HH mm SS*/
    char filename_datetime[SIZE_CHAR_FOLDER+DATETIME_SIZE_FORMAT] = FOLDER"/"; 

    char *current_empty_buffer=NULL;

    //maximum size of the route (number of characters)
    uint8_t max_size_route= SIZE_CHAR_FOLDER;
    
    //file handler
    FILE* file = NULL;

    while (1)
    {
        //Check if WiFi is connected 
        printf("READ SD TASK: check if wifi connected\n");
        xEventGroupWaitBits(flags_hardware_available, FLAG_WIFI_CONNECTED, false, true, portMAX_DELAY);
        
        //if there is a filename in the queue take it an read the file
        xQueueReceive(queue_filename_list,&filename_datetime[max_size_route],portMAX_DELAY);   
        printf("READ SD TASK: filename taked from the queue %s\n",&filename_datetime[max_size_route]);
        
        //SD busy flag
        printf("READ SD TASK: look if SD available\n");
        xEventGroupWaitBits(flags_hardware_available, FLAG_SD_AVAILABLE, true, true, portMAX_DELAY);
        
        //if there is some free buffer then fill it with sensor information
        xQueueReceive(queue_empty_buffers,&current_empty_buffer,portMAX_DELAY);   
        
        printf("READ SD TASK: Current buffer %p    Openning the route %s\n",current_empty_buffer,filename_datetime);
        /* Use POSIX and C standard library functions to work with files.
        Open the file (rb = read binary)
        */
        file = fopen(filename_datetime,"rb");
        if (file == NULL) {
            ESP_LOGE(TAG, "Failed to open file for reading");
            return;
        }
        
        //parameters: buffer_write, size in bytes of each element, number of elements, file
        printf("READ SD TASK: Reading file content\n");
        fread(current_empty_buffer,max_buffer_size,1,file); // write the buffer in SD card
        
        printf("READ SD TASK: closing file\n");
        fclose(file);

        printf("READ SD TASK: Deleting file: %s \n",filename_datetime);
        remove(filename_datetime);   

        //Buffer was filled with SD card information
        current_empty_buffer[max_buffer_size]=STATUS_BYTE_SD_DATA; 

        //SD free Flag
        printf("READ SD TASK: Reading file done, SD available again\n");
        xEventGroupSetBits(flags_hardware_available, FLAG_SD_AVAILABLE);
        
        //Clear the current buffer
        xQueueSendToBack(queue_full_buffers, &current_empty_buffer,portMAX_DELAY);
    }
}

/*======================================================================
 *5  FILL BUFFER WITH SENSOR DATA TASK
 
 *  This task joins all data from every sensor and prepare the buffer
 *  (package) to send it over WiFi. Once one package is full is passed to 
 *  the task "send_over_wifi_task" to send it. Empty buffers are filled
 *  here. 
 =======================================================================*/
void fill_buffer_with_sensor_task(void *pvParameters){
    //All data, 3 bytes adc (24 bits), 6 bytes accl (16 bits x 3 channels), 9 bytes accl (24 bits x 3 channels) 
    uint8_t data_queue[total_bytes_sensors]; 

    //to recieve the current empty buffer and fill it
    char *current_empty_buffer=NULL;
    
    //to add items to the empty buffer
    uint16_t each_item=0;
    //to add each byte per item
    uint16_t each_byte_per_item=0;
    //to configurate each sensor in buffer
    uint8_t each_sensor=0;

    //to count positions in the current empty buffer
    uint16_t empty_buff_pos=0;
    //to count positions in the data_queue buffer
    uint16_t data_buff_pos=0;

    printf("fill_buffer_with_sensor_task : Prepared\n");    

    //delay for task and resource initialization...
    vTaskDelay(DELAY_FOR_INIT / portTICK_PERIOD_MS);

    while (1)
    {
        //if there is some free buffer then fill it with sensor information
        xQueueReceive(queue_empty_buffers,&current_empty_buffer,portMAX_DELAY);
        printf("Make_buffer: Current buffer = %p\n",current_empty_buffer);

        //look for current date and time 
        xSemaphoreGive(semaphore_rtc_ds3231);

        //Store each value in one general buffer
        for(each_item=0;each_item<ITEMS_PER_SENSOR;each_item++){
            /*    SM-24 GEOHPONE (24 BIT ADC MCP3561 )
            BYTES:     |  0  |   1   |  2  |
            adxl355:     MBS   middle  LSB  
            */
            xQueueReceive(queue_adc_mcp3561,&data_queue[0],portMAX_DELAY);
            
            /*    ADXL355
            BYTES:     |  0  |   1   |  2  |  3  |   4   |  5  |  6  |   7   |  8  |
            adxl355:    MBSX   middle LSBX   MSBY  middle LSBY  MBSZ   middle  LSBZ
            */
            xQueueReceive(queue_adxl355,&data_queue[3],portMAX_DELAY);
            
            /*    MMA8451Q
            BYTES:     |  0  |  1  |  2  |  3  |  4  |  5  |
            mma8451q:   MBSX  LSBX   MSBY  LSBY  MBSZ  LSBZ
            */
            xQueueReceive(queue_mma8451q,&data_queue[12],portMAX_DELAY);
        
            data_buff_pos=0;
            for(each_sensor=0;each_sensor<NUMBER_OF_SENSORS;each_sensor++){
                empty_buff_pos=offset_buffer_per_sensor[each_sensor]+each_item*bytes_p_item[each_sensor];
                for(each_byte_per_item=0;each_byte_per_item<bytes_p_item[each_sensor];each_byte_per_item++){
                    // TEST MODE
                    //current_empty_buffer[empty_buff_pos+each_byte_per_item]='s';
                    
                    //NORMAL OPERATION
                    current_empty_buffer[empty_buff_pos+each_byte_per_item]=data_queue[data_buff_pos];
                    data_buff_pos++;
                }   
            }
        }
        //Save date and time into the current empty buffer (from position 6 to 17, 12 bytes)
        xQueueReceive(queue_date_time,&current_empty_buffer[6],portMAX_DELAY); 
        printf("fill_buffer_with_sensor_task: date and time %.12s\n",&current_empty_buffer[6]);

        //Buffer was filled with sensor information
        current_empty_buffer[max_buffer_size]=STATUS_BYTE_SENSOR_DATA;

        //once buffer is full pass it to the full buffers queue
        printf("fill_buffer_with_sensor_task: Full buffer\n");
        //printf("Buffer content: %s\n",current_empty_buffer);

        xQueueSendToBack(queue_full_buffers,&current_empty_buffer,portMAX_DELAY);
    }
}


/*======================================================================
 *6  MMA8451Q TASK 14 BIT ACCELEROMETER TASK

 *      3ro en muestrearse
 *  Get data of the MMA8451Q accelerometer. This task only excecutes when
 *  receives a notification from the TIMER_ISR
 =======================================================================*/
void get_data_mma8451q_task(void *pvParameter)
{
    //Configurates the mma8451q sensor
    mma8451q_reg_config();
    
    //se declara el buffer de recepcion de datos
    uint8_t data_received[6]; // 8 bit data to save 14 bits per channel of the accelerometer (total 8*6 bits)

    //delay for task and resource initialization...
    vTaskDelay(DELAY_FOR_INIT / portTICK_PERIOD_MS);

    while(1){        
        // Sleep until the ISR gives us something to do, if nothing is recieved then waits forever
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

        mma8451q_read_accl(data_received,sizeof(data_received));

        
        //We send the acceleration values of all axis (x,y,z)
        xQueueSendToBack(queue_mma8451q, data_received,portMAX_DELAY);     
	}
}


/*======================================================================
 *7  ADXL355 TASK 20 BIT ACCELEROMETER TASK

 *     2do en muestrearse
 *  Get data of the ADXL355 accelerometer. This task only excecutes when
 *  receives a notification from the TIMER_ISR
 =======================================================================*/
void get_data_adxl355_task(void *pvParameter)
{
    /*Configurates SPI clock and SPI PINS*/
    adxl355_config_spi(); 
    vTaskDelay(100 / portTICK_PERIOD_MS);

    /*Configurates the accelerometer's sample rate*/
    adxl355_100hz_rate(); 
    /*Configurates and turns on the accelerometer (+-2G range)*/
    adxl355_range_conf();
    
    //se declara el buffer de recepcion de datos
    uint8_t data_received[9]; //data 3 bytes per channel (24 bits, only 20 used). In total 9 bytes considering three channels

    //delay for task and resource initialization...
    vTaskDelay(DELAY_FOR_INIT / portTICK_PERIOD_MS);

    while(1){        
        // Sleep until the ISR gives us something to do, if nothing is recieved then waits forever
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

        adxl355_read_accl(data_received,sizeof(data_received));

        //We send the acceleration values of all axis (x,y,z)
        xQueueSendToBack(queue_adxl355, data_received,portMAX_DELAY);     
	}
}


/*======================================================================
 *8  ADC MCP3561 24 BIT RESOLUTION TASK

 *      1ro en muestrearse
 *  Get data of the MCP3561 ADC. This task only excecutes when
 *  receives a notification from the TIMER_ISR
 =======================================================================*/
void get_data_adc_mcp3561_task(void *pvParameter)
{
    mcp356x_config_spi();   
    vTaskDelay(100 / portTICK_PERIOD_MS);      
    mcp356x_reg_config();
    
    //se declara el buffer de recepcion de datos
    uint8_t data_received[3]; //24 bits to store 24 bit adc resolution
    

    //delay for task and resource initialization...
    vTaskDelay(DELAY_FOR_INIT / portTICK_PERIOD_MS);

    while(1){        
        // Sleep until the ISR gives us something to do, if nothing is recieved then waits forever
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

        mcp356x_read_adc(data_received,sizeof(data_received));    
        
        //We send the acceleration values of 24 bit ADC (SM-24 geophone)
        xQueueSendToBack(queue_adc_mcp3561, data_received,portMAX_DELAY);      
    }
}


/*=================================================================================
 *9 GET DATA REAL TIME CLOCK DS3231 by I2C TASK 
 * 
 * Check date and time by the i2c bus
 =================================================================================*/
void get_data_rtc_ds3231_task(void *pvParameter)
{
    /*Array to store 12 bytes date and time 
    */
    char local_datetime[DATETIME_SIZE_FORMAT] = {0}; 

    printf("RTC-TASK: setting intial register configuration\n");
    ds3231_reg_config();

    //delay for task and resource initialization...
    vTaskDelay(DELAY_FOR_INIT / portTICK_PERIOD_MS);

    /*Initial date and time are set i sntp_config.c file*/
    while (1)
    {
        /* Sleep until the fill_buffer_with_sensor_task gives something to do, 
        if nothing is recieved then waits forever*/
        xSemaphoreTake(semaphore_rtc_ds3231,portMAX_DELAY);
        
        ds3231_get_datetime(local_datetime);
        
        //We send the date and time values to the queue
        xQueueSendToBack(queue_date_time, local_datetime,portMAX_DELAY);   
    }
    
}



/*=================================================================================
 *10 CHECK FREE RAM TASK 
 * 
 * Check free ram memory during system excecution
 =================================================================================*/
void check_free_ram_task(void *pvParameter)
{
    //Check free ram size in bytes
    while(1){
		ESP_LOGI(TAG, "free RAM: %d bytes",esp_get_free_heap_size());
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}


/*======================================================================
 *  MAIN TASK

 *  Main task, this function is excecuted when the ESP32 is rebooting
 =======================================================================*/
void app_main()
{
    ESP_LOGI(TAG,"APP MAIN: Allow other core to finish initialization...");
	//Allow other core to finish initialization
    vTaskDelay(200 / portTICK_PERIOD_MS);
    
    /*
    ----------------------------------------------------------------------
    -------------------- { MEMORY ALLOCATION } --------------------
    ----------------------------------------------------------------------
    */
    //buffer offset and maximum number of bytes
    buffer_general_calc();
    vTaskDelay(100 / portTICK_PERIOD_MS);
   
    printf("Memory allocation\n");
    //Prepare semaphore
    semaphore_rtc_ds3231 = xSemaphoreCreateBinary();
    
    //Prepare queues    
    queue_mma8451q = xQueueCreate(MAX_NUM_MSG, BYTES_PER_MSG_16_BIT*3); //16 bit per message (only 14 used) per channel (in total 16*3 bits) considering 3 channels
    queue_adxl355  = xQueueCreate(MAX_NUM_MSG, BYTES_PER_MSG_24_BIT*3); //24 bit per message (only 20 used) per channel (in total 24*3 bits) considering 3 channels
    queue_adc_mcp3561 = xQueueCreate(MAX_NUM_MSG, BYTES_PER_MSG_24_BIT); //24 bit per message (24 used) 
    ESP_LOGI(TAG, "Queues for data sensing have been created");
	vTaskDelay(100 / portTICK_PERIOD_MS);

    //Queue to store date and time of each buffers
    queue_date_time = xQueueCreate(NUMBER_OF_BUFFERS, DATETIME_SIZE_FORMAT);
    ESP_LOGI(TAG, "Queue for date and time created");
	vTaskDelay(100 / portTICK_PERIOD_MS);

    //Queues to storage full and empty buffers (queues storage only pointers)
    queue_full_buffers= xQueueCreate(NUMBER_OF_BUFFERS, sizeof(uint32_t)); //Number of messages = NUMBER_OF_BUFFERS, size of messages in bytes = 32 bits (4 bytes)
	queue_empty_buffers= xQueueCreate(NUMBER_OF_BUFFERS, sizeof(uint32_t));
    ESP_LOGI(TAG, "Queues for buffer pointers have been created");
	vTaskDelay(100 / portTICK_PERIOD_MS);

    //Queues to save in SD card or send over WIFI
    queue_to_save_in_sd= xQueueCreate(NUMBER_OF_BUFFERS, sizeof(uint32_t)); //Number of messages = NUMBER_OF_BUFFERS, size of messages in bytes = 32 bits (4 bytes)
	queue_to_send_wifi= xQueueCreate(NUMBER_OF_BUFFERS, sizeof(uint32_t));
    ESP_LOGI(TAG, "Queues to save in SD card or send over WIFI have been created");
	vTaskDelay(100 / portTICK_PERIOD_MS);
    
    //Queue to save filenames of pending files in SD card (MAX_OPEN_FILES defined in sd_config.h)
    queue_filename_list = xQueueCreate(MAX_OPEN_FILES*2, MAX_FILENAME_SIZE);
    ESP_LOGI(TAG, "Queue to read buffers from SD card");
	vTaskDelay(100 / portTICK_PERIOD_MS);

    //asing memory to the buffers
    char *buffer_list[NUMBER_OF_BUFFERS];  /* declare the array that will store the buffer list */
    
    for (int8_t i=0;i<NUMBER_OF_BUFFERS;i++){
        /*heap_caps_malloc(bytes_to_allocate, type_of_information)
        max_buffer_size + status byte (more information at the begining)
        */
        buffer_list[i] = heap_caps_malloc(max_buffer_size+1, MALLOC_CAP_8BIT);  
        /* allocate N bytes buffer (BUFFER_SIZE defined at the begining) in a block of internal RAM memory*/

        /*if memory allocated succesfully (malloc succesfully) will store the direction of memory
        of the allocation, other case buffers_pinters[i] = NULL
        */
        if (buffer_list[i]==NULL){
            ESP_LOGE("DRAM (Data RAM)","Failed to allocate buffer in DRAM");
        }
        else{
            //prints the id (pointer) of the memory allocation
            ESP_LOGI("DRAM (Data RAM)","Success in DRAM memory allocation, buffer_id[%d] = %p",i,buffer_list[i]);
            //if memory allocation ok then send the id pointer to the free (not busy) buffer queue
            ESP_LOGI("DRAM (Data RAM)","Sending buffer id to the empty buffer queue...");
            xQueueSendToBack(queue_empty_buffers, &buffer_list[i],portMAX_DELAY);
            ESP_LOGI("DRAM (Data RAM)","Preparing buffer (adding labels)");
            reset_buffer(buffer_list[i]);    
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
    
    /*
    ----------------------------------------------------------------------
    -------------------- { OFFLINE CONFIGURATIONS } ----------------------
    ----------------------------------------------------------------------
    */
    //set SPI bus configurations
    spi_bus_set_config();   
    vTaskDelay(100 / portTICK_PERIOD_MS);

    //Configurates i2c clock and pins
    i2c_bus_set_config();
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Attempt to create the event group (global flags)
    flags_hardware_available = xEventGroupCreate();
    /* Was the event group created successfully? */
    if( flags_hardware_available == NULL )
    {
        ESP_LOGE("EVENT_GROUP","Error not enought memory to create global flags");
    }
    else
    {
        ESP_LOGI("EVENT_GROUP","Success, global flags created");
    }
    printf("Reseting all event flags\n");
    //Clear flags_hardware_available
    xEventGroupClearBits(flags_hardware_available, ALL_FLAGS_HARDWARE);              
    vTaskDelay(100 / portTICK_PERIOD_MS);


    //set SD card configuration (SD card bus and SD card interface)
    sd_config_card(max_buffer_size);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    /*
    ---------------------------------------------------------------------- 
    -------------------- { OFFLINE TASKS } --------------------
    ----------------------------------------------------------------------
    */
    printf("Offline configurations\n");

    //1  create task: Fill empty buffers task with all data
    ESP_LOGI(TAG,"\nCreating task to fill the empty buffers with information..."); 
	xTaskCreate(fill_buffer_with_sensor_task, "fill_buffer_with_sensor_task", 8*1024, NULL, 7, NULL);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    //2  create task: Select where the full buffers go to (sd or wifi)
    ESP_LOGI(TAG,"\nCreating task to select where the full buffers go to (sd or wifi)"); 
	xTaskCreate(full_buffer_selection_go_to_task,"full_buffer_selection_go_to_task", 8*1024, NULL, 6, NULL);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    //3  create task: get data from SPI 24 Bit ADC mcp3561
    ESP_LOGI(TAG,"\nCreating task to take data from SPI 24 bit ADC mcp3561 ..."); 
	xTaskCreate(get_data_adc_mcp3561_task, "get_data_adc_mcp3561_task", 2*1024, NULL, 6, &get_data_adc_mcp3561_taskID);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    //4  create task: get data from SPI 20 bit accelerometer adxl355
    ESP_LOGI(TAG,"\nCreating task to take data from adxl355 SPI 20 bit accelerometer..."); 
	xTaskCreate(get_data_adxl355_task, "get_data_adxl355_task", 2*1024, NULL, 6, &get_data_adxl355_taskID);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    //5  create task: get data from i2c 14 bit accelerometer mma8451q
    ESP_LOGI(TAG,"\nCreating task to take data from mma8451q i2c 14 bit accelerometer..."); 
	xTaskCreate(get_data_mma8451q_task, "get_data_mma8451q_task", 2*1024, NULL, 6, &get_data_mma8451q_taskID);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    //6  create task: Check free ram memory available
    ESP_LOGI(TAG,"\nCreating task to check how much free RAM memory available..."); 
	xTaskCreate(check_free_ram_task, "check_free_ram_task", 2*1024, NULL, 1, NULL);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    //7  create task: Real time clock task (RTC)
    ESP_LOGI(TAG,"\nCreating RTC i2c task..."); 
	xTaskCreate(get_data_rtc_ds3231_task, "get_data_rtc_ds3231_task", 2*1024, NULL, 6, NULL);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    //8  create task: Send buffer to SD card task
    ESP_LOGI(TAG,"\nCreating send buffer to SD task..."); 
	xTaskCreate(send_buffer_to_SD_task, "send_buffer_to_SD_task", 2*1024, NULL, 7, NULL);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    //9  create task: Get filename list task
    ESP_LOGI(TAG,"\nCreating filename list SD task..."); 
	xTaskCreate(get_filename_list_task, "get_filename_list_task", 2*1024, NULL, 3, NULL);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    //10  create task: Fill buffer with SD data task
    ESP_LOGI(TAG,"\nFill buffer with SD data..."); 
	xTaskCreate(fill_buffer_with_sd_task, "fill_buffer_with_sd_task", 2*1024, NULL, 3, NULL);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    //0  create task: To send full buffers to the server by wifi
    ESP_LOGI(TAG,"\nCreating the task to send buffer to server by Wifi..."); 
	xTaskCreate(send_buffer_wifi_task, "send_buffer_wifi_task", 8*1024, NULL, 7, NULL);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    //conf_timer(); //configurate the timer 100 HZ sample rate

    printf("\n\n" 
    "                            ******\n"
    "          ***********      ***********\n"
    "       ******************************\n"
    "    ****************************\n"
    "  ********************************\n"
    "*******************************************\n"
    "                 ****************************\n"      
    "                   ****************************\n"
    "                 ******************************\n"
    "                *********              *******\n"
    "              **********                *****\n"
    "             ***********                ***\n"
    "             ***********  \n"
    "              **********\n"
    "                ******  \n"
    "            __          __   _       _     ____  _         _ \n"
    "            \\ \\        / /  | |     | |   |  _ \\(_)       | |\n"
    "             \\ \\  /\\  / __ _| |_ ___| |__ | |_) |_ _ __ __| |\n"
    "              \\ \\/  \\/ / _` | __/ __| \'_ \\|  _ <| | \'__/ _` |\n"
    "               \\  /\\  | (_| | || (__| | | | |_) | | | | (_| |\n"
    "                \\/  \\/ \\____|\\__\\___|_| |_|____/|_|_|  \\____|\n"
    "                                          ___  ___      _                 \n"
    "                                          |  \\/  | ___ | |_ _  ___  _ __\n"
    "                                          | |\\/| |/ _ \\| __| |/ _ \\| '_ \\ \n"
    "                                          | |  | | (_) | |_| | (_) | | | |\n"
    "                                          |_|  |_|\\___/ \\__|_|\\___/|_| |_|  \n"
    "\n"
    );  

    vTaskDelay(4000 / portTICK_PERIOD_MS);

    /*
    ---------------------------------------------------------------------- 
    -------------------- { ONLINE CONFIGURATIONS } --------------------
    ----------------------------------------------------------------------
    */
    printf("Online configurations\n");
    // initialize NVS (non volatil storage, required for wifi)
	ESP_ERROR_CHECK(nvs_flash_init());
    
    ESP_LOGI(TAG,"Wifi configurations..."); 
    wifi_init_sta();    
    vTaskDelay(200 / portTICK_PERIOD_MS);

    printf ("Timer configurations...\n");
    conf_timer(); //configurate the timer 100 HZ sample rate
    printf ("All configurations done, initializing system...\n");

    printf ("SNTP clock configurations...\n");
    sntp_config();
}