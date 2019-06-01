#pragma once

//#define IEEE754_64FLOAT 1
// For some reason, this macro has different value in my code and in the asio code.
// As a result, ASIOSampleRate is defined as double in one place and as a struct holding an array in other.
// This produces an "undefined reference to ASIOGetSampleRate(ASIOSampleRate*)" linker error.
// TODO!

#include "exceptions.hpp"
#include "api/asio/tools.hpp"
#include "api/asio/driver.hpp"
#include "devices/oscillator.hpp"
#include "devices/filter.hpp"

//#include "asiosys.h"
#include "asio.h"
#include "asiodrivers.h"

#undef min

//#include <timeapi.h>
//#include <windows.h>

#include <iostream>
#include <cstddef>
#include <cstring>

#include <chrono>
#include <mutex>
#include <thread>

namespace cynth::api::asio {

    class wrapper {
    public:
        wrapper () {

            /* -- Load driver: ---------------------------------------------- */

            auto driver_names = driver::list_drivers();

            std::cout << "ASIO setup:\n\n";
            std::cout << "Available drivers:\n";

            std::size_t driver_count = 0;
            for (auto&& driver_name: driver_names) {
                std::cout << driver_count << ": " << driver_name << '\n';
                ++driver_count;
            }
            std::cout << '\n';

            std::cout
                << "Choose a driver to use:\n"
                << "Enter a number from 0" << " to " << driver_count - 1 << ".\n";

            std::size_t chosen;
            while (std::cin >> chosen) {
                if (!(chosen >= 0 && chosen < driver_count)) {
                    std::cout << "No such driver::\n";
                    continue;
                }
                break;
            }

            bool loaded = driver::load_driver(driver_names[chosen]);
            if (loaded)
                std::cout << driver_names[chosen] << " was loaded.\n\n";
            else {
                std::cout << driver_names[chosen] << " could not be loaded.\n\n";
                return;
            }

            // TODO: getCurrentDriverName to skip this when driver is set.

            /* -- Start: ---------------------------------------------------- */

            std::thread{&wrapper::start, this}.detach();
        }

        void start () {
            driver::full_init();
            
            while (true) {
                std::cout << "ASIOStart()\n";
                ASIOStart() >> ase_handler{"ASIOStart"};
                //break;

                /* The locking process:

                ASIO callbacks remain lock-free as they acquire a shared ownership of the operation_mutex.
                These callbacks owning the operation_mutex means, that at this moment
                it is not safe to dispose buffers or even exit ASIO completely (ASIODisposeBuffers, ASIOExit).
                To stop or reset ASIO, the operation_mutex must be owned exclusively.
                When anyone manages to acquire exclusive ownership,
                ASIO callbacks fail to acquire shared ownership and return immediately.
                Callbacks that are writing to buffers cannot write when the mutex is owned exclusively
                as the driver may be in the process of stopping/restarting and the buffers may not be valid.
                Callbacks that are responsible for various events notifications are simply ignored,
                as no such events are relevant when the driver is stopping/restarting.
                TODO: Check if my understanding of shared/exclusive ownership if correct.
                */

                std::unique_lock<std::mutex> stop_guard{driver::stop_mutex};
                driver::stop_signal.wait(stop_guard);
                std::unique_lock<std::shared_mutex> operation_guard{driver::operation_mutex};

                switch (driver::stop_type) {
                case driver::stop_enum::RESET:
                    ASIOStop() >> ase_handler{"ASIOStop"};
                    ASIODisposeBuffers() >> ase_handler{"ASIODisposeBuffers"};
                    ASIOExit() >> ase_handler{"ASIOExit"};
                    driver::full_init();
                    continue;
                case driver::stop_enum::SRATE_RESET:
                    // TODO
                    driver::full_init();
                    continue;
                case driver::stop_enum::FULL:
                default:
                    break;
                }

                ASIOStop() >> ase_handler{"ASIOStop"};
                std::cout << "ASIOStop()\n";
                break;
            }
        }
    };

}