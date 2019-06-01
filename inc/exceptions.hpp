#pragma once

#include "platform.hpp"

#include "string_tools.hpp"

#ifdef CYNTH_API_ASIO
#include "asio.h"
#endif

#ifdef CYNTH_OS_WINDOWS
#include <objbase.h>
#endif

#include <exception>
#include <string>

namespace cynth {

    class exception: public std::exception {
    public:
        template <typename Message>
        exception (const Message& message, std::string source = "Unknown source") noexcept:
            std::exception{},
            message_{string_tools::to_string(message)},
            source_{source},
            what_{this->what_format()} {}

        std::string message () const noexcept {
            return this->message_;
        }
        std::string source () const noexcept {
            return this->source_;
        }
        std::string what_format () const {
            return "[" + this->source() + "] " + this->message();
        }
        const char* what () const noexcept override {
            return this->what_.data();
        }

    protected:
        std::string message_;
        std::string source_;
        std::string what_;
    };

    /*/ Platform exception: /*/
    class platform_exception: public exception {
    public:
        platform_exception (const std::string& message = "Platform not suppored.") noexcept:
            exception{message, "Platform"} {}
    };

    #ifdef CYNTH_OS_WINDOWS
    /*/ COM exception: /*/
    class com_exception: public exception {
    public:
        com_exception (HRESULT message) noexcept:
            exception{message, "COM"} {} // TODO: Get the message from HRESULT value.
        com_exception (HRESULT message, std::string source) noexcept:
            exception{source + ": " + string_tools::to_string(message), "COM"} {}
    };
    class hr_handler {
    public:
        hr_handler (std::string source = ""):
            source_{source} {}

        friend HRESULT operator>> (HRESULT hr, const hr_handler& self) {
            if (FAILED(hr)) {
                if (self.source_ != "")
                    throw com_exception{hr, self.source_};
                else
                    throw com_exception{hr};
            }
            return hr;
        }
    
    private:
        std::string source_;
    };
    #endif

    /*/ WASAPI exception: /*/
    class wasapi_exception: public exception {
    public:
        wasapi_exception (const std::string& message) noexcept:
            exception{message, "WASAPI"} {}
    };

    /*/ ASIO exception: /*/
    #ifdef CYNTH_API_ASIO
    class asio_exception: public exception {
    public:
        asio_exception (const std::string& message) noexcept: exception{message, "ASIO"} {}
    };

    class ase_handler {
    public:
        ase_handler (const std::string& source = ""): source_{source} {}

        friend ASIOError operator>> (ASIOError er, const ase_handler& self) {
            if (er != ASE_OK)
                throw asio_exception{self.message(ase_handler::error_status(er))};
            return er;
        }

        std::string message (const std::string& msg) const {
            if (this->source_ != "")
                return this->source_ + ": " + msg;
            else
                return msg;
        }

        static std::string error_status (ASIOError er) {
        switch (er) {
        case ASE_OK:
            return "OK.";
        case ASE_SUCCESS:
            return "ASIOFurute success.";
        case ASE_NotPresent:
            return "Hardware input or output is not present or available.";
        case ASE_HWMalfunction:
            return "Hardware is malfunctioning.";
        case ASE_InvalidParameter:
            return "Invalid parameter.";
        case ASE_InvalidMode:
            return "Invalid mode.";
        case ASE_SPNotAdvancing:
            return "Hardware is not running when sample position is inquired.";
        case ASE_NoClock:
            return "No clock.";
        case ASE_NoMemory:
            return "No memory.";
        default:
            return std::to_string(er);
        }
    }

    private:
        std::string source_;
    };
    #endif

    /*/ Cynth exception: /*/
    class cynth_exception: public exception {
    public:
        cynth_exception (const std::string& message) noexcept:
            exception{message, "Cynth"} {}
    };

}