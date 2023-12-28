#include <Arduino.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>

#include "ArduinoJson.h"

#include "common.h"
#include "settings.h"

#define DEFAULT_TIMEZONE 13

#define DEFAULT_TRAIL_LENGTH 5
#define DEFAULT_REVERSE_CLOCK_DIRECTION false
#define DEFAULT_SHOW_FIVE_MINUTE_MARKS true
#define DEFAULT_SHOW_SECONDS true
#define DEFAULT_FIVE_MINUTE_MARK_BRIGHTNESS 20
#define DEFAULT_LED_MAX_BRIGHTNESS 200
#define DEFAULT_LAST_OF_TRAIL_BRIGHTNESS 30

namespace settings
{
    //  Saved values
    char wifiSSID[22];
    char wifiPassword[32] = "";

    char adminPassword[32] = DEFAULT_ADMIN_PASSWORD;

    char nodeFriendlyName[32] = DEFAULT_NODE_FRIENDLY_NAME;
    u_int heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;

    signed char timeZone = DEFAULT_TIMEZONE;

    char mqttServer[64] = DEFAULT_MQTT_SERVER;
    int mqttPort = DEFAULT_MQTT_PORT;
    char mqttTopic[32];

    bool dst;

    uint8_t trailLength = DEFAULT_TRAIL_LENGTH;
    bool reverseClockDirection = DEFAULT_REVERSE_CLOCK_DIRECTION;
    bool showFiveMinuteMarks = DEFAULT_SHOW_FIVE_MINUTE_MARKS;
    bool showSeconds = DEFAULT_SHOW_SECONDS;

    unsigned char fiveMinuteMarkBrightness = DEFAULT_FIVE_MINUTE_MARK_BRIGHTNESS;
    unsigned char ledMaxBrightness = DEFAULT_LED_MAX_BRIGHTNESS;
    unsigned char lastOfTrailBrightness = DEFAULT_LAST_OF_TRAIL_BRIGHTNESS;

    //  Calculated values
    char accessPointPassword[32];
    char localHost[32];

    bool LoadSettings()
    {

        sprintf(localHost, "%s-%s", DEFAULT_MQTT_TOPIC, GetDeviceMAC().substring(6).c_str());

        File configFile = LittleFS.open("/config.json", "r");
        if (!configFile)
        {
            Serial.println("Failed to open config file.");
            return false;
        }

        size_t size = configFile.size();
        if (size > 1024)
        {
            Serial.println("Config file size is too large.");
            return false;
        }

        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        // We don't use String here because ArduinoJson library requires the input
        // buffer to be mutable. If you don't use ArduinoJson, you may as well
        // use configFile.readString instead.
        configFile.readBytes(buf.get(), size);
        configFile.close();

        StaticJsonDocument<768> doc;
        DeserializationError error = deserializeJson(doc, buf.get());

        if (error)
        {
            Serial.println("Failed to parse config file.");
            Serial.println(error.c_str());
            return false;
        }

#ifdef __debugSettings
        serializeJsonPretty(doc, Serial);
        Serial.println();
#endif

        if (doc["ssid"])
        {
            strcpy(wifiSSID, doc["ssid"]);
        }
        else
        {
            sprintf(wifiSSID, "%s-%s", DEFAULT_MQTT_TOPIC, GetDeviceMAC().substring(6).c_str());
        }

        if (doc["password"])
        {
            strcpy(wifiPassword, doc["password"]);
        }

        if (doc["accessPointPassword"])
        {
            strcpy(accessPointPassword, doc["accessPointPassword"]);
        }

        if (doc["mqttServer"])
        {
            strcpy(mqttServer, doc["mqttServer"]);
        }

        if (doc["mqttPort"])
        {
            mqttPort = doc["mqttPort"];
        }

        if (doc["mqttTopic"])
        {
            strcpy(mqttTopic, doc["mqttTopic"]);
        }
        else
        {
            sprintf(mqttTopic, localHost);
        }

        if (doc["friendlyName"])
        {
            strcpy(nodeFriendlyName, doc["friendlyName"]);
        }

        if (doc["timezone"])
        {
            timeZone = doc["timezone"];
        }

        if (doc["heartbeatInterval"])
        {
            heartbeatInterval = doc["heartbeatInterval"];
        }

        if (doc["trailLength"])
        {
            trailLength = doc["trailLength"];
        }

        if (doc["reverseClockDirection"])
        {
            reverseClockDirection = doc["reverseClockDirection"];
        }

        if (doc["showFiveMinuteMarks"])
        {
            showFiveMinuteMarks = doc["showFiveMinuteMarks"];
        }

        if (doc["showSeconds"])
        {
            showSeconds = doc["showSeconds"];
        }

        if (doc["fiveMinuteMarkBrightness"])
        {
            fiveMinuteMarkBrightness = (unsigned char)doc["fiveMinuteMarkBrightness"];
        }

        if (doc["ledMaxBrightness"])
        {
            ledMaxBrightness = (unsigned char)doc["ledMaxBrightness"];
        }

        if (doc["lastOfTrailBrightness"])
        {
            lastOfTrailBrightness = (unsigned char)doc["lastOfTrailBrightness"];
        }

        if (strcmp(localHost, mqttTopic) != 0)
        {
            char mac[7];
            strcpy(mac, GetDeviceMAC().substring(6).c_str());

            char topic[sizeof(localHost) + sizeof(mac) + 1];
            for (size_t i = 0; i < sizeof(topic); i++)
            {
                topic[i] = 0;
            }

            size_t pos = 0;
            for (size_t i = 0; i < 25; i++)
            {
                if (mqttTopic[i] != 0)
                {
                    topic[i] = mqttTopic[i];
                    pos = i;
                }
                else
                {
                    pos++;
                    break;
                }
            }

            topic[pos++] = '-';

            for (size_t i = 0; i < sizeof(mac); i++)
            {
                topic[pos + i] = mac[i];
            }

            strcpy(localHost, topic);
        }

        return true;
    }

    bool SaveSettings()
    {
        StaticJsonDocument<384> doc;

        doc["ssid"] = wifiSSID;
        doc["password"] = wifiPassword;
        doc["accessPointPassword"] = accessPointPassword;

        doc["heartbeatInterval"] = heartbeatInterval;

        doc["timezone"] = timeZone;

        doc["mqttServer"] = mqttServer;
        doc["mqttPort"] = mqttPort;
        doc["mqttTopic"] = mqttTopic;

        doc["friendlyName"] = nodeFriendlyName;

        doc["trailLength"] = trailLength;
        doc["reverseClockDirection"] = reverseClockDirection;
        doc["showFiveMinuteMarks"] = showFiveMinuteMarks;
        doc["showSeconds"] = showSeconds;
        doc["fiveMinuteMarkBrightness"] = fiveMinuteMarkBrightness;
        doc["ledMaxBrightness"] = ledMaxBrightness;
        doc["lastOfTrailBrightness"] = lastOfTrailBrightness;

#ifdef __debugSettings
        serializeJsonPretty(doc, Serial);
        Serial.println();
#endif

        File configFile = LittleFS.open("/config.json", "w");
        if (!configFile)
        {
            Serial.println("Failed to open config file for writing");
            return false;
        }
        serializeJson(doc, configFile);
        configFile.close();

        return true;
    }

    void DefaultSettings()
    {
        sprintf(localHost, "%s-%s", DEFAULT_MQTT_TOPIC, GetDeviceMAC().substring(6).c_str());

        strcpy(wifiSSID, localHost);
        strcpy(wifiPassword, DEFAULT_AP_PASSWORD);
        strcpy(mqttServer, DEFAULT_MQTT_SERVER);

        strcpy(accessPointPassword, DEFAULT_AP_PASSWORD);

        mqttPort = DEFAULT_MQTT_PORT;

        strcpy(mqttTopic, localHost);

        timeZone = DEFAULT_TIMEZONE;

        strcpy(nodeFriendlyName, DEFAULT_NODE_FRIENDLY_NAME);
        heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;

        trailLength = DEFAULT_TRAIL_LENGTH;
        reverseClockDirection = DEFAULT_REVERSE_CLOCK_DIRECTION;
        showFiveMinuteMarks = DEFAULT_SHOW_FIVE_MINUTE_MARKS;
        showSeconds = DEFAULT_SHOW_SECONDS;
        fiveMinuteMarkBrightness = DEFAULT_FIVE_MINUTE_MARK_BRIGHTNESS;
        ledMaxBrightness = DEFAULT_LED_MAX_BRIGHTNESS;
        lastOfTrailBrightness = DEFAULT_LAST_OF_TRAIL_BRIGHTNESS;

        if (!SaveSettings())
        {
            Serial.println("Failed to save config!");
        }
        else
        {
            ESP.restart();
#ifdef __debugSettings
            Serial.println("Settings saved.");
#endif
        }
    }

    void setup()
    {
        if (!LoadSettings())
            DefaultSettings();
    }
}