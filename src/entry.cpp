#include "cynth.hpp"

#include <exception>
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace cynth;

int main () {
    try {
        api::init i;

        oscillator o;
        filter f;
        wave_function::cache_t cache1, cache2;

        o.freq = 500;
        o.amp  = 0.05;
        o.wave = wave_fs::saw;

        o.set_cache(1./500, cache1);

        f.cutoff = 2000;
        f.set_cache(cache2);

        auto out = f.impulse_response | o.out;
        //auto out = f.impulse_response;
        //out.set_cache(1./500, cache2);

        api::driver::input = out;

        std::this_thread::sleep_for(std::chrono::seconds{30});

    } catch (std::exception& e) {
        std::cout << e.what() << '\n';
    }
    std::cout << "DONE\n";
}