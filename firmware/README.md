# Vryo GPU lighting board firmware

A small board (Raspberry Pi Pico / Seeed XIAO RP2040 recommended, ESP32 also works)
that connects to the PC over USB and drives one ARGB data line on the card.

## What it does
Vryo Control sends a frame over USB serial whenever the colours change:
`0xAA 0x55, LED count (2 bytes, low first), R G B for every LED, XOR checksum`.
The board checks the checksum and shows the colours. Bad or half-received frames are
ignored. `vryo_protocol.h` is the parser; `tests/test_protocol.cpp` checks it against
the packet format the PC side sends (`g++ -std=c++17 -I../vryo_gpu_controller test_protocol.cpp`).

## Set it up
1. Install the Arduino IDE. For a Pico, add the "Raspberry Pi Pico/RP2040" board package
   (Earle Philhower). Install the **Adafruit NeoPixel** library.
2. Open `vryo_gpu_controller/vryo_gpu_controller.ino`.
3. Set `LED_COUNT` to the total LEDs on the card. It must equal the sum of the zone
   LED counts for that model in `Controllers/VryoGPUController/VryoGPUConfig.h`.
4. Upload, then plug the board into the PC.
5. In `OpenRGB.json`, add the board's COM port (see Device Manager, Ports):
   `"VryoGPU": { "model": "glacier-1660-super", "serial_port": "COM5" }`

## Wiring and power (read before connecting LEDs)
- **A PCIe slot has no 5V rail** (only 3.3V and 12V), so the card cannot supply 5V ARGB
  LEDs by itself. Power the LEDs from a 5V source such as a SATA/Molex 5V line, and
  connect that ground to the board's ground.
- A motherboard USB 2.0 header gives at most about 0.5 A in total. That is only enough
  for roughly 20 LEDs at moderate brightness. Each LED can draw up to 60 mA at full white
  (the 36-LED placeholder layout is about 2.2 A at full white). `MAX_BRIGHTNESS` in the sketch caps this.
- Data line: board pin (`LED_PIN`, default GP2) to the first LED's DIN through a 330 ohm
  resistor. A 3.3V board driving 5V LEDs often works, but a 74AHCT125 level shifter makes it reliable.
- Put a 1000 uF capacitor across the LED 5V and ground near the first LED.
- Common ground between the board, the LED supply and the LEDs.

## Not done yet
- The board does not tell the PC which card model it is. The model is chosen in
  `OpenRGB.json` for now.
- One data line only. Separate fan and strip outputs would need a small change.
