//==========================================
/// @file       event_manager.hpp
/// @brief      kernel event handler
//==========================================

#pragma once

#ifndef __EVENT_MANAGER_HPP__
#define __EVENT_MANAGER_HPP__

#define EVENT_HANDLER_COUNT 30

typedef bool(*event_handler_t)(void);

#include "common.hpp"

class event_t {
public:
    event_t(char* name) : id(hash_fnv1a_64(name)) {}
    event_t(uint64_t id) : id(id) {}

    size_t current_size = 0;
    event_handler_t array[EVENT_HANDLER_COUNT] = {};
    uint64_t id;
};

static const event_t events[] = {
    event_t((char*)"TEST_EVENT"),
};

bool add_handler(const char* event_name, event_handler_t handler);
bool fire_event(const char* event_name);

#endif // __EVENT_MANAGER_HPP__