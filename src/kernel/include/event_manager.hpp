//==========================================
/// @file       event_manager.hpp
/// @brief      kernel event handler
//==========================================

#pragma once

#ifndef __EVENT_MANAGER_HPP__
#define __EVENT_MANAGER_HPP__

#include "common.hpp"
#include "vector.hpp"
#include "mutex.hpp"

template <typename _argument>
class event_manager_t {
public:
    event_manager_t() {
        mutex_init(&mutex);
    };
    
    typedef void (*handler_t)(_argument);

    void fire_event(_argument arg) {
        mutex_lock_guard guard(&mutex);
        for (VECTOR_LOOP((&handlers), handler_node))
            handler_node->value(arg);
    }

    void add_handler(handler_t handler) {
        mutex_lock_guard guard(&mutex);
        handlers.insert_back(handler);
    }

private:
    vector<handler_t> handlers {};
    mutex_t mutex;
};

#endif // __EVENT_MANAGER_HPP__