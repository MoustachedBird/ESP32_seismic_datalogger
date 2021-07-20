#include <string.h>

#include "driver/spi_master.h"
#include "esp_log.h"

#include "spi_mcp356x.h"


//manejador de la comunicacion SPI
spi_device_handle_t mcp356x_handler;


void mcp356x_config_spi(void){
    esp_err_t ret;
    
    /*
    mode0 means CPOL=0, CPHA=0.
    When SPI is idle, the clock output is logic low; 
    data changes on the falling edge of the SPI clock 
    and is sampled on the rising edge;

    mode1 means CPOL=0, CPHA=1.
    When SPI is idle, the clock output is logic low; 
    data changes on the rising edge of the SPI clock 
    and is sampled on the falling edge;

    mode2 means when CPOL=1, CPHA=0.
    When SPI is idle, the clock output is logic high; 
    data changes on the rising edge of the SPI clock and 
    is sampled on the falling edge;

    mode3 means when CPOL=1, CPHA=1.
    When SPI is idle, the clock output is logic high; 
    data changes on the falling edge of the SPI clock and is 
    sampled on the rising edge.

    SPI mode, representing a pair of (CPOL, CPHA) configuration:

    extracted from: 
    https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.htm
    */

    //configuraciones del dispositivo 
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=1*1000*1000,  //Clock out at 10 MHz, overclock = 26 MHz
        .mode=0,    //SPI mode 0
        .spics_io_num=CS_ADC_MCP356X, //CS pin
        .queue_size=7,   //We want to be able to queue 7 transactions at a time
        .address_bits=8, //El largo de la direccion de memoria del dispositivo
        //.pre_cb=lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
    };
    
    //Se añade el dispositivo
    ret=spi_bus_add_device(SPI3_HOST, &devcfg, &mcp356x_handler);
    ESP_ERROR_CHECK(ret);

}

/*Configurates the ADC MCP3561 measure mode*/
void mcp356x_reg_config(void){
    //error variable
    esp_err_t ret;
    
    //se declara la variable de transaccion
    spi_transaction_t transaccion;
    
    /*
    Registers to modify from 0x1 to 0x7. Total bytes = 9 (0x7 register length is 3 bytes)
    */
    uint8_t data_to_send[9];

    //-=-=-=-=-=-=-=-=-=[ Configuracion ADC (CONFIG0=0x1)]-=-=-=-=-=-=-=-=-=-=-=-=
    printf("CONFIG0: Reloj, Corriente y modo ADC\n");
    data_to_send[0]=(0B11<<6)|CLOCK_INT_OUT_EN|CURNT_SRC_DIS|MODE_CONVERTION; 
    
    //-=-=-=-=-=-=-=-=-=[ Cambia la frecuencia de muestreo (CONFIG1=0x2)]-=-=-=-=-=-=-=-=-=-=-=-=
    
    /*La frecuencia de muestreo se ve afectada por el reloj DRCLK,

        AMCLK=MCLK/PRESCALER

        DRCLK = DMCLK/OSR = AMCLK/(4*OSR) = MCLK/(4*OSR*PRESCALER)

        Example if you select internal clock (value equal to: 3.3 MHz to 6.6 MHz)
        NO_PRESCALER means PRESCALER = 1

        Internal clock value measured: 4.56 MHz

        Prescaler = 8     OSR = 1024
        DRCLK = (4.56 MHZ)/(4*1024*8) = 139.16 HZ sample rate
    
        Prescaler = 1     OSR = 1024
        DRCLK = (4.56 MHZ)/(4*1024*1) = 1.11 KHz sample rate
    
    */
    printf("CONFIG1: Frecuencia de muestreo\n");
    data_to_send[1]=PRESCALER_8_MCLK|OSR_1024; //Frecuencia de muestreo de 100 Hz aproximadamente

    //-=-=-=-=-=-=-=-=-=[ Cambia la ganancia del ADC (CONFIG2=0x3)]-=-=-=-=-=-=-=-=-=-=-=-=    
    printf("CONFIG2: Ganancia ADC\n");
    data_to_send[2]=ADC_CURRENT_X2|GAIN_X2|MUX_DIS_AUTO_ZEROING;

    //-=-=-=-=-=-=-=-=-=[ Cambia al modo conversion continuo (CONFIG3=0x4)]-=-=-=-=-=-=-=-=-=-=-=-=    
    printf("CONFIG3: Modo conversion continuo\n");
    data_to_send[3]=CONV_MODE_CONTINUOUS|FORMAT_24_BIT|CRC_DISABLED|OFFCAL_DISABLED|GAINCAL_DISABLED;

    //-=-=-=-=-=-=-=-=-=[ Modificacion de las interrupciones (IRQ REGISTER0x5)]-=-=-=-=-=-=-=-=-=-=-=-=    
    printf("IRQ Register: Modificacion de las interrupciones\n");
    data_to_send[4]=DATA_NOT_UPDATED|CRC_ERROR_NOT_OCURRED|POR_NOT_OCURRED|IRQ_MODE_IRQ_SELECTED|IRQ_MODE_NO_EXT_PULL_UP|FAST_COMMANDS_ENABLE|STP_ENABLE;

    //-=-=-=-=-=-=-=-=-=[ Modificacion del multiplexor, default (MUX=0x6)]-=-=-=-=-=-=-=-=-=-=-=-=    
    printf("MUX Register: Default Value 0000 0001\n");
    data_to_send[5]=0B00000001;

    //-=-=-=-=-=-=-=-=-=[ Configuracion del modo SCAN o continuo (SCAN) 24 bits]-=-=-=-=-=-=-=-=-=-=-=-=    
    printf("SCAN: Modo continuo, diferente entre canal 1 y 0\n");
    data_to_send[6]=SCAN_DLY_0_DMCLK; //MSB
    
    data_to_send[7]=SCAN_DIF_CH0_CH1; //middle 
    
    data_to_send[8]=0; //for single channel scan mode (LSB)
    

    // =====================================================================
    // =========== [Para ENVÍO de informacion por el bus SPI] ==============
    // =====================================================================
    
    //se resetea el manejador de transaccion, se llena con 0
    memset(&transaccion, 0, sizeof(transaccion)); 

    transaccion.length=8*sizeof(data_to_send); //Largo de los datos en bits.
    transaccion.tx_buffer=&data_to_send;   //Puntero de datos a enviar
    
    /*Direccion (address) a partir de la cual se enviaran los datos*/
    transaccion.addr=DEVICE_ADDRESS|CONFIG0|FLAG_INCREMENTAL_WRITE; 

    //se transmiten los datos
    ret=spi_device_transmit(mcp356x_handler, &transaccion); 
    assert(ret==ESP_OK); //Se verifica si no hubo errores

    printf("Configuracion terminada\n");
}


void mcp356x_read_adc(uint8_t * data_received, uint8_t size_buffer){
    esp_err_t ret;

    //se declara la variable de transaccion
    spi_transaction_t transaccion;
 
    // =====================================================================
    // =========== [Para RECIBIR de informacion por el bus SPI] ==============
    // =====================================================================
        
    //se resetea el manejador de transaccion
    memset(&transaccion, 0, sizeof(transaccion));
    //Se limpia con 0 la variable para evitar errores

    transaccion.length=8*size_buffer;      //tamaño de transaccion en bits
    transaccion.rx_buffer= data_received;  //Puntero del buffer para recibir los datos
    transaccion.addr=DEVICE_ADDRESS|ADCDATA|FLAG_INCREMENTAL_READ ; //Leer el registro del eje Z byte más significativo

    ret=spi_device_transmit(mcp356x_handler, &transaccion);  //Transmitir
    assert(ret==ESP_OK);            //Should have had no issues.
}