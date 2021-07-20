#include "driver/spi_master.h"
#include "esp_log.h"


#include "spi_config.h"


/*set SPI bus general configurations*/
void spi_bus_set_config(void){
    esp_err_t ret;
    ESP_LOGI("SPI_BUS_CONFIG", "Initializing bus SPI%d...", SPI3_HOST+1);
    
    //configuraciones del bus SPI
    spi_bus_config_t buscfg={
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,  //-1 means not used
        .quadhd_io_num = -1,  // not used
        .max_transfer_sz = 32, //maximum 4094 minimum 0 in bytes
    };

    /*
      Initialize the SPI bus
      se selecciona el bus SPI a utilizar, 
      se caga la configuracion del bus SPI, 
      dma_chan es un tipo de comunicacion derivada del SPI, en caso de necesitarse
      este valor es igual a 0 
    */ 
    ret = spi_bus_initialize(SPI3_HOST, &buscfg, 0);
    ESP_ERROR_CHECK(ret);
    printf("SPI_BUS_CONFIG: Se inicializo correctamente el bus\n");

}
