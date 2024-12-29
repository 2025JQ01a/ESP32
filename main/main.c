#include <stdio.h>
#include <stdlib.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <string.h>
#include "esp_mac.h"
#include "mqtt_client.h"
#include "freertos/semphr.h"

#include "TSkin.h"
#include "ECG.h"
#include "GSR.h"
#include "PPG.h"

#define USER_UART       UART_NUM_0

#define TAG_UART     "uart0"
#define TAG_STA      "sta"
#define TAG_MQTT     "mqtt"

//#define MY_SSID  "H158-381_6B75"
#define MY_SSID  "dly"
//#define MY_PASSWORD  "sweethome10B"
#define MY_PASSWORD  "11111111"
#define MQTT_ADDRESS  "mqtt://broker.emqx.io"
#define MY_CLIENT_ID  "esp32_dly"
#define MQTT_USERNAME  "dly"
#define MQTT_PASSWORD  "11111111"
#define MQTT_TOPIC1    "/topic/dly_esp32send"
#define MQTT_TOPIC2    "/topic/dly_mqttxsend"

static char bufUART[1024];
static QueueHandle_t queueUART;
static esp_mqtt_client_handle_t mqtt_handle = NULL;
static SemaphoreHandle_t s_wifi_connect_sem = NULL;

void wifi_event_callback(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void mqtt_event_callback(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

void mqtt_start(void);


void app_main(void)
{
    temp_ntc_init();
    
    uart_event_t evUART;
    int resultUART;

    uart_config_t cfg_uart = 
    {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .parity = UART_PARITY_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
        .stop_bits = UART_STOP_BITS_1,
    };
    uart_param_config(USER_UART, &cfg_uart);
    uart_set_pin(USER_UART, GPIO_NUM_32, GPIO_NUM_33, -1, -1);
    uart_driver_install(USER_UART, 1024, 1024, 20, &queueUART, 0);
    
    // ESP_ERROR_CHECK(nvs_flash_init());
    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    // esp_netif_create_default_wifi_sta();
    // wifi_init_config_t cfg_wifi_init = WIFI_INIT_CONFIG_DEFAULT();
    // ESP_ERROR_CHECK(esp_wifi_init(&cfg_wifi_init));

    // s_wifi_connect_sem = xSemaphoreCreateBinary();

    // esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_callback, NULL);
    // esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_callback, NULL);

    // wifi_config_t wifi_config = {

    //     .sta.threshold.authmode = WIFI_AUTH_WPA2_PSK,
    //     .sta.pmf_cfg.capable = true,
    //     .sta.pmf_cfg.required = false
    // };
    // memset(&wifi_config.sta.ssid, 0, sizeof(wifi_config.sta.ssid));
    // memset(&wifi_config.sta.password, 0, sizeof(wifi_config.sta.password));
    // memcpy(wifi_config.sta.ssid, MY_SSID, strlen(MY_SSID));
    // memcpy(wifi_config.sta.password, MY_PASSWORD, strlen(MY_PASSWORD));

    // ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    // ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    // ESP_ERROR_CHECK(esp_wifi_start());

    // xQueueSemaphoreTake(s_wifi_connect_sem, portMAX_DELAY);
    // mqtt_start();

    // vTaskDelay(pdMS_TO_TICKS(2000));

    // int count = 0;
    // char countstr[50];
    // while (count < 30){

    //      snprintf(countstr, sizeof(countstr), "{\"count\": %d}", count);
    //      esp_mqtt_client_publish(mqtt_handle, MQTT_TOPIC1, countstr, strlen(countstr), 1, 0);
    //      esp_mqtt_client_publish(mqtt_handle, MQTT_TOPIC1, countstr, strlen(countstr), 1, 0);
    //     count ++;
    //      vTaskDelay(pdMS_TO_TICKS(2000));
    // }

    while(1){

        static float currentTemprature;
        currentTemprature = get_temp();
        snprintf(bufUART, sizeof(bufUART), "%f\n", currentTemprature);
        uart_write_bytes(USER_UART, bufUART, 100);
        vTaskDelay(100);
        
    //     if (xQueueReceive(queueUART, &evUART, portMAX_DELAY) == pdTRUE){

    //         switch(evUART.type){

    //             case UART_DATA:

    //                 ESP_LOGI(TAG_UART, "UART0 Receive length: %i", evUART.size);
    //                 uart_read_bytes(USER_UART, bufUART, evUART.size, pdMS_TO_TICKS(1000));
    //                 uart_write_bytes(USER_UART, bufUART, evUART.size);
    //                 break;
    //             case UART_BUFFER_FULL:

    //                 uart_flush_input(USER_UART);
    //                 xQueueReset(queueUART);
    //                 break;
    //             case UART_FIFO_OVF:

    //                 uart_flush_input(USER_UART);
    //                 xQueueReset(queueUART);
    //                 break;
    //             default: break;
    //         }
    //     }
    // }

}
}

void mqtt_start(void){

    esp_mqtt_client_config_t cfg_mqtt = {0};
    cfg_mqtt.broker.address.uri = MQTT_ADDRESS;
    cfg_mqtt.broker.address.port = 1883;
    cfg_mqtt.credentials.client_id = MY_CLIENT_ID;
    cfg_mqtt.credentials.username = MQTT_USERNAME;
    cfg_mqtt.credentials.authentication.password = MQTT_PASSWORD;

    mqtt_handle = esp_mqtt_client_init(&cfg_mqtt);

    esp_mqtt_client_register_event(mqtt_handle, ESP_EVENT_ANY_ID, mqtt_event_callback, NULL);

    esp_mqtt_client_start(mqtt_handle);
}

void wifi_event_callback(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

    if (event_base == WIFI_EVENT){

        switch(event_id){

            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG_STA, "esp connected to ap");
                xSemaphoreGive(s_wifi_connect_sem);
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG_STA, "esp disconnected, retrying");
                esp_wifi_connect();
                break;
            default: break;
        }
    }
    else if (event_base == IP_EVENT){

        switch(event_id){

            case IP_EVENT_STA_GOT_IP:
                ESP_LOGI(TAG_STA, "esp got ip address");
                break;
            default: break;
        }
    }
}

void mqtt_event_callback(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

    esp_mqtt_event_handle_t receivedData = (esp_mqtt_event_handle_t)event_data;
    
    switch (event_id){

        case MQTT_EVENT_CONNECTED:
            esp_mqtt_client_subscribe_single(mqtt_handle, MQTT_TOPIC2, 1);
            ESP_LOGI(TAG_MQTT, "mqtt connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG_MQTT, "mqtt disconnected");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG_MQTT, "mqtt received publish ack");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG_MQTT, "mqtt received subscribe ack");
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG_MQTT, "topic->%s", receivedData->topic);
            ESP_LOGI(TAG_MQTT, "payload->%s", receivedData->data);
            break;
        default: break;
    }
}
