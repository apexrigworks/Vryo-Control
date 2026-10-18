/*---------------------------------------------------------*\
| VryoPresetsPage.cpp                                       |
|                                                           |
|   The Presets tab.                                        |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "VryoPresetsPage.h"

#include <algorithm>

#include <QFont>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QScrollArea>
#include <QShowEvent>
#include <QStringList>
#include <QTimer>

#include "ResourceManager.h"
#include "SettingsManager.h"
#include "VryoPresetLibrary.h"

namespace
{
    const char *    VRYO_PRESETS_SETTINGS_KEY   = "VryoPresets";
    const int       VRYO_PRESETS_MARGIN         = 12;
    const int       VRYO_PRESETS_CARD_WIDTH     = 300;
    const int       VRYO_PRESETS_MAX_COLUMNS    = 4;

    /*-----------------------------------------------------*\
    | Wait this long after waking before the power-on       |
    | animation starts, so the devices are back first       |
    \*-----------------------------------------------------*/
    const int       VRYO_PRESETS_RESUME_DELAY_MS = 2500;
}

VryoPresetsPage::VryoPresetsPage(QWidget * parent) :
    QWidget(parent)
{
    player = new VryoPresetPlayer(this);

    connect(player, &VryoPresetPlayer::Started, this, &VryoPresetsPage::OnPlayerStarted);
    connect(player, &VryoPresetPlayer::Stopped, this, &VryoPresetsPage::OnPlayerStopped);

    /*-----------------------------------------------------*\
    | Filling the pickers fires change signals, so keep the |
    | settings from being saved until loading is finished   |
    \*-----------------------------------------------------*/
    loading_settings = true;

    BuildUi();
    LoadSettings();
    RefreshDevices();
    UpdatePlayState();
}

VryoPresetsPage::~VryoPresetsPage()
{
}

void VryoPresetsPage::BuildUi()
{
    QVBoxLayout * root = new QVBoxLayout(this);
    root->setContentsMargins(VRYO_PRESETS_MARGIN, VRYO_PRESETS_MARGIN, VRYO_PRESETS_MARGIN, VRYO_PRESETS_MARGIN);
    root->setSpacing(12);

    /*-----------------------------------------------------*\
    | Header: title on the left, power button on the right  |
    \*-----------------------------------------------------*/
    QHBoxLayout * header = new QHBoxLayout();
    QVBoxLayout * title_box = new QVBoxLayout();
    title_box->setSpacing(2);

    QLabel * title = new QLabel(QStringLiteral("Presets"), this);
    QFont title_font = title->font();
    title_font.setPointSize(18);
    title_font.setWeight(QFont::DemiBold);
    title->setFont(title_font);

    QLabel * subtitle = new QLabel(QStringLiteral("Lighting animations for your RGB devices"), this);
    subtitle->setObjectName(QStringLiteral("VryoMuted"));

    title_box->addWidget(title);
    title_box->addWidget(subtitle);
    header->addLayout(title_box, 1);

    power_button = new QPushButton(this);
    power_button->setObjectName(QStringLiteral("VryoPower"));
    power_button->setCheckable(true);
    power_button->setCursor(Qt::PointingHandCursor);
    power_button->setToolTip(QStringLiteral("Plays the preset you used last, or stops the one that is playing"));
    connect(power_button, &QPushButton::toggled, this, &VryoPresetsPage::OnPowerToggled);
    header->addWidget(power_button, 0, Qt::AlignVCenter);

    root->addLayout(header);

    devices_label = new QLabel(this);
    devices_label->setObjectName(QStringLiteral("VryoMuted"));
    devices_label->setWordWrap(true);
    root->addWidget(devices_label);

    status_label = new QLabel(this);
    status_label->setObjectName(QStringLiteral("VryoMuted"));
    status_label->setWordWrap(true);
    root->addWidget(status_label);

    /*-----------------------------------------------------*\
    | "Play automatically" card                             |
    \*-----------------------------------------------------*/
    QFrame * auto_card = new QFrame(this);
    auto_card->setObjectName(QStringLiteral("VryoCard"));

    QGridLayout * auto_grid = new QGridLayout(auto_card);
    auto_grid->setContentsMargins(14, 12, 14, 12);
    auto_grid->setHorizontalSpacing(12);
    auto_grid->setVerticalSpacing(8);

    QLabel * auto_title = new QLabel(QStringLiteral("Play automatically"), auto_card);
    auto_title->setObjectName(QStringLiteral("VryoCardTitle"));
    auto_grid->addWidget(auto_title, 0, 0, 1, 2);

    startup_check = new QCheckBox(QStringLiteral("When Vryo Control opens"), auto_card);
    startup_combo = new QComboBox(auto_card);
    startup_combo->setMinimumWidth(200);
    FillCombo(startup_combo);

    power_on_check = new QCheckBox(QStringLiteral("When the PC wakes from sleep"), auto_card);
    power_on_combo = new QComboBox(auto_card);
    power_on_combo->setMinimumWidth(200);
    FillCombo(power_on_combo);

    auto_grid->addWidget(startup_check, 1, 0);
    auto_grid->addWidget(startup_combo, 1, 1);
    auto_grid->addWidget(power_on_check, 2, 0);
    auto_grid->addWidget(power_on_combo, 2, 1);
    auto_grid->setColumnStretch(0, 1);

    connect(startup_check,  &QCheckBox::toggled, this, &VryoPresetsPage::OnAutoSettingsChanged);
    connect(power_on_check, &QCheckBox::toggled, this, &VryoPresetsPage::OnAutoSettingsChanged);
    connect(startup_combo,  QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { OnAutoSettingsChanged(); });
    connect(power_on_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { OnAutoSettingsChanged(); });

    root->addWidget(auto_card);

    /*-----------------------------------------------------*\
    | Scrolling list of preset cards, one section per kind  |
    \*-----------------------------------------------------*/
    QScrollArea * scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget * content = new QWidget();
    QVBoxLayout * content_layout = new QVBoxLayout(content);
    content_layout->setContentsMargins(0, 0, 6, 0);
    content_layout->setSpacing(10);

    const VryoPresetKind kinds[3] = { VRYO_PRESET_STARTUP, VRYO_PRESET_POWER_ON, VRYO_PRESET_CASUAL };

    const QString headings[3] =
    {
        QStringLiteral("Start-up animations  -  play once"),
        QStringLiteral("Power-on animations  -  play once"),
        QStringLiteral("Casual animations  -  loop until stopped")
    };

    for(int kind_idx = 0; kind_idx < 3; kind_idx++)
    {
        QLabel * heading = new QLabel(headings[kind_idx], content);
        heading->setObjectName(QStringLiteral("VryoMuted"));

        QFont heading_font = heading->font();
        heading_font.setWeight(QFont::DemiBold);
        heading->setFont(heading_font);

        content_layout->addWidget(heading);

        QWidget * grid_host = new QWidget(content);
        QGridLayout * grid = new QGridLayout(grid_host);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setSpacing(12);
        content_layout->addWidget(grid_host);

        Section section;
        section.grid = grid;

        std::vector<const VryoPreset *> presets = VryoPresetLibrary::OfKind(kinds[kind_idx]);

        for(size_t preset_idx = 0; preset_idx < presets.size(); preset_idx++)
        {
            section.cards.push_back(BuildCard(*presets[preset_idx], grid_host));
        }

        sections.push_back(section);
    }

    content_layout->addStretch(1);

    scroll->setWidget(content);
    root->addWidget(scroll, 1);

    Reflow();
}

QFrame * VryoPresetsPage::BuildCard(const VryoPreset& preset, QWidget * parent)
{
    QFrame * card = new QFrame(parent);
    card->setObjectName(QStringLiteral("VryoCard"));

    QVBoxLayout * layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(8);

    /*-----------------------------------------------------*\
    | Name on the left, kind and length badge on the right  |
    \*-----------------------------------------------------*/
    QHBoxLayout * top = new QHBoxLayout();
    top->setSpacing(8);

    QLabel * name_label = new QLabel(preset.name, card);
    name_label->setObjectName(QStringLiteral("VryoCardTitle"));

    QString length_text = QStringLiteral("loops");

    if(preset.duration_seconds > 0.0)
    {
        length_text = QString::number(preset.duration_seconds, 'f', 1) + QStringLiteral(" s");
    }

    QLabel * badge = new QLabel(VryoPresetLibrary::KindName(preset.kind) + QStringLiteral("  -  ") + length_text, card);
    badge->setObjectName(QStringLiteral("VryoBadge"));

    top->addWidget(name_label, 1);
    top->addWidget(badge, 0, Qt::AlignRight | Qt::AlignVCenter);
    layout->addLayout(top);

    QLabel * description = new QLabel(preset.description, card);
    description->setObjectName(QStringLiteral("VryoMuted"));
    description->setWordWrap(true);
    layout->addWidget(description, 1);

    QPushButton * button = new QPushButton(QStringLiteral("Play"), card);
    button->setCursor(Qt::PointingHandCursor);
    layout->addWidget(button);

    play_buttons[preset.id] = button;

    const QString id = preset.id;

    connect(button, &QPushButton::clicked, this, [this, id]()
    {
        OnCardButtonClicked(id);
    });

    return(card);
}

void VryoPresetsPage::FillCombo(QComboBox * combo)
{
    const std::vector<VryoPreset>& presets = VryoPresetLibrary::All();

    for(size_t preset_idx = 0; preset_idx < presets.size(); preset_idx++)
    {
        combo->addItem(presets[preset_idx].name, presets[preset_idx].id);
    }
}

void VryoPresetsPage::Reflow()
{
    int new_columns = (width() - (2 * VRYO_PRESETS_MARGIN)) / VRYO_PRESETS_CARD_WIDTH;

    new_columns = (std::max)(1, (std::min)(VRYO_PRESETS_MAX_COLUMNS, new_columns));

    if(new_columns == columns)
    {
        return;
    }

    columns = new_columns;

    for(size_t section_idx = 0; section_idx < sections.size(); section_idx++)
    {
        Section& section = sections[section_idx];

        for(size_t card_idx = 0; card_idx < section.cards.size(); card_idx++)
        {
            section.grid->removeWidget(section.cards[card_idx]);
        }

        for(size_t card_idx = 0; card_idx < section.cards.size(); card_idx++)
        {
            section.grid->addWidget(section.cards[card_idx], (int)card_idx / columns, (int)card_idx % columns);
        }

        for(int column_idx = 0; column_idx < VRYO_PRESETS_MAX_COLUMNS; column_idx++)
        {
            section.grid->setColumnStretch(column_idx, (column_idx < columns) ? 1 : 0);
        }
    }
}

void VryoPresetsPage::resizeEvent(QResizeEvent * event)
{
    QWidget::resizeEvent(event);

    Reflow();
}

void VryoPresetsPage::showEvent(QShowEvent * event)
{
    QWidget::showEvent(event);

    RefreshDevices();
}

/*---------------------------------------------------------*\
| Settings, stored under "VryoPresets" in OpenRGB.json      |
\*---------------------------------------------------------*/
void VryoPresetsPage::LoadSettings()
{
    loading_settings    = true;

    startup_enabled     = true;
    power_on_enabled    = true;
    startup_id          = VryoPresetLibrary::DefaultStartupId();
    power_on_id         = VryoPresetLibrary::DefaultPowerOnId();
    last_id             = VryoPresetLibrary::DefaultCasualId();

    SettingsManager * settings = ResourceManager::get()->GetSettingsManager();

    if(settings != nullptr)
    {
        nlohmann::json stored = settings->GetSettings(VRYO_PRESETS_SETTINGS_KEY);

        if(stored.is_object())
        {
            if(stored.contains("startup_enabled") && stored["startup_enabled"].is_boolean())
            {
                startup_enabled = stored["startup_enabled"].get<bool>();
            }

            if(stored.contains("power_on_enabled") && stored["power_on_enabled"].is_boolean())
            {
                power_on_enabled = stored["power_on_enabled"].get<bool>();
            }

            if(stored.contains("startup_preset") && stored["startup_preset"].is_string())
            {
                const QString id = QString::fromStdString(stored["startup_preset"].get<std::string>());

                if(VryoPresetLibrary::Find(id) != nullptr)
                {
                    startup_id = id;
                }
            }

            if(stored.contains("power_on_preset") && stored["power_on_preset"].is_string())
            {
                const QString id = QString::fromStdString(stored["power_on_preset"].get<std::string>());

                if(VryoPresetLibrary::Find(id) != nullptr)
                {
                    power_on_id = id;
                }
            }

            if(stored.contains("last_preset") && stored["last_preset"].is_string())
            {
                const QString id = QString::fromStdString(stored["last_preset"].get<std::string>());

                if(VryoPresetLibrary::Find(id) != nullptr)
                {
                    last_id = id;
                }
            }
        }
    }

    startup_check->setChecked(startup_enabled);
    power_on_check->setChecked(power_on_enabled);

    int index = startup_combo->findData(startup_id);
    startup_combo->setCurrentIndex((index >= 0) ? index : 0);

    index = power_on_combo->findData(power_on_id);
    power_on_combo->setCurrentIndex((index >= 0) ? index : 0);

    loading_settings = false;
}

void VryoPresetsPage::SaveSettings()
{
    SettingsManager * settings = ResourceManager::get()->GetSettingsManager();

    if(settings == nullptr)
    {
        return;
    }

    nlohmann::json stored = settings->GetSettings(VRYO_PRESETS_SETTINGS_KEY);

    if(!stored.is_object())
    {
        stored = nlohmann::json::object();
    }

    stored["startup_enabled"]   = startup_enabled;
    stored["startup_preset"]    = startup_id.toStdString();
    stored["power_on_enabled"]  = power_on_enabled;
    stored["power_on_preset"]   = power_on_id.toStdString();
    stored["last_preset"]       = last_id.toStdString();

    settings->SetSettings(VRYO_PRESETS_SETTINGS_KEY, stored);
    settings->SaveSettings();
}

void VryoPresetsPage::OnAutoSettingsChanged()
{
    if(loading_settings)
    {
        return;
    }

    startup_enabled     = startup_check->isChecked();
    power_on_enabled    = power_on_check->isChecked();

    const QString startup_pick = startup_combo->currentData().toString();
    const QString power_on_pick = power_on_combo->currentData().toString();

    if(!startup_pick.isEmpty())
    {
        startup_id = startup_pick;
    }

    if(!power_on_pick.isEmpty())
    {
        power_on_id = power_on_pick;
    }

    SaveSettings();
}

/*---------------------------------------------------------*\
| Devices                                                   |
\*---------------------------------------------------------*/
void VryoPresetsPage::RefreshDevices()
{
    std::vector<RGBController *>& controllers = ResourceManager::get()->GetRGBControllers();
    QStringList names;

    for(size_t controller_idx = 0; controller_idx < controllers.size(); controller_idx++)
    {
        if(!controllers[controller_idx]->GetHidden())
        {
            names << QString::fromStdString(controllers[controller_idx]->GetDisplayName());
        }
    }

    if(names.isEmpty())
    {
        devices_label->setText(QStringLiteral("No RGB devices detected yet."));
    }
    else
    {
        devices_label->setText(QStringLiteral("Devices: ") + names.join(QStringLiteral(", ")));
    }
}

/*---------------------------------------------------------*\
| Playing                                                   |
\*---------------------------------------------------------*/
void VryoPresetsPage::PlayPreset(const QString& id, bool remember)
{
    const VryoPreset * preset = VryoPresetLibrary::Find(id);

    if(preset == nullptr)
    {
        return;
    }

    std::vector<RGBController *> controllers = ResourceManager::get()->GetRGBControllers();

    if(player->Play(preset, controllers))
    {
        if(remember && (last_id != id))
        {
            last_id = id;
            SaveSettings();
        }
    }
    else
    {
        UpdatePlayState();

        status_label->setText(QStringLiteral("No RGB device can play animations right now. A device needs a Direct or Custom mode."));
    }
}

void VryoPresetsPage::OnCardButtonClicked(const QString& id)
{
    if(player->IsPlaying() && (player->PlayingId() == id))
    {
        player->Stop();
    }
    else
    {
        PlayPreset(id, true);
    }
}

void VryoPresetsPage::OnPowerToggled(bool checked)
{
    if(checked)
    {
        PlayPreset(last_id, true);
    }
    else
    {
        player->Stop();
    }
}

void VryoPresetsPage::OnPlayerStarted(const QString&)
{
    UpdatePlayState();
}

void VryoPresetsPage::OnPlayerStopped()
{
    UpdatePlayState();
}

void VryoPresetsPage::UpdatePlayState()
{
    const bool      playing     = player->IsPlaying();
    const QString   playing_id  = player->PlayingId();

    power_button->blockSignals(true);
    power_button->setChecked(playing);
    power_button->setText(playing ? QStringLiteral("Stop animation") : QStringLiteral("Play animation"));
    power_button->blockSignals(false);

    for(std::map<QString, QPushButton *>::iterator entry = play_buttons.begin(); entry != play_buttons.end(); ++entry)
    {
        const bool this_one = playing && (entry->first == playing_id);

        entry->second->setText(this_one ? QStringLiteral("Stop") : QStringLiteral("Play"));
    }

    if(playing)
    {
        const VryoPreset * preset = VryoPresetLibrary::Find(playing_id);

        status_label->setText(QStringLiteral("Playing: ") + ((preset != nullptr) ? preset->name : playing_id));
    }
    else
    {
        status_label->setText(QStringLiteral("Not playing."));
    }
}

void VryoPresetsPage::StopAnimation()
{
    player->Stop();
}

void VryoPresetsPage::PlayStartupIfWanted()
{
    if(startup_played)
    {
        return;
    }

    /*-----------------------------------------------------*\
    | Wait until there is a device to play on. Detection    |
    | can end more than once, and this runs only one time.  |
    \*-----------------------------------------------------*/
    std::vector<RGBController *>& controllers = ResourceManager::get()->GetRGBControllers();
    bool device_found = false;

    for(size_t controller_idx = 0; controller_idx < controllers.size(); controller_idx++)
    {
        if(!controllers[controller_idx]->GetHidden() && (controllers[controller_idx]->GetLEDCount() > 0))
        {
            device_found = true;
            break;
        }
    }

    if(!device_found)
    {
        return;
    }

    startup_played = true;

    if(startup_enabled && !player->IsPlaying())
    {
        PlayPreset(startup_id, false);
    }
}

void VryoPresetsPage::OnSystemResumed()
{
    if(!power_on_enabled)
    {
        return;
    }

    /*-----------------------------------------------------*\
    | The context object (this) cancels the call if the     |
    | page is deleted before the delay is up                |
    \*-----------------------------------------------------*/
    QTimer::singleShot(VRYO_PRESETS_RESUME_DELAY_MS, this, [this]()
    {
        /*-------------------------------------------------*\
        | A looping animation keeps drawing and so already  |
        | brings the lights back; leave it alone            |
        \*-------------------------------------------------*/
        if(!player->IsPlaying())
        {
            PlayPreset(power_on_id, false);
        }
    });
}
