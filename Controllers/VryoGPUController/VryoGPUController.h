/*---------------------------------------------------------*\
| VryoGPUController.h                                  |
|                                                           |
|   Low-level driver for the custom GPU lighting board.     |
|   Two backends: VIRTUAL (no hardware, frames are only     |
|   logged) and SERIAL (frames sent to a controller board). |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <mutex>
#include <string>
#include <vector>
#include "RGBController.h"
#include "serial_port.h"

class VryoGPUController
{
public:
    /*-----------------------------------------------------*\
    | Virtual device: no hardware needed                    |
    \*-----------------------------------------------------*/
    VryoGPUController();

    /*-----------------------------------------------------*\
    | Real device on a serial port, e.g. "COM5"             |
    \*-----------------------------------------------------*/
    VryoGPUController(const std::string& port_name_arg);

    ~VryoGPUController();

    bool            IsPresent() const;
    bool            IsVirtual() const;

    void            SetLEDs(const std::vector<RGBColor>& colors);
    void            SetBlack(unsigned int led_count);

    std::string     GetLocation() const;
    std::string     GetSerial() const;

private:
    bool            WriteFrame(const std::vector<RGBColor>& colors);

    std::string     port_name;
    bool            virtual_device;
    bool            port_open;
    serial_port     serial_port_interface;
    std::mutex      write_mutex;
};
