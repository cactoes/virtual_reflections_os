//==========================================
/// @file       pointer.hpp
/// @brief      custom pointer class
//==========================================

#pragma once

#ifndef __UTILS_POINTER_HPP__
#define __UTILS_POINTER_HPP__

#include "memory/heap.hpp"

namespace ptr {

template <typename T>
class unique {
public:
    unique() : ptr(nullptr) {}
    explicit unique(T* p_ptr) : ptr(p_ptr) {}

    // no copy
    unique(const unique& other) = delete;
    unique& operator=(const unique& other) = delete;
    
    template <typename U>
    unique(unique<U>&& other) {
        ptr = other.ptr;
        other.ptr = nullptr;
    }

    template <typename U>
    unique& operator=(unique<U>&& other) {
        if ((void*)(this) != (void*)(&other)) {
            GFREE(ptr);
            ptr = other.ptr;
            other.ptr = nullptr;
        }

        return *this;
    }
    
    ~unique() {
        if (ptr)
            GFREE(ptr);
    }

    T& operator*() const {
        return *ptr;
    }

    T* operator->() const {
        return ptr;
    }

    T& operator[](size_t index) const {
        return ptr[index];
    };

    T* get() const {
        return ptr;
    }

    T* release() {
        T* temp = ptr;
        ptr = nullptr;
        return temp;
    }

    explicit operator bool() const {
        return ptr != nullptr;
    }

private:
    template<typename> friend class unique;
    T* ptr;
};

template <typename T, typename... Args>
ptr::unique<T> make_unique(Args&&... args) {
    void* raw = GALLOC(sizeof(T));
    T* obj = new (raw) T(forward<Args>(args)...);
    return ptr::unique<T>(obj);
}

} // namespace ptr

#endif // __UTILS_POINTER_HPP__