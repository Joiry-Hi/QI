// Equipment growth, artifact resonance, and lightweight alchemy for Web run mode.

typedef struct {
    int index;
    ElixirID elixir_id;
    const char *name;
    int herb_cost;
    XIUWEI required_realm;
} AlchemyRecipe;

static const AlchemyRecipe g_alchemy_recipes[] = {
    {0, ELIXIR_HEALING, "回春丹", 3, MORTAL},
    {1, ELIXIR_QI, "聚气丹", 3, MORTAL},
    {2, ELIXIR_CLEAR_MIND, "清心丹", 4, MORTAL},
    {3, ELIXIR_BERSERK, "爆元丹", 7, FOUNDATION},
    {4, ELIXIR_BREAKTHROUGH, "破境丹", 10, CORE_FORM},
    {5, ELIXIR_WOOD_SPIRIT, "木灵丹", 5, REFINING},
    {6, ELIXIR_EXORCISM, "辟邪丹", 6, FOUNDATION},
    {7, ELIXIR_FOCUS_INSIGHT, "凝神丹", 8, FOUNDATION},
    {8, ELIXIR_MERIDIAN_GUARD, "护脉丹", 9, CORE_FORM},
};

static int Alchemy_RecipeCount(void)
{
    return (int)(sizeof(g_alchemy_recipes) / sizeof(g_alchemy_recipes[0]));
}

static const AlchemyRecipe *Alchemy_GetRecipe(int index)
{
    if (index < 0 || index >= Alchemy_RecipeCount())
        return NULL;
    return &g_alchemy_recipes[index];
}

static bool Alchemy_RecipeKnown(const Player *player, const AlchemyRecipe *recipe)
{
    return player && recipe && recipe->index >= 0 && recipe->index < MAX_ALCHEMY_RECIPES &&
           player->unlocked_alchemy_recipes[recipe->index];
}

static bool Alchemy_RecipeRealmReady(const Player *player, const AlchemyRecipe *recipe)
{
    return player && recipe && player->XIUWEI >= recipe->required_realm;
}

static bool Alchemy_RecipeUnlocked(const Player *player, const AlchemyRecipe *recipe)
{
    return Alchemy_RecipeKnown(player, recipe) && Alchemy_RecipeRealmReady(player, recipe);
}

static bool Alchemy_CanBrew(const Player *player, const AlchemyRecipe *recipe)
{
    return Alchemy_RecipeUnlocked(player, recipe) && player->herb_materials >= recipe->herb_cost;
}

static int Artifact_UpgradeCost(int level)
{
    switch (level)
    {
    case 0: return 4;
    case 1: return 8;
    case 2: return 14;
    case 3: return 22;
    case 4: return 32;
    case 5: return 45;
    default: return 0;
    }
}

static int Artifact_LevelAtSlot(const Player *player, int slot)
{
    if (!player || slot < 0 || slot >= MAX_ARTIFACT_SLOTS)
        return 0;
    int level = player->artifact_levels[slot];
    if (level < 0) level = 0;
    if (level > MAX_ARTIFACT_LEVEL) level = MAX_ARTIFACT_LEVEL;
    return level;
}

static int Player_ArtifactSlot(const Player *player, ArtifactID id)
{
    if (!player)
        return -1;
    for (int i = 0; i < player->artifact_count; i++)
    {
        if (player->artifacts[i] == id)
            return i;
    }
    return -1;
}

static int Player_ArtifactLevel(const Player *player, ArtifactID id)
{
    return Artifact_LevelAtSlot(player, Player_ArtifactSlot(player, id));
}

static bool Artifact_ResonanceActive(const Player *player, int slot)
{
    if (!player || slot < 0 || slot >= player->artifact_count)
        return false;
    ArtifactID id = player->artifacts[slot];
    if (id <= ARTIFACT_NONE || id >= TOTAL_ARTIFACTS)
        return false;
    SchoolTag school = g_artifact_database[id].reward_school_bias;
    if (school <= SCHOOL_NONE || school >= TOTAL_SCHOOLS)
        return false;
    return player->school_counts[school] >= 2;
}

static void Artifact_ResonanceSummary(const Player *player, int slot, char *buf, size_t size)
{
    if (size == 0)
        return;
    buf[0] = '\0';
    if (!player || slot < 0 || slot >= player->artifact_count)
        return;
    ArtifactID id = player->artifacts[slot];
    if (id <= ARTIFACT_NONE || id >= TOTAL_ARTIFACTS)
        return;
    SchoolTag school = g_artifact_database[id].reward_school_bias;
    if (school <= SCHOOL_NONE || school >= TOTAL_SCHOOLS)
        snprintf(buf, size, "暂无共鸣");
    else if (Artifact_ResonanceActive(player, slot))
        snprintf(buf, size, "%s共鸣", SchoolTagName(school));
    else
        snprintf(buf, size, "%s功法 2 门可共鸣", SchoolTagName(school));
}

static int Artifact_GenericLevelBonus(int base, int level)
{
    if (base <= 0 || level <= 0)
        return base;
    int bonus = (base * level + 4) / 5;
    return base + (bonus > 0 ? bonus : level);
}

static int Artifact_EffectiveIntBonusAtSlot(const Player *player, int slot, int (*read_bonus)(const Artifact *artifact))
{
    if (!player || slot < 0 || slot >= player->artifact_count)
        return 0;
    ArtifactID id = player->artifacts[slot];
    if (id <= ARTIFACT_NONE || id >= TOTAL_ARTIFACTS)
        return 0;
    const Artifact *artifact = &g_artifact_database[id];
    int base = read_bonus(artifact);
    int level = Artifact_LevelAtSlot(player, slot);
    if (!artifact->is_custom)
    {
        if (read_bonus == ArtifactBreakthroughPct && id == ARTIFACT_BREAKTHROUGH_SEAL)
            return base + level * 3;
        if (read_bonus == ArtifactPostBattleCultivation && id == ARTIFACT_DEVOURING_ORB)
            return base + level;
        return base;
    }
    return Artifact_GenericLevelBonus(base, level);
}

static int Player_ArtifactEffectiveIntBonus(const Player *player, int (*read_bonus)(const Artifact *artifact))
{
    int total = 0;
    if (!player)
        return 0;
    for (int i = 0; i < player->artifact_count; i++)
        total += Artifact_EffectiveIntBonusAtSlot(player, i, read_bonus);
    return total;
}

static int Artifact_QiGourdExtra(const Player *player)
{
    int level = Player_ArtifactLevel(player, ARTIFACT_QI_GOURD);
    return Player_HasArtifact(player, ARTIFACT_QI_GOURD) ? 1 + level : 0;
}

static float Artifact_TurtleArmorDamageMultiplier(const Player *player)
{
    int level = Player_ArtifactLevel(player, ARTIFACT_TURTLE_ARMOR);
    float multiplier = 0.75f - 0.05f * level;
    return multiplier < 0.60f ? 0.60f : multiplier;
}

static int Artifact_BloodBannerBleedPct(const Player *player)
{
    return Player_HasArtifact(player, ARTIFACT_BLOOD_BANNER) ? 2 + Player_ArtifactLevel(player, ARTIFACT_BLOOD_BANNER) : 0;
}

static int Artifact_ThunderWoodChance(const Player *player)
{
    return Player_HasArtifact(player, ARTIFACT_THUNDER_WOOD_SWORD) ? 30 + Player_ArtifactLevel(player, ARTIFACT_THUNDER_WOOD_SWORD) * 5 : 0;
}

static int Artifact_ThunderWoodDamagePct(const Player *player)
{
    return Player_HasArtifact(player, ARTIFACT_THUNDER_WOOD_SWORD) ? 4 + Player_ArtifactLevel(player, ARTIFACT_THUNDER_WOOD_SWORD) : 0;
}

static int Artifact_ScarletSunDamagePct(const Player *player)
{
    return Player_HasArtifact(player, ARTIFACT_SCARLET_SUN_FURNACE) ? 3 + Player_ArtifactLevel(player, ARTIFACT_SCARLET_SUN_FURNACE) : 0;
}

static int Artifact_SoulBellStatusBonus(const Player *player)
{
    return Player_HasArtifact(player, ARTIFACT_SOUL_BELL) ? 1 + Player_ArtifactLevel(player, ARTIFACT_SOUL_BELL) / 2 : 0;
}

static int Artifact_GreenWoodHealPct(const Player *player)
{
    return Player_HasArtifact(player, ARTIFACT_GREEN_WOOD_TABLET) ? 10 + Player_ArtifactLevel(player, ARTIFACT_GREEN_WOOD_TABLET) * 5 : 0;
}

static int Artifact_ThunderJadeQiPct(const Player *player)
{
    return Player_HasArtifact(player, ARTIFACT_THUNDER_JADE) ? 6 + Player_ArtifactLevel(player, ARTIFACT_THUNDER_JADE) * 2 : 0;
}

static float Artifact_HundredMirrorLowHpMultiplier(const Player *player)
{
    if (!Player_HasArtifact(player, ARTIFACT_HUNDRED_REFINED_MIRROR))
        return 1.0f;
    int level = Player_ArtifactLevel(player, ARTIFACT_HUNDRED_REFINED_MIRROR);
    float multiplier = 0.88f - 0.04f * level;
    return multiplier < 0.72f ? 0.72f : multiplier;
}

static float Artifact_WindCharmEvadeBonus(const Player *player)
{
    if (!Player_HasArtifact(player, ARTIFACT_WIND_CHARM))
        return 0.0f;
    return 0.08f + 0.02f * Player_ArtifactLevel(player, ARTIFACT_WIND_CHARM);
}

static int Equipment_ResonanceExtraLayer(const Player *player, SchoolTag school)
{
    if (!player || school <= SCHOOL_NONE || school >= TOTAL_SCHOOLS)
        return 0;
    for (int i = 0; i < player->artifact_count; i++)
    {
        ArtifactID id = player->artifacts[i];
        if (id > ARTIFACT_NONE && id < TOTAL_ARTIFACTS &&
            g_artifact_database[id].reward_school_bias == school &&
            Artifact_ResonanceActive(player, i))
            return 1;
    }
    return 0;
}
