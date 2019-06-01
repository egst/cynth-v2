#pragma once

#include "functional.hpp"

#include "api/asio/wrapper.hpp"
//#include "api/wasapi/wrapper.hpp"

namespace cynth::api {

    class init: public asio::wrapper {};
    //class api_init: public wasapi::wrapper {};

    struct driver {
        inline static wave_function& input = asio::driver::sample;
        //inline static wave_function& input = wasapi::driver::sample;
    };

}