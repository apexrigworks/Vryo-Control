/*---------------------------------------------------------*\
| VryoDashboardPage.cpp                                     |
|                                                           |
|   The Dashboard tab: live GPU readings from the NVIDIA    |
|   driver.                                                 |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "VryoDashboardPage.h"

#include <QEasingCurve>
#include <QFont>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QPointer>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QScrollArea>
#include <QShowEvent>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>

#include "ResourceManager.h"
#include "SettingsManager.h"
#include "../../Controllers/VryoGPUController/VryoGPUConfig.h"
#include "../../startup/VryoTheme.h"

namespace
{
    const int       VRYO_DASH_POLL_MS       = 1000;
    const int       VRYO_DASH_MARGIN        = 12;
    const int       VRYO_DASH_SPACING       = 12;
    const int       VRYO_DASH_TILE_WIDTH    = 200;
    const int       VRYO_DASH_MAX_COLUMNS   = 4;

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
    | Blend two colours (amount 0.0 gives from, 1.0 to)     |
    \*-----------------------------------------------------*/
    QColor BlendColours(const QColor& from, const QColor& to, double amount)
    {
        const double t = Clamp01(amount);

        return(QColor::fromRgbF(from.redF()   + ((to.redF()   - from.redF())   * t),
                                from.greenF() + ((to.greenF() - from.greenF()) * t),
                                from.blueF()  + ((to.blueF()  - from.blueF())  * t)));
    }

    /*-----------------------------------------------------*\
    | Temperature colour, changing smoothly with the heat:  |
    |   up to 45 C   Vryo blue                              |
    |   65 C         amber                                  |
    |   85 C and up  red                                    |
    \*-----------------------------------------------------*/
    const int       VRYO_TEMP_COOL_C    = 45;
    const int       VRYO_TEMP_WARM_C    = 65;
    const int       VRYO_TEMP_HOT_C     = 85;

    QColor TemperatureColour(int celsius)
    {
        const QColor cool = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_ACCENT);
        const QColor warm = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_WARN);
        const QColor hot  = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_HOT);

        if(celsius <= VRYO_TEMP_COOL_C)
        {
            return(cool);
        }

        if(celsius < VRYO_TEMP_WARM_C)
        {
            return(BlendColours(cool, warm, (double)(celsius - VRYO_TEMP_COOL_C) / (double)(VRYO_TEMP_WARM_C - VRYO_TEMP_COOL_C)));
        }

        if(celsius < VRYO_TEMP_HOT_C)
        {
            return(BlendColours(warm, hot, (double)(celsius - VRYO_TEMP_WARM_C) / (double)(VRYO_TEMP_HOT_C - VRYO_TEMP_WARM_C)));
        }

        return(hot);
    }

    /*-----------------------------------------------------*\
    | "NVIDIA GeForce GTX 1660 SUPER" -> "gtx 1660 super"   |
    \*-----------------------------------------------------*/
    QString CoreGpuName(QString name)
    {
        name = name.toLower();
        name.remove(QStringLiteral("nvidia"));
        name.remove(QStringLiteral("geforce"));

        return(name.simplified());
    }

    /*-----------------------------------------------------*\
    | "Glacier GTX 1660 Super 6GB" -> "gtx 1660 super"      |
    | (drops the series word and the memory size)           |
    \*-----------------------------------------------------*/
    QString CoreModelName(const QString& model_name)
    {
        QStringList words = model_name.toLower().split(QLatin1Char(' '), Qt::SkipEmptyParts);

        if(!words.isEmpty())
        {
            words.removeFirst();
        }

        if(!words.isEmpty() && words.last().endsWith(QStringLiteral("gb")))
        {
            words.removeLast();
        }

        return(words.join(QLatin1Char(' ')));
    }

    /*-----------------------------------------------------*\
    | The Vryo series name for this card, or an empty       |
    | string if it is not a Vryo model. Order:              |
    |   1. the model named under "VryoGPU" in OpenRGB.json  |
    |      (only when there is a single GPU)                |
    |   2. a Vryo model whose GPU matches the driver name   |
    \*-----------------------------------------------------*/
    QString ResolveSeriesName(const QString& gpu_name, bool single_gpu)
    {
        const size_t model_count = sizeof(YB_GPU_MODELS) / sizeof(YB_GPU_MODELS[0]);

        if(single_gpu)
        {
            SettingsManager * settings = ResourceManager::get()->GetSettingsManager();

            if(settings != nullptr)
            {
                nlohmann::json gpu_settings = settings->GetSettings(YB_GPU_SETTINGS_KEY);

                if(gpu_settings.is_object() && gpu_settings.contains("model") && gpu_settings["model"].is_string())
                {
                    const QString wanted = QString::fromStdString(gpu_settings["model"].get<std::string>()).toLower();

                    for(size_t i = 0; i < model_count; i++)
                    {
                        if((wanted == QString::fromLatin1(YB_GPU_MODELS[i].id).toLower())
                         || (wanted == QString::fromLatin1(YB_GPU_MODELS[i].name).toLower()))
                        {
                            return(QString::fromLatin1(YB_GPU_MODELS[i].name));
                        }
                    }
                }
            }
        }

        const QString core = CoreGpuName(gpu_name);

        if(!core.isEmpty())
        {
            for(size_t i = 0; i < model_count; i++)
            {
                if(CoreModelName(QString::fromLatin1(YB_GPU_MODELS[i].name)) == core)
                {
                    return(QString::fromLatin1(YB_GPU_MODELS[i].name));
                }
            }
        }

        return(QString());
    }
}

VryoDashboardPage::VryoDashboardPage(QWidget * parent) :
    QWidget(parent)
{
    timer = new QTimer(this);
    timer->setInterval(VRYO_DASH_POLL_MS);
    connect(timer, &QTimer::timeout, this, &VryoDashboardPage::Poll);

    BuildUi();
}

VryoDashboardPage::~VryoDashboardPage()
{
}

void VryoDashboardPage::BuildUi()
{
    const QString muted_style = QStringLiteral("color: %1;").arg(QString::fromLatin1(VryoTheme::VRYO_COLOR_MUTED));

    QVBoxLayout * root = new QVBoxLayout(this);
    root->setContentsMargins(VRYO_DASH_MARGIN, VRYO_DASH_MARGIN, VRYO_DASH_MARGIN, VRYO_DASH_MARGIN);
    root->setSpacing(10);

    /*-----------------------------------------------------*\
    | Header: GPU name, driver version, GPU picker          |
    | (kept in its own widget so it can fade in)            |
    \*-----------------------------------------------------*/
    header_widget = new QWidget(this);

    QHBoxLayout * header = new QHBoxLayout(header_widget);
    header->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout * title_box = new QVBoxLayout();
    title_box->setSpacing(0);

    gpu_name_label = new QLabel(tr("GPU Dashboard"), header_widget);
    QFont name_font = gpu_name_label->font();
    name_font.setPointSizeF(18.0);
    name_font.setWeight(QFont::DemiBold);
    gpu_name_label->setFont(name_font);

    driver_label = new QLabel(header_widget);
    driver_label->setStyleSheet(muted_style);

    title_box->addWidget(gpu_name_label);
    title_box->addWidget(driver_label);

    gpu_picker = new QComboBox(header_widget);
    gpu_picker->setVisible(false);
    connect(gpu_picker, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VryoDashboardPage::OnGpuPicked);

    header->addLayout(title_box, 1);
    header->addWidget(gpu_picker, 0, Qt::AlignTop);
    root->addWidget(header_widget);

    /*-----------------------------------------------------*\
    | Message shown when there is no NVIDIA driver or GPU   |
    \*-----------------------------------------------------*/
    QHBoxLayout * status_row = new QHBoxLayout();

    status_label = new QLabel(this);
    status_label->setWordWrap(true);
    status_label->setStyleSheet(muted_style);
    status_label->setVisible(false);

    retry_button = new QPushButton(tr("Try again"), this);
    retry_button->setVisible(false);
    connect(retry_button, &QPushButton::clicked, this, &VryoDashboardPage::OnRetryClicked);

    status_row->addWidget(status_label, 1);
    status_row->addWidget(retry_button, 0, Qt::AlignTop);
    root->addLayout(status_row);

    /*-----------------------------------------------------*\
    | The cards, inside a scroll area so a small window     |
    | still works                                           |
    \*-----------------------------------------------------*/
    QScrollArea * scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; border: none; }"));

    tiles_container = new QWidget();
    QVBoxLayout * container_layout = new QVBoxLayout(tiles_container);
    container_layout->setContentsMargins(0, 0, 0, 0);

    tiles_grid = new QGridLayout();
    tiles_grid->setSpacing(VRYO_DASH_SPACING);
    container_layout->addLayout(tiles_grid);
    container_layout->addStretch(1);

    scroll->setWidget(tiles_container);
    tiles_container->setAutoFillBackground(false);
    root->addWidget(scroll, 1);

    tiles.resize(TILE_COUNT, nullptr);
    tiles[TILE_TEMPERATURE]     = new VryoStatTile(tr("Temperature"),   tiles_container);
    tiles[TILE_LOAD]            = new VryoStatTile(tr("GPU Load"),      tiles_container);
    tiles[TILE_CORE_CLOCK]      = new VryoStatTile(tr("Core Clock"),    tiles_container);
    tiles[TILE_MEMORY_CLOCK]    = new VryoStatTile(tr("Memory Clock"),  tiles_container);
    tiles[TILE_FAN]             = new VryoStatTile(tr("Fan Speed"),     tiles_container);
    tiles[TILE_VIDEO_MEMORY]    = new VryoStatTile(tr("Video Memory"),  tiles_container);
    tiles[TILE_POWER]           = new VryoStatTile(tr("Power"),         tiles_container);

    Reflow();
}

/*---------------------------------------------------------*\
| Arrange the cards in as many columns as fit               |
\*---------------------------------------------------------*/
void VryoDashboardPage::Reflow()
{
    const int usable_width  = width() - (2 * VRYO_DASH_MARGIN);
    int       wanted        = (usable_width + VRYO_DASH_SPACING) / (VRYO_DASH_TILE_WIDTH + VRYO_DASH_SPACING);

    if(wanted < 1)
    {
        wanted = 1;
    }

    if(wanted > VRYO_DASH_MAX_COLUMNS)
    {
        wanted = VRYO_DASH_MAX_COLUMNS;
    }

    if((wanted == columns) && (tiles_grid->count() == (int)tiles.size()))
    {
        return;
    }

    columns = wanted;

    for(size_t i = 0; i < tiles.size(); i++)
    {
        tiles_grid->removeWidget(tiles[i]);
    }

    for(size_t i = 0; i < tiles.size(); i++)
    {
        tiles_grid->addWidget(tiles[i], (int)i / columns, (int)i % columns);
    }

    for(int column = 0; column < VRYO_DASH_MAX_COLUMNS; column++)
    {
        tiles_grid->setColumnStretch(column, (column < columns) ? 1 : 0);
    }
}

void VryoDashboardPage::StartDriver()
{
    driver_tried = true;

    for(size_t i = 0; i < tiles.size(); i++)
    {
        tiles[i]->ClearHistory();
    }

    gpu_picker->blockSignals(true);
    gpu_picker->clear();

    if(nvml.Start())
    {
        for(int i = 0; i < nvml.GpuCount(); i++)
        {
            gpu_picker->addItem(nvml.GpuName(i));
        }

        gpu_index = 0;
        gpu_picker->setCurrentIndex(0);
        ShowStatus(QString());
    }
    else
    {
        ShowStatus(nvml.StatusMessage());
    }

    gpu_picker->blockSignals(false);

    ShowHeader();
}

void VryoDashboardPage::ShowStatus(const QString& message)
{
    const bool has_message = !message.isEmpty();

    status_label->setText(message);
    status_label->setVisible(has_message);
    retry_button->setVisible(has_message);
    tiles_container->setVisible(!has_message);
}

void VryoDashboardPage::ShowHeader()
{
    if(nvml.IsReady())
    {
        const QString real_name     = nvml.GpuName(gpu_index);
        const QString series_name   = ResolveSeriesName(real_name, nvml.GpuCount() == 1);
        QStringList   details;

        /*-------------------------------------------------*\
        | A Vryo card shows its Vryo name as the title, with |
        | the real GPU and driver underneath                 |
        \*-------------------------------------------------*/
        gpu_name_label->setText(series_name.isEmpty() ? real_name : series_name);

        if(!series_name.isEmpty())
        {
            details << real_name;
        }

        if(!nvml.DriverVersion().isEmpty())
        {
            details << tr("NVIDIA driver %1").arg(nvml.DriverVersion());
        }

        driver_label->setText(details.join(QStringLiteral("  |  ")));

        gpu_picker->setVisible(nvml.GpuCount() > 1);
    }
    else
    {
        gpu_name_label->setText(tr("GPU Dashboard"));
        driver_label->clear();
        gpu_picker->setVisible(false);
    }
}

void VryoDashboardPage::SetTilesUnavailable()
{
    for(size_t i = 0; i < tiles.size(); i++)
    {
        tiles[i]->SetUnavailable();
    }
}

/*---------------------------------------------------------*\
| Read the GPU once and update every card                   |
\*---------------------------------------------------------*/
void VryoDashboardPage::Poll()
{
    if(!nvml.IsReady())
    {
        return;
    }

    const VryoGpuSample sample = nvml.Read(gpu_index);

    if(!sample.valid)
    {
        SetTilesUnavailable();
        return;
    }

    ApplySample(sample);
}

void VryoDashboardPage::ApplySample(const VryoGpuSample& sample)
{
    const QColor accent = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_ACCENT);
    const QColor warn   = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_WARN);
    const QColor hot    = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_HOT);

    /*-----------------------------------------------------*\
    | Temperature: 0 to 100 C on the bar. The colour slides |
    | from blue through amber to red as it heats up, and    |
    | from 60 C the number, border and glow follow it.      |
    \*-----------------------------------------------------*/
    if(sample.temperature_c >= 0)
    {
        const QColor    colour      = TemperatureColour(sample.temperature_c);
        const double    fraction    = Clamp01((double)sample.temperature_c / 100.0);
        const double    alert       = Clamp01((double)(sample.temperature_c - 60) / 25.0);

        tiles[TILE_TEMPERATURE]->SetReading(QString::number(sample.temperature_c), QString(QChar(0x00B0)) + QStringLiteral("C"), QString(), fraction, colour);
        tiles[TILE_TEMPERATURE]->SetAlert(alert);
        tiles[TILE_TEMPERATURE]->AddHistory(fraction);
    }
    else
    {
        tiles[TILE_TEMPERATURE]->SetAlert(0.0);
        tiles[TILE_TEMPERATURE]->SetUnavailable();
    }

    /*-----------------------------------------------------*\
    | Load                                                  |
    \*-----------------------------------------------------*/
    if(sample.load_percent >= 0)
    {
        const double fraction = Clamp01((double)sample.load_percent / 100.0);

        tiles[TILE_LOAD]->SetReading(QString::number(sample.load_percent), QStringLiteral("%"), QString(), fraction, accent);
        tiles[TILE_LOAD]->AddHistory(fraction);
    }
    else
    {
        tiles[TILE_LOAD]->SetUnavailable();
    }

    /*-----------------------------------------------------*\
    | Core clock                                            |
    \*-----------------------------------------------------*/
    if(sample.core_clock_mhz >= 0)
    {
        double  fraction    = -1.0;
        QString detail;

        if(sample.core_clock_max_mhz > 0)
        {
            fraction    = Clamp01((double)sample.core_clock_mhz / (double)sample.core_clock_max_mhz);
            detail      = tr("max %1 MHz").arg(sample.core_clock_max_mhz);
        }

        tiles[TILE_CORE_CLOCK]->SetReading(QString::number(sample.core_clock_mhz), QStringLiteral("MHz"), detail, fraction, accent);
        tiles[TILE_CORE_CLOCK]->AddHistory(fraction);
    }
    else
    {
        tiles[TILE_CORE_CLOCK]->SetUnavailable();
    }

    /*-----------------------------------------------------*\
    | Memory clock                                          |
    \*-----------------------------------------------------*/
    if(sample.memory_clock_mhz >= 0)
    {
        double  fraction    = -1.0;
        QString detail;

        if(sample.memory_clock_max_mhz > 0)
        {
            fraction    = Clamp01((double)sample.memory_clock_mhz / (double)sample.memory_clock_max_mhz);
            detail      = tr("max %1 MHz").arg(sample.memory_clock_max_mhz);
        }

        tiles[TILE_MEMORY_CLOCK]->SetReading(QString::number(sample.memory_clock_mhz), QStringLiteral("MHz"), detail, fraction, accent);
        tiles[TILE_MEMORY_CLOCK]->AddHistory(fraction);
    }
    else
    {
        tiles[TILE_MEMORY_CLOCK]->SetUnavailable();
    }

    /*-----------------------------------------------------*\
    | Fan                                                   |
    \*-----------------------------------------------------*/
    if(sample.fan_percent >= 0)
    {
        const double fraction = Clamp01((double)sample.fan_percent / 100.0);

        tiles[TILE_FAN]->SetReading(QString::number(sample.fan_percent), QStringLiteral("%"), QString(), fraction, accent);
        tiles[TILE_FAN]->AddHistory(fraction);
    }
    else
    {
        tiles[TILE_FAN]->SetUnavailable();
    }

    /*-----------------------------------------------------*\
    | Video memory: shown in GB (1 GB = 1024 MB, the same   |
    | way Windows shows it)                                 |
    \*-----------------------------------------------------*/
    if((sample.memory_used_mib >= 0.0) && (sample.memory_total_mib > 0.0))
    {
        const double    fraction    = Clamp01(sample.memory_used_mib / sample.memory_total_mib);
        const QString   used_text   = QString::number(sample.memory_used_mib / 1024.0, 'f', 1);
        const QString   total_text  = QString::number(sample.memory_total_mib / 1024.0, 'f', 0);
        QColor          colour      = accent;

        if(fraction >= 0.95)
        {
            colour = hot;
        }
        else if(fraction >= 0.85)
        {
            colour = warn;
        }

        tiles[TILE_VIDEO_MEMORY]->SetReading(used_text, QStringLiteral("GB"), tr("of %1 GB").arg(total_text), fraction, colour);
        tiles[TILE_VIDEO_MEMORY]->SetAlert((fraction >= 0.85) ? 1.0 : 0.0);
        tiles[TILE_VIDEO_MEMORY]->AddHistory(fraction);
    }
    else
    {
        tiles[TILE_VIDEO_MEMORY]->SetUnavailable();
    }

    /*-----------------------------------------------------*\
    | Power                                                 |
    \*-----------------------------------------------------*/
    if(sample.power_watts >= 0.0)
    {
        double  fraction    = -1.0;
        QString detail;
        QColor  colour      = accent;

        if(sample.power_limit_watts > 0.0)
        {
            fraction    = Clamp01(sample.power_watts / sample.power_limit_watts);
            detail      = tr("limit %1 W").arg(QString::number(sample.power_limit_watts, 'f', 0));

            if(fraction >= 0.98)
            {
                colour = hot;
            }
            else if(fraction >= 0.90)
            {
                colour = warn;
            }
        }

        tiles[TILE_POWER]->SetReading(QString::number(sample.power_watts, 'f', 0), QStringLiteral("W"), detail, fraction, colour);
        tiles[TILE_POWER]->SetAlert((fraction >= 0.90) ? 1.0 : 0.0);
        tiles[TILE_POWER]->AddHistory(fraction);
    }
    else
    {
        tiles[TILE_POWER]->SetUnavailable();
    }
}

void VryoDashboardPage::OnGpuPicked(int index)
{
    if((index < 0) || (index >= nvml.GpuCount()))
    {
        return;
    }

    gpu_index = index;

    for(size_t i = 0; i < tiles.size(); i++)
    {
        tiles[i]->ClearHistory();
    }

    ShowHeader();
    Poll();
}

void VryoDashboardPage::OnRetryClicked()
{
    StartDriver();
    Poll();
}

/*---------------------------------------------------------*\
| Only talk to the driver while this tab is on screen       |
\*---------------------------------------------------------*/
void VryoDashboardPage::showEvent(QShowEvent * event)
{
    QWidget::showEvent(event);

    if(!driver_tried)
    {
        StartDriver();
    }

    Poll();
    timer->start();

    PlayIntro();
}

/*---------------------------------------------------------*\
| The header fades in, then the cards pop in one after the  |
| other. The first time is the long version, later visits   |
| to the tab are quicker.                                   |
\*---------------------------------------------------------*/
void VryoDashboardPage::PlayIntro()
{
    const bool  first       = !intro_played;
    const int   start       = first ? 120 : 0;
    const int   stagger     = first ? 85 : 40;
    const int   duration    = first ? 650 : 400;

    intro_played = true;

    QGraphicsOpacityEffect * effect = new QGraphicsOpacityEffect(header_widget);
    effect->setOpacity(0.0);
    header_widget->setGraphicsEffect(effect);

    QPropertyAnimation * fade = new QPropertyAnimation(effect, "opacity", effect);
    fade->setDuration(first ? 700 : 350);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->setEasingCurve(QEasingCurve(QEasingCurve::OutCubic));

    /*-----------------------------------------------------*\
    | Remove the effect once it is done (deferred, so the   |
    | animation is not deleted while it is still emitting)  |
    \*-----------------------------------------------------*/
    QPointer<QGraphicsOpacityEffect> effect_guard(effect);

    connect(fade, &QPropertyAnimation::finished, header_widget, [this, effect_guard]()
    {
        QTimer::singleShot(0, header_widget, [this, effect_guard]()
        {
            if(!effect_guard.isNull() && (header_widget->graphicsEffect() == effect_guard.data()))
            {
                header_widget->setGraphicsEffect(nullptr);
            }
        });
    });

    fade->start();

    for(size_t i = 0; i < tiles.size(); i++)
    {
        tiles[i]->PlayIntro(start + ((int)i * stagger), duration);
    }
}

void VryoDashboardPage::hideEvent(QHideEvent * event)
{
    QWidget::hideEvent(event);

    timer->stop();
}

void VryoDashboardPage::resizeEvent(QResizeEvent * event)
{
    QWidget::resizeEvent(event);

    Reflow();
}
