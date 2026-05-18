/**
 * entity.h
 * 统一实体: 物理体(Caster) + 战斗属性(Player) + 实时状态
 */
#ifndef ENTITY_H
#define ENTITY_H

#include "vfx.h"
#include <stdbool.h>

#define MAX_ENTITIES 32
#define MAX_ENEMIES 24

typedef enum {
    ENTITY_PLAYER,
    ENTITY_ENEMY,
    ENTITY_NEUTRAL
} EntityFaction;

typedef enum {
    ENTITY_STATE_IDLE,
    ENTITY_STATE_CASTING,    // 引导施法中
    ENTITY_STATE_BREAKING,   // 突破引导中
    ENTITY_STATE_STUNNED,    // 受击硬直
    ENTITY_STATE_DEAD
} EntityState;

typedef struct {
    // --- Physics ---
    Vector2 position;
    Vector2 velocity;
    float radius;
    float move_speed;
    float dash_cooldown;

    // --- Combat ---
    int hp, max_hp;
    int qi, max_qi;
    int atk;
    float evade;
    XIUWEI xiuwei;
    SpiritualRootID root;
    int gain_bonus;
    int burst_count;

    // --- Status effects ---
    int bleeding;
    int cursed;
    int enraged;
    int healing;
    float invuln_timer;
    float bleed_timer;
    float rage_timer;
    float curse_timer;
    float qi_timer;
    int combo;

    // --- Visual ---
    Caster caster;
    SDL_Color color;

    // --- Skills ---
    Skill learned_skills[TOTAL_ACTION_TYPES];
    float cooldowns[TOTAL_SKILLS];
    int qi_regen_rate;
    int selected_skill_slot;
    SkillID equipped_skills[9];

    // --- Breakthrough ---
    float break_guide_timer;   // 突破引导计时
    float break_fail_cooldown; // 突破失败冷却

    // --- AI ---
    int ai_personality;   // 0-5 对应原版AI人格
    float ai_think_timer; // AI决策间隔

    // --- Identity ---
    int entity_id;
    EntityFaction faction;
    EntityState state;
    float state_timer;
    char name[32];
    bool is_alive;
} Entity;

// 全局实体注册表
extern Entity g_Entities[MAX_ENTITIES];
extern int g_EntityCount;

// --- Entity API ---
void Entity_Init(Entity* e, int id, const char* name, XIUWEI level, EntityFaction faction);
void Entity_Update(Entity* e, float dt);
void Entity_TakeDamage(Entity* e, int damage, int source_id, int skill_id);
bool Entity_CanCast(const Entity* e, SkillID id);
void Entity_CastSkill(Entity* e, SkillID id, Vector2 target);
int  Entity_Register(Entity* e);
void Entity_Unregister(int id);
Entity* Entity_GetByID(int id);
void Entity_FindNearestEnemy(Entity* self, Entity** out_target, float* out_dist);

// 突破
void Entity_TryBreakthrough(Entity* e);

#endif
