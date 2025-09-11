#include "QI.h"
// 2025.8.16

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

Act_list act_cost[COST_LEVELS] = {
    {1, {Gain_qi}},
    {3, {Melee, Defend, Heal}},
    {3, {Boost, Parry, Ranged}},
    {2, {Smite, Burst}},
    {0, {}},
    {0, {}},
    {1, {Terminate}},
};
Act_list act_able[COST_LEVELS] = {0};
Act_list act_able_xiuwei[REALM_COUNT][COST_LEVELS] = {0};
Act_list act_mortal = {7, {Gain_qi, Melee, Defend, Heal, Boost, Parry, Smite}};
Act_list act_refine = {9, {Gain_qi, Melee, Defend, Heal, Boost, Parry, Smite, Ranged, Burst}};
Act_list act_foundation = {10, {Gain_qi, Melee, Defend, Heal, Boost, Parry, Smite, Ranged, Burst, Terminate}};

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
#pragma endregion Global Variable Definitions

// --- NEW: Interaction Handler Implementations ---

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

// Override Handler for the complex interaction between Smite and Defend.
ResolutionResult Override_SmiteVsDefend(const InteractionRule *rule, const Player *sbj, const Player *obj)
{
    ResolutionResult result = {0};
    switch (sbj->XIUWEI)
    {
    case 0:
    case 1: // smite
        switch (obj->XIUWEI)
        {
        case 0:
            result.message = "[%s's smite was partly blocked by %s's defense!]\n";
            result.damage_to_obj = 4.0f * sbj->ATK / 2.0f; // Use float division
            break;
        case 1:
            result.message = "[%s's smite was mostly blocked by %s's energy shield!]\n";
            result.damage_to_obj = 4.0f * sbj->ATK / 4.0f; // Use float division
            break;
        default:
            result.message = "[%s's smite was fully blocked by %s's Gold light warding!]\n";
            break;
        }
    default: // greatsword mastery
        switch (obj->XIUWEI)
        {
        case 0:
            result.message = "[%s's greatsword destroy %s's defense!]\n";
            result.damage_to_obj = 6.0f * sbj->ATK; // Use float division
            break;
        case 1:
            result.message = "[%s's greatsword break through %s's energy shield!]\n";
            result.damage_to_obj = 6.0f * sbj->ATK / 1.5f; // Use float division
            break;
        default:
            result.message = "[%s's greatsword was partly blocked by %s's Gold light warding!]\n";
            result.damage_to_obj = 6.0f * sbj->ATK / 2.0f;
            break;
        }
    }
    if (obj->root == ROOT_Solid && obj->action == Defend)
    {
        // 如果是厚土灵根在防御，减伤效果额外提升
        result.damage_to_obj *= 0.8f;
    }
    if (sbj->root == ROOT_Sharp && sbj->action == Smite)
    {
        // 如果是锐金灵根在攻击，伤害额外提升
        result.damage_to_obj *= 1.2f;
    }
    return result;
}

// Override Handler for Ranged vs Defend.
ResolutionResult Override_RangedVsDefend(const InteractionRule *rule, const Player *sbj, const Player *obj)
{
    ResolutionResult result = {0};
    switch (sbj->XIUWEI)
    {
    case 1:
        switch (obj->XIUWEI)
        {
        case 0:
            result.message = "[%s's fireball was partly blocked by %s's defense!]\n";
            result.damage_to_obj = 1.0f * sbj->ATK / 2.0f;
            break;
        case 1:
            result.message = "[%s's fireball was fully blocked by %s's energy shield!]\n";
            break;
        default:
            result.message = "[%s's fireball was completely absorbed by %s's Gold light warding!]\n";
            break;
        }
    default:
        switch (obj->XIUWEI)
        {
        case 0:
            result.message = "[%s's firebullet was slightly blocked by %s's defense!]\n";
            result.damage_to_obj = 1.0f * sbj->ATK / 1.25f;
            break;
        case 1:
            result.message = "[%s's firebullet was partly blocked by %s's energy shield!]\n";
            result.damage_to_obj = 1.0f * sbj->ATK / 2.0f;
            break;
        default:
            result.message = "[%s's firebullet was completely absorbed by %s's Gold light warding!]\n";
            break;
        }
        break;
    }
    return result;
}

// Override Handler for Burst vs Defend.
ResolutionResult Override_BurstVsDefend(const InteractionRule *rule, const Player *sbj, const Player *obj)
{
    ResolutionResult result = {0};
    float total_damage = 1.0f * sbj->ATK * sbj->burst_count;
    switch (sbj->XIUWEI)
    {
    case 1:
        switch (obj->XIUWEI)
        {
        case 0:
            result.message = "[%s's wind blade was weaken by %s's defense!]\n";
            result.damage_to_obj = total_damage / 2.0f;
            break;
        case 1:
            if (total_damage <= 6)
            {
                result.message = "[%s's windblades was fully blocked by %s's energy shield!]\n";
                result.damage_to_obj = 0;
            }
            else
            {
                result.message = "[%s's windblades was partly blocked by %s's energy shield!]\n";
                result.damage_to_obj = total_damage - 6;
            }
        default:
            result.message = "[%s's fireball was completely absorbed by %s's Gold light warding!]\n";
            break;
        }
        break;
    default:
        switch (obj->XIUWEI)
        {
        case 0:
            result.message = "[%s's flying sword penetrates through %s's defense!]\n";
            result.damage_to_obj = total_damage / 1.25f;
            break;
        case 1:
            if (total_damage <= 6)
            {
                result.message = "[%s's flying sword was fully blocked by %s's energy shield!]\n";
                result.damage_to_obj = 0;
            }
            else
            {
                result.message = "[%s's flying swords pierced through %s's energy shield!]\n";
                result.damage_to_obj = total_damage - 6;
            }
        default:
            result.message = "[%s's flying sword was partly deflected by %s's Gold light warding!]\n";
            result.damage_to_obj = total_damage / 2.0f;
            break;
        }
    }

    return result;
}

// Override Handler for Burst vs Parry.
ResolutionResult Override_BurstVsParry(const InteractionRule *rule, const Player *sbj, const Player *obj)
{
    ResolutionResult result = {0};
    if (sbj->burst_count <= 2)
    {
        result.message = "[%s's windblade was parried by %s!]\n";
    }
    else
    {
        result.message = "[%s's windblades flooded %s!]\n";
        result.damage_to_obj = 1.0f * sbj->ATK * (sbj->burst_count - 2);
    }
    return result;
}

// Override Handler for Ranged vs Defend.
ResolutionResult Override_TerminateVsDefend(const InteractionRule *rule, const Player *sbj, const Player *obj)
{
    ResolutionResult result = {0};
    switch (sbj->XIUWEI)
    {
    default:
        switch (obj->XIUWEI)
        {
        case 0:
            result.message = "[%s evoke thunderbolt to roast %s!]\n";
            result.damage_to_obj = 5.0f * sbj->ATK;
            break;
        case 1:
            result.message = "[%s's thunder strike blasted %s's energy shield to pieces!]\n";
            result.damage_to_obj = 5.0f * sbj->ATK / 1.5f;
            break;
        default:
            result.message = "[%s's thunder bolt was partly deflected by %s's Gold light warding!]\n";
            result.damage_to_obj = 5.0f * sbj->ATK / 2.0f;
            break;
        }
        break;
    }
    return result;
}

// --- NEW: The Interaction Matrix ---
// This is the heart of the new system. It defines all game rules as data.
const InteractionRule g_interactionMatrix[TOTAL_ACTION_TYPES][TOTAL_ACTION_TYPES] = {
    // === Melee (近战) ===
    [Melee] = {
        // 特殊交互
        [Defend] = {0.0f, 0.0f, CHN("[%s的轻击被%s防住了!]\n") ENG("[%s's attack is defended by %s!]\n"), NULL},

        [Parry] = {0.0f, 1.0f, CHN("[%s的轻击被%s弹反了回来!]\n") ENG("[%s's attack was bounced back by %s!]\n"), NULL},
        // **默认情况**: 定义攻击无防备目标(Gain_qi)时的效果
        [Gain_qi] = {1.0f, 0.0f, CHN("[%s击中了%s!]\n") ENG("[%s hit %s!]\n"), NULL},
    },

    // === Smite (重锤) ===
    [Smite] = {
        // 特殊交互
        [Defend] = {0.0f, 0.0f, NULL, Override_SmiteVsDefend},

        [Parry] = {0.0f, 2.0f, CHN("[%s的重击大部分被%s格挡住了!]\n") ENG("[%s's smite was mostly parried by %s!]\n"), NULL},
        // **默认情况**
        [Gain_qi] = {4.0f, 0.0f, CHN("[%s重击在了%s身上!]\n") ENG("[%s smited %s!]\n"), NULL},
    },

    // === Ranged (远程) ===
    [Ranged] = {
        // 特殊交互
        [Defend] = {0.0f, 0.0f, NULL, Override_RangedVsDefend},

        [Parry] = {0.5f, 0.0f, CHN("[%s的远距离攻击被%s部分格挡住了!]\n") ENG("[%s's ranged attack was partly blocked by %s's parry!]\n"), NULL},
        // **默认情况**
        [Gain_qi] = {1.0f, 0.0f, CHN("[%s的攻击打在了%s身上!]\n") ENG("[%s burned %s with a fireball!]\n"), NULL},
    },

    // === Burst (爆发) ===
    [Burst] = {
        // 特殊交互
        [Defend] = {0.0f, 0.0f, NULL, Override_BurstVsDefend},
        
        [Parry] = {0.0f, 0.0f, NULL, Override_BurstVsParry},
        // **默认情况**
        [Gain_qi] = {1.0f, 0.0f, CHN("[%s的武器爆射在%s身上!]\n") ENG("[%s's burst attack shot on %s!]\n"), NULL},
    },

    // === Terminate (终结) ===
    [Terminate] = {
        // 特殊交互: 防御对终结技无效，所以也当作默认情况处理
        [Defend] = {0.0f, 0.0f, NULL, Override_TerminateVsDefend},
        // **默认情况**
        [Gain_qi] = {5.0f, 0.0f, CHN("[%s的天雷将%s轰杀至渣!]\n") ENG("[%s evoked thunder to strike %s!]\n"), NULL},
    },
    // 注意：像 Boost, Heal, Gain_qi, Defend, Parry 这些非攻击性或纯防御性技能，
    // 它们的行可以是空的，因为它们作为“攻击方”时，不会与其他动作产生伤害性交互。
    // 我们会在 Oneway_Solution 中单独处理它们的自身效果。
};

// --- REWRITTEN Oneway_Solution ---
// This function is now a clean dispatcher. It doesn't know any specific game rules.
void Oneway_Solution(Player *SBJ, Player *OBJ)
{
    // 首先检查这次攻击是否是可被闪避的类型
    if (SBJ->action == Ranged || SBJ->action == Burst || SBJ->action == Terminate)
    {
        // 进行概率检定
        // rand() % 100 会产生一个 0-99 的随机整数
        // 如果这个随机数小于防御方的闪避率*100，则闪避成功
        if ((rand() % 100) < (OBJ->evade * 100.0f))
        {
            // 闪避成功，打印信息并直接结束函数，后续所有伤害计算都将被跳过
            ENG_PRINT("\033[36m[%s's attack was gracefully evaded by %s's movement!]\033[0m\n", SBJ->name, OBJ->name);
            CHN_PRINT("\033[36m[%s的攻击被%s以灵动身法闪避了！]\033[0m\n", SBJ->name, OBJ->name);
            return; // 攻击无效，函数结束
        }
    }

    if (SBJ->action == Boost)
    {
        switch (SBJ->XIUWEI)
        {
        case 0: // warcry
            // ...
            if (OBJ->action != Defend && OBJ->action != Parry)
            {
                // ...
                if (OBJ->action == Gain_qi)
                {
                    // --- 【核心修复】 ---
                    // 在扣减QI之前，先检查。
                    if (OBJ->QI >= OBJ->gain_combo)
                    {
                        // 如果QI足够，则正常扣减
                        OBJ->QI -= OBJ->gain_combo;
                    }
                    else
                    {
                        // 如果QI不够扣，则直接清零，而不是变成负数
                        OBJ->QI = 0;
                    }
                    OBJ->gain_combo = 0;
                }
                InterruptHealing(SBJ, OBJ);
            }
            // ...
            break;
            // ...
        }
        return;
    }

    if (SBJ->action == Parry)
    {
        SBJ->evade += 0.2f; // 增加闪避率
        if (SBJ->evade > 0.8f)
        {
            SBJ->evade = 0.8f; // 确保闪避率不会超过 60%
        }
        return; // 动作已完全处理，结束
    }

    // 步骤 3: 对于交互性动作，查找规则
    // **第一步：精确查找**
    const InteractionRule *rule = &g_interactionMatrix[SBJ->action][OBJ->action];

    // **第二步：默认查找 (如果精确查找失败)**
    // 我们通过检查 message 和 override_handler 是否都为 NULL 来判断是否查找失败
    if (rule->message == NULL && rule->override_handler == NULL)
    {
        // 精确规则不存在，这是一个默认的对攻情况。
        // 我们查找该攻击对 "Gain_qi" 的规则作为默认规则。
        rule = &g_interactionMatrix[SBJ->action][Gain_qi];
    }

    // 步骤 4: 如果最终找到了一个有效的规则，则处理它
    ResolutionResult result = {0};
    if (rule->message || rule->override_handler)
    {
        if (rule->override_handler)
        {
            // 如果是复杂规则，调用它的处理函数
            result = rule->override_handler(rule, SBJ, OBJ);
        }
        else
        {
            // 如果是简单规则，直接使用数据
            result.message = rule->message;
            result.damage_to_obj = rule->damage_multiplier * SBJ->ATK;
            result.damage_to_sbj = rule->reflect_multiplier * SBJ->ATK;

            // 对 Burst 伤害进行特殊计算
            if (SBJ->action == Burst)
            {
                // 如果是默认情况(对攻), 伤害系数应用到 burst_count
                // 如果是特殊情况(如被格挡), 这个逻辑应该在 override_handler 里处理
                result.damage_to_obj *= SBJ->burst_count;
            }
        }

        // 步骤 5: 应用计算结果
        if (result.message)
        {
            printf(result.message, SBJ->name, OBJ->name);
        }
        if (result.damage_to_obj > 0)
        {
            OBJ->damage_received += result.damage_to_obj;
            InterruptHealing(SBJ, OBJ); // 打断治疗
        }
        if (result.damage_to_sbj > 0)
        {
            SBJ->damage_received += result.damage_to_sbj;
        }
    }
    // 如果连默认规则都找不到 (例如 Heal vs Melee)，则什么都不发生，函数正常结束。
}

// --- Main Game Loop and Other Functions (with fixes) ---
int main()
{
    // SetConsoleOutputCP(GetConsoleOutputCP());
    // 激活Windows控制台的虚拟终端处理功能
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

    Load_Config();

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

    Initialize_Action_Lists();

    int train_reps = g_config.train_reps;
    do
    {
        AI_TRAINING(freopen("NUL", "w", stdout));

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

            // 让"YOU"(计分对象)使用V2学习型AI
            AI_TRAINING(CPU_logic_V2(&YOU, &CPU));
            AI_TRAINING(CPU_action(&YOU));
            HUMAN_PLAYING(Player_action(game, &YOU));
            ///*
            printf("\033[91m");
            switch (game.opponent_type)
            {
            case 0:
                CPU_logic_V1A(&CPU, &YOU);
                break; // 生存主义者
            case 1:
                CPU_logic_V1B(&CPU, &YOU);
                break; // 狂战士
            case 2:
                CPU_logic_V1C(&CPU, &YOU);
                break; // 神龟流
            case 3:
                CPU_logic_V1D(&CPU, &YOU);
                break; // 苦修者
            case 4:
                CPU_logic_V1E(&CPU, &YOU);
                break; // 快攻手
            case 5:
                CPU_logic_V2A(&CPU, &YOU, 0);
                break;
            }
            printf("\033[0m");

            CPU_action(&CPU);

            Action_resolve(&YOU, &CPU);
        }
        Game_summary(&YOU, &CPU);

        AI_TRAINING(AI_Learn_From_Game(YOU.HP > 0));

        //AI_TRAINING(Save_AI_Weights());

        HUMAN_PLAYING(CHN_PRINT("\n按任意键继续...\n"));
        HUMAN_PLAYING(ENG_PRINT("\nPress any key to continue...\n"));
        HUMAN_PLAYING(_getch());
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
    _getch();

    return 0;
}

#pragma region tool_function
void act_list_join(Act_list *list, Act_list *source)
{
    for (int i = 0; i < source->length; i++)
    {
        list->list[list->length + i] = source->list[i];
    }
    list->length += source->length;
}

void list_print(int *list, int length)
{
    printf("List length: %d\n", length);
    for (int i = 0; i < length; i++)
    {
        printf("%d ", list[i]);
    }
    printf("\n");
}

void clear_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

int check_list(int *list, int length, int target)
{
    for (int i = 0; i < length; i++)
    {
        if (list[i] == target)
        {
            return 1; // Found
        }
    }
    return 0; // Not found
}

void delete_element(Act_list *source, int element)
{
    int temp;
    if (!check_list(source->list, source->length, element))
    {
        printf("Element %d not found in the list.\n", element);
        return; // Element not found
    }
    for (int i = 0; i < source->length; i++)
    {
        if (source->list[i] == element)
        {
            temp = source->list[i];
            for (int j = i; j < source->length - 1; j++)
            {
                source->list[j] = source->list[j + 1];
            }
            source->length--;
        }
    }
}

void sort_list(int *list, int length)
{
    for (int i = 0; i < length; i++)
    {
        for (int j = i + 1; j < length; j++)
        {
            if (list[i] > list[j])
            {
                int temp = list[i];
                list[i] = list[j];
                list[j] = temp;
            }
        }
    }
}

void delete_to(Act_list list_of_list[], int target, int from, int to)
{
    for (int i = from; i < to; i++)
    {
        // if (check_list(list_of_list[i]->list, list_of_list[i]->length, target))
        {
            delete_element(&list_of_list[i], target);
        }
    }
}

// 一个私有的、内联的辅助函数，用于检查AI是否能执行某个行动
// static 关键字确保它只在当前 .c 文件中可见，避免任何链接问题
static inline int can_perform_action(int *list, int length, ActionID action)
{
    return check_list(list, length, action);
}

// 一个绝对安全的工具函数，用于获取AI的可行行动列表
int get_affordable_actions(Player *cpu, int affordable_actions[])
{
    // 防火墙 1: 严格检查传入的状态值
    if (cpu->XIUWEI < 0 || cpu->XIUWEI >= REALM_COUNT || cpu->QI < 0)
    {
        return 0; // 发现非法状态，立即拒绝服务
    }

    int qi_index = (cpu->QI >= COST_LEVELS) ? COST_LEVELS - 1 : cpu->QI;
    int affordable_count = act_able_xiuwei[cpu->XIUWEI][qi_index].length;

    // 防火墙 2: 检查获取到的数量是否在合理范围内
    if (affordable_count <= 0 || affordable_count > TOTAL_ACTION_AMOUNT)
    {
        return 0; // 数量不合理，同样拒绝
    }

    // 只有在所有检查都通过后，才执行内存复制
    memcpy(affordable_actions, act_able_xiuwei[cpu->XIUWEI][qi_index].list, affordable_count * sizeof(int));

    return affordable_count;
}

void Act_list_print()
{
    for (int i = 0; i < REALM_COUNT; i++)
    {
        for (int j = 0; j < COST_LEVELS; j++)
        {
            list_print(act_able_xiuwei[i][j].list, act_able_xiuwei[i][j].length);
        }
    }
}
#pragma endregion tool_function

// Game_init (最终修正版 - 完整初始化所有状态)
void Game_init(Player *YOU, Player *CPU, Game *game)
{
    game->round_number = 0;
    // srand() 最好在 main 函数开始时只调用一次，但放在这里问题也不大
    ENG_PRINT("\n\033[32mWelcome to the QI Game!\033[0m\n");
    CHN_PRINT("\n\033[32m欢迎来到气之游戏！\033[0m\n");

    // --- 初始化 YOU ---
    CHN(YOU->name = "你");
    ENG(YOU->name = "You");
    YOU->XIUWEI = 0;
    YOU->XIUWEI = g_config.initial_xiuwei;
    YOU->HP = max_HP[YOU->XIUWEI];
    // YOU->HP = g_config.initial_hp;
    YOU->QI = 0;
    YOU->QI = g_config.initial_qi;
    YOU->ATK = Yuan[YOU->XIUWEI];
    YOU->YUAN = Yuan[YOU->XIUWEI];
    YOU->gain_combo = YOU->XIUWEI + 1;
    YOU->action = None;
    YOU->burst_count = 0;
    YOU->healing = 0;
    YOU->enraged = 0;
    YOU->evade = 0.05f;
    YOU->evade = g_config.initial_evade;
    YOU->root = (rand() % (TOTAL_ROOT_TYPES - 1)) + 1;
    YOU->damage_received = 0;
    YOU->action_cost = 0;

    // --- 初始化 CPU ---
    CPU->XIUWEI = 0;
    CPU->XIUWEI = g_config.initial_xiuwei;
    CPU->HP = max_HP[CPU->XIUWEI];
    // CPU->HP = g_config.initial_hp;
    CPU->QI = 0;
    CPU->QI = g_config.initial_qi;
    CPU->ATK = Yuan[CPU->XIUWEI];
    CPU->YUAN = Yuan[CPU->XIUWEI];
    CPU->gain_combo = CPU->XIUWEI + 1;
    CPU->action = None;
    CPU->burst_count = 0;
    CPU->healing = 0;
    CPU->enraged = 0;
    CPU->evade = 0.05f;
    CPU->evade = g_config.initial_evade;
    CPU->root = (rand() % (TOTAL_ROOT_TYPES - 1)) + 1;
    CPU->damage_received = 0;
    CPU->action_cost = 0;

    if (g_config.enemy_type >= 0 && g_config.enemy_type <= 6)
    {
        game->opponent_type = g_config.enemy_type;
    }
    else
    {
        game->opponent_type = rand() % 5;
    }

    switch (game->opponent_type)
    {
    case 0:
        CHN(CPU->name = "生存主义者");
        ENG(CPU->name = "Survivor");
        break; // 生存主义者
    case 1:
        CHN(CPU->name = "狂战士");
        ENG(CPU->name = "Berserker");
        break; // 狂战士
    case 2:
        CHN(CPU->name = "神龟流");
        ENG(CPU->name = "Turtle");
        break; // 神龟流
    case 3:
        CHN(CPU->name = "苦修者");
        ENG(CPU->name = "Ascetic");
        break; // 苦修者
    case 4:
        CHN(CPU->name = "快攻手");
        ENG(CPU->name = "Quick Hand");
        break; // 快攻手
    case 5:
        CHN(CPU->name = "狂才");
        ENG(CPU->name = "Crazy Genius");
        break;
    }

    printf("\033[93m"); // 使用亮黄色显示天赋信息
    CHN_PRINT("[天赋觉醒] %s 乃是 %s, ", YOU->name, CHN_Root_Names[YOU->root]);
    ENG_PRINT("[Talent Awakened] %s possesses the %s, ", YOU->name, Eng_Root_Names[YOU->root]);

    CHN_PRINT("%s 则是 %s!\n", CPU->name, CHN_Root_Names[CPU->root]);
    ENG_PRINT("while %s has the %s!\n", CPU->name, Eng_Root_Names[CPU->root]);
    printf("\033[0m");

    if (YOU->root == ROOT_Solid)
    {
        YOU->HP *= 1.2f; // 厚土灵根初始额外生命值
    }
    if (CPU->root == ROOT_Solid)
    {
        CPU->HP *= 1.2f; // 厚土灵根初始额外生命值
    }
}

void Status_settlement(Player *player)
{
    player->QI -= player->action_cost;
    player->action_cost = 0;

    // Termination
    if (game.round_number >= MAX_ROUNDS)
    {
        player->HP = 0; // Force end the game if max rounds reached
        return;         // Early exit to avoid further processing
    }

    // QI combo gain logic FIRST
    if (player->action == Gain_qi)
    {
        if (player->gain_combo < 1 << player->XIUWEI)
            ++player->gain_combo;
    }
    else
    {
        player->gain_combo = player->XIUWEI + 1;
    }

    // Reset action AFTER all checks depending on it are done.
    player->action = None;

    if (player->healing)
        player->enraged = 0;
    if (player->healing && player->HP < max_HP[player->XIUWEI])
    {
        ENG_PRINT("[%s healed for %d HP!]\n", player->name, player->healing);
        CHN_PRINT("[%s 恢复了 %d 点生命值！]\n", player->name, player->healing);
        player->HP += player->healing;
    }
    if (player->root == ROOT_Solid)
    {
        player->HP += player->healing / 2; // Earthly root heals 50% more
    }
    if (player->root == ROOT_Solid)
    {
        if (player->HP > max_HP[player->XIUWEI] * 1.2f)
            player->HP = max_HP[player->XIUWEI] * 1.2f;
    }
    else
    {
        if (player->HP > max_HP[player->XIUWEI])
            player->HP = max_HP[player->XIUWEI];
    }

    player->ATK = Yuan[player->XIUWEI] + player->enraged--;
    if (player->enraged < 0)
        player->enraged = 0;

    if (player->evade > 1)
        player->evade = 1; // 确保闪避率不超过 100%
    if (player->root == ROOT_Ethereal)
    {
        player->evade -= 0.5f * (player->evade - 0.1f * player->XIUWEI);
    }
    else
    {
        player->evade -= 0.5f * (player->evade - 0.02f * player->XIUWEI);
    }

    if (player->damage_received >= 1)
    {
        ENG_PRINT("[%s took \033[35m%d\033[33m damage!]\n", player->name, player->damage_received);
        CHN_PRINT("[%s 受到 \033[35m%d\033[33m 点伤害!]\n", player->name, player->damage_received);
    }
    player->HP -= player->damage_received;
    player->damage_received = 0;

    if (player->QI >= max_QI[player->XIUWEI])
    {
        float breakthrough_chance = 90.0f * exp(-player->XIUWEI / 2.0f); // 基础50%成功率
        if (player->root == ROOT_Heavenly)
        {
            breakthrough_chance = 90.0f; // 天灵根，90%成功率！
        }
        printf("breakthrough_chance: %f", breakthrough_chance);
        // 突破有概率不成功
        if (rand() % 100 < breakthrough_chance) // 50% 概率成功突破
        {
            player->XIUWEI++;
            player->QI = 0;
            if (player->root == ROOT_Solid)
            {
                player->HP = max_HP[player->XIUWEI] * 1.2f; // 土灵根，生命值增加20%
            }
            else
            {
                player->HP = max_HP[player->XIUWEI];
            }
            player->ATK = Yuan[player->XIUWEI]; // Changed from max_QI / 10
            player->YUAN = Yuan[player->XIUWEI];
            player->gain_combo = player->XIUWEI + 1;
            player->enraged = 0;
            if (player->root == ROOT_Ethereal)
            {
                player->evade = 0.1f * player->XIUWEI;
            }
            else
            {
                player->evade = 0.02f * player->XIUWEI;
            } // 每个境界增加2%的闪避率
            CHN_PRINT("[%s 突破至 %s 期!]\n", player->name, Realm[player->XIUWEI]);
            ENG_PRINT("[%s has broken through to the %s realm!]\n", player->name, Eng_Realm[player->XIUWEI]);
            // Act_list_maintain(player); // Pass the current player who broke through
        }
        else
        {
            CHN_PRINT("[%s 突破失败!]\n", player->name);
            ENG_PRINT("[%s failed to break through!]\n", player->name);
            player->QI = max_QI[player->XIUWEI] * 3 / 4; //
        }
    }
}

void Initialize_Action_Lists(void)
{
    for (int i = 0; i < COST_LEVELS; i++)
    {
        if (i > 0)
        {
            act_list_join(&act_able[i], &act_able[i - 1]);
        }
        act_list_join(&act_able[i], &act_cost[i]);
        sort_list(act_able[i].list, act_able[i].length);
    }

    for (int i = 0; i < REALM_COUNT; i++)
    {
        for (int j = 0; j < COST_LEVELS; j++)
        {
            memcpy(act_able_xiuwei[i][j].list, act_able[j].list, act_able[j].length * sizeof(int));
            act_able_xiuwei[i][j].length = act_able[j].length;
        }
    }

    for (int x = 0; x < REALM_COUNT; x++)
    {
        switch (x)
        {
        default:
        case 2:
            delete_to(act_able_xiuwei[x], Defend, 2, 4); // Gold light ward
            delete_to(act_able_xiuwei[x], Heal, 2, 5);   // heal
            delete_to(act_able_xiuwei[x], Smite, 3, 4);  // Greatsword mastery
            delete_to(act_able_xiuwei[x], Ranged, 2, 3); // Fire blast
        case 1:
            delete_to(act_able_xiuwei[x], Defend, 1, 2); // Energy shield
            delete_to(act_able_xiuwei[x], Heal, 1, 2);   // heal
            delete_to(act_able_xiuwei[x], Boost, 2, 4);  // Mind focus art
        case 0:
            break;
        }
    }
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

    if (g_log_count < MAX_LOG_TURNS)
    {
        AI_TurnLog *log = &g_game_log[g_log_count];
        log->round_number = game->round_number;

        // 记录当时的“战况快照”
        log->cpu_hp = CPU.HP;
        log->opponent_hp = YOU.HP;
        log->cpu_qi = CPU.QI;
        log->opponent_qi = YOU.QI;
    }

    return 0;
}

void Player_action(Game game, Player *YOU)
{
    while (YOU->action == None)
    {
        printf("\033[0m");
        ENG_PRINT("What's your next action?\n");
        CHN_PRINT("请选择你的下一次行动：\n");
        switch (YOU->XIUWEI)
        {
        case 0:
            ENG_PRINT("Q: Gain QI(0), A: Strike(1), D: Defend(1), H: Heal(1), C: Warcry(2), P: Parry(2), S: Smite(3)\n");
            CHN_PRINT("Q: 集气(0), A: 轻击(1), D: 防御(1), H: 养元(1), C: 战吼(2), P: 格挡(2), S: 重击(3)\n");
            break;
        case 1:
            ENG_PRINT("Q: Gain QI(0), A: Strike(1), F: Fireball(2), D: Shield(2), H: Heal(2), C: Mind focus art(4), P: Parry(2), B: Wind blades(3*), S: Smite(3)\n");
            CHN_PRINT("Q: 集气(0), A: 轻击(1), F: 火球(2), D: 灵力盾(2), H: 养元(1), C: 聚精会神(4), P: 格挡(2), B: 风刃(3*), S: 重击(3)\n");
            break;
        default:
            ENG_PRINT("Q: Gain QI(0), A: Strike(1), F: Fire blast(3), D: Gold light ward(4), H: Heal(5), C: Mind focus art(4), P: Parry(2), B: Sword burst(3*), S: Greatsword mastery(4), T: Thunder bolt(6)\n");
            CHN_PRINT("Q: 集气(0), A: 轻击(1), F: 火弹(2), D: 金光护体(4), H: 养元(5), C: 聚精会神(4), P: 格挡(2), B: 御剑(3*), S: 巨剑术(4), T: 唤雷(6)\n");
            break;
        }
        //  Prompt for action
        scanf("%c", &game.action);
        clear_buffer();
        // getchar();                          // Clear the newline character from the input buffer
        game.action = toupper(game.action); // Convert action to uppercase for consistency
        printf("\033[34m");
        switch (game.action) // 每个提示词都不同，难以抽象为统一函数
        {
        case 'Q': // Gain QI
            ENG_PRINT("<You chose to gain QI!>\n");
            CHN_PRINT("<你选择了集气!>\n");
            YOU->action = Gain_qi; // Set action to gain QI
            YOU->QI += YOU->gain_combo;
            ENG_PRINT("You gained %d QI! Your QI is now %d.\n", YOU->gain_combo, YOU->QI);
            CHN_PRINT("你集气成功，获得%d点气力！你的气力已变为%d。\n", YOU->gain_combo, YOU->QI);
            break;
        case 'A': // Melee attack
            if (YOU->QI < 1)
            {
                ENG_PRINT("You need at least 1 QI to attack!\n");
                CHN_PRINT("你需要至少1点气力才能攻击！\n");
                continue; // Skip the rest of the loop and prompt for action again
            }
            ENG_PRINT("<You chose to attack!>\n");
            CHN_PRINT("<你选择轻击!>\n");
            YOU->action_cost = 1; // Deduct QI for attacking
            YOU->action = Melee;  // Set action to attack
            break;
        case 'F': // Ranged attack
            switch (YOU->XIUWEI)
            {
            case 0:
                ENG_PRINT("Invalid action. Please choose again.\n");
                CHN_PRINT("无效操作，请重新选择。\n");
                continue;
            case 1:
                if (YOU->QI < 2)
                {
                    ENG_PRINT("You need at least 2 QI to throw a fireball!\n");
                    CHN_PRINT("你至少需要2点气才能释放火球！\n");
                    continue; // Skip the rest of the loop and prompt for action again
                }
                YOU->action_cost = 2;
                ENG_PRINT("<You throw a fireball to your enemy!>\n");
                CHN_PRINT("<你释放了一个火球！>\n");
                break;
            default:
                if (YOU->QI < 3)
                {
                    ENG_PRINT("You need at least 2 QI to shoot a fire blast!\n");
                    CHN_PRINT("你需要至少2点气来释放火球！\n");
                    continue; // Skip the rest of the loop and prompt for action again
                }
                YOU->action_cost = 3;
                ENG_PRINT("<You shoot a fire blast!>\n");
                CHN_PRINT("<你发射了一个火弹！>\n");
                break;
            }
            YOU->action = Ranged; // Set action to ranged attack
            break;
        case 'D': // Defend
            switch (YOU->XIUWEI)
            {
            case 0:
                if (YOU->QI < 1)
                {
                    ENG_PRINT("You need at least 1 QI to defend!\n");
                    CHN_PRINT("你至少需要1点气才能防御！\n");
                    continue; // Skip the rest of the loop and prompt for action again
                }
                ENG_PRINT("<You chose to defend!>\n");
                CHN_PRINT("<你选择了防御！>\n");
                YOU->action_cost = 1;
                break;
            case 1:
                if (YOU->QI < 2)
                {
                    ENG_PRINT("You need at least 2 QI to conjure an energy shield!\n");
                    CHN_PRINT("你需要至少2点气来凝聚一面灵力盾！\n");
                    continue; // Skip the rest of the loop and prompt for action again
                }
                ENG_PRINT("<You successfully conjured an energy shield!>\n");
                CHN_PRINT("<你成功凝聚了一面灵力盾！>\n");
                YOU->action_cost = 2;
                break;
            default:
                if (YOU->QI < 4)
                {
                    ENG_PRINT("You need at least 4 QI to summon Gold light warding!\n");
                    CHN_PRINT("你需要至少4点QI才能召唤金光护体！\n");
                    continue; // Skip the rest of the loop and prompt for action again
                }
                ENG_PRINT("<You summoned Gold light warding around you!>\n");
                CHN_PRINT("<你召唤了金光护体！>\n");
                YOU->action_cost = 4;
                break;
            }
            YOU->action = Defend; // Set action to defend
            break;
        case 'H': // Heal
            if (YOU->QI < YOU->YUAN)
            {
                ENG_PRINT("You need at least %d QI to heal!\n", YOU->YUAN);
                CHN_PRINT("你需要至少%d点气才能开始疗伤！\n", YOU->YUAN);
                continue; // Skip the rest of the loop and prompt for action again
            }
            YOU->action = Heal;
            YOU->healing = YOU->XIUWEI + 1; // Set healing status based on YOUR XIUWEI level
            YOU->action_cost = YOU->YUAN;   // Deduct QI for healing
            ENG_PRINT("<You start healing.\n");
            CHN_PRINT("你开始治疗。\n");
            break;
        case 'C': // Boost
            switch (YOU->XIUWEI)
            {
            case 0:
                if (YOU->QI < 2)
                {
                    ENG_PRINT("You need at least 2 QI to roar!\n");
                    CHN_PRINT("你需要至少两口气才能发出战吼!\n");
                    continue;
                }
                ENG_PRINT("<You performed warcry!>\n");
                CHN_PRINT("<你选择战吼！>\n");
                YOU->action_cost = 2;
                break;
            default:
                if (YOU->QI < 2)
                {
                    ENG_PRINT("You need at least 4 QI to focus your mind!\n");
                    CHN_PRINT("你至少需要4点气来聚精会神！\n");
                    continue;
                }
                ENG_PRINT("<You choose to focus your mind!>\n");
                CHN_PRINT("<你选择聚精会神！>\n");
                YOU->action_cost = 4; // Skip the rest of the loop and prompt for action again
                break;
            }
            YOU->action = Boost;
            break;
        case 'P': // Parry
            if (YOU->QI < 2)
            {
                ENG_PRINT("You need at least 2 QI to parry!\n");
                CHN_PRINT("<你没有足够的气力来格挡！>\n");
                continue;
            }
            YOU->action_cost = 2;
            YOU->action = Parry;
            ENG_PRINT("<You are ready to parry!>\n");
            CHN_PRINT("<你准备好格挡了！>\n");
            break;
        case 'B': // Burst attack
            switch (YOU->XIUWEI)
            {
            case 0:
                ENG_PRINT("<Invalid action. Please choose again.>\n");
                CHN_PRINT("<无效操作，请重新选择。>\n");
                continue;
            case 1:
                if (YOU->QI < 3)
                {
                    ENG_PRINT("You need 3 QI for each wind blade to form!\n");
                    CHN_PRINT("每把风刃需要3点气才能形成！\n");
                    continue; // Skip the rest of the loop and prompt for action again
                }
                YOU->burst_count += (YOU->QI - YOU->QI % 3) / 3; // Calculate the number of wind blades based on available QI
                YOU->action_cost = (YOU->QI - YOU->QI % 3);
                ENG_PRINT("<You formed %d wind blade!>\n", YOU->burst_count);
                CHN_PRINT("<你已形成%d把风刀!>\n", YOU->burst_count);
                break;
            default:
                if (YOU->QI < 3)
                {
                    ENG_PRINT("You need 3 QI to control each sword!\n");
                    CHN_PRINT("你需要3点气来控制每把剑!\n");
                    continue; // Skip the rest of the loop and prompt for action again
                }
                YOU->burst_count += (YOU->QI - YOU->QI % 3) / 3;
                YOU->action_cost = (YOU->QI - YOU->QI % 3);
                ENG_PRINT("<You controlled %d swords fly towards your enemy!>\n", YOU->burst_count);
                CHN_PRINT("<你控制了%d把剑向敌人飞去!>\n", YOU->burst_count);
                break;
            }
            YOU->action = Burst;
            break;
        case 'S': // Smite
            switch (YOU->XIUWEI)
            {
            case 0:
            case 1:
                if (YOU->QI < 3)
                {
                    ENG_PRINT("You need at least 3 QI to smite!\n");
                    CHN_PRINT("你至少需要3点气才能重击!\n");
                    continue; // Skip the rest of the loop and prompt for action again
                }
                YOU->action_cost = 3;
                ENG_PRINT("<You smite toward your enemy!>\n");
                CHN_PRINT("<你重击你的敌人！>\n");
                break;
            default:
                if (YOU->QI < 4)
                {
                    ENG_PRINT("You need 4 QI to form the Greatsword!\n");
                    CHN_PRINT("你需要4点气来凝聚巨剑！\n");
                    continue; // Skip the rest of the loop and prompt for action again
                }
                YOU->action_cost = 4;
                ENG_PRINT("<You form a Greatsword and smite toward your enemy!>\n");
                CHN_PRINT("<你凝聚巨剑，劈向你的敌人！>\n");
                break;
            }
            YOU->action = Smite;
            break;
        case 'T':                // Termination
            switch (YOU->XIUWEI) // Replace with actual condition or expression
            {
            case 0:
            case 1:
                ENG_PRINT("Invalid action. Please choose again.\n");
                CHN_PRINT("无效操作。请重新选择。\n");
                continue;
            default:
                if (YOU->QI < 5)
                {
                    ENG_PRINT("You need at least 5 QI to evoke thunder!\n");
                    CHN_PRINT("你需要至少5点气来唤起天雷!\n");
                    continue; // Skip the rest of the loop and prompt for action again
                }
                YOU->action_cost = 5;
                ENG_PRINT("<You evoked thunder!>\n");
                CHN_PRINT("<你唤起了天雷！>\n");
                break;
            }
            YOU->action = Terminate;
            break;
        default:
            ENG_PRINT("Invalid action. Please choose again.\n");
            CHN_PRINT("无效的技能。请重新选择。\n");
            break; // Skip the rest of the loop and prompt for action again
        }
    }
}

void CPU_action(Player *player) // 将参数名改为通用的 'player'
{
    printf("\033[91m"); // AI行动统一用红色

    if (player->action == Gain_qi)
    {
        ENG_PRINT("<%s chose to gain QI!>\n", player->name);
        CHN_PRINT("<%s 选择了集气!>\n", player->name);
        player->QI += player->gain_combo;
        ENG_PRINT("%s gained %d QI! Its QI is now %d.\n", player->name, player->gain_combo, player->QI);
        CHN_PRINT("%s 集气了 %d 点气! 它的气力现在为 %d.\n", player->name, player->gain_combo, player->QI);
    }
    else if (player->action == Melee)
    {
        ENG_PRINT("<%s chose to attack!>\n", player->name);
        CHN_PRINT("<%s 选择了轻击!>\n", player->name);
        player->action_cost = 1;
    }
    else if (player->action == Defend)
    {
        switch (player->XIUWEI)
        {
        case 0:
            ENG_PRINT("<%s chose to defend!>\n", player->name);
            CHN_PRINT("<%s 选择了防御!>\n", player->name);
            player->action_cost = 1;
            break;
        case 1:
            ENG_PRINT("<%s successfully conjured an energy shield!>\n", player->name);
            CHN_PRINT("<%s 成功凝聚了一面灵力盾!>\n", player->name);
            player->action_cost = 2;
            break;
        default:
            ENG_PRINT("<%s successfully conjured a Gold light barrier!>\n", player->name);
            CHN_PRINT("<%s 成功凝聚了一面金光屏障!>\n", player->name);
            player->action_cost = 4;
            break;
        }
    }
    else if (player->action == Heal)
    {
        player->action_cost = player->YUAN; // 治疗消耗应与境界相关
        player->healing = (player->XIUWEI + 1);
        ENG_PRINT("<%s starts to heal!>\n", player->name);
        CHN_PRINT("<%s 开始治疗自己!>\n", player->name);
    }
    else if (player->action == Boost)
    {
        switch (player->XIUWEI)
        {
        case 0:
            player->action_cost = 2;
            ENG_PRINT("<%s performed warcry!>\n", player->name);
            CHN_PRINT("<%s 怒吼!>\n", player->name);
            break;
        default:
            player->action_cost = 4;
            ENG_PRINT("<%s are focused!>\n", player->name);
            CHN_PRINT("<%s 聚精会神!>\n", player->name);
            break;
        }
    }
    else if (player->action == Parry)
    {
        player->action_cost = 2;
        ENG_PRINT("<%s set up a parry posture!>\n", player->name);
        CHN_PRINT("<%s 摆起格挡架势!>\n", player->name);
    }
    else if (player->action == Smite)
    {
        player->action_cost = 3;
        ENG_PRINT("<%s smites at you!>\n", player->name);
        CHN_PRINT("<%s 重击了你!>\n", player->name);
    }
    else if (player->action == Ranged)
    {
        switch (player->XIUWEI)
        {
        case 1:
            player->action_cost = 2;
            ENG_PRINT("<%s threw a fireball!>\n", player->name);
            CHN_PRINT("<%s 释放火球!>\n", player->name);
            break;
        default:
            player->action_cost = 3;
            ENG_PRINT("<%s shot a fire bullet!>\n", player->name);
            CHN_PRINT("<%s 发射火弹!>\n", player->name);
            break;
        }
    }
    else if (player->action == Burst)
    {
        int num_burst;
        switch (player->XIUWEI)
        {
        case 1:
            num_burst = (player->QI - player->QI % 3) / 3;
            player->burst_count += num_burst;
            player->action_cost = num_burst * 3;
            ENG_PRINT("<%s formed %d wind blade(s)!>\n", player->name, player->burst_count);
            CHN_PRINT("<%s 形成 %d 把风刃！>\n", player->name, player->burst_count);
            break;
        default:
            num_burst = (player->QI - player->QI % 3) / 3;
            player->burst_count += num_burst;
            player->action_cost = num_burst * 3;
            ENG_PRINT("<%s controlled %d swords fly towards you!>\n", player->name, player->burst_count);
            CHN_PRINT("<%s 控制了 %d 把飞剑向你飞来！>\n", player->name, player->burst_count);
            break;
        }
    }
    else if (player->action == Terminate)
    {
        player->action_cost = 5;
        ENG_PRINT("<%s evoked thunder!>\n", player->name);
        CHN_PRINT("<%s 唤起天雷！>\n", player->name);
    }

    printf("\033[0m");
}

void Action_resolve(Player *YOU, Player *CPU) // 互动解算
{
    printf("\033[33m"); // Set color to yellow for action resolution

    // **记录AI决策和结果之前，先保存当前状态**
    if (g_log_count < MAX_LOG_TURNS)
    {
        AI_TurnLog *log = &g_game_log[g_log_count];
        log->chosen_action = CPU->action;
    }

    Oneway_Solution(YOU, CPU);
    Oneway_Solution(CPU, YOU);

    // **记录AI决策和结果**
    if (g_log_count < MAX_LOG_TURNS)
    {
        AI_TurnLog *log = &g_game_log[g_log_count];
        log->damage_dealt = YOU->damage_received;
        log->damage_taken = CPU->damage_received;
        log->opponent_action = CPU->action;
        log->action_cost = YOU->action_cost;
        g_log_count++;
    }

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
    freopen("CON", "w", stdout);
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
void CPU_logic_V0(Player *CPU, const Player *opponent)
{
    // 1. 安全地获取当前QI值，防止越界
    int current_qi = CPU->QI;
    if (current_qi >= COST_LEVELS)
    {
        current_qi = COST_LEVELS - 1; // 如果QI过高, 使用最高等级的行动列表
    }

    // 2. 直接从预计算好的列表中获取可用行动
    //    这里的 act_able_xiuwei 必须是由修复后的 Initialize_Action_Lists 生成的
    Act_list *available_actions = &act_able_xiuwei[CPU->XIUWEI][current_qi];

    // 3. 如果有可用的行动，就随机选一个；否则，执行默认行动
    if (available_actions->length > 0)
    {
        int random_index = rand() % available_actions->length;
        CPU->action = available_actions->list[random_index];
    }
    else
    {
        // 保底措施: 理论上不应发生，因为总可以“集气”
        CPU->action = Gain_qi;
    }
}

// CPU_logic_V1A - 生存主义者 (最终优化版 - 懂得变通)
void CPU_logic_V1A(Player *cpu, const Player *opponent)
{
    int affordable_actions[TOTAL_ACTION_AMOUNT];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);
    if (affordable_count <= 0)
    {
        cpu->action = Gain_qi;
        return;
    }
    memcpy(affordable_actions, act_able_xiuwei[cpu->XIUWEI][(cpu->QI >= COST_LEVELS) ? COST_LEVELS - 1 : cpu->QI].list, affordable_count * sizeof(int));

    // 默认行动设为最稳妥的集气
    cpu->action = Gain_qi;

    // --- 决策逻辑 ---
    // 1. 低血量时，有80%的概率会治疗，20%的概率会继续默认的集气
    if (cpu->HP < max_HP[cpu->XIUWEI] * 0.3f && can_perform_action(affordable_actions, affordable_count, Heal))
    {
        if ((rand() % 10) < 8)
        { // 80% chance to heal
            cpu->action = Heal;
            return;
        }
    }

    // 2. 对手治疗时，有70%的概率攻击打断
    if (opponent->healing > 0 && can_perform_action(affordable_actions, affordable_count, Melee))
    {
        if ((rand() % 10) < 7)
        { // 70% chance to interrupt
            cpu->action = Melee;
            return;
        }
    }

    // 3. 灵气极低时，有90%的概率集气 (已经是默认值，但可以明确写出)
    if (cpu->QI < 3 && can_perform_action(affordable_actions, affordable_count, Gain_qi))
    {
        // 在这里，我们几乎总是会执行默认的 Gain_qi
        return;
    }

    // 4. 如果不满足以上任何高优先级倾向，则在所有可用行动中随机选择
    cpu->action = affordable_actions[rand() % affordable_count];
}

// CPU_logic_V1B - 狂战士 (最终优化版 - 懂得蓄力)
void CPU_logic_V1B(Player *cpu, const Player *opponent)
{
    int affordable_actions[TOTAL_ACTION_AMOUNT];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);
    if (affordable_count <= 0)
    {
        cpu->action = Gain_qi;
        return;
    }
    memcpy(affordable_actions, act_able_xiuwei[cpu->XIUWEI][(cpu->QI >= COST_LEVELS) ? COST_LEVELS - 1 : cpu->QI].list, affordable_count * sizeof(int));

    // 默认是集气，为进攻做准备
    cpu->action = Gain_qi;

    // 1. 斩杀是绝对优先，没有犹豫
    if (can_perform_action(affordable_actions, affordable_count, Smite) && opponent->HP <= 4 * cpu->ATK)
    {
        cpu->action = Smite;
        return;
    }
    if (can_perform_action(affordable_actions, affordable_count, Melee) && opponent->HP <= 1 * cpu->ATK)
    {
        cpu->action = Melee;
        return;
    }

    // 2. 没被激怒时，有80%概率使用Boost
    if (cpu->enraged == 0 && can_perform_action(affordable_actions, affordable_count, Boost))
    {
        if ((rand() % 10) < 8)
        {
            cpu->action = Boost;
            return;
        }
    }

    // 3. 猛攻！有70%的概率使用最高伤害技能
    if (can_perform_action(affordable_actions, affordable_count, Smite))
    {
        if ((rand() % 10) < 7)
        {
            cpu->action = Smite;
            return;
        }
    }
    if (can_perform_action(affordable_actions, affordable_count, Melee))
    {
        if ((rand() % 10) < 7)
        {
            cpu->action = Melee;
            return;
        }
    }
}

// CPU_logic_V1C - 神龟流 (Turtle)
void CPU_logic_V1C(Player *cpu, const Player *opponent)
{
    int affordable_actions[TOTAL_ACTION_AMOUNT];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);
    if (affordable_count <= 0)
    {
        cpu->action = Gain_qi;
        return;
    }
    memcpy(affordable_actions, act_able_xiuwei[cpu->XIUWEI][(cpu->QI >= COST_LEVELS) ? COST_LEVELS - 1 : cpu->QI].list, affordable_count * sizeof(int));

    // 默认行动是集气
    cpu->action = Gain_qi;

    // 1. 保命依然是最高优先级，但不再是100%
    if (cpu->HP < max_HP[cpu->XIUWEI] * 0.6f && can_perform_action(affordable_actions, affordable_count, Heal))
    {
        if ((rand() % 10) < 9)
        { // 90%的概率治疗
            cpu->action = Heal;
            return;
        }
    }

    // 2. 预判大招时，有80%的概率防御
    if ((opponent->QI > 4 || opponent->enraged > 0) && can_perform_action(affordable_actions, affordable_count, Defend))
    {
        if ((rand() % 10) < 8)
        {
            cpu->action = Defend;
            return;
        }
    }
}

// CPU_logic_V1D - 苦修者 (最终修正版 - 修复了逻辑漏洞)
void CPU_logic_V1D(Player *cpu, const Player *opponent)
{
    int affordable_actions[TOTAL_ACTION_AMOUNT];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);
    if (affordable_count <= 0)
    {
        cpu->action = Gain_qi;
        return;
    }
    memcpy(affordable_actions, act_able_xiuwei[cpu->XIUWEI][(cpu->QI >= COST_LEVELS) ? COST_LEVELS - 1 : cpu->QI].list, affordable_count * sizeof(int));

    // 策略1: 濒死时(<25%)才考虑治疗，否则修炼最重要
    if (cpu->HP < max_HP[cpu->XIUWEI] * 0.25f && can_perform_action(affordable_actions, affordable_count, Heal))
    {
        cpu->action = Heal;
        return;
    }

    // --- 【核心修改】重构决策逻辑，确保全覆盖 ---

    // 默认行动：在任何决策之前，先设定一个安全的默认值
    cpu->action = Gain_qi;

    // 当境界领先时，才考虑进攻
    if (cpu->XIUWEI > opponent->XIUWEI)
    {
        // 随机决定是进攻还是继续修炼
        if ((rand() % 3) != 0) // 2/3的概率选择进攻
        {
            // 优先使用最高效的攻击
            if (can_perform_action(affordable_actions, affordable_count, Smite))
            {
                cpu->action = Smite;
            }
            else if (can_perform_action(affordable_actions, affordable_count, Melee))
            {
                cpu->action = Melee;
            }
            // 如果都不能用，则会执行默认的 Gain_qi
        }
        // 如果随机到另外1/3的概率，则会跳过这个if，执行默认的 Gain_qi，继续修炼
    }

    // 如果境界不领先，则永远执行默认的 Gain_qi
}

// CPU_logic_V1E - 快攻压制 (最终优化版 - 压制更坚决)
void CPU_logic_V1E(Player *cpu, const Player *opponent)
{
    int affordable_actions[TOTAL_ACTION_AMOUNT];
    int affordable_count = get_affordable_actions(cpu, affordable_actions);
    if (affordable_count <= 0)
    {
        cpu->action = Gain_qi;
        return;
    }
    memcpy(affordable_actions, act_able_xiuwei[cpu->XIUWEI][(cpu->QI >= COST_LEVELS) ? COST_LEVELS - 1 : cpu->QI].list, affordable_count * sizeof(int));

    // 默认是集气
    cpu->action = Gain_qi;

    // 1. 斩杀是100%执行
    if (can_perform_action(affordable_actions, affordable_count, Melee) && opponent->HP <= 1 * cpu->ATK)
    {
        cpu->action = Melee;
        return;
    }

    // 2. 惩罚对手集气，90%概率执行
    if (opponent->action == Gain_qi && can_perform_action(affordable_actions, affordable_count, Boost))
    {
        if ((rand() % 10) < 9)
        {
            cpu->action = Boost;
            return;
        }
    }

    // 3. 持续攻击，90%概率执行
    if (can_perform_action(affordable_actions, affordable_count, Melee))
    {
        if ((rand() % 10) < 9)
        {
            cpu->action = Melee;
            return;
        }
    }
}

// EvaluateAction - AI的大脑 (V2 - 拥有长远规划和风险意识)
float EvaluateAction(ActionID action, const Player *cpu, const Player *opponent, const AI_Weights *weights)
{
    float score = 0.0f;
    int damage = 0;
    int qi_cost = 0;

    // --- 步骤 1: 计算该行动的基础伤害和QI消耗 ---
    // (我们需要一个简单的查找表或switch来获取QI消耗)
    switch (action)
    {
    case Melee:
        damage = 1 * cpu->ATK;
        qi_cost = 1;
        break;
    case Smite:
        damage = 4 * cpu->ATK;
        qi_cost = 3;
        break; // 简化为3，实际可能变化
    case Ranged:
        damage = 1 * cpu->ATK;
        qi_cost = 2;
        break; // 简化为2
    case Burst:
        damage = 1 * cpu->ATK * (cpu->QI / 3);
        qi_cost = cpu->QI - (cpu->QI % 3);
        break;
    case Terminate:
        damage = 5 * cpu->ATK;
        qi_cost = 5;
        break;
    case Heal:
        qi_cost = 1 + cpu->XIUWEI;
        break; // 治疗消耗与境界有关
    case Defend:
        qi_cost = 1 + cpu->XIUWEI;
        break; // 防御消耗与境界有关
    case Parry:
        qi_cost = 2;
        break;
    case Boost:
        qi_cost = 2;
        break;
    default:
        qi_cost = 0;
        break;
    }

    // 1. 基于自身灵根的策略调整
    switch (cpu->root)
    {
    case ROOT_Heavenly:
        // 我是天灵根，突破是我的王道！大幅增加集气的价值。
        if (action == Gain_qi)
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
        if (action == Melee)
            score -= 30.0f;
        if (action == Smite)
            score += 50.0f;
        break;
    case ROOT_Ethereal:
        // 对手是风灵根，闪避太高了。
        // 降低所有远程技能的价值，提升Boost(强化近战)和Melee的价值。
        if (action == Ranged || action == Burst)
            score -= 50.0f;
        if (action == Boost)
            score += 100.0f;
        break;
        // ...
    }

    // --- 步骤 2: 基于新旧指标进行综合评分 ---

    // 1. 【生存】治疗的价值 (旧逻辑)
    if (action == Heal)
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
        if (action == Ranged || action == Burst || action == Terminate)
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
    if (action == Defend || action == Parry)
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
    if (action == Gain_qi)
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
        if (action != Defend && action != Parry && action != Heal && action != Gain_qi)
        {
            score -= weights->w_low_hp_penalty;
        }
    }

    if (action == Boost)
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

        if (opponent->action == Gain_qi && opponent->QI >= 2)
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
    // 1. 获取所有当前可负担的行动（与V1相同）
    int current_qi = cpu->QI;
    int xiuwei = cpu->XIUWEI;
    if (current_qi >= COST_LEVELS)
    {
        current_qi = COST_LEVELS - 1;
    }
    Act_list *affordable_actions_list = &act_able_xiuwei[xiuwei][current_qi];
    int affordable_count = affordable_actions_list->length;

    if (affordable_count == 0)
    {
        cpu->action = Gain_qi; // 保底措施
        return;
    }

    // 2. 为每一个可行的行动打分
    ActionScore best_action = {None, -1.0f}; // 初始化一个“最佳行动”

    for (int i = 0; i < affordable_count; i++)
    {
        ActionID current_action_id = affordable_actions_list->list[i];

        // 调用AI大脑，为当前行动评分
        float score = EvaluateAction(current_action_id, cpu, opponent, &g_ai_weights);

        // 为了增加一点随机性，避免AI行为过于死板，可以给分数加上一个小的随机值
        score += (rand() % 10); // 增加0-9分的随机性

        // 3. 如果当前行动的得分超过了已知的最高分，就更新“最佳行动”
        if (score > best_action.score)
        {
            best_action.score = score;
            best_action.action_id = current_action_id;
        }
    }

    // 4. 做出最终决定
    cpu->action = best_action.action_id;
}

// 加载另一套权重
void CPU_logic_V2A(Player *cpu, const Player *opponent, int A)
{
    // 1. 获取所有当前可负担的行动（与V1相同）
    int current_qi = cpu->QI;
    int xiuwei = cpu->XIUWEI;
    if (current_qi >= COST_LEVELS)
    {
        current_qi = COST_LEVELS - 1;
    }
    Act_list *affordable_actions_list = &act_able_xiuwei[xiuwei][current_qi];
    int affordable_count = affordable_actions_list->length;

    if (affordable_count == 0)
    {
        cpu->action = Gain_qi; // 保底措施
        return;
    }

    // 2. 为每一个可行的行动打分
    ActionScore best_action = {None, -1.0f}; // 初始化一个“最佳行动”

    for (int i = 0; i < affordable_count; i++)
    {
        ActionID current_action_id = affordable_actions_list->list[i];

        // 调用AI大脑，为当前行动评分
        float score = EvaluateAction(current_action_id, cpu, opponent, &g_ai_weights_A[A]);

        // 为了增加一点随机性，避免AI行为过于死板，可以给分数加上一个小的随机值
        score += (rand() % 10); // 增加0-9分的随机性

        // 3. 如果当前行动的得分超过了已知的最高分，就更新“最佳行动”
        if (score > best_action.score)
        {
            best_action.score = score;
            best_action.action_id = current_action_id;
        }
    }

    // 4. 做出最终决定
    cpu->action = best_action.action_id;
}

// AI_Learn_From_Game - AI的学习方法 (V5.1 - 最终修正版)
void AI_Learn_From_Game(int cpu_won)
{
    float learning_rate = 0.01f;

    for (int i = 0; i < g_log_count; i++)
    {
        AI_TurnLog *log = &g_game_log[i];

        float turn_reward = 0.0f;

        if (log->opponent_hp > 0 && (log->opponent_hp - log->damage_dealt) <= 0)
            turn_reward += 100;
        if (i > 0 && log->cpu_xiuwei > g_game_log[i - 1].cpu_xiuwei)
            turn_reward += 200;
        if (log->action_cost > 0 && ((float)log->damage_dealt / log->action_cost) > 2.0f)
            turn_reward += 30;
        if ((log->chosen_action == Defend || log->chosen_action == Parry) && log->opponent_action == Smite && log->damage_taken < 4)
            turn_reward += 50;
        if (log->cpu_hp > 0 && (log->cpu_hp - log->damage_taken) <= 0)
            turn_reward -= 100;
        if (log->chosen_action == Heal && log->cpu_hp >= max_HP[log->cpu_xiuwei])
            turn_reward -= 20;
        if (log->opponent_action == Parry && log->damage_taken > 0)
            turn_reward -= 30;
        if ((log->cpu_hp - log->damage_taken) <= 0 && log->cpu_qi > max_QI[log->cpu_xiuwei] * 0.5f)
            turn_reward -= 50;
        if (log->damage_taken > log->cpu_hp * 0.3f)
            turn_reward -= 40;

        if (turn_reward == 0.0f)
        {
            turn_reward = cpu_won ? 1.0f : -1.0f;
        }

        float update_amount = learning_rate * turn_reward;

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
