#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include <os_type.h>

static const int32_t DEBUG_SPEED = 115200;
static const String HARDWARE_ID = "Word Clock Base";
static const String HARDWARE_VERSION = "1.0";
static const String FIRMWARE_ID = "neopixel-clock";

extern String GetDeviceMAC();
extern String DateTimeToString(const time_t time);
extern String TimeIntervalToString(const time_t time);
extern String GetDeviceMAC();

extern void setupCommon();

#endif