#include "aliiot.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "mbedtls/md5.h"
#include "mbedtls/md.h"
#include "esp_wifi.h"
#include <stdio.h>

#define TAG_ALIIOT "aliiot"

esp_mqtt_client_handle_t mqtt_handle_aliiot = NULL;
extern const char *g_aliot_ca;

char *getClientID(void)
{

    uint8_t mac[6];
    static char clientID[32] = {0};
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(clientID, sizeof(clientID), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return clientID;
}

void calc_hmd5(char *key, char *content, unsigned char *output)
{
    mbedtls_md_context_t md5_ctx;
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_MD5);

    mbedtls_md_init(&md5_ctx);
    mbedtls_md_setup(&md5_ctx, md_info, 1);
    mbedtls_md_hmac_starts(&md5_ctx, (const unsigned char *)key, strlen(key));
    mbedtls_md_hmac_update(&md5_ctx, (const unsigned char *)content, strlen(content));
    mbedtls_md_hmac_finish(&md5_ctx, output);
    mbedtls_md_free(&md5_ctx);
}

void core_hex2str(uint8_t *input, uint32_t input_len, char *output, uint8_t lowercase)
{
    char *upper = "0123456789ABCDEF";
    char *lower = "0123456789abcdef";
    char *encode = upper;
    int i = 0, j = 0;

    if (lowercase)
    {
        encode = lower;
    }

    for (i = 0; i < input_len; i++)
    {
        output[j++] = encode[(input[i] >> 4) & 0xf];
        output[j++] = encode[(input[i]) & 0xf];
    }
    output[j] = 0;
}

void mqtt_aliiot_event_callback(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    esp_mqtt_event_handle_t receivedData = (esp_mqtt_event_handle_t)event_data;

    switch (event_id)
    {

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_ALIIOT, "aliiot mqtt connected");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_ALIIOT, "aliiot mqtt disconnected");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG_ALIIOT, "aliiot mqtt received publish ack");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG_ALIIOT, "aliiot mqtt received subscribe ack");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG_ALIIOT, "topic->%s", receivedData->topic);
        ESP_LOGI(TAG_ALIIOT, "payload->%s", receivedData->data);
        break;
    default:
        break;
    }
}

void aliiot_start(void)
{

    esp_mqtt_client_config_t cfg_mqtt = {0};
    cfg_mqtt.broker.address.uri = ALIIOT_MQTT_URL;
    cfg_mqtt.broker.address.port = 8883;

    // client id
    char client_id[128];
    snprintf(client_id, sizeof(client_id), "%s|securemode=2,signmethod=hmacmd5|", getClientID());
    cfg_mqtt.credentials.client_id = client_id;

    // username
    char username[128];
    snprintf(username, sizeof(username), "%s&%s", ALIIOT_DEVICENAME, ALIIOT_PRODUCTKEY);
    cfg_mqtt.credentials.username = username;

    // password
    char sign_content[256];
    snprintf(sign_content, sizeof(sign_content), "clientId%sdeviceName%sproductKey%s", getClientID(), ALIIOT_DEVICENAME, ALIIOT_PRODUCTKEY);
    unsigned char password_hex[16];
    char password_str[33];
    calc_hmd5(ALIIOT_DEVICESECRET, sign_content, password_hex);
    core_hex2str(password_hex, sizeof(password_hex), password_str, 0);
    cfg_mqtt.credentials.authentication.password = password_str;

    // set TLS certificate
    cfg_mqtt.broker.verification.certificate = g_aliot_ca;

    // start mqtt connection
    mqtt_handle_aliiot = esp_mqtt_client_init(&cfg_mqtt);
    esp_mqtt_client_register_event(mqtt_handle_aliiot, ESP_EVENT_ANY_ID, mqtt_aliiot_event_callback, NULL);
    esp_mqtt_client_start(mqtt_handle_aliiot);
}

// 根证书
const char *g_aliot_ca = "-----BEGIN CERTIFICATE-----\n"
                         "MIID3zCCAsegAwIBAgISfiX6mTa5RMUTGSC3rQhnestIMA0GCSqGSIb3DQEBCwUA"
                         "MHcxCzAJBgNVBAYTAkNOMREwDwYDVQQIDAhaaGVqaWFuZzERMA8GA1UEBwwISGFu"
                         "Z3pob3UxEzARBgNVBAoMCkFsaXl1biBJb1QxEDAOBgNVBAsMB1Jvb3QgQ0ExGzAZ"
                         "BgNVBAMMEkFsaXl1biBJb1QgUm9vdCBDQTAgFw0yMzA3MDQwNjM2NThaGA8yMDUz"
                         "MDcwNDA2MzY1OFowdzELMAkGA1UEBhMCQ04xETAPBgNVBAgMCFpoZWppYW5nMREw"
                         "DwYDVQQHDAhIYW5nemhvdTETMBEGA1UECgwKQWxpeXVuIElvVDEQMA4GA1UECwwH"
                         "Um9vdCBDQTEbMBkGA1UEAwwSQWxpeXVuIElvVCBSb290IENBMIIBIjANBgkqhkiG"
                         "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAoK//6vc2oXhnvJD7BVhj6grj7PMlN2N4iNH4"
                         "GBmLmMdkF1z9eQLjksYc4Zid/FX67ypWFtdycOei5ec0X00m53Gvy4zLGBo2uKgi"
                         "T9IxMudmt95bORZbaph4VK82gPNU4ewbiI1q2loRZEHRdyPORTPpvNLHu8DrYBnY"
                         "Vg5feEYLLyhxg5M1UTrT/30RggHpaa0BYIPxwsKyylQ1OskOsyZQeOyPe8t8r2D4"
                         "RBpUGc5ix4j537HYTKSyK3Hv57R7w1NzKtXoOioDOm+YySsz9sTLFajZkUcQci4X"
                         "aedyEeguDLAIUKiYicJhRCZWljVlZActorTgjCY4zRajodThrQIDAQABo2MwYTAO"
                         "BgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUkWHoKi2h"
                         "DlS1/rYpcT/Ue+aKhP8wHwYDVR0jBBgwFoAUkWHoKi2hDlS1/rYpcT/Ue+aKhP8w"
                         "DQYJKoZIhvcNAQELBQADggEBADrrLcBY7gDXN8/0KHvPbGwMrEAJcnF9z4MBxRvt"
                         "rEoRxhlvRZzPi7w/868xbipwwnksZsn0QNIiAZ6XzbwvIFG01ONJET+OzDy6ZqUb"
                         "YmJI09EOe9/Hst8Fac2D14Oyw0+6KTqZW7WWrP2TAgv8/Uox2S05pCWNfJpRZxOv"
                         "Lr4DZmnXBJCMNMY/X7xpcjylq+uCj118PBobfH9Oo+iAJ4YyjOLmX3bflKIn1Oat"
                         "vdJBtXCj3phpfuf56VwKxoxEVR818GqPAHnz9oVvye4sQqBp/2ynrKFxZKUaJtk0"
                         "7UeVbtecwnQTrlcpWM7ACQC0OO0M9+uNjpKIbksv1s11xu0=\n"
                         "-----END CERTIFICATE-----";
