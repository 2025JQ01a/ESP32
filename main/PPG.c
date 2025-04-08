#include "driver/i2c.h"
// #include "driver/i2c_master.h"
#include "esp_log.h"
#include "PPG.h"

static const char *TAG = "PPG";

static float max30102_data[2];
static float ppg_data_cache_RED[CACHE_NUMS] = {0}; // 缓存区
static float ppg_data_cache_IR[CACHE_NUMS] = {0};  // 缓存区
static uint16_t cache_counter = 0;                 // 缓存计数器

esp_err_t PPG_init(PPG_cfg_t *cfg)
{
    int i2c_master_port = I2C_PORT;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = cfg->sda,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = cfg->scl,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = cfg->fre,
    };
    ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0));

    // configure MAX30102
    i2c_write(MAX30102_ADDR, MODE_CONFIGURATION, 1, (uint8_t[]){0x40});
    vTaskDelay(5);
    i2c_write(MAX30102_ADDR, INTERRUPT_ENABLE1, 1, (uint8_t[]){0xE0});
    i2c_write(MAX30102_ADDR, INTERRUPT_ENABLE2, 1, (uint8_t[]){0x00});
    i2c_write(MAX30102_ADDR, FIFO_WR_POINTER, 1, (uint8_t[]){0x00});
    i2c_write(MAX30102_ADDR, FIFO_OV_COUNTER, 1, (uint8_t[]){0x00});
    i2c_write(MAX30102_ADDR, FIFO_RD_POINTER, 1, (uint8_t[]){0x00});
    i2c_write(MAX30102_ADDR, FIFO_CONFIGURATION, 1, (uint8_t[]){0x4F});   // FIFO configuration: sample averaging(4),FIFO rolls on full(0), FIFO almost full value(15 empty data samples when interrupt is issued)
    i2c_write(MAX30102_ADDR, MODE_CONFIGURATION, 1, (uint8_t[]){0x03});   // MODE configuration:SpO2 mode
    i2c_write(MAX30102_ADDR, SPO2_CONFIGURATION, 1, (uint8_t[]){0x2A});   // SpO2 configuration:ACD resolution:15.63pA,sample rate control:200Hz, LED pulse width:215 us
    i2c_write(MAX30102_ADDR, LED1_PULSE_AMPLITUDE, 1, (uint8_t[]){0x2f}); // IR LED
    i2c_write(MAX30102_ADDR, LED2_PULSE_AMPLITUDE, 1, (uint8_t[]){0x2f}); // RED LED current
    i2c_write(MAX30102_ADDR, TEMPERATURE_CONFIG, 1, (uint8_t[]){0x01});   // temp

    return ESP_OK;
}

esp_err_t i2c_read(uint8_t slave_addr, uint8_t register_addr, uint8_t read_len, uint8_t *data_buf)
{
    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    if (!i2c_cmd)
    {
        ESP_LOGE(TAG, "Error i2c_cmd creat fail!");
        return ESP_FAIL;
    }
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, (slave_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(i2c_cmd, register_addr, I2C_MASTER_ACK);

    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, (slave_addr << 1) | I2C_MASTER_READ, true);
    for (int i = 0; i < read_len; i++)
    {
        if (i == read_len - 1)
            i2c_master_read_byte(i2c_cmd, &data_buf[i], I2C_MASTER_NACK);
        else
            i2c_master_read_byte(i2c_cmd, &data_buf[i], I2C_MASTER_ACK);
    }
    i2c_master_stop(i2c_cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, i2c_cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(i2c_cmd);
    return ret;
}

esp_err_t i2c_write(uint8_t slave_addr, uint8_t register_addr, uint8_t write_len, uint8_t *data_buf)
{
    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    if (!i2c_cmd)
    {
        ESP_LOGE(TAG, "Error i2c_cmd create fail!");
        return ESP_FAIL;
    }

    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, (slave_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(i2c_cmd, register_addr, I2C_MASTER_ACK);
    for (int i = 0; i < write_len; i++)
    {
        i2c_master_write_byte(i2c_cmd, data_buf[i], I2C_MASTER_ACK);
    }
    i2c_master_stop(i2c_cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, i2c_cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(i2c_cmd);
    return ret;
}

void max30102_fifo_read(float *output_data)
{
    uint8_t receive_data[6];
    uint32_t data[2];
    i2c_read(MAX30102_ADDR, FIFO_DATA, 6, receive_data);
    data[0] = ((receive_data[0] << 16 | receive_data[1] << 8 | receive_data[2]) & 0x03ffff);
    data[1] = ((receive_data[3] << 16 | receive_data[4] << 8 | receive_data[5]) & 0x03ffff);
    *output_data = data[0];
    *(output_data + 1) = data[1];
}

float max30102_getSpO2(void)
{
    max30102_fifo_read(max30102_data);
    // ESP_LOGI(TAG, "IR: %f, RED: %f", max30102_data[0], max30102_data[1]);
    if ((max30102_data[0] > PPG_DATA_THRESHOLD) && (max30102_data[1] > PPG_DATA_THRESHOLD)) // 大于阈值，说明传感器有接触
    {
        ppg_data_cache_IR[cache_counter] = max30102_data[0];
        ppg_data_cache_RED[cache_counter] = max30102_data[1];
        cache_counter++;
        // ESP_LOGI(TAG, "reach, %d", cache_counter);
    }
    else // 小于阈值
    {
        cache_counter = 0;
        // ESP_LOGI(TAG, "not reach, %d", cache_counter);
    }
    // ESP_LOGI(TAG, "then, %d", cache_counter);
    if (cache_counter >= CACHE_NUMS) // 收集满了数据
    {
        float ir_max = *ppg_data_cache_IR, ir_min = *ppg_data_cache_IR;
        float red_max = *ppg_data_cache_RED, red_min = *ppg_data_cache_RED;
        float R;
        uint16_t i;
        for (i = 1; i < CACHE_NUMS; i++)
        {
            if (ir_max < *(ppg_data_cache_IR + i))
            {
                ir_max = *(ppg_data_cache_IR + i);
            }
            if (ir_min > *(ppg_data_cache_IR + i))
            {
                ir_min = *(ppg_data_cache_IR + i);
            }
            if (red_max < *(ppg_data_cache_RED + i))
            {
                red_max = *(ppg_data_cache_RED + i);
            }
            if (red_min > *(ppg_data_cache_RED + i))
            {
                red_min = *(ppg_data_cache_RED + i);
            }
        }
        R = ((ir_max - ir_min) * red_min) / ((red_max - red_min) * ir_min);
        //			 R=((ir_max+ir_min)*(red_max-red_min))/((red_max+red_min)*(ir_max-ir_min));
        cache_counter = 0;
        return ((-45.060) * R * R + 30.354 * R + 94.845);
    }
    else
    {
        return 1.15f;
    }
}
