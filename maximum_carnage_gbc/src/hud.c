// ============================================================
//  MAXIMUM CARNAGE DEMAKE - GBC
//  hud.c  -  On-screen display: HP bars, score, special meter
// ============================================================

#include "main.h"
#include "boss_abilities.h"

// HUD tile indices (in BG layer row 0, which is always on-screen)
#define HUD_TILE_BLANK      200
#define HUD_TILE_HP_FULL    201
#define HUD_TILE_HP_HALF    202
#define HUD_TILE_HP_EMPTY   203
#define HUD_TILE_SP_FULL    204
#define HUD_TILE_SP_EMPTY   205
#define HUD_TILE_BOSS_HP    206
#define HUD_TILE_DIGIT_0    210  // digits 210-219

#define HUD_HP_TILES        10   // tiles wide for HP bar
#define HUD_SP_TILES        5    // tiles wide for special bar

// HUD lives icons use OAM sprites
#define HUD_LIVES_X_START   128
#define HUD_LIVES_Y         150
#define HUD_LIVES_TILE      240

// Score display (tile-based digits, top-center)
static uint8_t score_buf[8] = {0,0,0,0,0,0,0,0};

// ============================================================
//  INIT
// ============================================================
void hud_init(void) {
    // Draw static HUD labels to BG at row 0
    // In real GBDK: set_bkg_tiles(0, 0, 20, 1, hud_static_tiles);
    // For now this stubs the setup

    // Clear lives OAM slots
    for (uint8_t i = 0; i < 4; i++) {
        shadow_OAM[HUD_SPR_SLOT_START + i].y = 0;
    }
}

// ============================================================
//  HP BAR
// ============================================================
void hud_draw_bar(uint8_t bg_x, uint8_t bg_y,
                  int16_t current, int16_t max_hp,
                  uint8_t full_tile, uint8_t half_tile, uint8_t empty_tile,
                  uint8_t width) {
    // Compute how many tiles are full/half/empty
    uint8_t filled = (uint8_t)((current * width * 2) / max_hp);
    uint8_t full   = filled >> 1;
    uint8_t has_half = filled & 1;

    uint8_t tiles[HUD_HP_TILES];
    for (uint8_t i = 0; i < width; i++) {
        if (i < full)                tiles[i] = full_tile;
        else if (i == full && has_half) tiles[i] = half_tile;
        else                         tiles[i] = empty_tile;
    }
    // set_bkg_tiles(bg_x, bg_y, width, 1, tiles);  // real GBDK
    (void)tiles; (void)bg_x; (void)bg_y; // stub
}

void hud_update_hp(void) {
    // Player HP bar: top-left
    hud_draw_bar(1, 0,
                 g_player.hp, g_player.max_hp,
                 HUD_TILE_HP_FULL, HUD_TILE_HP_HALF, HUD_TILE_HP_EMPTY,
                 HUD_HP_TILES);

    // Boss HP bar: top-right (if boss active)
    if (g_level.boss_active) {
        for (uint8_t i = 0; i < MAX_ENEMIES; i++) {
            Enemy *e = &g_level.enemies[i];
            if (e->alive && e->type >= ENEMY_BOSS_CARNAGE) {
                hud_draw_bar(11, 0,
                             e->hp, ENEMY_MAX_HP[e->type], // use first boss
                             HUD_TILE_HP_FULL, HUD_TILE_HP_HALF, HUD_TILE_HP_EMPTY,
                             HUD_HP_TILES);
                break;
            }
        }
    }
}

// ============================================================
//  SPECIAL METER
// ============================================================
void hud_update_special(void) {
    uint8_t filled = (uint8_t)((g_player.special_meter * HUD_SP_TILES) / 100);
    uint8_t tiles[HUD_SP_TILES];
    for (uint8_t i = 0; i < HUD_SP_TILES; i++) {
        tiles[i] = (i < filled) ? HUD_TILE_SP_FULL : HUD_TILE_SP_EMPTY;
    }
    // set_bkg_tiles(1, 1, HUD_SP_TILES, 1, tiles); // real GBDK
    (void)tiles;
}

// ============================================================
//  SCORE  (8-digit display, top-center of screen)
// ============================================================
void hud_update_score(void) {
    uint32_t s = g_game.score;
    for (int8_t i = 7; i >= 0; i--) {
        score_buf[i] = HUD_TILE_DIGIT_0 + (uint8_t)(s % 10);
        s /= 10;
    }
    // set_bkg_tiles(6, 0, 8, 1, score_buf);  // real GBDK
}

// ============================================================
//  LIVES  (sprite icons, bottom-right)
// ============================================================
static void hud_draw_lives(void) {
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t slot = HUD_SPR_SLOT_START + i;
        if (i < g_game.lives) {
            shadow_OAM[slot].y    = HUD_LIVES_Y;
            shadow_OAM[slot].x    = HUD_LIVES_X_START + (i * 10);
            shadow_OAM[slot].tile = HUD_LIVES_TILE + g_game.campaign * 2;
            shadow_OAM[slot].prop = g_game.campaign; // palette
        } else {
            shadow_OAM[slot].y = 0; // hide
        }
    }
}

// ============================================================
//  ABILITY SLOT INDICATOR
//  Bottom-left corner: shows active ability icon + slot dots
//  Layout: [ICON] [● ● ○ ○]   (filled = unlocked, lit = active)
// ============================================================
static void hud_draw_ability_bar(void) {
    // Only draw if player is Spider-Man (primary web-swing user)
    // or if any boss ability is unlocked
    if (boss_ability_slot_count() <= 1) return;

    uint8_t active = boss_ability_active_slot();
    uint8_t count  = boss_ability_slot_count();

    // Draw active ability icon (OAM sprite, bottom-left)
    uint8_t icon_tile = boss_ability_hud_icon();
    uint8_t slot = HUD_SPR_SLOT_START + 4;   // slot 24 (after lives slots)
    shadow_OAM[slot].y    = 140;
    shadow_OAM[slot].x    = 12;
    shadow_OAM[slot].tile = icon_tile;
    shadow_OAM[slot].prop = 0;

    // Draw slot pip indicators (OAM, beside icon)
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t pip_slot = HUD_SPR_SLOT_START + 5 + i;
        if (i < count) {
            shadow_OAM[pip_slot].y    = 144;
            shadow_OAM[pip_slot].x    = 24 + i * 6;
            shadow_OAM[pip_slot].tile = (i == active) ? 0xFA : 0xF8;  // lit/dim pip tile
            shadow_OAM[pip_slot].prop = 0;
        } else {
            shadow_OAM[pip_slot].y = 0;  // hide
        }
    }
}

// ============================================================
//  ABILITY UNLOCK NOTIFICATION  (new power acquired banner)
//  Flashes the ability name for ~2 seconds center-bottom
// ============================================================
static void hud_draw_ability_notify(void) {
    if (!boss_ability_show_notify()) return;

    // Flash effect: visible only when timer is even
    uint8_t t = boss_ability_notify_timer();
    if (t & 4) return;   // flicker every 4 frames

    // In real GBDK, print text to BG window layer row 17:
    //   const char *name = boss_ability_name();
    //   set_win_tiles(2, 0, strlen(name), 1, name_tile_indices);
    //   move_win(7, 136);   // show window at bottom
    // Stubbed here:
    (void)t;
}

// ============================================================
//  DRAW ALL HUD
// ============================================================
void hud_draw(void) {
    hud_draw_lives();
    hud_draw_ability_bar();
    hud_draw_ability_notify();
    // BG-based elements are updated via hud_update_* calls in main loop
}
