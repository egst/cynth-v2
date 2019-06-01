#pragma once

#include "config.hpp"

#include "asio.h"

#include <string>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <algorithm>
#include <vector>

namespace cynth::api::asio::tools {

    template <typename T> const unsigned_t& native_unsigned (const T& aggr) { return *reinterpret_cast<const unsigned_t*>(&aggr); }
    template <typename T> unsigned_t&       native_unsigned (T& aggr)       { return *reinterpret_cast<unsigned_t*>(&aggr); }
    template <typename T> const signed_t&   native_signed   (const T& aggr) { return *reinterpret_cast<const signed_t*>(&aggr); }
    template <typename T> signed_t&         native_signed   (T& aggr)       { return *reinterpret_cast<signed_t*>(&aggr); }

    floating_t native_floating (double from) { return static_cast<floating_t>(from); }
    template <typename T, typename... Dummy> floating_t native_floating (T aggr, Dummy...) { return aggr.lo + static_cast<floating_t>(aggr.hi) * (1ULL << 32); }

    std::size_t sample_type_size_bytes (ASIOSampleType type) {
        switch(type) {
        case ASIOSTInt16MSB:
        case ASIOSTInt16LSB:
            return 2;
        case ASIOSTInt24MSB:
        case ASIOSTInt24LSB:
            return 3;
        case ASIOSTInt32MSB:
        case ASIOSTInt32LSB:
        case ASIOSTInt32MSB16:
        case ASIOSTInt32LSB16:
        case ASIOSTInt32MSB18:
        case ASIOSTInt32LSB18:
        case ASIOSTInt32MSB20:
        case ASIOSTInt32LSB20:
        case ASIOSTInt32MSB24:
        case ASIOSTInt32LSB24:
            return 4;
        case ASIOSTFloat32MSB:
        case ASIOSTFloat32LSB:
            return 4;
        case ASIOSTFloat64MSB:
        case ASIOSTFloat64LSB:
            return 8;
        default:
            return 0;
        }
    }
    
    bool sample_type_is_floating (ASIOSampleType type) {
        switch(type) {
        case ASIOSTInt16MSB:
        case ASIOSTInt16LSB:
        case ASIOSTInt24MSB:
        case ASIOSTInt24LSB:
        case ASIOSTInt32MSB:
        case ASIOSTInt32LSB:
        case ASIOSTInt32MSB16:
        case ASIOSTInt32LSB16:
        case ASIOSTInt32MSB18:
        case ASIOSTInt32LSB18:
        case ASIOSTInt32MSB20:
        case ASIOSTInt32LSB20:
        case ASIOSTInt32MSB24:
        case ASIOSTInt32LSB24:
            return false;
        case ASIOSTFloat32MSB:
        case ASIOSTFloat32LSB:
        case ASIOSTFloat64MSB:
        case ASIOSTFloat64LSB:
            return true;
        default:
            return false;
        }
    }

    bool sample_type_is_integral (ASIOSampleType type) { return !sample_type_is_floating(type); }
    
    bool sample_type_is_big_endian (ASIOSampleType type) {
        switch(type) {
        case ASIOSTInt16LSB:
        case ASIOSTInt24LSB:
        case ASIOSTInt32LSB:
        case ASIOSTInt32LSB16:
        case ASIOSTInt32LSB18:
        case ASIOSTInt32LSB20:
        case ASIOSTInt32LSB24:
        case ASIOSTFloat32LSB:
        case ASIOSTFloat64LSB:
            return false;
        case ASIOSTInt16MSB:
        case ASIOSTInt24MSB:
        case ASIOSTInt32MSB:
        case ASIOSTInt32MSB16:
        case ASIOSTInt32MSB18:
        case ASIOSTInt32MSB20:
        case ASIOSTInt32MSB24:
        case ASIOSTFloat32MSB:
        case ASIOSTFloat64MSB:
            return true;
        default:
            return false;
        }
    }

    bool sample_type_is_little_endian (ASIOSampleType type) { return !sample_type_is_big_endian(type); }

    std::string sample_type_name (ASIOSampleType type) {
        // Comments from the docs.

        switch (type) {
        case ASIOSTInt16MSB:
            return "INT 16 MSB";
        case ASIOSTInt24MSB: // used for 20 bits as well
            return "INT 24 MSB";
        case ASIOSTInt32MSB:
            return "INT 32 MSB";
        case ASIOSTFloat32MSB: // IEEE 754 32 bit float
            return "FLOAT 32 MSB";
        case ASIOSTFloat64MSB: // IEEE 754 64 bit double float
            return "FLOAT 64 MSB";

        // these are used for 32 bit data buffer, with different alignment of the data inside
        // 32 bit PCI bus systems can be more easily used with these
        case ASIOSTInt32MSB16: // 32 bit data with 16 bit alignment
            return "INT 32 MSB 16";
        case ASIOSTInt32MSB18: // 32 bit data with 18 bit alignment
            return "INT 32 MSB 18";
        case ASIOSTInt32MSB20: // 32 bit data with 20 bit alignment
            return "INT 32 MSB 20";
        case ASIOSTInt32MSB24: // 32 bit data with 24 bit alignment
            return "INT 32 MSB 24";
        
        case ASIOSTInt16LSB:
            return "INT 16 LSB";
        case ASIOSTInt24LSB: // used for 20 bits as well
            return "INT 24 LSB";
        case ASIOSTInt32LSB:
            return "INT 32 LSB";
        case ASIOSTFloat32LSB: // IEEE 754 32 bit float, as found on Intel x86 architecture
            return "FLOAT 32 LSB";
        case ASIOSTFloat64LSB: // IEEE 754 64 bit double float, as found on Intel x86 architecture
            return "FLOAT 64 LSB";

        // these are used for 32 bit data buffer, with different alignment of the data inside
        // 32 bit PCI bus systems can more easily used with these
        case ASIOSTInt32LSB16: // 32 bit data with 18 bit alignment
            return "INT 32 LSB 16";
        case ASIOSTInt32LSB18: // 32 bit data with 18 bit alignment
            return "INT 32 LSB 18";
        case ASIOSTInt32LSB20: // 32 bit data with 20 bit alignment
            return "INT 32 LSB 20";
        case ASIOSTInt32LSB24: // 32 bit data with 24 bit alignment
            return "INT 32 LSB 24";

        //	ASIO DSD format.
        case ASIOSTDSDInt8LSB1: // DSD 1 bit data, 8 samples per byte. First sample in Least significant bit.
            return "DSD INT 8 LSB 1";
        case ASIOSTDSDInt8MSB1: // DSD 1 bit data, 8 samples per byte. First sample in Most significant bit.
            return "DSD INT 8 MSB 1";
        case ASIOSTDSDInt8NER8: // DSD 8 bit data, 1 sample per byte. No Endianness required.
            return "DSD INT 8 NER 8";
        case ASIOSTLastEntry:
            return "Last Entry"; // ?
        default:
            return "Unknown";
        }
    }
}