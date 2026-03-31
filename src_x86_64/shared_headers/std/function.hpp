#pragma once

#ifndef __STD_FUNCTION_HPP__
#define __STD_FUNCTION_HPP__

#include "common.hpp"

namespace std {

template <typename R, typename... Args>
class function_t {
public:
    function_t() = default;

    function_t(R(*func)(Args...)) 
        : obj(reinterpret_cast<void*>(func)), 
          caller([](void* f, Args... args){ 
              auto fn = reinterpret_cast<R(*)(Args...)>(f);
              fn(forward<Args>(args)...);
          }) {}

    template<typename T>
    function_t(T* instance, R(T::*func)(Args...)) {
        struct Holder {
            T* inst;
            R(T::*memfunc)(Args...);
        };
        static Holder h; // only works for single binding
        h = { instance, func };

        obj = &h;
        caller = [](void* ctx, Args... args) {
            Holder* h = static_cast<Holder*>(ctx);
            (h->inst->*h->memfunc)(forward<Args>(args)...);
        };
    }

    void operator()(Args... args) const {
        if (caller) caller(obj, forward<Args>(args)...);
    }

private:
    void* obj = nullptr;
    R(*caller)(void*, Args...) = nullptr;
};

} // namespace std

#endif // __STD_FUNCTION_HPP__