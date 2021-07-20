

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "wifi_functions.h"
#include "task_list.h"


static const char *TAG = "wifi station";

// Wifi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    //If first time, then tries to connect to wifi 
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } 
    //If wifi disconnected, then tries to reconnect 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGE(TAG,"connect to the AP fail");
        xEventGroupClearBits(flags_hardware_available, FLAG_WIFI_AVAILABLE|FLAG_WIFI_CONNECTED);
        xEventGroupSetBits(flags_hardware_available, FLAG_WIFI_FAIL); 
        esp_wifi_connect();
    } 
    //If got IP address then show it 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(flags_hardware_available, FLAG_WIFI_AVAILABLE|FLAG_WIFI_CONNECTED);
        xEventGroupClearBits(flags_hardware_available, FLAG_WIFI_FAIL);
        
    }
}


void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,&wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}