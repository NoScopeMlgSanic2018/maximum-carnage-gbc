// ============================================================
//  MAXIMUM CARNAGE DEMAKE - GBC
//  player.c  -  Player controller, animation, attacks
//               Spider-Man: web-swing, web-shot
//               Venom: symbiote grab, wall-crawl
//               Carnage: tendrils, rage mode
// ============================================================

#include "main.h"
#include "web_swing.h"
#include "boss_abilities.h"
#include "spiderman_sprites.h"
#include "venom_sprites.h"
#include "carnage_sprites.h"

// ============================================================
//  ANIMATION FRAME TABLES
//  Each entry: { tile_index, duration_frames }
// ============================================================

// --- Spider-Man ---
static const uint8_t SPIDEY_ANIM_IDLE[]    = {0,  8, 2, 8, 0xFF};
static const uint8_t SPIDEY_ANIM_WALK[]    = {4,  4, 6, 4, 8,  4, 10, 4, 0xFF};
static const uint8_t SPIDEY_ANIM_JUMP[]    = {12, 99, 0xFF};
static const uint8_t SPIDEY_ANIM_FALL[]    = {14, 99, 0xFF};
static const uint8_t SPIDEY_ANIM_ATK1[]    = {16, 6, 18, 6, 0xFF};   // punch
static const uint8_t SPIDEY_ANIM_ATK2[]    = {20, 5, 22, 5, 24, 5, 0xFF}; // kick combo
static const uint8_t SPIDEY_ANIM_SPECIAL[] = {26, 4, 28, 4, 30, 4, 0xFF}; // web-shot
static const uint8_t SPIDEY_ANIM_HURT[]    = {32, 12, 0xFF};
static const uint8_t SPIDEY_ANIM_SWING[]   = {34, 4, 36, 4, 38, 4, 0xFF};

// --- Venom ---
static const uint8_t VENOM_ANIM_IDLE[]     = {40, 10, 42, 10, 0xFF};
static const uint8_t VENOM_ANIM_WALK[]     = {44, 5, 46, 5, 48, 5, 50, 5, 0xFF};
static const uint8_t VENOM_ANIM_JUMP[]     = {52, 99, 0xFF};
static const uint8_t VENOM_ANIM_FALL[]     = {54, 99, 0xFF};
static const uint8_t VENOM_ANIM_ATK1[]     = {56, 6, 58, 6, 0xFF};
static const uint8_t VENOM_ANIM_ATK2[]     = {60, 5, 62, 5, 64, 5, 0xFF};
static const uint8_t VENOM_ANIM_SPECIAL[]  = {66, 4, 68, 4, 70, 8, 0xFF}; // symbiote whip
static const uint8_t VENOM_ANIM_HURT[]     = {72, 12, 0xFF};
static const uint8_t VENOM_ANIM_SYMBIOTE[] = {74, 4, 76, 4, 78, 4, 0xFF}; // wall crawl

// --- Carnage ---
static const uint8_t CARNAGE_ANIM_IDLE[]   = {80, 8, 82, 8, 84, 8, 0xFF};
static const uint8_t CARNAGE_ANIM_WALK[]   = {86, 4, 88, 4, 90, 4, 92, 4, 0xFF};
static const uint8_t CARNAGE_ANIM_JUMP[]   = {94, 99, 0xFF};
static const uint8_t CARNAGE_ANIM_FALL[]   = {96, 99, 0xFF};
static const uint8_t CARNAGE_ANIM_ATK1[]   = {98, 5, 100, 5, 0xFF};
static const uint8_t CARNAGE_ANIM_ATK2[]   = {102, 4, 104, 4, 106, 4, 0xFF};
static const uint8_t CARNAGE_ANIM_SPECIAL[] = {108, 3, 110, 3, 112, 3, 114, 6, 0xFF}; // tendril burst
static const uint8_t CARNAGE_ANIM_HURT[]   = {116, 12, 0xFF};
static const uint8_t CARNAGE_ANIM_RAGE[]   = {118, 4, 120, 4, 122, 4, 0xFF}; // rage mode

// Hitbox sizes per character (w, h)
static const uint8_t CHAR_HITBOX_W[] = {12, 14, 13};
static const uint8_t CHAR_HITBOX_H[] = {28, 30, 28};

// Attack hitbox offsets (facing right, x_off, y_off, w, h)
static const int8_t ATK1_HIT_X[]  = {10,  12,  10};
static const int8_t ATK1_HIT_Y[]  = {8,   10,  6};
static const uint8_t ATK1_HIT_W[] = {18,  20,  18};
static const uint8_t ATK1_HIT_H[] = {12,  14,  14};

static const int8_t ATK2_HIT_X[]  = {8,   10,  8};
static const int8_t ATK2_HIT_Y[]  = {16,  14,  12};
static const uint8_t ATK2_HIT_W[] = {22,  24,  22};
static const uint8_t ATK2_HIT_H[] = {16,  18,  20};

// Special hitbox (wider range)
static const int8_t SPEC_HIT_X[]  = {10,  0,   -20};
static const int8_t SPEC_HIT_Y[]  = {8,   -4,  0};
static const uint8_t SPEC_HIT_W[] = {60,  80,  48};  // web-shot range
static const uint8_t SPEC_HIT_H[] = {12,  40,  48};

// Max HP per character
static const int16_t CHAR_MAX_HP[] = {100, 130, 110};

// Web-swing state (Spider-Man only)
static uint8_t  swing_timer   = 0;
static int8_t   swing_angle   = 0;   // -4 to 4

// Carnage rage mode
static uint8_t  rage_active   = 0;
static uint8_t  rage_timer    = 0;
#define RAGE_THRESHOLD  30   // HP below this triggers rage

// Wall crawl (Venom)
static uint8_t  on_wall       = 0;
static uint8_t  wall_side     = 0;  // 0=right 1=left

// ============================================================
//  ANIMATION HELPERS
// ============================================================
static const uint8_t *anim_get_table(uint8_t character, uint8_t state) {
    switch (character) {
        case CHAR_SPIDERMAN:
            switch (state) {
                case PSTATE_IDLE:     return SPIDEY_ANIM_IDLE;
                case PSTATE_WALK:     return SPIDEY_ANIM_WALK;
                case PSTATE_JUMP:     return SPIDEY_ANIM_JUMP;
                case PSTATE_FALL:     return SPIDEY_ANIM_FALL;
                case PSTATE_ATTACK1:  return SPIDEY_ANIM_ATK1;
                case PSTATE_ATTACK2:  return SPIDEY_ANIM_ATK2;
                case PSTATE_SPECIAL:  return SPIDEY_ANIM_SPECIAL;
                case PSTATE_HURT:     return SPIDEY_ANIM_HURT;
                case PSTATE_WEBSWING: return SPIDEY_ANIM_SWING;
                default:              return SPIDEY_ANIM_IDLE;
            }
        case CHAR_VENOM:
            switch (state) {
                case PSTATE_IDLE:     return VENOM_ANIM_IDLE;
                case PSTATE_WALK:     return VENOM_ANIM_WALK;
                case PSTATE_JUMP:     return VENOM_ANIM_JUMP;
                case PSTATE_FALL:     return VENOM_ANIM_FALL;
                case PSTATE_ATTACK1:  return VENOM_ANIM_ATK1;
                case PSTATE_ATTACK2:  return VENOM_ANIM_ATK2;
                case PSTATE_SPECIAL:  return VENOM_ANIM_SPECIAL;
                case PSTATE_HURT:     return VENOM_ANIM_HURT;
                case PSTATE_SYMBIOTE: return VENOM_ANIM_SYMBIOTE;
                default:              return VENOM_ANIM_IDLE;
            }
        case CHAR_CARNAGE:
        default:
            switch (state) {
                case PSTATE_IDLE:     return CARNAGE_ANIM_IDLE;
                case PSTATE_WALK:     return CARNAGE_ANIM_WALK;
                case PSTATE_JUMP:     return CARNAGE_ANIM_JUMP;
                case PSTATE_FALL:     return CARNAGE_ANIM_FALL;
                case PSTATE_ATTACK1:  return CARNAGE_ANIM_ATK1;
                case PSTATE_ATTACK2:  return CARNAGE_ANIM_ATK2;
                case PSTATE_SPECIAL:  return CARNAGE_ANIM_SPECIAL;
                case PSTATE_HURT:     return CARNAGE_ANIM_HURT;
                case PSTATE_SYMBIOTE: return CARNAGE_ANIM_RAGE;
                default:              return CARNAGE_ANIM_IDLE;
            }
    }
}

static void anim_advance(void) {
    const uint8_t *tbl = anim_get_table(g_player.character, g_player.state);
    g_player.frame_timer--;
    if (g_player.frame_timer == 0) {
        g_player.frame += 2;                   // step to next pair {tile, dur}
        if (tbl[g_player.frame] == 0xFF) {     // end of animation
            g_player.frame = 0;
            // Non-looping states: return to idle when done
            if (g_player.state == PSTATE_ATTACK1 ||
                g_player.state == PSTATE_ATTACK2 ||
                g_player.state == PSTATE_SPECIAL  ||
                g_player.state == PSTATE_HURT) {
                g_player.state = PSTATE_IDLE;
                g_player.attack_frame = 0;
            }
        }
        g_player.frame_timer = tbl[g_player.frame + 1];
    }
}

static void anim_set_state(uint8_t new_state) {
    if (g_player.state == new_state) return;
    g_player.state       = new_state;
    g_player.frame       = 0;
    const uint8_t *tbl   = anim_get_table(g_player.character, new_state);
    g_player.frame_timer = tbl[1]; // first duration
    g_player.attack_frame = 0;
}

// ============================================================
//  INIT
// ============================================================
void player_init(uint8_t character) {
    memset(&g_player, 0, sizeof(Player));
    g_player.character = character;
    g_player.x         = 32 << 4;   // start position (fixed point)
    g_player.y         = 80 << 4;
    g_player.max_hp    = CHAR_MAX_HP[character];
    g_player.hp        = g_player.max_hp;
    g_player.alive     = 1;
    g_player.facing    = 0; // right
    g_player.state     = PSTATE_IDLE;
    g_player.frame_timer = 8;

    swing_timer  = 0;
    on_wall      = 0;
    rage_active  = 0;
    rage_timer   = 0;

    palette_set_character(character);
}

// ============================================================
//  DAMAGE
// ============================================================
void player_take_damage(uint8_t dmg) {
    if (g_player.inv_frames > 0) return;
    if (g_player.state == PSTATE_DEAD) return;

    // Carnage rage mode: 50% resistance
    if (rage_active && g_player.character == CHAR_CARNAGE) {
        dmg = dmg >> 1;
    }

    g_player.hp -= dmg;
    g_player.inv_frames = INVINCIBILITY_FRAMES;
    sfx_play(0); // hurt sfx

    if (g_player.hp <= 0) {
        g_player.hp    = 0;
        g_player.alive = 0;
        anim_set_state(PSTATE_DEAD);
    } else {
        anim_set_state(PSTATE_HURT);
        // Knockback
        g_player.vx = g_player.facing ? 3 : -3;
        g_player.vy = -4;

        // Carnage rage trigger
        if (g_player.character == CHAR_CARNAGE &&
            g_player.hp < RAGE_THRESHOLD && !rage_active) {
            rage_active = 1;
            rage_timer  = 180; // 3 seconds
            sfx_play(3);       // rage sfx
        }
    }
}

// ============================================================
//  ATTACK HIT DETECTION
// ============================================================
void player_check_attack_hit(void) {
    uint8_t c = g_player.character;
    int16_t px = g_player.x >> 4;
    int16_t py = g_player.y >> 4;

    int8_t  hx_off, hy_off;
    uint8_t hw, hh;
    uint8_t dmg;

    if (g_player.state == PSTATE_ATTACK1 && g_player.attack_frame == 1) {
        hx_off = ATK1_HIT_X[c];
        hy_off = ATK1_HIT_Y[c];
        hw     = ATK1_HIT_W[c];
        hh     = ATK1_HIT_H[c];
        dmg    = ATTACK1_DAMAGE + (rage_active ? 5 : 0);
        g_player.attack_frame = 2; // only hit once per swing
    } else if (g_player.state == PSTATE_ATTACK2 && g_player.attack_frame == 1) {
        hx_off = ATK2_HIT_X[c];
        hy_off = ATK2_HIT_Y[c];
        hw     = ATK2_HIT_W[c];
        hh     = ATK2_HIT_H[c];
        dmg    = ATTACK2_DAMAGE + (rage_active ? 8 : 0);
        g_player.attack_frame = 2;
    } else if (g_player.state == PSTATE_SPECIAL && g_player.attack_frame == 1) {
        hx_off = SPEC_HIT_X[c];
        hy_off = SPEC_HIT_Y[c];
        hw     = SPEC_HIT_W[c];
        hh     = SPEC_HIT_H[c];
        dmg    = SPECIAL_DAMAGE;
        g_player.attack_frame = 2;
        g_player.special_meter = 0; // drain special
    } else {
        return;
    }

    // Flip hitbox if facing left
    int16_t hx = px + (g_player.facing ? -hx_off - hw : hx_off);
    int16_t hy = py + hy_off;

    // Check against all enemies
    for (uint8_t i = 0; i < MAX_ENEMIES; i++) {
        if (!g_level.enemies[i].alive) continue;
        int16_t ex = g_level.enemies[i].x >> 4;
        int16_t ey = g_level.enemies[i].y >> 4;
        if (physics_collide_rect(hx, hy, hw, hh, ex, ey, 14, 28)) {
            enemy_take_damage(i, dmg);
            g_game.score += dmg * 10;
        }
    }
}

// ============================================================
//  CHARACTER-SPECIFIC MECHANICS
// ============================================================

// ── Spider-Man web systems ────────────────────────────────────────────────────

// Primary special: web-shot (grounded or airborne when not swinging)
static void spiderman_web_shot(void) {
    if (g_player.special_meter < 25) return;
    g_player.special_meter -= 25;
    anim_set_state(PSTATE_SPECIAL);
    g_player.attack_frame = 1;
    sfx_play(2);
}

// Handle A button for web-swing (anywhere = arcade feel)
// A = jump when on ground, A = attach swing when airborne
static void spiderman_handle_a(void) {
    if (g_player.on_ground) {
        // Normal jump
        g_player.vy = JUMP_FORCE;
        g_player.on_ground = 0;
        anim_set_state(PSTATE_JUMP);
        sfx_play(6);
    } else if (!web_swing_is_active()) {
        // Attach swing
        web_swing_attach(&g_player);
        sfx_play(1);
    }
}

// Called when player releases A mid-swing
static void spiderman_handle_a_release(void) {
    if (web_swing_is_active()) {
        web_swing_detach(&g_player);
        sfx_play(3);   // web release whoosh
    }
}

// Venom: wall crawl on left/right against wall, symbiote whip as special
static void venom_special(void) {
    if (g_player.special_meter < 30) return;
    anim_set_state(PSTATE_SPECIAL);
    g_player.attack_frame = 1;
    sfx_play(4);
}

static void venom_check_wall(void) {
    int16_t px = g_player.x >> 4;
    int16_t py = g_player.y >> 4;
    uint8_t hit_right = level_is_solid(px + 16, py + 4);
    uint8_t hit_left  = level_is_solid(px - 2,  py + 4);

    if (hit_right || hit_left) {
        on_wall   = 1;
        wall_side = hit_right ? 0 : 1;
        g_player.vy = 0; // stick to wall
    } else {
        on_wall = 0;
    }
}

// Carnage: tendril burst as special, rage mode passive
static void carnage_special(void) {
    if (g_player.special_meter < 35) return;
    anim_set_state(PSTATE_SPECIAL);
    g_player.attack_frame = 1;
    sfx_play(5);
}

static void carnage_update_rage(void) {
    if (!rage_active) return;
    rage_timer--;
    if (rage_timer == 0) {
        rage_active = 0;
    }
    // Rage: regenerate a tiny bit of HP
    if ((g_frame_count & 0x1F) == 0 && g_player.hp < g_player.max_hp) {
        g_player.hp++;
    }
}

// ============================================================
//  MAIN PLAYER UPDATE
// ============================================================
void player_update(void) {
    if (!g_player.alive) return;

    // --- Invincibility countdown ---
    if (g_player.inv_frames) g_player.inv_frames--;

    // --- Special meter fill (time-based) ---
    if ((g_frame_count & 0x3F) == 0 && g_player.special_meter < 100) {
        g_player.special_meter++;
    }

    // --- Character-specific passives ---
    if (g_player.character == CHAR_CARNAGE)  carnage_update_rage();
    if (g_player.character == CHAR_VENOM)    venom_check_wall();

    // --- Input handling (only when not in locked state) ---
    uint8_t can_input = (g_player.state != PSTATE_HURT  &&
                         g_player.state != PSTATE_DEAD  &&
                         g_player.state != PSTATE_SPECIAL);

    if (can_input) {
        // --- Movement ---
        if (g_joypad & J_LEFT) {
            g_player.facing = 1;
            if (on_wall && g_player.character == CHAR_VENOM) {
                // Slide down wall
                g_player.vy = 1;
            } else {
                g_player.vx = -WALK_SPEED;
                if (g_player.on_ground) anim_set_state(PSTATE_WALK);
            }
        } else if (g_joypad & J_RIGHT) {
            g_player.facing = 0;
            if (on_wall && g_player.character == CHAR_VENOM) {
                g_player.vy = 1;
            } else {
                g_player.vx = WALK_SPEED;
                if (g_player.on_ground) anim_set_state(PSTATE_WALK);
            }
        } else {
            // Friction
            if (g_player.vx > 0) g_player.vx--;
            else if (g_player.vx < 0) g_player.vx++;
            if (g_player.on_ground && g_player.vx == 0) {
                anim_set_state(PSTATE_IDLE);
            }
        }

        // --- Jump (A button) ---
        // Spider-Man: A = jump OR attach web-swing
        // Venom/Carnage: A = jump
        if (g_joypad_pressed & J_A) {
            if (g_player.character == CHAR_SPIDERMAN) {
                spiderman_handle_a();
            } else if (g_player.on_ground || on_wall) {
                g_player.vy = JUMP_FORCE;
                g_player.on_ground = 0;
                on_wall = 0;
                anim_set_state(PSTATE_JUMP);
                sfx_play(6);
            }
        }

        // Spider-Man: release A to detach web-swing
        if (g_player.character == CHAR_SPIDERMAN) {
            if (!(g_joypad & J_A) && web_swing_is_active()) {
                spiderman_handle_a_release();
            }
        }

        // --- Attack 1 (B) ---
        if (g_joypad_pressed & J_B) {
            if (!web_swing_is_active()) {
                if (g_player.state == PSTATE_ATTACK1) {
                    anim_set_state(PSTATE_ATTACK2);
                    g_player.attack_frame = 1;
                } else {
                    anim_set_state(PSTATE_ATTACK1);
                    g_player.attack_frame = 1;
                }
                sfx_play(7);
            }
        }

        // --- Primary special (A+B) - fire active ability slot --------
        if ((g_joypad & J_A) && (g_joypad_pressed & J_B) && !web_swing_is_active()) {
            uint8_t used = boss_ability_fire(&g_player, g_enemies, g_level.enemy_count);
            if (!used) {
                // Slot 0 = primary character special
                switch (g_player.character) {
                    case CHAR_SPIDERMAN: spiderman_web_shot(); break;
                    case CHAR_VENOM:     venom_special();      break;
                    case CHAR_CARNAGE:   carnage_special();    break;
                }
            }
        }

        // --- SELECT: cycle ability slots (Spider-Man + unlocked bosses) ---
        if (g_joypad_pressed & J_SELECT) {
            boss_ability_cycle();
            sfx_play(8);  // soft click SFX
        }

    // --- Web-swing physics (Spider-Man - full pendulum system) ---
    if (g_player.character == CHAR_SPIDERMAN) {
        if (web_swing_is_active()) {
            uint8_t a_held = (g_joypad & J_A) ? 1 : 0;
            web_swing_update(&g_player, a_held);
        }
    }

    // --- Boss ability projectile / clone update ---
    boss_abilities_update(g_enemies, g_level.enemy_count);
    physics_apply_gravity(&g_player);
    physics_move_player(&g_player);

    // --- Attack hit detection ---
    if (g_player.attack_frame == 1) {
        player_check_attack_hit();
    }

    // --- Advance animation ---
    anim_advance();
}

// ============================================================
//  DRAW  (OAM sprite output)
// ============================================================
void player_draw(void) {
    if (!g_player.alive) return;

    const uint8_t *tbl   = anim_get_table(g_player.character, g_player.state);
    uint8_t tile         = tbl[g_player.frame];

    int16_t sx = (g_player.x >> 4) - (g_level.scroll_x) + 8;
    int16_t sy = (g_player.y >> 4) + 16;    // OAM Y offset

    // Blink during invincibility
    if (g_player.inv_frames && (g_frame_count & 0x02)) return;

    // 8x16 mode: 2 sprites make a 16x16 character, we use 2 columns = 4 sprites total
    uint8_t flip = g_player.facing ? S_FLIPX : 0;
    uint8_t pal  = g_player.character; // palette 0/1/2

    // Rage mode: extra red tint for Carnage
    if (rage_active && g_player.character == CHAR_CARNAGE) {
        pal = PAL_EFFECTS;
    }

    // Top-left sprite
    shadow_OAM[PLAYER_SPR_SLOT + 0].y    = sy;
    shadow_OAM[PLAYER_SPR_SLOT + 0].x    = sx;
    shadow_OAM[PLAYER_SPR_SLOT + 0].tile = tile;
    shadow_OAM[PLAYER_SPR_SLOT + 0].prop = flip | pal;

    // Top-right sprite
    shadow_OAM[PLAYER_SPR_SLOT + 1].y    = sy;
    shadow_OAM[PLAYER_SPR_SLOT + 1].x    = sx + 8;
    shadow_OAM[PLAYER_SPR_SLOT + 1].tile = tile + 2;
    shadow_OAM[PLAYER_SPR_SLOT + 1].prop = flip | pal;

    // Bottom-left
    shadow_OAM[PLAYER_SPR_SLOT + 2].y    = sy + 16;
    shadow_OAM[PLAYER_SPR_SLOT + 2].x    = sx;
    shadow_OAM[PLAYER_SPR_SLOT + 2].tile = tile + 4;
    shadow_OAM[PLAYER_SPR_SLOT + 2].prop = flip | pal;

    // Bottom-right
    shadow_OAM[PLAYER_SPR_SLOT + 3].y    = sy + 16;
    shadow_OAM[PLAYER_SPR_SLOT + 3].x    = sx + 8;
    shadow_OAM[PLAYER_SPR_SLOT + 3].tile = tile + 6;
    shadow_OAM[PLAYER_SPR_SLOT + 3].prop = flip | pal;
}
