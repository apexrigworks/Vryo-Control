/*---------------------------------------------------------*\
| VryoNvml.cpp                                              |
|                                                           |
|   Reads GPU telemetry from the NVIDIA driver (NVML).      |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "VryoNvml.h"

#include <QStringList>

namespace
{
    /*-----------------------------------------------------*\
    | Values from nvml.h, written out here so no NVIDIA     |
    | header is needed                                      |
    \*-----------------------------------------------------*/
    const int VRYO_NVML_OK                  = 0;
    const int VRYO_NVML_SENSOR_GPU          = 0;
    const int VRYO_NVML_CLOCK_GRAPHICS      = 0;
    const int VRYO_NVML_CLOCK_MEMORY        = 2;
}

VryoNvml::VryoNvml()
{
}

VryoNvml::~VryoNvml()
{
    Stop();
}

bool VryoNvml::OpenDriverLibrary()
{
    QStringList candidates;

#ifdef _WIN32
    candidates << QStringLiteral("nvml")
               << QStringLiteral("C:/Windows/System32/nvml.dll")
               << QStringLiteral("C:/Program Files/NVIDIA Corporation/NVSMI/nvml.dll");
#else
    candidates << QStringLiteral("libnvidia-ml.so.1")
               << QStringLiteral("libnvidia-ml.so")
               << QStringLiteral("nvidia-ml");
#endif

    for(int i = 0; i < candidates.size(); i++)
    {
        library.setFileName(candidates[i]);

        if(library.load())
        {
            return(true);
        }
    }

    return(false);
}

QFunctionPointer VryoNvml::Resolve(const char * first_name, const char * second_name)
{
    QFunctionPointer pointer = library.resolve(first_name);

    if((pointer == nullptr) && (second_name != nullptr))
    {
        pointer = library.resolve(second_name);
    }

    return(pointer);
}

bool VryoNvml::Start()
{
    Stop();

    if(!OpenDriverLibrary())
    {
        status_message = QStringLiteral("The NVIDIA driver was not found. Install an NVIDIA graphics driver to see GPU readings here.");
        return(false);
    }

    /*-----------------------------------------------------*\
    | The functions every reading needs                     |
    \*-----------------------------------------------------*/
    fn_init             = reinterpret_cast<FnVoid>(Resolve("nvmlInit_v2", "nvmlInit"));
    fn_shutdown         = reinterpret_cast<FnVoid>(Resolve("nvmlShutdown"));
    fn_get_count        = reinterpret_cast<FnGetCount>(Resolve("nvmlDeviceGetCount_v2", "nvmlDeviceGetCount"));
    fn_get_handle       = reinterpret_cast<FnGetHandle>(Resolve("nvmlDeviceGetHandleByIndex_v2", "nvmlDeviceGetHandleByIndex"));
    fn_get_temperature  = reinterpret_cast<FnGetTemperature>(Resolve("nvmlDeviceGetTemperature"));
    fn_get_utilization  = reinterpret_cast<FnGetUtilization>(Resolve("nvmlDeviceGetUtilizationRates"));
    fn_get_clock        = reinterpret_cast<FnGetClock>(Resolve("nvmlDeviceGetClockInfo"));
    fn_get_memory       = reinterpret_cast<FnGetMemory>(Resolve("nvmlDeviceGetMemoryInfo"));

    /*-----------------------------------------------------*\
    | Optional functions: a missing one only hides one tile |
    \*-----------------------------------------------------*/
    fn_get_name         = reinterpret_cast<FnGetName>(Resolve("nvmlDeviceGetName"));
    fn_get_max_clock    = reinterpret_cast<FnGetClock>(Resolve("nvmlDeviceGetMaxClockInfo"));
    fn_get_fan          = reinterpret_cast<FnGetUInt>(Resolve("nvmlDeviceGetFanSpeed"));
    fn_get_power        = reinterpret_cast<FnGetUInt>(Resolve("nvmlDeviceGetPowerUsage"));
    fn_get_power_limit  = reinterpret_cast<FnGetUInt>(Resolve("nvmlDeviceGetEnforcedPowerLimit"));
    fn_get_driver       = reinterpret_cast<FnGetDriver>(Resolve("nvmlSystemGetDriverVersion"));

    if((fn_init == nullptr) || (fn_shutdown == nullptr) || (fn_get_count == nullptr) || (fn_get_handle == nullptr)
     || (fn_get_temperature == nullptr) || (fn_get_utilization == nullptr) || (fn_get_clock == nullptr) || (fn_get_memory == nullptr))
    {
        Stop();
        status_message = QStringLiteral("The NVIDIA driver library is too old or incomplete. Update your NVIDIA graphics driver.");
        return(false);
    }

    if(fn_init() != VRYO_NVML_OK)
    {
        Stop();
        status_message = QStringLiteral("The NVIDIA driver could not be started. Try restarting the computer or reinstalling the driver.");
        return(false);
    }

    initialised = true;

    /*-----------------------------------------------------*\
    | Find the GPUs                                         |
    \*-----------------------------------------------------*/
    unsigned int count = 0;

    if((fn_get_count(&count) != VRYO_NVML_OK) || (count == 0))
    {
        Stop();
        status_message = QStringLiteral("No NVIDIA GPU was found.");
        return(false);
    }

    for(unsigned int index = 0; index < count; index++)
    {
        void * handle = nullptr;

        if((fn_get_handle(index, &handle) != VRYO_NVML_OK) || (handle == nullptr))
        {
            continue;
        }

        QString name;

        if(fn_get_name != nullptr)
        {
            char buffer[128] = { 0 };

            if(fn_get_name(handle, buffer, (unsigned int)sizeof(buffer)) == VRYO_NVML_OK)
            {
                name = QString::fromLatin1(buffer);
            }
        }

        if(name.isEmpty())
        {
            name = QStringLiteral("GPU %1").arg((int)index + 1);
        }

        handles.push_back(handle);
        names.push_back(name);
    }

    if(handles.empty())
    {
        Stop();
        status_message = QStringLiteral("No NVIDIA GPU could be opened.");
        return(false);
    }

    if(fn_get_driver != nullptr)
    {
        char buffer[96] = { 0 };

        if(fn_get_driver(buffer, (unsigned int)sizeof(buffer)) == VRYO_NVML_OK)
        {
            driver_version = QString::fromLatin1(buffer);
        }
    }

    status_message.clear();
    ready = true;

    return(true);
}

void VryoNvml::Stop()
{
    if(initialised && (fn_shutdown != nullptr))
    {
        fn_shutdown();
    }

    initialised = false;
    ready       = false;

    handles.clear();
    names.clear();
    driver_version.clear();
    status_message.clear();

    fn_init             = nullptr;
    fn_shutdown         = nullptr;
    fn_get_count        = nullptr;
    fn_get_handle       = nullptr;
    fn_get_name         = nullptr;
    fn_get_temperature  = nullptr;
    fn_get_utilization  = nullptr;
    fn_get_clock        = nullptr;
    fn_get_max_clock    = nullptr;
    fn_get_fan          = nullptr;
    fn_get_memory       = nullptr;
    fn_get_power        = nullptr;
    fn_get_power_limit  = nullptr;
    fn_get_driver       = nullptr;

    if(library.isLoaded())
    {
        library.unload();
    }
}

bool VryoNvml::IsReady() const
{
    return(ready);
}

QString VryoNvml::StatusMessage() const
{
    return(status_message);
}

int VryoNvml::GpuCount() const
{
    return((int)handles.size());
}

QString VryoNvml::GpuName(int index) const
{
    if((index < 0) || (index >= (int)names.size()))
    {
        return(QString());
    }

    return(names[index]);
}

QString VryoNvml::DriverVersion() const
{
    return(driver_version);
}

VryoGpuSample VryoNvml::Read(int index)
{
    VryoGpuSample sample;

    if(!ready || (index < 0) || (index >= (int)handles.size()))
    {
        return(sample);
    }

    void *          handle  = handles[index];
    unsigned int    value   = 0;

    /*-----------------------------------------------------*\
    | Temperature                                           |
    \*-----------------------------------------------------*/
    if(fn_get_temperature(handle, VRYO_NVML_SENSOR_GPU, &value) == VRYO_NVML_OK)
    {
        sample.temperature_c    = (int)value;
        sample.valid            = true;
    }

    /*-----------------------------------------------------*\
    | Load                                                  |
    \*-----------------------------------------------------*/
    Utilization utilization = { 0, 0 };

    if(fn_get_utilization(handle, &utilization) == VRYO_NVML_OK)
    {
        sample.load_percent     = (int)utilization.gpu;
        sample.valid            = true;
    }

    /*-----------------------------------------------------*\
    | Clocks                                                |
    \*-----------------------------------------------------*/
    value = 0;
    if(fn_get_clock(handle, VRYO_NVML_CLOCK_GRAPHICS, &value) == VRYO_NVML_OK)
    {
        sample.core_clock_mhz   = (int)value;
        sample.valid            = true;
    }

    value = 0;
    if(fn_get_clock(handle, VRYO_NVML_CLOCK_MEMORY, &value) == VRYO_NVML_OK)
    {
        sample.memory_clock_mhz = (int)value;
        sample.valid            = true;
    }

    if(fn_get_max_clock != nullptr)
    {
        value = 0;
        if(fn_get_max_clock(handle, VRYO_NVML_CLOCK_GRAPHICS, &value) == VRYO_NVML_OK)
        {
            sample.core_clock_max_mhz   = (int)value;
        }

        value = 0;
        if(fn_get_max_clock(handle, VRYO_NVML_CLOCK_MEMORY, &value) == VRYO_NVML_OK)
        {
            sample.memory_clock_max_mhz = (int)value;
        }
    }

    /*-----------------------------------------------------*\
    | Fan (some cards, laptops and zero-fan modes report    |
    | nothing here, which leaves the value at -1)           |
    \*-----------------------------------------------------*/
    if(fn_get_fan != nullptr)
    {
        value = 0;
        if(fn_get_fan(handle, &value) == VRYO_NVML_OK)
        {
            sample.fan_percent  = (int)value;
        }
    }

    /*-----------------------------------------------------*\
    | Video memory                                          |
    \*-----------------------------------------------------*/
    MemoryInfo memory = { 0, 0, 0 };

    if(fn_get_memory(handle, &memory) == VRYO_NVML_OK)
    {
        sample.memory_used_mib  = (double)memory.used  / (1024.0 * 1024.0);
        sample.memory_total_mib = (double)memory.total / (1024.0 * 1024.0);
        sample.valid            = true;
    }

    /*-----------------------------------------------------*\
    | Power (NVML reports milliwatts)                       |
    \*-----------------------------------------------------*/
    if(fn_get_power != nullptr)
    {
        value = 0;
        if(fn_get_power(handle, &value) == VRYO_NVML_OK)
        {
            sample.power_watts  = (double)value / 1000.0;
        }
    }

    if(fn_get_power_limit != nullptr)
    {
        value = 0;
        if(fn_get_power_limit(handle, &value) == VRYO_NVML_OK)
        {
            sample.power_limit_watts = (double)value / 1000.0;
        }
    }

    return(sample);
}
