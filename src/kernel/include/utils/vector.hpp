//==========================================
/// @file       vector.hpp
/// @brief      vector class implementation
//==========================================

#pragma once

#ifndef __VECTOR_HPP__
#define __VECTOR_HPP__

#include "memory/heap.hpp"
#include "mutex.hpp"

template <class T>
struct vector_node_t {
    vector_node_t<T>* next;
    T value;
};

template <class T>
class iterator {
public:
    iterator(vector_node_t<T>* p_ptr) : node(p_ptr) {}

    iterator& operator++() {
        if (node)
            node = node->next;

        return *this;
    }

    bool operator!=(const iterator& other) const {
        return node != other.node;
    }

    T& operator*() const {
        return node->value;
    }

private:
    vector_node_t<T>* node;
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
    vector& operator=(vector&& other) noexcept {
        if (this == &other)
            return *this;
        
        clear();
        
        mutex_lock_guard other_guard(&other.mutex);
        
        first_node = other.first_node;
        size = other.size;
        
        other.first_node = nullptr;
        other.size = 0;
        
        return *this;
    }

    ~vector() {
        clear();
    }

    iterator<T> begin() {
        return iterator<T>(first_node);
    }

    iterator<T> end() {
        return iterator<T>(nullptr);
    }

    size_t length() {
        return size;
    }

    bool delete_at(size_t index) {
        mutex_lock_guard guard(&mutex);
        const auto result = delete_at_unprotected(index);
        return result;
    }

    bool insert_at(size_t index, T value) {
        mutex_lock_guard guard(&mutex);
        const auto result = insert_at_unprotected(index, value);
        return result;
    }

    bool insert_back(T value) {
        mutex_lock_guard guard(&mutex);
        const auto result = insert_at_unprotected(length(), value);
        return result;
    }

    T* get_at(size_t index) {
        mutex_lock_guard guard(&mutex);
        const auto result = get_at_unprotected(index);
        return result;
    }

    vector_node_t<T>* first() {
        return first_node;
    }

    void clear() {
        mutex_lock_guard guard(&mutex);
        vector_node_t<T>* node = first();
        while (node) {
            vector_node_t<T>* next = node->next;
            heap_free(get_global_heap(), node);
            node = next;
        }
        first_node = nullptr;
        size = 0;
    }

private:
    T* get_at_unprotected(size_t index) {
        if (!first())
            return nullptr;

        size_t current_index = 0;
        for (auto node = first_node; node; node = node->next) {
            if (current_index == index)
                return &node->value;

            current_index++;
        }

        return nullptr;
    }

    bool delete_at_unprotected(size_t index) {
        if (!first())
            return false;
        
        if (index == 0) {
            vector_node_t<T>* first_node = first();
            first_node = first_node->next;
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
            first_node = new_node;
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
            new_node->next = prev_node->next;
            prev_node->next = new_node;
            size++;
            return true;
        }

        return false;
    }

private:
    vector_node_t<T>* first_node = nullptr;
    mutex_t mutex {};
    size_t size = 0;
};

#endif // __VECTOR_HPP__