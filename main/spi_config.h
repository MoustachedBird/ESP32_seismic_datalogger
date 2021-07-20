/*
PINES SPI

PIN      SPI2     SPI3
CS0       15        5
SCLK      14       18
MISO      12       19
MOSI      13       23
QUADWP    2        22
QUADHD    4        21

Only the first Device attached to the bus can use the CS0 pin.

Si el pin CS esta en 0 se activa el dispositivo esclavo
*/

/*
 This code tries to read ADXL355 accelerometer 
 */

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18

/*set SPI bus general configurations*/
void spi_bus_set_config(void);
