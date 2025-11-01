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

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#define CLEAR_SCREEN() system("cls")
#define NULL_DEVICE "NUL"
#define GET_CHAR() _getch()
#else
#include <termios.h>
#include <unistd.h>
#define CLEAR_SCREEN() system("clear")
#define NULL_DEVICE "/dev/null"
int getch_linux();
#define GET_CHAR() getch_linux()
#endif

#define TOTAL_ACTION_AMOUNT 10
#define REALM_COUNT 10
#define MAX_LOG_TURNS 100
#define MAX_ROUNDS 500    // 每局最大回合数
#define STRATEGIC_CYCLE 5 // 定义战略周期为5回合

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

// AI训练开关
#define AI_TRAINING_SET

#define INTERACTIVE_AI_MODE // 是否保持输出流

#ifdef AI_TRAINING_SET
#define AI_TRAINING(...) __VA_ARGS__
#define HUMAN_PLAYING(...)
#else
#define AI_TRAINING(...)
#define HUMAN_PLAYING(...) __VA_ARGS__
#endif

// 权重直接写入开关
// #define WEIGHT_DIRECT_WRITE_ENABLE

#ifdef WEIGHT_DIRECT_WRITE_ENABLE
#define DIRECT_WRITE(code_block) code_block
#else
#define DIRECT_WRITE(code_block)
#endif
#pragma endregion definitions &macros

#pragma region Core_Data_Structures
// 1. 先定义所有依赖的基础枚举 (Enums)
typedef enum
{
    TARGET_SELF,
    TARGET_ENEMY,
    TARGET_NONE
} TargetType;

typedef enum
{
    TYPE_NONE,
    TYPE_SLASH,
    TYPE_SMASH,
    TYPE_PIERCE,
    TYPE_BURST,
    TYPE_SHIELD,
    TYPE_PARRY,
    TYPE_HEAL,
    TYPE_BUFF,
    TYPE_DEBUFF
} TypeID;

typedef enum
{
    ATTR_NONE,
    ATTR_PHYSICAL,
    ATTR_FIRE,
    ATTR_WIND,
    ATTR_THUNDER,
    ATTR_EARTH,
    ATTR_LIGHT,
    ATTR_DARK
} AttributeID;

typedef enum
{
    Gain_qi = 0,
    Melee,
    Defend,
    Heal,
    Boost,
    Parry,
    Smite,
    Ranged,
    Burst,
    Terminate,
    TOTAL_ACTION_TYPES,
    None = -1
} ActionID;

typedef enum
{
    ROOT_Mortal,
    ROOT_Heavenly,
    ROOT_Solid,
    ROOT_Sharp,
    ROOT_Ethereal,
    TOTAL_ROOT_TYPES
} SpiritualRootID;

typedef enum
{
    FATE_None,
    FATE_Qi_Infusion,
    FATE_Vitality_Blessing,
    FATE_Enlightenment,
    FATE_Agile_Wind,
    FATE_Calamity,
    TOTAL_FATE_TYPES
} FateID;

// 2. 接着定义所有核心数据结构体 (Structs)
typedef struct Skill_s
{
    ActionID skill_id;
    const char *name_chn;
    const char *name_eng;
    char hotkey;
    int cost;
    int rank;
    TypeID type_id;
    AttributeID attribute_id;
    float base_power;
    int effect_duration;
    float effect_chance;
    TargetType target_type;
} Skill;

typedef struct Player_s
{
    char *name;
    int HP, QI, ATK, YUAN, XIUWEI;
    int current_action_id;
    Skill learned_skills[TOTAL_ACTION_TYPES];
    int gain_combo, burst_count, healing, enraged;
    float evade;
    SpiritualRootID root;
    int damage_received, action_cost;
} Player;

// 为清晰起见，定义一个新的、简单的日志结构
typedef struct
{
    ActionID action_id;
} TurnHistoryLog;

typedef struct Game_s
{
    int round_number;
    char action;
    int opponent_type;
    int current_general_id;
    TurnHistoryLog player_turn_history[STRATEGIC_CYCLE];
    int history_log_count;
} Game;

typedef struct
{
    int initial_hp, initial_qi, initial_xiuwei;
    float initial_evade;
    int train_reps, enemy_type;
    int enable_ai_randomness;
} GameConfig;

// AI相关的数据结构也属于核心数据，应在此处定义
typedef struct
{
    ActionID action_id;
    float score;
} ActionScore;

typedef struct
{
    float w_health_urgency, w_damage_per_point, w_kill_shot_bonus, w_interrupt_heal_bonus,
        w_low_qi_gather, w_defend_vs_high_qi, w_qi_advantage, w_damage_per_qi,
        w_low_hp_penalty, w_breakthrough_urgency, w_self_buff_value;
} AI_Weights;

typedef struct
{
    int round_number;
    ActionID chosen_action, opponent_action;
    int ai_hp, opponent_hp, ai_qi, opponent_qi, ai_xiuwei;
    int action_cost, damage_dealt, damage_taken;
} AI_TurnLog;

#pragma endregion Core_Data_Structures

#pragma region Global_Variable_Declarations
// 声明所有全局变量
extern Player YOU;
extern Player CPU;
extern Game game;
extern GameConfig g_config;
extern char *Realm[10];
extern int max_HP[10], max_QI[10], Yuan[10];
extern const char *Eng_Root_Names[TOTAL_ROOT_TYPES];
extern const char *CHN_Root_Names[TOTAL_ROOT_TYPES];
#pragma endregion Global_Variable_Declarations

#pragma region Function_Prototypes
// 3. 最后声明所有函数原型
// Tool Functions
void clear_buffer();

// Data Manager
void Initialize_Databases();
void Load_Config();

// Game Flow Functions
void Game_init(Player *YOU, Player *CPU, Game *game);
int Start_new_round(Game *game);
void Player_action(Game game, Player *YOU);
void CPU_action(Player *CPU);
void Action_resolve(Player *YOU, Player *CPU);
void Status_settlement(Player *player);
void Game_summary(Player *YOU, Player *CPU);
int Trigger_Fate(Player *player);

// Combat Resolution Engine
void Oneway_Solution(Player *attacker, Player *defender);
int InterruptHealing(const Player *attacker, Player *target);

// AI Decision System
int get_affordable_actions(const Player *player, ActionID affordable_actions[]);
float EvaluateAction(ActionID action_id, const Player *cpu, const Player *opponent, const AI_Weights *weights);
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
void Build_Genesis_Prompt();
void Build_Turn_Update_Prompt(const Player *cpu, const Player *opponent);
void CPU_logic_LLM(Player *cpu, const Player *opponent);

// 模式2: Grand Marshal (将帅分级)
void Build_Marshal_Genesis_Prompt();
void Build_Strategic_Report_Prompt(const Player *cpu, const Player *opponent, const Game *game);
void Request_Strategic_Decision(Player *cpu, Player *opponent, Game *game);

#pragma endregion Function_Prototypes

#endif // QI_H