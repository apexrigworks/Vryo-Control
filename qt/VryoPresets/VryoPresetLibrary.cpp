/*---------------------------------------------------------*\
| VryoPresetLibrary.cpp                                     |
|                                                           |
|   Built-in lighting presets.                              |
|                                                           |
|   HOW TO ADD YOUR OWN                                     |
|   Scroll to "YOUR PRESETS" near the bottom. Copy the      |
|   "My Preset" block, give it a new id and name, change    |
|   the colour code, rebuild. It then appears on the        |
|   Presets tab and in the start-up / power-on pickers.     |
|                                                           |
|   A preset is one function:                               |
|       QColor f(double t, const VryoLedInfo& pos)          |
|   t    seconds since the preset started                   |
|   pos  where this LED sits: pos.position runs 0.0 to 1.0  |
|        along the whole card, pos.zone says which fan or   |
|        strip it is on. Return the colour for that LED.    |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "VryoPresetLibrary.h"

#include <cmath>

#include "../../startup/VryoTheme.h"

namespace
{
    const double VRYO_PI = 3.14159265358979323846;

    /*-----------------------------------------------------*\
    | Small helpers for writing presets                     |
    \*-----------------------------------------------------*/
    double Clamp01(double value)
    {
        if(value < 0.0)
        {
            return(0.0);
        }

        if(value > 1.0)
        {
            return(1.0);
        }

        return(value);
    }

    /*-----------------------------------------------------*\
    | Wrap any number into 0.0 .. 1.0 (for hue)             |
    \*-----------------------------------------------------*/
    double Wrap01(double value)
    {
        return(value - std::floor(value));
    }

    /*-----------------------------------------------------*\
    | 0.0 before edge0, 1.0 after edge1, smooth in between  |
    \*-----------------------------------------------------*/
    double SmoothStep(double edge0, double edge1, double x)
    {
        const double t = Clamp01((x - edge0) / (edge1 - edge0));

        return(t * t * (3.0 - (2.0 * t)));
    }

    /*-----------------------------------------------------*\
    | Blend two colours (amount 0.0 gives a, 1.0 gives b)   |
    \*-----------------------------------------------------*/
    QColor Mix(const QColor& a, const QColor& b, double amount)
    {
        const double t = Clamp01(amount);

        return(QColor::fromRgbF(a.redF()   + ((b.redF()   - a.redF())   * t),
                                a.greenF() + ((b.greenF() - a.greenF()) * t),
                                a.blueF()  + ((b.blueF()  - a.blueF())  * t)));
    }

    /*-----------------------------------------------------*\
    | Brightness 0.0 (black) to 1.0 (unchanged)             |
    \*-----------------------------------------------------*/
    QColor Dim(const QColor& colour, double brightness)
    {
        const double level = Clamp01(brightness);

        return(QColor::fromRgbF(colour.redF() * level, colour.greenF() * level, colour.blueF() * level));
    }

    /*-----------------------------------------------------*\
    | Colour from hue / saturation / value, each 0.0 to 1.0 |
    | (hue 0.0 red, 0.33 green, 0.66 blue)                  |
    \*-----------------------------------------------------*/
    QColor Hsv(double hue, double saturation, double value)
    {
        return(QColor::fromHsvF(Wrap01(hue), Clamp01(saturation), Clamp01(value)));
    }

    /*-----------------------------------------------------*\
    | The Vryo theme colours, so presets follow the brand   |
    \*-----------------------------------------------------*/
    QColor Accent()
    {
        return(VryoTheme::HexColor(VryoTheme::VRYO_COLOR_ACCENT));
    }

    QColor AccentLight()
    {
        return(VryoTheme::HexColor(VryoTheme::VRYO_COLOR_ACCENT_LIGHT));
    }

    QColor White()
    {
        return(QColor(255, 255, 255));
    }

    /*-----------------------------------------------------*\
    | Fade the very end of a one-shot preset to black so    |
    | the card's own lighting comes back smoothly.          |
    | progress is 0.0 (start) to 1.0 (end).                 |
    \*-----------------------------------------------------*/
    double TailFade(double progress)
    {
        return(1.0 - SmoothStep(0.85, 1.0, progress));
    }

    /*-----------------------------------------------------*\
    | Add one preset to the list                            |
    \*-----------------------------------------------------*/
    void Add(std::vector<VryoPreset>& list, const char * id, const char * name, const char * description, VryoPresetKind kind, double duration_seconds, VryoPresetRender render)
    {
        VryoPreset preset;

        preset.id               = QString::fromLatin1(id);
        preset.name             = QString::fromLatin1(name);
        preset.description      = QString::fromLatin1(description);
        preset.kind             = kind;
        preset.duration_seconds = duration_seconds;
        preset.render           = render;

        list.push_back(preset);
    }

    /*-----------------------------------------------------*\
    | Build the whole list                                  |
    \*-----------------------------------------------------*/
    std::vector<VryoPreset> BuildPresets()
    {
        std::vector<VryoPreset> list;

        /*=================================================*\
        | START-UP: play once when Vryo Control opens       |
        \*=================================================*/

        /*-------------------------------------------------*\
        | Boot Sweep: a bright blue head runs along the     |
        | card and leaves a fading trail behind it          |
        \*-------------------------------------------------*/
        {
            const double duration = 2.8;

            Add(list, "boot-sweep", "Boot Sweep", "A bright blue head sweeps along the card.", VRYO_PRESET_STARTUP, duration,
                [duration](double t, const VryoLedInfo& pos) -> QColor
                {
                    const double progress   = t / duration;
                    const double head       = (progress * 1.5) - 0.25;      // starts before the first LED, ends past the last
                    const double behind     = head - pos.position;          // above 0.0: the head has passed this LED
                    double       level      = 0.0;

                    if(behind >= 0.0)
                    {
                        level = std::exp(-behind * 4.5);
                    }
                    else
                    {
                        level = std::exp(behind * 25.0);
                    }

                    QColor colour = Mix(Accent(), AccentLight(), level);
                    colour        = Mix(colour, White(), Clamp01((level - 0.85) / 0.15) * 0.8);

                    return(Dim(colour, level * TailFade(progress)));
                });
        }

        /*-------------------------------------------------*\
        | Ignition: light spreads out from the middle of    |
        | the card with a white-hot front                   |
        \*-------------------------------------------------*/
        {
            const double duration = 3.0;

            Add(list, "ignition", "Ignition", "Light spreads out from the middle with a white-hot edge.", VRYO_PRESET_STARTUP, duration,
                [duration](double t, const VryoLedInfo& pos) -> QColor
                {
                    const double progress   = t / duration;
                    const double distance   = std::fabs(pos.position - 0.5) * 2.0;     // 0.0 in the middle, 1.0 at the ends
                    const double front      = progress * 1.6;
                    const double lit        = Clamp01((front - distance) * 4.0);
                    const double flash      = std::exp(-std::fabs(front - distance) * 12.0);

                    QColor colour = Mix(Accent(), AccentLight(), lit);
                    colour        = Mix(colour, White(), flash * 0.7);

                    return(Dim(colour, Clamp01((lit * 0.75) + flash) * TailFade(progress)));
                });
        }

        /*-------------------------------------------------*\
        | Aurora Rise: slow blue to violet waves fade in    |
        | and out                                           |
        \*-------------------------------------------------*/
        {
            const double duration = 3.4;

            Add(list, "aurora-rise", "Aurora Rise", "Slow blue and violet waves rise and fall.", VRYO_PRESET_STARTUP, duration,
                [duration](double t, const VryoLedInfo& pos) -> QColor
                {
                    const double progress   = t / duration;
                    const double envelope   = SmoothStep(0.0, 0.25, progress) * TailFade(progress);
                    const double wave       = 0.5 + (0.5 * std::sin(2.0 * VRYO_PI * ((pos.position * 1.5) - (t * 0.6))));

                    return(Dim(Hsv(0.60 + (0.14 * wave), 0.85, 0.25 + (0.75 * wave)), envelope));
                });
        }

        /*=================================================*\
        | POWER-ON: play once when the PC wakes up          |
        \*=================================================*/

        /*-------------------------------------------------*\
        | Power Up: everything surges on, then settles      |
        | into the Vryo blue                                |
        \*-------------------------------------------------*/
        {
            const double duration = 2.2;

            Add(list, "power-up", "Power Up", "The whole card surges on, then settles to blue.", VRYO_PRESET_POWER_ON, duration,
                [duration](double t, const VryoLedInfo&) -> QColor
                {
                    const double progress   = t / duration;
                    const double surge      = SmoothStep(0.0, 0.40, progress);
                    const double settle     = SmoothStep(0.40, 0.80, progress);
                    const QColor flash      = Mix(AccentLight(), White(), 0.65);

                    return(Dim(Mix(flash, Accent(), settle), surge * TailFade(progress)));
                });
        }

        /*-------------------------------------------------*\
        | Zone Cascade: each fan and strip lights up one    |
        | after the other                                   |
        \*-------------------------------------------------*/
        {
            const double duration = 2.6;

            Add(list, "zone-cascade", "Zone Cascade", "Each fan and strip lights up one after the other.", VRYO_PRESET_POWER_ON, duration,
                [duration](double t, const VryoLedInfo& pos) -> QColor
                {
                    const double progress   = t / duration;
                    const double zones      = (double)((pos.zone_count > 1u) ? pos.zone_count : 1u);
                    const double delay      = ((double)pos.zone / zones) * 0.45;
                    const double local      = Clamp01((progress - delay) / 0.35);
                    const double level      = SmoothStep(0.0, 1.0, local);

                    return(Dim(Mix(AccentLight(), Accent(), local), level * TailFade(progress)));
                });
        }

        /*-------------------------------------------------*\
        | Heartbeat: two soft pulses                        |
        \*-------------------------------------------------*/
        {
            const double duration = 2.4;

            Add(list, "heartbeat", "Heartbeat", "Two soft pulses, like a heartbeat.", VRYO_PRESET_POWER_ON, duration,
                [](double t, const VryoLedInfo&) -> QColor
                {
                    const double first      = (t - 0.35) / 0.14;
                    const double second     = (t - 0.85) / 0.14;
                    const double level      = Clamp01(std::exp(-first * first) + (0.8 * std::exp(-second * second)));

                    return(Dim(Mix(Accent(), White(), level * 0.3), level));
                });
        }

        /*=================================================*\
        | CASUAL: loop until stopped                        |
        \*=================================================*/

        /*-------------------------------------------------*\
        | Slow Breathe: the whole card swells and fades     |
        | every five seconds                                |
        \*-------------------------------------------------*/
        Add(list, "slow-breathe", "Slow Breathe", "The whole card slowly swells and fades in blue.", VRYO_PRESET_CASUAL, 0.0,
            [](double t, const VryoLedInfo&) -> QColor
            {
                const double level = 0.18 + (0.82 * (0.5 - (0.5 * std::cos(2.0 * VRYO_PI * t / 5.0))));

                return(Dim(Mix(Accent(), AccentLight(), level), level));
            });

        /*-------------------------------------------------*\
        | Ocean Drift: cyan and indigo currents             |
        \*-------------------------------------------------*/
        Add(list, "ocean-drift", "Ocean Drift", "Cyan and indigo currents drifting across the card.", VRYO_PRESET_CASUAL, 0.0,
            [](double t, const VryoLedInfo& pos) -> QColor
            {
                const double hue    = 0.57 + (0.09 * std::sin(2.0 * VRYO_PI * ((pos.position * 0.8) - (t / 10.0))));
                const double value  = 0.65 + (0.30 * std::sin(2.0 * VRYO_PI * ((pos.position * 1.0) + (t / 7.0))));

                return(Hsv(hue, 0.90, value));
            });

        /*-------------------------------------------------*\
        | Rainbow Flow: the full colour wheel moving along  |
        | the card                                          |
        \*-------------------------------------------------*/
        Add(list, "rainbow-flow", "Rainbow Flow", "The full colour wheel flowing along the card.", VRYO_PRESET_CASUAL, 0.0,
            [](double t, const VryoLedInfo& pos) -> QColor
            {
                return(Hsv((pos.position * 0.9) - (t / 7.0), 1.0, 0.9));
            });

        /*-------------------------------------------------*\
        | Ember: warm, flickering orange                    |
        \*-------------------------------------------------*/
        Add(list, "ember", "Ember", "Warm orange that flickers like glowing coals.", VRYO_PRESET_CASUAL, 0.0,
            [](double t, const VryoLedInfo& pos) -> QColor
            {
                const double seed   = (double)pos.index * 1.7;
                const double noise  = (0.5 * std::sin((t * 3.1) + seed))
                                    + (0.3 * std::sin((t * 7.3) + (seed * 2.3)))
                                    + (0.2 * std::sin((t * 13.7) + (seed * 0.7)));         // roughly -1.0 .. 1.0
                const double value  = Clamp01(0.50 + (0.38 * noise));

                return(Hsv(0.03 + (0.04 * ((noise * 0.5) + 0.5)), 1.0, (value < 0.12) ? 0.12 : value));
            });

        /*=================================================*\
        | YOUR PRESETS                                      |
        |                                                   |
        | Copy this block, change the id and the text,      |
        | change the colour code, rebuild.                  |
        |                                                   |
        |   id           short, unique, no spaces           |
        |   name         shown on the card                  |
        |   description  one line under the name            |
        |   kind         VRYO_PRESET_STARTUP, _POWER_ON or  |
        |                _CASUAL (only decides which list   |
        |                it shows up in)                    |
        |   duration     seconds, or 0.0 to loop forever    |
        |                                                   |
        | Inside the function:                              |
        |   t                    seconds since it started   |
        |   pos.position         0.0 to 1.0 along the card  |
        |   pos.zone             0, 1, 2 ... (fan, fan, strip)|
        |   pos.zone_position    0.0 to 1.0 inside a zone   |
        | Helpers: Mix(a, b, 0..1), Dim(colour, 0..1),      |
        |   Hsv(hue, sat, value), Accent(), AccentLight()   |
        |                                                   |
        | This example fades the whole card between the     |
        | Vryo blue and pink every six seconds.             |
        \*=================================================*/
        Add(list, "my-preset", "My Preset (edit me)", "Template: fades between blue and pink. Edit it in VryoPresetLibrary.cpp.", VRYO_PRESET_CASUAL, 0.0,
            [](double t, const VryoLedInfo&) -> QColor
            {
                const double swing = 0.5 - (0.5 * std::cos(2.0 * VRYO_PI * t / 6.0));      // 0.0 .. 1.0 .. 0.0

                return(Mix(Accent(), QColor(255, 60, 160), swing));
            });

        return(list);
    }
}

const std::vector<VryoPreset>& VryoPresetLibrary::All()
{
    static const std::vector<VryoPreset> presets = BuildPresets();

    return(presets);
}

const VryoPreset * VryoPresetLibrary::Find(const QString& id)
{
    const std::vector<VryoPreset>& presets = All();

    for(size_t preset_idx = 0; preset_idx < presets.size(); preset_idx++)
    {
        if(presets[preset_idx].id == id)
        {
            return(&presets[preset_idx]);
        }
    }

    return(nullptr);
}

std::vector<const VryoPreset *> VryoPresetLibrary::OfKind(VryoPresetKind kind)
{
    std::vector<const VryoPreset *>     result;
    const std::vector<VryoPreset>&      presets = All();

    for(size_t preset_idx = 0; preset_idx < presets.size(); preset_idx++)
    {
        if(presets[preset_idx].kind == kind)
        {
            result.push_back(&presets[preset_idx]);
        }
    }

    return(result);
}

QString VryoPresetLibrary::KindName(VryoPresetKind kind)
{
    switch(kind)
    {
        case VRYO_PRESET_STARTUP:
            return(QStringLiteral("Start-up"));

        case VRYO_PRESET_POWER_ON:
            return(QStringLiteral("Power-on"));

        case VRYO_PRESET_CASUAL:
        default:
            return(QStringLiteral("Casual"));
    }
}

QString VryoPresetLibrary::DefaultStartupId()
{
    return(QStringLiteral("boot-sweep"));
}

QString VryoPresetLibrary::DefaultPowerOnId()
{
    return(QStringLiteral("power-up"));
}

QString VryoPresetLibrary::DefaultCasualId()
{
    return(QStringLiteral("slow-breathe"));
}
