#include "driver/i2c.h" //Headers for I2C
#include "i2c_config.h"

/*Configurates i2c clock and i2c PINS*/
void i2c_bus_set_config(void){
    

    //I2C configuration
	printf("Drive configuration \n");
	i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = PIN_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000
    };


    i2c_param_config(I2C_NUM_0, &conf);
	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);

    printf("I2C_CONFIG: i2c driver installed  \n");	
}

