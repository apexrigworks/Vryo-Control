/*---------------------------------------------------------*\
| VryoStatTile.h                                            |
|                                                           |
|   One card on the Dashboard: a title, a big value, a      |
|   history graph and a fill bar.                           |
|                                                           |
|   The card animates: it lifts and glows under the mouse,  |
|   springs when pressed (with a soft ripple), pops in when |
|   the Dashboard opens, and the bar glides to new values.  |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <vector>
#include <QColor>
#include <QEasingCurve>
#include <QPointF>
#include <QSize>
#include <QString>
#include <QWidget>

class QEvent;
class QMouseEvent;
class QPaintEvent;
class QVariantAnimation;

class VryoStatTile : public QWidget
{
public:
    explicit VryoStatTile(const QString& title, QWidget * parent = nullptr);

    /*-----------------------------------------------------*\
    | value     the big text, e.g. "63"                     |
    | unit      small text after it, e.g. "C"               |
    | detail    small text top right, e.g. "of 24 GB"       |
    | fraction  0.0 to 1.0 for the bar and graph, or -1     |
    | colour    colour of the bar and graph                 |
    \*-----------------------------------------------------*/
    void SetReading(const QString& value, const QString& unit, const QString& detail, double fraction, const QColor& colour);
    void SetUnavailable();

    void AddHistory(double fraction);
    void ClearHistory();

    /*-----------------------------------------------------*\
    | Hide the card, then pop it in after delay_ms          |
    \*-----------------------------------------------------*/
    void PlayIntro(int delay_ms, int duration_ms);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent * event) override;
    void mousePressEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    bool event(QEvent * event) override;

private:
    QString                 title_text;
    QString                 value_text;
    QString                 unit_text;
    QString                 detail_text;
    double                  bar_fraction    = -1.0;
    double                  shown_fraction  = -1.0;
    QColor                  bar_colour;
    bool                    available       = false;

    std::vector<double>     history;

    /*-----------------------------------------------------*\
    | Animation state, each value is driven by one          |
    | QVariantAnimation                                     |
    \*-----------------------------------------------------*/
    double                  hover_amount    = 0.0;
    double                  press_amount    = 0.0;
    double                  ripple_amount   = 0.0;
    double                  intro_amount    = 1.0;
    bool                    ripple_active   = false;
    QPointF                 ripple_origin;
    int                     intro_token     = 0;

    QVariantAnimation *     hover_anim      = nullptr;
    QVariantAnimation *     press_anim      = nullptr;
    QVariantAnimation *     ripple_anim     = nullptr;
    QVariantAnimation *     intro_anim      = nullptr;
    QVariantAnimation *     bar_anim        = nullptr;

    QVariantAnimation *     MakeAnimation(double * target);
    void                    Animate(QVariantAnimation * animation, double from, double to, int milliseconds, QEasingCurve::Type curve);
};
