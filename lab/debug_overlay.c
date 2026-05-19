#include "debug_overlay.h"
#include "engine.h"

static bool g_DebugOverlayEnabled = false;

void DebugOverlay_SetEnabled(bool enabled)
{
    g_DebugOverlayEnabled = enabled;
}

void DebugOverlay_Toggle()
{
    g_DebugOverlayEnabled = !g_DebugOverlayEnabled;
}

bool DebugOverlay_IsEnabled()
{
    return g_DebugOverlayEnabled;
}

void DebugOverlay_DrawSandbox(const Caster* player, const Caster* dummies, int dummy_count)
{
    if (!g_DebugOverlayEnabled) return;

    if (player) {
        Engine_DrawCircleOutline((int)player->position.x, (int)player->position.y,
                                 18, (SDL_Color){0, 255, 255, 220});
    }
    for (int i = 0; i < dummy_count; i++) {
        Engine_DrawCircleOutline((int)dummies[i].position.x, (int)dummies[i].position.y,
                                 18, (SDL_Color){255, 120, 40, 220});
    }
    Engine_DrawFillRect(12, 78, 120, 8, (SDL_Color){0, 255, 255, 180});
    Engine_DrawFillRect(12, 90, dummy_count * 24, 8, (SDL_Color){255, 120, 40, 180});
}
