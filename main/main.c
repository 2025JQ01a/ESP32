#include "TSkin.h"
#include "ECG.h"
#include "GSR.h"
#include "PPG.h"
#include "mqtt.h"
#include "aliiot.h"
#include "aliiot_dm.h"

#define USER_UART UART_NUM_0
#define TAG_UART "uart0"

static char bufUART[10];
static char tempData[10];
static char spO2Data[30];
static QueueHandle_t queueUART;
uint32_t count = 0;

void app_main(void)
{
    temp_ntc_init();

    PPG_cfg_t PPG_config;
    PPG_config.sda = GPIO_NUM_21;
    PPG_config.scl = GPIO_NUM_22;
    PPG_config.fre = 400000;
    PPG_init(&PPG_config);

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

    mqtt_init();
    aliiot_start();
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1)
    {

        // print temperature to serial port
        static float currentTemprature;
        currentTemprature = get_temp();
        // uart_write_bytes(USER_UART, bufUART, 100);
        snprintf(tempData, sizeof(tempData), "%f\n", currentTemprature);

        // uint8_t spO2[1];
        // i2c_read(MAX30102_ADDR, FIFO_DATA, 1, spO2);
        // snprintf(spO2Data, sizeof(spO2Data), "%d\n", spO2[0]);
        static float spO2;
        spO2 = max30102_getSpO2();
        snprintf(spO2Data, sizeof(spO2Data), "%.2f\n", spO2);
        if (spO2 != 1.15f)
        {
            // esp_mqtt_client_publish(mqtt_handle, MQTT_TOPIC1, spO2Data, strlen(spO2Data), 1, 0);
        }
        if (isAliiotConnected())
        {
            // aliot_post_property_int("Testing", count);
        }

        count++;
        vTaskDelay(1000);

        // if (xQueueReceive(queueUART, &evUART, 1) == pdTRUE)
        // {

        //     switch (evUART.type)
        //     {

        //     case UART_DATA:

        //         ESP_LOGI(TAG_UART, "UART0 Receive length: %i", evUART.size);
        //         uart_read_bytes(USER_UART, bufUART, evUART.size, pdMS_TO_TICKS(1000));
        //         uart_write_bytes(USER_UART, bufUART, evUART.size);
        //         break;
        //     case UART_BUFFER_FULL:

        //         uart_flush_input(USER_UART);
        //         xQueueReset(queueUART);
        //         break;
        //     case UART_FIFO_OVF:

        //         uart_flush_input(USER_UART);
        //         xQueueReset(queueUART);
        //         break;
        //     default:
        //         break;
        //     }
        // }
    }
}
