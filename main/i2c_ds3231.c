#include "driver/i2c.h" //Headers for I2C
#include "i2c_ds3231.h"



/*====================================================================================
FUNCTION: DS3231_REG_CONFIG

Turns on the RTC 
=====================================================================================*/
void ds3231_reg_config(void){
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	//vTaskDelay(200/portTICK_PERIOD_MS);
	printf("DS3231 RTC: i2c cmd link created\n");	
    
	//-----------------------[ Modify Control Register (0x0E) ]-----------------------------------------------
	/*Start I2C communication as MASTER*/
    i2c_master_start(cmd);
    /*Select the I2C device, in this case we write ds3231 adress to select it
    i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | I2C_MASTER_WRITE, ACK_EN);
    ACK= acknowledge bit.*/
    
    i2c_master_write_byte(cmd, DS3231_ADDRESS | I2C_MASTER_WRITE, ACK_EN);
    /*Select control register*/
    i2c_master_write_byte(cmd, REG_CONTROL, ACK_EN);
    /*Turns on the internal oscillator to initialize the clock functions*/
    i2c_master_write_byte(cmd, OSCILLATOR_EN|SUPPLY_BY_VCC|TEMP_ZERO|RATE_1_HZ|INT_SQW_AS_SQUARE|ALARM2_DIS|ALARM1_DIS, ACK_EN);
    //----------------------------------------------------------------------------------------------------

	/* Stop I2C communication*/
    i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

    printf("DS3231: i2c configuration finish\n");
    
}

/*====================================================================================
FUNCTION: DS3231_SET_DATETIME

Set an specific date and time 
=====================================================================================*/
void ds3231_set_datetime(char * local_datetime){
    uint8_t raw_datetime[7]={0};
    
    /*Parse local_datetime*/
    //YY 
    raw_datetime[6]=(local_datetime[0]<<4)|local_datetime[1];
    //MM 
    raw_datetime[5]=(local_datetime[2]<<4)|local_datetime[3];
    //DD 
    raw_datetime[4]=(local_datetime[4]<<4)|local_datetime[5];
    //Day
    raw_datetime[3]=1;
    //HH 
    raw_datetime[2]=(local_datetime[6]<<4)|local_datetime[7];
    //mm 
    raw_datetime[1]=(local_datetime[8]<<4)|local_datetime[9];
    //SS
    raw_datetime[0]=(local_datetime[10]<<4)|local_datetime[11];

    //se crea el link de la comunicacion 
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	
    //-----------------------[ Modify Control Register (0x0E) ]-----------------------------------------------
	/*Start I2C communication as MASTER*/
    i2c_master_start(cmd);

    i2c_master_write_byte(cmd, DS3231_ADDRESS | I2C_MASTER_WRITE, ACK_EN);
    /*Select control register*/
    i2c_master_write_byte(cmd, REG_SECONDS, ACK_EN);
    /*Turns on the internal oscillator to initialize the clock functions*/
    i2c_master_write(cmd, raw_datetime, sizeof(raw_datetime), ACK_EN);
     //----------------------------------------------------------------------------------------------------

    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

/*====================================================================================
FUNCTION: DS3231_GET_DATETIME

Get date and time of the RTC 
=====================================================================================*/
void ds3231_get_datetime(char * data_received){
    uint8_t raw_datetime[7]={0};

    //se crea el link de la comunicacion 
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	
    //-----------------------[ Read multiple bytes ]-----------------------------------------------
    i2c_master_start(cmd);
    //Se envia la direccion del dispositivo 
    i2c_master_write_byte(cmd, DS3231_ADDRESS | I2C_MASTER_WRITE, ACK_EN);
    //el registro a partir del cual se empezará a leer
    i2c_master_write_byte(cmd, REG_SECONDS, ACK_EN);
    
    i2c_master_start(cmd);
    //se lee a partir de la direccion anterior en el mismo dispositivo
    i2c_master_write_byte(cmd, DS3231_ADDRESS | I2C_MASTER_READ, ACK_EN);
    i2c_master_read(cmd, raw_datetime, sizeof(raw_datetime),ACK_DIS); //read 3 axis of the accelerometer
    //--------------------------------------------------------------------------------------------

    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    //YY 
    data_received[0]=raw_datetime[6]>>4;
    data_received[1]=raw_datetime[6]&0B00001111;
    //MM 
    data_received[2]=(raw_datetime[5]&0B00010000)>>4;
    data_received[3]= raw_datetime[5]&0B00001111;
    //DD 
    data_received[4]=raw_datetime[4]>>4;
    data_received[5]=raw_datetime[4]&0B00001111;
    //raw_datetime[3] deprecated, it's for indentify the day of week (1-7)
    //HH 
    data_received[6]=(raw_datetime[2]&0B00110000)>>4;
    data_received[7]= raw_datetime[2]&0B00001111;
    //mm 
    data_received[8]=raw_datetime[1]>>4;
    data_received[9]=raw_datetime[1]&0B00001111;
    //SS 
    data_received[10]=raw_datetime[0]>>4;
    data_received[11]=raw_datetime[0]&0B00001111;

    //parse to ascii format
    for(uint8_t i=0;i<DATETIME_SIZE_FORMAT;i++){
        data_received[i]+=48;
    }
}