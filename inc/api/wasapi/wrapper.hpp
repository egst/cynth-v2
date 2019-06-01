#pragma once

#include "api/wasapi/interface_wrapper.hpp"
#include "api/wasapi/device_enumerator.hpp"
#include "api/wasapi/device_collection.hpp"
#include "api/wasapi/device.hpp"
//#include "user_library/devices/sound_card.hpp"

#include <mmdeviceapi.h> // IMMDevice, IMMDeviceEnumerator, IMMDeviceCollection
#include <objbase.h> // CoInitialize

#include <vector>

#include <iostream>

namespace cynth::api::wasapi {

    class wrapper {
    public:
        wrapper () {
            CoInitialize(NULL);
            interface_wrapper<IMMDeviceEnumerator> device_enumerator;
            auto device_collection = device_enumerator.get_device_collection();

            //auto default_device = device_enumerator.get_default_device();

            std::cout << "WASAPI setup:\n\n";
            std::cout << "Available devices:\n";

            auto count = device_collection.count();
            for (std::size_t i = 0; i < count; ++i)
                std::cout << i << ": " << device_collection.item(i).name() << "\n";
            std::cout << "\n";

            std::cout
                << "Choose devices to use:\n"
                << "Enter numbers from 0" << " to " << count - 1 << " separated by spaces or new lines.\n"
                << "Enter ^D after the last one.\n"
                << "The order of added devices will dictate their order as separate channels.\n";

            std::size_t chosen;
            while (std::cin >> chosen) {
                if (!(chosen >= 0 && chosen < count)) {
                    std::cout << "No such device.\n";
                    continue;
                }
                this->rendering_devices_.emplace_back(device_collection.item(chosen));
                std::cout << this->rendering_devices_.back().name() << " was added as channel " << this->rendering_devices_.size() - 1 << ".\n";
            }
            std::cout << "\n";

            std::cout << "You're good to go.\n";

        }

    private:

        std::vector<interface_wrapper<IMMDevice>> rendering_devices_;
    };

}