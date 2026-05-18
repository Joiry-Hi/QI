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

// 实体 ID 定义 (协议部分)
#define ENTITY_ID_YOU 0
#define ENTITY_ID_CPU 1

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
#define AI_TRAINING_SET
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

#pragma endregion Core_Enums

#pragma region Protocol_Defs
// --- 2. 通信协议定义 (原 qi_shared.h 内容) ---

// 事件类型
typedef enum {
    EVENT_TYPE_DAMAGE,          // 伤害
    EVENT_TYPE_HEAL,            // 治疗
    EVENT_TYPE_MODIFIER_APPLY,  // 加Buff
    EVENT_TYPE_MODIFIER_FADE,   // Buff消失
    EVENT_TYPE_QI_CHANGE,       // 气变化
    EVENT_TYPE_BREAKTHROUGH,    // 突破
    EVENT_TYPE_GAME_STATE       // 游戏状态
} EventType;

// 事件结果类型 (用于视觉反馈)
typedef enum {
    RESULT_TYPE_NORMAL,
    RESULT_TYPE_BLOCKED,        // 被护盾减伤
    RESULT_TYPE_PARRIED,        // 被弹反
    RESULT_TYPE_REFLECTED,      // 伤害反弹
    RESULT_TYPE_EVADED,         // 闪避
    RESULT_TYPE_INTERRUPTED     // 打断
} ResultType;

// 核心战斗事件包
typedef struct {
    EventType  event_type;
    ResultType result_type;
    int source_entity_id;
    int target_entity_id;
    int skill_id_used;      // SkillID
    int value;              // 伤害值/治疗量
    int value2;             // 备用数值
} CombatEvent;

// 实体快照 (用于UI渲染)
typedef struct {
    char name[32];
    int hp, max_hp;
    int qi, max_qi;
    int xiuwei_level;
} EntitySnapshot;

#pragma endregion Protocol_Defs

#pragma region Logic_Structs
// --- 3. 逻辑核心结构体 ---

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
} Skill;

typedef struct Player_s {
    char *name;
    int HP, QI, ATK, YUAN;
    XIUWEI XIUWEI;
    ActionType current_action_type;
    Skill learned_skills[TOTAL_ACTION_TYPES];
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
#pragma endregion Global_Vars

#pragma region Prototypes
// --- 4. 函数原型 ---

// Data Manager
void Initialize_Databases();
void Load_Config();

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

// *** Bridge Interface (桥接接口) ***
// 图形引擎调用的入口点
void Record_Event(EventType type, ResultType result, int src_id, int tgt_id, int skill_id, int val, int val2);
void Engine_Initialize_Bridge();
CombatEvent* Engine_CalculateNextTurn(ActionType player_action, int* out_event_count);
void Engine_GetEntitySnapshot(int entity_id, EntitySnapshot* out_snapshot);

#pragma endregion Prototypes

#endif // QI_H