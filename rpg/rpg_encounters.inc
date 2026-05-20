static void Encounter_EnsureValidReward(Player *player, RewardChoice *choice)
{
    if (choice->type == REWARD_SKILL && choice->id == SKILL_ID_NONE)
        *choice = (RewardChoice){REWARD_CULTIVATION, choice->rarity, 0, Yuan[player->XIUWEI] * 4, false};
}

void Run_MaybeTriggerEncounter(Player *player)
{
    if (!g_config.run_mode || player != &YOU || g_run.battle_index <= 0)
        return;
    if ((g_run.battle_index % 2) != 0)
        return;

    EncounterID encounter = (EncounterID)(Run_Rand() % TOTAL_ENCOUNTERS);
    RewardChoice choice = {REWARD_CULTIVATION, RARITY_COMMON, 0, Yuan[player->XIUWEI] * 3, false};
    int penalty = 0;

    CHN_PRINT("\n--- 奇遇 ---\n");
    ENG_PRINT("\n--- Encounter ---\n");
    switch (encounter)
    {
    case ENCOUNTER_ANCIENT_CAVE:
        CHN_PRINT("古修洞府：残碑上的道纹仍在发光。\n");
        ENG_PRINT("Ancient Cave: Dao marks on a broken stele still glow.\n");
        choice = (RewardChoice){REWARD_SKILL, RARITY_RARE, PickRewardSkill(player, RARITY_RARE, true), 1, true};
        penalty = max_HP[player->XIUWEI] / 10;
        break;
    case ENCOUNTER_WOUNDED_BEAST:
        CHN_PRINT("濒死妖兽：一只妖兽伏在路旁，妖丹隐现。\n");
        ENG_PRINT("Wounded Beast: a dying beast lies beside the road, its core flickering.\n");
        choice = (RewardChoice){REWARD_ELIXIR, RARITY_RARE, ELIXIR_BREAKTHROUGH, 1, false};
        penalty = 0;
        break;
    case ENCOUNTER_TRIBULATION_CLOUD:
        CHN_PRINT("天劫残云：劫雷余威尚未散去。\n");
        ENG_PRINT("Tribulation Cloud: remnant thunder still churns overhead.\n");
        choice = (RewardChoice){REWARD_ARTIFACT, RARITY_RARE, ARTIFACT_THUNDER_WOOD_SWORD, 1, false};
        penalty = max_HP[player->XIUWEI] / 8;
        break;
    case ENCOUNTER_MARKET:
        CHN_PRINT("坊市交易：散修摊位上真假难辨。\n");
        ENG_PRINT("Market Trade: a rogue cultivator's stall offers uncertain treasures.\n");
        choice = (RewardChoice){REWARD_ELIXIR, RARITY_COMMON, PickRewardElixir(RARITY_RARE), 2, false};
        break;
    case ENCOUNTER_SWORD_TOMB:
        CHN_PRINT("无名剑冢：断剑低鸣，似在择主。\n");
        ENG_PRINT("Nameless Sword Tomb: broken blades hum as if choosing a master.\n");
        choice = (RewardChoice){REWARD_SKILL, RARITY_EPIC, SKILL_ID_GREAT_GOLDEN_SWORDFORMATION, 1, true};
        penalty = max_HP[player->XIUWEI] / 6;
        break;
    case ENCOUNTER_BLOOD_POOL:
        CHN_PRINT("血池秘境：血雾翻滚，魔功唾手可得。\n");
        ENG_PRINT("Blood Pool: blood mist rolls, and demonic arts are within reach.\n");
        choice = (RewardChoice){REWARD_SKILL, RARITY_EPIC, SKILL_ID_BLOOD_DEVIL_SLASH, 1, true};
        player->cursed += Yuan[player->XIUWEI];
        break;
    default:
        break;
    }

    Encounter_EnsureValidReward(player, &choice);

    if (penalty > 0)
    {
        if (choice.bypass_prereq && Player_HasArtifact(player, ARTIFACT_FORMLESS_JADE) &&
            !player->formless_jade_used)
        {
            penalty = 0;
            player->formless_jade_used = true;
            CHN_PRINT("[无相玉] 化去破格领悟的代价。\n");
            ENG_PRINT("[Formless Jade] Dissolved the cost of bypassed insight.\n");
        }
        if (player->disaster_warded)
        {
            penalty /= 2;
            player->disaster_warded = false;
            CHN_PRINT("[化劫丹] 劫数减轻。\n");
            ENG_PRINT("[Disaster Ward Pill] The calamity is softened.\n");
        }
        player->HP -= penalty;
        CHN_PRINT("[代价] 元神受损 %d 点。\n", penalty);
        ENG_PRINT("[Price] Lost %d HP.\n", penalty);
        if (player->HP <= 0)
            player->HP = 1;
    }
    Apply_RewardChoice(player, &choice);
}
