#include "QI.h"
// 2025.10.31

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
    .enemy_type = -1};

char *Realm[10] = {"凡人", "炼气", "筑基", "结丹", "元婴", "化神", "炼虚", "合体", "大乘", "飞升"};
char *Eng_Realm[10] = {"Mortal", "Qi Refining", "Foundation", "Core Formation", "Nascent Soul", "Spirit Severing", "Void Refinement", "Unity", "Great Ascension", "Ascension"};
int max_HP[10] = {10, 20, 50, 100, 200, 1000, 5000, 10000, 50000, 100000};
int max_QI[10] = {10, 20, 50, 100, 200, 1000, 5000, 10000, 50000, 100000};
int Yuan[10] = {1, 2, 5, 10, 20, 100, 500, 1000, 5000, 10000};

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

// --- Blueprint: Data Manager ---
// 静态技能数据库，定义了游戏中的所有技能规则
Skill g_skill_database[TOTAL_ACTION_TYPES];
#pragma endregion Global Variable Definitions

// 一个用于初始化所有数据库的函数
void Initialize_Databases()
{
    // --- 凡人境 (Mortal Realm) 技能 ---
    g_skill_database[Gain_qi] = (Skill){
        .skill_id = Gain_qi, .name_chn = "集气", .name_eng = "Gain QI", .hotkey = 'Q', .cost = 0, .rank = 0, .type_id = TYPE_BUFF, .attribute_id = ATTR_NONE, .base_power = 1.0f, .target_type = TARGET_SELF};
    g_skill_database[Melee] = (Skill){
        .skill_id = Melee, .name_chn = "轻击", .name_eng = "Melee", .hotkey = 'A', .cost = 1, .rank = 0, .type_id = TYPE_SLASH, .attribute_id = ATTR_PHYSICAL, .base_power = 1.0f, .target_type = TARGET_ENEMY};
    g_skill_database[Defend] = (Skill){
        .skill_id = Defend, .name_chn = "防御", .name_eng = "Defend", .hotkey = 'D', .cost = 1, .rank = 0, .type_id = TYPE_SHIELD, .attribute_id = ATTR_NONE, .base_power = 1.0f, .target_type = TARGET_SELF};
    g_skill_database[Heal] = (Skill){
        .skill_id = Heal, .name_chn = "养元", .name_eng = "Heal", .hotkey = 'H', .cost = 1, .rank = 0, .type_id = TYPE_HEAL, .attribute_id = ATTR_LIGHT, .base_power = 1.0f, .target_type = TARGET_SELF};
    g_skill_database[Boost] = (Skill){
        .skill_id = Boost, .name_chn = "战吼", .name_eng = "Warcry", .hotkey = 'C', .cost = 2, .rank = 0, .type_id = TYPE_DEBUFF, .attribute_id = ATTR_NONE, .base_power = 1.0f, .target_type = TARGET_ENEMY};
    g_skill_database[Parry] = (Skill){
        .skill_id = Parry, .name_chn = "格挡", .name_eng = "Parry", .hotkey = 'P', .cost = 2, .rank = 0, .type_id = TYPE_PARRY, .attribute_id = ATTR_PHYSICAL, .base_power = 0.2f, .target_type = TARGET_SELF};
    g_skill_database[Smite] = (Skill){
        .skill_id = Smite, .name_chn = "重击", .name_eng = "Smite", .hotkey = 'S', .cost = 3, .rank = 0, .type_id = TYPE_SMASH, .attribute_id = ATTR_PHYSICAL, .base_power = 4.0f, .target_type = TARGET_ENEMY};

    // --- 炼气境 (Qi Refining) 及以上技能 ---
    g_skill_database[Ranged] = (Skill){
        .skill_id = Ranged, .name_chn = "火球", .name_eng = "Fireball", .hotkey = 'F', .cost = 2, .rank = 1, .type_id = TYPE_PIERCE, .attribute_id = ATTR_FIRE, .base_power = 1.0f, .target_type = TARGET_ENEMY};
    g_skill_database[Burst] = (Skill){
        .skill_id = Burst, .name_chn = "风刃", .name_eng = "Wind Blade", .hotkey = 'B', .cost = 3, .rank = 1, .type_id = TYPE_BURST, .attribute_id = ATTR_WIND, .base_power = 1.0f, .target_type = TARGET_ENEMY};
    g_skill_database[Terminate] = (Skill){
        .skill_id = Terminate, .name_chn = "唤雷", .name_eng = "Thunderbolt", .hotkey = 'T', .cost = 6, .rank = 2, .type_id = TYPE_SMASH, .attribute_id = ATTR_THUNDER, .base_power = 5.0f, .target_type = TARGET_ENEMY};
}

// This helper function reduces code duplication for interrupting healing.
int InterruptHealing(const Player *attacker, Player *target)
{
    if (target->healing > 0)
    {
        printf("[%s's healing was interrupted by %s's attack!]\n", target->name, attacker->name);
        target->healing = 0;
        return 1;
    }
    return 0;
}

// --- BLUEPRINT REFACTOR: The New Combat Resolution Engine ---
void Oneway_Solution(Player *attacker, Player *defender)
{
    // --- 步骤 1: 获取双方的技能实例 ---
    // 如果任何一方没有行动，则直接结束
    if (attacker->current_action_id == None || defender->current_action_id == None)
    {
        return;
    }
    const Skill *attacker_skill = &g_skill_database[attacker->current_action_id];

    // --- 步骤 2: 处理非交互性技能 ---
    // 如果攻击方的技能目标是自己 (如治疗、格挡架势)，则它不与防御方发生交互
    if (attacker_skill->target_type == TARGET_SELF)
    {
        return;
    }

    // --- 步骤 3: 前置检定 (闪避) ---
    // 只有特定类型的攻击可以被闪避
    if (attacker_skill->type_id == TYPE_PIERCE || attacker_skill->type_id == TYPE_BURST || attacker_skill->type_id == TYPE_SMASH)
    {
        if ((rand() % 100) < (defender->evade * 100.0f))
        {
            ENG_PRINT("\033[36m[%s's %s was gracefully evaded by %s!]\033[0m\n", attacker->name, attacker_skill->name_eng, defender->name);
            CHN_PRINT("\033[36m[%s的%s被%s灵巧地闪避了！]\033[0m\n", attacker->name, attacker_skill->name_chn, defender->name);
            return; // 闪避成功，交互结束
        }
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
    const Skill *defender_skill = &g_skill_database[defender->current_action_id];

    // **蓝图核心：基于防御方技能的 TypeID 进入不同的处理模板**
    switch (defender_skill->type_id)
    {
    case TYPE_SHIELD: // 如果防御方在用“护盾”类技能...
        CHN_PRINT("[%s 使用 %s 抵挡 %s 的 %s!]\n", defender->name, defender_skill->name_chn, attacker->name, attacker_skill->name_chn);
        ENG_PRINT("[%s uses %s to block %s's %s!]\n", defender->name, defender_skill->name_eng, attacker->name, attacker_skill->name_eng);

        // **...然后基于攻击方技能的 TypeID 进行克制判断**
        switch (attacker_skill->type_id)
        {
        case TYPE_SMASH:                        // 重击克制护盾
            final_damage = base_damage * 0.75f; // 依然能造成 75% 伤害
            break;
        case TYPE_PIERCE: // 穿刺对护盾效果很差
            final_damage = base_damage * 0.25f;
            break;
        case TYPE_BURST: // 爆发攻击会被大幅削弱
            final_damage = base_damage * 0.3f;
            break;
        default: // 其他类型的攻击被普遍削弱
            final_damage = base_damage * 0.5f;
            break;
        }
        break;

    case TYPE_PARRY: // 如果防御方在用“格挡/弹反”类技能...
        CHN_PRINT("[%s 试图 %s %s 的 %s!]\n", defender->name, defender_skill->name_chn, attacker->name, attacker_skill->name_chn);
        ENG_PRINT("[%s attempts to %s %s's %s!]\n", defender->name, defender_skill->name_eng, attacker->name, attacker_skill->name_eng);

        switch (attacker_skill->type_id)
        {
        case TYPE_SLASH: // 轻度的斩击会被完全弹反
            reflect_damage = base_damage * 1.0f;
            break;
        case TYPE_SMASH: // 沉重的重击无法被弹反，反而会破防
            final_damage = base_damage * 0.8f;
            break;
        default: // 其他攻击被部分格挡
            final_damage = base_damage * 0.6f;
            break;
        }
        break;

    // --- 默认情况 ---
    // 如果防御方的技能不是防御性的 (例如，他也在攻击或集气)
    default:
        final_damage = base_damage;
        CHN_PRINT("[%s 的 %s 击中了正在 %s 的 %s!]\n", attacker->name, attacker_skill->name_chn, defender_skill->name_chn, defender->name);
        ENG_PRINT("[%s's %s hits %s who is using %s!]\n", attacker->name, attacker_skill->name_eng, defender_skill->name_chn, defender->name);
        break;
    }

    // --- 步骤 5: 应用灵根和特殊效果修正 ---
    // (这是一个扩展点，暂时保持简单)
    if (attacker->root == ROOT_Sharp && (attacker_skill->attribute_id == ATTR_PHYSICAL || attacker_skill->attribute_id == ATTR_WIND))
    {
        final_damage *= 1.2f; // 锐金灵根使用物理/风系技能伤害增加
    }
    if (defender->root == ROOT_Solid && defender_skill->type_id == TYPE_SHIELD)
    {
        final_damage *= 0.8f; // 厚土灵根使用护盾技能时，减伤效果更强
    }

    // --- 步骤 6: 应用最终伤害 ---
    if (final_damage > 0)
    {
        defender->damage_received += final_damage;
        InterruptHealing(attacker, defender); // 造成伤害即可打断治疗
    }
    if (reflect_damage > 0)
    {
        attacker->damage_received += reflect_damage;
        CHN_PRINT("[%s 的攻击被反弹，受到了 %d 点伤害!]\n", attacker->name, (int)reflect_damage);
        ENG_PRINT("[%s's attack was reflected, taking %d damage!]\n", attacker->name, (int)reflect_damage);
    }
}

// --- Main Game Loop and Other Functions (with fixes) ---
int main()
{
// SetConsoleOutputCP(GetConsoleOutputCP());
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
    do
    {
#ifndef INTERACTIVE_AI_MODE
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

            // --- 模式7: 将帅分级 AI ---
            if (game.opponent_type == 7)
            {
                if (game.round_number > 0 && game.round_number % STRATEGIC_CYCLE == 0)
                {
                    Request_Strategic_Decision(&CPU, &YOU, &game);
                    game.history_log_count = 0;
                }
            }
            // --- 模式6: 每回合决策 AI ---
            else if (game.opponent_type == 6)
            {
                CPU_logic_LLM(&CPU, &YOU);
            }

            HUMAN_PLAYING(Player_action(game, &YOU));

            AI_TRAINING(
                // 1. 让“YOU”这个角色使用5号AI的逻辑进行决策
                // CPU_logic_V2A 的第一个参数是决策者，第二个是其对手
                CPU_logic_V2A(&YOU, &CPU, 0);

                // 2. 调用通用的行动宣告函数，来打印“YOU”的决策
                CPU_action(&YOU);)

            printf("\033[91m");
            switch (game.opponent_type)
            {
            case 0:
                CPU_logic_V1A(&CPU, &YOU);
                break;
            case 1:
                CPU_logic_V1B(&CPU, &YOU);
                break;
            case 2:
                CPU_logic_V1C(&CPU, &YOU);
                break;
            case 3:
                CPU_logic_V1D(&CPU, &YOU);
                break;
            case 4:
                CPU_logic_V1E(&CPU, &YOU);
                break;
            case 5:
                CPU_logic_V2A(&CPU, &YOU, 0);
                break;
            case 6:
                CPU_logic_LLM(&CPU, &YOU);
                break;
            case 7:
                switch (game.current_general_id)
                {
                case 0:
                    CPU_logic_V1A(&CPU, &YOU);
                    break;
                case 1:
                    CPU_logic_V1B(&CPU, &YOU);
                    break;
                case 2:
                    CPU_logic_V1C(&CPU, &YOU);
                    break;
                case 3:
                    CPU_logic_V1D(&CPU, &YOU);
                    break;
                case 4:
                    CPU_logic_V1E(&CPU, &YOU);
                    break;
                default:
                    CPU_logic_V0(&CPU, &YOU);
                    break;
                }
                break;

            default:
                CPU_logic_V0(&CPU, &YOU);
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
#ifdef _WIN32
        #ifndef INTERACTIVE_AI_MODE
        AI_TRAINING(freopen("CONOUT$", "w", stdout));
        #endif
#else
        #ifndef INTERACTIVE_AI_MODE
        AI_TRAINING(freopen("/dev/tty", "w", stdout));
        #endif
#endif
        // --- END REFACTOR ---

        Game_summary(&YOU, &CPU);

        AI_TRAINING(AI_Learn_From_Game(YOU.HP > 0));

        // AI_TRAINING(Save_AI_Weights());

        HUMAN_PLAYING(CHN_PRINT("\n按任意键继续...\n"));
        HUMAN_PLAYING(ENG_PRINT("\nPress any key to continue...\n"));
        HUMAN_PLAYING(fflush(stdout)); // <-- 关键修正
        HUMAN_PLAYING(GET_CHAR());
    } while (--train_reps); //  (0); //

    end_time = clock();

    cpu_time_used = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("\n========================================\n");
    CHN_PRINT(" AI 训练全部完成!\n");
    CHN_PRINT(" 总耗时: %.2f 秒\n", cpu_time_used);
    ENG_PRINT(" AI training is completed!\n");
    ENG_PRINT(" Time elapsed: %.2f s\n", cpu_time_used);
    printf("========================================\n");

    CHN_PRINT("\n按任意键退出程序...\n");
    ENG_PRINT("\nPress any key to exit...\n");
    GET_CHAR();

    return 0;
}

#pragma region tool_function
#ifndef _WIN32 // 这段代码只在非Windows环境下编译
int getch_linux()
{
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#endif

void clear_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}
// --- BLUEPRINT REFACTOR: New AI Utility Functions ---

// 蓝图核心：一个绝对安全的工具函数，用于获取AI的可行行动列表
// 它直接查询玩家实例，而不是依赖任何全局变量
int get_affordable_actions(const Player *player, ActionID affordable_actions[])
{
    int count = 0;
    for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
    {
        const Skill *skill = &player->learned_skills[i];
        // --- 核心修正: 检查 skill_id 是否不为 None ---
        if (skill->skill_id != None && player->QI >= skill->cost)
        {
            affordable_actions[count] = skill->skill_id;
            count++;
        }
    }
    return count;
}

// 一个更简单的辅助函数，用于检查某个特定行动是否可行
static inline int can_perform_action(const Player *player, ActionID action_id)
{
    // --- 核心修正: 检查 skill_id 是否不为 None ---
    if (player->learned_skills[action_id].skill_id != None && player->QI >= player->learned_skills[action_id].cost)
    {
        return 1;
    }
    return 0;
}
#pragma endregion tool_function

// --- BLUEPRINT REFACTOR: Player Initialization Module ---
// 一个内聚的、可重用的函数，负责初始化一个玩家的所有状态
static void Initialize_Player(Player *player, const char *name_eng, const char *name_chn)
{
    // 1. 设置基础信息
    ENG(player->name = (char *)name_eng);
    CHN(player->name = (char *)name_chn);

    // 2. 从全局配置或默认值加载初始属性 (已移除冗余赋值)
    player->XIUWEI = g_config.initial_xiuwei;
    player->QI = g_config.initial_qi;
    player->evade = g_config.initial_evade > 0 ? g_config.initial_evade : 0.05f;

    // 3. 根据境界，从数据表中派生属性
    player->HP = max_HP[player->XIUWEI];
    player->ATK = Yuan[player->XIUWEI];
    player->YUAN = Yuan[player->XIUWEI];
    player->gain_combo = player->XIUWEI + 1;

    // 4. 重置所有动态战斗状态
    player->current_action_id = None;
    player->burst_count = 0;
    player->healing = 0;
    player->enraged = 0;
    player->damage_received = 0;
    player->action_cost = 0;

    // 5. 随机化灵根
    player->root = (rand() % (TOTAL_ROOT_TYPES - 1)) + 1;

    // 6. 初始化技能槽
    // 首先，将所有技能槽明确设置为空 (None = -1)
    for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
    {
        player->learned_skills[i].skill_id = None;
    }
    // 然后，只填充玩家当前境界可用的技能
    for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
    {
        if (g_skill_database[i].rank <= player->XIUWEI)
        {
            player->learned_skills[i] = g_skill_database[i];
        }
    }
}
// --- END REFACTOR ---

void Game_init(Player *YOU, Player *CPU, Game *game)
{
    game->round_number = 0;
    game->current_general_id = 1; // 默认以“狂战士”开局
    game->history_log_count = 0;

    ENG_PRINT("\n\033[32mWelcome to the QI Game!\033[0m\n");
    CHN_PRINT("\n\033[32m欢迎来到气之游戏！\033[0m\n");
    fflush(stdout); // <-- 关键修正: 强制发送欢迎信息

    // --- BLUEPRINT REFACTOR: Simplified High-Level Coordinator ---
    // 1. 调用模块化函数，分别初始化YOU和CPU
    Initialize_Player(YOU, "You", "你");
    Initialize_Player(CPU, "CPU", "CPU"); // 临时名字，稍后会被覆盖

    // 2. 根据游戏模式确定并设置CPU的具体“人格”
    // --- BLUEPRINT REFACTOR: Unified Opponent Configuration ---
    // 1. 修正边界检查，使其包含LLM对手类型(6)
    if (g_config.enemy_type >= 0 && g_config.enemy_type <= 7)
    {
        game->opponent_type = g_config.enemy_type;
    }
    else
    {
        game->opponent_type = rand() % 5; // 如果配置无效，则随机选择一个普通对手
    }

    // 2. 在初始化时就正确设置LLM对手的名字
    switch (game->opponent_type)
    {
    case 0:
        CHN(CPU->name = "生存主义者");
        ENG(CPU->name = "Survivor");
        break;
    case 1:
        CHN(CPU->name = "狂战士");
        ENG(CPU->name = "Berserker");
        break;
    case 2:
        CHN(CPU->name = "神龟流");
        ENG(CPU->name = "Turtle");
        break;
    case 3:
        CHN(CPU->name = "苦修者");
        ENG(CPU->name = "Ascetic");
        break;
    case 4:
        CHN(CPU->name = "快攻手");
        ENG(CPU->name = "Quick Hand");
        break;
    case 5:
        CHN(CPU->name = "狂才");
        ENG(CPU->name = "Crazy Genius");
        break;
    case 6:
        CHN(CPU->name = "悟道者");
        ENG(CPU->name = "Enlightened One");
        // 模式6: 发送 NEW_GAME_START 和 创世提示词
        printf("##CMD##:NEW_GAME_START\n");
        fflush(stdout);
        Build_Genesis_Prompt();
        break;
    case 7:
        CHN(CPU->name = "大元帅");
        ENG(CPU->name = "Grand Marshal");
        // 模式7: 只发送 NEW_GAME_START，等待战略周期
        printf("##CMD##:NEW_GAME_START\n");
        fflush(stdout);
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

// --- BLUEPRINT REFACTOR: Modular Status Resolution ---

// 模块 1: 处理行动消耗与状态重置
static void Resolve_Action_Costs_And_Resets(Player *player)
{
    player->QI -= player->action_cost;
    player->action_cost = 0;

    // 集气连击逻辑
    if (player->current_action_id == Gain_qi)
    {
        if (player->gain_combo < 1 << player->XIUWEI)
        {
            player->gain_combo++;
        }
    }
    else
    {
        player->gain_combo = player->XIUWEI + 1;
    }

    player->current_action_id = None;
}

// 模块 2: 处理持续性效果 (如治疗、激怒、闪避衰减)
static void Resolve_Persistent_Effects(Player *player)
{
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
        player->healing = 0; // 治疗是一次性效果
    }

    // 激怒效果
    if (player->enraged > 0)
    {
        player->enraged--;
    }
    player->ATK = Yuan[player->XIUWEI] + player->enraged;

    // 闪避率衰减
    float base_evade = (player->root == ROOT_Ethereal) ? 0.1f * player->XIUWEI : 0.02f * player->XIUWEI;
    if (player->evade > base_evade)
    {
        player->evade -= 0.5f * (player->evade - base_evade);
    }
}

// 模块 3: 处理伤害结算
static void Resolve_Damage(Player *player)
{
    if (player->damage_received > 0)
    {
        ENG_PRINT("[%s took \033[35m%d\033[33m damage!]\n", player->name, player->damage_received);
        CHN_PRINT("[%s 受到 \033[35m%d\033[33m 点伤害!]\n", player->name, player->damage_received);
        player->HP -= player->damage_received;
        player->damage_received = 0;
    }
}

// --- BLUEPRINT REFACTOR: Dedicated Breakthrough Module ---
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

    // 4. 应用灵根的突破奖励
    if (player->root == ROOT_Solid)
    {
        player->HP *= 1.2f;
    }
    if (player->root == ROOT_Ethereal)
    {
        player->evade = 0.1f * player->XIUWEI;
    }
    else
    {
        player->evade = 0.02f * player->XIUWEI;
    }

    // 5. 重新授予技能！这是至关重要的一步
    for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
    {
        if (g_skill_database[i].rank <= player->XIUWEI)
        {
            player->learned_skills[i] = g_skill_database[i];
        }
    }
}
// --- END REFACTOR ---

// 模块 4: 处理突破判定
static void Resolve_Breakthrough(Player *player)
{
    if (player->XIUWEI < REALM_COUNT - 1 && player->QI >= max_QI[player->XIUWEI])
    {
        float breakthrough_chance = (player->root == ROOT_Heavenly) ? 90.0f : 90.0f * exp(-player->XIUWEI / 2.0f);

        if ((rand() % 100) < breakthrough_chance)
        {
            // --- 核心修正: 正确的流程 ---
            // 1. 先提升境界等级
            player->XIUWEI++;

            // 2. 再调用专用的奖励函数
            Apply_Breakthrough_Rewards(player);
            // --- END ---

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
}

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
    Resolve_Action_Costs_And_Resets(player);

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
    // --- END REFACTOR ---
}

int Start_new_round(Game *game)
{
    game->round_number++;

    if (game->round_number > 1 && game->round_number % 5 == 0 && (rand() % 100) < 30)
    {
        // --- 【核心修改】 ---
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

    // --- BLUEPRINT REFACTOR: Correct Logging Perspective ---
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
    // --- END REFACTOR ---

    return 0;
}

// --- BLUEPRINT REFACTOR: Hotkey-Driven Player Input ---
void Player_action(Game game, Player *YOU)
{
    YOU->current_action_id = None;

    while (YOU->current_action_id == None)
    {
        printf("\033[0m");
        CHN_PRINT("请选择你的行动：\n");
        ENG_PRINT("What's your next action?\n");

        // 1. 动态生成并显示可用行动列表
        for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
        {
            const Skill *skill = &YOU->learned_skills[i];
            // --- 核心修正: 检查 skill_id 是否不为 None ---
            if (skill->skill_id != None && YOU->QI >= skill->cost)
            {
                CHN_PRINT("[%c]: %s (%d)  ", skill->hotkey, skill->name_chn, skill->cost);
                ENG_PRINT("[%c]: %s (%d)  ", skill->hotkey, skill->name_eng, skill->cost);
            }
        }
        printf("\n");
        // --- BLUEPRINT REFACTOR: Precise Input Signaling ---
        // 1. 发送一个明确的、机器可读的信号
        printf("##CMD##:INPUT_REQUIRED\n");
        // 2. 立即刷新，确保信号被Python立即收到
        fflush(stdout);
        // --- END REFACTOR ---

        // --- BLUEPRINT REFACTOR: Robust IPC Input ---
        char buffer[16];   // 一个小缓冲区来接收输入行
        char choice = ' '; // 默认值

        // 使用fgets从标准输入读取一行
        if (fgets(buffer, sizeof(buffer), stdin) != NULL)
        {
            // 安全地取出第一个非空白字符作为选择
            sscanf(buffer, " %c", &choice);
        }
        // --- END REFACTOR ---

        // clear_buffer(); // fgets已经消费了换行符，不再需要这个
        choice = toupper(choice);

        // 3. 验证输入并设置行动 (通过匹配hotkey)
        int action_found = 0;
        for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
        {
            const Skill *skill = &YOU->learned_skills[i];
            // 检查是否学习、QI足够，并且快捷键匹配
            if (skill->skill_id >= 0 && YOU->QI >= skill->cost && choice == skill->hotkey)
            {
                YOU->current_action_id = skill->skill_id;
                YOU->action_cost = skill->cost;

                printf("\033[34m"); // 蓝色
                CHN_PRINT("<你选择了 %s!>\n", skill->name_chn);
                ENG_PRINT("<You chose %s!>\n", skill->name_eng);

                // 特殊逻辑处理
                if (skill->skill_id == Gain_qi)
                {
                    YOU->QI += YOU->gain_combo;
                    CHN_PRINT("你集气成功，获得%d点气力！你的气力已变为%d。\n", YOU->gain_combo, YOU->QI);
                    ENG_PRINT("You gained %d QI! Your QI is now %d.\n", YOU->gain_combo, YOU->QI);
                }
                if (skill->skill_id == Burst)
                {
                    int burst_cost_per_hit = skill->cost;
                    YOU->burst_count = YOU->QI / burst_cost_per_hit;
                    YOU->action_cost = YOU->burst_count * burst_cost_per_hit;
                    CHN_PRINT("<你已形成%d把%s!>\n", YOU->burst_count, skill->name_chn);
                    ENG_PRINT("<You formed %d %s(s)!>\n", YOU->burst_count, skill->name_eng);
                }

                action_found = 1;
                break; // 找到后跳出循环
            }
        }

        if (!action_found)
        {
            CHN_PRINT("无效的选择，请重新输入。\n");
            ENG_PRINT("Invalid choice, please try again.\n");
        }
    }
}

// --- BLUEPRINT REFACTOR: Simplified, Presentation-Only AI Action ---
void CPU_action(Player *player)
{
    // 防火墙：如果AI没有做出任何决定，直接返回
    if (player->current_action_id == None)
        return;

    // 从数据库中获取AI选择的技能的静态数据
    const Skill *chosen_skill = &g_skill_database[player->current_action_id];

    // 设置通用数据
    player->action_cost = chosen_skill->cost;

    printf("\033[91m"); // AI行动统一用红色

    // 打印AI的选择，完全由数据驱动
    CHN_PRINT("<%s 选择了 %s!>\n", player->name, chosen_skill->name_chn);
    ENG_PRINT("<%s chose to use %s!>\n", player->name, chosen_skill->name_eng);

    // --- 特殊逻辑前置处理 ---
    // 和Player_action类似，处理一些即时或需要预计算的效果
    switch (player->current_action_id)
    {
    case Gain_qi:
        player->QI += player->gain_combo;
        CHN_PRINT("%s 集气了 %d 点气! 它的气力现在为 %d.\n", player->name, player->gain_combo, player->QI);
        ENG_PRINT("%s gained %d QI! Its QI is now %d.\n", player->name, player->gain_combo, player->QI);
        break;
    case Burst:
    {
        int burst_cost_per_hit = chosen_skill->cost;
        player->burst_count = player->QI / burst_cost_per_hit;
        player->action_cost = player->burst_count * burst_cost_per_hit;
        CHN_PRINT("<%s 准备了 %d 次 %s!>\n", player->name, player->burst_count, chosen_skill->name_chn);
        ENG_PRINT("<%s prepared %d %s(s)!>\n", player->name, player->burst_count, chosen_skill->name_eng);
    }
    break;
    case Heal:
        player->healing = (player->XIUWEI + 1); // 治疗效果依然和境界有关，可以后续数据化
        break;
    // 其他技能没有需要在这里预处理的逻辑
    default:
        break;
    }

    printf("\033[0m");
}

void Action_resolve(Player *YOU, Player *CPU) // 互动解算
{
    printf("\033[33m"); // Set color to yellow for action resolution

    // 在战斗结算前，双方的 current_action_id 已经确定
    // 因此我们可以安全地在这里记录双方的意图

    Oneway_Solution(YOU, CPU);
    Oneway_Solution(CPU, YOU);

    if (game.history_log_count < STRATEGIC_CYCLE)
    {
        game.player_turn_history[game.history_log_count].action_id = YOU->current_action_id;
        game.history_log_count++;
    }

    // --- BLUEPRINT REFACTOR: Correct Logging Perspective ---
    if (g_log_count < MAX_LOG_TURNS)
    {
        AI_TurnLog *log = &g_game_log[g_log_count];

        // 记录行动 (从 YOU 的视角)
        log->chosen_action = YOU->current_action_id;
        log->opponent_action = CPU->current_action_id;
        log->action_cost = YOU->action_cost;

        // 记录结果 (从 YOU 的视角)
        log->damage_dealt = CPU->damage_received; // YOU 造成的伤害，就是 CPU 受到的伤害
        log->damage_taken = YOU->damage_received; // YOU 受到的伤害

        g_log_count++;
    }
    // --- END REFACTOR ---

    Status_settlement(YOU);
    Status_settlement(CPU);
    printf("\033[0m\n");
}

void Game_summary(Player *YOU, Player *CPU)
{
    // 1. 无论胜负，总游戏场次都加一
    total_games_played++;

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
        CHN_PRINT("\033[35m{游戏结束！你被CPU击败了。}\033[0m\n");
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

int Trigger_Fate(Player *player)
{
    // 随机选择一个机缘事件 (避开 FATE_None)
    FateID fate = (rand() % (TOTAL_FATE_TYPES - 1)) + 1;

    printf("\033[95m"); // 用亮紫色显示机缘信息

    switch (fate)
    {
    case FATE_Qi_Infusion:
    {                                   // 使用花括号创建局部作用域
        int qi_gain = 5 + (rand() % 6); // 获得 5-10 点QI
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
        int dmg = player->YUAN * (1 + (rand() % 3)); // 受到 1-3 点伤害
        player->HP -= dmg;
        CHN_PRINT("[天道无常!] 一道微小的劫雷劈中了 %s, 造成了 %d 点伤害!\n", player->name, dmg);
        ENG_PRINT("[Way of Heaven is Unpredictable!] A minor calamity tribulation strikes %s, dealing %d damage!\n", player->name, dmg);

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
        }
    }

    fclose(file);

    CHN_PRINT("[提示] 已成功加载 config.txt 配置文件。\n");
    ENG_PRINT("[Info] Successfully loaded config.txt.\n");
}

// --- AI Optimization ---
#pragma region AIs
// --- BLUEPRINT REFACTOR: All Rule-Based AIs Adapted ---

// V0: 最简单的AI，从所有可用行动中随机选择
void CPU_logic_V0(Player *cpu, const Player *opponent)
{
    ActionID affordable_actions[TOTAL_ACTION_TYPES];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);

    if (affordable_count > 0)
    {
        int random_index = rand() % affordable_count;
        cpu->current_action_id = affordable_actions[random_index];
    }
    else
    {
        cpu->current_action_id = Gain_qi; // 保底措施
    }
}

// V1A - 生存主义者 (已适配)
void CPU_logic_V1A(Player *cpu, const Player *opponent)
{
    ActionID affordable_actions[TOTAL_ACTION_TYPES];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);
    if (affordable_count <= 0)
    {
        cpu->current_action_id = Gain_qi;
        return;
    }

    // 默认行动设为最稳妥的集气
    cpu->current_action_id = Gain_qi;

    if (cpu->HP < max_HP[cpu->XIUWEI] * 0.3f && can_perform_action(cpu, Heal))
    {
        if ((rand() % 10) < 8)
        {
            cpu->current_action_id = Heal;
            return;
        }
    }
    if (opponent->healing > 0 && can_perform_action(cpu, Melee))
    {
        if ((rand() % 10) < 7)
        {
            cpu->current_action_id = Melee;
            return;
        }
    }
    if (cpu->QI < 3 && can_perform_action(cpu, Gain_qi))
    {
        return; // 执行默认的集气
    }

    cpu->current_action_id = affordable_actions[rand() % affordable_count];
}

// V1B - 狂战士 (已适配)
void CPU_logic_V1B(Player *cpu, const Player *opponent)
{
    ActionID affordable_actions[TOTAL_ACTION_TYPES];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);
    if (affordable_count <= 0)
    {
        cpu->current_action_id = Gain_qi;
        return;
    }

    cpu->current_action_id = Gain_qi; // 默认是集气

    if (can_perform_action(cpu, Smite) && opponent->HP <= 4 * cpu->ATK)
    {
        cpu->current_action_id = Smite;
        return;
    }
    if (can_perform_action(cpu, Melee) && opponent->HP <= 1 * cpu->ATK)
    {
        cpu->current_action_id = Melee;
        return;
    }
    if (cpu->enraged == 0 && can_perform_action(cpu, Boost))
    {
        if ((rand() % 10) < 8)
        {
            cpu->current_action_id = Boost;
            return;
        }
    }
    if (can_perform_action(cpu, Smite))
    {
        if ((rand() % 10) < 7)
        {
            cpu->current_action_id = Smite;
            return;
        }
    }
    if (can_perform_action(cpu, Melee))
    {
        if ((rand() % 10) < 7)
        {
            cpu->current_action_id = Melee;
            return;
        }
    }
}

// V1C - 神龟流 (已适配)
void CPU_logic_V1C(Player *cpu, const Player *opponent)
{
    ActionID affordable_actions[TOTAL_ACTION_TYPES];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);
    if (affordable_count <= 0)
    {
        cpu->current_action_id = Gain_qi;
        return;
    }

    cpu->current_action_id = Gain_qi; // 默认行动是集气

    if (cpu->HP < max_HP[cpu->XIUWEI] * 0.6f && can_perform_action(cpu, Heal))
    {
        if ((rand() % 10) < 9)
        {
            cpu->current_action_id = Heal;
            return;
        }
    }
    // 检查对手的 *意图* (current_action_id) 而非已发生的状态
    if ((opponent->QI > 4 || opponent->enraged > 0) && can_perform_action(cpu, Defend))
    {
        if ((rand() % 10) < 8)
        {
            cpu->current_action_id = Defend;
            return;
        }
    }
}

// V1D - 苦修者 (已适配)
void CPU_logic_V1D(Player *cpu, const Player *opponent)
{
    ActionID affordable_actions[TOTAL_ACTION_TYPES];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);
    if (affordable_count <= 0)
    {
        cpu->current_action_id = Gain_qi;
        return;
    }

    if (cpu->HP < max_HP[cpu->XIUWEI] * 0.25f && can_perform_action(cpu, Heal))
    {
        cpu->current_action_id = Heal;
        return;
    }

    cpu->current_action_id = Gain_qi; // 默认永远是修炼

    if (cpu->XIUWEI > opponent->XIUWEI)
    {
        if ((rand() % 3) != 0)
        { // 2/3概率进攻
            if (can_perform_action(cpu, Smite))
            {
                cpu->current_action_id = Smite;
            }
            else if (can_perform_action(cpu, Melee))
            {
                cpu->current_action_id = Melee;
            }
        }
    }
}

// V1E - 快攻压制 (已适配)
void CPU_logic_V1E(Player *cpu, const Player *opponent)
{
    ActionID affordable_actions[TOTAL_ACTION_TYPES];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);
    if (affordable_count <= 0)
    {
        cpu->current_action_id = Gain_qi;
        return;
    }

    cpu->current_action_id = Gain_qi; // 默认是集气

    if (can_perform_action(cpu, Melee) && opponent->HP <= 1 * cpu->ATK)
    {
        cpu->current_action_id = Melee;
        return;
    }
    // 检查对手的 *意图*
    if (opponent->current_action_id == Gain_qi && can_perform_action(cpu, Boost))
    {
        if ((rand() % 10) < 9)
        {
            cpu->current_action_id = Boost;
            return;
        }
    }
    if (can_perform_action(cpu, Melee))
    {
        if ((rand() % 10) < 9)
        {
            cpu->current_action_id = Melee;
            return;
        }
    }
}

// Evaluateaction - AI的大脑 (V2 - 拥有长远规划和风险意识)
float EvaluateAction(ActionID action_id, const Player *cpu, const Player *opponent, const AI_Weights *weights)
{
    float score = 0.0f;

    // --- BLUEPRINT REFACTOR: Read from database, don't hardcode ---
    const Skill *skill = &g_skill_database[action_id];
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
        if (action_id == Gain_qi)
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
        if (action_id == Melee)
            score -= 30.0f;
        if (action_id == Smite)
            score += 50.0f;
        break;
    case ROOT_Ethereal:
        // 对手是风灵根，闪避太高了。
        // 降低所有远程技能的价值，提升Boost(强化近战)和Melee的价值。
        if (action_id == Ranged || action_id == Burst)
            score -= 50.0f;
        if (action_id == Boost)
            score += 100.0f;
        break;
        // ...
    }

    // --- 步骤 2: 基于新旧指标进行综合评分 ---

    // 1. 【生存】治疗的价值 (旧逻辑)
    if (action_id == Heal)
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
        if (action_id == Ranged || action_id == Burst || action_id == Terminate)
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
    if (action_id == Defend || action_id == Parry)
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
    if (action_id == Gain_qi)
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
        if (cpu->XIUWEI < REALM_COUNT - 1)
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
        if (action_id != Defend && action_id != Parry && action_id != Heal && action_id != Gain_qi)
        {
            score -= weights->w_low_hp_penalty;
        }
    }

    if (action_id == Boost)
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

        if (opponent->current_action_id == Gain_qi && opponent->QI >= 2)
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
void CPU_logic_V2(Player *cpu, const Player *opponent)
{
    ActionID affordable_actions[TOTAL_ACTION_TYPES];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);

    if (affordable_count == 0)
    {
        cpu->current_action_id = Gain_qi; // 保底措施
        return;
    }

    ActionScore best_action = {None, -1.0f};

    for (int i = 0; i < affordable_count; i++)
    {
        ActionID current_action_id = affordable_actions[i];
        float score = EvaluateAction(current_action_id, cpu, opponent, &g_ai_weights);
        score += (rand() % 10);

        if (score > best_action.score)
        {
            best_action.score = score;
            best_action.action_id = current_action_id;
        }
    }
    cpu->current_action_id = best_action.action_id;
}

// 加载另一套权重
void CPU_logic_V2A(Player *cpu, const Player *opponent, int A)
{
    ActionID affordable_actions[TOTAL_ACTION_TYPES];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);

    if (affordable_count == 0)
    {
        cpu->current_action_id = Gain_qi; // 保底措施
        return;
    }

    ActionScore best_action = {None, -1.0f};

    for (int i = 0; i < affordable_count; i++)
    {
        ActionID current_action_id = affordable_actions[i];
        float score = EvaluateAction(current_action_id, cpu, opponent, &g_ai_weights_A[A]);
        score += (rand() % 10);

        if (score > best_action.score)
        {
            best_action.score = score;
            best_action.action_id = current_action_id;
        }
    }
    cpu->current_action_id = best_action.action_id;
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
        if ((log->chosen_action == Defend || log->chosen_action == Parry) && log->opponent_action == Smite && log->damage_taken < 4)
            turn_reward += 50;

        // [负向] 被斩杀
        if (log->ai_hp > 0 && (log->ai_hp - log->damage_taken) <= 0)
            turn_reward -= 100;
        // [负向] 满血治疗
        if (log->chosen_action == Heal && log->ai_hp >= max_HP[log->ai_xiuwei])
            turn_reward -= 20;
        // [负向] 攻击被弹反
        if (log->opponent_action == Parry && log->damage_taken > 0)
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
        case Heal:
            g_ai_weights.w_health_urgency += update_amount;
            break;
        case Melee:
        case Smite:
        case Ranged:
        case Burst:
        case Terminate:
            g_ai_weights.w_damage_per_point += update_amount * 0.7f;
            g_ai_weights.w_damage_per_qi += update_amount * 0.3f;
            break;
        case Defend:
        case Parry:
            g_ai_weights.w_defend_vs_high_qi += update_amount;
            break;
        case Gain_qi:
            g_ai_weights.w_low_qi_gather += update_amount * 0.5f;
            g_ai_weights.w_breakthrough_urgency += update_amount * 0.5f;
            break;
        case Boost:
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
void Build_Genesis_Prompt()
{
    // --- 1. 角色与目标 (Role & Goal) ---
    printf("You are a master strategist in a mystical world of cultivation. Your goal is to win. ");
    printf("You are calm, calculating, and think step-by-step.\n");

    // --- 2. 世界法则 (World Rules) ---
    printf("== Core Rules ==\n");
    printf("- You win if the opponent's HP drops to 0.\n");
    printf("- QI is the resource for most actions. Gaining QI is a fundamental move.\n");
    printf("- Breakthroughs to higher realms (XIUWEI) are critical for long-term victory, which requires accumulating QI to the maximum.\n");

    // --- 3. 沟通契约 (Communication Contract) ---
    printf("== Communication Protocol ==\n");
    printf("From now on, for every turn, you MUST respond in a strict JSON format. ");
    printf("The JSON object must contain two keys: 'action_id' (an integer representing your chosen move) and 'reasoning' (a brief explanation of your strategy).\n");
    printf("Example response:\n{\n  \"action_id\": 0,\n  \"reasoning\": \"My QI is low, so I must gather more to prepare for future attacks.\"\n}\n");

    // --- 4. 结束信号 ---
    printf("END_OF_PROMPT\n");
    fflush(stdout); // 极其重要！确保创世指令被立即发送
}

// 由 Build_LLM_Prompt 重命名而来
void Build_Turn_Update_Prompt(const Player *cpu, const Player *opponent)
{
    // --- 1. 描述当前战局 (Current State) ---
    printf("== Turn Update: Round %d ==\n", game.round_number);
    printf("Your Status: {HP: %d/%d, QI: %d/%d, Realm: %s}\n", cpu->HP, max_HP[cpu->XIUWEI], cpu->QI, max_QI[cpu->XIUWEI], Eng_Realm[cpu->XIUWEI]);
    printf("Opponent Status: {HP: %d/%d, QI: %d/%d, Realm: %s}\n", opponent->HP, max_HP[opponent->XIUWEI], opponent->QI, max_QI[opponent->XIUWEI], Eng_Realm[opponent->XIUWEI]);

    // --- 2. 列出可用技能 (Available Actions) ---
    printf("== Available Actions (Provide ID only) ==\n");
    ActionID affordable_actions[TOTAL_ACTION_TYPES];
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
    // 函数现在唯一的职责就是阻塞并等待Python端喂入LLM的决策结果
    int chosen_id = -1;
    char buffer[16];

    if (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        chosen_id = atoi(buffer);
        ActionID affordable_actions[TOTAL_ACTION_TYPES];
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
            cpu->current_action_id = chosen_id;
        }
        else
        {
            cpu->current_action_id = affordable_actions[rand() % affordable_count];
        }
    }
    else
    {
        cpu->current_action_id = Gain_qi;
    }
}

void Build_Strategic_Report_Prompt(const Player *cpu, const Player *opponent, const Game *game)
{
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
            if (game->player_turn_history[i].action_id >= 0)
            {
                action_counts[game->player_turn_history[i].action_id]++;
            }
        }
        printf("In the last %d turns, opponent actions were: ", game->history_log_count);
        for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
        {
            if (action_counts[i] > 0)
            {
                printf("%s: %d times; ", g_skill_database[i].name_eng, action_counts[i]);
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
    const char *general_names[] = {"Survivor", "Berserker", "Turtle", "Ascetic", "Quick Hand"};
    printf("We are currently executing strategy ID %d (%s).\n", game->current_general_id, general_names[game->current_general_id]);

    // --- 5. 可用将领列表 ---
    printf("== Available Generals for Deployment ==\n");
    printf("[0: Survivor], [1: Berserker], [2: Turtle], [3: Ascetic], [4: Quick Hand]\n");

    // --- 6. 核心问题 ---
    printf("Grand Marshal, should we switch generals? Please respond in JSON format: {\"next_general_id\": <id>, \"reasoning\": \"...\"}\n");

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
        // 安全检查：确保ID在有效范围内
        if (new_general_id >= 0 && new_general_id <= 4)
        { // 假设我们有5个将军
            if (game->current_general_id != new_general_id)
            {
                printf("\033[95m[STRATEGIC SHIFT] Grand Marshal has ordered a change of command! General %d is now in charge!\033[0m\n", new_general_id);
                game->current_general_id = new_general_id;
            }
            else
            {
                printf("\033[95m[STRATEGIC CONFIRMATION] Grand Marshal confirms current strategy is optimal. Proceeding with General %d.\033[0m\n", new_general_id);
            }
        }
    }
    // 如果没有收到有效指令，则维持原状
}

#pragma endregion LLM
