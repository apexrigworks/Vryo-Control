/*---------------------------------------------------------*\
| VryoPresetLibrary.h                                       |
|                                                           |
|   The list of built-in presets. To add your own, see the  |
|   "YOUR PRESETS" section in VryoPresetLibrary.cpp.        |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <vector>
#include <QString>

#include "VryoPreset.h"

class VryoPresetLibrary
{
public:
    /*-----------------------------------------------------*\
    | Every preset, in the order shown on the Presets tab.  |
    | The list is built once and never changes afterwards,  |
    | so the pointers below stay valid for the whole run.   |
    \*-----------------------------------------------------*/
    static const std::vector<VryoPreset>&   All();

    /*-----------------------------------------------------*\
    | One preset by id, or nullptr if there is none         |
    \*-----------------------------------------------------*/
    static const VryoPreset *               Find(const QString& id);

    /*-----------------------------------------------------*\
    | All presets of one kind                               |
    \*-----------------------------------------------------*/
    static std::vector<const VryoPreset *>  OfKind(VryoPresetKind kind);

    /*-----------------------------------------------------*\
    | "Start-up", "Power-on" or "Casual"                    |
    \*-----------------------------------------------------*/
    static QString                          KindName(VryoPresetKind kind);

    /*-----------------------------------------------------*\
    | Ids used when nothing has been chosen yet             |
    \*-----------------------------------------------------*/
    static QString                          DefaultStartupId();
    static QString                          DefaultPowerOnId();
    static QString                          DefaultCasualId();
};
