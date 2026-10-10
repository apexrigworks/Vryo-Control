/*---------------------------------------------------------*\
| VryoGPUController.cpp                                |
|                                                           |
|   Low-level driver for the custom GPU lighting board.     |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <cstdint>
#include "LogManager.h"
#include "VryoGPUConfig.h"
#include "VryoGPUController.h"

/*---------------------------------------------------------*\
| Frame packet sent to the controller board:                |
|                                                           |
|   byte 0      0xAA            magic                       |
|   byte 1      0x55            magic                       |
|   byte 2      LED count, low byte                         |
|   byte 3      LED count, high byte                        |
|   byte 4..    R, G, B for each LED, in zone order         |
|   last byte   XOR of every byte before it                 |
\*---------------------------------------------------------*/
#define YB_GPU_MAGIC_0          0xAA
#define YB_GPU_MAGIC_1          0x55

VryoGPUController::VryoGPUController()
{
    port_name       = "virtual";
    virtual_device  = true;
    port_open       = false;
}

VryoGPUController::VryoGPUController(const std::string& port_name_arg)
{
    port_name       = port_name_arg;
    virtual_device  = false;
    port_open       = serial_port_interface.serial_open(port_name.c_str(), YB_GPU_SERIAL_BAUD);

    if(!port_open)
    {
        LOG_ERROR("[%s] Could not open serial port %s", YB_GPU_DETECTOR_NAME, port_name.c_str());
    }
}

VryoGPUController::~VryoGPUController()
{
    if(port_open)
    {
        serial_port_interface.serial_close();
    }
}

bool VryoGPUController::IsPresent() const
{
    return(virtual_device || port_open);
}

bool VryoGPUController::IsVirtual() const
{
    return(virtual_device);
}

std::string VryoGPUController::GetLocation() const
{
    return(virtual_device ? "Virtual" : ("Serial: " + port_name));
}

std::string VryoGPUController::GetSerial() const
{
    /*-----------------------------------------------------*\
    | Placeholder. Later your board can report its own      |
    | serial number (e.g. the #0042 collector number).      |
    \*-----------------------------------------------------*/
    return("");
}

void VryoGPUController::SetLEDs(const std::vector<RGBColor>& colors)
{
    if(virtual_device)
    {
        LOG_DEBUG("[%s] Virtual frame, %d LEDs", YB_GPU_DETECTOR_NAME, (int)colors.size());
        return;
    }

    WriteFrame(colors);
}

void VryoGPUController::SetBlack(unsigned int led_count)
{
    std::vector<RGBColor> black(led_count, 0x00000000);

    SetLEDs(black);
}

bool VryoGPUController::WriteFrame(const std::vector<RGBColor>& colors)
{
    if(!port_open)
    {
        return(false);
    }

    std::vector<uint8_t> packet;
    packet.reserve(5 + (colors.size() * 3));

    packet.push_back(YB_GPU_MAGIC_0);
    packet.push_back(YB_GPU_MAGIC_1);
    packet.push_back((uint8_t)(colors.size() & 0xFF));
    packet.push_back((uint8_t)((colors.size() >> 8) & 0xFF));

    for(std::size_t led_idx = 0; led_idx < colors.size(); led_idx++)
    {
        packet.push_back((uint8_t)RGBGetRValue(colors[led_idx]));
        packet.push_back((uint8_t)RGBGetGValue(colors[led_idx]));
        packet.push_back((uint8_t)RGBGetBValue(colors[led_idx]));
    }

    uint8_t checksum = 0;

    for(std::size_t byte_idx = 0; byte_idx < packet.size(); byte_idx++)
    {
        checksum ^= packet[byte_idx];
    }

    packet.push_back(checksum);

    std::lock_guard<std::mutex> guard(write_mutex);

    int written = serial_port_interface.serial_write((char*)packet.data(), (int)packet.size());

    return(written == (int)packet.size());
}
