#define MQTT_CUSTOMER           "viktak"
#define MQTT_PROJECT            "spiti"

#define HARDWARE_ID             "ESP-12E-12F BB"
#define HARDWARE_VERSION        "1.0"
#define SOFTWARE_ID             "NeoPixel Clock"
#define FIRMWARE_VERSION        "1.0"

#define JSON_SETTINGS_SIZE  (JSON_OBJECT_SIZE(16) + 260)
#define JSON_MQTT_COMMAND_SIZE  300

#define CONNECTION_STATUS_LED_GPIO  0

#define DEFAULT_HEARTBEAT_INTERVAL  300

#define DEFAULT_TRAIL_LENGTH    5
#define DEFAULT_REVERSE_CLOCK_DIRECTION false
#define DEFAULT_SHOW_FIVE_MINUTE_MARKS true
#define DEFAULT_SHOW_SECONDS true

#define NUMBER_OF_LEDS 60
#define FIVE_MINUTE_MARK_BRIGHTNESS 50
#define LED_MAX_BRIGHTNESS 255
#define LAST_OF_TRAIL_BRIGHTNESS 50

