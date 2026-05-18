#ifndef ENGINE_H
#define ENGINE_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <math.h>

// --- 1. 基础类型 (原 qi_types.h) ---
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

typedef struct { float x, y; } Vector2;
static inline Vector2 Vec2_Add(Vector2 a, Vector2 b) { return (Vector2){a.x + b.x, a.y + b.y}; }
static inline Vector2 Vec2_Sub(Vector2 a, Vector2 b) { return (Vector2){a.x - b.x, a.y - b.y}; }

typedef enum {
    TYPE_GENERIC = 0, TYPE_FIRE, TYPE_WATER, 
    TYPE_SWORD_STEEL, TYPE_SHIELD_ENERGY, TYPE_EARTH, TYPE_VOID 
} ParticleType;

// engine.h 中的 Particle 结构体更新
typedef struct {
    Vector2 position;
    Vector2 prev_position;
    Vector2 velocity;
    Vector2 acceleration;
    SDL_Color color;
    float life;
    float decay_rate;
    float mass;
    float drag;
    float gravity;
    int size;
    ParticleType type;
} Particle;

// --- 2. 引擎核心接口 ---
bool Engine_Init(const char* title);
void Engine_Cleanup();
void Engine_Clear();
void Engine_Present();
SDL_Renderer* Engine_GetRenderer();
SDL_Window* Engine_GetWindow(); 
void Engine_TriggerShake(float intensity);

// --- 3. 粒子系统接口 ---
#define MAX_PARTICLES 10000
Particle* Particle_Emit(Vector2 pos, Vector2 vel, SDL_Color color, int lifetime, int size);
void Particle_UpdateAll();
void Particle_RenderAll();

// --- 4. 力场网格接口 (原 force_grid.h) ---
#define CELL_SIZE 20
#define GRID_W (SCREEN_WIDTH / CELL_SIZE)
#define GRID_H (SCREEN_HEIGHT / CELL_SIZE)

// 将力场视为引擎内置的物理环境，不再独立
void ForceGrid_Clear();
void ForceGrid_AddRadialForce(Vector2 pos, float radius, float strength);
void ForceGrid_AddDirectionalForce(Vector2 pos, float radius, Vector2 force);
Vector2 ForceGrid_GetForceAt(Vector2 pos);

#endif // ENGINE_H