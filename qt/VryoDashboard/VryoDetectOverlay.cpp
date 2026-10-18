/*---------------------------------------------------------*\
| VryoDetectOverlay.cpp                                     |
|                                                           |
|   The "Detecting RGB compatible hardware" screen.         |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "VryoDetectOverlay.h"

#include <algorithm>
#include <cmath>

#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>

#include "ResourceManager.h"
#include "RGBController.h"
#include "../../startup/VryoTheme.h"

namespace
{
    /*-----------------------------------------------------*\
    | Timing, in milliseconds                               |
    \*-----------------------------------------------------*/
    const int       VRYO_DETECT_FRAME_MS        = 16;       // about 60 frames a second
    const qint64    VRYO_DETECT_MIN_MS          = 1800;     // the spinner shows at least this long
    const qint64    VRYO_DETECT_MAX_MS          = 12000;    // and gives up waiting for detection after this
    const qint64    VRYO_DETECT_HAND_OVER_MS    = 250;      // spinner fades out before the names start
    const qint64    VRYO_NAME_SLOT_MS           = 1700;     // each device name stays this long
    const qint64    VRYO_NAME_FADE_IN_MS        = 350;
    const qint64    VRYO_NAME_FADE_OUT_MS       = 250;
    const qint64    VRYO_FINAL_FADE_MS          = 700;      // the whole screen fades away over this
    const int       VRYO_MAX_NAMES              = 4;        // more devices than this are summed up
    const qint64    VRYO_SPIN_PERIOD_MS         = 1400;

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
    | The largest font (in pixels) that still fits the text |
    | inside max_width, never smaller than min_pixels       |
    \*-----------------------------------------------------*/
    QFont FitFont(const QString& text, int max_pixels, int min_pixels, int max_width, QFont::Weight weight)
    {
        QFont font(QStringLiteral("Segoe UI"));
        font.setWeight(weight);

        for(int size = max_pixels; size > min_pixels; size -= 2)
        {
            font.setPixelSize(size);

            if(QFontMetrics(font).horizontalAdvance(text) <= max_width)
            {
                return(font);
            }
        }

        font.setPixelSize(min_pixels);

        return(font);
    }
}

VryoDetectOverlay::VryoDetectOverlay(QWidget * parent) :
    QWidget(parent)
{
    hide();

    timer.setTimerType(Qt::PreciseTimer);
    timer.setInterval(VRYO_DETECT_FRAME_MS);

    connect(&timer, &QTimer::timeout, this, &VryoDetectOverlay::Tick);

    /*-----------------------------------------------------*\
    | Follow the parent's size so the overlay always covers |
    | the whole tab                                         |
    \*-----------------------------------------------------*/
    if(parent != nullptr)
    {
        parent->installEventFilter(this);
    }
}

VryoDetectOverlay::~VryoDetectOverlay()
{
    timer.stop();
}

bool VryoDetectOverlay::eventFilter(QObject * watched, QEvent * event)
{
    if((watched == parentWidget()) && (event->type() == QEvent::Resize) && (stage != STAGE_IDLE))
    {
        FitToParent();
    }

    return(QWidget::eventFilter(watched, event));
}

void VryoDetectOverlay::FitToParent()
{
    QWidget * host = parentWidget();

    if((host != nullptr) && (geometry() != host->rect()))
    {
        setGeometry(host->rect());
    }
}

bool VryoDetectOverlay::DetectionDone() const
{
    return(ResourceManager::get()->GetDetectionPercent() >= 100);
}

void VryoDetectOverlay::Start()
{
    names.clear();
    extra_devices       = 0;
    none_found          = false;
    names_frozen_ms     = 0;

    stage               = STAGE_DETECTING;
    ready_at_ms         = -1;
    progress            = 0.0;
    master_opacity      = 1.0;
    angle               = 0.0;

    clock.start();
    stage_start_ms      = 0;

    FitToParent();
    raise();
    show();

    timer.start();
    update();
}

void VryoDetectOverlay::Skip()
{
    if((stage == STAGE_DETECTING) || (stage == STAGE_NAMES))
    {
        EnterFading();
    }
}

void VryoDetectOverlay::mousePressEvent(QMouseEvent * event)
{
    Skip();

    event->accept();
}

void VryoDetectOverlay::EnterNames()
{
    names.clear();
    extra_devices   = 0;
    none_found      = false;

    std::vector<RGBController *>& controllers = ResourceManager::get()->GetRGBControllers();

    for(size_t controller_idx = 0; controller_idx < controllers.size(); controller_idx++)
    {
        if(controllers[controller_idx]->GetHidden())
        {
            continue;
        }

        if(names.size() < VRYO_MAX_NAMES)
        {
            names << QString::fromStdString(controllers[controller_idx]->GetDisplayName());
        }
        else
        {
            extra_devices++;
        }
    }

    if(names.isEmpty())
    {
        none_found = true;
        names << QStringLiteral("No RGB hardware found");
    }

    stage           = STAGE_NAMES;
    stage_start_ms  = clock.elapsed();
}

void VryoDetectOverlay::EnterFading()
{
    const qint64 now = clock.elapsed();

    /*-----------------------------------------------------*\
    | Freeze the name screen where it is so it can fade     |
    | out as it stands (a natural end freezes it on the     |
    | last name, fully shown)                               |
    \*-----------------------------------------------------*/
    if(stage == STAGE_NAMES)
    {
        const qint64 total_ms = (qint64)names.size() * VRYO_NAME_SLOT_MS;

        names_frozen_ms = (std::min)(now - stage_start_ms, total_ms - 1);
    }
    else
    {
        names.clear();
    }

    stage           = STAGE_FADING;
    stage_start_ms  = now;
}

void VryoDetectOverlay::Tick()
{
    if(stage == STAGE_IDLE)
    {
        timer.stop();
        return;
    }

    FitToParent();

    const qint64 now        = clock.elapsed();
    const qint64 in_stage   = now - stage_start_ms;

    angle = (double)(now % VRYO_SPIN_PERIOD_MS) / (double)VRYO_SPIN_PERIOD_MS * 360.0;

    if(stage == STAGE_DETECTING)
    {
        const bool ready = (in_stage >= VRYO_DETECT_MIN_MS) && (DetectionDone() || (in_stage >= VRYO_DETECT_MAX_MS));

        if(ready && (ready_at_ms < 0))
        {
            ready_at_ms = now;
        }

        if(ready_at_ms >= 0)
        {
            progress = 1.0;

            if((now - ready_at_ms) >= VRYO_DETECT_HAND_OVER_MS)
            {
                EnterNames();
            }
        }
        else if(DetectionDone())
        {
            progress = Clamp01((double)in_stage / (double)VRYO_DETECT_MIN_MS);
        }
        else
        {
            progress = (std::min)(0.95, (double)ResourceManager::get()->GetDetectionPercent() / 100.0);
        }
    }
    else if(stage == STAGE_NAMES)
    {
        if(in_stage >= ((qint64)names.size() * VRYO_NAME_SLOT_MS))
        {
            EnterFading();
        }
    }
    else if(stage == STAGE_FADING)
    {
        master_opacity = 1.0 - Clamp01((double)in_stage / (double)VRYO_FINAL_FADE_MS);

        if(in_stage >= VRYO_FINAL_FADE_MS)
        {
            timer.stop();
            stage           = STAGE_IDLE;
            master_opacity  = 1.0;
            hide();

            emit Finished();
            return;
        }
    }

    update();
}

void VryoDetectOverlay::paintEvent(QPaintEvent *)
{
    if(stage == STAGE_IDLE)
    {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setOpacity(master_opacity);

    PaintBackground(painter);

    const qint64 now        = clock.elapsed();
    const qint64 in_stage   = now - stage_start_ms;

    if(stage == STAGE_DETECTING)
    {
        double opacity = 1.0;

        if(ready_at_ms >= 0)
        {
            opacity = 1.0 - Clamp01((double)(now - ready_at_ms) / (double)VRYO_DETECT_HAND_OVER_MS);
        }

        PaintDetecting(painter, opacity);
    }
    else if(stage == STAGE_NAMES)
    {
        PaintNames(painter, in_stage);
    }
    else if(stage == STAGE_FADING)
    {
        if(names.isEmpty())
        {
            PaintDetecting(painter, 1.0);
        }
        else
        {
            PaintNames(painter, names_frozen_ms);
        }
    }
}

void VryoDetectOverlay::PaintBackground(QPainter& painter)
{
    QLinearGradient background(0.0, 0.0, 0.0, (qreal)height());
    background.setColorAt(0.0, VryoTheme::HexColor(VryoTheme::VRYO_COLOR_HERO));
    background.setColorAt(1.0, VryoTheme::HexColor(VryoTheme::VRYO_COLOR_BACKGROUND));
    painter.fillRect(rect(), background);

    /*-----------------------------------------------------*\
    | Soft blue glow in the middle                          |
    \*-----------------------------------------------------*/
    const QPointF   centre((qreal)width() / 2.0, (qreal)height() * 0.42);
    const qreal     radius = (qreal)(std::max)(width(), height()) * 0.45;

    QColor glow_inner = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_ACCENT);
    QColor glow_outer = glow_inner;
    glow_inner.setAlpha(60);
    glow_outer.setAlpha(0);

    QRadialGradient glow(centre, radius);
    glow.setColorAt(0.0, glow_inner);
    glow.setColorAt(1.0, glow_outer);

    painter.setPen(Qt::NoPen);
    painter.setBrush(glow);
    painter.drawEllipse(centre, radius, radius);
}

void VryoDetectOverlay::PaintDetecting(QPainter& painter, double opacity)
{
    painter.save();
    painter.setOpacity(master_opacity * opacity);

    const QColor accent_light = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_ACCENT_LIGHT);

    const qreal     w               = (qreal)width();
    const qreal     h               = (qreal)height();
    const qreal     ring_radius     = (std::max)((qreal)28.0, (std::min)((qreal)64.0, (std::min)(w, h) * (qreal)0.12));
    const QPointF   centre(w / 2.0, h * 0.38);
    const QRectF    ring_rect(centre.x() - ring_radius, centre.y() - ring_radius, ring_radius * 2.0, ring_radius * 2.0);

    /*-----------------------------------------------------*\
    | Spinner: faint ring and a bright head with a tail     |
    \*-----------------------------------------------------*/
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(VryoTheme::HexColor(VryoTheme::VRYO_COLOR_BORDER), 2.0));
    painter.drawEllipse(ring_rect);

    const int   tail_segments   = 16;
    const qreal segment_degrees = 8.0;
    const qreal head_degrees    = 90.0 - angle;

    for(int segment = 0; segment < tail_segments; segment++)
    {
        QColor segment_color = accent_light;
        segment_color.setAlphaF(1.0 - ((qreal)segment / (qreal)tail_segments));

        QPen segment_pen(segment_color, 3.5);
        segment_pen.setCapStyle(Qt::FlatCap);
        painter.setPen(segment_pen);

        const qreal start_degrees = head_degrees + (segment * segment_degrees);
        painter.drawArc(ring_rect, (int)(start_degrees * 16.0), (int)((segment_degrees + 0.6) * 16.0));
    }

    /*-----------------------------------------------------*\
    | Title and a quiet line under it                       |
    \*-----------------------------------------------------*/
    const QString title = QStringLiteral("Detecting RGB compatible hardware");
    const QFont   title_font = FitFont(title, (std::max)(16, (int)(h / 16.0)), 14, (int)(w * 0.9), QFont::DemiBold);

    painter.setFont(title_font);
    painter.setPen(VryoTheme::HexColor(VryoTheme::VRYO_COLOR_TEXT));

    const qreal title_top = centre.y() + ring_radius + 24.0;
    painter.drawText(QRectF(0.0, title_top, w, 40.0), Qt::AlignCenter, title);

    QFont sub_font(QStringLiteral("Segoe UI"));
    sub_font.setPixelSize(13);
    painter.setFont(sub_font);
    painter.setPen(VryoTheme::HexColor(VryoTheme::VRYO_COLOR_MUTED));
    painter.drawText(QRectF(0.0, title_top + 40.0, w, 22.0), Qt::AlignCenter, QStringLiteral("Looking for lighting devices..."));

    /*-----------------------------------------------------*\
    | Progress bar                                          |
    \*-----------------------------------------------------*/
    const qreal bar_width   = (std::min)((qreal)280.0, w * 0.6);
    const qreal bar_height  = 6.0;
    const QRectF track((w - bar_width) / 2.0, title_top + 78.0, bar_width, bar_height);

    painter.setPen(Qt::NoPen);
    painter.setBrush(VryoTheme::HexColor(VryoTheme::VRYO_COLOR_SURFACE));
    painter.drawRoundedRect(track, bar_height / 2.0, bar_height / 2.0);

    QRectF fill = track;
    fill.setWidth((std::max)(bar_height, track.width() * Clamp01(progress)));

    QLinearGradient fill_gradient(fill.left(), 0.0, fill.right(), 0.0);
    fill_gradient.setColorAt(0.0, VryoTheme::HexColor(VryoTheme::VRYO_COLOR_ACCENT_DARK));
    fill_gradient.setColorAt(1.0, accent_light);
    painter.setBrush(fill_gradient);
    painter.drawRoundedRect(fill, bar_height / 2.0, bar_height / 2.0);

    painter.restore();
}

void VryoDetectOverlay::PaintNames(QPainter& painter, qint64 elapsed_ms)
{
    if(names.isEmpty())
    {
        return;
    }

    /*-----------------------------------------------------*\
    | Which name is showing, and how far into its time      |
    \*-----------------------------------------------------*/
    int name_idx = (int)(elapsed_ms / VRYO_NAME_SLOT_MS);

    if(name_idx > (names.size() - 1))
    {
        name_idx = names.size() - 1;
    }

    const qint64    local_ms    = elapsed_ms - ((qint64)name_idx * VRYO_NAME_SLOT_MS);
    const bool      last_name   = (name_idx == (names.size() - 1));

    const double    fade_in     = Clamp01((double)local_ms / (double)VRYO_NAME_FADE_IN_MS);
    double          fade_out    = 1.0;

    if(!last_name)
    {
        fade_out = 1.0 - Clamp01((double)(local_ms - (VRYO_NAME_SLOT_MS - VRYO_NAME_FADE_OUT_MS)) / (double)VRYO_NAME_FADE_OUT_MS);
    }

    /*-----------------------------------------------------*\
    | Ease the name in with a short upward slide            |
    \*-----------------------------------------------------*/
    const double    eased       = 1.0 - ((1.0 - fade_in) * (1.0 - fade_in) * (1.0 - fade_in));
    const double    slide       = (1.0 - eased) * 14.0;

    painter.save();
    painter.setOpacity(master_opacity * fade_in * fade_out);

    const qreal w = (qreal)width();
    const qreal h = (qreal)height();

    /*-----------------------------------------------------*\
    | Small label above the name                            |
    \*-----------------------------------------------------*/
    const qreal name_top = (h * 0.40) + slide;

    QFont eyebrow_font(QStringLiteral("Segoe UI"));
    eyebrow_font.setPixelSize(13);
    eyebrow_font.setWeight(QFont::DemiBold);
    painter.setFont(eyebrow_font);
    painter.setPen(VryoTheme::HexColor(VryoTheme::VRYO_COLOR_ACCENT_LIGHT));
    painter.drawText(QRectF(0.0, name_top - 30.0, w, 22.0), Qt::AlignCenter, none_found ? QStringLiteral("RGB HARDWARE") : QStringLiteral("RGB DEVICE FOUND"));

    /*-----------------------------------------------------*\
    | The device name, as large as fits                     |
    \*-----------------------------------------------------*/
    const QString name      = names.at(name_idx);
    const QFont   name_font = FitFont(name, (std::max)(28, (int)(h / 7.0)), 18, (int)(w * 0.88), QFont::DemiBold);

    painter.setFont(name_font);
    painter.setPen(VryoTheme::HexColor(VryoTheme::VRYO_COLOR_TEXT));

    const qreal name_height = (qreal)QFontMetrics(name_font).height() + 8.0;
    painter.drawText(QRectF(0.0, name_top, w, name_height), Qt::AlignCenter, name);

    /*-----------------------------------------------------*\
    | Short blue bar under the name                         |
    \*-----------------------------------------------------*/
    const QRectF bar((w - 64.0) / 2.0, name_top + name_height + 8.0, 64.0, 3.0);

    QLinearGradient bar_gradient(bar.left(), 0.0, bar.right(), 0.0);
    bar_gradient.setColorAt(0.0, VryoTheme::HexColor(VryoTheme::VRYO_COLOR_ACCENT_DARK));
    bar_gradient.setColorAt(1.0, VryoTheme::HexColor(VryoTheme::VRYO_COLOR_ACCENT_LIGHT));

    painter.setPen(Qt::NoPen);
    painter.setBrush(bar_gradient);
    painter.drawRoundedRect(bar, 1.5, 1.5);

    /*-----------------------------------------------------*\
    | Quiet line under that                                 |
    \*-----------------------------------------------------*/
    QString note;

    if(none_found)
    {
        note = QStringLiteral("Check the connections, then press Rescan Devices.");
    }
    else
    {
        const int total_found = names.size() + extra_devices;

        if(total_found > 1)
        {
            note = QStringLiteral("Device %1 of %2").arg(name_idx + 1).arg(total_found);

            if(last_name && (extra_devices > 0))
            {
                note += QStringLiteral("  (+%1 more)").arg(extra_devices);
            }
        }
    }

    if(!note.isEmpty())
    {
        QFont note_font(QStringLiteral("Segoe UI"));
        note_font.setPixelSize(13);
        painter.setFont(note_font);
        painter.setPen(VryoTheme::HexColor(VryoTheme::VRYO_COLOR_MUTED));
        painter.drawText(QRectF(0.0, bar.bottom() + 12.0, w, 22.0), Qt::AlignCenter, note);
    }

    painter.restore();
}
