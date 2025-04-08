#ifndef MQTT_H
#define MQTT_H

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include <stdio.h>
#include <stdlib.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <string.h>
#include "esp_mac.h"
#include "freertos/semphr.h"

#define TAG_STA "sta"
#define TAG_MQTT "mqtt"

// #define MY_SSID  "H158-381_6B75"
#define MY_SSID "dly"
// #define MY_PASSWORD  "sweethome10B"
#define MY_PASSWORD "11111111"
#define MQTT_ADDRESS "mqtt://broker.emqx.io"
#define MY_CLIENT_ID "esp32_dly"
#define MQTT_USERNAME "dly"
#define MQTT_PASSWORD "11111111"
#define MQTT_TOPIC1 "/topic/dly_esp32send"
#define MQTT_TOPIC2 "/topic/dly_mqttxsend"

extern esp_mqtt_client_handle_t mqtt_handle;
extern SemaphoreHandle_t s_wifi_connect_sem;

void wifi_event_callback(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void mqtt_event_callback(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void mqtt_init(void);
void mqtt_start(void);

#endif
