// ============================================================
//  MAXIMUM CARNAGE DEMAKE - GBC
//  main.c  -  Entry point, game loop, state machine
// ============================================================

#include "main.h"

// ============================================================
//  GLOBALS
// ============================================================
Player    g_player;
Level     g_level;
GameState g_game;
uint8_t   g_frame_count  = 0;
uint8_t   g_joypad       = 0;
uint8_t   g_joypad_prev  = 0;
uint8_t   g_joypad_pressed = 0;

// ============================================================
//  JOYPAD HELPER
// ============================================================
static void joypad_update(void) {
    g_joypad_prev    = g_joypad;
    g_joypad         = joypad();
    g_joypad_pressed = (~g_joypad_prev) & g_joypad; // newly pressed this frame
}

// ============================================================
//  INIT
// ============================================================
void game_init(void) {
    DISPLAY_OFF;

    // GBC mode
    cpu_fast();

    // Clear OAM
    for (uint8_t i = 0; i < 40; i++) {
        shadow_OAM[i].y = 0;
        shadow_OAM[i].x = 0;
        shadow_OAM[i].tile = 0;
        shadow_OAM[i].prop = 0;
    }

    // Default game state
    g_game.game_state    = STATE_BOOT;
    g_game.campaign      = CAMPAIGN_SPIDERMAN;
    g_game.current_level = LEVEL_DOWNTOWN;
    g_game.lives         = 3;
    g_game.score         = 0;
    g_game.continue_count = 3;

    palette_init();
    music_init();
    hud_init();

    SPRITES_8x16;   // Use 8x16 sprite mode for taller characters
    SHOW_SPRITES;
    SHOW_BKG;

    DISPLAY_ON;

    // Boot: show Carnage logo then go to title
    g_game.game_state = STATE_TITLE;
    title_init();
}

// ============================================================
//  STATE MACHINE
// ============================================================
void game_state_update(void) {
    switch (g_game.game_state) {

        // ---- TITLE SCREEN ----
        case STATE_TITLE:
            title_update();
            title_draw();
            if (g_joypad_pressed & J_START) {
                g_game.game_state = STATE_CHAR_SEL;
                char_select_init();
            }
            break;

        // ---- CHARACTER / CAMPAIGN SELECT ----
        case STATE_CHAR_SEL:
            char_select_update();
            char_select_draw();
            break;

        // ---- INTRO CUTSCENE ----
        case STATE_CUTSCENE:
            cutscene_update();
            cutscene_draw();
            break;

        // ---- MAIN GAMEPLAY ----
        case STATE_GAMEPLAY:
            player_update();
            enemies_update();
            level_update();
            level_scroll_update();
            hud_update_hp();
            hud_update_score();
            hud_update_special();
            music_update();

            // Check win condition
            if (g_level.complete) {
                g_game.current_level++;
                if (g_game.current_level >= MAX_LEVELS) {
                    g_game.game_state = STATE_CREDITS;
                } else {
                    // Cutscene between levels
                    g_game.game_state = STATE_CUTSCENE;
                    cutscene_play(g_game.current_level);
                }
            }

            // Check player death
            if (!g_player.alive) {
                g_game.lives--;
                if (g_game.lives == 0) {
                    if (g_game.continue_count > 0) {
                        g_game.continue_count--;
                        g_game.game_state = STATE_GAMEOVER;
                    } else {
                        // Hard game over - back to title
                        g_game.game_state = STATE_TITLE;
                        g_game.lives = 3;
                        g_game.score = 0;
                        g_game.current_level = 0;
                        title_init();
                    }
                } else {
                    // Respawn at checkpoint
                    player_init(g_game.campaign);
                    level_load(g_game.current_level, g_game.campaign);
                }
            }

            // Draw
            level_draw_bg();
            enemies_draw();
            player_draw();
            hud_draw();
            break;

        // ---- BOSS FIGHT ----
        case STATE_BOSS:
            player_update();
            enemies_update();
            hud_update_hp();
            music_update();

            if (!g_player.alive) {
                g_game.lives--;
                if (g_game.lives == 0) {
                    g_game.game_state = STATE_GAMEOVER;
                } else {
                    player_init(g_game.campaign);
                }
            }

            // Boss dead?
            if (g_level.enemies[0].hp <= 0) {
                g_level.complete = 1;
                g_game.game_state = STATE_GAMEPLAY;
            }

            level_draw_bg();
            enemies_draw();
            player_draw();
            hud_draw();
            break;

        // ---- GAME OVER ----
        case STATE_GAMEOVER:
            // Simple: press Start to continue or A to quit
            if (g_joypad_pressed & J_START) {
                g_game.lives = 3;
                g_game.game_state = STATE_GAMEPLAY;
                player_init(g_game.campaign);
                level_load(g_game.current_level, g_game.campaign);
            }
            if (g_joypad_pressed & J_SELECT) {
                g_game.game_state = STATE_TITLE;
                g_game.lives = 3;
                g_game.score = 0;
                g_game.current_level = 0;
                title_init();
            }
            break;

        // ---- CREDITS ----
        case STATE_CREDITS:
            // Scroll credits text; any button returns to title
            if (g_joypad_pressed & (J_START | J_A)) {
                g_game.game_state = STATE_TITLE;
                g_game.lives = 3;
                g_game.score = 0;
                g_game.current_level = 0;
                title_init();
            }
            break;
    }
}

// ============================================================
//  MAIN GAME LOOP
// ============================================================
void game_loop(void) {
    while (1) {
        wait_vbl_done();        // sync to 60fps VBlank
        joypad_update();
        g_frame_count++;
        game_state_update();
    }
}

// ============================================================
//  ENTRY POINT
// ============================================================
void main(void) {
    game_init();
    game_loop();
}
