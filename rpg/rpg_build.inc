static const char *SchoolTagName(SchoolTag school)
{
#ifdef CHINESE_GAME_LOG
    static const char *names[TOTAL_SCHOOLS] = {
        "无", "基础", "火法", "剑修", "血道", "护体", "气修", "雷法", "幽冥"};
#else
    static const char *names[TOTAL_SCHOOLS] = {
        "None", "Basic", "Fire", "Sword", "Blood", "Defense", "Qi", "Thunder", "Dark"};
#endif
    if (school < SCHOOL_NONE || school >= TOTAL_SCHOOLS)
        return names[SCHOOL_NONE];
    return names[school];
}

static const char *BuildArchetypeName(const BuildProfile *profile)
{
    if (profile->primary_school == SCHOOL_NONE || profile->primary_count <= 0)
        return CHN("散修") ENG("Wanderer");
    if (profile->primary_count >= 4)
    {
        switch (profile->primary_school)
        {
        case SCHOOL_FIRE: return CHN("焚天法修") ENG("Inferno Adept");
        case SCHOOL_SWORD: return CHN("御剑宗师") ENG("Swordmaster");
        case SCHOOL_BLOOD: return CHN("血煞魔修") ENG("Blood Cultivator");
        case SCHOOL_DEFENSE: return CHN("玄甲守御") ENG("Aegis Guardian");
        case SCHOOL_QI: return CHN("聚元行者") ENG("Qi Channeler");
        case SCHOOL_THUNDER: return CHN("雷劫使") ENG("Thunder Caller");
        case SCHOOL_DARK: return CHN("幽冥夺元") ENG("Nether Plunderer");
        default: break;
        }
    }
    return CHN("初成流派") ENG("Emerging Build");
}

BuildProfile Player_GetBuildProfile(const Player *player)
{
    BuildProfile profile = {SCHOOL_NONE, SCHOOL_NONE, 0, 0, 0, 0, player->artifact_count, player->elixir_count, "", ""};

    for (int i = 0; i < TOTAL_SKILLS; i++)
    {
        if (Skill_IsDefined((SkillID)i) && player->unlocked_skills[i])
            profile.unlocked_skill_count++;
    }
    for (int i = 0; i < TOTAL_ACTION_TYPES; i++)
    {
        if (player->equipped_skills[i] != SKILL_ID_NONE)
            profile.equipped_skill_count++;
    }
    for (int i = SCHOOL_BASIC; i < TOTAL_SCHOOLS; i++)
    {
        int count = player->school_counts[i];
        if (count > profile.primary_count)
        {
            profile.secondary_school = profile.primary_school;
            profile.secondary_count = profile.primary_count;
            profile.primary_school = (SchoolTag)i;
            profile.primary_count = count;
        }
        else if (count > profile.secondary_count)
        {
            profile.secondary_school = (SchoolTag)i;
            profile.secondary_count = count;
        }
    }
    profile.synergy_label = BuildSynergyLabel(&profile);
    profile.synergy_bonus = BuildSynergyBonusText(&profile);
    return profile;
}

void Player_PrintBuildSummary(const Player *player)
{
    BuildProfile profile = Player_GetBuildProfile(player);
    CHN_PRINT("\n--- 构筑画像 ---\n");
    ENG_PRINT("\n--- Build Profile ---\n");
    CHN_PRINT("定位: %s | 主流派: %s(%d) | 副流派: %s(%d)\n",
              BuildArchetypeName(&profile),
              SchoolTagName(profile.primary_school), profile.primary_count,
              SchoolTagName(profile.secondary_school), profile.secondary_count);
    CHN_PRINT("协同: %s | %s\n", profile.synergy_label, profile.synergy_bonus);
    ENG_PRINT("Archetype: %s | Main: %s(%d) | Secondary: %s(%d)\n",
              BuildArchetypeName(&profile),
              SchoolTagName(profile.primary_school), profile.primary_count,
              SchoolTagName(profile.secondary_school), profile.secondary_count);
    ENG_PRINT("Synergy: %s | %s\n", profile.synergy_label, profile.synergy_bonus);
    CHN_PRINT("技能 %d/%d, 装备槽 %d/%d, 法器 %d/%d, 丹药 %d/%d\n",
              profile.unlocked_skill_count, TOTAL_SKILLS,
              profile.equipped_skill_count, TOTAL_ACTION_TYPES,
              profile.artifact_count, MAX_ARTIFACT_SLOTS,
              profile.elixir_count, MAX_ELIXIR_SLOTS);
    ENG_PRINT("Skills %d/%d, slots %d/%d, artifacts %d/%d, elixirs %d/%d\n",
              profile.unlocked_skill_count, TOTAL_SKILLS,
              profile.equipped_skill_count, TOTAL_ACTION_TYPES,
              profile.artifact_count, MAX_ARTIFACT_SLOTS,
              profile.elixir_count, MAX_ELIXIR_SLOTS);
}

static int RewardChoiceScore(const Player *player, const RewardChoice *choice)
{
    int score = (int)choice->rarity * 100;

    switch (choice->type)
    {
    case REWARD_SKILL:
    {
        if (choice->id <= SKILL_ID_NONE || choice->id >= TOTAL_SKILLS)
            return -10000;
        if (player->unlocked_skills[choice->id])
            return -200;
        const Skill *skill = &g_skill_database[choice->id];
        score += skill->rank * 18;
        score += player->school_counts[skill->school_tag] * 10;
        if (skill->school_tag != SCHOOL_NONE)
            score += 20;
        if (choice->bypass_prereq)
            score += 30;
        if (Player_HasTalent(player, TALENT_SWORD_HEART) && skill->school_tag == SCHOOL_SWORD)
            score += 45;
        if (Player_HasTalent(player, TALENT_CAVE_INSIGHT) && choice->bypass_prereq)
            score += 35;
        if (player->equipped_skills[skill->action_type] == SKILL_ID_NONE)
            score += 45;
        else if (skill->rank > g_skill_database[player->equipped_skills[skill->action_type]].rank)
            score += 35;
        break;
    }
    case REWARD_ARTIFACT:
        if (choice->id <= ARTIFACT_NONE || choice->id >= TOTAL_ARTIFACTS)
            return -10000;
        if (Player_HasArtifact(player, (ArtifactID)choice->id))
            return -180;
        score += player->artifact_count < MAX_ARTIFACT_SLOTS ? 55 : 15;
        if (g_artifact_database[choice->id].is_custom &&
            g_artifact_database[choice->id].reward_school_bias == Player_GetBuildProfile(player).primary_school)
            score += 30;
        break;
    case REWARD_ELIXIR:
        if (choice->id <= ELIXIR_NONE || choice->id >= TOTAL_ELIXIRS)
            return -10000;
        score += player->elixir_count < MAX_ELIXIR_SLOTS ? 30 : 5;
        score += choice->amount * 12;
        if (Player_HasTalent(player, TALENT_ALCHEMY_RESIDUE))
            score += 25;
        break;
    case REWARD_CULTIVATION:
        score += choice->amount / (Yuan[player->XIUWEI] > 0 ? Yuan[player->XIUWEI] : 1);
        if (player->QI >= Player_MaxQI(player) * 3 / 4)
            score += 25;
        break;
    }

    return score;
}
