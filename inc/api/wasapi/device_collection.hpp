#pragma once

#include "api/wasapi/interface_wrapper.hpp"
#include "api/wasapi/device.hpp"
#include "exceptions.hpp"

#include <mmdeviceapi.h> // IMMDevice, IMMDeviceCollection

namespace cynth::api::wasapi {

    template <>
    class interface_wrapper<IMMDeviceCollection>: public interface_base<IMMDeviceCollection> {
        friend class interface_base<IMMDeviceCollection>;
        template <typename OtherInterface> friend class interface_wrapper;

    public:
        using interface_base<IMMDeviceCollection>::interface_base;
        using interface_base<IMMDeviceCollection>::operator=;

        std::size_t count () const {
            UINT device_count;
            (*this)->GetCount(&device_count) >> hr_handler{"IMMDeviceCollection::GetCount"};
            return device_count;
        }

        interface_wrapper<IMMDevice> item (std::size_t i) const {
            IMMDevice* device_ptr;
            (*this)->Item(i, &device_ptr) >> hr_handler{"IMMDeviceCollection::Item"};
            return {device_ptr};
        }
    };

}