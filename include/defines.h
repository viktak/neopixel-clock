#ifndef DEFINES_H
#define DEFINES_H

#define MQTT_CUSTOMER           "viktak"
#define MQTT_PROJECT            "spiti"

#define HARDWARE_ID             "Word Clock 1.0"
#define HARDWARE_VERSION        "1.0"
#define SOFTWARE_ID             "NeoPixel Clock"

#define DEBUG_SPEED 921600

#define JSON_SETTINGS_SIZE  (JSON_OBJECT_SIZE(16) + 260)
#define JSON_MQTT_COMMAND_SIZE  300

#define CONNECTION_STATUS_LED_GPIO  0

#define DEFAULT_TIMEZONE                        2

#define WIFI_CONNECTION_TIMEOUT                 30
#define NTP_REFRESH_INTERVAL 3600

#define DEFAULT_MQTT_SERVER                     "test.mosquitto.org"
#define DEFAULT_MQTT_PORT                       1883
#define DEFAULT_MQTT_TOPIC                      "vnode"

#define DEFAULT_HEARTBEAT_INTERVAL              300 //  seconds

#define DEFAULT_TRAIL_LENGTH    5
#define DEFAULT_REVERSE_CLOCK_DIRECTION false
#define DEFAULT_SHOW_FIVE_MINUTE_MARKS true
#define DEFAULT_SHOW_SECONDS true
#define DEFAULT_FIVE_MINUTE_MARK_BRIGHTNESS 20
#define DEFAULT_LED_MAX_BRIGHTNESS 200
#define DEFAULT_LAST_OF_TRAIL_BRIGHTNESS 30

#define NUMBER_OF_LEDS 60

#endif