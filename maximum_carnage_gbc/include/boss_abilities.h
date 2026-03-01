#pragma once
// ============================================================
//  boss_abilities.h  -  Maximum Carnage GBC Boss Power System
// ============================================================

#include "main.h"

// ── Ability IDs ───────────────────────────────────────────
#define ABILITY_NONE                0
#define ABILITY_CARNAGE_TENDRILS    1   // Defeat boss Carnage
#define ABILITY_SHRIEK_SCREAM       2   // Defeat Shriek
#define ABILITY_DEMOGOBLIN_BOMB     3   // Defeat Demogoblin
#define ABILITY_DOPPELGANGER_CLONE  4   // Defeat Doppelganger
#define BOSS_ABILITY_COUNT          5

#define MAX_ABILITY_SLOTS           4   // primary + up to 3 boss powers
#define SELECT_COOLDOWN_FRAMES      12  // debounce for SELECT cycling

// ── Animation IDs (for player.c anim table lookup) ────────
#define ANIM_PRIMARY        0
#define ANIM_CARNAGE_TENDRIL 26
#define ANIM_SHRIEK_SCREAM  28
#define ANIM_DEMO_BOMB      30
#define ANIM_CLONE_SPAWN    32

// ── HUD icon tile indices ─────────────────────────────────
#define HUD_ICON_SPECIAL    0xB0
#define HUD_ICON_TENDRIL    0xB2
#define HUD_ICON_SCREAM     0xB4
#define HUD_ICON_BOMB       0xB6
#define HUD_ICON_CLONE      0xB8

// ── Projectile types ──────────────────────────────────────
#define PROJ_PUMPKIN_BOMB   1

// ── Bomb physics ─────────────────────────────────────────
#define BOMB_VX             4
#define BOMB_VY_INITIAL    -5    // upward arc
#define BOMB_GRAVITY        1    // +1 vy/frame
#define BOMB_BLAST_RADIUS   28   // pixels
#define BOMB_MAX_FLIGHT     90   // frames before forced explode
#define GROUND_BOMB_Y       120  // y in screen pixels

// ── Clone constants ───────────────────────────────────────
#define CLONE_HP            60
#define CLONE_DURATION      300  // 5 seconds at 60fps
#define CLONE_SPEED         2
#define CLONE_DAMAGE        12
#define CLONE_ATK_COOLDOWN  30

// ── New player state ──────────────────────────────────────
#ifndef PSTATE_BOSS_ABILITY
#define PSTATE_BOSS_ABILITY  11
#endif

// ── Facing directions ─────────────────────────────────────
#ifndef FACING_RIGHT
#define FACING_RIGHT 0
#define FACING_LEFT  1
#endif

// ── Ability definition ────────────────────────────────────
typedef struct {
    uint8_t      id;
    const char  *name;
    uint8_t      damage;
    uint8_t      meter_cost;
    uint8_t      is_projectile;
    uint8_t      is_aoe;
    uint8_t      stun_frames;
    uint8_t      anim_id;
    const char  *description;
} BossAbilityDef;

// ── Manager struct ────────────────────────────────────────
typedef struct {
    uint8_t slots[MAX_ABILITY_SLOTS];  // ability IDs in each slot
    uint8_t slot_count;                // how many slots unlocked
    uint8_t active_slot;               // currently selected slot
    uint8_t select_cooldown;
} BossAbilityManager;

// ── Projectile struct ─────────────────────────────────────
typedef struct {
    uint8_t  active;
    int16_t  x, y;         // world pos (fixed-point *16)
    int8_t   vx, vy;
    uint8_t  type;
    uint8_t  damage;
    uint8_t  radius;
    uint8_t  timer;
} BossProjectile;

// ── Clone struct ──────────────────────────────────────────
typedef struct {
    uint8_t  active;
    int16_t  x, y;
    uint8_t  facing;
    uint8_t  hp;
    uint16_t timer;
    uint8_t  target_idx;
    uint8_t  atk_timer;
    uint8_t  frame;
} CloneState;

extern BossAbilityManager g_abilities;

// ── API ───────────────────────────────────────────────────
void                  boss_abilities_init       (void);
void                  boss_ability_unlock       (uint8_t boss_type);
void                  boss_ability_cycle        (void);
uint8_t               boss_ability_active_id    (void);
const BossAbilityDef *boss_ability_get_def      (uint8_t ability_id);
uint8_t               boss_ability_fire         (PlayerState *p, EnemyState *enemies, uint8_t count);
void                  boss_abilities_update     (EnemyState *enemies, uint8_t count);
uint8_t               boss_ability_hud_icon     (void);
uint8_t               boss_ability_show_notify  (void);
uint8_t               boss_ability_notify_timer (void);
const char           *boss_ability_name         (void);
uint8_t               boss_ability_slot_count   (void);
uint8_t               boss_ability_active_slot  (void);
BossProjectile       *boss_ability_get_projectile(void);
CloneState           *boss_ability_get_clone    (void);
