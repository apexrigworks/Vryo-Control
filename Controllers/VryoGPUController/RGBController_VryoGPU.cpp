/*---------------------------------------------------------*\
| RGBController_VryoGPU.cpp                            |
|                                                           |
|   RGBController for the custom GPU lighting board         |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <string>
#include "RGBController_VryoGPU.h"
#include "VryoGPUConfig.h"

/**------------------------------------------------------------------*\
    @name Vryo GPU
    @category GPU
    @type Serial
    @save :x:
    @direct :white_check_mark:
    @effects :x:
    @detectors DetectVryoGPUControllers
    @comment Custom GPU lighting board. Virtual mode when no serial port is configured.
\*-------------------------------------------------------------------*/

RGBController_VryoGPU::RGBController_VryoGPU(VryoGPUController* controller_ptr)
{
    controller          = controller_ptr;

    /*-----------------------------------------------------*\
    | This is where the name you chose is applied           |
    \*-----------------------------------------------------*/
    name                = YB_GPU_DEVICE_NAME;
    vendor              = YB_GPU_VENDOR;
    description         = YB_GPU_DESCRIPTION;
    version             = YB_GPU_VERSION;
    type                = DEVICE_TYPE_GPU;
    location            = controller->GetLocation();
    serial              = controller->GetSerial();

    /*-----------------------------------------------------*\
    | One mode for now: Direct, per-LED colour.             |
    | Hardware effects can be added later as more modes.    |
    \*-----------------------------------------------------*/
    mode Direct;
    Direct.name         = "Direct";
    Direct.value        = 0;
    Direct.flags        = MODE_FLAG_HAS_PER_LED_COLOR;
    Direct.color_mode   = MODE_COLORS_PER_LED;
    modes.push_back(Direct);

    SetupZones();
}

RGBController_VryoGPU::~RGBController_VryoGPU()
{
    Shutdown();

    controller->SetBlack((unsigned int)colors.size());

    delete controller;
}

void RGBController_VryoGPU::SetupZones()
{
    zones.clear();
    leds.clear();
    colors.clear();

    const unsigned int zone_count = sizeof(YB_GPU_ZONES) / sizeof(YB_GPU_ZONES[0]);

    for(unsigned int zone_idx = 0; zone_idx < zone_count; zone_idx++)
    {
        zone new_zone;

        new_zone.name           = YB_GPU_ZONES[zone_idx].name;
        new_zone.type           = ZONE_TYPE_LINEAR;
        new_zone.leds_min       = YB_GPU_ZONES[zone_idx].leds;
        new_zone.leds_max       = YB_GPU_ZONES[zone_idx].leds;
        new_zone.leds_count     = YB_GPU_ZONES[zone_idx].leds;

        zones.push_back(new_zone);

        for(unsigned int led_idx = 0; led_idx < YB_GPU_ZONES[zone_idx].leds; led_idx++)
        {
            led new_led;
            new_led.name = std::string(YB_GPU_ZONES[zone_idx].name) + " LED " + std::to_string(led_idx + 1);

            leds.push_back(new_led);
        }
    }

    SetupColors();
}

void RGBController_VryoGPU::DeviceConfigureZone(int)
{
    /*-----------------------------------------------------*\
    | Zones are fixed, nothing to configure                 |
    \*-----------------------------------------------------*/
}

void RGBController_VryoGPU::DeviceUpdateLEDs()
{
    controller->SetLEDs(colors);
}

void RGBController_VryoGPU::DeviceUpdateZoneLEDs(int)
{
    DeviceUpdateLEDs();
}

void RGBController_VryoGPU::DeviceUpdateSingleLED(int)
{
    DeviceUpdateLEDs();
}

void RGBController_VryoGPU::DeviceUpdateMode()
{
    /*-----------------------------------------------------*\
    | Direct mode only, nothing to send                     |
    \*-----------------------------------------------------*/
}
