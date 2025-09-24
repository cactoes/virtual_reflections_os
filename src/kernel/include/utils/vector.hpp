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
        for (size_t i = 0; i < arr_size; ++i)
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
            heap_free(get_global_heap(), node_to_delete);
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
            heap_free(get_global_heap(), node_to_delete);
            size--;
            return true;
        }

        return false;
    }

    bool insert_at(size_t index, T&& value) {
        node_t<T>* new_node = (node_t<T>*)heap_alloc(get_global_heap(), sizeof(node_t<T>));
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
            heap_free(get_global_heap(), node);
            node = next;
        }
        first_node = nullptr;
        size = 0;
    }

private:
    node_t<T>* first_node = nullptr;
    size_t size = 0;
};

template <typename T>
class dynamic_array {
public:
    dynamic_array() = default;

    dynamic_array(const T* p_arr, size_t arr_size) {
        resize(arr_size);
        size = arr_size;

        for (size_t i = 0; i < arr_size; ++i)
            new (&data[i]) T(p_arr[i]);
    }

    template <size_t arr_size>
    dynamic_array(const T (&p_arr)[arr_size]) {
        dynamic_array(p_arr, arr_size);
    }

    dynamic_array(const dynamic_array<T>& other) {
        size = other.size;
        capacity = other.capacity;

        data = (T*)heap_alloc(get_global_heap(), capacity * sizeof(T));
        for (size_t i = 0; i < size; ++i) {
            new (&data[i]) T(other.data[i]);
        }
    }

    dynamic_array(const T& data) {
        insert_back(data);
    }

    dynamic_array& operator=(const dynamic_array<T>& other) {
        if (this == &other)
            return *this;

        clear();
        if (capacity < other.size) {
            heap_free(get_global_heap(), data);
            capacity = other.capacity;
            data = (T*)heap_alloc(get_global_heap(), capacity * sizeof(T));
        }

        size = other.size;
        for (size_t i = 0; i < size; ++i) {
            new (&data[i]) T(other.data[i]);
        }

        return *this;
    }

    dynamic_array(dynamic_array<T>&& other) noexcept {
        data = other.data;
        size = other.size;
        capacity = other.capacity;

        other.data = nullptr;
        other.size = 0;
        other.capacity = 0;
    }

    dynamic_array& operator=(dynamic_array<T>&& other) noexcept {
        if (this == &other)
            return *this;

        clear();
        heap_free(get_global_heap(), data);

        data = other.data;
        size = other.size;
        capacity = other.capacity;

        other.data = nullptr;
        other.size = 0;
        other.capacity = 0;

        return *this;
    }

    ~dynamic_array() {
        clear();
        heap_free(get_global_heap(), data);
    }

    array_iterator<T> begin() {
        return array_iterator<T>(data);
    }

    array_iterator<T> end() {
        return array_iterator<T>(data + size);
    }

    array_iterator<T> find(const T& value) {
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
        if (index >= size)
            return false;

        data[index].~T();

        for (size_t i = index; i < size - 1; ++i) {
            new (&data[i]) T(move(data[i + 1]));
            data[i + 1].~T();
        }

        size--;
        return true;
    }

    bool insert_at(size_t index, T&& value) {
        if (index > size)
            return false;

        if (size == capacity)
            resize(capacity + 2);

        for (size_t i = size; i > index; --i) {
            new (&data[i]) T(move(data[i - 1]));
            data[i - 1].~T();
        }

        new (&data[index]) T(value);
        size++;
        return true;
    }

    bool insert_at(size_t index, const T& value) {
        return insert_at(index, T(value));
    }

    void insert_back(T&& value) {
        if (size == capacity)
            resize(capacity + 2);

        new (&data[size]) T(move(value));
        size++;
    }

    void insert_back(const T& value) {
        insert_back(T(value));
    }

    T* get_at(size_t index) {
        return &data[index];
    }

    const T* get_at(size_t index) const {
        return &data[index];
    }

    void clear() {
        for (size_t i = 0; i < size; ++i)
            data[i].~T();

        size = 0;
    }

    T* get_data() const {
        return data;
    }

    bool resize(size_t new_size) {
        if (new_size <= capacity)
            return false;

        T* new_data = (T*)heap_alloc(get_global_heap(), new_size * sizeof(T));
        if (!new_data)
            return false;

        memzero(new_data, new_size * sizeof(T));
        memcpy(new_data, data, capacity * sizeof(T));

        if (data)
            heap_free(get_global_heap(), data);

        data = new_data;
        capacity = new_size;

        return true;
    }

    void assign(const T* p_data, size_t item_count) {
        clear();
        resize(item_count);

        for (size_t i = 0; i < item_count; i++)
            new (&data[i]) T(p_data[i]);

        size = item_count;
    }

private:
    T* data;
    size_t size;
    size_t capacity;
};

template <typename T, size_t size>
class static_array {
public:
    static_array() = default;

    template <typename... Args>
    static_array(Args&&... args) {
        size_t i = 0;
        ( (i < size ? insert_at(i++, forward<Args>(args)) : false), ...);
    }

    static_array(const T (&p_arr)[size]) {
        for (size_t i = 0; i < size; i++)
            new (&data[i]) T(p_arr[i]);
    }

    static_array(const static_array& other) {
        for(size_t i = 0; i < size; i++)
            new (&data[i]) T(other.data[i]);
    }

    static_array(static_array&& other) noexcept {
        for(size_t i = 0; i < size; i++)
            new (&data[i]) T(move(other.data[i]));
    }

    static_array& operator=(const static_array& other) {
        if (this != &other) {
            clear();
            for(size_t i = 0; i < size; i++)
                new (&data[i]) T(other.data[i]);
        }
        return *this;
    }

    static_array& operator=(static_array&& other) noexcept {
        if (this != &other) {
            clear();
            for(size_t i = 0; i < size; i++)
                new (&data[i]) T(move(other.data[i]));
        }
        return *this;
    }

    ~static_array() {
        clear();
    }

    array_iterator<T> begin() {
        return array_iterator<T>(data);
    }

    array_iterator<T> end() {
        return array_iterator<T>(data + size);
    }

    array_iterator<T> find(const T& value) {
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

    bool insert_at(size_t index, T&& value) {
        if (index > size)
            return false;

        new (&data[index]) T(value);
        return true;
    }

    T* get_at(size_t index) {
        return &data[index];
    }

    const T* get_at(size_t index) const {
        return &data[index];
    }

    T* get_data() const {
        return data;
    }

    void clear() {
        for(size_t i = 0; i < size; i++)
            data[i].~T();
    }

private:
    T data[size];
};

#endif // __VECTOR_HPP__