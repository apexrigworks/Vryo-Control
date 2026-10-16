/*---------------------------------------------------------*\
| VryoStatTile.cpp                                          |
|                                                           |
|   One card on the Dashboard.                              |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "VryoStatTile.h"

#include <QFont>
#include <QFontMetricsF>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPen>
#include <QRectF>

#include "../../startup/VryoTheme.h"

namespace
{
    const size_t    VRYO_TILE_HISTORY_LENGTH    = 90;
    const qreal     VRYO_TILE_MARGIN            = 14.0;
}

VryoStatTile::VryoStatTile(const QString& title, QWidget * parent) :
    QWidget(parent),
    title_text(title)
{
    bar_colour = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_ACCENT);
    setMinimumSize(190, 128);
}

void VryoStatTile::SetReading(const QString& value, const QString& unit, const QString& detail, double fraction, const QColor& colour)
{
    value_text      = value;
    unit_text       = unit;
    detail_text     = detail;
    bar_fraction    = fraction;
    bar_colour      = colour;
    available       = true;

    update();
}

void VryoStatTile::SetUnavailable()
{
    value_text.clear();
    unit_text.clear();
    detail_text.clear();
    bar_fraction    = -1.0;
    available       = false;

    update();
}

void VryoStatTile::AddHistory(double fraction)
{
    if(fraction < 0.0)
    {
        return;
    }

    if(fraction > 1.0)
    {
        fraction = 1.0;
    }

    history.push_back(fraction);

    while(history.size() > VRYO_TILE_HISTORY_LENGTH)
    {
        history.erase(history.begin());
    }
}

void VryoStatTile::ClearHistory()
{
    history.clear();
    update();
}

QSize VryoStatTile::sizeHint() const
{
    return(QSize(230, 140));
}

QSize VryoStatTile::minimumSizeHint() const
{
    return(QSize(190, 128));
}

void VryoStatTile::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QColor panel_colour   = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_PANEL);
    const QColor border_colour  = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_BORDER);
    const QColor surface_colour = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_SURFACE);
    const QColor text_colour    = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_TEXT);
    const QColor muted_colour   = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_MUTED);

    const qreal  tile_width     = (qreal)width();
    const qreal  tile_height    = (qreal)height();

    /*-----------------------------------------------------*\
    | Card background                                       |
    \*-----------------------------------------------------*/
    const QRectF card(0.5, 0.5, tile_width - 1.0, tile_height - 1.0);

    painter.setPen(QPen(border_colour, 1.0));
    painter.setBrush(panel_colour);
    painter.drawRoundedRect(card, 12.0, 12.0);

    const qreal left    = VRYO_TILE_MARGIN;
    const qreal right   = tile_width - VRYO_TILE_MARGIN;
    const qreal inner   = right - left;

    /*-----------------------------------------------------*\
    | Title (left) and detail (right)                       |
    \*-----------------------------------------------------*/
    QFont small_font = font();
    small_font.setPointSizeF(8.5);
    small_font.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    painter.setFont(small_font);
    painter.setPen(muted_colour);
    painter.drawText(QRectF(left, 10.0, inner, 18.0), Qt::AlignLeft | Qt::AlignVCenter, title_text.toUpper());

    if(available && !detail_text.isEmpty())
    {
        QFont detail_font = font();
        detail_font.setPointSizeF(8.5);
        painter.setFont(detail_font);
        painter.drawText(QRectF(left, 10.0, inner, 18.0), Qt::AlignRight | Qt::AlignVCenter, detail_text);
    }

    /*-----------------------------------------------------*\
    | Big value with a small unit after it                  |
    \*-----------------------------------------------------*/
    qreal value_pixels = tile_height * 0.24;

    if(value_pixels < 22.0)
    {
        value_pixels = 22.0;
    }

    if(value_pixels > 40.0)
    {
        value_pixels = 40.0;
    }

    QFont value_font = font();
    value_font.setPixelSize((int)value_pixels);
    value_font.setWeight(QFont::DemiBold);

    const QString shown_value = available ? value_text : QStringLiteral("N/A");

    painter.setFont(value_font);
    painter.setPen(available ? text_colour : muted_colour);

    const QFontMetricsF value_metrics(value_font);
    const qreal         value_top   = 30.0;
    const qreal         value_wide  = value_metrics.horizontalAdvance(shown_value);

    painter.drawText(QRectF(left, value_top, inner, value_metrics.height()), Qt::AlignLeft | Qt::AlignVCenter, shown_value);

    if(available && !unit_text.isEmpty())
    {
        QFont unit_font = font();
        unit_font.setPixelSize((int)(value_pixels * 0.45));
        painter.setFont(unit_font);
        painter.setPen(muted_colour);

        painter.drawText(QRectF(left + value_wide + 5.0, value_top, inner - value_wide - 5.0, value_metrics.height()),
                         Qt::AlignLeft | Qt::AlignBottom, unit_text);
    }

    /*-----------------------------------------------------*\
    | Bar along the bottom                                  |
    \*-----------------------------------------------------*/
    const qreal bar_height  = 6.0;
    const QRectF bar_track(left, tile_height - VRYO_TILE_MARGIN - bar_height, inner, bar_height);

    painter.setPen(Qt::NoPen);
    painter.setBrush(surface_colour);
    painter.drawRoundedRect(bar_track, bar_height / 2.0, bar_height / 2.0);

    if(available && (bar_fraction >= 0.0))
    {
        double clamped = bar_fraction;

        if(clamped > 1.0)
        {
            clamped = 1.0;
        }

        QRectF bar_fill = bar_track;
        bar_fill.setWidth(bar_height + ((bar_track.width() - bar_height) * clamped));

        QLinearGradient bar_gradient(bar_fill.left(), 0, bar_fill.right(), 0);
        bar_gradient.setColorAt(0.0, bar_colour.darker(130));
        bar_gradient.setColorAt(1.0, bar_colour);

        painter.setBrush(bar_gradient);
        painter.drawRoundedRect(bar_fill, bar_height / 2.0, bar_height / 2.0);
    }

    /*-----------------------------------------------------*\
    | History graph between the value and the bar           |
    \*-----------------------------------------------------*/
    const qreal graph_top       = value_top + value_metrics.height() + 4.0;
    const qreal graph_bottom    = bar_track.top() - 8.0;

    if(available && (history.size() >= 2) && (graph_bottom - graph_top >= 12.0))
    {
        const QRectF graph(left, graph_top, inner, graph_bottom - graph_top);
        const qreal  step = graph.width() / (qreal)(VRYO_TILE_HISTORY_LENGTH - 1);

        QPainterPath line;
        QPainterPath area;

        const qreal first_x = graph.right() - (step * (qreal)(history.size() - 1));

        for(size_t i = 0; i < history.size(); i++)
        {
            const qreal x = first_x + (step * (qreal)i);
            const qreal y = graph.bottom() - (graph.height() * history[i]);

            if(i == 0)
            {
                line.moveTo(x, y);
                area.moveTo(x, graph.bottom());
                area.lineTo(x, y);
            }
            else
            {
                line.lineTo(x, y);
                area.lineTo(x, y);
            }
        }

        area.lineTo(graph.right(), graph.bottom());
        area.closeSubpath();

        QColor fill_top     = bar_colour;
        QColor fill_bottom  = bar_colour;
        fill_top.setAlpha(70);
        fill_bottom.setAlpha(0);

        QLinearGradient area_gradient(0, graph.top(), 0, graph.bottom());
        area_gradient.setColorAt(0.0, fill_top);
        area_gradient.setColorAt(1.0, fill_bottom);

        painter.setPen(Qt::NoPen);
        painter.setBrush(area_gradient);
        painter.drawPath(area);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(bar_colour, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(line);
    }
}
