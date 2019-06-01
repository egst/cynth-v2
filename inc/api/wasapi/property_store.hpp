#pragma once

#include "api/wasapi/interface_wrapper.hpp"
#include "exceptions.hpp"
#include "string_tools.hpp"

#include <propsys.h> // IPropertyStore
#include <propidl.h> // PROPVARIANT

#include <string>

namespace cynth::api::wasapi {

    template <>
    class interface_wrapper<IPropertyStore>: public interface_base<IPropertyStore> {
        friend class interface_base<IPropertyStore>;
        template <typename OtherInterface> friend class interface_wrapper;

        class propvariant_wrapper {
        public:
            propvariant_wrapper () {
                PropVariantInit(&this->propvariant_);
            }
            ~propvariant_wrapper () {
                PropVariantClear(&this->propvariant_);
            }
            propvariant_wrapper (const propvariant_wrapper&) = delete;
            propvariant_wrapper (propvariant_wrapper&&) = default;
            propvariant_wrapper& operator= (const propvariant_wrapper&) = delete;
            propvariant_wrapper& operator= (propvariant_wrapper&&) = default;
            PROPVARIANT& get () { return this->propvariant_; }
            
        private:
            PROPVARIANT propvariant_;
        };

    public:
        using interface_base<IPropertyStore>::interface_base;
        using interface_base<IPropertyStore>::operator=;

        /*/ Move: /*/
        interface_wrapper (interface_wrapper&& other):
            interface_base      {std::move(other.base())},
            propvariant_wrapper_{std::move(other.propvariant_wrapper_)} {}
        interface_wrapper& operator= (interface_wrapper&& other) {
            this->base()               = std::move(other.base());
            this->propvariant_wrapper_ = std::move(other.propvariant_wrapper_);
            return *this;
        }

        /*/ Retrieving the value: /*/
        std::string value (PROPERTYKEY property_key) {
            (*this)->GetValue(property_key, &this->init_propvariant()) >> hr_handler{"IPropertyStore::GetValue"};
            return string_tools::to_string(this->propvariant().pwszVal);
        }
        // In case the needed PROPERTYKEYs are not defined, alternative pkey type may be used:
        enum pkey {
            PKEY_Device_FriendlyName
            // More pkeys to be added as needed...
        };
        std::string value (pkey property_key) {
            switch (property_key) {
            case pkey::PKEY_Device_FriendlyName: default:
                PROPERTYKEY PKEY_Device_FriendlyName;
                PKEY_Device_FriendlyName.pid = 14;
                PKEY_Device_FriendlyName.fmtid = GUID{0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}};
                return this->value(PKEY_Device_FriendlyName);
                // More pkeys to be added as needed...
            }
        }
    
    private:
        PROPVARIANT& propvariant () {
            return this->propvariant_wrapper_.get();
        }

        PROPVARIANT& init_propvariant () {
            this->propvariant_wrapper_ = {};
            return this->propvariant();
        }

        propvariant_wrapper propvariant_wrapper_;
    };

}