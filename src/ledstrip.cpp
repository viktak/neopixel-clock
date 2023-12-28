#include <Arduino.h>
#include <TimeLib.h>

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

#include "settings.h"
#include "TimeChangeRules.h"

//? Pick one: WS2812B, SK6812
#define WS2812B

#ifdef WS2812B
struct RotatingPanelAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
    uint16_t IndexPixel; // which pixel this animation is effecting
};
#endif

#ifdef SK6812
struct RotatingPanelAnimationState
{
    RgbwColor StartingColor;
    RgbwColor EndingColor;
    uint16_t IndexPixel; // which pixel this animation is effecting
};
#endif

#ifdef WS2812B
struct ChaserAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
    uint16_t IndexPixel; // which pixel this animation is effecting
};
#endif

#ifdef SK6812
struct ChaserAnimationState
{
    RgbwColor StartingColor;
    RgbwColor EndingColor;
    uint16_t IndexPixel; // which pixel this animation is effecting
};
#endif

#ifdef WS2812B
struct SlowPanelsAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
};
#endif

#ifdef SK6812
struct SlowPanelsAnimationState
{
    RgbwColor StartingColor;
    RgbwColor EndingColor;
};
#endif

#ifdef WS2812B
struct FastSegmentsAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
    uint16_t Segment;
    bool dir;
};
#endif

#ifdef SK6812
struct FastSegmentsAnimationState
{
    RgbwColor StartingColor;
    RgbwColor EndingColor;
    uint16_t Segment;
    bool dir;
};
#endif

#ifdef WS2812B
struct RGBPixelData
{
    uint8_t Red;
    uint8_t Green;
    uint8_t Blue;
};
#endif

#ifdef SK6812
struct RGBPixelData
{
    uint8_t Red;
    uint8_t Green;
    uint8_t Blue;
    uint8_t White;
};
#endif

const uint16_t NUMBER_OF_LEDS = 60;
uint8_t oldSecs = -1;

NeoGamma<NeoGammaTableMethod> colorGamma; // for any fade animations, best to correct gamma

#ifdef WS2812B
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(NUMBER_OF_LEDS);
#endif
#ifdef SK6812
NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(NUMBER_OF_LEDS);
#endif

RGBPixelData buf[NUMBER_OF_LEDS];

void DisplayTime()
{
    TimeChangeRule *tcr; // Pointer to the time change rule

    //  Clear pixels
    for (size_t i = 0; i < NUMBER_OF_LEDS; i++)
    {
        buf[i].Red = 0;
        buf[i].Green = 0;
        buf[i].Blue = 0;
    }

    //  Calculate time
    time_t localTime = timechangerules::timezones[settings::timeZone]->toLocal(now(), &tcr);

    //  Display time markers
    int8_t myHours = hourFormat12(localTime);
    int8_t myMinutes = minute(localTime);
    int8_t mySeconds = second(localTime);

    // myHours = hh;
    // myMinutes = mm;
    // mySeconds = ss;

    myHours %= 12;

    myHours *= 5;
    myHours += myMinutes / 12;

    //  Display time and trail
    int8_t diff = (settings::ledMaxBrightness - settings::lastOfTrailBrightness) / (settings::trailLength);

    for (int8_t i = 0; i < settings::trailLength; i++)
    {
        int8_t offset = 0;

        if (myHours - i < 0)
            offset = NUMBER_OF_LEDS;
        else
            offset = 0;
        buf[((myHours - i + offset > 59 ? 0 : myHours - i + offset)) % NUMBER_OF_LEDS].Red = settings::ledMaxBrightness - diff * i;

        // Serial.printf("i: %i\thours: %i\tposition: %i\t\r\n", i, myHours, myHours - i + offset);

        if (myMinutes - i < 0)
            offset = NUMBER_OF_LEDS;
        else
            offset = 0;
        buf[myMinutes - i + offset].Blue = settings::ledMaxBrightness - diff * i;

        // Serial.printf("i: %i\tminutes: %i\tposition: %i\t\r\n", i, myMinutes, myMinutes - i + offset);

        if (settings::showSeconds)
        {
            if (mySeconds - i < 0)
                offset = NUMBER_OF_LEDS;
            else
                offset = 0;
            buf[mySeconds - i + offset].Green = settings::ledMaxBrightness - diff * i;
        }
        // Serial.printf("i: %i\tseconds: %i\tposition: %i\t\r\n\r\n", i, mySeconds, mySeconds - i + offset);
    }

    // Serial.println();

    //  Display 5 minute marks
    if (settings::showFiveMinuteMarks)
    {
        for (size_t i = 0; i < 12; i++)
        {

            bool isRedLit = buf[i * 5].Red;
            bool isGreenLit = buf[i * 5].Green;
            bool isBlueLit = buf[i * 5].Blue;

            buf[i * 5].Red = isRedLit ? buf[i * 5].Red : settings::fiveMinuteMarkBrightness;
            buf[i * 5].Green = isGreenLit ? buf[i * 5].Green : settings::fiveMinuteMarkBrightness;
            buf[i * 5].Blue = isBlueLit ? buf[i * 5].Blue : settings::fiveMinuteMarkBrightness;
        }
    }

    //  If clock is reversed, it needs its pixels shifted by one,
    //  so that the 12 hour mark stays put.
    if (settings::reverseClockDirection)
    {
        RGBPixelData pd;
        pd.Red = buf[0].Red;
        pd.Green = buf[0].Green;
        pd.Blue = buf[0].Blue;
        // //Serial.printf("%i\t%i\t%i\r\n", pd.Red, pd.Green, pd.Blue);
        for (size_t i = 1; i < NUMBER_OF_LEDS; i++)
        {
            buf[i - 1].Red = buf[i].Red;
            buf[i - 1].Green = buf[i].Green;
            buf[i - 1].Blue = buf[i].Blue;
        }
        buf[NUMBER_OF_LEDS - 1].Red = pd.Red;
        buf[NUMBER_OF_LEDS - 1].Green = pd.Green;
        buf[NUMBER_OF_LEDS - 1].Blue = pd.Blue;
    }

    //  Move buffer to strip
    for (size_t i = 0; i < NUMBER_OF_LEDS; i++)
    {
        if (settings::reverseClockDirection)
            strip.SetPixelColor(NUMBER_OF_LEDS - i - 1, colorGamma.Correct(RgbColor(buf[i].Red, buf[i].Green, buf[i].Blue)));
        else
            strip.SetPixelColor(i, colorGamma.Correct(RgbColor(buf[i].Red, buf[i].Green, buf[i].Blue)));
    }
    strip.Show();
}

void setupLedStrip()
{
    strip.Begin();
    for (size_t i = 0; i < NUMBER_OF_LEDS; i++)
    {
        strip.ClearTo(RgbColor(0));
    }
    strip.Show();
}

void loopLedStrip()
{
    uint8_t currentSecond = second(now());
    if (oldSecs != currentSecond)
    {
        DisplayTime();
        oldSecs = currentSecond;
    }
}
