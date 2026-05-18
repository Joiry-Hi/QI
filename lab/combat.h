/**
 * combat.h
 * 实时碰撞检测系统: Hitbox投射物池 + 空间哈希网格 + 伤害事件
 */
#ifndef COMBAT_H
#define COMBAT_H

#include "entity.h"

#define MAX_HITBOXES 256
#define COLLISION_CELL_SIZE 64
#define GRID_COLS ((SCREEN_WIDTH / COLLISION_CELL_SIZE) + 2)
#define GRID_ROWS ((SCREEN_HEIGHT / COLLISION_CELL_SIZE) + 2)
#define MAX_EVENTS_PER_FRAME 64

typedef enum {
    HITBOX_PROJECTILE,
    HITBOX_SLASH,
    HITBOX_AOE,
    HITBOX_BEAM
} HitboxType;

typedef struct {
    HitboxType type;
    int source_entity_id;
    int skill_id;
    AttributeID attribute;
    Vector2 position;
    Vector2 velocity;
    float radius;
    int damage;
    float lifetime;
    float max_lifetime;
    bool has_hit;
    int pierce_count;
    int target_entity_id;      // for homing
    float homing_strength;

    // AOE
    float aoe_inner_radius;
    float aoe_outer_radius;
    bool aoe_source_is_entity; // true if AOE follows the caster
    int aoe_source_entity_id;

    // Slash
    float arc_angle;
    Vector2 arc_dir;

    // Beam: already processed (instant), just for visual
    bool beam_hit_ids[MAX_ENTITIES];

    // Visual
    SpellFX* vfx;
} Hitbox;

typedef struct {
    int target_entity_id;
    int source_entity_id;
    int skill_id;
    int raw_damage;
    int final_damage;
    ResultType result;
    Vector2 knockback;
} DamageEvent;

// --- Hitbox pool ---
extern Hitbox g_Hitboxes[MAX_HITBOXES];
extern int g_HitboxCount;
extern DamageEvent g_DamageEvents[MAX_EVENTS_PER_FRAME];
extern int g_DamageEventCount;

Hitbox* Hitbox_Spawn(int source_id, SkillID skill_id, Vector2 pos, Vector2 vel,
                     float radius, int damage, float lifetime, HitboxType type);
void Hitbox_Update(Hitbox* h, float dt);
void Hitbox_UpdateAll(float dt);
void Hitbox_Destroy(Hitbox* h);

// --- Collision Grid ---
void CollisionGrid_Clear();
void CollisionGrid_InsertEntity(int entity_id, Vector2 pos, float radius);
void CollisionGrid_InsertHitbox(int hitbox_idx);
void CollisionGrid_Resolve();

// --- Beam check (instant raycast) ---
void Beam_Check(Vector2 from, Vector2 to, int source_id, SkillID skill_id,
                int damage, int* out_hit_ids, int* out_count);

#endif
