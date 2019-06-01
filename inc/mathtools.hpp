#pragma once

namespace cynth::math_tools {

    /*template <std::size_t FROM, std::size_t TO, std::size_t STEP, typename T, std::size_t SIZE>
    std::array<T, (TO - FROM + 1) / STEP> slice (const std::array<T, SIZE>& arr) {
        static_assert (TO >= FROM);
        static_assert (STEP >= 1);
        static_assert (TO - FROM <= SIZE);

        std::array<T, (TO - FROM + 1) / STEP> result;
        for (auto [i, j] = std::tuple{std::size_t{0}, FROM}; j < TO; ++i, j += STEP)
            result[i] = arr[j];
        
        return result;
    }

    template <std::size_t SIZE, typename T, typename Func>
    void apply (Func f, std::array<T, SIZE>& arr) {
        for (auto&& elem: arr)
            elem = f(elem);
    }

    template <typename T, std::size_t SIZE>
    std::array<std::complex<T>, SIZE> complex_samples (const wave_function& func, double from, double spacing) {
        double width = SIZE * spacing;
        double to    = from + width;
        std::array<std::complex<T>, SIZE> result;
        for (auto [i, t] = std::tuple{std::size_t{0}, from}; i < SIZE && t < to; ++i, t += spacing)
            result[i] = {func(t), 0};
        return result;
    }

    template <typename T, std::size_t SIZE>
    void fft (std::array<std::complex<T>, SIZE>& x) {
        // Slower recursive algorithm.
        // TODO: Cooley–Tukey algorithm
        constexpr std::size_t n = SIZE;
        if constexpr (n >= 2) {
            auto even = slice<0, n, 2>(x); // std::array<complex, n/4>
            auto odd  = slice<1, n, 2>(x); // std::array<complex, (n/2-1)/2>
        
            fft(even);
            fft(odd);

            for (std::size_t i = 0; i < n/2; ++i) {
                complex c = std::polar(1.0, -2 * constants::pi * i / n) * odd[i];
                x[i]     = even[i] + c;
                x[i+n/2] = even[i] - c;
            }
        }
    }

    template <typename T, std::size_t SIZE>
    void inverse_fft (std::array<complex, SIZE>& x) {
        for (auto&& c: x)
            c = std::conj(c);
        fft(x);
        double n = static_cast<T>(SIZE);
        for (auto&& c: x)
            c = std::conj(c)/n;
    }*/

}