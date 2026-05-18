/**
 * main_sandbox.c
 * QI-Anima: Physics Dojo (Dojo de Física)
 * WASD 移动, 鼠标施法, 四个角落有沙包靶子
 */
//gcc -o qi_sandbox main_sandbox.c engine.c vfx.c QI.c -lSDL2 -lm -DQI_LIBRARY
#include "engine.h"
#include "vfx.h"
#include "QI.h"
#include <stdio.h>
#include <string.h>

#define MAX_MANUAL_SPELLS 64
#define NUM_DUMMIES 4 // 沙包数量

static SpellFX g_ManualSpells[MAX_MANUAL_SPELLS];
static int g_SkillIntensity = 1;

// --- 辅助函数 ---
SpellFX *GetFreeSpellSlot()
{
    for (int i = 0; i < MAX_MANUAL_SPELLS; i++)
    {
        if (!g_ManualSpells[i].active)
            return &g_ManualSpells[i];
    }
    return NULL;
}

const char *GetSkillName(SkillID id)
{
    if (id >= 0 && id < TOTAL_SKILLS)
        return g_skill_database[id].name_chn;
    return "Unknown";
}

int main(int argc, char *argv[])
{
    // 1. 初始化
    if (!Engine_Init("QI-Anima: Physics Dojo"))
        return 1;
    Initialize_Databases();

    // --- 2. 创建角色 ---
    // 主角 (Player)
    Caster player = Caster_Create(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, (SDL_Color){0, 255, 255, 255});
    float player_speed = 3.0f;

    // 沙包靶子 (Dummies)
    Caster dummies[NUM_DUMMIES];
    SDL_Color dummy_color = {200, 100, 50, 255};                                              // 棕色
    float padding = 100.0f;                                                                   // 离屏幕边缘的距离
    dummies[0] = Caster_Create(padding, padding, dummy_color);                                // 左上
    dummies[1] = Caster_Create(SCREEN_WIDTH - padding, padding, dummy_color);                 // 右上
    dummies[2] = Caster_Create(padding, SCREEN_HEIGHT - padding, dummy_color);                // 左下
    dummies[3] = Caster_Create(SCREEN_WIDTH - padding, SCREEN_HEIGHT - padding, dummy_color); // 右下

    // --- 3. 状态变量 ---
    bool is_running = true;
    SDL_Event event;
    SkillID current_skill = SKILL_ID_FIREBALL;
    int mouse_x, mouse_y;
    bool left_mouse_down = false;
    bool right_mouse_down = false;
    const Uint8 *key_state = SDL_GetKeyboardState(NULL); // 获取键盘状态快照

    // 打印操作说明
    printf("=== QI-Anima Physics Dojo ===\n");
    printf("[WASD]: Move Player\n");
    printf("[L-Click]: Cast Spell\n");
    printf("[R-Click]: Repulsive Force\n");
    printf("[1-8]: Switch Skills\n");
    printf("[0]: Increase Intensity\n");
    printf("=============================\n");

    // 初始化窗口标题
    char title[128];
    sprintf(title, "Dojo - Skill: %s | Intensity: %d", GetSkillName(current_skill), g_SkillIntensity);
    SDL_SetWindowTitle(Engine_GetWindow(), title);

    while (is_running)
    {
        // --- 输入处理 ---
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                is_running = false;

            // 键盘按键事件 (用于切换技能等一次性操作)
            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_1:
                    current_skill = SKILL_ID_FIREBALL;
                    break;
                case SDLK_2:
                    current_skill = SKILL_ID_WINDBLADE;
                    break;
                case SDLK_3:
                    current_skill = SKILL_ID_TERMINATE_THUNDER;
                    break;
                case SDLK_4:
                    current_skill = SKILL_ID_BLOOD_DEVIL_DRILL;
                    break;
                case SDLK_5:
                    current_skill = SKILL_ID_SMITE;
                    break;
                case SDLK_6:
                    current_skill = SKILL_ID_COMMANDING_SWORDS;
                    break;
                case SDLK_7:
                    current_skill = SKILL_ID_STRIKE;
                    break;
                case SDLK_8:
                    current_skill = SKILL_ID_GREAT_GOLDEN_SWORDFORMATION;
                    break;
                case SDLK_9:
                    g_SkillIntensity = 1;
                    break;
                case SDLK_0:
                    g_SkillIntensity++;
                    break;
                }
                sprintf(title, "Dojo - Skill: %s | Intensity: %d", GetSkillName(current_skill), g_SkillIntensity);
                SDL_SetWindowTitle(Engine_GetWindow(), title);
            }

            // 鼠标点击事件
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                    left_mouse_down = true;
                if (event.button.button == SDL_BUTTON_RIGHT)
                    right_mouse_down = true;
            }
            if (event.type == SDL_MOUSEBUTTONUP)
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                    left_mouse_down = false;
                if (event.button.button == SDL_BUTTON_RIGHT)
                    right_mouse_down = false;
            }
        }

        // --- 持续按键处理 (用于移动) ---
        if (key_state[SDL_SCANCODE_W])
            player.position.y -= player_speed;
        if (key_state[SDL_SCANCODE_S])
            player.position.y += player_speed;
        if (key_state[SDL_SCANCODE_A])
            player.position.x -= player_speed;
        if (key_state[SDL_SCANCODE_D])
            player.position.x += player_speed;

        // 获取鼠标位置
        SDL_GetMouseState(&mouse_x, &mouse_y);

        // --- 物理与逻辑更新 ---
        ForceGrid_Clear();

        // 2. 右键交互：鼠标变成一个强斥力场 (上帝之手)
        if (right_mouse_down)
        {
            Vector2 mouse_pos = {(float)mouse_x, (float)mouse_y};
            // 半径 100，强度 2.0 (斥力)
            ForceGrid_AddRadialForce(mouse_pos, 100.0f, 2.0f);

            // 视觉反馈：在鼠标位置画一点特效
            if (rand() % 5 == 0)
            {
                SDL_Color col = {255, 255, 255, 100};
                Particle_Emit(mouse_pos, (Vector2){0, 0}, col, 10, 1);
            }
        }
        // 3. 左键交互：发射技能
        // 为了避免每帧都发射太快，我们加个简单的冷却检查
        static int cooldown = 0;
        if (cooldown > 0)
            cooldown--;

        if (left_mouse_down && cooldown <= 0)
        {
            SpellFX *slot = GetFreeSpellSlot();
            if (slot)
            {
                Vector2 start_pos = player.position;
                Vector2 target_pos = {(float)mouse_x, (float)mouse_y};

                // 计算方向向量
                Vector2 dir = Vec2_Sub(target_pos, start_pos);
                float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
                if (len > 0)
                {
                    dir.x /= len;
                    dir.y /= len;
                }

                // 施法！
                // duration 设为 60 帧 (1秒)，对于 Instant 技能会被内部逻辑处理
                SpellFX_Cast(slot, current_skill, start_pos, dir, 60, g_SkillIntensity);

                // 针对定点技能（如巨剑术），更新目标位置为鼠标位置
                slot->target_pos = target_pos;

                // 设置冷却 (根据技能类型不同可调整)
                if (current_skill == SKILL_ID_WINDBLADE)
                    cooldown = 30;
                else if (current_skill == SKILL_ID_GREATSWORD)
                    cooldown = 40;
                else
                    cooldown = 8; // 火球射速快
            }
        }

        // 更新所有法术
        Vector2 mouse_vec = {(float)mouse_x, (float)mouse_y};
        for (int i = 0; i < MAX_MANUAL_SPELLS; i++)
        {
            SpellFX *s = &g_ManualSpells[i];
            if (s->active && s->id == VFX_HOMING_MISSILE)
            {
                s->target_pos = mouse_vec;
            }
            SpellFX_Update(s);
        }

        // 更新角色动画
        Caster_Update(&player);
        for (int i = 0; i < NUM_DUMMIES; i++)
        {
            Caster_Update(&dummies[i]);
        }

        // 更新粒子物理
        Particle_UpdateAll();

        // --- 渲染 ---
        Engine_Clear();
        Particle_RenderAll();
        Engine_Present();

        SDL_Delay(16);
    }

    Engine_Cleanup();
    return 0;
}