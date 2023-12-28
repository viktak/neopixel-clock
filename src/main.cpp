#include "version.h"
#include "common.h"
#include "settings.h"

#include "filesystem.h"
#include "mqtt.h"
#include "connection.h"
#include "leds.h"
#include "ledstrip.h"

void setup()
{
    Serial.begin(DEBUG_SPEED);

    String FirmwareVersionString = String(FIRMWARE_VERSION) + " @ " + String(__TIME__) + " - " + String(__DATE__);

    Serial.printf("\r\n\n\nBooting ESP node %u...\r\n", ESP.getChipId());
    Serial.println("Hardware ID:      " + HARDWARE_ID);
    Serial.println("Hardware version: " + HARDWARE_VERSION);
    Serial.println("Software ID:      " + FIRMWARE_ID);
    Serial.println("Software version: " + FirmwareVersionString);
    Serial.println();

    setupFilesystem();
    settings::setup();
    setupCommon();
    setupLEDs();
    setupMQTT();
    setupLedStrip();

    //  Finished setup()
    Serial.println("Setup finished successfully.");
}

void loop()
{
    loopConnection();
}