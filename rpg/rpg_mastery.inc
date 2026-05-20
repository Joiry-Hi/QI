// Skill mastery and lightweight school combat loops for Web run mode.

static int RPG_MasteryLevel(const Player *player, SkillID skill_id)
{
    if (skill_id <= SKILL_ID_NONE || skill_id >= TOTAL_SKILLS)
        return 0;
    int xp = player->skill_mastery[skill_id];
    if (xp >= 15)
        return 3;
    if (xp >= 8)
        return 2;
    if (xp >= 3)
        return 1;
    return 0;
}

static int RPG_MasteryNextThreshold(const Player *player, SkillID skill_id)
{
    if (skill_id <= SKILL_ID_NONE || skill_id >= TOTAL_SKILLS)
        return 3;
    int xp = player->skill_mastery[skill_id];
    if (xp < 3)
        return 3;
    if (xp < 8)
        return 8;
    if (xp < 15)
        return 15;
    return 15;
}

static int RPG_SchoolCycleThreshold(SchoolTag school)
{
    switch (school)
    {
    case SCHOOL_THUNDER:
        return 2;
    case SCHOOL_DEFENSE:
        return 0;
    default:
        return 3;
    }
}

static const char *RPG_SchoolCycleLabel(SchoolTag school)
{
    switch (school)
    {
    case SCHOOL_BASIC: return "生机";
    case SCHOOL_FIRE: return "灼痕";
    case SCHOOL_SWORD: return "剑势";
    case SCHOOL_BLOOD: return "血煞";
    case SCHOOL_DEFENSE: return "护体架势";
    case SCHOOL_QI: return "气韵";
    case SCHOOL_THUNDER: return "雷势";
    case SCHOOL_DARK: return "魂火";
    default: return "流派";
    }
}

static bool RPG_ShouldTrackMastery(const Player *player)
{
    return g_config.run_mode && player == &YOU;
}

static void RPG_RecordSkillCast(Player *caster, const Skill *skill)
{
    if (!RPG_ShouldTrackMastery(caster) || !Skill_IsDefined(skill->skill_id))
        return;

    int old_level = RPG_MasteryLevel(caster, skill->skill_id);
    if (caster->skill_mastery[skill->skill_id] < 9999)
        caster->skill_mastery[skill->skill_id]++;
    int new_level = RPG_MasteryLevel(caster, skill->skill_id);
    if (new_level > old_level)
    {
        CHN_PRINT("[熟练] %s 熟练度提升至 Lv%d。\n", skill->name_chn, new_level);
        ENG_PRINT("[Mastery] %s reached Lv%d.\n", skill->name_eng, new_level);
    }

    switch (skill->school_tag)
    {
    case SCHOOL_QI:
        caster->school_cycle[SCHOOL_QI] += 1 + Equipment_ResonanceExtraLayer(caster, SCHOOL_QI);
        if (caster->school_cycle[SCHOOL_QI] >= 3)
        {
            caster->school_cycle[SCHOOL_QI] = 0;
            int gain = Player_PercentOfMaxQI(caster, 10);
            caster->QI_change += gain;
            CHN_PRINT("[气韵] 周天一转，回涌 %d 点灵气。\n", gain);
        }
        break;
    case SCHOOL_DEFENSE:
        caster->school_cycle[SCHOOL_DEFENSE] += 1 + Equipment_ResonanceExtraLayer(caster, SCHOOL_DEFENSE);
        CHN_PRINT("[护体架势] 架势积累至 %d 层。\n", caster->school_cycle[SCHOOL_DEFENSE]);
        break;
    case SCHOOL_BASIC:
        caster->school_cycle[SCHOOL_BASIC] += 1 + Equipment_ResonanceExtraLayer(caster, SCHOOL_BASIC);
        if (caster->school_cycle[SCHOOL_BASIC] >= 3)
        {
            caster->school_cycle[SCHOOL_BASIC] = 0;
            int heal = Player_PercentOfMaxHP(caster, 8);
            caster->HP_change += heal;
            if (caster->bleeding > 0) caster->bleeding--;
            if (caster->cursed > 0) caster->cursed -= caster->cursed > 1 ? 1 : caster->cursed;
            CHN_PRINT("[生机] 生机流转，恢复 %d 点元神并洗去一缕浊气。\n", heal);
        }
        break;
    default:
        break;
    }
}

static int RPG_ApplySchoolOnDefense(Player *defender, const Skill *defender_skill, int final_damage)
{
    if (!RPG_ShouldTrackMastery(defender) || final_damage <= 0)
        return final_damage;
    if (defender->school_cycle[SCHOOL_DEFENSE] <= 0)
        return final_damage;
    if (defender_skill->school_tag != SCHOOL_DEFENSE &&
        defender_skill->action_type != ACTION_TYPE_DEFEND &&
        defender_skill->action_type != ACTION_TYPE_COUNTER)
        return final_damage;

    int layers = defender->school_cycle[SCHOOL_DEFENSE];
    int reduction = defender->YUAN * layers;
    if (reduction > final_damage)
        reduction = final_damage;
    defender->school_cycle[SCHOOL_DEFENSE] = 0;
    CHN_PRINT("[护体架势] 消耗 %d 层架势，抵消 %d 点伤害。\n", layers, reduction);
    return final_damage - reduction;
}

static int RPG_ApplySchoolOnHit(Player *attacker, Player *defender, const Skill *skill, int final_damage)
{
    if (!RPG_ShouldTrackMastery(attacker) || final_damage <= 0 || skill->school_tag <= SCHOOL_NONE)
        return final_damage;

    int mastery = RPG_MasteryLevel(attacker, skill->skill_id);
    switch (skill->school_tag)
    {
    case SCHOOL_SWORD:
        attacker->school_cycle[SCHOOL_SWORD] += 1 + Equipment_ResonanceExtraLayer(attacker, SCHOOL_SWORD);
        if (attacker->school_cycle[SCHOOL_SWORD] >= 3)
        {
            attacker->school_cycle[SCHOOL_SWORD] = 0;
            int bonus = attacker->YUAN * (1 + mastery);
            final_damage += bonus;
            CHN_PRINT("[剑势] 捕捉破绽，追加 %d 点伤害。\n", bonus);
        }
        break;
    case SCHOOL_BLOOD:
        attacker->school_cycle[SCHOOL_BLOOD] += 1 + Equipment_ResonanceExtraLayer(attacker, SCHOOL_BLOOD);
        if (attacker->school_cycle[SCHOOL_BLOOD] >= 3)
        {
            attacker->school_cycle[SCHOOL_BLOOD] = 0;
            defender->bleeding += 1 + mastery;
            int heal = Player_PercentOfMaxHP(attacker, 6);
            attacker->HP_change += heal;
            CHN_PRINT("[血煞] 血气回涌，流血加深并恢复 %d 点元神。\n", heal);
        }
        break;
    case SCHOOL_THUNDER:
        attacker->school_cycle[SCHOOL_THUNDER] += 1 + Equipment_ResonanceExtraLayer(attacker, SCHOOL_THUNDER);
        if (attacker->school_cycle[SCHOOL_THUNDER] >= 2)
        {
            attacker->school_cycle[SCHOOL_THUNDER] = 0;
            int bonus = attacker->YUAN * (1 + mastery);
            final_damage += bonus;
            defender->QI_change -= bonus;
            CHN_PRINT("[雷势] 雷光贯体，追加 %d 点伤害并震散等量灵气。\n", bonus);
        }
        break;
    case SCHOOL_DARK:
        attacker->school_cycle[SCHOOL_DARK] += 1 + Equipment_ResonanceExtraLayer(attacker, SCHOOL_DARK);
        if (attacker->school_cycle[SCHOOL_DARK] >= 3)
        {
            attacker->school_cycle[SCHOOL_DARK] = 0;
            int curse = attacker->YUAN * (1 + mastery);
            int qi = Player_PercentOfMaxQI(attacker, 5);
            defender->cursed += curse;
            attacker->QI_change += qi;
            if (attacker->soul_state == SOUL_GHOST)
                attacker->HP_change += Player_PercentOfMaxHP(attacker, 4);
            CHN_PRINT("[魂火] 阴火缠身，施加 %d 点诅咒并返还灵气。\n", curse);
        }
        break;
    case SCHOOL_FIRE:
        attacker->school_cycle[SCHOOL_FIRE] += 1 + Equipment_ResonanceExtraLayer(attacker, SCHOOL_FIRE);
        if (attacker->school_cycle[SCHOOL_FIRE] >= 3)
        {
            attacker->school_cycle[SCHOOL_FIRE] = 0;
            int burn = attacker->YUAN * (1 + mastery);
            defender->cursed += burn;
            CHN_PRINT("[灼痕] 火毒入体，转化为 %d 点持续伤害。\n", burn);
        }
        break;
    default:
        break;
    }
    return final_damage;
}

static void RPG_SchoolCycleSummary(const Player *player, SchoolTag school, char *buf, size_t size)
{
    if (size == 0)
        return;
    buf[0] = '\0';
    if (school <= SCHOOL_NONE || school >= TOTAL_SCHOOLS)
        return;
    int value = player->school_cycle[school];
    int threshold = RPG_SchoolCycleThreshold(school);
    if (threshold > 0)
        snprintf(buf, size, "%s %d/%d", RPG_SchoolCycleLabel(school), value, threshold);
    else
        snprintf(buf, size, "%s %d", RPG_SchoolCycleLabel(school), value);
}
