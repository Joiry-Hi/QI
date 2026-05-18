/**
 * engine.c
 * 核心引擎实现
 */
#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 内部全局状态
static SDL_Window *g_Window = NULL;
static SDL_Renderer *g_Renderer = NULL;

// 粒子池 - 静态分配以避免运行时的内存碎片
static Particle g_Particles[MAX_PARTICLES];
static int g_ParticleCount = 0;

static Vector2 g_CameraOffset = {0, 0}; // 新增：摄像机震动偏移
static float g_ShakeIntensity = 0.0f;   // 震动强度

// 新增：震动控制函数
void Engine_TriggerShake(float intensity)
{
    g_ShakeIntensity = intensity;
}

bool Engine_Init(const char *title)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "SDL Initialisation Failed: %s\n", SDL_GetError());
        return false;
    }

    // 1. 设置 Hint：在创建窗口前，告诉 SDL 不要自作聪明地处理 IME
    // 这通常能解决一部分 Linux 发行版的问题
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "0");

    g_Window = SDL_CreateWindow(title,
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                SCREEN_WIDTH, SCREEN_HEIGHT,
                                SDL_WINDOW_SHOWN);
    if (!g_Window)
        return false;

    // 使用硬件加速渲染器
    g_Renderer = SDL_CreateRenderer(g_Window, -1, SDL_RENDERER_ACCELERATED);
    if (!g_Renderer)
        return false;

    // 启用混合模式以支持透明度（对于粒子效果至关重要）
    SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_BLEND);

    srand((unsigned int)time(NULL)); // 播种随机数

    // --- 2. 关键修复：显式关闭文本输入模式 ---
    // 这告诉操作系统：我不想要 IME 干扰
    SDL_StopTextInput();

    return true;
}

void Engine_Cleanup()
{
    if (g_Renderer)
        SDL_DestroyRenderer(g_Renderer);
    if (g_Window)
        SDL_DestroyWindow(g_Window);
    SDL_Quit();
}

void Engine_Clear()
{
    SDL_SetRenderDrawColor(g_Renderer, 10, 10, 15, 255); // 深色背景，类似虚空
    SDL_RenderClear(g_Renderer);
}

void Engine_Present()
{
    SDL_RenderPresent(g_Renderer);
}

SDL_Renderer *Engine_GetRenderer()
{
    return g_Renderer;
}

SDL_Window *Engine_GetWindow()
{
    return g_Window;
}

// --- 粒子系统实现 ---

Particle *Particle_Emit(Vector2 pos, Vector2 vel, SDL_Color color, int lifetime_frames, int size)
{
    if (g_ParticleCount >= MAX_PARTICLES)
        return NULL; // 粒子池已满，忽略新粒子

    Particle *p = &g_Particles[g_ParticleCount++];
    p->position = pos;
    p->prev_position = pos;
    p->velocity = vel;
    p->acceleration = (Vector2){0, 0};
    p->color = color;
    p->life = 1.0f; // 标准化生命周期
    p->decay_rate = 1.0f / (float)lifetime_frames;
    p->size = size;
    p->type = TYPE_GENERIC; // 默认为通用类型
    p->mass = 1.0f;
    p->drag = 1.0f;    // 默认无阻力 (太空环境)
    p->gravity = 0.0f; // 默认无重力 (魔法粒子通常悬浮)

    return p; // 返回指针
}

void Particle_UpdateAll()
{
    // *** 警告：不要在这里调用 ForceGrid_Clear() ***
    // 我们在 main.c 的主循环开头调用它，以便 Spell_Update 能写入数据。
    // 如果在这里 Clear，就会把技能刚刚写入的力场擦除掉！

    // 注意：通常我们在这里计算所有的力场源（比如技能），
    // 但为了解耦，我们假设外部逻辑已经在本帧调用了 ForceGrid_Add...
    // 在 main loop 中，顺序应该是：
    // 1. Update Spells (Add forces to grid)
    // 2. Update Particles (Read forces from grid)

    // --- 更新震动 ---
    if (g_ShakeIntensity > 0.1f)
    {
        g_CameraOffset.x = (float)((rand() % 100) - 50) / 50.0f * g_ShakeIntensity;
        g_CameraOffset.y = (float)((rand() % 100) - 50) / 50.0f * g_ShakeIntensity;
        g_ShakeIntensity *= 0.9f; // 震动衰减
    }
    else
    {
        g_CameraOffset = (Vector2){0, 0};
    }

    for (int i = 0; i < g_ParticleCount; i++)
    {
        Particle *p = &g_Particles[i];
        p->prev_position = p->position;

        // --- A. 应用力场 ---
        // 只有非虚无粒子才受力场影响
        if (p->type != TYPE_VOID)
        {
            Vector2 env_force = ForceGrid_GetForceAt(p->position);
            if (p->mass > 0.001f)
            {
                p->velocity.x += env_force.x / p->mass;
                p->velocity.y += env_force.y / p->mass;
            }
        }

        p->velocity.x += p->acceleration.x;
        p->velocity.y += p->acceleration.y;

        // 重力和阻力通常也应该对 Void 无效（或者有效，看你设定），
        // 这里假设 Void 是纯粹的能量体，也不受重力/阻力影响，保持恒定速度
        if (p->type != TYPE_VOID)
        {
            p->velocity.y += p->gravity;
            p->velocity.x *= p->drag;
            p->velocity.y *= p->drag;
        }

        // 3. 基础位移积分 (原有逻辑)
        p->position.x += p->velocity.x;
        p->position.y += p->velocity.y;

        // 4. 生命衰减 (原有逻辑)
        p->life -= p->decay_rate;

        // 5. 死亡检测 (原有逻辑)
        if (p->life <= 0.0f)
        {
            g_Particles[i] = g_Particles[g_ParticleCount - 1];
            g_ParticleCount--;
            i--;
        }
    }
}

// 修改渲染逻辑：速度快的粒子画线，慢的画点
void Particle_RenderAll()
{
    for (int i = 0; i < g_ParticleCount; i++)
    {
        Particle *p = &g_Particles[i];

        Uint8 alpha = (Uint8)(p->life * 255.0f);
        SDL_SetRenderDrawColor(g_Renderer, p->color.r, p->color.g, p->color.b, alpha);

        // 应用摄像机震动偏移
        int x = (int)(p->position.x + g_CameraOffset.x);
        int y = (int)(p->position.y + g_CameraOffset.y);
        int px = (int)(p->prev_position.x + g_CameraOffset.x);
        int py = (int)(p->prev_position.y + g_CameraOffset.y);

        // 计算速度平方
        float speed_sq = (p->position.x - p->prev_position.x) * (p->position.x - p->prev_position.x) +
                         (p->position.y - p->prev_position.y) * (p->position.y - p->prev_position.y);

        // 如果速度够快，画拖尾线 (Motion Blur)
        if (speed_sq == 3000.0f)
        {
            SDL_RenderDrawLine(g_Renderer, px, py, x, y);
        }
        else
        {
            if (p->size == 1)
            {
                SDL_RenderDrawPoint(g_Renderer, x, y);
            }
            else
            {
                SDL_Rect rect = {x, y, p->size, p->size};
                SDL_RenderFillRect(g_Renderer, &rect);
            }
        }
    }
}

static Vector2 g_Grid[GRID_W][GRID_H];

void ForceGrid_Clear()
{

    for (int x = 0; x < GRID_W; x++)
    {
        for (int y = 0; y < GRID_H; y++)
        {
            g_Grid[x][y].x = 0.0f;
            g_Grid[x][y].y = 0.0f;
        }
    }
}

// 将世界坐标转换为网格索引
static void GetGridIndex(Vector2 pos, int *x, int *y)
{
    *x = (int)(pos.x / CELL_SIZE);
    *y = (int)(pos.y / CELL_SIZE);
    // 边界钳制
    if (*x < 0)
        *x = 0;
    if (*x >= GRID_W)
        *x = GRID_W - 1;
    if (*y < 0)
        *y = 0;
    if (*y >= GRID_H)
        *y = GRID_H - 1;
}

void ForceGrid_AddRadialForce(Vector2 center, float radius, float strength)
{
    // 优化：只遍历受影响的网格区域，而不是全图
    int min_x, min_y, max_x, max_y;
    GetGridIndex((Vector2){center.x - radius, center.y - radius}, &min_x, &min_y);
    GetGridIndex((Vector2){center.x + radius, center.y + radius}, &max_x, &max_y);

    for (int x = min_x; x <= max_x; x++)
    {
        for (int y = min_y; y <= max_y; y++)
        {
            // 网格中心的世界坐标
            Vector2 cell_center = {
                x * CELL_SIZE + CELL_SIZE / 2.0f,
                y * CELL_SIZE + CELL_SIZE / 2.0f};

            float dx = cell_center.x - center.x;
            float dy = cell_center.y - center.y;
            float dist_sq = dx * dx + dy * dy;
            float rad_sq = radius * radius;

            if (dist_sq < rad_sq && dist_sq > 0.001f)
            {
                float dist = sqrtf(dist_sq);
                float factor = 1.0f - (dist / radius); // 越靠近中心力越大

                // 归一化向量 * 强度 * 衰减因子
                Vector2 force = {(dx / dist) * strength * factor, (dy / dist) * strength * factor};

                // 累加力
                g_Grid[x][y] = Vec2_Add(g_Grid[x][y], force);
            }
        }
    }
}

void ForceGrid_AddDirectionalForce(Vector2 center, float radius, Vector2 force)
{
    // 简化版：直接在范围内叠加恒定力
    int min_x, min_y, max_x, max_y;
    GetGridIndex((Vector2){center.x - radius, center.y - radius}, &min_x, &min_y);
    GetGridIndex((Vector2){center.x + radius, center.y + radius}, &max_x, &max_y);

    for (int x = min_x; x <= max_x; x++)
    {
        for (int y = min_y; y <= max_y; y++)
        {
            // 简单的矩形范围检查，实际可以做圆形
            g_Grid[x][y] = Vec2_Add(g_Grid[x][y], force);
        }
    }
}

// 辅助：向量线性插值
static Vector2 Vec2_Lerp(Vector2 a, Vector2 b, float t)
{
    return (Vector2){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t};
}

// 升级版：双线性插值获取力场 (Bilinear Interpolation)
Vector2 ForceGrid_GetForceAt(Vector2 position)
{
    // 将坐标映射到网格浮点坐标
    float grid_x = position.x / CELL_SIZE;
    float grid_y = position.y / CELL_SIZE;

    // 为了让力场中心对齐网格中心，我们做一个 0.5 的偏移采样
    grid_x -= 0.5f;
    grid_y -= 0.5f;

    // 获取左上角的整数索引
    int x0 = (int)floorf(grid_x);
    int y0 = (int)floorf(grid_y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // 边界安全检查 (Clamp)
    if (x0 < 0)
        x0 = 0;
    if (x0 >= GRID_W)
        x0 = GRID_W - 1;
    if (y0 < 0)
        y0 = 0;
    if (y0 >= GRID_H)
        y0 = GRID_H - 1;
    if (x1 < 0)
        x1 = 0;
    if (x1 >= GRID_W)
        x1 = GRID_W - 1;
    if (y1 < 0)
        y1 = 0;
    if (y1 >= GRID_H)
        y1 = GRID_H - 1;

    // 计算权重 (0.0 ~ 1.0)
    float u = grid_x - floorf(grid_x);
    float v = grid_y - floorf(grid_y);

    // 获取四个邻居的力
    Vector2 f00 = g_Grid[x0][y0];
    Vector2 f10 = g_Grid[x1][y0];
    Vector2 f01 = g_Grid[x0][y1];
    Vector2 f11 = g_Grid[x1][y1];

    // 两次插值
    Vector2 top = Vec2_Lerp(f00, f10, u);
    Vector2 bottom = Vec2_Lerp(f01, f11, u);

    return Vec2_Lerp(top, bottom, v);
}

// --- 5. 辅助绘制函数实现 ---

void Engine_DrawFillRect(int x, int y, int w, int h, SDL_Color color)
{
    SDL_SetRenderDrawColor(g_Renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(g_Renderer, &rect);
}

void Engine_DrawFillCircle(int cx, int cy, int radius, SDL_Color color)
{
    SDL_SetRenderDrawColor(g_Renderer, color.r, color.g, color.b, color.a);
    for (int dy = -radius; dy <= radius; dy++) {
        int dx_max = (int)sqrtf((float)(radius * radius - dy * dy));
        SDL_RenderDrawLine(g_Renderer, cx - dx_max, cy + dy, cx + dx_max, cy + dy);
    }
}

void Engine_DrawCircleOutline(int cx, int cy, int radius, SDL_Color color)
{
    SDL_SetRenderDrawColor(g_Renderer, color.r, color.g, color.b, color.a);
    int x = radius, y = 0;
    int err = 0;
    while (x >= y) {
        SDL_RenderDrawPoint(g_Renderer, cx + x, cy + y);
        SDL_RenderDrawPoint(g_Renderer, cx + y, cy + x);
        SDL_RenderDrawPoint(g_Renderer, cx - y, cy + x);
        SDL_RenderDrawPoint(g_Renderer, cx - x, cy + y);
        SDL_RenderDrawPoint(g_Renderer, cx - x, cy - y);
        SDL_RenderDrawPoint(g_Renderer, cx - y, cy - x);
        SDL_RenderDrawPoint(g_Renderer, cx + y, cy - x);
        SDL_RenderDrawPoint(g_Renderer, cx + x, cy - y);
        y++;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) {
            x--;
            err += 1 - 2 * x;
        }
    }
}

void Engine_DrawLine(int x1, int y1, int x2, int y2, SDL_Color color)
{
    SDL_SetRenderDrawColor(g_Renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(g_Renderer, x1, y1, x2, y2);
}

void Engine_DrawBar(int x, int y, int w, int h, float pct, SDL_Color fill, SDL_Color bg)
{
    if (pct < 0) pct = 0; if (pct > 1.0f) pct = 1.0f;
    SDL_Rect bg_rect = {x, y, w, h};
    SDL_SetRenderDrawColor(g_Renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(g_Renderer, &bg_rect);
    SDL_Rect fg_rect = {x, y, (int)(w * pct), h};
    SDL_SetRenderDrawColor(g_Renderer, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(g_Renderer, &fg_rect);
}

void ForceGrid_DebugRender(SDL_Renderer *renderer)
{

    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    for (int x = 0; x < GRID_W; x++)
    {
        for (int y = 0; y < GRID_H; y++)
        {
            Vector2 f = g_Grid[x][y];
            if (fabs(f.x) > 0.1f || fabs(f.y) > 0.1f)
            {
                int cx = x * CELL_SIZE + CELL_SIZE / 2;
                int cy = y * CELL_SIZE + CELL_SIZE / 2;
                // 画一条线代表力的方向
                SDL_RenderDrawLine(renderer, cx, cy, cx + (int)(f.x * 10), cy + (int)(f.y * 10));
            }
        }
    }
}
