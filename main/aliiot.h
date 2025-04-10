#ifndef _ALIIOT__H
#define _ALIIOT__H

// 设备名称
#define ALIIOT_DEVICENAME "jq01esp3201"
// 产品密钥
#define ALIIOT_PRODUCTKEY "k1w0l6HT0Kb"
// 设备密钥
#define ALIIOT_DEVICESECRET "2e0111c2e0444552024044dda1fdeffb"
// MQTT域名
#define ALIIOT_MQTT_URL "mqtts://iot-06z00gckmuw74xc.mqtt.iothub.aliyuncs.com"

extern char temp_cmd;
extern char spO2_cmd;
extern char ECG_cmd;
extern char GSR_cmd;

void aliiot_start(void);
char isAliiotConnected(void);

/**
 * 上报单个属性值（整形）
 * @param name 属性值名字
 * @param value 值
 * @return 无
 */
void aliot_post_property_int(const char *name, int value);

/**
 * 上报单个属性值（浮点）
 * @param name 属性值名字
 * @param value 值
 * @return 无
 */
void aliot_post_property_double(const char *name, double value);

#endif
