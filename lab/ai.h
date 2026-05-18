/**
 * ai.h
 * Enemy AI: FSM + threat assessment + utility-based skill selection
 */
#ifndef AI_H
#define AI_H

#include "entity.h"

typedef enum {
    AI_STATE_IDLE,
    AI_STATE_CHASE,
    AI_STATE_ATTACK,
    AI_STATE_DODGE,
    AI_STATE_GATHER_QI,
    AI_STATE_FLEE
} AIState;

void AI_Update(Entity* e, Entity* player, float dt);

#endif
