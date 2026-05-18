#include "engine.h"
#include "vfx.h"
#include <stdlib.h>
#include <math.h>

// 辅助随机函数
static float rand_float(float min, float max)
{
    return min + (float)rand() / (float)(RAND_MAX / (max - min));
}

Caster Caster_Create(float x, float y, SDL_Color color)
{
    Caster c;
    c.position.x = x;
    c.position.y = y;
    c.color = color;
    c.energy_radius = 20.0f; // 默认大小
    c.animation_timer = 0.0f;
    return c;
}

void Caster_Update(Caster *c)
{
    // 更新呼吸动画计时器
    c->animation_timer += 0.05f;

    // 1. 物理：实体立场 (The Body Force)
    // 半径很小 (10-12px)，但强度很大 (3.0f)
    // 这意味着粒子只有极其靠近时才会受到剧烈推斥，模拟“撞击感”
    ForceGrid_AddRadialForce(c->position, 15.0f, 3.0f);

    // 计算当前的“呼吸”强度 (0.8 ~ 1.2 倍)
    float breathe_factor = 1.0f + sinf(c->animation_timer) * 0.2f;

    // --- 1. 生成核心粒子 (构成身体) ---
    // 每帧生成多个粒子以形成密集的视觉核心
    int body_particles = 5;
    for (int i = 0; i < body_particles; i++)
    {
        // 在核心半径内随机分布
        float angle = rand_float(0, 6.283f);
        float dist = rand_float(0, 5.0f * breathe_factor);

        Vector2 offset = {cosf(angle) * dist, sinf(angle) * dist};
        Vector2 spawn_pos = Vec2_Add(c->position, offset);

        // 核心粒子几乎不动，或者轻微抖动
        Vector2 vel = {rand_float(-0.2f, 0.2f), rand_float(-0.2f, 0.2f)};

        // 核心粒子寿命短，必须快速刷新以保持“闪烁”的能量感
        Particle *p = Particle_Emit(spawn_pos, vel, c->color, 10, 2);
        if (p)
            p->mass = 100.0f; // size 2 for core
    }

    // --- 2. 生成气场粒子 (Aura) ---
    // 气场粒子向四周逸散，主要是向上（模拟气焰）
    int aura_particles = 2;
    for (int i = 0; i < aura_particles; i++)
    {
        float angle = rand_float(0, 6.283f);
        float dist = rand_float(5.0f, c->energy_radius * breathe_factor);

        Vector2 offset = {cosf(angle) * dist, sinf(angle) * dist};
        Vector2 spawn_pos = Vec2_Add(c->position, offset);

        // 气场向上的趋势 (y 为负是向上)
        Vector2 vel = {
            cosf(angle) * 0.5f,       // 径向扩散
            sinf(angle) * 0.5f - 1.0f // 整体向上漂浮
        };

        // 气场颜色稍微暗淡一些
        SDL_Color aura_col = c->color;
        aura_col.a = 150; // Engine 如果支持 alpha，这里会有半透明效果

        Particle_Emit(spawn_pos, vel, aura_col, 40, 1); // size 1 for aura
    }
}

// --- 核心：元素调色盘 ---
static SDL_Color GetAttributeColor(AttributeID attr)
{

    switch (attr)
    {
    case ATTR_FIRE:
        return (SDL_Color){255, 80, 0, 255}; // 烈火红
    case ATTR_ICE:
        return (SDL_Color){0, 255, 255, 200}; // 寒冰蓝
    case ATTR_WIND:
        return (SDL_Color){180, 255, 200, 150}; // 疾风青
    case ATTR_THUNDER:
        return (SDL_Color){200, 50, 255, 255}; // 雷霆紫
    case ATTR_WOOD:
        return (SDL_Color){50, 255, 100, 200}; // 乙木绿
    case ATTR_METAL:
        return (SDL_Color){255, 255, 200, 255}; // 庚金白
    case ATTR_EARTH:
        return (SDL_Color){160, 100, 50, 255}; // 厚土褐
    case ATTR_LIGHT:
        return (SDL_Color){255, 255, 150, 255}; // 圣光金
    case ATTR_DARK:
        return (SDL_Color){50, 0, 80, 255}; // 幽暗紫
    case ATTR_BLOOD:
        return (SDL_Color){180, 0, 0, 255}; // 鲜血红
    case ATTR_SPIRITUAL:
        return (SDL_Color){100, 200, 255, 150}; // 灵力蓝
    case ATTR_KARMA:
        return (SDL_Color){255, 255, 255, 255}; // 因果(纯白)
    case ATTR_SPACE:
        return (SDL_Color){20, 20, 100, 255}; // 空间(深邃蓝)
    case ATTR_PHYSICAL:
        return (SDL_Color){200, 200, 200, 255}; // 物理灰
    default:
        return (SDL_Color){255, 255, 255, 255};
    }
}

// --- 映射逻辑 SkillID -> VisualID ---
// 这个函数决定了每个技能长什么样
static VisualSpellID MapSkillToVisual(SkillID id)
{
    switch (id)
    {
    case SKILL_ID_GAIN_QI:
    case SKILL_ID_ESSENCE_PLUNDER:
        return VFX_GATHER_QI;

    case SKILL_ID_HEAL:
    case SKILL_ID_EVERGREEN_ART:
    case SKILL_ID_CORE_RESTORATION:
    case SKILL_ID_WARCRY:
    case SKILL_ID_CONCENTRATION:
    case SKILL_ID_CORE_ERUPTION:
        return VFX_BUFF_AURA;

    case SKILL_ID_FIREBALL:
    case SKILL_ID_ICE_FLAME:
        return VFX_PROJECTILE;

    case SKILL_ID_BLOOD_DEVIL_DRILL:
        return VFX_DRILL_SHOT;

    case SKILL_ID_STRIKE:
    case SKILL_ID_BLOOD_DEVIL_SLASH:
    case SKILL_ID_SPIRIT_SLAYING_SWORD:
        return VFX_SLASH;

    case SKILL_ID_BEETLE_SWARM:
        return VFX_SWARM;

    case SKILL_ID_GREAT_GOLDEN_SWORDFORMATION:
        return VFX_SWORD_ARRAY;

    case SKILL_ID_DEFEND:
    case SKILL_ID_ENERGY_SHIELD:
    case SKILL_ID_GOLD_LIGHT_WARDING:
    case SKILL_ID_IMMOVABLE_KING:
    case SKILL_ID_STELLAR_SHIFT:
        return VFX_SHIELD;

        // 将灵剑映射为追踪导弹
    case SKILL_ID_COMMANDING_SWORDS:
    case SKILL_ID_SWORD_PHANTOM:
        return VFX_HOMING_MISSILE;

    // 风刃
    case SKILL_ID_WINDBLADE:
        return VFX_WIND_BLADE;

    // AOE
    case SKILL_ID_FLAMEBLAST:
    case SKILL_ID_SMITE:
    case SKILL_ID_GREATSWORD:
    case SKILL_ID_COSMIC_DHARMA_AVATAR:
        return VFX_AOE_CIRCLE;

    // Beam
    case SKILL_ID_TERMINATE_THUNDER:
        return VFX_BEAM;

    default:
        return VFX_PROJECTILE;
    }
}

// --- 核心施法接口 ---
void SpellFX_Cast(SpellFX *spell, SkillID skill_id, Vector2 pos, Vector2 dir, int duration, int param)
{
    spell->active = true;
    spell->position = pos;
    spell->start_pos = pos;
    spell->direction = dir;

    spell->duration = duration;
    spell->max_duration = duration;

    // 1. 自动查表获取属性
    spell->attribute = g_skill_database[skill_id].attribute_id;

    // 2. 映射视觉原型 (必须在velocity初始化之前)
    spell->id = MapSkillToVisual(skill_id);

    // 3. 初始化 velocity (基于映射后的视觉ID)
    float initial_speed = 0.0f;
    if (spell->id == VFX_PROJECTILE)
        initial_speed = 12.0f;
    else if (spell->id == VFX_DRILL_SHOT)
        initial_speed = 15.0f;
    else if (spell->id == VFX_HOMING_MISSILE)
        initial_speed = 20.0f;

    spell->velocity.x = dir.x * initial_speed;
    spell->velocity.y = dir.y * initial_speed;

    // 4. 预计算目标位置
    spell->target_pos = (Vector2){pos.x + dir.x * 400, pos.y + dir.y * 400};

    // 施法震动反馈
    if (spell->id == VFX_HEAVY_SMASH || spell->id == VFX_BEAM)
        Engine_TriggerShake(5.0f);

    // 灵剑初始化
    if (spell->id == VFX_HOMING_MISSILE)
    {
        spell->sub_count = spell->param;
        if (spell->sub_count > MAX_SUB_FX)
            spell->sub_count = MAX_SUB_FX;

        float spawn_radius = 50.0f;
        for (int i = 0; i < spell->sub_count; i++)
        {
            spell->subs[i].active = true;
            float angle = rand_float(0, 6.28f);
            spell->subs[i].position.x = pos.x + cosf(angle) * spawn_radius;
            spell->subs[i].position.y = pos.y + sinf(angle) * spawn_radius;
            spell->subs[i].velocity.x = cosf(angle) * 2.0f;
            spell->subs[i].velocity.y = sinf(angle) * 2.0f;
            spell->subs[i].phase_offset = rand_float(0, 3.14f);
        }
    }

    // BEAM: target_pos从方向计算(与Hitbox的Beam_Check端点一致)
    if (spell->id == VFX_BEAM)
    {
        spell->target_pos = (Vector2){pos.x + dir.x * 800.0f, pos.y + dir.y * 800.0f};
    }

    spell->param = (param <= 0) ? 1 : param;
}

// 辅助：向量归一化
static Vector2 Vec2_Normalize(Vector2 v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y);
    if (len == 0)
        return (Vector2){0, 0};
    return (Vector2){v.x / len, v.y / len};
}

// --- 核心更新循环 ---
void SpellFX_Update(SpellFX *spell)
{
    if (!spell->active)
        return;

    spell->duration--;
    if (spell->duration <= 0)
    {
        spell->active = false;
        return;
    }

    SDL_Color color = GetAttributeColor(spell->attribute);

    // ==========================================
    // 1. 聚气 (Implosion)
    // ==========================================
    if (spell->id == VFX_GATHER_QI)
    {
        // 粒子从四周生成，飞向施法者中心
        for (int i = 0; i < 2; i++)
        {
            float angle = rand_float(0, 6.28f);
            float dist = rand_float(30, 60);
            Vector2 spawn = {
                spell->start_pos.x + cosf(angle) * dist,
                spell->start_pos.y + sinf(angle) * dist};
            // 速度指向中心
            Vector2 vel = {-cosf(angle) * 2.0f, -sinf(angle) * 2.0f};
            Particle *p = Particle_Emit(spawn, vel, color, 30, 1);
            // 聚气应该是吸入感，所以粒子不用太大
            if (p)
            {
                // 关键：设置为虚无粒子
                p->type = TYPE_VOID;
            }
        }
    }

    // ==========================================
    // 2. Buff/治疗 (Rising Aura)
    // ==========================================
    else if (spell->id == VFX_BUFF_AURA)
    {
        Vector2 spawn = {
            spell->start_pos.x + rand_float(-20, 20),
            spell->start_pos.y + rand_float(20, 40) // 脚底
        };
        Vector2 vel = {0, -3.0f}; // 快速上升
        Particle_Emit(spawn, vel, color, 40, 2);
    }

    // ==========================================
    // 3. 通用投射物 (Projectile)
    // ==========================================
    else if (spell->id == VFX_PROJECTILE)
    {
        float speed = 12.0f;
        spell->position.x += spell->direction.x * speed;
        spell->position.y += spell->direction.y * speed;

        // 尾焰
        int trail_count = (spell->attribute == ATTR_FIRE) ? 5 : 2;
        for (int i = 0; i < trail_count; i++)
        {
            Vector2 spawn = {
                spell->position.x + rand_float(-8, 8),
                spell->position.y + rand_float(-8, 8)};
            // 粒子速度 = 继承一点火球速度 + 随机扩散
            Vector2 vel = {
                spell->direction.x * 2.0f + rand_float(-1, 1),
                spell->direction.y * 2.0f + rand_float(-1, 1)};
            Particle *p = Particle_Emit(spawn, vel, color, 40, 2);
            if (p)
                p->mass = 0.5f; // 较轻，会被护盾轻易吹散
        }
        // for (int i = 0; i < trail_count; i++)
        // {
        //     Vector2 spawn = {
        //         spell->position.x + rand_float(-5, 5),
        //         spell->position.y + rand_float(-5, 5)};
        //     Vector2 vel = {-spell->direction.x + rand_float(-0.5f, 0.5f), -spell->direction.y + rand_float(-0.5f, 0.5f)};
        //     Particle *p = Particle_Emit(spawn, vel, color, 30, 2);
        //     if (p)
        //         p->mass = 0.5f;
        // }

        // 冰焰特效：特殊的冷气下沉效果
        if (spell->attribute == ATTR_ICE)
        {
            Vector2 spawn = spell->position;
            Vector2 vel = {rand_float(-1, 1), 1.0f}; // 下沉
            Particle_Emit(spawn, vel, color, 40, 1);
        }
    }

    // ==========================================
    // 改进版：血魔钻 (Drill Shot)
    // 特性：有钻头，惯性制导，甚至可以像灵剑一样回转
    // ==========================================
    else if (spell->id == VFX_DRILL_SHOT)
    {
        // 1. 物理更新 (简单的惯性制导)
        // 计算指向目标的向量
        Vector2 to_target = Vec2_Sub(spell->target_pos, spell->position);
        Vector2 desired_dir = Vec2_Normalize(to_target);

        // 转向力 (Steering Force)：让速度慢慢偏向目标
        float turn_speed = 0.8f; // 转向灵敏度
        spell->velocity.x += desired_dir.x * turn_speed;
        spell->velocity.y += desired_dir.y * turn_speed;

        // 限制最大速度 (阻尼)，防止无限加速
        // ... (简化起见略过，靠 duration 自动结束)

        // 更新位置
        spell->position.x += spell->velocity.x;
        spell->position.y += spell->velocity.y;

        // 2. 视觉：绘制钻头 (Tip)
        // 这是一个高亮的大粒子，永远在最前端
        SDL_Color tip_col = {255, 255, 255, 255}; // 白热化的钻头
        Particle *tip = Particle_Emit(spell->position, spell->velocity, tip_col, 2, 4);
        if (tip)
            tip->type = TYPE_VOID; // 钻头无坚不摧

        // 3. 视觉：绘制双螺旋尾迹
        float time = (float)(spell->max_duration - spell->duration) * 0.5f;
        // 计算当前速度的垂直向量，用于螺旋展开
        Vector2 forward = Vec2_Normalize(spell->velocity);
        Vector2 perp = {-forward.y, forward.x};

        for (int k = 0; k < 2; k++)
        {
            float angle = time + k * 3.14f;
            float radius = 12.0f; // 螺旋半径

            Vector2 offset = {
                perp.x * cosf(angle) * radius,
                perp.y * cosf(angle) * radius};

            Vector2 spawn = Vec2_Add(spell->position, offset);

            // 粒子向后喷射
            Vector2 p_vel = {-spell->velocity.x * 0.2f, -spell->velocity.y * 0.2f};
            Particle_Emit(spawn, p_vel, color, 15, 2);
        }
    }

    // ==========================================
    // 升级版：乱舞风刃 (Scattered Crescent Wind Blades)
    // 特性：四周随机生成 -> 汇聚攻击 -> 微型月牙形状
    // ==========================================
    else if (spell->id == VFX_WIND_BLADE)
    {
        // 仅在第一帧生成所有风刃
        if (spell->duration == spell->max_duration - 1)
        {

            // 1. 决定风刃数量 (参数化)
            // param 1 -> 3把, param 2 -> 6把, param 3 -> 9把...
            int blade_count = 3 * spell->param;

            // 生成半径 (在施法者身边多远生成)
            float spawn_radius = 60.0f;

            for (int b = 0; b < blade_count; b++)
            {
                // --- A. 确定每一把刀的位置和朝向 ---

                // 随机角度：在施法者四周 360 度随机分布
                float spawn_angle = rand_float(0, 6.283f);

                // 计算这把刀的生成原点
                Vector2 blade_origin = {
                    spell->start_pos.x + cosf(spawn_angle) * spawn_radius,
                    spell->start_pos.y + sinf(spawn_angle) * spawn_radius};

                // 计算这把刀的飞行方向 (指向目标)
                Vector2 to_target = Vec2_Sub(spell->target_pos, blade_origin);
                float dist = sqrtf(to_target.x * to_target.x + to_target.y * to_target.y);
                Vector2 blade_dir = {1.0f, 0.0f};
                if (dist > 0)
                {
                    blade_dir.x = to_target.x / dist;
                    blade_dir.y = to_target.y / dist;
                }

                // --- 物理参数修改 ---
                float thrust_strength = 1.0f; // 加速度大小 (每帧增加 1.5px 速度)
                Vector2 blade_accel = {
                    blade_dir.x * thrust_strength,
                    blade_dir.y * thrust_strength};

                // --- B. 构建微型月牙 (Micro Crescent Construction) ---

                // 基础飞行角度
                float base_angle = atan2f(blade_dir.y, blade_dir.x);
                float blade_speed = rand_float(18.0f, 22.0f); // 略微随机的速度

                // 微型月牙参数
                int particle_density = 15; // 粒子数少一点，因为刀很小
                float arc_width = 1.2f;    // 弧度较窄

                for (int p_idx = 0; p_idx < particle_density; p_idx++)
                {
                    float t = (float)p_idx / (float)particle_density;

                    // 角度偏移
                    float current_angle = base_angle + (t - 0.5f) * arc_width;

                    // 厚度因子 (两头尖，中间厚)
                    float thickness_factor = sinf(t * 3.14159f);

                    // 层数：风刃比较薄，最大 2-3 层即可
                    int layers = 1 + (int)(thickness_factor * 2.0f);

                    for (int layer = 0; layer < layers; layer++)
                    {
                        float layer_offset = (float)layer * 1.0f;
                        float r = layer_offset; // 这里的半径是相对于“刀心”的偏移

                        // 粒子生成位置 (相对于刀的原点)
                        // 注意：我们需要垂直于飞行方向扩展厚度，或者简单地沿径向扩展
                        // 这里简单地沿当前的弧度方向扩展
                        Vector2 p_pos = {
                            blade_origin.x + cosf(current_angle) * r,
                            blade_origin.y + sinf(current_angle) * r};

                        // 稍微让起始位置随机一点点，制造能量边缘的模糊感 (这是安全的，不会导致散架)
                        p_pos.x += rand_float(-1.5f, 1.5f);
                        p_pos.y += rand_float(-1.5f, 1.5f);

                        // --- 修正点 1: 严丝合缝的初始速度 ---
                        // 必须设为 0！任何随机性都会导致几十帧后形状解体
                        Vector2 p_vel = {0, 0};

                        // --- 修正点 2: 增加寿命 ---
                        // 因为起步慢，需要更长的寿命才能飞到目标
                        int life = 50;

                        Particle *p = Particle_Emit(p_pos, p_vel, color, life, 2);
                        if (p)
                        {
                            p->mass = 3.0f;
                            // 阻力不要太小，否则后期速度过快会穿模；也不要太大，否则加不起来速度
                            p->drag = 0.98f;

                            // 赋予统一的加速度，确保整体移动
                            p->acceleration = blade_accel;

                            // 视觉修正：因为初速度为0，上一帧位置就是当前位置
                            p->prev_position = p_pos;
                        }
                    }
                }
            }
            Engine_TriggerShake(2.0f);
        }
    }

    // ==========================================
    // 5. 修正版：可变宽度的月牙斩 (VFX_SLASH)
    // ==========================================
    else if (spell->id == VFX_SLASH)
    {
        // 仅在施法的第一帧生成完整的刀波
        if (spell->duration == spell->max_duration - 1)
        {

            // --- 参数化控制 ---
            // 1. 密度：参数越大，粒子越密，防止变大后出现空隙
            int particle_density = 60 + (spell->param - 1) * 20;

            // 2. 角度：参数越大，扇面越宽 (微调增长率，0.3f 比较自然)
            float arc_angle = 2.0f + (spell->param - 1) * 0.3f;

            // 3. 厚度 (关键修改)：参数越大，层数越多
            // param=1 -> max ~4层
            // param=2 -> max ~7层
            // param=3 -> max ~10层 (非常厚!)
            float max_layers_base = 3.0f + (float)spell->param * 3.0f;

            float radius = 30.0f;
            float speed = 18.0f;

            float base_angle = atan2f(spell->direction.y, spell->direction.x);
            SDL_Color core_color = GetAttributeColor(spell->attribute);

            for (int i = 0; i < particle_density; i++)
            {
                float t = (float)i / (float)particle_density;
                float current_angle = base_angle + (t - 0.5f) * arc_angle;

                // 使用正弦波决定当前的厚度因子 (中间厚，两头尖)
                float thickness_factor = sinf(t * 3.14159f);

                // --- 修正点：使用动态计算的 max_layers_base ---
                int layers = 1 + (int)(thickness_factor * max_layers_base);

                for (int layer = 0; layer < layers; layer++)
                {
                    // 层级偏移：让粒子在径向上排开
                    // 稍微调小间距 (1.5f)，让多层堆叠看起来更致密，像实体光刃
                    float layer_offset = (float)layer * 1.5f;

                    // 半径向外扩展
                    float r = radius + layer_offset;

                    Vector2 spawn_pos = {
                        spell->start_pos.x + cosf(current_angle) * r,
                        spell->start_pos.y + sinf(current_angle) * r};

                    Vector2 vel = {
                        spell->direction.x * speed,
                        spell->direction.y * speed};

                    SDL_Color col = core_color;
                    // 越外层越透明，制造“辉光”边缘感
                    if (layer > 0)
                    {
                        // 动态透明度：层数越高透明度越低
                        int alpha = 255 - (layer * 255 / layers);
                        if (alpha < 50)
                            alpha = 50;
                        col.a = (Uint8)alpha;
                    }

                    Particle *p = Particle_Emit(spawn_pos, vel, col, 25, 2);
                    if (p)
                    {
                        p->mass = 5.0f;
                        p->drag = 0.98f;
                        // 强行拉长拖尾，增加速度感
                        p->prev_position.x = spawn_pos.x - vel.x * 0.6f;
                        p->prev_position.y = spawn_pos.y - vel.y * 0.6f;
                    }
                }
            }
            Engine_TriggerShake(2.0f + spell->param * 1.0f); // 震动也随参数增强
        }
    }

    // ==========================================
    // 6. 重击/巨剑 (Heavy Smash)
    // ==========================================
    else if (spell->id == VFX_HEAVY_SMASH)
    {
        // 模拟一把巨剑从天而降砸向目标
        float progress = 1.0f - (float)spell->duration / spell->max_duration;

        // 简单的线性插值位置
        Vector2 current_pos = {
            spell->start_pos.x + (spell->target_pos.x - spell->start_pos.x) * progress,
            spell->start_pos.y + (spell->target_pos.y - spell->start_pos.y) * progress};

        // 剑身粒子
        int particles = 10;
        for (int i = 0; i < particles; i++)
        {
            Vector2 spawn = {
                current_pos.x + rand_float(-20, 20),
                current_pos.y + rand_float(-20, 20)};
            Particle_Emit(spawn, (Vector2){0, 0}, color, 10, 4); // 大粒子
        }

        // 命中瞬间 (最后几帧)
        if (spell->duration < 5)
        {
            // 施加巨大的冲击力场
            ForceGrid_AddRadialForce(current_pos, 80.0f, 5.0f);
            Engine_TriggerShake(5.0f);

            // 爆炸效果
            for (int i = 0; i < 20; i++)
            {
                float a = rand_float(0, 6.28f);
                float s = rand_float(2, 10);
                Vector2 v = {cosf(a) * s, sinf(a) * s};
                Particle_Emit(current_pos, v, color, 40, 2);
            }
        }
    }

    // ==========================================
    // 7. 虫群 (Swarm)
    // ==========================================
    else if (spell->id == VFX_SWARM)
    {
        // 持续生成混乱移动的粒子
        if (spell->duration % 2 == 0)
        {
            Vector2 spawn = spell->position; // 随投射物移动
            spell->position.x += spell->direction.x * 8.0f;
            spell->position.y += spell->direction.y * 8.0f;

            for (int i = 0; i < 3; i++)
            {
                Vector2 offset = {rand_float(-20, 20), rand_float(-20, 20)};
                Vector2 real_spawn = Vec2_Add(spell->position, offset);

                // 噪点运动
                Vector2 vel = {
                    spell->direction.x * 5.0f + rand_float(-3, 3),
                    spell->direction.y * 5.0f + rand_float(-3, 3)};
                SDL_Color bug_col = {255, 215, 0, 255}; // 金色噬金虫
                Particle_Emit(real_spawn, vel, bug_col, 60, 3);
            }
        }
    }

    // ==========================================
    // 8. 雷击 (Thunder) & 剑阵 (Array)
    // ==========================================
    else if (spell->id == VFX_THUNDER_STRIKE)
    {
        // 逻辑：每隔几帧重新绘制一次电弧，模拟闪烁
        if (spell->duration % 4 == 0)
        {
            Vector2 target = {
                spell->start_pos.x + spell->direction.x * 600.0f,
                spell->start_pos.y + spell->direction.y * 600.0f};

            // 生成折线
            Vector2 current = spell->start_pos;
            int segments = 15;
            for (int i = 0; i < segments; i++)
            {
                float t = (float)(i + 1) / segments;
                Vector2 next_ideal = {
                    spell->start_pos.x + (target.x - spell->start_pos.x) * t,
                    spell->start_pos.y + (target.y - spell->start_pos.y) * t};

                // 随机偏移，制造“折线”感
                Vector2 next_real = {
                    next_ideal.x + rand_float(-30, 30),
                    next_ideal.y + rand_float(-30, 30)};

                // 在两点之间填充粒子
                int sub_steps = 5;
                for (int j = 0; j < sub_steps; j++)
                {
                    float sub_t = (float)j / sub_steps;
                    Vector2 p_pos = {
                        current.x + (next_real.x - current.x) * sub_t,
                        current.y + (next_real.y - current.y) * sub_t};
                    SDL_Color col = {220, 100, 255, 255};                           // 紫
                    Particle *p = Particle_Emit(p_pos, (Vector2){0, 0}, col, 5, 3); // 寿命极短(5帧)
                    if (p)
                        p->mass = 0.1f;
                }
                current = next_real;
            }
        }
    }

    // ==========================================
    // 升级版：多层旋转剑阵 (VFX_SWORD_ARRAY)
    // ==========================================
    else if (spell->id == VFX_SWORD_ARRAY)
    {
        // param 代表层数 (Layers)
        int layers = spell->param;

        // 基础旋转速度 (随时间变化)
        float base_angle = (float)spell->duration * 0.05f;

        for (int L = 0; L < layers; L++)
        {
            // 每一层的半径不同：第一层 50，第二层 80，第三层 110...
            float radius = 50.0f + L * 30.0f;

            // 每一层的粒子数量不同：外层需要更多粒子才不显得稀疏
            int sword_count = 8 + L * 4;

            // 奇偶层旋转方向相反，增加视觉复杂度
            float direction_sign = (L % 2 == 0) ? 1.0f : -1.0f;
            float current_layer_angle = base_angle * direction_sign;

            // 稍微错开每一层的初始角度
            current_layer_angle += L * 0.5f;

            for (int i = 0; i < sword_count; i++)
            {
                float angle = current_layer_angle + i * (6.283f / sword_count);

                Vector2 pos = {
                    spell->target_pos.x + cosf(angle) * radius,
                    spell->target_pos.y + sinf(angle) * radius};

                // 发射粒子 (静态粒子，寿命短，依靠高频刷新形成视觉残留)
                Particle *p = Particle_Emit(pos, (Vector2){0, 0}, color, 5, 2);

                // 外层的剑阵粒子稍微大一点
                if (p && L > 0)
                    p->size = 3;
            }
        }

        // 中心引力场：层数越多，引力越强
        ForceGrid_AddRadialForce(spell->target_pos, 40.0f + layers * 20.0f, -0.5f * layers);
    }

    // ==========================================
    // 9. 护盾 (Shield)
    // ==========================================
    else if (spell->id == VFX_SHIELD)
    {
        int particles = 2;
        for (int i = 0; i < particles; i++)
        {
            float angle = rand_float(0, 6.28f);
            float dist = rand_float(35, 40);
            Vector2 pos = {
                spell->start_pos.x + cosf(angle) * dist,
                spell->start_pos.y + sinf(angle) * dist};
            Particle_Emit(pos, (Vector2){0, 0}, color, 10, 1);
        }
        ForceGrid_AddRadialForce(spell->start_pos, 50.0f, 2.0f); // 斥力
    }

    // ==========================================
    // 终极版：灵剑 (Homing Missile - Phase Launch)
    // 节奏：生成散开 -> 悬停瞄准 -> 极速穿刺 -> 往复回转
    // ==========================================
    else if (spell->id == VFX_HOMING_MISSILE)
    {
        // 计算存活时间
        int age = spell->max_duration - spell->duration;

        // 设定发射延迟 (帧数)
        int launch_delay = 15;

        for (int i = 0; i < spell->sub_count; i++)
        {
            SubEntity *sword = &spell->subs[i];
            if (!sword->active)
                continue;

            Vector2 to_target = Vec2_Sub(spell->target_pos, sword->position);

            // --- 第一阶段：悬停与整备 (Hover Phase) ---
            if (age < launch_delay)
            {
                // 在这个阶段，飞剑刚生成并散开
                // 我们施加极大的阻尼，让它们迅速从“炸开”的状态变成“静止悬浮”
                sword->velocity.x *= 0.85f;
                sword->velocity.y *= 0.85f;

                // 微妙的上下浮动 (Idle Animation)
                sword->position.y += sinf(age * 0.5f + sword->phase_offset) * 0.2f;

                // 剑尖稍微转向目标 (视觉预瞄)
                // 这里简单处理，不动物理位置，只在渲染时可能有体现
            }
            // --- 第二阶段：点火与穿刺 (Ignition Phase) ---
            else
            {
                // 1. 弹性牵引 (保持原有逻辑，负责冲过头后的拉回)
                // 稍微降低一点 k 值，因为我们有了主动推力
                float k = 0.03f + (i % 3) * 0.005f;

                Vector2 spring_force = {
                    to_target.x * k,
                    to_target.y * k};

                // 2. 主动推力 (Thrust) - 这就是你要的加速感！
                // 计算归一化方向
                float dist = sqrtf(to_target.x * to_target.x + to_target.y * to_target.y);
                Vector2 dir_norm = {0, 0};
                if (dist > 0)
                {
                    dir_norm.x = to_target.x / dist;
                    dir_norm.y = to_target.y / dist;
                }

                // 推力随着时间稍微增加，越飞越快
                float thrust_strength = 1.2f;

                // 组合力：弹性力(拉回) + 推力(冲刺)
                sword->velocity.x += spring_force.x + dir_norm.x * thrust_strength;
                sword->velocity.y += spring_force.y + dir_norm.y * thrust_strength;

                // 3. 飞行阻尼
                // 此时阻尼要小，保持高速
                sword->velocity.x *= 0.97f;
                sword->velocity.y *= 0.97f;
            }

            // 更新位置
            sword->position.x += sword->velocity.x;
            sword->position.y += sword->velocity.y;

            // --- 渲染部分 (保持不变) ---
            SDL_Color color = GetAttributeColor(spell->attribute);
            SDL_Color tip_col = {255, 255, 200, 255};

            // 剑尖
            Particle *head = Particle_Emit(sword->position, sword->velocity, tip_col, 2, 3);
            if (head)
            {
                head->type = TYPE_VOID;
                head->drag = 1.0f;
            }

            // 拖尾 (仅在高速移动或发射阶段后显示)
            float speed_sq = sword->velocity.x * sword->velocity.x + sword->velocity.y * sword->velocity.y;
            if (speed_sq > 10.0f && age > launch_delay)
            {
                float len = sqrtf(speed_sq);
                Vector2 dir_norm = {sword->velocity.x / len, sword->velocity.y / len};
                Vector2 tail_pos = {
                    sword->position.x - dir_norm.x * 8.0f,
                    sword->position.y - dir_norm.y * 8.0f};
                color.a = 150;
                Particle_Emit(tail_pos, (Vector2){0, 0}, color, 10, 3);
            }

            // 命中反馈
            float dist_sq = to_target.x * to_target.x + to_target.y * to_target.y;
            if (dist_sq < 30 * 30 && speed_sq > 200.0f)
            {
                if (spell->duration % 10 == 0)
                {
                    Engine_TriggerShake(0.5f);
                    for (int s = 0; s < 2; s++)
                    {
                        Vector2 spark_vel = {rand_float(-3, 3), rand_float(-3, 3)};
                        Particle_Emit(spell->target_pos, spark_vel, tip_col, 10, 1);
                    }
                }
            }
        }
    }

    // ==========================================
    // 10. AOE Circle (预警圈 + 爆发环)
    // ==========================================
    else if (spell->id == VFX_AOE_CIRCLE)
    {
        float progress = 1.0f - (float)spell->duration / spell->max_duration;

        // 预警阶段 (前30%时间): 显示收缩的预警圈
        if (progress < 0.3f) {
            float warn_pct = progress / 0.3f; // 0→1 during warning
            float ring_radius = spell->param * (1.0f - warn_pct * 0.3f); // 微微收缩
            int ring_particles = 30 + (int)(warn_pct * 20); // 越来越密集
            for (int i = 0; i < ring_particles; i++) {
                float angle = (float)i / ring_particles * 6.283f + spell->duration * 0.1f;
                Vector2 p_pos = {
                    spell->position.x + cosf(angle) * ring_radius,
                    spell->position.y + sinf(angle) * ring_radius};
                SDL_Color warn_col = {255, 200, 50, 150};
                Particle *p = Particle_Emit(p_pos, (Vector2){0, 0}, warn_col, 8, 2);
                if (p) p->mass = 0.1f;
            }
        }
        // 爆发阶段 (30%-70%): 冲击波扩散 + 力场
        else if (progress < 0.7f) {
            float burst_pct = (progress - 0.3f) / 0.4f;
            float burst_radius = spell->param * (0.6f + burst_pct * 0.8f);
            int burst_particles = 15;
            for (int i = 0; i < burst_particles; i++) {
                float angle = rand_float(0, 6.283f);
                float dist = burst_radius * rand_float(0.5f, 1.0f);
                Vector2 p_pos = {
                    spell->position.x + cosf(angle) * dist,
                    spell->position.y + sinf(angle) * dist};
                Vector2 vel = {
                    cosf(angle) * (1.0f - burst_pct) * 6.0f,
                    sinf(angle) * (1.0f - burst_pct) * 6.0f};
                Particle *p = Particle_Emit(p_pos, vel, color, 15, 2);
                if (p) p->mass = 0.5f;
            }
            ForceGrid_AddRadialForce(spell->position, burst_radius, 3.0f * (1.0f - burst_pct));
            if (progress < 0.35f) Engine_TriggerShake(4.0f);
        }
        // 消散阶段 (70%-100%): 残留粒子
        else {
            if (spell->duration % 3 == 0) {
                for (int i = 0; i < 5; i++) {
                    float angle = rand_float(0, 6.283f);
                    float dist = spell->param * rand_float(0.8f, 1.2f);
                    Vector2 p_pos = {
                        spell->position.x + cosf(angle) * dist,
                        spell->position.y + sinf(angle) * dist};
                    Particle_Emit(p_pos, (Vector2){0, 0.5f}, color, 20, 1);
                }
            }
        }
    }

    // ==========================================
    // 11. 雷电射线 (Beam - 即时折线闪电)
    // ==========================================
    else if (spell->id == VFX_BEAM)
    {
        // 第一帧: 生成完整的折线闪电
        if (spell->duration == spell->max_duration - 1)
        {
            Vector2 from = spell->start_pos;
            Vector2 to = spell->target_pos;
            float total_dist = sqrtf((to.x-from.x)*(to.x-from.x) + (to.y-from.y)*(to.y-from.y));

            int segments = 15;
            Vector2 prev = from;
            for (int i = 1; i <= segments; i++)
            {
                float t = (float)i / segments;
                Vector2 ideal = {
                    from.x + (to.x - from.x) * t,
                    from.y + (to.y - from.y) * t};
                float jitter = (total_dist / segments) * 0.5f;
                Vector2 current = {
                    ideal.x + rand_float(-jitter, jitter),
                    ideal.y + rand_float(-jitter, jitter)};

                // 在prev和current之间填充密集粒子
                int sub_steps = 8;
                for (int j = 0; j < sub_steps; j++)
                {
                    float sub_t = (float)j / sub_steps;
                    Vector2 p_pos = {
                        prev.x + (current.x - prev.x) * sub_t,
                        prev.y + (current.y - prev.y) * sub_t};
                    SDL_Color beam_col = (spell->attribute == ATTR_THUNDER)
                        ? (SDL_Color){220, 100, 255, 255}
                        : color;
                    Particle *p = Particle_Emit(p_pos, (Vector2){0, 0}, beam_col, 3, 3);
                    if (p) p->mass = 0.05f;
                }
                prev = current;
            }
            Engine_TriggerShake(3.0f);
        }

        // 后续帧: 残留电弧闪烁 (只在前几帧)
        if (spell->duration < spell->max_duration - 3 && spell->duration % 3 == 0)
        {
            Vector2 from = spell->start_pos;
            Vector2 to = spell->target_pos;
            float dist = sqrtf((to.x-from.x)*(to.x-from.x) + (to.y-from.y)*(to.y-from.y));
            int sparks = 8;
            for (int i = 0; i < sparks; i++)
            {
                float t = rand_float(0, 1);
                Vector2 p_pos = {
                    from.x + (to.x - from.x) * t + rand_float(-8, 8),
                    from.y + (to.y - from.y) * t + rand_float(-8, 8)};
                SDL_Color spark_col = {255, 255, 200, 200};
                Particle_Emit(p_pos, (Vector2){0, -2}, spark_col, 6, 1);
            }
        }
    }
}
