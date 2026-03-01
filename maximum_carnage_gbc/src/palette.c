// ============================================================
//  MAXIMUM CARNAGE DEMAKE - GBC
//  palette.c  -  GBC color palettes for all assets
//  GBC format: 5-bit RGB (0x0000-0x7FFF), little-endian pairs
// ============================================================

#include "main.h"

// ============================================================
//  COLOR MACRO  (5-bit R,G,B -> GBC word)
// ============================================================
#define RGB(r,g,b) ((uint16_t)((b)<<10|(g)<<5|(r)))

// ============================================================
//  SPRITE PALETTES  (OBP - 4 colors each, color 0 = transparent)
// ============================================================

// OBP0 - Spider-Man (red/blue/black/white)
static const uint16_t PAL_SPR_SPIDERMAN[4] = {
    RGB(31, 31, 31),  // 0: transparent (white, unused)
    RGB(25,  2,  2),  // 1: dark red
    RGB(31, 10, 10),  // 2: red
    RGB( 2,  4, 20),  // 3: blue
};

// OBP1 - Venom (black/white/grey/dark)
static const uint16_t PAL_SPR_VENOM[4] = {
    RGB(31, 31, 31),  // 0: transparent
    RGB( 2,  2,  2),  // 1: near-black
    RGB(10, 10, 10),  // 2: dark grey
    RGB(31, 31, 31),  // 3: white (teeth/eyes)
};

// OBP2 - Carnage (red/black/dark red)
static const uint16_t PAL_SPR_CARNAGE[4] = {
    RGB(31, 31, 31),  // 0: transparent
    RGB( 8,  0,  0),  // 1: very dark red
    RGB(20,  0,  0),  // 2: deep red
    RGB(31,  5,  5),  // 3: bright red
};

// OBP3 - Enemies (shared palette, varied by type in code)
static const uint16_t PAL_SPR_ENEMIES[4] = {
    RGB(31, 31, 31),  // 0: transparent
    RGB( 5,  0, 10),  // 1: dark purple (Shriek, Carrion)
    RGB(15,  5,  0),  // 2: dark orange (Demogoblin)
    RGB(20,  0, 20),  // 3: purple (Doppelganger)
};

// OBP4 - HUD / Effects
static const uint16_t PAL_SPR_HUD[4] = {
    RGB(31, 31, 31),  // 0: transparent
    RGB(31, 20,  0),  // 1: gold (score)
    RGB(31,  0,  0),  // 2: red (HP)
    RGB( 0, 20, 31),  // 3: blue (special)
};

// Carnage rage tint (replaces player palette temporarily)
static const uint16_t PAL_SPR_RAGE[4] = {
    RGB(31, 31, 31),
    RGB(31,  0,  0),
    RGB(31, 10,  0),
    RGB(25,  0,  0),
};

// ============================================================
//  BG PALETTES  (BGP - 4 colors each)
// ============================================================

// BGP0 - City/Downtown (daytime)
static const uint16_t PAL_BG_CITY_DAY[4] = {
    RGB(28, 28, 31),  // sky blue
    RGB(18, 18, 22),  // building grey
    RGB(10, 10, 14),  // dark concrete
    RGB( 2,  2,  4),  // shadow/dark
};

// BGP1 - Night/Rooftops
static const uint16_t PAL_BG_NIGHT[4] = {
    RGB( 2,  2, 10),  // dark night sky
    RGB( 8,  8, 16),  // rooftop grey
    RGB( 4,  4,  8),  // dark grey
    RGB( 0,  0,  2),  // near black
};

// BGP2 - Sewers (brown/green)
static const uint16_t PAL_BG_SEWERS[4] = {
    RGB( 5, 12,  3),  // murky green
    RGB( 8,  6,  2),  // brown sludge
    RGB( 3,  4,  1),  // dark green
    RGB( 1,  1,  0),  // very dark
};

// BGP3 - Warehouse (industrial orange/grey)
static const uint16_t PAL_BG_WAREHOUSE[4] = {
    RGB(20, 14,  6),  // warm grey
    RGB(12,  8,  4),  // dark brown
    RGB( 6,  4,  2),  // very dark brown
    RGB( 1,  0,  0),  // near black
};

// BGP4 - Ravencroft (clinical white/blue)
static const uint16_t PAL_BG_RAVENCROFT[4] = {
    RGB(25, 25, 28),  // white/light blue
    RGB(14, 14, 18),  // medium grey-blue
    RGB( 6,  6, 10),  // dark blue-grey
    RGB( 0,  0,  3),  // near black
};

// BGP5 - Carnage Lair (deep red/black)
static const uint16_t PAL_BG_LAIR[4] = {
    RGB(20,  2,  2),  // dark red
    RGB(10,  0,  0),  // very dark red
    RGB( 5,  0,  0),  // near black red
    RGB( 1,  0,  0),  // almost black
};

// BGP6 - HUD bar (always black/red/green/blue)
static const uint16_t PAL_BG_HUD[4] = {
    RGB( 0,  0,  0),  // black
    RGB(31,  0,  0),  // red HP
    RGB( 0, 28,  0),  // green HP full
    RGB( 0, 10, 31),  // blue special
};

// BGP7 - Cutscene text box (dark semi-transparent feel)
static const uint16_t PAL_BG_TEXTBOX[4] = {
    RGB( 0,  0,  0),  // black
    RGB(28, 28, 28),  // white text
    RGB(14, 14, 14),  // grey
    RGB( 4,  4,  4),  // dark bg
};

// ============================================================
//  FLASH STATE
// ============================================================
static uint8_t  flash_timer  = 0;
static uint16_t flash_color  = 0;
static uint16_t saved_pal[4] = {0};

// ============================================================
//  INIT  - Load all palettes into GBC registers
// ============================================================
void palette_init(void) {
    // Sprite palettes
    set_sprite_palette(PAL_SPIDERMAN, 1, PAL_SPR_SPIDERMAN);
    set_sprite_palette(PAL_VENOM,     1, PAL_SPR_VENOM);
    set_sprite_palette(PAL_CARNAGE,   1, PAL_SPR_CARNAGE);
    set_sprite_palette(PAL_ENEMIES,   1, PAL_SPR_ENEMIES);
    set_sprite_palette(PAL_EFFECTS,   1, PAL_SPR_RAGE);
    set_sprite_palette(PAL_HUD,       1, PAL_SPR_HUD);

    // BG palettes
    set_bkg_palette(PAL_BG_CITY,  1, PAL_BG_CITY_DAY);
    set_bkg_palette(PAL_BG_DARK,  1, PAL_BG_NIGHT);
    set_bkg_palette(PAL_HUD,      1, PAL_BG_HUD);
    set_bkg_palette(PAL_EFFECTS,  1, PAL_BG_TEXTBOX);
}

// ============================================================
//  CHARACTER PALETTE SWAP
// ============================================================
void palette_set_character(uint8_t character) {
    switch (character) {
        case CHAR_SPIDERMAN:
            set_sprite_palette(0, 1, PAL_SPR_SPIDERMAN); break;
        case CHAR_VENOM:
            set_sprite_palette(0, 1, PAL_SPR_VENOM);     break;
        case CHAR_CARNAGE:
            set_sprite_palette(0, 1, PAL_SPR_CARNAGE);   break;
    }
}

// ============================================================
//  LEVEL PALETTE
// ============================================================
void palette_set_level(uint8_t level_id) {
    switch (level_id) {
        case LEVEL_DOWNTOWN:
            set_bkg_palette(PAL_BG_CITY,  1, PAL_BG_CITY_DAY);
            set_bkg_palette(PAL_BG_DARK,  1, PAL_BG_NIGHT);
            break;
        case LEVEL_ROOFTOPS:
            set_bkg_palette(PAL_BG_CITY,  1, PAL_BG_NIGHT);
            set_bkg_palette(PAL_BG_DARK,  1, PAL_BG_NIGHT);
            break;
        case LEVEL_SEWERS:
            set_bkg_palette(PAL_BG_CITY,  1, PAL_BG_SEWERS);
            set_bkg_palette(PAL_BG_DARK,  1, PAL_BG_SEWERS);
            break;
        case LEVEL_WAREHOUSE:
            set_bkg_palette(PAL_BG_CITY,  1, PAL_BG_WAREHOUSE);
            set_bkg_palette(PAL_BG_DARK,  1, PAL_BG_WAREHOUSE);
            break;
        case LEVEL_RAVENCROFT:
            set_bkg_palette(PAL_BG_CITY,  1, PAL_BG_RAVENCROFT);
            set_bkg_palette(PAL_BG_DARK,  1, PAL_BG_RAVENCROFT);
            break;
        case LEVEL_CARNAGE_LAIR:
            set_bkg_palette(PAL_BG_CITY,  1, PAL_BG_LAIR);
            set_bkg_palette(PAL_BG_DARK,  1, PAL_BG_LAIR);
            break;
    }
}

// ============================================================
//  SCREEN FLASH  (hit feedback, boss death, etc.)
// ============================================================
void palette_flash(uint8_t color_idx, uint8_t duration) {
    flash_timer = duration;
    // White flash: set all BG palette entries to white momentarily
    uint16_t white[4] = {
        RGB(31,31,31), RGB(31,31,31), RGB(31,31,31), RGB(31,31,31)
    };
    // Red flash for damage:
    uint16_t red[4] = {
        RGB(31,0,0), RGB(31,5,5), RGB(20,0,0), RGB(10,0,0)
    };
    (void)color_idx;
    // set_bkg_palette(0, 1, (color_idx == 0) ? white : red);
    (void)white; (void)red;
    (void)flash_timer;
}
