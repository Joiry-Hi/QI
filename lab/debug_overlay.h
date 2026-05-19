#ifndef DEBUG_OVERLAY_H
#define DEBUG_OVERLAY_H

#include "vfx.h"
#include <stdbool.h>

void DebugOverlay_SetEnabled(bool enabled);
void DebugOverlay_Toggle();
bool DebugOverlay_IsEnabled();

void DebugOverlay_DrawAction();
void DebugOverlay_DrawSandbox(const Caster* player, const Caster* dummies, int dummy_count);

#endif
