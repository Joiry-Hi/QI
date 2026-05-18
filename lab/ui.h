/**
 * ui.h
 * HUD rendering: health/QI bars, skill bar, damage numbers, overlays
 */
#ifndef UI_H
#define UI_H

#include "game.h"

void UI_DrawPlayerHUD(const Entity* p);
void UI_DrawEnemyHealthBars();
void UI_DrawDamageNumbers();
void UI_DrawSkillBar(const Entity* p);
void UI_DrawWaveInfo(int wave, int score, float game_time);
void UI_DrawGameOver(int wave, int score, float game_time);

#endif
