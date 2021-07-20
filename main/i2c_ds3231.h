
/*
The time and calendar data are set or initialized by writing the appropriate 
register bytes. The contents of the time and calendar registers are in the
binary-coded decimal (BCD) format. 

The DS3231 can be run in either 12-hour or 24-hour mode. 
Bit 6 of the hours register is defined as the 12- or 24-hour mode select bit.

When high (Bit 6), the 12-hour mode is selected. In the 12-hour mode, bit 5 is the 
AM/PM bit with logic-high being PM. In the 24-hour mode, bit 5 is the second 
10-hour bit (20–23 hours). The century bit (bit 7 of the month register) is 
toggled when the years register overflows from 99 to 00.
*/


/*Configurates the RTC, turns on the clock*/
void ds3231_reg_config(void);

/*Configurates date and time values of ds3231 rtc*/
void ds3231_set_datetime(char * data_received);

/*Read date and time values of ds3231 rtc*/
void ds3231_get_datetime(char * data_received);

#define DATETIME_SIZE_FORMAT 12 //12 bytes maximum format YYMMDDHHmmSS


//device address (i2c address)
#define DS3231_ADDRESS (0B1101000 << 1) // DS3231 I2C address is 0x1C(28)

#define ACK_EN 1
#define ACK_DIS 0


/*===============================================
=================[ REGISTER MAP ]================
=================================================*/
/*Identifies the seconds*/
#define REG_SECONDS 0x00

    #define SECONDS_10 4

    #define SECONDS_1  0

/*Identifies the minutes*/
#define REG_MINUTES 0x01

    #define MINUTES_10 4

    #define MINUTES_1  0

/*Identifies the hour*/
#define REG_HOURS 0x02

    #define BIT_12_24 6

        #define FORMAT_12_HRS (1<<BIT_12_24)
        #define FORMAT_24_HRS (0<<BIT_12_24)

    #define HOURS_10 4

    #define HOURS_1  0

/*Identifies the day of the week (1-7)*/
#define REG_DAY 0x03

    #define DAY_1 0

/*Identifies the day of the month (0-31, depending of the month)*/
#define REG_DATE 0x04

    #define DATE_10 4

    #define DATE_1 0

/*Identifies the  month and century flag goes to 1 when years overflow 99 years*/
#define REG_MONTH 0x05

    #define CENTURY_FLAG 7

    #define MONTH_10 4

    #define MONTH_1 0

/*Identifies year (0-99)*/
#define REG_YEAR 0x06
    
    #define YEAR_10 4

    #define YEAR_1 0

/*Control register*/
#define REG_CONTROL 0x0E

    #define EN_DIS_OSC_BIT 7
        #define OSCILLATOR_EN  (0<<EN_DIS_OSC_BIT)
        #define OSCILLATOR_DIS (1<<EN_DIS_OSC_BIT)

    #define BATTERY_BIT 6
        #define SUPPLY_BY_BATTERY  (1<<BATTERY_BIT)
        #define SUPPLY_BY_VCC      (0<<BATTERY_BIT)

    #define TEMPERATURE_CONV_BIT 5
        #define TEMP_TCXO  (1<<TEMPERATURE_CONV_BIT)
        #define TEMP_ZERO  (0<<TEMPERATURE_CONV_BIT)

    #define RATE_SELECT_BITS 3
        #define RATE_1_HZ      (0B00<<RATE_SELECT_BITS)
        #define RATE_1_024_HZ  (0B01<<RATE_SELECT_BITS)
        #define RATE_4_096_HZ  (0B10<<RATE_SELECT_BITS)
        #define RATE_8_192_HZ  (0B11<<RATE_SELECT_BITS)
    
    #define INTERRUPT_BIT 2
        /*INT_SQW PIN as an output. The squared signal defined in
        RATE_SELECT_BITS shown in this pin*/
        #define INT_SQW_AS_SQUARE (0<<INTERRUPT_BIT)
        /*INT_SQW PIN as an output. The pin shows the interrupt status*/
        #define INT_SQW_AS_INT    (1<<INTERRUPT_BIT)
    
    #define ALARM2_BIT 1
        #define ALARM2_DIS (0<<ALARM2_BIT)
        #define ALARM2_EN  (1<<ALARM2_BIT)

    #define ALARM1_BIT 0
        #define ALARM1_DIS (0<<ALARM1_BIT)
        #define ALARM1_EN  (1<<ALARM1_BIT)
    


/*Control status register*/
#define REG_CONTROL_STATUS 0x0F


