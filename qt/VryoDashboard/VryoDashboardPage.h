/*---------------------------------------------------------*\
| VryoDashboardPage.h                                       |
|                                                           |
|   The Dashboard tab: live GPU temperature, load, clocks,  |
|   fan, memory and power, read from the NVIDIA driver.     |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <vector>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

#include "VryoNvml.h"
#include "VryoStatTile.h"

class VryoDashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit VryoDashboardPage(QWidget * parent = nullptr);
    ~VryoDashboardPage();

protected:
    void showEvent(QShowEvent * event) override;
    void hideEvent(QHideEvent * event) override;
    void resizeEvent(QResizeEvent * event) override;

private slots:
    void Poll();
    void OnGpuPicked(int index);
    void OnRetryClicked();

private:
    enum
    {
        TILE_TEMPERATURE,
        TILE_LOAD,
        TILE_CORE_CLOCK,
        TILE_MEMORY_CLOCK,
        TILE_FAN,
        TILE_VIDEO_MEMORY,
        TILE_POWER,
        TILE_COUNT
    };

    VryoNvml                        nvml;
    QTimer *                        timer           = nullptr;
    int                             gpu_index       = 0;
    int                             columns         = 0;
    bool                            driver_tried    = false;
    bool                            intro_played    = false;

    QWidget *                       header_widget   = nullptr;
    QLabel *                        gpu_name_label  = nullptr;
    QLabel *                        driver_label    = nullptr;
    QComboBox *                     gpu_picker      = nullptr;
    QLabel *                        status_label    = nullptr;
    QPushButton *                   retry_button    = nullptr;
    QWidget *                       tiles_container = nullptr;
    QGridLayout *                   tiles_grid      = nullptr;
    std::vector<VryoStatTile *>     tiles;

    void BuildUi();
    void StartDriver();
    void ShowStatus(const QString& message);
    void ShowHeader();
    void Reflow();
    void SetTilesUnavailable();
    void ApplySample(const VryoGpuSample& sample);
    void PlayIntro();
};
