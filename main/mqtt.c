#include "mqtt.h"

esp_mqtt_client_handle_t mqtt_handle = NULL;
SemaphoreHandle_t s_wifi_connect_sem = NULL;

void mqtt_init(void)
{

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg_wifi_init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg_wifi_init));

    s_wifi_connect_sem = xSemaphoreCreateBinary();

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_callback, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_callback, NULL);

    wifi_config_t wifi_config = {

        .sta.threshold.authmode = WIFI_AUTH_WPA2_PSK,
        .sta.pmf_cfg.capable = true,
        .sta.pmf_cfg.required = false};
    memset(&wifi_config.sta.ssid, 0, sizeof(wifi_config.sta.ssid));
    memset(&wifi_config.sta.password, 0, sizeof(wifi_config.sta.password));
    memcpy(wifi_config.sta.ssid, MY_SSID, strlen(MY_SSID));
    memcpy(wifi_config.sta.password, MY_PASSWORD, strlen(MY_PASSWORD));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    xQueueSemaphoreTake(s_wifi_connect_sem, portMAX_DELAY);
}

void mqtt_start(void)
{

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

void wifi_event_callback(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    if (event_base == WIFI_EVENT)
    {

        switch (event_id)
        {

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
        default:
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {

        switch (event_id)
        {

        case IP_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG_STA, "esp got ip address");
            break;
        default:
            break;
        }
    }
}

void mqtt_event_callback(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    esp_mqtt_event_handle_t receivedData = (esp_mqtt_event_handle_t)event_data;

    switch (event_id)
    {

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
    default:
        break;
    }
}
