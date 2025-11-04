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
#define GET_CHAR() getchar()
#else
#include <termios.h>
#include <unistd.h>
#define CLEAR_SCREEN() system("clear")
#define NULL_DEVICE "/dev/null"
#define GET_CHAR() getchar()
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
// #define AI_TRAINING_SET

#define INTERACTIVE_AI_MODE // 是否保持输出流（若是要让受训AI与LLM对战则需打开）

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
    TYPE_SLASH,     //劈砍
    TYPE_SMASH,     //重锤
    TYPE_PIERCE,    //穿刺
    TYPE_BURST,     //爆发
    TYPE_BLAST,     //爆破
    TYPE_SHIELD,    //护盾
    TYPE_FORCEFIELD,//力场 
    TYPE_PARRY,     //招架
    TYPE_HEAL,      //疗愈
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
    ATTR_DARK,
    ATTR_SPIRITUAL
} AttributeID;


// --- BLUEPRINT REFACTOR: Clarify Naming ---
// ActionType: 技能的“宏观战斗类别”。决定了它在逻辑上属于哪一“族”的行为。
// 玩家的技能槽位、AI的决策目标，都基于这个类型。
typedef enum {
    ACTION_TYPE_GAIN_QI,
    ACTION_TYPE_MELEE,
    ACTION_TYPE_DEFEND,
    ACTION_TYPE_HEAL,
    ACTION_TYPE_BOOST,
    ACTION_TYPE_PARRY,
    ACTION_TYPE_SMITE,
    ACTION_TYPE_RANGED,
    ACTION_TYPE_BURST,
    ACTION_TYPE_TERMINATE,
    TOTAL_ACTION_TYPES, // 宏观类别的总数
    ACTION_TYPE_NONE = -1
} ActionType;

// SkillID: 技能数据在数据库中的“唯一标识符”。
// 每个新技能（包括升级版）都应该有一个独一无二的SkillID。
typedef enum {
    SKILL_ID_NONE = -1,
    // 凡人
    SKILL_ID_GAIN_QI = 0,
    SKILL_ID_MELEE,
    SKILL_ID_DEFEND,
    SKILL_ID_HEAL,
    SKILL_ID_BOOST_WARCRY,
    SKILL_ID_PARRY,
    SKILL_ID_SMITE_BASIC,
    // 炼气
    SKILL_ID_RANGED_FIREBALL,
    SKILL_ID_ENERGY_SHIELD,
    SKILL_ID_BURST_WINDBLADE,
    // 筑基
    SKILL_ID_RANGED_FLAMEBLAST,
    SKILL_ID_GOLD_LIGHT_WARDING,
    SKILL_ID_SMITE_GREATSWORD,
    SKILL_ID_COMMANDING_SWORDS,
    SKILL_ID_TERMINATE_THUNDER,
    // ... 未来所有新技能都在此添加唯一ID
    TOTAL_SKILLS // 技能数据总数
} SkillID;

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
typedef struct Skill_s {
    SkillID       skill_id;      // 技能的唯一ID
    ActionType    action_type;   // 技能的宏观类别
    const char*   name_chn;
    const char*   name_eng;
    char          hotkey;
    int           cost;
    int           rank;          // 技能阶级
    TypeID        type_id;       // 微观物理/效果分类
    AttributeID   attribute_id;  // 元素属性
    float         base_power;
    int effect_duration;
    float effect_chance;
    TargetType target_type;
} Skill;

typedef struct Player_s
{
    char *name;
    int HP, QI, ATK, YUAN, XIUWEI;
    ActionType current_action_type; // 玩家本回合的“意图”是哪个宏观类别
    Skill      learned_skills[TOTAL_ACTION_TYPES]; // 玩家每个宏观类别下学会的最高阶技能
    int gain_combo, burst_count, healing, enraged;
    float evade;
    SpiritualRootID root;
    int damage_received, action_cost;
} Player;

// 为清晰起见，定义一个新的、简单的日志结构
typedef struct
{
    ActionType action_type;
} TurnHistoryLog;

typedef struct Game_s
{
    int round_number;
    char action;
    int opponent_type;
    int current_general_id;
    TurnHistoryLog player_turn_history[STRATEGIC_CYCLE];
    int history_log_count;
    int is_bridge_mode; // 1 表示桥接模式，0 表示独立模式
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
    ActionType action_type;
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
    ActionType chosen_action, opponent_action;
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

#pragma endregion Function_Prototypes

#endif // QI_H