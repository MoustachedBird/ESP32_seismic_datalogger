
/*
Maximum filename size WITHOUT FILENAME EXTENSION
In this case for datalogger: YYMMDDHHmmSS
filenames in this example don't have extension (example: .txt, .bin, etc). 

YY = year
MM = month
DD = day
HH = hours
mm = minutes 
SS = seconds

TOTAL: 12 bytes or chars without file extension

Note: to prevent reading of trash files, the data files will be into a folder named "data"
*/
#define MAX_FILENAME_SIZE 12

/*Maximum number of open files*/
#define MAX_OPEN_FILES 5


#define MOUNT_POINT "/sd"
#define FOLDER MOUNT_POINT"/data"
#define SIZE_CHAR_FOLDER sizeof(FOLDER) //length of /sd/data = 8 char


//      SD pin  | ESP32 gpio numbers

#define SD_PIN_9       12
#define SD_PIN_1       13
#define SD_PIN_2       15
// SD_PIN3 = GND
// SD_PIN4 = 3.3 V
#define SD_PIN_5       14
// SD_PIN6 = GND
#define SD_PIN_7       2
#define SD_PIN_8       4

    /* GPIOs LIST:

        =============[SLOT 0]=============
        ESP32  PULL-UP     SD-CARD or Adapter

        9      +10K        9 (DAT2)
        10     +10K        1 (CD/DAT3)  
        11     +10K        2 (CMD)
        GND                3 (VSS)
        3.3v               4 (VDD)
        6      +10K*       5 (CLK)
        GND                6 (VSS)
        7      +10K        7 (DAT0)
        8      +10K        8 (DAT1)


        =============[SLOT 1]=============
        ESP32  PULL-UP     SD-CARD or Adapter

        12     +10K        9 (DAT2)
        13     +10K        1 (CD/DAT3)  
        15     +10K        2 (CMD)
        GND                3 (VSS)
        3.3v               4 (VDD)
        14     +10K*       5 (CLK)
        GND                6 (VSS)
        2      +10K        7 (DAT0)
        4      +10K        8 (DAT1)

    */

//Card detection pin (CD pin) this is a personalized pin (it can be changed to any other)
#define SD_PIN_CD      27 

//Maximum number of messages for sd interrupt queue 
#define SD_QUEUE_MAX_MSSG 5



//Unmount SD card
void sd_unmount_card(void);

//Mount the SD card, set maximum number of open files, etc
void sd_mount_card(void);

//Look for the list of all files, returns the read files (value could be lower than max_number_files)
int sd_list_files (char (*filename_list)[MAX_FILENAME_SIZE],int max_number_files, int offset_files);

/*This funcion must be called form main file. Starts the SD main process: it indentifies when 
SD card is inserted or not inserted, if inserted then mounts the unit*/
void sd_config_card(uint16_t max_buffer_size);