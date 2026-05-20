// Run-local minor realms and lightweight skill refinement.

static int RPG_MinorRealmThreshold(const Player *player)
{
    if (!player)
        return 3;
    return 3 + (int)player->XIUWEI;
}

static void RPG_ResetMinorRealmProgress(Player *player)
{
    if (!player)
        return;
    player->minor_understanding = 0;
    player->minor_realm_level = 0;
}

static bool RPG_AddMinorUnderstanding(Player *player, int amount)
{
    if (!player || amount <= 0)
        return false;
    player->minor_understanding += amount;
    bool leveled = false;
    while (player->minor_realm_level < MAX_MINOR_REALM_LEVEL)
    {
        int threshold = RPG_MinorRealmThreshold(player);
        if (player->minor_understanding < threshold)
            break;
        player->minor_understanding -= threshold;
        player->minor_realm_level++;
        leveled = true;
    }
    if (player->minor_realm_level >= MAX_MINOR_REALM_LEVEL)
    {
        player->minor_realm_level = MAX_MINOR_REALM_LEVEL;
        int threshold = RPG_MinorRealmThreshold(player);
        if (player->minor_understanding > threshold)
            player->minor_understanding = threshold;
    }
    return leveled;
}

static const char *SkillRefineName(SkillRefineType refine)
{
    switch (refine)
    {
    case SKILL_REFINE_COST_DOWN: return "气耗降低";
    case SKILL_REFINE_DAMAGE: return "威力精修";
    case SKILL_REFINE_QI_FLOW: return "回气顺脉";
    case SKILL_REFINE_HEALING: return "疗愈精修";
    case SKILL_REFINE_STATUS: return "术法淬毒";
    case SKILL_REFINE_NONE:
    default: return "未洗练";
    }
}

static const char *SkillRefineDesc(SkillRefineType refine)
{
    switch (refine)
    {
    case SKILL_REFINE_COST_DOWN: return "气耗 -1，最低为 0。";
    case SKILL_REFINE_DAMAGE: return "命中伤害提高 10%。";
    case SKILL_REFINE_QI_FLOW: return "获得灵气效果提高 10%。";
    case SKILL_REFINE_HEALING: return "治疗效果提高 10%。";
    case SKILL_REFINE_STATUS: return "流血、诅咒或激怒强度 +1。";
    case SKILL_REFINE_NONE:
    default: return "尚未洗练。";
    }
}

static SkillRefineType Player_SkillRefine(const Player *player, SkillID id)
{
    if (!player || id <= SKILL_ID_NONE || id >= TOTAL_SKILLS)
        return SKILL_REFINE_NONE;
    return player->skill_refines[id];
}

static int Skill_EffectiveCostForPlayer(const Player *player, const Skill *skill)
{
    if (!skill)
        return 0;
    int cost = skill->cost;
    if (Player_SkillRefine(player, skill->skill_id) == SKILL_REFINE_COST_DOWN)
        cost--;
    return cost < 0 ? 0 : cost;
}

static int Skill_RefineCost(const Player *player, const Skill *skill)
{
    if (!player || !skill)
        return 0;
    int base = Yuan[player->XIUWEI] * (4 + skill->rank);
    return base > 1 ? base : 1;
}

static void SkillRefineSummary(const Player *player, const Skill *skill, char *buf, size_t size)
{
    if (size == 0)
        return;
    buf[0] = '\0';
    if (!skill)
        return;
    SkillRefineType refine = Player_SkillRefine(player, skill->skill_id);
    if (refine == SKILL_REFINE_NONE)
        return;
    snprintf(buf, size, "%s：%s", SkillRefineName(refine), SkillRefineDesc(refine));
}

static SkillRefineType SkillRefine_RandomForSkill(const Skill *skill)
{
    if (!skill)
        return SKILL_REFINE_COST_DOWN;
    SkillRefineType pool[5];
    int count = 0;
    pool[count++] = SKILL_REFINE_COST_DOWN;
    if (skill->target_type == TARGET_ENEMY)
        pool[count++] = SKILL_REFINE_DAMAGE;
    if (skill->action_type == ACTION_TYPE_GAIN_QI || skill->attribute_id == ATTR_DARK)
        pool[count++] = SKILL_REFINE_QI_FLOW;
    if (skill->action_type == ACTION_TYPE_HEAL || skill->target_type == TARGET_SELF)
        pool[count++] = SKILL_REFINE_HEALING;
    if (skill->effect_strength > 0 || skill->attribute_id == ATTR_BLOOD ||
        skill->attribute_id == ATTR_DARK || skill->action_type == ACTION_TYPE_BOOST)
        pool[count++] = SKILL_REFINE_STATUS;
    if (count <= 0)
        return SKILL_REFINE_COST_DOWN;
    return pool[Run_Rand() % (unsigned int)count];
}
