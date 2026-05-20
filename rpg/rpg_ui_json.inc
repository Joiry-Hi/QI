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
    UI_PHASE_NEAR_DEATH_CHOICE,
    UI_PHASE_ENCOUNTER,
    UI_PHASE_BETWEEN_BATTLES,
    UI_PHASE_VICTORY,
    UI_PHASE_DEFEAT
} UIPhase;

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
    UI_ROUTE_SPECIAL_NONE,
    UI_ROUTE_SPECIAL_SWORD_TRIAL,
    UI_ROUTE_SPECIAL_BLOOD_BEAST_DEN,
    UI_ROUTE_SPECIAL_GHOST_MARKET_RITE
} UIRouteSpecialRule;

typedef enum {
    UI_HEART_STEADY,
    UI_HEART_RISKY,
    UI_HEART_SUPPRESS
} UIHeartDemonMode;

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
static int g_ui_calamity_debt = 0;
static int g_ui_age = 0;
static int g_ui_last_years_elapsed = 0;
static int g_ui_tribulation_pressure = 0;
static bool g_ui_has_reborn = false;
static char g_ui_ending_type[32] = "";
static char g_ui_ending_title[64] = "";
static char g_ui_ending_summary[256] = "";
static int g_ui_chronicle_age[16];
static char g_ui_chronicle_realm[16][32];
static char g_ui_chronicle_kind[16][24];
static char g_ui_chronicle_text[16][256];
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

static const int UI_GHOST_ROUTE_TEMPLATE_COUNT = (int)(sizeof(UI_GHOST_ROUTE_TEMPLATES) / sizeof(UI_GHOST_ROUTE_TEMPLATES[0]));

static void UI_PrintEscaped(const char *text);

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
    case REFINING: return 200;
    case FOUNDATION: return 350;
    case CORE_FORM: return 650;
    case NASCENT_SOUL: return 1200;
    case SEVERING: return 2400;
    case REFINEMENT: return 4800;
    case UNITY: return 9000;
    case GREAT: return 16000;
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
    if (g_ui_tribulation_pressure >= 12)
        return "雷劫压力极高，Boss 与天劫路线会明显更危险。";
    if (g_ui_tribulation_pressure >= 8)
        return "劫云已厚，高风险路线收益提高但反噬更重。";
    if (g_ui_tribulation_pressure >= 4)
        return "天机开始偏移，可用清修或雷劫亲和压住压力。";
    return "雷劫尚远，可以按构筑节奏择路。";
}

static int UI_TribulationBossRealmBonus(void)
{
    int next_battle = g_run.battle_index + 1;
    if (next_battle <= 0 || next_battle % 6 != 0)
        return 0;
    return g_ui_tribulation_pressure / 8;
}

static void UI_AddChronicle(const char *kind, const char *text)
{
    if (!text || !text[0])
        return;
    int index;
    if (g_ui_chronicle_count < 16)
    {
        index = g_ui_chronicle_count++;
    }
    else
    {
        memmove(g_ui_chronicle_age, g_ui_chronicle_age + 1, sizeof(g_ui_chronicle_age[0]) * 15);
        memmove(g_ui_chronicle_realm, g_ui_chronicle_realm + 1, sizeof(g_ui_chronicle_realm[0]) * 15);
        memmove(g_ui_chronicle_kind, g_ui_chronicle_kind + 1, sizeof(g_ui_chronicle_kind[0]) * 15);
        memmove(g_ui_chronicle_text, g_ui_chronicle_text + 1, sizeof(g_ui_chronicle_text[0]) * 15);
        index = 15;
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

static int UI_RouteYearAdvance(const UIRouteTemplate *route)
{
    int yuan = Yuan[YOU.XIUWEI] > 0 ? Yuan[YOU.XIUWEI] : 1;
    int base = 3 + yuan * 2;
    int variance = 3 + yuan;
    if (base > 80)
        base = 80 + yuan / 4;
    if (variance > 40)
        variance = 40;
    int years = base + (int)(Run_Rand() % variance);
    if (route)
    {
        if (route->reward_bias == UI_ROUTE_REWARD_CULTIVATION || route->reward_bias == UI_ROUTE_REWARD_SKILL)
            years += 2 + yuan / 2;
        if (route->risk >= 2)
            years -= 1 + yuan / 4;
        if (route->special_rule != UI_ROUTE_SPECIAL_NONE)
            years += 2;
    }
    if (YOU.XIUWEI == MORTAL && years > 10)
        years = 10;
    if (years < 3)
        years = 3;
    return years;
}

static int UI_RouteYearPreview(const UIRouteTemplate *route)
{
    int yuan = Yuan[YOU.XIUWEI] > 0 ? Yuan[YOU.XIUWEI] : 1;
    int base = 3 + yuan * 2;
    if (base > 80)
        base = 80 + yuan / 4;
    int years = base + (3 + yuan) / 2;
    if (route)
    {
        if (route->reward_bias == UI_ROUTE_REWARD_CULTIVATION || route->reward_bias == UI_ROUTE_REWARD_SKILL)
            years += 2 + yuan / 2;
        if (route->risk >= 2)
            years -= 1 + yuan / 4;
        if (route->special_rule != UI_ROUTE_SPECIAL_NONE)
            years += 2;
    }
    if (YOU.XIUWEI == MORTAL && years > 10)
        years = 10;
    if (years < 3)
        years = 3;
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
            delta += 2;
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

static int UI_BossDebtRealmBonus(void)
{
    int next_battle = g_run.battle_index + 1;
    if (next_battle > 0 && next_battle % 6 == 0)
        return g_ui_calamity_debt / 3 + UI_TribulationBossRealmBonus();
    return 0;
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
        offset += 1 + UI_BossDebtRealmBonus();
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

static void UI_SetEnemyIntent(const char *label, const char *hint, int confidence)
{
    g_ui_enemy_intent_label = label;
    g_ui_enemy_intent_hint = hint;
    if (confidence < 0) confidence = 0;
    if (confidence > 100) confidence = 100;
    g_ui_enemy_intent_confidence = confidence;
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
    printf("\",\"reward_bias\":\"");
    UI_PrintEscaped(g_ui_has_next_route ? UI_RouteBiasName(g_ui_next_route.reward_bias) : "均衡");
    printf("\",\"calamity_debt\":%d,\"debt_realm_bonus\":%d,\"tribulation_pressure\":%d,\"special_rule\":\"",
           g_ui_calamity_debt, UI_BossDebtRealmBonus(), g_ui_tribulation_pressure);
    UI_PrintEscaped(g_ui_has_next_route ? UI_RouteSpecialName(g_ui_next_route.special_rule) : "");
    printf("\",\"threat\":\"");
    if (g_ui_has_next_route)
    {
        char threat[256];
        snprintf(threat, sizeof(threat), "%s 劫气 %d，雷劫%s%s", g_ui_next_route.desc, g_ui_calamity_debt,
                 UI_TribulationLabel(), UI_BossDebtRealmBonus() > 0 ? "，Boss 境界受压力抬升。" : "。");
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

static void UI_PrintRouteChoiceJSON(void)
{
    printf("\"route_options\":[");
    for (int i = 0; g_ui_phase == UI_PHASE_ROUTE_CHOICE && i < REWARD_CHOICE_COUNT; i++)
    {
        if (i) printf(",");
        const UIRouteTemplate *route = &g_ui_route_options[i].data;
        int next_battle = g_run.battle_index + 1;
        int enemy_realm = UI_ClampRealm((int)YOU.XIUWEI + route->realm_offset +
                                        ((next_battle > 0 && next_battle % 6 == 0) ? 1 + UI_BossDebtRealmBonus() : 0));
        int material_spirit = 2 + (route->reward_bias == UI_ROUTE_REWARD_ARTIFACT || route->reward_bias == UI_ROUTE_REWARD_SWORD || route->reward_bias == UI_ROUTE_REWARD_THUNDER ? 2 : 0) + (route->special_rule == UI_ROUTE_SPECIAL_BLOOD_BEAST_DEN ? 3 : 0);
        int material_herb = 1 + (route->reward_bias == UI_ROUTE_REWARD_ELIXIR || route->reward_bias == UI_ROUTE_REWARD_REBIRTH ? 2 : 0);
        int years = UI_RouteYearPreview(route);
        int predicted_age = g_ui_age + years;
        int predicted_pressure = g_ui_tribulation_pressure + (route->risk >= 2 ? 2 : (route->risk <= 0 ? -1 : 0)) + YOU.XIUWEI / 2;
        if (predicted_pressure < 0) predicted_pressure = 0;
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
        printf("\",\"material_preview\":\"灵材 +%d / 药材 +%d\",\"years_preview\":%d,\"predicted_age\":%d,\"predicted_tribulation\":%d,\"enemy_hint\":\"",
               material_spirit, material_herb, years, predicted_age, predicted_pressure);
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
    for (int i = 0; i < Alchemy_RecipeCount(); i++)
    {
        const AlchemyRecipe *recipe = Alchemy_GetRecipe(i);
        if (!recipe)
            continue;
        if (i) printf(",");
        const Elixir *elixir = &g_elixir_database[recipe->elixir_id];
        printf("{\"index\":%d,\"id\":%d,\"name\":\"", recipe->index, recipe->elixir_id);
        UI_PrintEscaped(elixir->name_chn ? elixir->name_chn : recipe->name);
        printf("\",\"desc\":\"");
        UI_PrintEscaped(elixir->desc_chn ? elixir->desc_chn : "炼制丹药。");
        printf("\",\"cost\":%d,\"required_realm\":\"", recipe->herb_cost);
        UI_PrintEscaped(Realm[recipe->required_realm]);
        printf("\",\"unlocked\":%s,\"can_brew\":%s,\"custom\":%s}",
               Alchemy_RecipeUnlocked(&YOU, recipe) ? "true" : "false",
               Alchemy_CanBrew(&YOU, recipe) ? "true" : "false",
               elixir->is_custom ? "true" : "false");
    }
    printf("]}");
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
    printf("\"tribulation\":{\"pressure\":%d,\"label\":\"", g_ui_tribulation_pressure);
    UI_PrintEscaped(UI_TribulationLabel());
    printf("\",\"boss_modifier\":%d,\"hint\":\"", UI_TribulationBossRealmBonus());
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
    printf(",\"route_meta\":{\"calamity_debt\":%d,\"last_route\":\"", g_ui_calamity_debt);
    UI_PrintEscaped(g_ui_has_next_route ? g_ui_next_route.title : "未择路线");
    printf("\",\"route_streak\":%d,\"sword_chain\":%d,\"beast_chain\":%d,\"ghost_market_chain\":%d,\"hint\":\"",
           g_ui_route_streak, g_ui_sword_tomb_chain, g_ui_beast_den_chain, g_ui_ghost_market_chain);
    if (g_ui_calamity_debt >= 6)
        UI_PrintEscaped("劫气深重：奖励更好，但 Boss 会明显变危险。");
    else if (g_ui_sword_tomb_chain >= 2)
        UI_PrintEscaped("剑冢剑意相连，可能触发剑修试炼。");
    else if (g_ui_beast_den_chain >= 2)
        UI_PrintEscaped("妖兽巢穴气息延伸，血煞兽窟正在成形。");
    else if (g_ui_ghost_market_chain >= 2)
        UI_PrintEscaped("阴市因果加深，还阳机会更近。");
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
        printf("{\"slot\":%d,\"id\":%d,\"level\":%d,\"upgrade_cost\":%d,\"can_upgrade\":%s,\"resonance_active\":%s,\"resonance\":\"",
               i, id, level, upgrade_cost,
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
    printf("\",\"cycle\":%d},\"last_elixir_slot\":%d,", g_ui_boss_cycle, g_ui_last_elixir_slot);
    UI_PrintReplacementJSON();
    printf(",");
    UI_PrintBreakthroughJSON();
    printf(",");
    UI_PrintHeartDemonJSON();
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
        g_ui_boss_cycle++;
        int phase_index = (g_ui_boss_cycle - 1) % 3;
        g_ui_boss_phase = phase_index == 0 ? UI_BOSS_CHARGE : (phase_index == 1 ? UI_BOSS_PRESSURE : UI_BOSS_OPENING);
        if (g_ui_boss_phase == UI_BOSS_CHARGE)
        {
            UI_SetEnemyIntent("气机暴涨", "Boss 正在蓄势，下一轮可能转入强压。", 82);
            if (can_perform_action(&CPU, ACTION_TYPE_BOOST))
                CPU.current_action_type = ACTION_TYPE_BOOST;
            else
                CPU.current_action_type = ACTION_TYPE_GAIN_QI;
            UI_AddCombatFeed("enemy", "intent", "Boss 气机暴涨，天地灵压向它汇聚。");
            return;
        }
        if (g_ui_boss_phase == UI_BOSS_PRESSURE)
        {
            UI_SetEnemyIntent("杀意凝聚", "Boss 将尝试把灵气优势转为爆发伤害。", 88);
            if (can_perform_action(&CPU, ACTION_TYPE_TERMINATE))
                CPU.current_action_type = ACTION_TYPE_TERMINATE;
            else if (can_perform_action(&CPU, ACTION_TYPE_BURST))
                CPU.current_action_type = ACTION_TYPE_BURST;
            else if (can_perform_action(&CPU, ACTION_TYPE_SMITE))
                CPU.current_action_type = ACTION_TYPE_SMITE;
            else
                CPU.current_action_type = ACTION_TYPE_GAIN_QI;
            UI_AddCombatFeed("enemy", "intent", "Boss 杀意凝聚，攻势即将落下。");
            return;
        }
        UI_SetEnemyIntent("气机回落", "Boss 大势用尽，短暂露出破绽。", 78);
        if (can_perform_action(&CPU, ACTION_TYPE_DEFEND))
            CPU.current_action_type = ACTION_TYPE_DEFEND;
        else if (can_perform_action(&CPU, ACTION_TYPE_COUNTER))
            CPU.current_action_type = ACTION_TYPE_COUNTER;
        else
            CPU.current_action_type = ACTION_TYPE_GAIN_QI;
        UI_AddCombatFeed("enemy", "intent", "Boss 气机回落，露出一线破绽。");
        return;
    }

    g_ui_boss_phase = UI_BOSS_NONE;
    switch (game.opponent_type)
    {
    case 0: CPU_logic_V0_Random(&CPU, &YOU); break;
    case 1: CPU_logic_V1A_Disruptor(&CPU, &YOU); break;
    case 2: CPU_logic_V1B_Berserker(&CPU, &YOU); break;
    case 3: CPU_logic_V1C_Turtle(&CPU, &YOU); break;
    case 4: CPU_logic_V1D_Ascetic(&CPU, &YOU); break;
    case 5: CPU_logic_V1E_Gambler(&CPU, &YOU); break;
    case 6: CPU_logic_V2A_Tuned(&CPU, &YOU, 0); break;
    default: CPU_logic_V0_Random(&CPU, &YOU); break;
    }

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
    if (!UI_IsBossBattle())
        g_ui_boss_cycle = 0;
    UI_AddCombatFeed("system", UI_IsBossBattle() ? "boss" : (UI_IsEliteBattle() ? "elite" : "info"),
                     UI_IsBossBattle() ? "强敌压境，Boss 战开始。" :
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
    g_ui_calamity_debt = 0;
    g_ui_age = 12 + (int)(Run_Rand() % 39);
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
        g_ui_tribulation_pressure += 1 + g_ui_calamity_debt / 4;
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
    int spirit_gain = 2;
    int herb_gain = 1;
    if (g_run.battle_index % 3 == 0)
    {
        spirit_gain += 1;
        herb_gain += 1;
    }
    if (g_run.battle_index % 6 == 0)
    {
        spirit_gain += 2;
        herb_gain += 1;
    }
    if (g_ui_has_next_route)
    {
        switch (g_ui_next_route.reward_bias)
        {
        case UI_ROUTE_REWARD_ARTIFACT:
        case UI_ROUTE_REWARD_THUNDER:
        case UI_ROUTE_REWARD_SWORD:
            spirit_gain += 2;
            break;
        case UI_ROUTE_REWARD_ELIXIR:
        case UI_ROUTE_REWARD_REBIRTH:
            herb_gain += 2;
            break;
        case UI_ROUTE_REWARD_SKILL:
        case UI_ROUTE_REWARD_CULTIVATION:
            spirit_gain += 1;
            herb_gain += 1;
            break;
        default:
            break;
        }
    }
    spirit_gain += g_ui_calamity_debt / 3;
    if (g_ui_calamity_debt >= 5)
        herb_gain += 1;
    if (g_ui_has_next_route && g_ui_next_route.special_rule == UI_ROUTE_SPECIAL_BLOOD_BEAST_DEN)
        spirit_gain += 3;
    if (Player_HasTalent(&YOU, TALENT_ALCHEMY_RESIDUE))
        herb_gain += 1;
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
            g_ui_pending_rewards[2] = (RewardChoice){REWARD_ELIXIR, RARITY_RARE, PickRewardElixir(RARITY_RARE), 2, false};
            break;
        case UI_ROUTE_REWARD_ARTIFACT:
            g_ui_pending_rewards[1] = (RewardChoice){REWARD_ARTIFACT, RARITY_RARE, PickRewardArtifactForPlayer(&YOU, RARITY_RARE), 1, false};
            if (g_ui_pending_rewards[1].id == ARTIFACT_NONE)
                g_ui_pending_rewards[1] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[YOU.XIUWEI] / 2, false};
            break;
        case UI_ROUTE_REWARD_SKILL:
        {
            RewardRarity rarity = Player_HasTalent(&YOU, TALENT_CAVE_INSIGHT) ? RARITY_EPIC : RARITY_RARE;
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, rarity, PickRewardSkill(&YOU, rarity, true), 1, true};
            if (g_ui_pending_rewards[0].id == SKILL_ID_NONE)
                g_ui_pending_rewards[0] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[YOU.XIUWEI] / 2, false};
            break;
        }
        case UI_ROUTE_REWARD_THUNDER:
            if (!Player_HasArtifact(&YOU, ARTIFACT_THUNDER_WOOD_SWORD))
                g_ui_pending_rewards[1] = (RewardChoice){REWARD_ARTIFACT, RARITY_RARE, ARTIFACT_THUNDER_WOOD_SWORD, 1, false};
            else
                g_ui_pending_rewards[1] = (RewardChoice){REWARD_ARTIFACT, RARITY_RARE, PickRewardArtifactForPlayer(&YOU, RARITY_RARE), 1, false};
            if (g_ui_pending_rewards[1].id == ARTIFACT_NONE)
                g_ui_pending_rewards[1] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[YOU.XIUWEI] / 2, false};
            break;
        case UI_ROUTE_REWARD_SWORD:
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_RARE, PickRewardSkill(&YOU, RARITY_RARE, true), 1, true};
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
                g_ui_pending_rewards[0] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[YOU.XIUWEI] / 2, false};
            break;
        case UI_ROUTE_REWARD_GHOST:
            g_ui_pending_rewards[0] = (RewardChoice){REWARD_SKILL, RARITY_RARE, PickRewardSkill(&YOU, RARITY_RARE, true), 1, true};
            if (g_ui_pending_rewards[0].id == SKILL_ID_NONE)
                g_ui_pending_rewards[0] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[YOU.XIUWEI] / 2, false};
            break;
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
            g_ui_pending_rewards[1] = (RewardChoice){REWARD_ARTIFACT, RARITY_EPIC, PickRewardArtifactForPlayer(&YOU, RARITY_EPIC), 1, false};
            if (g_ui_pending_rewards[1].id == ARTIFACT_NONE)
                g_ui_pending_rewards[1] = (RewardChoice){REWARD_CULTIVATION, RARITY_EPIC, 0, max_QI[YOU.XIUWEI], false};
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
    if (YOU.soul_state == SOUL_GHOST)
    {
        for (int i = 0; i < REWARD_CHOICE_COUNT; i++)
        {
            int pick = i;
            if (pick >= UI_GHOST_ROUTE_TEMPLATE_COUNT)
                pick = UI_GHOST_ROUTE_TEMPLATE_COUNT - 1;
            g_ui_route_options[i].template_id = pick;
            g_ui_route_options[i].data = UI_GHOST_ROUTE_TEMPLATES[pick];
            int scale = Yuan[YOU.XIUWEI] > 0 ? Yuan[YOU.XIUWEI] : 1;
            if (g_ui_route_options[i].data.immediate_reward.type == REWARD_CULTIVATION)
                g_ui_route_options[i].data.immediate_reward.amount *= scale;
        }
        if (g_ui_ghost_market_chain >= 2)
        {
            g_ui_route_options[0].template_id = 1002;
            g_ui_route_options[0].data = UI_SPECIAL_ROUTE_GHOST_MARKET;
            int scale = Yuan[YOU.XIUWEI] > 0 ? Yuan[YOU.XIUWEI] : 1;
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
        int scale = Yuan[YOU.XIUWEI] > 0 ? Yuan[YOU.XIUWEI] : 1;
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
        int scale = Yuan[YOU.XIUWEI] > 0 ? Yuan[YOU.XIUWEI] : 1;
        g_ui_route_options[0].data.immediate_reward.amount *= scale;
    }
    else if (g_ui_beast_den_chain >= 2)
    {
        g_ui_route_options[0].template_id = 1001;
        g_ui_route_options[0].data = UI_SPECIAL_ROUTE_BLOOD_BEAST;
        int scale = Yuan[YOU.XIUWEI] > 0 ? Yuan[YOU.XIUWEI] : 1;
        g_ui_route_options[0].data.immediate_reward.amount *= scale;
    }
    else if (g_ui_last_route_template_id == 2 && g_ui_route_streak >= 1)
    {
        g_ui_route_options[1].template_id = 1001;
        g_ui_route_options[1].data = UI_SPECIAL_ROUTE_BLOOD_BEAST;
        int scale = Yuan[YOU.XIUWEI] > 0 ? Yuan[YOU.XIUWEI] : 1;
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
    if (route->special_rule == UI_ROUTE_SPECIAL_SWORD_TRIAL)
        UI_TryInsightSkill(RARITY_EPIC, true, "剑修试炼破阵成功，剑意化作真传。");

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
        snprintf(chronicle, sizeof(chronicle), "选择%s，下一场生死搏杀将至。", g_ui_next_route.title);
        UI_AddChronicle("route", chronicle);
    }
    g_ui_phase = UI_PHASE_BETWEEN_BATTLES;
}

static void UI_UseElixirSlot(int slot)
{
    if (g_ui_phase != UI_PHASE_BATTLE_ACTION)
        return;
    if (slot >= 0 && slot < YOU.elixir_count && !YOU.elixir_used_this_turn)
    {
        g_ui_last_elixir_slot = slot;
        Use_Elixir(&YOU, slot);
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
