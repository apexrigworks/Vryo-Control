/*---------------------------------------------------------*\
| VryoDetectOverlay.h                                       |
|                                                           |
|   The "Detecting RGB compatible hardware" screen laid     |
|   over the RGB tab the first time it is opened. It spins  |
|   while it waits, then shows the name of each device      |
|   large, then fades away to reveal the tab. A click on    |
|   it skips straight to the fade.                          |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <QElapsedTimer>
#include <QStringList>
#include <QTimer>
#include <QWidget>

class QPainter;

class VryoDetectOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit VryoDetectOverlay(QWidget * parent);
    ~VryoDetectOverlay();

    /*-----------------------------------------------------*\
    | Cover the parent and run the whole sequence from the  |
    | start                                                 |
    \*-----------------------------------------------------*/
    void    Start();

    /*-----------------------------------------------------*\
    | Begin the fade out now                                |
    \*-----------------------------------------------------*/
    void    Skip();

signals:
    void    Finished();

protected:
    void    paintEvent(QPaintEvent * event) override;
    void    mousePressEvent(QMouseEvent * event) override;
    bool    eventFilter(QObject * watched, QEvent * event) override;

private slots:
    void    Tick();

private:
    enum Stage
    {
        STAGE_IDLE,
        STAGE_DETECTING,
        STAGE_NAMES,
        STAGE_FADING
    };

    QTimer          timer;
    QElapsedTimer   clock;

    Stage           stage               = STAGE_IDLE;
    qint64          stage_start_ms      = 0;
    qint64          ready_at_ms         = -1;
    double          angle               = 0.0;
    double          progress            = 0.0;
    double          master_opacity      = 1.0;

    QStringList     names;
    int             extra_devices       = 0;
    bool            none_found          = false;
    qint64          names_frozen_ms     = 0;

    void    EnterNames();
    void    EnterFading();
    void    FitToParent();
    bool    DetectionDone() const;

    void    PaintBackground(QPainter& painter);
    void    PaintDetecting(QPainter& painter, double opacity);
    void    PaintNames(QPainter& painter, qint64 elapsed_ms);
};
