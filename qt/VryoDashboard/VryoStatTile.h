/*---------------------------------------------------------*\
| VryoStatTile.h                                            |
|                                                           |
|   One card on the Dashboard: a title, a big value, a      |
|   short history graph and a fill bar.                     |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <vector>
#include <QColor>
#include <QSize>
#include <QString>
#include <QWidget>

class VryoStatTile : public QWidget
{
public:
    explicit VryoStatTile(const QString& title, QWidget * parent = nullptr);

    /*-----------------------------------------------------*\
    | value     the big text, e.g. "63"                     |
    | unit      small text after it, e.g. "°C"              |
    | detail    small text top right, e.g. "of 24 GB"       |
    | fraction  0.0 to 1.0 for the bar and graph, or -1     |
    | colour    colour of the bar and graph                 |
    \*-----------------------------------------------------*/
    void SetReading(const QString& value, const QString& unit, const QString& detail, double fraction, const QColor& colour);
    void SetUnavailable();

    void AddHistory(double fraction);
    void ClearHistory();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent * event) override;

private:
    QString                 title_text;
    QString                 value_text;
    QString                 unit_text;
    QString                 detail_text;
    double                  bar_fraction    = -1.0;
    QColor                  bar_colour;
    bool                    available       = false;

    std::vector<double>     history;
};
