/**
 * game.c
 * 主游戏循环: 输入/更新/渲染 编排
 */
#include "game.h"
#include "skill_config.h"
#include "engine.h"
#include "ui.h"
#include "ai.h"
#include "QI.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

GameState g_Game;
SpellFX g_VFXPool[MAX_VFX];
DamageNumber g_DamageNumbers[MAX_DAMAGE_NUMBERS];

// 外部声明 (QI.c)
extern void Initialize_Databases();
extern int max_HP[TOTAL_XIUWEI_LEVEL];
extern int max_QI[TOTAL_XIUWEI_LEVEL];
extern int Yuan[TOTAL_XIUWEI_LEVEL];

// 鼠标状态
static int g_MouseX = 400, g_MouseY = 300;
static bool g_MouseLeft = false;
static bool g_MouseRight = false;
static bool g_KeySkill[9] = {false};
static bool g_KeyGainQi = false;
static bool g_KeyDash = false;

void Game_Init()
{
    memset(&g_Game, 0, sizeof(GameState));
    memset(g_VFXPool, 0, sizeof(g_VFXPool));
    memset(g_DamageNumbers, 0, sizeof(g_DamageNumbers));

    // 初始化技能数据库和实时配置
    Initialize_Databases();
    SkillConfig_Init();

    // 清空全局注册表
    g_EntityCount = 0;
    g_HitboxCount = 0;
    g_DamageEventCount = 0;

    // 创建玩家 - 飞升境界以展示所有技能
    Entity_Init(&g_Game.player, 0, "Joiry", ASCENSION, ENTITY_PLAYER);
    // Entity_Init copies into g_Entities[0]; set extra props on the authoritative copy
    g_Entities[0].position = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    g_Entities[0].move_speed = 280.0f;
    g_Entities[0].qi = g_Entities[0].max_qi / 2;
    g_Entities[0].qi_regen_rate = 2;

    // 生成第一波
    g_Game.wave_number = 0;
    g_Game.is_game_over = false;
    Game_SpawnWave(1);
}

void Game_SpawnWave(int wave)
{
    g_Game.wave_number = wave;
    g_Game.wave_cleared = false;
    g_Game.wave_rest_timer = 0;

    int enemy_count = 2 + wave; // 基础2个, 每波+1
    if (enemy_count > MAX_ENEMIES) enemy_count = MAX_ENEMIES;

    // Boss波: 每5波
    bool is_boss = (wave % 5 == 0);
    if (is_boss) enemy_count = 1;

    // 境界随波次递增: 每3波提升一个境界
    int xiuwei_level = (wave - 1) / 3;
    if (xiuwei_level >= TOTAL_XIUWEI_LEVEL) xiuwei_level = TOTAL_XIUWEI_LEVEL - 1;

    for (int i = 0; i < enemy_count; i++) {
        Entity enemy;
        char name[32];
        if (is_boss) {
            snprintf(name, 32, "Boss W%d", wave);
        } else {
            snprintf(name, 32, "Enemy %d", i + 1);
        }
        Entity_Init(&enemy, g_EntityCount, name, (XIUWEI)xiuwei_level, ENTITY_ENEMY);

        // 随机生成在屏幕边缘
        int edge = rand() % 4;
        float padding = 40.0f;
        switch (edge) {
            case 0: enemy.position = (Vector2){rand() % SCREEN_WIDTH, -padding}; break;
            case 1: enemy.position = (Vector2){rand() % SCREEN_WIDTH, SCREEN_HEIGHT + padding}; break;
            case 2: enemy.position = (Vector2){-padding, rand() % SCREEN_HEIGHT}; break;
            default: enemy.position = (Vector2){SCREEN_WIDTH + padding, rand() % SCREEN_HEIGHT}; break;
        }

        // Boss有额外加成
        if (is_boss) {
            enemy.hp *= 5;
            enemy.max_hp = enemy.hp;
            enemy.move_speed *= 0.7f;
            enemy.radius = 28.0f;
            enemy.color = (SDL_Color){255, 200, 0, 255};
            enemy.caster.color = enemy.color;
        }

        // 随机AI人格 (0-5)
        enemy.ai_personality = rand() % 6;

        // 注册 (Entity_Init内部已经调用Register, 但ID需要更新)
        // Entity_Init uses g_EntityCount, so the entity is already in the array
        // We just need to update its local copy - actually Entity_Init calls
        // Register which copies the entity in. So the local is just a temp.
    }

    g_Game.enemies_remaining = enemy_count;
    printf("[Wave %d] %d enemies spawned (Xiuwei: %s)%s\n",
           wave, enemy_count,
           (xiuwei_level < TOTAL_XIUWEI_LEVEL) ? "level" : "?",
           is_boss ? " -- BOSS WAVE!" : "");

    // 玩家回复一些状态
    g_Entities[0].hp += g_Entities[0].max_hp / 4;
    if (g_Entities[0].hp > g_Entities[0].max_hp) g_Entities[0].hp = g_Entities[0].max_hp;
    g_Entities[0].qi += g_Entities[0].max_qi / 4;
    if (g_Entities[0].qi > g_Entities[0].max_qi) g_Entities[0].qi = g_Entities[0].max_qi;
}

void Game_AddDamageNumber(Vector2 pos, int dmg, SDL_Color color)
{
    for (int i = 0; i < MAX_DAMAGE_NUMBERS; i++) {
        if (!g_DamageNumbers[i].active) {
            g_DamageNumbers[i].position = pos;
            g_DamageNumbers[i].damage = dmg;
            g_DamageNumbers[i].lifetime = 1.0f;
            g_DamageNumbers[i].max_lifetime = 1.0f;
            g_DamageNumbers[i].color = color;
            g_DamageNumbers[i].active = true;
            return;
        }
    }
}

void Game_ProcessInput(SDL_Event* event, const Uint8* keystate, float dt)
{
    Entity* p = &g_Entities[0]; // player is always entity 0

    // --- 移动 (WASD连续按键) ---
    Vector2 move_dir = {0, 0};
    if (keystate[SDL_SCANCODE_W]) move_dir.y -= 1;
    if (keystate[SDL_SCANCODE_S]) move_dir.y += 1;
    if (keystate[SDL_SCANCODE_A]) move_dir.x -= 1;
    if (keystate[SDL_SCANCODE_D]) move_dir.x += 1;
    float mlen = sqrtf(move_dir.x*move_dir.x + move_dir.y*move_dir.y);
    if (mlen > 0.01f) {
        move_dir.x /= mlen; move_dir.y /= mlen;
    }
    p->velocity.x = move_dir.x * p->move_speed;
    p->velocity.y = move_dir.y * p->move_speed;

    // --- 鼠标 ---
    SDL_GetMouseState(&g_MouseX, &g_MouseY);

    // 鼠标滚轮切换技能 (通过event)
    if (event->type == SDL_MOUSEWHEEL) {
        if (event->wheel.y > 0) {
            p->selected_skill_slot = (p->selected_skill_slot - 1 + 9) % 9;
        } else if (event->wheel.y < 0) {
            p->selected_skill_slot = (p->selected_skill_slot + 1) % 9;
        }
    }

    // 数字键切换技能槽
    if (event->type == SDL_KEYDOWN) {
        SDL_Keycode key = event->key.keysym.sym;
        if (key >= SDLK_1 && key <= SDLK_9) {
            p->selected_skill_slot = (int)(key - SDLK_1);
        }
        // Q: Gain QI
        if (key == SDLK_q && !event->key.repeat) {
            SkillID id = SKILL_ID_GAIN_QI;
            if (Entity_CanCast(p, id)) {
                Entity_CastSkill(p, id, (Vector2){0,0});
            }
        }
        // Space: 闪避
        if (key == SDLK_SPACE && p->dash_cooldown <= 0 && p->qi >= 2) {
            p->dash_cooldown = 1.5f;
            p->qi -= 2;
            p->invuln_timer = 0.2f;
            Vector2 dash_dir = move_dir;
            if (mlen < 0.01f) { dash_dir.x = 0; dash_dir.y = -1; } // 默认向上闪
            p->position.x += dash_dir.x * 120.0f;
            p->position.y += dash_dir.y * 120.0f;
            // 闪避粒子
            for (int i = 0; i < 10; i++) {
                SDL_Color white = {255,255,255,150};
                Particle_Emit(p->position, (Vector2){-dash_dir.x * 2, -dash_dir.y * 2}, white, 15, 1);
            }
        }
    }

    // --- 鼠标施法 ---
    Uint32 mouse_state = SDL_GetMouseState(NULL, NULL);
    bool left_click = (mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    bool right_click = (mouse_state & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;

    Vector2 mouse_world = {(float)g_MouseX, (float)g_MouseY};

    // 左键: 施放当前装备技能 (带简单冷却防连点)
    static float left_click_timer = 0;
    left_click_timer -= dt;
    if (left_click && left_click_timer <= 0) {
        SkillID id = p->equipped_skills[p->selected_skill_slot];
        if (id != SKILL_ID_NONE && Entity_CanCast(p, id)) {
            Entity_CastSkill(p, id, mouse_world);
            left_click_timer = 0.15f; // 最小施法间隔
        }
    }

    // 右键: 防御技能
    static float right_click_timer = 0;
    right_click_timer -= dt;
    if (right_click && right_click_timer <= 0) {
        SkillID id = SKILL_ID_DEFEND;
        // 使用可用的最高级防御技能
        if (p->learned_skills[ACTION_TYPE_DEFEND].skill_id != SKILL_ID_NONE) {
            id = p->learned_skills[ACTION_TYPE_DEFEND].skill_id;
        }
        if (Entity_CanCast(p, id)) {
            Entity_CastSkill(p, id, mouse_world);
            right_click_timer = 0.5f;
        }
    }
}

void Game_Update(float dt)
{
    GameState* gs = &g_Game;
    Entity* p = &g_Entities[0]; // player is always entity 0

    if (gs->is_game_over) return;

    gs->game_time += dt;

    // 1. 更新玩家位置
    p->position.x += p->velocity.x * dt;
    p->position.y += p->velocity.y * dt;
    if (p->dash_cooldown > 0) p->dash_cooldown -= dt;

    // 2. 更新实体
    Entity_Update(p, dt);
    for (int i = 0; i < g_EntityCount; i++) {
        Entity* e = &g_Entities[i];
        if (e->entity_id == p->entity_id) continue; // 玩家已更新
        if (!e->is_alive) continue;

        // AI decision and movement
        AI_Update(e, p, dt);
        e->position.x += e->velocity.x * dt;
        e->position.y += e->velocity.y * dt;

        Entity_Update(e, dt);
    }

    // 3. 更新Hitbox
    Hitbox_UpdateAll(dt);

    // 4. 清空并重建碰撞网格
    CollisionGrid_Clear();
    for (int i = 0; i < g_EntityCount; i++) {
        if (g_Entities[i].is_alive)
            CollisionGrid_InsertEntity(i, g_Entities[i].position, g_Entities[i].radius);
    }
    for (int i = 0; i < g_HitboxCount; i++) {
        CollisionGrid_InsertHitbox(i);
    }

    // 5. 碰撞解析
    CollisionGrid_Resolve();

    // 6. 应用伤害事件
    for (int i = 0; i < g_DamageEventCount; i++) {
        DamageEvent* ev = &g_DamageEvents[i];
        Entity* target = Entity_GetByID(ev->target_entity_id);
        if (target && target->is_alive) {
            Entity_TakeDamage(target, ev->final_damage, ev->source_entity_id, ev->skill_id);
            // 浮动伤害数字
            Game_AddDamageNumber(target->position, ev->final_damage,
                                 (SDL_Color){255, 100, 50, 255});
            // 击退
            target->position.x += ev->knockback.x;
            target->position.y += ev->knockback.y;
        }
    }

    // 7. 更新VFX
    for (int i = 0; i < MAX_VFX; i++) {
        if (g_VFXPool[i].active) {
            SpellFX_Update(&g_VFXPool[i]);
        }
    }

    // 8. 更新伤害数字
    for (int i = 0; i < MAX_DAMAGE_NUMBERS; i++) {
        if (g_DamageNumbers[i].active) {
            g_DamageNumbers[i].lifetime -= dt;
            g_DamageNumbers[i].position.y -= 40.0f * dt; // 向上浮动
            if (g_DamageNumbers[i].lifetime <= 0) {
                g_DamageNumbers[i].active = false;
            }
        }
    }

    // 9. 检查敌人存活
    int alive_enemies = 0;
    for (int i = 0; i < g_EntityCount; i++) {
        if (g_Entities[i].faction == ENTITY_ENEMY && g_Entities[i].is_alive)
            alive_enemies++;
    }
    gs->enemies_remaining = alive_enemies;

    // 10. 波次清理检查
    if (alive_enemies == 0 && !gs->wave_cleared && gs->wave_number > 0) {
        gs->wave_cleared = true;
        gs->wave_rest_timer = 2.0f; // 2秒后下一波
        gs->score += gs->wave_number * 100;
        printf("[Wave %d Cleared!] Score: %d\n", gs->wave_number, gs->score);
    }

    if (gs->wave_cleared) {
        gs->wave_rest_timer -= dt;
        if (gs->wave_rest_timer <= 0) {
            Game_SpawnWave(gs->wave_number + 1);
        }
    }

    // 11. 检查玩家死亡
    if (!p->is_alive) {
        gs->is_game_over = true;
        printf("=== GAME OVER ===\n");
        printf("Waves: %d | Score: %d | Time: %.1fs\n", gs->wave_number, gs->score, gs->game_time);
    }

    // 12. 突破检查
    if (p->is_alive && p->qi >= p->max_qi && p->xiuwei < TOTAL_XIUWEI_LEVEL - 1) {
        Entity_TryBreakthrough(p);
    }

    // 13. 粒子物理
    Particle_UpdateAll();
}

void Game_Render()
{
    Engine_Clear();

    UI_DrawEnemyHealthBars();
    UI_DrawPlayerHUD(&g_Entities[0]);
    UI_DrawSkillBar(&g_Entities[0]);
    UI_DrawWaveInfo(g_Game.wave_number, g_Game.score, g_Game.game_time);
    UI_DrawDamageNumbers();

    // 渲染粒子 (在UI之后, 让粒子在UI上层)
    Particle_RenderAll();

    if (g_Game.is_game_over) {
        UI_DrawGameOver(g_Game.wave_number, g_Game.score, g_Game.game_time);
    }

    Engine_Present();
}
