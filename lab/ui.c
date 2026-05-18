/**
 * ui.c
 * HUD rendering implementation using engine draw helpers
 */
#include "ui.h"
#include "engine.h"
#include "skill_config.h"
#include "QI.h"
#include <stdio.h>
#include <math.h>

void UI_DrawPlayerHUD(const Entity* p)
{
    int bar_x = 20, bar_y = 20;
    int bar_w = 200, bar_h = 14;

    // HP bar
    float hp_pct = (float)p->hp / p->max_hp;
    Engine_DrawBar(bar_x, bar_y, bar_w, bar_h, hp_pct,
                   (SDL_Color){0, 220, 80, 255},
                   (SDL_Color){20, 20, 20, 220});

    // QI bar
    int qi_y = bar_y + bar_h + 3;
    float qi_pct = (float)p->qi / p->max_qi;
    Engine_DrawBar(bar_x, qi_y, bar_w, 10, qi_pct,
                   (SDL_Color){30, 150, 255, 255},
                   (SDL_Color){20, 20, 20, 220});

    // Cultivation level name (simple colored bar indicator)
    const char* realm_names[] = {
        "Mortal", "QI Refining", "Foundation", "Core Formation",
        "Nascent Soul", "Deity Transformation", "Void Refining",
        "Body Integration", "Mahayana", "Ascension"
    };
    int label_y = qi_y + 14;
    // Draw a small colored dot for cultivation level
    float realm_pct = (float)p->xiuwei / (TOTAL_XIUWEI_LEVEL - 1);
    Engine_DrawBar(bar_x, label_y, 60, 6, realm_pct,
                   (SDL_Color){255, 200, 50, 255},
                   (SDL_Color){30, 30, 30, 200});
}

void UI_DrawEnemyHealthBars()
{
    for (int i = 0; i < g_EntityCount; i++) {
        Entity* e = &g_Entities[i];
        if (!e->is_alive) continue;
        if (e->faction != ENTITY_ENEMY) continue;

        float hp_pct = (float)e->hp / e->max_hp;
        int bar_w = 40;
        int bar_x = (int)e->position.x - bar_w / 2;
        int bar_y = (int)e->position.y - (int)e->radius - 12;
        Engine_DrawBar(bar_x, bar_y, bar_w, 4, hp_pct,
                       (SDL_Color){255, 50, 50, 255},
                       (SDL_Color){40, 40, 40, 200});
    }
}

void UI_DrawDamageNumbers()
{
    for (int i = 0; i < MAX_DAMAGE_NUMBERS; i++) {
        DamageNumber* dn = &g_DamageNumbers[i];
        if (!dn->active) continue;
        Uint8 a = (Uint8)(dn->lifetime / dn->max_lifetime * 255);
        int sx = (int)dn->position.x - 8;
        int sy = (int)dn->position.y - 8;
        Engine_DrawFillRect(sx, sy, 16, 6, (SDL_Color){dn->color.r, dn->color.g, dn->color.b, a});
    }
}

void UI_DrawSkillBar(const Entity* p)
{
    int start_x = (SCREEN_WIDTH - 9 * 44 + 4) / 2; // center 9 slots
    int bar_y = SCREEN_HEIGHT - 56;

    for (int slot = 0; slot < 9; slot++) {
        SkillID id = p->equipped_skills[slot];
        int sx = start_x + slot * 44;

        // Slot background
        SDL_Color slot_bg = {30, 30, 40, 200};
        if (slot == p->selected_skill_slot) {
            slot_bg = (SDL_Color){60, 60, 100, 230}; // highlight selected
        }
        Engine_DrawFillRect(sx, bar_y, 40, 40, slot_bg);

        // Slot border
        Engine_DrawFillRect(sx, bar_y, 40, 1, (SDL_Color){80, 80, 120, 255});
        Engine_DrawFillRect(sx, bar_y + 39, 40, 1, (SDL_Color){80, 80, 120, 255});
        Engine_DrawFillRect(sx, bar_y, 1, 40, (SDL_Color){80, 80, 120, 255});
        Engine_DrawFillRect(sx + 39, bar_y, 1, 40, (SDL_Color){80, 80, 120, 255});

        // Skill name abbreviation
        if (id != SKILL_ID_NONE && id < TOTAL_SKILLS) {
            // Cooldown overlay
            float cd = p->cooldowns[id];
            float max_cd = g_rt_skill_config[id].cooldown_seconds;
            if (max_cd > 0 && cd > 0) {
                float cd_pct = cd / max_cd;
                int cd_h = (int)(40.0f * cd_pct);
                Engine_DrawFillRect(sx, bar_y, 40, cd_h, (SDL_Color){0, 0, 0, 180});
            }

            // Simple colored dot indicator
            SDL_Color attr_col = (SDL_Color){150, 150, 150, 255};
            AttributeID attr = g_skill_database[id].attribute_id;
            switch (attr) {
                case ATTR_FIRE:  attr_col = (SDL_Color){255, 80, 0, 255}; break;
                case ATTR_ICE:   attr_col = (SDL_Color){0, 200, 255, 255}; break;
                case ATTR_WIND:  attr_col = (SDL_Color){180, 255, 200, 255}; break;
                case ATTR_THUNDER: attr_col = (SDL_Color){200, 50, 255, 255}; break;
                case ATTR_METAL: attr_col = (SDL_Color){255, 255, 200, 255}; break;
                case ATTR_WOOD:  attr_col = (SDL_Color){50, 255, 100, 255}; break;
                case ATTR_EARTH: attr_col = (SDL_Color){160, 100, 50, 255}; break;
                case ATTR_BLOOD: attr_col = (SDL_Color){180, 0, 0, 255}; break;
                case ATTR_DARK:  attr_col = (SDL_Color){80, 0, 120, 255}; break;
                case ATTR_LIGHT: attr_col = (SDL_Color){255, 255, 150, 255}; break;
                default: break;
            }
            Engine_DrawFillRect(sx + 16, bar_y + 16, 8, 8, attr_col);
        }

        // Hotkey number
        Engine_DrawFillRect(sx + 2, bar_y + 2, 10, 10, (SDL_Color){200, 200, 200, 150});
    }
}

void UI_DrawWaveInfo(int wave, int score, float game_time)
{
    // Simple text proxy: draw small bars/indicators
    // Wave indicator (top-right)
    int bar_w = 120;
    Engine_DrawBar(SCREEN_WIDTH - bar_w - 20, 20, bar_w, 10, 1.0f,
                   (SDL_Color){200, 180, 50, 255},
                   (SDL_Color){20, 20, 20, 200});
}

void UI_DrawGameOver(int wave, int score, float game_time)
{
    // Dark overlay
    Engine_DrawFillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (SDL_Color){0, 0, 0, 180});

    // Game Over box (centered)
    int box_w = 300, box_h = 160;
    int bx = (SCREEN_WIDTH - box_w) / 2;
    int by = (SCREEN_HEIGHT - box_h) / 2;
    Engine_DrawFillRect(bx, by, box_w, box_h, (SDL_Color){20, 20, 30, 240});
    Engine_DrawFillRect(bx, by, box_w, 2, (SDL_Color){255, 50, 50, 255});
    Engine_DrawFillRect(bx, by + box_h - 2, box_w, 2, (SDL_Color){255, 50, 50, 255});
    Engine_DrawFillRect(bx, by, 2, box_h, (SDL_Color){255, 50, 50, 255});
    Engine_DrawFillRect(bx + box_w - 2, by, 2, box_h, (SDL_Color){255, 50, 50, 255});

    // Restart hint bar
    Engine_DrawFillRect(bx + 60, by + box_h - 40, 180, 6, (SDL_Color){255, 255, 255, 150});
}
