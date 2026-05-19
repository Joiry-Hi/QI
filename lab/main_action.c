/**
 * main_action.c
 * QI-Anima: 俯视2D动作对战游戏 入口点
 *
 * 编译:
 * gcc -o qi_action main_action.c engine.c vfx.c QI.c entity.c combat.c \
 *     skill_config.c game.c ui.c ai.c -lSDL2 -lm -DQI_LIBRARY -O2
 */
#include "engine.h"
#include "game.h"
#include "input_state.h"
#include "debug_overlay.h"
#include <stdio.h>
#include <stdbool.h>

int main(int argc, char* argv[])
{
    if (!Engine_Init("QI-Anima: Cultivation Arena")) {
        fprintf(stderr, "Failed to initialize engine.\n");
        return 1;
    }

    // SDL_StopTextInput(); // 已在 Engine_Init 中调用

    Game_Init();

    bool running = true;
    SDL_Event event;
    InputState input;
    Uint32 last_time = SDL_GetTicks();

    printf("=== QI-Anima: Cultivation Arena ===\n");
    printf("[WASD] Move  [Mouse] Aim  [Left Click] Cast Selected Skill\n");
    printf("[Right Click] Defend  [Space] Dash  [Q] Gain QI\n");
    printf("[1-9] Select Skill  [Scroll] Cycle Skills  [Esc] Quit\n");
    printf("====================================\n");

    while (running) {
        InputState_BeginFrame(&input);

        Uint32 now = SDL_GetTicks();
        float dt = (now - last_time) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f; // cap
        last_time = now;

        // --- 事件处理 ---
        while (SDL_PollEvent(&event)) {
            InputState_HandleEvent(&input, &event);
        }

        if (input.quit_requested || input.key_escape_pressed) running = false;
        if (g_Game.is_game_over && input.key_enter_pressed) Game_Init();
        if (input.key_f3_pressed) {
            DebugOverlay_Toggle();
            Combat_SetTracePlayerDamage(DebugOverlay_IsEnabled());
            printf("[Debug Overlay] %s\n", DebugOverlay_IsEnabled() ? "ON" : "OFF");
        }

        const Uint8* keystate = SDL_GetKeyboardState(NULL);

        // --- 力场清空 ---
        ForceGrid_Clear();
        DamageEvents_Clear();

        // --- 输入处理 ---
        Game_ProcessInput(&input, keystate, dt);

        // --- 游戏更新 ---
        Game_Update(dt);

        // --- 渲染 ---
        Game_Render();

        SDL_Delay(8); // ~120fps cap, 实际约60fps
    }

    Engine_Cleanup();
    return 0;
}
