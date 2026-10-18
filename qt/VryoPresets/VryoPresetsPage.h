/*---------------------------------------------------------*\
| VryoPresetsPage.h                                         |
|                                                           |
|   The Presets tab: cards for every lighting preset, a     |
|   play / stop button, and the choice of which animation   |
|   plays by itself at start-up and after waking the PC.    |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <map>
#include <vector>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "VryoPreset.h"
#include "VryoPresetPlayer.h"

class VryoPresetsPage : public QWidget
{
    Q_OBJECT

public:
    explicit VryoPresetsPage(QWidget * parent = nullptr);
    ~VryoPresetsPage();

    /*-----------------------------------------------------*\
    | The device list changed (detection finished, device   |
    | added or removed)                                     |
    \*-----------------------------------------------------*/
    void    RefreshDevices();

    /*-----------------------------------------------------*\
    | Play the chosen start-up animation, once per launch,  |
    | if the person has it switched on                      |
    \*-----------------------------------------------------*/
    void    PlayStartupIfWanted();

    /*-----------------------------------------------------*\
    | Stop whatever is playing (devices go back as they     |
    | were). Used before devices are rescanned.             |
    \*-----------------------------------------------------*/
    void    StopAnimation();

public slots:
    /*-----------------------------------------------------*\
    | The PC woke from sleep. Plays the power-on animation  |
    | after a short wait for the devices to come back.      |
    \*-----------------------------------------------------*/
    void    OnSystemResumed();

protected:
    void    resizeEvent(QResizeEvent * event) override;
    void    showEvent(QShowEvent * event) override;

private slots:
    void    OnPowerToggled(bool checked);
    void    OnPlayerStarted(const QString& id);
    void    OnPlayerStopped();
    void    OnAutoSettingsChanged();

private:
    struct Section
    {
        QGridLayout *               grid    = nullptr;
        std::vector<QFrame *>       cards;
    };

    VryoPresetPlayer *              player              = nullptr;

    QPushButton *                   power_button        = nullptr;
    QLabel *                        devices_label       = nullptr;
    QLabel *                        status_label        = nullptr;

    QCheckBox *                     startup_check       = nullptr;
    QComboBox *                     startup_combo       = nullptr;
    QCheckBox *                     power_on_check      = nullptr;
    QComboBox *                     power_on_combo      = nullptr;

    std::vector<Section>            sections;
    std::map<QString, QPushButton *> play_buttons;

    int                             columns             = 0;
    bool                            loading_settings    = false;
    bool                            startup_played      = false;

    bool                            startup_enabled     = true;
    bool                            power_on_enabled    = true;
    QString                         startup_id;
    QString                         power_on_id;
    QString                         last_id;

    void    BuildUi();
    QFrame *BuildCard(const VryoPreset& preset, QWidget * parent);
    void    FillCombo(QComboBox * combo);
    void    Reflow();

    void    LoadSettings();
    void    SaveSettings();

    void    PlayPreset(const QString& id, bool remember);
    void    OnCardButtonClicked(const QString& id);
    void    UpdatePlayState();
};
