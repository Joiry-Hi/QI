#ifndef INPUT_STATE_H
#define INPUT_STATE_H

#include <stdbool.h>
#include <SDL2/SDL.h>

typedef struct {
    bool key_1_to_9_pressed[9];
    bool key_q_pressed;
    bool key_space_pressed;
    bool key_escape_pressed;
    bool key_enter_pressed;
    bool key_f3_pressed;

    int wheel_delta;
    bool quit_requested;
} InputState;

void InputState_BeginFrame(InputState* in);
void InputState_HandleEvent(InputState* in, const SDL_Event* event);

#endif
