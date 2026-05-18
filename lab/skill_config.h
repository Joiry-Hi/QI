/**
 * skill_config.h
 * 29技能的实时参数配置 - 冷却/弹速/AOE范围等
 * 与 QI.h 的 SkillID 枚举一一对应
 */
#ifndef SKILL_CONFIG_H
#define SKILL_CONFIG_H

#include "QI.h"
#include "combat.h"

typedef struct {
    float cooldown_seconds;
    float cast_time;
    float projectile_speed;
    float projectile_lifetime;
    int   projectile_count;
    float hitbox_radius;
    int   damage_scalar;     // 伤害倍率 (base_power * ATK * scalar)
    HitboxType hitbox_type;
    float aoe_radius;
    float slash_arc;
    bool  is_self_target;
    bool  is_homing;
    float homing_strength;
    int   qi_regen_bonus;
    int   heal_amount_scalar; // heal = Yuan * scalar
    int   buff_enraged_amt;   // enraged增加量
    float buff_duration;
    bool  is_charge_skill;    // 需要引导的集气类技能
} RealTimeSkillConfig;

extern RealTimeSkillConfig g_rt_skill_config[TOTAL_SKILLS];

void SkillConfig_Init();

#endif
