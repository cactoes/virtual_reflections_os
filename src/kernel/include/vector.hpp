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

    size_t length() {
        return size;
    }

    bool delete_at(size_t index) {
        mutex_lock(&mutex);
        const auto result = delete_at_unprotected(index);
        mutex_unlock(&mutex);
        return result;
    }

    bool insert_at(size_t index, T value) {
        mutex_lock(&mutex);
        const auto result = insert_at_unprotected(index, value);
        mutex_unlock(&mutex);
        return result;
    }

    bool insert_back(T value) {
        mutex_lock(&mutex);
        const auto result = insert_at_unprotected(length(), value);
        mutex_unlock(&mutex);
        return result;
    }

    vector_node_t<T>* first() {
        return _first;
    }

private:
    bool delete_at_unprotected(size_t index) {
        if (!first())
            return false;
        
        if (index == 0) {
            vector_node_t<T>* first_node = first();
            _first = first_node->next;
            heap_free(get_global_heap(), first_node);
            size--;
            return true;
        }

        vector_node_t<T>* prev_node = first();
        for (size_t i = 0; i < index - 1; i++) {
            if (!prev_node->next)
                return false;

            prev_node = prev_node->next;
        }

        if (vector_node_t<T>* node_to_delete = prev_node->next) {
            prev_node->next = node_to_delete->next;
            heap_free(get_global_heap(), node_to_delete);
            size--;
            return true;
        }

        return false;
    }

    bool insert_at_unprotected(size_t index, T value) {
        vector_node_t<T>* new_node = (vector_node_t<T>*)heap_alloc(get_global_heap(), sizeof(vector_node_t<T>));
        if (!new_node)
            return false;

        new_node->next = nullptr;
        new_node->value = value;

        if (index == 0) {
            new_node->next = first();
            _first = new_node;
            size++;
            return true;
        }

        vector_node_t<T>* prev_node = first();
        for (size_t i = 0; i < index - 1; i++) {
            if (!prev_node->next)
                return false;

            prev_node = prev_node->next;
        }

        if (prev_node) {
            new_node->next = prev_node;
            prev_node->next = new_node;
            size++;
            return true;
        }

        return false;
    }

private:
    vector_node_t<T>* _first = nullptr;
    mutex_t mutex {};
    size_t size = 0;
};

#endif // __VECTOR_HPP__