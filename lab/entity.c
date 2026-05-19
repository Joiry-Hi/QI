/**
 * entity.c
 * 统一实体实现: 初始化/更新/受伤/施法/突破
 */
#include "entity.h"
#include "skill_config.h"
#include "combat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// --- 全局实体注册表 ---
Entity g_Entities[MAX_ENTITIES];
int g_EntityCount = 0;

// --- 外部引用(QI.c的技能数据库) ---
extern Skill g_skill_database[TOTAL_SKILLS];
extern int max_HP[TOTAL_XIUWEI_LEVEL];
extern int max_QI[TOTAL_XIUWEI_LEVEL];
extern int Yuan[TOTAL_XIUWEI_LEVEL];
extern char *Realm[TOTAL_XIUWEI_LEVEL];
extern char *Eng_Root_Names[TOTAL_ROOT_TYPES];
extern char *CHN_Root_Names[TOTAL_ROOT_TYPES];

// --- VFX池 - 由game.c定义, 这里extern引用 ---
#define MAX_VFX 64
extern SpellFX g_VFXPool[MAX_VFX];

// 辅助: 查找空闲VFX槽位
static SpellFX* VFX_GetFreeSlot() {
    for (int i = 0; i < MAX_VFX; i++) {
        if (!g_VFXPool[i].active)
            return &g_VFXPool[i];
    }
    return NULL;
}

// ============================================================
// Entity 初始化
// ============================================================
void Entity_Init(Entity* e, int id, const char* name, XIUWEI level, EntityFaction faction)
{
    memset(e, 0, sizeof(Entity));
    e->entity_id = id;
    e->faction = faction;
    strncpy(e->name, name, 31);
    e->is_alive = true;

    e->xiuwei = level;
    e->max_hp = max_HP[level];
    e->hp = e->max_hp;
    e->max_qi = max_QI[level];
    e->qi = 0;
    e->atk = Yuan[level];
    e->evade = 0.05f;
    e->gain_bonus = level + 1;
    e->move_speed = 200.0f + level * 20.0f; // 境界越高移速稍快
    e->radius = 18.0f;
    e->qi_regen_rate = 1; // 每秒回复1 QI

    // 随机灵根
    e->root = (SpiritualRootID)((rand() % (TOTAL_ROOT_TYPES - 1)) + 1);

    // 灵根修正
    if (e->root == ROOT_Solid) {
        e->hp = (int)(e->hp * 1.2f);
        e->max_hp = e->hp;
    }
    if (e->root == ROOT_Ethereal) {
        e->evade = 0.1f * level + 0.05f;
    }

    // 视觉初始化
    SDL_Color col;
    if (faction == ENTITY_PLAYER) {
        col = (SDL_Color){0, 255, 255, 255}; // 青色
    } else {
        col = (SDL_Color){255, 50, 50, 255}; // 红色
    }
    e->caster = Caster_Create(e->position.x, e->position.y, col);
    e->color = col;

    // 技能授予 - 遍历数据库, 给每个类别最高阶的可学技能
    for (int i = 0; i < TOTAL_ACTION_TYPES; i++) {
        e->learned_skills[i].skill_id = SKILL_ID_NONE;
    }
    for (int i = 0; i < TOTAL_SKILLS; i++) {
        const Skill* sk = &g_skill_database[i];
        if (sk->skill_id != SKILL_ID_NONE && sk->rank <= level) {
            ActionType cat = sk->action_type;
            if (e->learned_skills[cat].skill_id == SKILL_ID_NONE ||
                sk->rank > e->learned_skills[cat].rank) {
                e->learned_skills[cat] = *sk;
            }
        }
    }

    // 默认装备前6个可用技能到快捷键
    int slot = 0;
    for (int i = 0; i < TOTAL_ACTION_TYPES && slot < 9; i++) {
        if (e->learned_skills[i].skill_id != SKILL_ID_NONE) {
            e->equipped_skills[slot] = e->learned_skills[i].skill_id;
            slot++;
        }
    }
    e->selected_skill_slot = 0;

    Entity_Register(e);
}

int Entity_Register(Entity* e)
{
    if (g_EntityCount >= MAX_ENTITIES) return -1;
    e->entity_id = g_EntityCount;
    g_Entities[g_EntityCount++] = *e;
    return e->entity_id;
}

void Entity_Unregister(int id)
{
    if (id < 0 || id >= g_EntityCount) return;
    g_Entities[id] = g_Entities[g_EntityCount - 1];
    g_EntityCount--;
}

Entity* Entity_GetByID(int id)
{
    if (id < 0 || id >= g_EntityCount) return NULL;
    return &g_Entities[id];
}

void Entity_FindNearestEnemy(Entity* self, Entity** out_target, float* out_dist)
{
    *out_target = NULL;
    *out_dist = 99999.0f;
    for (int i = 0; i < g_EntityCount; i++) {
        Entity* other = &g_Entities[i];
        if (!other->is_alive) continue;
        if (other->faction == self->faction) continue; // 同阵营不攻击
        float dx = other->position.x - self->position.x;
        float dy = other->position.y - self->position.y;
        float dist = sqrtf(dx*dx + dy*dy);
        if (dist < *out_dist) {
            *out_dist = dist;
            *out_target = other;
        }
    }
}

// ============================================================
// Entity 每帧更新
// ============================================================
void Entity_Update(Entity* e, float dt)
{
    if (!e->is_alive) return;
    if (dt > 0.1f) dt = 0.1f; // 防止帧率暴跌导致数值异常

    // 1. 冷却计时
    for (int i = 0; i < TOTAL_SKILLS; i++) {
        if (e->cooldowns[i] > 0.0f) {
            e->cooldowns[i] -= dt;
            if (e->cooldowns[i] < 0.0f) e->cooldowns[i] = 0.0f;
        }
    }

    // 2. QI被动回复
    e->qi_timer += e->qi_regen_rate * dt;
    if (e->qi_timer >= 1.0f) {
        int add = (int)e->qi_timer;
        e->qi += add;
        e->qi_timer -= (float)add;
        if (e->qi > e->max_qi) e->qi = e->max_qi;
    }

    // 3. 状态计时
    if (e->invuln_timer > 0) e->invuln_timer -= dt;
    if (e->state_timer > 0) {
        e->state_timer -= dt;
        if (e->state_timer <= 0 && e->state != ENTITY_STATE_DEAD) {
            e->state = ENTITY_STATE_IDLE;
        }
    }
    if (e->break_fail_cooldown > 0) e->break_fail_cooldown -= dt;

    // 4. 流血DoT - 每秒bleeding点伤害
    if (e->bleeding > 0) {
        e->bleed_timer += dt;
        if (e->bleed_timer >= 1.0f) {
            e->hp -= e->bleeding;
            e->bleeding--;
            e->bleed_timer -= 1.0f;
        }
    } else {
        e->bleed_timer = 0;
    }

    // 5. 持续治疗 (每0.5秒一跳)
    if (e->healing > 0 && e->hp < e->max_hp) {
        e->hp += e->healing;
        if (e->hp > e->max_hp) e->hp = e->max_hp;
    }

    // 6. 激怒衰减 (每3秒衰减1点)
    if (e->enraged > 0) {
        e->rage_timer += dt;
        if (e->rage_timer >= 3.0f) {
            e->enraged--;
            e->rage_timer -= 3.0f;
        }
    } else {
        e->rage_timer = 0;
    }
    e->atk = Yuan[e->xiuwei] + e->enraged;

    // 7. 诅咒固定扣血
    if (e->cursed > 0) {
        e->curse_timer += dt;
        if (e->curse_timer >= 1.0f) {
            e->hp -= e->cursed;
            e->curse_timer -= 1.0f;
        }
    } else {
        e->curse_timer = 0;
    }

    // 8. 死亡检查
    if (e->hp <= 0) {
        e->hp = 0;
        e->is_alive = false;
        e->state = ENTITY_STATE_DEAD;
    }

    // 9. 边界钳制
    if (e->position.x < e->radius) e->position.x = e->radius;
    if (e->position.x > SCREEN_WIDTH - e->radius) e->position.x = SCREEN_WIDTH - e->radius;
    if (e->position.y < e->radius) e->position.y = e->radius;
    if (e->position.y > SCREEN_HEIGHT - e->radius) e->position.y = SCREEN_HEIGHT - e->radius;

    // 10. 同步视觉Caster位置
    e->caster.position = e->position;
    if (e->is_alive) {
        Caster_Update(&e->caster);
    }
}

// ============================================================
// 受伤
// ============================================================
void Entity_TakeDamage(Entity* e, int damage, int source_id, int skill_id)
{
    if (!e->is_alive) {
        if (e->entity_id == 0 && Combat_GetTracePlayerDamage()) {
            printf("[APPLY->HP] blocked=dead dmg=%d\n", damage);
        }
        return;
    }
    if (damage <= 0) {
        if (e->entity_id == 0 && Combat_GetTracePlayerDamage()) {
            printf("[APPLY->HP] blocked=zero_damage dmg=%d\n", damage);
        }
        return;
    }
    if (e->invuln_timer > 0) {
        if (e->entity_id == 0 && Combat_GetTracePlayerDamage()) {
            printf("[APPLY->HP] blocked=invuln timer=%.3f dmg=%d\n", e->invuln_timer, damage);
        }
        return;
    }

    // 防御性技能减伤逻辑 (如果正在使用护盾类技能)
    int final_dmg = damage;

    // TODO: 护盾减伤/反弹 (Phase 4整合)

    e->hp -= final_dmg;
    e->invuln_timer = 0.15f; // 0.15秒无敌帧, 防止多段伤害重复计算
    e->state = ENTITY_STATE_STUNNED;
    e->state_timer = 0.1f;

    // 打断治疗
    e->healing = 0;

    if (e->hp <= 0) {
        e->hp = 0;
        e->is_alive = false;
        e->state = ENTITY_STATE_DEAD;

        // 死亡爆发粒子
        for (int p = 0; p < 40; p++) {
            float a = (float)(rand() % 6283) / 1000.0f;
            float s = (float)(rand() % 500) / 100.0f + 2.0f;
            Vector2 v = {cosf(a) * s, sinf(a) * s};
            SDL_Color col = e->color;
            col.a = 200;
            Particle_Emit(e->position, v, col, 30, 2);
        }
        Engine_TriggerShake(8.0f);
    }

    if (e->entity_id == 0 && Combat_GetTracePlayerDamage()) {
        printf("[APPLY->HP] hp_after=%d dmg=%d src=%d skill=%d\n",
               e->hp, final_dmg, source_id, skill_id);
    }
}

// ============================================================
// 技能检查与施放
// ============================================================
bool Entity_CanCast(const Entity* e, SkillID id)
{
    if (id == SKILL_ID_NONE || id < 0 || id >= TOTAL_SKILLS) return false;
    if (!e->is_alive) return false;
    if (e->state == ENTITY_STATE_CASTING || e->state == ENTITY_STATE_BREAKING) return false;

    const Skill* sk = &g_skill_database[id];
    RealTimeSkillConfig* cfg = &g_rt_skill_config[id];

    if (e->qi < sk->cost) return false;
    if (e->cooldowns[id] > 0.0f) return false;

    return true;
}

void Entity_CastSkill(Entity* e, SkillID id, Vector2 target)
{
    if (!Entity_CanCast(e, id)) return;

    const Skill* sk = &g_skill_database[id];
    RealTimeSkillConfig* cfg = &g_rt_skill_config[id];

    // 扣除QI
    e->qi -= sk->cost;
    if (e->qi < 0) e->qi = 0;

    // 设置冷却
    e->cooldowns[id] = cfg->cooldown_seconds;

    // 计算方向
    Vector2 dir = Vec2_Sub(target, e->position);
    float len = sqrtf(dir.x*dir.x + dir.y*dir.y);
    if (len < 0.1f) { dir.x = 1.0f; dir.y = 0.0f; }
    else { dir.x /= len; dir.y /= len; }

    int damage = (int)(sk->base_power * e->atk * cfg->damage_scalar);

    // 根据Hitbox类型分派
    switch (cfg->hitbox_type) {
    case HITBOX_PROJECTILE: {
        int count = cfg->projectile_count;
        if (sk->type_id == TYPE_BURST && count <= 1) {
            // Burst类技能按QI分段数量
            count = e->qi / (sk->cost > 0 ? sk->cost : 1);
            if (count > MAX_SUB_FX) count = MAX_SUB_FX;
            if (count < 1) count = 1;
        }
        for (int i = 0; i < count; i++) {
            Vector2 spawn = e->position;
            Vector2 vel = {dir.x * cfg->projectile_speed, dir.y * cfg->projectile_speed};
            // 多投射物散布
            if (count > 1) {
                float spread = (float)(i - (count-1)/2.0f) * 0.3f;
                float cos_s = cosf(spread), sin_s = sinf(spread);
                vel.x = (dir.x * cos_s - dir.y * sin_s) * cfg->projectile_speed;
                vel.y = (dir.x * sin_s + dir.y * cos_s) * cfg->projectile_speed;
                float offset_dist = 25.0f;
                spawn.x += -dir.y * (i - (count-1)/2.0f) * offset_dist;
                spawn.y += dir.x * (i - (count-1)/2.0f) * offset_dist;
            }
            Hitbox* h = Hitbox_Spawn(e->entity_id, id, spawn, vel, cfg->hitbox_radius,
                                      damage, cfg->projectile_lifetime, cfg->hitbox_type);
            if (h && cfg->is_homing) {
                Entity* nearest = NULL;
                float nearest_dist = 0.0f;
                Entity_FindNearestEnemy(e, &nearest, &nearest_dist);
                if (nearest) {
                    h->target_entity_id = nearest->entity_id;
                    h->homing_strength = cfg->homing_strength;
                }
            }
        }
        break;
    }
    case HITBOX_SLASH: {
        Vector2 slash_pos = e->position;
        Hitbox* h = Hitbox_Spawn(e->entity_id, id, slash_pos, (Vector2){0,0},
                                  cfg->hitbox_radius, damage, 0.2f, HITBOX_SLASH);
        if (h) {
            h->arc_angle = cfg->slash_arc;
            h->arc_dir = dir;
        }
        break;
    }
    case HITBOX_AOE: {
        Vector2 aoe_pos = cfg->is_self_target ? e->position : target;
        Hitbox* h = Hitbox_Spawn(e->entity_id, id, aoe_pos, (Vector2){0,0},
                                  cfg->aoe_radius, damage, cfg->projectile_lifetime, HITBOX_AOE);
        if (h) {
            h->aoe_outer_radius = cfg->aoe_radius;
            h->aoe_inner_radius = 0;
            h->aoe_source_is_entity = cfg->is_self_target;
            h->aoe_source_entity_id = cfg->is_self_target ? e->entity_id : -1;
        }
        // AOE有施法延迟, 设置施法引导状态
        if (cfg->cast_time > 0) {
            e->state = ENTITY_STATE_CASTING;
            e->state_timer = cfg->cast_time;
        }
        break;
    }
    case HITBOX_BEAM: {
        // 即时射线
        int hit_ids[MAX_ENTITIES];
        int hit_count = 0;
        Vector2 beam_end = {e->position.x + dir.x * 800.0f, e->position.y + dir.y * 800.0f};
        Beam_Check(e->position, beam_end, e->entity_id, id, damage, hit_ids, &hit_count);
        break;
    }
    }

    // --- 自指向技能(治疗/增益/护盾) ---
    if (cfg->is_self_target && cfg->hitbox_type != HITBOX_AOE) {
        if (cfg->heal_amount_scalar > 0) {
            int heal = cfg->heal_amount_scalar * Yuan[e->xiuwei];
            e->hp += heal;
            if (e->hp > e->max_hp) e->hp = e->max_hp;
            e->healing = Yuan[e->xiuwei]; // 持续治疗
        }
        if (cfg->buff_enraged_amt > 0) {
            e->enraged += cfg->buff_enraged_amt;
        }
        if (cfg->qi_regen_bonus > 0) {
            e->qi_regen_rate += cfg->qi_regen_bonus;
        }
    }

    // --- 集气技能特殊处理 ---
    if (cfg->is_charge_skill) {
        e->qi += e->gain_bonus;
        if (e->qi > e->max_qi) e->qi = e->max_qi;
        // 连续集气增益
        e->gain_bonus++;
        int max_gain = 1 << e->xiuwei;
        if (e->gain_bonus > max_gain) e->gain_bonus = max_gain;
    }

    // --- 生成视觉特效 ---
    SpellFX* fx_slot = VFX_GetFreeSlot();
    if (fx_slot) {
        int duration = (int)(cfg->projectile_lifetime * 60.0f);
        if (duration < 15) duration = 15;
        if (duration > 300) duration = 300;
        int param = cfg->projectile_count > 0 ? cfg->projectile_count : 1;
        float visual_radius = cfg->hitbox_radius;
        float visual_arc = cfg->slash_arc;
        float visual_speed = cfg->projectile_speed;
        if (cfg->hitbox_type == HITBOX_AOE) {
            visual_radius = cfg->aoe_radius;
            param = cfg->projectile_count > 0 ? cfg->projectile_count : 1;
        }
        if (cfg->hitbox_type == HITBOX_SLASH) {
            param = (int)(cfg->hitbox_radius / 50.0f);
            if (param < 1) param = 1;
        }

        // AOE技能VFX位置使用实际AOE中心
        Vector2 vfx_pos = e->position;
        if (cfg->hitbox_type == HITBOX_AOE && !cfg->is_self_target) vfx_pos = target;

        SpellFX_Cast(fx_slot, id, vfx_pos, dir, duration, param);
        SpellFX_SetCombatShape(fx_slot, visual_radius, visual_arc, visual_speed);
    }

    // 重置非MELEE/SMITE的连击计数
    if (sk->action_type != ACTION_TYPE_MELEE && sk->action_type != ACTION_TYPE_SMITE) {
        e->combo = 0;
    } else {
        e->combo++;
    }

    // 非集气技能重置集气增益
    if (!cfg->is_charge_skill) {
        e->gain_bonus = e->xiuwei + 1;
    }
}

// ============================================================
// 战斗中突破系统
// ============================================================
void Entity_TryBreakthrough(Entity* e)
{
    if (!e->is_alive) return;
    if (e->xiuwei >= TOTAL_XIUWEI_LEVEL - 1) return; // 已飞升
    if (e->qi < e->max_qi) return;
    if (e->break_fail_cooldown > 0) return;

    // 开始突破引导
    if (e->state != ENTITY_STATE_BREAKING) {
        e->state = ENTITY_STATE_BREAKING;
        e->state_timer = 0.8f; // 0.8秒引导
        return;
    }

    // 引导完成, 进行突破判定
    float chance = (e->root == ROOT_Heavenly) ? 90.0f : 90.0f * expf(-e->xiuwei / 2.0f);
    if ((rand() % 100) < chance) {
        // 突破成功!
        e->xiuwei++;
        e->qi = 0;
        e->max_qi = max_QI[e->xiuwei];
        e->max_hp = max_HP[e->xiuwei];
        e->hp = e->max_hp; // 满血
        e->atk = Yuan[e->xiuwei];
        e->gain_bonus = e->xiuwei + 1;
        e->enraged = 0;
        e->healing = 0;
        e->cursed = 0;
        e->combo = 0;
        if (e->root == ROOT_Solid) {
            e->hp = (int)(e->hp * 1.2f);
            e->max_hp = e->hp;
        }
        e->evade = (e->root == ROOT_Ethereal) ? 0.1f * e->xiuwei : 0.05f * e->xiuwei;

        // 刷新技能
        for (int i = 0; i < TOTAL_ACTION_TYPES; i++) e->learned_skills[i].skill_id = SKILL_ID_NONE;
        for (int i = 0; i < TOTAL_SKILLS; i++) {
            const Skill* sk = &g_skill_database[i];
            if (sk->skill_id != SKILL_ID_NONE && sk->rank <= e->xiuwei) {
                ActionType cat = sk->action_type;
                if (e->learned_skills[cat].skill_id == SKILL_ID_NONE ||
                    sk->rank > e->learned_skills[cat].rank) {
                    e->learned_skills[cat] = *sk;
                }
            }
        }

        // 突破特效: 金光爆发
        for (int p = 0; p < 60; p++) {
            float a = (float)(rand() % 6283) / 1000.0f;
            float s = 3.0f + (float)(rand() % 800) / 100.0f;
            Vector2 v = {cosf(a) * s, sinf(a) * s};
            SDL_Color gold = {255, 255, 100, 255};
            Particle_Emit(e->position, v, gold, 50, 3);
        }
        Engine_TriggerShake(12.0f);
        printf("[突破!] %s 晋升至 %s!\n", e->name, Realm[e->xiuwei]);

        // 检查是否飞升
        if (e->xiuwei >= TOTAL_XIUWEI_LEVEL - 1) {
            e->hp = 999999999;
            e->max_hp = 999999999;
            e->qi = 999999999;
            e->max_qi = 999999999;
            e->atk = 999999999;
            printf("[飞升!] %s 已踏入真仙之境!\n", e->name);
        }
    } else {
        // 突破失败
        e->qi = e->max_qi * 3 / 4;
        e->break_fail_cooldown = 1.0f;
        printf("[失败] %s 突破失败!\n", e->name);
    }
    e->state = ENTITY_STATE_IDLE;
}
