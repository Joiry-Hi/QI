/**
 * main.c - Final Integration
 */
#include "engine.h"
#include "vfx.h"
#include "animator.h"
#include <stdbool.h>
#include <stdio.h>
#include "QI.h"

// --- 声明 QI.c 提供的外部接口 ---
extern void Engine_Initialize_Bridge();
extern CombatEvent* Engine_CalculateNextTurn(ActionType player_action, int* out_event_count);
extern void Engine_GetEntitySnapshot(int entity_id, EntitySnapshot* out_snapshot);

// 简单的 UI 状态机
typedef enum {
    STATE_PLAYER_INPUT,
    STATE_ANIMATING
} GameState;

int main(int argc, char* argv[]) {
    if (!Engine_Init("QI-Anima: The Real Cultivation")) return 1;

    // 初始化逻辑引擎
    Engine_Initialize_Bridge();

    Caster caster_you = Caster_Create(200, 300, (SDL_Color){0, 255, 255, 255});
    Caster caster_cpu = Caster_Create(600, 300, (SDL_Color){255, 50, 50, 255});
    
    Animator_Init(&caster_you, &caster_cpu);

    bool is_running = true;
    SDL_Event event;
    GameState state = STATE_PLAYER_INPUT;
    
    // 默认玩家动作为 None
    ActionType next_action = ACTION_TYPE_NONE;

    while (is_running) {
        // 1. 输入处理
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) is_running = false;
            
            // 简单的键盘映射用于测试：
            if (state == STATE_PLAYER_INPUT && event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_a: next_action = ACTION_TYPE_MELEE; break;     // A: 普攻
                    case SDLK_q: next_action = ACTION_TYPE_GAIN_QI; break;   // Q: 集气
                    case SDLK_f: next_action = ACTION_TYPE_RANGED; break;    // F: 火球
                    case SDLK_d: next_action = ACTION_TYPE_DEFEND; break;    // D: 防御
                    case SDLK_s: next_action = ACTION_TYPE_SMITE; break;     // S: 重击
                    case SDLK_h: next_action = ACTION_TYPE_HEAL; break;      // H: 治疗
                    case SDLK_b: next_action = ACTION_TYPE_BURST; break;     // B: 爆发
                    case SDLK_t: next_action = ACTION_TYPE_TERMINATE; break; // T: 终结
                    case SDLK_c: next_action = ACTION_TYPE_COUNTER; break;   // C: 反击
                    case SDLK_p: next_action = ACTION_TYPE_BOOST; break;     // P: 增益
                    case SDLK_ESCAPE: is_running = false; break;            // Esc: 退出
                    default: break;
                }
                
                if (next_action != ACTION_TYPE_NONE) {
                    // 触发回合！
                    int count = 0;
                    // 调用真正的逻辑引擎计算
                    CombatEvent* events = Engine_CalculateNextTurn(next_action, &count);
                    
                    // 将事件推给导演
                    Animator_PushRound(events, count);
                    
                    state = STATE_ANIMATING; // 锁定输入
                    next_action = ACTION_TYPE_NONE; // 重置
                }
            }
        }

        // 2. 物理帧前置
        ForceGrid_Clear();

        // 3. 逻辑更新
        if (state == STATE_ANIMATING) {
            if (!Animator_IsPlaying()) {
                // 动画播放完毕，切回输入模式
                state = STATE_PLAYER_INPUT;
                printf(">> Waiting for Player Input (A/Q/F/D/S/H)...\n");
            }
        }

        // 更新UI数据 (每帧同步血量)
        EntitySnapshot snap_you, snap_cpu;
        Engine_GetEntitySnapshot(ENTITY_ID_YOU, &snap_you);
        Engine_GetEntitySnapshot(ENTITY_ID_CPU, &snap_cpu);
        // TODO: 这里以后可以绘制血条，现在先打印到控制台或者只是更新内部数值
        
        // 引擎更新
        Animator_Update();
        Caster_Update(&caster_you);
        Caster_Update(&caster_cpu);
        Particle_UpdateAll();

        // 4. 渲染
        Engine_Clear();
        Particle_RenderAll();
        Engine_Present();
        
        SDL_Delay(16); 
    }

    Engine_Cleanup();
    return 0;
}