// vfx.h

#ifndef VFX_H
#define VFX_H

#include "engine.h"
#include "QI.h"

// 定义最大支持的子实体数量 (例如最多 12 把飞剑)
#define MAX_SUB_FX 12

// 子实体结构：每一把飞剑都有独立的物理状态
typedef struct {
    bool active;
    Vector2 position;
    Vector2 velocity;
    // 可以增加一个相位偏移，让它们“呼吸”频率不同
    float phase_offset; 
} SubEntity;


// Caster 定义 (保持不变)
typedef struct
{
    Vector2 position;
    SDL_Color color;
    float energy_radius;
    float animation_timer;
} Caster;

Caster Caster_Create(float x, float y, SDL_Color color);
void Caster_Update(Caster *c);

// 视觉原型 ID
typedef enum
{
    VFX_NONE = 0,
    VFX_GATHER_QI,
    VFX_BUFF_AURA,
    VFX_PROJECTILE,
    VFX_DRILL_SHOT,
    VFX_SLASH,
    VFX_WIND_BLADE, // 独立的风刃类型
    VFX_HEAVY_SMASH,
    VFX_SWARM,
    VFX_THUNDER_STRIKE,
    VFX_SWORD_ARRAY,
    VFX_SHIELD,
    VFX_HOMING_MISSILE // <--- 新增：灵剑/追踪导弹
} VisualSpellID;

typedef struct {
    VisualSpellID id;
    bool active;
    
    // 原有的属性保留，作为“中心”或“领队”
    Vector2 position;
    Vector2 start_pos;
    Vector2 target_pos;
    Vector2 direction;
    Vector2 velocity; 
    
    int duration;
    int max_duration;
    AttributeID attribute;
    int caster_idx;
    int param; 

    // --- 新增：子实体数组 ---
    // 用于处理多重灵剑、多重虫群等
    SubEntity subs[MAX_SUB_FX];
    int sub_count; // 当前激活的子实体数量

} SpellFX;

// 注意：函数名已按你的要求修改
void SpellFX_Cast(SpellFX *spellfx, SkillID skill_id, Vector2 pos, Vector2 dir, int duration, int param);
void SpellFX_Update(SpellFX *spellfx);

#endif // VFX_H