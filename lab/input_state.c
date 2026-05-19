#include "input_state.h"
#include <string.h>

void InputState_BeginFrame(InputState* in)
{
    memset(in, 0, sizeof(*in));
}

void InputState_HandleEvent(InputState* in, const SDL_Event* event)
{
    switch (event->type) {
    case SDL_QUIT:
        in->quit_requested = true;
        break;
    case SDL_MOUSEWHEEL:
        if (event->wheel.y > 0) in->wheel_delta--;
        else if (event->wheel.y < 0) in->wheel_delta++;
        break;
    case SDL_KEYDOWN:
        if (event->key.repeat) break;
        if (event->key.keysym.sym >= SDLK_1 && event->key.keysym.sym <= SDLK_9) {
            int idx = (int)(event->key.keysym.sym - SDLK_1);
            if (idx >= 0 && idx < 9) in->key_1_to_9_pressed[idx] = true;
        }
        if (event->key.keysym.sym == SDLK_q) in->key_q_pressed = true;
        if (event->key.keysym.sym == SDLK_SPACE) in->key_space_pressed = true;
        if (event->key.keysym.sym == SDLK_ESCAPE) in->key_escape_pressed = true;
        if (event->key.keysym.sym == SDLK_RETURN) in->key_enter_pressed = true;
        if (event->key.keysym.sym == SDLK_F3) in->key_f3_pressed = true;
        break;
    default:
        break;
    }
}
