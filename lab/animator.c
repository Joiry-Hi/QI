/**
 * animator.c
 * 修复版 V3: 支持并发演出 (Simultaneous Execution)
 */
#include "animator.h"
#include "engine.h"
#include <stdio.h>
#include <math.h>

#define MAX_EVENT_QUEUE 64 // 增加队列深度，因为现在一个回合会有多个指令

// 内部动画指令结构
typedef struct
{
    int type; // 0: Wait, 1: Spell
    int duration;
    bool is_parallel; // <--- 新增：是否并行执行（不阻塞队列）

    // Spell Info
    int caster_idx;
    SkillID skill_id;
    Vector2 target_pos;

    // Event Context
    ResultType result;
    int vfx_param;
} AnimCmd;

static Caster *g_Casters[2];
static SpellFX g_Spells[2];

static AnimCmd g_Queue[MAX_EVENT_QUEUE];
static int g_QueueHead = 0;
static int g_QueueTail = 0;
static int g_QueueCount = 0;

// 状态机
static int g_Timer = 0;
static bool g_Busy = false;

void Animator_Init(Caster *p1, Caster *p2)
{
    g_Casters[0] = p1;
    g_Casters[1] = p2;
    g_Spells[0].active = false;
    g_Spells[1].active = false;
}

static void Enqueue(AnimCmd cmd)
{
    if (g_QueueCount >= MAX_EVENT_QUEUE)
    {
        printf("Animator: Queue Full!\n");
        return;
    }
    g_Queue[g_QueueTail] = cmd;
    g_QueueTail = (g_QueueTail + 1) % MAX_EVENT_QUEUE;
    g_QueueCount++;
}

/*
// static VisualSpellID MapSkillToVisual(int skill_id, AttributeID attr)
// {
//     // 优先匹配特定的 SkillID
//     switch (skill_id)
//     {
//     case SKILL_ID_FIREBALL:
//     case SKILL_ID_FLAMEBLAST:
//         return VFX_FIREBALL;

//     case SKILL_ID_WINDBLADE:
//     case SKILL_ID_COMMANDING_SWORDS:
//         return VFX_WIND_BLADE;

//     case SKILL_ID_TERMINATE_THUNDER:
//         return VFX_THUNDER_STRIKE;

//     case SKILL_ID_ENERGY_SHIELD:
//     case SKILL_ID_GOLD_LIGHT_WARDING:
//     case SKILL_ID_IMMOVABLE_KING:
//     case SKILL_ID_DEFEND:
//         return VFX_SHIELD;

//     case SKILL_ID_HEAL:
//     case SKILL_ID_EVERGREEN_ART:
//     case SKILL_ID_CORE_RESTORATION:
//         return VFX_HEAL_AURA;
//     }

//     // 如果没有特定匹配，根据属性 (AttributeID) 匹配通用特效
//     switch (attr)
//     {
//     case ATTR_FIRE:
//         return VFX_FIREBALL;
//     case ATTR_WIND:
//         return VFX_WIND_BLADE;
//     case ATTR_THUNDER:
//         return VFX_THUNDER_STRIKE;
//     case ATTR_WOOD:
//         return VFX_HEAL_AURA;
//     default:
//         return VFX_GENERIC_CAST; // 默认
//     }
// }
*/

// --- 核心：解析逻辑事件并编排演出 ---
void Animator_PushRound(CombatEvent *events, int count)
{
    printf("Animator: Batching %d events for simultaneous playback.\n", count);

    int max_duration = 0;

    // 1. 遍历事件，生成“非阻塞”指令
    for (int i = 0; i < count; i++)
    {
        CombatEvent e = events[i];
        AnimCmd cmd;
        cmd.is_parallel = true; // 默认并行：不要等我，立刻执行下一个
        int current_duration = 0;

        AttributeID attr = ATTR_NONE;
        if (e.skill_id_used >= 0 && e.skill_id_used < TOTAL_SKILLS)
        {
            attr = g_skill_database[e.skill_id_used].attribute_id;
        }

        if (e.event_type == EVENT_TYPE_DAMAGE)
        {

            cmd.type = 1; // Spell
            cmd.caster_idx = e.source_entity_id;
            int target_idx = (e.source_entity_id == ENTITY_ID_YOU) ? ENTITY_ID_CPU : ENTITY_ID_YOU;
            if (target_idx < 0 || target_idx > 1)
                target_idx = 0;
            cmd.target_pos = g_Casters[target_idx]->position;
            cmd.skill_id = e.skill_id_used;
            cmd.result = e.result_type;

            cmd.skill_id = e.skill_id_used;

            // --- 参数推断逻辑 ---
            cmd.vfx_param = 1; // 默认值

            // 大庚剑阵：3层
            if (e.skill_id_used == SKILL_ID_GREAT_GOLDEN_SWORDFORMATION)
            {
                cmd.vfx_param = 3;
            }
            // 巨剑术：加大特效
            else if (e.skill_id_used == SKILL_ID_GREATSWORD)
            {
                cmd.vfx_param = 2;
            }
            // 玄天灭灵斩：超宽刀波
            else if (e.skill_id_used == SKILL_ID_SPIRIT_SLAYING_SWORD)
            {
                cmd.vfx_param = 3;
            }
            // 噬金虫群：数量更多
            else if (e.skill_id_used == SKILL_ID_BEETLE_SWARM)
            {
                cmd.vfx_param = 2;
            }

            current_duration = 60; // 攻击动画耗时
            if (e.skill_id_used == SKILL_ID_COMMANDING_SWORDS)
            {
                cmd.duration = 120; // 给它 2秒钟时间来回穿梭
            }
            cmd.duration = current_duration;
            Enqueue(cmd);
        }
        else if (e.event_type == EVENT_TYPE_MODIFIER_APPLY)
        {
            if (e.skill_id_used == SKILL_ID_ENERGY_SHIELD || e.skill_id_used == SKILL_ID_DEFEND)
            {
                cmd.type = 1;
                cmd.caster_idx = e.source_entity_id;
                cmd.target_pos = g_Casters[e.source_entity_id]->position; // 目标是自己
                cmd.skill_id = e.skill_id_used;
                cmd.result = RESULT_TYPE_NORMAL;

                current_duration = 100; // 护盾耗时
                cmd.duration = current_duration;
                Enqueue(cmd);
            }
        }

        // 记录这一轮最长的动画时间
        if (current_duration > max_duration)
        {
            max_duration = current_duration;
        }
    }

    // 2. 在所有并行指令之后，插入一个“同步屏障” (Wait)
    // 这确保了回合结束前，所有动画都能播完
    if (max_duration > 0)
    {
        AnimCmd wait = {
            .type = 0,
            .duration = max_duration + 20, // 额外加点缓冲
            .is_parallel = false           // <--- 阻塞！这就是回合的结束点
        };
        Enqueue(wait);
    }
}

void Animator_Update()
{
    // 1. 物理更新 (始终运行)
    SpellFX_Update(&g_Spells[0]);
    SpellFX_Update(&g_Spells[1]);

    // 2. 阻塞计时器
    if (g_Timer > 0)
    {
        g_Timer--;
        if (g_Timer <= 0)
        {
            g_Busy = false;
        }
        return;
    }

    // 3. 循环处理指令，直到遇到一个阻塞指令或队列为空
    while (g_QueueCount > 0)
    {
        AnimCmd cmd = g_Queue[g_QueueHead];
        g_QueueHead = (g_QueueHead + 1) % MAX_EVENT_QUEUE;
        g_QueueCount--;

        // --- 执行指令 ---
        if (cmd.type == 1)
        { // Cast Spell
            int idx = cmd.caster_idx;
            if (idx >= 0 && idx <= 1)
            {
                Vector2 start = g_Casters[idx]->position;
                Vector2 end = cmd.target_pos;
                Vector2 dir = (Vector2){end.x - start.x, end.y - start.y};

                float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
                if (len > 0.1f)
                {
                    dir.x /= len;
                    dir.y /= len;
                }
                else
                {
                    dir = (Vector2){1.0f, 0.0f};
                }

                // 施法!
                SpellFX_Cast(&g_Spells[idx], cmd.skill_id, start, dir, cmd.duration, cmd.vfx_param);
            }
        }

        // --- 决策：继续还是等待？---
        if (!cmd.is_parallel)
        {
            // 这是一个阻塞指令 (Wait)，停下来等待
            g_Busy = true;
            g_Timer = cmd.duration;
            break; // 跳出 while 循环
        }
        // 如果 is_parallel == true，循环继续，立即处理下一条指令！
    }
}

bool Animator_IsPlaying()
{
    return g_Busy || g_QueueCount > 0;
}