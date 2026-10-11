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
#define YB_GPU_VENDOR           "Apex Rigworks"
#define YB_GPU_VERSION          "0.1"

/*---------------------------------------------------------*\
| Product models. Device names are series + GPU only (no    |
| brand name in them), like iCUE showing "K70" not          |
| "Corsair K70". The vendor field above carries the brand.  |
|                                                           |
|   id      short id you can put in OpenRGB.json            |
|   name    exactly what Vryo Control shows for the card    |
\*---------------------------------------------------------*/
struct VryoModelDef
{
    const char*     id;
    const char*     name;
};

static const VryoModelDef YB_GPU_MODELS[] =
{
    { "frost-1650",         "Frost GTX 1650 4GB"            },
    { "frost-1650-super",   "Frost GTX 1650 Super 4GB"      },
    { "glacier-1660",       "Glacier GTX 1660 6GB"          },
    { "glacier-1660-super", "Glacier GTX 1660 Super 6GB"    },
    { "glacier-1660-ti",    "Glacier GTX 1660 Ti 6GB"       },
    { "nova-2060",          "Nova RTX 2060 6GB"             },
    { "nova-2060-super",    "Nova RTX 2060 Super 8GB"       },
    { "nova-3050",          "Nova RTX 3050 8GB"             },
    { "lumen-3060",         "Lumen RTX 3060 12GB"           },
    { "halo-5050",          "Halo RTX 5050 8GB"             },
    { "halo-5060",          "Halo RTX 5060 8GB"             },
};

/*---------------------------------------------------------*\
| Model used when OpenRGB.json does not name one            |
\*---------------------------------------------------------*/
#define YB_GPU_DEFAULT_MODEL    "glacier-1660-super"

/*---------------------------------------------------------*\
| The string shown on the "Supported Devices" settings page |
| and used as the key in OpenRGB.json to enable/disable the |
| detector. Keep it unique.                                 |
\*---------------------------------------------------------*/
#define YB_GPU_DETECTOR_NAME    "Vryo GPU"

/*---------------------------------------------------------*\
| Settings key in OpenRGB.json for this device, e.g.        |
|   "VryoGPU": { "model": "nova-2060",                     |
|               "serial_port": "COM5" }                    |
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
