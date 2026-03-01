// ============================================================
//  MAXIMUM CARNAGE DEMAKE - GBC
//  level.c  -  Level loading, scrolling, tilemap, enemy spawns
//  6 levels x 3 campaigns
// ============================================================

#include "main.h"

// ============================================================
//  TILEMAP DATA
//  Each level has a 32x18 tilemap (BG layer)
//  Tile 0 = empty/air, Tile 1-15 = solid platforms
//  Tile 16+ = decorative
// ============================================================

// --- Level 0: Downtown (City Streets) ---
static const uint8_t MAP_DOWNTOWN[MAP_H_TILES][MAP_W_TILES] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, // Floor
    {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
    {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
    {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3},
    {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3},
};

// --- Level 1: Rooftops ---
static const uint8_t MAP_ROOFTOPS[MAP_H_TILES][MAP_W_TILES] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1},
    {2,2,2,2,2,2,2,2,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,2,2,2,2,2,2,2,2,2},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

// For brevity, levels 2-5 use similar patterns
// In the real project, each would be fully hand-crafted
static const uint8_t MAP_SEWERS[MAP_H_TILES][MAP_W_TILES]       = {{1}}; // placeholder
static const uint8_t MAP_WAREHOUSE[MAP_H_TILES][MAP_W_TILES]    = {{1}}; // placeholder
static const uint8_t MAP_RAVENCROFT[MAP_H_TILES][MAP_W_TILES]   = {{1}}; // placeholder
static const uint8_t MAP_CARNAGE_LAIR[MAP_H_TILES][MAP_W_TILES] = {{1}}; // placeholder

static const uint8_t * const LEVEL_MAPS[] = {
    (const uint8_t *)MAP_DOWNTOWN,
    (const uint8_t *)MAP_ROOFTOPS,
    (const uint8_t *)MAP_SEWERS,
    (const uint8_t *)MAP_WAREHOUSE,
    (const uint8_t *)MAP_RAVENCROFT,
    (const uint8_t *)MAP_CARNAGE_LAIR,
};

// ============================================================
//  SOLID TILE IDS (tiles 1-15 are solid platforms/floors)
// ============================================================
#define SOLID_TILE_MIN 1
#define SOLID_TILE_MAX 15

// ============================================================
//  ENEMY SPAWN TABLES
//  { type, x, y }  for each level x campaign
//  Terminated by { 0xFF, 0, 0 }
// ============================================================

typedef struct { uint8_t type; uint8_t x; uint8_t y; } SpawnEntry;

// Spider-Man campaign spawns
static const SpawnEntry SPAWN_SM_DOWNTOWN[] = {
    {ENEMY_SHRIEK,       80,  80},
    {ENEMY_DOPPELGANGER,140,  80},
    {ENEMY_DEMOGOBLIN,  200,  80},
    {0xFF, 0, 0}
};
static const SpawnEntry SPAWN_SM_ROOFTOPS[] = {
    {ENEMY_CARRION,      60,  80},
    {ENEMY_SPIDERCIDE,  130,  80},
    {ENEMY_DEMOGOBLIN,  210,  80},
    {0xFF, 0, 0}
};
static const SpawnEntry SPAWN_SM_SEWERS[]       = { {ENEMY_SHRIEK,60,80},{ENEMY_CARRION,140,80},{0xFF,0,0} };
static const SpawnEntry SPAWN_SM_WAREHOUSE[]    = { {ENEMY_DOPPELGANGER,80,80},{ENEMY_SPIDERCIDE,160,80},{0xFF,0,0} };
static const SpawnEntry SPAWN_SM_RAVENCROFT[]   = { {ENEMY_DEMOGOBLIN,80,80},{ENEMY_SHRIEK,160,80},{0xFF,0,0} };
static const SpawnEntry SPAWN_SM_CARNAGE_LAIR[] = { {ENEMY_BOSS_CARNAGE,200,70},{0xFF,0,0} };

// Venom campaign spawns (same locations, different enemy mix)
static const SpawnEntry SPAWN_VM_DOWNTOWN[] = {
    {ENEMY_DOPPELGANGER, 80,  80},
    {ENEMY_SHRIEK,      150,  80},
    {ENEMY_CARRION,     210,  80},
    {0xFF, 0, 0}
};
static const SpawnEntry SPAWN_VM_ROOFTOPS[]   = { {ENEMY_SPIDERCIDE,60,80},{ENEMY_DEMOGOBLIN,140,80},{0xFF,0,0} };
static const SpawnEntry SPAWN_VM_SEWERS[]     = { {ENEMY_CARRION,70,80},{ENEMY_SHRIEK,150,80},{0xFF,0,0} };
static const SpawnEntry SPAWN_VM_WAREHOUSE[]  = { {ENEMY_DEMOGOBLIN,90,80},{ENEMY_DOPPELGANGER,170,80},{0xFF,0,0} };
static const SpawnEntry SPAWN_VM_RAVENCROFT[] = { {ENEMY_SPIDERCIDE,80,80},{ENEMY_CARRION,160,80},{0xFF,0,0} };
static const SpawnEntry SPAWN_VM_CARNAGE_LAIR[]= { {ENEMY_BOSS_CARNAGE,200,70},{0xFF,0,0} };

// Carnage campaign spawns (villain perspective - fights heroes + rivals)
// Note: In Carnage campaign, bosses are Venom and Spider-Man
static const SpawnEntry SPAWN_CM_DOWNTOWN[] = {
    {ENEMY_SHRIEK,       80,  80},  // Shriek is Carnage's ally here - could be reskinned
    {ENEMY_DOPPELGANGER,160,  80},
    {ENEMY_CARRION,     220,  80},
    {0xFF, 0, 0}
};
static const SpawnEntry SPAWN_CM_ROOFTOPS[]   = { {ENEMY_DEMOGOBLIN,60,80},{ENEMY_SPIDERCIDE,140,80},{0xFF,0,0} };
static const SpawnEntry SPAWN_CM_SEWERS[]     = { {ENEMY_CARRION,80,80},{ENEMY_DOPPELGANGER,160,80},{0xFF,0,0} };
static const SpawnEntry SPAWN_CM_WAREHOUSE[]  = { {ENEMY_SHRIEK,70,80},{ENEMY_SPIDERCIDE,150,80},{0xFF,0,0} };
static const SpawnEntry SPAWN_CM_RAVENCROFT[] = { {ENEMY_DEMOGOBLIN,90,80},{ENEMY_CARRION,170,80},{0xFF,0,0} };
static const SpawnEntry SPAWN_CM_SPIDER_BOSS[]= { {ENEMY_BOSS_SPIDER,100,70},{ENEMY_BOSS_VENOM,200,70},{0xFF,0,0} };

// Table of spawn tables [campaign][level]
static const SpawnEntry * const SPAWN_TABLE[3][MAX_LEVELS] = {
    // CAMPAIGN_SPIDERMAN
    { SPAWN_SM_DOWNTOWN, SPAWN_SM_ROOFTOPS, SPAWN_SM_SEWERS,
      SPAWN_SM_WAREHOUSE, SPAWN_SM_RAVENCROFT, SPAWN_SM_CARNAGE_LAIR },
    // CAMPAIGN_VENOM
    { SPAWN_VM_DOWNTOWN, SPAWN_VM_ROOFTOPS, SPAWN_VM_SEWERS,
      SPAWN_VM_WAREHOUSE, SPAWN_VM_RAVENCROFT, SPAWN_VM_CARNAGE_LAIR },
    // CAMPAIGN_CARNAGE
    { SPAWN_CM_DOWNTOWN, SPAWN_CM_ROOFTOPS, SPAWN_CM_SEWERS,
      SPAWN_CM_WAREHOUSE, SPAWN_CM_RAVENCROFT, SPAWN_CM_SPIDER_BOSS },
};

// ============================================================
//  LEVEL LOAD
// ============================================================
void level_load(uint8_t id, uint8_t campaign) {
    memset(&g_level, 0, sizeof(Level));
    g_level.id       = id;
    g_level.campaign = campaign;
    g_level.scroll_x = 0;
    g_level.scroll_y = 0;
    g_level.complete = 0;

    // Spawn enemies
    enemies_init();
    const SpawnEntry *spawns = SPAWN_TABLE[campaign][id];
    uint8_t slot = 0;
    while (spawns->type != 0xFF && slot < MAX_ENEMIES) {
        enemy_spawn(slot, spawns->type, spawns->x, spawns->y);
        if (spawns->type >= ENEMY_BOSS_CARNAGE) {
            g_level.boss_active = 1;
        }
        spawns++;
        slot++;
    }
    g_level.enemy_count = slot;

    // Set BG palette for level
    palette_set_level(id);

    // Set BG tiles (would call set_bkg_tiles in real GBDK)
    // set_bkg_tiles(0, 0, MAP_W_TILES, MAP_H_TILES, LEVEL_MAPS[id]);

    // Play level music
    switch (id) {
        case LEVEL_DOWNTOWN:    music_play(0); break;
        case LEVEL_ROOFTOPS:    music_play(1); break;
        case LEVEL_SEWERS:      music_play(2); break;
        case LEVEL_WAREHOUSE:   music_play(3); break;
        case LEVEL_RAVENCROFT:  music_play(4); break;
        case LEVEL_CARNAGE_LAIR:music_play(5); break;
    }
}

// ============================================================
//  SOLID TILE CHECK
// ============================================================
uint8_t level_is_solid(int16_t wx, int16_t wy) {
    if (wx < 0 || wy < 0) return 1; // world boundary is solid
    uint8_t tx = (uint8_t)(wx / TILE_SIZE);
    uint8_t ty = (uint8_t)(wy / TILE_SIZE);
    if (tx >= MAP_W_TILES || ty >= MAP_H_TILES) return 1;

    // Read tile from current map
    const uint8_t *map = LEVEL_MAPS[g_level.id];
    uint8_t tile = map[(ty * MAP_W_TILES) + tx];

    return (tile >= SOLID_TILE_MIN && tile <= SOLID_TILE_MAX) ? 1 : 0;
}

// ============================================================
//  SCROLL
// ============================================================
void level_scroll_update(void) {
    int16_t px = g_player.x >> 4;

    // Scroll to follow player (dead-zone style)
    int16_t target_scroll = px - (SCREEN_W / 2);
    int16_t max_scroll    = (MAP_W_TILES * TILE_SIZE) - SCREEN_W;

    if (target_scroll < 0)           target_scroll = 0;
    if (target_scroll > max_scroll)  target_scroll = max_scroll;

    // Smooth scroll
    int16_t diff = target_scroll - g_level.scroll_x;
    if (diff > 2)  g_level.scroll_x += 2;
    else if (diff < -2) g_level.scroll_x -= 2;
    else g_level.scroll_x = target_scroll;

    // Apply to BG
    SCX_REG = (uint8_t)g_level.scroll_x;
    SCY_REG = 0;
}

// ============================================================
//  LEVEL DRAW BG  (handled by hardware scroll + tile VRAM)
//  Real GBDK: tiles are already in VRAM, hardware scrolls them.
//  We just need to update scroll registers (done above).
//  For partial VRAM updates (streaming), update one column per frame.
// ============================================================
void level_draw_bg(void) {
    // Stream in new column on the edge of the screen as we scroll
    // This simulates how real GB games handle level scrolling:
    // 1 column of 18 tiles per frame keeps things smooth
    static uint8_t last_col = 0xFF;
    uint8_t visible_col_right = (uint8_t)((g_level.scroll_x + SCREEN_W) / TILE_SIZE);

    if (visible_col_right != last_col && visible_col_right < MAP_W_TILES) {
        last_col = visible_col_right;
        const uint8_t *map = LEVEL_MAPS[g_level.id];
        // set_bkg_tiles(visible_col_right, 0, 1, MAP_H_TILES,
        //               &map[visible_col_right]);  // real GBDK call
        (void)map; // suppress unused in stub
    }
}

// ============================================================
//  LEVEL UPDATE  (spawn waves, checkpoints, etc.)
// ============================================================
void level_update(void) {
    // Check if all enemies defeated to mark level complete
    // (Boss levels need boss dead specifically)
    if (g_level.boss_active) {
        // Boss level: complete only when boss is dead
        uint8_t boss_alive = 0;
        for (uint8_t i = 0; i < MAX_ENEMIES; i++) {
            if (g_level.enemies[i].alive &&
                g_level.enemies[i].type >= ENEMY_BOSS_CARNAGE) {
                boss_alive = 1;
                break;
            }
        }
        if (!boss_alive) g_level.complete = 1;
    } else {
        // Normal level: complete when all enemies dead
        uint8_t any_alive = 0;
        for (uint8_t i = 0; i < MAX_ENEMIES; i++) {
            if (g_level.enemies[i].alive) { any_alive = 1; break; }
        }
        // Also require player to reach right edge of scroll
        int16_t px = g_player.x >> 4;
        int16_t level_end_x = MAP_W_TILES * TILE_SIZE - 24;
        if (!any_alive && px >= level_end_x) {
            g_level.complete = 1;
        }
    }
}
