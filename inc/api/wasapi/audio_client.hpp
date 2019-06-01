#pragma once

#include "api/wasapi/interface_wrapper.hpp"
#include "api/wasapi/property_store.hpp"
#include "api/wasapi/render_client.hpp"
#include "exceptions.hpp"

#include <audioclient.h> // IAudioClient, IAudioRenderCLient
#include <mmreg.h>       // WAVEFORMATEX
#include <synchapi.h>    // events

#include <cstddef>
#include <tuple>

namespace cynth::api::wasapi {

    template <>
    class interface_wrapper<IAudioClient>: public interface_base<IAudioClient> {
        friend class interface_base<IAudioClient>;
        template <typename OtherInterface> friend class interface_wrapper;

        class wave_format_wrapper {
        public:
            wave_format_wrapper (WAVEFORMATEX* wave_format_ptr):
                wave_format_ptr_{wave_format_ptr} {}
            
            bool isset () const { return this->wave_format_ptr_; }
            WAVEFORMATEX& get () const {
                if (!this->isset())
                    throw exception{"IMMDevice: Trying to access WAVEFORMAT of an uninitialized waveformat_wrapper."};
                return *this->wave_format_ptr_;
            }
            WAVEFORMATEX& operator*  () const { return this->get(); }
            WAVEFORMATEX* operator-> () const { return &this->get(); }
            
            std::size_t channel_count () const { return (*this)->nChannels; }
            std::size_t bit_depth     () const { return (*this)->wBitsPerSample; }
            std::size_t sample_rate   () const { return (*this)->nSamplesPerSec; }
        
        private:
            WAVEFORMATEX* wave_format_ptr_;
        };

    public:
        using interface_base<IAudioClient>::interface_base;
        using interface_base<IAudioClient>::operator=;

        /*/ Units of the properties below: /*/
        enum time_units {
            MS,
            HNS // 100-ns, default WASAPI buffer period meassure.
        };
        enum size_units {
            SAMPLES,
            FRAMES,
            BYTES
        };
        
        /*/ Properties: /*/
        std::size_t buffer_period (time_units units = MS) const {
            switch(units) {
            case MS: default:
                return this->default_device_period() / 10000;
            case HNS:
                return this->default_device_period();
            }
        }
        std::size_t buffer_size (size_units units = SAMPLES) const {
            switch(units) {
            case BYTES:
                return this->buffer_size_ * this->channel_count() * this->bit_depth() / 8;
            case FRAMES:
                return this->buffer_size_;
            case SAMPLES: default:
                return this->buffer_size_ * this->channel_count();
            }
        }
        std::size_t padding (size_units units = SAMPLES) const {
            switch(units) {
            case BYTES:
                return this->get_padding() * this->channel_count() * this->bit_depth() / 8;
            case FRAMES:
                return this->get_padding();
            case SAMPLES: default:
                return this->get_padding() * this->channel_count();
            }
        }
        std::size_t padded_buffer_size (size_units units = SAMPLES) const {
            return this->buffer_size(units) - this->padding(units);
        }
        std::size_t channel_count () const { return this->wave_format().channel_count(); }
        std::size_t bit_depth     () const { return this->wave_format().bit_depth(); }
        std::size_t sample_rate   () const { return this->wave_format().sample_rate(); }

        /*/ Control: /*/
        void wait_for_buffer () const {
            if (!this->initialized())
                throw exception{"IAudioClient: Trying to wait for a buffer of an uninitialized IAudioClient."};
            if (!this->initialized())
                throw exception{"IAudioClient: Trying to wait for a buffer of an IAudioClient without a buffer event."};
            if (WaitForSingleObject(this->event_buffer_, 2000) != WAIT_OBJECT_0) {
                this->stop();
                throw exception{"IAudioClient: Buffer event did not trigger."};
            }
        }
        void start () const {
            if (!this->initialized())
                throw exception{"IAudioClient: Trying to start an uninitialized IAudioClient."};
            (*this)->Start() >> hr_handler{"IAudioClient::Start"};
        }
        void stop () const {
            if (!this->initialized())
                throw exception{"IAudioClient: Trying to stop an uninitialized IAudioClient."};
            (*this)->Stop() >> hr_handler{"IAudioClient::Stop"};
        }

        /*/ Destruction: /*/
        ~interface_wrapper () {
            if (this->initialized())
                this->stop();
        }

        /*/ Move: /*/
        interface_wrapper (interface_wrapper&& other):
            interface_base {std::move(other.base())},
            initialized_   {std::move(other.initialized_)},
            wave_format_   {std::move(other.wave_format_)},
            device_periods_{std::move(other.device_periods_)},
            initializer_   {},
            buffer_size_   {std::move(other.buffer_size_)},
            event_buffer_  {std::move(other.event_buffer_)},
            render_client_ {std::move(other.render_client_)} {}

        interface_wrapper& operator= (interface_wrapper&& other) {
            this->base()          = std::move(other.base());
            this->initialized_    = std::move(other.initialized_);
            this->wave_format_    = std::move(other.wave_format_);
            this->device_periods_ = std::move(other.device_periods_);
            this->buffer_size_    = std::move(other.buffer_size_);
            this->event_buffer_   = std::move(other.event_buffer_);
            this->render_client_  = std::move(other.render_client_);
            return *this;
        }

    private:
        /*/ Construction: /*/
        interface_wrapper (IAudioClient* interface_ptr):
            interface_base {interface_ptr},
            initialized_   {false},
            wave_format_   {this->get_mix_format()},
            device_periods_{this->get_device_periods()},
            initializer_   {this}, // Workaround to execute the Initialize method in-between bember initialization.
            buffer_size_   {this->get_buffer_size()},
            event_buffer_  {this->get_event_buffer()},
            render_client_ {this->get_render_client()} {

            this->set_event_handle();

            // TODO: Get amplitude format.
            /*WAVEFORMATEXTENSIBLE* ptr_wave_format_extensible
                = (WAVEFORMATEXTENSIBLE*) this->ptr_wave_format;
            std::string sub_format;
            sub_format
                = Cynth::Tools::guidToString(ptr_wave_format_extensible->SubFormat);
            Logger::log(sub_format);
            Logger::errCynth("STOP");*/
        }

        /*/ Internal setup methods: /*/
        // Theese should be called in the correct order upon construction.
        wave_format_wrapper get_mix_format () const {
            WAVEFORMATEX* mix_format_ptr;
            (*this)->GetMixFormat(&mix_format_ptr) >> hr_handler{"IAudioClient::GetMixFormat"};

            WAVEFORMATEX* closest_format_ptr = NULL;
            (*this)->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, mix_format_ptr, &closest_format_ptr) >> hr_handler{"IAudioClient::IsFormatSupported"};

            if (closest_format_ptr)
                return {closest_format_ptr};
            return {mix_format_ptr};
        }
        std::tuple<std::size_t, std::size_t> get_device_periods () const {
            REFERENCE_TIME default_device_period, minimum_device_period;
            (*this)->GetDevicePeriod(&default_device_period, &minimum_device_period) >> hr_handler{"IAudioClient::GetDevicePeriod"};
            return {default_device_period, minimum_device_period};
        }
        interface_wrapper& initialize () {
            (*this)->Initialize(
                AUDCLNT_SHAREMODE_SHARED,
                AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                this->buffer_period(HNS),
                0,
                &*this->wave_format(),
                NULL)
                >> hr_handler{"IAudioClient::Initialize"};
            this->initialized_ = true;
            return *this;
        }
        std::size_t get_buffer_size () const {
            if (!this->initialized())
                throw exception{"IAudioClient: Trying to get buffer size of an uninitialized IAudioClient."};
            UINT32 buffer_size;
            (*this)->GetBufferSize(&buffer_size) >> hr_handler{"IAudioClient::GetBufferSize"};
            return buffer_size;
        }
        interface_wrapper<IAudioRenderClient> get_render_client() const {
            if (!this->initialized())
                throw exception{"IAudioClient: Trying to get render client for an uninitialized IAudioClient."};
            IAudioRenderClient* audio_render_client_ptr;
            (*this)->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&audio_render_client_ptr)) >> hr_handler{"IAudioClient::GetService"};
            return {audio_render_client_ptr};
        }
        void* get_event_buffer () const {
            void* event_buffer = CreateEventA(NULL, FALSE, FALSE, NULL);
            if (!event_buffer)
                throw exception{"IAudioClient: Event creation failed."};
            return event_buffer;
        }
        void set_event_handle () const {
            if (!this->initialized())
                throw exception{"IAudioClient: Trying to set event handle for an uninitialized IAudioClient."};
            if (!this->event_buffer_isset())
                throw exception{"IAudioClient: Event buffer not set. Cannot set event handle."};
            (*this)->SetEventHandle(this->event_buffer_) >> hr_handler{"IAudioClient::SetEventHandle"};
        }

        /*/ Other internal methods: /*/
        std::size_t get_padding () const {
            if (!this->initialized())
                throw exception{"IAudioClient: Trying to get buffer padding of an uninitialized IAudioClient."};
            UINT32 padding = 0;
            (*this)->GetCurrentPadding(&padding) >> hr_handler{"IAudioClient::GetCurrentPadding"};
            return padding;
        }
        std::uint8_t* get_buffer () const {
            return this->render_client().get_buffer(this->padded_buffer_size(FRAMES));
        }
        void release_buffer () const {
            this->render_client().release_buffer(this->padded_buffer_size(FRAMES));
        }

        /*/ Internal accessors and state: /*/
        bool event_buffer_isset () const { return this->event_buffer_; }
        bool initialized        () const { return this->initialized_; }
        const interface_wrapper<IAudioRenderClient>& render_client () const { return this->render_client_; }
        const wave_format_wrapper&                   wave_format   () const { return this->wave_format_; }
        std::size_t default_device_period () const { return std::get<0>(this->device_periods_); } // In HNS
        std::size_t minimum_device_period () const { return std::get<1>(this->device_periods_); } // In HNS

        // Pre-init properties:
        bool initialized_;
        wave_format_wrapper wave_format_;
        std::tuple<std::size_t, std::size_t> device_periods_;
        struct init {
            init () {}
            init (interface_wrapper* ptr) { ptr->initialize(); }
        } initializer_;
        // Post-init properties:
        std::size_t buffer_size_; // In HNS
        void* event_buffer_;      // In HNS
        interface_wrapper<IAudioRenderClient> render_client_;
    };

}