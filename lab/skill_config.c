/**
 * skill_config.c
 * 29技能实时参数配置表
 * 索引: SkillID (与 QI.h 枚举严格对应)
 */
#include "skill_config.h"

RealTimeSkillConfig g_rt_skill_config[TOTAL_SKILLS];

void SkillConfig_Init()
{
    // 先全部清零
    for (int i = 0; i < TOTAL_SKILLS; i++) {
        g_rt_skill_config[i] = (RealTimeSkillConfig){0};
    }

    // ================================================================
    // 凡人期 (Rank 0)
    // ================================================================

    // SKILL_ID_GAIN_QI — 集气 (引导)
    g_rt_skill_config[SKILL_ID_GAIN_QI] = (RealTimeSkillConfig){
        .cooldown_seconds = 1.0f, .cast_time = 0.5f,
        .hitbox_radius = 0, .damage_scalar = 0,
        .hitbox_type = HITBOX_AOE, .is_self_target = true,
        .is_charge_skill = true, .qi_regen_bonus = 0,
    };

    // SKILL_ID_STRIKE — 轻击 (近战弧斩)
    g_rt_skill_config[SKILL_ID_STRIKE] = (RealTimeSkillConfig){
        .cooldown_seconds = 0.5f, .cast_time = 0.0f,
        .projectile_speed = 0, .projectile_lifetime = 0.15f,
        .projectile_count = 1, .hitbox_radius = 60.0f,
        .damage_scalar = 1, .hitbox_type = HITBOX_SLASH,
        .slash_arc = 1.5f,
    };

    // SKILL_ID_DEFEND — 防御 (自身减伤护盾)
    g_rt_skill_config[SKILL_ID_DEFEND] = (RealTimeSkillConfig){
        .cooldown_seconds = 3.0f, .cast_time = 0.0f,
        .projectile_lifetime = 2.0f, .hitbox_radius = 55.0f,
        .damage_scalar = 0, .hitbox_type = HITBOX_AOE,
        .aoe_radius = 55.0f, .is_self_target = true,
    };

    // SKILL_ID_HEAL — 养元 (自疗)
    g_rt_skill_config[SKILL_ID_HEAL] = (RealTimeSkillConfig){
        .cooldown_seconds = 5.0f, .cast_time = 0.3f,
        .hitbox_radius = 0, .damage_scalar = 0,
        .hitbox_type = HITBOX_AOE, .is_self_target = true,
        .heal_amount_scalar = 2,
    };

    // SKILL_ID_WARCRY — 战吼 (降低周围敌人ATK,自身enrage)
    g_rt_skill_config[SKILL_ID_WARCRY] = (RealTimeSkillConfig){
        .cooldown_seconds = 8.0f, .cast_time = 0.1f,
        .projectile_lifetime = 0.5f, .hitbox_radius = 120.0f,
        .damage_scalar = 0, .hitbox_type = HITBOX_AOE,
        .aoe_radius = 120.0f, .is_self_target = true,
        .buff_enraged_amt = 3, .buff_duration = 5.0f,
    };

    // SKILL_ID_PARRY — 格挡 (反击架势)
    g_rt_skill_config[SKILL_ID_PARRY] = (RealTimeSkillConfig){
        .cooldown_seconds = 6.0f, .cast_time = 0.0f,
        .projectile_lifetime = 1.5f, .hitbox_radius = 50.0f,
        .damage_scalar = 0, .hitbox_type = HITBOX_AOE,
        .aoe_radius = 50.0f, .is_self_target = true,
    };

    // SKILL_ID_SMITE — 重击 (单体猛击AOE)
    g_rt_skill_config[SKILL_ID_SMITE] = (RealTimeSkillConfig){
        .cooldown_seconds = 2.0f, .cast_time = 0.2f,
        .projectile_lifetime = 0.3f, .hitbox_radius = 80.0f,
        .damage_scalar = 4, .hitbox_type = HITBOX_AOE,
        .aoe_radius = 80.0f,
    };

    // ================================================================
    // 炼气期 (Rank 1)
    // ================================================================

    // SKILL_ID_FIREBALL — 火球
    g_rt_skill_config[SKILL_ID_FIREBALL] = (RealTimeSkillConfig){
        .cooldown_seconds = 0.8f, .cast_time = 0.05f,
        .projectile_speed = 450.0f, .projectile_lifetime = 2.0f,
        .projectile_count = 1, .hitbox_radius = 8.0f,
        .damage_scalar = 1, .hitbox_type = HITBOX_PROJECTILE,
    };

    // SKILL_ID_ENERGY_SHIELD — 灵力盾
    g_rt_skill_config[SKILL_ID_ENERGY_SHIELD] = (RealTimeSkillConfig){
        .cooldown_seconds = 6.0f, .cast_time = 0.1f,
        .projectile_lifetime = 3.0f, .hitbox_radius = 65.0f,
        .damage_scalar = 0, .hitbox_type = HITBOX_AOE,
        .aoe_radius = 65.0f, .is_self_target = true,
    };

    // SKILL_ID_WINDBLADE — 风刃 (多段散射)
    g_rt_skill_config[SKILL_ID_WINDBLADE] = (RealTimeSkillConfig){
        .cooldown_seconds = 1.5f, .cast_time = 0.0f,
        .projectile_speed = 380.0f, .projectile_lifetime = 1.5f,
        .projectile_count = 3, .hitbox_radius = 10.0f,
        .damage_scalar = 1, .hitbox_type = HITBOX_PROJECTILE,
    };

    // SKILL_ID_EVERGREEN_ART — 长春功 (强效治疗+净化)
    g_rt_skill_config[SKILL_ID_EVERGREEN_ART] = (RealTimeSkillConfig){
        .cooldown_seconds = 10.0f, .cast_time = 0.5f,
        .hitbox_radius = 0, .damage_scalar = 0,
        .hitbox_type = HITBOX_AOE, .is_self_target = true,
        .heal_amount_scalar = 5,
    };

    // SKILL_ID_CONCENTRATION — 凝神 (闪避率大幅提升+enrage)
    g_rt_skill_config[SKILL_ID_CONCENTRATION] = (RealTimeSkillConfig){
        .cooldown_seconds = 8.0f, .cast_time = 0.1f,
        .projectile_lifetime = 0.5f, .hitbox_radius = 60.0f,
        .damage_scalar = 0, .hitbox_type = HITBOX_AOE,
        .is_self_target = true, .buff_enraged_amt = 2,
        .buff_duration = 4.0f,
    };

    // ================================================================
    // 筑基期 (Rank 2)
    // ================================================================

    // SKILL_ID_FLAMEBLAST — 炎爆弹 (大火球)
    g_rt_skill_config[SKILL_ID_FLAMEBLAST] = (RealTimeSkillConfig){
        .cooldown_seconds = 2.5f, .cast_time = 0.15f,
        .projectile_speed = 350.0f, .projectile_lifetime = 2.0f,
        .projectile_count = 1, .hitbox_radius = 16.0f,
        .damage_scalar = 2, .hitbox_type = HITBOX_PROJECTILE,
    };

    // SKILL_ID_GOLD_LIGHT_WARDING — 金光护体 (强力力场)
    g_rt_skill_config[SKILL_ID_GOLD_LIGHT_WARDING] = (RealTimeSkillConfig){
        .cooldown_seconds = 10.0f, .cast_time = 0.2f,
        .projectile_lifetime = 4.0f, .hitbox_radius = 80.0f,
        .damage_scalar = 0, .hitbox_type = HITBOX_AOE,
        .aoe_radius = 80.0f, .is_self_target = true,
    };

    // SKILL_ID_GREATSWORD — 巨剑术 (大范围AOE)
    g_rt_skill_config[SKILL_ID_GREATSWORD] = (RealTimeSkillConfig){
        .cooldown_seconds = 4.0f, .cast_time = 0.3f,
        .projectile_lifetime = 0.4f, .hitbox_radius = 120.0f,
        .damage_scalar = 4, .hitbox_type = HITBOX_AOE,
        .aoe_radius = 120.0f,
    };

    // SKILL_ID_COMMANDING_SWORDS — 灵剑 (追踪导弹)
    g_rt_skill_config[SKILL_ID_COMMANDING_SWORDS] = (RealTimeSkillConfig){
        .cooldown_seconds = 3.0f, .cast_time = 0.1f,
        .projectile_speed = 350.0f, .projectile_lifetime = 3.0f,
        .projectile_count = 3, .hitbox_radius = 8.0f,
        .damage_scalar = 2, .hitbox_type = HITBOX_PROJECTILE,
        .is_homing = true, .homing_strength = 0.8f,
    };

    // SKILL_ID_TERMINATE_THUNDER — 唤雷 (BEAM射线)
    g_rt_skill_config[SKILL_ID_TERMINATE_THUNDER] = (RealTimeSkillConfig){
        .cooldown_seconds = 8.0f, .cast_time = 0.25f,
        .projectile_lifetime = 0.1f, .hitbox_radius = 15.0f,
        .damage_scalar = 5, .hitbox_type = HITBOX_BEAM,
    };

    // ================================================================
    // 结丹期 (Rank 3)
    // ================================================================

    // SKILL_ID_SWORD_PHANTOM — 剑影分光术 (多发追踪灵剑)
    g_rt_skill_config[SKILL_ID_SWORD_PHANTOM] = (RealTimeSkillConfig){
        .cooldown_seconds = 4.0f, .cast_time = 0.15f,
        .projectile_speed = 400.0f, .projectile_lifetime = 3.5f,
        .projectile_count = 6, .hitbox_radius = 6.0f,
        .damage_scalar = 1, .hitbox_type = HITBOX_PROJECTILE,
        .is_homing = true, .homing_strength = 0.6f,
    };

    // SKILL_ID_BLOOD_DEVIL_DRILL — 血魔钻 (穿透+流血)
    g_rt_skill_config[SKILL_ID_BLOOD_DEVIL_DRILL] = (RealTimeSkillConfig){
        .cooldown_seconds = 4.0f, .cast_time = 0.2f,
        .projectile_speed = 600.0f, .projectile_lifetime = 2.5f,
        .projectile_count = 1, .hitbox_radius = 10.0f,
        .damage_scalar = 5, .hitbox_type = HITBOX_PROJECTILE,
    };

    // SKILL_ID_CORE_RESTORATION — 丹元归一 (大治疗+净化)
    g_rt_skill_config[SKILL_ID_CORE_RESTORATION] = (RealTimeSkillConfig){
        .cooldown_seconds = 15.0f, .cast_time = 0.6f,
        .hitbox_radius = 0, .damage_scalar = 0,
        .hitbox_type = HITBOX_AOE, .is_self_target = true,
        .heal_amount_scalar = 6,
    };

    // SKILL_ID_CORE_ERUPTION — 丹元爆发 (自伤+巨幅增伤)
    g_rt_skill_config[SKILL_ID_CORE_ERUPTION] = (RealTimeSkillConfig){
        .cooldown_seconds = 20.0f, .cast_time = 0.1f,
        .projectile_lifetime = 0.3f, .hitbox_radius = 100.0f,
        .damage_scalar = 0, .hitbox_type = HITBOX_AOE,
        .aoe_radius = 100.0f, .is_self_target = true,
        .buff_enraged_amt = 6, .buff_duration = 8.0f,
    };

    // SKILL_ID_BLOOD_DEVIL_SLASH — 血魔斩 (大弧斩+流血)
    g_rt_skill_config[SKILL_ID_BLOOD_DEVIL_SLASH] = (RealTimeSkillConfig){
        .cooldown_seconds = 3.0f, .cast_time = 0.1f,
        .projectile_speed = 0, .projectile_lifetime = 0.2f,
        .projectile_count = 1, .hitbox_radius = 150.0f,
        .damage_scalar = 10, .hitbox_type = HITBOX_SLASH,
        .slash_arc = 2.5f,
    };

    // SKILL_ID_BEETLE_SWARM — 噬金虫群 (多发散射投射物)
    g_rt_skill_config[SKILL_ID_BEETLE_SWARM] = (RealTimeSkillConfig){
        .cooldown_seconds = 2.5f, .cast_time = 0.1f,
        .projectile_speed = 250.0f, .projectile_lifetime = 3.0f,
        .projectile_count = 12, .hitbox_radius = 5.0f,
        .damage_scalar = 1, .hitbox_type = HITBOX_PROJECTILE,
    };

    // SKILL_ID_ICE_FLAME — 乾蓝冰焰 (慢速穿透冻结弹)
    g_rt_skill_config[SKILL_ID_ICE_FLAME] = (RealTimeSkillConfig){
        .cooldown_seconds = 12.0f, .cast_time = 0.3f,
        .projectile_speed = 200.0f, .projectile_lifetime = 5.0f,
        .projectile_count = 1, .hitbox_radius = 14.0f,
        .damage_scalar = 3, .hitbox_type = HITBOX_PROJECTILE,
    };

    // ================================================================
    // 元婴期 (Rank 4)
    // ================================================================

    // SKILL_ID_IMMOVABLE_KING — 不动明王 (超强防御光环)
    g_rt_skill_config[SKILL_ID_IMMOVABLE_KING] = (RealTimeSkillConfig){
        .cooldown_seconds = 15.0f, .cast_time = 0.3f,
        .projectile_lifetime = 5.0f, .hitbox_radius = 90.0f,
        .damage_scalar = 0, .hitbox_type = HITBOX_AOE,
        .aoe_radius = 90.0f, .is_self_target = true,
    };

    // SKILL_ID_STELLAR_SHIFT — 斗转星移 (反射护盾)
    g_rt_skill_config[SKILL_ID_STELLAR_SHIFT] = (RealTimeSkillConfig){
        .cooldown_seconds = 12.0f, .cast_time = 0.1f,
        .projectile_lifetime = 2.0f, .hitbox_radius = 70.0f,
        .damage_scalar = 0, .hitbox_type = HITBOX_AOE,
        .aoe_radius = 70.0f, .is_self_target = true,
    };

    // SKILL_ID_ESSENCE_PLUNDER — 夺元诀 (AOE吸QI)
    g_rt_skill_config[SKILL_ID_ESSENCE_PLUNDER] = (RealTimeSkillConfig){
        .cooldown_seconds = 10.0f, .cast_time = 0.3f,
        .projectile_lifetime = 0.8f, .hitbox_radius = 200.0f,
        .damage_scalar = 0, .hitbox_type = HITBOX_AOE,
        .aoe_radius = 200.0f, .qi_regen_bonus = 5,
    };

    // SKILL_ID_COSMIC_DHARMA_AVATAR — 法相天地 (超巨大AOE)
    g_rt_skill_config[SKILL_ID_COSMIC_DHARMA_AVATAR] = (RealTimeSkillConfig){
        .cooldown_seconds = 15.0f, .cast_time = 0.5f,
        .projectile_lifetime = 0.5f, .hitbox_radius = 250.0f,
        .damage_scalar = 40, .hitbox_type = HITBOX_AOE,
        .aoe_radius = 250.0f,
    };

    // SKILL_ID_GREAT_GOLDEN_SWORDFORMATION — 大庚剑阵 (持续旋转AOE)
    g_rt_skill_config[SKILL_ID_GREAT_GOLDEN_SWORDFORMATION] = (RealTimeSkillConfig){
        .cooldown_seconds = 30.0f, .cast_time = 0.4f,
        .projectile_lifetime = 5.0f, .hitbox_radius = 150.0f,
        .damage_scalar = 2, .hitbox_type = HITBOX_AOE,
        .aoe_radius = 150.0f, .projectile_count = 3,
    };

    // ================================================================
    // 化神期 (Rank 5)
    // ================================================================

    // SKILL_ID_SPIRIT_SLAYING_SWORD — 玄天灭灵斩 (超大弧斩+因果属性)
    g_rt_skill_config[SKILL_ID_SPIRIT_SLAYING_SWORD] = (RealTimeSkillConfig){
        .cooldown_seconds = 20.0f, .cast_time = 0.3f,
        .projectile_speed = 0, .projectile_lifetime = 0.25f,
        .projectile_count = 1, .hitbox_radius = 400.0f,
        .damage_scalar = 100, .hitbox_type = HITBOX_SLASH,
        .slash_arc = 3.0f,
    };
}
