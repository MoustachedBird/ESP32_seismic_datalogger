#include "freertos/FreeRTOS.h" //Main FreeRTOS header

#include "freertos/task.h"  //task manager header
#include "freertos/event_groups.h" //global flags


/*-=-=-=-=-=-=-=-=-=-=- Event Flags -=-=-=-=-=-=-=-=-=-=*/

//Define event flags
#define FLAG_SD_MOUNTED         (1 << 0) // true when sd is mounted, false unmounted
#define FLAG_SD_AVAILABLE       (1 << 1) // true when sd is not busy, false if it is busy
#define FLAG_FILES_AVAILABLE    (1 << 2) // true there are pending files in SD card, false there are not files

#define FLAG_WIFI_CONNECTED     (1 << 3) // true when connected, false disconnected
#define FLAG_WIFI_AVAILABLE     (1 << 4) // true when sd is not busy, false if it is busy
#define FLAG_WIFI_FAIL          (1 << 5) // true when wifi fails, false everyting ok

#define FLAG_ITEMS_IN_WIFI_QUEUE  (1 << 6) // true when there're elements in wifi queue, false no items 
#define FLAG_ITEMS_IN_SD_QUEUE    (1 << 7) // true when there're elements in SD queue, false no items

#define ALL_FLAGS_HARDWARE   FLAG_SD_MOUNTED|FLAG_SD_AVAILABLE|FLAG_FILES_AVAILABLE|FLAG_WIFI_CONNECTED|FLAG_WIFI_AVAILABLE|FLAG_WIFI_FAIL|FLAG_ITEMS_IN_WIFI_QUEUE|FLAG_ITEMS_IN_SD_QUEUE

// Declare a variable to hold the created event group
EventGroupHandle_t flags_hardware_available;



/*-=-=-=-=-=-=-=-=-=-=- Handlers -=-=-=-=-=-=-=-=-=-=*/
//Task IDs for calling the tasks from the interrupt vector (timer_isr)

TaskHandle_t get_data_adc_mcp3561_taskID; //Task handler ADC 24 bit 

TaskHandle_t get_data_mma8451q_taskID; //Task handler mma8451q 14 bit accelerometer 

TaskHandle_t get_data_adxl355_taskID; //Task handler adxl355 20 bit accelerometer 





/*=================================================================
             TO CONVERT BITS TO ACCELERATION/VELOCITY UNITS
===================================================================*/

/* -----------    MMA8451Q (14 BITS ACCELEROMETER)    ----------------------

Para convertir a aceleracion mili g (mg) 
medicion_procesada = ((data_mma8451q[0] << 6) | (data_mma8451q[1]) >> 2); para 14 bits
if (medicion_procesada > 8191) medicion_procesada -= 2*2047; 

aceleracion=(medicion_procesada*1000)/4096 para sacar el valor de aceleracion en mg (14 bits)


 -----------    ADXL355 (20 BITS ACCELEROMETER)    ----------------------

Para convertir a aceleracion mili g (mg)
medicion_procesada=(data_received[0]<<12)|(data_received[1]<<4)|(data_received[2]>>4); //para 20 bits    
if (medicion_procesada > 524287) medicion_procesada -= 524287*2; //para 20 bits

aceleracion=medicion_procesada/256    para 20 bits en mg
    

 -----------    SM-24 (24 BITS ADC)    ----------------------
Para convertir a unidades de velocidad cm/s
medicion_procesada=(data_received[0]<<16)|(data_received[1]<<8)|(data_received[2]); //para 24 bits
if (medicion_procesada > 8388607) medicion_procesada -= 8388607*2; //para 24 bits

sensibilidad del SM-24 = 28.8 V/m/s = 0.288 V/cm/s
para calcular velocidad = (medicion_procesada*1.65)/(8388607*0.288*amplificacion); en cm/s
        

*/
        