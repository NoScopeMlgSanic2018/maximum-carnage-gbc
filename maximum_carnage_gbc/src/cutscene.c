// ============================================================
//  MAXIMUM CARNAGE DEMAKE - GBC
//  cutscene.c  -  Story cutscenes for all 3 campaigns
//  Format: series of text panels + BG art, B to skip
// ============================================================

#include "main.h"

// ============================================================
//  CUTSCENE SCRIPT FORMAT
//  Each entry: { bg_tile_id, text_ptr }
//  bg_tile_id: which full-screen BG to load (0=none/black)
//  text_ptr:   pointer to dialog string (max 40 chars per card)
//  Terminated by { 0xFF, NULL }
// ============================================================
typedef struct {
    uint8_t     bg_id;
    const char *text;
} CutsceneCard;

// ============================================================
//  SPIDER-MAN CAMPAIGN CUTSCENES
// ============================================================
static const CutsceneCard SM_INTRO[] = {
    {1,  "NEW YORK CITY."},
    {1,  "A WAVE OF VILLAINY\nHAS ENGULFED THE CITY."},
    {2,  "CARNAGE AND HIS CULT\nTERRORIZE THE STREETS."},
    {2,  "SPIDER-MAN MUST\nSTOP THEM ALONE."},
    {3,  "BUT A DARK ALLY\nEMERGES FROM THE SHADOWS."},
    {3,  "VENOM.\n'WE NEED EACH OTHER,\nSPIDER.'"},
    {4,  "TOGETHER THEY WILL\nFACE THE MAXIMUM\nCARNAGE!"},
    {0xFF, NULL}
};

static const CutsceneCard SM_LVL1_END[] = {
    {5,  "THE STREETS ARE\nCLEARED... FOR NOW."},
    {5,  "CARNAGE RETREATS\nTO THE ROOFTOPS."},
    {0xFF, NULL}
};

static const CutsceneCard SM_LVL2_END[] = {
    {6,  "SHRIEK UNLEASHES\nHER SONIC SCREAM!"},
    {6,  "SPIDER-MAN PUSHES\nTHROUGH THE PAIN."},
    {0xFF, NULL}
};

static const CutsceneCard SM_FINAL[] = {
    {10, "CARNAGE LIES DEFEATED."},
    {10, "THE CITY IS SAFE\nONCE MORE."},
    {11, "BUT VENOM VANISHES\nBACK INTO THE DARK."},
    {11, "'UNTIL NEXT TIME,\nSPIDER.'"},
    {12, "THE END.\n\nYOU SAVED NEW YORK!"},
    {0xFF, NULL}
};

// ============================================================
//  VENOM CAMPAIGN CUTSCENES
// ============================================================
static const CutsceneCard VM_INTRO[] = {
    {1,  "VENOM.\nSYMBIOTE ANTI-HERO."},
    {20, "CARNAGE - THE OFFSPRING\nOF HIS OWN SYMBIOTE -\nHAS GONE TOO FAR."},
    {20, "EVEN WE HAVE LINES\nWE DO NOT CROSS."},
    {21, "VENOM WILL STOP\nCARNAGE... HIS WAY."},
    {21, "WE ARE VENOM."},
    {0xFF, NULL}
};

static const CutsceneCard VM_FINAL[] = {
    {22, "CARNAGE IS FINISHED."},
    {22, "THIS IS NOT MERCY.\nTHIS IS VENOM'S LAW."},
    {23, "SPIDER-MAN: 'THANKS\nFOR THE HELP, VENOM.'"},
    {23, "VENOM: 'WE DID NOT\nDO IT FOR YOU.'"},
    {24, "NEW YORK ENDURES.\nSO DOES VENOM."},
    {0xFF, NULL}
};

// ============================================================
//  CARNAGE CAMPAIGN CUTSCENES  (villain perspective)
// ============================================================
static const CutsceneCard CM_INTRO[] = {
    {1,  "KASADY.\nKILLER. CHAOS.\nCARNAGE."},
    {30, "THE SYMBIOTE CHOSE\nWELL. TOGETHER WE ARE\nPURE DESTRUCTION."},
    {30, "SPIDER-MAN AND VENOM\nSTAND IN OUR WAY."},
    {31, "THEY WILL LEARN...\nTHERE ARE NO RULES.\nTHERE IS ONLY CARNAGE!"},
    {31, "LET THE SLAUGHTER\nBEGIN!"},
    {0xFF, NULL}
};

static const CutsceneCard CM_LVL3_MID[] = {
    {32, "SHRIEK: 'KILL THEM ALL,\nMY LOVE!'"},
    {32, "YES. YES. YES."},
    {0xFF, NULL}
};

static const CutsceneCard CM_FINAL[] = {
    {33, "VENOM FALLS.\nSPIDER-MAN FALLS."},
    {33, "NEW YORK IS OURS."},
    {34, "CHAOS REIGNS.\nCARNAGE WINS."},
    {34, "OR DOES HE...?"},
    {35, "TO BE CONTINUED...\n\nYOU SPREAD CARNAGE!"},
    {0xFF, NULL}
};

// ============================================================
//  CUTSCENE ID TABLE
//  Cutscene IDs are passed from level.c / main.c
// ============================================================
#define CS_SM_INTRO     0
#define CS_SM_LVL1_END  1
#define CS_SM_LVL2_END  2
#define CS_SM_FINAL     5
#define CS_VM_INTRO     6
#define CS_VM_FINAL     11
#define CS_CM_INTRO     12
#define CS_CM_MID       15
#define CS_CM_FINAL     17

static const CutsceneCard * const CUTSCENE_TABLE[] = {
    SM_INTRO,       // 0
    SM_LVL1_END,    // 1
    SM_LVL2_END,    // 2
    SM_INTRO,       // 3 (reuse placeholder)
    SM_INTRO,       // 4
    SM_FINAL,       // 5
    VM_INTRO,       // 6
    VM_INTRO,       // 7
    VM_INTRO,       // 8
    VM_INTRO,       // 9
    VM_INTRO,       // 10
    VM_FINAL,       // 11
    CM_INTRO,       // 12
    CM_INTRO,       // 13
    CM_INTRO,       // 14
    CM_LVL3_MID,    // 15
    CM_INTRO,       // 16
    CM_FINAL,       // 17
};

// ============================================================
//  RUNTIME STATE
// ============================================================
static const CutsceneCard *cs_cards   = NULL;
static uint8_t             cs_card_idx = 0;
static uint8_t             cs_timer    = 0;
static uint8_t             cs_typing   = 0;   // current char being typed
static uint8_t             cs_done     = 0;

#define CS_CHAR_DELAY   3    // frames per character (typewriter)
#define CS_MIN_DISPLAY  60   // min frames before next card allowed

// ============================================================
//  PLAY
// ============================================================
void cutscene_play(uint8_t id) {
    if (id >= (sizeof(CUTSCENE_TABLE) / sizeof(CUTSCENE_TABLE[0]))) {
        // No cutscene: go straight to gameplay
        g_game.game_state = STATE_GAMEPLAY;
        level_load(g_game.current_level, g_game.campaign);
        return;
    }
    cs_cards    = CUTSCENE_TABLE[id];
    cs_card_idx = 0;
    cs_timer    = 0;
    cs_typing   = 0;
    cs_done     = 0;
    music_play(20); // cutscene music
}

// ============================================================
//  UPDATE
// ============================================================
void cutscene_update(void) {
    if (!cs_cards) {
        g_game.game_state = STATE_GAMEPLAY;
        level_load(g_game.current_level, g_game.campaign);
        return;
    }

    if (cs_cards[cs_card_idx].bg_id == 0xFF) {
        // End of cutscene
        cs_done = 1;
        g_game.game_state = STATE_GAMEPLAY;
        level_load(g_game.current_level, g_game.campaign);
        return;
    }

    cs_timer++;

    // Typewriter effect
    const char *txt = cs_cards[cs_card_idx].text;
    uint8_t txt_len = 0;
    while (txt[txt_len]) txt_len++;

    if (cs_typing < txt_len) {
        if ((cs_timer % CS_CHAR_DELAY) == 0) {
            cs_typing++;
            if (txt[cs_typing - 1] != ' ' && txt[cs_typing - 1] != '\n') {
                sfx_play(12); // text blip sfx
            }
        }
    }

    // B = skip card / fast forward text
    if (g_joypad_pressed & J_B) {
        if (cs_typing < txt_len) {
            cs_typing = txt_len; // show all text
        } else if (cs_timer >= CS_MIN_DISPLAY) {
            // Next card
            cs_card_idx++;
            cs_typing = 0;
            cs_timer  = 0;
            if (cs_cards[cs_card_idx].bg_id == 0xFF) {
                g_game.game_state = STATE_GAMEPLAY;
                level_load(g_game.current_level, g_game.campaign);
            }
        }
    }

    // Auto-advance after long pause
    if (cs_typing >= txt_len && cs_timer > 180) {
        cs_card_idx++;
        cs_typing = 0;
        cs_timer  = 0;
    }

    // START = skip entire cutscene
    if (g_joypad_pressed & J_START) {
        g_game.game_state = STATE_GAMEPLAY;
        level_load(g_game.current_level, g_game.campaign);
    }
}

// ============================================================
//  DRAW
// ============================================================
void cutscene_draw(void) {
    if (!cs_cards || cs_cards[cs_card_idx].bg_id == 0xFF) return;

    // Load BG art for this card
    // set_bkg_tiles(0, 0, 20, 18, cutscene_bg_data[cs_cards[cs_card_idx].bg_id]);

    // Draw text box (bottom 4 rows of BG)
    // In real GBDK: use set_bkg_tiles + a font tileset
    // Typewriter: only render cs_typing characters of the string
    const char *txt = cs_cards[cs_card_idx].text;

    uint8_t col = 0, row = 14;
    for (uint8_t i = 0; i < cs_typing; i++) {
        if (txt[i] == '\n') { col = 0; row++; continue; }
        uint8_t tile = 220 + (txt[i] - ' '); // font tile offset
        // set_bkg_tile(col, row, tile);  // real GBDK
        col++;
        if (col >= 20) { col = 0; row++; }
        (void)tile;
    }

    // Blink "PRESS B" indicator when text fully shown
    uint8_t txt_len = 0;
    while (txt[txt_len]) txt_len++;
    if (cs_typing >= txt_len && (g_frame_count & 0x10)) {
        // set_bkg_tile(8, 17, TILE_PRESS_B);  // real GBDK
    }
}
