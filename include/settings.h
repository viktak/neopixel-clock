#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

#include "common.h"

namespace settings
{
 
#define DEFAULT_ADMIN_PASSWORD "admin"
#define DEFAULT_NODE_FRIENDLY_NAME "Neopixel Clock"
#define DEFAULT_AP_PASSWORD "esp12345678"

#ifdef __debugSettings
#define DEFAULT_MQTT_SERVER "192.168.123.42"
#else
#define DEFAULT_MQTT_SERVER "test.mosquitto.org"
#endif

#define DEFAULT_MQTT_PORT 1883
#define DEFAULT_MQTT_TOPIC "vNode"

#define DEFAULT_HEARTBEAT_INTERVAL 300 //  seconds

   //  Saved values
    extern char wifiSSID[22];
    extern char wifiPassword[32];

    extern char adminPassword[32];

    extern char accessPointPassword[32];

    extern char nodeFriendlyName[32];
    extern u_int heartbeatInterval;

    extern signed char timeZone;

    extern char mqttServer[64];
    extern int mqttPort;
    extern char mqttTopic[32];

    extern bool dst;

    extern uint8_t trailLength;
    extern bool reverseClockDirection;
    extern bool showFiveMinuteMarks;
    extern bool showSeconds;

    extern unsigned char fiveMinuteMarkBrightness;
    extern unsigned char ledMaxBrightness;
    extern unsigned char lastOfTrailBrightness;

    //  Calculated values
    extern char localHost[32];

    enum OPERATION_MODES
    {
        LED_CHASER,
        SLOW_PANELS,
        FAST_CHANGING_RANDOM_SEGMENTS,
        ROTATING_PANELS,
        ROTATING_PANELS_INVERTED,
        NUMBER_OF_OPERATION_MODES
    };

    extern void setup();

    extern bool LoadSettings();
    extern bool SaveSettings();
    extern void DefaultSettings();
}

#endif