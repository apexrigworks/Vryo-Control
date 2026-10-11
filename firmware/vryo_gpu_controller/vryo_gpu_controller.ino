/*---------------------------------------------------------*\
| vryo_gpu_controller.ino                                   |
|                                                           |
|   Lighting board for Vryo Control. Reads frames from the  |
|   PC over USB serial and drives one ARGB (WS2812 type)    |
|   data line.                                              |
|                                                           |
|   Works on RP2040 (Raspberry Pi Pico, Seeed XIAO RP2040), |
|   ESP32 and Arduino boards with USB serial.               |
|   Library needed: "Adafruit NeoPixel" (Library Manager).  |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <Adafruit_NeoPixel.h>
#include "vryo_protocol.h"

/*---------------------------------------------------------*\
| SETTINGS. Change these to match your card.                |
\*---------------------------------------------------------*/

// Pin the LED data line is connected to (GP2 on a Pico).
#define LED_PIN             2

// Total LEDs on the data line. Must equal the sum of the zone
// LED counts for this card in VryoGPUConfig.h on the PC side.
#define LED_COUNT           44

// 0 to 255. Caps total current. Keep this LOW while the board is
// powered from a USB port (see firmware/README.md).
#define MAX_BRIGHTNESS      96

// If a frame stops half-way, forget it after this long (ms).
#define BYTE_GAP_TIMEOUT_MS 100

// Strip colour order. Most ARGB fans are GRB; if red and green
// are swapped, change NEO_GRB to NEO_RGB.
#define COLOR_ORDER         NEO_GRB

/*---------------------------------------------------------*/

Adafruit_NeoPixel           strip(LED_COUNT, LED_PIN, COLOR_ORDER + NEO_KHZ800);
VryoFrameParser<LED_COUNT>  parser;
unsigned long               last_byte_ms = 0;

void setup()
{
    Serial.begin(115200);       // ignored by USB-native boards, harmless

    strip.begin();
    strip.setBrightness(MAX_BRIGHTNESS);
    strip.clear();
    strip.show();
}

void loop()
{
    bool got_frame = false;

    while(Serial.available() > 0)
    {
        last_byte_ms = millis();

        if(parser.Feed((uint8_t)Serial.read()))
        {
            got_frame = true;       // keep reading, only show the newest
        }
    }

    // Drop a half-received frame if the PC went quiet mid-way
    if((millis() - last_byte_ms) > BYTE_GAP_TIMEOUT_MS)
    {
        parser.Reset();
    }

    if(got_frame)
    {
        const uint16_t   count  = parser.StoredCount();
        const uint8_t *  colors = parser.Colors();

        for(uint16_t led = 0; led < count; led++)
        {
            strip.setPixelColor(led, strip.Color(colors[led * 3], colors[led * 3 + 1], colors[led * 3 + 2]));
        }

        strip.show();
    }
}
