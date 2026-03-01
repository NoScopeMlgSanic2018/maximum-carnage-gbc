// ============================================================
//  MAXIMUM CARNAGE DEMAKE - GBC
//  enemy.c  -  Enemy AI, animation, damage, bosses
//  Enemies: Shriek, Doppelganger, Demogoblin, Carrion,
//           Spidercide, Boss Carnage, Boss Venom, Boss Spider
// ============================================================

#include "main.h"
#include "boss_abilities.h"

// ============================================================
//  ENEMY DATA TABLES
// ============================================================

// HP per enemy type
static const int16_t ENEMY_MAX_HP[] = {
    40,   // SHRIEK
    50,   // DOPPELGANGER
    45,   // DEMOGOBLIN
    35,   // CARRION
    55,   // SPIDERCIDE
    200,  // BOSS_CARNAGE
    180,  // BOSS_VENOM
    160,  // BOSS_SPIDER
};

// Speed per enemy type
static const uint8_t ENEMY_SPEED[] = {
    1,  // SHRIEK
    2,  // DOPPELGANGER
    2,  // DEMOGOBLIN
    1,  // CARRION
    2,  // SPIDERCIDE
    2,  // BOSS_CARNAGE
    2,  // BOSS_VENOM
    3,  // BOSS_SPIDER
};

// Attack damage per enemy type
static const uint8_t ENEMY_DAMAGE[] = {
    8,   // SHRIEK (ranged scream)
    12,  // DOPPELGANGER (melee)
    15,  // DEMOGOBLIN (goblin bomb)
    10,  // CARRION (plague touch)
    14,  // SPIDERCIDE
    20,  // BOSS_CARNAGE
    18,  // BOSS_VENOM
    16,  // BOSS_SPIDER
};

// Attack range (pixel distance to trigger attack)
static const uint8_t ENEMY_ATTACK_RANGE[] = {
    80,  // SHRIEK (screams from far)
    20,  // DOPPELGANGER
    60,  // DEMOGOBLIN (throws bombs)
    18,  // CARRION
    24,  // SPIDERCIDE
    28,  // BOSS_CARNAGE
    26,  // BOSS_VENOM
    32,  // BOSS_SPIDER
};

// Aggro distance (when enemy starts chasing)
static const uint8_t ENEMY_AGGRO_DIST[] = {
    90,  80, 80, 70, 80, 200, 200, 200
};

// Attack cooldown frames
static const uint8_t ENEMY_ATK_COOLDOWN[] = {
    60, 45, 55, 50, 40, 35, 40, 30
};

// Sprite tile base per enemy type
static const uint8_t ENEMY_TILE_BASE[] = {
    128, 136, 144, 152, 160, 168, 180, 192
};

// ============================================================
//  ANIMATION FRAME DATA (tile_offset, dur pairs, 0xFF = end)
// ============================================================
// Format: relative to ENEMY_TILE_BASE[type]
static const uint8_t ENM_ANIM_IDLE[]   = {0, 8, 2, 8, 0xFF};
static const uint8_t ENM_ANIM_WALK[]   = {4, 5, 6, 5, 8, 5, 10, 5, 0xFF};
static const uint8_t ENM_ANIM_ATK[]    = {12, 6, 14, 6, 16, 8, 0xFF};
static const uint8_t ENM_ANIM_HURT[]   = {18, 10, 0xFF};
static const uint8_t ENM_ANIM_DEAD[]   = {20, 6, 22, 6, 24, 6, 26, 99, 0xFF};

// Boss special animations (larger sprites, extra frames)
static const uint8_t BOSS_ANIM_IDLE[]  = {0, 10, 2, 10, 4, 10, 0xFF};
static const uint8_t BOSS_ANIM_WALK[]  = {6, 5, 8, 5, 10, 5, 12, 5, 0xFF};
static const uint8_t BOSS_ANIM_ATK1[]  = {14, 5, 16, 5, 18, 8, 0xFF};
static const uint8_t BOSS_ANIM_ATK2[]  = {20, 4, 22, 4, 24, 4, 26, 8, 0xFF};
static const uint8_t BOSS_ANIM_HURT[]  = {28, 8, 0xFF};

#define ENEMY_STATE_IDLE    0
#define ENEMY_STATE_WALK    1
#define ENEMY_STATE_ATTACK  2
#define ENEMY_STATE_HURT    3
#define ENEMY_STATE_DEAD    4

// ============================================================
//  HELPERS
// ============================================================
static uint8_t is_boss(uint8_t type) {
    return (type >= ENEMY_BOSS_CARNAGE);
}

static int16_t abs16(int16_t v) {
    return v < 0 ? -v : v;
}

static const uint8_t *enemy_anim_table(uint8_t type, uint8_t estate) {
    if (is_boss(type)) {
        switch (estate) {
            case ENEMY_STATE_IDLE:   return BOSS_ANIM_IDLE;
            case ENEMY_STATE_WALK:   return BOSS_ANIM_WALK;
            case ENEMY_STATE_ATTACK: return BOSS_ANIM_ATK1;
            case ENEMY_STATE_HURT:   return BOSS_ANIM_HURT;
            default:                 return BOSS_ANIM_IDLE;
        }
    }
    switch (estate) {
        case ENEMY_STATE_IDLE:   return ENM_ANIM_IDLE;
        case ENEMY_STATE_WALK:   return ENM_ANIM_WALK;
        case ENEMY_STATE_ATTACK: return ENM_ANIM_ATK;
        case ENEMY_STATE_HURT:   return ENM_ANIM_HURT;
        case ENEMY_STATE_DEAD:   return ENM_ANIM_DEAD;
        default:                 return ENM_ANIM_IDLE;
    }
}

static void enemy_anim_advance(uint8_t i) {
    Enemy *e = &g_level.enemies[i];
    e->frame_timer--;
    if (e->frame_timer == 0) {
        e->frame += 2;
        const uint8_t *tbl = enemy_anim_table(e->type, e->state);
        if (tbl[e->frame] == 0xFF) {
            e->frame = 0;
            // Dead: stay on last frame, mark fully dead
            if (e->state == ENEMY_STATE_DEAD) {
                e->alive = 0;
                return;
            }
            // Attack done: return to walk/idle
            if (e->state == ENEMY_STATE_ATTACK) {
                e->state = e->aggro ? ENEMY_STATE_WALK : ENEMY_STATE_IDLE;
            }
            // Hurt done: return to walk
            if (e->state == ENEMY_STATE_HURT) {
                e->state = ENEMY_STATE_WALK;
            }
        }
        e->frame_timer = tbl[e->frame + 1];
    }
}

static void enemy_set_state(uint8_t i, uint8_t new_state) {
    Enemy *e = &g_level.enemies[i];
    if (e->state == new_state) return;
    e->state = new_state;
    e->frame = 0;
    const uint8_t *tbl = enemy_anim_table(e->type, new_state);
    e->frame_timer = tbl[1];
}

// ============================================================
//  SPAWN
// ============================================================
void enemy_spawn(uint8_t idx, uint8_t type, int16_t x, int16_t y) {
    Enemy *e      = &g_level.enemies[idx];
    memset(e, 0, sizeof(Enemy));
    e->type       = type;
    e->x          = x << 4;
    e->y          = y << 4;
    e->hp         = ENEMY_MAX_HP[type];
    e->alive      = 1;
    e->state      = ENEMY_STATE_IDLE;
    e->frame      = 0;
    e->frame_timer= 8;
    e->patrol_left  = (x - 40) << 4;
    e->patrol_right = (x + 40) << 4;
    e->attack_timer = 0;
    e->aggro      = 0;
    e->facing     = 1; // face left (toward player start)
}

// ============================================================
//  DAMAGE
// ============================================================
void enemy_take_damage(uint8_t idx, uint8_t dmg) {
    Enemy *e = &g_level.enemies[idx];
    if (!e->alive) return;

    e->hp -= dmg;
    sfx_play(8);

    if (e->hp <= 0) {
        e->hp    = 0;
        enemy_set_state(idx, ENEMY_STATE_DEAD);
        e->vx    = 0;
        e->vy    = -3;

        // Boss death: trigger level complete + unlock ability
        if (is_boss(e->type)) {
            g_level.boss_active = 0;
            g_level.complete    = 1;
            boss_ability_unlock(e->type);   // player absorbs boss power
            sfx_play(10); // boss death sfx
        } else {
            sfx_play(9); // enemy death sfx
        }
    } else {
        enemy_set_state(idx, ENEMY_STATE_HURT);
        // Knockback away from player
        int16_t px = g_player.x >> 4;
        int16_t ex = e->x >> 4;
        e->vx = (ex > px) ? 3 : -3;
        e->vy = -2;
    }
}

// ============================================================
//  AI  (per enemy type)
// ============================================================

// --- Generic melee AI ---
static void ai_melee(uint8_t i) {
    Enemy *e  = &g_level.enemies[i];
    int16_t px = g_player.x >> 4;
    int16_t py = g_player.y >> 4;
    int16_t ex = e->x >> 4;
    int16_t ey = e->y >> 4;
    int16_t dx = px - ex;
    int16_t dy = py - ey;
    int16_t dist = abs16(dx);

    // Aggro check
    if (dist < ENEMY_AGGRO_DIST[e->type]) e->aggro = 1;

    if (!e->aggro) {
        // Patrol
        if (e->facing == 0) {
            e->vx = ENEMY_SPEED[e->type];
            if (e->x > e->patrol_right) e->facing = 1;
        } else {
            e->vx = -ENEMY_SPEED[e->type];
            if (e->x < e->patrol_left) e->facing = 0;
        }
        enemy_set_state(i, ENEMY_STATE_WALK);
    } else {
        // Chase
        e->facing = (dx < 0) ? 1 : 0;
        if (dist > ENEMY_ATTACK_RANGE[e->type]) {
            e->vx = (dx > 0) ? ENEMY_SPEED[e->type] : -ENEMY_SPEED[e->type];
            enemy_set_state(i, ENEMY_STATE_WALK);
        } else {
            // In range: attack
            e->vx = 0;
            if (e->attack_timer == 0 && e->state != ENEMY_STATE_ATTACK) {
                enemy_set_state(i, ENEMY_STATE_ATTACK);
                e->attack_timer = ENEMY_ATK_COOLDOWN[e->type];
                // Deal damage to player when attack animation hits
                if (abs16(dy) < 32) {
                    player_take_damage(ENEMY_DAMAGE[e->type]);
                }
            }
        }
    }

    if (e->attack_timer > 0) e->attack_timer--;
}

// --- Ranged AI (Shriek, Demogoblin) ---
static void ai_ranged(uint8_t i) {
    Enemy *e  = &g_level.enemies[i];
    int16_t px = g_player.x >> 4;
    int16_t ex = e->x >> 4;
    int16_t dx = px - ex;
    int16_t dist = abs16(dx);

    if (dist < ENEMY_AGGRO_DIST[e->type]) e->aggro = 1;

    if (!e->aggro) {
        // Patrol slowly
        e->vx = e->facing ? -1 : 1;
        if (e->x > e->patrol_right) e->facing = 1;
        if (e->x < e->patrol_left)  e->facing = 0;
        enemy_set_state(i, ENEMY_STATE_WALK);
    } else {
        e->facing = (dx < 0) ? 1 : 0;
        // Maintain preferred attack distance
        uint8_t pref_dist = ENEMY_ATTACK_RANGE[e->type];
        if (dist > pref_dist + 20) {
            e->vx = (dx > 0) ? 1 : -1;
            enemy_set_state(i, ENEMY_STATE_WALK);
        } else if (dist < pref_dist - 20) {
            e->vx = (dx > 0) ? -1 : 1;
            enemy_set_state(i, ENEMY_STATE_WALK);
        } else {
            e->vx = 0;
            if (e->attack_timer == 0) {
                enemy_set_state(i, ENEMY_STATE_ATTACK);
                e->attack_timer = ENEMY_ATK_COOLDOWN[e->type];
                // Ranged attack always hits if on same Y level
                int16_t py = g_player.y >> 4;
                int16_t ey = e->y >> 4;
                if (abs16(py - ey) < 24) {
                    player_take_damage(ENEMY_DAMAGE[e->type]);
                }
            }
        }
    }
    if (e->attack_timer > 0) e->attack_timer--;
}

// --- Doppelganger: mirrors player moves ---
static void ai_doppelganger(uint8_t i) {
    Enemy *e  = &g_level.enemies[i];
    int16_t px = g_player.x >> 4;
    int16_t ex = e->x >> 4;
    int16_t dx = px - ex;

    if (abs16(dx) < ENEMY_AGGRO_DIST[e->type]) e->aggro = 1;

    if (e->aggro) {
        e->facing = (dx < 0) ? 1 : 0;
        // Mirrors player velocity (opponent spirit)
        e->vx = -g_player.vx;
        if (abs16(dx) < ENEMY_ATTACK_RANGE[e->type]) {
            e->vx = 0;
            if (e->attack_timer == 0) {
                enemy_set_state(i, ENEMY_STATE_ATTACK);
                e->attack_timer = ENEMY_ATK_COOLDOWN[e->type];
                player_take_damage(ENEMY_DAMAGE[e->type]);
            }
        } else {
            enemy_set_state(i, ENEMY_STATE_WALK);
        }
    }
    if (e->attack_timer > 0) e->attack_timer--;
}

// --- Boss AI (phases) ---
static uint8_t boss_phase[MAX_ENEMIES] = {0, 0, 0, 0};

static void ai_boss(uint8_t i) {
    Enemy *e   = &g_level.enemies[i];
    int16_t px  = g_player.x >> 4;
    int16_t ex  = e->x >> 4;
    int16_t dx  = px - ex;
    int16_t dist = abs16(dx);

    e->aggro  = 1; // bosses always aggro
    e->facing = (dx < 0) ? 1 : 0;

    // Phase transitions based on HP
    uint8_t max_hp = ENEMY_MAX_HP[e->type];
    if (e->hp < (max_hp / 3) && boss_phase[i] < 2) {
        boss_phase[i] = 2; // Enraged: faster, harder
        sfx_play(11);
    } else if (e->hp < (max_hp * 2 / 3) && boss_phase[i] < 1) {
        boss_phase[i] = 1; // Phase 2
    }

    uint8_t spd = ENEMY_SPEED[e->type] + boss_phase[i];

    if (dist > ENEMY_ATTACK_RANGE[e->type]) {
        e->vx = (dx > 0) ? spd : -spd;
        enemy_set_state(i, ENEMY_STATE_WALK);
    } else {
        e->vx = 0;
        if (e->attack_timer == 0) {
            // Phase 2+: also leap/charge
            if (boss_phase[i] >= 1 && (g_frame_count & 0x3F) == 0) {
                e->vy = -8;  // jump at player
                e->vx = (dx > 0) ? 4 : -4;
            }
            enemy_set_state(i, ENEMY_STATE_ATTACK);
            e->attack_timer = ENEMY_ATK_COOLDOWN[e->type] - (boss_phase[i] * 8);
            player_take_damage(ENEMY_DAMAGE[e->type] + (boss_phase[i] * 4));
        }
    }

    // Boss gravity
    if (!e->on_ground) {
        e->vy += 1;
        if (e->vy > 8) e->vy = 8;
    }

    if (e->attack_timer > 0) e->attack_timer--;
}

// ============================================================
//  INIT
// ============================================================
void enemies_init(void) {
    for (uint8_t i = 0; i < MAX_ENEMIES; i++) {
        g_level.enemies[i].alive = 0;
        boss_phase[i] = 0;
    }
}

// ============================================================
//  UPDATE ALL
// ============================================================
void enemies_update(void) {
    for (uint8_t i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g_level.enemies[i];
        if (!e->alive) continue;

        // Skip AI when hurt/dead (physics still applies)
        if (e->state != ENEMY_STATE_HURT && e->state != ENEMY_STATE_DEAD) {
            switch (e->type) {
                case ENEMY_SHRIEK:      ai_ranged(i);      break;
                case ENEMY_DOPPELGANGER:ai_doppelganger(i);break;
                case ENEMY_DEMOGOBLIN:  ai_ranged(i);      break;
                case ENEMY_CARRION:     ai_melee(i);       break;
                case ENEMY_SPIDERCIDE:  ai_melee(i);       break;
                case ENEMY_BOSS_CARNAGE:
                case ENEMY_BOSS_VENOM:
                case ENEMY_BOSS_SPIDER: ai_boss(i);        break;
            }
        }

        // Simple gravity for enemies
        if (!e->on_ground) {
            e->vy += 1;
            if (e->vy > 8) e->vy = 8;
        }

        // Move
        e->x += e->vx << 4;
        e->y += e->vy << 4;

        // Floor clamp (simplified)
        int16_t ey = e->y >> 4;
        if (ey > 100) {
            e->y = 100 << 4;
            e->vy = 0;
            e->on_ground = 1;
        }

        // Advance animation
        enemy_anim_advance(i);
    }
}

// ============================================================
//  DRAW
// ============================================================
void enemies_draw(void) {
    for (uint8_t i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g_level.enemies[i];
        if (!e->alive) continue;

        const uint8_t *tbl = enemy_anim_table(e->type, e->state);
        uint8_t tile_off   = tbl[e->frame];
        uint8_t base_tile  = ENEMY_TILE_BASE[e->type] + tile_off;

        int16_t sx = (e->x >> 4) - g_level.scroll_x + 8;
        int16_t sy = (e->y >> 4) + 16;

        if (sx < -16 || sx > 176) continue; // off screen

        uint8_t flip = e->facing ? 0 : S_FLIPX;
        uint8_t pal  = PAL_ENEMIES;

        uint8_t slot = ENEMY_SPR_SLOT_START + (i * 4);

        shadow_OAM[slot + 0].y    = sy;
        shadow_OAM[slot + 0].x    = sx;
        shadow_OAM[slot + 0].tile = base_tile;
        shadow_OAM[slot + 0].prop = flip | pal;

        shadow_OAM[slot + 1].y    = sy;
        shadow_OAM[slot + 1].x    = sx + 8;
        shadow_OAM[slot + 1].tile = base_tile + 2;
        shadow_OAM[slot + 1].prop = flip | pal;

        shadow_OAM[slot + 2].y    = sy + 16;
        shadow_OAM[slot + 2].x    = sx;
        shadow_OAM[slot + 2].tile = base_tile + 4;
        shadow_OAM[slot + 2].prop = flip | pal;

        shadow_OAM[slot + 3].y    = sy + 16;
        shadow_OAM[slot + 3].x    = sx + 8;
        shadow_OAM[slot + 3].tile = base_tile + 6;
        shadow_OAM[slot + 3].prop = flip | pal;
    }
}
