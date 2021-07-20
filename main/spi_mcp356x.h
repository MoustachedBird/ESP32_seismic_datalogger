#define CS_ADC_MCP356X   5


//SPI configuration for the 24 bit ADC 
void mcp356x_reg_config(void);

//Configurates the SPI communication (clock, cs pin, etc)
void mcp356x_config_spi(void);

//Read ADC value of the mcp3561
void mcp356x_read_adc(uint8_t * data_received, uint8_t size_buffer);


//=========== [ COMMANDS DESCRIPTION ] =============
/*
Address/command byte structure...

    | DEVICE ADDRESS | REGISTER ADDRESS | COMMAND TYPE |
        [7:6]             [5:2]             [1:0]

or if you use fast commands...

    | DEVICE ADDRESS |          FAST COMMANDS          |
        [7:6]                        [5:0]         
*/

//=====================================
//SPI address of MCP356X (by default is 0b01)
//=====================================
#define DEVICE_ADDRESS (0b01<<6)

/*=====================================
Fast commands (if you use one of them dont need to 
send any content after the address)
=======================================
*/
#define FAST_ADC_CONVERTION_START  (0B101000)
#define FAST_ADC_STANDBY           (0B101100)
#define FAST_ADC_SHUTDOWN          (0B110000)
#define FAST_ADC_FULL_SHUTDOWN     (0B110100)
#define FAST_ADC_FULL_RESET        (0B111000)

/*=================================
        Command type flags
===================================        
*/
/*Just read one register*/
#define FLAG_STATIC_READ       (0x01) 

/*Incremental Write Starting at Register Address*/
#define FLAG_INCREMENTAL_WRITE (0x02) 

/*Incremental Read Starting at Register Address*/
#define FLAG_INCREMENTAL_READ  (0x03) 


/*=============================== 
        Register addresses 
=================================
*/
/* 4/24/32 bits register, Latest A/D 
conversion data output value (24 or 32 bits depending on
DATA_FORMAT[1:0]) or modulator output stream (4-bit wide) in MDAT
Output mode*/
#define ADCDATA (0x0<<2) 

/* 8 bits register, ADC Operating mode, 
Master Clock mode and Input Bias Current Source mode */
#define CONFIG0 (0x1<<2) 

/* 8 bits register,  Prescale and OSR settings*/ 
#define CONFIG1 (0x2<<2) 

/* 8 bits register, ADC boost and gain settings, 
auto-zeroing settings for analog multiplexer, voltage reference and ADC*/
#define CONFIG2 (0x3<<2) 

/* 8 bits register, Conversion mode, data and CRC 
format settings; enable for CRC on communications, enable for digital 
offset and gain error calibrations*/
#define CONFIG3 (0x4<<2) 

/*8 bits register, IRQ Status bits and IRQ mode settings; 
enable for Fast commands and for conversion start pulse*/
#define IRQ (0x5<<2) 

/*8 bits register,  Analog multiplexer input selection (MUX mode only)*/
#define MUX (0x6<<2) 

/*24 bits register, SCAN mode settings*/
#define SCAN (0x7<<2) 

/*24 bits register, Delay value for TIMER between SCAN cycles*/
#define TIMER (0x8<<2) 

/*24 bits register, ADC digital offset calibration value*/ 
#define OFFSETCAL (0x9<<2) 

/*24 bits register, ADC digital gain calibration value*/
#define GAINCAL (0xA<<2) 

/*8 bits register, Password value for SPI Write mode locking*/
#define LOCK (0xD<<2) 

/*16 bits register, CRC checksum for device configuration*/
#define CRCREG (0xF<<2) 


/*======================================
CONFIG0: CONFIGURATION REGISTER 0
ADC Operating mode, Master Clock mode and Input Bias Current
Source mode
========================================
*/
/*bits 7 and 6 are writable but have no effect except that they force 
Full Shutdown mode when they are set to ‘00’ and when all other CONFIG0 
bits are set to ‘0’.*/

/*Position of CLK_SEL bits: CLock selection bits*/
#define CLK_SEL 4 
    
    /*Internal clock is enabled and present on the analog master clock output pin
    
    The device includes an internal RC-type oscillator powered by the digital 
    power supply (DVDD/DGND). The frequency of this internal oscillator ranges from 
    3.3 MHz to 6.6 MHz. 
    
    The oscillator is not trimmed in production, therefore, the precision of 
    the center frequency is approximately ±30% from chip to chip. 

    The duty cycle of the internal oscillator is centered around 50% and varies 
    very slightly from chip to chip. The internal oscillator has no Reset feature and keeps 
    running once selected.
    */
    #define CLOCK_INT_OUT_EN (0B11<<CLK_SEL) 

    /* Internal clock is selected and no clock output is present on the MCLK pin
    
    The device includes an internal RC-type oscillator powered by the digital 
    power supply (DVDD/DGND). The frequency of this internal oscillator ranges from 
    3.3 MHz to 6.6 MHz. 
    
    The oscillator is not trimmed in production, therefore, the precision of 
    the center frequency is approximately ±30% from chip to chip. 

    The duty cycle of the internal oscillator is centered around 50% and varies 
    very slightly from chip to chip. The internal oscillator has no Reset feature and keeps 
    running once selected.
    */
    #define CLOCK_INT_OUT_DIS (0B10<<CLK_SEL) 

    /*External digital clock (default) connected on the MCLK pin, 01 has the same effect*/
    #define CLOCK_EXTERNAL (0B00<<CLK_SEL) 

/*Position of CS_SEL bits: Burnout current sources
for sensor open/short detection bits*/
#define CS_SEL 2 

    /*15 µA is applied to the ADC inputs. The ADC inputs, VIN-/VIN, 
    feature a selectable burnout current source, which enables open or 
    short-circuit detection, as well as biasing very low-current external 
    sensors.*/
    #define CURNT_SRC_15uA (0B11<<CS_SEL)
    
    /*3.7 µA is applied to the ADC inputs. The ADC inputs, VIN-/VIN, 
    feature a selectable burnout current source, which enables open or 
    short-circuit detection, as well as biasing very low-current external 
    sensors.*/
    #define CURNT_SRC_3_7uA (0B10<<CS_SEL)
    
    /*0.9 µA is applied to the ADC inputs. The ADC inputs, VIN-/VIN, 
    feature a selectable burnout current source, which enables open or 
    short-circuit detection, as well as biasing very low-current external 
    sensors.*/
    #define CURNT_SRC_0_9uA (0B01<<CS_SEL)
    
    /*No current source is applied to the ADC inputs (default). The ADC inputs, 
    VIN-/VIN, feature a selectable burnout current source, which enables open or 
    short-circuit detection, as well as biasing very low-current external 
    sensors.*/
    #define CURNT_SRC_DIS (0B00<<CS_SEL) 

/*Position of ADC_MODE bits: ADC operation mode bits*/
#define ADC_MODE 0 

    /*The ADC is placed into Conversion 
    mode and consumes the specified current (2.2 mA maximum deppending of BOOST). 
    A/D conversions can be reset and restarted immediately once this
    mode is effectively reached. This mode may be reached after a maximum 
    of TADC_SETUP time, depending of the current state of the ADC.*/
    #define MODE_CONVERTION (0B11<<ADC_MODE) 

    /* Conversions are stopped. ADC is 
    placed into Reset but consumes almost as much current as in Conversion mode. 
    A/D conversions can start immediately once this mode is effectively reached. 
    This mode may be reached aftera maximum of TADC_SETUP time, depending 
    of the current state of the ADC. */
    #define MODE_STANDBY (0B10<<ADC_MODE) 

    /*ADC Shutdown mode (default). 
    Conversions are stopped. ADC is placed into ADC Shutdown mode and does not 
    consume  any current. A/D conversions can only start after TADC_SETUP 
    start-up time. This mode is effective immediately after being programmed. 
    01 has the same effect*/
    #define MODE_SHUTDOWN (0B00<<ADC_MODE)  

/*======================================
CONFIG1: CONFIGURATION REGISTER 1
Prescale and OSR settings
========================================*/

/*Position of PRE bits: Prescaler Value Selection for AMCLK*/
#define PRE 6
    /*AMCLK = MCLK/8*/
    #define PRESCALER_8_MCLK (0B11<<PRE)

    /*AMCLK = MCLK/4*/
    #define PRESCALER_4_MCLK (0B10<<PRE)

    /*AMCLK = MCLK/2*/
    #define PRESCALER_2_MCLK (0B01<<PRE)

    /*AMCLK = MCLK (default), no prescaler or prescaler = 1 */
    #define PRESCALER_NO_MCLK (0B00<<PRE)

/*Position of OSR bits: Oversampling Ratio for Delta-Sigma A/D Conversion

With a higher OSR setting, the noise is also reduced as the oversampling 
diminishes both thermal noise and quantization noise induced by the 
Delta-Sigma modulator loop.
*/
#define OSR 2

    /*OSR: 98304*/
    #define OSR_98304 (0B1111<<OSR)

    /*OSR: 81920*/
    #define OSR_81920 (0B1110<<OSR)
    
    /*OSR: 49152*/
    #define OSR_49152 (0B1101<<OSR)

    /*OSR: 40960*/
    #define OSR_40960 (0B1100<<OSR)

    /*OSR: 24576*/
    #define OSR_24576 (0B1011<<OSR)

    /*OSR: 20480*/
    #define OSR_20480 (0B1010<<OSR)

    /*OSR: 16384*/
    #define OSR_16384 (0B1001<<OSR)

    /*OSR: 8192*/
    #define OSR_8192 (0B1000<<OSR)

    /*OSR: 4096*/
    #define OSR_4096 (0B0111<<OSR)

    /*OSR: 2048*/
    #define OSR_2048 (0B0110<<OSR)

    /*OSR: 1024*/
    #define OSR_1024 (0B0101<<OSR)

    /*OSR: 512*/
    #define OSR_512 (0B0100<<OSR)

    /*OSR: 256 (default)*/
    #define OSR_256 (0B0011<<OSR)

    /*OSR: 128*/
    #define OSR_128 (0B0010<<OSR)

    /*OSR: 64*/
    #define OSR_64 (0B0001<<OSR)

    /*OSR: 32*/
    #define OSR_32 (0B0000<<OSR)

/*======================================
CONFIG2: CONFIGURATION REGISTER 2

ADC boost and gain settings, auto-zeroing settings for analog
multiplexer, voltage reference and ADC
========================================*/

/*Position of BOOST bits: ADC Bias Current Selection

The Delta-Sigma modulator includes a programmable
biasing circuit in order to further adjust the power
consumption to the sampling speed applied through
the MCLK. This can be programmed through the
BOOST[1:0] bits in the CONFIG2 register. The
different BOOST settings are applied to the entire
modulator circuit, including the voltage reference
buffers. 

The higher GAIN settings require higher BOOST settings
to maintain high bandwidth, as the input sampling
capacitors have a larger value.
*/
#define BOOST 6

    /*ADC channel has current x 2*/
    #define ADC_CURRENT_X2 (0B11<<BOOST)

    /*ADC channel has current x 1 (default)*/
    #define ADC_CURRENT_X1 (0B10<<BOOST)

    /* ADC channel has current x 0.66*/
    #define ADC_CURRENT_X0_66 (0B01<<BOOST)
    
    /*ADC channel has current x 0.5*/
    #define ADC_CURRENT_X0_5 (0B00<<BOOST)

/*ADC Gain Selection

The gain of the converter is programmable and controlled by the GAIN[2:0] 
bits in the CONFIG2 register.
The ADC programmable gain is divided in two gain
stages: one in the analog domain, one in the digital
domain

Page 35 datasheet.
*/
#define GAIN 3
    
    /*Gain is x64 (x16 analog, x4 digital)*/
    #define GAIN_X64 (0B111<<GAIN)
    
    /*Gain is x32 (x16 analog, x2 digital)*/
    #define GAIN_X32 (0B110<<GAIN)

    /*Gain is x16*/
    #define GAIN_X16 (0B101<<GAIN)

    /*Gain is x8*/
    #define GAIN_X8 (0B100<<GAIN)
    
    /*Gain is x4*/
    #define GAIN_X4 (0B011<<GAIN)

    /*Gain is x2*/
    #define GAIN_X2 (0B010<<GAIN)

    /*Gain is x1 (default)*/
    #define GAIN_X1 (0B001<<GAIN)

    /*Gain is x1/3 = x0.33*/
    #define GAIN_X0_33 (0B000<<GAIN)

/*AZ_MUX: Auto-Zeroing MUX Setting*/

#define AZ_MUX 0

    /*ADC auto-zeroing algorithm is enabled. This setting multiplies the 
    conversion time by two and does not allow Continuous Conversion mode 
    operation (which is then replaced by a series of consecutive One-Shot 
    mode conversions).
    
    The last two bits (B0 and B1) are always equal to 11, they're reserved. 
    */
    #define MUX_EN_AUTO_ZEROING (0B111<<AZ_MUX)

    /*Analog input multiplexer auto-zeroing algorithm is disabled (default)
    
    The last two bits (B0 and B1) are always equal to 11, they're reserved.
    */
    #define MUX_DIS_AUTO_ZEROING (0B011<<AZ_MUX)
    

/*======================================
CONFIG3: CONFIGURATION REGISTER 3

Conversion mode, data and CRC format settings; enable for CRC on
communications, enable for digital offset and gain error calibrations
========================================*/

/*(CRC)*/

/*Conversion Mode Selection*/
#define CONV_MODE 6
    /*Continuous Conversion mode or continuous conversion cycle in SCAN mode*/
    #define CONV_MODE_CONTINUOUS (0B11<<CONV_MODE)

    /*One-shot conversion or one-shot cycle in SCAN mode. It sets ADC_MODE[1:0] 
    to ‘10’ (standby) at the end of the conversion or at the end of the 
    conversion cycle in SCAN mode*/
    #define CONV_MODE_1_SHOT_STBY (0B10<<CONV_MODE)

    /*One-shot conversion or one-shot cycle in SCAN mode. It sets ADC_MODE[1:0] 
    to ‘0x’ (ADC Shutdown) at the end of the conversion or at the end of the 
    conversion cycle in SCAN mode (default).*/
    #define CONV_MODE_1_SHOT_SHTDWN (0B00<<CONV_MODE)

/*ADC Output Data Format Selection*/
#define DATA_FORMAT 4

    /*32-bit (25-bit right justified data + Channel ID): CHID[3:0] + 
    SGN extension (4 bits) + 24-bit ADC
    data. It allows overrange with the SGN extension*/
    #define FORMAT_32_RIGHT_PLUS_ID (0B11<<DATA_FORMAT)

    /*32-bit (25-bit right justified data): SGN extension (8-bit) + 
    24-bit ADC data. It allows overrange with the SGN extension.*/
    #define FORMAT_32_RIGHT (0B10<<DATA_FORMAT)

    /* 32-bit (24-bit left justified data): 24-bit ADC data + 0x00 (8-bit). 
    It does not allow overrange (ADC code locked to 0xFFFFFF or 0x800000).*/
    #define FORMAT_32_LEFT (0B01<<DATA_FORMAT)

    /*24-bit (default ADC coding): 24-bit ADC data. It does not allow 
    overrange (ADC code locked to 0xFFFFFF or 0x800000).*/
    #define FORMAT_24_BIT (0B00<<DATA_FORMAT)

/*CRC_FORMAT: CRC Checksum Format Selection on Read Communications
CRC= Cyclic Redundancy Check, basically SPI communication secured by a password 

(it does not affect CRCCFG coding)
*/
#define CRC_FORMAT 3

    /*32-bit wide (CRC-16 followed by 16 zeros)*/
    #define CRC_32_BIT_WIDE (0B1<<CRC_FORMAT)

    /*16-bit wide (CRC-16 only) (default)*/
    #define CRC_16_BIT_WIDE (0B0<<CRC_FORMAT)


/*CRC Checksum Selection on Read Communications
(it does not affect CRCCFG calculations)*/
#define EN_CRCCOM 2
    /*CRC on communications enabled*/
    #define CRC_ENABLED (0B1<<EN_CRCCOM)

    /*CRC on communications disabled (default)*/
    #define CRC_DISABLED (0B0<<EN_CRCCOM)

/*EN_OFFCAL: Enable Digital Offset Calibration*/
#define EN_OFFCAL 1
    /*Enable Digital Offset Calibration */
    #define OFFCAL_ENABLED (0B1<<EN_OFFCAL)

    /*Disable Digital Offset Calibration (default) */
    #define OFFCAL_DISABLED (0B0<<EN_OFFCAL)

/*EN_OFFCAL: Enable Digital Gain Calibration*/
#define EN_GAINCAL 0
    /*Enable Digital Gain Calibration */
    #define GAINCAL_ENABLED (0B1<<EN_GAINCAL)

    /*Disable Digital Gain Calibration (default) */
    #define GAINCAL_DISABLED (0B0<<EN_GAINCAL)


/*======================================
SCAN: SCAN REGISTER 

List of predefined differential inputs (also referred to as 
input channels) in a defined order for 
sequentially and automatically convertion 
========================================*/
#define SCAN_DLY_512_DMCLK (111<<5)

#define SCAN_DLY_256_DMCLK (110<<5)

#define SCAN_DLY_128_DMCLK (101<<5)

#define SCAN_DLY_64_DMCLK (100<<5)

#define SCAN_DLY_32_DMCLK (011<<5)

#define SCAN_DLY_16_DMCLK (010<<5)

#define SCAN_DLY_8_DMCLK (001<<5)

#define SCAN_DLY_0_DMCLK (000<<5)


/*ADC enters SCAN mode as soon as one of the
SCAN[15:0] bits in the SCAN register is set to 1*/

#define SCAN_OFFSET  (0B1<<7)

#define SCAN_VCM  (0B1<<6)

#define SCAN_AVDD  (0B1<<5)

#define SCAN_TEMP  (0B1<<4)

#define SCAN_DIF_CH6_CH7  (0B1<<3)

#define SCAN_DIF_CH4_CH5  (0B1<<2)

#define SCAN_DIF_CH2_CH3  (0B1<<1)

#define SCAN_DIF_CH0_CH1  (0B1<<0)

#define SCAN_SINGLE_CH7  (0B1<<7)

#define SCAN_SINGLE_CH6  (0B1<<6)

#define SCAN_SINGLE_CH5  (0B1<<5)

#define SCAN_SINGLE_CH4  (0B1<<4)

#define SCAN_SINGLE_CH3  (0B1<<3)

#define SCAN_SINGLE_CH2  (0B1<<2)

#define SCAN_SINGLE_CH1  (0B1<<1)

#define SCAN_SINGLE_CH0  (0B1<<0)


/*======================================
IRQ: INTERRUPT REQUEST REGISTER

In case of a CRCCFG error, the CRCCFG_STATUS bit 
in the IRQ register is set to ‘0’. Once the IRQ 
register is fully read, the CRCCFG_STATUS bit is reset to ‘1’.  
========================================*/

/*Bit 7 have to be 0, Data Ready Status Flag*/
#define DR_STATUS 6

    /* ADCDATA has not been updated since last reading or last Reset (default)*/
    #define DATA_NOT_UPDATED (0B1<<DR_STATUS)

    /*New ADCDATA ready for reading*/
    #define DATA_NEW_AVAILABLE (0B0<<DR_STATUS) 

/* CRC Error Status Flag Bit for Internal Registers*/
#define CRCCFG_STATUS 5

    /*CRC error has not occurred for the Configuration registers (default)*/
    #define CRC_ERROR_NOT_OCURRED (0B1<<CRCCFG_STATUS)
    
    /*CRC error has occurred for the Configuration registers*/
    #define CRC_ERROR_OCURRED (0B0<<CRCCFG_STATUS)

/* POR Status Flag*/
#define POR_STATUS 4

    /*POR has not occurred since the last reading (default)*/
    #define POR_NOT_OCURRED (1<<POR_STATUS)

    /*POR has occurred since the last reading*/
    #define POR_OCURRED (0<<POR_STATUS)
    
/* Configuration for the IRQ/MDAT Pin*/
#define IRQ_MODE_1 3

    /* MDAT output is selected. Only POR and CRC interrupts can be present on this 
    pin and take priority over the MDAT output. When IRQ_MODE[1:0] = 10 or 11, 
    the modulator output codes (MDAT stream) are available at both the
    MDAT pin and ADCDATA register (0x0).*/
    #define IRQ_MODE_MDAT_SELECTED (0B1<<IRQ_MODE_1)

    /*IRQ output is selected. All interrupts can appear on the IRQ/MDAT pin. 
    When IRQ_MODE[1:0] = 10 or 11, the modulator output codes (MDAT stream) are 
    available at both the MDAT pin and ADCDATA register (0x0).*/
    #define IRQ_MODE_IRQ_SELECTED (0B0<<IRQ_MODE_1)

/* Configuration for the IRQ/MDAT Pin*/
#define IRQ_MODE_0 2

    /*The Inactive state is logic high (does not require a pull-up resistor to DVDD)*/
    #define IRQ_MODE_NO_EXT_PULL_UP (0B1<<IRQ_MODE_0)

    /*The Inactive state is high-Z (requires a pull-up resistor to DVDD) (default)*/
    #define IRQ_MODE_EXT_PULL_UP (0B0<<IRQ_MODE_0)

/*Enable Fast Commands in the COMMAND Byte*/
#define EN_FASTCMD 1

    /*Fast commands are enabled (default)*/
    #define FAST_COMMANDS_ENABLE (0B1<<EN_FASTCMD)

    /*Fast commands are disabled*/
    #define FAST_COMMANDS_DISABLE (0B0<<EN_FASTCMD)

/*Enable Conversion Start Interrupt Output*/
#define EN_STP 0

    /*Enable conversion start interrupt output*/
    #define STP_ENABLE (0B1<<EN_STP)

    /*Disable conversion start interrupt output*/
    #define STP_DISABLE (0B0<<EN_STP)

