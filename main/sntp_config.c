#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"

#include "esp_sntp.h"
#include "task_list.h"

//custom headers
#include "sntp_config.h"
#include "i2c_ds3231.h" //real time clock header

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
//RTC_DATA_ATTR static int boot_count = 0;


static const char *TAG = "sntp";


void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP...");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
 
    sntp_init();
}


void obtain_time(void)
{
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}


void sntp_parse_string (char * date, char * time, char * parsered_datetime){
    /*Date initial format = MM/DD/YY   final format = YY MM DD
      Time initial format = HH:mm:SS   final format = HH mm SS 
    */
    //YY
    parsered_datetime[0]=date[6]-48;
    parsered_datetime[1]=date[7]-48;
    
    //MM
    parsered_datetime[2]=date[0]-48;
    parsered_datetime[3]=date[1]-48;

    //DD
    parsered_datetime[4]=date[3]-48;
    parsered_datetime[5]=date[4]-48;

    //HH
    parsered_datetime[6]=time[0]-48;
    parsered_datetime[7]=time[1]-48;
    
    //mm
    parsered_datetime[8]=time[3]-48;
    parsered_datetime[9]=time[4]-48;

    //SS
    parsered_datetime[10]=time[6]-48;
    parsered_datetime[11]=time[7]-48;
    /*
    for (int i=0; i<12; i++){
        printf("Parsered[%d] = %d\n",i,parsered_datetime[i]);
    }*/
}


void sntp_config(void)
{
    char local_datetime[DATETIME_SIZE_FORMAT] = {0}; 
    
    //Wait until wifi is connected
    xEventGroupWaitBits(flags_hardware_available, FLAG_WIFI_CONNECTED, false, true, portMAX_DELAY);

    //Wait until wifi available
    xEventGroupWaitBits(flags_hardware_available, FLAG_WIFI_AVAILABLE, true, true, portMAX_DELAY);
    
    //++boot_count;
    //ESP_LOGI(TAG, "Boot count: %d", boot_count);

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }

    //for some reason it doesnt work if we put 8 characters 
    char date[9];
    char time[9];

    /* Set timezone to Central time (Mexico) and print local time
    https://sites.google.com/a/usapiens.com/opnode/time-zones
    */
    setenv("TZ", "CST6CDT,M3.2.0,M11.1.0", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    
    /* Convert date and time to string 
    https://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm 
    
    %x = Date representation, example	08/19/12
    %X = Time representation, example	02:50:06
    */
    strftime(date, sizeof(date), "%x", &timeinfo);
    strftime(time, sizeof(time), "%X", &timeinfo);
    
    //set wifi available again...
    xEventGroupSetBits(flags_hardware_available, FLAG_WIFI_AVAILABLE);    

    ESP_LOGI(TAG, "The current date/time in Mexico City is: %s %s", date,time);
    sntp_parse_string(date,time,local_datetime);

    //Set initial date and time to the real time clock
    ds3231_set_datetime(local_datetime);
}



