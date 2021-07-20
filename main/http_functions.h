
/*Max retry times to send buffer to SD card*/
#define MAX_RETRY_SEND_WIFI 5 


#define MONGODB_SERVER  "https://www.watchbird.org/datalogger"

#define DEFAULT_BUFFER_SIZE_RESPONSE 64

/*
http_get_receive: get server response/page content using HTTP GET method

Parameters:
local_response_buffer: buffer to store server response 
HTTP_RESPONSE_SIZE: int number of maximum size of local_response_buffer
HTTP_GET_RECEIVE_URL: web page where look for information using HTTP GET

*/

void http_get_receive ( char *local_response_buffer, int HTTP_RESPONSE_SIZE, char* HTTP_GET_RECEIVE_URL);


/*
 * HTTP Post request function (send buffer by HTTP POST)
 */
esp_err_t http_post_send(char * data_buffer, int DATA_BUFFER_SIZE, char * HTTP_POST_SEND_URL);
