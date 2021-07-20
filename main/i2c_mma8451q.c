#include "driver/i2c.h" //Headers for I2C
#include "i2c_mma8451q.h"



/*Configurates the accelerometer's sample rate, range, etc*/
void mma8451q_reg_config(void){
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	//vTaskDelay(200/portTICK_PERIOD_MS);
	
	//-----------------------[ Select Active Mode (Accelerometer configuration) ]-----------------------------------------------
	printf("MMA8451Q: i2c cmd link created\n");	
    /*Start I2C communication as MASTER*/
    i2c_master_start(cmd);
    /*Select the I2C device, in this case we write mma8452Q adress to select it
    i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | I2C_MASTER_WRITE, ACK_EN);
    ACK= acknowledge bit.*/
    i2c_master_write_byte(cmd, (MMA8452Q_ADDRESS << 1) | I2C_MASTER_WRITE, 1);
    /*Select control register*/
    i2c_master_write_byte(cmd, CTRL_REG1, true);
    /* Select active mode (sensor sends data continuously) and ODR=100HZ (sample rate=100 Hz and bandwithd=50 Hz)*/
    i2c_master_write_byte(cmd, 0B00011001, true);
    
	//-----------------------[  Set range to +/- 2g (Accelerometer configuration)]--------------------------------------------
	printf("MMA8451Q: i2c sensor initialized\n");	
	/* Start I2C communication as MASTER*/
    i2c_master_start(cmd);
    /* Select the I2C device, in this case we write mma8452Q adress to select it*/
    i2c_master_write_byte(cmd, (MMA8452Q_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    /* XYZ_DATA_CFG register*/
    i2c_master_write_byte(cmd, XYZ_DATA_CFG, true);
    /* Set range to +/- 2g*/
    i2c_master_write_byte(cmd, 0B00000000, true);
    //--------------------------------------------------------------------------------------------
 
	/* Stop I2C communication*/
    i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

    printf("MMA8451Q: i2c configuration finish\n");
    
}


/*Read acceleration values of mma8451q, parameters: pointer of buffer where data will be saved*/
void mma8451q_read_accl(uint8_t * data_received, uint8_t size_buffer){
    
    //se crea el link de la comunicacion 
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	
    i2c_master_start(cmd);
    //Se envia la direccion del dispositivo 
    i2c_master_write_byte(cmd, (MMA8452Q_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    //el registro a partir del cual se empezará a leer
    i2c_master_write_byte(cmd, MMA8452Q_ACCEL_XOUT_H, true);
    
    i2c_master_start(cmd);
    //se lee a partir de la direccion anterior en el mismo dispositivo
    i2c_master_write_byte(cmd, (MMA8452Q_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data_received, size_buffer, false); //read 3 axis of the accelerometer

    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    //printf("MMA8451Q: DATA[0]:%d  DATA[1]: %d\n",data_received[0],data_received[1]);
}