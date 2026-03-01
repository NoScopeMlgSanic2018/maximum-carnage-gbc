// ============================================================
//  MAXIMUM CARNAGE DEMAKE - GBC
//  physics.c  -  Gravity, AABB collision, player movement
// ============================================================

#include "main.h"

// ============================================================
//  AABB COLLISION CHECK
// ============================================================
int8_t physics_collide_rect(int16_t ax, int16_t ay, uint8_t aw, uint8_t ah,
                            int16_t bx, int16_t by, uint8_t bw, uint8_t bh) {
    if (ax + aw <= bx) return 0;
    if (bx + bw <= ax) return 0;
    if (ay + ah <= by) return 0;
    if (by + bh <= ay) return 0;
    return 1;
}

// ============================================================
//  GRAVITY
// ============================================================
void physics_apply_gravity(Player *p) {
    if (!p->on_ground) {
        p->vy += GRAVITY;
        if (p->vy > MAX_FALL_SPEED) p->vy = MAX_FALL_SPEED;
    }
}

// ============================================================
//  PLAYER MOVEMENT + COLLISION
// ============================================================
void physics_move_player(Player *p) {
    // --- Horizontal movement ---
    int16_t new_x = p->x + (p->vx << 4);

    // World bounds
    if ((new_x >> 4) < 4)               new_x = 4 << 4;
    if ((new_x >> 4) > (MAP_W_TILES * TILE_SIZE - 20))
        new_x = (MAP_W_TILES * TILE_SIZE - 20) << 4;

    // Wall collision (check tile to the side)
    int16_t nx_tile = new_x >> 4;
    int16_t cy = (p->y >> 4) + 8;  // check middle of character

    if (p->vx > 0 && level_is_solid(nx_tile + 16, cy)) {
        // Clamp to wall
        new_x = ((nx_tile + 16) / TILE_SIZE * TILE_SIZE - 17) << 4;
        p->vx = 0;
    } else if (p->vx < 0 && level_is_solid(nx_tile - 1, cy)) {
        new_x = ((nx_tile / TILE_SIZE + 1) * TILE_SIZE) << 4;
        p->vx = 0;
    }
    p->x = new_x;

    // --- Vertical movement ---
    int16_t new_y = p->y + (p->vy << 4);
    int16_t ny_tile = new_y >> 4;
    int16_t cx = (p->x >> 4) + 8; // horizontal center

    p->on_ground = 0;

    if (p->vy > 0) {
        // Falling: check floor
        if (level_is_solid(cx, ny_tile + 28)) {
            // Land on floor
            new_y = ((ny_tile + 28) / TILE_SIZE * TILE_SIZE - 29) << 4;
            p->vy       = 0;
            p->on_ground = 1;
            if (p->state == PSTATE_FALL || p->state == PSTATE_JUMP) {
                // Landing state: reset to idle (player.c anim will pick up)
                p->state = PSTATE_IDLE;
            }
        }
    } else if (p->vy < 0) {
        // Rising: check ceiling
        if (level_is_solid(cx, ny_tile)) {
            new_y = (((ny_tile / TILE_SIZE) + 1) * TILE_SIZE) << 4;
            p->vy = 0;
        }
    }

    p->y = new_y;

    // Update anim state for falling
    if (!p->on_ground && p->vy > 2 &&
        p->state != PSTATE_HURT &&
        p->state != PSTATE_ATTACK1 &&
        p->state != PSTATE_ATTACK2 &&
        p->state != PSTATE_SPECIAL &&
        p->state != PSTATE_WEBSWING) {
        p->state = PSTATE_FALL;
    }
}
