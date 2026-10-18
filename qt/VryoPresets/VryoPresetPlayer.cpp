/*---------------------------------------------------------*\
| VryoPresetPlayer.cpp                                      |
|                                                           |
|   Plays one preset on a set of RGB devices.               |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "VryoPresetPlayer.h"

#include <algorithm>
#include <cstddef>

#include "ResourceManager.h"

namespace
{
    /*-----------------------------------------------------*\
    | About 25 frames a second: smooth, and gentle on the   |
    | serial / USB link to the card                         |
    \*-----------------------------------------------------*/
    const int VRYO_PLAYER_INTERVAL_MS = 40;

    /*-----------------------------------------------------*\
    | Devices can disappear on a rescan. Check against the  |
    | live list before touching one.                        |
    \*-----------------------------------------------------*/
    bool ControllerStillExists(RGBController * controller)
    {
        std::vector<RGBController *>& list = ResourceManager::get()->GetRGBControllers();

        return(std::find(list.begin(), list.end(), controller) != list.end());
    }
}

VryoPresetPlayer::VryoPresetPlayer(QObject * parent) :
    QObject(parent)
{
    timer.setTimerType(Qt::PreciseTimer);
    timer.setInterval(VRYO_PLAYER_INTERVAL_MS);

    connect(&timer, &QTimer::timeout, this, &VryoPresetPlayer::Tick);
}

VryoPresetPlayer::~VryoPresetPlayer()
{
    /*-----------------------------------------------------*\
    | No restore here: the devices may already be gone when |
    | the window is closing                                 |
    \*-----------------------------------------------------*/
    timer.stop();
}

bool VryoPresetPlayer::IsPlaying() const
{
    return(playing);
}

QString VryoPresetPlayer::PlayingId() const
{
    if(playing && (current != nullptr))
    {
        return(current->id);
    }

    return(QString());
}

std::vector<VryoLedInfo> VryoPresetPlayer::BuildInfos(RGBController * controller)
{
    const unsigned int          led_count   = controller->GetLEDCount();
    const unsigned int          zone_count  = controller->GetZoneCount();
    std::vector<VryoLedInfo>    infos(led_count);

    /*-----------------------------------------------------*\
    | Start with every LED on one long strip                |
    \*-----------------------------------------------------*/
    for(unsigned int led_idx = 0; led_idx < led_count; led_idx++)
    {
        VryoLedInfo& info   = infos[led_idx];

        info.index          = led_idx;
        info.count          = led_count;
        info.position       = (led_count > 1u) ? ((double)led_idx / (double)(led_count - 1u)) : 0.5;
        info.zone           = 0;
        info.zone_count     = (zone_count > 1u) ? zone_count : 1u;
        info.zone_index     = led_idx;
        info.zone_leds      = led_count;
        info.zone_position  = info.position;
    }

    /*-----------------------------------------------------*\
    | Then fill in where each zone starts and how long it is|
    \*-----------------------------------------------------*/
    for(unsigned int zone_idx = 0; zone_idx < zone_count; zone_idx++)
    {
        const unsigned int start    = controller->GetZoneStartIndex(zone_idx);
        const unsigned int length   = controller->GetZoneLEDsCount(zone_idx);

        for(unsigned int zone_led = 0; zone_led < length; zone_led++)
        {
            const unsigned int led_idx = start + zone_led;

            if(led_idx < led_count)
            {
                VryoLedInfo& info   = infos[led_idx];

                info.zone           = zone_idx;
                info.zone_index     = zone_led;
                info.zone_leds      = length;
                info.zone_position  = (length > 1u) ? ((double)zone_led / (double)(length - 1u)) : 0.5;
            }
        }
    }

    return(infos);
}

bool VryoPresetPlayer::Play(const VryoPreset * preset, const std::vector<RGBController *>& controllers)
{
    Stop();

    if((preset == nullptr) || !preset->render)
    {
        return(false);
    }

    for(size_t controller_idx = 0; controller_idx < controllers.size(); controller_idx++)
    {
        RGBController * controller = controllers[controller_idx];

        if((controller == nullptr) || controller->GetHidden() || (controller->GetLEDCount() == 0))
        {
            continue;
        }

        /*-------------------------------------------------*\
        | Remember how the device was                       |
        \*-------------------------------------------------*/
        Target target;

        target.controller       = controller;
        target.previous_mode    = controller->GetActiveMode();

        const unsigned int led_count = controller->GetLEDCount();

        target.previous_colours.resize(led_count);

        for(unsigned int led_idx = 0; led_idx < led_count; led_idx++)
        {
            target.previous_colours[led_idx] = controller->GetColor(led_idx);
        }

        /*-------------------------------------------------*\
        | Switch to a per-LED mode (Direct or Custom). A    |
        | device without one cannot show an animation, so   |
        | put it back and leave it out.                     |
        \*-------------------------------------------------*/
        controller->SetCustomMode();

        const int mode = controller->GetActiveMode();

        if((mode < 0) || (controller->GetModeColorMode((unsigned int)mode) != (unsigned int)MODE_COLORS_PER_LED))
        {
            if((mode != target.previous_mode) && (target.previous_mode >= 0))
            {
                controller->SetActiveMode(target.previous_mode);
                controller->UpdateMode();
            }

            continue;
        }

        target.infos = BuildInfos(controller);

        targets.push_back(target);
    }

    if(targets.empty())
    {
        return(false);
    }

    current     = preset;
    playing     = true;

    clock.start();
    timer.start();

    emit Started(preset->id);

    /*-----------------------------------------------------*\
    | Draw the first frame straight away                    |
    \*-----------------------------------------------------*/
    Tick();

    return(true);
}

void VryoPresetPlayer::Stop()
{
    if(playing)
    {
        EndPlayback(true);
    }
}

void VryoPresetPlayer::Restore(const Target& target)
{
    RGBController * controller = target.controller;

    if(target.previous_mode >= 0)
    {
        controller->SetActiveMode(target.previous_mode);
    }

    const unsigned int led_count    = controller->GetLEDCount();
    const unsigned int saved_count  = (unsigned int)target.previous_colours.size();
    const unsigned int count        = (led_count < saved_count) ? led_count : saved_count;

    for(unsigned int led_idx = 0; led_idx < count; led_idx++)
    {
        controller->SetColor(led_idx, target.previous_colours[led_idx]);
    }

    controller->UpdateMode();
    controller->UpdateLEDs();
}

void VryoPresetPlayer::EndPlayback(bool restore)
{
    timer.stop();

    playing = false;

    /*-----------------------------------------------------*\
    | Take the targets out first so nothing can reach them  |
    | again while the devices are being restored            |
    \*-----------------------------------------------------*/
    std::vector<Target> finished;
    finished.swap(targets);

    current = nullptr;

    if(restore)
    {
        for(size_t target_idx = 0; target_idx < finished.size(); target_idx++)
        {
            if(ControllerStillExists(finished[target_idx].controller))
            {
                Restore(finished[target_idx]);
            }
        }
    }

    emit Stopped();
}

void VryoPresetPlayer::Tick()
{
    if(!playing || (current == nullptr))
    {
        return;
    }

    const double seconds = (double)clock.elapsed() / 1000.0;

    /*-----------------------------------------------------*\
    | One-shot presets end when their time is up            |
    \*-----------------------------------------------------*/
    if((current->duration_seconds > 0.0) && (seconds >= current->duration_seconds))
    {
        EndPlayback(true);
        return;
    }

    /*-----------------------------------------------------*\
    | Drop devices that vanished (for example on a rescan)  |
    \*-----------------------------------------------------*/
    for(size_t target_idx = targets.size(); target_idx > 0; target_idx--)
    {
        if(!ControllerStillExists(targets[target_idx - 1].controller))
        {
            targets.erase(targets.begin() + (std::ptrdiff_t)(target_idx - 1));
        }
    }

    if(targets.empty())
    {
        EndPlayback(false);
        return;
    }

    /*-----------------------------------------------------*\
    | Ask the preset for every LED's colour and send it     |
    \*-----------------------------------------------------*/
    for(size_t target_idx = 0; target_idx < targets.size(); target_idx++)
    {
        Target&             target      = targets[target_idx];
        RGBController *     controller  = target.controller;
        const unsigned int  led_count   = controller->GetLEDCount();
        const unsigned int  info_count  = (unsigned int)target.infos.size();
        const unsigned int  count       = (led_count < info_count) ? led_count : info_count;

        for(unsigned int led_idx = 0; led_idx < count; led_idx++)
        {
            const QColor colour = current->render(seconds, target.infos[led_idx]);

            const int red       = colour.red();
            const int green     = colour.green();
            const int blue      = colour.blue();

            controller->SetColor(led_idx, ToRGBColor(red, green, blue));
        }

        controller->UpdateLEDs();
    }
}
