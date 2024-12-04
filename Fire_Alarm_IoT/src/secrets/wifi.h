#include <cstdint>
namespace WiFiSecrets
{
    const char *ssid = "NgThanh";
    const char *pass = "ntthanh251103";
    const char *Temp_topic = "Temperature";
    const char *Hum_topic = "Humidity";
    const char *gasLevel_topic = "Gas Level";
    const char *alarm_topic = "Alarm Control";
    unsigned int publish_count = 0;
    uint16_t keepAlive = 15;
    uint16_t socketTimeout = 5;
}