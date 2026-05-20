// Skill effect helpers. Kept as an include to preserve the single-file build.

typedef struct {
    float damage_multiplier;
    float reflect_multiplier;
    bool use_shield_block;
    bool defensive_interaction;
    const char *verb_chn;
    const char *verb_eng;
} SkillTypeInteraction;

static bool SkillEffect_ShouldTrigger(float chance)
{
    if (chance <= 0.0f)
        return false;
    if (chance >= 1.0f)
        return true;
    return (rand() % 10000) < (int)(chance * 10000.0f);
}

static const SkillEffect *Skill_GetEffects(const Skill *skill, SkillEffect fallback[MAX_SKILL_EFFECTS], int *count)
{
    *count = 0;
    if (skill->effect_count > 0)
    {
        int safe_count = skill->effect_count;
        if (safe_count > MAX_SKILL_EFFECTS)
            safe_count = MAX_SKILL_EFFECTS;
        *count = safe_count;
        return skill->effects;
    }

    if (skill->is_custom && skill->custom_effect != CUSTOM_EFFECT_NONE)
    {
        SkillEffect effect = {SKILL_EFFECT_NONE, skill->custom_value, 1.0f};
        switch (skill->custom_effect)
        {
        case CUSTOM_EFFECT_DAMAGE: effect.type = SKILL_EFFECT_DAMAGE_PCT; break;
        case CUSTOM_EFFECT_HEAL_HP_PCT: effect.type = SKILL_EFFECT_HEAL_HP_PCT; break;
        case CUSTOM_EFFECT_GAIN_QI_PCT: effect.type = SKILL_EFFECT_GAIN_QI_PCT; break;
        case CUSTOM_EFFECT_BLEED: effect.type = SKILL_EFFECT_BLEED; break;
        case CUSTOM_EFFECT_CURSE: effect.type = SKILL_EFFECT_CURSE; break;
        case CUSTOM_EFFECT_ENRAGE: effect.type = SKILL_EFFECT_ENRAGE; break;
        case CUSTOM_EFFECT_CLEANSE: effect.type = SKILL_EFFECT_CLEANSE; break;
        case CUSTOM_EFFECT_CULTIVATION: effect.type = SKILL_EFFECT_CULTIVATION; break;
        default: break;
        }
        if (effect.type != SKILL_EFFECT_NONE)
        {
            fallback[0] = effect;
            *count = 1;
        }
    }
    return fallback;
}

static void Apply_OnCast_SkillEffects(Player *caster, const Skill *skill)
{
    SkillEffect fallback[MAX_SKILL_EFFECTS];
    int count = 0;
    const SkillEffect *effects = Skill_GetEffects(skill, fallback, &count);
    for (int i = 0; i < count; i++)
    {
        const SkillEffect *effect = &effects[i];
        if (!SkillEffect_ShouldTrigger(effect->chance))
            continue;
        switch (effect->type)
        {
        case SKILL_EFFECT_HEAL_HP_PCT:
            caster->HP_change += Player_PercentOfMaxHP(caster, effect->value);
            break;
        case SKILL_EFFECT_GAIN_QI_PCT:
            caster->QI_change += Player_PercentOfMaxQI(caster, effect->value);
            break;
        case SKILL_EFFECT_ENRAGE:
            caster->enraged += effect->value;
            break;
        case SKILL_EFFECT_CLEANSE:
            caster->bleeding = 0;
            caster->cursed = 0;
            break;
        case SKILL_EFFECT_SELF_DAMAGE_PCT:
            caster->HP_change -= Player_PercentOfMaxHP(caster, effect->value);
            break;
        case SKILL_EFFECT_CULTIVATION:
            if (g_config.run_mode)
                caster->cultivation += effect->value;
            break;
        default:
            break;
        }
    }
}

static void Apply_OnHit_SkillEffects(Player *attacker, Player *defender, const Skill *skill, int final_damage)
{
    SkillEffect fallback[MAX_SKILL_EFFECTS];
    int count = 0;
    const SkillEffect *effects = Skill_GetEffects(skill, fallback, &count);
    for (int i = 0; i < count; i++)
    {
        const SkillEffect *effect = &effects[i];
        if (!SkillEffect_ShouldTrigger(effect->chance))
            continue;
        switch (effect->type)
        {
        case SKILL_EFFECT_DAMAGE_PCT:
            defender->HP_change -= Player_PercentOfMaxHP(attacker, effect->value);
            break;
        case SKILL_EFFECT_BLEED:
            defender->bleeding += effect->value;
            break;
        case SKILL_EFFECT_CURSE:
            defender->cursed += effect->value > 0 ? effect->value : final_damage;
            break;
        case SKILL_EFFECT_STEAL_QI:
        {
            if (skill->skill_id == SKILL_ID_ESSENCE_PLUNDER && defender->current_action_type != ACTION_TYPE_GAIN_QI)
                break;
            int requested = effect->value > 0 ? effect->value : defender->QI / 4;
            int stolen = defender->QI >= requested ? requested : defender->QI;
            defender->QI_change -= stolen;
            attacker->QI_change += stolen;
            if (stolen > 0)
            {
                CHN_PRINT("[%s 夺取了 %s 的 %d 点气力!]\n", attacker->name, defender->name, stolen);
                ENG_PRINT("[%s stole %s's %d QI!]\n", attacker->name, defender->name, stolen);
            }
            break;
        }
        case SKILL_EFFECT_LIFESTEAL_PCT:
            if (final_damage > 0)
                attacker->HP_change += final_damage * effect->value / 100;
            break;
        default:
            break;
        }
    }
}

static SkillTypeInteraction Get_TypeInteraction(TypeID attack, TypeID defense, AttributeID attr)
{
    SkillTypeInteraction result = {1.0f, 0.0f, false, false, NULL, NULL};
    switch (defense)
    {
    case TYPE_RESIST:
        result.defensive_interaction = true;
        result.verb_chn = "抵抗";
        result.verb_eng = "resist";
        break;
    case TYPE_SHIELD:
        result.use_shield_block = true;
        result.defensive_interaction = true;
        result.verb_chn = "抵挡";
        result.verb_eng = "block";
        break;
    case TYPE_PARRY:
        result.defensive_interaction = true;
        result.verb_chn = "格挡";
        result.verb_eng = "parry";
        if (attack == TYPE_SLASH)
        {
            result.damage_multiplier = 0.0f;
            result.reflect_multiplier = 1.0f;
        }
        else if (attack == TYPE_SMASH)
        {
            result.damage_multiplier = 0.5f;
            result.reflect_multiplier = 0.5f;
        }
        else
        {
            result.damage_multiplier = 0.6f;
        }
        break;
    case TYPE_REFLECT:
        result.defensive_interaction = true;
        result.verb_chn = "反射";
        result.verb_eng = "reflect";
        if (attr == ATTR_KARMA)
        {
            result.damage_multiplier = 1.0f;
            result.reflect_multiplier = 0.0f;
        }
        else if (attack == TYPE_SMASH)
        {
            result.damage_multiplier = 0.5f;
            result.reflect_multiplier = 0.5f;
        }
        else
        {
            result.damage_multiplier = 0.0f;
            result.reflect_multiplier = 1.0f;
        }
        break;
    case TYPE_FORCEFIELD:
        result.defensive_interaction = true;
        result.verb_chn = "弹开";
        result.verb_eng = "scatter";
        if (attack == TYPE_BURST || attack == TYPE_PROJECT)
            result.damage_multiplier = 0.2f;
        else if (attack == TYPE_BLAST)
            result.damage_multiplier = 0.3f;
        else
            result.damage_multiplier = 0.6f;
        break;
    default:
        break;
    }
    return result;
}

static bool Skill_TypeCanEvade(TypeID type)
{
    return type == TYPE_PIERCE || type == TYPE_SLASH || type == TYPE_SMASH || type == TYPE_PROJECT;
}

static void SkillEffectSummary(const Skill *skill, char *buf, size_t size)
{
    if (size == 0)
        return;
    buf[0] = '\0';
    if (skill->attribute_id == ATTR_BLOOD && skill->effect_strength > 0)
    {
        char part[64];
        snprintf(part, sizeof(part), "命中：流血 %d；命中：吸血", skill->effect_strength);
        strncat(buf, part, size - strlen(buf) - 1);
    }
    if (skill->attribute_id == ATTR_DARK && skill->effect_strength > 0)
    {
        char part[64];
        snprintf(part, sizeof(part), "命中：夺气 %d", skill->effect_strength);
        if (buf[0] != '\0')
            strncat(buf, "；", size - strlen(buf) - 1);
        strncat(buf, part, size - strlen(buf) - 1);
    }
    SkillEffect fallback[MAX_SKILL_EFFECTS];
    int count = 0;
    const SkillEffect *effects = Skill_GetEffects(skill, fallback, &count);
    for (int i = 0; i < count; i++)
    {
        const SkillEffect *effect = &effects[i];
        const char *label = NULL;
        char part[64];
        switch (effect->type)
        {
        case SKILL_EFFECT_DAMAGE_PCT: label = "命中：额外伤害"; break;
        case SKILL_EFFECT_HEAL_HP_PCT: label = "施法：恢复生命"; break;
        case SKILL_EFFECT_GAIN_QI_PCT: label = "施法：获得灵气"; break;
        case SKILL_EFFECT_BLEED: label = "命中：流血"; break;
        case SKILL_EFFECT_CURSE: label = "命中：诅咒"; break;
        case SKILL_EFFECT_ENRAGE: label = "施法：激怒"; break;
        case SKILL_EFFECT_CLEANSE: label = "施法：净化"; break;
        case SKILL_EFFECT_STEAL_QI: label = "命中：夺气"; break;
        case SKILL_EFFECT_SELF_DAMAGE_PCT: label = "施法：自损生命"; break;
        case SKILL_EFFECT_CULTIVATION: label = "施法：修为"; break;
        case SKILL_EFFECT_LIFESTEAL_PCT: label = "命中：吸血"; break;
        default: break;
        }
        if (!label)
            continue;
        if (effect->type == SKILL_EFFECT_CLEANSE)
            snprintf(part, sizeof(part), "%s", label);
        else
            snprintf(part, sizeof(part), "%s %d", label, effect->value);
        if (buf[0] != '\0')
            strncat(buf, "；", size - strlen(buf) - 1);
        strncat(buf, part, size - strlen(buf) - 1);
    }
}
