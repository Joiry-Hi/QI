// RPG database definitions. Kept as an include to preserve the single-file build.
void Initialize_Databases()
{
    Initialize_TalentDatabase();

    g_skill_database[SKILL_ID_GAIN_QI] = (Skill){
        .skill_id = SKILL_ID_GAIN_QI, .action_type = ACTION_TYPE_GAIN_QI, .name_chn = "集气", .name_eng = "Gain QI",
        .hotkey = 'Q', .cost = 0, .rank = 0, .type_id = TYPE_BUFF, .attribute_id = ATTR_NONE, .base_power = 1.0f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_SELF,
        .prompt_chn = "<%s 开始引导天地灵气...>", .prompt_eng = "<%s begins to channel worldly energy...>",
        .prereq_skill_id = SKILL_ID_NONE, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = MORTAL, .school_tag = SCHOOL_QI};
    g_skill_database[SKILL_ID_STRIKE] = (Skill){
        .skill_id = SKILL_ID_STRIKE, .action_type = ACTION_TYPE_MELEE, .name_chn = "轻击", .name_eng = "Melee",
        .hotkey = 'A', .cost = 1, .rank = 0, .type_id = TYPE_SLASH, .attribute_id = ATTR_PHYSICAL, .base_power = 1.0f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 发动了迅捷的攻击!>", .prompt_eng = "<%s unleashes a swift strike!>",
        .prereq_skill_id = SKILL_ID_NONE, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = MORTAL, .school_tag = SCHOOL_SWORD};
    g_skill_database[SKILL_ID_DEFEND] = (Skill){
        .skill_id = SKILL_ID_DEFEND, .action_type = ACTION_TYPE_DEFEND, .name_chn = "防御", .name_eng = "Defend",
        .hotkey = 'D', .cost = 1, .rank = 0, .type_id = TYPE_RESIST, .attribute_id = ATTR_NONE, .base_power = 0.6f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_SELF,
        .prompt_chn = "<%s 摆开了防御架势!>", .prompt_eng = "<%s raises a defensive guard!>",
        .prereq_skill_id = SKILL_ID_NONE, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = MORTAL, .school_tag = SCHOOL_DEFENSE};
    g_skill_database[SKILL_ID_HEAL] = (Skill){
        .skill_id = SKILL_ID_HEAL, .action_type = ACTION_TYPE_HEAL, .name_chn = "养元", .name_eng = "Heal",
        .hotkey = 'H', .cost = 1, .rank = 0, .type_id = TYPE_HEAL, .attribute_id = ATTR_WOOD, .base_power = 1.0f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_SELF,
        .prompt_chn = "<%s 开始运气修复全身经脉>", .prompt_eng = "<%s starts to circulate Qi to repair the body's meridians>",
        .prereq_skill_id = SKILL_ID_NONE, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = MORTAL, .school_tag = SCHOOL_BASIC};
    g_skill_database[SKILL_ID_WARCRY] = (Skill){
        .skill_id = SKILL_ID_WARCRY, .action_type = ACTION_TYPE_BOOST, .name_chn = "战吼", .name_eng = "Warcry",
        .hotkey = 'C', .cost = 2, .rank = 0, .type_id = TYPE_DEBUFF, .attribute_id = ATTR_NONE, .base_power = 1.0f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 发出一声震慑心魄的战吼!>", .prompt_eng = "<%s lets out a deafening warcry!>",
        .prereq_skill_id = SKILL_ID_NONE, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = MORTAL, .school_tag = SCHOOL_QI};
    g_skill_database[SKILL_ID_PARRY] = (Skill){
        .skill_id = SKILL_ID_PARRY, .action_type = ACTION_TYPE_COUNTER, .name_chn = "格挡", .name_eng = "Parry",
        .hotkey = 'P', .cost = 2, .rank = 0, .type_id = TYPE_PARRY, .attribute_id = ATTR_PHYSICAL, .base_power = 0.5f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_SELF,
        .prompt_chn = "<%s 眼神一凝，摆出了完美的格挡架势。>", .prompt_eng = "<%s takes a perfect parry stance, ready to counter!>",
        .prereq_skill_id = SKILL_ID_NONE, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = MORTAL, .school_tag = SCHOOL_DEFENSE};
    g_skill_database[SKILL_ID_SMITE] = (Skill){
        .skill_id = SKILL_ID_SMITE, .action_type = ACTION_TYPE_SMITE, .name_chn = "重击", .name_eng = "Smite",
        .hotkey = 'S', .cost = 3, .rank = 0, .type_id = TYPE_SMASH, .attribute_id = ATTR_PHYSICAL, .base_power = 4.0f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 汇聚全身力气，发动了沉重的猛击!>", .prompt_eng = "<%s channels their strength into a powerful smite!>",
        .prereq_skill_id = SKILL_ID_NONE, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = MORTAL, .school_tag = SCHOOL_SWORD};

    g_skill_database[SKILL_ID_FIREBALL] = (Skill){
        .skill_id = SKILL_ID_FIREBALL, .action_type = ACTION_TYPE_RANGED, .name_chn = "火球", .name_eng = "Fireball",
        .hotkey = 'R', .cost = 2, .rank = 1, .type_id = TYPE_PROJECT, .attribute_id = ATTR_FIRE, .base_power = 1.0f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 掌心凝聚出一颗灼热的火球!>", .prompt_eng = "<%s conjures and hurls a searing fireball!>",
        .prereq_skill_id = SKILL_ID_NONE, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = REFINING, .school_tag = SCHOOL_FIRE};
    g_skill_database[SKILL_ID_ENERGY_SHIELD] = (Skill){
        .skill_id = SKILL_ID_ENERGY_SHIELD, .action_type = ACTION_TYPE_DEFEND, .name_chn = "灵力盾", .name_eng = "Energy Shield",
        .hotkey = 'D', .cost = 2, .rank = 1, .type_id = TYPE_SHIELD, .attribute_id = ATTR_SPIRITUAL, .base_power = 5.0f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_SELF,
        .prompt_chn = "<%s 以灵力在身前构筑了一面无形的护盾!>", .prompt_eng = "<%s summons a shimmering shield of pure energy!>",
        .prereq_skill_id = SKILL_ID_NONE, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = REFINING, .school_tag = SCHOOL_DEFENSE};
    g_skill_database[SKILL_ID_WINDBLADE] = (Skill){
        .skill_id = SKILL_ID_WINDBLADE, .action_type = ACTION_TYPE_BURST, .name_chn = "风刃", .name_eng = "Wind Blade",
        .hotkey = 'B', .cost = 3, .rank = 1, .type_id = TYPE_BURST, .attribute_id = ATTR_WIND, .base_power = 1.0f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 挥手间，数道锋利的风刃破空而去!>", .prompt_eng = "<%s unleashes a flurry of razor-sharp wind blades!>",
        .prereq_skill_id = SKILL_ID_NONE, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = REFINING, .school_tag = SCHOOL_SWORD};
    g_skill_database[SKILL_ID_EVERGREEN_ART] = (Skill){
        .skill_id = SKILL_ID_EVERGREEN_ART, .action_type = ACTION_TYPE_HEAL, .name_chn = "长春功", .name_eng = "Evergreen Art",
        .hotkey = 'H', .cost = 4, .rank = 1, .type_id = TYPE_HEAL, .attribute_id = ATTR_WOOD, .base_power = 2.5f,
        .effect_strength = 3, .effect_chance = 1.0f, .target_type = TARGET_SELF,
        .prompt_chn = "<%s 运转长春功，周身泛起绿色荧光，生命力缓缓恢复。>", .prompt_eng = "<%s channels the Evergreen Art, glowing with a green light as life force recovers.>",
        .prereq_skill_id = SKILL_ID_NONE, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = REFINING, .school_tag = SCHOOL_BASIC};
    g_skill_database[SKILL_ID_CONCENTRATION] = (Skill){
        .skill_id = SKILL_ID_CONCENTRATION, .action_type = ACTION_TYPE_BOOST, .name_chn = "凝神", .name_eng = "Get Focused",
        .hotkey = 'C', .cost = 3, .rank = 1, .type_id = TYPE_BUFF, .attribute_id = ATTR_SPIRITUAL, .base_power = 1.0f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_SELF,
        .prompt_chn = "<%s 凝气入体，聚精会神>", .prompt_eng = "<%s saves all his spirits for next move>",
        .prereq_skill_id = SKILL_ID_NONE, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = REFINING, .school_tag = SCHOOL_QI};

    g_skill_database[SKILL_ID_FLAMEBLAST] = (Skill){
        .skill_id = SKILL_ID_FLAMEBLAST, .action_type = ACTION_TYPE_RANGED, .name_chn = "炎爆弹", .name_eng = "Flame Blast",
        .hotkey = 'R', .cost = 3, .rank = 2, .type_id = TYPE_BLAST, .attribute_id = ATTR_FIRE, .base_power = 2.0f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 吟唱片刻，一颗毁灭性的炎爆弹呼啸而出!>", .prompt_eng = "<%s unleashes a devastating blast of fire and flame!>",
        .prereq_skill_id = SKILL_ID_FIREBALL, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = FOUNDATION, .school_tag = SCHOOL_FIRE};
    g_skill_database[SKILL_ID_GOLD_LIGHT_WARDING] = (Skill){
        .skill_id = SKILL_ID_GOLD_LIGHT_WARDING, .action_type = ACTION_TYPE_DEFEND, .name_chn = "金光护体", .name_eng = "Gold Light Warding",
        .hotkey = 'D', .cost = 4, .rank = 2, .type_id = TYPE_FORCEFIELD, .attribute_id = ATTR_LIGHT, .base_power = 1.0f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_SELF,
        .prompt_chn = "<%s 周身金光大盛，形成了坚不可摧的护体神功!>", .prompt_eng = "<%s is enveloped in a brilliant ward of golden light!>",
        .prereq_skill_id = SKILL_ID_ENERGY_SHIELD, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = FOUNDATION, .school_tag = SCHOOL_DEFENSE};
    g_skill_database[SKILL_ID_GREATSWORD] = (Skill){
        .skill_id = SKILL_ID_GREATSWORD, .action_type = ACTION_TYPE_SMITE, .name_chn = "巨剑术", .name_eng = "Greatsword Mastery",
        .hotkey = 'S', .cost = 6, .rank = 2, .type_id = TYPE_SMASH, .attribute_id = ATTR_METAL, .base_power = 4.0f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 凝聚出一柄巨剑，携万钧之势劈下!>", .prompt_eng = "<%s forms a Greatsword and smites with immense force!>",
        .prereq_skill_id = SKILL_ID_SMITE, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = FOUNDATION, .school_tag = SCHOOL_SWORD};
    g_skill_database[SKILL_ID_COMMANDING_SWORDS] = (Skill){
        .skill_id = SKILL_ID_COMMANDING_SWORDS, .action_type = ACTION_TYPE_BURST, .name_chn = "灵剑", .name_eng = "Soul Swords",
        .hotkey = 'B', .cost = 3, .rank = 2, .type_id = TYPE_BURST, .attribute_id = ATTR_PHYSICAL, .base_power = 1.5f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 轻喝一声：“剑来！”，无数飞剑应声而至!>", .prompt_eng = "<%s utters a single command: \"Swords, arise!\", and countless blades answer the call!>",
        .prereq_skill_id = SKILL_ID_WINDBLADE, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = FOUNDATION, .school_tag = SCHOOL_SWORD};
    g_skill_database[SKILL_ID_TERMINATE_THUNDER] = (Skill){
        .skill_id = SKILL_ID_TERMINATE_THUNDER, .action_type = ACTION_TYPE_TERMINATE, .name_chn = "唤雷", .name_eng = "Thunderbolt",
        .hotkey = 'T', .cost = 10, .rank = 2, .type_id = TYPE_SMASH, .attribute_id = ATTR_THUNDER, .base_power = 5.0f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 吟诵咒语，引九天神雷轰向敌人!>", .prompt_eng = "<%s chants and evokes thunder from the heavens!>",
        .prereq_skill_id = SKILL_ID_FIREBALL, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = FOUNDATION, .school_tag = SCHOOL_THUNDER};

    g_skill_database[SKILL_ID_SWORD_PHANTOM] = (Skill){
        .skill_id = SKILL_ID_SWORD_PHANTOM, .action_type = ACTION_TYPE_MELEE, .name_chn = "剑影分光术", .name_eng = "Sword Phantom Art",
        .hotkey = 'A', .cost = 6, .rank = 3, .type_id = TYPE_BURST, .attribute_id = ATTR_WIND, .base_power = 0.5f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 剑诀一引，一道剑光骤然分化为万千幻影，如狂风骤雨般射向所有敌人！>", .prompt_eng = "<%s summons the art, splitting one sword light into myriad phantoms that rain upon all foes!>",
        .prereq_skill_id = SKILL_ID_COMMANDING_SWORDS, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = CORE_FORM, .school_tag = SCHOOL_SWORD};
    g_skill_database[SKILL_ID_BLOOD_DEVIL_DRILL] = (Skill){
        .skill_id = SKILL_ID_BLOOD_DEVIL_DRILL, .action_type = ACTION_TYPE_RANGED, .name_chn = "血魔钻", .name_eng = "Blood-Devil Drill",
        .hotkey = 'R', .cost = 9, .rank = 3, .type_id = TYPE_PIERCE, .attribute_id = ATTR_BLOOD, .base_power = 5.0f,
        .effect_strength = 3, .effect_chance = 0.5f, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 指尖凝聚出一道血色螺旋，化作血魔钻激射而出，直取对手要害！>", .prompt_eng = "<%s condenses a spiraling bolt of blood energy, unleashing it as a drill that pierces towards the enemy!>",
        .prereq_skill_id = SKILL_ID_FLAMEBLAST, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = CORE_FORM, .school_tag = SCHOOL_BLOOD};
    g_skill_database[SKILL_ID_CORE_RESTORATION] = (Skill){
        .skill_id = SKILL_ID_CORE_RESTORATION, .action_type = ACTION_TYPE_HEAL, .name_chn = "丹元归一", .name_eng = "Golden Core Restoration",
        .hotkey = 'H', .cost = 10, .rank = 3, .type_id = TYPE_HEAL, .attribute_id = ATTR_LIGHT, .base_power = 6.5f,
        .effect_strength = 2, .effect_chance = 0.5f, .target_type = TARGET_SELF,
        .prompt_chn = "<%s 催动金丹，精纯的丹元之力流转百骸，迅速修复着受损的经脉。>", .prompt_eng = "<%s activates the Golden Core, its pure essence flowing through the meridians, rapidly mending the damage.>",
        .prereq_skill_id = SKILL_ID_EVERGREEN_ART, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = CORE_FORM, .school_tag = SCHOOL_BASIC};
    g_skill_database[SKILL_ID_CORE_ERUPTION] = (Skill){
        .skill_id = SKILL_ID_CORE_ERUPTION, .action_type = ACTION_TYPE_BOOST, .name_chn = "丹元爆发", .name_eng = "Core Eruption",
        .hotkey = 'C', .cost = 0, .rank = 3, .type_id = TYPE_BUFF, .attribute_id = ATTR_NONE, .base_power = 2.0f,
        .effect_strength = 2, .effect_chance = 0, .target_type = TARGET_SELF,
        .prompt_chn = "<%s 碎丹求道，霎时气场狂涨！>", .prompt_eng = "<%s shattering the Core, his presence surged with terrifying might.>",
        .prereq_skill_id = SKILL_ID_CONCENTRATION, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = CORE_FORM, .school_tag = SCHOOL_QI,
        .effects = {{SKILL_EFFECT_SELF_DAMAGE_PCT, 30, 1.0f}}, .effect_count = 1};
    g_skill_database[SKILL_ID_BLOOD_DEVIL_SLASH] = (Skill){
        .skill_id = SKILL_ID_BLOOD_DEVIL_SLASH, .action_type = ACTION_TYPE_SMITE, .name_chn = "血魔斩", .name_eng = "Blood-Devil Slash",
        .hotkey = 'X', .cost = 12, .rank = 3, .type_id = TYPE_SLASH, .attribute_id = ATTR_BLOOD, .base_power = 10.0f,
        .effect_strength = 3, .effect_chance = 0.4f, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 催动魔功，一道血色斩击裹挟着无尽凶煞之气，当头劈下！>", .prompt_eng = "<%s channels demonic power, unleashing a blood-colored slash filled with baleful energy!>",
        .prereq_skill_id = SKILL_ID_BLOOD_DEVIL_DRILL, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = CORE_FORM, .school_tag = SCHOOL_BLOOD};
    g_skill_database[SKILL_ID_BEETLE_SWARM] = (Skill){
        .skill_id = SKILL_ID_BEETLE_SWARM, .action_type = ACTION_TYPE_BURST, .name_chn = "噬金虫群", .name_eng = "Gold Devouring Beetle Swarm",
        .hotkey = 'B', .cost = 3, .rank = 3, .type_id = TYPE_BURST, .attribute_id = ATTR_PHYSICAL, .base_power = 1.0f,
        .effect_strength = 3, .effect_chance = 0.8f, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 放出噬金虫群，嗡鸣声中，一片金色虫云遮天蔽日般扑去！>", .prompt_eng = "<%s releases a swarm of Gold Devouring Beetles, a golden cloud that blots out the sky and engulfs the enemy!>",
        .prereq_skill_id = SKILL_ID_COMMANDING_SWORDS, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = CORE_FORM, .school_tag = SCHOOL_SWORD};
    g_skill_database[SKILL_ID_ICE_FLAME] = (Skill){
        .skill_id = SKILL_ID_ICE_FLAME, .action_type = ACTION_TYPE_TERMINATE, .name_chn = "乾蓝冰焰", .name_eng = "Dry Blue Ice Flame",
        .hotkey = 'I', .cost = 20, .rank = 3, .type_id = TYPE_DEBUFF, .attribute_id = ATTR_ICE, .base_power = 3.0f,
        .effect_strength = 5, .effect_chance = 0.95f, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 祭出乾蓝冰焰，一朵幽蓝冷火悄然印向对手，欲冻结其元神！>", .prompt_eng = "<%s summons the Dry Blue Ice Flame, a chilling blue fire that silently seeks to freeze the enemy's spirit!>",
        .prereq_skill_id = SKILL_ID_BLOOD_DEVIL_DRILL, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = CORE_FORM, .school_tag = SCHOOL_BASIC,
        .effects = {{SKILL_EFFECT_CURSE, 0, 1.0f}}, .effect_count = 1};

    g_skill_database[SKILL_ID_IMMOVABLE_KING] = (Skill){
        .skill_id = SKILL_ID_IMMOVABLE_KING, .action_type = ACTION_TYPE_DEFEND, .name_chn = "不动明王", .name_eng = "Immovable Bright King",
        .hotkey = 'D', .cost = 6, .rank = 4, .type_id = TYPE_RESIST, .attribute_id = ATTR_METAL, .base_power = 0.2f,
        .effect_strength = 3, .effect_chance = 1.0f, .target_type = TARGET_SELF,
        .prompt_chn = "<%s 宝相庄严，体表浮现金色梵文，化为不动明王法身，稳如山岳！>", .prompt_eng = "<%s's body glows with golden sanskrit, forming the Immovable King's aegis, as steadfast as a mountain!>",
        .prereq_skill_id = SKILL_ID_GOLD_LIGHT_WARDING, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = NASCENT_SOUL, .school_tag = SCHOOL_DEFENSE};
    g_skill_database[SKILL_ID_STELLAR_SHIFT] = (Skill){
        .skill_id = SKILL_ID_STELLAR_SHIFT, .action_type = ACTION_TYPE_COUNTER, .name_chn = "斗转星移", .name_eng = "Stellar Shift",
        .hotkey = 'P', .cost = 10, .rank = 4, .type_id = TYPE_REFLECT, .attribute_id = ATTR_SPACE, .base_power = 0.8f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_SELF,
        .prompt_chn = "<%s 身形一晃，引动敌方攻势，周天星斗仿佛随之逆转！>", .prompt_eng = "<%s's form blurs, redirecting the attack as if shifting the very stars in the cosmos!>",
        .prereq_skill_id = SKILL_ID_PARRY, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = NASCENT_SOUL, .school_tag = SCHOOL_DEFENSE};
    g_skill_database[SKILL_ID_ESSENCE_PLUNDER] = (Skill){
        .skill_id = SKILL_ID_ESSENCE_PLUNDER, .action_type = ACTION_TYPE_BOOST, .name_chn = "夺元诀", .name_eng = "Essence Plundering Art",
        .hotkey = 'C', .cost = 20, .rank = 4, .type_id = TYPE_DEBUFF, .attribute_id = ATTR_DARK, .base_power = 1.5f,
        .effect_strength = 6, .effect_chance = 0, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 运起诡异步法，掌心产生一股吸力，强行夺取对手正在汇聚的真元！>", .prompt_eng = "<%s uses an uncanny art, creating a vortex in their palm to forcibly plunder the opponent's gathering essence!>",
        .prereq_skill_id = SKILL_ID_CORE_ERUPTION, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = NASCENT_SOUL, .school_tag = SCHOOL_QI,
        .effects = {{SKILL_EFFECT_STEAL_QI, 0, 1.0f}}, .effect_count = 1};
    g_skill_database[SKILL_ID_COSMIC_DHARMA_AVATAR] = (Skill){
        .skill_id = SKILL_ID_COSMIC_DHARMA_AVATAR, .action_type = ACTION_TYPE_SMITE, .name_chn = "法相天地", .name_eng = "Cosmic Dharma Avatar",
        .hotkey = 'S', .cost = 50, .rank = 4, .type_id = TYPE_SMASH, .attribute_id = ATTR_SPIRITUAL, .base_power = 40.0f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 身后浮现出通天彻地的法相，一掌拍下，恍若天倾地陷！>", .prompt_eng = "<%s summons a colossal Dharma Avatar that strikes down with a single palm, like the very heavens collapsing!>",
        .prereq_skill_id = SKILL_ID_GREATSWORD, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = NASCENT_SOUL, .school_tag = SCHOOL_BASIC};
    g_skill_database[SKILL_ID_GREAT_GOLDEN_SWORDFORMATION] = (Skill){
        .skill_id = SKILL_ID_GREAT_GOLDEN_SWORDFORMATION, .action_type = ACTION_TYPE_TERMINATE, .name_chn = "大庚剑阵", .name_eng = "Great Golden Sword Formation",
        .hotkey = 'G', .cost = 100, .rank = 4, .type_id = TYPE_BURST, .attribute_id = ATTR_METAL, .base_power = 2.0f,
        .effect_strength = 3, .effect_chance = 1.0f, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 祭出飞剑，引动天地庚金之气，一座无边剑阵瞬间成型，将对手困入其中！>", .prompt_eng = "<%s summons his swords, forming a vast formation of golden energy that traps the enemy within!>",
        .prereq_skill_id = SKILL_ID_SWORD_PHANTOM, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = NASCENT_SOUL, .school_tag = SCHOOL_SWORD};

    g_skill_database[SKILL_ID_SPIRIT_SLAYING_SWORD] = (Skill){
        .skill_id = SKILL_ID_SPIRIT_SLAYING_SWORD, .action_type = ACTION_TYPE_TERMINATE, .name_chn = "玄天灭灵斩", .name_eng = "Profound Heaven Spirit-Slaying Slash",
        .hotkey = 'Z', .cost = 200, .rank = 5, .type_id = TYPE_SLASH, .attribute_id = ATTR_KARMA, .base_power = 100.0f,
        .effect_strength = 0, .effect_chance = 0, .target_type = TARGET_ENEMY,
        .prompt_chn = "<%s 祭出斩灵剑，一道看似朴实无华的剑光斩出，却蕴含着斩断法则的无上之力！>", .prompt_eng = "<%s wields the Spirit-Slaying Sword, its simple slash containing the supreme power to sever the laws of reality!>",
        .prereq_skill_id = SKILL_ID_GREAT_GOLDEN_SWORDFORMATION, .prereq_action_type = ACTION_TYPE_NONE, .required_realm = SEVERING, .school_tag = SCHOOL_SWORD};

    g_artifact_database[ARTIFACT_QI_GOURD] = (Artifact){.id = ARTIFACT_QI_GOURD, .name_chn = "聚灵葫芦", .name_eng = "Qi Gourd", .desc_chn = "集气额外获得1点气。", .desc_eng = "Gain 1 extra QI when gathering.", .rarity = RARITY_COMMON};
    g_artifact_database[ARTIFACT_TURTLE_ARMOR] = (Artifact){.id = ARTIFACT_TURTLE_ARMOR, .name_chn = "玄龟甲", .name_eng = "Turtle Armor", .desc_chn = "防御或反击时减伤。", .desc_eng = "Reduce damage while defending or countering.", .rarity = RARITY_RARE};
    g_artifact_database[ARTIFACT_BLOOD_BANNER] = (Artifact){.id = ARTIFACT_BLOOD_BANNER, .name_chn = "血煞幡", .name_eng = "Blood Banner", .desc_chn = "血系伤害追加流血，但治疗降低。", .desc_eng = "Blood attacks add bleeding; healing is weaker.", .rarity = RARITY_RARE};
    g_artifact_database[ARTIFACT_THUNDER_WOOD_SWORD] = (Artifact){.id = ARTIFACT_THUNDER_WOOD_SWORD, .name_chn = "雷击木剑", .name_eng = "Thunder Wood Sword", .desc_chn = "雷法与剑系攻击有概率追加伤害。", .desc_eng = "Thunder and sword attacks may deal bonus damage.", .rarity = RARITY_EPIC};
    g_artifact_database[ARTIFACT_KUN_MIRROR] = (Artifact){.id = ARTIFACT_KUN_MIRROR, .name_chn = "乾坤镜", .name_eng = "Kun Mirror", .desc_chn = "每场战斗一次致命伤保留1点元神。", .desc_eng = "Once per battle, survive lethal damage at 1 HP.", .rarity = RARITY_EPIC};
    g_artifact_database[ARTIFACT_DEVOURING_ORB] = (Artifact){.id = ARTIFACT_DEVOURING_ORB, .name_chn = "噬灵珠", .name_eng = "Devouring Orb", .desc_chn = "战斗开始受诅咒，胜利后获得气。", .desc_eng = "Begin cursed, gain QI after victory.", .rarity = RARITY_RARE};
    g_artifact_database[ARTIFACT_GREEN_WOOD_BOTTLE] = (Artifact){.id = ARTIFACT_GREEN_WOOD_BOTTLE, .name_chn = "青木瓶", .name_eng = "Green Wood Bottle", .desc_chn = "治疗额外净化流血与诅咒。", .desc_eng = "Healing also purges bleeding and curses.", .rarity = RARITY_RARE};
    g_artifact_database[ARTIFACT_BREAKTHROUGH_SEAL] = (Artifact){.id = ARTIFACT_BREAKTHROUGH_SEAL, .name_chn = "破境印", .name_eng = "Breakthrough Seal", .desc_chn = "突破概率提高，失败惩罚加重。", .desc_eng = "Better breakthrough odds, harsher failure.", .rarity = RARITY_EPIC};
    g_artifact_database[ARTIFACT_WIND_CHARM] = (Artifact){.id = ARTIFACT_WIND_CHARM, .name_chn = "风行符佩", .name_eng = "Wind Charm", .desc_chn = "闪避提高，受重击后短暂失效。", .desc_eng = "Higher evasion, disabled briefly after smites.", .rarity = RARITY_COMMON};
    g_artifact_database[ARTIFACT_FORMLESS_JADE] = (Artifact){.id = ARTIFACT_FORMLESS_JADE, .name_chn = "无相玉", .name_eng = "Formless Jade", .desc_chn = "第一次破格获得技能无代价。", .desc_eng = "First bypassed skill is free.", .rarity = RARITY_LEGENDARY};

    g_elixir_database[ELIXIR_HEALING] = (Elixir){.id = ELIXIR_HEALING, .name_chn = "回春丹", .name_eng = "Healing Pill", .desc_chn = "立即恢复元神。", .desc_eng = "Restore HP immediately.", .rarity = RARITY_COMMON};
    g_elixir_database[ELIXIR_QI] = (Elixir){.id = ELIXIR_QI, .name_chn = "聚气丹", .name_eng = "Qi Pill", .desc_chn = "立即获得气。", .desc_eng = "Gain QI immediately.", .rarity = RARITY_COMMON};
    g_elixir_database[ELIXIR_CLEAR_MIND] = (Elixir){.id = ELIXIR_CLEAR_MIND, .name_chn = "清心丹", .name_eng = "Clear Mind Pill", .desc_chn = "清除流血和诅咒。", .desc_eng = "Clear bleeding and curse.", .rarity = RARITY_RARE};
    g_elixir_database[ELIXIR_BERSERK] = (Elixir){.id = ELIXIR_BERSERK, .name_chn = "爆元丹", .name_eng = "Essence Burst Pill", .desc_chn = "本场战斗激怒，战后扣血。", .desc_eng = "Gain rage this battle, lose HP after.", .rarity = RARITY_RARE};
    g_elixir_database[ELIXIR_BREAKTHROUGH] = (Elixir){.id = ELIXIR_BREAKTHROUGH, .name_chn = "破境丹", .name_eng = "Breakthrough Pill", .desc_chn = "本场突破概率提高。", .desc_eng = "Improve breakthrough odds this battle.", .rarity = RARITY_EPIC};
    g_elixir_database[ELIXIR_DISASTER_WARD] = (Elixir){.id = ELIXIR_DISASTER_WARD, .name_chn = "化劫丹", .name_eng = "Disaster Ward Pill", .desc_chn = "下一次奇遇负面结果降级。", .desc_eng = "Soften the next bad encounter result.", .rarity = RARITY_EPIC};

    Initialize_CustomContent();
}
