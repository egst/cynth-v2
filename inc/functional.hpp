#pragma once

#include "config.hpp"
#include "exceptions.hpp"
#include "wavetables.hpp"

#include <tuple>
#include <complex>
#include <array>

namespace cynth {
    
    // Like std::function, but with no capturing and no dynamic memory allocation.
    template <typename Out, typename... Ins>
    class function_wrapper {
    public:
        using out_t = Out;
        using in_ts = std::tuple<Ins...>;
        using func_ptr_t = Out (*) (Ins...);

        constexpr function_wrapper (func_ptr_t ptr = nullptr): ptr_{ptr} {}
        function_wrapper& operator = (func_ptr_t ptr) { this->ptr_ = ptr; return *this; }

        constexpr operator bool () const { return this->ptr_; }

        constexpr operator func_ptr_t () const { return this->ptr_; }

        constexpr Out operator() (Ins... ins) const {
            if (!*this)
                throw cynth_exception{"Uninitialized function."};
            return (*this->ptr_)(ins...);
        }

    private:
        func_ptr_t ptr_;
    };

    enum operation_enum { CONSTANT, ADD, SUB, MULT, DIV, COMP, CONV };

    template <typename T>
    class composite_function {
    public:
        using func_t     = function_wrapper<T, T>;
        using func_ptr_t = typename func_t::func_ptr_t;

        inline constexpr static std::size_t cache_size    = 1024;

        using cache_t    = std::array<T, cache_size>;

        //inline constexpr static std::size_t filter_order  = 512;
        inline constexpr static std::size_t filter_order  = 32;

        inline static floating_t sample_rate   = 44100; // Default value. This should be set during API initialization.
        inline static floating_t sample_length = 1. / sample_rate;

        static floating_t floating_time (unsigned_t i) { return i * sample_length; }
        static unsigned_t integral_time (T t)          { return static_cast<unsigned_t>(t / sample_length); } // Maybe signed?

        constexpr composite_function (const composite_function&) = default;
        composite_function& operator = (const composite_function&) = default;

        constexpr composite_function (const func_t& func): func_ptr_{func} {}

        // Two composite functions:
        constexpr composite_function (operation_enum operation, const composite_function& first, const composite_function& second):
            operation_       {operation},
            first_ptr_       {&first},
            second_ptr_      {&second},
            func_ptr_        {nullptr} {}
        // Two constants:
        constexpr composite_function (operation_enum operation, const T& first, const T& second):
            operation_       {operation},
            func_ptr_        {nullptr},
            first_constant_  {first},
            second_constant_ {second} {}
        // Two identities:
        constexpr composite_function (operation_enum operation, std::nullptr_t, std::nullptr_t):
            operation_       {operation},
            func_ptr_        {nullptr},
            first_identity_  {true},
            second_identity_ {true} {}
        // One constant:
        constexpr composite_function (const T& constant):
            operation_       {CONSTANT},
            func_ptr_        {nullptr},
            first_constant_  {constant} {}
        // Identity:
        constexpr composite_function ():
            operation_       {CONSTANT},
            func_ptr_        {nullptr},
            first_identity_  {true} {}

        // Composite function and identity:
        constexpr composite_function (operation_enum operation, std::nullptr_t, const composite_function& second):
            operation_       {operation},
            second_ptr_      {&second},
            func_ptr_        {nullptr},
            first_identity_  {true} {}
        constexpr composite_function (operation_enum operation, const composite_function& first, std::nullptr_t):
            operation_       {operation},
            first_ptr_       {&first},
            func_ptr_        {nullptr},
            second_identity_ {true} {}
        // Composite function and constant:
        constexpr composite_function (operation_enum operation, const composite_function& first, const T& second):
            operation_       {operation},
            first_ptr_       {&first},
            func_ptr_        {nullptr},
            second_constant_ {second} {}
        constexpr composite_function (operation_enum operation, const T& first, const composite_function& second):
            operation_       {operation},
            second_ptr_      {&second},
            func_ptr_        {nullptr},
            first_constant_  {first} {}
        // Constant and identity:
        constexpr composite_function (operation_enum operation, std::nullptr_t, const T& second):
            operation_       {operation},
            func_ptr_        {nullptr},
            first_identity_  {true},
            second_constant_ {second} {}
        constexpr composite_function (operation_enum operation, const T& first, std::nullptr_t):
            operation_       {operation},
            func_ptr_        {nullptr},
            second_identity_ {true},
            first_constant_  {first} {}

        // TODO: Prohibit rvalues.
        constexpr composite_function operator +  (const composite_function& other) const {
            if (this->identity() && other.identity())
                return {ADD, nullptr, nullptr};
            if (this->identity())
                return {ADD, nullptr, other};
            if (other.identity())
                return {ADD, *this, nullptr};
            return {ADD, *this, other};
        }
        constexpr composite_function operator -  (const composite_function& other) const {
            if (this->identity() && other.identity())
                return {SUB, nullptr, nullptr};
            if (this->identity())
                return {SUB, nullptr, other};
            if (other.identity())
                return {SUB, *this, nullptr};
            return {SUB, *this, other};
        }
        constexpr composite_function operator *  (const composite_function& other) const {
            if (this->identity() && other.identity())
                return {MULT, nullptr, nullptr};
            if (this->identity())
                return {MULT, nullptr, other};
            if (other.identity())
                return {MULT, *this, nullptr};
            return {MULT, *this, other};
        }
        constexpr composite_function operator /  (const composite_function& other) const {
            if (this->identity() && other.identity())
                return {DIV, nullptr, nullptr};
            if (this->identity())
                return {DIV, nullptr, other};
            if (other.identity())
                return {DIV, *this, nullptr};
            return {DIV, *this, other};
        }
        constexpr composite_function operator |  (const composite_function& other) const {
            if (this->identity() && other.identity())
                return {CONV, nullptr, nullptr};
            if (this->identity())
                return {CONV, nullptr, other};
            if (other.identity())
                return {CONV, *this, nullptr};
            return {CONV, *this, other};
        }
        constexpr composite_function operator () (const composite_function& other) const {
            if (this->identity() && other.identity())
                return {};
            if (this->identity())
                return other;
            if (other.identity())
                return *this;
            return {COMP, *this, other};
        }
        friend constexpr composite_function operator + (const T& constant, const composite_function& func) {
            if (func.identity())
                return {ADD, constant, nullptr};
            return {ADD, constant, func};
        }
        friend constexpr composite_function operator + (const composite_function& func, const T& constant) {
            if (func.identity())
                return {ADD, nullptr, constant};
            return {ADD, func, constant};
        }
        friend constexpr composite_function operator - (const T& constant, const composite_function& func) {
            if (func.identity())
                return {ADD, constant, nullptr};
            return {ADD, constant, func};
        }
        friend constexpr composite_function operator - (const composite_function& func, const T& constant) {
            if (func.identity())
                return {SUB, nullptr, constant};
            return {SUB, func, constant};
        }
        friend constexpr composite_function operator * (const T& constant, const composite_function& func) {
            if (func.identity())
                return {MULT, constant, nullptr};
            return {MULT, constant, func};
        }
        friend constexpr composite_function operator * (const composite_function& func, const T& constant) {
            if (func.identity())
                return {MULT, nullptr, constant};
            return {MULT, func, constant};
        }
        friend constexpr composite_function operator / (const T& constant, const composite_function& func) {
            if (func.identity())
                return {DIV, constant, nullptr};
            return {DIV, constant, func};
        }
        friend constexpr composite_function operator / (const composite_function& func, const T& constant) {
            if (func.identity())
                return {DIV, nullptr, constant};
            return {DIV, func, constant};
        }
        friend constexpr composite_function operator | (const T& constant, const composite_function& func) {
            if (func.identity())
                return {CONV, constant, nullptr};
            return {CONV, constant, func};
        }
        friend constexpr composite_function operator | (const composite_function& func, const T& constant) {
            if (func.identity())
                return {CONV, nullptr, constant};
            return {CONV, func, constant};
        }

        bool identity () const { return this->operation_ == CONSTANT && this->first_identity_ == true; }

        T operator () (T in) const {
            if (this->cache_ptr_) {
                return this->cache(in);
            }
            if (this->func_ptr_)
                return (*this->func_ptr_)(in/*, func_ptr*/);
            switch (this->operation_) {
            case CONSTANT: default:
                return this->first(in);
            case ADD:
                return this->first(in) + this->second(in);
            case SUB:
                return this->first(in) - this->second(in);
            case MULT:
                return this->first(in) * this->second(in);
            case DIV:
                return this->first(in) / this->second(in);
            case COMP:
                return this->first(this->second(in));
            case CONV:
                return this->conv(in);
            }
        }

        T conv (T in) const {
            T result = 0;
            for (std::size_t i = 0; i < filter_order; ++i) {
                result += this->first(floating_time(i)) * this->second(in - floating_time(i));
            }

            return result;
        }

        T cache (T in) const {
            if (!this->cache_ptr_)
                throw cynth_exception{"Uninitialized function cache."};
            std::size_t i = integral_time(fmod(in, this->cache_period_));
            return (*this->cache_ptr_)[i];
        }

        T first  (T in) const {
            if (this->first_ptr_)
                return (*this->first_ptr_)(in/*, func_ptr*/);
            if (this->first_identity_)
                return in;
            return this->first_constant_;
        }
        T second (T in) const {
            if (this->second_ptr_)
                return (*this->second_ptr_)(in/*, func_ptr*/);
            if (this->second_identity_)
                return in;
            return this->second_constant_;
        }

        void set_cache (T period, cache_t& cache) {
            for (auto [i, t] = std::tuple<std::size_t, T>{0, 0}; i < cache_size && t < period; ++i, t += sample_length)
                cache[i] = (*this)(t);
            this->cache_period_ = period;
            this->cache_ptr_    = &cache;
        }

    private:
        operation_enum            operation_       = CONSTANT;
        const composite_function* first_ptr_       = nullptr;
        const composite_function* second_ptr_      = nullptr;
        func_ptr_t                func_ptr_        = nullptr;
        bool                      first_identity_  = false;
        bool                      second_identity_ = false;
        T                         first_constant_  = 0;
        T                         second_constant_ = 0;
        cache_t*                  cache_ptr_       = nullptr;
        T                         cache_period_    = 0;
    };

    // Identity function can be used to declare the input variable:
    template <typename T>
    class var: public composite_function<T> {
    public:
        constexpr var (): composite_function<T>::composite_function{} {}
    };

    /*template <> class composite_function<double>;
    template <> class var<double>;*/

    using wave_function         = composite_function<floating_t>;
    using wave_function_wrapper = function_wrapper<floating_t, floating_t>;
    // Shorthands:
    using f = wave_function;
    using t = var<floating_t>;

    struct wave_fs {
        inline constexpr static wave_function sin  = {wave_function_wrapper{ [] (floating_t t) -> floating_t { return math::sin(t);} }};
        inline constexpr static wave_function cos  = {wave_function_wrapper{ [] (floating_t t) -> floating_t { return math::cos(t);} }};
        
        inline constexpr static wave_function sinc = {wave_function_wrapper{ [] (floating_t t) -> floating_t { return math::sinc(t);} }};

        inline constexpr static wave_function blackman = {wave_function_wrapper{ [] (floating_t t) -> floating_t {
            return
                + 0.42
                - 0.5  * math::cos((2 * constants::pi * t) / (wave_function::floating_time(wave_function::filter_order - 1)))
                + 0.08 * math::cos((4 * constants::pi * t) / (wave_function::floating_time(wave_function::filter_order - 1)));
        } }};

        inline constexpr static wave_function saw  = {wave_function_wrapper{ [] (floating_t t) -> floating_t {
            t = t < 0
                ? 2*constants::pi - std::fmod(std::abs(t), 2*constants::pi)
                : std::fmod(t, 2*constants::pi);
            return (1/constants::pi) * t - 1;
        } }};
    };

    template <std::size_t SIZE>
    class function_holder {
    protected:
        wave_function& f (wave_function&& temp) {
            if (holder_last_ >= SIZE)
                throw cynth_exception{"No more space for intermediate functions."};
            return this->holder_[holder_last_++] = std::move(temp);
        }

        std::size_t holder_last_ = 0;
        std::array<wave_function, SIZE> holder_;
    };
}