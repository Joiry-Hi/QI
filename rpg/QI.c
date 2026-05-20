#include "QI.h"

#pragma region Global Variable Definitions
// ---  ---
Player CPU = {.name = "CPU", .HP = 10, .ATK = 1, .YUAN = 1, .current_action_type = ACTION_TYPE_NONE};
Player YOU = {.name = "You", .HP = 10, .ATK = 1, .YUAN = 1, .current_action_type = ACTION_TYPE_NONE};
Game game = {.world = Mortal_World, .action = ' ', .opponent_type = -1, .AI_type = -1};

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
    .run_mode = 0,
    .auto_reward = 1,
    .reward_seed = 0,
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

static const char *ActionTypeNames_CHN[TOTAL_ACTION_TYPES] = {
    "集气", "近战", "远程", "防御", "治疗", "反击", "增益", "惩击", "爆发", "终结"};
static const char *ActionTypeNames_ENG[TOTAL_ACTION_TYPES] = {
    "Gain QI", "Melee", "Ranged", "Defend", "Heal", "Counter", "Boost", "Smite", "Burst", "Terminate"};

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
static bool g_run_player_ready = false;
static bool g_ui_json_mode = false;

// 静态技能数据库，定义了游戏中的所有技能规则
Skill g_skill_database[TOTAL_SKILLS];
Artifact g_artifact_database[TOTAL_ARTIFACTS];
Elixir g_elixir_database[TOTAL_ELIXIRS];
Talent g_talent_database[TOTAL_TALENTS];
RunState g_run;

#pragma endregion Global Variable Definitions

static unsigned int Run_Rand()
{
    g_run.reward_seed = g_run.reward_seed * 1103515245u + 12345u;
    return (g_run.reward_seed / 65536u) % 32768u;
}

static bool Player_HasArtifact(const Player *player, ArtifactID id)
{
    for (int i = 0; i < player->artifact_count; i++)
    {
        if (player->artifacts[i] == id)
            return true;
    }
    return false;
}

static bool Skill_IsCustomID(SkillID id)
{
    return id >= CUSTOM_SKILL_START && id < TOTAL_SKILLS;
}

static bool Skill_IsDefined(SkillID id)
{
    if (id <= SKILL_ID_NONE || id >= TOTAL_SKILLS)
        return false;
    const Skill *skill = &g_skill_database[id];
    return skill->skill_id == id && skill->name_chn != NULL &&
           skill->action_type >= 0 && skill->action_type < TOTAL_ACTION_TYPES;
}

static bool Artifact_IsCustomID(ArtifactID id)
{
    return id >= CUSTOM_ARTIFACT_START && id < TOTAL_ARTIFACTS;
}

static bool Elixir_IsCustomID(ElixirID id)
{
    return id >= CUSTOM_ELIXIR_START && id < TOTAL_ELIXIRS;
}

static int Player_ArtifactIntBonus(const Player *player, int (*read_bonus)(const Artifact *artifact)) __attribute__((unused));
static int Player_ArtifactIntBonus(const Player *player, int (*read_bonus)(const Artifact *artifact))
{
    int total = 0;
    for (int i = 0; i < player->artifact_count; i++)
    {
        ArtifactID id = player->artifacts[i];
        if (id > ARTIFACT_NONE && id < TOTAL_ARTIFACTS)
            total += read_bonus(&g_artifact_database[id]);
    }
    return total;
}

static int ArtifactMaxHPPct(const Artifact *artifact) { return artifact->max_hp_pct; }
static int ArtifactMaxQIPct(const Artifact *artifact) { return artifact->max_qi_pct; }
static int ArtifactBreakthroughPct(const Artifact *artifact) { return artifact->breakthrough_pct; }
static int ArtifactPostBattleCultivation(const Artifact *artifact) { return artifact->post_battle_cultivation; }
static int Player_ArtifactEffectiveIntBonus(const Player *player, int (*read_bonus)(const Artifact *artifact));

static int RPG_MinorRealmBonusPct(const Player *player)
{
    if (!player)
        return 0;
    int level = player->minor_realm_level;
    if (level < 0) level = 0;
    if (level > MAX_MINOR_REALM_LEVEL) level = MAX_MINOR_REALM_LEVEL;
    return level * 3;
}

static bool Player_HasTalent(const Player *player, TalentID id);
static int Player_MaxHP(const Player *player);
static int Player_MaxQI(const Player *player);
static inline int can_perform_action(const Player *player, ActionType action_type);

typedef struct {
    SchoolTag primary_school;
    int primary_count;
    int elixir_count;
    int artifact_count;
    bool sword_pressure;
    bool qi_pressure;
    bool defensive_shell;
    bool dark_pressure;
    bool blood_pressure;
} AIThreatProfile;

static AIThreatProfile AI_ReadThreatProfile(const Player *opponent)
{
    BuildProfile build = Player_GetBuildProfile(opponent);
    AIThreatProfile profile = {
        build.primary_school,
        build.primary_count,
        opponent->elixir_count,
        opponent->artifact_count,
        build.primary_school == SCHOOL_SWORD && build.primary_count >= 3,
        build.primary_school == SCHOOL_QI && build.primary_count >= 3,
        build.primary_school == SCHOOL_DEFENSE && build.primary_count >= 3,
        build.primary_school == SCHOOL_DARK && build.primary_count >= 2,
        build.primary_school == SCHOOL_BLOOD && build.primary_count >= 2};
    return profile;
}

static int Player_PercentOfMaxHP(const Player *player, int pct)
{
    int value = Player_MaxHP(player) * pct / 100;
    return value > 0 ? value : 1;
}

static int Player_PercentOfMaxQI(const Player *player, int pct)
{
    int value = Player_MaxQI(player) * pct / 100;
    return value > 0 ? value : 1;
}

static int Player_TalentBonusPct(const Player *player, TalentID id)
{
    if (!Player_HasTalent(player, id))
        return 0;
    switch (id)
    {
    case TALENT_QI_SEA: return 20;
    case TALENT_GOLDEN_BODY: return 15;
    case TALENT_THUNDER_AFFINITY: return 8;
    default: return 0;
    }
}

static int Player_MaxHP(const Player *player)
{
    if (player->XIUWEI < 0 || player->XIUWEI >= TOTAL_XIUWEI_LEVEL)
        return 1;
    int max_hp = max_HP[player->XIUWEI];
    if (player->root == ROOT_Solid)
        max_hp = (int)(max_hp * 1.2f);
    max_hp = max_hp * (100 + RPG_MinorRealmBonusPct(player)) / 100;
    max_hp = max_hp * (100 + Player_TalentBonusPct(player, TALENT_GOLDEN_BODY)) / 100;
    max_hp = max_hp * (100 + Player_ArtifactEffectiveIntBonus(player, ArtifactMaxHPPct)) / 100;
    if (player->soul_state == SOUL_GHOST)
        max_hp = (max_hp * 60 + 99) / 100;
    if (max_hp < 1)
        max_hp = 1;
    return max_hp;
}

static int Player_MaxQI(const Player *player)
{
    if (player->XIUWEI < 0 || player->XIUWEI >= TOTAL_XIUWEI_LEVEL)
        return 1;
    int max_qi = max_QI[player->XIUWEI];
    max_qi = max_qi * (100 + RPG_MinorRealmBonusPct(player)) / 100;
    max_qi = max_qi * (100 + Player_TalentBonusPct(player, TALENT_QI_SEA)) / 100;
    max_qi = max_qi * (100 + Player_ArtifactEffectiveIntBonus(player, ArtifactMaxQIPct)) / 100;
    if (player->soul_state == SOUL_GHOST)
        max_qi = (max_qi * 80 + 99) / 100;
    if (max_qi < 1)
        max_qi = 1;
    return max_qi;
}

static void Player_ClampVitals(Player *player)
{
    int max_hp = Player_MaxHP(player);
    int max_qi = Player_MaxQI(player);
    if (player->HP > max_hp) player->HP = max_hp;
    if (player->HP < 0) player->HP = 0;
    if (player->QI > max_qi) player->QI = max_qi;
    if (player->QI < 0) player->QI = 0;
}

static void Player_AddSchool(Player *player, SchoolTag school, int amount)
{
    if (school <= SCHOOL_NONE || school >= TOTAL_SCHOOLS)
        return;
    player->school_counts[school] += amount;
}

static bool Skill_PrereqMet(const Player *player, SkillID skill_id)
{
    if (!Skill_IsDefined(skill_id))
        return false;
    const Skill *skill = &g_skill_database[skill_id];
    if (player->bypassed_prereq_skills[skill_id])
        return true;
    if (player->XIUWEI < skill->required_realm)
        return false;
    if (skill->prereq_skill_id != SKILL_ID_NONE &&
        !player->unlocked_skills[skill->prereq_skill_id])
        return false;
    if (skill->prereq_action_type != ACTION_TYPE_NONE)
    {
        bool has_action_prereq = false;
        for (int i = 0; i < TOTAL_SKILLS; i++)
        {
            if (Skill_IsDefined((SkillID)i) && player->unlocked_skills[i] &&
                g_skill_database[i].action_type == skill->prereq_action_type)
            {
                has_action_prereq = true;
                break;
            }
        }
        if (!has_action_prereq)
            return false;
    }
    if (skill->school_tag > SCHOOL_NONE && skill->rank >= 3 &&
        player->school_counts[skill->school_tag] <= 0)
        return false;
    return true;
}

static bool Player_IsHumanFacing(const Player *player)
{
    return player == &YOU && g_config.run_mode && !g_config.auto_reward;
}

static void Player_ResetRunFields(Player *player)
{
    memset(player->unlocked_skills, 0, sizeof(player->unlocked_skills));
    memset(player->bypassed_prereq_skills, 0, sizeof(player->bypassed_prereq_skills));
    memset(player->school_counts, 0, sizeof(player->school_counts));
    memset(player->skill_mastery, 0, sizeof(player->skill_mastery));
    memset(player->school_cycle, 0, sizeof(player->school_cycle));
    memset(player->skill_refines, 0, sizeof(player->skill_refines));
    for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
        player->equipped_skills[i] = SKILL_ID_NONE;
    for (int i = 0; i < MAX_ARTIFACT_SLOTS; i++)
    {
        player->artifacts[i] = ARTIFACT_NONE;
        player->artifact_levels[i] = 0;
    }
    for (int i = 0; i < MAX_ELIXIR_SLOTS; i++)
        player->elixirs[i] = ELIXIR_NONE;
    memset(player->talents, 0, sizeof(player->talents));
    player->artifact_count = 0;
    player->elixir_count = 0;
    player->talent_count = 0;
    player->soul_state = SOUL_BODY;
    player->elixir_used_this_turn = false;
    player->mirror_used_this_battle = false;
    player->formless_jade_used = false;
    player->disaster_warded = false;
    player->breakthrough_bonus = 0;
    player->breakthrough_fail_penalty = 0;
    player->cultivation = 0;
    player->spirit_materials = 0;
    player->herb_materials = 0;
    player->minor_understanding = 0;
    player->minor_realm_level = 0;
    player->berserk_elixir_turns = 0;
    player->wind_charm_disabled_turns = 0;
}

static void Player_ResetBattleFields(Player *player)
{
    player->current_action_type = ACTION_TYPE_NONE;
    player->burst_count = 0;
    player->healing = 0;
    player->enraged = 0;
    player->bleeding = 0;
    player->cursed = 0;
    player->combo = 0;
    player->HP_change = 0;
    player->QI_change = 0;
    player->damage_received = 0;
    player->elixir_used_this_turn = false;
    player->mirror_used_this_battle = false;
    player->breakthrough_bonus = 0;
    player->breakthrough_fail_penalty = 0;
    player->berserk_elixir_turns = 0;
    player->wind_charm_disabled_turns = 0;
}

static void Player_AddArtifact(Player *player, ArtifactID id);
static void Player_AddElixir(Player *player, ElixirID id);
static void Use_Elixir(Player *player, int slot);
static void Update_Player_Skills(Player *player);
static void Apply_Breakthrough_Rewards(Player *player);
static int Run_UI_JSON_Mode(void);

#include "rpg_talents.inc"

#include "rpg_build.inc"

#include "rpg_growth.inc"

#include "rpg_skill_effects.inc"

#include "rpg_equipment.inc"

#include "rpg_mastery.inc"

#include "rpg_custom_content.inc"
#include "rpg_data.inc"

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
    if (Skill_TypeCanEvade(attacker_skill->type_id))
    {
        if ((rand() % 100) < (defender->evade * 100.0f) && attacker_skill->attribute_id != ATTR_KARMA)
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
    if (Player_SkillRefine(attacker, attacker_skill->skill_id) == SKILL_REFINE_DAMAGE)
        base_damage *= 1.10f;

    if (attacker_skill->action_type == ACTION_TYPE_MELEE || attacker_skill->action_type == ACTION_TYPE_SMITE)
    {
        base_damage *= (1.0f + 0.2f * attacker->combo); // 近战连击增益
    }
    if (Player_HasArtifact(attacker, ARTIFACT_THUNDER_WOOD_SWORD) &&
        (attacker_skill->attribute_id == ATTR_THUNDER || attacker_skill->school_tag == SCHOOL_SWORD) &&
        (rand() % 100) < Artifact_ThunderWoodChance(attacker))
    {
        int bonus = Player_PercentOfMaxHP(attacker, Artifact_ThunderWoodDamagePct(attacker));
        base_damage += bonus;
        CHN_PRINT("[雷击木剑] 雷痕爆发，追加 %d 点伤害！\n", bonus);
    }

    // 特殊处理 Burst 类型的伤害
    if (attacker_skill->type_id == TYPE_BURST)
    {
        base_damage *= attacker->burst_count;
    }

    // 从数据库获取防御方技能实例
    const Skill *defender_skill = &defender->learned_skills[defender->current_action_type];

    SkillTypeInteraction interaction = Get_TypeInteraction(attacker_skill->type_id, defender_skill->type_id, attacker_skill->attribute_id);
    if (interaction.defensive_interaction)
    {
        if (defender_skill->type_id == TYPE_RESIST)
        {
            CHN_PRINT("[%s 使用 %s 来抵抗 %s 的 %s!]\n", defender->name, defender_skill->name_chn, attacker->name, attacker_skill->name_chn);
            ENG_PRINT("[%s uses %s to resist %s's %s!]\n", defender->name, defender_skill->name_eng, attacker->name, attacker_skill->name_eng);
            final_damage = base_damage * defender_skill->base_power;
        }
        else if (defender_skill->type_id == TYPE_SHIELD)
        {
            CHN_PRINT("[%s 使用 %s 来抵挡 %s 的 %s!]\n", defender->name, defender_skill->name_chn, attacker->name, attacker_skill->name_chn);
            ENG_PRINT("[%s uses %s to block %s's %s!]\n", defender->name, defender_skill->name_eng, attacker->name, attacker_skill->name_eng);
            int dmg_blocked = (base_damage > defender_skill->base_power * defender->ATK) ? defender_skill->base_power * defender->ATK : base_damage;
            final_damage = ((base_damage - dmg_blocked) > 0) ? (base_damage - dmg_blocked) : 0;
        }
        else if (defender_skill->type_id == TYPE_PARRY)
        {
            CHN_PRINT("[%s 试图 %s %s 的 %s!]\n", defender->name, defender_skill->name_chn, attacker->name, attacker_skill->name_chn);
            ENG_PRINT("[%s attempts to %s %s's %s!]\n", defender->name, defender_skill->name_eng, attacker->name, attacker_skill->name_eng);
            final_damage = base_damage * interaction.damage_multiplier;
            reflect_damage = base_damage * interaction.reflect_multiplier;
        }
        else if (defender_skill->type_id == TYPE_REFLECT)
        {
            final_damage = base_damage * interaction.damage_multiplier;
            reflect_damage = base_damage * interaction.reflect_multiplier;
        }
        else if (defender_skill->type_id == TYPE_FORCEFIELD)
        {
            CHN_PRINT("[%s 发动 %s 来弹开 %s 的 %s!]\n", defender->name, defender_skill->name_chn, attacker->name, attacker_skill->name_chn);
            ENG_PRINT("[%s launched %s to scatter %s's %s!]\n", defender->name, defender_skill->name_eng, attacker->name, attacker_skill->name_eng);
            final_damage = base_damage * interaction.damage_multiplier;
        }
    }
    else
    {
        final_damage = base_damage;
        CHN_PRINT("[%s 的 %s 击中了正在发动 %s 的 %s!]\n", attacker->name, attacker_skill->name_chn, defender_skill->name_chn, defender->name);
        ENG_PRINT("[%s's %s hits %s who is using %s!]\n", attacker->name, attacker_skill->name_eng, defender_skill->name_chn, defender->name);
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
    if (Player_HasArtifact(defender, ARTIFACT_TURTLE_ARMOR) &&
        (defender_skill->action_type == ACTION_TYPE_DEFEND ||
         defender_skill->action_type == ACTION_TYPE_COUNTER))
    {
        final_damage *= Artifact_TurtleArmorDamageMultiplier(defender);
        CHN_PRINT("[玄龟甲] 护住要害，伤害降低。\n");
    }

    // --- 步骤 6: 根据技能属性施加效果 ---
    switch (attacker_skill->attribute_id)
    {
    case ATTR_BLOOD:
        defender->bleeding += attacker_skill->effect_strength;
        if (Player_SkillRefine(attacker, attacker_skill->skill_id) == SKILL_REFINE_STATUS)
            defender->bleeding += 1;
        if (Player_HasArtifact(attacker, ARTIFACT_BLOOD_BANNER))
            defender->bleeding += Player_PercentOfMaxHP(defender, Artifact_BloodBannerBleedPct(attacker));
        attacker->HP_change += final_damage / 5;
        break;
    case ATTR_DARK:
    {
        int qi_stolen = (defender->QI >= attacker_skill->effect_strength) ? attacker_skill->effect_strength : defender->QI;
        if (Player_SkillRefine(attacker, attacker_skill->skill_id) == SKILL_REFINE_STATUS && qi_stolen < defender->QI)
            qi_stolen += 1;
        defender->QI_change -= qi_stolen;
        break;
    }
    default:
        break;
    }

    Apply_OnHit_SkillEffects(attacker, defender, attacker_skill, (int)final_damage);
    final_damage = RPG_ApplySchoolOnHit(attacker, defender, attacker_skill, (int)final_damage);

    // --- 步骤 7: 应用最终伤害 ---
    if (final_damage > 0)
    {
        final_damage = RPG_ApplySchoolOnDefense(defender, defender_skill, (int)final_damage);
        defender->HP_change -= final_damage;
        if (Player_HasArtifact(defender, ARTIFACT_WIND_CHARM) &&
            attacker_skill->action_type == ACTION_TYPE_SMITE)
        {
            defender->wind_charm_disabled_turns = 1;
            CHN_PRINT("[风行符佩] 被重击震散，短暂失效。\n");
        }
        if (defender_skill->action_type != ACTION_TYPE_DEFEND && defender_skill->action_type != ACTION_TYPE_COUNTER)
            InterruptHealing(attacker, defender); // 造成伤害且对方无防即可打断治疗
    }
    if (reflect_damage > 0)
    {
        attacker->HP_change -= reflect_damage;
        CHN_PRINT("[%s 的攻击被反弹，受到了 %d 点伤害!]\n", attacker->name, (int)reflect_damage);
        ENG_PRINT("[%s's attack was reflected, taking %d damage!]\n", attacker->name, (int)reflect_damage);
    }
}


#ifndef QI_LIBRARY

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
    if (argc > 1 && strcmp(argv[1], "--ui-json") == 0)
    {
        g_ui_json_mode = true;
    }

    Load_Config();
    Initialize_Databases();
    Initialize_RunState();

    if (g_ui_json_mode)
        return Run_UI_JSON_Mode();

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
    srand(g_config.reward_seed ? g_config.reward_seed : (unsigned int)time(NULL));

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
#endif // QI_LIBRARY

void Initialize_RunState()
{
    memset(&g_run, 0, sizeof(g_run));
    g_run.reward_seed = g_config.reward_seed ? g_config.reward_seed : (unsigned int)time(NULL);
    g_run_player_ready = false;
}

void Player_SyncLearnedSkillsFromLoadout(Player *player)
{
    for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
    {
        player->learned_skills[i].skill_id = SKILL_ID_NONE;
        SkillID id = player->equipped_skills[i];
        if (Skill_IsDefined(id) && player->unlocked_skills[id])
            player->learned_skills[i] = g_skill_database[id];
    }
}

int Player_EquipSkill(Player *player, SkillID skill_id)
{
    if (!Skill_IsDefined(skill_id))
        return 0;
    if (!player->unlocked_skills[skill_id])
        return 0;

    ActionType action = g_skill_database[skill_id].action_type;
    if (action < 0 || action >= TOTAL_ACTION_TYPES)
        return 0;
    player->equipped_skills[action] = skill_id;
    Player_SyncLearnedSkillsFromLoadout(player);
    return 1;
}

int Player_UnlockSkill(Player *player, SkillID skill_id, bool bypass_prereq)
{
    if (!Skill_IsDefined(skill_id))
        return 0;
    if (!bypass_prereq && !Skill_PrereqMet(player, skill_id))
        return 0;

    bool was_unlocked = player->unlocked_skills[skill_id];
    player->unlocked_skills[skill_id] = true;
    if (bypass_prereq)
        player->bypassed_prereq_skills[skill_id] = true;

    const Skill *skill = &g_skill_database[skill_id];
    if (!was_unlocked)
        Player_AddSchool(player, skill->school_tag, 1);

    if (!was_unlocked)
    {
        CHN_PRINT("[领悟] %s 获得了 %s。\n", player->name, skill->name_chn);
        ENG_PRINT("[Insight] %s learned %s.\n", player->name, skill->name_eng);
    }

    ActionType action = skill->action_type;
    if (action >= 0 && action < TOTAL_ACTION_TYPES)
    {
        SkillID old_id = player->equipped_skills[action];
        bool should_equip = old_id == SKILL_ID_NONE;
        if (!should_equip && Player_IsHumanFacing(player) && !g_ui_json_mode)
        {
            CHN_PRINT("是否将 [%s] 替换为 [%s]? (y/N): ",
                      g_skill_database[old_id].name_chn, skill->name_chn);
            ENG_PRINT("Replace [%s] with [%s]? (y/N): ",
                      g_skill_database[old_id].name_eng, skill->name_eng);
            fflush(stdout);
            char buffer[16];
            if (fgets(buffer, sizeof(buffer), stdin) && toupper(buffer[0]) == 'Y')
                should_equip = true;
        }
        else if (!should_equip && !Player_IsHumanFacing(player))
        {
            should_equip = skill->rank >= g_skill_database[old_id].rank;
        }

        if (should_equip)
            Player_EquipSkill(player, skill_id);
        else
            Player_SyncLearnedSkillsFromLoadout(player);
    }
    return 1;
}

static void Player_AddArtifact(Player *player, ArtifactID id)
{
    if (id <= ARTIFACT_NONE || id >= TOTAL_ARTIFACTS)
        return;
    if (Player_HasArtifact(player, id))
        return;

    if (player->artifact_count < MAX_ARTIFACT_SLOTS)
    {
        int slot = player->artifact_count++;
        player->artifacts[slot] = id;
        player->artifact_levels[slot] = 0;
    }
    else
    {
        int replace = 0;
        if (Player_IsHumanFacing(player))
        {
            CHN_PRINT("法器已满，选择替换槽位 1-%d，或 0 放弃：\n", MAX_ARTIFACT_SLOTS);
            for (int i = 0; i < MAX_ARTIFACT_SLOTS; i++)
                CHN_PRINT("[%d] %s  ", i + 1, g_artifact_database[player->artifacts[i]].name_chn);
            CHN_PRINT("\n新法器: %s\n", g_artifact_database[id].name_chn);
            char buffer[16];
            if (fgets(buffer, sizeof(buffer), stdin))
                replace = atoi(buffer);
        }
        else
        {
            replace = 1;
        }
        if (replace < 1 || replace > MAX_ARTIFACT_SLOTS)
            return;
        player->artifacts[replace - 1] = id;
        player->artifact_levels[replace - 1] = 0;
    }
    CHN_PRINT("[法器] 获得 %s。\n", g_artifact_database[id].name_chn);
    ENG_PRINT("[Artifact] Gained %s.\n", g_artifact_database[id].name_eng);
}

static void Player_AddElixir(Player *player, ElixirID id)
{
    if (id <= ELIXIR_NONE || id >= TOTAL_ELIXIRS)
        return;
    if (player->elixir_count < MAX_ELIXIR_SLOTS)
    {
        player->elixirs[player->elixir_count++] = id;
    }
    else
    {
        int replace = 0;
        if (Player_IsHumanFacing(player))
        {
            CHN_PRINT("丹药袋已满，选择替换槽位 1-%d，或 0 放弃：\n", MAX_ELIXIR_SLOTS);
            for (int i = 0; i < MAX_ELIXIR_SLOTS; i++)
                CHN_PRINT("[%d] %s  ", i + 1, g_elixir_database[player->elixirs[i]].name_chn);
            CHN_PRINT("\n新丹药: %s\n", g_elixir_database[id].name_chn);
            char buffer[16];
            if (fgets(buffer, sizeof(buffer), stdin))
                replace = atoi(buffer);
        }
        else
        {
            replace = 1;
        }
        if (replace < 1 || replace > MAX_ELIXIR_SLOTS)
            return;
        player->elixirs[replace - 1] = id;
    }
    CHN_PRINT("[丹药] 获得 %s。\n", g_elixir_database[id].name_chn);
    ENG_PRINT("[Elixir] Gained %s.\n", g_elixir_database[id].name_eng);
}

static void Player_PrintSkillLoadout(const Player *player)
{
    CHN_PRINT("\n--- 当前技能槽 ---\n");
    ENG_PRINT("\n--- Current Skill Loadout ---\n");
    for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
    {
        const Skill *skill = &player->learned_skills[i];
        if (skill->skill_id != SKILL_ID_NONE)
        {
            CHN_PRINT("[%s] %s (阶 %d) 成本 %d\n", ActionTypeNames_CHN[i], skill->name_chn, skill->rank, skill->cost);
            ENG_PRINT("[%s] %s (Rank %d) Cost %d\n", ActionTypeNames_ENG[i], skill->name_eng, skill->rank, skill->cost);
        }
        else
        {
            CHN_PRINT("[%s] 空槽\n", ActionTypeNames_CHN[i]);
            ENG_PRINT("[%s] Empty slot\n", ActionTypeNames_ENG[i]);
        }
    }
}

static void Player_PrintArtifacts(const Player *player)
{
    CHN_PRINT("\n--- 法器 ---\n");
    ENG_PRINT("\n--- Artifacts ---\n");
    if (player->artifact_count == 0)
    {
        CHN_PRINT("无\n");
        ENG_PRINT("None\n");
        return;
    }
    for (int i = 0; i < player->artifact_count; i++)
    {
        ArtifactID id = player->artifacts[i];
        if (id >= ARTIFACT_QI_GOURD && id < TOTAL_ARTIFACTS)
        {
            CHN_PRINT("[%d] %s - %s\n", i + 1, g_artifact_database[id].name_chn, g_artifact_database[id].desc_chn);
            ENG_PRINT("[%d] %s - %s\n", i + 1, g_artifact_database[id].name_eng, g_artifact_database[id].desc_eng);
        }
    }
}

static void Player_PrintElixirs(const Player *player)
{
    CHN_PRINT("\n--- 丹药 ---\n");
    ENG_PRINT("\n--- Elixirs ---\n");
    if (player->elixir_count == 0)
    {
        CHN_PRINT("无\n");
        ENG_PRINT("None\n");
        return;
    }
    for (int i = 0; i < player->elixir_count; i++)
    {
        ElixirID id = player->elixirs[i];
        if (id >= ELIXIR_HEALING && id < TOTAL_ELIXIRS)
        {
            CHN_PRINT("[%d] %s - %s\n", i + 1, g_elixir_database[id].name_chn, g_elixir_database[id].desc_chn);
            ENG_PRINT("[%d] %s - %s\n", i + 1, g_elixir_database[id].name_eng, g_elixir_database[id].desc_eng);
        }
    }
}

static void Player_PrintRunProgress(const RunState *run)
{
    CHN_PRINT("\n--- 运行进度 ---\n");
    ENG_PRINT("\n--- Run Progress ---\n");
    CHN_PRINT("已战斗次数: %d, 精英击杀: %d, Boss击杀: %d\n", run->battle_index, run->elite_kills, run->boss_kills);
    ENG_PRINT("Battles: %d, Elites: %d, Bosses: %d\n", run->battle_index, run->elite_kills, run->boss_kills);
}

#include "rpg_rewards.inc"

#include "rpg_encounters.inc"

#include "rpg_ui_json.inc"

static void Use_Elixir(Player *player, int slot)
{
    if (slot < 0 || slot >= player->elixir_count || player->elixir_used_this_turn)
        return;
    ElixirID id = player->elixirs[slot];
    if (id <= ELIXIR_NONE || id >= TOTAL_ELIXIRS)
        return;

    switch (id)
    {
    case ELIXIR_HEALING:
    {
        int heal = Player_MaxHP(player) / 3;
        player->HP += heal;
        CHN_PRINT("[回春丹] 恢复 %d 点元神。\n", heal);
        break;
    }
    case ELIXIR_QI:
    {
        int qi = Player_PercentOfMaxQI(player, 50);
        player->QI += qi;
        CHN_PRINT("[聚气丹] 获得 %d 点气。\n", qi);
        break;
    }
    case ELIXIR_CLEAR_MIND:
        player->bleeding = 0;
        player->cursed = 0;
        CHN_PRINT("[清心丹] 流血与诅咒尽散。\n");
        break;
    case ELIXIR_BERSERK:
        player->enraged += 4;
        player->berserk_elixir_turns = 3;
        CHN_PRINT("[爆元丹] 真元暴涨。\n");
        break;
    case ELIXIR_BREAKTHROUGH:
        player->breakthrough_bonus += 25;
        CHN_PRINT("[破境丹] 本场突破机率提高。\n");
        break;
    case ELIXIR_DISASTER_WARD:
        player->disaster_warded = true;
        CHN_PRINT("[化劫丹] 下一次劫数将被削弱。\n");
        break;
    default:
        if (g_elixir_database[id].is_custom)
        {
            const Elixir *elixir = &g_elixir_database[id];
            if (elixir->heal_hp_pct > 0)
            {
                int heal = Player_PercentOfMaxHP(player, elixir->heal_hp_pct);
                player->HP += heal;
                CHN_PRINT("[%s] 恢复 %d 点元神。\n", elixir->name_chn, heal);
            }
            if (elixir->gain_qi_pct > 0)
            {
                int qi = Player_PercentOfMaxQI(player, elixir->gain_qi_pct);
                player->QI += qi;
                CHN_PRINT("[%s] 获得 %d 点气。\n", elixir->name_chn, qi);
            }
            if (elixir->clear_negative)
            {
                player->bleeding = 0;
                player->cursed = 0;
                CHN_PRINT("[%s] 流血与诅咒尽散。\n", elixir->name_chn);
            }
            if (elixir->breakthrough_pct > 0)
            {
                player->breakthrough_bonus += elixir->breakthrough_pct;
                CHN_PRINT("[%s] 突破机率提高。\n", elixir->name_chn);
            }
            if (elixir->cultivation_gain > 0)
            {
                player->cultivation += elixir->cultivation_gain;
                CHN_PRINT("[%s] 获得 %d 点修为。\n", elixir->name_chn, elixir->cultivation_gain);
            }
        }
        break;
    }

    for (int i = slot; i < player->elixir_count - 1; i++)
        player->elixirs[i] = player->elixirs[i + 1];
    player->elixir_count--;
    player->elixirs[player->elixir_count] = ELIXIR_NONE;
    player->elixir_used_this_turn = true;
    Player_ClampVitals(player);
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
    {
        int gain = player->gain_bonus;
        if (Player_SkillRefine(player, chosen_skill->skill_id) == SKILL_REFINE_QI_FLOW)
            gain = gain * 110 / 100 + 1;
        player->QI_change += gain;
        if (Player_HasArtifact(player, ARTIFACT_QI_GOURD))
        {
            int extra = Artifact_QiGourdExtra(player);
            player->QI_change += extra;
            CHN_PRINT("[聚灵葫芦] 额外聚来 %d 点气。\n", extra);
        }
        CHN_PRINT("%s 集了 %d 点气!\n", player->name, gain);
        ENG_PRINT("%s gained %d QI!\n", player->name, gain);
        break;
    }
    case ACTION_TYPE_BURST:
    {
        int burst_cost_per_hit = Skill_EffectiveCostForPlayer(player, chosen_skill);
        if (burst_cost_per_hit <= 0)
            burst_cost_per_hit = 1;
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
            if (Player_SkillRefine(player, chosen_skill->skill_id) == SKILL_REFINE_STATUS)
                player->enraged += 1;
            break;
        case SKILL_ID_CONCENTRATION:
            player->enraged += 2;
            if (Player_SkillRefine(player, chosen_skill->skill_id) == SKILL_REFINE_STATUS)
                player->enraged += 1;
            player->evade = 0.75f;
            break;
        case SKILL_ID_CORE_ERUPTION:
            player->enraged += 6;
            if (Player_SkillRefine(player, chosen_skill->skill_id) == SKILL_REFINE_STATUS)
                player->enraged += 1;
            break;
        default:
            break;
        }
        break;
    case ACTION_TYPE_HEAL:
        int heal_amount = 0;
        switch (chosen_skill->skill_id)
        {
        case SKILL_ID_HEAL:
            player->healing = Yuan[player->XIUWEI]; // 治疗效果依然和境界有关，可以后续数据化
            heal_amount = 2 * Yuan[player->XIUWEI];
            break;
        case SKILL_ID_EVERGREEN_ART:
            player->healing = Yuan[player->XIUWEI];
            heal_amount = 5 * Yuan[player->XIUWEI];
            player->cursed -= Yuan[player->XIUWEI];
            break;
        case SKILL_ID_CORE_RESTORATION:
            player->healing = Yuan[player->XIUWEI];
            heal_amount = 6 * Yuan[player->XIUWEI];
            player->cursed = 0;
            break;
        }
        if (Player_HasArtifact(player, ARTIFACT_BLOOD_BANNER))
            heal_amount = heal_amount * 3 / 4;
        if (Player_SkillRefine(player, chosen_skill->skill_id) == SKILL_REFINE_HEALING)
            heal_amount = heal_amount * 110 / 100 + 1;
        if (Player_HasArtifact(player, ARTIFACT_GREEN_WOOD_BOTTLE))
        {
            player->bleeding = 0;
            player->cursed = 0;
            CHN_PRINT("[青木瓶] 治疗洗去了流血与诅咒。\n");
        }
        player->HP_change += heal_amount;
        ENG_PRINT("[%s healed for %d HP immediately!]\n", player->name, heal_amount);
        CHN_PRINT("[%s 立即恢复了 %d 点生命值！]\n", player->name, heal_amount);
        break;
    // 其他技能没有需要在这里预处理的逻辑
    default:
        break;
    }

    Apply_OnCast_SkillEffects(player, chosen_skill);

    if (chosen_skill->skill_id == SKILL_ID_SWORD_PHANTOM)
        player->burst_count = 6;
    if (chosen_skill->skill_id == SKILL_ID_GREAT_GOLDEN_SWORDFORMATION)
        player->burst_count = 32;

    if (chosen_skill->action_type == ACTION_TYPE_MELEE || chosen_skill->action_type == ACTION_TYPE_SMITE)
    {
        player->combo++;
    }
    else
    {
        player->combo = 0;
    }
    RPG_RecordSkillCast(player, chosen_skill);
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
        if (skill_in_slot->skill_id != SKILL_ID_NONE &&
            player->QI >= Skill_EffectiveCostForPlayer(player, skill_in_slot))
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
    if (skill_in_slot->skill_id != SKILL_ID_NONE &&
        player->QI >= Skill_EffectiveCostForPlayer(player, skill_in_slot))
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
    if (g_config.run_mode && player == &YOU)
    {
        Player_SyncLearnedSkillsFromLoadout(player);
        return;
    }

    for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
    {
        player->learned_skills[i].skill_id = SKILL_ID_NONE;
        player->equipped_skills[i] = SKILL_ID_NONE;
    }
    for (int i = 0; i < TOTAL_SKILLS; i++)
    {
        const Skill *new_skill = &g_skill_database[i];

        if (Skill_IsDefined((SkillID)i) && new_skill->rank <= player->XIUWEI)
        {
            ActionType category_id = new_skill->action_type;

            if (player->learned_skills[category_id].skill_id == SKILL_ID_NONE ||
                new_skill->rank > player->learned_skills[category_id].rank)
            {
                player->learned_skills[category_id] = *new_skill;
                player->equipped_skills[category_id] = new_skill->skill_id;
                player->unlocked_skills[new_skill->skill_id] = true;
                Player_AddSchool(player, new_skill->school_tag, 1);
            }
        }
    }
}

// 一个内聚的、可重用的函数，负责初始化一个玩家的所有状态
static void Initialize_Player(Player *player, const char *name_eng, const char *name_chn)
{
    bool preserve_run_player = (g_config.run_mode && player == &YOU && g_run_player_ready);

    ENG(player->name = (char *)name_eng);
    CHN(player->name = (char *)name_chn);
    if (!preserve_run_player)
    {
        Player_ResetRunFields(player);
        player->XIUWEI = g_config.initial_xiuwei;
        player->QI = g_config.initial_qi;
        player->evade = g_config.initial_evade > 0 ? g_config.initial_evade : 0.1f;
        player->ATK = Yuan[player->XIUWEI];
        player->YUAN = Yuan[player->XIUWEI];
        player->gain_bonus = player->XIUWEI + 1;
        player->root = (rand() % (TOTAL_ROOT_TYPES - 1)) + 1;
        player->HP = Player_MaxHP(player);
        Player_ResetBattleFields(player);

        if (g_config.run_mode && player == &YOU)
        {
            for (int i = 0; i < TOTAL_SKILLS; i++)
            {
                if (Skill_IsDefined((SkillID)i) && g_skill_database[i].rank == 0)
                    Player_UnlockSkill(player, (SkillID)i, true);
            }
            g_run_player_ready = true;
        }
        else
        {
            Update_Player_Skills(player);
        }
    }
    else
    {
        Player_ResetBattleFields(player);
        player->HP += Player_MaxHP(player) / 4;
        player->QI += Player_MaxQI(player) / 5;
        Player_ClampVitals(player);
        Player_SyncLearnedSkillsFromLoadout(player);
    }

    if (Player_HasArtifact(player, ARTIFACT_DEVOURING_ORB))
        player->cursed += Yuan[player->XIUWEI];
    if (Player_HasArtifact(player, ARTIFACT_WIND_CHARM))
        player->evade += 0.08f;
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
        int hp_heal = Player_MaxHP(player) / 5; // 恢复20%最大生命值
        player->HP += hp_heal;
        Player_ClampVitals(player);
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
        int dmg = Player_MaxHP(player) * (1 + (rand() % 3)) / 10; // 受到10%-30%最大生命伤害
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
    return 0;
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

    Player_ClampVitals(YOU);
    Player_ClampVitals(CPU);
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
        if (player->gain_bonus < 1 << player->XIUWEI)
        {
            player->gain_bonus += 1;
        }
    }
    else
    {
        player->gain_bonus = player->XIUWEI + 1;
    }

    player->current_action_type = ACTION_TYPE_NONE;

    int QI_absorb = 0;
    if (player->XIUWEI >= SEVERING)
        QI_absorb += player->XIUWEI;
    if (game.world >= Spiritual_World)
        QI_absorb += (player->XIUWEI + 1);
    if (QI_absorb > 0)
    {
        player->QI += QI_absorb;
        CHN_PRINT("天人合一！ %d 点气自行从天地间涌入 %s 体内!\n", QI_absorb, player->name);
        ENG_PRINT("Unity with the Cosmos! %d Qi spontaneously surges from the world into %s's body!\n", QI_absorb, player->name);
    }
    Player_ClampVitals(player);
}

// 模块 2: 处理持续性效果 (如治疗、激怒、闪避衰减、流血、诅咒)
static void Resolve_Persistent_Effects(Player *player)
{
    printf("\033[33m");
    if (player->berserk_elixir_turns > 0)
    {
        player->enraged += 1;
        player->berserk_elixir_turns--;
    }
    // 治疗效果
    if (player->healing > 0)
    {
        int max_hp_for_realm = Player_MaxHP(player);
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
    if (Player_HasArtifact(player, ARTIFACT_WIND_CHARM) && player->wind_charm_disabled_turns <= 0)
        base_evade += Artifact_WindCharmEvadeBonus(player);
    if (player->wind_charm_disabled_turns > 0)
        player->wind_charm_disabled_turns--;
    if (player->evade > base_evade)
    {
        player->evade -= 0.5f * (player->evade - base_evade);
    }
    else if (player->evade < base_evade)
    {
        player->evade = base_evade;
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
    Player_ClampVitals(player);

    if (player->HP_change < 0)
    {
        ENG_PRINT("[%s took \033[35m%d\033[33m net damage!]\n", player->name, -(player->HP_change));
        CHN_PRINT("[%s 受到 \033[35m%d\033[33m 点净伤害!]\n", player->name, -(player->HP_change));
    }
    int hp_before = player->HP;
    player->HP += player->HP_change;
    if (player->HP <= 0 && hp_before > 0 &&
        Player_HasArtifact(player, ARTIFACT_KUN_MIRROR) &&
        !player->mirror_used_this_battle)
    {
        player->HP = 1;
        player->mirror_used_this_battle = true;
        CHN_PRINT("[乾坤镜] 逆转生死，保住了一缕元神！\n");
    }
    player->HP_change = 0;
}

// Dedicated Breakthrough Module
static void Apply_Breakthrough_Rewards(Player *player)
{
    // 1. 清空QI (突破消耗)
    player->QI = 0;

    // 2. 根据新的境界，刷新所有派生属性
    player->ATK = Yuan[player->XIUWEI];
    player->YUAN = Yuan[player->XIUWEI];
    player->gain_bonus = player->XIUWEI + 1;
    player->HP = Player_MaxHP(player);

    // 3. 重置动态状态
    player->enraged = 0;
    player->healing = 0;
    player->cursed = 0;
    player->combo = 0;

    // 4. 应用灵根的突破奖励
    float base_evade = (player->root == ROOT_Ethereal) ? 0.1f * player->XIUWEI : 0.05f * player->XIUWEI;
    base_evade += g_config.initial_evade;
    player->evade = base_evade; // 突破后闪避率直接刷新，而不是衰减

    // 5. 重新授予或同步技能
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
    player->combo = 0;

    // 4. 应用灵根的突破奖励
    float base_evade = 1;
    player->evade = base_evade; // 突破后闪避率直接刷新，而不是衰减
}

// 模块 4: 处理突破判定
static void Resolve_Breakthrough(Player *player)
{
    if (g_config.run_mode)
        return;
    if (player->QI >= max_QI[player->XIUWEI])
    {
        if (player->XIUWEI < TOTAL_XIUWEI_LEVEL - 1)
        {
            float breakthrough_chance = (player->root == ROOT_Heavenly) ? 90.0f : 90.0f * exp(-player->XIUWEI / 2.0f);
            if (Player_HasArtifact(player, ARTIFACT_BREAKTHROUGH_SEAL))
                breakthrough_chance += 15.0f + 3.0f * Player_ArtifactLevel(player, ARTIFACT_BREAKTHROUGH_SEAL);
            breakthrough_chance += player->breakthrough_bonus;
            if (breakthrough_chance > 95.0f)
                breakthrough_chance = 95.0f;

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
                int keep_pct = Player_HasArtifact(player, ARTIFACT_BREAKTHROUGH_SEAL) ? 55 : 75;
                keep_pct -= player->breakthrough_fail_penalty;
                if (keep_pct < 25) keep_pct = 25;
                player->QI = max_QI[player->XIUWEI] * keep_pct / 100;
            }
        }
        else
        {
            CHN_PRINT("\033[92m[%s 飞升了!]\033[0m\n", player->name);
            ENG_PRINT("\033[92m[%s made it to Ascension!]\033[0m\n", player->name);

            player->XIUWEI++;

            Apply_Ascension(player);
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
    player->elixir_used_this_turn = false;

    // (回合数上限检查已移至 main 循环，因为它属于游戏全局逻辑)
}

int Start_new_round(Game *game)
{
    game->round_number++;
    YOU.elixir_used_this_turn = false;
    CPU.elixir_used_this_turn = false;

    if (!g_config.run_mode && game->round_number > 1 && game->round_number % 5 == 0 && (rand() % 100) < 30)
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

    if (g_config.run_mode && YOU->elixir_count > 0 && !YOU->elixir_used_this_turn)
    {
        CHN_PRINT("丹药袋：");
        for (int i = 0; i < YOU->elixir_count; i++)
            CHN_PRINT("[%d]%s  ", i + 1, g_elixir_database[YOU->elixirs[i]].name_chn);
        CHN_PRINT("[0]不用\n");
        if (Player_IsHumanFacing(YOU))
        {
            CHN_PRINT("本回合使用丹药? ");
            fflush(stdout);
            char buffer[16];
            if (fgets(buffer, sizeof(buffer), stdin))
            {
                int slot = atoi(buffer);
                if (slot > 0)
                    Use_Elixir(YOU, slot - 1);
            }
        }
        else
        {
            if (YOU->HP < Player_MaxHP(YOU) / 2)
            {
                for (int i = 0; i < YOU->elixir_count; i++)
                {
                    if (YOU->elixirs[i] == ELIXIR_HEALING)
                    {
                        Use_Elixir(YOU, i);
                        break;
                    }
                }
            }
        }
    }

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
        Run_AfterBattleReward(YOU);
        Run_MaybeTriggerEncounter(YOU);
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
            else if (strcmp(key, "RunMode") == 0)
            {
                g_config.run_mode = atoi(value);
            }
            else if (strcmp(key, "AutoReward") == 0)
            {
                g_config.auto_reward = atoi(value);
            }
            else if (strcmp(key, "RewardSeed") == 0)
            {
                g_config.reward_seed = (unsigned int)strtoul(value, NULL, 10);
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
    if (can_perform_action(cpu, ACTION_TYPE_COUNTER) && cpu->XIUWEI >= NASCENT_SOUL)
    {
        cpu->current_action_type = ACTION_TYPE_COUNTER;
        return;
    }
    if (can_perform_action(cpu, ACTION_TYPE_DEFEND) && (rand() % 100 < 50))
    {
        cpu->current_action_type = ACTION_TYPE_DEFEND;
        return;
    }
    if (can_perform_action(cpu, ACTION_TYPE_COUNTER))
    {
        cpu->current_action_type = ACTION_TYPE_COUNTER;
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
    AIThreatProfile threat = AI_ReadThreatProfile(opponent);

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

    if (threat.sword_pressure)
    {
        if (action_type == ACTION_TYPE_DEFEND || action_type == ACTION_TYPE_COUNTER)
            score += 80.0f + opponent->QI * 6.0f;
        if (action_type == ACTION_TYPE_GAIN_QI && opponent->QI >= 4)
            score -= 60.0f;
    }
    if (threat.qi_pressure)
    {
        if (damage > 0 && opponent->QI >= 3)
            score += 55.0f;
        if (action_type == ACTION_TYPE_BOOST)
            score += 35.0f;
        if (action_type == ACTION_TYPE_GAIN_QI && cpu->QI < opponent->QI)
            score += 45.0f;
    }
    if (threat.defensive_shell)
    {
        if (action_type == ACTION_TYPE_MELEE)
            score -= 45.0f;
        if (action_type == ACTION_TYPE_SMITE || action_type == ACTION_TYPE_TERMINATE || action_type == ACTION_TYPE_BURST)
            score += 70.0f;
        if (action_type == ACTION_TYPE_GAIN_QI)
            score += 35.0f;
    }
    if (threat.blood_pressure || threat.dark_pressure || threat.elixir_count >= 3)
    {
        if (opponent->HP <= damage * 1.4f && damage > 0)
            score += 120.0f;
        if (action_type == ACTION_TYPE_HEAL && cpu->HP < max_HP[cpu->XIUWEI] * 0.55f)
            score += 45.0f;
    }

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
    if (action_type == ACTION_TYPE_DEFEND || action_type == ACTION_TYPE_COUNTER)
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
        if (action_type != ACTION_TYPE_DEFEND && action_type != ACTION_TYPE_COUNTER && action_type != ACTION_TYPE_HEAL && action_type != ACTION_TYPE_GAIN_QI)
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
        if ((log->chosen_action == ACTION_TYPE_DEFEND || log->chosen_action == ACTION_TYPE_COUNTER) && log->opponent_action == ACTION_TYPE_SMITE && log->damage_taken < 4)
            turn_reward += 50;

        // [负向] 被斩杀
        if (log->ai_hp > 0 && (log->ai_hp - log->damage_taken) <= 0)
            turn_reward -= 100;
        // [负向] 满血治疗
        if (log->chosen_action == ACTION_TYPE_HEAL && log->ai_hp >= max_HP[log->ai_xiuwei])
            turn_reward -= 20;
        // [负向] 攻击被弹反
        if (log->opponent_action == ACTION_TYPE_COUNTER && log->damage_taken > 0)
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
        case ACTION_TYPE_COUNTER:
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
        size_t nread = fread(&g_ai_weights, sizeof(AI_Weights), 1, file);
        fclose(file);
        if (nread == 1)
        {
            printf("\n[AI weights loaded from file.]\n");
        }
        else
        {
            printf("\n[AI weights file read failed, using defaults.]\n");
        }
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
    if (g_config.run_mode && cpu == &YOU)
    {
        printf("Artifacts: ");
        for (int i = 0; i < cpu->artifact_count; i++)
            printf("%s; ", g_artifact_database[cpu->artifacts[i]].name_eng);
        printf("\nElixirs: ");
        for (int i = 0; i < cpu->elixir_count; i++)
            printf("{Slot:%d, Name:%s}; ", i + 1, g_elixir_database[cpu->elixirs[i]].name_eng);
        printf("\n");
    }

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
