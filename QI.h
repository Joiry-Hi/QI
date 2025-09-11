#ifndef QI_H
#define QI_H

// 在包含 windows.h 之前，手动定义我们需要的最低Windows版本。
// 0x0A00 对应 Windows 10。
// 这会确保 <windows.h> 包含所有现代终端功能的宏定义。
#define _WIN32_WINNT 0x0A00

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <windows.h>
#include <conio.h>

#define TOTAL_ACTION_AMOUNT 10
#define REALM_COUNT 10
#define COST_LEVELS 7
#define MAX_LOG_TURNS 100
#define MAX_ROUNDS 500

// --- 汉化控制开关 ---
// #define CHINESE_GAME_LOG 1

// 定义一个我们自己的打印函数 LOG_PRINTF
#ifdef CHINESE_GAME_LOG
// 如果开关是打开的，LOG_PRINTF 就等同于普通的 printf
#define CHN_PRINT(...) printf(__VA_ARGS__)
#define CHN(...) __VA_ARGS__
#define ENG_PRINT(...) (void)0
#define ENG(...) 
#else
// 如果开关是关闭的，LOG_PRINTF 就什么也不做
#define ENG_PRINT(...) printf(__VA_ARGS__)
#define ENG(...) __VA_ARGS__
#define CHN_PRINT(...) (void)0
#define CHN(...) 
#endif
// --------------------

// --- AI训练开关 ---
 #define AI_TRAINING_SET 1

// 定义一个宏来控制 AI_TRAINING 和 HUMAN_PLAYING 的行为
#ifdef AI_TRAINING_SET
// 如果开关是打开的，AI_TRAINING 后面内容生效
#define AI_TRAINING(...) __VA_ARGS__
#define HUMAN_PLAYING(...) 
#else
// 如果开关是关闭的，HUMAN_PLAYING 后面内容生效
#define AI_TRAINING(...) 
#define HUMAN_PLAYING(...) __VA_ARGS__
#endif
// --------------------

// --- 权重直接写入 ---
// #define WEIGHT_DIRECT_WRITE_ENABLE 1

#ifdef WEIGHT_DIRECT_WRITE_ENABLE
#define DIRECT_WRITE(code_block) code_block
#else
#define DIRECT_WRITE(code_block)
#endif
// --------------------

// Forward-declarations for circular dependencies
struct Player_s;
struct InteractionRule_s;

// --- NEW: Core Structures for Data-Driven Resolution ---

// Defines the results of an interaction, keeping logic and printing separate.
typedef struct
{
    float damage_to_obj;
    float damage_to_sbj;
    const char *message;
} ResolutionResult;

// Defines the function pointer for complex interactions that need to check player state.
typedef ResolutionResult (*RuleOverrideHandler)(const struct InteractionRule_s *rule, const struct Player_s *sbj, const struct Player_s *obj);

// Defines a rule for a single interaction.
// It contains simple data for common cases and a function pointer for complex ones.
typedef struct InteractionRule_s
{
    float damage_multiplier;              // Default damage multiplier to the object (target).
    float reflect_multiplier;             // Default damage multiplier to the subject (attacker).
    const char *message;                  // The default message for this interaction.
    RuleOverrideHandler override_handler; // A pointer to a function for special logic. NULL if none.
} InteractionRule;

// --- Original Structures ---

typedef struct
{
    int round_number;
    char action;
    int opponent_type;
} Game;

typedef enum {
    ROOT_Mortal,        // 凡根 (无特殊效果)
    ROOT_Heavenly,      // 天灵根 (突破成功率极高)
    ROOT_Solid,         // 厚土灵根 (HP成长更高，防御效果更好)
    ROOT_Sharp,         // 锐金灵根 (ATK成长更高，攻击技能更强)
    ROOT_Ethereal,      // 风灵根 (闪避率成长更高，身法技能更强)
    TOTAL_ROOT_TYPES
} SpiritualRootID;

typedef enum
{
    Gain_qi = 0,
    Melee = 1,
    Defend = 2,
    Heal = 3,
    Boost = 4,
    Parry = 5,
    Smite = 6,
    Ranged = 7,
    Burst = 8,
    Terminate = 9,
    TOTAL_ACTION_TYPES, // Automatically counts the number of real actions
    None = -1
} ActionID;

// Player structure to hold all data related to a player.
typedef struct Player_s
{
    char *name;
    int HP;
    int QI;
    int ATK;
    int YUAN;
    int XIUWEI;
    ActionID action; // Will now use ActionID enum
    int gain_combo;
    int burst_count;
    int healing;
    int enraged;
    float evade; // 思考该如何处理
    SpiritualRootID root;
    int damage_received;
    int action_cost;
} Player;

typedef enum
{
    Slash = 0,
    Smash = 1,
    Pierce = 2,
    Sweep = 3,
    Tear = 4,
    Entangle = 5,
    Shield = 6,
    Force_field = 7,
    Absorb = 8,
    Block = 9,
    Reflect = 10
} TypeID;

typedef enum
{
    Metal = 0,
    Wood = 1,
    Water = 2,
    Fire = 3,
    Earth = 4,
    Spirit = 5,
    Shadow = 6,
    Light = 7,
    Time = 8,
    Lightning = 9,
    Dark = 10,
} AttributeID;

typedef struct Skill_s
{
    char *name; // 技能名称
    int cost;   // 技能消耗的QI
    ActionID action;  // 技能类型
    int rank;  // 同一大类的高等级技能会覆盖低等级技能
    TypeID type;  // 细分类型
    AttributeID attribute; //技能属性
} Skill;

// --- configurable parameters ---

typedef struct {
    int initial_hp;
    int initial_qi;
    int initial_xiuwei;
    float initial_evade;
    int train_reps;
    int enemy_type;
} GameConfig;

extern GameConfig g_config;

void Load_Config();

typedef struct
{
    Player you;
    Player cpu;
    Game game_info;
} GameState;

typedef struct
{
    int length;
    int list[TOTAL_ACTION_AMOUNT];
} Act_list;

// --- Global Data Declarations ---

extern Player YOU;
extern Player CPU;
extern Game game;

extern char *Realm[10];
extern int max_HP[10];
extern int max_QI[10];
extern int Yuan[10];

#pragma region action_lists
// ActionID Enum provides readable names for all actions.

extern Act_list act_cost[COST_LEVELS];
extern Act_list act_able[COST_LEVELS];
extern Act_list act_able_xiuwei[REALM_COUNT][COST_LEVELS];
extern Act_list act_mortal;
extern Act_list act_refine;
extern Act_list act_foundation;
#pragma endregion action_lists

// --- Function Prototypes ---

#pragma region functions
// Tool Functions
void act_list_join(Act_list *list, Act_list *source);
void list_print(int *list, int length);
void clear_buffer();
int check_list(int *list, int length, int target);
void delete_element(Act_list *source, int element);
void sort_list(int *list, int length);
void delete_to(Act_list list_of_list[], int target, int from, int to);

// Game Flow Functions
void Initialize_Action_Lists();
void Act_list_print();
void Game_init(Player *YOU, Player *CPU, Game *game);
int Start_new_round(Game *game);
void Player_action(Game game, Player *YOU);
void CPU_logic(Player *CPU);
void CPU_action(Player *CPU);
void Action_resolve(Player *YOU, Player *CPU);
void Oneway_Solution(Player *SBJ, Player *OBJ); // This will be rewritten
void Status_settlement(Player *player);
void Game_summary(Player *YOU, Player *CPU);
int Trigger_Fate(Player *player);

// NEW: Helper function for action resolution
int InterruptHealing(const Player *attacker, Player *target);
#pragma endregion functions

// --- 机缘与灵根系统 ---

typedef enum {
    FATE_None,                  // 无事发生
    FATE_Qi_Infusion,           // 天地灵气灌体 (瞬间获得大量QI)
    FATE_Vitality_Blessing,     // 生命甘露 (瞬间恢复部分HP)
    FATE_Enlightenment,         // 顿悟 (下一次攻击伤害翻倍)
    FATE_Agile_Wind,            // 风灵庇佑 (接下来3回合闪避率临时提升)
    FATE_Calamity,              // 天降小劫 (随机受到少量伤害，作为负面机缘)
    TOTAL_FATE_TYPES
} FateID;

extern const char* Eng_Root_Names[TOTAL_ROOT_TYPES];
extern const char* CHN_Root_Names[TOTAL_ROOT_TYPES];

// --- AI Optimization ---

#pragma region AI

typedef struct
{
    ActionID action_id;
    float score;
} ActionScore;

// 存储AI所有可调整的决策权重 (已扩展)
typedef struct
{
    // --- 原有权重 ---
    float w_health_urgency;
    float w_damage_per_point;
    float w_kill_shot_bonus;
    float w_interrupt_heal_bonus;
    float w_low_qi_gather;
    float w_defend_vs_high_qi;
    float w_qi_advantage;
    float w_damage_per_qi;
    float w_low_hp_penalty;
    float w_breakthrough_urgency; 
    float w_self_buff_value;

} AI_Weights;

// 记录AI在一回合内的关键决策信息 (已扩展)
typedef struct
{
    int round_number;
    ActionID chosen_action;

    // 当时的“战况快照”
    int cpu_hp;
    int opponent_hp;
    int cpu_qi;
    int opponent_qi;
    int cpu_xiuwei;

    ActionID opponent_action; // 对手的行动
    int action_cost;          // 我方行动的QI消耗

    // 这回合造成的实际伤害/受到的伤害
    int damage_dealt;
    int damage_taken;

} AI_TurnLog;

void CPU_logic_V0(Player *CPU, const Player *opponent);

void CPU_logic_V1(Player *cpu, const Player *opponent);
void CPU_logic_V1A(Player *cpu, const Player *opponent);
void CPU_logic_V1B(Player *cpu, const Player *opponent);
void CPU_logic_V1C(Player *cpu, const Player *opponent);
void CPU_logic_V1D(Player *cpu, const Player *opponent);
void CPU_logic_V1E(Player *cpu, const Player *opponent);
void CPU_logic_V1F(Player *cpu, const Player *opponent);

float EvaluateAction(ActionID action, const Player *cpu, const Player *opponent, const AI_Weights* weights);

void CPU_logic_V2(Player *cpu, const Player *opponent);
void CPU_logic_V2A(Player *cpu, const Player *opponent, int A);

void AI_Learn_From_Game(int cpu_won);

void Load_AI_Weights();

void Save_AI_Weights();
#pragma endregion AI

#endif // QI_H