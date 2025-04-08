#ifndef _ALIIOT_DM__H
#define _ALIIOT_DM__H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "cJSON.h"

// 物模型
typedef struct
{
    cJSON *dm_js;
    char *dm_js_str;
    int data_len;
} ALIOT_DM_DES;

typedef enum
{
    ALIOT_DM_POST,    // 常规属性上报
    ALIOT_DM_SET_ACK, // 属性设置回复
    ALIOT_DM_EVENT,   // 事件上报
} ALIOT_DM_TYPE;

/**
 * 生成一dm
 * @param 无
 * @return dm
 */
ALIOT_DM_DES *aliot_malloc_dm(ALIOT_DM_TYPE dm_type);

/**
 * 往物模型(dm)里面添加一整形
 * @param dm 物模型结构体
 * @param name 属性名字
 * @param value 值
 * @return 无
 */
void aliot_set_dm_int(ALIOT_DM_DES *dm, const char *name, int value);

/**
 * 生成字符串保存在dm->dm_js_str
 * @param dm 无
 * @return 无
 */
void aliot_dm_serialize(ALIOT_DM_DES *dm);

/**
 * 释放一dm
 * @param 无
 * @return dm
 */
void aliot_free_dm(ALIOT_DM_DES *dm);

/**
 * 添加属性设置回复
 * @param code 错误码
 * @param message 信息
 * @return 无
 */
void aliot_set_property_ack(ALIOT_DM_DES *dm, int code, const char *message);

#endif
