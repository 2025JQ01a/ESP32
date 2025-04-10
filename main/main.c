#include "esp_log.h"
#include <math.h>

#include "TSkin.h"
#include "ECG.h"
#include "GSR.h"
#include "PPG.h"
#include "mqtt.h"
#include "aliiot.h"
#include "aliiot_dm.h"
#include "computeFFT.h"

// for UART debugging, not used
#define USER_UART UART_NUM_2
static char bufUART[10];
static QueueHandle_t queueUART;

// debugging data
uint32_t count = 0;

// for MATLAB algo testing
/*
static double signal_data[200] = {0}; // 输入信号数据
static creal_T fft_result[200] = {0}; // 存储 FFT 结果
*/

#define TAG_MAIN "main"

static uint32_t counter_transmit = 0;

static char temp_data[30];
static char spO2_data[30];
static char ECG_data[30];
static char GSR_data[30];

static float currentTemprature;
static float spO2;
static float spO2_recorded;
static int ECG_value;
static int GSR_value;

void app_main(void)
{
    // for MATLAB algo testing
    /*
    // 填充示例信号数据（例如：正弦波）
    for (int i = 0; i < 200; i++)
    {
        signal_data[i] = sin(2 * M_PI * i / 200); // 示例信号
    }
    for (int i = 0; i < 200; i++)
    {
        ESP_LOGI(TAG_MAIN, "input:%.2f", signal_data[i]);
    }
    // 调用 computeFFT 函数
    computeFFT(signal_data, fft_result);
    // 输出 FFT 结果
    for (int i = 0; i < 200; i++)
    {
        ESP_LOGI(TAG_MAIN, "Y[%d] = %.2f + %.2fi\n", i, fft_result[i].re, fft_result[i].im);
    }
    */

    // for UART debugging, not used
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

    temp_ntc_init();

    PPG_cfg_t PPG_config;
    PPG_config.sda = GPIO_NUM_21;
    PPG_config.scl = GPIO_NUM_22;
    PPG_config.fre = 400000;
    PPG_init(&PPG_config);

    mqtt_init();
    aliiot_start();
    // mqtt_start();
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1)
    {
        TickType_t xLastWakeTime = xTaskGetTickCount();

        // for UART debugging, not used
        uart_write_bytes(USER_UART, temp_data, 100);

        if (temp_cmd)
        {
            currentTemprature = get_temp();
        }
        else
        {
            currentTemprature = -1;
        }
        snprintf(temp_data, sizeof(temp_data), "%f\n", currentTemprature);
        if (ECG_cmd)
        {
            ECG_value = get_ECG();
        }
        else
        {
            ECG_value = -1;
        }
        snprintf(ECG_data, sizeof(ECG_data), "%d\n", ECG_value);
        if (GSR_cmd)
        {
            GSR_value = get_GSR();
        }
        else
        {
            GSR_value = -1;
        }
        snprintf(GSR_data, sizeof(GSR_data), "%d\n", GSR_value);
        if (spO2_cmd)
        {
            spO2 = max30102_getSpO2();
        }
        else
        {
            spO2 = 0;
        }
        if (spO2 != 0)
        {
            spO2_recorded = spO2;
        }
        snprintf(spO2_data, sizeof(spO2_data), "%.2f\n", spO2);

        if (isAliiotConnected() && (counter_transmit >= 400))
        {
            aliot_post_property_int("Testing", 0);
            if (spO2_recorded != 0)
            {
                aliot_post_property_double("Value_PPG", spO2_recorded);
            }
            if (currentTemprature != -1)
            {
                aliot_post_property_double("Value_Temp", currentTemprature);
            }
            if (ECG_value != -1)
            {
                aliot_post_property_int("Value_ECG", ECG_value);
            }
            if (GSR_value != -1)
            {
                aliot_post_property_int("Value_GSR", GSR_value);
            }

            counter_transmit = 0;
        }

        counter_transmit++;

        // for UART debugging, not used
        if (xQueueReceive(queueUART, &evUART, 1) == pdTRUE)
        {

            switch (evUART.type)
            {

            case UART_DATA:

                ESP_LOGI(TAG_MAIN, "UART0 Receive length: %i", evUART.size);
                uart_read_bytes(USER_UART, bufUART, evUART.size, pdMS_TO_TICKS(1000));
                uart_write_bytes(USER_UART, bufUART, evUART.size);
                break;
            case UART_BUFFER_FULL:

                uart_flush_input(USER_UART);
                xQueueReset(queueUART);
                break;
            case UART_FIFO_OVF:

                uart_flush_input(USER_UART);
                xQueueReset(queueUART);
                break;
            default:
                break;
            }
        }

        vTaskDelayUntil(&xLastWakeTime, 5);
    }
}
