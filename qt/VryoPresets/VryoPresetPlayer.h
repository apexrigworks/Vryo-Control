/*---------------------------------------------------------*\
| VryoPresetPlayer.h                                        |
|                                                           |
|   Plays one preset on a set of RGB devices. Runs on the   |
|   Qt thread with a timer. When the preset ends, or is     |
|   stopped, every device goes back to the mode and colours |
|   it had before.                                          |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <vector>
#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>

#include "RGBController.h"
#include "VryoPreset.h"

class VryoPresetPlayer : public QObject
{
    Q_OBJECT

public:
    explicit VryoPresetPlayer(QObject * parent = nullptr);
    ~VryoPresetPlayer();

    /*-----------------------------------------------------*\
    | Start a preset on these devices. Anything already     |
    | playing is stopped first. Returns false when none of  |
    | the devices can show per-LED colours (nothing starts).|
    \*-----------------------------------------------------*/
    bool    Play(const VryoPreset * preset, const std::vector<RGBController *>& controllers);

    /*-----------------------------------------------------*\
    | Stop and put the devices back as they were            |
    \*-----------------------------------------------------*/
    void    Stop();

    bool    IsPlaying() const;
    QString PlayingId() const;

signals:
    void    Started(const QString& id);

    /*-----------------------------------------------------*\
    | Sent every time playing ends, whether the preset ran  |
    | out, Stop() was called or the devices went away       |
    \*-----------------------------------------------------*/
    void    Stopped();

private slots:
    void    Tick();

private:
    struct Target
    {
        RGBController *             controller      = nullptr;
        int                         previous_mode   = -1;
        std::vector<RGBColor>       previous_colours;
        std::vector<VryoLedInfo>    infos;
    };

    QTimer                  timer;
    QElapsedTimer           clock;
    std::vector<Target>     targets;
    const VryoPreset *      current         = nullptr;
    bool                    playing         = false;

    void    EndPlayback(bool restore);
    void    Restore(const Target& target);
    static  std::vector<VryoLedInfo> BuildInfos(RGBController * controller);
};
