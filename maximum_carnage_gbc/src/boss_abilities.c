// ============================================================
//  MAXIMUM CARNAGE DEMAKE - GBC
//  boss_abilities.c  -  Boss Ability Inheritance System
//
//  After defeating each boss, the player absorbs that boss's
//  signature power as a second special slot.
//  Press SELECT to cycle between primary special and any
//  unlocked boss abilities.
//  A small HUD icon shows which ability is currently active.
//
//  Ability slots:
//    Slot 0 = Primary special (character default)
//    Slot 1 = First unlocked boss ability
//    Slot 2 = Second unlocked boss ability
//    Slot 3 = Third unlocked boss ability  (max 4 total)
// ============================================================

#include "main.h"
#include "boss_abilities.h"

// ────────────────────────────────────────────────────────────
//  ABILITY DEFINITIONS
//  Each boss grants a unique power with distinct mechanics.
// ────────────────────────────────────────────────────────────

// Ability parameter tables
static const BossAbilityDef ABILITY_TABLE[BOSS_ABILITY_COUNT] = {

    // ABILITY_NONE (slot 0 placeholder for primary special)
    { ABILITY_NONE,
      "Primary",
      25,   // damage
      60,   // special meter cost (out of 255)
      0,    // projectile: no
      0,    // aoe: no
      0,    // stun_frames
      ANIM_PRIMARY,
      "Your signature move" },

    // ABILITY_CARNAGE_TENDRILS  (defeat boss Carnage)
    { ABILITY_CARNAGE_TENDRILS,
      "Tendrils",
      40,   // heavy damage
      80,   // high meter cost
      0,    // not a projectile (melee AoE)
      1,    // AoE: hits all on-screen enemies
      0,    // no stun
      ANIM_CARNAGE_TENDRIL,
      "Symbiote tendril burst - screen-wide AoE" },

    // ABILITY_SHRIEK_SCREAM  (defeat Shriek)
    { ABILITY_SHRIEK_SCREAM,
      "Sonic Scream",
      20,   // moderate damage
      70,   // medium cost
      0,    // not a projectile (wave)
      1,    // AoE: stuns all on screen
      90,   // stun: ~1.5 seconds
      ANIM_SHRIEK_SCREAM,
      "Sonic scream - stuns all enemies + knockback" },

    // ABILITY_DEMOGOBLIN_BOMB  (defeat Demogoblin)
    { ABILITY_DEMOGOBLIN_BOMB,
      "Pumpkin Bomb",
      35,   // good damage + splash
      65,   // medium-high cost
      1,    // projectile: yes (arcing)
      1,    // AoE: explosion radius on impact
      30,   // brief stun on blast
      ANIM_DEMO_BOMB,
      "Arcing pumpkin bomb - explosion radius" },

    // ABILITY_DOPPELGANGER_CLONE  (defeat Doppelganger)
    { ABILITY_DOPPELGANGER_CLONE,
      "Shadow Clone",
      15,   // clone deals damage on contact
      90,   // expensive
      0,    // not a projectile
      0,    // not AoE
      0,    // no stun
      ANIM_CLONE_SPAWN,
      "Spawns shadow clone that fights for 5 seconds" },
};

// ────────────────────────────────────────────────────────────
//  GLOBAL STATE
// ────────────────────────────────────────────────────────────

BossAbilityManager g_abilities;

// Active projectile (pumpkin bomb)
static BossProjectile g_projectile;

// Clone state (Doppelganger ability)
static CloneState g_clone;

// HUD notification timer (show ability name briefly on pickup)
static uint8_t notify_timer = 0;

// ────────────────────────────────────────────────────────────
//  INIT
// ────────────────────────────────────────────────────────────
void boss_abilities_init(void) {
    g_abilities.slot_count    = 1;         // starts with primary special
    g_abilities.active_slot   = 0;
    g_abilities.select_cooldown = 0;
    for (uint8_t i = 0; i < MAX_ABILITY_SLOTS; i++) {
        g_abilities.slots[i] = ABILITY_NONE;
    }
    g_abilities.slots[0] = ABILITY_NONE;   // primary = character default

    g_projectile.active = 0;
    g_clone.active = 0;
    notify_timer = 0;
}

// ────────────────────────────────────────────────────────────
//  UNLOCK  -  called when a boss is defeated
// ────────────────────────────────────────────────────────────
void boss_ability_unlock(uint8_t boss_type) {
    if (g_abilities.slot_count >= MAX_ABILITY_SLOTS) return;

    uint8_t ability_id = ABILITY_NONE;
    switch (boss_type) {
        case ENEMY_BOSS_CARNAGE:   ability_id = ABILITY_CARNAGE_TENDRILS; break;
        case ENEMY_SHRIEK:         ability_id = ABILITY_SHRIEK_SCREAM;    break;
        case ENEMY_DEMOGOBLIN:     ability_id = ABILITY_DEMOGOBLIN_BOMB;  break;
        case ENEMY_DOPPELGANGER:   ability_id = ABILITY_DOPPELGANGER_CLONE; break;
        default: return;
    }

    // Don't add duplicates
    for (uint8_t i = 0; i < g_abilities.slot_count; i++) {
        if (g_abilities.slots[i] == ability_id) return;
    }

    g_abilities.slots[g_abilities.slot_count] = ability_id;
    g_abilities.slot_count++;
    notify_timer = 120;   // show notification for 2 seconds
}

// ────────────────────────────────────────────────────────────
//  CYCLE  -  called on SELECT press
// ────────────────────────────────────────────────────────────
void boss_ability_cycle(void) {
    if (g_abilities.select_cooldown > 0) return;
    if (g_abilities.slot_count <= 1) return;

    g_abilities.active_slot = (g_abilities.active_slot + 1) % g_abilities.slot_count;
    g_abilities.select_cooldown = SELECT_COOLDOWN_FRAMES;
}

// ────────────────────────────────────────────────────────────
//  GET ACTIVE ABILITY
// ────────────────────────────────────────────────────────────
uint8_t boss_ability_active_id(void) {
    return g_abilities.slots[g_abilities.active_slot];
}

const BossAbilityDef *boss_ability_get_def(uint8_t ability_id) {
    if (ability_id >= BOSS_ABILITY_COUNT) return &ABILITY_TABLE[0];
    for (uint8_t i = 0; i < BOSS_ABILITY_COUNT; i++) {
        if (ABILITY_TABLE[i].id == ability_id) return &ABILITY_TABLE[i];
    }
    return &ABILITY_TABLE[0];
}

// ────────────────────────────────────────────────────────────
//  FIRE  -  execute the currently selected ability
//           Returns 1 if executed, 0 if not enough meter
// ────────────────────────────────────────────────────────────
uint8_t boss_ability_fire(PlayerState *p, EnemyState *enemies, uint8_t enemy_count) {
    uint8_t id = boss_ability_active_id();
    if (id == ABILITY_NONE) {
        // Primary special - handled by player.c
        return 0;
    }

    const BossAbilityDef *def = boss_ability_get_def(id);

    // Check special meter
    if (p->special_meter < def->meter_cost) return 0;
    p->special_meter -= def->meter_cost;

    // Set player animation
    p->state      = PSTATE_BOSS_ABILITY;
    p->frame      = 0;
    p->anim_table = def->anim_id;

    switch (id) {

        // ── Carnage Tendrils: AoE burst, hits all on-screen enemies ──
        case ABILITY_CARNAGE_TENDRILS:
            for (uint8_t i = 0; i < enemy_count; i++) {
                if (enemies[i].hp > 0) {
                    enemies[i].hp -= def->damage;
                    enemies[i].state = ESTATE_HURT;
                    enemies[i].hurt_timer = 20;
                    // Knockback: away from player
                    enemies[i].vx = (enemies[i].x > p->x) ? 4 : -4;
                    enemies[i].vy = -3;
                    if (enemies[i].hp <= 0) {
                        enemies[i].state = ESTATE_DEAD;
                        enemies[i].death_timer = 30;
                    }
                }
            }
            break;

        // ── Shriek Scream: stun + knockback all on screen ────────────
        case ABILITY_SHRIEK_SCREAM:
            for (uint8_t i = 0; i < enemy_count; i++) {
                if (enemies[i].hp > 0) {
                    enemies[i].hp -= def->damage;
                    enemies[i].state      = ESTATE_STUNNED;
                    enemies[i].stun_timer = def->stun_frames;
                    // Knockback
                    enemies[i].vx = (enemies[i].x > p->x) ? 3 : -3;
                    enemies[i].vy = -2;
                }
            }
            break;

        // ── Demogoblin Bomb: launch arcing projectile ─────────────────
        case ABILITY_DEMOGOBLIN_BOMB:
            if (!g_projectile.active) {
                g_projectile.active  = 1;
                g_projectile.x       = p->x;
                g_projectile.y       = p->y - (8 << 4);
                g_projectile.vx      = (p->facing == FACING_RIGHT) ?  BOMB_VX : -BOMB_VX;
                g_projectile.vy      = BOMB_VY_INITIAL;   // upward arc
                g_projectile.type    = PROJ_PUMPKIN_BOMB;
                g_projectile.damage  = def->damage;
                g_projectile.radius  = BOMB_BLAST_RADIUS;
                g_projectile.timer   = BOMB_MAX_FLIGHT;
            }
            break;

        // ── Doppelganger Clone: summon fighting clone for N frames ────
        case ABILITY_DOPPELGANGER_CLONE:
            if (!g_clone.active) {
                g_clone.active      = 1;
                g_clone.x           = p->x + ((p->facing == FACING_RIGHT) ? (24 << 4) : -(24 << 4));
                g_clone.y           = p->y;
                g_clone.facing      = p->facing;
                g_clone.hp          = CLONE_HP;
                g_clone.timer       = CLONE_DURATION;
                g_clone.target_idx  = 0xFF;  // no target yet
                g_clone.atk_timer   = 0;
            }
            break;
    }

    return 1;
}

// ────────────────────────────────────────────────────────────
//  UPDATE  -  update projectile + clone each frame
// ────────────────────────────────────────────────────────────
void boss_abilities_update(EnemyState *enemies, uint8_t enemy_count) {

    // Cooldown
    if (g_abilities.select_cooldown > 0) g_abilities.select_cooldown--;
    if (notify_timer > 0) notify_timer--;

    // ── Pumpkin Bomb ──────────────────────────────────────────
    if (g_projectile.active) {
        g_projectile.x += (int16_t)(g_projectile.vx << 2);   // fixed point
        g_projectile.y += (int16_t)(g_projectile.vy << 2);
        g_projectile.vy += BOMB_GRAVITY;   // arc downward
        g_projectile.timer--;

        // Ground impact or time expiry
        uint8_t explode = 0;
        if ((g_projectile.y >> 4) >= GROUND_BOMB_Y) explode = 1;
        if (g_projectile.timer == 0) explode = 1;

        // Enemy hit detection
        if (!explode) {
            for (uint8_t i = 0; i < enemy_count; i++) {
                if (enemies[i].hp <= 0) continue;
                int16_t dx = (g_projectile.x - enemies[i].x) >> 4;
                int16_t dy = (g_projectile.y - enemies[i].y) >> 4;
                if (dx < 0) dx = -dx;
                if (dy < 0) dy = -dy;
                if (dx < 16 && dy < 24) { explode = 1; break; }
            }
        }

        if (explode) {
            // Blast radius damage
            for (uint8_t i = 0; i < enemy_count; i++) {
                if (enemies[i].hp <= 0) continue;
                int16_t dx = ((g_projectile.x - enemies[i].x) >> 4);
                int16_t dy = ((g_projectile.y - enemies[i].y) >> 4);
                int16_t dist = (dx*dx + dy*dy);
                int16_t r = g_projectile.radius;
                if (dist < r*r) {
                    enemies[i].hp -= g_projectile.damage;
                    enemies[i].state = ESTATE_HURT;
                    enemies[i].hurt_timer = 25;
                    enemies[i].vx = (dx > 0) ? 3 : -3;
                    enemies[i].vy = -4;
                    if (enemies[i].hp <= 0) {
                        enemies[i].state = ESTATE_DEAD;
                        enemies[i].death_timer = 30;
                    }
                }
            }
            g_projectile.active = 0;
        }
    }

    // ── Shadow Clone AI ───────────────────────────────────────
    if (g_clone.active) {
        g_clone.timer--;
        if (g_clone.timer == 0 || g_clone.hp <= 0) {
            g_clone.active = 0;
        } else {
            // Find nearest enemy
            uint8_t best  = 0xFF;
            int16_t best_d = 9999;
            for (uint8_t i = 0; i < enemy_count; i++) {
                if (enemies[i].hp <= 0) continue;
                int16_t dx = (g_clone.x - enemies[i].x) >> 4;
                if (dx < 0) dx = -dx;
                if (dx < best_d) { best_d = dx; best = i; }
            }
            g_clone.target_idx = best;

            if (best != 0xFF) {
                // Move toward target
                int16_t tx = enemies[best].x;
                if (g_clone.x < tx - (8 << 4))  { g_clone.x += (CLONE_SPEED << 4); g_clone.facing = FACING_RIGHT; }
                else if (g_clone.x > tx + (8<<4)){ g_clone.x -= (CLONE_SPEED << 4); g_clone.facing = FACING_LEFT; }

                // Attack if close enough
                if (g_clone.atk_timer > 0) {
                    g_clone.atk_timer--;
                } else if (best_d < 20) {
                    enemies[best].hp -= CLONE_DAMAGE;
                    enemies[best].state = ESTATE_HURT;
                    enemies[best].hurt_timer = 15;
                    g_clone.atk_timer = CLONE_ATK_COOLDOWN;
                    if (enemies[best].hp <= 0) {
                        enemies[best].state = ESTATE_DEAD;
                        enemies[best].death_timer = 30;
                    }
                }
            }
        }
    }
}

// ────────────────────────────────────────────────────────────
//  HUD DRAW HELPERS (call from hud.c)
// ────────────────────────────────────────────────────────────

// Returns active ability icon tile index for HUD rendering
uint8_t boss_ability_hud_icon(void) {
    uint8_t id = boss_ability_active_id();
    switch (id) {
        case ABILITY_NONE:              return HUD_ICON_SPECIAL;       // default star
        case ABILITY_CARNAGE_TENDRILS:  return HUD_ICON_TENDRIL;
        case ABILITY_SHRIEK_SCREAM:     return HUD_ICON_SCREAM;
        case ABILITY_DEMOGOBLIN_BOMB:   return HUD_ICON_BOMB;
        case ABILITY_DOPPELGANGER_CLONE:return HUD_ICON_CLONE;
        default:                        return HUD_ICON_SPECIAL;
    }
}

// Returns 1 if we should show the ability unlock notification
uint8_t boss_ability_show_notify(void) {
    return notify_timer > 0;
}

uint8_t boss_ability_notify_timer(void) {
    return notify_timer;
}

// Returns the name string of the active ability (for HUD text)
const char *boss_ability_name(void) {
    uint8_t id = boss_ability_active_id();
    const BossAbilityDef *def = boss_ability_get_def(id);
    return def->name;
}

// Returns how many ability slots are unlocked (including primary)
uint8_t boss_ability_slot_count(void) {
    return g_abilities.slot_count;
}

uint8_t boss_ability_active_slot(void) {
    return g_abilities.active_slot;
}

// Query projectile for drawing
BossProjectile *boss_ability_get_projectile(void) {
    return &g_projectile;
}

// Query clone for drawing
CloneState *boss_ability_get_clone(void) {
    return &g_clone;
}
