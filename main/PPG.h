#ifndef _PPG__H
#define _PPG__H
#include "driver/gpio.h"
#include "esp_err.h"

#define I2C_PORT I2C_NUM_0
#define CACHE_NUMS 150            // 缓存数
#define PPG_DATA_THRESHOLD 100000 // 检测阈值

#define MAX30102_ADDR 0x57
// #define MAX30102_ADDR 0x68

#define INTERRUPT_STATUS1 0X00
#define INTERRUPT_STATUS2 0X01
#define INTERRUPT_ENABLE1 0X02
#define INTERRUPT_ENABLE2 0X03
#define FIFO_WR_POINTER 0X04
#define FIFO_OV_COUNTER 0X05
#define FIFO_RD_POINTER 0X06
#define FIFO_DATA 0X07
#define FIFO_CONFIGURATION 0X08
#define MODE_CONFIGURATION 0X09
#define SPO2_CONFIGURATION 0X0A
#define LED1_PULSE_AMPLITUDE 0X0C
#define LED2_PULSE_AMPLITUDE 0X0D
#define MULTILED1_MODE 0X11
#define MULTILED2_MODE 0X12
#define TEMPERATURE_INTEGER 0X1F
#define TEMPERATURE_FRACTION 0X20
#define TEMPERATURE_CONFIG 0X21
#define VERSION_ID 0XFE
#define PART_ID 0XFF
// #define PART_ID 0X75

typedef struct
{
    gpio_num_t scl; // SCL管脚
    gpio_num_t sda; // SDA管脚
    uint32_t fre;   // I2C速率
} PPG_cfg_t;

esp_err_t PPG_init(PPG_cfg_t *cfg);
esp_err_t i2c_read(uint8_t slave_addr, uint8_t register_addr, uint8_t read_len, uint8_t *data_buf);
esp_err_t i2c_write(uint8_t slave_addr, uint8_t register_addr, uint8_t write_len, uint8_t *data_buf);
void max30102_fifo_read(float *output_data);
float max30102_getSpO2();

#endif