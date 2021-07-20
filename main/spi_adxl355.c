#include <string.h>

#include "driver/spi_master.h"
#include "esp_log.h"

#include "spi_adxl355.h"


//manejador de la comunicacion SPI
spi_device_handle_t adxl355_handler;


/*Configurates SPI clock and SPI PINS*/
void adxl355_config_spi(void){
    
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
    
    */

    //configuraciones del dispositivo 
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=1*1000*1000,           //Clock out at 10 MHz, overclock = 26 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=CS_ADXL355,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
        .address_bits=8, //El largo de la direccion de memoria del dispositivo
        //.pre_cb=lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
    };
    
    //Se añade el dispositivo
    ret=spi_bus_add_device(SPI3_HOST, &devcfg, &adxl355_handler);
    ESP_ERROR_CHECK(ret);

    printf("ADXL355: Se agregó el dispositivo correctamente\n");
}

/*Configurates the accelerometer's sample rate*/
void adxl355_100hz_rate(void){
    //error variable
    esp_err_t ret;
    
    //se declara la variable de transaccion
    spi_transaction_t transaccion;
    
    //Se declaran los datos a enviar (8 bits)
    uint8_t datos_enviar=0;

    // =====================================================================
    // =========== [Para ENVÍO de informacion por el bus SPI] ==============
    // =====================================================================
    
    //-=-=-=-=-=-=-=-=-=[ Cambia la frecuencia de muestreo ]-=-=-=-=-=-=-=-=-=-=-=-=
    
    /*La frecuencia de muestreo incluye un filtro pasa bajas
    La relacion de la frecuencia de corte del filtro es 

    frecuencia_corte = frecuencia_muestreo/4
    
    Registro: 0x28

    Dato      Frecuencia muestreo  Filtro pasa bajas (Hz)  
    0x00            4000                 1000
    0x01            2000                 500
    0x02            1000                 250
    0x03            500                  125
    0x04            250                  62.5
    0x05            125                  31.25
    0x06            62.5                 15.625 
    0x07            31.25                7.813
    0x08            15.625               3.906
    0x09            7.813                1.953
    0x0A            3.906                0.977
    */

    memset(&transaccion, 0, sizeof(transaccion));       //Se limpia con 0 la variable para evitar errores
    datos_enviar=0x05; //Frecuencia de muestreo de 100 Hz

    //se escribe el dato 1 en la direccion 0x28, el registro de filtros
    transaccion.length=8;      //total de datos 8 bits.
    transaccion.tx_buffer=&datos_enviar;   //Puntero de datos a enviar
    transaccion.addr=(0x28<<1)|WRITE_FLAG_ADXL355; //Registro para activar el modo medicion

    ret=spi_device_transmit(adxl355_handler, &transaccion);  //Transmite los datos
    assert(ret==ESP_OK);            //Should have had no issues.
    printf("ADXL355: Frecuencia de muestreo 100 Hz \n");

}


/*Configurates and turns on the accelerometer (+-2G range)*/
void adxl355_range_conf(void){
    //error variable
    esp_err_t ret;
    
    //se declara la variable de transaccion
    spi_transaction_t transaccion;
    
    //Se declaran los datos a enviar (8 bits)
    uint8_t datos_enviar[2];

    //-=-=-=-=-=-=-=-=-=[ Cambia el rango de medicion ]-=-=-=-=-=-=-=-=-=-=-=-=
    datos_enviar[0]= RANGE_2G; //segun la hoja de datos poner un 01 en 
    //los bits b0 y b1 establece el ranglo de +-2g en el registro 0x2C

    //-=-=-=-=-=-=-=-=-=[ Cambia a modo medicion ]-=-=-=-=-=-=-=-=-=-=-=-=
    datos_enviar[1]=0x06; //solo acelerometros, se desabilita el sensor de temperatura
    //se cambia a modo medicion, se quita modo standby


    // =====================================================================
    // =========== [Para ENVÍO de informacion por el bus SPI] ==============
    // =====================================================================
    //se resetea el manejador de transaccion
    memset(&transaccion, 0, sizeof(transaccion));       //Se limpia con 0 la variable para evitar errores

    transaccion.length=8*sizeof(datos_enviar);      //Largo de los datos en bits.
    transaccion.tx_buffer=&datos_enviar;   //Puntero de datos a enviar
    
    transaccion.addr=(0x2C<<1)|WRITE_FLAG_ADXL355; /*Direccion (address) a la  
    cual enviar el dato, el largo de la address se establece en 
    spi_device_interface_config_t con el parametro address_bits
    */
    //se transmiten los datos
    ret=spi_device_transmit(adxl355_handler, &transaccion); 
    assert(ret==ESP_OK);            //Se verifica si no hubo error        

    printf("ADXL355: Rango +- 2G y sensor en modo medicion \n");
}

/*Read acceleration values, parameters: pointer of buffer where data will be saved*/
void adxl355_read_accl(uint8_t * data_received, uint8_t size_buffer){
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
    transaccion.addr=XDATA3|READ_FLAG_ADXL355 ; //Leer el registro del eje X byte más significativo (XDATA3)

    ret=spi_device_transmit(adxl355_handler, &transaccion);  //Transmitir
    assert(ret==ESP_OK);     //Should have had no issues.
}
