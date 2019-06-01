#pragma once

#include "api/wasapi/interface_wrapper.hpp"
#include "api/wasapi/device_collection.hpp"
#include "api/wasapi/device.hpp"
#include "exceptions.hpp"

#include <mmdeviceapi.h> // IMMDevice, IMMDeviceCollection, IMMDeviceEnumerator

#include <iostream>

namespace cynth::api::wasapi {

    template <>
    class interface_wrapper<IMMDeviceEnumerator>: public interface_base<IMMDeviceEnumerator> {
        friend class interface_base<IMMDeviceEnumerator>;
        template <typename OtherInterface> friend class interface_wrapper;

    public:
        using interface_base<IMMDeviceEnumerator>::interface_base;
        using interface_base<IMMDeviceEnumerator>::operator=;

        interface_wrapper ():
            interface_base{this->create_instance()} {}

        IMMDeviceEnumerator* create_instance () const {
            IMMDeviceEnumerator* device_enumerator_ptr = nullptr;
            CoCreateInstance(
                static_cast<const CLSID>(__uuidof(MMDeviceEnumerator)),
                NULL,
                CLSCTX_ALL,
                static_cast<const IID>(__uuidof(IMMDeviceEnumerator)),
                reinterpret_cast<void**>(&device_enumerator_ptr)) >> hr_handler{"CoCreateInstance(IMMDeviceEnumerator)"};
            return device_enumerator_ptr;
        }

        interface_wrapper<IMMDeviceCollection> get_device_collection () {
            EDataFlow data_flow = EDataFlow::eRender; // TODO?
            DWORD mask = DEVICE_STATE_ACTIVE; // TODO?

            IMMDeviceCollection* device_collection_ptr;
            (*this)->EnumAudioEndpoints(data_flow, mask, &device_collection_ptr) >> hr_handler{"IMMDeviceEnumerator::EnumAudioEndpoints"};
            return {device_collection_ptr};
        }
        interface_wrapper<IMMDevice> get_default_device () {
            EDataFlow data_flow = EDataFlow::eRender; // TODO?
            ERole role = ERole::eMultimedia; // TODO?

            IMMDevice* device_ptr;
            (*this)->GetDefaultAudioEndpoint(data_flow, role, &device_ptr) >> hr_handler{"IMMDeviceEnumerator::GetDefaultAudioEndpoint"};
            return {device_ptr};
        }
    };

}