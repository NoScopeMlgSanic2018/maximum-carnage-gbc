# ============================================================
#  MAXIMUM CARNAGE DEMAKE - GBC
#  Makefile for GBDK/C
#
#  Requirements:
#    - GBDK 4.x installed (https://github.com/gbdk-2020/gbdk-2020)
#    - Set GBDK path below or in environment
# ============================================================

GBDK        ?= /opt/gbdk
CC           = $(GBDK)/bin/lcc
CFLAGS       = -Wa-l -Wl-m -Wl-j -DSGB_SUPPORT -DCGB
CFLAGS      += -I./include
CFLAGS      += -msm83:cgb         # Target GBC (Color Game Boy)
CFLAGS      += -Wl-yt0x1B         # CGB-only mode (not DMG compatible)
CFLAGS      += -Wl-yo4            # 4 ROM banks (256KB)
CFLAGS      += -Wl-ya8            # 8 RAM banks (64KB SRAM)
CFLAGS      += -O2                # Optimize

SRCS         = src/main.c          \
               src/player.c        \
               src/enemy.c         \
               src/physics.c       \
               src/level.c         \
               src/hud.c           \
               src/cutscene.c      \
               src/title.c         \
               src/palette.c       \
               src/music.c         \
               src/web_swing.c     \
               src/boss_abilities.c \
               assets/generated/spiderman_sprites.c  \
               assets/generated/venom_sprites.c      \
               assets/generated/carnage_sprites.c    \
               assets/generated/enemies_sprites.c    \
               assets/generated/shriek_sprites.c     \
               assets/generated/demogoblin_sprites.c \
               assets/generated/doppelganger_sprites.c \
               assets/generated/alleyway_bg.c        \
               assets/generated/manhattan_roof_bg.c  \
               assets/generated/bg_park_bg.c         \
               assets/generated/bg_city_bg.c

TARGET       = maximum_carnage_gbc.gbc

# ============================================================
#  BUILD
# ============================================================
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)
	@echo ""
	@echo "  BUILD COMPLETE: $(TARGET)"
	@echo "  Load in BGB, SameBoy, mGBA, or flash to cart."
	@echo ""

# ============================================================
#  ASSET PIPELINE
#  In real development, use png2asset or GBTDK tools:
#    png2asset assets/sprites/spiderman.png -c src/spr_spiderman.c
# ============================================================
assets:
	@echo "Running asset converter..."
	cd assets && python3 convert_sprites.py
	@echo "Done. Generated files in assets/generated/"

clean:
	rm -f $(TARGET) *.map *.noi *.asm *.o *.lst

# ============================================================
#  EMULATOR SHORTCUT
# ============================================================
run: all
	bgb $(TARGET)

# Debug build
debug: CFLAGS += -DDEBUG
debug: all
