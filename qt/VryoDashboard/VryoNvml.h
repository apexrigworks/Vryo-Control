/*---------------------------------------------------------*\
| VryoNvml.h                                                |
|                                                           |
|   Reads GPU temperature, load, clocks, fan, memory and    |
|   power from the NVIDIA driver (NVML).                    |
|                                                           |
|   NVML is loaded at run time with QLibrary, so the build  |
|   needs no NVIDIA SDK and no extra libraries. If the      |
|   driver is missing the dashboard simply says so.         |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <vector>
#include <QLibrary>
#include <QString>

/*---------------------------------------------------------*\
| One reading. A value of -1 means "not available on this   |
| card or driver" (for example the fan on a passive card).  |
\*---------------------------------------------------------*/
struct VryoGpuSample
{
    bool        valid                   = false;

    int         temperature_c           = -1;
    int         load_percent            = -1;

    int         core_clock_mhz          = -1;
    int         core_clock_max_mhz      = -1;
    int         memory_clock_mhz        = -1;
    int         memory_clock_max_mhz    = -1;

    int         fan_percent             = -1;

    double      memory_used_mib         = -1.0;
    double      memory_total_mib        = -1.0;

    double      power_watts             = -1.0;
    double      power_limit_watts       = -1.0;
};

class VryoNvml
{
public:
    VryoNvml();
    ~VryoNvml();

    /*-----------------------------------------------------*\
    | Load the driver library and find the GPUs. Safe to    |
    | call again after a failure to retry.                  |
    \*-----------------------------------------------------*/
    bool            Start();
    void            Stop();

    bool            IsReady() const;
    QString         StatusMessage() const;

    int             GpuCount() const;
    QString         GpuName(int index) const;
    QString         DriverVersion() const;

    VryoGpuSample   Read(int index);

private:
    /*-----------------------------------------------------*\
    | Function types, matching nvml.h                       |
    \*-----------------------------------------------------*/
    struct Utilization
    {
        unsigned int    gpu;
        unsigned int    memory;
    };

    struct MemoryInfo
    {
        unsigned long long  total;
        unsigned long long  free_bytes;
        unsigned long long  used;
    };

    typedef int (*FnVoid)();
    typedef int (*FnGetCount)(unsigned int *);
    typedef int (*FnGetHandle)(unsigned int, void **);
    typedef int (*FnGetName)(void *, char *, unsigned int);
    typedef int (*FnGetTemperature)(void *, int, unsigned int *);
    typedef int (*FnGetUtilization)(void *, Utilization *);
    typedef int (*FnGetClock)(void *, int, unsigned int *);
    typedef int (*FnGetUInt)(void *, unsigned int *);
    typedef int (*FnGetMemory)(void *, MemoryInfo *);
    typedef int (*FnGetDriver)(char *, unsigned int);

    QLibrary                library;
    bool                    ready               = false;
    bool                    initialised         = false;
    QString                 status_message;
    QString                 driver_version;

    std::vector<void *>     handles;
    std::vector<QString>    names;

    FnVoid                  fn_init             = nullptr;
    FnVoid                  fn_shutdown         = nullptr;
    FnGetCount              fn_get_count        = nullptr;
    FnGetHandle             fn_get_handle       = nullptr;
    FnGetName               fn_get_name         = nullptr;
    FnGetTemperature        fn_get_temperature  = nullptr;
    FnGetUtilization        fn_get_utilization  = nullptr;
    FnGetClock              fn_get_clock        = nullptr;
    FnGetClock              fn_get_max_clock    = nullptr;
    FnGetUInt               fn_get_fan          = nullptr;
    FnGetMemory             fn_get_memory       = nullptr;
    FnGetUInt               fn_get_power        = nullptr;
    FnGetUInt               fn_get_power_limit  = nullptr;
    FnGetDriver             fn_get_driver       = nullptr;

    bool                    OpenDriverLibrary();
    QFunctionPointer        Resolve(const char * first_name, const char * second_name = nullptr);
};
