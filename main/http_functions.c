#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_tls.h"

#include "http_functions.h"

static const char TAG[] = "HTTP_FUNCTIONS";

// esp_http_client event handler
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
	 static int output_len;       // Stores number of bytes read
    
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            break;
        case HTTP_EVENT_ON_CONNECTED:
            break;
        case HTTP_EVENT_HEADER_SENT:
            break;
        case HTTP_EVENT_ON_HEADER:
            break;
        case HTTP_EVENT_ON_DATA:
			//ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA");
			if (!esp_http_client_is_chunked_response(evt->client)) {
				// If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                    //strncpy(evt->user_data, evt->data, evt->data_len);
                } 
                output_len += evt->data_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            output_len = 0;
			break;
        case HTTP_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != ESP_OK) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
			break;
    }
    return ESP_OK;
}

//look for web page content using http get method
void http_get_receive ( char *local_response_buffer, int HTTP_RESPONSE_SIZE, char* HTTP_GET_RECEIVE_URL){
	
	// configure the esp_http_client
	esp_http_client_config_t http_config = {
		.url = HTTP_GET_RECEIVE_URL,
		.event_handler = _http_event_handler,
		.buffer_size = HTTP_RESPONSE_SIZE,
		.user_data = local_response_buffer, // Pass address of local buffer to get response    
		.timeout_ms=3000,
	};
	esp_http_client_handle_t client;	

	// downloading the json file
	client = esp_http_client_init(&http_config);
	esp_http_client_perform(client);

	ESP_LOGI(TAG, "RESPONSE FROM SERVER (http_get_receive): %s", local_response_buffer);
	
	// cleanup
	esp_http_client_cleanup(client);
}


/*
 * HTTP Post request function (send buffer by HTTP POST)
 */
esp_err_t http_post_send(char * data_buffer, int DATA_BUFFER_SIZE, char * HTTP_POST_SEND_URL)
{
    printf("POST_BUFFER: current buffer:%p\n",data_buffer);

    //ESP_LOGI(TAG,"Memory allocation");
    char * local_response_buffer=NULL;
    local_response_buffer = heap_caps_malloc(DEFAULT_BUFFER_SIZE_RESPONSE * sizeof *local_response_buffer, MALLOC_CAP_8BIT);  
    
    if (local_response_buffer==NULL){
            ESP_LOGE(TAG,"Memory allocation failed");    
        }else{
            //ESP_LOGI(TAG,"Reseting array, filling it with zeros");
            memset(local_response_buffer, 0, DEFAULT_BUFFER_SIZE_RESPONSE); // <---- reset array / fill array with zeros
        }

    esp_http_client_config_t config = {
        .url = HTTP_POST_SEND_URL,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .method=HTTP_METHOD_POST,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,
        .buffer_size = DEFAULT_BUFFER_SIZE_RESPONSE,
        .timeout_ms=5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t error_handler=ESP_OK;

    // POST Request
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_post_field(client, data_buffer, DATA_BUFFER_SIZE);
    
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        ESP_LOGI(TAG, "RESPONSE FROM SERVER (http_post_send): %s", local_response_buffer);

    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        error_handler=ESP_FAIL;
    }

    //cleanup
    esp_http_client_cleanup(client);
    
    //ESP_LOGI(TAG,"Deleting buffer");
    heap_caps_free(local_response_buffer);
    local_response_buffer=NULL;
    
    return error_handler;
}
