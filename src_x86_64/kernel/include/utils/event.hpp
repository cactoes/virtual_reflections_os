//==========================================
/// @file       event.hpp
/// @brief      event system
//==========================================

#pragma once

#ifndef __UTILS_EVENT_HPP__
#define __UTILS_EVENT_HPP__

#include "common.hpp"
#include "utils/vector.hpp"
#include "utils/mutex.hpp"

template <typename _argument>
class event_manager_t {
public:
    event_manager_t() {
        mutex_init(&mutex);
    }
    
    typedef void (*handler_t)(_argument);

    void fire_event(_argument arg) {
        mutex_lock_guard guard(&mutex);
        for (auto& handler : handlers)
            handler(arg);
    }

    void subscribe(handler_t p_handler) {
        mutex_lock_guard guard(&mutex);
        handlers.insert_back(p_handler);
    }

private:
    linked_list<handler_t> handlers {};
    mutex_t mutex;
};

#endif // __UTILS_EVENT_HPP__