//==========================================
/// @file       bitmap.hpp
/// @brief      basic bitmap implementation
///             with STATIC memory
//==========================================

#pragma once

#ifndef __BITMAP_HPP__
#define __BITMAP_HPP__

#include "common.hpp"

class abstract_bitmap {
public:
    // virtual ~abstract_bitmap() = default; // causes compiler error qq

    /// @brief          get the state of a bit at index n
    /// @param index    index of the bit
    /// @return         true if the bit is set else false
    virtual bool get(size_t index) const = 0;

    /// @brief          sets a bit to a state at index n
    /// @param index    index of the bit
    /// @param state    state to set the bit to
    virtual void set(size_t index, bool state) = 0;

    /// @brief      sets the bit at the current highest index to true
    /// @return     index of the bit
    /// @remarks    this will move the current highest index by +1
    virtual size_t set_at_top() = 0;

    /// @brief      returns the top of the bitmap
    /// @return     current highest index
    virtual size_t get_top() const = 0;

    /// @brief      returns the total size of the bitmaps
    /// @return     size of bitmap
    virtual size_t get_size() const = 0;

    /// @brief      loops over ALL bits in the bitmap & finds the first free index & sets it
    /// @return     returns the index of the bit
    virtual size_t set_next_free() = 0;
};

template <size_t max_size>
class bitmap : public abstract_bitmap {
public:
    bitmap() = default;
    ~bitmap() = default;
    
    bitmap(const bitmap& other) = delete;
    bitmap& operator=(const bitmap& other) = delete;
    bitmap(bitmap&& other) = delete;
    bitmap& operator=(bitmap&& other) = delete;

    bool get(size_t index) const override{
        // FIXME: out of range can crash
        //        this is on purpose since
        //        no error handling is implemented
        size_t item_index = index / 64;
        size_t bit_index = index % 64;
        return (m_field[item_index] >> bit_index) & 1;
    }

    void set(size_t index, bool state) override {
        // FIXME: out of range can crash
        //        this is on purpose since
        //        no error handling is implemented

        size_t item_index = index / 64;
        size_t bit_index = index % 64;

        if (state) {
            m_field[item_index] |= (1ULL << bit_index);
            if (index > m_top_pointer)
                m_top_pointer = index;
        } else {
            m_field[item_index] &= ~(1ULL << bit_index);
        }
    }

    size_t set_at_top() override {
        // FIXME: out of range can crash
        //        this is on purpose since
        //        no error handling is implemented
        set(m_top_pointer + 1, true);
        return m_top_pointer;
    }

    size_t get_top() const override {
        return m_top_pointer;
    }

    size_t get_size() const override {
        return max_size;
    }

    size_t set_next_free() override {
        for (size_t i = 0; i < max_size; i++) {
            if (!get(i)) {
                set(i, true);
                return i;
            }
        }

        // FIXME: basically another out of range error
        return -1;
    }

private:
    static constexpr size_t item_count = (max_size + 63) / 64;
    uint64_t m_field[item_count] = {};
    size_t m_top_pointer = 0;
};

#endif // __BITMAP_HPP__