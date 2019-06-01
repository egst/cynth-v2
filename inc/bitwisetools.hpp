#pragma once

#include "config.hpp"

#include <cstdint>

namespace cynth::bitwise_tools {
    constexpr bool big_endian () {
        union { std::uint32_t i; char c[4]; } u = {0x01020304};
        return u.c[0] == 1; 
    }

    constexpr bool little_endian () { return !big_endian(); }

    constexpr bool shift_is_arithmetic () { return (-1 >> 1) < 0; }

    static_assert(shift_is_arithmetic()); // TODO: Implement arithmetic shift, when the default one isn't arithmetic.

    //uint8_t reverse_byte (uint8_t byte) { return ((byte * 0x0802LU & 0x22110LU) | (byte * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16; }

    /*template <typename T>
    T reverse (T n) {
        auto b = reinterpret_cast<T*>(&n);
        for (std::size_t i = 0; i < sizeof(T); ++i)
            b[i] = reverse_byte(b[i]);
        for (std::size_t i = 0; i < sizeof(T) / 2; ++i)
            std::swap(b[i], b[sizeof(T) - 1 - i]);
    }*/

    template <typename T>
    T switch_endianness (T n) {
        auto b = reinterpret_cast<T*>(&n);
        for (std::size_t i = 0; i < sizeof(T) / 2; ++i)
            std::swap(b[i], b[sizeof(T) - 1 - i]);
        return n;
    }

    template <typename T>
    byte_t at (T val, std::size_t n) {
        auto a = val >> ((sizeof(T) - n - 1) * 8);
        return static_cast<byte_t>(a);
    }

    std::size_t max_for_bits  (std::size_t bits)  { return (1ULL << (bits - 1)) - 1; }
    std::size_t max_for_bytes (std::size_t bytes) { return max_for_bits(bytes * 8); }

    /*template <typename T>
    bool is_power_of_two (T n) { return n > 0 && (n & (n - 1)) == 0; }*/
}