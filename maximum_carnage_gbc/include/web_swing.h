#pragma once
// ============================================================
//  web_swing.h  -  Maximum Carnage GBC Web-Swing System
// ============================================================

#include "main.h"

// ── Tuning constants ──────────────────────────────────────
#define WEB_ROPE_LEN_DEFAULT    48      // rope length in world pixels
#define WEB_MAX_HOLD_FRAMES     90      // ~1.5 seconds at 60fps max swing
#define WEB_GRAVITY_SCALED      8       // pendulum gravity (GRAVITY * 4)
#define WEB_PUMP_FORCE          1       // angular impulse when player steers
#define WEB_DAMPING_NUM         31      // damping factor numerator   (31/32)
#define WEB_DAMPING_DEN         32      // damping factor denominator
#define WEB_MAX_ANG_VEL         12      // max angular velocity (deg/frame)
#define WEB_MAX_LAUNCH_SPEED    8       // max px/frame on release
#define WEB_MAX_LAUNCH_VY       10      // max upward velocity on release
#define WEB_MIN_ANGLE           90      // arc min: horizontal left
#define WEB_MAX_ANGLE           270     // arc max: horizontal right

// Player ground Y reference (screen pixels, set per level)
#define GROUND_Y                128

// OAM tile index for web dot sprites
#define WEB_DOT_TILE            0xF0

// ── State struct ──────────────────────────────────────────
typedef struct {
    uint8_t  active;
    uint16_t angle_deg;     // current pendulum angle in degrees
    int8_t   angle_vel;     // angular velocity (deg/frame, signed)
    uint8_t  rope_len;      // current rope length in world pixels
    int16_t  anchor_x;      // anchor world position (fixed-point *16)
    int16_t  anchor_y;
    int8_t   launch_vx;     // velocity at moment of release
    int8_t   launch_vy;
    uint8_t  hold_frames;   // frames held so far this swing
} WebSwingState;

extern WebSwingState g_swing;

// ── API ───────────────────────────────────────────────────
void    web_swing_init   (void);
void    web_swing_attach (PlayerState *p);
void    web_swing_detach (PlayerState *p);
void    web_swing_update (PlayerState *p, uint8_t a_held);
void    web_swing_draw   (PlayerState *p, int16_t cam_x);
uint8_t web_swing_is_active(void);
uint8_t web_swing_at_peak(void);
