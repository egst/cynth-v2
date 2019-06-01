#pragma once

#include "api/wasapi/interface_wrapper.hpp"
#include "exceptions.hpp"

#include <audioclient.h> // IAudioRenderClient

#include <cstdint>

namespace cynth::api::wasapi {

    template <>
    class interface_wrapper<IAudioRenderClient>: public interface_base<IAudioRenderClient> {
        friend class interface_base<IAudioRenderClient>;
        template <typename OtherInterface> friend class interface_wrapper;
    
    private:
        using interface_base<IAudioRenderClient>::interface_base;
        using interface_base<IAudioRenderClient>::operator=;

        std::uint8_t* get_buffer (std::size_t size) const {
            std::uint8_t* buffer_ptr;
            (*this)->GetBuffer(size, &buffer_ptr) >> hr_handler{"IAudioRenderClient::GetBuffer"};
            return buffer_ptr;
        }
        void release_buffer (std::size_t size) const {
            DWORD flags = 0; // TODO?
            (*this)->ReleaseBuffer(size, flags) >> hr_handler{"IAudioRenderClient::ReleaseBuffer"};
        }
    };

}