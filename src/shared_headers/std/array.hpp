//==========================================
/// @file       array.hpp
/// @brief      array implementation
//==========================================

#pragma once

#ifndef __STD_ARRAY_HPP__
#define __STD_ARRAY_HPP__

#include "common.hpp"

namespace std {

template <typename T>
class array_iterator {
public:
    array_iterator(T* p_ptr) : ptr(p_ptr) {}

    array_iterator& operator++() {
        ptr++;
        return *this;
    }

    bool operator!=(const array_iterator& other) const {
        return ptr != other.ptr;
    }

    T& operator*() const {
        return *ptr;
    }

    T* operator->() const {
        return ptr;
    }

private:
    T* ptr;
};

template <typename T, size_t size>
class static_array {
public:
    static_array() = default;

    template <typename... Args>
    static_array(Args&&... args) {
        size_t i = 0;
        ( (i < size ? insert_at(i++, forward<Args>(args)) : false), ...);
    }

    static_array(const T (&p_arr)[size]) {
        for (size_t i = 0; i < size; i++)
            new (&data[i]) T(p_arr[i]);
    }

    static_array(const static_array& other) {
        for(size_t i = 0; i < size; i++)
            new (&data[i]) T(other.data[i]);
    }

    static_array(static_array&& other) noexcept {
        for(size_t i = 0; i < size; i++)
            new (&data[i]) T(move(other.data[i]));
    }

    static_array& operator=(const static_array& other) {
        if (this != &other) {
            clear();
            for(size_t i = 0; i < size; i++)
                new (&data[i]) T(other.data[i]);
        }
        return *this;
    }

    static_array& operator=(static_array&& other) noexcept {
        if (this != &other) {
            clear();
            for(size_t i = 0; i < size; i++)
                new (&data[i]) T(move(other.data[i]));
        }
        return *this;
    }

    ~static_array() {
        clear();
    }

    array_iterator<T> begin() {
        return array_iterator<T>(data);
    }

    array_iterator<T> end() {
        return array_iterator<T>(data + size);
    }

    array_iterator<T> find(const T& value) {
        for (auto it = begin(); it != end(); it++) {
            if (*it == value)
                return it;
        }

        return end();
    }

    bool contains(const T& value) {
        return find(value) != end();
    }

    T& operator[](size_t index) {
        return *get_at(index);
    }

    const T& operator[](size_t index) const {
        return *get_at(index);
    }

    size_t length() const {
        return size;
    }

    bool insert_at(size_t index, T&& value) {
        if (index > size)
            return false;

        new (&data[index]) T(value);
        return true;
    }

    T* get_at(size_t index) {
        return &data[index];
    }

    const T* get_at(size_t index) const {
        return &data[index];
    }

    T* get_data() const {
        return data;
    }

    void clear() {
        for(size_t i = 0; i < size; i++)
            data[i].~T();
    }

private:
    T data[size];
};

} // namespace std

#endif // __STD_ARRAY_HPP__