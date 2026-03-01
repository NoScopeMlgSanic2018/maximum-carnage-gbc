#!/usr/bin/env python3
"""
Maximum Carnage GBC - Sprite Converter
Converts SNES/Genesis sprite sheets to GBC tile format (4-color per palette, 8x8 tiles)
"""

from PIL import Image
import numpy as np
import os

ASSETS_DIR = os.path.dirname(os.path.abspath(__file__))
GEN_DIR    = os.path.join(ASSETS_DIR, 'generated')
os.makedirs(GEN_DIR, exist_ok=True)

GBC_SPRITE_W = 16
GBC_SPRITE_H = 32

def is_bg_pixel(r, g, b, a=255):
    if a < 32: return True
    if r > 180 and g < 80 and b > 180: return True  # magenta
    if r < 60 and g > 160 and b > 130: return True   # teal
    return False

def remove_bg(img):
    img = img.convert('RGBA')
    data = np.array(img, dtype=np.int32)
    r, g, b, a = data[...,0], data[...,1], data[...,2], data[...,3]
    mask = (
        (a < 32) |
        ((r > 180) & (g < 80) & (b > 180)) |
        ((r < 60) & (g > 160) & (b > 130))
    )
    result = np.array(img.convert('RGBA'))
    result[mask, 3] = 0
    return Image.fromarray(result.astype(np.uint8))

def quantize_to_gbc(img_rgba):
    """Quantize to 4 colors for GBC. Color 0 = transparent."""
    data = np.array(img_rgba.convert('RGBA'), dtype=np.uint8)
    h, w = data.shape[:2]
    mask = data[..., 3] > 32
    
    if not np.any(mask):
        return [(0,0,0),(31<<3,31<<3,31<<3),(15<<3,15<<3,15<<3),(8<<3,8<<3,8<<3)], \
               np.zeros((h,w), dtype=np.uint8)

    # Get 3 representative colors from opaque pixels
    opaque = img_rgba.copy().convert('RGB')
    try:
        quant = opaque.quantize(colors=3, method=Image.Quantize.MEDIANCUT)
        pal_flat = quant.getpalette()
        pal_colors = [(pal_flat[i*3], pal_flat[i*3+1], pal_flat[i*3+2]) for i in range(min(3, len(pal_flat)//3))]
    except:
        pal_colors = [(255,0,0),(128,0,0),(64,0,0)]
    
    while len(pal_colors) < 3:
        pal_colors.append((0,0,0))
    
    palette = [(0,0,0)] + pal_colors[:3]

    # Map pixels to palette indices
    indexed = np.zeros((h, w), dtype=np.uint8)
    pal_arr = np.array(palette[1:], dtype=np.int32)  # 3 colors, shape (3,3)
    
    px_data = data.astype(np.int32)
    for y in range(h):
        for x in range(w):
            if px_data[y, x, 3] < 32:
                indexed[y, x] = 0
            else:
                r, g, b = px_data[y, x, 0], px_data[y, x, 1], px_data[y, x, 2]
                diffs = (pal_arr[:,0]-r)**2 + (pal_arr[:,1]-g)**2 + (pal_arr[:,2]-b)**2
                indexed[y, x] = int(np.argmin(diffs)) + 1
    
    return palette, indexed

def indexed_to_gbc_tiles(indexed, x, y, w, h):
    """Convert indexed region to GBC 2bpp tile bytes (16 bytes per 8x8 tile)."""
    tiles_x = (w + 7) // 8
    tiles_y = (h + 7) // 8
    out = []
    for ty in range(tiles_y):
        for tx in range(tiles_x):
            for row in range(8):
                py = y + ty*8 + row
                lo, hi = 0, 0
                for col in range(8):
                    px_x = x + tx*8 + col
                    if py < indexed.shape[0] and px_x < indexed.shape[1]:
                        val = int(indexed[py, px_x]) & 3
                    else:
                        val = 0
                    bit = 7 - col
                    lo |= (val & 1) << bit
                    hi |= ((val >> 1) & 1) << bit
                out.append(lo)
                out.append(hi)
    return out

def rgb_to_gbc_word(r, g, b):
    return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)

def has_content(frame):
    arr = np.array(frame.convert('RGBA'))
    return bool(np.any(arr[..., 3] > 32))

def crop_and_resize(frame):
    """Crop to content bounding box, then scale to GBC sprite size."""
    frame = frame.convert('RGBA')
    arr = np.array(frame)
    mask = arr[..., 3] > 32
    if not np.any(mask):
        return Image.new('RGBA', (GBC_SPRITE_W, GBC_SPRITE_H))
    rows = np.any(mask, axis=1)
    cols = np.any(mask, axis=0)
    r0, r1 = np.where(rows)[0][[0,-1]]
    c0, c1 = np.where(cols)[0][[0,-1]]
    frame = frame.crop((c0, r0, c1+1, r1+1))
    fw, fh = frame.size
    scale = min(GBC_SPRITE_W/max(fw,1), GBC_SPRITE_H/max(fh,1))
    nw = max(1, int(fw*scale))
    nh = max(1, int(fh*scale))
    frame = frame.resize((nw, nh), Image.NEAREST)
    canvas = Image.new('RGBA', (GBC_SPRITE_W, GBC_SPRITE_H))
    canvas.paste(frame, ((GBC_SPRITE_W-nw)//2, (GBC_SPRITE_H-nh)//2), frame)
    return canvas

def extract_frames(img, fw, fh, max_frames=48):
    img_w, img_h = img.size
    cols = max(1, img_w // fw)
    rows = max(1, img_h // fh)
    frames = []
    for r in range(rows):
        for c in range(cols):
            frame = img.crop((c*fw, r*fh, c*fw+fw, r*fh+fh))
            frames.append(frame)
            if len(frames) >= max_frames:
                return frames
    return frames

def convert_sprite_sheet(name, path, src_fw, src_fh, max_frames=40):
    print(f"  [{name}] Loading {path}...")
    img = Image.open(path)
    img = remove_bg(img)
    img_w, img_h = img.size
    
    # auto-detect better frame size
    actual_fw = min(src_fw, img_w)
    actual_fh = min(src_fh, img_h)
    
    raw_frames = extract_frames(img, actual_fw, actual_fh, max_frames*2)
    frames = [f for f in raw_frames if has_content(f)][:max_frames]
    
    if not frames:
        print(f"  [{name}] WARNING: no content frames found, using raw")
        frames = raw_frames[:max_frames]

    all_tiles = []
    offsets = []
    palette = None
    
    for i, frame in enumerate(frames):
        gbc = crop_and_resize(frame)
        pal, idx = quantize_to_gbc(gbc)
        if palette is None:
            palette = pal
        tiles = indexed_to_gbc_tiles(idx, 0, 0, GBC_SPRITE_W, GBC_SPRITE_H)
        offsets.append(len(all_tiles) // 16)
        all_tiles.extend(tiles)
    
    print(f"  [{name}] {len(frames)} frames -> {len(all_tiles)} tile bytes")
    
    # Write .c
    c = [
        f"// {name.upper()} sprite tiles - Maximum Carnage GBC",
        f"// {len(frames)} frames @ {GBC_SPRITE_W}x{GBC_SPRITE_H} (2bpp, 4-color)",
        "#include <stdint.h>",
        "",
    ]
    # palette
    pal_words = [rgb_to_gbc_word(*rgb) for rgb in palette]
    c.append(f"const uint16_t {name}_pal[4] = {{")
    for w in pal_words:
        c.append(f"    0x{w:04X},")
    c.append("};")
    c.append("")
    # tiles
    c.append(f"const uint8_t {name}_tiles[{len(all_tiles)}] = {{")
    for i in range(0, len(all_tiles), 16):
        chunk = all_tiles[i:i+16]
        c.append("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    c.append("};")
    c.append("")
    # offsets
    c.append(f"const uint8_t {name}_frame_offsets[{len(offsets)}] = {{")
    c.append("    " + ", ".join(str(o) for o in offsets))
    c.append("};")
    c.append(f"const uint8_t {name}_frame_count = {len(frames)};")
    
    # Write .h
    h = [
        "#pragma once",
        "#include <stdint.h>",
        f"extern const uint16_t {name}_pal[4];",
        f"extern const uint8_t  {name}_tiles[];",
        f"extern const uint8_t  {name}_frame_offsets[];",
        f"extern const uint8_t  {name}_frame_count;",
    ]
    
    with open(f"{GEN_DIR}/{name}_sprites.c", 'w') as f: f.write('\n'.join(c))
    with open(f"{GEN_DIR}/{name}_sprites.h", 'w') as f: f.write('\n'.join(h))

def convert_background(name, path):
    print(f"  [{name}] BG from {path}...")
    img = Image.open(path).convert('RGBA')
    iw, ih = img.size
    # Crop out watermark (bottom ~25%)
    img = img.crop((0, 0, iw, int(ih * 0.72)))
    img = img.resize((160, 144), Image.LANCZOS).convert('RGB')
    
    # quantize to 4 colors
    quant = img.quantize(colors=4, method=Image.Quantize.MEDIANCUT).convert('RGB')
    arr = np.array(quant, dtype=np.uint8)
    
    # build simple index (just map to 2 bits)
    flat = arr.reshape(-1, 3).astype(np.int32)
    unique = np.unique(flat, axis=0)[:4]
    indexed = np.zeros((144, 160), dtype=np.uint8)
    for y in range(144):
        for x in range(160):
            r,g,b = int(arr[y,x,0]), int(arr[y,x,1]), int(arr[y,x,2])
            diffs = [(r-int(u[0]))**2+(g-int(u[1]))**2+(b-int(u[2]))**2 for u in unique]
            indexed[y,x] = int(np.argmin(diffs)) & 3
    
    tiles = indexed_to_gbc_tiles(indexed, 0, 0, 160, 144)
    tilemap = list(range(min(256, 360)))
    palette = [(int(u[0]), int(u[1]), int(u[2])) for u in unique]
    while len(palette) < 4: palette.append((0,0,0))
    
    c = [
        f"// {name.upper()} background - Maximum Carnage GBC",
        "#include <stdint.h>",
        "",
        f"const uint16_t {name}_bg_pal[4] = {{",
    ]
    for r,g,b in palette:
        c.append(f"    0x{rgb_to_gbc_word(r,g,b):04X},  // ({r},{g},{b})")
    c.append("};")
    c.append(f"const uint8_t {name}_bg_tiles[{len(tiles)}] = {{")
    for i in range(0, len(tiles), 16):
        c.append("    " + ", ".join(f"0x{b:02X}" for b in tiles[i:i+16]) + ",")
    c.append("};")
    c.append(f"const uint8_t {name}_bg_map[{len(tilemap)}] = {{")
    for i in range(0, len(tilemap), 20):
        c.append("    " + ", ".join(str(t) for t in tilemap[i:i+20]) + ",")
    c.append("};")
    
    h = [
        "#pragma once", "#include <stdint.h>",
        f"extern const uint16_t {name}_bg_pal[4];",
        f"extern const uint8_t  {name}_bg_tiles[];",
        f"extern const uint8_t  {name}_bg_map[];",
    ]
    
    with open(f"{GEN_DIR}/{name}_bg.c", 'w') as f: f.write('\n'.join(c))
    with open(f"{GEN_DIR}/{name}_bg.h", 'w') as f: f.write('\n'.join(h))
    print(f"  [{name}] {len(tiles)} tile bytes")

# ── RUN ──────────────────────────────────────────────────────
print("=== Maximum Carnage GBC Asset Converter ===")
SD = f"{ASSETS_DIR}/sprites"
BD = f"{ASSETS_DIR}/backgrounds"

print("\n[CHARACTERS]")
convert_sprite_sheet("spiderman",   f"{SD}/spiderman.png",   48, 56, 40)
convert_sprite_sheet("venom",       f"{SD}/venom.png",       48, 56, 40)
convert_sprite_sheet("carnage",     f"{SD}/carnage.png",     44, 56, 36)

print("\n[ENEMIES]")
convert_sprite_sheet("enemies",     f"{SD}/enemies.png",     40, 50, 48)
convert_sprite_sheet("shriek",      f"{SD}/shriek.png",      40, 52, 24)
convert_sprite_sheet("demogoblin",  f"{SD}/demogoblin.png",  36, 48, 20)
convert_sprite_sheet("doppelganger",f"{SD}/doppelganger.png",40, 44, 20)

print("\n[BACKGROUNDS]")
convert_background("alleyway",       f"{BD}/alleyway.png")
convert_background("manhattan_roof", f"{BD}/manhattan_rooftop.png")
convert_background("bg_park",        f"{BD}/bg_park.png")
convert_background("bg_city",        f"{BD}/bg_city.png")

print("\n=== DONE ===")
gen = sorted(os.listdir(GEN_DIR))
total = sum(os.path.getsize(f"{GEN_DIR}/{f}") for f in gen)
print(f"{len(gen)} files, {total:,} bytes total")
for f in gen:
    print(f"  {f}  ({os.path.getsize(f'{GEN_DIR}/{f}'):,} bytes)")
