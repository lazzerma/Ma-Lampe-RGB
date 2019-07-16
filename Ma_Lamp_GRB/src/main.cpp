#include <BluetoothSerial.h>
#include <Arduino.h>
#include <NeoPixelAnimator.h>
#include <NeoPixelBus.h>

const uint8_t PIXELNUM = 24;         // this example assumes 4 strips, making it smaller will cause a failure
const uint8_t PIXELPIN = 5;          // make sure to set this to the correct pin, ignored for Esp8266
const uint8_t AnimationChannels = 1; // we only need one as all the pixels are animated at once

#define colorSaturation 128

// three element strips, in different order and speeds
NeoPixelBus<NeoGrbFeature, NeoEsp32BitBang800KbpsMethod> strip(PIXELNUM, PIXELPIN);
NeoPixelAnimator animations(AnimationChannels); // NeoPixel animation management object
// one entry per pixel to match the animation timing manager


struct MyAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
};

MyAnimationState animationState[AnimationChannels];

boolean fadeToColor = true; // general purpose variable used to store effect state

BluetoothSerial SerialBT; //define SerialBT, referance to library
uint8_t dataS[3];         //uint = byte

void rainbow(uint8_t wait);
RgbColor Wheel(uint8_t WheelPos);
void stopRainbow();
void rgbManager();
void FadeInFadeOutRinseRepeat(float luminance);

bool rainbowFlag = true;
RgbColor color;
uint8_t pos;

void setup()
{
    Serial.begin(115200); //speed of serial
    SerialBT.begin("Val lampe");
    pinMode(0,INPUT);
    strip.Begin();
    rainbowFlag = true;
 
}

void loop()
{
    rgbManager();

    if (rainbowFlag)
    {
        rainbow(10); //speed
    }

    while (!digitalRead(0))
    {

        if (animations.IsAnimating())
        {
            // the normal loop just needs these two to run the active animations
            animations.UpdateAnimations();
            strip.Show();
        }
        else
        {
            FadeInFadeOutRinseRepeat(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
        }
    }

    //set the brightness of pin 5:
}
void stopRainbow()
{
    RgbColor myColor(0, 0, 0); //define data RGB
    rainbowFlag = false;
    for (int i = 0; i < PIXELNUM; i++)
    {
        strip.SetPixelColor(i, myColor);
    }
    strip.Show();
    delay(5);
}

void rgbManager()
{
    if (SerialBT.available())
    {
        stopRainbow();

        for (int i = 0; i < 3; i++)
        {
            dataS[i] = SerialBT.read();
            Serial.println(dataS[i]);
        }
        if (1)
        {
            RgbColor myColor(dataS[0], dataS[1], dataS[2]); //define data RGB

            for (int i = 0; i < PIXELNUM; i++)
            {
                strip.SetPixelColor(i, myColor);
                strip.Show();
                delay(5);
            }
        }
    }

    if (Serial.available() > 4)
    {

        String command = Serial.readString();
        if (command == "rainbow\n")
        {
            Serial.println("rainbow enabled");
            rainbowFlag = true;
        }
        else
        {
            Serial.println(command);
        }
    }
    else if (Serial.available() < 4 && Serial.available() > 1)
    {
        stopRainbow();

        for (int i = 0; i < 3; i++)
        {
            dataS[i] = Serial.read();
            Serial.println(dataS[i]);
        }
        if (1)
        {
            RgbColor myColor(dataS[0], dataS[1], dataS[2]); //define data RGB

            for (int i = 0; i < PIXELNUM; i++)
            {
                strip.SetPixelColor(i, myColor);
                strip.Show();
                delay(5);
            }
        }
    }
}

void rainbow(uint8_t wait)
{
    for (uint16_t j = 0; j < 256; j++) // complete 5 cycles around the color wheel
    {

        for (uint16_t i = 0; i < PIXELNUM; i++)
        {
            // generate a value between 0~255 according to the position of the pixel
            // along the strip
            pos = ((i * 256 / PIXELNUM) + j) & 0xFF;
            // calculate the color for the ith pixel
            color = Wheel(pos);
            // set the color of the ith pixel
            strip.SetPixelColor(i, color);
        }
        strip.Show();
        // strip.Darken(100);
        delay(wait);
    }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
RgbColor Wheel(uint8_t WheelPos)
{
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85)
    {
        return RgbColor(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    else if (WheelPos < 170)
    {
        WheelPos -= 85;
        return RgbColor(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    else
    {
        WheelPos -= 170;
        return RgbColor(WheelPos * 3, 255 - WheelPos * 3, 0);
    }
}

// simple blend function
void BlendAnimUpdate(const AnimationParam &param)
{
    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation
    RgbColor updatedColor = RgbColor::LinearBlend(
        animationState[param.index].StartingColor,
        animationState[param.index].EndingColor,
        param.progress);

    // apply the color to the strip
    for (uint16_t pixel = 0; pixel < PIXELNUM; pixel++)
    {
        strip.SetPixelColor(pixel, updatedColor);
    }
}

void FadeInFadeOutRinseRepeat(float luminance)
{
    if (fadeToColor)
    {
        // Fade upto a random color
        // we use HslColor object as it allows us to easily pick a hue
        // with the same saturation and luminance so the colors picked
        // will have similiar overall brightness
        RgbColor target = HslColor(random(360) / 360.0f, 1.0f, luminance);
        uint16_t time = random(800, 2000);

        animationState[0].StartingColor = strip.GetPixelColor(0);
        animationState[0].EndingColor = target;

        animations.StartAnimation(0, time, BlendAnimUpdate);
    }
    else
    {
        // fade to black
        uint16_t time = random(600, 700);

        animationState[0].StartingColor = strip.GetPixelColor(0);
        animationState[0].EndingColor = RgbColor(0);

        animations.StartAnimation(0, time, BlendAnimUpdate);
    }

    // toggle to the next effect state
    fadeToColor = !fadeToColor;
}