//==========================================
/// @file       vector.hpp
/// @brief      vector class impementation
//==========================================

#pragma once

#ifndef __VECTOR_HPP__
#define __VECTOR_HPP__

#include "memory.hpp"

template <typename T>
class vector {
public:
    struct vector_node_t {
        vector_node_t* next;
        T* value;
    };

    vector() = default;

    // copy
    vector(const vector& other) = delete;
    vector& operator=(const vector& other) = delete;

    // move
    vector(vector&& other) = delete;
    vector& operator=(vector&& other) = delete;

    ~vector() {
        vector_node_t* node = first();
        while (node) {
            vector_node_t* next = node->next;
            heap_free(get_global_heap(), node);
            node = next;
        }
    }

    void insert_back(T& value) {
        vector_node_t* new_node = (vector_node_t*)heap_alloc(get_global_heap(), sizeof(vector_node_t));
        new_node->next = nullptr;
        new_node->value = value;

        if (!_first) {
            _first = new_node;
        } else {
            vector_node_t* last_node = _first;
            while (last_node->next)
                last_node = last_node->next;
            last_node->next = new_node;
        }
    }

    vector_node_t* first() {
        return _first;
    }

private:
    vector_node_t* _first = nullptr;
};

#endif // __VECTOR_HPP__