// ============================================================
//  MAXIMUM CARNAGE DEMAKE - GBC
//  title.c  -  Title screen + Campaign/Character select
// ============================================================

#include "main.h"

// ============================================================
//  TITLE SCREEN
// ============================================================
static uint8_t title_frame = 0;

void title_init(void) {
    title_frame = 0;
    music_play(30); // title theme
    palette_set_level(0);
    // set_bkg_tiles(0, 0, 20, 18, title_screen_tiles); // real GBDK
}

void title_update(void) {
    title_frame++;
    // Animate the title logo (pulse/flicker)
    // set_bkg_tile(9, 14, (title_frame & 0x10) ? TILE_PRESS_START_A : TILE_PRESS_START_B);
}

void title_draw(void) {
    // BG is already set in title_init
    // Animate "PRESS START" text blinking
}

// ============================================================
//  CHARACTER / CAMPAIGN SELECT
// ============================================================
#define SEL_SPIDER   0
#define SEL_VENOM    1
#define SEL_CARNAGE  2

static uint8_t sel_cursor     = 0;
static uint8_t sel_anim_timer = 0;
static uint8_t sel_confirmed  = 0;

// Character preview positions on screen
static const uint8_t PREVIEW_X[3] = {32, 80, 128};
static const uint8_t PREVIEW_Y[3] = {64, 64, 64};

// Character names for display
static const char * const CHAR_NAMES[] = {
    "SPIDER-MAN",
    "VENOM",
    "CARNAGE"
};

// Palette per character for preview highlight
static const uint8_t CHAR_PREVIEW_TILES[] = {
    0,  // Spider-Man preview tile base
    40, // Venom
    80  // Carnage
};

void char_select_init(void) {
    sel_cursor    = 0;
    sel_anim_timer = 0;
    sel_confirmed = 0;
    music_play(31); // character select music (shorter loop)
    // Load character select BG art
    // set_bkg_tiles(0, 0, 20, 18, char_select_bg_tiles);
}

void char_select_update(void) {
    sel_anim_timer++;

    if (sel_confirmed) {
        // Brief flash then go to intro cutscene
        if (sel_anim_timer > 30) {
            g_game.campaign      = sel_cursor;
            g_game.current_level = LEVEL_DOWNTOWN;
            g_game.lives         = 3;
            g_game.score         = 0;

            // Init player with selected character
            player_init(sel_cursor);

            // Play campaign intro cutscene
            uint8_t cutscene_id;
            switch (sel_cursor) {
                case CAMPAIGN_SPIDERMAN: cutscene_id = CS_SM_INTRO; break;
                case CAMPAIGN_VENOM:     cutscene_id = CS_VM_INTRO; break;
                case CAMPAIGN_CARNAGE:   cutscene_id = CS_CM_INTRO; break;
                default:                 cutscene_id = 0; break;
            }
            g_game.game_state = STATE_CUTSCENE;
            cutscene_play(cutscene_id);
        }
        return;
    }

    // Navigate
    if (g_joypad_pressed & J_RIGHT) {
        sel_cursor = (sel_cursor + 1) % 3;
        sel_anim_timer = 0;
        sfx_play(13); // cursor move sfx
        palette_set_character(sel_cursor);
    }
    if (g_joypad_pressed & J_LEFT) {
        sel_cursor = (sel_cursor == 0) ? 2 : sel_cursor - 1;
        sel_anim_timer = 0;
        sfx_play(13);
        palette_set_character(sel_cursor);
    }

    // Confirm
    if (g_joypad_pressed & (J_A | J_START)) {
        sel_confirmed = 1;
        sel_anim_timer = 0;
        sfx_play(14); // confirm sfx
    }

    // B = back to title
    if (g_joypad_pressed & J_B) {
        g_game.game_state = STATE_TITLE;
        title_init();
    }
}

void char_select_draw(void) {
    // Draw cursor/highlight under selected character
    // In real GBDK: update BG tiles or move a sprite arrow

    // Animate selected character preview sprite (idle loop)
    uint8_t anim_tile = CHAR_PREVIEW_TILES[sel_cursor] +
                        ((sel_anim_timer >> 3) & 1) * 2;

    for (uint8_t i = 0; i < 3; i++) {
        uint8_t slot = HUD_SPR_SLOT_START + i * 2;
        uint8_t tile = CHAR_PREVIEW_TILES[i];
        uint8_t pal  = i;

        // Dim unselected characters
        // (in real GBC: swap palette to greyed-out version)

        // Preview top sprite
        shadow_OAM[slot].y    = PREVIEW_Y[i];
        shadow_OAM[slot].x    = PREVIEW_X[i];
        shadow_OAM[slot].tile = (i == sel_cursor) ? anim_tile : tile;
        shadow_OAM[slot].prop = pal;

        // Preview bottom sprite
        shadow_OAM[slot + 1].y    = PREVIEW_Y[i] + 16;
        shadow_OAM[slot + 1].x    = PREVIEW_X[i];
        shadow_OAM[slot + 1].tile = (i == sel_cursor) ? anim_tile + 4 : tile + 4;
        shadow_OAM[slot + 1].prop = pal;
    }

    // Flash selected character when confirmed
    if (sel_confirmed && (sel_anim_timer & 0x04)) {
        // Blank out sprites briefly (flash effect)
        uint8_t slot = HUD_SPR_SLOT_START + sel_cursor * 2;
        shadow_OAM[slot].y = 0;
        shadow_OAM[slot + 1].y = 0;
    }
}

// Cutscene ID constants (match cutscene.c)
#define CS_SM_INTRO  0
#define CS_VM_INTRO  6
#define CS_CM_INTRO  12
