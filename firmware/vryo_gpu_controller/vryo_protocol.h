/*---------------------------------------------------------*\
| vryo_protocol.h                                           |
|                                                           |
|   Frame parser for the Vryo GPU lighting board. Plain     |
|   C++ with no Arduino calls, so the same file runs on the |
|   board and in the PC test (firmware/tests).              |
|                                                           |
|   Packet (matches VryoGPUController.cpp on the PC side):  |
|     0xAA 0x55 count_lo count_hi  R G B (x count)  xor     |
|   xor = XOR of every byte before it.                      |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <stdint.h>
#include <stddef.h>

#define VRYO_MAGIC_0        0xAA
#define VRYO_MAGIC_1        0x55
#define VRYO_MAX_LEDS       1024        // frames claiming more are rejected

template <size_t MAX_STORED_LEDS>
class VryoFrameParser
{
public:
    VryoFrameParser() { Reset(); }

    void Reset()
    {
        state       = WAIT_MAGIC_0;
        led_total   = 0;
        byte_idx    = 0;
        checksum    = 0;
    }

    /*-----------------------------------------------------*\
    | Feed one received byte. Returns true when a complete  |
    | valid frame has just been received; the colours are   |
    | then in Colors() (first StoredCount() LEDs).          |
    \*-----------------------------------------------------*/
    bool Feed(uint8_t value)
    {
        switch(state)
        {
            case WAIT_MAGIC_0:
                if(value == VRYO_MAGIC_0)
                {
                    checksum = value;
                    state    = WAIT_MAGIC_1;
                }
                break;

            case WAIT_MAGIC_1:
                if(value == VRYO_MAGIC_1)
                {
                    checksum ^= value;
                    state     = COUNT_LO;
                }
                else
                {
                    /* a stray 0xAA followed by 0xAA is a new start */
                    state = (value == VRYO_MAGIC_0) ? WAIT_MAGIC_1 : WAIT_MAGIC_0;
                    checksum = (value == VRYO_MAGIC_0) ? value : 0;
                }
                break;

            case COUNT_LO:
                led_total = value;
                checksum ^= value;
                state     = COUNT_HI;
                break;

            case COUNT_HI:
                led_total |= (uint16_t)value << 8;
                checksum  ^= value;

                if(led_total == 0 || led_total > VRYO_MAX_LEDS)
                {
                    /* the byte we just read may be the start of the  */
                    /* next real header, so do not throw it away      */
                    Reset();

                    if(value == VRYO_MAGIC_0)
                    {
                        checksum = value;
                        state    = WAIT_MAGIC_1;
                    }
                }
                else
                {
                    byte_idx = 0;
                    state    = PIXELS;
                }
                break;

            case PIXELS:
                checksum ^= value;

                if(byte_idx < (MAX_STORED_LEDS * 3))
                {
                    rgb[byte_idx] = value;
                }

                byte_idx++;

                if(byte_idx >= ((uint32_t)led_total * 3))
                {
                    state = CHECKSUM;
                }
                break;

            case CHECKSUM:
            {
                const bool ok = (value == checksum);
                const uint16_t total = led_total;

                Reset();

                if(ok)
                {
                    last_total = total;
                    return(true);
                }
                break;
            }
        }

        return(false);
    }

    /*-----------------------------------------------------*\
    | LEDs the PC sent in the last good frame               |
    \*-----------------------------------------------------*/
    uint16_t        FrameLedCount() const   { return(last_total); }

    /*-----------------------------------------------------*\
    | LEDs actually kept (frames longer than the buffer are |
    | cut, shorter frames leave the rest unchanged)         |
    \*-----------------------------------------------------*/
    uint16_t        StoredCount() const
    {
        return((last_total < MAX_STORED_LEDS) ? last_total : (uint16_t)MAX_STORED_LEDS);
    }

    const uint8_t * Colors() const          { return(rgb); }

private:
    enum State { WAIT_MAGIC_0, WAIT_MAGIC_1, COUNT_LO, COUNT_HI, PIXELS, CHECKSUM };

    State       state;
    uint16_t    led_total;
    uint16_t    last_total = 0;
    uint32_t    byte_idx;
    uint8_t     checksum;
    uint8_t     rgb[MAX_STORED_LEDS * 3];
};
