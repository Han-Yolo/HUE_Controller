#include "Wifi.h"

#include "main.h"

#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_err.h>

#include <nvs_flash.h>

#include <netdb.h>
#include <sys/socket.h>

#include <FreeRTOS.h>
#include <task.h>
#include <event_groups.h>

#include <esp_event_loop.h>

#include <string.h>


#define LOG_TAG "Wifi"

#define ERROR_HANDLER(message, ...) { \
    ESP_LOGE(LOG_TAG, message, ##__VA_ARGS__); \
    errorHandler(); }


/* The event group allows multiple bits for each event,
    but we only care about one event - are we connected
    to the AP with an IP? */
#define CONNECTED_BIT BIT0

/* FreeRTOS event group to signal when we are connected & ready 
    to make a request */
static EventGroupHandle_t wifi_event_group;

static int socket;

static struct sockaddr_in addr;


static void errorHandler(void);

static esp_err_t eventHandler(void* ctx, system_event_t* event);


void wifi_init(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );

    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(eventHandler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD
        },
    };

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    ESP_LOGI(LOG_TAG, "Connect to AP SSID:%s password:%s ...",
        CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);

    /* set up address to connect to */
    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = PP_HTONS(HUE_PORT);
    addr.sin_addr.s_addr = inet_addr(HUE_IP);

	/* Wait for the callback to set the CONNECTED_BIT in the
	   event group.
	*/
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
	ESP_LOGI(LOG_TAG, "Connected to AP");
}


int32_t wifi_send(const char* sendData, const uint32_t sendDataLen,
        char* recDataBuffer, uint32_t recDataBufferLen, uint32_t recDelay)
{
    socket = socket(AF_INET, SOCK_STREAM, 0);
    if(socket < 0)
    {
        ERROR_HANDLER("... Failed to allocate socket.");
    }

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 1;
    receiving_timeout.tv_usec = 0;

    if(setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout)) < 0)
    {
        ERROR_HANDLER("... failed to set socket receiving timeout");
    }

    if(connect(socket, (struct sockaddr*)&addr, sizeof(addr)) != 0)
    {
        ERROR_HANDLER("... socket connect failed errno=%d", errno);
    }

    /* Send request */
    if (write(socket, sendData, sendDataLen) < 0)
    {
        ERROR_HANDLER("... socket send failed");
    }

    if(recDelay > 0) vTaskDelay(recDelay);

    /* Read HTTP response */
    int32_t retVal = 0;
    int32_t readLen = 0;
    do
    {
        retVal = read(socket, recDataBuffer + readLen, 
                recDataBufferLen - readLen);

        readLen += retVal;
        //printf("retVal: %d\n", retVal);
    }
    while(retVal > 0);

    close(socket);

    if(retVal < 0) return retVal;
    return readLen;
}


static void errorHandler(void)
{
	close(socket);
}


static esp_err_t eventHandler(void* ctx, system_event_t* event)
{
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;

    switch(event->event_id)
    {
		case SYSTEM_EVENT_STA_START:
			esp_wifi_connect();
			break;

		case SYSTEM_EVENT_STA_GOT_IP:
			xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
			break;

		case SYSTEM_EVENT_STA_DISCONNECTED:
			ESP_LOGE(LOG_TAG, "Disconnect reason : %d", info->disconnected.reason);
			if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
				/*Switch to 802.11 bgn mode */
				esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
			}
			esp_wifi_connect();
			xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
			break;

		default:
			break;
    }
    return ESP_OK;
}
