# Maximum Carnage GBC — Asset Integration Guide

## Sprite Sources
All sprites were ripped from SNES/Genesis *Spider-Man & Venom: Maximum Carnage* (1994)
and converted to GBC 2bpp / 4-color format via the Python pipeline in `assets/convert_sprites.py`.

| File | Source | Frames | Notes |
|------|--------|--------|-------|
| `spiderman_sprites.c` | SpiderMan.png (Cabanaman rip) | 40 | Walk, run, jump, web-swing, punch, kick, web-shot |
| `venom_sprites.c` | 11355.gif (Bonzai/Grim rip) | 40 | Walk, wall-crawl, whip, slam |
| `carnage_sprites.c` | Carnage.png (Apocalypse rip) | 36 | Walk, tendril, rage mode |
| `enemies_sprites.c` | Enemies.png (Apocalypse rip) | 48 | 7 enemy types (thug variants, Chaos Drone) |
| `shriek_sprites.c` | Shriek (Apocalypse rip) | 24 | Walk, scream, sonic wave |
| `demogoblin_sprites.c` | Demogoblin.png (Cabanaman rip) | 20 | Walk, bomb throw, glide |
| `doppelganger_sprites.c` | Doppelganger.png (Cabanaman rip) | 12 | Walk, leap, web-spray |

## Background Sources
| File | Source | Level |
|------|--------|-------|
| `alleyway_bg.c` | SNES Alleyway rip (SomeThingEviL) | Level 1: Downtown |
| `manhattan_roof_bg.c` | Manhattan Rooftop (Apocalypse rip) | Level 2: Rooftops |
| `bg_park_bg.c` | Prospect Park bg | Level 3: Park |
| `bg_city_bg.c` | San Francisco street | Level 4: Warehouse exterior |

## Regenerating Assets
```bash
cd assets
python3 convert_sprites.py
```
Requirements: `pip install Pillow numpy`

## GBC Color Reduction
Each sprite sheet is:
1. Background-removed (magenta/teal erased → transparent)
2. Each frame cropped to bounding box, scaled to 16×32 GBC sprite size
3. Quantized to **4 colors** (1 transparent + 3 opaque) per sprite
4. Converted to 2bpp tile format (16 bytes per 8×8 tile)

Palette colors are derived via median-cut quantization and stored as
GBC 15-bit RGB words (`BBBBBGGGGGRRRRR`).
