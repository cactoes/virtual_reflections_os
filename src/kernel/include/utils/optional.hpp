//==========================================
/// @file       optional.hpp
/// @brief      optional return type
/// @todo       complete this implementation
//==========================================

#pragma once

#ifndef __UTILS_OPTIONAL_HPP__
#define __UTILS_OPTIONAL_HPP__

struct nullopt_t {
    nullopt_t() = default;
};

// NOLINTNEXTLINE
static nullopt_t nullopt {};

template <typename T>
class optional {
public:
    T* operator->() const {
        return &value;
    }

    T& operator*() const {
        return value;
    }

    bool operator==(const optional& other) const {
        if (has_value != other.has_value)
            return false;
        
        if (!has_value)
            return true;

        return value_ == other.value_;
    }

    bool operator!=(const optional& other) const {
        return !(*this == other);
    }

    bool operator==(nullopt_t) const {
        return !has_value;
    }

    bool operator!=(nullopt_t) const {
        return has_value;
    }

private:
    T value;
    bool has_value;
};

#endif // __UTILS_OPTIONAL_HPP__