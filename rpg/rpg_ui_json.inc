typedef enum {
    UI_PHASE_RUN_START,
    UI_PHASE_BATTLE_ACTION,
    UI_PHASE_BATTLE_RESULT,
    UI_PHASE_REWARD_CHOICE,
    UI_PHASE_REWARD_REPLACE,
    UI_PHASE_BREAKTHROUGH_CHOICE,
    UI_PHASE_TALENT_CHOICE,
    UI_PHASE_ROUTE_CHOICE,
    UI_PHASE_NEAR_DEATH_CHOICE,
    UI_PHASE_ENCOUNTER,
    UI_PHASE_BETWEEN_BATTLES,
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
} UIRouteTemplate;

typedef struct {
    int template_id;
    UIRouteTemplate data;
} UIRouteOption;

static UIPhase g_ui_phase = UI_PHASE_RUN_START;
static RewardChoice g_ui_pending_rewards[REWARD_CHOICE_COUNT];
static RewardChoice g_ui_pending_replacement_reward;
static UIReplacementType g_ui_replacement_type = UI_REPLACE_NONE;
static int g_ui_replacement_slot_count = 0;
static int g_ui_replacement_action_type = ACTION_TYPE_NONE;
static UIRouteOption g_ui_route_options[REWARD_CHOICE_COUNT];
static TalentID g_ui_talent_options[REWARD_CHOICE_COUNT];
static UIRouteTemplate g_ui_next_route;
static bool g_ui_has_next_route = false;
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

static const UIRouteTemplate UI_ROUTE_TEMPLATES[] = {
    {"清修山径", "沿灵脉缓行，适合沉淀修为。", "低风险", 0, -1, UI_ROUTE_REWARD_CULTIVATION,
     {REWARD_CULTIVATION, RARITY_COMMON, 0, 8, false}, 0, 0},
    {"坊市交易", "散修摊位真假难辨，但补给充足。", "低风险", 0, -1, UI_ROUTE_REWARD_ELIXIR,
     {REWARD_ELIXIR, RARITY_COMMON, ELIXIR_QI, 1, false}, 0, 0},
    {"妖兽巢穴", "妖气盘踞，战利品也更丰厚。", "高风险", 2, 1, UI_ROUTE_REWARD_ARTIFACT,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 12, false}, 10, 0},
    {"古修洞府", "残缺传承仍有余韵，可能指向新的功法。", "中风险", 1, 0, UI_ROUTE_REWARD_SKILL,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 10, false}, 0, 4},
    {"天劫残云", "劫雷未散，危险与破境机缘并存。", "高风险", 2, 1, UI_ROUTE_REWARD_THUNDER,
     {REWARD_ELIXIR, RARITY_RARE, ELIXIR_BREAKTHROUGH, 1, false}, 15, 0},
    {"无名剑冢", "断剑沉埋于荒丘，剑意仍在择主。", "中风险", 1, 0, UI_ROUTE_REWARD_SWORD,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 10, false}, 8, 0},
};

static const int UI_ROUTE_TEMPLATE_COUNT = (int)(sizeof(UI_ROUTE_TEMPLATES) / sizeof(UI_ROUTE_TEMPLATES[0]));

static const UIRouteTemplate UI_GHOST_ROUTE_TEMPLATES[] = {
    {"还阳坛", "残破祭坛以阴火照骨，足够修为可换回肉身。", "还阳", 1, 0, UI_ROUTE_REWARD_REBIRTH,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 8, false}, 0, 0},
    {"阴市", "纸灯照着幽暗街巷，鬼商兜售丹药与魂契。", "幽冥", 1, 0, UI_ROUTE_REWARD_ELIXIR,
     {REWARD_ELIXIR, RARITY_RARE, ELIXIR_CLEAR_MIND, 1, false}, 0, 0},
    {"乱葬岗", "野坟间鬼火浮动，残魂记得许多失传招式。", "凶险", 2, 1, UI_ROUTE_REWARD_GHOST,
     {REWARD_CULTIVATION, RARITY_RARE, 0, 10, false}, 8, 0},
};

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
    case UI_PHASE_BREAKTHROUGH_CHOICE: return "breakthrough_choice";
    case UI_PHASE_TALENT_CHOICE: return "talent_choice";
    case UI_PHASE_ROUTE_CHOICE: return "route_choice";
    case UI_PHASE_NEAR_DEATH_CHOICE: return "near_death_choice";
    case UI_PHASE_ENCOUNTER: return "encounter";
    case UI_PHASE_BETWEEN_BATTLES: return "between_battles";
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
    chance += Player_ArtifactIntBonus(player, ArtifactBreakthroughPct);
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
    gain += Player_ArtifactIntBonus(&YOU, ArtifactPostBattleCultivation);
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
           g_ui_phase == UI_PHASE_BETWEEN_BATTLES ||
           g_ui_phase == UI_PHASE_BREAKTHROUGH_CHOICE ||
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
    printf("\",\"hp\":%d,\"max_hp\":%d,\"qi\":%d,\"max_qi\":%d,\"atk\":%d,\"yuan\":%d,\"cultivation\":%d,\"realm\":\"",
           player->HP, Player_MaxHP(player), player->QI, Player_MaxQI(player), player->ATK, player->YUAN, player->cultivation);
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
    printf("\",\"threat\":\"");
    if (g_ui_has_next_route)
        UI_PrintEscaped(g_ui_next_route.desc);
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
        printf("\",\"hp_cost_pct\":%d,\"cultivation_cost\":%d,\"immediate_reward\":",
               UI_EffectiveRouteHpCostPct(route), route->cultivation_cost);
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
        printf("{\"id\":%d,\"name\":\"", skill->skill_id);
        UI_PrintEscaped(skill->name_chn);
        printf("\",\"action_type\":%d,\"action\":\"", skill->action_type);
        UI_PrintEscaped(UI_ActionName(skill->action_type));
        printf("\",\"school\":\"");
        UI_PrintEscaped(SchoolTagName(skill->school_tag));
        printf("\",\"rank\":%d,\"cost\":%d,\"qi_cost\":%d,\"equipped\":%s,\"bypassed\":%s,\"custom\":%s,\"desc\":\"",
               skill->rank, UI_RespecSkillCost(skill->skill_id), skill->cost,
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

static void UI_PrintSnapshot(void)
{
    BuildProfile profile = Player_GetBuildProfile(&YOU);
    printf("##UI_JSON##:{\"phase\":\"%s\",\"round\":%d,\"run\":{\"battle_index\":%d,\"elite_kills\":%d,\"boss_kills\":%d},",
           UI_PhaseName(g_ui_phase), game.round_number, g_run.battle_index, g_run.elite_kills, g_run.boss_kills);
    UI_PrintPlayerJSON("player", &YOU);
    printf(",");
    UI_PrintPlayerJSON("enemy", &CPU);

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
    printf("\"}");

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
        printf("{\"slot\":%d,\"action_type\":%d,\"skill_id\":%d,\"name\":\"", i, skill->action_type, skill->skill_id);
        UI_PrintEscaped(skill->name_chn);
        printf("\",\"cost\":%d,\"rank\":%d,\"school\":\"", skill->cost, skill->rank);
        UI_PrintEscaped(SchoolTagName(skill->school_tag));
        printf("\",\"available\":%s,\"custom\":%s,\"effect_summary\":\"", YOU.QI >= skill->cost ? "true" : "false",
               skill->is_custom ? "true" : "false");
        UI_PrintEscaped(effect_summary);
        printf("\"}");
    }
    printf("]");

    printf(",\"artifacts\":[");
    for (int i = 0; i < YOU.artifact_count; i++)
    {
        ArtifactID id = YOU.artifacts[i];
        if (i) printf(",");
        printf("{\"id\":%d,\"name\":\"", id);
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
    g_ui_replacement_slot_count = 0;
    g_ui_replacement_action_type = ACTION_TYPE_NONE;
    g_ui_has_next_route = false;
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
    UI_StartBattle();
}

static int UI_SetPlayerAction(int action_type)
{
    if (action_type < 0 || action_type >= TOTAL_ACTION_TYPES)
        return 0;
    const Skill *skill = &YOU.learned_skills[action_type];
    if (skill->skill_id == SKILL_ID_NONE || YOU.QI < skill->cost)
        return 0;
    YOU.current_action_type = skill->action_type;
    YOU.QI_change -= skill->cost;
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
    g_ui_breakthrough_offered = false;
    int battle_cultivation = UI_BattleCultivationGain();
    YOU.cultivation += battle_cultivation;
    char insight_log[128];
    snprintf(insight_log, sizeof(insight_log), "搏杀感悟：获得 %d 点修为。", battle_cultivation);
    UI_AddLog(insight_log);
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
        case UI_ROUTE_REWARD_BALANCED:
        default:
            break;
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
        UI_GenerateRouteOptions();
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

static void UI_AfterRewardDecision(void)
{
    g_ui_pending_replacement_reward = (RewardChoice){REWARD_CULTIVATION, RARITY_COMMON, 0, 0, false};
    g_ui_replacement_type = UI_REPLACE_NONE;
    g_ui_replacement_slot_count = 0;
    g_ui_replacement_action_type = ACTION_TYPE_NONE;
    if (UI_CanBreakthrough(&YOU) && !g_ui_breakthrough_offered)
    {
        g_ui_breakthrough_offered = true;
        g_ui_phase = UI_PHASE_BREAKTHROUGH_CHOICE;
    }
    else
    {
        UI_GenerateRouteOptions();
    }
}

static void UI_FinishRewardFlow(void)
{
    g_ui_pending_replacement_reward = (RewardChoice){REWARD_CULTIVATION, RARITY_COMMON, 0, 0, false};
    g_ui_replacement_type = UI_REPLACE_NONE;
    g_ui_replacement_slot_count = 0;
    g_ui_replacement_action_type = ACTION_TYPE_NONE;
    UI_AfterRewardDecision();
}

static void UI_AddArtifactDirect(ArtifactID id)
{
    if (id <= ARTIFACT_NONE || id >= TOTAL_ARTIFACTS)
        return;
    if (Player_HasArtifact(&YOU, id))
        return;
    if (YOU.artifact_count < MAX_ARTIFACT_SLOTS)
        YOU.artifacts[YOU.artifact_count++] = id;
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
    g_ui_phase = UI_PHASE_BATTLE_RESULT;
    if (YOU.HP <= 0)
    {
        if (YOU.soul_state == SOUL_GHOST)
        {
            g_ui_phase = UI_PHASE_DEFEAT;
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
        UI_FinishRewardFlow();
        return;
    }
    if (g_ui_replacement_type == UI_REPLACE_SKILL)
    {
        if (choice.id > SKILL_ID_NONE && choice.id < TOTAL_SKILLS)
        {
            Player_EquipSkill(&YOU, (SkillID)choice.id);
            UI_AddLog("新技能已装备。");
        }
        UI_FinishRewardFlow();
        return;
    }
    if (g_ui_replacement_type == UI_REPLACE_ARTIFACT)
    {
        if (slot >= 0 && slot < YOU.artifact_count &&
            choice.id > ARTIFACT_NONE && choice.id < TOTAL_ARTIFACTS)
        {
            YOU.artifacts[slot] = (ArtifactID)choice.id;
            UI_AddLog("法器替换完成。");
        }
        UI_FinishRewardFlow();
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
        UI_FinishRewardFlow();
    }
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

static void UI_AttemptBreakthrough(void)
{
    if (g_ui_phase != UI_PHASE_BREAKTHROUGH_CHOICE)
        return;
    int cost = UI_BreakthroughCost(&YOU);
    if (cost <= 0 || YOU.cultivation < cost)
    {
        UI_GenerateRouteOptions();
        return;
    }
    int chance = UI_BreakthroughChance(&YOU);
    YOU.cultivation -= cost;
    if ((int)(Run_Rand() % 100) < chance)
    {
        YOU.XIUWEI++;
        Apply_Breakthrough_Rewards(&YOU);
        UI_AddLog("闭关功成，境界突破。");
        YOU.breakthrough_bonus = 0;
        YOU.breakthrough_fail_penalty = 0;
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
    UI_GenerateRouteOptions();
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
    }
    else
    {
        YOU.cultivation += Yuan[YOU.XIUWEI] * 3;
        UI_AddLog("机缘未能成形，转化为修为。");
    }
    UI_GenerateRouteOptions();
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
        g_ui_near_death_can_ghost = false;
        g_ui_near_death_reason = "";
        UI_GenerateRouteOptions();
        return;
    }

    g_ui_phase = UI_PHASE_DEFEAT;
    UI_AddLog("道途暂止，你被击败了。");
}

static void UI_SkipBreakthrough(void)
{
    if (g_ui_phase == UI_PHASE_BREAKTHROUGH_CHOICE)
    {
        UI_AddLog("暂缓突破，继续积蓄修为。");
        g_ui_breakthrough_offered = false;
        UI_GenerateRouteOptions();
    }
}

static void UI_ApplyRouteImmediateReward(const UIRouteTemplate *route)
{
    if (route->reward_bias == UI_ROUTE_REWARD_REBIRTH)
    {
        int cost = UI_RebirthCultivationCost();
        if (YOU.soul_state == SOUL_GHOST && YOU.cultivation >= cost)
        {
            YOU.cultivation -= cost;
            YOU.soul_state = SOUL_BODY;
            YOU.HP = UI_MaxInt(1, Player_MaxHP(&YOU) / 2);
            YOU.QI = 0;
            Player_ClampVitals(&YOU);
            UI_AddLog("还阳坛阴火退散，肉身重凝。");
            return;
        }
        int gain = Yuan[YOU.XIUWEI] * 4;
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
}

static void UI_ChooseRoute(int index)
{
    if (g_ui_phase != UI_PHASE_ROUTE_CHOICE)
        return;
    if (index < 0 || index >= REWARD_CHOICE_COUNT)
        index = 0;
    g_ui_next_route = g_ui_route_options[index].data;
    g_ui_has_next_route = true;
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
            g_ui_phase = UI_PHASE_DEFEAT;
            UI_AddLog("路线劫数击散魂火，魂飞魄散。");
            return;
        }
        if (YOU.HP <= 0)
            YOU.HP = 1;
    }
    UI_ApplyRouteImmediateReward(&g_ui_next_route);
    if (Player_HasTalent(&YOU, TALENT_WANDERER) && g_ui_next_route.risk <= 0)
    {
        int gain = Yuan[YOU.XIUWEI];
        YOU.cultivation += gain;
        UI_AddLog("行脚老成，从安稳路线中沉淀了额外修为。");
    }
    UI_AddLog(g_ui_next_route.title);
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
        else if (strstr(line, "equip_skill"))
            UI_EquipManagedSkill(UI_ParseCommandValue(line, "id", SKILL_ID_NONE));
        else if (strstr(line, "attempt_breakthrough"))
            UI_AttemptBreakthrough();
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
