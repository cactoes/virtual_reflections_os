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

template <typename T>
class dynamic_array {
public:
    dynamic_array() = default;

    dynamic_array(const T* p_arr, size_t arr_size) {
        resize(arr_size);
        size = arr_size;

        for (size_t i = 0; i < arr_size; i++)
            new (&data[i]) T(p_arr[i]);
    }

    template <size_t arr_size>
    dynamic_array(const T (&p_arr)[arr_size]) : dynamic_array(p_arr, arr_size) {}

    dynamic_array(const dynamic_array<T>& other) {
        size = other.size;
        capacity = other.capacity;

        data = (T*)malloc_aligned(capacity * sizeof(T), alignof(T));
        for (size_t i = 0; i < size; i++) {
            new (&data[i]) T(other.data[i]);
        }
    }

    dynamic_array(const T& data) {
        insert_back(data);
    }

    dynamic_array& operator=(const dynamic_array<T>& other) {
        if (this == &other)
            return *this;

        clear();
        if (capacity < other.size) {
            free_aligned(data);
            capacity = other.capacity;
            data = (T*)malloc_aligned(capacity * sizeof(T), alignof(T));
        }

        size = other.size;
        for (size_t i = 0; i < size; i++) {
            new (&data[i]) T(other.data[i]);
        }

        return *this;
    }

    dynamic_array(dynamic_array<T>&& other) noexcept {
        data = other.data;
        size = other.size;
        capacity = other.capacity;

        other.data = nullptr;
        other.size = 0;
        other.capacity = 0;
    }

    dynamic_array& operator=(dynamic_array<T>&& other) noexcept {
        if (this == &other)
            return *this;

        clear();
        free_aligned(data);

        data = other.data;
        size = other.size;
        capacity = other.capacity;

        other.data = nullptr;
        other.size = 0;
        other.capacity = 0;

        return *this;
    }

    ~dynamic_array() {
        clear();
        free_aligned(data);
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

    bool delete_at(size_t index) {
        if (index >= size)
            return false;

        data[index].~T();

        for (size_t i = index; i < size - 1; i++) {
            new (&data[i]) T(move(data[i + 1]));
            data[i + 1].~T();
        }

        size--;
        return true;
    }

    bool insert_at(size_t index, T&& value) {
        if (index > size)
            return false;

        if (size == capacity)
            resize(capacity + 2);

        for (size_t i = size; i > index; --i) {
            new (&data[i]) T(move(data[i - 1]));
            data[i - 1].~T();
        }

        new (&data[index]) T(value);
        size++;
        return true;
    }

    bool insert_at(size_t index, const T& value) {
        return insert_at(index, T(value));
    }

    void insert_back(T&& value) {
        if (size == capacity)
            resize(capacity + 2);

        new (&data[size]) T(move(value));
        size++;
    }

    void insert_back(const T& value) {
        insert_back(T(value));
    }

    T* get_at(size_t index) {
        return &data[index];
    }

    const T* get_at(size_t index) const {
        return &data[index];
    }

    void clear() {
        for (size_t i = 0; i < size; i++)
            data[i].~T();

        size = 0;
    }

    T* get_data() const {
        return data;
    }

    bool resize(size_t new_size) {
        if (new_size <= capacity)
            return false;

        T* new_data = (T*)malloc_aligned(new_size * sizeof(T), alignof(T));
        if (!new_data)
            return false;

        for (size_t i = 0; i < size; i++) {
            new (&new_data[i]) T(move(data[i]));
            data[i].~T();
        }

        if (data)
            free_aligned(data);

        data = new_data;
        capacity = new_size;
        return true;
    }

    void assign(const T* p_data, size_t item_count) {
        clear();
        resize(item_count);

        for (size_t i = 0; i < item_count; i++)
            new (&data[i]) T(p_data[i]);

        size = item_count;
    }

private:
    T* data = nullptr;
    size_t size = 0;
    size_t capacity = 0;
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
    T data[size] {};
};

} // namespace std

#endif // __STD_ARRAY_HPP__