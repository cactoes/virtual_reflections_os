//==========================================
/// @file       vector.hpp
/// @brief      vector class impementation
//==========================================

#pragma once

#ifndef __VECTOR_HPP__
#define __VECTOR_HPP__

#include "memory.hpp"
#include "mutex.hpp"

template <class T>
struct vector_node_t {
    vector_node_t<T>* next;
    T value;
};

template <class T>
class vector {
public:
    vector() {
        mutex_init(&mutex);
    }

    // copy
    vector(const vector& other) = delete;
    vector& operator=(const vector& other) = delete;

    // move
    vector(vector&& other) = delete;
    vector& operator=(vector&& other) = delete;

    ~vector() {
        vector_node_t<T>* node = first();
        while (node) {
            vector_node_t<T>* next = node->next;
            heap_free(get_global_heap(), node);
            node = next;
        }
    }

    void insert_back(T value) {
        mutex_lock(&mutex);
        
        vector_node_t<T>* new_node = (vector_node_t<T>*)heap_alloc(get_global_heap(), sizeof(vector_node_t<T>));
        new_node->next = nullptr;
        new_node->value = value;

        if (!_first) {
            _first = new_node;
        } else {
            vector_node_t<T>* last_node = _first;
            while (last_node->next)
                last_node = last_node->next;
            last_node->next = new_node;
        }
        
        mutex_unlock(&mutex);
    }

    vector_node_t<T>* first() {
        return _first;
    }

private:
    vector_node_t<T>* _first = nullptr;
    mutex_t mutex {};
};

#endif // __VECTOR_HPP__