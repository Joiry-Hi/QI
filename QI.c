#include "QI.h"

#pragma region Global Variable Definitions
// ---  ---
Player CPU = {"CPU", 10, 0, 1, 1, 0, -1, 1, 0, 0, 0, 0.00f, -1, 0, 0};
Player YOU = {"You", 10, 0, 1, 1, 0, -1, 1, 0, 0, 0, 0.00f, -1, 0, 0};
Game game = {0, ' ', -1};

GameConfig g_config = {
    .initial_hp = 10,
    .initial_qi = 0,
    .initial_xiuwei = 0,
    .initial_evade = 0.05f,
    .train_reps = 1000,
    .enemy_type = -1,
    .ai_type = -1,
    .enable_ai_randomness = 1,
    .world = 0,
    .time_delay = 1,
};

char *Realm[TOTAL_XIUWEI_LEVEL] = {"凡人", "炼气", "筑基", "结丹", "元婴", "化神", "炼虚", "合体", "大乘", "飞升", "真仙"};
char *Eng_Realm[TOTAL_XIUWEI_LEVEL] = {"Mortal", "Qi Refining", "Foundation", "Core Formation", "Nascent Soul", "Spirit Severing", "Void Refinement", "Unity", "Great Ascension", "Ascension", "Immortal"};
int max_HP[TOTAL_XIUWEI_LEVEL] = {10, 20, 50, 200, 1000, 5000, 20000, 100000, 500000, 1000000, 99999999};
int max_QI[TOTAL_XIUWEI_LEVEL] = {10, 20, 40, 80, 160, 320, 640, 1280, 2560, 5120, 9999999};
int Yuan[TOTAL_XIUWEI_LEVEL] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 99999999};

const char *GENERAL_NAMES[] = {
    "Disruptor", // ID 0
    "Berserker", // ID 1
    "Turtle",    // ID 2
    "Ascetic",   // ID 3
    "Gambler",   // ID 4
    "Random"     // ID 5 (用于default case)
};

const char *Eng_Root_Names[TOTAL_ROOT_TYPES] = {
    "Mortal Root",
    "Heavenly Root",
    "Solid Soil Root",
    "Sharp Metal Root",
    "Ethereal Wind Root"};

const char *CHN_Root_Names[TOTAL_ROOT_TYPES] = {
    "凡根",
    "天灵根",
    "厚土灵根",
    "锐金灵根",
    "风灵根"};

// 全局的AI权重，包含默认值。这些值将在游戏结束后被修改。
AI_Weights g_ai_weights = {
    // --- 原有权重 ---
    1.0f,    // w_health_urgency
    1.0f,    // w_damage_per_point
    100.0f,  // w_kill_shot_bonus
    10.0f,   // w_interrupt_heal_bonus
    1000.0f, // w_low_qi_gather
    10.0f,   // w_defend_vs_high_qi
    1000.0f, // w_qi_advantage
    900.0f,  // w_damage_per_qi
    10.0f,   // w_low_hp_penalty (这个惩罚值可以设高一些)
    900.0f,  // w_breakthrough_urgency (新增的“攒气突破”渴望权重)
    30.0f};

AI_Weights g_ai_weights_A[10] = {
    {         // --- 狂才 ---
     1.0f,    // w_health_urgency
     1.0f,    // w_damage_per_point
     100.0f,  // w_kill_shot_bonus
     10.0f,   // w_interrupt_heal_bonus
     1000.0f, // w_low_qi_gather
     10.0f,   // w_defend_vs_high_qi
     1000.0f, // w_qi_advantage
     900.0f,  // w_damage_per_qi
     10.0f,   // w_low_hp_penalty (这个惩罚值可以设高一些)
     900.0f,  // w_breakthrough_urgency (新增的“攒气突破”渴望权重)
     30.0f},
};

AI_TurnLog g_game_log[MAX_LOG_TURNS];
int g_log_count = 0;

static int total_games_played = 0;
static int player_wins = 0;

// 静态技能数据库，定义了游戏中的所有技能规则
Skill g_skill_database[TOTAL_SKILLS];

#pragma endregion Global Variable Definitions

// --- Database Initialization ---
void Initialize_Databases()
{
    g_skill_database[SKILL_ID_GAIN_QI] = (Skill){SKILL_ID_GAIN_QI, ACTION_TYPE_GAIN_QI, "集气", "Gain QI", 'Q', 0, 0, TYPE_BUFF, ATTR_NONE, 1.0f, 0, 0, TARGET_SELF, "<%s 开始引导天地灵气...>", "<%s begins to channel worldly energy...>"};
    g_skill_database[SKILL_ID_STRIKE] = (Skill){SKILL_ID_STRIKE, ACTION_TYPE_MELEE, "轻击", "Melee", 'A', 1, 0, TYPE_SLASH, ATTR_PHYSICAL, 1.0f, 0, 0, TARGET_ENEMY, "<%s 发动了迅捷的攻击!>", "<%s unleashes a swift strike!>"};
    g_skill_database[SKILL_ID_DEFEND] = (Skill){SKILL_ID_DEFEND, ACTION_TYPE_DEFEND, "防御", "Defend", 'D', 1, 0, TYPE_RESIST, ATTR_NONE, 0.6f, 0, 0, TARGET_SELF, "<%s 摆开了防御架势!>", "<%s raises a defensive guard!>"};
    g_skill_database[SKILL_ID_HEAL] = (Skill){SKILL_ID_HEAL, ACTION_TYPE_HEAL, "养元", "Heal", 'H', 1, 0, TYPE_HEAL, ATTR_WOOD, 1.0f, 0, 0, TARGET_SELF, "<%s 开始运气修复全身经脉>", "<%s starts to circulate Qi to repair the body's meridians>"};
    g_skill_database[SKILL_ID_WARCRY] = (Skill){SKILL_ID_WARCRY, ACTION_TYPE_BOOST, "战吼", "Warcry", 'C', 2, 0, TYPE_DEBUFF, ATTR_NONE, 1.0f, 0, 0, TARGET_ENEMY, "<%s 发出一声震慑心魄的战吼!>", "<%s lets out a deafening warcry!>"};
    g_skill_database[SKILL_ID_PARRY] = (Skill){SKILL_ID_PARRY, ACTION_TYPE_PARRY, "格挡", "Parry", 'P', 2, 0, TYPE_PARRY, ATTR_PHYSICAL, 0.5f, 0, 0, TARGET_SELF, "<%s 眼神一凝，摆出了完美的格挡架势。>", "<%s takes a perfect parry stance, ready to counter!>"};
    g_skill_database[SKILL_ID_SMITE] = (Skill){SKILL_ID_SMITE, ACTION_TYPE_SMITE, "重击", "Smite", 'S', 3, 0, TYPE_SMASH, ATTR_PHYSICAL, 4.0f, 0, 0, TARGET_ENEMY, "<%s 汇聚全身力气，发动了沉重的猛击!>", "<%s channels their strength into a powerful smite!>"};

    g_skill_database[SKILL_ID_FIREBALL] = (Skill){SKILL_ID_FIREBALL, ACTION_TYPE_RANGED, "火球", "Fireball", 'R', 2, 1, TYPE_PROJECT, ATTR_FIRE, 1.0f, 0, 0, TARGET_ENEMY, "<%s 掌心凝聚出一颗灼热的火球!>", "<%s conjures and hurls a searing fireball!>"};
    g_skill_database[SKILL_ID_ENERGY_SHIELD] = (Skill){SKILL_ID_ENERGY_SHIELD, ACTION_TYPE_DEFEND, "灵力盾", "Energy Shield", 'D', 2, 1, TYPE_SHIELD, ATTR_SPIRITUAL, 5.0f, 0, 0, TARGET_SELF, "<%s 以灵力在身前构筑了一面无形的护盾!>", "<%s summons a shimmering shield of pure energy!>"};
    g_skill_database[SKILL_ID_EVERGREEN_ART] = (Skill){SKILL_ID_EVERGREEN_ART, ACTION_TYPE_HEAL, "长春功", "Evergreen Art", 'H', 4, 1, TYPE_HEAL, ATTR_WOOD, 2.5f, 3, 1.0f, TARGET_SELF, "<%s 运转长春功，周身泛起绿色荧光，生命力缓缓恢复。>", "<%s channels the Evergreen Art, glowing with a green light as life force recovers.>"};
    g_skill_database[SKILL_ID_CONCENTRATION] = (Skill){SKILL_ID_CONCENTRATION, ACTION_TYPE_BOOST, "凝神", "Get Focused", 'C', 3, 1, TYPE_BUFF, ATTR_SPIRITUAL, 1.0f, 0, 0, TARGET_SELF, "<%s 凝气入体，聚精会神>", "<%s saves all his spirits for next move>"};
    g_skill_database[SKILL_ID_WINDBLADE] = (Skill){SKILL_ID_WINDBLADE, ACTION_TYPE_BURST, "风刃", "Wind Blade", 'B', 3, 1, TYPE_BURST, ATTR_WIND, 1.0f, 0, 0, TARGET_ENEMY, "<%s 挥手间，数道锋利的风刃破空而去!>", "<%s unleashes a flurry of razor-sharp wind blades!>"};

    g_skill_database[SKILL_ID_FLAMEBLAST] = (Skill){SKILL_ID_FLAMEBLAST, ACTION_TYPE_RANGED, "炎爆弹", "Flame Blast", 'R', 3, 2, TYPE_BLAST, ATTR_FIRE, 2.0f, 0, 0, TARGET_ENEMY, "<%s 吟唱片刻，一颗毁灭性的炎爆弹呼啸而出!>", "<%s unleashes a devastating blast of fire and flame!>"};
    g_skill_database[SKILL_ID_GOLD_LIGHT_WARDING] = (Skill){SKILL_ID_GOLD_LIGHT_WARDING, ACTION_TYPE_DEFEND, "金光护体", "Gold Light Warding", 'D', 4, 2, TYPE_FORCEFIELD, ATTR_LIGHT, 1.0f, 0, 0, TARGET_SELF, "<%s 周身金光大盛，形成了坚不可摧的护体神功!>", "<%s is enveloped in a brilliant ward of golden light!>"};
    g_skill_database[SKILL_ID_GREATSWORD] = (Skill){SKILL_ID_GREATSWORD, ACTION_TYPE_SMITE, "巨剑术", "Greatsword Mastery", 'S', 6, 2, TYPE_SMASH, ATTR_METAL, 4.0f, 0, 0, TARGET_ENEMY, "<%s 凝聚出一柄巨剑，携万钧之势劈下!>", "<%s forms a Greatsword and smites with immense force!>"};
    g_skill_database[SKILL_ID_COMMANDING_SWORDS] = (Skill){SKILL_ID_COMMANDING_SWORDS, ACTION_TYPE_BURST, "灵剑", "Soul Swords", 'B', 3, 2, TYPE_BURST, ATTR_PHYSICAL, 1.5f, 0, 0, TARGET_ENEMY, "<%s 轻喝一声：“剑来！”，无数飞剑应声而至!>", "<%s utters a single command: \"Swords, arise!\", and countless blades answer the call!>"};
    g_skill_database[SKILL_ID_TERMINATE_THUNDER] = (Skill){SKILL_ID_TERMINATE_THUNDER, ACTION_TYPE_TERMINATE, "唤雷", "Thunderbolt", 'T', 10, 2, TYPE_SMASH, ATTR_THUNDER, 5.0f, 0, 0, TARGET_ENEMY, "<%s 吟诵咒语，引九天神雷轰向敌人!>", "<%s chants and evokes thunder from the heavens!>"};

    g_skill_database[SKILL_ID_SWORD_PHANTOM] = (Skill){SKILL_ID_SWORD_PHANTOM, ACTION_TYPE_MELEE, "剑影分光术", "Sword Phantom Art", 'A', 6, 3, TYPE_BURST, ATTR_WIND, 0.5f, 0, 0, TARGET_ENEMY, "<%s 剑诀一引，一道剑光骤然分化为万千幻影，如狂风骤雨般射向所有敌人！>", "<%s summons the art, splitting one sword light into myriad phantoms that rain upon all foes!>"};
    g_skill_database[SKILL_ID_CORE_RESTORATION] = (Skill){SKILL_ID_CORE_RESTORATION, ACTION_TYPE_HEAL, "丹元归一", "Golden Core Restoration", 'H', 10, 3, TYPE_HEAL, ATTR_LIGHT, 6.5f, 2, 0.5f, TARGET_SELF, "<%s 催动金丹，精纯的丹元之力流转百骸，迅速修复着受损的经脉。>", "<%s activates the Golden Core, its pure essence flowing through the meridians, rapidly mending the damage.>"};
    g_skill_database[SKILL_ID_CORE_ERUPTION] = (Skill){SKILL_ID_CORE_ERUPTION, ACTION_TYPE_BOOST, "丹元爆发", "Core Eruption", 'C', 0, 3, TYPE_BUFF, ATTR_NONE, 2.0f, 2, 0, TARGET_SELF, "<%s 碎丹求道，霎时气场狂涨！>", "<%s shattering the Core, his presence surged with terrifying might.>"};
    g_skill_database[SKILL_ID_BLOOD_DEVIL_SLASH] = (Skill){SKILL_ID_BLOOD_DEVIL_SLASH, ACTION_TYPE_SMITE, "血魔斩", "Blood-Devil Slash", 'X', 12, 3, TYPE_SLASH, ATTR_BLOOD, 10.0f, 3, 0.4f, TARGET_ENEMY, "<%s 催动魔功，一道血色斩击裹挟着无尽凶煞之气，当头劈下！>", "<%s channels demonic power, unleashing a blood-colored slash filled with baleful energy!>"};
    g_skill_database[SKILL_ID_BEETLE_SWARM] = (Skill){SKILL_ID_BEETLE_SWARM, ACTION_TYPE_BURST, "噬金虫群", "Gold Devouring Beetle Swarm", 'B', 3, 3, TYPE_BURST, ATTR_PHYSICAL, 1.0f, 3, 0.8f, TARGET_ENEMY, "<%s 放出噬金虫群，嗡鸣声中，一片金色虫云遮天蔽日般扑去！>", "<%s releases a swarm of Gold Devouring Beetles, a golden cloud that blots out the sky and engulfs the enemy!>"};
    g_skill_database[SKILL_ID_ICE_FLAME] = (Skill){SKILL_ID_ICE_FLAME, ACTION_TYPE_TERMINATE, "乾蓝冰焰", "Dry Blue Ice Flame", 'I', 20, 3, TYPE_DEBUFF, ATTR_ICE, 3.0f, 5, 0.95f, TARGET_ENEMY, "<%s 祭出乾蓝冰焰，一朵幽蓝冷火悄然印向对手，欲冻结其元神！>", "<%s summons the Dry Blue Ice Flame, a chilling blue fire that silently seeks to freeze the enemy's spirit!>"};

    g_skill_database[SKILL_ID_IMMOVABLE_KING] = (Skill){SKILL_ID_IMMOVABLE_KING, ACTION_TYPE_DEFEND, "不动明王", "Immovable Bright King", 'M', 10, 4, TYPE_RESIST, ATTR_METAL, 0.2f, 3, 1.0f, TARGET_SELF, "<%s 宝相庄严，体表浮现金色梵文，化为不动明王法身，稳如山岳！>", "<%s's body glows with golden sanskrit, forming the Immovable King's aegis, as steadfast as a mountain!>"};
    g_skill_database[SKILL_ID_STELLAR_SHIFT] = (Skill){SKILL_ID_STELLAR_SHIFT, ACTION_TYPE_PARRY, "斗转星移", "Stellar Shift", 'S', 10, 8, TYPE_PARRY, ATTR_SPIRITUAL, 0.8f, 0, 0, TARGET_SELF, "<%s 身形一晃，引动敌方攻势，周天星斗仿佛随之逆转！>", "<%s's form blurs, redirecting the attack as if shifting the very stars in the cosmos!>"};
    g_skill_database[SKILL_ID_ESSENCE_PLUNDER] = (Skill){SKILL_ID_ESSENCE_PLUNDER, ACTION_TYPE_BOOST, "夺元诀", "Essence Plundering Art", 'C', 20, 4, TYPE_DEBUFF, ATTR_DARK, 1.5f, 6, 0, TARGET_ENEMY, "<%s 运起诡异步法，掌心产生一股吸力，强行夺取对手正在汇聚的真元！>", "<%s uses an uncanny art, creating a vortex in their palm to forcibly plunder the opponent's gathering essence!>"};
    g_skill_database[SKILL_ID_GREAT_GOLDEN_SWORDFORMATION] = (Skill){SKILL_ID_GREAT_GOLDEN_SWORDFORMATION, ACTION_TYPE_TERMINATE, "大庚剑阵", "Great Golden Sword Formation", 'G', 100, 4, TYPE_BURST, ATTR_METAL, 2.0f, 3, 1.0f, TARGET_ENEMY, "<%s 祭出飞剑，引动天地庚金之气，一座无边剑阵瞬间成型，将对手困入其中！>", "<%s summons his swords, forming a vast formation of golden energy that traps the enemy within!>"};
}

// --- BLUEPRINT REFACTOR: The New Combat Resolution Engine ---
void Oneway_Solution(Player *attacker, Player *defender)
{
    // --- 步骤 1: 获取双方的技能实例 ---
    // 如果任何一方没有行动，则直接结束
    if (attacker->current_action_type == ACTION_TYPE_NONE || defender->current_action_type == ACTION_TYPE_NONE)
    {
        return;
    }
    const Skill *attacker_skill = &attacker->learned_skills[attacker->current_action_type];

    // --- 步骤 2: 处理非交互性技能 ---
    // 如果攻击方的技能目标是自己 (如治疗、格挡架势)，则它不与防御方发生交互
    if (attacker_skill->target_type == TARGET_SELF)
    {
        return;
    }
    else
    {
        attacker->healing = 0;
    }

    // --- 步骤 3: 前置检定 (闪避) ---
    // 只有特定类型的攻击可以被闪避
    if (attacker_skill->type_id == TYPE_PIERCE || attacker_skill->type_id == TYPE_SLASH || attacker_skill->type_id == TYPE_SMASH || attacker_skill->type_id == TYPE_PROJECT)
    {
        if ((rand() % 100) < (defender->evade * 100.0f))
        {
            ENG_PRINT("\033[36m[%s's %s was gracefully evaded by %s!]\n", attacker->name, attacker_skill->name_eng, defender->name);
            CHN_PRINT("\033[36m[%s 的 %s 被 %s 灵巧地闪避了！]\n", attacker->name, attacker_skill->name_chn, defender->name);
            return; // 闪避成功，交互结束
        }
    }

    if (attacker_skill->type_id == TYPE_BURST)
    {
        int origin_burst_count = attacker->burst_count;
        while (origin_burst_count-- > 0)
            if ((rand() % 100) < (defender->evade * 100.0f))
                attacker->burst_count--;
        CHN_PRINT("[%s 被 %s 命中 %d 次!]\n", defender->name, attacker_skill->name_chn, attacker->burst_count);
        ENG_PRINT("[%s was hit by %s %d time(s)!]\n", defender->name, attacker_skill->name_eng, attacker->burst_count);
    }

    // --- 步骤 4: 核心结算流程 ---
    // 初始化最终伤害
    float final_damage = 0.0f;
    float reflect_damage = 0.0f; // 反弹伤害

    // 获取基础伤害
    float base_damage = attacker_skill->base_power * attacker->ATK;

    // 特殊处理 Burst 类型的伤害
    if (attacker_skill->type_id == TYPE_BURST)
    {
        base_damage *= attacker->burst_count;
    }

    // 从数据库获取防御方技能实例
    const Skill *defender_skill = &defender->learned_skills[defender->current_action_type];

    // **蓝图核心：基于防御方技能的 TypeID 进入不同的处理模板**
    switch (defender_skill->type_id)
    {
    case TYPE_RESIST:
        CHN_PRINT("[%s 使用 %s 来抵抗 %s 的 %s!]\n", defender->name, defender_skill->name_chn, attacker->name, attacker_skill->name_chn);
        ENG_PRINT("[%s uses %s to resist %s's %s!]\n", defender->name, defender_skill->name_eng, attacker->name, attacker_skill->name_eng);

        final_damage = base_damage * defender_skill->base_power;
        break;

    case TYPE_SHIELD: // 如果防御方在用“护盾”类技能...
        CHN_PRINT("[%s 使用 %s 来抵挡 %s 的 %s!]\n", defender->name, defender_skill->name_chn, attacker->name, attacker_skill->name_chn);
        ENG_PRINT("[%s uses %s to block %s's %s!]\n", defender->name, defender_skill->name_eng, attacker->name, attacker_skill->name_eng);

        int dmg_blocked = (base_damage > defender_skill->base_power * defender->ATK) ? defender_skill->base_power * defender->ATK : base_damage;
        final_damage = ((base_damage - dmg_blocked) > 0) ? (base_damage - dmg_blocked) : 0;
        break;

    case TYPE_PARRY: // 如果防御方在用“格挡/弹反”类技能...
        CHN_PRINT("[%s 试图 %s %s 的 %s!]\n", defender->name, defender_skill->name_chn, attacker->name, attacker_skill->name_chn);
        ENG_PRINT("[%s attempts to %s %s's %s!]\n", defender->name, defender_skill->name_eng, attacker->name, attacker_skill->name_eng);

        switch (attacker_skill->type_id)
        {
        case TYPE_SLASH: // 轻度的斩击会被完全弹反
            final_damage = base_damage * 0.2f;
            break;
        case TYPE_SMASH: // 沉重的重击无法被弹反，反而会破防
            final_damage = base_damage * 0.8f;
            break;
        default: // 其他攻击被部分格挡
            final_damage = base_damage * 0.6f;
            break;
        }
        if (attacker_skill->action_type != ACTION_TYPE_TERMINATE)
            reflect_damage = final_damage * defender_skill->base_power;
        break;

    case TYPE_FORCEFIELD:
        CHN_PRINT("[%s 发动 %s 来弹开 %s 的 %s!]\n", defender->name, defender_skill->name_chn, attacker->name, attacker_skill->name_chn);
        ENG_PRINT("[%s launched %s to scatter %s's %s!]\n", defender->name, defender_skill->name_eng, attacker->name, attacker_skill->name_eng);

        switch (attacker_skill->type_id)
        {
        case TYPE_BURST: // 爆发攻击几乎被尽数弹开
            final_damage = base_damage * 0.2f;
            break;
        case TYPE_PROJECT: // 投射物无法近身
            final_damage = base_damage * 0.2f;
            break;
        case TYPE_BLAST:
            final_damage = base_damage * 0.3f;
            break;
        default: // 其他攻击被部分格挡
            final_damage = base_damage * 0.6f;
            break;
        }
        break;

    // 如果防御方的技能不是防御性的 (例如，他也在攻击或集气)
    default:
        final_damage = base_damage;
        CHN_PRINT("[%s 的 %s 击中了正在发动 %s 的 %s!]\n", attacker->name, attacker_skill->name_chn, defender_skill->name_chn, defender->name);
        ENG_PRINT("[%s's %s hits %s who is using %s!]\n", attacker->name, attacker_skill->name_eng, defender_skill->name_chn, defender->name);
        break;
    }

    // --- 步骤 5: 应用灵根和特殊效果修正 ---
    if (attacker->root == ROOT_Sharp && (attacker_skill->attribute_id == ATTR_PHYSICAL || attacker_skill->attribute_id == ATTR_WIND))
    {
        final_damage *= 1.2f; // 锐金灵根使用物理/风系技能伤害增加
    }
    if (defender->root == ROOT_Solid && defender_skill->type_id == TYPE_SHIELD)
    {
        final_damage *= 0.8f; // 厚土灵根使用护盾技能时，减伤效果更强
    }

    // --- 步骤 6: 根据技能属性施加效果 ---
    int QI_stolen = 0;
    switch (attacker_skill->attribute_id)
    {
    case ATTR_BLOOD:
        defender->bleeding += attacker_skill->effect_strength;
        attacker->HP_change += final_damage / 5;
        break;
    case ATTR_DARK:
        QI_stolen += (defender->QI >= attacker_skill->effect_strength) ? attacker_skill->effect_strength : defender->QI;
        defender->QI_change -= QI_stolen;
        break;
    default:
        break;
    }

    int QI_plundered = 0;
    switch (attacker_skill->skill_id)
    {
    case SKILL_ID_ICE_FLAME:
        defender->cursed += final_damage;
        break;
    case SKILL_ID_ESSENCE_PLUNDER:
        QI_plundered = defender->QI / 4;
        if (defender_skill->action_type == ACTION_TYPE_GAIN_QI)
        {
            defender->QI_change -= QI_plundered;
            attacker->QI_change += QI_plundered;
            CHN_PRINT("[%s 夺取了 %s 的 %d 点气力!]\n", attacker->name, defender->name, QI_plundered);
            ENG_PRINT("[%s had plundered %s's %d point(s) of QI essence!]\n", attacker->name, defender->name, QI_stolen);
        }
        break;
    default:
        break;
    }

    // --- 步骤 7: 应用最终伤害 ---
    if (final_damage > 0)
    {
        defender->HP_change -= final_damage;
        if (defender_skill->action_type != ACTION_TYPE_DEFEND && defender_skill->action_type != ACTION_TYPE_PARRY)
            InterruptHealing(attacker, defender); // 造成伤害且对方无防即可打断治疗
    }
    if (reflect_damage > 0)
    {
        attacker->HP_change -= reflect_damage;
        CHN_PRINT("[%s 的攻击被反弹，受到了 %d 点伤害!]\n", attacker->name, (int)reflect_damage);
        ENG_PRINT("[%s's attack was reflected, taking %d damage!]\n", attacker->name, (int)reflect_damage);
    }
}

// --- Main Game Loop and Other Functions (with fixes) ---
int main(int argc, char *argv[])
{

// 激活Windows控制台的虚拟终端处理功能
// --- 【核心修改】使用条件编译进行平台适配 ---
#ifdef _WIN32
    // 只在Windows下执行的代码
    SetConsoleOutputCP(GetConsoleOutputCP());

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE)
    {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode))
        {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif

    // 默认是独立模式
    game.is_bridge_mode = 0;
    // 检查命令行参数
    if (argc > 1 && strcmp(argv[1], "--bridge") == 0)
    {
        // 如果程序的第一个参数是 "--bridge"，则切换到桥接模式
        game.is_bridge_mode = 1;
    }

    Load_Config();
    Initialize_Databases();

    DIRECT_WRITE(
        Save_AI_Weights();

        printf("\n========================================\n");
        CHN_PRINT(" 权重写入完成!\n");
        ENG_PRINT(" Weight writing is completed!\n");
        printf("========================================\n");

        CHN_PRINT("\n按任意键退出程序...\n");
        ENG_PRINT("\nPress any key to exit...\n");
        return 0;)

    // 将随机数种子的初始化，移到整个程序的最开始，确保它只执行一次！
    srand(time(NULL));

    // 1. 定义计时变量
    clock_t start_time, end_time;
    double cpu_time_used;

    // 2. 在开始训练前，获取当前的处理器时间
    start_time = clock();
    CHN_PRINT("AI 训练开始...\n");
    ENG_PRINT("AI training start...\n");

    int train_reps = g_config.train_reps;
    total_games_played = 0;
    player_wins = 0;
    do
    {
#if !defined(INTERACTIVE_AI_MODE)
        // 只有在非交互模式下，才重定向输出以加速
        AI_TRAINING(freopen(NULL_DEVICE, "w", stdout));
#endif

        Load_AI_Weights();

        Game_init(&YOU, &CPU, &game);

        while (CPU.HP > 0 && YOU.HP > 0)
        {
            // 调用 Start_new_round 并检查其返回值
            if (Start_new_round(&game) == 1)
            {
                // 如果返回1，说明有玩家因机缘死亡，游戏需要提前结束
                break; // 跳出这个 while 循环，直接进入 Game_summary
            }

            // 只要是需要LLM思考的模式（6号或7号的特定周期），就提前生成Prompt
            if (game.opponent_type == 7 && game.is_bridge_mode)
            { // “事无巨细”模式，每回合都提前生成
                Build_Turn_Update_Prompt(&CPU, &YOU);
            }
            else if (game.opponent_type == 8 && game.is_bridge_mode && (game.round_number % STRATEGIC_CYCLE == 0)) // “将帅分级”模式，在战略周期生成
            {
                // a. 请求大元帅（LLM）进行战略决策
                Request_Strategic_Decision(&CPU, &YOU, &game);

                // b. 决策后，重置历史记录
                game.history_log_count = 0;
            }

            HUMAN_PLAYING(Player_action(game, &YOU));

            AI_TRAINING(
                switch (game.AI_type) {
                    case 0:
                        CPU_logic_V0_Random(&YOU, &CPU);
                        break;
                    case 1:
                        CPU_logic_V1A_Disruptor(&YOU, &CPU);
                        break;
                    case 2:
                        CPU_logic_V1B_Berserker(&YOU, &CPU);
                        break;
                    case 3:
                        CPU_logic_V1C_Turtle(&YOU, &CPU);
                        break;
                    case 4:
                        CPU_logic_V1D_Ascetic(&YOU, &CPU);
                        break;
                    case 5:
                        CPU_logic_V1E_Gambler(&YOU, &CPU);
                        break;
                    case 6:
                    default:
                        CPU_logic_V2A_Tuned(&YOU, &CPU, 0);
                        break;
                }

                CPU_action(&YOU);
                fflush(stdout); // 确保V2 AI的行动结果被立即发送

                if (game.is_bridge_mode && game.opponent_type == 7) {
                    printf("##CMD##:GET_LLM_RESULT_FOR_AI_TURN\n");
                    fflush(stdout);
                })

            printf("\033[91m");
            switch (game.opponent_type)
            {
            case 0:
                CPU_logic_V0_Random(&CPU, &YOU);
                break;
            case 1:
                CPU_logic_V1A_Disruptor(&CPU, &YOU);
                break;
            case 2:
                CPU_logic_V1B_Berserker(&CPU, &YOU);
                break;
            case 3:
                CPU_logic_V1C_Turtle(&CPU, &YOU);
                break;
            case 4:
                CPU_logic_V1D_Ascetic(&CPU, &YOU);
                break;
            case 5:
                CPU_logic_V1E_Gambler(&CPU, &YOU);
                break;
            case 6:
                CPU_logic_V2A_Tuned(&CPU, &YOU, 0);
                break;
            case 7:
                CPU_logic_LLM(&CPU, &YOU);
                break;
            case 8:
                switch (game.current_general_id)
                {
                case 0:
                    CPU_logic_V1A_Disruptor(&CPU, &YOU);
                    break;
                case 1:
                    CPU_logic_V1B_Berserker(&CPU, &YOU);
                    break;
                case 2:
                    CPU_logic_V1C_Turtle(&CPU, &YOU);
                    break;
                case 3:
                    CPU_logic_V1D_Ascetic(&CPU, &YOU);
                    break;
                case 4:
                    CPU_logic_V1E_Gambler(&CPU, &YOU);
                    break;
                default:
                    CPU_logic_V0_Random(&CPU, &YOU);
                    break;
                }
                break;

            default:
                CPU_logic_V0_Random(&CPU, &YOU);
                break;
            }
            printf("\033[0m");

            CPU_action(&CPU);

            Action_resolve(&YOU, &CPU);

            // 回合数上限检查，属于游戏规则，而非玩家状态
            if (game.round_number >= MAX_ROUNDS)
            {
                break; // 强制跳出循环
            }
        }

// --- BLUEPRINT REFACTOR: Correct I/O Management ---
#if !defined(INTERACTIVE_AI_MODE)
#ifdef _WIN32
        AI_TRAINING(freopen("CONOUT$", "w", stdout));
#else
        AI_TRAINING(freopen("/dev/tty", "w", stdout));
#endif
#endif

        // --- END REFACTOR ---

        Game_summary(&YOU, &CPU);

        AI_TRAINING(AI_Learn_From_Game(YOU.HP > 0));

        // AI_TRAINING(Save_AI_Weights());

        HUMAN_PLAYING(CHN_PRINT("\n按回车键继续...\n"));
        HUMAN_PLAYING(ENG_PRINT("\nPress ENTER to continue...\n"));
        HUMAN_PLAYING(fflush(stdout));

        // 在AI训练模式下，我们不需要任何等待，直接进入下一轮
        AI_TRAINING(
        // 在这里可以加一个极短的延时，如果需要的话，但通常不需要
        // AI训练观察模式：自动延时
#if defined(SLOW_DOWN)
            SLEEP_MS(2000); // 在每局结束后暂停1000毫秒（1秒）
#endif
        )

        // 只在人类对战模式下，执行等待逻辑
        HUMAN_PLAYING(
            // 1. 发送一个明确的信号，告诉Python现在轮到人类输入了
            if (game.is_bridge_mode) {
                printf("##CMD##:INPUT_REQUIRED\n");
                fflush(stdout);
            }

            // 2. 调用 getchar() 来等待Python端发送过来的任何字符
            // Python端的 input() 会等待用户按回车，然后将整行发过来
            // 这里的 getchar() 只是为了消耗掉那个输入，起到阻塞等待的作用
            getchar();)
    } while (--train_reps); //  (0); //

    end_time = clock();

    cpu_time_used = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("\n========================================\n");
    CHN_PRINT(" AI 训练全部完成!\n");
    CHN_PRINT(" 总耗时: %.2f 秒\n", cpu_time_used);
    ENG_PRINT(" AI training is completed!\n");
    ENG_PRINT(" Time elapsed: %.2f s\n", cpu_time_used);
    printf("========================================\n");

    CHN_PRINT("\n按回车键退出程序...\n");
    ENG_PRINT("\nPress ENTER to exit...\n");
    GET_CHAR();

    return 0;
}

// --- BLUEPRINT REFACTOR: Centralized Immediate Effect Resolution ---
// 这个函数是处理所有“施法时”即时效果的唯一模块
static void Resolve_Immediate_Effects(Player *player)
{
    // 如果玩家未选择任何行动，则直接返回
    if (player->current_action_type == ACTION_TYPE_NONE)
    {
        return;
    }

    // 从玩家自己的“技能书”中，获取他当前使用的最高阶技能
    const Skill *chosen_skill = &player->learned_skills[player->current_action_type];

    // --- 核心逻辑：合并后的通用效果结算 ---
    switch (player->current_action_type)
    {
    case ACTION_TYPE_GAIN_QI:
        player->QI_change += player->gain_combo;
        CHN_PRINT("%s 集了 %d 点气!\n", player->name, player->gain_combo);
        ENG_PRINT("%s gained %d QI!\n", player->name, player->gain_combo);
        break;
    case ACTION_TYPE_BURST:
    {
        int burst_cost_per_hit = chosen_skill->cost;
        player->burst_count = player->QI / burst_cost_per_hit;
        player->QI_change -= player->burst_count * burst_cost_per_hit;
        switch (chosen_skill->skill_id)
        {
        case SKILL_ID_WINDBLADE:
            CHN_PRINT("<%s 已形成 %d 把风刃!>\n", player->name, player->burst_count);
            ENG_PRINT("<%s formed %d windblade(s)!>\n", player->name, player->burst_count);
            break;
        case SKILL_ID_COMMANDING_SWORDS:
            CHN_PRINT("<%s 唤来 %d 把灵剑!>\n", player->name, player->burst_count);
            ENG_PRINT("<%s called %d Soul sword(s)!>\n", player->name, player->burst_count);
            break;
        case SKILL_ID_BEETLE_SWARM:
            CHN_PRINT("<%s 操控起 %d 只噬金虫!>\n", player->name, player->burst_count);
            ENG_PRINT("<%s controlled %d devour metal beetle(s)!>\n", player->name, player->burst_count);
            break;
        default:
            CHN_PRINT("<%s 已形成 %d 把 %s!>\n", player->name, player->burst_count, chosen_skill->name_chn);
            ENG_PRINT("<%s formed %d %s(s)!>\n", player->name, player->burst_count, chosen_skill->name_eng);
            break;
        }
    }
    break;
    case ACTION_TYPE_BOOST:
        switch (chosen_skill->skill_id)
        {
        case SKILL_ID_WARCRY:
            player->enraged += 3;
            break;
        case SKILL_ID_CONCENTRATION:
            player->enraged += 2;
            player->evade = 0.75f;
            break;
        case SKILL_ID_CORE_ERUPTION:
            player->enraged += 6;
            player->HP_change -= max_HP[player->XIUWEI] * 3 / 10;
            break;
        default:
            break;
        }
        break;
    case ACTION_TYPE_HEAL:
        int heal_amount;
        switch (chosen_skill->skill_id)
        {
        case SKILL_ID_HEAL:
            player->healing = (player->XIUWEI + 1); // 治疗效果依然和境界有关，可以后续数据化
            heal_amount = 2 * Yuan[player->XIUWEI];
            break;
        case SKILL_ID_EVERGREEN_ART:
            player->healing = (player->XIUWEI + 1);
            heal_amount = 5 * Yuan[player->XIUWEI];
            player->cursed -= Yuan[player->XIUWEI];
            break;
        case SKILL_ID_CORE_RESTORATION:
            player->healing = (player->XIUWEI + 1);
            heal_amount = 6 * Yuan[player->XIUWEI];
            player->cursed = 0;
            break;
        }
        player->HP_change += heal_amount;
        ENG_PRINT("[%s healed for %d HP immediately!]\n", player->name, heal_amount);
        CHN_PRINT("[%s 立即恢复了 %d 点生命值！]\n", player->name, heal_amount);
        break;
    // 其他技能没有需要在这里预处理的逻辑
    default:
        break;
    }

    if (chosen_skill->skill_id == SKILL_ID_SWORD_PHANTOM)
        player->burst_count = 6;
    if (chosen_skill->skill_id == SKILL_ID_GREAT_GOLDEN_SWORDFORMATION)
        player->burst_count = 32;
}
// --- END REFACTOR ---

// --- Tool Funcs that havs special use ---
#pragma region tool_function
// 蓝图核心：一个绝对安全的工具函数，用于获取AI的可行行动列表
// 它直接查询玩家实例，而不是依赖任何全局变量
int get_affordable_actions(const Player *player, ActionType affordable_actions[])
{
    int count = 0;
    // 遍历玩家的“技能槽”（由ActionType索引）
    for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
    {
        const Skill *skill_in_slot = &player->learned_skills[i];

        // 检查这个技能槽中是否有已学习的技能，并且QI足够
        if (skill_in_slot->skill_id != SKILL_ID_NONE && player->QI >= skill_in_slot->cost)
        {
            // 如果可用，将该槽位对应的【宏观行动类别】添加到列表中
            affordable_actions[count] = skill_in_slot->action_type;
            count++;
        }
    }
    return count;
}

// 一个更简单的辅助函数，用于检查某个特定的【宏观行动类别】是否可行
static inline int can_perform_action(const Player *player, ActionType action_type)
{
    // 根据传入的宏观类别，直接查找对应的技能槽
    const Skill *skill_in_slot = &player->learned_skills[action_type];

    // 检查该槽位是否有技能，且QI足够
    if (skill_in_slot->skill_id != SKILL_ID_NONE && player->QI >= skill_in_slot->cost)
    {
        return 1;
    }
    return 0;
}

// This helper function reduces code duplication for interrupting healing.
int InterruptHealing(const Player *attacker, Player *target)
{
    if (target->healing > 0)
    {
        ENG_PRINT("[%s's healing was interrupted by %s's attack!]\n", target->name, attacker->name);
        CHN_PRINT("[%s 的恢复被 %s 的攻击打断了!]\n", target->name, attacker->name);
        target->healing = 0;
        return 1;
    }
    return 0;
}

// 这个函数是“更新玩家技能”的唯一逻辑来源
static void Update_Player_Skills(Player *player)
{
    // 1. 清空现有技能书架
    for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
    {
        player->learned_skills[i].skill_id = SKILL_ID_NONE;
    }

    // 2. 遍历全局数据库，授予每个类别中最高阶的可学技能
    for (int i = 0; i < TOTAL_SKILLS; i++)
    {
        const Skill *new_skill = &g_skill_database[i];

        if (new_skill->skill_id != SKILL_ID_NONE && new_skill->rank <= player->XIUWEI)
        {
            ActionType category_id = new_skill->action_type;

            if (player->learned_skills[category_id].skill_id == SKILL_ID_NONE ||
                new_skill->rank > player->learned_skills[category_id].rank)
            {
                player->learned_skills[category_id] = *new_skill;
            }
        }
    }
}

// 一个内聚的、可重用的函数，负责初始化一个玩家的所有状态
static void Initialize_Player(Player *player, const char *name_eng, const char *name_chn)
{
    ENG(player->name = (char *)name_eng);
    CHN(player->name = (char *)name_chn);
    player->XIUWEI = g_config.initial_xiuwei;
    player->QI = g_config.initial_qi;
    player->evade = g_config.initial_evade > 0 ? g_config.initial_evade : 0.1f;
    player->HP = max_HP[player->XIUWEI];
    player->ATK = Yuan[player->XIUWEI];
    player->YUAN = Yuan[player->XIUWEI];
    player->gain_combo = player->XIUWEI + 1;
    player->current_action_type = ACTION_TYPE_NONE;
    player->burst_count = 0;
    player->healing = 0;
    player->enraged = 0;
    player->bleeding = 0;
    player->cursed = 0;
    player->HP_change = 0;
    player->QI_change = 0;
    player->root = (rand() % (TOTAL_ROOT_TYPES - 1)) + 1;

    // 初始化技能槽，通过调用唯一的技能更新模块
    Update_Player_Skills(player);
}

// 随机事件
int Trigger_Fate(Player *player)
{
    // 随机选择一个机缘事件 (避开 FATE_None)
    FateID fate = (rand() % (TOTAL_FATE_TYPES - 1)) + 1;

    printf("\033[95m"); // 用亮紫色显示机缘信息

    switch (fate)
    {
    case FATE_Qi_Infusion:
    {                                                              // 使用花括号创建局部作用域
        int qi_gain = 5 + (rand() % 6 + 1) * Yuan[player->XIUWEI]; // 获得 5-10 点QI
        player->QI += qi_gain;
        CHN_PRINT("[机缘降临!] %s 偶感天地灵气灌体, 瞬间获得了 %d 点QI!\n", player->name, qi_gain);
        ENG_PRINT("[Fate Arrives!] %s feels the infusion of worldly spiritual qi, instantly gaining %d QI!\n", player->name, qi_gain);
        break;
    }
    case FATE_Vitality_Blessing:
    {
        int hp_heal = max_HP[player->XIUWEI] * 0.2f; // 恢复20%最大生命值
        player->HP += hp_heal;
        if (player->HP > max_HP[player->XIUWEI])
            player->HP = max_HP[player->XIUWEI];
        CHN_PRINT("[机缘降临!] 一滴生命甘露融入 %s 体内, 恢复了 %d 点HP!\n", player->name, hp_heal);
        ENG_PRINT("[Fate Arrives!] A drop of vitality dew merges into %s's body, restoring %d HP!\n", player->name, hp_heal);
        break;
    }
    case FATE_Enlightenment:
        player->enraged = 3; // 效果持续1次攻击
        CHN_PRINT("[机缘降临!] %s 陷入顿悟, 攻击将蕴含天地之力!\n", player->name);
        ENG_PRINT("[Fate Arrives!] %s has an epiphany, attack will be empowered by heaven and earth!\n", player->name);
        break;
    case FATE_Agile_Wind:
        player->evade += 0.5f; // 效果持续3回合
        CHN_PRINT("[机缘降临!] %s 的身法变得飘逸, 获得了风灵庇佑!\n", player->name);
        ENG_PRINT("[Fate Arrives!] %s's movement becomes ethereal, blessed by the agile wind!\n", player->name);
        break;
    case FATE_Calamity:
    {
        int dmg = max_HP[player->XIUWEI] * (1 + (rand() % 3)) / 10; // 受到 1-3 点伤害
        player->HP -= dmg;
        CHN_PRINT("[天道无常!] 一道劫雷劈中了 %s, 造成了 %d 点伤害!\n", player->name, dmg);
        ENG_PRINT("[Way of Heaven is Unpredictable!] A calamity tribulation strikes %s, dealing %d damage!\n", player->name, dmg);

        if (player->HP <= 0)
        {
            CHN_PRINT("[天命已尽!] %s 未能渡过此劫, 身死道消!\n", player->name);
            ENG_PRINT("[Mandate of Heaven is over!] %s failed to survive the tribulation and perished!\n", player->name);
            printf("\033[0m");
            return 1; // 返回 1，明确表示有玩家死亡
        }
        break;
    }
    default:
        break;
    }
    printf("\033[0m");
}

#pragma endregion

// --- Stable Core Process Functions ---
#pragma region Core Process

void Game_init(Player *YOU, Player *CPU, Game *game)
{
    g_log_count = 0;
    game->world = (g_config.world > 0 && g_config.world < TOTAL_WORLD_COUNT) ? g_config.world : 0;
    game->round_number = 0;
    game->current_general_id = rand() % 5; // 开局随机选一个将军
    game->history_log_count = 0;

    ENG_PRINT("\n\033[32mWelcome to the QI Game!\033[0m\n");
    CHN_PRINT("\n\033[32m欢迎来到气之游戏！\033[0m\n");
    fflush(stdout); // <-- 关键修正: 强制发送欢迎信息

    // --- BLUEPRINT REFACTOR: Simplified High-Level Coordinator ---
    // 1. 调用模块化函数，分别初始化YOU和CPU
    Initialize_Player(YOU, "Joiry", "你");
    Initialize_Player(CPU, "CPU", "CPU"); // 临时名字，稍后会被覆盖

    // 2. 根据游戏模式确定并设置CPU的具体“人格”
    // --- BLUEPRINT REFACTOR: Unified Opponent Configuration ---
    // 1. 修正边界检查，使其包含LLM对手类型(6)
    if (g_config.enemy_type >= 0 && g_config.enemy_type <= 8)
    {
        game->opponent_type = g_config.enemy_type;
    }
    else
    {
        game->opponent_type = rand() % 5 + 1; // 如果配置无效，则随机选择一个普通对手
    }

    if (g_config.ai_type == -1)
    {
        game->AI_type = rand() % 5 + 1;
    }
    else
    {
        game->AI_type = g_config.ai_type;
    }

    // 2. 在初始化时就正确设置LLM对手的名字
    switch (game->opponent_type)
    {
    case 0:
        CHN(CPU->name = "混沌");
        ENG(CPU->name = "Random");
        break;
    case 1:
        CHN(CPU->name = "破法者");
        ENG(CPU->name = "Disruptor");
        break;
    case 2:
        CHN(CPU->name = "狂战士");
        ENG(CPU->name = "Berserker");
        break;
    case 3:
        CHN(CPU->name = "神龟流");
        ENG(CPU->name = "Turtle");
        break;
    case 4:
        CHN(CPU->name = "苦修者");
        ENG(CPU->name = "Ascetic");
        break;
    case 5:
        CHN(CPU->name = "豪赌徒");
        ENG(CPU->name = "Gambler");
        break;
    case 6:
        CHN(CPU->name = "狂才");
        ENG(CPU->name = "Crazy Genius");
        break;
    case 7: // “事无巨细”模式
        CHN(CPU->name = "悟道者");
        ENG(CPU->name = "Enlightened One");
        if (game->is_bridge_mode)
        {
            printf("##CMD##:NEW_GAME_START_PER_TURN\n"); // <-- 新的、专用的信号
            fflush(stdout);
            Build_Per_Turn_Genesis_Prompt(); // <-- 调用新的开场白函数
        }
        break;
    case 8: // “将帅分级”模式
        CHN(CPU->name = "大元帅");
        ENG(CPU->name = "Grand Marshal");
        if (game->is_bridge_mode)
        {
            printf("##CMD##:NEW_GAME_START_MARSHAL\n"); // <-- 为将帅模式也创建一个专用信号
            fflush(stdout);
            Build_Marshal_Genesis_Prompt();
        }
        break;
    }
    // --- END REFACTOR ---

    // 3. 打印初始信息并应用灵根修正
    printf("\033[93m");
    CHN_PRINT("[天赋觉醒] %s 乃是 %s, ", YOU->name, CHN_Root_Names[YOU->root]);
    ENG_PRINT("[Talent Awakened] %s possesses the %s, ", YOU->name, Eng_Root_Names[YOU->root]);
    CHN_PRINT("%s 则是 %s!\n", CPU->name, CHN_Root_Names[CPU->root]);
    ENG_PRINT("while %s has the %s!\n", CPU->name, Eng_Root_Names[CPU->root]);
    printf("\033[0m");

    if (YOU->root == ROOT_Solid)
    {
        YOU->HP *= 1.2f;
    }
    if (CPU->root == ROOT_Solid)
    {
        CPU->HP *= 1.2f;
    }
    // --- END REFACTOR ---
}

#pragma region status_resolve
// 模块 1: 处理行动消耗与状态重置
static void Resolve_QI_Change_And_Resets(Player *player)
{
    player->QI += player->QI_change;
    player->QI_change = 0;
    player->QI = (player->QI > 0) ? player->QI : 0;

    // 集气连击逻辑
    if (player->current_action_type == ACTION_TYPE_GAIN_QI)
    {
        if (player->gain_combo < 1 << player->XIUWEI)
        {
            player->gain_combo += player->XIUWEI;
        }
    }
    else
    {
        player->gain_combo = player->XIUWEI + 1;
    }

    player->current_action_type = ACTION_TYPE_NONE;

    int QI_absorb = 0;
    if (player->XIUWEI >= SEVERING)
        QI_absorb += player->XIUWEI * 2;
    if (game.world >= Spiritual_World)
        QI_absorb += player->XIUWEI * 2;
    if (QI_absorb > 0)
    {
        player->QI += QI_absorb;
        CHN_PRINT("天人合一！ %d 点气自行从天地间涌入 %s 体内!\n", QI_absorb, player->name);
        ENG_PRINT("Unity with the Cosmos! %d Qi spontaneously surges from the world into %s's body!\n", QI_absorb, player->name);
    }
}

// 模块 2: 处理持续性效果 (如治疗、激怒、闪避衰减、流血、诅咒)
static void Resolve_Persistent_Effects(Player *player)
{
    printf("\033[33m");
    // 治疗效果
    if (player->healing > 0)
    {
        int max_hp_for_realm = (player->root == ROOT_Solid) ? max_HP[player->XIUWEI] * 1.2f : max_HP[player->XIUWEI];
        if (player->HP < max_hp_for_realm)
        {
            int heal_amount = player->healing;
            if (player->root == ROOT_Solid)
            {
                heal_amount *= 1.5f; // 厚土灵根治疗效果增强
            }
            ENG_PRINT("[%s healed for %d HP!]\n", player->name, heal_amount);
            CHN_PRINT("[%s 恢复了 %d 点生命值！]\n", player->name, heal_amount);
            player->HP += heal_amount;
            if (player->HP > max_hp_for_realm)
            {
                player->HP = max_hp_for_realm;
            }
        }
    }

    // 激怒效果
    if (player->enraged > 0)
    {
        player->enraged--;
    }
    player->ATK = Yuan[player->XIUWEI] + player->enraged;

    // 闪避率衰减
    float base_evade = (player->root == ROOT_Ethereal) ? 0.1f * player->XIUWEI : 0.02f * player->XIUWEI;
    base_evade += g_config.initial_evade;
    if (player->evade > base_evade)
    {
        player->evade -= 0.5f * (player->evade - base_evade);
    }

    // 流血效果
    if (player->bleeding > 0)
    {
        player->HP_change -= player->bleeding--;
    }
    else
    {
        player->bleeding = 0;
    }

    // 诅咒效果
    if (player->cursed > 0)
    {
        player->HP_change -= player->cursed;
    }
    else
    {
        player->cursed = 0;
    }
}

// 模块 3: 处理伤害结算
static void Resolve_Damage(Player *player)
{
    if (player->root == ROOT_Solid)
    {
        player->HP = (player->HP > max_HP[player->XIUWEI] * 1.2f) ? max_HP[player->XIUWEI] * 1.2f : player->HP;
    }
    else
    {
        player->HP = (player->HP > max_HP[player->XIUWEI]) ? max_HP[player->XIUWEI] : player->HP;
    }

    if (player->HP_change < 0)
    {
        ENG_PRINT("[%s took \033[35m%d\033[33m net damage!]\n", player->name, -(player->HP_change));
        CHN_PRINT("[%s 受到 \033[35m%d\033[33m 点净伤害!]\n", player->name, -(player->HP_change));
    }
    player->HP += player->HP_change;
    player->HP_change = 0;
}

// Dedicated Breakthrough Module
static void Apply_Breakthrough_Rewards(Player *player)
{
    // 1. 清空QI (突破消耗)
    player->QI = 0;

    // 2. 根据新的境界，刷新所有派生属性
    player->HP = max_HP[player->XIUWEI];
    player->ATK = Yuan[player->XIUWEI];
    player->YUAN = Yuan[player->XIUWEI];
    player->gain_combo = player->XIUWEI + 1;

    // 3. 重置动态状态
    player->enraged = 0;
    player->healing = 0;
    player->cursed = 0;

    // 4. 应用灵根的突破奖励
    if (player->root == ROOT_Solid)
    {
        player->HP *= 1.2f;
    }
    float base_evade = (player->root == ROOT_Ethereal) ? 0.1f * player->XIUWEI : 0.05f * player->XIUWEI;
    base_evade += g_config.initial_evade;
    player->evade = base_evade; // 突破后闪避率直接刷新，而不是衰减

    // 5. 重新授予技能！通过调用唯一的技能更新模块
    Update_Player_Skills(player);
}

static void Apply_Ascension(Player *player)
{
    // 1. 清空QI (突破消耗)
    player->QI = 999999999;

    // 2. 根据新的境界，刷新所有派生属性
    player->HP = 999999999;
    player->ATK = 999999999;
    player->YUAN = 999999999;

    // 3. 重置动态状态
    player->enraged = 0;
    player->healing = 0;
    player->cursed = 0;

    // 4. 应用灵根的突破奖励
    float base_evade = 1;
    player->evade = base_evade; // 突破后闪避率直接刷新，而不是衰减
}

// 模块 4: 处理突破判定
static void Resolve_Breakthrough(Player *player)
{
    if (player->QI >= max_QI[player->XIUWEI])
    {
        if (player->XIUWEI < TOTAL_XIUWEI_LEVEL - 1)
        {
            float breakthrough_chance = (player->root == ROOT_Heavenly) ? 90.0f : 90.0f * exp(-player->XIUWEI / 2.0f);

            if ((rand() % 100) < breakthrough_chance)
            {
                // --- 核心修正: 正确的流程 ---
                // 1. 先提升境界等级
                player->XIUWEI++;

                // 2. 再调用专用的奖励函数
                Apply_Breakthrough_Rewards(player);

                CHN_PRINT("\033[92m[%s 突破至 %s 期!]\033[0m\n", player->name, Realm[player->XIUWEI]);
                ENG_PRINT("\033[92m[%s has broken through to the %s realm!]\033[0m\n", player->name, Eng_Realm[player->XIUWEI]);
            }
            else
            {
                CHN_PRINT("\033[91m[%s 突破失败!]\033[0m\n", player->name);
                ENG_PRINT("\033[91m[%s failed to break through!]\033[0m\n", player->name);
                player->QI = max_QI[player->XIUWEI] * 3 / 4;
            }
        }
        else
        {
            CHN_PRINT("\033[92m[%s 飞升了!]\033[0m\n", player->name);
            ENG_PRINT("\033[92m[%s made it to Ascension!]\033[0m\n", player->name);

            player->XIUWEI++;

            Apply_Ascension;
        }
    }
}

#pragma endregion

void Status_settlement(Player *player)
{
    // 如果玩家已死亡，则跳过所有结算
    if (player->HP <= 0)
    {
        return;
    }

    // --- BLUEPRINT REFACTOR: High-Level Settlement Coordinator ---
    // 流程清晰，如同清单

    // 1. 结算行动消耗与状态重置
    Resolve_QI_Change_And_Resets(player);

    // 2. 结算持续性效果 (治疗、Buff/Debuff)
    Resolve_Persistent_Effects(player);

    // 3. 结算本回合受到的伤害
    Resolve_Damage(player);

    // 4. 在所有状态变化后，检查是否满足突破条件
    // (需要先检查一次血量，防止死亡后突破)
    if (player->HP > 0)
    {
        Resolve_Breakthrough(player);
    }

    // (回合数上限检查已移至 main 循环，因为它属于游戏全局逻辑)
}

int Start_new_round(Game *game)
{
    game->round_number++;

    if (game->round_number > 1 && game->round_number % 5 == 0 && (rand() % 100) < 30)
    {
        // 检查 Trigger_Fate 的返回值
        if (Trigger_Fate(&YOU) == 1 || Trigger_Fate(&CPU) == 1)
        {
            // 如果任一玩家因机缘死亡，就返回 1，通知主循环
            return 1;
        }
    }

    ENG_PRINT("\033[32m----- Round %d -----\033[0m\n", game->round_number);
    CHN_PRINT("\033[32m----- 第 %d 轮 -----\033[0m\n", game->round_number);

    ENG_PRINT("Your HP: \033[36m%d\033[0m, Your QI: \033[33m%d\033[0m\n", YOU.HP, YOU.QI);
    ENG_PRINT("%s's HP: \033[36m%d\033[0m, %s's QI: \033[33m%d\033[0m\n", CPU.name, CPU.HP, CPU.name, CPU.QI);
    CHN_PRINT("你的元神: \033[36m%d\033[0m, 你的气力: \033[33m%d\033[0m\n", YOU.HP, YOU.QI);
    CHN_PRINT("%s的元神: \033[36m%d\033[0m, %s的气力: \033[33m%d\033[0m\n", CPU.name, CPU.HP, CPU.name, CPU.QI);

    // 我们已经在 Build_LLM_Prompt 的末尾加了 fflush，所以这里可以不加。
    // 但是为了代码的健壮性，在所有需要与外部交互的打印块末尾加上它都是一个好习惯。
    fflush(stdout);

    // 日志记录始终以 YOU (学习中的AI) 为第一视角
    if (g_log_count < MAX_LOG_TURNS)
    {
        AI_TurnLog *log = &g_game_log[g_log_count];
        log->round_number = game->round_number;

        // 记录当时“战况快照”
        log->ai_hp = YOU.HP;         // AI (YOU) 的HP
        log->opponent_hp = CPU.HP;   // 对手 (CPU) 的HP
        log->ai_qi = YOU.QI;         // AI (YOU) 的QI
        log->opponent_qi = CPU.QI;   // 对手 (CPU) 的QI
        log->ai_xiuwei = YOU.XIUWEI; // AI (YOU) 的境界
    }

#ifdef AI_TRAINING_SET
#if defined(SLOW_DOWN)
    SLEEP_MS(g_config.time_delay * 1000); // 在每局结束后暂停1000毫秒（1秒）
#endif
#endif

    return 0;
}

void Player_action(Game game, Player *YOU)
{
    YOU->current_action_type = ACTION_TYPE_NONE;

    while (YOU->current_action_type == ACTION_TYPE_NONE)
    {
        printf("\033[0m");
        CHN_PRINT("请选择你的行动：\n");
        ENG_PRINT("What's your next action?\n");

        // 1. 动态生成并显示可用行动列表
        for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
        {
            const Skill *skill = &YOU->learned_skills[i];
            if (skill->skill_id != SKILL_ID_NONE && YOU->QI >= skill->cost)
            {
                CHN_PRINT("[%c] %s(%d)  ", skill->hotkey, skill->name_chn, skill->cost);
                ENG_PRINT("[%c] %s(%d)  ", skill->hotkey, skill->name_eng, skill->cost);
            }
        }
        printf("\n");

        // (为兼容性保留) 发送“取回结果”信号
        if (game.is_bridge_mode)
        {
            printf("##CMD##:INPUT_REQUIRED\n");
            fflush(stdout);
        }

        char buffer[16];
        char choice = ' ';
        if (fgets(buffer, sizeof(buffer), stdin) != NULL)
        {
            sscanf(buffer, " %c", &choice);
        }
        choice = toupper(choice);

        // 2. 验证输入并设置行动
        int action_found = 0;
        for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
        {
            const Skill *skill = &YOU->learned_skills[i];
            if (skill->skill_id != SKILL_ID_NONE && YOU->QI >= skill->cost && choice == skill->hotkey)
            {
                // --- BLUEPRINT REFACTOR: Purified Logic Flow ---

                // a. 设置玩家的行动意图和基础消耗
                YOU->current_action_type = skill->action_type;
                YOU->QI_change -= skill->cost;

                // b. 打印唯一的、数据驱动的“行动宣言”
                printf("\033[34m");
                CHN_PRINT(skill->prompt_chn, YOU->name);
                ENG_PRINT(skill->prompt_eng, YOU->name);
                printf("\n");

                // c. 调用统一的即时效果模块，处理所有后续逻辑和打印

                action_found = 1;
                break;
                // --- END REFACTOR ---
            }
        }

        if (!action_found)
        {
            CHN_PRINT("无效的选择，请重新输入。\n");
            ENG_PRINT("Invalid choice, please try again.\n");
        }
    }
}

void CPU_action(Player *player)
{
    if (player->current_action_type == ACTION_TYPE_NONE)
    {
        return;
    }

    // 从玩家自己的“技能书”中获取正确的、当前最高阶的技能
    const Skill *chosen_skill = &player->learned_skills[player->current_action_type];

    // 设置基础消耗
    player->QI_change -= chosen_skill->cost;

    // --- BLUEPRINT REFACTOR: Purified Logic Flow ---

    // 1. 打印唯一的、数据驱动的“行动宣言”
    printf("\033[91m"); // AI行动统一用红色
    CHN_PRINT(chosen_skill->prompt_chn, player->name);
    ENG_PRINT(chosen_skill->prompt_eng, player->name);
    printf("\n");

    // 2. 调用统一的即时效果模块，处理所有后续逻辑和打印

    // 3. 恢复默认颜色
    printf("\033[0m");
    // --- END REFACTOR ---
}

void Action_resolve(Player *YOU, Player *CPU)
{
    // --- BLUEPRINT REFACTOR: The True Turn Resolution Engine ---

    printf("\033[33m"); // 结算信息统一用黄色

    // === 阶段一: 即时效果结算 ===
    // 在所有战斗交互之前，先结算双方选择的技能所附带的“施法时”效果。
    // 这个顺序依然重要：通常玩家（YOU）的行动优先结算。
    Resolve_Immediate_Effects(YOU);
    Resolve_Immediate_Effects(CPU);

    // === 阶段二: 日志记录 (行动意图) ===
    // 记录下双方在所有状态变化后的最终行动选择和消耗
    if (g_log_count < MAX_LOG_TURNS)
    {
        AI_TurnLog *log = &g_game_log[g_log_count];
        log->chosen_action = YOU->current_action_type;
        log->opponent_action = CPU->current_action_type;
        log->action_cost = YOU->QI_change;
    }

    // === 阶段三: 战斗交互结算 ===
    // 调用我们的战斗引擎，处理双方的直接对抗
    Oneway_Solution(YOU, CPU);
    Oneway_Solution(CPU, YOU);

    // === 阶段四: 日志记录 (战斗结果) ===
    if (g_log_count < MAX_LOG_TURNS)
    {
        AI_TurnLog *log = &g_game_log[g_log_count];
        log->damage_dealt = -(CPU->HP_change);
        log->damage_taken = -(YOU->HP_change);
        g_log_count++;
    }

    // === 阶段五: 回合结束状态结算 ===
    // 处理所有回合结束时的状态变化（中毒、治疗、buff/debuff持续时间减少等）
    Status_settlement(YOU);
    Status_settlement(CPU);

    printf("\033[0m\n");
    // --- END REFACTOR ---
}

void Game_summary(Player *YOU, Player *CPU)
{
    // 1. 无论胜负，总游戏场次都加一
    total_games_played++;
    AI_TRAINING(printf("\n\033[0m(round played: %d)\n", game.round_number));

    ENG_PRINT("YOU HP: %d, QI: %d\n", YOU->HP, YOU->QI);
    ENG_PRINT("%s HP: %d, QI: %d\n", CPU->name, CPU->HP, CPU->QI);
    CHN_PRINT("你 元神: %d, 气力: %d\n", YOU->HP, YOU->QI);
    CHN_PRINT("%s 元神: %d, 气力: %d\n", CPU->name, CPU->HP, CPU->QI);

    if (CPU->HP <= 0 && YOU->HP > 0)
    {
        CHN_PRINT("\033[92m{恭喜！你获胜了！}\033[0m\n");
        ENG_PRINT("\033[92m{Congratulations! You won!}\033[0m\n");
        // 2. 如果玩家获胜，胜利场次加一
        player_wins++;
    }
    else if (YOU->HP <= 0 && CPU->HP > 0)
    {
        CHN_PRINT("\033[35m{游戏结束！你被CPU击败了}\033[0m\n");
        ENG_PRINT("\033[35m{Game Over! You were defeated by CPU。}\033[0m\n");
    }
    else if (game.round_number >= MAX_ROUNDS)
    {
        CHN_PRINT("\033[35m{回合数超出上限 %d ，游戏强制结束！}\033[0m\n", MAX_ROUNDS);
        ENG_PRINT("\033[35m{Round exceed max limit %d ,game was forced to stop!}\033[0m\n", MAX_ROUNDS);
        YOU->HP = 1;
    }
    else
    {
        CHN_PRINT("\033[35m{游戏结束！你们同归于尽了。}\033[0m\n");
        ENG_PRINT("\033[35m{Game Over! You and your enemy perished together。}\033[0m\n");
    }
    CHN_PRINT("\033[35m{%s 达到了 %s 修为}\033[0m\n", YOU->name, Realm[YOU->XIUWEI]);
    ENG_PRINT("\033[35m{%s've reached %s Realm}\033[0m\n", YOU->name, Eng_Realm[YOU->XIUWEI]);
    AI_TRAINING(CHN_PRINT("\033[35m{%s 达到了 %s 修为}\033[0m\n", CPU->name, Realm[CPU->XIUWEI]));
    AI_TRAINING(ENG_PRINT("\033[35m{%s've reached %s Realm}\033[0m\n", CPU->name, Eng_Realm[CPU->XIUWEI]));

    if (total_games_played > 0)
    {
        // 3. 计算胜率
        // 关键：必须将其中一个整数强制转换为float，否则C语言会执行整数除法，结果总是0
        float win_rate = ((float)player_wins / total_games_played) * 100.0f;

        // 4. 打印格式化的统计信息
        CHN_PRINT("\n--- 战绩统计 ---\n");
        ENG_PRINT("\n--- Performance Statics ---\n");
        CHN_PRINT("当前胜率: \033[96m%.2f%%\033[0m (%d胜 / %d场)\n", win_rate, player_wins, total_games_played);
        ENG_PRINT("Current Win Rate : \033[96m%.2f%%\033[0m (%d Win / %d Game)\n", win_rate, player_wins, total_games_played);
        printf("----------------\n");
    }
}

void Load_Config()
{
    FILE *file = fopen("config.txt", "r");
    if (file == NULL)
    {
        // 如果文件不存在，不用担心，直接使用我们已经设置好的默认值
        CHN_PRINT("[提示] 未找到 config.txt 配置文件, 将使用默认设置。\n");
        ENG_PRINT("[Info] config.txt not found, using default settings.\n");
        return;
    }

    char line[100];
    char key[50];
    char value[50];

    // 逐行读取文件
    while (fgets(line, sizeof(line), file))
    {
        // 忽略注释行 ('#') 和空行
        if (line[0] == '#' || line[0] == '\n' || line[0] == '[')
        {
            continue;
        }

        // 解析 "键 = 值" 格式
        if (sscanf(line, "%49s = %49s", key, value) == 2)
        {
            // 根据键(key)来更新配置变量
            if (strcmp(key, "HP") == 0)
            {
                g_config.initial_hp = atoi(value); // atoi 将字符串转为整数
            }
            else if (strcmp(key, "QI") == 0)
            {
                g_config.initial_qi = atoi(value);
            }
            else if (strcmp(key, "XIUWEI") == 0)
            {
                g_config.initial_xiuwei = atoi(value);
            }
            else if (strcmp(key, "Evade") == 0)
            {
                g_config.initial_evade = atof(value); // atof 将字符串转为浮点数
            }
            else if (strcmp(key, "REPS") == 0)
            {
                g_config.train_reps = atoi(value);
            }
            else if (strcmp(key, "Enemy") == 0)
            {
                g_config.enemy_type = atoi(value);
            }
            else if (strcmp(key, "AI") == 0)
            {
                g_config.ai_type = atoi(value);
            }
            else if (strcmp(key, "AI_Randomness") == 0)
            {
                g_config.enable_ai_randomness = atoi(value);
            }
            else if (strcmp(key, "World") == 0)
            {
                g_config.world = atoi(value);
            }
            else if (strcmp(key, "Time_delay") == 0)
            {
                g_config.time_delay = atof(value);
            }
        }
    }

    fclose(file);

    CHN_PRINT("[提示] 已成功加载 config.txt 配置文件。\n");
    ENG_PRINT("[Info] Successfully loaded config.txt.\n");
}

#pragma endregion

// --- AI Optimization ---
#pragma region AIs
// --- BLUEPRINT REFACTOR: All Rule-Based AIs Adapted ---

// V0: 最简单的AI，从所有可用行动中随机选择
void CPU_logic_V0_Random(Player *cpu, const Player *opponent)
{
    ActionType affordable_actions[TOTAL_ACTION_TYPES];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);

    if (affordable_count > 0)
    {
        int random_index = rand() % affordable_count;
        cpu->current_action_type = affordable_actions[random_index];
    }
    else
    {
        cpu->current_action_type = ACTION_TYPE_GAIN_QI; // 保底措施
    }
}

// V1A - 破法者 (Disruptor)
// 战术思想: 胜利不是通过击败对手，而是通过让他无法执行自己的战术来取得。
void CPU_logic_V1A_Disruptor(Player *cpu, const Player *opponent)
{
    if (can_perform_action(cpu, ACTION_TYPE_SMITE) && opponent->HP <= cpu->learned_skills[ACTION_TYPE_SMITE].base_power * cpu->ATK)
    {
        cpu->current_action_type = ACTION_TYPE_SMITE;
        return;
    }

    if (can_perform_action(cpu, ACTION_TYPE_BURST) && opponent->HP <= cpu->learned_skills[ACTION_TYPE_BURST].base_power * cpu->QI / cpu->learned_skills[ACTION_TYPE_BURST].cost * cpu->ATK)
    {
        cpu->current_action_type = ACTION_TYPE_BURST;
        return;
    }

    SkillID boost_skill = cpu->learned_skills[ACTION_TYPE_BOOST].skill_id;
    // 优先级2：破坏资源 (假设已有 Qi Siphon 技能，ID为 Qi_Siphon)
    if (opponent->QI > 80 && can_perform_action(cpu, ACTION_TYPE_BOOST) && boost_skill == SKILL_ID_ESSENCE_PLUNDER && (rand() % 100 < 50))
    {
        cpu->current_action_type = ACTION_TYPE_BOOST;
        return;
    }

    if ((opponent->QI - cpu->QI) > 30)
    {
        cpu->current_action_type = ACTION_TYPE_GAIN_QI;
        return;
    }

    // 优先级3：削弱对手 (假设已有 Warcry 技能)
    if (boost_skill != SKILL_ID_CORE_ERUPTION && boost_skill != SKILL_ID_ESSENCE_PLUNDER)
    {
        if (opponent->enraged == 0 && opponent->QI > 1 && can_perform_action(cpu, ACTION_TYPE_BOOST))
        {
            cpu->current_action_type = ACTION_TYPE_BOOST; // 在对手准备进攻时削弱他
            return;
        }
    }

    // 如果没有可干扰的，就进行低成本骚扰或集气
    if (can_perform_action(cpu, ACTION_TYPE_MELEE))
    {
        cpu->current_action_type = ACTION_TYPE_MELEE;
    }
    else
    {
        cpu->current_action_type = ACTION_TYPE_GAIN_QI;
    }
}

// V1B - 狂战士 (重写版)
// 战术思想: 纯粹的进攻。它的字典里没有防御和治疗。
// 决策优先级: 1.斩杀 -> 2.最高伤害 -> 3.为下一次攻击集气。
void CPU_logic_V1B_Berserker(Player *cpu, const Player *opponent)
{
    // --- 斩杀判定 (依然是最高优先级，不受随机性影响) ---
    if (can_perform_action(cpu, ACTION_TYPE_SMITE) && opponent->HP <= cpu->learned_skills[ACTION_TYPE_MELEE].base_power * cpu->ATK)
    {
        cpu->current_action_type = ACTION_TYPE_SMITE;
        return;
    }
    if (can_perform_action(cpu, ACTION_TYPE_MELEE) && opponent->HP <= cpu->learned_skills[ACTION_TYPE_MELEE].base_power * cpu->ATK)
    {
        cpu->current_action_type = ACTION_TYPE_MELEE;
        return;
    }

    // --- 随机决策门：是强化自己还是直接攻击？ ---
    // 有25%的几率，狂战士会选择强化自己而不是直接攻击
    if (g_config.enable_ai_randomness == 1 && can_perform_action(cpu, ACTION_TYPE_BOOST) && (rand() % 100 < 25))
    {
        cpu->current_action_type = ACTION_TYPE_BOOST;
        return;
    }

    // --- 默认进攻逻辑 (75%的几率会走到这里) ---
    if (can_perform_action(cpu, ACTION_TYPE_SMITE))
    {
        cpu->current_action_type = ACTION_TYPE_SMITE;
        return;
    }
    if (can_perform_action(cpu, ACTION_TYPE_MELEE))
    {
        cpu->current_action_type = ACTION_TYPE_MELEE;
        return;
    }

    // --- 准备阶段 ---
    cpu->current_action_type = ACTION_TYPE_GAIN_QI;
}

// V1C - 神龟流 (重写版)
// 战术思想: 纯粹的生存。在确保自身安全之前，绝不主动进攻。
// 决策优先级: 1.紧急治疗 -> 2.预判性防御 -> 3.为防御和治疗集气。
void CPU_logic_V1C_Turtle(Player *cpu, const Player *opponent)
{
    // --- 生存判定 (最高优先级) ---
    if (cpu->HP < max_HP[cpu->XIUWEI] * 0.8f && can_perform_action(cpu, ACTION_TYPE_HEAL))
    {
        cpu->current_action_type = ACTION_TYPE_HEAL;
        return;
    }

    if (can_perform_action(cpu, ACTION_TYPE_SMITE) && opponent->HP <= cpu->learned_skills[ACTION_TYPE_SMITE].base_power * cpu->ATK)
    {
        cpu->current_action_type = ACTION_TYPE_SMITE;
        return;
    }

    if (can_perform_action(cpu, ACTION_TYPE_BURST) && opponent->HP <= cpu->learned_skills[ACTION_TYPE_BURST].base_power * cpu->QI / cpu->learned_skills[ACTION_TYPE_BURST].cost * cpu->ATK)
    {
        cpu->current_action_type = ACTION_TYPE_BURST;
        return;
    }

    // --- 随机决策门：是稳妥防御还是抓住机会偷袭？ ---
    // (需要预知对手行动，若无法预知，可改为判断对手QI)
    // 假设对手正在集气，有30%的几率，神龟会放弃防御进行一次骚扰攻击
    if (g_config.enable_ai_randomness == 1 && opponent->QI < 2 && can_perform_action(cpu, ACTION_TYPE_MELEE) && (rand() % 100 < 30))
    {
        cpu->current_action_type = ACTION_TYPE_MELEE;
        return;
    }

    // --- 默认防御逻辑 (70%的几率会走到这里) ---
    if (opponent->QI >= 4 && can_perform_action(cpu, ACTION_TYPE_DEFEND))
    {
        cpu->current_action_type = ACTION_TYPE_DEFEND;
        return;
    }
    if (opponent->enraged > 0 && can_perform_action(cpu, ACTION_TYPE_DEFEND))
    {
        cpu->current_action_type = ACTION_TYPE_DEFEND;
        return;
    }
    if (can_perform_action(cpu, ACTION_TYPE_PARRY))
    {
        cpu->current_action_type = ACTION_TYPE_PARRY;
        return;
    }

    // --- 准备阶段 ---
    cpu->current_action_type = ACTION_TYPE_GAIN_QI;
}

// V1D - 苦修者 (新增)
// 战术思想: 纯粹的发育。以最快速度达到境界巅峰是唯一目标。HP只是用于换取时间的资源。
// 决策优先级: 1.能赢吗？ -> 2.集气！
void CPU_logic_V1D_Ascetic(Player *cpu, const Player *opponent)
{
    // --- 胜利判定 (最高优先级) ---
    if (can_perform_action(cpu, ACTION_TYPE_SMITE) && opponent->HP <= cpu->learned_skills[ACTION_TYPE_SMITE].base_power * cpu->ATK)
    {
        cpu->current_action_type = ACTION_TYPE_SMITE;
        return;
    }

    if (can_perform_action(cpu, ACTION_TYPE_BURST) && opponent->HP <= cpu->learned_skills[ACTION_TYPE_BURST].base_power * cpu->QI / cpu->learned_skills[ACTION_TYPE_BURST].cost * cpu->ATK)
    {
        cpu->current_action_type = ACTION_TYPE_BURST;
        return;
    }

    if (can_perform_action(cpu, ACTION_TYPE_TERMINATE) && opponent->HP <= cpu->learned_skills[ACTION_TYPE_TERMINATE].base_power * cpu->ATK)
    {
        cpu->current_action_type = ACTION_TYPE_TERMINATE;
        return;
    }

    // --- 随机决策门：是继续修炼还是保命？ ---
    // 只有在HP低于25%的极端情况下，才有可能触发求生欲
    if (g_config.enable_ai_randomness == 1 && cpu->HP < max_HP[cpu->XIUWEI] * 0.25f && can_perform_action(cpu, ACTION_TYPE_HEAL))
    {
        // 血量越低，求生欲越强，治疗的概率越大
        // 例如，20%血量时，有 (25-20)*5 = 25% 的概率治疗
        int heal_chance = (int)((max_HP[cpu->XIUWEI] * 0.25f - cpu->HP) * 5.0f);
        if (rand() % 100 < heal_chance)
        {
            cpu->current_action_type = ACTION_TYPE_HEAL;
            return;
        }
    }

    // --- 默认修炼逻辑 ---
    cpu->current_action_type = ACTION_TYPE_GAIN_QI;
}

// V1E - 豪赌徒 (Gambler)
// 战术思想: 要么不出手，出手必定是雷霆一击。
void CPU_logic_V1E_Gambler(Player *cpu, const Player *opponent)
{
    // 寻找当前可用的、威力最大的攻击技能
    ActionType best_attack = ACTION_TYPE_NONE;
    if (can_perform_action(cpu, ACTION_TYPE_BURST) && opponent->HP / 2 <= cpu->learned_skills[ACTION_TYPE_BURST].base_power * cpu->QI / cpu->learned_skills[ACTION_TYPE_BURST].cost * cpu->ATK)
    {
        cpu->current_action_type = ACTION_TYPE_BURST;
        return;
    }

    if (can_perform_action(cpu, ACTION_TYPE_TERMINATE))
    {
        best_attack = ACTION_TYPE_TERMINATE;
    }
    else if (can_perform_action(cpu, ACTION_TYPE_SMITE))
    {
        best_attack = ACTION_TYPE_SMITE;
    } // ...可以扩展更多高威力技能

    // 如果能发动最强攻击，则不惜一切代价发动
    if (best_attack != ACTION_TYPE_NONE)
    {
        cpu->current_action_type = best_attack;
        return;
    }

    // 否则，心无旁骛地集气，为下一次的全力一击做准备
    cpu->current_action_type = ACTION_TYPE_GAIN_QI;
}

// Evaluateaction - AI的大脑 (V2 - 拥有长远规划和风险意识)
float EvaluateAction(ActionType action_type, const Player *cpu, const Player *opponent, const AI_Weights *weights)
{
    float score = 0.0f;

    // --- BLUEPRINT REFACTOR: Read from database, don't hardcode ---
    const Skill *skill = &cpu->learned_skills[action_type];
    float damage = 0;
    int qi_cost = skill->cost;

    if (skill->target_type == TARGET_ENEMY)
    {
        damage = skill->base_power * cpu->ATK;
        if (skill->type_id == TYPE_BURST)
        {
            // For Burst, the real cost is all QI, damage scales with it.
            int burst_cost_per_hit = (skill->cost > 0) ? skill->cost : 1;
            damage *= (cpu->QI / burst_cost_per_hit);
        }
    }
    // --- END REFACTOR ---

    // 1. 基于自身灵根的策略调整
    switch (cpu->root)
    {
    case ROOT_Heavenly:
        // 我是天灵根，突破是我的王道！大幅增加集气的价值。
        if (action_type == ACTION_TYPE_GAIN_QI)
            score += 100.0f;
        break;
    case ROOT_Sharp:
        // 我是锐金灵根，进攻就是最好的防御！
        if (damage > 0)
            score += 50.0f;
        break;
        // ...
    }

    // 2. 基于对手灵根的策略调整
    switch (opponent->root)
    {
    case ROOT_Solid:
        // 对手是厚土灵根，太肉了。低伤害的骚扰没用，必须攒大招。
        // 降低Melee的价值，提升Smite, Burst等高伤害技能的价值。
        if (action_type == ACTION_TYPE_MELEE)
            score -= 30.0f;
        if (action_type == ACTION_TYPE_SMITE)
            score += 50.0f;
        break;
    case ROOT_Ethereal:
        // 对手是风灵根，闪避太高了。
        // 降低所有远程技能的价值，提升Boost(强化近战)和Melee的价值。
        if (action_type == ACTION_TYPE_RANGED || action_type == ACTION_TYPE_BURST)
            score -= 50.0f;
        if (action_type == ACTION_TYPE_BOOST)
            score += 100.0f;
        break;
        // ...
    }

    // --- 步骤 2: 基于新旧指标进行综合评分 ---

    // 1. 【生存】治疗的价值 (旧逻辑)
    if (action_type == ACTION_TYPE_HEAL)
    {
        float health_percentage = (float)cpu->HP / max_HP[cpu->XIUWEI];
        if (health_percentage < 0.7f)
        {
            score += weights->w_health_urgency * (1.0f - health_percentage);
        }
    }

    // 2. 【进攻】伤害的价值 (旧逻辑，但更精细)
    if (damage > 0)
    {
        float hit_chance = 1.0f;
        if (action_type == ACTION_TYPE_RANGED || action_type == ACTION_TYPE_BURST || action_type == ACTION_TYPE_TERMINATE)
        {
            // 对于可被闪避的攻击，命中率 = 1 - 对手的闪避率
            hit_chance = 1.0f - opponent->evade;
        }

        float expected_damage = damage * hit_chance;

        score += expected_damage * weights->w_damage_per_point;
        if (opponent->HP <= expected_damage)
        {
            score += weights->w_kill_shot_bonus; // 斩杀奖励
        }
        if (opponent->healing > 0)
        {
            // 【优化】权衡打断的价值
            if (damage <= (opponent->XIUWEI + 1))
            {
                score += weights->w_interrupt_heal_bonus / 4.0f; // 低价值打断
            }
            else
            {
                score += weights->w_interrupt_heal_bonus; // 高价值打断
            }
        }
    }

    // 3. 【防御】防御的价值 (旧逻辑)
    if (action_type == ACTION_TYPE_DEFEND || action_type == ACTION_TYPE_PARRY)
    {
        if (opponent->QI > 4)
        {
            score += weights->w_defend_vs_high_qi;
        }
        if (opponent->enraged > 0)
        {
            score += 50.0f; // 硬编码的紧急防御奖励
        }
    }

    // 4. 【资源】集气的价值 (旧逻辑)
    if (action_type == ACTION_TYPE_GAIN_QI)
    {
        if (cpu->QI < 3)
        {
            score += weights->w_low_qi_gather;
        }
        else
        {
            score += 100.0f; // 默认待机分
        }

        // --- 【新增评估维度：突破潜力】 ---
        // 只有在满足特定条件下，AI才会认真考虑“突破”这个战略目标

        // 条件1: 不能是最高境界（已经是飞升大佬了，没法再突破了）
        if (cpu->XIUWEI < TOTAL_XIUWEI_LEVEL - 1)
        {
            // 条件2: 血量比较健康，有闭关的资本 (血量高于50%)
            if (cpu->HP > max_HP[cpu->XIUWEI] * 0.5f)
            {
                // 计算当前QI占突破所需QI的百分比，作为“紧迫感”
                float breakthrough_progress = (float)cpu->QI / max_QI[cpu->XIUWEI];

                // 核心评分公式：
                // 越接近突破（progress越高），集气的战略价值就越大
                score += breakthrough_progress * weights->w_breakthrough_urgency;
            }
        }
    }

    // === 【新增评估维度】 ===

    // 5. 【效率感】评估伤害效率
    if (damage > 0 && qi_cost > 0)
    {
        float efficiency = (float)damage / qi_cost;
        score += efficiency * weights->w_damage_per_qi;
    }

    // 6. 【资源优势感】利用QI优势
    if (cpu->QI > opponent->QI + 3)
    { // 当QI比对手多3点以上时
        // 拥有显著QI优势时，更倾向于将优势转化为胜势（攻击）
        if (damage > 0)
        {
            score += (cpu->QI - opponent->QI) * weights->w_qi_advantage;
        }
    }

    // 7. 【风险规避感】低血量时避免高风险行为
    if (cpu->HP < max_HP[cpu->XIUWEI] * 0.25f)
    { // 血量低于25%时
        // 对所有非防御、非治疗、非集气的行动施加惩罚
        if (action_type != ACTION_TYPE_DEFEND && action_type != ACTION_TYPE_PARRY && action_type != ACTION_TYPE_HEAL && action_type != ACTION_TYPE_GAIN_QI)
        {
            score -= weights->w_low_hp_penalty;
        }
    }

    if (action_type == ACTION_TYPE_BOOST)
    {
        float buff_score = weights->w_self_buff_value;

        if (opponent->evade > 0.1f)
        {
            buff_score *= (1.0f + opponent->evade * 5.0f);
        }
        if (cpu->HP < max_HP[cpu->XIUWEI] * 0.6f)
        {
            buff_score /= 2.0f;
        }

        if (opponent->current_action_type == ACTION_TYPE_GAIN_QI && opponent->QI >= 2)
        {
            buff_score += weights->w_self_buff_value * (opponent->QI);
        }

        score += buff_score;
    }
    // 增加一点随机性，避免AI行为过于死板
    score += (rand() % 10);

    return score;
}

// V2: 基于评分系统的智能AI
void CPU_logic_V2_Genius(Player *cpu, const Player *opponent)
{
    ActionType affordable_actions[TOTAL_ACTION_TYPES];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);

    if (affordable_count == 0)
    {
        cpu->current_action_type = ACTION_TYPE_GAIN_QI; // 保底措施
        return;
    }

    ActionScore best_action = {ACTION_TYPE_NONE, -1.0f};

    for (int i = 0; i < affordable_count; i++)
    {
        ActionType current_action_type = affordable_actions[i];
        float score = EvaluateAction(current_action_type, cpu, opponent, &g_ai_weights);
        score += (rand() % 10);

        if (score > best_action.score)
        {
            best_action.score = score;
            best_action.action_type = current_action_type;
        }
    }
    cpu->current_action_type = best_action.action_type;
}

// 加载另一套权重
void CPU_logic_V2A_Tuned(Player *cpu, const Player *opponent, int A)
{
    ActionType affordable_actions[TOTAL_ACTION_TYPES];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);

    if (affordable_count == 0)
    {
        cpu->current_action_type = ACTION_TYPE_GAIN_QI; // 保底措施
        return;
    }

    ActionScore best_action = {ACTION_TYPE_NONE, -1.0f};

    for (int i = 0; i < affordable_count; i++)
    {
        ActionType current_action_type = affordable_actions[i];
        float score = EvaluateAction(current_action_type, cpu, opponent, &g_ai_weights_A[A]);
        score += (rand() % 10);

        if (score > best_action.score)
        {
            best_action.score = score;
            best_action.action_type = current_action_type;
        }
    }
    cpu->current_action_type = best_action.action_type;
}

// AI_Learn_From_Game - AI的学习方法 (已校准日志系统)
void AI_Learn_From_Game(int ai_won)
{
    float learning_rate = 0.01f;

    for (int i = 0; i < g_log_count; i++)
    {
        AI_TurnLog *log = &g_game_log[i];
        float turn_reward = 0.0f;

        // --- 关键事件评估 (使用校准后的日志字段) ---

        // [正向] 斩杀对手
        if (log->opponent_hp > 0 && (log->opponent_hp - log->damage_dealt) <= 0)
            turn_reward += 100;
        // [正向] 成功突破
        if (i > 0 && log->ai_xiuwei > g_game_log[i - 1].ai_xiuwei)
            turn_reward += 200;
        // [正向] 高效伤害
        if (log->action_cost > 0 && ((float)log->damage_dealt / log->action_cost) > 2.0f)
            turn_reward += 30;
        // [正向] 成功防御大招
        if ((log->chosen_action == ACTION_TYPE_DEFEND || log->chosen_action == ACTION_TYPE_PARRY) && log->opponent_action == ACTION_TYPE_SMITE && log->damage_taken < 4)
            turn_reward += 50;

        // [负向] 被斩杀
        if (log->ai_hp > 0 && (log->ai_hp - log->damage_taken) <= 0)
            turn_reward -= 100;
        // [负向] 满血治疗
        if (log->chosen_action == ACTION_TYPE_HEAL && log->ai_hp >= max_HP[log->ai_xiuwei])
            turn_reward -= 20;
        // [负向] 攻击被弹反
        if (log->opponent_action == ACTION_TYPE_PARRY && log->damage_taken > 0)
            turn_reward -= 30;
        // [负向] 满气被杀
        if ((log->ai_hp - log->damage_taken) <= 0 && log->ai_qi > max_QI[log->ai_xiuwei] * 0.5f)
            turn_reward -= 50;
        // [负向] 受到重创
        if (log->damage_taken > log->ai_hp * 0.3f)
            turn_reward -= 40;

        // 如果没有触发任何关键事件，则根据最终胜负给予基础奖惩
        if (turn_reward == 0.0f)
        {
            turn_reward = ai_won ? 1.0f : -1.0f;
        }

        float update_amount = learning_rate * turn_reward;

        // --- 精确归因：更新权重 ---
        switch (log->chosen_action)
        {
        case ACTION_TYPE_HEAL:
            g_ai_weights.w_health_urgency += update_amount;
            break;
        case ACTION_TYPE_MELEE:
        case ACTION_TYPE_SMITE:
        case ACTION_TYPE_RANGED:
        case ACTION_TYPE_BURST:
        case ACTION_TYPE_TERMINATE:
            g_ai_weights.w_damage_per_point += update_amount * 0.7f;
            g_ai_weights.w_damage_per_qi += update_amount * 0.3f;
            break;
        case ACTION_TYPE_DEFEND:
        case ACTION_TYPE_PARRY:
            g_ai_weights.w_defend_vs_high_qi += update_amount;
            break;
        case ACTION_TYPE_GAIN_QI:
            g_ai_weights.w_low_qi_gather += update_amount * 0.5f;
            g_ai_weights.w_breakthrough_urgency += update_amount * 0.5f;
            break;
        case ACTION_TYPE_BOOST:
            g_ai_weights.w_self_buff_value += update_amount;
            break;
        default:
            break;
        }
    }

    const float TOTAL_WEIGHT_TARGET = 2000.0f;
    float current_total_weight = 0.0f;
    current_total_weight += g_ai_weights.w_health_urgency;
    current_total_weight += g_ai_weights.w_damage_per_point;
    current_total_weight += g_ai_weights.w_kill_shot_bonus;
    current_total_weight += g_ai_weights.w_interrupt_heal_bonus;
    current_total_weight += g_ai_weights.w_low_qi_gather;
    current_total_weight += g_ai_weights.w_defend_vs_high_qi;
    current_total_weight += g_ai_weights.w_qi_advantage;
    current_total_weight += g_ai_weights.w_damage_per_qi;
    current_total_weight += g_ai_weights.w_low_hp_penalty;
    current_total_weight += g_ai_weights.w_breakthrough_urgency;
    current_total_weight += g_ai_weights.w_self_buff_value;

    if (current_total_weight > TOTAL_WEIGHT_TARGET)
    {
        float scaling_factor = TOTAL_WEIGHT_TARGET / current_total_weight;
        g_ai_weights.w_health_urgency *= scaling_factor;
        g_ai_weights.w_damage_per_point *= scaling_factor;
        g_ai_weights.w_kill_shot_bonus *= scaling_factor;
        g_ai_weights.w_interrupt_heal_bonus *= scaling_factor;
        g_ai_weights.w_low_qi_gather *= scaling_factor;
        g_ai_weights.w_defend_vs_high_qi *= scaling_factor;
        g_ai_weights.w_qi_advantage *= scaling_factor;
        g_ai_weights.w_damage_per_qi *= scaling_factor;
        g_ai_weights.w_low_hp_penalty *= scaling_factor;
        g_ai_weights.w_breakthrough_urgency *= scaling_factor;
        g_ai_weights.w_self_buff_value *= scaling_factor;
    }

    const float MIN_WEIGHT = 1.0f;
    if (g_ai_weights.w_health_urgency < MIN_WEIGHT)
        g_ai_weights.w_health_urgency = MIN_WEIGHT;
    if (g_ai_weights.w_damage_per_point < MIN_WEIGHT)
        g_ai_weights.w_damage_per_point = MIN_WEIGHT;
    if (g_ai_weights.w_kill_shot_bonus < MIN_WEIGHT)
        g_ai_weights.w_kill_shot_bonus = MIN_WEIGHT;
    if (g_ai_weights.w_interrupt_heal_bonus < MIN_WEIGHT)
        g_ai_weights.w_interrupt_heal_bonus = MIN_WEIGHT;
    if (g_ai_weights.w_low_qi_gather < MIN_WEIGHT)
        g_ai_weights.w_low_qi_gather = MIN_WEIGHT;
    if (g_ai_weights.w_defend_vs_high_qi < MIN_WEIGHT)
        g_ai_weights.w_defend_vs_high_qi = MIN_WEIGHT;
    if (g_ai_weights.w_qi_advantage < MIN_WEIGHT)
        g_ai_weights.w_qi_advantage = MIN_WEIGHT;
    if (g_ai_weights.w_damage_per_qi < MIN_WEIGHT)
        g_ai_weights.w_damage_per_qi = MIN_WEIGHT;
    if (g_ai_weights.w_low_hp_penalty < MIN_WEIGHT)
        g_ai_weights.w_low_hp_penalty = MIN_WEIGHT;
    if (g_ai_weights.w_breakthrough_urgency < MIN_WEIGHT)
        g_ai_weights.w_breakthrough_urgency = MIN_WEIGHT;
    if (g_ai_weights.w_self_buff_value < MIN_WEIGHT)
        g_ai_weights.w_self_buff_value = MIN_WEIGHT;

    g_log_count = 0;
}

// 在 Game_init 中增加加载权重的逻辑
void Load_AI_Weights()
{
    FILE *file = fopen("ai_weights.dat", "rb"); // 以二进制读取模式打开
    if (file)
    {
        fread(&g_ai_weights, sizeof(AI_Weights), 1, file);
        fclose(file);
        printf("\n[AI weights loaded from file.]\n");
    }
}

// 在 main 函数返回前增加保存权重的逻辑
void Save_AI_Weights()
{
    FILE *file = fopen("ai_weights.dat", "wb"); // 以二进制写入模式打开
    if (file)
    {
        fwrite(&g_ai_weights, sizeof(AI_Weights), 1, file);
        fclose(file);
        printf("[AI weights saved to file.]\n");
    }
}
#pragma endregion AIs

// --- The true power —— —— LLM ---
#pragma region LLM
// --- BLUEPRINT v2.0: Genesis Prompt ---
void Build_Per_Turn_Genesis_Prompt()
{
    if (game.is_bridge_mode)
    {
        printf("##CMD##:START_PROMPT\n");
        fflush(stdout);
    }
    // 这是一个更简洁、更API友好的开场白
    printf("You are a master strategist in a turn-based cultivation game. Your goal is to defeat your opponent by reducing their HP to zero.\n");
    printf("For every turn, you will receive a status update and a list of available actions with their IDs.\n");
    printf("You MUST respond with a single, valid JSON object containing two keys: 'action_id' (the integer ID of your chosen action) and 'reasoning' (a brief strategic explanation).\n");
    printf("Example of a valid response: {\"action_id\": 0, \"reasoning\": \"My QI is low, so I need to gather more.\"}\n");
    printf("END_OF_PROMPT\n");
    fflush(stdout);
}

void Build_Marshal_Genesis_Prompt()
{
    if (game.is_bridge_mode)
    {
        printf("##CMD##:START_PROMPT\n");
        fflush(stdout); // <-- 关键修复：立刻发送命令
    }

    // === 核心世界观 (Core Worldview) ===
    printf("You are the Grand Marshal, a master strategist in a world of cultivation. Your goal is not just to win this battle, but to do so efficiently and decisively.\n");
    printf("Victory is achieved by depleting the opponent's HP to 0.\n");
    printf("The most critical long-term strategy is **Breakthrough**: accumulating enough QI to reach the maximum for your current realm and advancing to the next. A higher realm grants immense advantages in HP, QI capacity, and attack power (YUAN).\n");
    printf("Spiritual Roots grant passive bonuses: Heavenly aids breakthrough, Solid enhances HP/healing, Sharp boosts damage, Ethereal increases evasion.\n\n");

    // === 兵法总纲 (Principles of War) ===
    printf("== Grand Marshal's Principles of War ==\n");
    printf("1.  **Analyze and Adapt**: Your primary role is to analyze the opponent's strategy over a 5-turn cycle and deploy the general best suited to counter them.\n");
    printf("2.  **Resource Supremacy**: An advantage in QI is a strategic advantage. An opponent with high QI is a threat; an opponent low on QI is an opportunity.\n");
    printf("3.  **Calculated Aggression**: Do not engage in reckless battles. Switch to an aggressive general only when you have a clear advantage or when it's necessary to disrupt a vulnerable opponent.\n");
    printf("4.  **Preserve Strength**: In unfavorable situations, switch to a defensive general to minimize HP loss and buy time to accumulate resources for a counter-attack.\n\n");

    // === 将领名册 (Roster of Generals) ===
    printf("== Your Available Generals ==\n");
    printf("You command several generals, each a master of a specific combat doctrine. You will deploy them by their ID.\n");
    printf("- **[ID 0] V1A_Disruptor (The Saboteur)**: Master of interference...\n");
    printf("- **[ID 1] V1B_Berserker (The Vanguard)**: A pure offensive force...\n");
    printf("- **[ID 2] V1C_Turtle (The Iron Wall)**: The ultimate defensive specialist...\n");
    printf("- **[ID 3] V1D_Ascetic (The Monk)**: Single-mindedly focused on accumulating QI...\n");
    printf("- **[ID 4] V1E_Gambler (The Executioner)**: Saves all resources for a single, devastating blow...\n");
    printf("- **[ID 5] V0_Random (The Fool)**: Unpredictable and chaotic. A desperate last resort...\n\n");

    // === 任务指令 (Mission Command) ===
    printf("After each 5-turn strategic cycle, you will receive a battlefield report. Your task is to analyze it, reference this manual, and issue a command in the specified JSON format: {\"next_general_id\": <id>, \"reasoning\": \"...\"}. Your reasoning should reflect your strategic understanding.\n");

    printf("YOUR RESPONSE MUST BE A SINGLE, VALID JSON OBJECT AND NOTHING ELSE. This JSON object MUST contain two keys: 'next_general_id' (integer) and 'reasoning' (string).\n");
    printf("EXAMPLE of a valid response: {\"next_general_id\": 3, \"reasoning\": \"The opponent is passive, so I will switch to the Ascetic to gain a resource advantage.\"}\n");

    // === 结束信号 ===
    printf("END_OF_PROMPT\n");
    fflush(stdout);
}

void Build_Turn_Update_Prompt(const Player *cpu, const Player *opponent)
{
    if (game.is_bridge_mode)
    {
        printf("##CMD##:START_PROMPT\n");
        fflush(stdout); // <-- 关键修复：立刻发送命令
    }

    // --- 1. 描述当前战局 (Current State) ---
    printf("== Turn Update: Round %d ==\n", game.round_number);
    printf("Your Status: {HP: %d/%d, QI: %d/%d, Realm: %s}\n", cpu->HP, max_HP[cpu->XIUWEI], cpu->QI, max_QI[cpu->XIUWEI], Eng_Realm[cpu->XIUWEI]);
    printf("Opponent Status: {HP: %d/%d, QI: %d/%d, Realm: %s}\n", opponent->HP, max_HP[opponent->XIUWEI], opponent->QI, max_QI[opponent->XIUWEI], Eng_Realm[opponent->XIUWEI]);

    // --- 2. 列出可用技能 (Available Actions) ---
    printf("== Available Actions (Provide ID only) ==\n");
    ActionType affordable_actions[TOTAL_ACTION_TYPES];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);
    for (int i = 0; i < affordable_count; i++)
    {
        const Skill *skill = &cpu->learned_skills[affordable_actions[i]];
        printf("{ID: %d, Name: %s, Cost: %d}\n", skill->skill_id, skill->name_eng, skill->cost);
    }

    // --- 3. 提出明确的问题 (The Question) ---
    printf("Analyze the situation and provide your next move in the required JSON format.\n");

    // --- 4. 结束信号 ---
    printf("END_OF_PROMPT\n");
    fflush(stdout); // 极其重要！确保回合更新被立即发送
}

void CPU_logic_LLM(Player *cpu, const Player *opponent)
{
    // 2. (旧) 阻塞并等待Python返回结果
    int chosen_id = -1;
    char buffer[16];

    if (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        chosen_id = atoi(buffer);
        ActionType affordable_actions[TOTAL_ACTION_TYPES];
        int affordable_count = get_affordable_actions(cpu, affordable_actions);
        int is_valid = 0;
        for (int i = 0; i < affordable_count; i++)
        {
            if (chosen_id == affordable_actions[i])
            {
                is_valid = 1;
                break;
            }
        }

        if (is_valid)
        {
            cpu->current_action_type = chosen_id;
        }
        else
        {
            cpu->current_action_type = affordable_actions[rand() % affordable_count];
        }
    }
    else
    {
        cpu->current_action_type = ACTION_TYPE_GAIN_QI;
    }
}

void Build_Strategic_Report_Prompt(const Player *cpu, const Player *opponent, const Game *game)
{
    if (game->is_bridge_mode)
    {
        printf("##CMD##:START_PROMPT\n");
        fflush(stdout); // <-- 关键修复：立刻发送命令
    }

    // --- 1. 开场白 ---
    printf("Grand Marshal, this is the battlefield report for the last %d turns.\n", game->history_log_count);

    // --- 2. 宏观态势 ---
    printf("== Strategic Overview ==\n");
    printf("Our Status (CPU): {HP: %d/%d, QI: %d/%d, Realm: %s}\n", cpu->HP, max_HP[cpu->XIUWEI], cpu->QI, max_QI[cpu->XIUWEI], Eng_Realm[cpu->XIUWEI]);
    printf("Opponent Status (Player): {HP: %d/%d, QI: %d/%d, Realm: %s}\n", opponent->HP, max_HP[opponent->XIUWEI], opponent->QI, max_QI[opponent->XIUWEI], Eng_Realm[opponent->XIUWEI]);

    // --- 3. 对手行为分析 ---
    printf("== Opponent Behavior Analysis ==\n");
    if (game->history_log_count > 0)
    {
        int action_counts[TOTAL_ACTION_TYPES] = {0};
        for (int i = 0; i < game->history_log_count; i++)
        {
            if (game->player_turn_history[i].action_type >= 0)
            {
                action_counts[game->player_turn_history[i].action_type]++;
            }
        }
        printf("In the last %d turns, opponent actions were: ", game->history_log_count);
        for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
        {
            if (action_counts[i] > 0)
            {
                printf("%s: %d times; ", opponent->learned_skills[i].name_eng, action_counts[i]);
            }
        }
        printf("\n");
    }
    else
    {
        printf("No opponent actions recorded in this cycle yet.\n");
    }

    // --- 4. 我方现状 ---
    printf("== Current Strategy Assessment ==\n");
    // 这里可以根据 g_skill_database 动态获取将军名字，为简化先硬编码
    const char *general_names[] = {"Disruptor", "Berserker", "Turtle", "Ascetic", "Gambler"};
    printf("We are currently executing strategy ID %d (%s).\n", game->current_general_id, general_names[game->current_general_id]);

    // --- 5. 可用将领列表 ---
    printf("== Available Generals for Deployment ==\n");
    printf("[0: Disruptor], [1: Berserker], [2: Turtle], [3: Ascetic], [4: Gambler]\n");

    // --- 6. 核心问题 ---
    printf("Grand Marshal, what is your strategic order? Please respond in JSON format: {\"order\": \"<SWITCH|CONFIRM>\", \"general_id\": <id>, \"reasoning\": \"...\"}\n");
    printf("REMINDER: Your response MUST be a JSON object containing the 'next_general_id' key and a 'reasoning' key.\n");

    // --- 7. 结束信号 ---
    printf("END_OF_PROMPT\n");
    fflush(stdout);
}

void Request_Strategic_Decision(Player *cpu, Player *opponent, Game *game)
{
    // 1. 生成并发送“战情简报”
    Build_Strategic_Report_Prompt(cpu, opponent, game);

    // 2. 等待Python中间人返回决策
    int new_general_id = -1;
    char buffer[16];
    if (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        new_general_id = atoi(buffer);
        // 安全检查：确保ID在有效范围内 (假设我们有6个将军，ID 0-5)
        if (new_general_id >= 0 && new_general_id <= 5)
        {
            if (game->current_general_id != new_general_id)
            {
                printf("\033[95m[STRATEGIC SHIFT] Grand Marshal has ordered a change of command! General %s (ID %d) is now in charge!\033[0m\n",
                       GENERAL_NAMES[new_general_id], new_general_id);
                game->current_general_id = new_general_id;
            }
            else
            {
                printf("\033[95m[STRATEGIC CONFIRMATION] Grand Marshal confirms current strategy is optimal. Proceeding with General %s (ID %d).\033[0m\n",
                       GENERAL_NAMES[new_general_id], new_general_id);
            }
        }
    }
    // 如果没有收到有效指令，则维持原状
}

#pragma endregion LLM
