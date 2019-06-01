#pragma once

#include "config.hpp"
#include "constexprtools.hpp"

#include "gcem.hpp" // Compile-time math library.

#include <cstddef>
#include <array>
#include <cmath>

namespace cynth {
    struct constants {
        constexpr static floating_t pi = gcem::acos(-1);
    };

    class wavetable {
    public:
        constexpr static std::size_t size = 4086;
        constexpr wavetable (): content_{} {}

        //constexpr operator std::array<floating_t, wavetable::size> () const { return this->content_; }
        constexpr operator const floating_t* () const { return this->content_; }
        constexpr floating_t operator[] (std::size_t i) const { return this->content_[i]; }

    protected:
        //std::array<floating_t, wavetable::size> content_;
        floating_t content_[wavetable::size];
    };

    class sin_table: public wavetable {
    public:
        constexpr sin_table (): wavetable{} {
            constexpr_tools::for_constexpr<const std::size_t, wavetable::size>([this] (auto i) {
                constexpr auto t = (static_cast<floating_t>(i.value) / wavetable::size) * 2 * constants::pi;
                this->content_[i.value] = gcem::sin(t);
            });
        }
    };

    struct wavetables {
        constexpr static sin_table sin = {};
    };

    namespace math {
        floating_t sin (floating_t x) {
            auto t = (std::fmod(x, 2 * constants::pi) / (2 * constants::pi)) * wavetable::size;
            // Truncation:
            t = static_cast<std::size_t>(std::floor(t));
            // TODO: Interpolation (at least linear)
            return wavetables::sin[t];
        }
        floating_t cos (floating_t x) {
            return sin(x - (constants::pi / 4));
        }
        floating_t sinc (floating_t x) {
            return x == 0
                ? 1
                : sin(constants::pi * x) / (constants::pi * x);
        }
    }
}