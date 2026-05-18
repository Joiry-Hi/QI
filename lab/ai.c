/**
 * ai.c
 * Enemy AI: FSM state machine + threat assessment + personality-driven behavior
 */
#include "ai.h"
#include "combat.h"
#include "skill_config.h"
#include <math.h>
#include <stdlib.h>

// AI personalities from original QI game
// 0=BREAKER, 1=BERSERKER, 2=TURTLE, 3=ASCETIC, 4=GAMBLER, 5=RANDOM

static float AI_PreferredDist(int personality)
{
    switch (personality) {
        case 1: return 150.0f; // BERSERKER: close range
        case 0: return 250.0f; // BREAKER: mid range
        case 2: return 400.0f; // TURTLE: long range
        case 3: return 350.0f; // ASCETIC: mid-long
        case 4: return 200.0f; // GAMBLER: varies, but mid
        default: return 300.0f;
    }
}

static float AI_Aggression(int personality)
{
    switch (personality) {
        case 1: return 1.0f;  // BERSERKER: max aggression
        case 0: return 0.8f;
        case 4: return 0.7f;
        case 3: return 0.3f;
        case 2: return 0.4f;
        default: return 0.5f;
    }
}

static bool AI_PrefersDefense(int personality)
{
    return personality == 2; // TURTLE
}

static bool AI_PrefersQiGathering(int personality)
{
    return personality == 3; // ASCETIC
}

static bool AI_RiskySkills(int personality)
{
    return personality == 4; // GAMBLER
}

// Check if there are any hostile projectiles heading toward this entity
static bool IsUnderThreat(Entity* e)
{
    for (int i = 0; i < g_HitboxCount; i++) {
        Hitbox* h = &g_Hitboxes[i];
        // Only check projectiles from player (entity 0) or other enemies
        if (h->source_entity_id == e->entity_id) continue;
        if (h->type != HITBOX_PROJECTILE) continue;
        if (h->has_hit) continue;

        float dx = h->position.x - e->position.x;
        float dy = h->position.y - e->position.y;
        float dist = sqrtf(dx*dx + dy*dy);
        float threat_range = e->radius + h->radius + 80.0f; // 80px danger zone

        if (dist < threat_range) {
            // Check if moving toward us
            float vel_dot = (h->velocity.x * dx + h->velocity.y * dy);
            if (vel_dot < 0) { // moving closer
                return true;
            }
        }
    }
    return false;
}

// Get dodge direction perpendicular to nearest threat
static Vector2 GetDodgeDirection(Entity* e)
{
    Vector2 dodge = {0, 0};
    for (int i = 0; i < g_HitboxCount; i++) {
        Hitbox* h = &g_Hitboxes[i];
        if (h->source_entity_id == e->entity_id) continue;
        if (h->type != HITBOX_PROJECTILE) continue;
        if (h->has_hit) continue;

        float dx = h->position.x - e->position.x;
        float dy = h->position.y - e->position.y;
        float dist = sqrtf(dx*dx + dy*dy);
        float threat_range = e->radius + h->radius + 100.0f;

        if (dist < threat_range && dist > 0.01f) {
            // Perpendicular direction (rotate 90 degrees)
            float nx = dx / dist;
            float ny = dy / dist;
            // Pick perpendicular: cross with sign based on hash
            float sign = (e->entity_id % 2 == 0) ? 1.0f : -1.0f;
            dodge.x += -ny * sign * (1.0f - dist / threat_range) * 400.0f;
            dodge.y += nx * sign * (1.0f - dist / threat_range) * 400.0f;
        }
    }
    float dlen = sqrtf(dodge.x*dodge.x + dodge.y*dodge.y);
    if (dlen > 0.01f) {
        dodge.x /= dlen; dodge.y /= dlen;
        // scale to move_speed
        dodge.x *= e->move_speed;
        dodge.y *= e->move_speed;
    }
    return dodge;
}

// Select best skill for this personality and situation
static SkillID AI_SelectSkill(Entity* e, Entity* player, float dist_to_player)
{
    // Gather affordable skills
    SkillID affordable[TOTAL_SKILLS];
    int count = 0;

    for (int a = 0; a < TOTAL_ACTION_TYPES; a++) {
        SkillID sid = e->learned_skills[a].skill_id;
        if (sid == SKILL_ID_NONE) continue;
        if (!Entity_CanCast(e, sid)) continue;

        RealTimeSkillConfig* cfg = &g_rt_skill_config[sid];

        // Personality filters
        if (AI_PrefersDefense(e->ai_personality)) {
            // Prefer defense / shield skills
            if (cfg->is_self_target && cfg->heal_amount_scalar == 0 && cfg->buff_enraged_amt == 0)
                affordable[count++] = sid;
        }

        if (AI_RiskySkills(e->ai_personality)) {
            // Prefer high damage, long cooldown skills
            if (cfg->cooldown_seconds > 5.0f && cfg->damage_scalar > 10.0f)
                affordable[count++] = sid;
        }

        // Range check
        if (cfg->hitbox_type == HITBOX_SLASH) {
            if (dist_to_player > cfg->hitbox_radius + 100.0f) continue;
        }
        if (cfg->hitbox_type == HITBOX_PROJECTILE) {
            if (dist_to_player > 700.0f) continue;
        }

        affordable[count++] = sid;
    }

    // If nothing selected by filters, take anything affordable
    if (count == 0) {
        for (int a = 0; a < TOTAL_ACTION_TYPES; a++) {
            SkillID sid = e->learned_skills[a].skill_id;
            if (sid == SKILL_ID_NONE) continue;
            if (!Entity_CanCast(e, sid)) continue;
            affordable[count++] = sid;
        }
    }

    if (count == 0) return SKILL_ID_NONE;

    // Weighted random: higher damage_scalar → higher weight for aggressive personalities
    int total_weight = 0;
    int weights[TOTAL_SKILLS];
    for (int i = 0; i < count; i++) {
        RealTimeSkillConfig* cfg = &g_rt_skill_config[affordable[i]];
        weights[i] = 10 + (int)(cfg->damage_scalar * AI_Aggression(e->ai_personality));
        if (cfg->is_self_target) weights[i] = 15; // self-target always decent
        if (weights[i] < 1) weights[i] = 1;
        total_weight += weights[i];
    }

    int roll = rand() % total_weight;
    int acc = 0;
    for (int i = 0; i < count; i++) {
        acc += weights[i];
        if (roll < acc) return affordable[i];
    }
    return affordable[0];
}

void AI_Update(Entity* e, Entity* player, float dt)
{
    if (!e->is_alive || !player->is_alive) return;
    if (e->faction != ENTITY_ENEMY) return;

    // Don't act while casting or stunned
    if (e->state == ENTITY_STATE_CASTING || e->state == ENTITY_STATE_BREAKING ||
        e->state == ENTITY_STATE_STUNNED) {
        e->velocity = (Vector2){0, 0};
        return;
    }

    e->ai_think_timer -= dt;
    if (e->ai_think_timer > 0) return;

    // Think interval based on personality
    e->ai_think_timer = 0.25f + (float)(rand() % 100) / 400.0f; // 0.25-0.5s

    float dx = player->position.x - e->position.x;
    float dy = player->position.y - e->position.y;
    float dist_to_player = sqrtf(dx*dx + dy*dy);
    float preferred_dist = AI_PreferredDist(e->ai_personality);

    // --- Decision making ---

    // 1. Under threat? → DODGE
    if (IsUnderThreat(e) && e->invuln_timer <= 0) {
        Vector2 dodge = GetDodgeDirection(e);
        if (dodge.x != 0 || dodge.y != 0) {
            e->velocity = dodge;
            return;
        }
    }

    // 2. Low HP and has heal skill? → FLEE and heal
    float hp_pct = (float)e->hp / e->max_hp;
    if (hp_pct < 0.25f) {
        SkillID heal_id = e->learned_skills[ACTION_TYPE_HEAL].skill_id;
        if (heal_id != SKILL_ID_NONE && Entity_CanCast(e, heal_id)) {
            Entity_CastSkill(e, heal_id, e->position);
            // Move away
            if (dist_to_player > 0.01f) {
                e->velocity.x = -(dx / dist_to_player) * e->move_speed * 0.7f;
                e->velocity.y = -(dy / dist_to_player) * e->move_speed * 0.7f;
            }
            return;
        }
    }

    // 3. Low QI → GATHER_QI (personality dependent)
    float qi_pct = (float)e->qi / e->max_qi;
    if (qi_pct < 0.3f || AI_PrefersQiGathering(e->ai_personality)) {
        SkillID gather_id = SKILL_ID_GAIN_QI;
        if (Entity_CanCast(e, gather_id) && qi_pct < 0.7f) {
            Entity_CastSkill(e, gather_id, e->position);
            e->velocity = (Vector2){0, 0};
            return;
        }
    }

    // 4. Defense buff if available and under threat
    if (AI_PrefersDefense(e->ai_personality) && IsUnderThreat(e)) {
        SkillID def_id = e->learned_skills[ACTION_TYPE_DEFEND].skill_id;
        if (def_id != SKILL_ID_NONE && Entity_CanCast(e, def_id)) {
            Entity_CastSkill(e, def_id, e->position);
        }
    }

    // 5. Move toward/away from player based on preferred distance
    if (dist_to_player > 0.01f) {
        float dir_x = dx / dist_to_player;
        float dir_y = dy / dist_to_player;

        if (dist_to_player > preferred_dist + 50.0f) {
            // Move closer
            e->velocity.x = dir_x * e->move_speed;
            e->velocity.y = dir_y * e->move_speed;
        } else if (dist_to_player < preferred_dist - 30.0f) {
            // Move away
            e->velocity.x = -dir_x * e->move_speed * 0.6f;
            e->velocity.y = -dir_y * e->move_speed * 0.6f;
        } else {
            // Strafe at preferred distance
            float sign = (e->entity_id % 2 == 0) ? 1.0f : -1.0f;
            e->velocity.x = -dir_y * e->move_speed * 0.4f * sign;
            e->velocity.y = dir_x * e->move_speed * 0.4f * sign;
        }
    }

    // 6. Attack check (only when in range and not fleeing)
    float attack_chance = AI_Aggression(e->ai_personality) * 0.6f;
    if (dist_to_player < 600.0f && (float)rand() / RAND_MAX < attack_chance) {
        SkillID chosen = AI_SelectSkill(e, player, dist_to_player);
        if (chosen != SKILL_ID_NONE) {
            Entity_CastSkill(e, chosen, player->position);
        }
    }

    // Breakthrough check
    if (e->qi >= e->max_qi && e->xiuwei < TOTAL_XIUWEI_LEVEL - 1) {
        // AI breakthrough: higher chance for ASCETIC, lower for others
        float break_chance = (e->ai_personality == 3) ? 0.15f : 0.03f;
        if ((float)rand() / RAND_MAX < break_chance) {
            Entity_TryBreakthrough(e);
        }
    }
}
