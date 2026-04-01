//==========================================
/// @file       ring_buffer.hpp
/// @brief      head / tail based static array
//==========================================

#pragma once

#ifndef __RING_BUFFER_HPP__
#define __RING_BUFFER_HPP__

#include "common.hpp"

namespace std {

template <size_t buffer_size, typename T>
class ring_buffer {
public:
    ring_buffer() = default;
    ring_buffer(const ring_buffer& other) = delete;
    ring_buffer& operator=(const ring_buffer& other) = delete;
    ring_buffer(ring_buffer&& other) = delete;
    ring_buffer& operator=(ring_buffer&& other) = delete;
    ~ring_buffer() = default;

    bool insert(const T& data) {
        size_t next = (head + 1) % buffer_size;
        if (next == tail)
            return false;

        buffer[head] = data;
        head = next;
        return true;
    }

    bool insert(T&& data) {
        size_t next = (head + 1) % buffer_size;
        if (next == tail)
            return false;

        buffer[head] = move(data);
        head = next;
        return true;
    }

    bool get(T& out) {
        if (head == tail)
            return false;

        out = move(buffer[tail]);

        tail = (tail + 1) % buffer_size;

        return true;
    }

    size_t capacity() const {
        return buffer_size - 1;
    }

    size_t size() const {
        return (head + buffer_size - tail) % buffer_size;
    }

    bool empty() const {
        return head == tail;
    }

    bool full() const {
        return ((head + 1) % buffer_size) == tail;
    }

private:
    T buffer[buffer_size];
    size_t head = 0;
    size_t tail = 0;
};

};

#endif // __RING_BUFFER_HPP__