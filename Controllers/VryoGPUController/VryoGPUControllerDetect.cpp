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
#include <vector>
#include "DetectionManager.h"
#include "JsonUtils.h"
#include "LogManager.h"
#include "ResourceManager.h"
#include "RGBController_VryoGPU.h"
#include "SettingsManager.h"
#include "VryoGPUConfig.h"
#include "VryoGPUController.h"

static std::string VryoLower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return text;
}

static const VryoModelDef * VryoFindModel(const std::string& model_id)
{
    const unsigned int model_count = sizeof(YB_GPU_MODELS) / sizeof(YB_GPU_MODELS[0]);

    for(unsigned int model_idx = 0; model_idx < model_count; model_idx++)
    {
        if(VryoLower(model_id) == VryoLower(YB_GPU_MODELS[model_idx].id) || VryoLower(model_id) == VryoLower(YB_GPU_MODELS[model_idx].name))
        {
            return(&YB_GPU_MODELS[model_idx]);
        }
    }

    return(nullptr);
}

DetectedControllers DetectVryoGPUControllers()
{
    DetectedControllers             detected_controllers;
    json                            gpu_settings;
    std::string                     port_name;
    std::vector<const VryoModelDef*> models;

    const unsigned int model_count = sizeof(YB_GPU_MODELS) / sizeof(YB_GPU_MODELS[0]);

    /*-----------------------------------------------------*\
    | Optional settings block in OpenRGB.json:              |
    |   "VryoGPU": { "model": "nova-2060" }                 |
    |   "VryoGPU": { "models": "all" }                      |
    |   "VryoGPU": { "models": ["nova-2060", "halo-5060"] } |
    |   "VryoGPU": { "model": "nova-2060",                  |
    |                "serial_port": "COM5" }                |
    \*-----------------------------------------------------*/
    gpu_settings = ResourceManager::get()->GetSettingsManager()->GetSettings(YB_GPU_SETTINGS_KEY);

    if(gpu_settings.is_object())
    {
        port_name = JsonUtils::JsonGetString(gpu_settings, "serial_port");

        if(gpu_settings.contains("models"))
        {
            if(gpu_settings["models"].is_string())
            {
                if(VryoLower(gpu_settings["models"].get<std::string>()) == "all")
                {
                    for(unsigned int model_idx = 0; model_idx < model_count; model_idx++)
                    {
                        models.push_back(&YB_GPU_MODELS[model_idx]);
                    }
                }
                else
                {
                    const VryoModelDef * found = VryoFindModel(gpu_settings["models"].get<std::string>());

                    if(found != nullptr)
                    {
                        models.push_back(found);
                    }
                }
            }
            else if(gpu_settings["models"].is_array())
            {
                for(unsigned int entry_idx = 0; entry_idx < gpu_settings["models"].size(); entry_idx++)
                {
                    if(gpu_settings["models"][entry_idx].is_string())
                    {
                        std::string         model_id = gpu_settings["models"][entry_idx].get<std::string>();
                        const VryoModelDef * found   = VryoFindModel(model_id);

                        if(found != nullptr)
                        {
                            models.push_back(found);
                        }
                        else
                        {
                            LOG_WARNING("[%s] Unknown model \"%s\", skipping", YB_GPU_DETECTOR_NAME, model_id.c_str());
                        }
                    }
                }
            }
        }
        else if(gpu_settings.contains("model"))
        {
            std::string          model_id = JsonUtils::JsonGetString(gpu_settings, "model", YB_GPU_DEFAULT_MODEL);
            const VryoModelDef * found    = VryoFindModel(model_id);

            if(found != nullptr)
            {
                models.push_back(found);
            }
            else
            {
                LOG_WARNING("[%s] Unknown model \"%s\", using default", YB_GPU_DETECTOR_NAME, model_id.c_str());
            }
        }
    }

    /*-----------------------------------------------------*\
    | Nothing usable set: use the default model             |
    \*-----------------------------------------------------*/
    if(models.empty())
    {
        const VryoModelDef * found = VryoFindModel(YB_GPU_DEFAULT_MODEL);

        models.push_back((found != nullptr) ? found : &YB_GPU_MODELS[0]);
    }

    /*-----------------------------------------------------*\
    | A real board on a serial port is one card, so only    |
    | the first model is used when serial_port is set.      |
    | Without a port every listed model is a virtual device.|
    \*-----------------------------------------------------*/
    if(!port_name.empty() && models.size() > 1)
    {
        LOG_WARNING("[%s] serial_port set with several models, using only %s", YB_GPU_DETECTOR_NAME, models[0]->name);
        models.resize(1);
    }

    for(unsigned int model_idx = 0; model_idx < models.size(); model_idx++)
    {
        if(!models[model_idx]->has_rgb)
        {
            LOG_INFO("[%s] %s has no RGB, not listed", YB_GPU_DETECTOR_NAME, models[model_idx]->name);
            continue;
        }

        VryoGPUController * controller = port_name.empty()
                                       ? new VryoGPUController()
                                       : new VryoGPUController(port_name);

        if(controller->IsPresent())
        {
            detected_controllers.push_back(new RGBController_VryoGPU(controller, *models[model_idx]));

            LOG_INFO("[%s] Detected %s (%s)", YB_GPU_DETECTOR_NAME, models[model_idx]->name, controller->GetLocation().c_str());
        }
        else
        {
            delete controller;
        }
    }

    return(detected_controllers);
}

REGISTER_DETECTOR(YB_GPU_DETECTOR_NAME, DetectVryoGPUControllers);
