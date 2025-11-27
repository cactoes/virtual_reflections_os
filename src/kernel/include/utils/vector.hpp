//==========================================
/// @file       vector.hpp
/// @brief      vector class implementation
//==========================================

#pragma once

#ifndef __VECTOR_HPP__
#define __VECTOR_HPP__

#include "memory/heap.hpp"

template <typename T>
struct node_t {
    node_t<T>* next;
    T value;
};

template <typename T>
class linked_list_iterator {
public:
    linked_list_iterator(node_t<T>* p_ptr) : node(p_ptr) {}

    linked_list_iterator& operator++() {
        if (node)
            node = node->next;

        return *this;
    }

    bool operator!=(const linked_list_iterator& other) const {
        return node != other.node;
    }

    T& operator*() const {
        return node->value;
    }

    T* operator->() const {
        return &node->value;
    }

private:
    node_t<T>* node;
};

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
class linked_list {
public:
    linked_list() = default;

    template <typename... Args>
    linked_list(Args&&... args) {
        (insert_back(forward<Args>(args)), ...);
    }

    linked_list(const T* p_arr, size_t arr_size) {
        for (size_t i = 0; i < arr_size; i++)
            insert_back(p_arr[i]);
    }

    template <size_t arr_size>
    linked_list(const T (&p_arr)[arr_size]) {
        linked_list(p_arr, arr_size);
    }

    linked_list(const linked_list<T>& other) {
        node_t<T>* node = other.first_node;
        while (node) {
            insert_back(move(node->value));
            node = node->next;
        }
    }

    linked_list& operator=(const linked_list<T>& other) {
        if (this == &other)
            return *this;

        clear();

        node_t<T>* node = other.first_node;
        while (node) {
            insert_back(move(node->value));
            node = node->next;
        }

        return *this;
    }

    linked_list(linked_list<T>&& other) {
        first_node = other.first_node;
        size = other.size;

        other.first_node = nullptr;
        other.size = 0;
    }

    linked_list& operator=(linked_list<T>&& other) {
        if (this == &other)
            return *this;
        
        clear();
        
        first_node = other.first_node;
        size = other.size;
        
        other.first_node = nullptr;
        other.size = 0;
        
        return *this;
    }

    linked_list_iterator<T> begin() {
        return linked_list_iterator<T>(first_node);
    }

    linked_list_iterator<T> end() {
        return linked_list_iterator<T>(nullptr);
    }

    linked_list_iterator<T> find(const T& value) {
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
        if (!first_node)
            return false;
        
        if (index == 0) {
            node_t<T>* node_to_delete = first_node;
            first_node = first_node->next;
            free(node_to_delete);
            size--;
            return true;
        }

        node_t<T>* prev_node = first_node;
        for (size_t i = 0; i < index - 1; i++) {
            if (!prev_node->next)
                return false;

            prev_node = prev_node->next;
        }

        if (node_t<T>* node_to_delete = prev_node->next) {
            prev_node->next = node_to_delete->next;
            free(node_to_delete);
            size--;
            return true;
        }

        return false;
    }

    bool insert_at(size_t index, T&& value) {
        node_t<T>* new_node = (node_t<T>*)malloc(sizeof(node_t<T>));
        if (!new_node)
            return false;

        new_node->next = nullptr;
        new (&new_node->value) T(move(value));

        if (index == 0) {
            new_node->next = first_node;
            first_node = new_node;
            size++;
            return true;
        }

        node_t<T>* prev_node = first_node;
        for (size_t i = 0; i < index - 1; i++) {
            if (!prev_node)
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

    bool insert_at(size_t index, const T& value) {
        return insert_at(index, T(value));
    }

    bool insert_back(T&& value) {
        return insert_at(size, move(value));
    }

    bool insert_back(const T& value) {
        return insert_back(T(value));
    }

    T* get_at(size_t index) {
        if (!first_node || index >= size)
            return nullptr;

        size_t current_index = 0;
        for (auto node = first_node; node; node = node->next) {
            if (current_index == index)
                return &node->value;

            current_index++;
        }

        return nullptr;
    }

    const T* get_at(size_t index) const {
        if (!first_node || index >= size)
            return nullptr;

        size_t current_index = 0;
        for (auto node = first_node; node; node = node->next) {
            if (current_index == index)
                return &node->value;

            current_index++;
        }

        return nullptr;
    }

    void clear() {
        node_t<T>* node = first_node;
        while (node) {
            node_t<T>* next = node->next;
            node->value.~T();
            free(node);
            node = next;
        }
        first_node = nullptr;
        size = 0;
    }

private:
    node_t<T>* first_node = nullptr;
    size_t size = 0;
};

#endif // __VECTOR_HPP__