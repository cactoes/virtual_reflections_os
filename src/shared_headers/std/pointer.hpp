//==========================================
/// @file       pointer.hpp
/// @brief      pointer impl
//==========================================

#pragma once

#ifndef __STD_POINTER_HPP__
#define __STD_POINTER_HPP__

#include "common.hpp"

namespace std {

template <typename T>
class unique_ptr {
public:
    unique_ptr() : ptr(nullptr) {}
    explicit unique_ptr(T* p_ptr) : ptr(p_ptr) {}

    // no copy
    unique_ptr(const unique_ptr& other) = delete;
    unique_ptr& operator=(const unique_ptr& other) = delete;
    
    template <typename U>
    unique_ptr(unique_ptr<U>&& other) {
        ptr = other.ptr;
        other.ptr = nullptr;
    }

    template <typename U>
    unique_ptr& operator=(unique_ptr<U>&& other) {
        if ((void*)(this) != (void*)(&other)) {
            free(ptr);
            ptr = other.ptr;
            other.ptr = nullptr;
        }

        return *this;
    }
    
    ~unique_ptr() {
        if (ptr)
            free(ptr);
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
    template<typename> friend class unique_ptr;
    T* ptr;
};

template <>
class unique_ptr<void> {
public:
    unique_ptr() : ptr(nullptr) {}
    explicit unique_ptr(void* p_ptr) : ptr(p_ptr) {}

    // no copy
    unique_ptr(const unique_ptr& other) = delete;
    unique_ptr& operator=(const unique_ptr& other) = delete;
    
    template <typename U>
    unique_ptr(unique_ptr<U>&& other) {
        ptr = other.ptr;
        other.ptr = nullptr;
    }

    template <typename U>
    unique_ptr& operator=(unique_ptr<U>&& other) {
        if ((void*)(this) != (void*)(&other)) {
            free(ptr);
            ptr = other.ptr;
            other.ptr = nullptr;
        }

        return *this;
    }
    
    ~unique_ptr() {
        if (ptr)
            free(ptr);
    }

    void* operator->() const {
        return ptr;
    }

    void* get() const {
        return ptr;
    }

    void* release() {
        void* temp = ptr;
        ptr = nullptr;
        return temp;
    }

    explicit operator bool() const {
        return ptr != nullptr;
    }

private:
    template<typename> friend class unique_ptr;
    void* ptr;
};

template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) {
    void* raw = malloc(sizeof(T));
    T* obj = new (raw) T(forward<Args>(args)...);
    return unique_ptr<T>(obj);
}

} // namespace std

#endif // __STD_POINTER_HPP__