#ifndef MAIN_H
#define MAIN_H

#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>

// ============================================================
//  MAXIMUM CARNAGE DEMAKE - GBC
//  Engine: GBDK/C
//  Targets: Game Boy Color
// ============================================================

// --- Screen Constants ---
#define SCREEN_W        160
#define SCREEN_H        144
#define TILE_SIZE       8
#define MAP_W_TILES     32
#define MAP_H_TILES     18

// --- Game States ---
#define STATE_BOOT      0
#define STATE_TITLE     1
#define STATE_CHAR_SEL  2
#define STATE_CUTSCENE  3
#define STATE_GAMEPLAY  4
#define STATE_BOSS      5
#define STATE_GAMEOVER  6
#define STATE_CREDITS   7

// --- Campaigns ---
#define CAMPAIGN_SPIDERMAN  0
#define CAMPAIGN_VENOM      1
#define CAMPAIGN_CARNAGE    2

// --- Characters ---
#define CHAR_SPIDERMAN  0
#define CHAR_VENOM      1
#define CHAR_CARNAGE    2

// --- Player States ---
#define PSTATE_IDLE         0
#define PSTATE_WALK         1
#define PSTATE_JUMP         2
#define PSTATE_FALL         3
#define PSTATE_ATTACK1      4
#define PSTATE_ATTACK2      5
#define PSTATE_SPECIAL      6
#define PSTATE_HURT         7
#define PSTATE_DEAD         8
#define PSTATE_WEBSWING     9   // Spider-Man only (pendulum swing active)
#define PSTATE_SYMBIOTE     10  // Venom/Carnage only
#define PSTATE_BOSS_ABILITY 11  // Executing a stolen boss power

// Facing direction constants
#define FACING_RIGHT        0
#define FACING_LEFT         1

// --- Physics ---
#define GRAVITY             2
#define JUMP_FORCE          -12
#define WALK_SPEED          2
#define MAX_FALL_SPEED      8
#define WEBSWING_SPEED      4

// --- Combat ---
#define ATTACK1_DAMAGE      10
#define ATTACK2_DAMAGE      15
#define SPECIAL_DAMAGE      25
#define INVINCIBILITY_FRAMES 30

// --- Sprite OAM Slots ---
#define PLAYER_SPR_SLOT     0   // slots 0-3 (player uses 4 sprites for 16x32)
#define ENEMY_SPR_SLOT_START 4  // slots 4-19 (up to 4 enemies x 4 sprites)
#define HUD_SPR_SLOT_START  20  // slots 20-27

// --- Enemy Types ---
#define ENEMY_SHRIEK        0
#define ENEMY_DOPPELGANGER  1
#define ENEMY_DEMOGOBLIN    2
#define ENEMY_CARRION       3
#define ENEMY_SPIDERCIDE    4
#define ENEMY_BOSS_CARNAGE  5
#define ENEMY_BOSS_VENOM    6   // (Carnage campaign)
#define ENEMY_BOSS_SPIDER   7   // (Carnage campaign)
#define MAX_ENEMIES         4

// --- Level IDs ---
#define LEVEL_DOWNTOWN      0
#define LEVEL_ROOFTOPS      1
#define LEVEL_SEWERS        2
#define LEVEL_WAREHOUSE     3
#define LEVEL_RAVENCROFT    4
#define LEVEL_CARNAGE_LAIR  5
#define MAX_LEVELS          6

// --- Palette Indices (CGB) ---
#define PAL_SPIDERMAN   0
#define PAL_VENOM       1
#define PAL_CARNAGE     2
#define PAL_ENEMIES     3
#define PAL_BG_CITY     4
#define PAL_BG_DARK     5
#define PAL_HUD         6
#define PAL_EFFECTS     7

// ============================================================
//  DATA STRUCTURES
// ============================================================

typedef struct {
    int16_t x, y;           // world position (fixed point * 16)
    int8_t  vx, vy;         // velocity
    uint8_t state;
    uint8_t frame;
    uint8_t frame_timer;
    uint8_t facing;          // 0=right, 1=left
    int16_t hp;
    int16_t max_hp;
    uint8_t inv_frames;      // invincibility frames after hit
    uint8_t on_ground;
    uint8_t attack_frame;
    uint8_t special_meter;   // 0-100
    uint8_t character;       // CHAR_*
    uint8_t alive;
} Player;

typedef struct {
    int16_t x, y;
    int8_t  vx, vy;
    uint8_t type;
    uint8_t state;
    uint8_t frame;
    uint8_t frame_timer;
    uint8_t facing;
    int16_t hp;
    uint8_t alive;
    uint8_t on_ground;
    uint8_t attack_timer;
    uint8_t aggro;           // 0=patrol, 1=chase
    int16_t patrol_left;
    int16_t patrol_right;
} Enemy;

typedef struct {
    uint8_t id;
    uint8_t campaign;
    uint8_t scroll_x;        // camera scroll
    uint8_t scroll_y;
    uint8_t  enemy_count;
    Enemy   enemies[MAX_ENEMIES];
    uint8_t  boss_active;
    uint8_t  complete;
    uint8_t  checkpoint;
} Level;

typedef struct {
    uint8_t game_state;
    uint8_t campaign;
    uint8_t current_level;
    uint8_t lives;
    uint32_t score;
    uint8_t continue_count;
} GameState;

// ============================================================
//  GLOBALS (defined in main.c)
// ============================================================
extern Player      g_player;
extern Level       g_level;
extern GameState   g_game;
extern uint8_t     g_frame_count;
extern uint8_t     g_joypad;
extern uint8_t     g_joypad_prev;
extern uint8_t     g_joypad_pressed; // edge-triggered

// ============================================================
//  FUNCTION PROTOTYPES
// ============================================================

// main.c
void game_init(void);
void game_loop(void);
void game_state_update(void);

// player.c
void player_init(uint8_t character);
void player_update(void);
void player_draw(void);
void player_take_damage(uint8_t dmg);
void player_check_attack_hit(void);

// enemy.c
void enemies_init(void);
void enemies_update(void);
void enemies_draw(void);
void enemy_take_damage(uint8_t idx, uint8_t dmg);
void enemy_spawn(uint8_t idx, uint8_t type, int16_t x, int16_t y);

// level.c
void level_load(uint8_t id, uint8_t campaign);
void level_update(void);
void level_draw_bg(void);
void level_scroll_update(void);
uint8_t level_is_solid(int16_t x, int16_t y);

// hud.c
void hud_init(void);
void hud_draw(void);
void hud_update_hp(void);
void hud_update_score(void);
void hud_update_special(void);

// cutscene.c
void cutscene_play(uint8_t id);
void cutscene_update(void);
void cutscene_draw(void);

// title.c
void title_init(void);
void title_update(void);
void title_draw(void);

// char_select.c
void char_select_init(void);
void char_select_update(void);
void char_select_draw(void);

// physics.c
void physics_apply_gravity(Player *p);
void physics_move_player(Player *p);
int8_t  physics_collide_rect(int16_t ax, int16_t ay, uint8_t aw, uint8_t ah,
                              int16_t bx, int16_t by, uint8_t bw, uint8_t bh);

// palette.c
void palette_init(void);
void palette_set_character(uint8_t character);
void palette_set_level(uint8_t level_id);
void palette_flash(uint8_t color, uint8_t duration);

// music.c
void music_init(void);
void music_play(uint8_t track_id);
void music_stop(void);
void music_update(void); // call every frame

// sfx.c
void sfx_play(uint8_t sfx_id);

#endif // MAIN_H
