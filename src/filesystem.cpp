#include <LittleFS.h>

#define FORMAT_SPIFFS_IF_FAILED true

void setupFilesystem()
{
    if (!LittleFS.begin())
    {
        Serial.println("Error: Failed to initialize the filesystem!");
    }
}
