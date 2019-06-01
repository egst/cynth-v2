#pragma once

#include "config.hpp"
#include "containertools.hpp"
#include "exceptions.hpp"
#include "api/asio/tools.hpp"
#include "api/asio/buffertools.hpp"

#include "wavetables.hpp"
#include "devices/oscillator.hpp" 

#include "asio.h"
#include "asiodrivers.h"

#include <iostream>
#include <bitset>

#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

// From ASIO SDK:
extern AsioDrivers* asioDrivers;
bool loadAsioDriver (char *name);

namespace cynth::api::asio {
    struct driver {
    public:
        static std::vector<std::string> list_drivers () {
            constexpr unsigned_t max_driver_count = 32;
            constexpr unsigned_t driver_name_size = 32;

            AsioDrivers drivers;
            container_tools::dynamic_string_array driver_names{max_driver_count, driver_name_size};
            unsigned_t driver_count = drivers.getDriverNames(driver_names, max_driver_count);
            return driver_names.in_vector(driver_count);
        }

        static AsioDrivers& drivers () {
            if (!asioDrivers)
                throw asio_exception{"asioDrivers not initialized"};
            return *asioDrivers;
        }

        static bool load_driver (std::string name) { return loadAsioDriver(name.data()); }

        static void full_init () {
            driver::init();
            driver::get_channel_count();
            driver::get_buffer_sizes();
            driver::get_sample_rate();
            driver::check_outready_optimization();
            driver::set_callbacks();
            driver::configure_channels();
            driver::create_buffers();
            driver::get_channel_info();
            driver::get_latencies();
        }

        static void init () {
            ASIOInit(&driver::driver_info) >> ase_handler{"ASIOInit"};
        }

        static void get_channel_count () {
            long input_channel_count;
            long output_channel_count;
            ASIOGetChannels(
                &input_channel_count,
                &output_channel_count) >> ase_handler{"ASIOGetChannels"};
            driver::input_channel_count  = static_cast<unsigned_t>(input_channel_count);
            driver::output_channel_count = static_cast<unsigned_t>(output_channel_count);
        }

        static void get_buffer_sizes () {
            long min_buffer_size;
            long max_buffer_size;
            long preferred_buffer_size;
            long buffer_sizes_granularity;
            ASIOGetBufferSize(
                &min_buffer_size,
                &max_buffer_size,
                &preferred_buffer_size,
                &buffer_sizes_granularity) >> ase_handler{"ASIOGetBufferSize"};
            driver::min_buffer_size          = static_cast<unsigned_t>(min_buffer_size);
            driver::max_buffer_size          = static_cast<unsigned_t>(max_buffer_size);
            driver::preferred_buffer_size    = static_cast<unsigned_t>(preferred_buffer_size);
            driver::buffer_sizes_granularity = static_cast<signed_t>(buffer_sizes_granularity);
        }

        static void get_sample_rate () {
            ASIOSampleRate sample_rate;
            ASIOGetSampleRate(&sample_rate) >> ase_handler{"ASIOGetSampleRate"};
            // TODO: ASIOCanSampleRate, ASIOSetSampleRate when the sample rate is not stored in the driver.
            driver::sample_rate = tools::native_floating(sample_rate);
        }

        static void check_outready_optimization () {
            driver::outready_optimization = ASIOOutputReady() == ASE_OK;
        }

        static void set_callbacks () {
            driver::callbacks = {
                driver::buffer_switch,
                driver::sample_rate_changed,
                driver::asio_messages,
                driver::buffer_switch_time_info
            };
        }

        static void configure_channels () {
            driver::input_buffer_count = std::min(driver::input_channel_count, driver::max_input_channel_count);
            for(unsigned_t i = 0; i < driver::input_buffer_count; ++i) {
                auto& buffer_info = driver::buffer_infos[i];
                buffer_info.isInput    = ASIOTrue;
                buffer_info.channelNum = i;
                buffer_info.buffers[0] = 0;
                buffer_info.buffers[1] = 0;
            }
            
            driver::output_buffer_count = std::min(driver::output_channel_count, driver::max_output_channel_count);
            for(unsigned_t i = 0; i < driver::output_buffer_count; ++i) {
                auto& buffer_info = driver::buffer_infos[i + driver::input_buffer_count];
                buffer_info.isInput    = ASIOFalse;
                buffer_info.channelNum = i;
                buffer_info.buffers[0] = 0;
                buffer_info.buffers[1] = 0;
            }
        }

        static void create_buffers () {
            ASIOCreateBuffers(
                driver::buffer_infos,
                driver::input_channel_count + driver::output_channel_count,
                driver::preferred_buffer_size,
                &driver::callbacks) >> ase_handler{"ASIOCreateBuffers"};
        }

        static void get_channel_info () {
            for (unsigned_t i = 0; i < driver::input_buffer_count + driver::output_channel_count; ++i) {
                auto& buffer_info  = driver::buffer_infos[i];
                auto& channel_info = driver::channel_infos[i];

                channel_info.channel = buffer_info.channelNum;
                channel_info.isInput = buffer_info.isInput;

                ASIOGetChannelInfo(&channel_info) >> ase_handler{"ASIOGetChannelInfo"};
            }
        }

        static void get_latencies () {
            // Note from the docs:
            // input latency is the age of the first sample in the currently returned audio block
			// output latency is the time the first sample in the currently returned audio block requires to get to the output

            long input_latency;
            long output_latency;
            ASIOGetLatencies(
                &input_latency,
                &output_latency) >> ase_handler{"ASIOGetLatencies"};
            driver::input_latency  = static_cast<unsigned_t>(input_latency);
            driver::output_latency = static_cast<unsigned_t>(output_latency);
        }

        constexpr static unsigned_t max_input_channel_count  = 32;
        constexpr static unsigned_t max_output_channel_count = 32;

        // TODO: atomic
        inline static ASIODriverInfo  driver_info;
        inline static unsigned_t      input_channel_count;
        inline static unsigned_t      output_channel_count;
        inline static unsigned_t      min_buffer_size;
        inline static unsigned_t      max_buffer_size;
        inline static unsigned_t      preferred_buffer_size;
        inline static signed_t        buffer_sizes_granularity;
        inline static floating_t      sample_rate; // ASIOSampleRate
        inline static bool            outready_optimization;
        inline static unsigned_t      input_latency;
        inline static unsigned_t      output_latency;
        inline static unsigned_t      input_buffer_count;
        inline static unsigned_t      output_buffer_count;
        inline static ASIOBufferInfo  buffer_infos[max_input_channel_count + max_output_channel_count];
        inline static ASIOChannelInfo channel_infos[max_input_channel_count + max_output_channel_count];
        inline static floating_t      sample_pos_ns;
        inline static floating_t      sample_pos_samples;
        inline static floating_t      time_code_samples;
        inline static ASIOTime        time;
        inline static unsigned_t      system_reference_time;
        //inline static unsigned_t      processed_sample_count;
        inline static ASIOCallbacks   callbacks;
        
        enum stop_enum { FULL, RESET, SRATE_RESET };
        inline static std::mutex              stop_mutex;
        inline static std::shared_mutex       operation_mutex;
        inline static std::condition_variable stop_signal;
        inline static std::atomic<stop_enum>  stop_type;

        static void request_stop (stop_enum type = FULL) {
            driver::stop_type = type;
            driver::stop_signal.notify_all();
        }

        static void buffer_switch (long index, ASIOBool direct_process) {
            {
                std::shared_lock<std::shared_mutex> guard{operation_mutex, std::try_to_lock};
                if (!guard.owns_lock())
                    return;

                // From the docks: a timeInfo needs to be created though it will only set the timeInfo.samplePosition and timeInfo.systemTime fields and the according flags
                ASIOTime time{};

                // From the docs: get the time stamp of the buffer (for synchronization with other media)
                if (ASIOGetSamplePosition(&time.timeInfo.samplePosition, &time.timeInfo.systemTime) == ASE_OK)
                    time.timeInfo.flags = AsioTimeInfoFlags::kSystemTimeValid | AsioTimeInfoFlags::kSamplePositionValid;
            }

            driver::buffer_switch_time_info (&time, index, direct_process);
        }

        static void sample_rate_changed (ASIOSampleRate sRate) {
            std::shared_lock<std::shared_mutex> guard{operation_mutex, std::try_to_lock};
            if (!guard.owns_lock())
                return;

            /* From the docs:
                Do whatever you need to do if the sample rate changed.
                Usually this only happens during external sync.
                Audio processing is not stopped by the driver, actual sample rate
                might not have even changed, maybe only the sample rate status of an
                AES/EBU or S/PDIF digital input at the audio device.
                You might have to update time/sample related conversion routines, etc. */
            
            driver::request_stop(SRATE_RESET);
        }

        static long asio_messages (long selector, long value, void* message, double* opt) {
            (void) message;
            (void) opt;

            std::shared_lock<std::shared_mutex> guard{operation_mutex, std::try_to_lock};
            if (!guard.owns_lock())
                return 0;
            
            // TODO: This was just coppied from the docs.
            // The messages, that are not implemented perform a full stop of the driver.
            switch (selector) {
            case kAsioSelectorSupported:
                if(value == kAsioResetRequest
                || value == kAsioEngineVersion
                || value == kAsioResyncRequest
                || value == kAsioLatenciesChanged
                || value == kAsioSupportsTimeInfo
                || value == kAsioSupportsTimeCode
                || value == kAsioSupportsInputMonitor) {

                    driver::request_stop();
                    return 1;
                }
                break;
            case kAsioResetRequest:
                /* From the docs:
                    defer the task and perform the reset of the driver during the next "safe" situation
                    You cannot reset the driver right now, as this code is called from the driver.
                    Reset the driver is done by completely destruct is. I.e. ASIOStop(), ASIODisposeBuffers(), Destruction
                    Afterwards you initialize the driver again. */
                driver::request_stop(RESET);
                return 1;
            case kAsioResyncRequest:
                /* From the docs:
                    This informs the application, that the driver encountered some non fatal data loss.
                    It is used for synchronization purposes of different media.
                    Added mainly to work around the Win16Mutex problems in Windows 95/98 with the
                    Windows Multimedia system, which could loose data because the Mutex was hold too long
                    by another thread.
                    However a driver can issue it in other situations, too. */
                driver::request_stop();
                return 1;
            case kAsioLatenciesChanged:
                /* From the docs:
                    This will inform the host application that the drivers were latencies changed.
                    Beware, it this does not mean that the buffer sizes have changed!
                    You might need to update internal delay data. */
                driver::request_stop();
                return 1;
            case kAsioEngineVersion:
                /* From the docs:
                    return the supported ASIO version of the host application
                    If a host applications does not implement this selector, ASIO 1.0 is assumed
                    by the driver */
                driver::request_stop();
                return 2;
            case kAsioSupportsTimeInfo:
                /* From the docs:
                    informs the driver wether the asioCallbacks.bufferSwitchTimeInfo() callback
                    is supported.
                    For compatibility with ASIO 1.0 drivers the host application should always support
                    the "old" bufferSwitch method, too. */
                /*driver::stop_type = FULL;
                driver::stop_signal.notify_all();*/
                return 1;
            case kAsioSupportsTimeCode:
                /* From the docs:
                    informs the driver wether application is interested in time code info.
                    If an application does not need to know about time code, the driver has less work
                    to do. */
                driver::request_stop();
                return 0;
            }
            return 0;
        }

        static inline wave_function sample;

        static ASIOTime* buffer_switch_time_info (ASIOTime* time_ptr, long index, ASIOBool direct_process) {
            // TODO: Docs, page 8: First few call to bufferSwitch should be ignored.

            std::shared_lock<std::shared_mutex> guard{operation_mutex, std::try_to_lock};
            if (!guard.owns_lock())
                return 0;

            // From the docs: store the timeInfo for later use
            driver::time = *time_ptr;

            // From the docs: get the time stamp of the buffer (for synchronization with other media)
            driver::sample_pos_ns = time_ptr->timeInfo.flags & kSystemTimeValid
                ? tools::native_floating(time_ptr->timeInfo.systemTime)
                : 0;
            driver::sample_pos_samples = time_ptr->timeInfo.flags & kSamplePositionValid
                ? tools::native_floating(time_ptr->timeInfo.samplePosition)
                : 0;
            driver::time_code_samples = time_ptr->timeCode.flags & kTcValid
                ? tools::native_floating(time_ptr->timeCode.timeCodeSamples)
                : 0;

            // From the docs: get the system reference time
            driver::system_reference_time = timeGetTime(); // TODO: From which header is this?

            auto buffer_size = driver::preferred_buffer_size;
            
            for (std::size_t i = 0; i < driver::input_buffer_count + driver::output_buffer_count; ++i) {
                auto& buffer_info  = driver::buffer_infos[i];
                auto& channel_info = driver::channel_infos[i];
                if (buffer_info.isInput == ASIOFalse) {
                    auto buff = buffer{buffer_info.buffers[index], channel_info.type, buffer_size};
                    {
                        unsigned_t j = 0; for (auto&& s: buff) {
                            auto seconds = (sample_pos_samples + j) / driver::sample_rate;
                            s = driver::sample(seconds);
                            //std::cout << sample_pos_samples + j << ": " << std::to_string(driver::sample(seconds)) << '\n';
                            ++j;
                        }
                    }
                }
            }

            // From the docs: finally if the driver supports the ASIOOutputReady() optimization, do it here, all data are in place
            if (driver::outready_optimization)
                ASIOOutputReady() >> ase_handler{"ASIOOutputReady"};

            return 0;
        }
    };
}