#pragma once

#include "bitwisetools.hpp"
#include "exceptions.hpp"
#include "api/asio/tools.hpp"

#include "asio.h"

#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <memory>
#include <iterator>

/*

assuming little endian and arithmetic right shift:

ASIOSTInt16LSB:        2B int     min..max
case ASIOSTInt24LSB:   3B int     min..max
case ASIOSTInt32LSB:   4B int     min..max
case ASIOSTFloat32LSB: 4B float   -1..1  -- TODO: Handle cases, when sizeof(float)  != 4B (or even isn't IEEE 754?)
case ASIOSTFloat64LSB: 8B double  -1..1  -- TODO: Handle cases, when sizeof(double) != 8B (or even isn't IEEE 754?)

ASIOSTInt32LSB16:      4B int     min<2B>..max<2B> >> 16
ASIOSTInt32LSB18:      4B int     min<3B>..max<3B> >> 18
ASIOSTInt32LSB20:      4B int     min<3B>..max<3B> >> 18
ASIOSTInt32LSB24:      4B int     min<3B>..max<3B> >> 18

assuming big endian and arithmetic right shift:

ASIOSTInt16MSB:        2B int     min..max
case ASIOSTInt24MSB:   3B int     min..max
case ASIOSTInt32MSB:   4B int     min..max
case ASIOSTFloat32MSB: 4B float   -1..1  -- TODO: Handle cases, when sizeof(float)  != 4B (or even isn't IEEE 754?)
case ASIOSTFloat64MSB: 8B double  -1..1  -- TODO: Handle cases, when sizeof(double) != 8B (or even isn't IEEE 754?)

ASIOSTInt32MSB16:      4B int     min<2B>..max<2B> >> 16
ASIOSTInt32MSB18:      4B int     min<3B>..max<3B> >> 18
ASIOSTInt32MSB20:      4B int     min<3B>..max<3B> >> 18
ASIOSTInt32MSB24:      4B int     min<3B>..max<3B> >> 18

TODO: Handle cases, when right shift is not arithmetic.

When samples are "LSB" on big endian platforms or vice-versa, the value is simply reversed.

*/

namespace cynth::api::asio {

    template <std::size_t SIZE>
    struct sample_spacer {
        std::array<std::uint8_t, SIZE> _;
    };

    // TODO: DSD format
    // TODO: nullptr checks, out of range checks
    template <typename Derived, typename Ptr>
    class sample_base {
        using derived_t = Derived;
        using ptr_t     = Ptr;

    public:
        sample_base (ptr_t ptr, ASIOSampleType type):
            ptr_{ptr},
            type_{type},
            size_{tools::sample_type_size_bytes(type)}, // TODO: Check the size.
            floating_{tools::sample_type_is_floating(type)} {}

        std::size_t size_bytes () const { return this->size_; }
        std::size_t size_bits  () const { return this->size_bytes() * 8; }
        bool  floating         () const { return this->floating_; }
        bool  double_precision () const { return this->floating() && this->size_bytes() == 8; }
        bool  single_precision () const { return this->floating() && this->size_bytes() == 4; }

        void set (signed_t val) {
            this->set(static_cast<floating_t>(val) / bitwise_tools::max_for_bytes(8));
        }
        void set (floating_t val) {
            if (this->double_precision())
                this->get<double>() = this->correct_endiannes(val);
            if (this->single_precision())
                this->get<float>() = this->correct_endiannes(static_cast<float>(val));
            else
                this->fill(static_cast<signed_t>(std::floor(val * bitwise_tools::max_for_bytes(this->size_bytes()))));
        }
        void fill (signed_t val) {
            val = this->correct_endiannes(val);
            for (std::size_t i = 0; i < this->size_bytes(); ++i) {
                auto j = this->correct_endiannes() ? i : this->size_bytes() - i - 1;
                auto k = 8 - i - 1;
                this->at(j) = bitwise_tools::at(val, k);
            }
        }

        byte_t&       at (std::size_t pos)       { return reinterpret_cast<byte_t*>(this->ptr_)[pos]; }
        const byte_t& at (std::size_t pos) const { return reinterpret_cast<byte_t*>(this->ptr_)[pos]; }
        
        template <typename T> T&       get ()       { return *reinterpret_cast<T*>(this->ptr_); }
        template <typename T> const T& get () const { return *reinterpret_cast<const T*>(this->ptr_); }

        template <typename T>
        T correct_endiannes (T n) const {
            return this->correct_endiannes() ? n : bitwise_tools::switch_endianness(n);
        }

        bool correct_endiannes () const {
            return
                (tools::sample_type_is_big_endian(this->type_)    && bitwise_tools::big_endian()) ||
                (tools::sample_type_is_little_endian(this->type_) && bitwise_tools::little_endian());
        }

        derived_t*       operator-> ()       { return this; }
        const derived_t* operator-> () const { return this; }

    private:
        ptr_t ptr_;
        ASIOSampleType type_;
        std::size_t size_;
        bool floating_;
    };

    class sample_wrapper: public sample_base<sample_wrapper, void*> {
        using base_t = sample_base<sample_wrapper, void*>;

    public:
        using base_t::sample_base;
        template <typename T> sample_wrapper& operator= (T val) { this->set(val); return *this; }
    };
    
    class const_sample_wrapper: public sample_base<const_sample_wrapper, const void*> {
        using base_t = sample_base<const_sample_wrapper, const void*>;

    public:
        using base_t::sample_base;
        template <typename T> const_sample_wrapper& operator= (T val) { return this->set(val); return *this; }
    };

    class buffer {
    public:
        buffer (void* ptr, ASIOSampleType type, std::size_t size):
            ptr_{ptr},
            type_{type},
            size_{size},
            spacing_{tools::sample_type_size_bytes(type)} {}

        ASIOSampleType type () const { return this->type_; }

        sample_wrapper operator[] (std::size_t pos) {
            // TODO: Is there any other way?
            switch (this->spacing_) {
            case 2:
                return {this->at<2>(pos), this->type_};
            case 3:
                return {this->at<3>(pos), this->type_};
            case 4:
                return {this->at<4>(pos), this->type_};
            case 8:
                return {this->at<8>(pos), this->type_};
            default:
                throw asio_exception{"Sample type not implemented."};
            }
        }
        const_sample_wrapper operator[] (std::size_t pos) const {
            switch (this->spacing_) {
            case 2:
                return {this->at<2>(pos), this->type_};
            case 3:
                return {this->at<3>(pos), this->type_};
            case 4:
                return {this->at<4>(pos), this->type_};
            case 8:
                return {this->at<8>(pos), this->type_};
            default:
                throw asio_exception{"Sample type not implemented."};
            }
        }

        template <std::size_t SIZE> void*       at (std::size_t pos)       { return reinterpret_cast<sample_spacer<SIZE>*>(this->ptr_) + pos; }
        template <std::size_t SIZE> const void* at (std::size_t pos) const { return reinterpret_cast<const sample_spacer<SIZE>*>(this->ptr_) + pos; }

        using value_type      = sample_wrapper;
        using reference       = sample_wrapper&;
        using const_reference = const sample_wrapper&;
        
        template <typename Derived, typename Buffer, typename Wrapper>
        class iterator_base {
            using derived_t      = Derived;
            using cvref_buffer_t = Buffer;  // cv-qualified reference
            using wrapper_t      = Wrapper;

        public:
            using value_type        = wrapper_t;
            using reference         = wrapper_t;
            using pointer           = wrapper_t;
            using difference_type   = std::ptrdiff_t;
            using iterator_category = std::random_access_iterator_tag;

            iterator_base (cvref_buffer_t&& buff, std::size_t pos):
                buff_{buff},
                pos_{pos} {}
            
            derived_t  operator+  (std::size_t i) const { return {this->buff_, this->pos_ + i}; }
            derived_t  operator-  (std::size_t i) const { return {this->buff_, this->pos_ - i}; }
            derived_t& operator+= (std::size_t i)       { this->pos_ += i; return this->derived(); }
            derived_t& operator-= (std::size_t i)       { this->pos_ -= i; return this->derived(); }
            derived_t& operator++ ()                    { ++this->pos_;    return this->derived(); }
            derived_t  operator++ (int)                 { auto copy = this->derived(); ++(*this); return copy; }

            friend derived_t operator- (std::size_t i, const iterator_base& self) { return self - i; }

            difference_type operator-  (const iterator_base& other) const { return this->pos_ -  other.pos_; }
            difference_type operator<  (const iterator_base& other) const { return this->pos_ <  other.pos_; }
            difference_type operator>  (const iterator_base& other) const { return this->pos_ >  other.pos_; }
            difference_type operator<= (const iterator_base& other) const { return this->pos_ <= other.pos_; }
            difference_type operator>= (const iterator_base& other) const { return this->pos_ >= other.pos_; }
            difference_type operator== (const iterator_base& other) const { return this->pos_ == other.pos_; }
            difference_type operator!= (const iterator_base& other) const { return this->pos_ != other.pos_; }

            reference operator* ()       { return this->buff_[this->pos_]; }
            reference operator* () const { return this->buff_[this->pos_]; }
            
            pointer operator-> ()       { return this->buff_[this->pos_]; }
            pointer operator-> () const { return this->buff_[this->pos_]; }

            reference operator[] (std::size_t i)       { return *(*this + i); }
            reference operator[] (std::size_t i) const { return *(*this + i); }
        
        protected:
            cvref_buffer_t buff_;
            std::size_t    pos_;

        private:
            derived_t&       derived ()       { return *static_cast<derived_t*>(this); }
            const derived_t& derived () const { return *static_cast<const derived_t*>(this); }
        };

        class iterator: public iterator_base<iterator, buffer&, sample_wrapper> {
            using base_t = iterator_base<iterator, buffer&, sample_wrapper>;
            using base_t::iterator_base;
            using difference_type   = typename base_t::difference_type;
            using iterator_category = typename base_t::iterator_category;
            using value_type        = typename base_t::value_type;
            using reference         = typename base_t::reference;
            using pointer           = typename base_t::pointer;
        };
        class const_iterator: public iterator_base<const_iterator, const buffer&, const_sample_wrapper> {
        public:
            using base_t = iterator_base<const_iterator, const buffer&, const_sample_wrapper>;
            using base_t::iterator_base;
            using difference_type   = typename base_t::difference_type;
            using iterator_category = typename base_t::iterator_category;
            using value_type        = typename base_t::value_type;
            using reference         = typename base_t::reference;
            using pointer           = typename base_t::pointer;
            reference& operator*  () = delete;
            reference  operator-> () = delete;
        };

        using difference_type = std::ptrdiff_t;
        using size_type       = std::size_t;

        std::size_t size () const { return this->size_; }

        iterator       begin  ()       { return {*this, 0}; }
        iterator       end    ()       { return {*this, this->size()}; }
        const_iterator cbegin () const { return const_iterator{*this, 0}; }
        const_iterator cend   () const { return {*this, this->size()}; }
        const_iterator begin  () const { return this->cbegin(); }
        const_iterator end    () const { return this->cend(); }

    private:
        void* ptr_;
        ASIOSampleType type_;
        std::size_t size_;
        std::size_t spacing_;
    };

}