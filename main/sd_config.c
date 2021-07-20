/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

//to read directories
#include <dirent.h> 

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "task_list.h"

#include "sd_config.h"

static const char *TAG = "SD_CONFIG";
sdmmc_card_t* card; /*to mount/unmount the SD card*/

//Queue to handle gpio event (CD pin) from isr (interrupt vector)
xQueueHandle gpio_evt_queue = NULL;

//Maximum size of buffer
uint16_t max_allocation=0;


/* ==============================================================================
FUNCTION: SD MOUNT CARD

This function mounts the SD CARD and and some configurates some parameters:
maximum open files, allocation unit size, etc.
============================================================================== */
void sd_mount_card(void)
{
    esp_err_t ret;
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        //.format_if_mount_failed = true,
        .max_files = 1,
        .allocation_unit_size = max_allocation  
    };
    //more informations : https://docs.espressif.com/projects/esp-idf/en/release-v4.2/esp32/api-reference/storage/fatfs.html
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Configurating SD card options...");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
    
    ESP_LOGI(TAG, "Using SDMMC peripheral, selecting slot 1");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT(); //by default slot 1

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    ESP_LOGI(TAG, "Slot 1 default configurations");
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:
    //slot_config.width = 1;

    printf("SD_CONFIG: Mounting SD CARD... \n");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }else{
        //sd card mounted successfully
        xEventGroupSetBits(flags_hardware_available, FLAG_SD_MOUNTED); 
        //sd card is not busy    
        xEventGroupSetBits(flags_hardware_available, FLAG_SD_AVAILABLE);              
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
}



/* ==============================================================================
FUNCTION: SD UNMOUNT 

This function make a list of filenames in the FOLDER directory (FOLDER route defined
in sd_config.h)
============================================================================== */
void sd_unmount_card(void)
{
    // All done, unmount partition and disable SDMMC or SPI peripheral
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    ESP_LOGI(TAG, "Card unmounted");
}



/* ==============================================================================
INTERRUPT: SD ISR HANDLER (GPIO INTERRUPT) 

Uses the SD CARD detect pin (CD = chip detected) to indentify when SD CARD is 
inserted or removed.
============================================================================== */

void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}


/* ==============================================================================
TASK: SD CD MONITOR TASK

This task is called from the ISR GPIO interrupt service routine. Identifies when
SD CARD is removed or inserted.
============================================================================== */
void sd_cd_monitor_task(void* arg)
{
    bool last_state=1; 
    uint32_t gpio_num;

    //If SD_PIN_CD == LOW LEVEL during boot it means that SD CARD is already inserted 
    if (gpio_get_level(SD_PIN_CD)==0){
        printf("SD CARD detected\n");
        sd_mount_card();
        last_state=0;
    }
    
    while(1) {
        if(xQueueReceive(gpio_evt_queue, &gpio_num, portMAX_DELAY)) {
            if (gpio_get_level(gpio_num)!=last_state){
                last_state=gpio_get_level(gpio_num);
                if (last_state==0){
                    printf("SD CARD detected\n");
                    sd_mount_card();
                }
                else{
                    printf("SD CARD removed\n");
                    //if mounted then unmount 
                    if (xEventGroupGetBits(flags_hardware_available)&FLAG_SD_MOUNTED){
                        sd_unmount_card();
                        // set flag sd mounted to zero (false), means unmounted
                        xEventGroupClearBits(flags_hardware_available, FLAG_SD_MOUNTED);
                        // set flag sd available to zero (false), means sd not available
                        xEventGroupClearBits(flags_hardware_available, FLAG_SD_AVAILABLE);
                    }
                }
                //printf("GPIO[%d] intr, val: %d\n", gpio_num, gpio_get_level(gpio_num));
            }
        }
    }
}








/* ==============================================================================
FUNCTION: SD LIST FILES

This function make a list of filenames in the FOLDER directory (FOLDER route defined
in sd_config.h)
============================================================================== */
int sd_list_files (char (*filename_list)[MAX_FILENAME_SIZE],int max_number_files, int offset_files){
    //to save directory
    DIR *directory;
    //to read each filename
    struct dirent *file;
    //file counter
    int each_file=0;

    directory = opendir(FOLDER);
    //if the directory exist then...
    if (directory) {
        printf("SD_FUNCTION: LISTA COMPLETA DE ARCHIVOS\n");
        while (((file = readdir(directory)) != NULL) && (each_file<max_number_files+offset_files)) 
        {
            printf("SD_FUNCTION: %s\n",file->d_name);
            if (each_file>=offset_files){
                for (uint8_t each_char=0; each_char<MAX_FILENAME_SIZE; each_char++){
                    //save each file into list file matrix
                    filename_list[each_file-offset_files][each_char] = file->d_name[each_char];
                }
            }
            each_file++;
        }
        closedir(directory);
    }
    return each_file-offset_files;
}



/* ==============================================================================
FUNCTION: SD CONFIG CARD

This function must be called from main C file. It configurates the SDMMC bus, 
some pull up resistors, turns on the GPIO interrupt to identify when SD card is
inserted or not inserted. 
============================================================================== */
void sd_config_card(uint16_t max_buffer_size)
{
    max_allocation=max_buffer_size;
    // Internal pull-ups are not sufficient. However, enabling internal pull-ups
    // does make a difference some boards, so we do that here.
    ESP_LOGI(TAG, "Set GPIO PULL UP mode");    

    gpio_set_pull_mode(SD_PIN_2, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(SD_PIN_7, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode(SD_PIN_8, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
    gpio_set_pull_mode(SD_PIN_9, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
    gpio_set_pull_mode(SD_PIN_1, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes
        
    
    ESP_LOGI(TAG, "Setting SD gpio interrupt");

    //setting the interrupt
    gpio_config_t sd_interrupt_config= {
        .intr_type=GPIO_INTR_ANYEDGE,  // GPIO interrupt type : both rising and falling edge  
        .pin_bit_mask=(1ULL<<SD_PIN_CD),
        .mode=GPIO_MODE_INPUT,
        .pull_up_en=1,
    };
    printf("Adding SD gpio interrupt configurations\n");
    gpio_config(&sd_interrupt_config);

    printf("Creating SD interrupt queue\n");
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(SD_QUEUE_MAX_MSSG, sizeof(uint32_t));

    //start gpio task
    xTaskCreate(sd_cd_monitor_task, "sd_cd_monitor_task", 4*1024, NULL, 10, NULL);
    //xTaskCreatePinnedToCore(&sd_cd_monitor_task, "sd_cd_monitor_task", 10*1024, NULL, 10, NULL,0);

    //install gpio isr service
    printf("Installing ISR for SD CARD detection\n");

    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1); /*establish the level priority of the interrupt, in this case (lowest priority)
    if this parameter is equial to 0, it means default priority, it will default to allocating a non-shared interrupt of level 1, 2 or 3
    maximum priority = 7 
    
    more information:
    https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/intr_alloc.html#multiple-handlers-sharing-a-source
    
    */ 

    //hook isr handler for specific gpio pin
    
    printf("Adding pin to ISR\n");
    gpio_isr_handler_add(SD_PIN_CD, gpio_isr_handler, (void*) SD_PIN_CD);
    /*Parameters: 
        gpio_num: GPIO number
        isr_handler: ISR handler function for the corresponding GPIO number.
        args: parameter for ISR handler.
    */
   ESP_LOGI(TAG, "All done SD gpio interrupt");

}
