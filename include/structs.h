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

  uint8_t trailLength = DEFAULT_TRAIL_LENGTH;
  bool reverseClockDirection = DEFAULT_REVERSE_CLOCK_DIRECTION;
  bool showFiveMinuteMarks = DEFAULT_SHOW_FIVE_MINUTE_MARKS;
  bool showSeconds = DEFAULT_SHOW_SECONDS;

  u_char fiveMinuteMarkBrightness = DEFAULT_FIVE_MINUTE_MARK_BRIGHTNESS;
  u_char ledMaxBrightness = DEFAULT_LED_MAX_BRIGHTNESS;
  u_char lastOfTrailBrightness = DEFAULT_LAST_OF_TRAIL_BRIGHTNESS;

};

struct MyAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
};

struct RGBPixelData {
  uint8_t Red;
  uint8_t Green;
  uint8_t Blue;
};
