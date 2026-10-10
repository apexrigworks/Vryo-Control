/*---------------------------------------------------------*\
| VryoGPUConfig.h                                      |
|                                                           |
|   The one file to edit: your device name, branding and    |
|   LED layout. Everything OpenRGB shows for your GPU comes |
|   from here.                                              |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

/*---------------------------------------------------------*\
| Branding. This is exactly what appears in OpenRGB's       |
| device list and in its device info panel.                 |
| (Windows Device Manager / GPU-Z still show the NVIDIA     |
| name, that comes from the card's firmware, not OpenRGB.)  |
\*---------------------------------------------------------*/
#define YB_GPU_DEVICE_NAME      "Vryo GPU"          // CHANGE ME
#define YB_GPU_VENDOR           "Vryo"               // CHANGE ME
#define YB_GPU_DESCRIPTION      "Vryo Graphics Card"
#define YB_GPU_VERSION          "0.1"

/*---------------------------------------------------------*\
| The string shown on the "Supported Devices" settings page |
| and used as the key in OpenRGB.json to enable/disable the |
| detector. Keep it unique.                                 |
\*---------------------------------------------------------*/
#define YB_GPU_DETECTOR_NAME    "Vryo GPU"

/*---------------------------------------------------------*\
| Settings key in OpenRGB.json for this device, e.g.        |
|   "VryoGPU": { "serial_port": "COM5" }               |
| If no serial_port is given the device runs in VIRTUAL     |
| mode: it shows up in OpenRGB and accepts colours, but     |
| sends them nowhere. Useful for testing with no hardware.  |
\*---------------------------------------------------------*/
#define YB_GPU_SETTINGS_KEY     "VryoGPU"

/*---------------------------------------------------------*\
| LED layout, in the order the LEDs are wired. These are    |
| PLACEHOLDERS: change the zone names and LED counts to     |
| match your shroud. Each entry becomes one zone in OpenRGB.|
\*---------------------------------------------------------*/
struct VryoZoneDef
{
    const char*     name;
    unsigned int    leds;
};

static const VryoZoneDef YB_GPU_ZONES[] =
{
    { "Fan 1",          12 },
    { "Fan 2",          12 },
    { "Shroud Strip",   20 },
};

/*---------------------------------------------------------*\
| Serial link to your controller board (RP2040 / Arduino /  |
| ESP32 etc). See README.md for the packet format.          |
\*---------------------------------------------------------*/
#define YB_GPU_SERIAL_BAUD      115200
