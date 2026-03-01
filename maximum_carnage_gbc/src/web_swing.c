// ============================================================
//  MAXIMUM CARNAGE DEMAKE - GBC
//  web_swing.c  -  Arcade-feel web-swing: works anywhere on
//                  the map (no anchor point needed). Pure
//                  pendulum physics with momentum conservation.
//
//  Spider-Man only.  Hold A while airborne to swing.
//  Release A to launch forward with speed boost.
//
//  Fixed-point: positions are world pixels * 16 (4 frac bits)
//  Angles are stored as degrees * 4 (0..1439) for cheap trig
// ============================================================

#include "main.h"
#include "web_swing.h"

// ────────────────────────────────────────────────────────────
//  SINE TABLE  (degrees 0-89, scaled *128)
// ────────────────────────────────────────────────────────────
static const int8_t SIN_TABLE[90] = {
      0,  2,  4,  7,  9, 11, 13, 15, 17, 20,
     22, 24, 26, 28, 30, 32, 34, 36, 38, 40,
     42, 44, 46, 48, 49, 51, 53, 55, 56, 58,
     60, 61, 63, 64, 66, 67, 69, 70, 71, 73,
     74, 75, 77, 78, 79, 80, 81, 82, 83, 84,
     85, 86, 87, 88, 88, 89, 90, 91, 91, 92,
     93, 93, 94, 94, 95, 95, 96, 96, 96, 97,
     97, 97, 98, 98, 98, 99, 99, 99, 99, 99,
    100,100,100,100,100,100,100,100,100,100
};

// Fast approx sin/cos (*128) for angle 0..359
static int16_t fast_sin(uint16_t deg) {
    deg = deg % 360;
    if (deg <  90) return  (int16_t)SIN_TABLE[deg];
    if (deg < 180) return  (int16_t)SIN_TABLE[179-deg];
    if (deg < 270) return -(int16_t)SIN_TABLE[deg-180];
                   return -(int16_t)SIN_TABLE[359-deg];
}
static int16_t fast_cos(uint16_t deg) {
    return fast_sin((deg + 90) % 360);
}

// ────────────────────────────────────────────────────────────
//  WEB SWING STATE
// ────────────────────────────────────────────────────────────

WebSwingState g_swing;

// Web line sprite (single OAM slot, animated dotted line)
static uint8_t web_line_timer = 0;

// ────────────────────────────────────────────────────────────
//  INIT / RESET
// ────────────────────────────────────────────────────────────
void web_swing_init(void) {
    g_swing.active        = 0;
    g_swing.angle_deg     = 270;   // pointing straight up
    g_swing.angle_vel     = 0;
    g_swing.rope_len      = WEB_ROPE_LEN_DEFAULT;
    g_swing.anchor_x      = 0;
    g_swing.anchor_y      = 0;
    g_swing.launch_vx     = 0;
    g_swing.launch_vy     = 0;
    g_swing.hold_frames   = 0;
    web_line_timer        = 0;
}

// ────────────────────────────────────────────────────────────
//  ATTACH  -  called when player presses A while airborne
//             Anchor point is directly above player
// ────────────────────────────────────────────────────────────
void web_swing_attach(PlayerState *p) {
    if (g_swing.active) return;

    // Anchor is directly above at rope length
    // (arcade feel: no surface detection needed)
    g_swing.anchor_x  = p->x;
    g_swing.anchor_y  = p->y - (WEB_ROPE_LEN_DEFAULT << 4);  // fixed point

    // Initial angle: player is directly below anchor = 270 deg
    g_swing.angle_deg = 270;

    // Convert player's current velocity to angular velocity
    // ang_vel = vx / rope_len  (scaled)
    int16_t rope_px = WEB_ROPE_LEN_DEFAULT;
    if (rope_px < 1) rope_px = 1;
    g_swing.angle_vel = (p->vx * 8) / rope_px;

    g_swing.rope_len  = WEB_ROPE_LEN_DEFAULT;
    g_swing.active    = 1;
    g_swing.hold_frames = 0;

    p->state = PSTATE_WEBSWING;
}

// ────────────────────────────────────────────────────────────
//  DETACH  -  called when player releases A
//             Converts angular velocity back to linear velocity
// ────────────────────────────────────────────────────────────
void web_swing_detach(PlayerState *p) {
    if (!g_swing.active) return;

    // Launch velocity: tangential direction from current angle
    // v_tangential = omega * r
    // vx = -sin(angle) * |vel|
    // vy = -cos(angle) * |vel|  (negative = upward at arc peak)
    int16_t speed = g_swing.angle_vel * g_swing.rope_len / 16;
    if (speed < -WEB_MAX_LAUNCH_SPEED) speed = -WEB_MAX_LAUNCH_SPEED;
    if (speed >  WEB_MAX_LAUNCH_SPEED) speed =  WEB_MAX_LAUNCH_SPEED;

    // tangent to circle at current angle:
    // vx = omega * r * cos(angle)
    // vy = omega * r * sin(angle)  -- Y is flipped (screen coords)
    uint16_t ang = (uint16_t)g_swing.angle_deg;
    int16_t cx = fast_cos(ang);  // *128
    int16_t sy = fast_sin(ang);  // *128

    p->vx = (int8_t)((speed * cx) / 128);
    p->vy = (int8_t)(-(speed * sy) / 128);   // upward boost on release

    // Clamp launch velocity
    if (p->vx < -WEB_MAX_LAUNCH_SPEED) p->vx = -WEB_MAX_LAUNCH_SPEED;
    if (p->vx >  WEB_MAX_LAUNCH_SPEED) p->vx =  WEB_MAX_LAUNCH_SPEED;
    if (p->vy < -WEB_MAX_LAUNCH_VY)    p->vy = -WEB_MAX_LAUNCH_VY;
    if (p->vy >  MAX_FALL_SPEED)        p->vy =  MAX_FALL_SPEED;

    g_swing.active  = 0;
    p->state        = (p->vy < 0) ? PSTATE_JUMP : PSTATE_FALL;
}

// ────────────────────────────────────────────────────────────
//  UPDATE  -  call once per frame while swing is active
//
//  Pendulum equation (Euler integration):
//    alpha = -(g / L) * sin(theta)        (angular accel)
//    omega += alpha * dt
//    theta += omega * dt
//
//  All angles in degrees*4 stored as int16_t for precision.
//  g_scaled = GRAVITY constant (2) * 4 = 8
// ────────────────────────────────────────────────────────────
void web_swing_update(PlayerState *p, uint8_t a_held) {
    if (!g_swing.active) return;

    g_swing.hold_frames++;

    // Auto-release after max hold time
    if (g_swing.hold_frames > WEB_MAX_HOLD_FRAMES) {
        web_swing_detach(p);
        return;
    }

    // ── Pendulum physics ─────────────────────────────────────
    // angle_deg: 270 = directly below, 0/360 = right of anchor
    // We work relative to "hanging down" = 270 deg
    int16_t theta = (int16_t)g_swing.angle_deg - 270;  // offset from rest

    // alpha = -(g_scaled * sin(theta)) / rope_len
    int16_t s = fast_sin((theta < 0 ? (uint16_t)(theta + 360) : (uint16_t)theta) % 360);
    if (theta < 0 && theta > -180) s = -s;  // preserve sign
    int16_t alpha = -(WEB_GRAVITY_SCALED * s) / (int16_t)(g_swing.rope_len / 4 + 1);

    // Player can pump the swing: hold left/right adds angular impulse
    if (a_held) {
        if (p->vx > 0) g_swing.angle_vel += WEB_PUMP_FORCE;
        if (p->vx < 0) g_swing.angle_vel -= WEB_PUMP_FORCE;
    }

    // Damping (air resistance) - tiny, keeps arcade feel
    g_swing.angle_vel += alpha;
    g_swing.angle_vel = (g_swing.angle_vel * WEB_DAMPING_NUM) / WEB_DAMPING_DEN;

    // Clamp angular velocity
    if (g_swing.angle_vel < -WEB_MAX_ANG_VEL) g_swing.angle_vel = -WEB_MAX_ANG_VEL;
    if (g_swing.angle_vel >  WEB_MAX_ANG_VEL) g_swing.angle_vel =  WEB_MAX_ANG_VEL;

    // Integrate angle
    int16_t new_angle = (int16_t)g_swing.angle_deg + g_swing.angle_vel;

    // Clamp swing arc: don't allow full 360 (arcade feel: 180 deg max arc)
    // Range: 90 deg to 270+90=360 i.e. 90..450 normalized to 90..360
    if (new_angle < WEB_MIN_ANGLE) {
        new_angle     = WEB_MIN_ANGLE;
        g_swing.angle_vel = -g_swing.angle_vel / 2;  // bounce
    }
    if (new_angle > WEB_MAX_ANGLE) {
        new_angle     = WEB_MAX_ANGLE;
        g_swing.angle_vel = -g_swing.angle_vel / 2;
    }
    g_swing.angle_deg = (uint16_t)((new_angle + 360) % 360);

    // ── Update player position from anchor + rope ─────────────
    uint16_t ang   = g_swing.angle_deg;
    int16_t  cs    = fast_cos(ang);   // *128
    int16_t  sn    = fast_sin(ang);   // *128
    int16_t  rope  = (int16_t)g_swing.rope_len;

    // player_x = anchor_x + cos(angle) * rope_len
    // player_y = anchor_y + sin(angle) * rope_len   (Y down)
    p->x  = g_swing.anchor_x + (cs * rope) / 128;
    p->y  = g_swing.anchor_y + (sn * rope) / 128;

    // Store velocities for momentum transfer on release
    // vx = tangential x component
    p->vx = (int8_t)((-(fast_sin(ang)) * g_swing.angle_vel * rope) / (128 * 16));
    p->vy = (int8_t)(((fast_cos(ang))  * g_swing.angle_vel * rope) / (128 * 16));

    // Select swing animation frame based on angle
    // angle 90-180 = swinging right, 180-270 = swinging left
    if (ang > 180 && ang <= 360) {
        p->frame = 34;  // SPIDEY_SWING_R
    } else {
        p->frame = 36;  // SPIDEY_SWING_L
    }

    // Floor collision: if player hits ground while swinging, land
    if (p->y >= (int16_t)(GROUND_Y << 4)) {
        p->y = (GROUND_Y << 4);
        web_swing_detach(p);
        p->vy = 0;
        p->state = PSTATE_IDLE;
    }

    web_line_timer = (web_line_timer + 1) & 7;
}

// ────────────────────────────────────────────────────────────
//  DRAW WEB LINE  (dotted OAM line from player to anchor)
//  Uses 2 OAM slots in the HUD range
// ────────────────────────────────────────────────────────────
void web_swing_draw(PlayerState *p, int16_t cam_x) {
    if (!g_swing.active) return;

    // Compute screen positions
    int16_t px_scr = (p->x >> 4) - cam_x;
    int16_t py_scr = (p->y >> 4);
    int16_t ax_scr = (g_swing.anchor_x >> 4) - cam_x;
    int16_t ay_scr = (g_swing.anchor_y >> 4);

    // Draw 3 evenly-spaced web dots along the rope
    // (cheap: no actual line drawing, just interpolated OAM sprites)
    for (uint8_t i = 1; i <= 3; i++) {
        int16_t wx = px_scr + ((ax_scr - px_scr) * i) / 4;
        int16_t wy = py_scr + ((ay_scr - py_scr) * i) / 4;

        // WEB_DOT_TILE: a small 2x2 white dot tile
        // Slot 28+i-1 (OAM slots 28-30, beyond HUD range)
        uint8_t slot = 28 + (i - 1);
        if (wx >= -4 && wx <= 164 && wy >= -4 && wy <= 148) {
            // Flicker animation: skip every other frame for dash effect
            if (!((web_line_timer + i) & 1)) {
                // set_sprite_tile(slot, WEB_DOT_TILE);
                // move_sprite(slot, wx + 8, wy + 16);
                // In real GBDK:
                // OAM_ADDR[slot] = { y+16, x+8, WEB_DOT_TILE, 0 }
                (void)slot; // placeholder until GBDK OAM API called
            }
        }
    }
}

// ────────────────────────────────────────────────────────────
//  QUERY HELPERS
// ────────────────────────────────────────────────────────────
uint8_t web_swing_is_active(void) {
    return g_swing.active;
}

// Returns 1 if player is at the peak of the swing arc (good time to detach for max height)
uint8_t web_swing_at_peak(void) {
    if (!g_swing.active) return 0;
    // Peak: angle near 90 or 270 (horizontal), angular velocity near 0
    uint16_t ang = g_swing.angle_deg;
    int8_t  near_horiz = (ang > 80 && ang < 100) || (ang > 260 && ang < 280);
    int8_t  vel_low    = (g_swing.angle_vel > -2 && g_swing.angle_vel < 2);
    return near_horiz && vel_low;
}
