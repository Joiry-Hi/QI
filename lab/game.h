/**
 * game.h
 * 主游戏状态和循环
 */
#ifndef GAME_H
#define GAME_H

#include "entity.h"
#include "combat.h"
#include "vfx.h"

#define MAX_VFX 64
#define MAX_DAMAGE_NUMBERS 40

// 浮动伤害数字
typedef struct {
    Vector2 position;
    int damage;
    float lifetime;
    float max_lifetime;
    SDL_Color color;
    bool active;
} DamageNumber;

typedef struct {
    Entity player;
    int wave_number;
    int score;
    int enemies_remaining;
    float game_time;
    float wave_rest_timer;
    bool is_paused;
    bool is_game_over;
    bool wave_cleared;
} GameState;

extern GameState g_Game;
extern SpellFX g_VFXPool[MAX_VFX];
extern DamageNumber g_DamageNumbers[MAX_DAMAGE_NUMBERS];

void Game_Init();
void Game_Update(float dt);
void Game_Render();
void Game_ProcessInput(SDL_Event* event, const Uint8* keystate, float dt);
void Game_SpawnWave(int wave);
void Game_AddDamageNumber(Vector2 pos, int dmg, SDL_Color color);

#endif
