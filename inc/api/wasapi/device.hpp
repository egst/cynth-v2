#pragma once

#include "api/wasapi/interface_wrapper.hpp"
#include "api/wasapi/audio_client.hpp"
#include "api/wasapi/property_store.hpp"

#include <audioclient.h> // IAudioClient
#include <mmdeviceapi.h> // IMMDevice

#include <string>

namespace cynth::api::wasapi {

    template <>
    class interface_wrapper<IMMDevice>: public interface_base<IMMDevice> {
        friend class interface_base<IMMDevice>;
        template <typename OtherInterface> friend class interface_wrapper;
    
    public:
        using interface_base<IMMDevice>::interface_base;
        using interface_base<IMMDevice>::operator=;

        //using byte_t = cynth::pcm::byte_t;

        std::string id   () const { return this->id_; }
        std::string name () const { return this->name_; }

        /*/ Move: /*/
        interface_wrapper (interface_wrapper&& other):
            interface_base{std::move(other.base())},
            audio_client_ {std::move(other.audio_client_)},
            id_           {std::move(other.id_)},
            name_         {std::move(other.name_)} {}
        interface_wrapper& operator= (interface_wrapper&& other) {
            this->base()        = std::move(other.base());
            this->audio_client_ = std::move(other.audio_client_);
            this->id_           = std::move(other.id_);
            this->name_         = std::move(other.name_);
            return *this;
        }
        
    private:
        /*/ Construction: /*/
        interface_wrapper (IMMDevice* interface_ptr):
            interface_base{interface_ptr},
            audio_client_{this->get_audio_client()},
            id_{this->get_id()},
            name_{this->get_name()} {}

        interface_wrapper<IAudioClient> get_audio_client () {
            IAudioClient* audio_client_ptr;
            (*this)->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, reinterpret_cast<void**>(&audio_client_ptr)) >> hr_handler{"IMMDevice::Activate"};
            return {audio_client_ptr};
        }

        std::string get_id () {
            LPWSTR id;
            (*this)->GetId(&id) >> hr_handler{"IMMDevice::GetId"};
            return string_tools::to_string(id);
        }

        interface_wrapper<IPropertyStore> open_property_store () {
            DWORD access = STGM_READ; // TODO?
            IPropertyStore* property_store_ptr;
            (*this)->OpenPropertyStore(access, &property_store_ptr) >> hr_handler{"IMMDevice::OpenPropertyStore"};
            return {property_store_ptr};
        }

        using pkey = interface_wrapper<IPropertyStore>::pkey;

        std::string get_name () {
            auto property_store = this->open_property_store();
            return property_store.value(pkey::PKEY_Device_FriendlyName);
        }

        const interface_wrapper<IAudioClient>& audio_client () const { return this->audio_client_; }

        interface_wrapper& move (interface_wrapper&& other) {
            this->id_           = std::move(other.id_);
            this->name_         = std::move(other.name_);
            this->audio_client_ = std::move(other.audio_client_);
            //auto audio_client{std::move(other.audio_client_)};

            return *this;
        }

        interface_wrapper<IAudioClient> audio_client_;
        std::string id_;
        std::string name_;
    };

}