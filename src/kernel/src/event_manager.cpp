#include "event_manager.hpp"

bool add_handler(const char* event_name, event_handler_t handler) {
    const auto event_name_hash = hash_fnv1a_64(event_name);

    event_t* event_ptr = nullptr;

    for (auto& event : events) {
        if (event.id == event_name_hash) {
            event_ptr = (event_t*)&event;
            break;
        }
    }

    if (!event_ptr)
        return false;

    if (event_ptr->current_size + 1 >= EVENT_HANDLER_COUNT)
        return false;

    event_ptr->array[event_ptr->current_size] = handler;
    event_ptr->current_size++;

    return true;
}

bool fire_event(const char* event_name) {
    const auto event_name_hash = hash_fnv1a_64(event_name);

    event_t* event_ptr = nullptr;

    for (auto& event : events) {
        if (event.id == event_name_hash) {
            event_ptr = (event_t*)&event;
            break;
        }
    }

    if (!event_ptr)
        return false;

    for (size_t i = 0; i < event_ptr->current_size; i++)
        event_ptr->array[i]();
    
    return true;
}