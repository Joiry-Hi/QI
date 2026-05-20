// Lightweight Dao foundation talents for the RPG run layer.

static void Initialize_TalentDatabase(void)
{
    g_talent_database[TALENT_QI_SEA] = (Talent){
        .id = TALENT_QI_SEA,
        .name_chn = "气海扩张",
        .name_eng = "Expanded Qi Sea",
        .desc_chn = "最大气上限提高20%，更适合高费用技能循环。",
        .desc_eng = "Increase maximum QI by 20%.",
        .school_tag = SCHOOL_QI};
    g_talent_database[TALENT_GOLDEN_BODY] = (Talent){
        .id = TALENT_GOLDEN_BODY,
        .name_chn = "金身淬体",
        .name_eng = "Golden Body Tempering",
        .desc_chn = "最大元神提高15%，提升路线冒险容错。",
        .desc_eng = "Increase maximum HP by 15%.",
        .school_tag = SCHOOL_DEFENSE};
    g_talent_database[TALENT_SWORD_HEART] = (Talent){
        .id = TALENT_SWORD_HEART,
        .name_chn = "剑心通明",
        .name_eng = "Lucid Sword Heart",
        .desc_chn = "剑修奖励评分提高，剑修技能更容易被判为核心补强。",
        .desc_eng = "Improve sword reward valuation.",
        .school_tag = SCHOOL_SWORD};
    g_talent_database[TALENT_ALCHEMY_RESIDUE] = (Talent){
        .id = TALENT_ALCHEMY_RESIDUE,
        .name_chn = "丹道余韵",
        .name_eng = "Alchemy Afterglow",
        .desc_chn = "每场胜利固定修为额外+1，丹药相关选择收益更高。",
        .desc_eng = "Gain extra cultivation after battle and value elixirs more.",
        .school_tag = SCHOOL_BASIC};
    g_talent_database[TALENT_THUNDER_AFFINITY] = (Talent){
        .id = TALENT_THUNDER_AFFINITY,
        .name_chn = "雷劫亲和",
        .name_eng = "Tribulation Affinity",
        .desc_chn = "突破成功率+8%，天劫路线元神代价降低。",
        .desc_eng = "Increase breakthrough chance and reduce tribulation route HP costs.",
        .school_tag = SCHOOL_THUNDER};
    g_talent_database[TALENT_BLOOD_REBIRTH] = (Talent){
        .id = TALENT_BLOOD_REBIRTH,
        .name_chn = "血煞转生",
        .name_eng = "Blood Rebirth",
        .desc_chn = "战斗胜利后恢复少量元神，风险判断更偏激进。",
        .desc_eng = "Restore some HP after wins and favor aggressive routes.",
        .school_tag = SCHOOL_BLOOD};
    g_talent_database[TALENT_CAVE_INSIGHT] = (Talent){
        .id = TALENT_CAVE_INSIGHT,
        .name_chn = "洞府悟法",
        .name_eng = "Cave Insight",
        .desc_chn = "破格功法奖励评分提高，古修洞府更容易带来功法机缘。",
        .desc_eng = "Improve bypass skill reward valuation.",
        .school_tag = SCHOOL_QI};
    g_talent_database[TALENT_WANDERER] = (Talent){
        .id = TALENT_WANDERER,
        .name_chn = "行脚老成",
        .name_eng = "Seasoned Wanderer",
        .desc_chn = "低风险路线额外获得少量修为。",
        .desc_eng = "Gain extra cultivation from low-risk routes.",
        .school_tag = SCHOOL_BASIC};
}

static bool Player_HasTalent(const Player *player, TalentID id)
{
    if (!player || id < 0 || id >= TOTAL_TALENTS)
        return false;
    return player->talents[id];
}

static int Player_AddTalent(Player *player, TalentID id)
{
    if (!player || id < 0 || id >= TOTAL_TALENTS)
        return 0;
    if (player->talents[id])
        return 0;
    player->talents[id] = true;
    player->talent_count++;
    Player_ClampVitals(player);
    return 1;
}

static int Talent_CountAvailable(const Player *player)
{
    int count = 0;
    for (int i = 0; i < TOTAL_TALENTS; i++)
    {
        if (!Player_HasTalent(player, (TalentID)i))
            count++;
    }
    return count;
}

static const char *BuildSynergyLabel(const BuildProfile *profile)
{
    if (!profile || profile->primary_school == SCHOOL_NONE || profile->primary_count <= 0)
        return CHN("未成型") ENG("Unformed");
    if (profile->primary_count >= 5)
        return CHN("核心流派") ENG("Core School");
    if (profile->primary_count >= 3)
        return CHN("流派成型") ENG("Formed School");
    return CHN("路线萌芽") ENG("Emerging Route");
}

static const char *BuildSynergyBonusText(const BuildProfile *profile)
{
    if (!profile || profile->primary_school == SCHOOL_NONE || profile->primary_count <= 0)
        return CHN("奖励评分会优先寻找能补齐动作槽或开启路线的功法。")
               ENG("Reward scoring favors action coverage and new routes.");
    if (profile->primary_count >= 5)
        return CHN("同流派5个以上，奖励会更偏向核心补强。")
               ENG("At 5+ same-school picks, rewards lean toward core reinforcement.");
    if (profile->primary_count >= 3)
        return CHN("同流派3个以上，相关功法会被视作路线契合。")
               ENG("At 3+ same-school picks, related skills are treated as synergistic.");
    return CHN("继续收集同流派功法可形成更稳定的构筑方向。")
           ENG("More same-school skills will stabilize the build direction.");
}
