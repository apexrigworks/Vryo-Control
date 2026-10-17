/*---------------------------------------------------------*\
| VryoUiFx.h                                                |
|                                                           |
|   App-wide interface effects. Installed once on the       |
|   application: every push button and tool button gets a   |
|   soft ripple spreading from the point that was clicked.  |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <QObject>

class QEvent;

class VryoUiFx : public QObject
{
public:
    explicit VryoUiFx(QObject * parent = nullptr);

    /*-----------------------------------------------------*\
    | Install on the application, once. Safe to call again. |
    \*-----------------------------------------------------*/
    static void Install();

protected:
    bool eventFilter(QObject * watched, QEvent * event) override;
};
