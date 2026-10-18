/*---------------------------------------------------------*\
| VryoPreset.h                                              |
|                                                           |
|   What a lighting preset is. A preset is one small        |
|   function: given the time and where an LED sits on the   |
|   card, it returns that LED's colour. The player calls    |
|   it about 25 times a second for every LED.               |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <functional>
#include <QColor>
#include <QString>

/*---------------------------------------------------------*\
| Where a preset is meant to be used. Only a label for the  |
| Presets tab and for the automatic start-up / power-on     |
| choices; any preset can still be played by hand.          |
\*---------------------------------------------------------*/
enum VryoPresetKind
{
    VRYO_PRESET_STARTUP     = 0,    // plays once when Vryo Control opens
    VRYO_PRESET_POWER_ON    = 1,    // plays once when the PC wakes up
    VRYO_PRESET_CASUAL      = 2     // loops until stopped
};

/*---------------------------------------------------------*\
| Where one LED sits. Use position (0.0 to 1.0 along the    |
| whole card) for effects that travel across the card, and  |
| zone / zone_position for effects that work per fan.       |
\*---------------------------------------------------------*/
struct VryoLedInfo
{
    unsigned int    index           = 0;        // 0-based number of this LED on the card
    unsigned int    count           = 1;        // LEDs on the whole card
    double          position        = 0.0;      // 0.0 first LED .. 1.0 last LED

    unsigned int    zone            = 0;        // 0-based zone number (fan 1, fan 2, strip...)
    unsigned int    zone_count      = 1;        // zones on the card
    unsigned int    zone_index      = 0;        // 0-based number of this LED inside its zone
    unsigned int    zone_leds       = 1;        // LEDs in this LED's zone
    double          zone_position   = 0.0;      // 0.0 first LED of the zone .. 1.0 last
};

/*---------------------------------------------------------*\
| The one function a preset is made of.                     |
|   seconds   time since the preset started                 |
|   pos       where the LED sits (see above)                |
| Returns the colour for that LED right now.                |
\*---------------------------------------------------------*/
typedef std::function<QColor(double seconds, const VryoLedInfo& pos)> VryoPresetRender;

struct VryoPreset
{
    QString             id;                                 // short, unique, saved in the settings
    QString             name;                               // shown on the card
    QString             description;                        // one line under the name
    VryoPresetKind      kind                = VRYO_PRESET_CASUAL;
    double              duration_seconds    = 0.0;          // 0 = loops until stopped
    VryoPresetRender    render;
};
