#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace cynth::container_tools {

    class dynamic_string_array {
    public:
        dynamic_string_array (std::size_t size, std::size_t string_size):
            size_{size},
            string_size_{string_size},
            data_{new char*[this->size()]} {
            
            for (std::size_t i = 0; i < this->size(); ++i)
                this->data()[i] = new char[this->string_size()];
        }

        ~dynamic_string_array () {
            for (std::size_t i = 0; i < this->size(); ++i)
                delete[] this->data()[i];
            delete[] this->data();
        }

        std::size_t size () const { return this->size_; }
        std::size_t string_size () const { return this->string_size_; }

        char** data () const { return this->data_; }

        operator char** () const { return this->data(); }
        char*& operator[] (std::size_t i) const { return this->data()[i]; }

        std::vector<std::string> in_vector () const { return this->in_vector(this->size() - 1); }
        std::vector<std::string> in_vector (std::size_t count) const {
            std::vector<std::string> result;
            for (std::size_t i = 0; i < count; ++i)
                result.push_back(this->data()[i]);
            return result;
        }

    private:
        std::size_t size_;
        std::size_t string_size_;
        char**      data_;
    };

    /*template <typename T>
    class dynamic_array {
    public:
        dynamic_array (std::size_t size):
            size_{size},
            data_{new T[this->size()]} {}

        ~dynamic_array () {
            delete[] this->data();
        }

        std::size_t size () { return this->size_; }

        T* data () { return this->data_; }

        operator T* () { return this->data(); }
        T& operator[] (std::size_t i) { return this->data()[i]; }

    private:
        std::size_t size_;
        T* data_;
    };*/

}