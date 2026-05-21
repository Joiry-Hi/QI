typedef enum {
    UI_PHASE_RUN_START,
    UI_PHASE_BATTLE_ACTION,
    UI_PHASE_BATTLE_RESULT,
    UI_PHASE_REWARD_CHOICE,
    UI_PHASE_REWARD_REPLACE,
    UI_PHASE_HEART_DEMON_CHOICE,
    UI_PHASE_BREAKTHROUGH_CHOICE,
    UI_PHASE_TALENT_CHOICE,
    UI_PHASE_PREPARATION,
    UI_PHASE_ROUTE_CHOICE,
    UI_PHASE_TRIBULATION_EVENT,
    UI_PHASE_NEAR_DEATH_CHOICE,
    UI_PHASE_ENCOUNTER,
    UI_PHASE_BETWEEN_BATTLES,
    UI_PHASE_VICTORY,
    UI_PHASE_DEFEAT
} UIPhase;

#define UI_CHRONICLE_CAPACITY 64

typedef enum {
    UI_REPLACE_NONE,
    UI_REPLACE_SKILL,
    UI_REPLACE_ARTIFACT,
    UI_REPLACE_ELIXIR
} UIReplacementType;

typedef enum {
    UI_ROUTE_REWARD_BALANCED,
    UI_ROUTE_REWARD_CULTIVATION,
    UI_ROUTE_REWARD_ELIXIR,
    UI_ROUTE_REWARD_ARTIFACT,
    UI_ROUTE_REWARD_SKILL,
    UI_ROUTE_REWARD_THUNDER,
    UI_ROUTE_REWARD_SWORD,
    UI_ROUTE_REWARD_GHOST,
    UI_ROUTE_REWARD_REBIRTH
} UIRouteRewardBias;

typedef enum {
    UI_NEAR_DEATH_ESCAPE,
    UI_NEAR_DEATH_GHOST,
    UI_NEAR_DEATH_ACCEPT
} UINearDeathOptionType;

typedef enum {
    UI_BOSS_NONE,
    UI_BOSS_CHARGE,
    UI_BOSS_PRESSURE,
    UI_BOSS_OPENING
} UIBossPhase;

typedef enum {
    UI_BOSS_ARCHETYPE_NONE,
    UI_BOSS_ARCHETYPE_SWORD_GUARDIAN,
    UI_BOSS_ARCHETYPE_BLOOD_EMBRYO,
    UI_BOSS_ARCHETYPE_THUNDER_AVATAR,
    UI_BOSS_ARCHETYPE_SECT_ENFORCER,
    UI_BOSS_ARCHETYPE_FORMLESS_HEART,
    UI_BOSS_ARCHETYPE_ASCENSION_ECHO
} UIBossArchetype;

typedef enum {
    UI_ROUTE_SPECIAL_NONE,
    UI_ROUTE_SPECIAL_SWORD_TRIAL,
    UI_ROUTE_SPECIAL_BLOOD_BEAST_DEN,
    UI_ROUTE_SPECIAL_GHOST_MARKET_RITE,
    UI_ROUTE_SPECIAL_ALCHEMY_CAULDRON,
    UI_ROUTE_SPECIAL_DEMON_CONTRACT,
    UI_ROUTE_SPECIAL_SECT_TOURNAMENT,
    UI_ROUTE_SPECIAL_MORTAL_FAREWELL
} UIRouteSpecialRule;

typedef enum {
    UI_HEART_STEADY,
    UI_HEART_RISKY,
    UI_HEART_SUPPRESS
} UIHeartDemonMode;

typedef enum {
    UI_TRIBULATION_CROSS,
    UI_TRIBULATION_PROTECT,
    UI_TRIBULATION_SUPPRESS
} UITribulationChoice;

typedef struct {
    int spirit;
    int herbs;
} UIRouteYield;

typedef struct {
    const char *title;
    const char *desc;
    const char *risk_label;
    int risk;
    int realm_offset;
    UIRouteRewardBias reward_bias;
    RewardChoice immediate_reward;
    int hp_cost_pct;
    int cultivation_cost;
    UIRouteSpecialRule special_rule;
} UIRouteTemplate;

typedef struct {
    int template_id;
    UIRouteTemplate data;
} UIRouteOption;

static void UI_UnlockDefaultAlchemyRecipes(void);
static void UI_UnlockAlchemyRecipe(ElixirID id, const char *source);

static UIPhase g_ui_phase = UI_PHASE_RUN_START;
static RewardChoice g_ui_pending_rewards[REWARD_CHOICE_COUNT];
static RewardChoice g_ui_pending_replacement_reward;
static UIReplacementType g_ui_replacement_type = UI_REPLACE_NONE;
static UIPhase g_ui_replacement_return_phase = UI_PHASE_RUN_START;
static int g_ui_replacement_slot_count = 0;
static int g_ui_replacement_action_type = ACTION_TYPE_NONE;
static UIRouteOption g_ui_route_options[REWARD_CHOICE_COUNT];
static TalentID g_ui_talent_options[REWARD_CHOICE_COUNT];
static UIRouteTemplate g_ui_next_route;
static bool g_ui_has_next_route = false;
static int g_ui_last_route_template_id = -1;
static int g_ui_route_streak = 0;
static int g_ui_sword_tomb_chain = 0;
static int g_ui_beast_den_chain = 0;
static int g_ui_ghost_market_chain = 0;
static int g_ui_herb_valley_chain = 0;
static int g_ui_demon_ruins_chain = 0;
static int g_ui_sect_trial_chain = 0;
static int g_ui_mortal_world_chain = 0;
static int g_ui_calamity_debt = 0;
static int g_ui_age = 0;
static int g_ui_last_years_elapsed = 0;
static int g_ui_tribulation_pressure = 0;
static int g_ui_pending_tribulation_chance = 0;
static char g_ui_pending_tribulation_reason[192] = "";
static bool g_ui_has_reborn = false;
static char g_ui_ending_type[32] = "";
static char g_ui_ending_title[64] = "";
static char g_ui_ending_summary[256] = "";
static int g_ui_chronicle_age[UI_CHRONICLE_CAPACITY];
static char g_ui_chronicle_realm[UI_CHRONICLE_CAPACITY][32];
static char g_ui_chronicle_kind[UI_CHRONICLE_CAPACITY][24];
static char g_ui_chronicle_text[UI_CHRONICLE_CAPACITY][256];
static int g_ui_chronicle_count = 0;
static bool g_ui_breakthrough_offered = false;
static bool g_ui_near_death_can_ghost = false;
static int g_ui_near_death_ghost_chance = 0;
static const char *g_ui_near_death_reason = "";
static char g_ui_recent_logs[12][256];
static int g_ui_log_count = 0;
static char g_ui_feed_side[16][12];
static char g_ui_feed_kind[16][16];
static char g_ui_feed_text[16][256];
static int g_ui_feed_round[16];
static int g_ui_feed_count = 0;
static UIBossPhase g_ui_boss_phase = UI_BOSS_NONE;
static UIBossArchetype g_ui_boss_archetype = UI_BOSS_ARCHETYPE_NONE;
static int g_ui_boss_cycle = 0;
static const char *g_ui_enemy_intent_label = "试探";
static const char *g_ui_enemy_intent_hint = "敌人正在观察你的气机。";
static int g_ui_enemy_intent_confidence = 40;
static int g_ui_last_elixir_slot = -1;
static RewardChoice g_ui_last_encounter_reward;
static int g_ui_last_encounter_penalty = 0;
static const char *g_ui_last_encounter_title = "";
static const char *g_ui_last_encounter_text = "";

static void UI_RunEncounter(void) __attribute__((unused));
static bool UI_TryInsightSkill(RewardRarity rarity, bool bypass, const char *source);
static void UI_AddLog(const char *message);
static void UI_AddCombatFeed(const char *side, const char *kind, const char *message);

static const UIRouteTemplate UI_ROUTE_TEMPLATES[] = {
    {"清修山径", "沿灵脉缓行，适合沉淀修为。", "低风险", 0, -1, UI_ROUTE_REWARD_CULTIVATION,
     {REWARD_CULTIVATION, RARITY_COMMON, 0, 8, false}, 0, 0, UI_ROUTE_SPECIAL_NONE},
    {"坊市交易", "散修摊位真假难辨，但补给充足。", "低风险", 0, -1, UI_ROUTE_REWARD_ELIXIR,
     {REWARD_ELIXIR, RARITY_COMMON, ELIXIR_QI, 1, false}, 0, 0, UI_ROUTE_SPECIAL_NONE},
    {"妖兽巢穴", "妖气盘踞，战利品也更丰厚。", "高风险", 2, 1, UI_ROUTE_REWARD_ARTIFACT,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 12, false}, 10, 0, UI_ROUTE_SPECIAL_NONE},
    {"古修洞府", "残缺传承仍有余韵，可能指向新的功法。", "中风险", 1, 0, UI_ROUTE_REWARD_SKILL,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 10, false}, 0, 4, UI_ROUTE_SPECIAL_NONE},
    {"天劫残云", "劫雷未散，危险与破境机缘并存。", "高风险", 2, 1, UI_ROUTE_REWARD_THUNDER,
     {REWARD_ELIXIR, RARITY_RARE, ELIXIR_BREAKTHROUGH, 1, false}, 15, 0, UI_ROUTE_SPECIAL_NONE},
    {"无名剑冢", "断剑沉埋于荒丘，剑意仍在择主。", "中风险", 1, 0, UI_ROUTE_REWARD_SWORD,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 10, false}, 8, 0, UI_ROUTE_SPECIAL_NONE},
    {"丹霞药谷", "霞光浸染药田，丹香能补足漫长道途的损耗。", "低风险", 0, -1, UI_ROUTE_REWARD_ELIXIR,
     {REWARD_ELIXIR, RARITY_RARE, ELIXIR_HEALING, 2, false}, 0, 0, UI_ROUTE_SPECIAL_NONE},
    {"魔修遗迹", "残碑下魔意未散，越深入越容易换来禁忌机缘。", "高风险", 2, 1, UI_ROUTE_REWARD_GHOST,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 14, false}, 12, 0, UI_ROUTE_SPECIAL_NONE},
    {"灵兽山", "灵兽循山而居，留下可炼器的骨角鳞羽。", "中风险", 1, 0, UI_ROUTE_REWARD_ARTIFACT,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 12, false}, 6, 0, UI_ROUTE_SPECIAL_NONE},
    {"宗门试炼", "旧宗门仍以阵法考校后人，胜者可取一门正统传承。", "中风险", 1, 0, UI_ROUTE_REWARD_SKILL,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 12, false}, 0, 0, UI_ROUTE_SPECIAL_NONE},
    {"凡尘旧地", "回望来处，故人旧事也会化作心境。", "低风险", 0, -1, UI_ROUTE_REWARD_CULTIVATION,
     {REWARD_CULTIVATION, RARITY_COMMON, 0, 10, false}, 0, 0, UI_ROUTE_SPECIAL_NONE},
    {"星陨禁地", "夜幕下星砂倒悬，空间与因果在此失去常理。", "高风险", 2, 1, UI_ROUTE_REWARD_SKILL,
     {REWARD_CULTIVATION, RARITY_EPIC, 0, 18, false}, 15, 0, UI_ROUTE_SPECIAL_NONE},
};

static const int UI_ROUTE_TEMPLATE_COUNT = (int)(sizeof(UI_ROUTE_TEMPLATES) / sizeof(UI_ROUTE_TEMPLATES[0]));

static const UIRouteTemplate UI_GHOST_ROUTE_TEMPLATES[] = {
    {"还阳坛", "残破祭坛以阴火照骨，足够修为可换回肉身。", "还阳", 1, 0, UI_ROUTE_REWARD_REBIRTH,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 8, false}, 0, 0, UI_ROUTE_SPECIAL_NONE},
    {"阴市", "纸灯照着幽暗街巷，鬼商兜售丹药与魂契。", "幽冥", 1, 0, UI_ROUTE_REWARD_ELIXIR,
     {REWARD_ELIXIR, RARITY_RARE, ELIXIR_CLEAR_MIND, 1, false}, 0, 0, UI_ROUTE_SPECIAL_NONE},
    {"乱葬岗", "野坟间鬼火浮动，残魂记得许多失传招式。", "凶险", 2, 1, UI_ROUTE_REWARD_GHOST,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 10, false}, 8, 0, UI_ROUTE_SPECIAL_NONE},
};

static const UIRouteTemplate UI_SPECIAL_ROUTE_SWORD_TRIAL =
    {"剑修试炼", "剑冢剑意连成杀阵，唯有正面破阵才能取走真传。", "试炼", 2, 1, UI_ROUTE_REWARD_SWORD,
     {REWARD_CULTIVATION, RARITY_EPIC, 0, 16, false}, 12, 0, UI_ROUTE_SPECIAL_SWORD_TRIAL};

static const UIRouteTemplate UI_SPECIAL_ROUTE_BLOOD_BEAST =
    {"血煞兽窟", "连续深入兽巢后，血煞妖王盘踞于灵材矿脉之上。", "凶险", 2, 1, UI_ROUTE_REWARD_ARTIFACT,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 18, false}, 12, 0, UI_ROUTE_SPECIAL_BLOOD_BEAST_DEN};

static const UIRouteTemplate UI_SPECIAL_ROUTE_GHOST_MARKET =
    {"阴市法会", "阴市灯火连成法会，鬼商愿以更低代价牵引还阳机缘。", "幽冥", 1, 0, UI_ROUTE_REWARD_REBIRTH,
     {REWARD_CULTIVATION, RARITY_EPIC, 0, 12, false}, 0, 0, UI_ROUTE_SPECIAL_GHOST_MARKET_RITE};

static const UIRouteTemplate UI_SPECIAL_ROUTE_ALCHEMY_CAULDRON =
    {"丹炉开劫", "药谷丹炉借天雷成丹，成则丹香满谷，败则劫意入身。", "丹劫", 1, 0, UI_ROUTE_REWARD_ELIXIR,
     {REWARD_ELIXIR, RARITY_EPIC, ELIXIR_DISASTER_WARD, 1, false}, 6, 0, UI_ROUTE_SPECIAL_ALCHEMY_CAULDRON};

static const UIRouteTemplate UI_SPECIAL_ROUTE_DEMON_CONTRACT =
    {"魔心旧契", "遗迹深处的旧契仍在跳动，可用劫气换取禁忌传承。", "魔契", 2, 1, UI_ROUTE_REWARD_GHOST,
     {REWARD_CULTIVATION, RARITY_EPIC, 0, 20, false}, 18, 0, UI_ROUTE_SPECIAL_DEMON_CONTRACT};

static const UIRouteTemplate UI_SPECIAL_ROUTE_SECT_TOURNAMENT =
    {"内门大比", "宗门残阵开启大比幻境，胜者可在诸峰传承中择一。", "试炼", 1, 0, UI_ROUTE_REWARD_SKILL,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 16, false}, 0, 0, UI_ROUTE_SPECIAL_SECT_TOURNAMENT};

static const UIRouteTemplate UI_SPECIAL_ROUTE_MORTAL_FAREWELL =
    {"故人归尘", "凡尘故人已老，离别使道心沉静，也使岁月更重。", "低风险", 0, -1, UI_ROUTE_REWARD_CULTIVATION,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 14, false}, 0, 0, UI_ROUTE_SPECIAL_MORTAL_FAREWELL};

static const int UI_GHOST_ROUTE_TEMPLATE_COUNT = (int)(sizeof(UI_GHOST_ROUTE_TEMPLATES) / sizeof(UI_GHOST_ROUTE_TEMPLATES[0]));

static void UI_PrintEscaped(const char *text);
static const char *UI_RouteSpecialHint(UIRouteSpecialRule rule);

static const char *UI_PhaseName(UIPhase phase)
{
    switch (phase)
    {
    case UI_PHASE_RUN_START: return "run_start";
    case UI_PHASE_BATTLE_ACTION: return "battle_action";
    case UI_PHASE_BATTLE_RESULT: return "battle_result";
    case UI_PHASE_REWARD_CHOICE: return "reward_choice";
    case UI_PHASE_REWARD_REPLACE: return "reward_replace";
    case UI_PHASE_HEART_DEMON_CHOICE: return "heart_demon_choice";
    case UI_PHASE_BREAKTHROUGH_CHOICE: return "breakthrough_choice";
    case UI_PHASE_TALENT_CHOICE: return "talent_choice";
    case UI_PHASE_PREPARATION: return "preparation";
    case UI_PHASE_ROUTE_CHOICE: return "route_choice";
    case UI_PHASE_TRIBULATION_EVENT: return "tribulation_event";
    case UI_PHASE_NEAR_DEATH_CHOICE: return "near_death_choice";
    case UI_PHASE_ENCOUNTER: return "encounter";
    case UI_PHASE_BETWEEN_BATTLES: return "between_battles";
    case UI_PHASE_VICTORY: return "victory";
    case UI_PHASE_DEFEAT: return "defeat";
    default: return "run_start";
    }
}

static const char *UI_OpponentName(int opponent_type)
{
    switch (opponent_type)
    {
    case 0: return "混沌";
    case 1: return "破法者";
    case 2: return "狂战士";
    case 3: return "神龟流";
    case 4: return "苦修者";
    case 5: return "豪赌徒";
    case 6: return "狂才";
    case 7: return "悟道者";
    case 8: return "大元帅";
    default: return "未知对手";
    }
}

static const char *UI_OpponentTendency(int opponent_type)
{
    switch (opponent_type)
    {
    case 0: return "混沌随机";
    case 1: return "破气干扰";
    case 2: return "近战爆发";
    case 3: return "防御消耗";
    case 4: return "蓄势苦修";
    case 5: return "攒气终结";
    case 6: return "高阶调参";
    case 7: return "逐回合悟道";
    case 8: return "将帅轮换";
    default: return "未知";
    }
}

static const char *UI_NextBattleTypeName(int next_battle)
{
    if (next_battle > 0 && next_battle % 6 == 0)
        return "boss";
    if (next_battle > 0 && next_battle % 3 == 0)
        return "elite";
    return "normal";
}

static const char *UI_NextBattleThreatText(int next_battle)
{
    if (next_battle > 0 && next_battle % 6 == 0)
        return "Boss 战将提供更高稀有度奖励，也更考验构筑完整度。";
    if (next_battle > 0 && next_battle % 3 == 0)
        return "精英战即将到来，建议确认丹药和防御手段。";
    return "普通战斗，适合继续打磨当前流派。";
}

static int UI_PredictOpponentType(void)
{
    if (g_config.enemy_type >= 0 && g_config.enemy_type <= 8)
        return g_config.enemy_type;
    unsigned int seed = g_run.reward_seed + (unsigned int)(g_run.battle_index * 17 + 11);
    return (int)(seed % 5) + 1;
}

static int UI_BreakthroughCost(const Player *player)
{
    if (player->XIUWEI >= TOTAL_XIUWEI_LEVEL - 2)
        return 0;
    return max_QI[player->XIUWEI] * 2;
}

static int UI_MaxInt(int a, int b)
{
    return a > b ? a : b;
}

static const char *UI_SoulStateName(SoulState state)
{
    return state == SOUL_GHOST ? "ghost" : "body";
}

static const char *UI_SoulStateLabel(SoulState state)
{
    return state == SOUL_GHOST ? "鬼魂" : "肉身";
}

static int UI_EscapeCultivationCost(void)
{
    int breakthrough_cost = UI_BreakthroughCost(&YOU);
    int yuan_cost = Yuan[YOU.XIUWEI] * 8;
    return UI_MaxInt(yuan_cost, breakthrough_cost / 3);
}

static int UI_RebirthCultivationCost(void)
{
    int breakthrough_cost = UI_BreakthroughCost(&YOU);
    int yuan_cost = Yuan[YOU.XIUWEI] * 12;
    return UI_MaxInt(yuan_cost, breakthrough_cost / 2);
}

static int UI_BreakthroughChance(const Player *player)
{
    if (player->XIUWEI >= TOTAL_XIUWEI_LEVEL - 2)
        return 0;
    int chance = 55;
    if (player->root == ROOT_Heavenly)
        chance += 15;
    if (Player_HasArtifact(player, ARTIFACT_BREAKTHROUGH_SEAL))
        chance += 10;
    if (Player_HasTalent(player, TALENT_THUNDER_AFFINITY))
        chance += 8;
    chance += Player_ArtifactEffectiveIntBonus(player, ArtifactBreakthroughPct);
    chance += player->breakthrough_bonus;
    if (chance < 20) chance = 20;
    if (chance > 90) chance = 90;
    return chance;
}

static bool UI_CanBreakthrough(const Player *player)
{
    int cost = UI_BreakthroughCost(player);
    return cost > 0 && player->cultivation >= cost;
}

static int UI_LifespanForRealm(XIUWEI realm)
{
    switch (realm)
    {
    case MORTAL: return 70;
    case REFINING: return 120;
    case FOUNDATION: return 240;
    case CORE_FORM: return 500;
    case NASCENT_SOUL: return 1100;
    case SEVERING: return 2500;
    case REFINEMENT: return 5000;
    case UNITY: return 10000;
    case GREAT: return 60000;
    case ASCENSION:
    case IMMORTAL:
    default: return 999999;
    }
}

static const char *UI_LifePressureLabel(void)
{
    int remain = UI_LifespanForRealm(YOU.XIUWEI) - g_ui_age;
    if (YOU.soul_state == SOUL_GHOST)
    {
        if (remain <= 10)
            return "魂火将熄";
        if (remain <= 30)
            return "魂火微弱";
        return "阴寿未尽";
    }
    if (remain <= 5)
        return "寿元将尽";
    if (remain <= 20)
        return "寿元紧迫";
    if (remain <= 60)
        return "需谋突破";
    return "寿元尚宽";
}

static const char *UI_TribulationLabel(void)
{
    if (YOU.XIUWEI < NASCENT_SOUL)
        return g_ui_tribulation_pressure >= 8 ? "业力深重" : (g_ui_tribulation_pressure >= 4 ? "业力渐积" : "因果尚轻");
    if (YOU.XIUWEI < REFINEMENT)
        return g_ui_tribulation_pressure >= 8 ? "小劫将至" : (g_ui_tribulation_pressure >= 4 ? "劫意渐生" : "天机平稳");
    if (g_ui_tribulation_pressure >= 12)
        return "雷劫迫近";
    if (g_ui_tribulation_pressure >= 8)
        return "劫云压顶";
    if (g_ui_tribulation_pressure >= 4)
        return "劫意渐生";
    return "天机平稳";
}

static const char *UI_TribulationHint(void)
{
    if (YOU.XIUWEI < NASCENT_SOUL)
        return "元婴前只积累业力，不会直接触发雷劫审判；高风险路线会让后续天劫更难。";
    if (YOU.XIUWEI < REFINEMENT)
        return "当前多为小雷劫，失败会损失修为与元神，不会直接身死道销。";
    if (g_ui_tribulation_pressure >= 12)
        return "雷劫压力极高，战间很可能遭遇雷劫清算。";
    if (g_ui_tribulation_pressure >= 8)
        return "劫云已厚，高风险路线收益提高，但更容易引动雷劫。";
    if (g_ui_tribulation_pressure >= 4)
        return "天机开始偏移，可用清修或雷劫亲和压住压力。";
    return "雷劫尚远，可以按构筑节奏择路。";
}

static void UI_AdjustTribulationPressure(int delta)
{
    g_ui_tribulation_pressure += delta;
    if (g_ui_tribulation_pressure < 0)
        g_ui_tribulation_pressure = 0;
}

static void UI_AddChronicle(const char *kind, const char *text)
{
    if (!text || !text[0])
        return;
    int index;
    if (g_ui_chronicle_count < UI_CHRONICLE_CAPACITY)
    {
        index = g_ui_chronicle_count++;
    }
    else
    {
        memmove(g_ui_chronicle_age, g_ui_chronicle_age + 1, sizeof(g_ui_chronicle_age[0]) * (UI_CHRONICLE_CAPACITY - 1));
        memmove(g_ui_chronicle_realm, g_ui_chronicle_realm + 1, sizeof(g_ui_chronicle_realm[0]) * (UI_CHRONICLE_CAPACITY - 1));
        memmove(g_ui_chronicle_kind, g_ui_chronicle_kind + 1, sizeof(g_ui_chronicle_kind[0]) * (UI_CHRONICLE_CAPACITY - 1));
        memmove(g_ui_chronicle_text, g_ui_chronicle_text + 1, sizeof(g_ui_chronicle_text[0]) * (UI_CHRONICLE_CAPACITY - 1));
        index = UI_CHRONICLE_CAPACITY - 1;
    }
    g_ui_chronicle_age[index] = g_ui_age;
    snprintf(g_ui_chronicle_realm[index], sizeof(g_ui_chronicle_realm[index]), "%s", Realm[YOU.XIUWEI]);
    snprintf(g_ui_chronicle_kind[index], sizeof(g_ui_chronicle_kind[index]), "%s", kind ? kind : "event");
    snprintf(g_ui_chronicle_text[index], sizeof(g_ui_chronicle_text[index]), "%s", text);
}

static void UI_SetEnding(const char *type, const char *title, const char *summary)
{
    snprintf(g_ui_ending_type, sizeof(g_ui_ending_type), "%s", type ? type : "defeat");
    snprintf(g_ui_ending_title, sizeof(g_ui_ending_title), "%s", title ? title : "道途暂止");
    snprintf(g_ui_ending_summary, sizeof(g_ui_ending_summary), "%s", summary ? summary : "");
    UI_AddChronicle(type ? type : "ending", title ? title : "道途暂止");
    g_ui_phase = (type && strcmp(type, "ascension") == 0) ? UI_PHASE_VICTORY : UI_PHASE_DEFEAT;
}

static bool UI_CheckAscensionVictory(void)
{
    if (YOU.XIUWEI >= ASCENSION)
    {
        UI_SetEnding("ascension", "羽化飞升", "劫尽道成，飞升上界。本次修行圆满结束。");
        UI_AddCombatFeed("system", "victory", "羽化飞升，道途圆满。");
        return true;
    }
    return false;
}

static bool UI_CheckLifespanEnd(void)
{
    int lifespan = UI_LifespanForRealm(YOU.XIUWEI);
    if (g_ui_age < lifespan)
        return false;
    UI_SetEnding(YOU.soul_state == SOUL_GHOST ? "soul_decay" : "lifespan",
                 YOU.soul_state == SOUL_GHOST ? "魂火衰败" : "寿尽坐化",
                 YOU.soul_state == SOUL_GHOST ? "阴寿已尽，魂火无声散入夜色。" : "寿元耗尽，坐化于漫长道途之中。");
    UI_AddCombatFeed("system", "defeat", YOU.soul_state == SOUL_GHOST ? "魂火衰败，阴寿已尽。" : "寿元耗尽，道途止步。");
    return true;
}

static int UI_RealmTimeBase(XIUWEI realm)
{
    switch (realm)
    {
    case MORTAL: return 1;
    case REFINING: return 2;
    case FOUNDATION: return 4;
    case CORE_FORM: return 8;
    case NASCENT_SOUL: return 16;
    case SEVERING: return 32;
    case REFINEMENT: return 64;
    case UNITY: return 128;
    case GREAT: return 256;
    case ASCENSION:
    case IMMORTAL:
    default: return 1024;
    }
}

static int UI_RouteBaseYears(const UIRouteTemplate *route)
{
    int base = UI_RealmTimeBase(YOU.XIUWEI);
    if (!route)
        return base + 1;

    if (strcmp(route->title, "宗门试炼") == 0)
        return 2 + base / 3;
    if (route->special_rule == UI_ROUTE_SPECIAL_SECT_TOURNAMENT)
        return 2 + base / 2;
    if (strcmp(route->title, "坊市交易") == 0)
        return 1 + base / 4;
    if (strcmp(route->title, "清修山径") == 0)
        return 5 + base * 2;
    if (strcmp(route->title, "凡尘旧地") == 0)
        return 4 + base;
    if (route->special_rule == UI_ROUTE_SPECIAL_MORTAL_FAREWELL)
        return 6 + base;
    if (strcmp(route->title, "丹霞药谷") == 0)
        return 2 + base / 2;
    if (route->special_rule == UI_ROUTE_SPECIAL_ALCHEMY_CAULDRON)
        return 4 + base;
    if (strcmp(route->title, "古修洞府") == 0)
        return 3 + base;
    if (strcmp(route->title, "灵兽山") == 0)
        return 2 + base / 2;
    if (strcmp(route->title, "妖兽巢穴") == 0 || route->special_rule == UI_ROUTE_SPECIAL_BLOOD_BEAST_DEN)
        return 1 + base / 2;
    if (strcmp(route->title, "无名剑冢") == 0 || route->special_rule == UI_ROUTE_SPECIAL_SWORD_TRIAL)
        return 2 + base / 2;
    if (strcmp(route->title, "魔修遗迹") == 0 || route->special_rule == UI_ROUTE_SPECIAL_DEMON_CONTRACT)
        return 1 + base / 2;
    if (strcmp(route->title, "天劫残云") == 0)
        return 1 + base / 3;
    if (strcmp(route->title, "星陨禁地") == 0)
        return 1 + base / 2;
    if (route->reward_bias == UI_ROUTE_REWARD_REBIRTH)
        return 2 + base / 2;
    if (route->risk >= 2)
        return 1 + base / 2;
    if (route->risk <= 0)
        return 4 + base;
    return 2 + base;
}

static int UI_RouteYearVariance(const UIRouteTemplate *route)
{
    int base = UI_RealmTimeBase(YOU.XIUWEI);
    if (route && strcmp(route->title, "宗门试炼") == 0)
        return 2 + base / 8;
    if (route && route->special_rule == UI_ROUTE_SPECIAL_SECT_TOURNAMENT)
        return 3 + base / 8;
    if (route && route->risk >= 2)
        return 2 + base / 8;
    if (route && route->risk <= 0)
        return 3 + base / 6;
    return 2 + base / 10;
}

static int UI_RouteYearAdvance(const UIRouteTemplate *route)
{
    int base = UI_RouteBaseYears(route);
    int variance = UI_RouteYearVariance(route);
    if (variance < 1)
        variance = 1;
    if (variance > 18)
        variance = 18;
    int years = base + (int)(Run_Rand() % variance);
    if (YOU.XIUWEI == MORTAL && years > 10)
        years = 10;
    if (years < 1)
        years = 1;
    return years;
}

static int UI_RouteYearPreview(const UIRouteTemplate *route)
{
    int variance = UI_RouteYearVariance(route);
    int years = UI_RouteBaseYears(route) + variance / 2;
    if (YOU.XIUWEI == MORTAL && years > 10)
        years = 10;
    if (years < 1)
        years = 1;
    return years;
}

static void UI_AdvanceYearsForRoute(const UIRouteTemplate *route)
{
    g_ui_last_years_elapsed = UI_RouteYearAdvance(route);
    g_ui_age += g_ui_last_years_elapsed;
    int delta = YOU.XIUWEI / 2;
    if (route)
    {
        if (route->risk >= 2)
            delta += 2;
        else if (route->risk <= 0)
            delta -= 1;
        if (route->reward_bias == UI_ROUTE_REWARD_THUNDER)
            delta += YOU.XIUWEI < NASCENT_SOUL ? 1 : 2;
        if (route->reward_bias == UI_ROUTE_REWARD_CULTIVATION)
            delta -= 1;
    }
    if (Player_HasTalent(&YOU, TALENT_THUNDER_AFFINITY) && delta > 0)
        delta--;
    g_ui_tribulation_pressure += delta;
    if (g_ui_tribulation_pressure < 0)
        g_ui_tribulation_pressure = 0;
    char message[192];
    snprintf(message, sizeof(message), "闭关行走 %d 年，年岁至 %d。", g_ui_last_years_elapsed, g_ui_age);
    UI_AddLog(message);
    UI_AddChronicle("time", message);
}

static UIRouteYield UI_RouteYieldFor(const UIRouteTemplate *route, int battle_index)
{
    UIRouteYield yield = {2, 1};
    if (battle_index > 0 && battle_index % 3 == 0)
    {
        yield.spirit += 1;
        yield.herbs += 1;
    }
    if (battle_index > 0 && battle_index % 6 == 0)
    {
        yield.spirit += 2;
        yield.herbs += 1;
    }
    if (route)
    {
        yield.spirit += route->risk;
        if (route->risk >= 2)
        {
            yield.spirit += 2;
            yield.herbs += 1;
        }
        switch (route->reward_bias)
        {
        case UI_ROUTE_REWARD_ARTIFACT:
            yield.spirit += 3;
            break;
        case UI_ROUTE_REWARD_THUNDER:
            yield.spirit += 3;
            yield.herbs += 1;
            break;
        case UI_ROUTE_REWARD_SWORD:
            yield.spirit += 2;
            break;
        case UI_ROUTE_REWARD_ELIXIR:
        case UI_ROUTE_REWARD_REBIRTH:
            yield.herbs += 3;
            break;
        case UI_ROUTE_REWARD_SKILL:
            yield.spirit += 1;
            yield.herbs += 2;
            break;
        case UI_ROUTE_REWARD_CULTIVATION:
            yield.spirit += 1;
            yield.herbs += 1;
            break;
        default:
            break;
        }
        if (route->special_rule == UI_ROUTE_SPECIAL_BLOOD_BEAST_DEN)
            yield.spirit += 6;
        if (route->special_rule == UI_ROUTE_SPECIAL_SWORD_TRIAL)
            yield.spirit += 2;
        if (route->special_rule == UI_ROUTE_SPECIAL_ALCHEMY_CAULDRON)
            yield.herbs += 6;
        if (route->special_rule == UI_ROUTE_SPECIAL_DEMON_CONTRACT)
            yield.spirit += 3;
        if (route->special_rule == UI_ROUTE_SPECIAL_SECT_TOURNAMENT)
        {
            yield.spirit += 1;
            yield.herbs += 2;
        }
        if (route->special_rule == UI_ROUTE_SPECIAL_MORTAL_FAREWELL)
            yield.herbs += 1;
    }
    yield.spirit += g_ui_calamity_debt / 3;
    if (g_ui_calamity_debt >= 5)
        yield.herbs += 1;
    if (Player_HasTalent(&YOU, TALENT_ALCHEMY_RESIDUE))
        yield.herbs += 1;
    return yield;
}

static int UI_TribulationChanceForRoute(const UIRouteTemplate *route)
{
    if (YOU.XIUWEI < NASCENT_SOUL)
        return 0;
    int chance = g_ui_calamity_debt * 4 + g_ui_tribulation_pressure * 3 + YOU.XIUWEI * 2;
    if (YOU.XIUWEI < REFINEMENT)
        chance = chance * 2 / 3;
    if (route)
    {
        if (route->risk >= 2)
            chance += 8;
        else if (route->risk <= 0)
            chance -= 10;
        if (route->reward_bias == UI_ROUTE_REWARD_THUNDER)
            chance += 18;
        if (route->reward_bias == UI_ROUTE_REWARD_CULTIVATION)
            chance -= 8;
    }
    if (Player_HasTalent(&YOU, TALENT_THUNDER_AFFINITY))
        chance -= 10;
    if (chance < 0) chance = 0;
    if (chance > 75) chance = 75;
    return chance;
}

static const char *UI_TribulationRiskLabelForChance(int chance)
{
    if (chance >= 55)
        return "极高";
    if (chance >= 35)
        return "高";
    if (chance >= 18)
        return "中";
    if (chance > 0)
        return "低";
    return "无";
}

static const char *UI_RouteStoryHint(const UIRouteTemplate *route)
{
    if (!route)
        return "";
    if (route->special_rule != UI_ROUTE_SPECIAL_NONE)
        return UI_RouteSpecialHint(route->special_rule);
    switch (route->reward_bias)
    {
    case UI_ROUTE_REWARD_CULTIVATION: return route->risk <= 0 ? "稳步沉淀心境与修为，耗时较长。" : "以险地磨砺修为。";
    case UI_ROUTE_REWARD_ELIXIR: return "更容易补充丹药与药材。";
    case UI_ROUTE_REWARD_ARTIFACT: return "更容易获得灵材与法器机缘。";
    case UI_ROUTE_REWARD_SKILL: return "更容易获得功法或破格感悟。";
    case UI_ROUTE_REWARD_THUNDER: return "雷劫机缘更浓，收益与清算并存。";
    case UI_ROUTE_REWARD_SWORD: return "剑意更盛，可能推进剑修事件链。";
    case UI_ROUTE_REWARD_GHOST: return "幽冥与血道机缘更浓，劫气也更重。";
    case UI_ROUTE_REWARD_REBIRTH: return "鬼修还阳与护魂补给更明确。";
    default: return "收益较均衡。";
    }
}

static const char *UI_RouteRewardPreviewText(const UIRouteTemplate *route)
{
    if (!route)
        return "均衡奖励";
    if (route->special_rule == UI_ROUTE_SPECIAL_ALCHEMY_CAULDRON)
        return "高品质丹药 / 大量药材";
    if (route->special_rule == UI_ROUTE_SPECIAL_DEMON_CONTRACT)
        return "禁忌功法 / 幽冥血道机缘";
    if (route->special_rule == UI_ROUTE_SPECIAL_SECT_TOURNAMENT)
        return "稳定高品质功法";
    if (route->special_rule == UI_ROUTE_SPECIAL_MORTAL_FAREWELL)
        return "修为理解 / 雷劫压力缓解";
    if (route->special_rule == UI_ROUTE_SPECIAL_SWORD_TRIAL)
        return "剑修真传";
    if (route->special_rule == UI_ROUTE_SPECIAL_BLOOD_BEAST_DEN)
        return "大量灵材 / 法器";
    if (route->risk >= 2)
        return "高稀有奖励 / 高材料";
    if (route->risk <= 0)
        return "稳定成长 / 低风险补给";
    return "中等收益 / 构筑倾向";
}

static const char *UI_RouteChainPreview(const UIRouteTemplate *route)
{
    if (!route)
        return "";
    if (strcmp(route->title, "丹霞药谷") == 0)
        return g_ui_herb_valley_chain >= 1 ? "药谷再深一步，可能触发丹炉开劫。" : "连续药谷会触发丹炉机缘。";
    if (strcmp(route->title, "魔修遗迹") == 0)
        return g_ui_demon_ruins_chain >= 1 ? "魔意已应契，可能触发魔心旧契。" : "连续遗迹会加深禁忌契约。";
    if (strcmp(route->title, "宗门试炼") == 0)
        return g_ui_sect_trial_chain >= 1 ? "诸峰试炼将汇成内门大比。" : "连续试炼会触发宗门大比。";
    if (strcmp(route->title, "凡尘旧地") == 0)
        return g_ui_mortal_world_chain >= 1 ? "故人旧事渐近终章。" : "连续回凡尘会触发故人归尘。";
    if (strcmp(route->title, "无名剑冢") == 0)
        return g_ui_sword_tomb_chain >= 1 ? "剑冢剑意即将连成试炼。" : "连续剑冢会触发剑修试炼。";
    if (strcmp(route->title, "妖兽巢穴") == 0)
        return g_ui_beast_den_chain >= 1 ? "兽巢血煞正在汇聚。" : "连续妖兽路线会触发血煞兽窟。";
    return "";
}

static const char *UI_RouteChronicleText(const UIRouteTemplate *route)
{
    if (!route)
        return "择路前行，下一场生死搏杀将至。";
    switch (route->special_rule)
    {
    case UI_ROUTE_SPECIAL_SWORD_TRIAL:
        return "剑冢杀阵洞开，剑意如潮，逼人以命证剑。";
    case UI_ROUTE_SPECIAL_BLOOD_BEAST_DEN:
        return "血煞妖王盘踞矿脉，兽吼震动群山。";
    case UI_ROUTE_SPECIAL_GHOST_MARKET_RITE:
        return "阴市灯火成会，纸钱与魂契一同飞入夜色。";
    case UI_ROUTE_SPECIAL_ALCHEMY_CAULDRON:
        return "药谷丹炉引雷开劫，一炉丹香压住满谷云火。";
    case UI_ROUTE_SPECIAL_DEMON_CONTRACT:
        return "魔修遗迹深处旧契复燃，心魔低语许下禁忌传承。";
    case UI_ROUTE_SPECIAL_SECT_TOURNAMENT:
        return "宗门残阵重开诸峰幻境，一场内门大比跨越岁月而来。";
    case UI_ROUTE_SPECIAL_MORTAL_FAREWELL:
        return "故人归尘，凡尘旧事如雪落肩，道心反而更定。";
    case UI_ROUTE_SPECIAL_NONE:
    default:
        break;
    }
    if (strcmp(route->title, "丹霞药谷") == 0)
        return "丹霞药谷药气氤氲，采药炼丹之间又添数年春秋。";
    if (strcmp(route->title, "魔修遗迹") == 0)
        return "魔修遗迹碑影森然，禁忌机缘与劫气一同缠身。";
    if (strcmp(route->title, "灵兽山") == 0)
        return "灵兽山云雾开合，骨角鳞羽皆可化作炼器灵材。";
    if (strcmp(route->title, "宗门试炼") == 0)
        return "旧宗门残阵考校后来者，正统功法藏于阵眼。";
    if (strcmp(route->title, "凡尘旧地") == 0)
        return "重返凡尘旧地，故园人事化作一缕心境。";
    if (strcmp(route->title, "星陨禁地") == 0)
        return "星陨禁地空间倒悬，一步踏错便是因果乱流。";
    if (strcmp(route->title, "清修山径") == 0)
        return "沿山径清修多年，锋芒暂敛，心火渐明。";
    if (strcmp(route->title, "坊市交易") == 0)
        return "坊市灯火连夜，真假机缘都在摊位间流转。";
    if (strcmp(route->title, "妖兽巢穴") == 0)
        return "深入妖兽巢穴，血气与灵材气息一同扑面。";
    if (strcmp(route->title, "古修洞府") == 0)
        return "古修洞府石门半启，残缺传承仍有余温。";
    if (strcmp(route->title, "天劫残云") == 0)
        return "天劫残云未散，雷光照出破境机缘。";
    if (strcmp(route->title, "无名剑冢") == 0)
        return "无名剑冢风声如刃，断剑仍在挑选后人。";
    return "择路前行，下一场生死搏杀将至。";
}

static int UI_TribulationSuccessChance(bool protected_crossing)
{
    int chance = 68 + YOU.XIUWEI * 2 - g_ui_calamity_debt * 3 - g_ui_tribulation_pressure * 2;
    if (YOU.XIUWEI < REFINEMENT)
        chance += 12;
    if (protected_crossing)
        chance += 18;
    if (Player_HasTalent(&YOU, TALENT_THUNDER_AFFINITY))
        chance += 12;
    if (Player_HasArtifact(&YOU, ARTIFACT_THUNDER_WOOD_SWORD))
        chance += 8;
    if (chance < 15) chance = 15;
    if (chance > 90) chance = 90;
    return chance;
}

static int UI_TribulationProtectCultivationCost(void)
{
    return UI_MaxInt(Yuan[YOU.XIUWEI] * 10, UI_BreakthroughCost(&YOU) / 4);
}

static int UI_TribulationProtectSpiritCost(void)
{
    return 4 + YOU.XIUWEI;
}

static bool UI_TribulationProtectAvailable(void)
{
    return YOU.cultivation >= UI_TribulationProtectCultivationCost() &&
           YOU.spirit_materials >= UI_TribulationProtectSpiritCost();
}

static void UI_MaybeTriggerTribulation(const UIRouteTemplate *route)
{
    int chance = UI_TribulationChanceForRoute(route);
    g_ui_pending_tribulation_chance = chance;
    snprintf(g_ui_pending_tribulation_reason, sizeof(g_ui_pending_tribulation_reason),
             "%s %d，雷劫压力 %d，%s后天机震动。", YOU.XIUWEI < NASCENT_SOUL ? "业力" : "劫气",
             g_ui_calamity_debt, g_ui_tribulation_pressure,
             route ? route->title : "修行");
    if (chance > 0 && (int)(Run_Rand() % 100) < chance)
    {
        g_ui_phase = UI_PHASE_TRIBULATION_EVENT;
        UI_AddLog("劫云骤聚，雷劫临身。");
        UI_AddChronicle("tribulation", "劫云骤聚，雷劫临身。");
    }
    else
    {
        g_ui_phase = UI_PHASE_BETWEEN_BATTLES;
    }
}

static int UI_EffectiveRouteHpCostPct(const UIRouteTemplate *route)
{
    if (!route)
        return 0;
    int pct = route->hp_cost_pct;
    if (pct > 0 && route->reward_bias == UI_ROUTE_REWARD_THUNDER &&
        Player_HasTalent(&YOU, TALENT_THUNDER_AFFINITY))
        pct = (pct + 1) / 2;
    return pct;
}

static void UI_PrintRouteRiskLabel(const UIRouteTemplate *route)
{
    if (!route)
    {
        UI_PrintEscaped("普通");
        return;
    }
    if (Player_HasTalent(&YOU, TALENT_BLOOD_REBIRTH) && route->risk > 0)
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "激进·%s", route->risk_label);
        UI_PrintEscaped(buffer);
        return;
    }
    UI_PrintEscaped(route->risk_label);
}

static const char *UI_RouteBiasName(UIRouteRewardBias bias)
{
    switch (bias)
    {
    case UI_ROUTE_REWARD_CULTIVATION: return "修为";
    case UI_ROUTE_REWARD_ELIXIR: return "丹药";
    case UI_ROUTE_REWARD_ARTIFACT: return "法器";
    case UI_ROUTE_REWARD_SKILL: return "功法";
    case UI_ROUTE_REWARD_THUNDER: return "雷劫";
    case UI_ROUTE_REWARD_SWORD: return "剑修";
    case UI_ROUTE_REWARD_GHOST: return "幽冥";
    case UI_ROUTE_REWARD_REBIRTH: return "还阳";
    case UI_ROUTE_REWARD_BALANCED:
    default: return "均衡";
    }
}

static const char *UI_RouteSpecialName(UIRouteSpecialRule rule)
{
    switch (rule)
    {
    case UI_ROUTE_SPECIAL_SWORD_TRIAL: return "剑修试炼";
    case UI_ROUTE_SPECIAL_BLOOD_BEAST_DEN: return "血煞兽窟";
    case UI_ROUTE_SPECIAL_GHOST_MARKET_RITE: return "阴市法会";
    case UI_ROUTE_SPECIAL_ALCHEMY_CAULDRON: return "丹炉开劫";
    case UI_ROUTE_SPECIAL_DEMON_CONTRACT: return "魔心旧契";
    case UI_ROUTE_SPECIAL_SECT_TOURNAMENT: return "内门大比";
    case UI_ROUTE_SPECIAL_MORTAL_FAREWELL: return "故人归尘";
    case UI_ROUTE_SPECIAL_NONE:
    default: return "";
    }
}

static const char *UI_RouteSpecialHint(UIRouteSpecialRule rule)
{
    switch (rule)
    {
    case UI_ROUTE_SPECIAL_SWORD_TRIAL: return "胜利后更容易获得剑修真传。";
    case UI_ROUTE_SPECIAL_BLOOD_BEAST_DEN: return "胜利后灵材更多，法器与血道收益更明显。";
    case UI_ROUTE_SPECIAL_GHOST_MARKET_RITE: return "还阳成本降低，否则转为护魂补给。";
    case UI_ROUTE_SPECIAL_ALCHEMY_CAULDRON: return "丹药奖励更强，但会牵动雷劫。";
    case UI_ROUTE_SPECIAL_DEMON_CONTRACT: return "用劫气换禁忌机缘，幽冥/血道收益更高。";
    case UI_ROUTE_SPECIAL_SECT_TOURNAMENT: return "稳定获得更高品质功法倾向。";
    case UI_ROUTE_SPECIAL_MORTAL_FAREWELL: return "沉淀心境，缓解雷劫压力并增进理解。";
    case UI_ROUTE_SPECIAL_NONE:
    default: return "无特殊规则。";
    }
}

static int UI_RouteDebtDelta(const UIRouteTemplate *route)
{
    if (!route)
        return 0;
    if (route->risk <= 0)
        return -1;
    if (route->risk >= 2)
        return 2;
    return 1;
}

static int UI_RoutePredictedDebt(const UIRouteTemplate *route)
{
    int value = g_ui_calamity_debt + UI_RouteDebtDelta(route);
    return value < 0 ? 0 : value;
}

static int UI_ClampRealm(int realm)
{
    if (realm < MORTAL)
        return MORTAL;
    if (realm > TOTAL_XIUWEI_LEVEL - 2)
        return TOTAL_XIUWEI_LEVEL - 2;
    return realm;
}

static int UI_NextEnemyRealm(void)
{
    int next_battle = g_run.battle_index + 1;
    int offset = g_ui_has_next_route ? g_ui_next_route.realm_offset : 0;
    if (next_battle > 0 && next_battle % 6 == 0)
        offset += 1;
    return UI_ClampRealm((int)YOU.XIUWEI + offset);
}

static int UI_BattleCultivationGain(void)
{
    int gain = Yuan[YOU.XIUWEI] * 2;
    if (g_run.battle_index > 0 && g_run.battle_index % 3 == 0)
        gain += Yuan[YOU.XIUWEI];
    if (g_run.battle_index > 0 && g_run.battle_index % 6 == 0)
        gain += Yuan[YOU.XIUWEI];
    if (Player_HasTalent(&YOU, TALENT_ALCHEMY_RESIDUE))
        gain += 1;
    gain += Player_ArtifactEffectiveIntBonus(&YOU, ArtifactPostBattleCultivation);
    return gain;
}

static void UI_AddLog(const char *message)
{
    if (!message || !message[0])
        return;
    if (g_ui_log_count < 12)
    {
        snprintf(g_ui_recent_logs[g_ui_log_count++], sizeof(g_ui_recent_logs[0]), "%s", message);
    }
    else
    {
        memmove(g_ui_recent_logs[0], g_ui_recent_logs[1], 11 * sizeof(g_ui_recent_logs[0]));
        snprintf(g_ui_recent_logs[11], sizeof(g_ui_recent_logs[0]), "%s", message);
    }
}

static void UI_AddCombatFeed(const char *side, const char *kind, const char *message)
{
    if (!message || !message[0])
        return;
    if (g_ui_feed_count < 16)
    {
        snprintf(g_ui_feed_side[g_ui_feed_count], sizeof(g_ui_feed_side[0]), "%s", side ? side : "system");
        snprintf(g_ui_feed_kind[g_ui_feed_count], sizeof(g_ui_feed_kind[0]), "%s", kind ? kind : "info");
        snprintf(g_ui_feed_text[g_ui_feed_count], sizeof(g_ui_feed_text[0]), "%s", message);
        g_ui_feed_round[g_ui_feed_count] = game.round_number;
        g_ui_feed_count++;
    }
    else
    {
        memmove(g_ui_feed_side[0], g_ui_feed_side[1], 15 * sizeof(g_ui_feed_side[0]));
        memmove(g_ui_feed_kind[0], g_ui_feed_kind[1], 15 * sizeof(g_ui_feed_kind[0]));
        memmove(g_ui_feed_text[0], g_ui_feed_text[1], 15 * sizeof(g_ui_feed_text[0]));
        memmove(g_ui_feed_round, g_ui_feed_round + 1, 15 * sizeof(g_ui_feed_round[0]));
        snprintf(g_ui_feed_side[15], sizeof(g_ui_feed_side[0]), "%s", side ? side : "system");
        snprintf(g_ui_feed_kind[15], sizeof(g_ui_feed_kind[0]), "%s", kind ? kind : "info");
        snprintf(g_ui_feed_text[15], sizeof(g_ui_feed_text[0]), "%s", message);
        g_ui_feed_round[15] = game.round_number;
    }
    UI_AddLog(message);
}

static bool UI_IsBossBattle(void)
{
    int next_battle = g_run.battle_index + 1;
    return next_battle > 0 && next_battle % 6 == 0;
}

static bool UI_IsEliteBattle(void)
{
    int next_battle = g_run.battle_index + 1;
    return next_battle > 0 && next_battle % 3 == 0 && !UI_IsBossBattle();
}

static const char *UI_BossPhaseName(UIBossPhase phase)
{
    switch (phase)
    {
    case UI_BOSS_CHARGE: return "蓄势";
    case UI_BOSS_PRESSURE: return "压迫";
    case UI_BOSS_OPENING: return "破绽";
    case UI_BOSS_NONE:
    default: return "无";
    }
}

static const char *UI_BossPhaseKey(UIBossPhase phase)
{
    switch (phase)
    {
    case UI_BOSS_CHARGE: return "charge";
    case UI_BOSS_PRESSURE: return "pressure";
    case UI_BOSS_OPENING: return "opening";
    case UI_BOSS_NONE:
    default: return "none";
    }
}

static const char *UI_BossArchetypeKey(UIBossArchetype archetype)
{
    switch (archetype)
    {
    case UI_BOSS_ARCHETYPE_SWORD_GUARDIAN: return "sword_guardian";
    case UI_BOSS_ARCHETYPE_BLOOD_EMBRYO: return "blood_embryo";
    case UI_BOSS_ARCHETYPE_THUNDER_AVATAR: return "thunder_avatar";
    case UI_BOSS_ARCHETYPE_SECT_ENFORCER: return "sect_enforcer";
    case UI_BOSS_ARCHETYPE_FORMLESS_HEART: return "formless_heart";
    case UI_BOSS_ARCHETYPE_ASCENSION_ECHO: return "ascension_echo";
    case UI_BOSS_ARCHETYPE_NONE:
    default: return "none";
    }
}

static const char *UI_BossArchetypeName(UIBossArchetype archetype)
{
    switch (archetype)
    {
    case UI_BOSS_ARCHETYPE_SWORD_GUARDIAN: return "剑冢守灵";
    case UI_BOSS_ARCHETYPE_BLOOD_EMBRYO: return "血池魔胎";
    case UI_BOSS_ARCHETYPE_THUNDER_AVATAR: return "雷劫化身";
    case UI_BOSS_ARCHETYPE_SECT_ENFORCER: return "宗门执法";
    case UI_BOSS_ARCHETYPE_FORMLESS_HEART: return "无相心魔";
    case UI_BOSS_ARCHETYPE_ASCENSION_ECHO: return "飞升残影";
    case UI_BOSS_ARCHETYPE_NONE:
    default: return "无名强敌";
    }
}

static const char *UI_BossArchetypeDesc(UIBossArchetype archetype)
{
    switch (archetype)
    {
    case UI_BOSS_ARCHETYPE_SWORD_GUARDIAN: return "断剑残魂守在剑冢深处，剑势越压越紧。";
    case UI_BOSS_ARCHETYPE_BLOOD_EMBRYO: return "血池中孕出的魔胎，越拖越容易被血煞吞没。";
    case UI_BOSS_ARCHETYPE_THUNDER_AVATAR: return "劫云残意化形而来，蓄势后会落下雷霆爆发。";
    case UI_BOSS_ARCHETYPE_SECT_ENFORCER: return "宗门残阵凝成执法幻身，守势严密，反击清晰。";
    case UI_BOSS_ARCHETYPE_FORMLESS_HEART: return "心魔借你的道法成形，会模仿主流派寻找破绽。";
    case UI_BOSS_ARCHETYPE_ASCENSION_ECHO: return "飞升者残影横亘道途尽头，三相节奏更稳定。";
    case UI_BOSS_ARCHETYPE_NONE:
    default: return "强敌压境，考验构筑完整度。";
    }
}

static const char *UI_BossCounterHint(UIBossArchetype archetype)
{
    switch (archetype)
    {
    case UI_BOSS_ARCHETYPE_SWORD_GUARDIAN: return "压迫期少硬拼，破绽期再抢输出。";
    case UI_BOSS_ARCHETYPE_BLOOD_EMBRYO: return "避免拖长战斗，清负面和斩杀窗口很关键。";
    case UI_BOSS_ARCHETYPE_THUNDER_AVATAR: return "蓄势期准备防御，压迫期优先保命。";
    case UI_BOSS_ARCHETYPE_SECT_ENFORCER: return "它的守势强，适合用爆发或高阶功法破局。";
    case UI_BOSS_ARCHETYPE_FORMLESS_HEART: return "它会模仿你的主流派，观察意图再反制。";
    case UI_BOSS_ARCHETYPE_ASCENSION_ECHO: return "三相更稳定，按蓄势、压迫、破绽节奏应对。";
    case UI_BOSS_ARCHETYPE_NONE:
    default: return "观察意图提示，避开高威胁回合。";
    }
}

static const char *UI_BossRewardHint(UIBossArchetype archetype)
{
    switch (archetype)
    {
    case UI_BOSS_ARCHETYPE_SWORD_GUARDIAN: return "偏剑修真传。";
    case UI_BOSS_ARCHETYPE_BLOOD_EMBRYO: return "偏血道/幽冥与灵材。";
    case UI_BOSS_ARCHETYPE_THUNDER_AVATAR: return "偏雷法、护脉与破境机缘。";
    case UI_BOSS_ARCHETYPE_SECT_ENFORCER: return "偏护体与正统功法。";
    case UI_BOSS_ARCHETYPE_FORMLESS_HEART: return "偏当前主流派补强。";
    case UI_BOSS_ARCHETYPE_ASCENSION_ECHO: return "偏高稀有功法与飞升年表。";
    case UI_BOSS_ARCHETYPE_NONE:
    default: return "偏高稀有奖励。";
    }
}

static UIBossArchetype UI_BossArchetypeForRoute(const UIRouteTemplate *route)
{
    if (YOU.XIUWEI >= REFINEMENT || g_run.battle_index + 1 >= 18)
        return UI_BOSS_ARCHETYPE_ASCENSION_ECHO;
    if (route)
    {
        if (route->special_rule == UI_ROUTE_SPECIAL_SWORD_TRIAL || route->reward_bias == UI_ROUTE_REWARD_SWORD ||
            strcmp(route->title, "无名剑冢") == 0)
            return UI_BOSS_ARCHETYPE_SWORD_GUARDIAN;
        if (route->special_rule == UI_ROUTE_SPECIAL_BLOOD_BEAST_DEN ||
            route->special_rule == UI_ROUTE_SPECIAL_DEMON_CONTRACT ||
            route->reward_bias == UI_ROUTE_REWARD_GHOST ||
            strcmp(route->title, "魔修遗迹") == 0)
            return UI_BOSS_ARCHETYPE_BLOOD_EMBRYO;
        if (route->reward_bias == UI_ROUTE_REWARD_THUNDER || strcmp(route->title, "天劫残云") == 0)
            return UI_BOSS_ARCHETYPE_THUNDER_AVATAR;
        if (route->special_rule == UI_ROUTE_SPECIAL_SECT_TOURNAMENT || strcmp(route->title, "宗门试炼") == 0)
            return UI_BOSS_ARCHETYPE_SECT_ENFORCER;
    }
    BuildProfile profile = Player_GetBuildProfile(&YOU);
    if (profile.primary_count >= 4)
        return UI_BOSS_ARCHETYPE_FORMLESS_HEART;
    return UI_BOSS_ARCHETYPE_SECT_ENFORCER;
}

static UIBossArchetype UI_PredictBossArchetype(void)
{
    int next_battle = g_run.battle_index + 1;
    if (next_battle <= 0 || next_battle % 6 != 0)
        return UI_BOSS_ARCHETYPE_NONE;
    return UI_BossArchetypeForRoute(g_ui_has_next_route ? &g_ui_next_route : NULL);
}

static void UI_SetEnemyIntent(const char *label, const char *hint, int confidence)
{
    g_ui_enemy_intent_label = label;
    g_ui_enemy_intent_hint = hint;
    if (confidence < 0) confidence = 0;
    if (confidence > 100) confidence = 100;
    g_ui_enemy_intent_confidence = confidence;
}

static void UI_SetCPUActionFallback(ActionType preferred, ActionType fallback_a, ActionType fallback_b, ActionType fallback_c)
{
    if (preferred != ACTION_TYPE_NONE && can_perform_action(&CPU, preferred))
        CPU.current_action_type = preferred;
    else if (fallback_a != ACTION_TYPE_NONE && can_perform_action(&CPU, fallback_a))
        CPU.current_action_type = fallback_a;
    else if (fallback_b != ACTION_TYPE_NONE && can_perform_action(&CPU, fallback_b))
        CPU.current_action_type = fallback_b;
    else if (fallback_c != ACTION_TYPE_NONE && can_perform_action(&CPU, fallback_c))
        CPU.current_action_type = fallback_c;
    else
        CPU.current_action_type = ACTION_TYPE_GAIN_QI;
}

static void UI_ChooseBossAction(void)
{
    g_ui_boss_cycle++;
    int phase_index = (g_ui_boss_cycle - 1) % 3;
    g_ui_boss_phase = phase_index == 0 ? UI_BOSS_CHARGE : (phase_index == 1 ? UI_BOSS_PRESSURE : UI_BOSS_OPENING);
    UIBossArchetype archetype = g_ui_boss_archetype == UI_BOSS_ARCHETYPE_NONE ? UI_PredictBossArchetype() : g_ui_boss_archetype;

    if (g_ui_boss_phase == UI_BOSS_CHARGE)
    {
        switch (archetype)
        {
        case UI_BOSS_ARCHETYPE_SWORD_GUARDIAN:
            UI_SetEnemyIntent("剑意归鞘", "剑冢守灵收束剑势，下一轮可能反击或终结。", 86);
            UI_SetCPUActionFallback(ACTION_TYPE_COUNTER, ACTION_TYPE_BOOST, ACTION_TYPE_GAIN_QI, ACTION_TYPE_NONE);
            break;
        case UI_BOSS_ARCHETYPE_BLOOD_EMBRYO:
            UI_SetEnemyIntent("血池翻涌", "血池魔胎正在蓄养血煞，拖久会更危险。", 84);
            CPU.enraged += 1;
            UI_SetCPUActionFallback(ACTION_TYPE_BOOST, ACTION_TYPE_GAIN_QI, ACTION_TYPE_MELEE, ACTION_TYPE_NONE);
            break;
        case UI_BOSS_ARCHETYPE_THUNDER_AVATAR:
            UI_SetEnemyIntent("劫云聚顶", "雷劫化身正在蓄雷，压迫期爆发很高。", 88);
            CPU.QI_change += Yuan[CPU.XIUWEI];
            UI_SetCPUActionFallback(ACTION_TYPE_GAIN_QI, ACTION_TYPE_BOOST, ACTION_TYPE_DEFEND, ACTION_TYPE_NONE);
            break;
        case UI_BOSS_ARCHETYPE_SECT_ENFORCER:
            UI_SetEnemyIntent("律令成阵", "宗门执法立起守势，贸然攻击可能被反制。", 84);
            UI_SetCPUActionFallback(ACTION_TYPE_DEFEND, ACTION_TYPE_COUNTER, ACTION_TYPE_GAIN_QI, ACTION_TYPE_NONE);
            break;
        case UI_BOSS_ARCHETYPE_FORMLESS_HEART:
            UI_SetEnemyIntent("无相映心", "无相心魔模仿你的道法，准备针对主流派出手。", 86);
            UI_SetCPUActionFallback(ACTION_TYPE_BOOST, ACTION_TYPE_COUNTER, ACTION_TYPE_GAIN_QI, ACTION_TYPE_NONE);
            break;
        case UI_BOSS_ARCHETYPE_ASCENSION_ECHO:
            UI_SetEnemyIntent("仙影垂目", "飞升残影稳定蓄势，三相节奏不会轻易错乱。", 90);
            UI_SetCPUActionFallback(ACTION_TYPE_BOOST, ACTION_TYPE_GAIN_QI, ACTION_TYPE_DEFEND, ACTION_TYPE_NONE);
            break;
        default:
            UI_SetEnemyIntent("气机暴涨", "Boss 正在蓄势，下一轮可能转入强压。", 82);
            UI_SetCPUActionFallback(ACTION_TYPE_BOOST, ACTION_TYPE_GAIN_QI, ACTION_TYPE_NONE, ACTION_TYPE_NONE);
            break;
        }
        UI_AddCombatFeed("enemy", "intent", UI_BossArchetypeDesc(archetype));
        return;
    }

    if (g_ui_boss_phase == UI_BOSS_PRESSURE)
    {
        switch (archetype)
        {
        case UI_BOSS_ARCHETYPE_SWORD_GUARDIAN:
            UI_SetEnemyIntent("剑势未尽", "守灵剑势压下，优先防御或等待破绽。", 90);
            UI_SetCPUActionFallback(ACTION_TYPE_TERMINATE, ACTION_TYPE_COUNTER, ACTION_TYPE_SMITE, ACTION_TYPE_BURST);
            break;
        case UI_BOSS_ARCHETYPE_BLOOD_EMBRYO:
            UI_SetEnemyIntent("血煞噬身", "魔胎以血煞强压，可能加深流血与诅咒。", 88);
            YOU.bleeding += 1;
            if (CPU.HP < Player_MaxHP(&CPU) / 2)
                CPU.enraged += 1;
            UI_SetCPUActionFallback(ACTION_TYPE_BURST, ACTION_TYPE_SMITE, ACTION_TYPE_MELEE, ACTION_TYPE_BOOST);
            break;
        case UI_BOSS_ARCHETYPE_THUNDER_AVATAR:
            UI_SetEnemyIntent("劫雷将落", "雷劫化身进入爆发回合，防御窗口很关键。", 92);
            UI_SetCPUActionFallback(ACTION_TYPE_BURST, ACTION_TYPE_TERMINATE, ACTION_TYPE_SMITE, ACTION_TYPE_RANGED);
            break;
        case UI_BOSS_ARCHETYPE_SECT_ENFORCER:
            UI_SetEnemyIntent("执法反震", "执法幻身以守代攻，反击和惩击都很危险。", 86);
            UI_SetCPUActionFallback(ACTION_TYPE_COUNTER, ACTION_TYPE_SMITE, ACTION_TYPE_DEFEND, ACTION_TYPE_MELEE);
            break;
        case UI_BOSS_ARCHETYPE_FORMLESS_HEART:
        {
            BuildProfile profile = Player_GetBuildProfile(&YOU);
            UI_SetEnemyIntent("借道反噬", "无相心魔借你的主流派压迫心神。", 88);
            if (profile.primary_school == SCHOOL_QI)
                UI_SetCPUActionFallback(ACTION_TYPE_BOOST, ACTION_TYPE_BURST, ACTION_TYPE_SMITE, ACTION_TYPE_RANGED);
            else if (profile.primary_school == SCHOOL_DEFENSE)
                UI_SetCPUActionFallback(ACTION_TYPE_SMITE, ACTION_TYPE_TERMINATE, ACTION_TYPE_BURST, ACTION_TYPE_GAIN_QI);
            else
                UI_SetCPUActionFallback(ACTION_TYPE_BURST, ACTION_TYPE_SMITE, ACTION_TYPE_COUNTER, ACTION_TYPE_MELEE);
            break;
        }
        case UI_BOSS_ARCHETYPE_ASCENSION_ECHO:
            UI_SetEnemyIntent("飞升一击", "飞升残影将灵压转为终局攻势。", 94);
            UI_SetCPUActionFallback(ACTION_TYPE_TERMINATE, ACTION_TYPE_BURST, ACTION_TYPE_SMITE, ACTION_TYPE_COUNTER);
            break;
        default:
            UI_SetEnemyIntent("杀意凝聚", "Boss 将尝试把灵气优势转为爆发伤害。", 88);
            UI_SetCPUActionFallback(ACTION_TYPE_TERMINATE, ACTION_TYPE_BURST, ACTION_TYPE_SMITE, ACTION_TYPE_GAIN_QI);
            break;
        }
        UI_AddCombatFeed("enemy", "intent", UI_BossCounterHint(archetype));
        return;
    }

    switch (archetype)
    {
    case UI_BOSS_ARCHETYPE_BLOOD_EMBRYO:
        UI_SetEnemyIntent("血潮短歇", "血池魔胎短暂回气，这是抢攻或净化的窗口。", 80);
        UI_SetCPUActionFallback(ACTION_TYPE_GAIN_QI, ACTION_TYPE_DEFEND, ACTION_TYPE_MELEE, ACTION_TYPE_NONE);
        break;
    case UI_BOSS_ARCHETYPE_SECT_ENFORCER:
        UI_SetEnemyIntent("阵脚显隙", "执法幻身阵脚松动，爆发技能更容易打开局面。", 80);
        UI_SetCPUActionFallback(ACTION_TYPE_DEFEND, ACTION_TYPE_GAIN_QI, ACTION_TYPE_NONE, ACTION_TYPE_NONE);
        break;
    case UI_BOSS_ARCHETYPE_FORMLESS_HEART:
        UI_SetEnemyIntent("心相紊乱", "心魔模仿失衡，短暂露出真实破绽。", 82);
        UI_SetCPUActionFallback(ACTION_TYPE_GAIN_QI, ACTION_TYPE_DEFEND, ACTION_TYPE_NONE, ACTION_TYPE_NONE);
        break;
    default:
        UI_SetEnemyIntent("气机回落", "Boss 大势用尽，短暂露出破绽。", 78);
        UI_SetCPUActionFallback(ACTION_TYPE_DEFEND, ACTION_TYPE_COUNTER, ACTION_TYPE_GAIN_QI, ACTION_TYPE_NONE);
        break;
    }
    UI_AddCombatFeed("enemy", "intent", "Boss 气机回落，露出一线破绽。");
}

static void UI_PrintEscaped(const char *text)
{
    if (!text)
        return;
    for (const unsigned char *p = (const unsigned char *)text; *p; p++)
    {
        switch (*p)
        {
        case '\\': fputs("\\\\", stdout); break;
        case '"': fputs("\\\"", stdout); break;
        case '\n': fputs("\\n", stdout); break;
        case '\r': fputs("\\r", stdout); break;
        case '\t': fputs("\\t", stdout); break;
        default:
            if (*p < 32)
                printf("\\u%04x", *p);
            else
                putchar(*p);
            break;
        }
    }
}

static const char *UI_ActionName(ActionType action)
{
    if (action >= 0 && action < TOTAL_ACTION_TYPES)
        return ActionTypeNames_CHN[action];
    return "无";
}

static const char *UI_RarityName(RewardRarity rarity)
{
    switch (rarity)
    {
    case RARITY_COMMON: return "common";
    case RARITY_RARE: return "rare";
    case RARITY_EPIC: return "epic";
    case RARITY_LEGENDARY: return "legendary";
    default: return "unknown";
    }
}

static const char *UI_RewardTypeName(RewardType type)
{
    switch (type)
    {
    case REWARD_ARTIFACT: return "artifact";
    case REWARD_ELIXIR: return "elixir";
    case REWARD_SKILL: return "skill";
    case REWARD_CULTIVATION: return "cultivation";
    default: return "unknown";
    }
}

static int UI_RespecSkillCost(SkillID id)
{
    if (id <= SKILL_ID_NONE || id >= TOTAL_SKILLS)
        return 0;
    const Skill *skill = &g_skill_database[id];
    int base = Yuan[YOU.XIUWEI] * (2 + skill->rank);
    if (base < 1)
        base = 1;
    return base;
}

static bool UI_CanManageSkills(void)
{
    return g_ui_phase == UI_PHASE_ROUTE_CHOICE ||
           g_ui_phase == UI_PHASE_PREPARATION ||
           g_ui_phase == UI_PHASE_BETWEEN_BATTLES ||
           g_ui_phase == UI_PHASE_BREAKTHROUGH_CHOICE ||
           g_ui_phase == UI_PHASE_HEART_DEMON_CHOICE ||
           g_ui_phase == UI_PHASE_TALENT_CHOICE ||
           g_ui_phase == UI_PHASE_REWARD_CHOICE;
}

static const char *UI_ReplacementTypeName(UIReplacementType type)
{
    switch (type)
    {
    case UI_REPLACE_SKILL: return "skill";
    case UI_REPLACE_ARTIFACT: return "artifact";
    case UI_REPLACE_ELIXIR: return "elixir";
    case UI_REPLACE_NONE:
    default: return "none";
    }
}

static const char *UI_RewardFitLabel(const Player *player, const RewardChoice *choice)
{
    switch (choice->type)
    {
    case REWARD_SKILL:
    {
        if (choice->id <= SKILL_ID_NONE || choice->id >= TOTAL_SKILLS)
            return "转化修炼";
        const Skill *skill = &g_skill_database[choice->id];
        if (Player_HasTalent(player, TALENT_SWORD_HEART) && skill->school_tag == SCHOOL_SWORD)
            return "剑心共鸣";
        if (Player_HasTalent(player, TALENT_CAVE_INSIGHT) && choice->bypass_prereq)
            return "洞府悟法";
        if (skill->school_tag != SCHOOL_NONE && player->school_counts[skill->school_tag] >= 3)
            return "核心补强";
        if (skill->school_tag != SCHOOL_NONE && player->school_counts[skill->school_tag] > 0)
            return "路线契合";
        if (player->equipped_skills[skill->action_type] == SKILL_ID_NONE)
            return "补齐动作";
        return "过渡可用";
    }
    case REWARD_ARTIFACT:
        if (choice->id > ARTIFACT_NONE && choice->id < TOTAL_ARTIFACTS &&
            Player_HasArtifact(player, (ArtifactID)choice->id))
            return "已有法器";
        return player->artifact_count >= MAX_ARTIFACT_SLOTS ? "需要取舍" : "稳定补强";
    case REWARD_ELIXIR:
        return player->elixir_count >= MAX_ELIXIR_SLOTS ? "丹囊已满" : "战术资源";
    case REWARD_CULTIVATION:
        return player->QI >= Player_MaxQI(player) * 3 / 4 ? "突破蓄势" : "修为积累";
    default:
        return "未知";
    }
}

static void UI_PrintRewardFitReason(const Player *player, const RewardChoice *choice)
{
    switch (choice->type)
    {
    case REWARD_SKILL:
    {
        if (choice->id <= SKILL_ID_NONE || choice->id >= TOTAL_SKILLS)
        {
            UI_PrintEscaped("技能池暂时见底，将转化为修炼资源。");
            return;
        }
        const Skill *skill = &g_skill_database[choice->id];
        SkillID old_id = player->equipped_skills[skill->action_type];
        char buffer[256];
        if (old_id != SKILL_ID_NONE)
        {
            snprintf(buffer, sizeof(buffer), "与当前%s槽的%s竞争；新技能为%d阶、耗气%d。",
                     UI_ActionName(skill->action_type), g_skill_database[old_id].name_chn,
                     skill->rank, skill->cost);
        }
        else if (Player_HasTalent(player, TALENT_SWORD_HEART) && skill->school_tag == SCHOOL_SWORD)
        {
            snprintf(buffer, sizeof(buffer), "剑心通明提高剑修奖励评价；当前剑修进度%d/5。",
                     player->school_counts[SCHOOL_SWORD]);
        }
        else if (Player_HasTalent(player, TALENT_CAVE_INSIGHT) && choice->bypass_prereq)
        {
            snprintf(buffer, sizeof(buffer), "洞府悟法让破格传承更有价值；该技能可绕过通常前置。");
        }
        else if (skill->school_tag != SCHOOL_NONE && player->school_counts[skill->school_tag] > 0)
        {
            int count = player->school_counts[skill->school_tag];
            snprintf(buffer, sizeof(buffer), "强化%s路线，当前该流派已有%d个已悟技能%s。",
                     SchoolTagName(skill->school_tag), count,
                     count == 4 ? "，接近核心流派" : "");
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "提供%s动作的新选择，可作为后续构筑分支。",
                     UI_ActionName(skill->action_type));
        }
        UI_PrintEscaped(buffer);
        return;
    }
    case REWARD_ARTIFACT:
        if (player->artifact_count >= MAX_ARTIFACT_SLOTS)
            UI_PrintEscaped("法器槽已满，选择后需要替换一件旧法器或放弃新法器。");
        else
            UI_PrintEscaped("直接加入法器栏，提供持续战斗修正。");
        return;
    case REWARD_ELIXIR:
        if (player->elixir_count >= MAX_ELIXIR_SLOTS)
            UI_PrintEscaped("丹药袋已满，选择后需要替换一枚旧丹药或放弃新丹药。");
        else
            UI_PrintEscaped("直接加入丹药袋，可在后续战斗行动前使用。");
        return;
    case REWARD_CULTIVATION:
        UI_PrintEscaped("获得气力资源，帮助维持高费用技能或推动突破节奏。");
        return;
    default:
        UI_PrintEscaped("未知奖励。");
        return;
    }
}

static bool UI_RewardWillReplace(const Player *player, const RewardChoice *choice)
{
    if (choice->type == REWARD_SKILL && choice->id > SKILL_ID_NONE && choice->id < TOTAL_SKILLS)
    {
        ActionType action = g_skill_database[choice->id].action_type;
        return action >= 0 && action < TOTAL_ACTION_TYPES && player->equipped_skills[action] != SKILL_ID_NONE;
    }
    if (choice->type == REWARD_ARTIFACT)
        return player->artifact_count >= MAX_ARTIFACT_SLOTS;
    if (choice->type == REWARD_ELIXIR)
        return player->elixir_count >= MAX_ELIXIR_SLOTS;
    return false;
}

static void UI_PrintRewardReplaceTarget(const Player *player, const RewardChoice *choice)
{
    if (choice->type == REWARD_SKILL && choice->id > SKILL_ID_NONE && choice->id < TOTAL_SKILLS)
    {
        const Skill *skill = &g_skill_database[choice->id];
        SkillID old_id = player->equipped_skills[skill->action_type];
        printf("{\"slot\":%d,\"kind\":\"skill\",\"action\":\"", skill->action_type);
        UI_PrintEscaped(UI_ActionName(skill->action_type));
        printf("\"");
        if (old_id != SKILL_ID_NONE)
        {
            printf(",\"id\":%d,\"name\":\"", old_id);
            UI_PrintEscaped(g_skill_database[old_id].name_chn);
            printf("\",\"rank\":%d,\"cost\":%d", g_skill_database[old_id].rank, g_skill_database[old_id].cost);
        }
        printf("}");
        return;
    }
    if (choice->type == REWARD_ARTIFACT)
    {
        printf("{\"slot_count\":%d,\"kind\":\"artifact\"}", MAX_ARTIFACT_SLOTS);
        return;
    }
    if (choice->type == REWARD_ELIXIR)
    {
        printf("{\"slot_count\":%d,\"kind\":\"elixir\"}", MAX_ELIXIR_SLOTS);
        return;
    }
    printf("null");
}

static void UI_PrintRewardJSON(const RewardChoice *choice, int index)
{
    int score = RewardChoiceScore(&YOU, choice);
    bool will_replace = UI_RewardWillReplace(&YOU, choice);
    bool is_custom = false;
    if (choice->type == REWARD_SKILL && choice->id > SKILL_ID_NONE && choice->id < TOTAL_SKILLS)
        is_custom = g_skill_database[choice->id].is_custom || Skill_IsCustomID((SkillID)choice->id);
    else if (choice->type == REWARD_ARTIFACT && choice->id > ARTIFACT_NONE && choice->id < TOTAL_ARTIFACTS)
        is_custom = g_artifact_database[choice->id].is_custom || Artifact_IsCustomID((ArtifactID)choice->id);
    else if (choice->type == REWARD_ELIXIR && choice->id > ELIXIR_NONE && choice->id < TOTAL_ELIXIRS)
        is_custom = g_elixir_database[choice->id].is_custom || Elixir_IsCustomID((ElixirID)choice->id);
    printf("{\"index\":%d,\"type\":\"%s\",\"rarity\":\"%s\",\"id\":%d,\"amount\":%d,\"bypass\":%s,\"custom\":%s,\"score\":%d,\"fit_label\":\"",
           index, UI_RewardTypeName(choice->type), UI_RarityName(choice->rarity),
           choice->id, choice->amount, choice->bypass_prereq ? "true" : "false",
           is_custom ? "true" : "false", score);
    UI_PrintEscaped(UI_RewardFitLabel(&YOU, choice));
    printf("\",\"fit_reason\":\"");
    UI_PrintRewardFitReason(&YOU, choice);
    printf("\",\"will_replace\":%s,\"replace_target\":", will_replace ? "true" : "false");
    UI_PrintRewardReplaceTarget(&YOU, choice);
    if (choice->type == REWARD_SKILL && choice->id > SKILL_ID_NONE && choice->id < TOTAL_SKILLS)
    {
        const Skill *skill = &g_skill_database[choice->id];
        char effect_summary[192];
        SkillEffectSummary(skill, effect_summary, sizeof(effect_summary));
        printf(",\"name\":\"");
        UI_PrintEscaped(skill->name_chn);
        printf("\",\"desc\":\"");
        UI_PrintEscaped(skill->prompt_chn);
        printf("\",\"effect_summary\":\"");
        UI_PrintEscaped(effect_summary);
        printf("\",\"action\":\"");
        UI_PrintEscaped(UI_ActionName(skill->action_type));
        printf("\",\"school\":\"");
        UI_PrintEscaped(SchoolTagName(skill->school_tag));
        printf("\",\"rank\":%d,\"cost\":%d,\"custom\":%s", skill->rank, skill->cost, skill->is_custom ? "true" : "false");
    }
    else if (choice->type == REWARD_ARTIFACT && choice->id > ARTIFACT_NONE && choice->id < TOTAL_ARTIFACTS)
    {
        const Artifact *artifact = &g_artifact_database[choice->id];
        printf(",\"name\":\"");
        UI_PrintEscaped(artifact->name_chn);
        printf("\",\"desc\":\"");
        UI_PrintEscaped(artifact->desc_chn);
        printf("\",\"custom\":%s", artifact->is_custom ? "true" : "false");
    }
    else if (choice->type == REWARD_ELIXIR && choice->id > ELIXIR_NONE && choice->id < TOTAL_ELIXIRS)
    {
        const Elixir *elixir = &g_elixir_database[choice->id];
        printf(",\"name\":\"");
        UI_PrintEscaped(elixir->name_chn);
        printf("\",\"desc\":\"");
        UI_PrintEscaped(elixir->desc_chn);
        printf("\",\"custom\":%s", elixir->is_custom ? "true" : "false");
    }
    else if (choice->type == REWARD_CULTIVATION)
    {
        printf(",\"name\":\"修炼资源\",\"desc\":\"获得修为并推动战后突破\"");
    }
    printf("}");
}

static void UI_PrintTalentJSON(TalentID id, int index)
{
    printf("{\"index\":%d,\"id\":%d,\"name\":\"", index, id);
    if (id >= 0 && id < TOTAL_TALENTS)
    {
        const Talent *talent = &g_talent_database[id];
        UI_PrintEscaped(talent->name_chn);
        printf("\",\"desc\":\"");
        UI_PrintEscaped(talent->desc_chn);
        printf("\",\"school\":\"");
        UI_PrintEscaped(SchoolTagName(talent->school_tag));
        printf("\"}");
    }
    else
    {
        UI_PrintEscaped("修为补偿");
        printf("\",\"desc\":\"天赋池已见底，转化为修为。\",\"school\":\"无\"}");
    }
}

static void UI_PrintPlayerJSON(const char *key, const Player *player)
{
    printf("\"%s\":{\"name\":\"", key);
    UI_PrintEscaped(player->name);
    printf("\",\"hp\":%d,\"max_hp\":%d,\"qi\":%d,\"max_qi\":%d,\"atk\":%d,\"yuan\":%d,\"cultivation\":%d,\"minor_understanding\":%d,\"minor_threshold\":%d,\"minor_realm_level\":%d,\"minor_bonus_pct\":%d,\"realm\":\"",
           player->HP, Player_MaxHP(player), player->QI, Player_MaxQI(player), player->ATK, player->YUAN,
           player->cultivation, player->minor_understanding, RPG_MinorRealmThreshold(player),
           player->minor_realm_level, RPG_MinorRealmBonusPct(player));
    UI_PrintEscaped(Realm[player->XIUWEI]);
    printf("\",\"realm_id\":%d,\"root\":\"", player->XIUWEI);
    UI_PrintEscaped(CHN_Root_Names[player->root]);
    printf("\",\"soul_state\":\"");
    UI_PrintEscaped(UI_SoulStateName(player->soul_state));
    printf("\",\"soul_state_label\":\"");
    UI_PrintEscaped(UI_SoulStateLabel(player->soul_state));
    printf("\",\"bleeding\":%d,\"cursed\":%d,\"enraged\":%d", player->bleeding, player->cursed, player->enraged);
    if (strcmp(key, "enemy") == 0)
    {
        printf(",\"intent\":{\"label\":\"");
        UI_PrintEscaped(g_ui_enemy_intent_label);
        printf("\",\"hint\":\"");
        UI_PrintEscaped(g_ui_enemy_intent_hint);
        printf("\",\"confidence\":%d}", g_ui_enemy_intent_confidence);
    }
    printf("}");
}

static void UI_PrintReplacementJSON(void)
{
    printf("\"replacement\":{\"active\":%s,\"type\":\"%s\",\"slot_count\":%d,\"action_type\":%d,\"reward\":",
           g_ui_phase == UI_PHASE_REWARD_REPLACE ? "true" : "false",
           UI_ReplacementTypeName(g_ui_replacement_type),
           g_ui_replacement_slot_count,
           g_ui_replacement_action_type);
    if (g_ui_phase == UI_PHASE_REWARD_REPLACE)
        UI_PrintRewardJSON(&g_ui_pending_replacement_reward, 0);
    else
        printf("null");
    printf(",\"options\":[");
    if (g_ui_phase == UI_PHASE_REWARD_REPLACE)
    {
        for (int i = 0; i < g_ui_replacement_slot_count; i++)
        {
            if (i) printf(",");
            printf("{\"slot\":%d", i);
            if (g_ui_replacement_type == UI_REPLACE_SKILL)
            {
                SkillID id = YOU.equipped_skills[g_ui_replacement_action_type];
                if (id != SKILL_ID_NONE)
                {
                    char effect_summary[192];
                    SkillEffectSummary(&g_skill_database[id], effect_summary, sizeof(effect_summary));
                    printf(",\"id\":%d,\"name\":\"", id);
                    UI_PrintEscaped(g_skill_database[id].name_chn);
                    printf("\",\"desc\":\"");
                    UI_PrintEscaped(g_skill_database[id].prompt_chn);
                    printf("\",\"effect_summary\":\"");
                    UI_PrintEscaped(effect_summary);
                    printf("\",\"rank\":%d,\"cost\":%d,\"custom\":%s,\"school\":\"",
                           g_skill_database[id].rank, g_skill_database[id].cost,
                           g_skill_database[id].is_custom ? "true" : "false");
                    UI_PrintEscaped(SchoolTagName(g_skill_database[id].school_tag));
                    printf("\"");
                }
            }
            else if (g_ui_replacement_type == UI_REPLACE_ARTIFACT)
            {
                ArtifactID id = YOU.artifacts[i];
                printf(",\"id\":%d,\"name\":\"", id);
                UI_PrintEscaped(g_artifact_database[id].name_chn);
                printf("\",\"desc\":\"");
                UI_PrintEscaped(g_artifact_database[id].desc_chn);
                printf("\",\"custom\":%s", g_artifact_database[id].is_custom ? "true" : "false");
            }
            else if (g_ui_replacement_type == UI_REPLACE_ELIXIR)
            {
                ElixirID id = YOU.elixirs[i];
                printf(",\"id\":%d,\"name\":\"", id);
                UI_PrintEscaped(g_elixir_database[id].name_chn);
                printf("\",\"desc\":\"");
                UI_PrintEscaped(g_elixir_database[id].desc_chn);
                printf("\",\"custom\":%s", g_elixir_database[id].is_custom ? "true" : "false");
            }
            printf("}");
        }
    }
    printf("]}");
}

static void UI_PrintNextBattleJSON(void)
{
    int next_battle = g_run.battle_index + 1;
    int opponent_type = UI_PredictOpponentType();
    int enemy_realm = UI_NextEnemyRealm();
    UIBossArchetype predicted_boss = UI_PredictBossArchetype();
    printf("\"next_battle\":{\"index\":%d,\"type\":\"%s\",\"risk\":\"",
           next_battle, UI_NextBattleTypeName(next_battle));
    if (g_ui_has_next_route)
        UI_PrintRouteRiskLabel(&g_ui_next_route);
    else
        UI_PrintEscaped("普通");
    printf("\",\"enemy\":\"");
    UI_PrintEscaped(UI_OpponentName(opponent_type));
    printf("\",\"tendency\":\"");
    UI_PrintEscaped(UI_OpponentTendency(opponent_type));
    printf("\",\"enemy_realm\":\"");
    UI_PrintEscaped(Realm[enemy_realm]);
    printf("\",\"enemy_realm_id\":%d,\"route\":\"", enemy_realm);
    UI_PrintEscaped(g_ui_has_next_route ? g_ui_next_route.title : "未择路线");
    printf("\",\"boss_name\":\"");
    UI_PrintEscaped(predicted_boss != UI_BOSS_ARCHETYPE_NONE ? UI_BossArchetypeName(predicted_boss) : "");
    printf("\",\"boss_hint\":\"");
    UI_PrintEscaped(predicted_boss != UI_BOSS_ARCHETYPE_NONE ? UI_BossCounterHint(predicted_boss) : "");
    printf("\",\"reward_bias\":\"");
    UI_PrintEscaped(g_ui_has_next_route ? UI_RouteBiasName(g_ui_next_route.reward_bias) : "均衡");
    printf("\",\"calamity_debt\":%d,\"debt_realm_bonus\":0,\"tribulation_pressure\":%d,\"special_rule\":\"",
           g_ui_calamity_debt, g_ui_tribulation_pressure);
    UI_PrintEscaped(g_ui_has_next_route ? UI_RouteSpecialName(g_ui_next_route.special_rule) : "");
    printf("\",\"threat\":\"");
    if (g_ui_has_next_route)
    {
        char threat[256];
        snprintf(threat, sizeof(threat), "%s 劫气 %d，雷劫%s，可能在战间清算。", g_ui_next_route.desc, g_ui_calamity_debt,
                 UI_TribulationLabel());
        UI_PrintEscaped(threat);
    }
    else
        UI_PrintEscaped(UI_NextBattleThreatText(next_battle));
    printf("\"}");
}

static void UI_PrintBreakthroughJSON(void)
{
    int cost = UI_BreakthroughCost(&YOU);
    int chance = UI_BreakthroughChance(&YOU);
    printf("\"breakthrough\":{\"available\":%s,\"cost\":%d,\"chance\":%d,\"current_realm\":\"",
           UI_CanBreakthrough(&YOU) ? "true" : "false", cost, chance);
    UI_PrintEscaped(Realm[YOU.XIUWEI]);
    printf("\",\"next_realm\":\"");
    if (YOU.XIUWEI < TOTAL_XIUWEI_LEVEL - 2)
        UI_PrintEscaped(Realm[YOU.XIUWEI + 1]);
    else
        UI_PrintEscaped("已近巅峰");
    printf("\",\"cultivation\":%d}", YOU.cultivation);
}

static void UI_PrintHeartDemonOptionJSON(int index, UIHeartDemonMode mode)
{
    int base_cost = UI_BreakthroughCost(&YOU);
    int cost = base_cost;
    int chance = UI_BreakthroughChance(&YOU);
    const char *key = "steady";
    const char *title = "稳妥突破";
    const char *desc = "多耗修为压住心魔，成功率更高，失败损伤更轻。";
    if (mode == UI_HEART_STEADY)
    {
        cost = base_cost * 5 / 4;
        chance += 10;
    }
    else if (mode == UI_HEART_RISKY)
    {
        key = "risky";
        title = "冒险突破";
        desc = "以较少修为冲关，成功后理解更深，失败反噬更重。";
        cost = base_cost * 3 / 4;
        chance -= 15;
    }
    else if (mode == UI_HEART_SUPPRESS)
    {
        key = "suppress";
        title = "压境不破";
        desc = "暂不突破，沉淀理解，但劫气随压境而涨。";
        cost = 0;
    }
    if (chance < 5) chance = 5;
    if (chance > 95) chance = 95;
    printf("{\"index\":%d,\"mode\":\"", index);
    UI_PrintEscaped(key);
    printf("\",\"title\":\"");
    UI_PrintEscaped(title);
    printf("\",\"desc\":\"");
    UI_PrintEscaped(desc);
    printf("\",\"cost\":%d,\"chance\":%d,\"available\":%s}", cost, chance,
           YOU.cultivation >= cost ? "true" : "false");
}

static void UI_PrintHeartDemonJSON(void)
{
    printf("\"heart_demon\":{\"active\":%s,\"options\":[",
           g_ui_phase == UI_PHASE_HEART_DEMON_CHOICE ? "true" : "false");
    if (g_ui_phase == UI_PHASE_HEART_DEMON_CHOICE)
    {
        UI_PrintHeartDemonOptionJSON(0, UI_HEART_STEADY);
        printf(",");
        UI_PrintHeartDemonOptionJSON(1, UI_HEART_RISKY);
        printf(",");
        UI_PrintHeartDemonOptionJSON(2, UI_HEART_SUPPRESS);
    }
    printf("]}");
}

static void UI_PrintTribulationOptionJSON(int index, UITribulationChoice choice)
{
    int chance = UI_TribulationSuccessChance(choice == UI_TRIBULATION_PROTECT);
    bool lethal = YOU.XIUWEI >= REFINEMENT;
    const char *key = "cross";
    const char *title = "渡劫";
    const char *desc = lethal ?
        "正面承受雷劫。成功清空劫气并获得高价值收益；失败身死道销。" :
        "正面承受小雷劫。成功清空劫气并获得收益；失败会损失修为与元神。";
    const char *cost = "无额外消耗";
    bool available = true;
    if (choice == UI_TRIBULATION_PROTECT)
    {
        key = "protect";
        title = "护身避劫";
        desc = "消耗修为与灵材布下护身阵，成功率更高，成功后只削减部分劫气。";
        static char cost_buf[96];
        snprintf(cost_buf, sizeof(cost_buf), "修为 -%d，灵材 -%d", UI_TribulationProtectCultivationCost(), UI_TribulationProtectSpiritCost());
        cost = cost_buf;
        available = UI_TribulationProtectAvailable();
    }
    else if (choice == UI_TRIBULATION_SUPPRESS)
    {
        key = "suppress";
        title = "强行压劫";
        desc = lethal ? "暂避死劫，但劫气和雷劫压力继续加深，并损伤元神。" :
                        "暂避小劫，但业力与雷劫压力继续加深，并损伤元神。";
        cost = "元神受损，劫气 +2";
        chance = 100;
    }
    printf("{\"index\":%d,\"mode\":\"", index);
    UI_PrintEscaped(key);
    printf("\",\"title\":\"");
    UI_PrintEscaped(title);
    printf("\",\"desc\":\"");
    UI_PrintEscaped(desc);
    printf("\",\"cost\":\"");
    UI_PrintEscaped(cost);
    printf("\",\"success_chance\":%d,\"available\":%s}", chance, available ? "true" : "false");
}

static void UI_PrintTribulationEventJSON(void)
{
    printf("\"tribulation_event\":{\"active\":%s,\"reason\":\"",
           g_ui_phase == UI_PHASE_TRIBULATION_EVENT ? "true" : "false");
    UI_PrintEscaped(g_ui_phase == UI_PHASE_TRIBULATION_EVENT ? g_ui_pending_tribulation_reason : "");
    printf("\",\"success_chance\":%d,\"options\":[", UI_TribulationSuccessChance(false));
    if (g_ui_phase == UI_PHASE_TRIBULATION_EVENT)
    {
        UI_PrintTribulationOptionJSON(0, UI_TRIBULATION_CROSS);
        printf(",");
        UI_PrintTribulationOptionJSON(1, UI_TRIBULATION_PROTECT);
        printf(",");
        UI_PrintTribulationOptionJSON(2, UI_TRIBULATION_SUPPRESS);
    }
    printf("]}");
}

static void UI_PrintRouteChoiceJSON(void)
{
    printf("\"route_options\":[");
    for (int i = 0; g_ui_phase == UI_PHASE_ROUTE_CHOICE && i < REWARD_CHOICE_COUNT; i++)
    {
        if (i) printf(",");
        const UIRouteTemplate *route = &g_ui_route_options[i].data;
        int next_battle = g_run.battle_index + 1;
        int enemy_realm = UI_ClampRealm((int)YOU.XIUWEI + route->realm_offset +
                                        ((next_battle > 0 && next_battle % 6 == 0) ? 1 : 0));
        UIRouteYield route_yield = UI_RouteYieldFor(route, next_battle);
        int years = UI_RouteYearPreview(route);
        int predicted_age = g_ui_age + years;
        int predicted_pressure = g_ui_tribulation_pressure + (route->risk >= 2 ? 2 : (route->risk <= 0 ? -1 : 0)) + YOU.XIUWEI / 2;
        if (predicted_pressure < 0) predicted_pressure = 0;
        int trib_chance = UI_TribulationChanceForRoute(route);
        printf("{\"index\":%d,\"title\":\"", i);
        UI_PrintEscaped(route->title);
        printf("\",\"desc\":\"");
        UI_PrintEscaped(route->desc);
        printf("\",\"risk\":\"");
        UI_PrintRouteRiskLabel(route);
        printf("\",\"risk_level\":%d,\"enemy_realm\":\"", route->risk);
        UI_PrintEscaped(Realm[enemy_realm]);
        printf("\",\"enemy_realm_id\":%d,\"reward_bias\":\"", enemy_realm);
        UI_PrintEscaped(UI_RouteBiasName(route->reward_bias));
        printf("\",\"hp_cost_pct\":%d,\"cultivation_cost\":%d,\"debt_delta\":%d,\"predicted_debt\":%d,\"special_rule\":\"",
               UI_EffectiveRouteHpCostPct(route), route->cultivation_cost,
               UI_RouteDebtDelta(route), UI_RoutePredictedDebt(route));
        UI_PrintEscaped(UI_RouteSpecialName(route->special_rule));
        printf("\",\"special_hint\":\"");
        UI_PrintEscaped(UI_RouteSpecialHint(route->special_rule));
        printf("\",\"material_preview\":\"灵材 +%d / 药材 +%d\",\"reward_preview\":\"",
               route_yield.spirit, route_yield.herbs);
        UI_PrintEscaped(UI_RouteRewardPreviewText(route));
        printf("\",\"story_hint\":\"");
        UI_PrintEscaped(UI_RouteStoryHint(route));
        printf("\",\"chain_preview\":\"");
        UI_PrintEscaped(UI_RouteChainPreview(route));
        printf("\",\"years_preview\":%d,\"predicted_age\":%d,\"predicted_tribulation\":%d,\"tribulation_preview\":\"雷劫风险%s %d%%\",\"enemy_hint\":\"",
               years, predicted_age, predicted_pressure,
               UI_TribulationRiskLabelForChance(trib_chance), trib_chance);
        UI_PrintEscaped(enemy_realm > (int)YOU.XIUWEI ? "敌人境界更高，建议备好防御或遁逃手段。" : "敌人境界可控，适合推进构筑。");
        printf("\",\"immediate_reward\":");
        UI_PrintRewardJSON(&route->immediate_reward, i);
        printf("}");
    }
    printf("]");
}

static void UI_PrintNearDeathOptionJSON(int index, UINearDeathOptionType type)
{
    printf("{\"index\":%d,\"type\":\"", index);
    switch (type)
    {
    case UI_NEAR_DEATH_ESCAPE:
        printf("escape\",\"title\":\"遁逃\",\"desc\":\"燃尽遁光脱离此战，不获得战利品，也不推进战斗计数。\",\"cost\":\"修为 -%d；若修为不足则清空修为并加重诅咒。\"}",
               UI_EscapeCultivationCost());
        break;
    case UI_NEAR_DEATH_GHOST:
        printf("ghost\",\"title\":\"阴魂不散\",\"desc\":\"舍弃肉身化为鬼魂继续冒险，寻找还阳坛。鬼魂状态下再次死亡将魂飞魄散。\",\"cost\":\"最大元神降至60%%，最大气降至80%%。\"}");
        break;
    case UI_NEAR_DEATH_ACCEPT:
    default:
        printf("accept\",\"title\":\"认命\",\"desc\":\"承认此世道途已尽，结束本次 run。\",\"cost\":\"本次修行结束。\"}");
        break;
    }
}

static void UI_PrintNearDeathJSON(void)
{
    printf("\"near_death\":{\"active\":%s,\"reason\":\"",
           g_ui_phase == UI_PHASE_NEAR_DEATH_CHOICE ? "true" : "false");
    UI_PrintEscaped(g_ui_phase == UI_PHASE_NEAR_DEATH_CHOICE ? g_ui_near_death_reason : "");
    printf("\",\"escape_cost\":%d,\"ghost_chance\":%d,\"can_ghost\":%s,\"options\":[",
           UI_EscapeCultivationCost(), g_ui_near_death_ghost_chance,
           (g_ui_phase == UI_PHASE_NEAR_DEATH_CHOICE && g_ui_near_death_can_ghost) ? "true" : "false");
    if (g_ui_phase == UI_PHASE_NEAR_DEATH_CHOICE)
    {
        UI_PrintNearDeathOptionJSON(0, UI_NEAR_DEATH_ESCAPE);
        int next_index = 1;
        if (g_ui_near_death_can_ghost)
        {
            printf(",");
            UI_PrintNearDeathOptionJSON(next_index++, UI_NEAR_DEATH_GHOST);
        }
        printf(",");
        UI_PrintNearDeathOptionJSON(next_index, UI_NEAR_DEATH_ACCEPT);
    }
    printf("]}");
}

static void UI_PrintCraftingJSON(void)
{
    printf("\"crafting\":{\"recipes\":[");
    int printed = 0;
    int locked_count = 0;
    for (int i = 0; i < Alchemy_RecipeCount(); i++)
    {
        const AlchemyRecipe *recipe = Alchemy_GetRecipe(i);
        if (!recipe)
            continue;
        if (!Alchemy_RecipeKnown(&YOU, recipe))
        {
            locked_count++;
            continue;
        }
        if (printed++) printf(",");
        const Elixir *elixir = &g_elixir_database[recipe->elixir_id];
        printf("{\"index\":%d,\"id\":%d,\"name\":\"", recipe->index, recipe->elixir_id);
        UI_PrintEscaped(elixir->name_chn ? elixir->name_chn : recipe->name);
        printf("\",\"desc\":\"");
        UI_PrintEscaped(elixir->desc_chn ? elixir->desc_chn : "炼制丹药。");
        printf("\",\"cost\":%d,\"required_realm\":\"", recipe->herb_cost);
        UI_PrintEscaped(Realm[recipe->required_realm]);
        printf("\",\"known\":true,\"realm_ready\":%s,\"unlocked\":%s,\"can_brew\":%s,\"custom\":%s}",
               Alchemy_RecipeRealmReady(&YOU, recipe) ? "true" : "false",
               Alchemy_RecipeUnlocked(&YOU, recipe) ? "true" : "false",
               Alchemy_CanBrew(&YOU, recipe) ? "true" : "false",
               elixir->is_custom ? "true" : "false");
    }
    printf("],\"locked_count\":%d,\"unlock_hint\":\"", locked_count);
    if (locked_count > 0)
        UI_PrintEscaped("药谷、宗门、天劫、阴市与魔修事件可领悟更多丹方。");
    else
        UI_PrintEscaped("丹方已尽数掌握。");
    printf("\"}");
}

static void UI_PrintSkillTreeJSON(void)
{
    printf("\"skill_tree\":{\"can_respec\":%s,\"skills\":[", UI_CanManageSkills() ? "true" : "false");
    bool first = true;
    for (int i = 0; i < TOTAL_SKILLS; i++)
    {
        if (!YOU.unlocked_skills[i])
            continue;
        const Skill *skill = &g_skill_database[i];
        if (!Skill_IsDefined((SkillID)i))
            continue;
        if (!first) printf(",");
        first = false;
        bool equipped = skill->action_type >= 0 && skill->action_type < TOTAL_ACTION_TYPES &&
                        YOU.equipped_skills[skill->action_type] == skill->skill_id;
        char effect_summary[192];
        SkillEffectSummary(skill, effect_summary, sizeof(effect_summary));
        char refine_summary[160];
        SkillRefineSummary(&YOU, skill, refine_summary, sizeof(refine_summary));
        int mastery_level = RPG_MasteryLevel(&YOU, skill->skill_id);
        int mastery_xp = YOU.skill_mastery[skill->skill_id];
        int mastery_next = RPG_MasteryNextThreshold(&YOU, skill->skill_id);
        int refine_cost = Skill_RefineCost(&YOU, skill);
        printf("{\"id\":%d,\"name\":\"", skill->skill_id);
        UI_PrintEscaped(skill->name_chn);
        printf("\",\"action_type\":%d,\"action\":\"", skill->action_type);
        UI_PrintEscaped(UI_ActionName(skill->action_type));
        printf("\",\"school\":\"");
        UI_PrintEscaped(SchoolTagName(skill->school_tag));
        printf("\",\"rank\":%d,\"cost\":%d,\"qi_cost\":%d,\"effective_qi_cost\":%d,\"refine_cost\":%d,\"refine\":\"",
               skill->rank, UI_RespecSkillCost(skill->skill_id), skill->cost,
               Skill_EffectiveCostForPlayer(&YOU, skill), refine_cost);
        UI_PrintEscaped(SkillRefineName(Player_SkillRefine(&YOU, skill->skill_id)));
        printf("\",\"refine_summary\":\"");
        UI_PrintEscaped(refine_summary);
        printf("\",\"mastery_level\":%d,\"mastery_xp\":%d,\"mastery_next\":%d,\"equipped\":%s,\"bypassed\":%s,\"custom\":%s,\"desc\":\"",
               mastery_level, mastery_xp, mastery_next,
               equipped ? "true" : "false",
               YOU.bypassed_prereq_skills[skill->skill_id] ? "true" : "false",
               skill->is_custom ? "true" : "false");
        UI_PrintEscaped(skill->prompt_chn);
        printf("\",\"effect_summary\":\"");
        UI_PrintEscaped(effect_summary);
        printf("\"}");
    }
    printf("]}");
}

static void UI_PrintSchoolCyclesJSON(void)
{
    printf("\"school_cycles\":[");
    bool first = true;
    for (int i = 1; i < TOTAL_SCHOOLS; i++)
    {
        int value = YOU.school_cycle[i];
        if (value <= 0)
            continue;
        if (!first) printf(",");
        first = false;
        char summary[96];
        RPG_SchoolCycleSummary(&YOU, (SchoolTag)i, summary, sizeof(summary));
        printf("{\"school\":\"");
        UI_PrintEscaped(SchoolTagName((SchoolTag)i));
        printf("\",\"value\":%d,\"threshold\":%d,\"summary\":\"", value, RPG_SchoolCycleThreshold((SchoolTag)i));
        UI_PrintEscaped(summary);
        printf("\"}");
    }
    printf("]");
}

static void UI_PrintCombatFeedJSON(void)
{
    printf("\"combat_feed\":[");
    for (int i = 0; i < g_ui_feed_count; i++)
    {
        if (i) printf(",");
        printf("{\"side\":\"");
        UI_PrintEscaped(g_ui_feed_side[i]);
        printf("\",\"kind\":\"");
        UI_PrintEscaped(g_ui_feed_kind[i]);
        printf("\",\"text\":\"");
        UI_PrintEscaped(g_ui_feed_text[i]);
        printf("\",\"round\":%d}", g_ui_feed_round[i]);
    }
    printf("]");
}

static void UI_PrintLifeJSON(void)
{
    int lifespan = UI_LifespanForRealm(YOU.XIUWEI);
    int remaining = lifespan - g_ui_age;
    if (remaining < 0)
        remaining = 0;
    printf("\"life\":{\"age\":%d,\"lifespan\":%d,\"years_remaining\":%d,\"last_years_elapsed\":%d,\"life_pressure_label\":\"",
           g_ui_age, lifespan, remaining, g_ui_last_years_elapsed);
    UI_PrintEscaped(UI_LifePressureLabel());
    printf("\"}");
}

static void UI_PrintTribulationJSON(void)
{
    int chance = UI_TribulationChanceForRoute(g_ui_has_next_route ? &g_ui_next_route : NULL);
    printf("\"tribulation\":{\"pressure\":%d,\"label\":\"", g_ui_tribulation_pressure);
    UI_PrintEscaped(UI_TribulationLabel());
    printf("\",\"boss_modifier\":0,\"event_chance\":%d,\"risk_label\":\"", chance);
    UI_PrintEscaped(UI_TribulationRiskLabelForChance(chance));
    printf("\",\"hint\":\"");
    UI_PrintEscaped(UI_TribulationHint());
    printf("\"}");
}

static void UI_PrintChronicleJSON(void)
{
    printf("\"chronicle\":[");
    for (int i = 0; i < g_ui_chronicle_count; i++)
    {
        if (i) printf(",");
        printf("{\"age\":%d,\"realm\":\"", g_ui_chronicle_age[i]);
        UI_PrintEscaped(g_ui_chronicle_realm[i]);
        printf("\",\"kind\":\"");
        UI_PrintEscaped(g_ui_chronicle_kind[i]);
        printf("\",\"text\":\"");
        UI_PrintEscaped(g_ui_chronicle_text[i]);
        printf("\"}");
    }
    printf("]");
}

static void UI_PrintEndingJSON(void)
{
    bool active = g_ui_phase == UI_PHASE_DEFEAT || g_ui_phase == UI_PHASE_VICTORY;
    printf("\"ending\":{\"active\":%s,\"type\":\"", active ? "true" : "false");
    UI_PrintEscaped(active ? g_ui_ending_type : "");
    printf("\",\"title\":\"");
    UI_PrintEscaped(active ? g_ui_ending_title : "");
    printf("\",\"summary\":\"");
    UI_PrintEscaped(active ? g_ui_ending_summary : "");
    printf("\",\"age\":%d,\"realm\":\"", g_ui_age);
    UI_PrintEscaped(Realm[YOU.XIUWEI]);
    printf("\",\"realm_id\":%d,\"battle_index\":%d,\"boss_kills\":%d,\"primary_school\":\"",
           YOU.XIUWEI, g_run.battle_index, g_run.boss_kills);
    BuildProfile profile = Player_GetBuildProfile(&YOU);
    UI_PrintEscaped(SchoolTagName(profile.primary_school));
    printf("\",\"talent_count\":%d,\"soul_state\":\"", YOU.talent_count);
    UI_PrintEscaped(UI_SoulStateName(YOU.soul_state));
    printf("\",\"reborn\":%s}", g_ui_has_reborn ? "true" : "false");
}

static void UI_PrintSnapshot(void)
{
    BuildProfile profile = Player_GetBuildProfile(&YOU);
    printf("##UI_JSON##:{\"phase\":\"%s\",\"round\":%d,\"run\":{\"battle_index\":%d,\"elite_kills\":%d,\"boss_kills\":%d},",
           UI_PhaseName(g_ui_phase), game.round_number, g_run.battle_index, g_run.elite_kills, g_run.boss_kills);
    UI_PrintPlayerJSON("player", &YOU);
    printf(",");
    UI_PrintPlayerJSON("enemy", &CPU);
    printf(",\"materials\":{\"spirit\":%d,\"herbs\":%d}", YOU.spirit_materials, YOU.herb_materials);
    printf(",");
    UI_PrintLifeJSON();
    printf(",");
    UI_PrintTribulationJSON();
    printf(",");
    UI_PrintChronicleJSON();
    printf(",");
    UI_PrintEndingJSON();
    printf(",\"limits\":{\"artifact_slots\":%d,\"artifact_max_level\":%d}", MAX_ARTIFACT_SLOTS, MAX_ARTIFACT_LEVEL);
    printf(",\"route_meta\":{\"calamity_debt\":%d,\"last_route\":\"", g_ui_calamity_debt);
    UI_PrintEscaped(g_ui_has_next_route ? g_ui_next_route.title : "未择路线");
    printf("\",\"route_streak\":%d,\"sword_chain\":%d,\"beast_chain\":%d,\"ghost_market_chain\":%d,"
           "\"herb_valley_chain\":%d,\"demon_ruins_chain\":%d,\"sect_trial_chain\":%d,\"mortal_world_chain\":%d,\"hint\":\"",
           g_ui_route_streak, g_ui_sword_tomb_chain, g_ui_beast_den_chain, g_ui_ghost_market_chain,
           g_ui_herb_valley_chain, g_ui_demon_ruins_chain, g_ui_sect_trial_chain, g_ui_mortal_world_chain);
    if (g_ui_calamity_debt >= 6)
        UI_PrintEscaped("劫气深重：奖励更好，但战间雷劫更容易清算。");
    else if (g_ui_sword_tomb_chain >= 2)
        UI_PrintEscaped("剑冢剑意相连，可能触发剑修试炼。");
    else if (g_ui_beast_den_chain >= 2)
        UI_PrintEscaped("妖兽巢穴气息延伸，血煞兽窟正在成形。");
    else if (g_ui_ghost_market_chain >= 2)
        UI_PrintEscaped("阴市因果加深，还阳机会更近。");
    else if (g_ui_herb_valley_chain >= 2)
        UI_PrintEscaped("药谷丹气入云，丹炉开劫将近。");
    else if (g_ui_demon_ruins_chain >= 2)
        UI_PrintEscaped("魔修旧契回应你的脚步，禁忌事件将近。");
    else if (g_ui_sect_trial_chain >= 2)
        UI_PrintEscaped("宗门残阵层层开启，内门大比将至。");
    else if (g_ui_mortal_world_chain >= 2)
        UI_PrintEscaped("凡尘因缘将尽，故人归尘正在靠近。");
    else
        UI_PrintEscaped("路线选择会积累连续性与风险债。");
    printf("\"}");

    printf(",\"build\":{\"archetype\":\"");
    UI_PrintEscaped(BuildArchetypeName(&profile));
    printf("\",\"primary\":\"");
    UI_PrintEscaped(SchoolTagName(profile.primary_school));
    printf("\",\"primary_count\":%d,\"secondary\":\"", profile.primary_count);
    UI_PrintEscaped(SchoolTagName(profile.secondary_school));
    printf("\",\"secondary_count\":%d,\"unlocked_skills\":%d,\"equipped_skills\":%d,\"synergy_label\":\"",
           profile.secondary_count, profile.unlocked_skill_count, profile.equipped_skill_count);
    UI_PrintEscaped(profile.synergy_label);
    printf("\",\"synergy_bonus\":\"");
    UI_PrintEscaped(profile.synergy_bonus);
    printf("\",");
    UI_PrintSchoolCyclesJSON();
    printf("}");

    printf(",\"actions\":[");
    bool first = true;
    for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
    {
        const Skill *skill = &YOU.learned_skills[i];
        if (skill->skill_id == SKILL_ID_NONE)
            continue;
        if (!first) printf(",");
        first = false;
        char effect_summary[192];
        SkillEffectSummary(skill, effect_summary, sizeof(effect_summary));
        char refine_summary[160];
        SkillRefineSummary(&YOU, skill, refine_summary, sizeof(refine_summary));
        int mastery_level = RPG_MasteryLevel(&YOU, skill->skill_id);
        int mastery_xp = skill->skill_id > SKILL_ID_NONE && skill->skill_id < TOTAL_SKILLS ? YOU.skill_mastery[skill->skill_id] : 0;
        int mastery_next = RPG_MasteryNextThreshold(&YOU, skill->skill_id);
        printf("{\"slot\":%d,\"action_type\":%d,\"skill_id\":%d,\"name\":\"", i, skill->action_type, skill->skill_id);
        UI_PrintEscaped(skill->name_chn);
        printf("\",\"cost\":%d,\"base_cost\":%d,\"rank\":%d,\"school\":\"", Skill_EffectiveCostForPlayer(&YOU, skill), skill->cost, skill->rank);
        UI_PrintEscaped(SchoolTagName(skill->school_tag));
        printf("\",\"available\":%s,\"custom\":%s,\"mastery_level\":%d,\"mastery_xp\":%d,\"mastery_next\":%d,\"effect_summary\":\"", YOU.QI >= Skill_EffectiveCostForPlayer(&YOU, skill) ? "true" : "false",
               skill->is_custom ? "true" : "false", mastery_level, mastery_xp, mastery_next);
        UI_PrintEscaped(effect_summary);
        printf("\",\"refine\":\"");
        UI_PrintEscaped(SkillRefineName(Player_SkillRefine(&YOU, skill->skill_id)));
        printf("\",\"refine_summary\":\"");
        UI_PrintEscaped(refine_summary);
        printf("\"}");
    }
    printf("]");

    printf(",\"artifacts\":[");
    for (int i = 0; i < YOU.artifact_count; i++)
    {
        ArtifactID id = YOU.artifacts[i];
        char resonance[96];
        Artifact_ResonanceSummary(&YOU, i, resonance, sizeof(resonance));
        int level = Artifact_LevelAtSlot(&YOU, i);
        int upgrade_cost = Artifact_UpgradeCost(level);
        if (i) printf(",");
        printf("{\"slot\":%d,\"id\":%d,\"level\":%d,\"max_level\":%d,\"slot_limit\":%d,\"upgrade_cost\":%d,\"can_upgrade\":%s,\"resonance_active\":%s,\"resonance\":\"",
               i, id, level, MAX_ARTIFACT_LEVEL, MAX_ARTIFACT_SLOTS, upgrade_cost,
               (upgrade_cost > 0 && YOU.spirit_materials >= upgrade_cost) ? "true" : "false",
               Artifact_ResonanceActive(&YOU, i) ? "true" : "false");
        UI_PrintEscaped(resonance);
        printf("\",\"school\":\"");
        UI_PrintEscaped(SchoolTagName(g_artifact_database[id].reward_school_bias));
        printf("\",\"name\":\"");
        UI_PrintEscaped(g_artifact_database[id].name_chn);
        printf("\",\"desc\":\"");
        UI_PrintEscaped(g_artifact_database[id].desc_chn);
        printf("\",\"custom\":%s}", g_artifact_database[id].is_custom ? "true" : "false");
    }
    printf("],\"elixirs\":[");
    for (int i = 0; i < YOU.elixir_count; i++)
    {
        ElixirID id = YOU.elixirs[i];
        if (i) printf(",");
        printf("{\"slot\":%d,\"id\":%d,\"name\":\"", i, id);
        UI_PrintEscaped(g_elixir_database[id].name_chn);
        printf("\",\"desc\":\"");
        UI_PrintEscaped(g_elixir_database[id].desc_chn);
        printf("\",\"custom\":%s}", g_elixir_database[id].is_custom ? "true" : "false");
    }
    printf("]");

    printf(",\"talents\":[");
    bool first_talent = true;
    for (int i = 0; i < TOTAL_TALENTS; i++)
    {
        if (!YOU.talents[i])
            continue;
        if (!first_talent) printf(",");
        first_talent = false;
        UI_PrintTalentJSON((TalentID)i, i);
    }
    printf("]");

    printf(",\"rewards\":[");
    for (int i = 0; g_ui_phase == UI_PHASE_REWARD_CHOICE && i < REWARD_CHOICE_COUNT; i++)
    {
        if (i) printf(",");
        UI_PrintRewardJSON(&g_ui_pending_rewards[i], i);
    }
    printf("]");

    printf(",\"encounter\":{\"title\":\"");
    UI_PrintEscaped(g_ui_phase == UI_PHASE_ENCOUNTER ? g_ui_last_encounter_title : "");
    printf("\",\"text\":\"");
    UI_PrintEscaped(g_ui_phase == UI_PHASE_ENCOUNTER ? g_ui_last_encounter_text : "");
    printf("\",\"penalty\":%d,\"reward\":", g_ui_phase == UI_PHASE_ENCOUNTER ? g_ui_last_encounter_penalty : 0);
    if (g_ui_phase == UI_PHASE_ENCOUNTER)
        UI_PrintRewardJSON(&g_ui_last_encounter_reward, 0);
    else
        printf("null");
    printf("}");

    printf(",\"logs\":[");
    for (int i = 0; i < g_ui_log_count; i++)
    {
        if (i) printf(",");
        printf("\"");
        UI_PrintEscaped(g_ui_recent_logs[i]);
        printf("\"");
    }
    printf("],");
    UI_PrintCombatFeedJSON();
    printf(",\"boss_phase\":{\"key\":\"");
    UI_PrintEscaped(UI_BossPhaseKey(g_ui_boss_phase));
    printf("\",\"label\":\"");
    UI_PrintEscaped(UI_BossPhaseName(g_ui_boss_phase));
    printf("\",\"cycle\":%d,\"archetype\":\"", g_ui_boss_cycle);
    UI_PrintEscaped(UI_BossArchetypeKey(g_ui_boss_archetype));
    printf("\",\"name\":\"");
    UI_PrintEscaped(UI_BossArchetypeName(g_ui_boss_archetype));
    printf("\",\"desc\":\"");
    UI_PrintEscaped(UI_BossArchetypeDesc(g_ui_boss_archetype));
    printf("\",\"counter_hint\":\"");
    UI_PrintEscaped(UI_BossCounterHint(g_ui_boss_archetype));
    printf("\",\"reward_hint\":\"");
    UI_PrintEscaped(UI_BossRewardHint(g_ui_boss_archetype));
    printf("\"},\"last_elixir_slot\":%d,", g_ui_last_elixir_slot);
    UI_PrintReplacementJSON();
    printf(",");
    UI_PrintBreakthroughJSON();
    printf(",");
    UI_PrintHeartDemonJSON();
    printf(",");
    UI_PrintTribulationEventJSON();
    printf(",");
    printf("\"talent_options\":[");
    bool first_talent_option = true;
    for (int i = 0; g_ui_phase == UI_PHASE_TALENT_CHOICE && i < REWARD_CHOICE_COUNT; i++)
    {
        if (g_ui_talent_options[i] == TALENT_NONE)
            continue;
        if (!first_talent_option) printf(",");
        first_talent_option = false;
        UI_PrintTalentJSON(g_ui_talent_options[i], i);
    }
    printf("],");
    UI_PrintRouteChoiceJSON();
    printf(",");
    UI_PrintNextBattleJSON();
    printf(",");
    UI_PrintNearDeathJSON();
    printf(",");
    UI_PrintSkillTreeJSON();
    printf(",");
    UI_PrintCraftingJSON();
    printf("}\n");
    fflush(stdout);
}

static void UI_ChooseCPUAction(void)
{
    AIThreatProfile threat = AI_ReadThreatProfile(&YOU);
    if (UI_IsBossBattle())
    {
        UI_ChooseBossAction();
        return;
    }

    g_ui_boss_phase = UI_BOSS_NONE;
    if (g_config.run_mode && game.opponent_type != 0)
        CPU_logic_V2A_Tuned(&CPU, &YOU, 0);
    else
        CPU_logic_V0_Random(&CPU, &YOU);

    if ((UI_IsEliteBattle() || threat.sword_pressure) && YOU.QI >= 4)
    {
        if (can_perform_action(&CPU, ACTION_TYPE_COUNTER))
            CPU.current_action_type = ACTION_TYPE_COUNTER;
        else if (can_perform_action(&CPU, ACTION_TYPE_DEFEND))
            CPU.current_action_type = ACTION_TYPE_DEFEND;
    }
    if (threat.qi_pressure && YOU.QI > CPU.QI + 3 && can_perform_action(&CPU, ACTION_TYPE_BOOST))
        CPU.current_action_type = ACTION_TYPE_BOOST;
    if (threat.defensive_shell && CPU.current_action_type == ACTION_TYPE_MELEE && can_perform_action(&CPU, ACTION_TYPE_SMITE))
        CPU.current_action_type = ACTION_TYPE_SMITE;

    switch (CPU.current_action_type)
    {
    case ACTION_TYPE_GAIN_QI:
        UI_SetEnemyIntent("气息内敛", "敌人倾向积蓄灵气，为后续招式做准备。", UI_IsEliteBattle() ? 72 : 58);
        break;
    case ACTION_TYPE_DEFEND:
    case ACTION_TYPE_COUNTER:
        UI_SetEnemyIntent("守势已成", "敌人可能防御或反击，贸然进攻会被化解。", UI_IsEliteBattle() ? 76 : 62);
        break;
    case ACTION_TYPE_BOOST:
        UI_SetEnemyIntent("法意扰动", "敌人正在酝酿增益或干扰手段。", UI_IsEliteBattle() ? 74 : 60);
        break;
    case ACTION_TYPE_SMITE:
    case ACTION_TYPE_BURST:
    case ACTION_TYPE_TERMINATE:
        UI_SetEnemyIntent("杀意凝聚", "敌人可能打出高威胁招式，注意防御窗口。", UI_IsEliteBattle() ? 80 : 65);
        break;
    default:
        UI_SetEnemyIntent("试探进攻", "敌人倾向以低成本招式试探。", UI_IsEliteBattle() ? 68 : 52);
        break;
    }
}

static void UI_StartBattle(void)
{
    g_ui_replacement_type = UI_REPLACE_NONE;
    g_ui_replacement_return_phase = UI_PHASE_RUN_START;
    g_ui_replacement_slot_count = 0;
    g_ui_replacement_action_type = ACTION_TYPE_NONE;
    Game_init(&YOU, &CPU, &game);
    int enemy_realm = UI_NextEnemyRealm();
    CPU.XIUWEI = (XIUWEI)enemy_realm;
    CPU.QI = 0;
    CPU.HP = Player_MaxHP(&CPU);
    CPU.ATK = Yuan[CPU.XIUWEI];
    CPU.YUAN = Yuan[CPU.XIUWEI];
    CPU.gain_bonus = CPU.XIUWEI + 1;
    Update_Player_Skills(&CPU);
    Start_new_round(&game);
    g_ui_phase = UI_PHASE_BATTLE_ACTION;
    if (UI_IsBossBattle())
    {
        g_ui_boss_archetype = UI_PredictBossArchetype();
        char boss_log[192];
        snprintf(boss_log, sizeof(boss_log), "%s现身：%s", UI_BossArchetypeName(g_ui_boss_archetype), UI_BossArchetypeDesc(g_ui_boss_archetype));
        UI_AddCombatFeed("system", "boss", boss_log);
        UI_AddChronicle("boss", boss_log);
    }
    else
    {
        g_ui_boss_archetype = UI_BOSS_ARCHETYPE_NONE;
        g_ui_boss_cycle = 0;
    }
    UI_AddCombatFeed("system", UI_IsBossBattle() ? "boss" : (UI_IsEliteBattle() ? "elite" : "info"),
                     UI_IsBossBattle() ? UI_BossCounterHint(g_ui_boss_archetype) :
                     (UI_IsEliteBattle() ? "精英对手现身，战斗开始。" : "新的对手出现了，战斗开始。"));
}

static void UI_StartRun(void)
{
    g_config.run_mode = 1;
    g_config.auto_reward = 0;
    Initialize_RunState();
    g_log_count = 0;
    total_games_played = 0;
    player_wins = 0;
    g_ui_log_count = 0;
    g_ui_feed_count = 0;
    g_ui_boss_phase = UI_BOSS_NONE;
    g_ui_boss_archetype = UI_BOSS_ARCHETYPE_NONE;
    g_ui_boss_cycle = 0;
    UI_SetEnemyIntent("试探", "敌人正在观察你的气机。", 40);
    g_ui_last_elixir_slot = -1;
    g_ui_pending_replacement_reward = (RewardChoice){REWARD_CULTIVATION, RARITY_COMMON, 0, 0, false};
    g_ui_replacement_type = UI_REPLACE_NONE;
    g_ui_replacement_return_phase = UI_PHASE_RUN_START;
    g_ui_replacement_slot_count = 0;
    g_ui_replacement_action_type = ACTION_TYPE_NONE;
    g_ui_has_next_route = false;
    g_ui_last_route_template_id = -1;
    g_ui_route_streak = 0;
    g_ui_sword_tomb_chain = 0;
    g_ui_beast_den_chain = 0;
    g_ui_ghost_market_chain = 0;
    g_ui_herb_valley_chain = 0;
    g_ui_demon_ruins_chain = 0;
    g_ui_sect_trial_chain = 0;
    g_ui_mortal_world_chain = 0;
    g_ui_calamity_debt = 0;
    g_ui_age = 12 + (int)(Run_Rand() % 27);
    g_ui_last_years_elapsed = 0;
    g_ui_tribulation_pressure = 0;
    g_ui_has_reborn = false;
    g_ui_ending_type[0] = '\0';
    g_ui_ending_title[0] = '\0';
    g_ui_ending_summary[0] = '\0';
    g_ui_chronicle_count = 0;
    g_ui_breakthrough_offered = false;
    g_ui_near_death_can_ghost = false;
    g_ui_near_death_ghost_chance = 0;
    g_ui_near_death_reason = "";
    g_ui_next_route = UI_ROUTE_TEMPLATES[0];
    g_ui_last_encounter_reward = (RewardChoice){REWARD_CULTIVATION, RARITY_COMMON, 0, 0, false};
    g_ui_last_encounter_penalty = 0;
    g_ui_last_encounter_title = "";
    g_ui_last_encounter_text = "";
    UI_AddCombatFeed("system", "info", "踏入修行之路。");
    char opening[192];
    snprintf(opening, sizeof(opening), "%d 岁，凡身入道，寿元不过七十。", g_ui_age);
    UI_AddChronicle("start", opening);
    UI_StartBattle();
}

static int UI_SetPlayerAction(int action_type)
{
    if (action_type < 0 || action_type >= TOTAL_ACTION_TYPES)
        return 0;
    const Skill *skill = &YOU.learned_skills[action_type];
    int effective_cost = Skill_EffectiveCostForPlayer(&YOU, skill);
    if (skill->skill_id == SKILL_ID_NONE || YOU.QI < effective_cost)
        return 0;
    YOU.current_action_type = skill->action_type;
    YOU.QI_change -= effective_cost;
    char log_message[256];
    snprintf(log_message, sizeof(log_message), skill->prompt_chn, YOU.name);
    UI_AddCombatFeed("player", "action", log_message);
    return 1;
}

static void UI_GenerateRewards(void)
{
    g_run.battle_index++;
    if (g_run.battle_index % 6 == 0) g_run.boss_kills++;
    else if (g_run.battle_index % 3 == 0) g_run.elite_kills++;
    if (g_run.battle_index % 6 == 0)
    {
        g_ui_tribulation_pressure += 1 + g_ui_calamity_debt / 4;
        UI_UnlockAlchemyRecipe(ELIXIR_BREAKTHROUGH, "Boss 战后，");
    }
    if (g_ui_has_next_route && g_ui_next_route.reward_bias == UI_ROUTE_REWARD_THUNDER)
        g_ui_tribulation_pressure += 1;
    g_ui_breakthrough_offered = false;
    int battle_cultivation = UI_BattleCultivationGain();
    YOU.cultivation += battle_cultivation;
    char insight_log[128];
    snprintf(insight_log, sizeof(insight_log), "搏杀感悟：获得 %d 点修为。", battle_cultivation);
    UI_AddLog(insight_log);
    int understanding_gain = 1;
    if (g_run.battle_index % 3 == 0)
        understanding_gain++;
    if (g_ui_has_next_route && g_ui_next_route.risk >= 2)
        understanding_gain++;
    if (RPG_AddMinorUnderstanding(&YOU, understanding_gain))
        UI_AddLog("小境界有所突破，元神与灵气上限更稳。");
    else
    {
        char understanding_log[128];
        snprintf(understanding_log, sizeof(understanding_log), "修为理解 +%d。", understanding_gain);
        UI_AddLog(understanding_log);
    }
    UIRouteYield route_yield = UI_RouteYieldFor(g_ui_has_next_route ? &g_ui_next_route : NULL, g_run.battle_index);
    int spirit_gain = route_yield.spirit;
    int herb_gain = route_yield.herbs;
    YOU.spirit_materials += spirit_gain;
    YOU.herb_materials += herb_gain;
    char material_log[128];
    snprintf(material_log, sizeof(material_log), "战后搜罗：灵材 +%d，药材 +%d。", spirit_gain, herb_gain);
    UI_AddLog(material_log);
    if (Player_HasTalent(&YOU, TALENT_BLOOD_REBIRTH))
    {
        int heal = Player_MaxHP(&YOU) / 12 + Yuan[YOU.XIUWEI];
        YOU.HP += heal;
        Player_ClampVitals(&YOU);
        UI_AddLog("血煞转生汲取余威，元神稍复。");
    }
    Generate_RewardChoices(&YOU, g_ui_pending_rewards);
    BuildProfile profile = Player_GetBuildProfile(&YOU);
    if (g_run.battle_index > 0 && g_run.battle_index % 6 == 0)
    {
        char boss_chronicle[192];
        snprintf(boss_chronicle, sizeof(boss_chronicle), "击败%s，%s", UI_BossArchetypeName(g_ui_boss_archetype), UI_BossRewardHint(g_ui_boss_archetype));
        UI_AddChronicle("boss", boss_chronicle);
        switch (g_ui_boss_archetype)
        {
        case UI_BOSS_ARCHETYPE_SWORD_GUARDIAN:
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_EPIC, PickRewardSkillForSchool(&YOU, RARITY_EPIC, true, SCHOOL_SWORD), 1, true};
            break;
        case UI_BOSS_ARCHETYPE_BLOOD_EMBRYO:
        {
            SkillID skill = PickRewardSkillForSchool(&YOU, RARITY_EPIC, true, SCHOOL_BLOOD);
            if (skill == SKILL_ID_NONE)
                skill = PickRewardSkillForSchool(&YOU, RARITY_EPIC, true, SCHOOL_DARK);
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_EPIC, skill, 1, true};
            YOU.spirit_materials += 4;
            break;
        }
        case UI_BOSS_ARCHETYPE_THUNDER_AVATAR:
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_EPIC, PickRewardSkillForSchool(&YOU, RARITY_EPIC, true, SCHOOL_THUNDER), 1, true};
            UI_UnlockAlchemyRecipe(ELIXIR_MERIDIAN_GUARD, "雷劫化身败散后，");
            UI_UnlockAlchemyRecipe(ELIXIR_BREAKTHROUGH, "雷劫化身败散后，");
            break;
        case UI_BOSS_ARCHETYPE_SECT_ENFORCER:
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_EPIC, PickRewardSkillForSchool(&YOU, RARITY_EPIC, true, SCHOOL_DEFENSE), 1, true};
            UI_UnlockAlchemyRecipe(ELIXIR_FOCUS_INSIGHT, "宗门执法败散后，");
            break;
        case UI_BOSS_ARCHETYPE_FORMLESS_HEART:
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_EPIC, PickRewardSkillForSchool(&YOU, RARITY_EPIC, true, profile.primary_school), 1, true};
            break;
        case UI_BOSS_ARCHETYPE_ASCENSION_ECHO:
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_LEGENDARY, PickRewardSkill(&YOU, RARITY_LEGENDARY, true), 1, true};
            RPG_AddMinorUnderstanding(&YOU, 2);
            break;
        case UI_BOSS_ARCHETYPE_NONE:
        default:
            break;
        }
        if (g_ui_pending_rewards[0].type == REWARD_SKILL && g_ui_pending_rewards[0].id == SKILL_ID_NONE)
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_CULTIVATION, RARITY_EPIC, 0, max_QI[YOU.XIUWEI], false};
    }
    int insight_chance = 8 + YOU.XIUWEI * 2;
    if (profile.primary_count >= 3)
        insight_chance += 7;
    if (Player_HasTalent(&YOU, TALENT_CAVE_INSIGHT))
        insight_chance += 10;
    if (Player_HasTalent(&YOU, TALENT_SWORD_HEART) && profile.primary_school == SCHOOL_SWORD)
        insight_chance += 6;
    if ((int)(Run_Rand() % 100) < insight_chance)
        UI_TryInsightSkill(RARITY_COMMON, false, "战后复盘招式，偶有所悟。");
    if (g_ui_has_next_route)
    {
        switch (g_ui_next_route.reward_bias)
        {
        case UI_ROUTE_REWARD_CULTIVATION:
            g_ui_pending_rewards[2] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[YOU.XIUWEI] / 2, false};
            break;
        case UI_ROUTE_REWARD_ELIXIR:
            g_ui_pending_rewards[2] = (RewardChoice){REWARD_ELIXIR, RARITY_RARE, PickRewardElixirForTheme(RARITY_RARE, REWARD_THEME_ELIXIR), 2, false};
            break;
        case UI_ROUTE_REWARD_ARTIFACT:
        {
            ArtifactID artifact = PickRewardArtifactForSchool(&YOU, RARITY_RARE, profile.primary_school);
            if (artifact == ARTIFACT_NONE)
                artifact = PickRewardArtifactForPlayer(&YOU, RARITY_RARE);
            g_ui_pending_rewards[1] = (RewardChoice){REWARD_ARTIFACT, RARITY_RARE, artifact, 1, false};
            if (g_ui_pending_rewards[1].id == ARTIFACT_NONE)
                g_ui_pending_rewards[1] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[YOU.XIUWEI] / 2, false};
            break;
        }
        case UI_ROUTE_REWARD_SKILL:
        {
            RewardRarity rarity = Player_HasTalent(&YOU, TALENT_CAVE_INSIGHT) ? RARITY_EPIC : RARITY_RARE;
            SkillID skill = PickRewardSkillForSchool(&YOU, rarity, true, profile.primary_school);
            if (skill == SKILL_ID_NONE)
                skill = PickRewardSkill(&YOU, rarity, true);
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, rarity, skill, 1, true};
            if (g_ui_pending_rewards[0].id == SKILL_ID_NONE)
                g_ui_pending_rewards[0] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[YOU.XIUWEI] / 2, false};
            break;
        }
        case UI_ROUTE_REWARD_THUNDER:
        {
            SkillID thunder_skill = PickRewardSkillForSchool(&YOU, RARITY_RARE, true, SCHOOL_THUNDER);
            if (thunder_skill != SKILL_ID_NONE)
                g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_RARE, thunder_skill, 1, true};
            ArtifactID artifact = PickRewardArtifactForSchool(&YOU, RARITY_RARE, SCHOOL_THUNDER);
            if (artifact == ARTIFACT_NONE && !Player_HasArtifact(&YOU, ARTIFACT_THUNDER_WOOD_SWORD))
                artifact = ARTIFACT_THUNDER_WOOD_SWORD;
            if (artifact == ARTIFACT_NONE)
                artifact = PickRewardArtifactForPlayer(&YOU, RARITY_RARE);
            g_ui_pending_rewards[1] = (RewardChoice){REWARD_ARTIFACT, RARITY_RARE, artifact, 1, false};
            g_ui_pending_rewards[2] = (RewardChoice){REWARD_ELIXIR, RARITY_RARE, PickRewardElixirForTheme(RARITY_RARE, REWARD_THEME_THUNDER), 1, false};
            if (g_ui_pending_rewards[1].id == ARTIFACT_NONE)
                g_ui_pending_rewards[1] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[YOU.XIUWEI] / 2, false};
            break;
        }
        case UI_ROUTE_REWARD_SWORD:
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_RARE, PickRewardSkillForSchool(&YOU, RARITY_RARE, true, SCHOOL_SWORD), 1, true};
            if (Player_HasTalent(&YOU, TALENT_SWORD_HEART))
            {
                for (int i = 1; i < REWARD_CHOICE_COUNT; i++)
                {
                    if (g_ui_pending_rewards[i].type != REWARD_SKILL)
                    {
                        g_ui_pending_rewards[i] = (RewardChoice){REWARD_SKILL, RARITY_RARE, PickRewardSkill(&YOU, RARITY_RARE, true), 1, true};
                        break;
                    }
                }
            }
            if (g_ui_pending_rewards[0].id == SKILL_ID_NONE)
                g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_RARE, PickRewardSkill(&YOU, RARITY_RARE, true), 1, true};
            if (g_ui_pending_rewards[0].id == SKILL_ID_NONE)
                g_ui_pending_rewards[0] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[YOU.XIUWEI] / 2, false};
            break;
        case UI_ROUTE_REWARD_GHOST:
        {
            SkillID dark_skill = PickRewardSkillForSchool(&YOU, RARITY_RARE, true, SCHOOL_DARK);
            if (dark_skill == SKILL_ID_NONE)
                dark_skill = PickRewardSkillForSchool(&YOU, RARITY_RARE, true, SCHOOL_BLOOD);
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_RARE, dark_skill, 1, true};
            ArtifactID artifact = PickRewardArtifactForSchool(&YOU, RARITY_RARE, SCHOOL_DARK);
            if (artifact != ARTIFACT_NONE)
                g_ui_pending_rewards[1] = (RewardChoice){REWARD_ARTIFACT, RARITY_RARE, artifact, 1, false};
            g_ui_pending_rewards[2] = (RewardChoice){REWARD_ELIXIR, RARITY_RARE, PickRewardElixirForTheme(RARITY_RARE, REWARD_THEME_GHOST), 1, false};
            if (g_ui_pending_rewards[0].id == SKILL_ID_NONE)
                g_ui_pending_rewards[0] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[YOU.XIUWEI] / 2, false};
            break;
        }
        case UI_ROUTE_REWARD_BALANCED:
        default:
            break;
        }
        if (g_ui_next_route.special_rule == UI_ROUTE_SPECIAL_SWORD_TRIAL)
        {
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_EPIC, PickRewardSkill(&YOU, RARITY_EPIC, true), 1, true};
            if (g_ui_pending_rewards[0].id == SKILL_ID_NONE)
                g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_RARE, PickRewardSkill(&YOU, RARITY_RARE, true), 1, true};
            if (g_ui_pending_rewards[0].id == SKILL_ID_NONE)
                g_ui_pending_rewards[0] = (RewardChoice){REWARD_CULTIVATION, RARITY_EPIC, 0, max_QI[YOU.XIUWEI], false};
        }
        else if (g_ui_next_route.special_rule == UI_ROUTE_SPECIAL_BLOOD_BEAST_DEN)
        {
            ArtifactID artifact = PickRewardArtifactForSchool(&YOU, RARITY_EPIC, SCHOOL_BLOOD);
            if (artifact == ARTIFACT_NONE)
                artifact = PickRewardArtifactForPlayer(&YOU, RARITY_EPIC);
            g_ui_pending_rewards[1] = (RewardChoice){REWARD_ARTIFACT, RARITY_EPIC, artifact, 1, false};
            if (g_ui_pending_rewards[1].id == ARTIFACT_NONE)
                g_ui_pending_rewards[1] = (RewardChoice){REWARD_CULTIVATION, RARITY_EPIC, 0, max_QI[YOU.XIUWEI], false};
        }
        else if (g_ui_next_route.special_rule == UI_ROUTE_SPECIAL_ALCHEMY_CAULDRON)
        {
            g_ui_pending_rewards[1] = (RewardChoice){REWARD_ELIXIR, RARITY_EPIC, PickRewardElixirForTheme(RARITY_EPIC, REWARD_THEME_ELIXIR), 2, false};
            g_ui_pending_rewards[2] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[YOU.XIUWEI], false};
        }
        else if (g_ui_next_route.special_rule == UI_ROUTE_SPECIAL_DEMON_CONTRACT)
        {
            SkillID skill = PickRewardSkillForSchool(&YOU, RARITY_EPIC, true, SCHOOL_DARK);
            if (skill == SKILL_ID_NONE)
                skill = PickRewardSkillForSchool(&YOU, RARITY_EPIC, true, SCHOOL_BLOOD);
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_EPIC, skill, 1, true};
            if (g_ui_pending_rewards[0].id == SKILL_ID_NONE)
                g_ui_pending_rewards[0] = (RewardChoice){REWARD_CULTIVATION, RARITY_EPIC, 0, max_QI[YOU.XIUWEI] * 3 / 2, false};
            g_ui_pending_rewards[2] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[YOU.XIUWEI], false};
        }
        else if (g_ui_next_route.special_rule == UI_ROUTE_SPECIAL_SECT_TOURNAMENT)
        {
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_EPIC, PickRewardSkill(&YOU, RARITY_EPIC, true), 1, true};
            if (g_ui_pending_rewards[0].id == SKILL_ID_NONE)
                g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_RARE, PickRewardSkill(&YOU, RARITY_RARE, true), 1, true};
            if (g_ui_pending_rewards[0].id == SKILL_ID_NONE)
                g_ui_pending_rewards[0] = (RewardChoice){REWARD_CULTIVATION, RARITY_EPIC, 0, max_QI[YOU.XIUWEI], false};
        }
        else if (g_ui_next_route.special_rule == UI_ROUTE_SPECIAL_MORTAL_FAREWELL)
        {
            g_ui_pending_rewards[1] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[YOU.XIUWEI], false};
            g_ui_pending_rewards[2] = (RewardChoice){REWARD_ELIXIR, RARITY_RARE, PickRewardElixir(RARITY_RARE), 1, false};
        }
    }
    if (g_ui_calamity_debt >= 4)
    {
        for (int i = 0; i < REWARD_CHOICE_COUNT; i++)
        {
            if (g_ui_pending_rewards[i].rarity < RARITY_RARE)
                g_ui_pending_rewards[i].rarity = RARITY_RARE;
        }
    }
    if (g_ui_calamity_debt >= 8)
    {
        for (int i = 0; i < REWARD_CHOICE_COUNT; i++)
        {
            if (g_ui_pending_rewards[i].rarity < RARITY_EPIC)
                g_ui_pending_rewards[i].rarity = RARITY_EPIC;
        }
    }
    if (g_ui_has_next_route && g_ui_next_route.risk >= 2)
    {
        for (int i = 0; i < REWARD_CHOICE_COUNT; i++)
        {
            if (g_ui_pending_rewards[i].rarity < RARITY_RARE)
                g_ui_pending_rewards[i].rarity = RARITY_RARE;
        }
        if (g_ui_pending_rewards[2].type == REWARD_CULTIVATION)
            g_ui_pending_rewards[2].amount += max_QI[YOU.XIUWEI] / 3;
    }
    if (g_ui_has_next_route && g_ui_next_route.risk <= 0)
    {
        RPG_AddMinorUnderstanding(&YOU, 1);
        if (g_ui_tribulation_pressure > 0 && g_ui_next_route.reward_bias == UI_ROUTE_REWARD_CULTIVATION)
            g_ui_tribulation_pressure--;
        UI_AddLog("安稳路线沉淀心境，修为理解 +1。");
    }
    if (g_run.battle_index > 0 && g_run.battle_index % 3 == 0)
    {
        for (int i = 0; i < REWARD_CHOICE_COUNT; i++)
        {
            if (g_ui_pending_rewards[i].type != REWARD_SKILL)
            {
                g_ui_pending_rewards[i] = (RewardChoice){REWARD_SKILL, RARITY_RARE, PickRewardSkill(&YOU, RARITY_RARE, false), 1, false};
                if (g_ui_pending_rewards[i].id == SKILL_ID_NONE)
                    g_ui_pending_rewards[i] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[YOU.XIUWEI] / 2, false};
                break;
            }
        }
    }
    g_ui_phase = UI_PHASE_REWARD_CHOICE;
}

static void UI_GenerateRouteOptions(void)
{
    int scale = Yuan[YOU.XIUWEI] > 0 ? Yuan[YOU.XIUWEI] : 1;
    if (YOU.soul_state == SOUL_GHOST)
    {
        for (int i = 0; i < REWARD_CHOICE_COUNT; i++)
        {
            int pick = i;
            if (pick >= UI_GHOST_ROUTE_TEMPLATE_COUNT)
                pick = UI_GHOST_ROUTE_TEMPLATE_COUNT - 1;
            g_ui_route_options[i].template_id = pick;
            g_ui_route_options[i].data = UI_GHOST_ROUTE_TEMPLATES[pick];
            if (g_ui_route_options[i].data.immediate_reward.type == REWARD_CULTIVATION)
                g_ui_route_options[i].data.immediate_reward.amount *= scale;
        }
        if (g_ui_ghost_market_chain >= 2)
        {
            g_ui_route_options[0].template_id = 1002;
            g_ui_route_options[0].data = UI_SPECIAL_ROUTE_GHOST_MARKET;
            g_ui_route_options[0].data.immediate_reward.amount *= scale;
        }
        g_ui_phase = UI_PHASE_ROUTE_CHOICE;
        return;
    }

    bool used[16] = {false};
    for (int i = 0; i < REWARD_CHOICE_COUNT; i++)
    {
        int pick = (int)(Run_Rand() % UI_ROUTE_TEMPLATE_COUNT);
        for (int guard = 0; guard < UI_ROUTE_TEMPLATE_COUNT && used[pick]; guard++)
            pick = (pick + 1) % UI_ROUTE_TEMPLATE_COUNT;
        used[pick] = true;
        g_ui_route_options[i].template_id = pick;
        g_ui_route_options[i].data = UI_ROUTE_TEMPLATES[pick];
        if (g_ui_route_options[i].data.immediate_reward.type == REWARD_CULTIVATION)
            g_ui_route_options[i].data.immediate_reward.amount *= scale;
        if (g_ui_route_options[i].data.immediate_reward.type == REWARD_ELIXIR &&
            Player_HasTalent(&YOU, TALENT_ALCHEMY_RESIDUE))
            g_ui_route_options[i].data.immediate_reward.amount += 1;
        g_ui_route_options[i].data.cultivation_cost *= scale;
    }
    if (g_ui_sword_tomb_chain >= 2)
    {
        g_ui_route_options[0].template_id = 1000;
        g_ui_route_options[0].data = UI_SPECIAL_ROUTE_SWORD_TRIAL;
        g_ui_route_options[0].data.immediate_reward.amount *= scale;
    }
    else if (g_ui_beast_den_chain >= 2)
    {
        g_ui_route_options[0].template_id = 1001;
        g_ui_route_options[0].data = UI_SPECIAL_ROUTE_BLOOD_BEAST;
        g_ui_route_options[0].data.immediate_reward.amount *= scale;
    }
    else if (g_ui_herb_valley_chain >= 2)
    {
        g_ui_route_options[0].template_id = 1003;
        g_ui_route_options[0].data = UI_SPECIAL_ROUTE_ALCHEMY_CAULDRON;
        g_ui_route_options[0].data.immediate_reward.amount *= scale;
    }
    else if (g_ui_demon_ruins_chain >= 2)
    {
        g_ui_route_options[0].template_id = 1004;
        g_ui_route_options[0].data = UI_SPECIAL_ROUTE_DEMON_CONTRACT;
        g_ui_route_options[0].data.immediate_reward.amount *= scale;
    }
    else if (g_ui_sect_trial_chain >= 2)
    {
        g_ui_route_options[0].template_id = 1005;
        g_ui_route_options[0].data = UI_SPECIAL_ROUTE_SECT_TOURNAMENT;
        g_ui_route_options[0].data.immediate_reward.amount *= scale;
    }
    else if (g_ui_mortal_world_chain >= 2)
    {
        g_ui_route_options[0].template_id = 1006;
        g_ui_route_options[0].data = UI_SPECIAL_ROUTE_MORTAL_FAREWELL;
        g_ui_route_options[0].data.immediate_reward.amount *= scale;
    }
    else if (g_ui_last_route_template_id == 2 && g_ui_route_streak >= 1)
    {
        g_ui_route_options[1].template_id = 1001;
        g_ui_route_options[1].data = UI_SPECIAL_ROUTE_BLOOD_BEAST;
        g_ui_route_options[1].data.immediate_reward.amount *= scale;
    }
    g_ui_phase = UI_PHASE_ROUTE_CHOICE;
}

static void UI_GenerateTalentOptions(void)
{
    for (int i = 0; i < REWARD_CHOICE_COUNT; i++)
        g_ui_talent_options[i] = TALENT_NONE;

    int available = Talent_CountAvailable(&YOU);
    if (available <= 0)
    {
        int compensation = Yuan[YOU.XIUWEI] * 4;
        YOU.cultivation += compensation;
        UI_AddLog("道基天赋已尽数圆满，机缘转化为修为。");
        g_ui_phase = UI_PHASE_PREPARATION;
        return;
    }

    bool used[TOTAL_TALENTS] = {false};
    for (int i = 0; i < REWARD_CHOICE_COUNT && i < available; i++)
    {
        int pick = (int)(Run_Rand() % TOTAL_TALENTS);
        for (int guard = 0; guard < TOTAL_TALENTS; guard++)
        {
            TalentID candidate = (TalentID)((pick + guard) % TOTAL_TALENTS);
            if (!used[candidate] && !Player_HasTalent(&YOU, candidate))
            {
                pick = candidate;
                break;
            }
        }
        used[pick] = true;
        g_ui_talent_options[i] = (TalentID)pick;
    }
    g_ui_phase = UI_PHASE_TALENT_CHOICE;
}

static void UI_EnterPreparation(void)
{
    Player_ClampVitals(&YOU);
    g_ui_phase = UI_PHASE_PREPARATION;
}

static void UI_AfterRewardDecision(void)
{
    g_ui_pending_replacement_reward = (RewardChoice){REWARD_CULTIVATION, RARITY_COMMON, 0, 0, false};
    g_ui_replacement_type = UI_REPLACE_NONE;
    g_ui_replacement_return_phase = UI_PHASE_RUN_START;
    g_ui_replacement_slot_count = 0;
    g_ui_replacement_action_type = ACTION_TYPE_NONE;
    if (UI_CanBreakthrough(&YOU) && !g_ui_breakthrough_offered)
    {
        g_ui_breakthrough_offered = true;
        g_ui_phase = UI_PHASE_HEART_DEMON_CHOICE;
    }
    else
    {
        UI_EnterPreparation();
    }
}

static void UI_FinishRewardFlow(void)
{
    g_ui_pending_replacement_reward = (RewardChoice){REWARD_CULTIVATION, RARITY_COMMON, 0, 0, false};
    g_ui_replacement_type = UI_REPLACE_NONE;
    g_ui_replacement_return_phase = UI_PHASE_RUN_START;
    g_ui_replacement_slot_count = 0;
    g_ui_replacement_action_type = ACTION_TYPE_NONE;
    UI_AfterRewardDecision();
}

static void UI_FinishReplacementFlow(void)
{
    UIPhase return_phase = g_ui_replacement_return_phase;
    g_ui_pending_replacement_reward = (RewardChoice){REWARD_CULTIVATION, RARITY_COMMON, 0, 0, false};
    g_ui_replacement_type = UI_REPLACE_NONE;
    g_ui_replacement_return_phase = UI_PHASE_RUN_START;
    g_ui_replacement_slot_count = 0;
    g_ui_replacement_action_type = ACTION_TYPE_NONE;
    if (return_phase == UI_PHASE_PREPARATION)
        UI_EnterPreparation();
    else
        UI_AfterRewardDecision();
}

static void UI_AddArtifactDirect(ArtifactID id)
{
    if (id <= ARTIFACT_NONE || id >= TOTAL_ARTIFACTS)
        return;
    if (Player_HasArtifact(&YOU, id))
        return;
    if (YOU.artifact_count < MAX_ARTIFACT_SLOTS)
    {
        int slot = YOU.artifact_count++;
        YOU.artifacts[slot] = id;
        YOU.artifact_levels[slot] = 0;
    }
}

static void UI_AddElixirDirect(ElixirID id)
{
    if (id <= ELIXIR_NONE || id >= TOTAL_ELIXIRS)
        return;
    if (YOU.elixir_count < MAX_ELIXIR_SLOTS)
        YOU.elixirs[YOU.elixir_count++] = id;
}

static const AlchemyRecipe *UI_FindAlchemyRecipeByElixir(ElixirID id)
{
    for (int i = 0; i < Alchemy_RecipeCount(); i++)
    {
        const AlchemyRecipe *recipe = Alchemy_GetRecipe(i);
        if (recipe && recipe->elixir_id == id)
            return recipe;
    }
    return NULL;
}

static void UI_UnlockAlchemyRecipe(ElixirID id, const char *source)
{
    const AlchemyRecipe *recipe = UI_FindAlchemyRecipeByElixir(id);
    if (!recipe || recipe->index < 0 || recipe->index >= MAX_ALCHEMY_RECIPES)
        return;
    if (YOU.unlocked_alchemy_recipes[recipe->index])
    {
        YOU.herb_materials += 1;
        RPG_AddMinorUnderstanding(&YOU, 1);
        char repeat_message[192];
        snprintf(repeat_message, sizeof(repeat_message), "重温%s丹方，转作药材与修为理解。", recipe->name);
        UI_AddLog(repeat_message);
        return;
    }
    YOU.unlocked_alchemy_recipes[recipe->index] = true;
    char message[192];
    snprintf(message, sizeof(message), "%s领悟丹方：%s。",
             (source && source[0]) ? source : "", recipe->name);
    UI_AddLog(message);
}

static void UI_UnlockFirstAlchemyRecipe(const ElixirID *ids, int count, const char *source)
{
    for (int i = 0; i < count; i++)
    {
        const AlchemyRecipe *recipe = UI_FindAlchemyRecipeByElixir(ids[i]);
        if (recipe && recipe->index >= 0 && recipe->index < MAX_ALCHEMY_RECIPES &&
            !YOU.unlocked_alchemy_recipes[recipe->index])
        {
            UI_UnlockAlchemyRecipe(ids[i], source);
            return;
        }
    }
    if (count > 0)
        UI_UnlockAlchemyRecipe(ids[0], source);
}

static void UI_UnlockDefaultAlchemyRecipes(void)
{
    const ElixirID defaults[] = {ELIXIR_HEALING, ELIXIR_QI, ELIXIR_CLEAR_MIND};
    for (int i = 0; i < 3; i++)
    {
        const AlchemyRecipe *recipe = UI_FindAlchemyRecipeByElixir(defaults[i]);
        if (recipe && recipe->index >= 0 && recipe->index < MAX_ALCHEMY_RECIPES)
            YOU.unlocked_alchemy_recipes[recipe->index] = true;
    }
}

static bool UI_UnlockSkillDirect(SkillID id, bool bypass, bool auto_equip)
{
    if (id <= SKILL_ID_NONE || id >= TOTAL_SKILLS)
        return false;
    const Skill *skill = &g_skill_database[id];
    bool was_unlocked = YOU.unlocked_skills[id];
    YOU.unlocked_skills[id] = true;
    if (bypass)
        YOU.bypassed_prereq_skills[id] = true;
    if (!was_unlocked)
    {
        Player_AddSchool(&YOU, skill->school_tag, 1);
        char message[160];
        snprintf(message, sizeof(message), "悟得功法：%s。", skill->name_chn);
        UI_AddLog(message);
    }
    if (auto_equip && YOU.equipped_skills[skill->action_type] == SKILL_ID_NONE)
        Player_EquipSkill(&YOU, id);
    else
        Player_SyncLearnedSkillsFromLoadout(&YOU);
    return !was_unlocked;
}

static bool UI_TryInsightSkill(RewardRarity rarity, bool bypass, const char *source)
{
    SkillID id = PickRewardSkill(&YOU, rarity, bypass);
    if (id == SKILL_ID_NONE)
        return false;
    if (source && source[0])
        UI_AddLog(source);
    return UI_UnlockSkillDirect(id, bypass, true);
}

static int UI_NearDeathGhostChance(void)
{
    BuildProfile profile = Player_GetBuildProfile(&YOU);
    int chance = 30;
    if (profile.primary_school == SCHOOL_BLOOD || profile.primary_school == SCHOOL_DARK)
        chance += 15;
    if (Player_HasTalent(&YOU, TALENT_BLOOD_REBIRTH))
        chance += 15;
    if (Player_HasArtifact(&YOU, ARTIFACT_DEVOURING_ORB))
        chance += 10;
    if (Player_HasArtifact(&YOU, ARTIFACT_FORMLESS_JADE))
        chance += 10;
    if (chance > 70)
        chance = 70;
    return chance;
}

static void UI_BeginNearDeath(const char *reason)
{
    g_ui_near_death_reason = reason && reason[0] ? reason : "肉身崩解，生死一线。";
    g_ui_near_death_ghost_chance = UI_NearDeathGhostChance();
    g_ui_near_death_can_ghost = ((int)(Run_Rand() % 100) < g_ui_near_death_ghost_chance);
    YOU.HP = 0;
    YOU.QI = 0;
    UI_AddLog(g_ui_near_death_reason);
    g_ui_phase = UI_PHASE_NEAR_DEATH_CHOICE;
}

static void UI_BeginReplacement(RewardChoice choice, UIReplacementType type)
{
    g_ui_pending_replacement_reward = choice;
    g_ui_replacement_type = type;
    g_ui_replacement_return_phase = g_ui_phase;
    g_ui_replacement_action_type = ACTION_TYPE_NONE;
    if (type == UI_REPLACE_SKILL)
    {
        const Skill *skill = &g_skill_database[choice.id];
        g_ui_replacement_action_type = skill->action_type;
        g_ui_replacement_slot_count = 1;
        UI_AddLog("领悟了新技能，是否替换当前动作槽？");
    }
    else if (type == UI_REPLACE_ARTIFACT)
    {
        g_ui_replacement_slot_count = YOU.artifact_count;
        UI_AddLog("法器槽已满，选择替换或放弃新法器。");
    }
    else if (type == UI_REPLACE_ELIXIR)
    {
        g_ui_replacement_slot_count = YOU.elixir_count;
        UI_AddLog("丹药袋已满，选择替换或放弃新丹药。");
    }
    g_ui_phase = UI_PHASE_REWARD_REPLACE;
}

static void UI_ApplyRewardNonBlocking(const RewardChoice *choice)
{
    switch (choice->type)
    {
    case REWARD_SKILL:
    {
        if (choice->id <= SKILL_ID_NONE || choice->id >= TOTAL_SKILLS)
            return;
        const Skill *skill = &g_skill_database[choice->id];
        bool was_unlocked = YOU.unlocked_skills[choice->id];
        YOU.unlocked_skills[choice->id] = true;
        if (choice->bypass_prereq)
            YOU.bypassed_prereq_skills[choice->id] = true;
        if (!was_unlocked)
        {
            Player_AddSchool(&YOU, skill->school_tag, 1);
            UI_AddLog("领悟了新的功法。");
        }
        if (YOU.equipped_skills[skill->action_type] == SKILL_ID_NONE)
        {
            Player_EquipSkill(&YOU, (SkillID)choice->id);
            UI_FinishRewardFlow();
        }
        else
        {
            Player_SyncLearnedSkillsFromLoadout(&YOU);
            UI_BeginReplacement(*choice, UI_REPLACE_SKILL);
        }
        break;
    }
    case REWARD_ARTIFACT:
        if (choice->id <= ARTIFACT_NONE || choice->id >= TOTAL_ARTIFACTS)
            return;
        if (Player_HasArtifact(&YOU, (ArtifactID)choice->id))
        {
            UI_AddLog("已有同名法器，灵韵转作修炼资源。");
            YOU.cultivation += Yuan[YOU.XIUWEI] * 2;
            Player_ClampVitals(&YOU);
            UI_FinishRewardFlow();
        }
        else if (YOU.artifact_count < MAX_ARTIFACT_SLOTS)
        {
            UI_AddArtifactDirect((ArtifactID)choice->id);
            UI_AddLog("获得了新的法器。");
            UI_FinishRewardFlow();
        }
        else
        {
            UI_BeginReplacement(*choice, UI_REPLACE_ARTIFACT);
        }
        break;
    case REWARD_ELIXIR:
        if (choice->id <= ELIXIR_NONE || choice->id >= TOTAL_ELIXIRS)
            return;
        for (int i = 0; i < choice->amount && YOU.elixir_count < MAX_ELIXIR_SLOTS; i++)
            UI_AddElixirDirect((ElixirID)choice->id);
        if (YOU.elixir_count < MAX_ELIXIR_SLOTS || choice->amount <= 0)
        {
            UI_AddLog("丹药已收入丹囊。");
            UI_FinishRewardFlow();
        }
        else
        {
            UI_BeginReplacement(*choice, UI_REPLACE_ELIXIR);
        }
        break;
    case REWARD_CULTIVATION:
        YOU.cultivation += choice->amount;
        Player_ClampVitals(&YOU);
        UI_AddLog("吸纳修炼资源，修为更加深厚。");
        UI_FinishRewardFlow();
        break;
    }
}

static void UI_RunEncounter(void)
{
    g_ui_last_encounter_reward = (RewardChoice){REWARD_CULTIVATION, RARITY_COMMON, 0, Yuan[YOU.XIUWEI] * 3, false};
    g_ui_last_encounter_penalty = 0;
    g_ui_last_encounter_title = "";
    g_ui_last_encounter_text = "";
    if (g_run.battle_index <= 0 || (g_run.battle_index % 2) != 0)
    {
        g_ui_phase = UI_PHASE_BETWEEN_BATTLES;
        return;
    }

    EncounterID encounter = (EncounterID)(Run_Rand() % TOTAL_ENCOUNTERS);
    int penalty = 0;
    switch (encounter)
    {
    case ENCOUNTER_ANCIENT_CAVE:
        g_ui_last_encounter_title = "古修洞府";
        g_ui_last_encounter_text = "残碑上的道纹仍在发光。";
        g_ui_last_encounter_reward = (RewardChoice){REWARD_SKILL, RARITY_RARE, PickRewardSkill(&YOU, RARITY_RARE, true), 1, true};
        penalty = max_HP[YOU.XIUWEI] / 10;
        break;
    case ENCOUNTER_WOUNDED_BEAST:
        g_ui_last_encounter_title = "濒死妖兽";
        g_ui_last_encounter_text = "一只妖兽伏在路旁，妖丹隐现。";
        g_ui_last_encounter_reward = (RewardChoice){REWARD_ELIXIR, RARITY_RARE, ELIXIR_BREAKTHROUGH, 1, false};
        break;
    case ENCOUNTER_TRIBULATION_CLOUD:
        g_ui_last_encounter_title = "天劫残云";
        g_ui_last_encounter_text = "劫雷余威尚未散去。";
        g_ui_last_encounter_reward = (RewardChoice){REWARD_ARTIFACT, RARITY_RARE, ARTIFACT_THUNDER_WOOD_SWORD, 1, false};
        penalty = max_HP[YOU.XIUWEI] / 8;
        break;
    case ENCOUNTER_MARKET:
        g_ui_last_encounter_title = "坊市交易";
        g_ui_last_encounter_text = "散修摊位上真假难辨。";
        g_ui_last_encounter_reward = (RewardChoice){REWARD_ELIXIR, RARITY_COMMON, PickRewardElixir(RARITY_RARE), 2, false};
        break;
    case ENCOUNTER_SWORD_TOMB:
        g_ui_last_encounter_title = "无名剑冢";
        g_ui_last_encounter_text = "断剑低鸣，似在择主。";
        g_ui_last_encounter_reward = (RewardChoice){REWARD_SKILL, RARITY_EPIC, SKILL_ID_GREAT_GOLDEN_SWORDFORMATION, 1, true};
        penalty = max_HP[YOU.XIUWEI] / 6;
        break;
    case ENCOUNTER_BLOOD_POOL:
        g_ui_last_encounter_title = "血池秘境";
        g_ui_last_encounter_text = "血雾翻滚，魔功唾手可得。";
        g_ui_last_encounter_reward = (RewardChoice){REWARD_SKILL, RARITY_EPIC, SKILL_ID_BLOOD_DEVIL_SLASH, 1, true};
        YOU.cursed += Yuan[YOU.XIUWEI];
        break;
    default:
        break;
    }

    Encounter_EnsureValidReward(&YOU, &g_ui_last_encounter_reward);
    if (penalty > 0 && g_ui_last_encounter_reward.bypass_prereq &&
        Player_HasArtifact(&YOU, ARTIFACT_FORMLESS_JADE) && !YOU.formless_jade_used)
    {
        penalty = 0;
        YOU.formless_jade_used = true;
    }
    if (penalty > 0 && YOU.disaster_warded)
    {
        penalty /= 2;
        YOU.disaster_warded = false;
    }
    if (penalty > 0)
    {
        YOU.HP -= penalty;
        if (YOU.HP <= 0) YOU.HP = 1;
    }
    g_ui_last_encounter_penalty = penalty;
    RewardChoice encounter_reward = g_ui_last_encounter_reward;
    switch (encounter_reward.type)
    {
    case REWARD_SKILL:
        if (encounter_reward.id > SKILL_ID_NONE && encounter_reward.id < TOTAL_SKILLS)
        {
            bool was_unlocked = YOU.unlocked_skills[encounter_reward.id];
            YOU.unlocked_skills[encounter_reward.id] = true;
            if (encounter_reward.bypass_prereq)
                YOU.bypassed_prereq_skills[encounter_reward.id] = true;
            if (!was_unlocked)
                Player_AddSchool(&YOU, g_skill_database[encounter_reward.id].school_tag, 1);
            if (YOU.equipped_skills[g_skill_database[encounter_reward.id].action_type] == SKILL_ID_NONE)
                Player_EquipSkill(&YOU, (SkillID)encounter_reward.id);
            else
                Player_SyncLearnedSkillsFromLoadout(&YOU);
        }
        break;
    case REWARD_ARTIFACT:
        if (!Player_HasArtifact(&YOU, (ArtifactID)encounter_reward.id) && YOU.artifact_count < MAX_ARTIFACT_SLOTS)
            UI_AddArtifactDirect((ArtifactID)encounter_reward.id);
        break;
    case REWARD_ELIXIR:
        for (int i = 0; i < encounter_reward.amount && YOU.elixir_count < MAX_ELIXIR_SLOTS; i++)
            UI_AddElixirDirect((ElixirID)encounter_reward.id);
        break;
    case REWARD_CULTIVATION:
        YOU.QI += encounter_reward.amount;
        Player_ClampVitals(&YOU);
        break;
    }
    UI_AddLog(g_ui_last_encounter_title);
    g_ui_phase = UI_PHASE_ENCOUNTER;
}

static void UI_ProcessBattleAction(int action_type)
{
    if (g_ui_phase != UI_PHASE_BATTLE_ACTION)
        return;
    if (!UI_SetPlayerAction(action_type))
    {
        UI_AddLog("当前行动不可用。");
        return;
    }
    UI_ChooseCPUAction();
    const Skill *enemy_skill = &CPU.learned_skills[CPU.current_action_type];
    if (enemy_skill->skill_id != SKILL_ID_NONE)
    {
        char enemy_action_log[256];
        snprintf(enemy_action_log, sizeof(enemy_action_log), "%s 准备施展 %s。", CPU.name, enemy_skill->name_chn);
        UI_AddCombatFeed("enemy", "action", enemy_action_log);
    }
    CPU_action(&CPU);
    int before_you_hp = YOU.HP;
    int before_you_qi = YOU.QI;
    int before_you_bleeding = YOU.bleeding;
    int before_you_cursed = YOU.cursed;
    int before_you_enraged = YOU.enraged;
    int before_mastery[TOTAL_SKILLS];
    int before_cycles[TOTAL_SCHOOLS];
    memcpy(before_mastery, YOU.skill_mastery, sizeof(before_mastery));
    memcpy(before_cycles, YOU.school_cycle, sizeof(before_cycles));
    int before_cpu_hp = CPU.HP;
    int before_cpu_qi = CPU.QI;
    int before_cpu_bleeding = CPU.bleeding;
    int before_cpu_cursed = CPU.cursed;
    int before_cpu_enraged = CPU.enraged;
    Action_resolve(&YOU, &CPU);
    Player_ClampVitals(&YOU);
    Player_ClampVitals(&CPU);
    if (YOU.HP < before_you_hp)
    {
        char message[128];
        snprintf(message, sizeof(message), "你损失 %d 点元神。", before_you_hp - YOU.HP);
        UI_AddCombatFeed("player", "damage", message);
    }
    else if (YOU.HP > before_you_hp)
    {
        char message[128];
        snprintf(message, sizeof(message), "你恢复 %d 点元神。", YOU.HP - before_you_hp);
        UI_AddCombatFeed("player", "heal", message);
    }
    if (CPU.HP < before_cpu_hp)
    {
        char message[128];
        snprintf(message, sizeof(message), "%s 损失 %d 点元神。", CPU.name, before_cpu_hp - CPU.HP);
        UI_AddCombatFeed("enemy", "damage", message);
    }
    else if (CPU.HP > before_cpu_hp)
    {
        char message[128];
        snprintf(message, sizeof(message), "%s 恢复 %d 点元神。", CPU.name, CPU.HP - before_cpu_hp);
        UI_AddCombatFeed("enemy", "heal", message);
    }
    if (YOU.QI != before_you_qi)
    {
        char message[128];
        snprintf(message, sizeof(message), "你的灵气 %+d。", YOU.QI - before_you_qi);
        UI_AddCombatFeed("player", "qi", message);
    }
    if (CPU.QI != before_cpu_qi)
    {
        char message[128];
        snprintf(message, sizeof(message), "%s 灵气 %+d。", CPU.name, CPU.QI - before_cpu_qi);
        UI_AddCombatFeed("enemy", "qi", message);
    }
    if (YOU.bleeding > before_you_bleeding)
    {
        char message[128];
        snprintf(message, sizeof(message), "你流血加深 %+d。", YOU.bleeding - before_you_bleeding);
        UI_AddCombatFeed("player", "damage", message);
    }
    if (CPU.bleeding > before_cpu_bleeding)
    {
        char message[128];
        snprintf(message, sizeof(message), "%s 流血加深 %+d。", CPU.name, CPU.bleeding - before_cpu_bleeding);
        UI_AddCombatFeed("enemy", "damage", message);
    }
    if (YOU.cursed > before_you_cursed)
    {
        char message[128];
        snprintf(message, sizeof(message), "你诅咒加深 %+d。", YOU.cursed - before_you_cursed);
        UI_AddCombatFeed("player", "damage", message);
    }
    if (CPU.cursed > before_cpu_cursed)
    {
        char message[128];
        snprintf(message, sizeof(message), "%s 诅咒加深 %+d。", CPU.name, CPU.cursed - before_cpu_cursed);
        UI_AddCombatFeed("enemy", "damage", message);
    }
    if (YOU.enraged > before_you_enraged)
    {
        char message[128];
        snprintf(message, sizeof(message), "你气势提高 %+d。", YOU.enraged - before_you_enraged);
        UI_AddCombatFeed("player", "qi", message);
    }
    if (CPU.enraged > before_cpu_enraged)
    {
        char message[128];
        snprintf(message, sizeof(message), "%s 气势提高 %+d。", CPU.name, CPU.enraged - before_cpu_enraged);
        UI_AddCombatFeed("enemy", "qi", message);
    }
    for (int i = 0; i < TOTAL_SKILLS; i++)
    {
        if (YOU.skill_mastery[i] <= before_mastery[i])
            continue;
        int old_level = 0;
        if (before_mastery[i] >= 15) old_level = 3;
        else if (before_mastery[i] >= 8) old_level = 2;
        else if (before_mastery[i] >= 3) old_level = 1;
        int new_level = RPG_MasteryLevel(&YOU, (SkillID)i);
        if (new_level > old_level && Skill_IsDefined((SkillID)i))
        {
            char message[160];
            snprintf(message, sizeof(message), "%s 熟练度提升至 Lv%d。", g_skill_database[i].name_chn, new_level);
            UI_AddCombatFeed("player", "mastery", message);
        }
    }
    for (int i = 1; i < TOTAL_SCHOOLS; i++)
    {
        if (YOU.school_cycle[i] == before_cycles[i])
            continue;
        char summary[96];
        char message[160];
        if (YOU.school_cycle[i] < before_cycles[i])
        {
            snprintf(message, sizeof(message), "%s循环触发。", SchoolTagName((SchoolTag)i));
        }
        else
        {
            RPG_SchoolCycleSummary(&YOU, (SchoolTag)i, summary, sizeof(summary));
            snprintf(message, sizeof(message), "%s。", summary[0] ? summary : SchoolTagName((SchoolTag)i));
        }
        UI_AddCombatFeed("player", "mastery", message);
    }
    g_ui_phase = UI_PHASE_BATTLE_RESULT;
    if (YOU.HP <= 0)
    {
        if (YOU.soul_state == SOUL_GHOST)
        {
            UI_SetEnding("soul_scattered", "魂飞魄散", "鬼魂之身再遭死劫，魂火彻底熄灭。");
            UI_AddCombatFeed("player", "defeat", "魂火熄灭，终至魂飞魄散。");
        }
        else
        {
            UI_BeginNearDeath("肉身濒毁，生死只在一念之间。");
        }
    }
    else if (CPU.HP <= 0 && YOU.HP > 0)
    {
        UI_AddCombatFeed("system", "victory", "战斗胜利，选择战利品。");
        UI_GenerateRewards();
    }
    else if (game.round_number >= MAX_ROUNDS)
    {
        UI_AddCombatFeed("system", "info", "战斗拖入僵局，强制结束。");
        UI_GenerateRewards();
    }
}

static void UI_Continue(void)
{
    if (g_ui_phase == UI_PHASE_BATTLE_RESULT)
    {
        if (CPU.HP > 0 && YOU.HP > 0)
        {
            Start_new_round(&game);
            g_ui_phase = UI_PHASE_BATTLE_ACTION;
        }
        return;
    }
    if (g_ui_phase == UI_PHASE_REWARD_CHOICE)
        return;
    if (g_ui_phase == UI_PHASE_ENCOUNTER || g_ui_phase == UI_PHASE_BETWEEN_BATTLES)
        UI_StartBattle();
}

static void UI_ChooseReward(int index)
{
    if (g_ui_phase != UI_PHASE_REWARD_CHOICE)
        return;
    if (index < 0 || index >= REWARD_CHOICE_COUNT)
        index = 0;
    UI_AddLog("选择了一项战利品。");
    UI_ApplyRewardNonBlocking(&g_ui_pending_rewards[index]);
}

static void UI_ChooseReplacement(int slot)
{
    if (g_ui_phase != UI_PHASE_REWARD_REPLACE)
        return;
    RewardChoice choice = g_ui_pending_replacement_reward;
    if (slot < 0)
    {
        UI_AddLog("保留原有配置，放弃替换。");
        UI_FinishReplacementFlow();
        return;
    }
    if (g_ui_replacement_type == UI_REPLACE_SKILL)
    {
        if (choice.id > SKILL_ID_NONE && choice.id < TOTAL_SKILLS)
        {
            Player_EquipSkill(&YOU, (SkillID)choice.id);
            UI_AddLog("新技能已装备。");
        }
        UI_FinishReplacementFlow();
        return;
    }
    if (g_ui_replacement_type == UI_REPLACE_ARTIFACT)
    {
        if (slot >= 0 && slot < YOU.artifact_count &&
            choice.id > ARTIFACT_NONE && choice.id < TOTAL_ARTIFACTS)
        {
            YOU.artifacts[slot] = (ArtifactID)choice.id;
            YOU.artifact_levels[slot] = 0;
            UI_AddLog("法器替换完成。");
        }
        UI_FinishReplacementFlow();
        return;
    }
    if (g_ui_replacement_type == UI_REPLACE_ELIXIR)
    {
        if (slot >= 0 && slot < YOU.elixir_count &&
            choice.id > ELIXIR_NONE && choice.id < TOTAL_ELIXIRS)
        {
            YOU.elixirs[slot] = (ElixirID)choice.id;
            UI_AddLog("丹药替换完成。");
        }
        UI_FinishReplacementFlow();
    }
}

static void UI_UpgradeArtifact(int slot)
{
    if (g_ui_phase != UI_PHASE_PREPARATION)
        return;
    if (slot < 0 || slot >= YOU.artifact_count)
    {
        UI_AddLog("没有可强化的法器槽。");
        return;
    }
    int level = Artifact_LevelAtSlot(&YOU, slot);
    int cost = Artifact_UpgradeCost(level);
    if (cost <= 0)
    {
        UI_AddLog("这件法器已强化至当前上限。");
        return;
    }
    if (YOU.spirit_materials < cost)
    {
        UI_AddLog("灵材不足，无法强化法器。");
        return;
    }
    YOU.spirit_materials -= cost;
    YOU.artifact_levels[slot] = level + 1;
    Player_ClampVitals(&YOU);
    char message[160];
    snprintf(message, sizeof(message), "消耗 %d 灵材，%s 强化至 +%d。",
             cost, g_artifact_database[YOU.artifacts[slot]].name_chn, YOU.artifact_levels[slot]);
    UI_AddLog(message);
}

static void UI_BrewElixirRecipe(int recipe_index)
{
    if (g_ui_phase != UI_PHASE_PREPARATION)
        return;
    const AlchemyRecipe *recipe = Alchemy_GetRecipe(recipe_index);
    if (!recipe)
    {
        UI_AddLog("未知丹方。");
        return;
    }
    if (!Alchemy_RecipeUnlocked(&YOU, recipe))
    {
        UI_AddLog("境界不足，尚不能炼制此丹。");
        return;
    }
    if (YOU.herb_materials < recipe->herb_cost)
    {
        UI_AddLog("药材不足，无法开炉炼丹。");
        return;
    }
    YOU.herb_materials -= recipe->herb_cost;
    RewardChoice brewed = {REWARD_ELIXIR, g_elixir_database[recipe->elixir_id].rarity, recipe->elixir_id, 1, false};
    if (YOU.elixir_count < MAX_ELIXIR_SLOTS)
    {
        UI_AddElixirDirect(recipe->elixir_id);
        char message[160];
        snprintf(message, sizeof(message), "开炉炼成 %s。", g_elixir_database[recipe->elixir_id].name_chn);
        UI_AddLog(message);
    }
    else
    {
        UI_AddLog("丹药袋已满，炼成的新丹需要决定去留。");
        UI_BeginReplacement(brewed, UI_REPLACE_ELIXIR);
    }
}

static void UI_SkipPreparation(void)
{
    if (g_ui_phase != UI_PHASE_PREPARATION)
        return;
    UI_AddLog("整备完毕，继续择路。");
    UI_GenerateRouteOptions();
}

static void UI_EquipManagedSkill(int skill_id)
{
    if (!UI_CanManageSkills())
    {
        UI_AddLog("当前阶段不适合重整功法。");
        return;
    }
    if (!Skill_IsDefined((SkillID)skill_id) || !YOU.unlocked_skills[skill_id])
    {
        UI_AddLog("尚未领悟该功法。");
        return;
    }
    const Skill *skill = &g_skill_database[skill_id];
    if (skill->action_type < 0 || skill->action_type >= TOTAL_ACTION_TYPES)
        return;
    if (YOU.equipped_skills[skill->action_type] == skill_id)
    {
        UI_AddLog("该功法已在当前动作槽中。");
        return;
    }
    int cost = UI_RespecSkillCost((SkillID)skill_id);
    if (YOU.cultivation < cost)
    {
        UI_AddLog("修为不足，无法洗点换诀。");
        return;
    }
    YOU.cultivation -= cost;
    Player_EquipSkill(&YOU, (SkillID)skill_id);
    char message[160];
    snprintf(message, sizeof(message), "洗点换诀：%s 已装备。", skill->name_chn);
    UI_AddLog(message);
}

static void UI_RefineSkill(int skill_id)
{
    if (!UI_CanManageSkills())
    {
        UI_AddLog("当前阶段不适合洗练功法。");
        return;
    }
    if (!Skill_IsDefined((SkillID)skill_id) || !YOU.unlocked_skills[skill_id])
    {
        UI_AddLog("尚未领悟该功法。");
        return;
    }
    const Skill *skill = &g_skill_database[skill_id];
    int cost = Skill_RefineCost(&YOU, skill);
    if (YOU.cultivation < cost)
    {
        UI_AddLog("修为不足，无法洗练功法。");
        return;
    }
    YOU.cultivation -= cost;
    SkillRefineType refine = SkillRefine_RandomForSkill(skill);
    YOU.skill_refines[skill_id] = refine;
    char message[160];
    snprintf(message, sizeof(message), "%s 洗练出词条：%s。", skill->name_chn, SkillRefineName(refine));
    UI_AddLog(message);
}

static void UI_AttemptBreakthrough(void)
{
    if (g_ui_phase != UI_PHASE_BREAKTHROUGH_CHOICE && g_ui_phase != UI_PHASE_HEART_DEMON_CHOICE)
        return;
    int cost = UI_BreakthroughCost(&YOU);
    if (cost <= 0 || YOU.cultivation < cost)
    {
        UI_EnterPreparation();
        return;
    }
    int chance = UI_BreakthroughChance(&YOU);
    YOU.cultivation -= cost;
    if ((int)(Run_Rand() % 100) < chance)
    {
        YOU.XIUWEI++;
        RPG_ResetMinorRealmProgress(&YOU);
        Apply_Breakthrough_Rewards(&YOU);
        UI_AddLog("闭关功成，境界突破。");
        {
            char chronicle[192];
            snprintf(chronicle, sizeof(chronicle), "%d 岁，闭关功成，突破至%s。", g_ui_age, Realm[YOU.XIUWEI]);
            UI_AddChronicle("breakthrough", chronicle);
        }
        YOU.breakthrough_bonus = 0;
        YOU.breakthrough_fail_penalty = 0;
        if (g_ui_tribulation_pressure > 0)
            g_ui_tribulation_pressure--;
        if (UI_CheckAscensionVictory())
            return;
        UI_GenerateTalentOptions();
        return;
    }
    else
    {
        int refund = cost / 4;
        int hp_loss_pct = Player_HasArtifact(&YOU, ARTIFACT_BREAKTHROUGH_SEAL) ? 25 : 15;
        int hp_loss = Player_MaxHP(&YOU) * hp_loss_pct / 100;
        YOU.cultivation += refund;
        YOU.HP -= hp_loss;
        if (YOU.HP <= 0)
            YOU.HP = 1;
        UI_AddLog("闭关未竟，气血受损。");
    }
    YOU.breakthrough_bonus = 0;
    YOU.breakthrough_fail_penalty = 0;
    UI_EnterPreparation();
}

static void UI_ResolveHeartBreakthrough(UIHeartDemonMode mode)
{
    if (g_ui_phase != UI_PHASE_HEART_DEMON_CHOICE)
        return;
    int base_cost = UI_BreakthroughCost(&YOU);
    if (mode == UI_HEART_SUPPRESS)
    {
        RPG_AddMinorUnderstanding(&YOU, 1);
        g_ui_calamity_debt++;
        g_ui_tribulation_pressure++;
        UI_AddLog("压境不破，理解更深，劫气也随之积聚。");
        UI_AddChronicle("heart", "压境不破，心魔暂伏，劫意更深。");
        g_ui_breakthrough_offered = false;
        UI_EnterPreparation();
        return;
    }

    int cost = base_cost;
    int chance = UI_BreakthroughChance(&YOU);
    int hp_loss_pct = Player_HasArtifact(&YOU, ARTIFACT_BREAKTHROUGH_SEAL) ? 25 : 15;
    if (mode == UI_HEART_STEADY)
    {
        cost = base_cost * 5 / 4;
        chance += 10;
        hp_loss_pct = hp_loss_pct * 2 / 3;
    }
    else if (mode == UI_HEART_RISKY)
    {
        cost = base_cost * 3 / 4;
        chance -= 15;
        hp_loss_pct += 10;
    }
    if (chance < 5) chance = 5;
    if (chance > 95) chance = 95;
    if (base_cost <= 0 || YOU.cultivation < cost)
    {
        UI_AddLog("修为不足，心魔关前只能暂退。");
        UI_EnterPreparation();
        return;
    }
    YOU.cultivation -= cost;
    if ((int)(Run_Rand() % 100) < chance)
    {
        YOU.XIUWEI++;
        RPG_ResetMinorRealmProgress(&YOU);
        Apply_Breakthrough_Rewards(&YOU);
        if (mode == UI_HEART_RISKY)
            RPG_AddMinorUnderstanding(&YOU, 1);
        UI_AddLog(mode == UI_HEART_STEADY ? "稳妥破关，心魔未能扰道。" : "冒险破关，险中悟得一线真意。");
        {
            char chronicle[192];
            snprintf(chronicle, sizeof(chronicle), "%d 岁，破心魔关，突破至%s。", g_ui_age, Realm[YOU.XIUWEI]);
            UI_AddChronicle("breakthrough", chronicle);
        }
        YOU.breakthrough_bonus = 0;
        YOU.breakthrough_fail_penalty = 0;
        if (mode == UI_HEART_STEADY && g_ui_tribulation_pressure > 0)
            g_ui_tribulation_pressure--;
        if (mode == UI_HEART_RISKY)
            g_ui_tribulation_pressure += 1;
        if (UI_CheckAscensionVictory())
            return;
        UI_GenerateTalentOptions();
        return;
    }

    int refund = cost / 4;
    int hp_loss = Player_MaxHP(&YOU) * hp_loss_pct / 100;
    YOU.cultivation += refund;
    YOU.HP -= hp_loss;
    if (YOU.HP <= 0)
        YOU.HP = 1;
    UI_AddLog(mode == UI_HEART_STEADY ? "稳妥破关仍未竟，所幸反噬较轻。" : "冒险破关失败，心魔反噬更重。");
    YOU.breakthrough_bonus = 0;
    YOU.breakthrough_fail_penalty = 0;
    UI_EnterPreparation();
}

static void UI_ChooseTalent(int index)
{
    if (g_ui_phase != UI_PHASE_TALENT_CHOICE)
        return;
    if (index < 0 || index >= REWARD_CHOICE_COUNT)
        index = 0;
    TalentID id = g_ui_talent_options[index];
    if (id >= 0 && id < TOTAL_TALENTS && Player_AddTalent(&YOU, id))
    {
        char message[128];
        snprintf(message, sizeof(message), "道基天赋成型：%s。", g_talent_database[id].name_chn);
        UI_AddLog(message);
        UI_AddChronicle("talent", message);
    }
    else
    {
        YOU.cultivation += Yuan[YOU.XIUWEI] * 3;
        UI_AddLog("机缘未能成形，转化为修为。");
    }
    UI_EnterPreparation();
}

static void UI_ChooseNearDeath(int index)
{
    if (g_ui_phase != UI_PHASE_NEAR_DEATH_CHOICE)
        return;

    UINearDeathOptionType selected = UI_NEAR_DEATH_ESCAPE;
    if (index == 1 && g_ui_near_death_can_ghost)
        selected = UI_NEAR_DEATH_GHOST;
    else if ((index == 1 && !g_ui_near_death_can_ghost) || index >= 2)
        selected = UI_NEAR_DEATH_ACCEPT;

    if (selected == UI_NEAR_DEATH_ESCAPE)
    {
        int cost = UI_EscapeCultivationCost();
        if (YOU.cultivation >= cost)
        {
            YOU.cultivation -= cost;
        }
        else
        {
            YOU.cultivation = 0;
            YOU.cursed += Yuan[YOU.XIUWEI] * 3;
            YOU.bleeding += Yuan[YOU.XIUWEI] * 2;
        }
        YOU.HP = UI_MaxInt(1, Player_MaxHP(&YOU) / 4);
        YOU.QI = 0;
        YOU.current_action_type = ACTION_TYPE_NONE;
        Player_ClampVitals(&YOU);
        UI_AddLog("燃尽遁光脱离死局，未得战利品。");
        UI_AddChronicle("escape", "濒死遁逃，舍弃战利以续道途。");
        g_ui_near_death_can_ghost = false;
        g_ui_near_death_reason = "";
        UI_GenerateRouteOptions();
        return;
    }

    if (selected == UI_NEAR_DEATH_GHOST)
    {
        YOU.soul_state = SOUL_GHOST;
        YOU.HP = UI_MaxInt(1, Player_MaxHP(&YOU) / 2);
        YOU.QI = 0;
        YOU.current_action_type = ACTION_TYPE_NONE;
        Player_ClampVitals(&YOU);
        UI_AddLog("阴魂不散，暂以鬼魂之身续行道途。");
        UI_AddChronicle("ghost", "肉身崩毁，阴魂不散，转入鬼修之途。");
        g_ui_near_death_can_ghost = false;
        g_ui_near_death_reason = "";
        UI_GenerateRouteOptions();
        return;
    }

    UI_SetEnding("defeat", "道途暂止", "你选择认命，此世修行止步于此。");
    UI_AddLog("道途暂止，你被击败了。");
}

static void UI_ResolveTribulation(UITribulationChoice choice)
{
    if (g_ui_phase != UI_PHASE_TRIBULATION_EVENT)
        return;
    if (choice == UI_TRIBULATION_PROTECT && !UI_TribulationProtectAvailable())
    {
        UI_AddLog("护身避劫所需资源不足。");
        return;
    }
    if (choice == UI_TRIBULATION_SUPPRESS)
    {
        g_ui_calamity_debt += 2;
        g_ui_tribulation_pressure += 4;
        int loss = Player_MaxHP(&YOU) / 5;
        YOU.HP -= loss;
        if (YOU.HP <= 0)
            YOU.HP = 1;
        Player_ClampVitals(&YOU);
        UI_AddLog("强行压劫，劫云暂退，反噬仍留在道基之中。");
        UI_AddChronicle("tribulation", "强行压劫，劫气更深。");
        g_ui_phase = UI_PHASE_BETWEEN_BATTLES;
        return;
    }

    bool protected_crossing = choice == UI_TRIBULATION_PROTECT;
    int chance = UI_TribulationSuccessChance(protected_crossing);
    if (protected_crossing)
    {
        YOU.cultivation -= UI_TribulationProtectCultivationCost();
        YOU.spirit_materials -= UI_TribulationProtectSpiritCost();
        if (YOU.cultivation < 0) YOU.cultivation = 0;
        if (YOU.spirit_materials < 0) YOU.spirit_materials = 0;
    }
    if ((int)(Run_Rand() % 100) < chance)
    {
        int cultivation_gain = Yuan[YOU.XIUWEI] * (protected_crossing ? 5 : 8);
        int spirit_gain = protected_crossing ? 2 + YOU.XIUWEI : 4 + YOU.XIUWEI * 2;
        YOU.cultivation += cultivation_gain;
        YOU.spirit_materials += spirit_gain;
        if (!protected_crossing)
        {
            YOU.herb_materials += 2;
            RPG_AddMinorUnderstanding(&YOU, 1);
            UI_TryInsightSkill(RARITY_EPIC, true, "雷劫洗身后，天机化作一线高阶功法。");
            UI_UnlockAlchemyRecipe(ELIXIR_BREAKTHROUGH, "雷劫洗身，");
        }
        else
            UI_UnlockAlchemyRecipe(ELIXIR_MERIDIAN_GUARD, "护身避劫后，");
        g_ui_calamity_debt = protected_crossing ? g_ui_calamity_debt / 2 : 0;
        g_ui_tribulation_pressure = protected_crossing ? g_ui_tribulation_pressure / 2 : 0;
        Player_ClampVitals(&YOU);
        char message[192];
        snprintf(message, sizeof(message), "%s成功，修为 +%d，灵材 +%d。",
                 protected_crossing ? "护身避劫" : "渡劫", cultivation_gain, spirit_gain);
        UI_AddLog(message);
        UI_AddChronicle("tribulation", protected_crossing ? "护身避劫成功，劫云散去大半。" : "正面渡劫成功，劫气尽散。");
        g_ui_phase = UI_PHASE_BETWEEN_BATTLES;
        return;
    }

    if (YOU.XIUWEI >= REFINEMENT)
    {
        UI_SetEnding("tribulation_death", "身死道销", protected_crossing ? "护身阵未能挡住天雷，元神俱灭。" : "雷劫落下，肉身元神皆化飞灰。");
        UI_AddLog("雷劫失败，身死道销。");
        UI_AddCombatFeed("system", "defeat", "雷劫失败，身死道销。");
        return;
    }

    int cultivation_loss = UI_MaxInt(Yuan[YOU.XIUWEI] * 8, UI_BreakthroughCost(&YOU) / 3);
    int hp_loss = Player_MaxHP(&YOU) * (protected_crossing ? 25 : 40) / 100;
    YOU.cultivation -= cultivation_loss;
    if (YOU.cultivation < 0)
        YOU.cultivation = 0;
    YOU.HP -= hp_loss;
    if (YOU.HP <= 0)
        YOU.HP = 1;
    YOU.QI = 0;
    g_ui_calamity_debt = UI_MaxInt(0, g_ui_calamity_debt / 2);
    g_ui_tribulation_pressure = UI_MaxInt(0, g_ui_tribulation_pressure / 2);
    Player_ClampVitals(&YOU);
    char message[192];
    snprintf(message, sizeof(message), "小雷劫失败，修为 -%d，元神重创但道途未断。", cultivation_loss);
    UI_AddLog(message);
    UI_AddCombatFeed("system", "tribulation", message);
    UI_AddChronicle("tribulation", "小雷劫失败，元神重创，道途侥幸未断。");
    UI_UnlockAlchemyRecipe(ELIXIR_EXORCISM, "小雷劫余烬中，");
    g_ui_phase = UI_PHASE_BETWEEN_BATTLES;
}

static void UI_SkipBreakthrough(void)
{
    if (g_ui_phase == UI_PHASE_BREAKTHROUGH_CHOICE || g_ui_phase == UI_PHASE_HEART_DEMON_CHOICE)
    {
        UI_AddLog("暂缓突破，继续积蓄修为。");
        g_ui_breakthrough_offered = false;
        UI_EnterPreparation();
    }
}

static void UI_ApplyRouteImmediateReward(const UIRouteTemplate *route)
{
    if (route->reward_bias == UI_ROUTE_REWARD_REBIRTH)
    {
        int cost = UI_RebirthCultivationCost();
        if (route->special_rule == UI_ROUTE_SPECIAL_GHOST_MARKET_RITE)
            cost = cost * 3 / 4;
        if (YOU.soul_state == SOUL_GHOST && YOU.cultivation >= cost)
        {
            YOU.cultivation -= cost;
            YOU.soul_state = SOUL_BODY;
            g_ui_has_reborn = true;
            YOU.HP = UI_MaxInt(1, Player_MaxHP(&YOU) / 2);
            YOU.QI = 0;
            Player_ClampVitals(&YOU);
            UI_AddLog("还阳坛阴火退散，肉身重凝。");
            UI_AddChronicle("rebirth", "还阳坛阴火退散，肉身重凝。");
            return;
        }
        int gain = Yuan[YOU.XIUWEI] * 4;
        if (route->special_rule == UI_ROUTE_SPECIAL_GHOST_MARKET_RITE)
        {
            YOU.herb_materials += 3;
            gain += Yuan[YOU.XIUWEI] * 2;
        }
        YOU.cultivation += gain;
        UI_AddLog("还阳机缘未足，坛火暂护残魂并转为修为。");
        return;
    }

    RewardChoice reward = route->immediate_reward;
    if (reward.type == REWARD_CULTIVATION)
    {
        YOU.cultivation += reward.amount;
        UI_AddLog("路线收益化作修为。");
    }
    else if (reward.type == REWARD_ELIXIR)
    {
        for (int i = 0; i < reward.amount && YOU.elixir_count < MAX_ELIXIR_SLOTS; i++)
            UI_AddElixirDirect((ElixirID)reward.id);
        UI_AddLog("路线途中获得丹药。");
    }
    else if (reward.type == REWARD_ARTIFACT)
    {
        if (YOU.artifact_count < MAX_ARTIFACT_SLOTS && !Player_HasArtifact(&YOU, (ArtifactID)reward.id))
            UI_AddArtifactDirect((ArtifactID)reward.id);
        UI_AddLog("路线途中获得法器线索。");
    }

    if (route->reward_bias == UI_ROUTE_REWARD_SKILL)
    {
        RewardRarity rarity = Player_HasTalent(&YOU, TALENT_CAVE_INSIGHT) ? RARITY_EPIC : RARITY_RARE;
        UI_TryInsightSkill(rarity, true, "洞府石刻残留着一线功法。");
    }
    else if (route->reward_bias == UI_ROUTE_REWARD_SWORD)
    {
        UI_TryInsightSkill(RARITY_RARE, true, "剑冢余意入心，化作一门剑诀。");
    }
    if (strcmp(route->title, "丹霞药谷") == 0)
        UI_UnlockAlchemyRecipe(ELIXIR_WOOD_SPIRIT, "丹霞药谷中，");
    else if (strcmp(route->title, "灵兽山") == 0)
        UI_UnlockAlchemyRecipe(ELIXIR_WOOD_SPIRIT, "灵兽山药性启发，");
    else if (strcmp(route->title, "魔修遗迹") == 0)
        UI_UnlockAlchemyRecipe(ELIXIR_BERSERK, "魔修遗迹残方中，");
    else if (strcmp(route->title, "古修洞府") == 0)
        UI_UnlockAlchemyRecipe(ELIXIR_FOCUS_INSIGHT, "洞府玉简中，");
    else if (strcmp(route->title, "宗门试炼") == 0)
        UI_UnlockAlchemyRecipe(ELIXIR_FOCUS_INSIGHT, "宗门残阵中，");
    else if (strcmp(route->title, "阴市") == 0 || strcmp(route->title, "乱葬岗") == 0)
        UI_UnlockAlchemyRecipe(ELIXIR_EXORCISM, "幽冥阴市里，");
    else if (strcmp(route->title, "天劫残云") == 0)
    {
        UI_UnlockAlchemyRecipe(ELIXIR_MERIDIAN_GUARD, "劫云残痕中，");
        if (route->risk >= 2)
            UI_UnlockAlchemyRecipe(ELIXIR_BREAKTHROUGH, "天劫残云深处，");
    }
    if (route->special_rule == UI_ROUTE_SPECIAL_SWORD_TRIAL)
        UI_TryInsightSkill(RARITY_EPIC, true, "剑修试炼破阵成功，剑意化作真传。");
    else if (route->special_rule == UI_ROUTE_SPECIAL_ALCHEMY_CAULDRON)
    {
        YOU.herb_materials += 5 + Player_HasTalent(&YOU, TALENT_ALCHEMY_RESIDUE);
        if (YOU.elixir_count < MAX_ELIXIR_SLOTS)
            UI_AddElixirDirect(ELIXIR_DISASTER_WARD);
        const ElixirID cauldron_recipes[] = {ELIXIR_MERIDIAN_GUARD, ELIXIR_EXORCISM, ELIXIR_FOCUS_INSIGHT};
        UI_UnlockFirstAlchemyRecipe(cauldron_recipes, 3, "丹炉开劫后，");
        UI_AdjustTribulationPressure(-1);
        UI_AddLog("丹炉开劫成丹，药材丰收，劫意稍缓。");
        UI_AddChronicle("route", "丹炉开劫成丹，药香一夜越过山谷。");
    }
    else if (route->special_rule == UI_ROUTE_SPECIAL_DEMON_CONTRACT)
    {
        int gain = Yuan[YOU.XIUWEI] * 4;
        YOU.cultivation += gain;
        g_ui_calamity_debt += 1;
        g_ui_tribulation_pressure += 1;
        UI_TryInsightSkill(RARITY_EPIC, true, "魔心旧契换来一线禁忌功法。");
        UI_UnlockAlchemyRecipe(ELIXIR_BERSERK, "魔心旧契中，");
        UI_AddLog("魔心旧契兑现，禁忌感悟入身，劫气也更深。");
        UI_AddChronicle("route", "魔心旧契入魂，禁忌传承与劫气一同落下。");
    }
    else if (route->special_rule == UI_ROUTE_SPECIAL_SECT_TOURNAMENT)
    {
        YOU.cultivation += Yuan[YOU.XIUWEI] * 2;
        UI_TryInsightSkill(RARITY_EPIC, true, "内门大比胜出，诸峰传承择主。");
        UI_UnlockAlchemyRecipe(ELIXIR_FOCUS_INSIGHT, "内门大比后，");
        UI_AddLog("内门大比幻境破阵，功法感悟更加清晰。");
        UI_AddChronicle("route", "内门大比胜出，诸峰残影同时稽首。");
    }
    else if (route->special_rule == UI_ROUTE_SPECIAL_MORTAL_FAREWELL)
    {
        YOU.cultivation += Yuan[YOU.XIUWEI] * 3;
        RPG_AddMinorUnderstanding(&YOU, 1);
        if (g_ui_calamity_debt > 0)
            g_ui_calamity_debt--;
        UI_AdjustTribulationPressure(-2);
        UI_AddLog("故人归尘，道心沉静，劫气与雷劫压力稍减。");
        UI_AddChronicle("route", "故人归尘，道心在离别里沉静下来。");
    }

    switch (route->reward_bias)
    {
    case UI_ROUTE_REWARD_ELIXIR:
    case UI_ROUTE_REWARD_REBIRTH:
        YOU.herb_materials += 2 + Player_HasTalent(&YOU, TALENT_ALCHEMY_RESIDUE);
        UI_AddLog("路线途中采得一批药材。");
        break;
    case UI_ROUTE_REWARD_ARTIFACT:
    case UI_ROUTE_REWARD_THUNDER:
    case UI_ROUTE_REWARD_SWORD:
        YOU.spirit_materials += 2;
        UI_AddLog("路线途中寻得一批灵材。");
        break;
    case UI_ROUTE_REWARD_SKILL:
        YOU.herb_materials += 1;
        YOU.spirit_materials += 1;
        UI_AddLog("洞府余藏中还有些灵材药材。");
        break;
    default:
        break;
    }
    if (route->special_rule == UI_ROUTE_SPECIAL_BLOOD_BEAST_DEN)
    {
        YOU.spirit_materials += 3;
        UI_AddLog("血煞兽窟深处还有额外灵材。");
    }
}

static void UI_ChooseRoute(int index)
{
    if (g_ui_phase != UI_PHASE_ROUTE_CHOICE)
        return;
    if (index < 0 || index >= REWARD_CHOICE_COUNT)
        index = 0;
    g_ui_next_route = g_ui_route_options[index].data;
    g_ui_has_next_route = true;
    int template_id = g_ui_route_options[index].template_id;
    if (template_id == g_ui_last_route_template_id)
        g_ui_route_streak++;
    else
        g_ui_route_streak = 1;
    g_ui_last_route_template_id = template_id;
    if (g_ui_next_route.special_rule == UI_ROUTE_SPECIAL_SWORD_TRIAL)
        g_ui_sword_tomb_chain = 0;
    else if (template_id == 5)
        g_ui_sword_tomb_chain++;
    else
        g_ui_sword_tomb_chain = 0;
    if (g_ui_next_route.special_rule == UI_ROUTE_SPECIAL_BLOOD_BEAST_DEN)
        g_ui_beast_den_chain = 0;
    else if (template_id == 2)
        g_ui_beast_den_chain++;
    else
        g_ui_beast_den_chain = 0;
    if (g_ui_next_route.special_rule == UI_ROUTE_SPECIAL_GHOST_MARKET_RITE)
        g_ui_ghost_market_chain = 0;
    else if (YOU.soul_state == SOUL_GHOST && template_id == 1)
        g_ui_ghost_market_chain++;
    else if (YOU.soul_state != SOUL_GHOST)
        g_ui_ghost_market_chain = 0;
    if (g_ui_next_route.special_rule == UI_ROUTE_SPECIAL_ALCHEMY_CAULDRON)
        g_ui_herb_valley_chain = 0;
    else if (template_id == 6)
        g_ui_herb_valley_chain++;
    else
        g_ui_herb_valley_chain = 0;
    if (g_ui_next_route.special_rule == UI_ROUTE_SPECIAL_DEMON_CONTRACT)
        g_ui_demon_ruins_chain = 0;
    else if (template_id == 7)
        g_ui_demon_ruins_chain++;
    else
        g_ui_demon_ruins_chain = 0;
    if (g_ui_next_route.special_rule == UI_ROUTE_SPECIAL_SECT_TOURNAMENT)
        g_ui_sect_trial_chain = 0;
    else if (template_id == 9)
        g_ui_sect_trial_chain++;
    else
        g_ui_sect_trial_chain = 0;
    if (g_ui_next_route.special_rule == UI_ROUTE_SPECIAL_MORTAL_FAREWELL)
        g_ui_mortal_world_chain = 0;
    else if (template_id == 10)
        g_ui_mortal_world_chain++;
    else
        g_ui_mortal_world_chain = 0;
    g_ui_calamity_debt += UI_RouteDebtDelta(&g_ui_next_route);
    if (g_ui_calamity_debt < 0)
        g_ui_calamity_debt = 0;
    if (g_ui_next_route.cultivation_cost > 0)
    {
        YOU.cultivation -= g_ui_next_route.cultivation_cost;
        if (YOU.cultivation < 0)
            YOU.cultivation = 0;
    }
    if (g_ui_next_route.hp_cost_pct > 0)
    {
        int loss = Player_MaxHP(&YOU) * UI_EffectiveRouteHpCostPct(&g_ui_next_route) / 100;
        YOU.HP -= loss;
        if (YOU.HP <= 0 && YOU.soul_state == SOUL_GHOST)
        {
            UI_SetEnding("soul_scattered", "魂飞魄散", "路线劫数击散魂火，本次鬼修道途终结。");
            UI_AddLog("路线劫数击散魂火，魂飞魄散。");
            return;
        }
        if (YOU.HP <= 0)
            YOU.HP = 1;
    }
    UI_ApplyRouteImmediateReward(&g_ui_next_route);
    UI_AdvanceYearsForRoute(&g_ui_next_route);
    if (UI_CheckLifespanEnd())
        return;
    if (Player_HasTalent(&YOU, TALENT_WANDERER) && g_ui_next_route.risk <= 0)
    {
        int gain = Yuan[YOU.XIUWEI];
        YOU.cultivation += gain;
        UI_AddLog("行脚老成，从安稳路线中沉淀了额外修为。");
    }
    UI_AddLog(g_ui_next_route.title);
    {
        char chronicle[192];
        snprintf(chronicle, sizeof(chronicle), "%s", UI_RouteChronicleText(&g_ui_next_route));
        UI_AddChronicle("route", chronicle);
    }
    UI_MaybeTriggerTribulation(&g_ui_next_route);
}

static void UI_UseElixirSlot(int slot)
{
    if (g_ui_phase != UI_PHASE_BATTLE_ACTION)
        return;
    if (slot >= 0 && slot < YOU.elixir_count && !YOU.elixir_used_this_turn)
    {
        ElixirID id = YOU.elixirs[slot];
        g_ui_last_elixir_slot = slot;
        Use_Elixir(&YOU, slot);
        if (id == ELIXIR_EXORCISM)
        {
            if (g_ui_calamity_debt > 0)
                g_ui_calamity_debt--;
            UI_AdjustTribulationPressure(-2);
            UI_AddLog("辟邪丹压下业力，雷劫压力下降。");
        }
        UI_AddLog("服用了一枚丹药。");
    }
}

static int UI_ParseCommandValue(const char *line, const char *key, int fallback)
{
    const char *pos = strstr(line, key);
    if (!pos)
        return fallback;
    pos += strlen(key);
    while (*pos == ' ' || *pos == ':' || *pos == '=' || *pos == '"' || *pos == '\t')
        pos++;
    return atoi(pos);
}

static int Run_UI_JSON_Mode(void)
{
    g_config.run_mode = 1;
    g_config.auto_reward = 0;
    srand(g_config.reward_seed ? g_config.reward_seed : (unsigned int)time(NULL));
    UI_AddLog("UI JSON 模式已启动。");
    UI_StartRun();
    UI_PrintSnapshot();

    char line[256];
    while (fgets(line, sizeof(line), stdin))
    {
        if (strstr(line, "start_run") || strstr(line, "reset_run"))
            UI_StartRun();
        else if (strstr(line, "choose_action"))
            UI_ProcessBattleAction(UI_ParseCommandValue(line, "action_type", UI_ParseCommandValue(line, "slot", -1)));
        else if (strstr(line, "use_elixir"))
            UI_UseElixirSlot(UI_ParseCommandValue(line, "slot", -1));
        else if (strstr(line, "choose_reward"))
            UI_ChooseReward(UI_ParseCommandValue(line, "index", 0));
        else if (strstr(line, "choose_replacement"))
            UI_ChooseReplacement(UI_ParseCommandValue(line, "slot", -1));
        else if (strstr(line, "upgrade_artifact"))
            UI_UpgradeArtifact(UI_ParseCommandValue(line, "slot", -1));
        else if (strstr(line, "brew_elixir"))
            UI_BrewElixirRecipe(UI_ParseCommandValue(line, "recipe", -1));
        else if (strstr(line, "skip_preparation"))
            UI_SkipPreparation();
        else if (strstr(line, "equip_skill"))
            UI_EquipManagedSkill(UI_ParseCommandValue(line, "id", SKILL_ID_NONE));
        else if (strstr(line, "refine_skill"))
            UI_RefineSkill(UI_ParseCommandValue(line, "id", SKILL_ID_NONE));
        else if (strstr(line, "attempt_breakthrough"))
            UI_AttemptBreakthrough();
        else if (strstr(line, "choose_heart_demon"))
            UI_ResolveHeartBreakthrough((UIHeartDemonMode)UI_ParseCommandValue(line, "mode", 0));
        else if (strstr(line, "skip_breakthrough"))
            UI_SkipBreakthrough();
        else if (strstr(line, "choose_talent"))
            UI_ChooseTalent(UI_ParseCommandValue(line, "index", 0));
        else if (strstr(line, "choose_near_death"))
            UI_ChooseNearDeath(UI_ParseCommandValue(line, "index", 0));
        else if (strstr(line, "choose_tribulation"))
            UI_ResolveTribulation((UITribulationChoice)UI_ParseCommandValue(line, "choice", 0));
        else if (strstr(line, "choose_route"))
            UI_ChooseRoute(UI_ParseCommandValue(line, "index", 0));
        else if (strstr(line, "continue"))
            UI_Continue();
        else
            UI_AddLog("未知 UI 命令。");
        UI_PrintSnapshot();
    }
    return 0;
}
