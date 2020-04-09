struct config{
  char ssid[32];
  char password[32];

  char friendlyName[30];
  uint heartbeatInterval;

  signed char timeZone;

  char mqttServer[64];
  int mqttPort;
  char mqttTopic[32];

  bool dst;

  int temperatureRefreshInterval;
};

struct MyAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
    uint16_t IndexPixel; // which pixel this animation is effecting
};
