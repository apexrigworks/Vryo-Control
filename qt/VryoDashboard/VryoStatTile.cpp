/*---------------------------------------------------------*\
| VryoStatTile.cpp                                          |
|                                                           |
|   One card on the Dashboard.                              |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "VryoStatTile.h"

#include <cmath>

#include <QEvent>
#include <QFont>
#include <QFontMetricsF>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPen>
#include <QRectF>
#include <QTimer>
#include <QVariantAnimation>

#include "../../startup/VryoTheme.h"

namespace
{
    const size_t    VRYO_TILE_HISTORY_LENGTH    = 90;
    /*-----------------------------------------------------*\
    | Everything on the card is drawn for a card of about   |
    | 224 x 176 pixels and then multiplied by a scale       |
    | factor, so a big window gets big text and big graphs  |
    \*-----------------------------------------------------*/
    const qreal     VRYO_TILE_BASE_WIDTH        = 224.0;
    const qreal     VRYO_TILE_BASE_HEIGHT       = 176.0;
    const qreal     VRYO_TILE_SCALE_MIN         = 0.80;
    const qreal     VRYO_TILE_SCALE_MAX         = 2.60;

    QColor Mix(const QColor& from, const QColor& to, qreal amount)
    {
        if(amount < 0.0)
        {
            amount = 0.0;
        }

        if(amount > 1.0)
        {
            amount = 1.0;
        }

        return(QColor::fromRgbF(from.redF()   + ((to.redF()   - from.redF())   * amount),
                                from.greenF() + ((to.greenF() - from.greenF()) * amount),
                                from.blueF()  + ((to.blueF()  - from.blueF())  * amount)));
    }

    qreal Clamp01(qreal value)
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
}

VryoStatTile::VryoStatTile(const QString& title, QWidget * parent) :
    QWidget(parent),
    title_text(title)
{
    bar_colour = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_ACCENT);
    setMinimumSize(190, 168);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    hover_anim  = MakeAnimation(&hover_amount);
    press_anim  = MakeAnimation(&press_amount);
    ripple_anim = MakeAnimation(&ripple_amount);
    intro_anim  = MakeAnimation(&intro_amount);
    bar_anim    = MakeAnimation(&shown_fraction);

    connect(ripple_anim, &QVariantAnimation::finished, this, [this]()
    {
        ripple_active = false;
        update();
    });
}

QVariantAnimation * VryoStatTile::MakeAnimation(double * target)
{
    QVariantAnimation * animation = new QVariantAnimation(this);

    connect(animation, &QVariantAnimation::valueChanged, this, [this, target](const QVariant& value)
    {
        *target = value.toDouble();
        update();
    });

    return(animation);
}

void VryoStatTile::Animate(QVariantAnimation * animation, double from, double to, int milliseconds, QEasingCurve::Type curve)
{
    animation->stop();
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->setDuration(milliseconds);
    animation->setEasingCurve(QEasingCurve(curve));
    animation->start();
}

void VryoStatTile::SetReading(const QString& value, const QString& unit, const QString& detail, double fraction, const QColor& colour)
{
    value_text      = value;
    unit_text       = unit;
    detail_text     = detail;
    bar_fraction    = fraction;
    bar_colour      = colour;
    available       = true;

    /*-----------------------------------------------------*\
    | The bar glides to the new value instead of jumping    |
    \*-----------------------------------------------------*/
    if(fraction < 0.0)
    {
        bar_anim->stop();
        shown_fraction = -1.0;
    }
    else if(shown_fraction < 0.0)
    {
        shown_fraction = fraction;
    }
    else if(std::fabs(shown_fraction - fraction) > 0.0005)
    {
        Animate(bar_anim, shown_fraction, fraction, 650, QEasingCurve::OutCubic);
    }

    update();
}

void VryoStatTile::SetUnavailable()
{
    value_text.clear();
    unit_text.clear();
    detail_text.clear();
    bar_fraction    = -1.0;
    shown_fraction  = -1.0;
    available       = false;

    bar_anim->stop();
    update();
}

void VryoStatTile::SetAlert(double amount)
{
    alert_amount = Clamp01(amount);

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

void VryoStatTile::PlayIntro(int delay_ms, int duration_ms)
{
    intro_token++;

    const int token = intro_token;

    intro_anim->stop();
    intro_amount = 0.0;
    update();

    QTimer::singleShot(delay_ms, this, [this, token, duration_ms]()
    {
        if(token != intro_token)
        {
            return;
        }

        Animate(intro_anim, 0.0, 1.0, duration_ms, QEasingCurve::Linear);
    });
}

QSize VryoStatTile::sizeHint() const
{
    return(QSize(236, 184));
}

QSize VryoStatTile::minimumSizeHint() const
{
    return(QSize(190, 168));
}

bool VryoStatTile::event(QEvent * event)
{
    if(event->type() == QEvent::Enter)
    {
        Animate(hover_anim, hover_amount, 1.0, 200, QEasingCurve::OutCubic);
    }
    else if(event->type() == QEvent::Leave)
    {
        Animate(hover_anim, hover_amount, 0.0, 320, QEasingCurve::OutCubic);
    }

    return(QWidget::event(event));
}

void VryoStatTile::mousePressEvent(QMouseEvent * event)
{
    if(event->button() == Qt::LeftButton)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        ripple_origin   = event->position();
#else
        ripple_origin   = event->localPos();
#endif
        ripple_active   = true;

        Animate(press_anim, press_amount, 1.0, 90, QEasingCurve::OutQuad);
        Animate(ripple_anim, 0.0, 1.0, 700, QEasingCurve::OutCubic);

        /*-------------------------------------------------*\
        | Accept the press, otherwise Qt never sends us the |
        | matching release and the card would stay shrunk   |
        \*-------------------------------------------------*/
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void VryoStatTile::mouseReleaseEvent(QMouseEvent * event)
{
    if(event->button() == Qt::LeftButton)
    {
        /*-------------------------------------------------*\
        | OutBack overshoots a little, so the card springs  |
        | back past its normal size and settles             |
        \*-------------------------------------------------*/
        Animate(press_anim, press_amount, 0.0, 450, QEasingCurve::OutBack);

        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void VryoStatTile::paintEvent(QPaintEvent *)
{
    /*-----------------------------------------------------*\
    | Before its pop-in starts the card is not drawn at all |
    \*-----------------------------------------------------*/
    if(intro_amount <= 0.0)
    {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QColor panel_colour   = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_PANEL);
    const QColor border_colour  = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_BORDER);
    const QColor surface_colour = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_SURFACE);
    const QColor text_colour    = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_TEXT);
    const QColor muted_colour   = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_MUTED);
    const QColor accent_colour  = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_ACCENT);
    const QColor accent_light   = VryoTheme::HexColor(VryoTheme::VRYO_COLOR_ACCENT_LIGHT);

    const qreal hover = Clamp01(hover_amount);
    const qreal intro = Clamp01(intro_amount);
    const qreal alert = Clamp01(alert_amount);

    /*-----------------------------------------------------*\
    | A hot card keeps a glow and a coloured border even    |
    | without the mouse on it                               |
    \*-----------------------------------------------------*/
    const qreal glow_strength   = (hover > (alert * 0.8)) ? hover : (alert * 0.8);
    const qreal border_strength = ((hover * 0.85) > (alert * 0.75)) ? (hover * 0.85) : (alert * 0.75);
    const QColor glow_base      = Mix(accent_colour, bar_colour, alert);
    const QColor border_tint    = Mix(accent_light, bar_colour, alert);

    /*-----------------------------------------------------*\
    | Room left around the card for the glow, the lift and  |
    | the pop-in overshoot. It grows a little with size.    |
    \*-----------------------------------------------------*/
    qreal pad = (qreal)(width() < height() ? width() : height()) * 0.035;

    if(pad < 6.0)
    {
        pad = 6.0;
    }

    if(pad > 16.0)
    {
        pad = 16.0;
    }

    const QRectF card = QRectF(rect()).adjusted(pad, pad * 1.3, -pad, -pad);

    /*-----------------------------------------------------*\
    | Scale factor from the size of the card                |
    \*-----------------------------------------------------*/
    qreal s = card.width() / VRYO_TILE_BASE_WIDTH;

    if((card.height() / VRYO_TILE_BASE_HEIGHT) < s)
    {
        s = card.height() / VRYO_TILE_BASE_HEIGHT;
    }

    if(s < VRYO_TILE_SCALE_MIN)
    {
        s = VRYO_TILE_SCALE_MIN;
    }

    if(s > VRYO_TILE_SCALE_MAX)
    {
        s = VRYO_TILE_SCALE_MAX;
    }

    const qreal radius  = 14.0 * s;
    const qreal margin  = 16.0 * s;

    /*-----------------------------------------------------*\
    | Pop-in: fade, rise and a slight overshoot in size.    |
    | Press: the card shrinks a little and springs back.    |
    \*-----------------------------------------------------*/
    const qreal rise        = QEasingCurve(QEasingCurve::OutCubic).valueForProgress(intro);
    const qreal grow        = QEasingCurve(QEasingCurve::OutBack).valueForProgress(intro);
    const qreal pop_scale   = (0.90 + (0.10 * grow)) * (1.0 - (0.03 * press_amount));
    const qreal lift        = (-3.0 * s * hover) + ((1.0 - rise) * 20.0 * s);

    painter.setOpacity(Clamp01(intro * 2.2));

    painter.translate(card.center().x(), card.center().y() + lift);
    painter.scale(pop_scale, pop_scale);
    painter.translate(-card.center().x(), -card.center().y());

    /*-----------------------------------------------------*\
    | Soft glow that fades in under the mouse               |
    \*-----------------------------------------------------*/
    if(glow_strength > 0.01)
    {
        painter.setPen(Qt::NoPen);

        for(int ring = 5; ring >= 1; ring--)
        {
            const qreal grow_by = ring * 1.4 * s;

            QColor glow = glow_base;
            glow.setAlpha((int)(30.0 * glow_strength / (qreal)ring));
            painter.setBrush(glow);
            painter.drawRoundedRect(card.adjusted(-grow_by, -grow_by, grow_by, grow_by), radius + grow_by, radius + grow_by);
        }
    }

    /*-----------------------------------------------------*\
    | Card: a faint top-to-bottom gradient and a border     |
    | that turns blue on hover                              |
    \*-----------------------------------------------------*/
    QLinearGradient card_gradient(card.topLeft(), card.bottomLeft());
    card_gradient.setColorAt(0.0, Mix(panel_colour, surface_colour, 0.45 + (0.35 * hover)));
    card_gradient.setColorAt(1.0, panel_colour);

    painter.setPen(QPen(Mix(border_colour, border_tint, border_strength), 1.0 + (0.25 * s)));
    painter.setBrush(card_gradient);
    painter.drawRoundedRect(card, radius, radius);

    const qreal left    = card.left() + margin;
    const qreal right   = card.right() - margin;
    const qreal inner   = right - left;

    /*-----------------------------------------------------*\
    | Title (left) and detail (right)                       |
    \*-----------------------------------------------------*/
    const qreal title_top       = card.top() + (12.0 * s);
    const qreal title_height    = 18.0 * s;

    QFont small_font = font();
    small_font.setPixelSize((int)(11.0 * s));
    small_font.setLetterSpacing(QFont::AbsoluteSpacing, 0.8 * s);
    painter.setFont(small_font);
    painter.setPen(Mix(muted_colour, text_colour, hover * 0.5));
    painter.drawText(QRectF(left, title_top, inner, title_height), Qt::AlignLeft | Qt::AlignVCenter, title_text.toUpper());

    if(available && !detail_text.isEmpty())
    {
        QFont detail_font = font();
        detail_font.setPixelSize((int)(11.0 * s));
        painter.setFont(detail_font);
        painter.setPen(muted_colour);
        painter.drawText(QRectF(left, title_top, inner, title_height), Qt::AlignRight | Qt::AlignVCenter, detail_text);
    }

    /*-----------------------------------------------------*\
    | Big value with a small unit after it                  |
    \*-----------------------------------------------------*/
    QFont value_font = font();
    value_font.setPixelSize((int)(36.0 * s));
    value_font.setWeight(QFont::DemiBold);

    const QString shown_value = available ? value_text : QStringLiteral("N/A");

    painter.setFont(value_font);
    painter.setPen(available ? Mix(text_colour, bar_colour.lighter(115), alert * 0.9) : muted_colour);

    const QFontMetricsF value_metrics(value_font);
    const qreal         value_top   = card.top() + (32.0 * s);
    const qreal         value_wide  = value_metrics.horizontalAdvance(shown_value);

    painter.drawText(QRectF(left, value_top, inner, value_metrics.height()), Qt::AlignLeft | Qt::AlignVCenter, shown_value);

    if(available && !unit_text.isEmpty())
    {
        QFont unit_font = font();
        unit_font.setPixelSize((int)(16.0 * s));
        painter.setFont(unit_font);
        painter.setPen(muted_colour);

        painter.drawText(QRectF(left + value_wide + (5.0 * s), value_top, inner - value_wide - (5.0 * s), value_metrics.height()),
                         Qt::AlignLeft | Qt::AlignBottom, unit_text);
    }

    /*-----------------------------------------------------*\
    | Bar along the bottom                                  |
    \*-----------------------------------------------------*/
    const qreal bar_height  = 6.0 * s;
    const QRectF bar_track(left, card.bottom() - (14.0 * s) - bar_height, inner, bar_height);

    painter.setPen(Qt::NoPen);
    painter.setBrush(surface_colour);
    painter.drawRoundedRect(bar_track, bar_height / 2.0, bar_height / 2.0);

    if(available && (shown_fraction >= 0.0))
    {
        const double clamped = Clamp01(shown_fraction);

        QRectF bar_fill = bar_track;
        bar_fill.setWidth(bar_height + ((bar_track.width() - bar_height) * clamped));

        QLinearGradient bar_gradient(bar_fill.left(), 0, bar_fill.right(), 0);
        bar_gradient.setColorAt(0.0, bar_colour.darker(130));
        bar_gradient.setColorAt(1.0, bar_colour);

        painter.setBrush(bar_gradient);
        painter.drawRoundedRect(bar_fill, bar_height / 2.0, bar_height / 2.0);
    }

    /*-----------------------------------------------------*\
    | History graph between the value and the bar. It takes |
    | all the room that is left, so a tall card gets a      |
    | tall graph.                                           |
    \*-----------------------------------------------------*/
    const qreal graph_top       = value_top + value_metrics.height() + (6.0 * s);
    const qreal graph_bottom    = bar_track.top() - (12.0 * s);

    if(available && (history.size() >= 2) && ((graph_bottom - graph_top) >= (14.0 * s)))
    {
        const QRectF graph(left, graph_top, inner, graph_bottom - graph_top);
        const qreal  step = graph.width() / (qreal)(VRYO_TILE_HISTORY_LENGTH - 1);

        QPainterPath line;
        QPainterPath area;
        QPointF      last_point;

        const qreal first_x = graph.right() - (step * (qreal)(history.size() - 1));

        for(size_t i = 0; i < history.size(); i++)
        {
            const qreal x = first_x + (step * (qreal)i);
            const qreal y = graph.bottom() - (graph.height() * history[i]);

            last_point = QPointF(x, y);

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
        fill_top.setAlpha(80);
        fill_bottom.setAlpha(0);

        QLinearGradient area_gradient(0, graph.top(), 0, graph.bottom());
        area_gradient.setColorAt(0.0, fill_top);
        area_gradient.setColorAt(1.0, fill_bottom);

        painter.setPen(Qt::NoPen);
        painter.setBrush(area_gradient);
        painter.drawPath(area);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(bar_colour, 1.8 * s, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(line);

        /*-------------------------------------------------*\
        | A glowing dot on the newest reading               |
        \*-------------------------------------------------*/
        QColor halo = bar_colour;
        halo.setAlpha(70);

        painter.setPen(Qt::NoPen);
        painter.setBrush(halo);
        painter.drawEllipse(last_point, 5.5 * s, 5.5 * s);
        painter.setBrush(bar_colour.lighter(130));
        painter.drawEllipse(last_point, 2.6 * s, 2.6 * s);
    }

    /*-----------------------------------------------------*\
    | Click ripple, spreading from where the card was       |
    | pressed and kept inside the rounded corners           |
    \*-----------------------------------------------------*/
    if(ripple_active)
    {
        QPainterPath clip;
        clip.addRoundedRect(card, radius, radius);

        const qreal reach = std::sqrt((card.width() * card.width()) + (card.height() * card.height()));

        QColor ripple = accent_light;
        ripple.setAlphaF(0.32 * (1.0 - Clamp01(ripple_amount)));

        painter.save();
        painter.setClipPath(clip);
        painter.setPen(Qt::NoPen);
        painter.setBrush(ripple);
        painter.drawEllipse(ripple_origin, reach * ripple_amount, reach * ripple_amount);
        painter.restore();
    }
}
