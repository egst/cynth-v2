#pragma once

#include "config.hpp"
#include "functional.hpp"

namespace cynth {

    class oscillator: public function_holder<4> {
    public:
        oscillator ():
            amp{0.5},
            freq{220},
            shift{0},
            wave{wave_fs::sin},
            out_{f(this->amp * f(this->wave(f(t{} * f(this->freq * (2*constants::pi)))))) + this->shift} {}
            // Equivalent to:
            // this->amp_ * this->wave_(t{} * this->freq_ * 2 * constants::pi)
            // But that would use pointers to temporary values. Method f() stores the values and returns a reference.
            // Up to 4 such intermediate functions may be stored. It's configurable with the template parameter of function_holder<N>.

        floating_t operator() (floating_t t) { return this->out(t); }

        operator wave_function () const { return this->out; }

        void set_cache (floating_t period, wave_function::cache_t& cache) {
            this->out_.set_cache(period, cache);
        }

        // Access to the wave functions is done without any accessor functions.
        // This is meant to resemble the "functional nature" of these members.
        // osc.out    refers to its output as a function.
        // osc.out(t) refers to it as a function applied to a parameter.

        // Input modulation:
        wave_function amp;
        wave_function freq;
        wave_function shift;

        // Wave function:
        wave_function wave;

    private:
        // Modulated output:
        wave_function out_;
    
    public:
        const wave_function& out = out_;
    };

}