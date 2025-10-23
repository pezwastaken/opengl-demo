#pragma once

#include <array>
#include "SDL3/SDL_scancode.h"

struct KeyHandler
{
    std::array<bool, 512> keys {0};
};

inline void set_key_pressed(KeyHandler& key_handler, SDL_Scancode key_code)
{
    key_handler.keys[key_code] = true;
}

inline void set_key_released(KeyHandler& key_handler, SDL_Scancode key_code)
{
    key_handler.keys[key_code] = false;
}

inline bool get_key_state(KeyHandler& key_handler, SDL_Scancode key_code)
{
    return key_handler.keys[key_code];
}
