/**
 * animator.h
 * 升级版：基于 CombatEvent 的动画解释器
 */
#ifndef ANIMATOR_H
#define ANIMATOR_H

#include "QI.h"
#include "vfx.h"
#include <stdbool.h>

// 初始化
void Animator_Init(Caster* p1, Caster* p2);

// 新的核心接口：接收来自逻辑层的一组事件，并生成演出队列
// events: 事件数组
// count: 事件数量
void Animator_PushRound(CombatEvent* events, int count);

// 更新与状态查询
void Animator_Update();
bool Animator_IsPlaying();

#endif // ANIMATOR_H