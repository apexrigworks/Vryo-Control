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
    | Colour from a "#RRGGBB" string. Written out in full so |
    | it never depends on implicit char* to QString casts.  |
    \*-----------------------------------------------------*/
    inline QColor HexColor(const char * hex)
    {
        return(QColor(QString::fromLatin1(hex)));
    }

    /*-----------------------------------------------------*\
    | COLOURS: edit these to match the website              |
    \*-----------------------------------------------------*/
    // Sampled from the apexrigworks.co.uk home page: near-black page,
    // deep indigo hero. The accent is a brighter blue of that same hue
    // so it stays readable on dark surfaces.
    static const char * VRYO_COLOR_BACKGROUND    = "#0D0E12";    // window background (site page colour)
    static const char * VRYO_COLOR_PANEL         = "#14161D";    // panels, list areas
    static const char * VRYO_COLOR_SURFACE       = "#1C1F2C";    // buttons, inputs, raised parts
    static const char * VRYO_COLOR_BORDER        = "#2B2F4D";    // outlines
    static const char * VRYO_COLOR_TEXT          = "#F2F4FF";    // main text
    static const char * VRYO_COLOR_MUTED         = "#8A8FA8";    // secondary text, disabled
    static const char * VRYO_COLOR_ACCENT        = "#4A55F5";    // main blue
    static const char * VRYO_COLOR_ACCENT_LIGHT  = "#7C86FF";    // hover / highlights
    static const char * VRYO_COLOR_ACCENT_DARK   = "#2F38C9";    // pressed
    static const char * VRYO_COLOR_HERO          = "#01075D";    // deep indigo (site hero), splash top
    static const char * VRYO_COLOR_WARN          = "#F5A623";    // dashboard: getting warm / busy
    static const char * VRYO_COLOR_HOT           = "#F0524F";    // dashboard: hot / at the limit

    /*-----------------------------------------------------*\
    | Qt palette: this is what most of the window uses      |
    \*-----------------------------------------------------*/
    inline QPalette MakePalette()
    {
        QPalette palette;

        const QColor background = HexColor(VRYO_COLOR_BACKGROUND);
        const QColor panel = HexColor(VRYO_COLOR_PANEL);
        const QColor surface = HexColor(VRYO_COLOR_SURFACE);
        const QColor text = HexColor(VRYO_COLOR_TEXT);
        const QColor muted = HexColor(VRYO_COLOR_MUTED);
        const QColor accent = HexColor(VRYO_COLOR_ACCENT);

        palette.setColor(QPalette::Window,           background);
        palette.setColor(QPalette::WindowText,       text);
        palette.setColor(QPalette::Base,             panel);
        palette.setColor(QPalette::AlternateBase,    surface);
        palette.setColor(QPalette::ToolTipBase,      surface);
        palette.setColor(QPalette::ToolTipText,      text);
        palette.setColor(QPalette::Text,             text);
        palette.setColor(QPalette::Button,           surface);
        palette.setColor(QPalette::ButtonText,       text);
        palette.setColor(QPalette::BrightText,       HexColor("#FFFFFF"));
        palette.setColor(QPalette::Link,             HexColor(VRYO_COLOR_ACCENT_LIGHT));
        palette.setColor(QPalette::Highlight,        accent);
        palette.setColor(QPalette::HighlightedText,  HexColor("#FFFFFF"));
        palette.setColor(QPalette::PlaceholderText,  muted);

        palette.setColor(QPalette::Light,            HexColor(VRYO_COLOR_BORDER).lighter(130));
        palette.setColor(QPalette::Midlight,         HexColor(VRYO_COLOR_BORDER));
        palette.setColor(QPalette::Mid,              HexColor(VRYO_COLOR_BORDER).darker(130));
        palette.setColor(QPalette::Dark,             background.darker(150));
        palette.setColor(QPalette::Shadow,           HexColor("#000000"));

        palette.setColor(QPalette::Disabled, QPalette::WindowText,  muted);
        palette.setColor(QPalette::Disabled, QPalette::Text,        muted);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText,  muted);
        palette.setColor(QPalette::Disabled, QPalette::Highlight,   HexColor(VRYO_COLOR_BORDER));

        return(palette);
    }

    /*-----------------------------------------------------*\
    | Style sheet: small touches on top of the palette.     |
    | Kept deliberately simple so it cannot break layouts.  |
    \*-----------------------------------------------------*/
    inline QString MakeStyleSheet()
    {
        /*-------------------------------------------------*\
        | Built in several pieces: one very long string      |
        | literal can hit a compiler limit.                  |
        \*-------------------------------------------------*/
        QString css;

        /*-------------------------------------------------*\
        | Basics: tooltips, buttons, inputs, menus           |
        \*-------------------------------------------------*/
        css += QStringLiteral(
            "QToolTip { color: %TEXT%; background-color: %SURFACE%; border: 1px solid %ACCENT%; padding: 4px; }"

            "QPushButton { background-color: %SURFACE%; color: %TEXT%; border: 1px solid %BORDER%; border-radius: 10px; padding: 6px 16px; }"
            "QPushButton:hover { border-color: %ACCENT_LIGHT%; background-color: %PANEL%; }"
            "QPushButton:pressed { background-color: %ACCENT_DARK%; border-color: %ACCENT_DARK%; color: #FFFFFF; }"
            "QPushButton:disabled { color: %MUTED%; border-color: %BORDER%; background-color: %PANEL%; }"

            "QLineEdit, QSpinBox, QDoubleSpinBox, QPlainTextEdit, QTextEdit { background-color: %PANEL%; color: %TEXT%; border: 1px solid %BORDER%; border-radius: 8px; padding: 4px 8px; selection-background-color: %ACCENT%; }"
            "QLineEdit:hover, QSpinBox:hover, QDoubleSpinBox:hover { border-color: %ACCENT_DARK%; }"
            "QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus { border-color: %ACCENT_LIGHT%; }"

            "QComboBox { background-color: %SURFACE%; color: %TEXT%; border: 1px solid %BORDER%; border-radius: 8px; padding: 5px 10px; }"
            "QComboBox:hover { border-color: %ACCENT_LIGHT%; }"
            "QComboBox QAbstractItemView { background-color: %PANEL%; color: %TEXT%; border: 1px solid %BORDER%; selection-background-color: %ACCENT%; selection-color: #FFFFFF; outline: none; }"

            "QMenu { background-color: %PANEL%; color: %TEXT%; border: 1px solid %BORDER%; padding: 4px; }"
            "QMenu::item { padding: 6px 22px; border-radius: 6px; }"
            "QMenu::item:selected { background-color: %ACCENT%; color: #FFFFFF; }"
        );

        /*-------------------------------------------------*\
        | Scroll bars and progress                           |
        \*-------------------------------------------------*/
        css += QStringLiteral(
            "QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }"
            "QScrollBar::handle:vertical { background: %BORDER%; border-radius: 5px; min-height: 24px; }"
            "QScrollBar::handle:vertical:hover { background: %ACCENT%; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
            "QScrollBar:horizontal { background: transparent; height: 10px; margin: 0; }"
            "QScrollBar::handle:horizontal { background: %BORDER%; border-radius: 5px; min-width: 24px; }"
            "QScrollBar::handle:horizontal:hover { background: %ACCENT%; }"
            "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"

            "QProgressBar { background-color: %PANEL%; border: 1px solid %BORDER%; border-radius: 6px; text-align: center; color: %TEXT%; }"
            "QProgressBar::chunk { background-color: %ACCENT%; border-radius: 5px; }"
        );

        /*-------------------------------------------------*\
        | Sliders: blue gradient fill, round glowing handle  |
        \*-------------------------------------------------*/
        css += QStringLiteral(
            "QSlider::groove:horizontal { height: 6px; background: %BORDER%; border-radius: 3px; }"
            "QSlider::sub-page:horizontal { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %ACCENT_DARK%, stop:1 %ACCENT_LIGHT%); border-radius: 3px; }"
            "QSlider::handle:horizontal { background: %TEXT%; border: 3px solid %ACCENT%; width: 12px; height: 12px; margin: -6px 0px; border-radius: 9px; }"
            "QSlider::handle:horizontal:hover { border-color: %ACCENT_LIGHT%; }"
            "QSlider::handle:horizontal:pressed { background: %ACCENT_LIGHT%; border-color: #FFFFFF; }"
        );

        /*-------------------------------------------------*\
        | Radio buttons and check boxes                      |
        \*-------------------------------------------------*/
        css += QStringLiteral(
            "QRadioButton, QCheckBox { spacing: 8px; }"
            "QRadioButton::indicator { width: 14px; height: 14px; border-radius: 9px; border: 2px solid %BORDER%; background: %PANEL%; }"
            "QRadioButton::indicator:hover { border-color: %ACCENT_LIGHT%; }"
            "QRadioButton::indicator:checked { border-color: %ACCENT%; background: qradialgradient(cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5, stop:0 %ACCENT_LIGHT%, stop:0.45 %ACCENT_LIGHT%, stop:0.55 %PANEL%, stop:1 %PANEL%); }"
            "QCheckBox::indicator { width: 14px; height: 14px; border-radius: 6px; border: 2px solid %BORDER%; background: %PANEL%; }"
            "QCheckBox::indicator:hover { border-color: %ACCENT_LIGHT%; }"
            "QCheckBox::indicator:checked { border-color: %ACCENT%; background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %ACCENT_LIGHT%, stop:1 %ACCENT%); }"
        );

        /*-------------------------------------------------*\
        | Tabs. Main tabs are pills with a blue gradient on  |
        | the selected one; device tabs are small cards; the |
        | vertical Information and Settings tabs get a blue  |
        | marker on the left.                                |
        \*-------------------------------------------------*/
        css += QStringLiteral(
            "QTabWidget::pane { border: none; background: transparent; }"
            "QTabBar { qproperty-drawBase: 0; }"

            "QTabWidget#MainTabBar > QTabBar::tab { background: transparent; color: %MUTED%; padding: 9px 22px; margin: 0px 6px 8px 0px; border: 1px solid transparent; border-radius: 11px; font-weight: 600; }"
            "QTabWidget#MainTabBar > QTabBar::tab:hover { background: %SURFACE%; color: %TEXT%; }"
            "QTabWidget#MainTabBar > QTabBar::tab:selected { color: #FFFFFF; border: 1px solid %ACCENT_LIGHT%; background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %ACCENT%, stop:1 %ACCENT_DARK%); }"

            "QTabWidget#DevicesTabBar > QTabBar::tab { background: %PANEL%; color: %TEXT%; border: 1px solid %BORDER%; border-radius: 11px; padding: 6px 12px; margin: 0px 6px 8px 0px; }"
            "QTabWidget#DevicesTabBar > QTabBar::tab:hover { border-color: %ACCENT_LIGHT%; }"
            "QTabWidget#DevicesTabBar > QTabBar::tab:selected { border: 1px solid %ACCENT%; background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %SURFACE%, stop:1 %PANEL%); }"

            "QTabWidget#InformationTabBar > QTabBar::tab, QTabWidget#SettingsTabBar > QTabBar::tab { background: transparent; border: none; border-left: 3px solid transparent; border-radius: 8px; padding: 8px 8px; margin: 2px 8px 2px 0px; }"
            "QTabWidget#InformationTabBar > QTabBar::tab:hover, QTabWidget#SettingsTabBar > QTabBar::tab:hover { background: %PANEL%; }"
            "QTabWidget#InformationTabBar > QTabBar::tab:selected, QTabWidget#SettingsTabBar > QTabBar::tab:selected { background: %SURFACE%; border-left: 3px solid %ACCENT%; }"
        );

        /*-------------------------------------------------*\
        | RGB page: colour picker and controls become cards, |
        | the main actions are filled blue                   |
        \*-------------------------------------------------*/
        css += QStringLiteral(
            "QFrame#ColorFrame, QFrame#ControlsFrame { background-color: %PANEL%; border: 1px solid %BORDER%; border-radius: 14px; }"
            "QFrame#ColorEntryFrame { background: transparent; border: none; }"

            "QScrollArea#DeviceViewBoxFrame { background-color: %PANEL%; border: 1px solid %BORDER%; border-radius: 14px; }"
            "QScrollArea#DeviceViewBoxFrame QWidget#DeviceViewBoxFrameContents { background: transparent; }"

            "QPushButton#ApplyColorsButton, QPushButton#DeviceSaveButton, QPushButton#SetAllButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %ACCENT%, stop:1 %ACCENT_DARK%); border: 1px solid %ACCENT_LIGHT%; color: #FFFFFF; font-weight: 600; padding: 8px 18px; }"
            "QPushButton#ApplyColorsButton:hover, QPushButton#DeviceSaveButton:hover, QPushButton#SetAllButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %ACCENT_LIGHT%, stop:1 %ACCENT%); border-color: #FFFFFF; }"
            "QPushButton#ApplyColorsButton:pressed, QPushButton#DeviceSaveButton:pressed, QPushButton#SetAllButton:pressed { background: %ACCENT_DARK%; border-color: %ACCENT_DARK%; }"

            "QFrame#MainButtonsFrame QToolButton { background-color: %SURFACE%; color: %TEXT%; border: 1px solid %BORDER%; border-radius: 10px; padding: 6px 12px; }"
            "QFrame#MainButtonsFrame QToolButton:hover { border-color: %ACCENT_LIGHT%; background-color: %PANEL%; }"
            "QFrame#MainButtonsFrame QToolButton:pressed { background-color: %ACCENT_DARK%; border-color: %ACCENT_DARK%; }"
        );

        css.replace(QStringLiteral("%BACKGROUND%"), QString::fromLatin1(VRYO_COLOR_BACKGROUND));
        css.replace(QStringLiteral("%PANEL%"), QString::fromLatin1(VRYO_COLOR_PANEL));
        css.replace(QStringLiteral("%SURFACE%"), QString::fromLatin1(VRYO_COLOR_SURFACE));
        css.replace(QStringLiteral("%BORDER%"), QString::fromLatin1(VRYO_COLOR_BORDER));
        css.replace(QStringLiteral("%TEXT%"), QString::fromLatin1(VRYO_COLOR_TEXT));
        css.replace(QStringLiteral("%MUTED%"), QString::fromLatin1(VRYO_COLOR_MUTED));
        css.replace(QStringLiteral("%ACCENT_LIGHT%"), QString::fromLatin1(VRYO_COLOR_ACCENT_LIGHT));
        css.replace(QStringLiteral("%ACCENT_DARK%"), QString::fromLatin1(VRYO_COLOR_ACCENT_DARK));
        css.replace(QStringLiteral("%ACCENT%"), QString::fromLatin1(VRYO_COLOR_ACCENT));

        return(css);
    }

    /*-----------------------------------------------------*\
    | Apply the theme to the whole application              |
    \*-----------------------------------------------------*/
    inline void Apply(QApplication& app)
    {
        QStyle * fusion = QStyleFactory::create(QStringLiteral("Fusion"));

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

        const QColor accent = HexColor(VRYO_COLOR_ACCENT);
        const QColor accent_light = HexColor(VRYO_COLOR_ACCENT_LIGHT);

        /*-------------------------------------------------*\
        | Background and border                             |
        \*-------------------------------------------------*/
        QLinearGradient background(0, 0, 0, height);
        background.setColorAt(0.0, HexColor(VRYO_COLOR_HERO));
        background.setColorAt(1.0, HexColor(VRYO_COLOR_BACKGROUND));
        painter.fillRect(QRectF(0, 0, width, height), background);

        painter.setPen(QPen(HexColor(VRYO_COLOR_BORDER), 1.0));
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

        QColor ring_faint = HexColor(VRYO_COLOR_BORDER);
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
        painter.setPen(HexColor(VRYO_COLOR_TEXT));
        QFont title_font(QStringLiteral("Segoe UI"), 24, QFont::DemiBold);
        painter.setFont(title_font);
        painter.drawText(QRectF(0, 262, width, 44), Qt::AlignCenter, QStringLiteral("Vryo Control"));

        painter.setPen(HexColor(VRYO_COLOR_MUTED));
        QFont sub_font(QStringLiteral("Segoe UI"), 10);
        painter.setFont(sub_font);
        painter.drawText(QRectF(0, 304, width, 24), Qt::AlignCenter, QStringLiteral("RGB lighting control"));

        /*-------------------------------------------------*\
        | Progress bar with a moving shimmer                |
        \*-------------------------------------------------*/
        const qreal bar_width   = 320.0;
        const qreal bar_height  = 6.0;
        const QRectF track((width - bar_width) / 2.0, 350.0, bar_width, bar_height);

        painter.setPen(Qt::NoPen);
        painter.setBrush(HexColor(VRYO_COLOR_SURFACE));
        painter.drawRoundedRect(track, bar_height / 2.0, bar_height / 2.0);

        double clamped = (std::max)(0.0, (std::min)(1.0, progress));
        QRectF fill = track;
        fill.setWidth((std::max)(bar_height, track.width() * clamped));

        QLinearGradient fill_gradient(fill.left(), 0, fill.right(), 0);
        fill_gradient.setColorAt(0.0, HexColor(VRYO_COLOR_ACCENT_DARK));
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
        painter.setPen(HexColor(VRYO_COLOR_MUTED));
        QFont msg_font(QStringLiteral("Segoe UI"), 9);
        painter.setFont(msg_font);
        painter.drawText(QRectF(0, 366, width, 24), Qt::AlignCenter, message);

        painter.end();

        return(pixmap);
    }
}
