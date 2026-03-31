//==========================================
/// @file       optional.hpp
/// @brief      optional return type
/// @todo       complete this implementation
//==========================================

#pragma once

#ifndef __UTILS_OPTIONAL_HPP__
#define __UTILS_OPTIONAL_HPP__

#include "common.hpp"

struct nullopt_t {
    constexpr nullopt_t() = default;
};

static constexpr nullopt_t nullopt {};

template <typename T>
class optional {
public:
    constexpr optional() : b_has_value(false) {}
    constexpr optional(nullopt_t) : b_has_value(false) {}
    constexpr optional(const T& val) : value(val), b_has_value(true) {}
    constexpr optional(T&& val) : value(move(val)), b_has_value(true) {}
    constexpr optional(const optional& other) = default;
    constexpr optional(optional&& other) = default;
    constexpr optional& operator=(const optional& other) = default;
    constexpr optional& operator=(optional&& other) = default;

    constexpr optional& operator=(const T& val) {
        value = val;
        b_has_value = true;
        return *this;
    }

    constexpr optional& operator=(T&& val) {
        value = move(val);
        b_has_value = true;
        return *this;
    }

    constexpr optional& operator=(nullopt_t) {
        b_has_value = false;
        return *this;
    }

    constexpr bool has_value() const {
        return b_has_value;
    }

    constexpr explicit operator bool() const {
        return b_has_value;
    }

    constexpr T& value_or(T& default_value) {
        return b_has_value ? value : default_value;
    }

    constexpr const T& value_or(const T& default_value) const {
        return b_has_value ? value : default_value;
    }

    constexpr T& get_value() {
        return value;
    }

    constexpr const T& get_value() const {
        return value;
    }

    constexpr T* operator->() {
        return &value;
    }

    constexpr const T* operator->() const {
        return &value;
    }

    constexpr T& operator*() {
        return value;
    }

    constexpr const T& operator*() const {
        return value;
    }

    constexpr bool operator==(const optional& other) const {
        if (b_has_value != other.b_has_value)
            return false;

        if (!b_has_value)
            return true;

        return value == other.value;
    }

    constexpr bool operator!=(const optional& other) const {
        return !(*this == other);
    }

    constexpr bool operator==(nullopt_t) const {
        return !b_has_value;
    }

    constexpr bool operator!=(nullopt_t) const {
        return b_has_value;
    }

private:
    T value {};
    bool b_has_value = false;
};

#endif // __UTILS_OPTIONAL_HPP__