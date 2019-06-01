#pragma once

#include "exceptions.hpp"

#include <iostream>

namespace cynth::api::wasapi {

    template <typename Interface>
    class interface_wrapper;

    template <typename Interface>
    class interface_base {
        template <typename OtherInterface> friend class interface_wrapper;
        template <typename OtherInterface> friend class interface_base;

    protected:
        /*/ CRTP: /*/
        using base_t    = interface_base<Interface>;
        using derived_t = interface_wrapper<Interface>;

    public:
        /*/ Destruction & cleanup: /*/
        bool auto_release () const {
            return this->auto_release_;
        }
        derived_t& auto_release (bool release) {
            this->auto_release_ = release;
            return this->derived();
        }
        derived_t& release () {
            if (this->isset()) {
                (*this)->Release();
                this->reset();
            }
            return this->derived();
        }
        ~interface_base () {
            if (this->auto_release())
                this->release();
        }

        /*/ Copy & move: /*/
        interface_base (const interface_base&) = delete;
        //interface_base (const derived_t&)      = delete;
        interface_base& operator= (const interface_base&) = delete;
        //derived_t&      operator= (const derived_t&)      = delete;
        interface_base (interface_base&& other):
            interface_ptr_{std::move(other.interface_ptr_)},
            auto_release_{std::move(other.auto_release_)} {
            other.auto_release(false);
        }
        derived_t& operator= (interface_base&& other) {
            this->interface_ptr_ = std::move(other.interface_ptr_);
            this->auto_release_  = std::move(other.auto_release_);
            other.auto_release(false);
            return this->derived();
        }
    
    protected:
        /*/ Access: /*/
        bool isset () const { return this->interface_ptr_; }
        derived_t& reset () {
            this->interface_ptr_ = nullptr;
            return this->derived();
        }
        Interface& get () const {
            if (!this->isset())
                throw exception{"Trying to access an interface of an uninitialized interface_wrapper."};
            return *this->interface_ptr_;
        }
        Interface& operator*  () const { return this->get(); }
        Interface* operator-> () const { return &this->get(); }

        /*/ CRTP: /*/
        base_t&          base    ()       { return *static_cast<interface_base<Interface>*>(this); }
        const base_t&    base    () const { return *static_cast<const interface_base<Interface>*>(this); }
        derived_t&       derived ()       { return *static_cast<interface_wrapper<Interface>*>(this); }
        const derived_t& derived () const { return *static_cast<const interface_wrapper<Interface>*>(this); }
    
    private:
        /*/ Construction: /*/
        interface_base (Interface* interface_ptr = nullptr):
            interface_ptr_{interface_ptr},
            auto_release_{true} {}
    
    protected:
        Interface* interface_ptr_;
        bool       auto_release_;
    };

    /*/ Default implementation: /*/
    // Implementation for specific interfaces is achived with template specialization.
    template <typename Interface>
    class interface_wrapper: public interface_base<Interface> {
        template <typename OtherInterface> friend class interface_wrapper;
        using interface_base<Interface>::interface_base;
        using interface_base<Interface>::operator=;
    };
}