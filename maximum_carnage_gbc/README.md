# MAXIMUM CARNAGE DEMAKE — Game Boy Color
### A GBDK/C Fan Project

---

## What's In This Project

A complete GBDK/C engine for a Maximum Carnage demake targeting the Game Boy Color. Three full campaigns, six levels each, all core gameplay systems implemented.

---

## Source Files

| File | Contents |
|---|---|
| `src/main.c` | Game loop, state machine, entry point |
| `src/player.c` | All three playable characters + unique mechanics |
| `src/enemy.c` | All villain AI (Shriek, Doppelganger, Demogoblin, Carrion, Spidercide) + boss phases |
| `src/physics.c` | AABB collision, gravity, movement |
| `src/level.c` | 6 levels × 3 campaigns, tilemaps, enemy spawns, scrolling |
| `src/hud.c` | HP bars, special meter, score display, lives icons |
| `src/cutscene.c` | Story scenes for all 3 campaigns with typewriter text |
| `src/title.c` | Title screen + character/campaign select |
| `src/palette.c` | All GBC color palettes (characters, levels, HUD) |
| `src/music.c` | GBT Player integration stubs + SFX register writes |
| `include/main.h` | All types, constants, global declarations |

---

## Three Campaigns

### Spider-Man Campaign
- Levels: Downtown → Rooftops → Sewers → Warehouse → Ravencroft → Carnage's Lair
- Final boss: **Carnage**
- Unique mechanic: **Web-Swing** (B+jump airborne), **Web-Shot** (special)
- Story: Spider-Man and Venom team up to stop Maximum Carnage

### Venom Campaign  
- Same levels, different enemy mix
- Final boss: **Carnage**
- Unique mechanic: **Wall Crawl** (press into walls), **Symbiote Whip** (long-range special)
- Story: Venom takes down his own offspring

### Carnage Campaign *(new for this demake)*
- Same levels, villain perspective
- Final bosses: **Venom** + **Spider-Man** (simultaneous two-boss fight!)
- Unique mechanic: **Tendril Burst** (AoE special), **Rage Mode** (passive — triggers below 30 HP, grants damage resistance + slow HP regen)
- Story: Carnage fights to dominate New York

---

## Controls

| Button | Action |
|---|---|
| D-Pad | Move |
| A | Jump |
| B | Attack (press twice for combo) |
| A + B | Special attack (costs special meter) |
| START | Pause |
| SELECT | — |

### Spider-Man Only
| Input | Action |
|---|---|
| A + B (airborne) | Web-Swing — swings forward, damages enemies |
| A + B (grounded) | Web-Shot — fires web projectile |

### Venom Only
| Input | Action |
|---|---|
| Into wall + D-Pad | Wall Crawl — slide down walls |
| A + B | Symbiote Whip — wide mid-range sweep |

### Carnage Only
| Input | Action |
|---|---|
| A + B | Tendril Burst — AoE around Carnage |
| Passive | Rage Mode — auto-activates below 30 HP |

---

## Building

### Prerequisites
1. **GBDK 4.x** — https://github.com/gbdk-2020/gbdk-2020/releases
2. **png2asset** — included with GBDK (for sprites)
3. **hUGETracker** or **GBT Player** — for music (optional, stubs included)

### Steps

```bash
# 1. Clone/extract this project
cd maximum_carnage_gbc

# 2. Set GBDK path
export GBDK=/path/to/gbdk

# 3. Build
make

# 4. Run in emulator
make run   # requires bgb in PATH
# or open .gbc in mGBA, SameBoy, BGB, etc.
```

---

## What You Still Need to Add

### 1. Sprite Assets
The engine references tile indices but you need actual graphics:

**Option A — Use GBC Spider-Man 2000 sprites as reference:**
- Open the GBC ROM in Tile Layer Pro or BGB tile viewer
- Export tiles to PNG
- Remap into this engine's tile layout (see `include/main.h` for tile indices)

**Option B — Draw original GBC-style sprites:**
- 16×16 characters in 4-color GBC palette
- Tools: Aseprite, Tile Layer Pro, GBTD

Use `png2asset` to convert:
```bash
png2asset assets/sprites/spiderman.png -c src/spr_spiderman.c -spr8x16
```
Then add the generated `.c` file to the `SRCS` list in `Makefile`.

### 2. Level Tilemaps
Levels 2–5 use placeholder maps. Build them in:
- **Tiled** (export to CSV → convert to C array)
- **GBTDK Map Builder**
- Or hand-author the `MAP_*` arrays in `level.c`

### 3. Music
Music stubs are in `music.c`. To add real GBC music:
1. Compose in **hUGETracker** (free, exports to GBDK-compatible C)
2. Or convert .MOD files using **GBT Player**
3. Replace `NULL` entries in `SONG_TABLE[]` with your song data pointers
4. Replace stub calls with: `gbt_play(song_data, GBT_SPEED_NORMAL);`

### 4. Boss Cutscene Art
`cutscene.c` references BG tile IDs (bg_id). Load your BG art tiles into VRAM and set the tile maps in `cutscene_draw()`.

---

## Architecture Notes

**Fixed-point math**: All world positions use `int16_t` with 4 fractional bits (`x >> 4` = pixel position). This gives smooth sub-pixel movement without floating point.

**OAM layout**:
- Slots 0–3: Player (4 sprites = 16×32 character)
- Slots 4–19: Enemies (4 per enemy × 4 max)
- Slots 20–27: HUD elements

**Palette layout**:
- OBP 0/1/2: Spider-Man / Venom / Carnage  
- OBP 3: Enemies  
- OBP 4: HUD/Effects  
- BGP 0–5: Level backgrounds  
- BGP 6: HUD bars  
- BGP 7: Cutscene text box

**State machine**: `STATE_BOOT → STATE_TITLE → STATE_CHAR_SEL → STATE_CUTSCENE → STATE_GAMEPLAY → STATE_BOSS → STATE_GAMEOVER/CREDITS`

---

## Recommended Emulators for Testing
- **mGBA** — best accuracy, GBC color support
- **BGB** — best debugger (tile viewer, VRAM inspector)
- **SameBoy** — high accuracy
- **Emulicious** — excellent debugging

---

## Legal Note
This is a fan/educational project. All Marvel characters are trademarks of Marvel Entertainment. The original Maximum Carnage game is © 2 Heaven. This project uses no original game assets.
