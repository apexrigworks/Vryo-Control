/*---------------------------------------------------------*\
| VryoGPUControllerDetect.cpp                          |
|                                                           |
|   Detector for the custom GPU lighting board              |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <algorithm>
#include <cctype>
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
    std::string         model_id    = YB_GPU_DEFAULT_MODEL;

    /*-----------------------------------------------------*\
    | Optional settings block in OpenRGB.json:              |
    |   "VryoGPU": { "serial_port": "COM5" }           |
    \*-----------------------------------------------------*/
    gpu_settings = ResourceManager::get()->GetSettingsManager()->GetSettings(YB_GPU_SETTINGS_KEY);

    if(gpu_settings.is_object())
    {
        port_name = JsonUtils::JsonGetString(gpu_settings, "serial_port");
        model_id  = JsonUtils::JsonGetString(gpu_settings, "model", YB_GPU_DEFAULT_MODEL);
    }

    /*-----------------------------------------------------*\
    | Find the model, by id or by full name (any case).     |
    | Unknown value: log it and use the default model.      |
    \*-----------------------------------------------------*/
    const unsigned int  model_count = sizeof(YB_GPU_MODELS) / sizeof(YB_GPU_MODELS[0]);
    const VryoModelDef* model       = nullptr;

    auto lower = [](std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c){ return (char)std::tolower(c); });
        return text;
    };

    for(unsigned int model_idx = 0; model_idx < model_count && model == nullptr; model_idx++)
    {
        if(lower(model_id) == lower(YB_GPU_MODELS[model_idx].id) || lower(model_id) == lower(YB_GPU_MODELS[model_idx].name))
        {
            model = &YB_GPU_MODELS[model_idx];
        }
    }

    if(model == nullptr)
    {
        LOG_WARNING("[%s] Unknown model \"%s\", using default", YB_GPU_DETECTOR_NAME, model_id.c_str());

        for(unsigned int model_idx = 0; model_idx < model_count && model == nullptr; model_idx++)
        {
            if(lower(YB_GPU_DEFAULT_MODEL) == lower(YB_GPU_MODELS[model_idx].id))
            {
                model = &YB_GPU_MODELS[model_idx];
            }
        }

        if(model == nullptr)
        {
            model = &YB_GPU_MODELS[0];
        }
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
        detected_controllers.push_back(new RGBController_VryoGPU(controller, *model));

        LOG_INFO("[%s] Detected %s (%s)", YB_GPU_DETECTOR_NAME, model->name, controller->GetLocation().c_str());
    }
    else
    {
        delete controller;
    }

    return(detected_controllers);
}

REGISTER_DETECTOR(YB_GPU_DETECTOR_NAME, DetectVryoGPUControllers);
