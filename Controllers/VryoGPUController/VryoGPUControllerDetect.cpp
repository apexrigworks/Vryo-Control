/*---------------------------------------------------------*\
| VryoGPUControllerDetect.cpp                          |
|                                                           |
|   Detector for the custom GPU lighting board              |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <string>
#include "DetectionManager.h"
#include "JsonUtils.h"
#include "LogManager.h"
#include "ResourceManager.h"
#include "RGBController_VryoGPU.h"
#include "SettingsManager.h"
#include "VryoGPUConfig.h"
#include "VryoGPUController.h"

DetectedControllers DetectVryoGPUControllers()
{
    DetectedControllers detected_controllers;
    json                gpu_settings;
    std::string         port_name;

    /*-----------------------------------------------------*\
    | Optional settings block in OpenRGB.json:              |
    |   "VryoGPU": { "serial_port": "COM5" }           |
    \*-----------------------------------------------------*/
    gpu_settings = ResourceManager::get()->GetSettingsManager()->GetSettings(YB_GPU_SETTINGS_KEY);

    if(gpu_settings.is_object())
    {
        port_name = JsonUtils::JsonGetString(gpu_settings, "serial_port");
    }

    /*-----------------------------------------------------*\
    | No port configured: virtual device (no hardware).     |
    | Port configured: real device on that port.            |
    \*-----------------------------------------------------*/
    VryoGPUController* controller = port_name.empty()
                                       ? new VryoGPUController()
                                       : new VryoGPUController(port_name);

    if(controller->IsPresent())
    {
        detected_controllers.push_back(new RGBController_VryoGPU(controller));

        LOG_INFO("[%s] Detected %s (%s)", YB_GPU_DETECTOR_NAME, YB_GPU_DEVICE_NAME, controller->GetLocation().c_str());
    }
    else
    {
        delete controller;
    }

    return(detected_controllers);
}

REGISTER_DETECTOR(YB_GPU_DETECTOR_NAME, DetectVryoGPUControllers);
