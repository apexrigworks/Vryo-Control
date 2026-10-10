/*---------------------------------------------------------*\
| RGBController_VryoGPU.h                              |
|                                                           |
|   RGBController for the custom GPU lighting board         |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include "RGBController.h"
#include "VryoGPUController.h"

class RGBController_VryoGPU : public RGBController
{
public:
    RGBController_VryoGPU(VryoGPUController* controller_ptr);
    ~RGBController_VryoGPU();

    void        SetupZones();

    void        DeviceConfigureZone(int zone_idx);

    void        DeviceUpdateLEDs();
    void        DeviceUpdateZoneLEDs(int zone);
    void        DeviceUpdateSingleLED(int led);

    void        DeviceUpdateMode();

private:
    VryoGPUController*     controller;
};
