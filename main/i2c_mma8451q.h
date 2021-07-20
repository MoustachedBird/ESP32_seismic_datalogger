


/*Configurates the accelerometer's sample rate, range, etc*/
void mma8451q_reg_config(void);

/*Read acceleration values of mma8451q*/
void mma8451q_read_accl(uint8_t * data_received, uint8_t size_buffer);


#define MMA8452Q_ADDRESS 0x1C // MMA8452Q I2C address is 0x1C(28)
#define MMA8452Q_ACCEL_XOUT_H 0x01  //Address of the first X acceleration data register
#define MMA8452Q_ACCEL_YOUT_H 0x03  //Address of the first Y acceleration data register
#define MMA8452Q_ACCEL_ZOUT_H 0x05  //Address of the first Z acceleration data register

#define XYZ_DATA_CFG 0x0E //Register for configurating sensivity range
#define CTRL_REG1 0x2A //Control register for selecting the operation mode
