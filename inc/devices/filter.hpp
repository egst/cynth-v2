#pragma once

#include "functional.hpp"

namespace cynth {

    class filter: public function_holder<5> {
    public:
        wave_function        cutoff           = 5000; // Hz
        const wave_function& impulse_response = this->windowed_;

        void set_cache (wave_function::cache_t& cache) {
            this->windowed_.set_cache(wave_function::floating_time(wave_function::filter_order), cache);
        }
    
    private:
        // sinc filter impulse response:
        wave_function sinc_     = f(wave_fs::sinc(f(f(2 * this->cutoff) * f(t{} - (wave_function::floating_time(wave_function::filter_order - 1) / 2)))));
        // Applying blackman window:
        //wave_function windowed_ = wave_fs::blackman * this->sinc_;
        //wave_function windowed_ = wave_fs::blackman;
        wave_function windowed_ = this->sinc_;
        // TODO: Normalize
        // TODO: Cutoff input
    };
}