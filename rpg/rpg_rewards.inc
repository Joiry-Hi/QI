static const char *RewardRarityName(RewardRarity rarity)
{
    switch (rarity)
    {
    case RARITY_COMMON: return "Common";
    case RARITY_RARE: return "Rare";
    case RARITY_EPIC: return "Epic";
    case RARITY_LEGENDARY: return "Legendary";
    default: return "Unknown";
    }
}

static void PrintRewardChoice(int index, const RewardChoice *choice)
{
    CHN_PRINT("[%d] ", index + 1);
    ENG_PRINT("[%d] ", index + 1);
    switch (choice->type)
    {
    case REWARD_SKILL:
        CHN_PRINT("技能: %s [%s/%s] 阶%d (%s)%s\n",
                  g_skill_database[choice->id].name_chn,
                  ActionTypeNames_CHN[g_skill_database[choice->id].action_type],
                  SchoolTagName(g_skill_database[choice->id].school_tag),
                  g_skill_database[choice->id].rank,
                  RewardRarityName(choice->rarity),
                  choice->bypass_prereq ? " 破格" : "");
        ENG_PRINT("Skill: %s [%s/%s] Rank %d (%s)%s\n",
                  g_skill_database[choice->id].name_eng,
                  ActionTypeNames_ENG[g_skill_database[choice->id].action_type],
                  SchoolTagName(g_skill_database[choice->id].school_tag),
                  g_skill_database[choice->id].rank,
                  RewardRarityName(choice->rarity),
                  choice->bypass_prereq ? " bypass" : "");
        break;
    case REWARD_ARTIFACT:
        CHN_PRINT("法器: %s (%s) - %s\n", g_artifact_database[choice->id].name_chn, RewardRarityName(choice->rarity), g_artifact_database[choice->id].desc_chn);
        ENG_PRINT("Artifact: %s (%s) - %s\n", g_artifact_database[choice->id].name_eng, RewardRarityName(choice->rarity), g_artifact_database[choice->id].desc_eng);
        break;
    case REWARD_ELIXIR:
        CHN_PRINT("丹药: %s x%d (%s) - %s\n", g_elixir_database[choice->id].name_chn, choice->amount, RewardRarityName(choice->rarity), g_elixir_database[choice->id].desc_chn);
        ENG_PRINT("Elixir: %s x%d (%s) - %s\n", g_elixir_database[choice->id].name_eng, choice->amount, RewardRarityName(choice->rarity), g_elixir_database[choice->id].desc_eng);
        break;
    case REWARD_CULTIVATION:
        CHN_PRINT("修炼: 获得 %d 点修为\n", choice->amount);
        ENG_PRINT("Cultivation: gain %d cultivation\n", choice->amount);
        break;
    }
}

static SkillID PickRewardSkill(Player *player, RewardRarity rarity, bool bypass)
{
    SkillID candidates[TOTAL_SKILLS];
    int count = 0;
    int max_rank = player->XIUWEI + 1 + (int)rarity;
    if (max_rank >= TOTAL_XIUWEI_LEVEL)
        max_rank = TOTAL_XIUWEI_LEVEL - 1;

    for (int i = 0; i < TOTAL_SKILLS; i++)
    {
        if (!Skill_IsDefined((SkillID)i) || player->unlocked_skills[i])
            continue;
        const Skill *skill = &g_skill_database[i];
        if (skill->rank > max_rank)
            continue;
        if (!bypass && !Skill_PrereqMet(player, (SkillID)i))
            continue;
        candidates[count++] = (SkillID)i;
    }
    if (count == 0)
        return SKILL_ID_NONE;
    return candidates[Run_Rand() % count];
}

static ArtifactID PickRewardArtifactForPlayer(const Player *player, RewardRarity rarity)
{
    ArtifactID candidates[TOTAL_ARTIFACTS];
    int count = 0;
    for (int i = 0; i < TOTAL_ARTIFACTS; i++)
    {
        if (g_artifact_database[i].id == i && g_artifact_database[i].rarity <= rarity &&
            (!player || !Player_HasArtifact(player, (ArtifactID)i)))
            candidates[count++] = (ArtifactID)i;
    }
    return count ? candidates[Run_Rand() % count] : ARTIFACT_NONE;
}

static ElixirID PickRewardElixir(RewardRarity rarity)
{
    ElixirID candidates[TOTAL_ELIXIRS];
    int count = 0;
    for (int i = 0; i < TOTAL_ELIXIRS; i++)
    {
        if (g_elixir_database[i].id == i && g_elixir_database[i].rarity <= rarity)
            candidates[count++] = (ElixirID)i;
    }
    return count ? candidates[Run_Rand() % count] : ELIXIR_HEALING;
}

void Generate_RewardChoices(Player *player, RewardChoice choices[REWARD_CHOICE_COUNT])
{
    int battle = g_run.battle_index;
    RewardRarity rarity = RARITY_COMMON;
    if (battle > 0 && battle % 6 == 0) rarity = RARITY_EPIC;
    else if (battle > 0 && battle % 3 == 0) rarity = RARITY_RARE;

    choices[0] = (RewardChoice){REWARD_SKILL, rarity, PickRewardSkill(player, rarity, false), 1, false};
    choices[1] = (RewardChoice){REWARD_ARTIFACT, rarity, PickRewardArtifactForPlayer(player, rarity), 1, false};
    choices[2] = (RewardChoice){REWARD_ELIXIR, rarity, PickRewardElixir(rarity), 1 + (rarity >= RARITY_RARE), false};
    if (choices[0].id == SKILL_ID_NONE)
        choices[0] = (RewardChoice){REWARD_CULTIVATION, rarity, 0, max_QI[player->XIUWEI] / 4, false};
    if (choices[1].id == ARTIFACT_NONE)
        choices[1] = (RewardChoice){REWARD_CULTIVATION, rarity, 0, max_QI[player->XIUWEI] / 3, false};

    if (battle > 0 && battle % 6 == 0)
    {
        choices[0] = (RewardChoice){REWARD_ARTIFACT, RARITY_EPIC, PickRewardArtifactForPlayer(player, RARITY_EPIC), 1, false};
        choices[1] = (RewardChoice){REWARD_SKILL, RARITY_EPIC, PickRewardSkill(player, RARITY_EPIC, false), 1, false};
        choices[2] = (RewardChoice){REWARD_CULTIVATION, RARITY_RARE, 0, max_QI[player->XIUWEI] / 3, false};
        if (choices[0].id == ARTIFACT_NONE)
            choices[0] = (RewardChoice){REWARD_CULTIVATION, RARITY_EPIC, 0, max_QI[player->XIUWEI] / 2, false};
        if (choices[1].id == SKILL_ID_NONE)
            choices[1] = (RewardChoice){REWARD_CULTIVATION, RARITY_EPIC, 0, max_QI[player->XIUWEI] / 2, false};
    }
}

void Apply_RewardChoice(Player *player, const RewardChoice *choice)
{
    switch (choice->type)
    {
    case REWARD_SKILL:
        Player_UnlockSkill(player, (SkillID)choice->id, choice->bypass_prereq);
        break;
    case REWARD_ARTIFACT:
        Player_AddArtifact(player, (ArtifactID)choice->id);
        break;
    case REWARD_ELIXIR:
        for (int i = 0; i < choice->amount; i++)
            Player_AddElixir(player, (ElixirID)choice->id);
        break;
    case REWARD_CULTIVATION:
        if (g_config.run_mode)
            player->cultivation += choice->amount;
        else
            player->QI += choice->amount;
        Player_ClampVitals(player);
        CHN_PRINT("[修炼] 获得 %d 点修为。\n", choice->amount);
        ENG_PRINT("[Cultivation] Gained %d cultivation.\n", choice->amount);
        break;
    }
}

void Run_AfterBattleReward(Player *player)
{
    if (!g_config.run_mode || player != &YOU || player->HP <= 0)
        return;

    g_run.battle_index++;
    if (g_run.battle_index % 6 == 0) g_run.boss_kills++;
    else if (g_run.battle_index % 3 == 0) g_run.elite_kills++;
    int battle_cultivation = Yuan[player->XIUWEI] * (2 + (g_run.battle_index % 3 == 0) + (g_run.battle_index % 6 == 0));
    battle_cultivation += Player_ArtifactEffectiveIntBonus(player, ArtifactPostBattleCultivation);
    player->cultivation += battle_cultivation;
    CHN_PRINT("[搏杀感悟] 获得 %d 点修为。\n", battle_cultivation);
    ENG_PRINT("[Battle Insight] Gained %d cultivation.\n", battle_cultivation);

    Generate_RewardChoices(player, g_run.pending_reward_choices);
    Player_PrintRunProgress(&g_run);
    Player_PrintBuildSummary(player);
    Player_PrintSkillLoadout(player);
    Player_PrintArtifacts(player);
    Player_PrintElixirs(player);

    int picked = 0;
    if (Player_IsHumanFacing(player))
    {
        CHN_PRINT("\n--- 战利品三选一 ---\n");
        ENG_PRINT("\n--- Choose One Reward ---\n");
        for (int i = 0; i < REWARD_CHOICE_COUNT; i++)
            PrintRewardChoice(i, &g_run.pending_reward_choices[i]);
        CHN_PRINT("选择奖励 1-%d: ", REWARD_CHOICE_COUNT);
        ENG_PRINT("Choose reward 1-%d: ", REWARD_CHOICE_COUNT);
        fflush(stdout);
        char buffer[16];
        if (fgets(buffer, sizeof(buffer), stdin))
            picked = atoi(buffer) - 1;
    }
    else
    {
        for (int i = 1; i < REWARD_CHOICE_COUNT; i++)
        {
            if (RewardChoiceScore(player, &g_run.pending_reward_choices[i]) >
                RewardChoiceScore(player, &g_run.pending_reward_choices[picked]))
                picked = i;
        }
        CHN_PRINT("[自动选择] ");
        ENG_PRINT("[Auto Pick] ");
        PrintRewardChoice(picked, &g_run.pending_reward_choices[picked]);
    }
    if (picked < 0 || picked >= REWARD_CHOICE_COUNT)
        picked = 0;
    Apply_RewardChoice(player, &g_run.pending_reward_choices[picked]);

    if (player->berserk_elixir_turns > 0)
    {
        int backlash = Player_MaxHP(player) / 10;
        player->HP -= backlash;
        if (player->HP <= 0) player->HP = 1;
        CHN_PRINT("[爆元丹] 药力反噬，战后损失 %d 点元神。\n", backlash);
        ENG_PRINT("[Essence Burst Pill] Backlash after battle: lost %d HP.\n", backlash);
        player->berserk_elixir_turns = 0;
    }

    if (Player_HasArtifact(player, ARTIFACT_DEVOURING_ORB))
    {
        int gain = Player_PercentOfMaxQI(player, 20);
        player->QI += gain;
        Player_ClampVitals(player);
        CHN_PRINT("[噬灵珠] 战后吞灵，获得 %d 点气。\n", gain);
        ENG_PRINT("[Devouring Orb] Devoured lingering Qi after battle, gaining %d QI.\n", gain);
    }
}
