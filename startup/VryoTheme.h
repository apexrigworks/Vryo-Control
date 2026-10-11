/*---------------------------------------------------------*\
| VryoTheme.h                                               |
|                                                           |
|   Vryo Control look: dark navy surfaces with a blue       |
|   accent. Every colour lives in the block below, so       |
|   matching the website means editing these lines only.    |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <algorithm>
#include <cmath>
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPen>
#include <QPixmap>
#include <QRadialGradient>
#include <QRectF>
#include <QString>
#include <QStyleFactory>

namespace VryoTheme
{
    /*-----------------------------------------------------*\
    | COLOURS: edit these to match the website              |
    \*-----------------------------------------------------*/
    // Sampled from the apexrigworks.co.uk home page: near-black page,
    // deep indigo hero. The accent is a brighter blue of that same hue
    // so it stays readable on dark surfaces.
    static const char * COLOR_BACKGROUND    = "#0D0E12";    // window background (site page colour)
    static const char * COLOR_PANEL         = "#14161D";    // panels, list areas
    static const char * COLOR_SURFACE       = "#1C1F2C";    // buttons, inputs, raised parts
    static const char * COLOR_BORDER        = "#2B2F4D";    // outlines
    static const char * COLOR_TEXT          = "#F2F4FF";    // main text
    static const char * COLOR_MUTED         = "#8A8FA8";    // secondary text, disabled
    static const char * COLOR_ACCENT        = "#4A55F5";    // main blue
    static const char * COLOR_ACCENT_LIGHT  = "#7C86FF";    // hover / highlights
    static const char * COLOR_ACCENT_DARK   = "#2F38C9";    // pressed
    static const char * COLOR_HERO          = "#01075D";    // deep indigo (site hero), splash top

    /*-----------------------------------------------------*\
    | Qt palette: this is what most of the window uses      |
    \*-----------------------------------------------------*/
    inline QPalette MakePalette()
    {
        QPalette palette;

        const QColor background(COLOR_BACKGROUND);
        const QColor panel(COLOR_PANEL);
        const QColor surface(COLOR_SURFACE);
        const QColor text(COLOR_TEXT);
        const QColor muted(COLOR_MUTED);
        const QColor accent(COLOR_ACCENT);

        palette.setColor(QPalette::Window,           background);
        palette.setColor(QPalette::WindowText,       text);
        palette.setColor(QPalette::Base,             panel);
        palette.setColor(QPalette::AlternateBase,    surface);
        palette.setColor(QPalette::ToolTipBase,      surface);
        palette.setColor(QPalette::ToolTipText,      text);
        palette.setColor(QPalette::Text,             text);
        palette.setColor(QPalette::Button,           surface);
        palette.setColor(QPalette::ButtonText,       text);
        palette.setColor(QPalette::BrightText,       QColor("#FFFFFF"));
        palette.setColor(QPalette::Link,             QColor(COLOR_ACCENT_LIGHT));
        palette.setColor(QPalette::Highlight,        accent);
        palette.setColor(QPalette::HighlightedText,  QColor("#FFFFFF"));
        palette.setColor(QPalette::PlaceholderText,  muted);

        palette.setColor(QPalette::Light,            QColor(COLOR_BORDER).lighter(130));
        palette.setColor(QPalette::Midlight,         QColor(COLOR_BORDER));
        palette.setColor(QPalette::Mid,              QColor(COLOR_BORDER).darker(130));
        palette.setColor(QPalette::Dark,             background.darker(150));
        palette.setColor(QPalette::Shadow,           QColor("#000000"));

        palette.setColor(QPalette::Disabled, QPalette::WindowText,  muted);
        palette.setColor(QPalette::Disabled, QPalette::Text,        muted);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText,  muted);
        palette.setColor(QPalette::Disabled, QPalette::Highlight,   QColor(COLOR_BORDER));

        return(palette);
    }

    /*-----------------------------------------------------*\
    | Style sheet: small touches on top of the palette.     |
    | Kept deliberately simple so it cannot break layouts.  |
    \*-----------------------------------------------------*/
    inline QString MakeStyleSheet()
    {
        QString css = QStringLiteral(
            "QToolTip { color: %TEXT%; background-color: %SURFACE%; border: 1px solid %ACCENT%; padding: 4px; }"

            "QPushButton { background-color: %SURFACE%; color: %TEXT%; border: 1px solid %BORDER%; border-radius: 10px; padding: 5px 14px; }"
            "QPushButton:hover { border-color: %ACCENT%; background-color: %PANEL%; }"
            "QPushButton:pressed { background-color: %ACCENT_DARK%; border-color: %ACCENT_DARK%; color: #FFFFFF; }"
            "QPushButton:disabled { color: %MUTED%; border-color: %BORDER%; background-color: %PANEL%; }"

            "QLineEdit, QSpinBox, QDoubleSpinBox, QPlainTextEdit, QTextEdit { background-color: %PANEL%; color: %TEXT%; border: 1px solid %BORDER%; border-radius: 5px; padding: 3px 6px; selection-background-color: %ACCENT%; }"
            "QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus { border-color: %ACCENT%; }"

            "QComboBox { background-color: %SURFACE%; color: %TEXT%; border: 1px solid %BORDER%; border-radius: 5px; padding: 3px 8px; }"
            "QComboBox:hover { border-color: %ACCENT%; }"
            "QComboBox QAbstractItemView { background-color: %PANEL%; color: %TEXT%; border: 1px solid %BORDER%; selection-background-color: %ACCENT%; selection-color: #FFFFFF; }"

            "QMenu { background-color: %PANEL%; color: %TEXT%; border: 1px solid %BORDER%; }"
            "QMenu::item:selected { background-color: %ACCENT%; color: #FFFFFF; }"

            "QScrollBar:vertical { background: %BACKGROUND%; width: 10px; margin: 0; }"
            "QScrollBar::handle:vertical { background: %BORDER%; border-radius: 5px; min-height: 24px; }"
            "QScrollBar::handle:vertical:hover { background: %ACCENT%; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
            "QScrollBar:horizontal { background: %BACKGROUND%; height: 10px; margin: 0; }"
            "QScrollBar::handle:horizontal { background: %BORDER%; border-radius: 5px; min-width: 24px; }"
            "QScrollBar::handle:horizontal:hover { background: %ACCENT%; }"
            "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"

            "QSlider::groove:horizontal { height: 4px; background: %BORDER%; border-radius: 2px; }"
            "QSlider::sub-page:horizontal { background: %ACCENT%; border-radius: 2px; }"
            "QSlider::handle:horizontal { background: %ACCENT_LIGHT%; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; }"
            "QSlider::handle:horizontal:hover { background: #FFFFFF; }"

            "QProgressBar { background-color: %PANEL%; border: 1px solid %BORDER%; border-radius: 5px; text-align: center; color: %TEXT%; }"
            "QProgressBar::chunk { background-color: %ACCENT%; border-radius: 4px; }"
        );

        css.replace("%BACKGROUND%",    COLOR_BACKGROUND);
        css.replace("%PANEL%",         COLOR_PANEL);
        css.replace("%SURFACE%",       COLOR_SURFACE);
        css.replace("%BORDER%",        COLOR_BORDER);
        css.replace("%TEXT%",          COLOR_TEXT);
        css.replace("%MUTED%",         COLOR_MUTED);
        css.replace("%ACCENT_LIGHT%",  COLOR_ACCENT_LIGHT);
        css.replace("%ACCENT_DARK%",   COLOR_ACCENT_DARK);
        css.replace("%ACCENT%",        COLOR_ACCENT);

        return(css);
    }

    /*-----------------------------------------------------*\
    | Apply the theme to the whole application              |
    \*-----------------------------------------------------*/
    inline void Apply(QApplication& app)
    {
        QStyle * fusion = QStyleFactory::create("Fusion");

        if(fusion != nullptr)
        {
            app.setStyle(fusion);
        }

        app.setPalette(MakePalette());
        app.setStyleSheet(MakeStyleSheet());
    }

    /*-----------------------------------------------------*\
    | Draw one frame of the loading screen.                 |
    |   angle     spinner rotation in degrees               |
    |   progress  0.0 to 1.0                                |
    |   phase     0.0 to 1.0, moves the bar shimmer         |
    \*-----------------------------------------------------*/
    inline QPixmap PaintSplash(const QPixmap& logo, double angle, double progress, double phase, const QString& message)
    {
        const int       width       = 560;
        const int       height      = 420;
        const qreal     scale       = 2.0;

        QPixmap pixmap((int)(width * scale), (int)(height * scale));
        pixmap.setDevicePixelRatio(scale);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const QColor accent(COLOR_ACCENT);
        const QColor accent_light(COLOR_ACCENT_LIGHT);

        /*-------------------------------------------------*\
        | Background and border                             |
        \*-------------------------------------------------*/
        QLinearGradient background(0, 0, 0, height);
        background.setColorAt(0.0, QColor(COLOR_HERO));
        background.setColorAt(1.0, QColor(COLOR_BACKGROUND));
        painter.fillRect(QRectF(0, 0, width, height), background);

        painter.setPen(QPen(QColor(COLOR_BORDER), 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(QRectF(0.5, 0.5, width - 1.0, height - 1.0));

        /*-------------------------------------------------*\
        | Soft blue glow behind the logo                    |
        \*-------------------------------------------------*/
        const QPointF   centre(width / 2.0, 140.0);
        QRadialGradient glow(centre, 170.0);
        QColor          glow_inner = accent;
        QColor          glow_outer = accent;
        glow_inner.setAlpha(70);
        glow_outer.setAlpha(0);
        glow.setColorAt(0.0, glow_inner);
        glow.setColorAt(1.0, glow_outer);
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow);
        painter.drawEllipse(centre, 170.0, 170.0);

        /*-------------------------------------------------*\
        | Logo                                              |
        \*-------------------------------------------------*/
        if(!logo.isNull())
        {
            QPixmap scaled = logo.scaled((int)(150 * scale), (int)(150 * scale), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            scaled.setDevicePixelRatio(scale);
            painter.drawPixmap(QPointF(centre.x() - 75.0, centre.y() - 75.0), scaled);
        }

        /*-------------------------------------------------*\
        | Spinner ring: faint full ring plus a bright arc   |
        \*-------------------------------------------------*/
        const qreal ring_radius = 100.0;
        QRectF      ring_rect(centre.x() - ring_radius, centre.y() - ring_radius, ring_radius * 2.0, ring_radius * 2.0);

        QColor ring_faint(COLOR_BORDER);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(ring_faint, 2.0));
        painter.drawEllipse(ring_rect);

        /*-------------------------------------------------*\
        | Bright head with a fading tail, turning clockwise |
        \*-------------------------------------------------*/
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

        /*-------------------------------------------------*\
        | Title and subtitle                                |
        \*-------------------------------------------------*/
        painter.setPen(QColor(COLOR_TEXT));
        QFont title_font("Segoe UI", 24, QFont::DemiBold);
        painter.setFont(title_font);
        painter.drawText(QRectF(0, 262, width, 44), Qt::AlignCenter, "Vryo Control");

        painter.setPen(QColor(COLOR_MUTED));
        QFont sub_font("Segoe UI", 10);
        painter.setFont(sub_font);
        painter.drawText(QRectF(0, 304, width, 24), Qt::AlignCenter, "RGB lighting control");

        /*-------------------------------------------------*\
        | Progress bar with a moving shimmer                |
        \*-------------------------------------------------*/
        const qreal bar_width   = 320.0;
        const qreal bar_height  = 6.0;
        const QRectF track((width - bar_width) / 2.0, 350.0, bar_width, bar_height);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(COLOR_SURFACE));
        painter.drawRoundedRect(track, bar_height / 2.0, bar_height / 2.0);

        double clamped = std::max(0.0, std::min(1.0, progress));
        QRectF fill = track;
        fill.setWidth(std::max(bar_height, track.width() * clamped));

        QLinearGradient fill_gradient(fill.left(), 0, fill.right(), 0);
        fill_gradient.setColorAt(0.0, QColor(COLOR_ACCENT_DARK));
        fill_gradient.setColorAt(1.0, accent_light);
        painter.setBrush(fill_gradient);
        painter.drawRoundedRect(fill, bar_height / 2.0, bar_height / 2.0);

        painter.save();
        QPainterPath clip;
        clip.addRoundedRect(fill, bar_height / 2.0, bar_height / 2.0);
        painter.setClipPath(clip);
        const qreal      shimmer_x = fill.left() - 60.0 + (fill.width() + 120.0) * phase;
        QLinearGradient  shimmer(shimmer_x, 0, shimmer_x + 60.0, 0);
        shimmer.setColorAt(0.0, QColor(255, 255, 255, 0));
        shimmer.setColorAt(0.5, QColor(255, 255, 255, 110));
        shimmer.setColorAt(1.0, QColor(255, 255, 255, 0));
        painter.setBrush(shimmer);
        painter.drawRect(QRectF(shimmer_x, fill.top(), 60.0, fill.height()));
        painter.restore();

        /*-------------------------------------------------*\
        | Status message                                    |
        \*-------------------------------------------------*/
        painter.setPen(QColor(COLOR_MUTED));
        QFont msg_font("Segoe UI", 9);
        painter.setFont(msg_font);
        painter.drawText(QRectF(0, 366, width, 24), Qt::AlignCenter, message);

        painter.end();

        return(pixmap);
    }
}
