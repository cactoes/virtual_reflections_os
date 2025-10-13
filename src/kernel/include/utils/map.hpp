//==========================================
/// @file       map.hpp
/// @brief      map implementations
//==========================================

#pragma once

#ifndef __UTILS_MAP_HPP__
#define __UTILS_MAP_HPP__

#include "memory/heap.hpp"
#include "std/array.hpp"

template <typename T, typename U>
struct key_value_pair_t {
    T key;
    U value;
};

template <typename T, typename U>
class linear_map_iterator {
public:
    linear_map_iterator(key_value_pair_t<T, U>* p_ptr) : ptr(p_ptr) {}

    linear_map_iterator& operator++() {
        return advance();
    }

    linear_map_iterator& advance() {
        ++ptr;
        return *this;
    }

    bool operator!=(const linear_map_iterator& other) const {
        return ptr != other.ptr;
    }

    bool operator==(const linear_map_iterator& other) const {
        return ptr == other.ptr;
    }

    key_value_pair_t<T, U>& operator*() const {
        return *ptr;
    }

    key_value_pair_t<T, U>* operator->() const {
        return ptr;
    }

private:
    key_value_pair_t<T, U>* ptr;
};

template <typename T, typename U>
class linear_map {
public:
    linear_map() = default;
    
    linear_map(const linear_map<T, U>& other) {
        data = other.data;
    }

    linear_map& operator=(const linear_map<T, U>& other) {
        if (this != &other)
            data = other.data;
        return *this;
    }

    linear_map(linear_map<T, U>&& other) noexcept {
        data = move(other.data);
    }

    linear_map& operator=(linear_map<T, U>&& other) noexcept {
        if (this != &other)
            data = move(other.data);
        return *this;
    }
    
    ~linear_map() = default;

    linear_map_iterator<T, U> begin() {
        return linear_map_iterator<T, U>(data.get_data());
    }

    linear_map_iterator<T, U> end() {
        return linear_map_iterator<T, U>(data.get_data() + data.length());
    }

    linear_map_iterator<T, U> get(const T& key) {
        for (auto it = begin(); it != end(); ++it) {
            if (it->key == key)
                return it;
        }

        return end();
    }

    bool contains(const T& key) {
        return get(key) != end();
    }

    U& operator[](const T& key) {
        linear_map_iterator<T, U> existing = get(key);
        if (existing != end())
            return existing->value;

        data.insert_back(key_value_pair_t<T, U>{ key, U{} });
        return data[data.length() - 1].value;
    }

    size_t size() const {
        return data.length();
    }

    bool insert(const T& key, U&& value) {
        if (contains(key))
            return false;

        data.insert_back(key_value_pair_t<T, U>{ key, move(value) });
        return true;
    }

    bool insert(const T& key, const U& value) {
        return insert(key, U(value));
    }

    bool remove(const T& key) {
        for (size_t i = 0; i < data.length(); i++) {
            if (data.get_at(i)->key == key) {
                data.delete_at(i);
                return true;
            }
        }

        return false;
    }

    void clear() {
        data.clear();
    }

    key_value_pair_t<T, U>* get_data() const {
        return data.get_data();
    }

private:
    std::dynamic_array<key_value_pair_t<T, U>> data {};
};

#endif // __UTILS_MAP_HPP__