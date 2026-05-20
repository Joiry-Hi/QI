#ifndef QI_H
#define QI_H

#pragma region definitions&macros

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h> // 引入 bool 类型

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#define CLEAR_SCREEN() system("cls")
#define NULL_DEVICE "NUL"
#define GET_CHAR() getchar()
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <termios.h>
#include <unistd.h>
#define CLEAR_SCREEN() system("clear")
#define NULL_DEVICE "/dev/null"
#define GET_CHAR() getchar()
#define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

#define TOTAL_ACTION_AMOUNT 10
#define MAX_LOG_TURNS 100
#define MAX_ROUNDS 500
#define STRATEGIC_CYCLE 5
#define MAX_ARTIFACT_SLOTS 3
#define MAX_ELIXIR_SLOTS 6
#define REWARD_CHOICE_COUNT 3
#define MAX_TALENTS 8
#define MAX_SKILL_EFFECTS 3
#define CUSTOM_SKILL_SLOTS 16
#define CUSTOM_ARTIFACT_SLOTS 12
#define CUSTOM_ELIXIR_SLOTS 12

// 中文开关
#define CHINESE_GAME_LOG

#ifdef CHINESE_GAME_LOG
#define CHN_PRINT(...) printf(__VA_ARGS__)
#define CHN(...) __VA_ARGS__
#define ENG_PRINT(...) (void)0
#define ENG(...)
#else
#define ENG_PRINT(...) printf(__VA_ARGS__)
#define ENG(...) __VA_ARGS__
#define CHN_PRINT(...) (void)0
#define CHN(...)
#endif

// 宏定义开关
//#define AI_TRAINING_SET
//#define INTERACTIVE_AI_MODE 
//#define SLOW_DOWN 1 
//#define WEIGHT_DIRECT_WRITE_ENABLE

#ifdef AI_TRAINING_SET
#define AI_TRAINING(...) __VA_ARGS__
#define HUMAN_PLAYING(...)
#else
#define AI_TRAINING(...)
#define HUMAN_PLAYING(...) __VA_ARGS__
#endif

#ifdef WEIGHT_DIRECT_WRITE_ENABLE
#define DIRECT_WRITE(code_block) code_block
#else
#define DIRECT_WRITE(code_block)
#endif

#pragma endregion definitions &macros

#pragma region Core_Enums
// --- 1. 基础枚举 (逻辑与渲染共用) ---

typedef enum {
    MORTAL = 0, REFINING, FOUNDATION, CORE_FORM, NASCENT_SOUL, SEVERING,
    REFINEMENT, UNITY, GREAT, ASCENSION, IMMORTAL, TOTAL_XIUWEI_LEVEL
} XIUWEI;

typedef enum {
    TARGET_SELF, TARGET_ENEMY, TARGET_NONE
} TargetType;

// 技能的具体物理/魔法效果分类
typedef enum {
    TYPE_NONE,
    TYPE_SLASH, TYPE_SMASH, TYPE_PIERCE, TYPE_BURST, TYPE_BLAST, TYPE_PROJECT,
    TYPE_RESIST, TYPE_SHIELD, TYPE_FORCEFIELD, TYPE_PARRY, TYPE_REFLECT,
    TYPE_HEAL, TYPE_BUFF, TYPE_DEBUFF
} TypeID;

// 元素属性 (渲染引擎据此决定颜色)
typedef enum {
    ATTR_NONE,
    ATTR_PHYSICAL,  // 白/灰
    ATTR_FIRE,      // 红/橙
    ATTR_ICE,       // 蓝白
    ATTR_WIND,      // 青
    ATTR_WOOD,      // 绿
    ATTR_METAL,     // 金/白
    ATTR_THUNDER,   // 紫
    ATTR_EARTH,     // 黄/褐
    ATTR_LIGHT,     // 金
    ATTR_DARK,      // 黑
    ATTR_BLOOD,     // 深红
    ATTR_SPIRITUAL, // 浅蓝/透明
    ATTR_KARMA,     // 神秘色
    ATTR_SPACE      // 扭曲/深蓝
} AttributeID;

// 宏观行动类别
typedef enum {
    ACTION_TYPE_GAIN_QI,
    ACTION_TYPE_MELEE,
    ACTION_TYPE_RANGED,
    ACTION_TYPE_DEFEND,
    ACTION_TYPE_HEAL,
    ACTION_TYPE_COUNTER,
    ACTION_TYPE_BOOST,
    ACTION_TYPE_SMITE,
    ACTION_TYPE_BURST,
    ACTION_TYPE_TERMINATE,
    TOTAL_ACTION_TYPES,
    ACTION_TYPE_NONE = -1
} ActionType;

// 技能 ID (唯一标识符)
typedef enum {
    SKILL_ID_NONE = -1,
    // 凡人
    SKILL_ID_GAIN_QI = 0, SKILL_ID_STRIKE, SKILL_ID_DEFEND, SKILL_ID_HEAL,
    SKILL_ID_WARCRY, SKILL_ID_PARRY, SKILL_ID_SMITE,
    // 炼气
    SKILL_ID_FIREBALL, SKILL_ID_ENERGY_SHIELD, SKILL_ID_WINDBLADE,
    SKILL_ID_EVERGREEN_ART, SKILL_ID_CONCENTRATION,
    // 筑基
    SKILL_ID_FLAMEBLAST, SKILL_ID_GOLD_LIGHT_WARDING, SKILL_ID_GREATSWORD,
    SKILL_ID_COMMANDING_SWORDS, SKILL_ID_TERMINATE_THUNDER,
    // 结丹
    SKILL_ID_SWORD_PHANTOM, SKILL_ID_BLOOD_DEVIL_DRILL, SKILL_ID_CORE_RESTORATION,
    SKILL_ID_CORE_ERUPTION, SKILL_ID_BLOOD_DEVIL_SLASH, SKILL_ID_BEETLE_SWARM,
    SKILL_ID_ICE_FLAME,
    // 元婴
    SKILL_ID_IMMOVABLE_KING, SKILL_ID_STELLAR_SHIFT, SKILL_ID_ESSENCE_PLUNDER,
    SKILL_ID_COSMIC_DHARMA_AVATAR, SKILL_ID_GREAT_GOLDEN_SWORDFORMATION,
    // 化神
    SKILL_ID_SPIRIT_SLAYING_SWORD,
    CUSTOM_SKILL_START,
    CUSTOM_SKILL_00 = CUSTOM_SKILL_START, CUSTOM_SKILL_01, CUSTOM_SKILL_02, CUSTOM_SKILL_03,
    CUSTOM_SKILL_04, CUSTOM_SKILL_05, CUSTOM_SKILL_06, CUSTOM_SKILL_07,
    CUSTOM_SKILL_08, CUSTOM_SKILL_09, CUSTOM_SKILL_10, CUSTOM_SKILL_11,
    CUSTOM_SKILL_12, CUSTOM_SKILL_13, CUSTOM_SKILL_14, CUSTOM_SKILL_15,
    TOTAL_SKILLS
} SkillID;

typedef enum {
    ROOT_Mortal, ROOT_Heavenly, ROOT_Solid, ROOT_Sharp, ROOT_Ethereal, TOTAL_ROOT_TYPES
} SpiritualRootID;

typedef enum {
    FATE_None, FATE_Qi_Infusion, FATE_Vitality_Blessing, FATE_Enlightenment,
    FATE_Agile_Wind, FATE_Calamity, TOTAL_FATE_TYPES
} FateID;

typedef enum {
    Mortal_World, Spiritual_World, TOTAL_WORLD_COUNT
} WORLD;

typedef enum {
    SCHOOL_NONE,
    SCHOOL_BASIC,
    SCHOOL_FIRE,
    SCHOOL_SWORD,
    SCHOOL_BLOOD,
    SCHOOL_DEFENSE,
    SCHOOL_QI,
    SCHOOL_THUNDER,
    SCHOOL_DARK,
    TOTAL_SCHOOLS
} SchoolTag;

typedef enum {
    REWARD_ARTIFACT,
    REWARD_ELIXIR,
    REWARD_SKILL,
    REWARD_CULTIVATION
} RewardType;

typedef enum {
    RARITY_COMMON,
    RARITY_RARE,
    RARITY_EPIC,
    RARITY_LEGENDARY,
    TOTAL_RARITIES
} RewardRarity;

typedef enum {
    ARTIFACT_NONE = -1,
    ARTIFACT_QI_GOURD,
    ARTIFACT_TURTLE_ARMOR,
    ARTIFACT_BLOOD_BANNER,
    ARTIFACT_THUNDER_WOOD_SWORD,
    ARTIFACT_KUN_MIRROR,
    ARTIFACT_DEVOURING_ORB,
    ARTIFACT_GREEN_WOOD_BOTTLE,
    ARTIFACT_BREAKTHROUGH_SEAL,
    ARTIFACT_WIND_CHARM,
    ARTIFACT_FORMLESS_JADE,
    CUSTOM_ARTIFACT_START,
    CUSTOM_ARTIFACT_00 = CUSTOM_ARTIFACT_START, CUSTOM_ARTIFACT_01, CUSTOM_ARTIFACT_02, CUSTOM_ARTIFACT_03,
    CUSTOM_ARTIFACT_04, CUSTOM_ARTIFACT_05, CUSTOM_ARTIFACT_06, CUSTOM_ARTIFACT_07,
    CUSTOM_ARTIFACT_08, CUSTOM_ARTIFACT_09, CUSTOM_ARTIFACT_10, CUSTOM_ARTIFACT_11,
    TOTAL_ARTIFACTS
} ArtifactID;

typedef enum {
    ELIXIR_NONE = -1,
    ELIXIR_HEALING,
    ELIXIR_QI,
    ELIXIR_CLEAR_MIND,
    ELIXIR_BERSERK,
    ELIXIR_BREAKTHROUGH,
    ELIXIR_DISASTER_WARD,
    CUSTOM_ELIXIR_START,
    CUSTOM_ELIXIR_00 = CUSTOM_ELIXIR_START, CUSTOM_ELIXIR_01, CUSTOM_ELIXIR_02, CUSTOM_ELIXIR_03,
    CUSTOM_ELIXIR_04, CUSTOM_ELIXIR_05, CUSTOM_ELIXIR_06, CUSTOM_ELIXIR_07,
    CUSTOM_ELIXIR_08, CUSTOM_ELIXIR_09, CUSTOM_ELIXIR_10, CUSTOM_ELIXIR_11,
    TOTAL_ELIXIRS
} ElixirID;

typedef enum {
    ENCOUNTER_ANCIENT_CAVE,
    ENCOUNTER_WOUNDED_BEAST,
    ENCOUNTER_TRIBULATION_CLOUD,
    ENCOUNTER_MARKET,
    ENCOUNTER_SWORD_TOMB,
    ENCOUNTER_BLOOD_POOL,
    TOTAL_ENCOUNTERS
} EncounterID;

typedef enum {
    TALENT_NONE = -1,
    TALENT_QI_SEA,
    TALENT_GOLDEN_BODY,
    TALENT_SWORD_HEART,
    TALENT_ALCHEMY_RESIDUE,
    TALENT_THUNDER_AFFINITY,
    TALENT_BLOOD_REBIRTH,
    TALENT_CAVE_INSIGHT,
    TALENT_WANDERER,
    TOTAL_TALENTS
} TalentID;

typedef enum {
    SOUL_BODY,
    SOUL_GHOST
} SoulState;

typedef enum {
    CUSTOM_EFFECT_NONE,
    CUSTOM_EFFECT_DAMAGE,
    CUSTOM_EFFECT_HEAL_HP_PCT,
    CUSTOM_EFFECT_GAIN_QI_PCT,
    CUSTOM_EFFECT_BLEED,
    CUSTOM_EFFECT_CURSE,
    CUSTOM_EFFECT_ENRAGE,
    CUSTOM_EFFECT_CLEANSE,
    CUSTOM_EFFECT_CULTIVATION
} CustomEffectType;

typedef enum {
    SKILL_EFFECT_NONE,
    SKILL_EFFECT_DAMAGE_PCT,
    SKILL_EFFECT_HEAL_HP_PCT,
    SKILL_EFFECT_GAIN_QI_PCT,
    SKILL_EFFECT_BLEED,
    SKILL_EFFECT_CURSE,
    SKILL_EFFECT_ENRAGE,
    SKILL_EFFECT_CLEANSE,
    SKILL_EFFECT_STEAL_QI,
    SKILL_EFFECT_SELF_DAMAGE_PCT,
    SKILL_EFFECT_CULTIVATION,
    SKILL_EFFECT_LIFESTEAL_PCT
} SkillEffectType;

#pragma endregion Core_Enums

#pragma region Logic_Structs
// --- 2. 逻辑核心结构体 ---

typedef struct {
    SkillEffectType type;
    int value;
    float chance;
} SkillEffect;

typedef struct Skill_s {
    SkillID skill_id;
    ActionType action_type;
    const char *name_chn;
    const char *name_eng;
    char hotkey;
    int cost;
    int rank;
    TypeID type_id;
    AttributeID attribute_id;
    float base_power;
    int effect_strength;
    float effect_chance;
    TargetType target_type;
    const char *prompt_chn;
    const char *prompt_eng;
    SkillID prereq_skill_id;
    ActionType prereq_action_type;
    XIUWEI required_realm;
    SchoolTag school_tag;
    bool is_custom;
    CustomEffectType custom_effect;
    int custom_value;
    SkillEffect effects[MAX_SKILL_EFFECTS];
    int effect_count;
} Skill;

typedef struct {
    ArtifactID id;
    const char *name_chn;
    const char *name_eng;
    const char *desc_chn;
    const char *desc_eng;
    RewardRarity rarity;
    bool is_custom;
    int max_hp_pct;
    int max_qi_pct;
    int breakthrough_pct;
    int post_battle_cultivation;
    SchoolTag reward_school_bias;
} Artifact;

typedef struct {
    ElixirID id;
    const char *name_chn;
    const char *name_eng;
    const char *desc_chn;
    const char *desc_eng;
    RewardRarity rarity;
    bool is_custom;
    int heal_hp_pct;
    int gain_qi_pct;
    int clear_negative;
    int breakthrough_pct;
    int cultivation_gain;
} Elixir;

typedef struct {
    TalentID id;
    const char *name_chn;
    const char *name_eng;
    const char *desc_chn;
    const char *desc_eng;
    SchoolTag school_tag;
} Talent;

typedef struct {
    RewardType type;
    RewardRarity rarity;
    int id;
    int amount;
    bool bypass_prereq;
} RewardChoice;

typedef struct {
    EncounterID id;
    const char *title_chn;
    const char *title_eng;
    RewardChoice choices[REWARD_CHOICE_COUNT];
    const char *choice_chn[REWARD_CHOICE_COUNT];
    const char *choice_eng[REWARD_CHOICE_COUNT];
} EncounterChoice;

typedef struct {
    int battle_index;
    int elite_kills;
    int boss_kills;
    unsigned int reward_seed;
    RewardChoice pending_reward_choices[REWARD_CHOICE_COUNT];
} RunState;

typedef struct {
    SchoolTag primary_school;
    SchoolTag secondary_school;
    int primary_count;
    int secondary_count;
    int unlocked_skill_count;
    int equipped_skill_count;
    int artifact_count;
    int elixir_count;
    const char *synergy_label;
    const char *synergy_bonus;
} BuildProfile;

typedef struct Player_s {
    char *name;
    int HP, QI, ATK, YUAN;
    int cultivation;
    XIUWEI XIUWEI;
    ActionType current_action_type;
    Skill learned_skills[TOTAL_ACTION_TYPES];
    bool unlocked_skills[TOTAL_SKILLS];
    SkillID equipped_skills[TOTAL_ACTION_TYPES];
    bool bypassed_prereq_skills[TOTAL_SKILLS];
    ArtifactID artifacts[MAX_ARTIFACT_SLOTS];
    int artifact_count;
    ElixirID elixirs[MAX_ELIXIR_SLOTS];
    int elixir_count;
    bool talents[TOTAL_TALENTS];
    int talent_count;
    SoulState soul_state;
    int school_counts[TOTAL_SCHOOLS];
    bool elixir_used_this_turn;
    bool mirror_used_this_battle;
    bool formless_jade_used;
    bool disaster_warded;
    int breakthrough_bonus;
    int breakthrough_fail_penalty;
    int berserk_elixir_turns;
    int wind_charm_disabled_turns;
    Skill action_snapshot; // 记录本回合使用的技能快照
    int gain_bonus, burst_count, healing, enraged, bleeding, cursed, combo;
    float evade;
    SpiritualRootID root;
    int HP_change, QI_change, damage_received;
} Player;

typedef struct {
    ActionType action_type;
} TurnHistoryLog;

typedef struct Game_s {
    WORLD world;
    int round_number;
    char action;
    int opponent_type;
    int AI_type;
    int current_general_id;
    TurnHistoryLog player_turn_history[STRATEGIC_CYCLE];
    int history_log_count;
    int is_bridge_mode;
} Game;

typedef struct {
    int initial_hp, initial_qi, initial_xiuwei;
    float initial_evade, time_delay;
    int train_reps, enemy_type, ai_type, world;
    int enable_ai_randomness;
    int run_mode, auto_reward;
    unsigned int reward_seed;
} GameConfig;

typedef struct {
    ActionType action_type;
    float score;
} ActionScore;

typedef struct {
    float w_health_urgency, w_damage_per_point, w_kill_shot_bonus, w_interrupt_heal_bonus,
          w_low_qi_gather, w_defend_vs_high_qi, w_qi_advantage, w_damage_per_qi,
          w_low_hp_penalty, w_breakthrough_urgency, w_self_buff_value;
} AI_Weights;

typedef struct {
    int round_number;
    ActionType chosen_action, opponent_action;
    int ai_hp, opponent_hp, ai_qi, opponent_qi, ai_xiuwei;
    int action_cost, damage_dealt, damage_taken;
} AI_TurnLog;

#pragma endregion Logic_Structs

#pragma region Global_Vars
extern Player YOU;
extern Player CPU;
extern Game game;
extern GameConfig g_config;
extern char *Realm[TOTAL_XIUWEI_LEVEL];
extern int max_HP[TOTAL_XIUWEI_LEVEL], max_QI[TOTAL_XIUWEI_LEVEL], Yuan[TOTAL_XIUWEI_LEVEL];
extern Skill g_skill_database[TOTAL_SKILLS];
extern Artifact g_artifact_database[TOTAL_ARTIFACTS];
extern Elixir g_elixir_database[TOTAL_ELIXIRS];
extern Talent g_talent_database[TOTAL_TALENTS];
extern RunState g_run;
#pragma endregion Global_Vars

#pragma region Prototypes
// --- 4. 函数原型 ---

// Data Manager
void Initialize_Databases();
void Load_Config();
void Initialize_RunState();
void Player_SyncLearnedSkillsFromLoadout(Player *player);
int Player_UnlockSkill(Player *player, SkillID skill_id, bool bypass_prereq);
int Player_EquipSkill(Player *player, SkillID skill_id);
BuildProfile Player_GetBuildProfile(const Player *player);
void Player_PrintBuildSummary(const Player *player);
void Generate_RewardChoices(Player *player, RewardChoice choices[REWARD_CHOICE_COUNT]);
void Apply_RewardChoice(Player *player, const RewardChoice *choice);
void Run_AfterBattleReward(Player *player);
void Run_MaybeTriggerEncounter(Player *player);

// Game Flow
void Game_init(Player *YOU, Player *CPU, Game *game);
int Start_new_round(Game *game);
void Player_action(Game game, Player *YOU);
void CPU_action(Player *CPU);
void Action_resolve(Player *YOU, Player *CPU);
void Status_settlement(Player *player);
void Game_summary(Player *YOU, Player *CPU);
int Trigger_Fate(Player *player);

// Combat Logic
void Oneway_Solution(Player *attacker, Player *defender);
int InterruptHealing(const Player *attacker, Player *target);

// AI System
int get_affordable_actions(const Player *player, ActionType affordable_actions[]);
float EvaluateAction(ActionType action_type, const Player *cpu, const Player *opponent, const AI_Weights *weights);
void CPU_logic_V0_Random(Player *cpu, const Player *opponent);
void CPU_logic_V1A_Disruptor(Player *cpu, const Player *opponent);
void CPU_logic_V1B_Berserker(Player *cpu, const Player *opponent);
void CPU_logic_V1C_Turtle(Player *cpu, const Player *opponent);
void CPU_logic_V1D_Ascetic(Player *cpu, const Player *opponent);
void CPU_logic_V1E_Gambler(Player *cpu, const Player *opponent);
void CPU_logic_V2_Genius(Player *cpu, const Player *opponent);
void CPU_logic_V2A_Tuned(Player *cpu, const Player *opponent, int A);

// AI Learning System
void AI_Learn_From_Game(int ai_won);
void Load_AI_Weights();
void Save_AI_Weights();

// 模式1: Per-Turn LLM (事无巨细)
void Build_Per_Turn_Genesis_Prompt();
void Build_Turn_Update_Prompt(const Player *cpu, const Player *opponent);
void CPU_logic_LLM(Player *cpu, const Player *opponent);

// 模式2: Grand Marshal (将帅分级)
void Build_Marshal_Genesis_Prompt();
void Build_Strategic_Report_Prompt(const Player *cpu, const Player *opponent, const Game *game);
void Request_Strategic_Decision(Player *cpu, Player *opponent, Game *game);

#pragma endregion Prototypes

#endif // QI_H
