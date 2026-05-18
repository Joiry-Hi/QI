/**
 * combat.c
 * 实时碰撞检测: 空间哈希网格 + Hitbox生命周期 + 伤害事件
 */
#include "combat.h"
#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// --- Hitbox池 ---
Hitbox g_Hitboxes[MAX_HITBOXES];
int g_HitboxCount = 0;

DamageEvent g_DamageEvents[MAX_EVENTS_PER_FRAME];
int g_DamageEventCount = 0;

// --- 空间哈希网格 ---
// 每格最多存储的数量
#define MAX_PER_CELL 64

typedef struct {
    int entity_ids[MAX_PER_CELL];
    int entity_count;
    int hitbox_indices[MAX_PER_CELL];
    int hitbox_count;
} CollisionCell;

static CollisionCell g_Grid[GRID_COLS][GRID_ROWS];

void CollisionGrid_Clear()
{
    for (int x = 0; x < GRID_COLS; x++) {
        for (int y = 0; y < GRID_ROWS; y++) {
            g_Grid[x][y].entity_count = 0;
            g_Grid[x][y].hitbox_count = 0;
        }
    }
    g_DamageEventCount = 0;
}

static void GridCoords(Vector2 pos, int* cx, int* cy)
{
    *cx = (int)(pos.x / COLLISION_CELL_SIZE);
    *cy = (int)(pos.y / COLLISION_CELL_SIZE);
    if (*cx < 0) *cx = 0;
    if (*cx >= GRID_COLS) *cx = GRID_COLS - 1;
    if (*cy < 0) *cy = 0;
    if (*cy >= GRID_ROWS) *cy = GRID_ROWS - 1;
}

void CollisionGrid_InsertEntity(int entity_id, Vector2 pos, float radius)
{
    int cx, cy;
    GridCoords(pos, &cx, &cy);
    // 也插入相邻格，因为大实体可能跨越格子边界
    int r_cells = (int)(radius / COLLISION_CELL_SIZE) + 1;
    for (int dx = -r_cells; dx <= r_cells; dx++) {
        for (int dy = -r_cells; dy <= r_cells; dy++) {
            int gx = cx + dx, gy = cy + dy;
            if (gx < 0 || gx >= GRID_COLS || gy < 0 || gy >= GRID_ROWS) continue;
            CollisionCell* cell = &g_Grid[gx][gy];
            if (cell->entity_count < MAX_PER_CELL) {
                cell->entity_ids[cell->entity_count++] = entity_id;
            }
        }
    }
}

void CollisionGrid_InsertHitbox(int hitbox_idx)
{
    Hitbox* h = &g_Hitboxes[hitbox_idx];
    int cx, cy;
    GridCoords(h->position, &cx, &cy);
    int r_cells = (int)(h->radius / COLLISION_CELL_SIZE) + 1;
    for (int dx = -r_cells; dx <= r_cells; dx++) {
        for (int dy = -r_cells; dy <= r_cells; dy++) {
            int gx = cx + dx, gy = cy + dy;
            if (gx < 0 || gx >= GRID_COLS || gy < 0 || gy >= GRID_ROWS) continue;
            CollisionCell* cell = &g_Grid[gx][gy];
            if (cell->hitbox_count < MAX_PER_CELL) {
                cell->hitbox_indices[cell->hitbox_count++] = hitbox_idx;
            }
        }
    }
}

// 精确圆形碰撞检查
static bool CircleOverlap(Vector2 a, float ra, Vector2 b, float rb)
{
    float dx = a.x - b.x, dy = a.y - b.y;
    float dist_sq = dx*dx + dy*dy;
    float sum_r = ra + rb;
    return dist_sq < sum_r * sum_r;
}

// 检查点是否在扇形内
static bool PointInArc(Vector2 point, Vector2 origin, Vector2 dir, float radius, float arc_half)
{
    float dx = point.x - origin.x, dy = point.y - origin.y;
    float dist_sq = dx*dx + dy*dy;
    if (dist_sq > radius * radius) return false;
    float dist = sqrtf(dist_sq);
    if (dist < 0.001f) return true;
    float dot = (dx * dir.x + dy * dir.y) / dist;
    float angle = acosf(dot);
    return angle <= arc_half;
}

void CollisionGrid_Resolve()
{
    // 处理所有active Hitbox
    for (int hi = 0; hi < g_HitboxCount; hi++) {
        Hitbox* h = &g_Hitboxes[hi];
        if (!h->lifetime || h->has_hit) continue;
        if (h->type == HITBOX_BEAM) continue; // Beam已在施放时处理

        if (h->type == HITBOX_PROJECTILE) {
            int cx, cy;
            GridCoords(h->position, &cx, &cy);

            // 检查同格和相邻格
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    int gx = cx + dx, gy = cy + dy;
                    if (gx < 0 || gx >= GRID_COLS || gy < 0 || gy >= GRID_ROWS) continue;
                    CollisionCell* cell = &g_Grid[gx][gy];

                    for (int ei = 0; ei < cell->entity_count; ei++) {
                        int eid = cell->entity_ids[ei];
                        Entity* e = Entity_GetByID(eid);
                        if (!e || !e->is_alive) continue;
                        if (e->entity_id == h->source_entity_id) continue;
                        if (e->faction == ENTITY_NEUTRAL) continue;

                        if (CircleOverlap(h->position, h->radius, e->position, e->radius)) {
                            // 生成伤害事件
                            if (g_DamageEventCount < MAX_EVENTS_PER_FRAME) {
                                DamageEvent* ev = &g_DamageEvents[g_DamageEventCount++];
                                ev->target_entity_id = eid;
                                ev->source_entity_id = h->source_entity_id;
                                ev->skill_id = h->skill_id;
                                ev->raw_damage = h->damage;
                                ev->final_damage = h->damage;
                                ev->result = RESULT_TYPE_NORMAL;
                                ev->knockback = (Vector2){h->velocity.x * 0.2f, h->velocity.y * 0.2f};
                            }

                            if (h->pierce_count == 0) {
                                h->has_hit = true;
                                Hitbox_Destroy(h);
                                break;
                            } else if (h->pierce_count > 0) {
                                h->pierce_count--;
                            }
                            // pierce_count == -1: infinite pierce
                        }
                    }
                    if (h->has_hit) break;
                }
                if (h->has_hit) break;
            }
        }
        else if (h->type == HITBOX_SLASH) {
            // Slash: 仅在第一帧检查扇形范围
            int cx, cy;
            GridCoords(h->position, &cx, &cy);

            float arc_half = h->arc_angle * 0.5f;
            for (int dx = -2; dx <= 2; dx++) {
                for (int dy = -2; dy <= 2; dy++) {
                    int gx = cx + dx, gy = cy + dy;
                    if (gx < 0 || gx >= GRID_COLS || gy < 0 || gy >= GRID_ROWS) continue;
                    CollisionCell* cell = &g_Grid[gx][gy];

                    for (int ei = 0; ei < cell->entity_count; ei++) {
                        int eid = cell->entity_ids[ei];
                        Entity* e = Entity_GetByID(eid);
                        if (!e || !e->is_alive) continue;
                        if (e->entity_id == h->source_entity_id) continue;
                        if (e->faction == ENTITY_NEUTRAL) continue;

                        if (PointInArc(e->position, h->position, h->arc_dir, h->radius, arc_half)) {
                            if (g_DamageEventCount < MAX_EVENTS_PER_FRAME) {
                                DamageEvent* ev = &g_DamageEvents[g_DamageEventCount++];
                                ev->target_entity_id = eid;
                                ev->source_entity_id = h->source_entity_id;
                                ev->skill_id = h->skill_id;
                                ev->raw_damage = h->damage;
                                ev->final_damage = h->damage;
                                ev->result = RESULT_TYPE_NORMAL;
                                ev->knockback = (Vector2){h->arc_dir.x * 5.0f, h->arc_dir.y * 5.0f};
                            }
                        }
                    }
                }
            }
            h->has_hit = true; // Slash只作用一帧
        }
        else if (h->type == HITBOX_AOE) {
            // AOE: 每帧检查范围内实体
            Vector2 aoe_pos = h->position;
            if (h->aoe_source_is_entity) {
                Entity* src = Entity_GetByID(h->aoe_source_entity_id);
                if (src) aoe_pos = src->position;
            }

            int cx, cy;
            GridCoords(aoe_pos, &cx, &cy);
            int r_cells = (int)(h->aoe_outer_radius / COLLISION_CELL_SIZE) + 1;

            for (int dx = -r_cells; dx <= r_cells; dx++) {
                for (int dy = -r_cells; dy <= r_cells; dy++) {
                    int gx = cx + dx, gy = cy + dy;
                    if (gx < 0 || gx >= GRID_COLS || gy < 0 || gy >= GRID_ROWS) continue;
                    CollisionCell* cell = &g_Grid[gx][gy];

                    for (int ei = 0; ei < cell->entity_count; ei++) {
                        int eid = cell->entity_ids[ei];
                        Entity* e = Entity_GetByID(eid);
                        if (!e || !e->is_alive) continue;
                        if (e->entity_id == h->source_entity_id && !h->aoe_source_is_entity) continue;
                        if (e->faction == ENTITY_NEUTRAL) continue;
                        // 自身护盾不伤自己
                        if (h->aoe_source_is_entity && e->entity_id == h->source_entity_id && h->damage > 0) continue;

                        if (CircleOverlap(aoe_pos, h->aoe_outer_radius, e->position, e->radius)) {
                            if (g_DamageEventCount < MAX_EVENTS_PER_FRAME && e->invuln_timer <= 0) {
                                DamageEvent* ev = &g_DamageEvents[g_DamageEventCount++];
                                ev->target_entity_id = eid;
                                ev->source_entity_id = h->source_entity_id;
                                ev->skill_id = h->skill_id;
                                ev->raw_damage = h->damage;
                                ev->final_damage = h->damage;
                                ev->result = RESULT_TYPE_NORMAL;
                                // 从AOE中心向外的击退
                                float kx = e->position.x - aoe_pos.x;
                                float ky = e->position.y - aoe_pos.y;
                                float klen = sqrtf(kx*kx + ky*ky);
                                if (klen > 0.1f) {
                                    ev->knockback.x = kx / klen * 3.0f;
                                    ev->knockback.y = ky / klen * 3.0f;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// ============================================================
// Hitbox 生命周期
// ============================================================
Hitbox* Hitbox_Spawn(int source_id, SkillID skill_id, Vector2 pos, Vector2 vel,
                     float radius, int damage, float lifetime, HitboxType type)
{
    if (g_HitboxCount >= MAX_HITBOXES) return NULL;

    Hitbox* h = &g_Hitboxes[g_HitboxCount++];
    memset(h, 0, sizeof(Hitbox));
    h->source_entity_id = source_id;
    h->skill_id = skill_id;
    h->position = pos;
    h->velocity = vel;
    h->radius = radius;
    h->damage = damage;
    h->lifetime = lifetime;
    h->max_lifetime = lifetime;
    h->type = type;
    h->pierce_count = 0;       // 默认穿透0个(命中即消失)
    h->target_entity_id = -1;
    h->vfx = NULL;

    // 从技能数据库获取属性
    h->attribute = g_skill_database[skill_id].attribute_id;

    // 特殊技能: 血魔钻穿3个, 冰焰穿所有
    if (skill_id == SKILL_ID_BLOOD_DEVIL_DRILL) h->pierce_count = 3;
    if (skill_id == SKILL_ID_ICE_FLAME) h->pierce_count = -1;

    return h;
}

void Hitbox_Update(Hitbox* h, float dt)
{
    if (h->lifetime <= 0) return;
    if (h->has_hit) { Hitbox_Destroy(h); return; }

    h->lifetime -= dt;
    if (h->lifetime <= 0) {
        Hitbox_Destroy(h);
        return;
    }

    // 更新位置
    if (h->type == HITBOX_PROJECTILE) {
        // 追踪逻辑
        if (h->target_entity_id >= 0) {
            Entity* target = Entity_GetByID(h->target_entity_id);
            if (target && target->is_alive) {
                float dx = target->position.x - h->position.x;
                float dy = target->position.y - h->position.y;
                float dist = sqrtf(dx*dx + dy*dy);
                if (dist > 0.1f) {
                    float nx = dx / dist, ny = dy / dist;
                    h->velocity.x += nx * h->homing_strength * 60.0f * dt;
                    h->velocity.y += ny * h->homing_strength * 60.0f * dt;
                }
            }
        }
        h->position.x += h->velocity.x * dt;
        h->position.y += h->velocity.y * dt;
    }
    else if (h->type == HITBOX_AOE && h->aoe_source_is_entity) {
        // AOE跟随施法者
        Entity* src = Entity_GetByID(h->aoe_source_entity_id);
        if (src) h->position = src->position;
    }

    // 边界检查: 投射物离开屏幕则销毁
    if (h->type == HITBOX_PROJECTILE) {
        if (h->position.x < -50 || h->position.x > SCREEN_WIDTH + 50 ||
            h->position.y < -50 || h->position.y > SCREEN_HEIGHT + 50) {
            Hitbox_Destroy(h);
        }
    }
}

void Hitbox_UpdateAll(float dt)
{
    for (int i = 0; i < g_HitboxCount; i++) {
        Hitbox_Update(&g_Hitboxes[i], dt);
    }
}

void Hitbox_Destroy(Hitbox* h)
{
    h->lifetime = 0;
    h->has_hit = true;
    // 从池中移除 (swap with last)
    int idx = (int)(h - g_Hitboxes);
    if (idx >= 0 && idx < g_HitboxCount) {
        g_Hitboxes[idx] = g_Hitboxes[g_HitboxCount - 1];
        g_HitboxCount--;
    }
}

// ============================================================
// Beam 即时射线检测
// ============================================================
void Beam_Check(Vector2 from, Vector2 to, int source_id, SkillID skill_id,
                int damage, int* out_hit_ids, int* out_count)
{
    *out_count = 0;
    // 简单: 遍历所有实体, 检查到射线的最短距离
    float line_dx = to.x - from.x, line_dy = to.y - from.y;
    float line_len = sqrtf(line_dx*line_dx + line_dy*line_dy);
    if (line_len < 0.001f) return;

    for (int i = 0; i < g_EntityCount; i++) {
        Entity* e = &g_Entities[i];
        if (!e->is_alive) continue;
        if (e->entity_id == source_id) continue;
        if (e->faction == ENTITY_NEUTRAL) continue;

        // 点(实体)到线段(射线)的最短距离
        float ex = e->position.x - from.x;
        float ey = e->position.y - from.y;
        float t = (ex * line_dx + ey * line_dy) / (line_len * line_len);
        if (t < 0) t = 0;
        if (t > 1) t = 1;

        float closest_x = from.x + t * line_dx;
        float closest_y = from.y + t * line_dy;
        float dx = e->position.x - closest_x;
        float dy = e->position.y - closest_y;
        float dist_sq = dx*dx + dy*dy;
        float threshold = e->radius + 15.0f; // BEAM宽度15px

        if (dist_sq < threshold * threshold) {
            if (*out_count < MAX_ENTITIES) {
                out_hit_ids[*out_count] = e->entity_id;
                (*out_count)++;

                if (g_DamageEventCount < MAX_EVENTS_PER_FRAME) {
                    DamageEvent* ev = &g_DamageEvents[g_DamageEventCount++];
                    ev->target_entity_id = e->entity_id;
                    ev->source_entity_id = source_id;
                    ev->skill_id = skill_id;
                    ev->raw_damage = damage;
                    ev->final_damage = damage;
                    ev->result = RESULT_TYPE_NORMAL;
                    ev->knockback = (Vector2){line_dx / line_len * 2.0f, line_dy / line_len * 2.0f};
                }
            }
        }
    }
}
