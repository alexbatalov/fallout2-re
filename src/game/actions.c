#include "game/actions.h"

#include <limits.h>
#include <string.h>

#include "game/anim.h"
#include "color.h"
#include "game/combat.h"
#include "game/combatai.h"
#include "game/config.h"
#include "core.h"
#include "game/critter.h"
#include "debug.h"
#include "game/display.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/gmouse.h"
#include "game/gsound.h"
#include "geometry.h"
#include "game/intface.h"
#include "game/item.h"
#include "game/map.h"
#include "memory.h"
#include "game/object.h"
#include "game/perk.h"
#include "game/proto.h"
#include "game/protinst.h"
#include "game/roll.h"
#include "scripts.h"
#include "skill.h"
#include "stat.h"
#include "text_object.h"
#include "tile.h"
#include "trait.h"

static int pick_death(Object* attacker, Object* defender, Object* weapon, int damage, int anim, bool isFallingBack);
static int check_death(Object* obj, int anim, int minViolenceLevel, bool isFallingBack);
static int internal_destroy(Object* a1, Object* a2);
static int show_death(Object* obj, int anim);
static int action_melee(Attack* attack, int a2);
static int action_ranged(Attack* attack, int a2);
static int is_next_to(Object* a1, Object* a2);
static int action_climb_ladder(Object* a1, Object* a2);
static int report_explosion(Attack* attack, Object* a2);
static int finished_explosion(Object* a1, Object* a2);
static int compute_explosion_damage(int min, int max, Object* a3, int* a4);
static int can_talk_to(Object* a1, Object* a2);
static int talk_to(Object* a1, Object* a2);
static int report_dmg(Attack* attack, Object* a2);
static int compute_dmg_damage(int min, int max, Object* obj, int* a4, int damage_type);

// 0x5106D0
static bool action_in_explode = false;

// 0x5106D4
unsigned int rotation = 0;

// 0x5106D8
int obj_fid = -1;

// 0x5106DC
int obj_pid_old = -1;

// TODO: Strange location in debug build, check if it's static in |pick_death|.
//
// 0x5106E0
static int death_2[DAMAGE_TYPE_COUNT] = {
    ANIM_DANCING_AUTOFIRE,
    ANIM_SLICED_IN_HALF,
    ANIM_CHARRED_BODY,
    ANIM_CHARRED_BODY,
    ANIM_ELECTRIFY,
    ANIM_FALL_BACK,
    ANIM_BIG_HOLE,
};

// TODO: Strange location in debug build, check if it's static in |pick_death|.
//
// 0x5106FC
static int death_3[DAMAGE_TYPE_COUNT] = {
    ANIM_CHUNKS_OF_FLESH,
    ANIM_SLICED_IN_HALF,
    ANIM_FIRE_DANCE,
    ANIM_MELTED_TO_NOTHING,
    ANIM_ELECTRIFIED_TO_NOTHING,
    ANIM_FALL_BACK,
    ANIM_EXPLODED_TO_NOTHING,
};

// NOTE: Unused.
//
// 0x410410
void switch_dude()
{
    Object* critter;
    int gender;

    critter = pick_object(OBJ_TYPE_CRITTER, false);
    if (critter != NULL) {
        gender = critterGetStat(critter, STAT_GENDER);
        critterSetBaseStat(obj_dude, STAT_GENDER, gender);

        obj_dude = critter;
        obj_fid = critter->fid;
        obj_pid_old = critter->pid;
        critter->pid = 0x1000000;
    }
}

// 0x410468
int action_knockback(Object* obj, int* anim, int maxDistance, int rotation, int delay)
{
    if (critter_flag_check(obj->pid, CRITTER_NO_KNOCKBACK)) {
        return -1;
    }

    if (*anim == ANIM_FALL_FRONT) {
        int fid = art_id(OBJ_TYPE_CRITTER, obj->fid & 0xFFF, *anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
        if (!art_exists(fid)) {
            *anim = ANIM_FALL_BACK;
        }
    }

    int distance;
    int tile;
    for (distance = 1; distance <= maxDistance; distance++) {
        tile = tileGetTileInDirection(obj->tile, rotation, distance);
        if (obj_blocking_at(obj, tile, obj->elevation) != NULL) {
            distance--;
            break;
        }
    }

    const char* soundEffectName = gsnd_build_character_sfx_name(obj, *anim, CHARACTER_SOUND_EFFECT_KNOCKDOWN);
    register_object_play_sfx(obj, soundEffectName, delay);

    // TODO: Check, probably step back because we've started with 1?
    distance--;

    if (distance <= 0) {
        tile = obj->tile;
        register_object_animate(obj, *anim, 0);
    } else {
        tile = tileGetTileInDirection(obj->tile, rotation, distance);
        register_object_animate_and_move_straight(obj, tile, obj->elevation, *anim, 0);
    }

    return tile;
}

// 0x410568
int action_blood(Object* obj, int anim, int delay)
{

    int violence_level = VIOLENCE_LEVEL_MAXIMUM_BLOOD;
    config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_VIOLENCE_LEVEL_KEY, &violence_level);
    if (violence_level == VIOLENCE_LEVEL_NONE) {
        return anim;
    }

    int bloodyAnim;
    if (anim == ANIM_FALL_BACK) {
        bloodyAnim = ANIM_FALL_BACK_BLOOD;
    } else if (anim == ANIM_FALL_FRONT) {
        bloodyAnim = ANIM_FALL_FRONT_BLOOD;
    } else {
        return anim;
    }

    int fid = art_id(OBJ_TYPE_CRITTER, obj->fid & 0xFFF, bloodyAnim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    if (art_exists(fid)) {
        register_object_animate(obj, bloodyAnim, delay);
    } else {
        bloodyAnim = anim;
    }

    return bloodyAnim;
}

// 0x41060C
static int pick_death(Object* attacker, Object* defender, Object* weapon, int damage, int anim, bool isFallingBack)
{
    int normalViolenceLevelDamageThreshold = 15;
    int maximumBloodViolenceLevelDamageThreshold = 45;

    int damageType = item_w_damage_type(attacker, weapon);

    if (weapon != NULL && weapon->pid == PROTO_ID_MOLOTOV_COCKTAIL) {
        normalViolenceLevelDamageThreshold = 5;
        maximumBloodViolenceLevelDamageThreshold = 15;
        damageType = DAMAGE_TYPE_FIRE;
        anim = ANIM_FIRE_SINGLE;
    }

    if (attacker == obj_dude && perkHasRank(attacker, PERK_PYROMANIAC) && damageType == DAMAGE_TYPE_FIRE) {
        normalViolenceLevelDamageThreshold = 1;
        maximumBloodViolenceLevelDamageThreshold = 1;
    }

    if (weapon != NULL && item_w_perk(weapon) == PERK_WEAPON_FLAMEBOY) {
        normalViolenceLevelDamageThreshold /= 3;
        maximumBloodViolenceLevelDamageThreshold /= 3;
    }

    int violenceLevel = VIOLENCE_LEVEL_MAXIMUM_BLOOD;
    config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_VIOLENCE_LEVEL_KEY, &violenceLevel);

    if (critter_flag_check(defender->pid, CRITTER_SPECIAL_DEATH)) {
        return check_death(defender, ANIM_EXPLODED_TO_NOTHING, VIOLENCE_LEVEL_NORMAL, isFallingBack);
    }

    bool hasBloodyMess = false;
    if (attacker == obj_dude && traitIsSelected(TRAIT_BLOODY_MESS)) {
        hasBloodyMess = true;
    }

    // NOTE: Original code is slightly different. There are lots of jumps and
    // conditions. It's easier to set the default in advance, rather than catch
    // it with bunch of "else" statements.
    int deathAnim = ANIM_FALL_BACK;

    if ((anim == ANIM_THROW_PUNCH && damageType == DAMAGE_TYPE_NORMAL)
        || anim == ANIM_KICK_LEG
        || anim == ANIM_THRUST_ANIM
        || anim == ANIM_SWING_ANIM
        || (anim == ANIM_THROW_ANIM && damageType != DAMAGE_TYPE_EXPLOSION)) {
        if (violenceLevel == VIOLENCE_LEVEL_MAXIMUM_BLOOD && hasBloodyMess) {
            deathAnim = ANIM_BIG_HOLE;
        }
    } else {
        if (anim == ANIM_FIRE_SINGLE && damageType == DAMAGE_TYPE_NORMAL) {
            if (violenceLevel == VIOLENCE_LEVEL_MAXIMUM_BLOOD) {
                if (hasBloodyMess || maximumBloodViolenceLevelDamageThreshold <= damage) {
                    deathAnim = ANIM_BIG_HOLE;
                }
            }
        } else {
            if (violenceLevel > VIOLENCE_LEVEL_MINIMAL && (hasBloodyMess || normalViolenceLevelDamageThreshold <= damage)) {
                if (violenceLevel > VIOLENCE_LEVEL_NORMAL && (hasBloodyMess || maximumBloodViolenceLevelDamageThreshold <= damage)) {
                    deathAnim = death_3[damageType];
                    if (check_death(defender, deathAnim, VIOLENCE_LEVEL_MAXIMUM_BLOOD, isFallingBack) != deathAnim) {
                        deathAnim = death_2[damageType];
                    }
                } else {
                    deathAnim = death_2[damageType];
                }
            }
        }
    }

    if (!isFallingBack && deathAnim == ANIM_FALL_BACK) {
        deathAnim = ANIM_FALL_FRONT;
    }

    return check_death(defender, deathAnim, VIOLENCE_LEVEL_NONE, isFallingBack);
}

// 0x410814
static int check_death(Object* obj, int anim, int minViolenceLevel, bool isFallingBack)
{
    int fid;

    int violenceLevel = VIOLENCE_LEVEL_MAXIMUM_BLOOD;
    config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_VIOLENCE_LEVEL_KEY, &violenceLevel);
    if (violenceLevel >= minViolenceLevel) {
        fid = art_id(OBJ_TYPE_CRITTER, obj->fid & 0xFFF, anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
        if (art_exists(fid)) {
            return anim;
        }
    }

    if (isFallingBack) {
        return ANIM_FALL_BACK;
    }

    fid = art_id(OBJ_TYPE_CRITTER, obj->fid & 0xFFF, ANIM_FALL_FRONT, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    if (art_exists(fid)) {
        return ANIM_FALL_BACK;
    }

    return ANIM_FALL_FRONT;
}

// 0x4108C8
static int internal_destroy(Object* a1, Object* a2)
{
    return obj_destroy(a2);
}

// TODO: Check very carefully, lots of conditions and jumps.
//
// 0x4108D0
void show_damage_to_object(Object* a1, int damage, int flags, Object* weapon, bool isFallingBack, int knockbackDistance, int knockbackRotation, int a8, Object* a9, int a10)
{
    int anim;
    int fid;
    const char* sfx_name;

    if (critter_flag_check(a1->pid, CRITTER_NO_KNOCKBACK)) {
        knockbackDistance = 0;
    }

    anim = FID_ANIM_TYPE(a1->fid);
    if (!critter_is_prone(a1)) {
        if ((flags & DAM_DEAD) != 0) {
            fid = art_id(OBJ_TYPE_MISC, 10, 0, 0, 0);
            if (fid == a9->fid) {
                anim = check_death(a1, ANIM_EXPLODED_TO_NOTHING, VIOLENCE_LEVEL_MAXIMUM_BLOOD, isFallingBack);
            } else if (a9->pid == PROTO_ID_0x20001EB) {
                anim = check_death(a1, ANIM_ELECTRIFIED_TO_NOTHING, VIOLENCE_LEVEL_MAXIMUM_BLOOD, isFallingBack);
            } else if (a9->fid == FID_0x20001F5) {
                anim = check_death(a1, a8, VIOLENCE_LEVEL_MAXIMUM_BLOOD, isFallingBack);
            } else {
                anim = pick_death(a9, a1, weapon, damage, a8, isFallingBack);
            }

            if (anim != ANIM_FIRE_DANCE) {
                if (knockbackDistance != 0 && (anim == ANIM_FALL_FRONT || anim == ANIM_FALL_BACK)) {
                    action_knockback(a1, &anim, knockbackDistance, knockbackRotation, a10);
                    anim = action_blood(a1, anim, -1);
                } else {
                    sfx_name = gsnd_build_character_sfx_name(a1, anim, CHARACTER_SOUND_EFFECT_DIE);
                    register_object_play_sfx(a1, sfx_name, a10);

                    anim = pick_fall(a1, anim);
                    register_object_animate(a1, anim, 0);

                    if (anim == ANIM_FALL_FRONT || anim == ANIM_FALL_BACK) {
                        anim = action_blood(a1, anim, -1);
                    }
                }
            } else {
                fid = art_id(OBJ_TYPE_CRITTER, a1->fid & 0xFFF, ANIM_FIRE_DANCE, (a1->fid & 0xF000) >> 12, a1->rotation + 1);
                if (art_exists(fid)) {
                    sfx_name = gsnd_build_character_sfx_name(a1, anim, CHARACTER_SOUND_EFFECT_UNUSED);
                    register_object_play_sfx(a1, sfx_name, a10);

                    register_object_animate(a1, anim, 0);

                    int randomDistance = roll_random(2, 5);
                    int randomRotation = roll_random(0, 5);

                    while (randomDistance > 0) {
                        int tile = tileGetTileInDirection(a1->tile, randomRotation, randomDistance);
                        Object* v35 = NULL;
                        make_straight_path(a1, a1->tile, tile, NULL, &v35, 4);
                        if (v35 == NULL) {
                            register_object_turn_towards(a1, tile);
                            register_object_move_straight_to_tile(a1, tile, a1->elevation, anim, 0);
                            break;
                        }
                        randomDistance--;
                    }
                }

                anim = ANIM_BURNED_TO_NOTHING;
                sfx_name = gsnd_build_character_sfx_name(a1, anim, CHARACTER_SOUND_EFFECT_UNUSED);
                register_object_play_sfx(a1, sfx_name, -1);
                register_object_animate(a1, anim, 0);
            }
        } else {
            if ((flags & (DAM_KNOCKED_OUT | DAM_KNOCKED_DOWN)) != 0) {
                anim = isFallingBack ? ANIM_FALL_BACK : ANIM_FALL_FRONT;
                sfx_name = gsnd_build_character_sfx_name(a1, anim, CHARACTER_SOUND_EFFECT_UNUSED);
                register_object_play_sfx(a1, sfx_name, a10);
                if (knockbackDistance != 0) {
                    action_knockback(a1, &anim, knockbackDistance, knockbackRotation, 0);
                } else {
                    anim = pick_fall(a1, anim);
                    register_object_animate(a1, anim, 0);
                }
            } else if ((flags & DAM_ON_FIRE) != 0 && art_exists(art_id(OBJ_TYPE_CRITTER, a1->fid & 0xFFF, ANIM_FIRE_DANCE, (a1->fid & 0xF000) >> 12, a1->rotation + 1))) {
                register_object_animate(a1, ANIM_FIRE_DANCE, a10);

                fid = art_id(OBJ_TYPE_CRITTER, a1->fid & 0xFFF, ANIM_STAND, (a1->fid & 0xF000) >> 12, a1->rotation + 1);
                register_object_change_fid(a1, fid, -1);
            } else {
                if (knockbackDistance != 0) {
                    anim = isFallingBack ? ANIM_FALL_BACK : ANIM_FALL_FRONT;
                    action_knockback(a1, &anim, knockbackDistance, knockbackRotation, a10);
                    if (anim == ANIM_FALL_BACK) {
                        register_object_animate(a1, ANIM_BACK_TO_STANDING, -1);
                    } else {
                        register_object_animate(a1, ANIM_PRONE_TO_STANDING, -1);
                    }
                } else {
                    if (isFallingBack || !art_exists(art_id(OBJ_TYPE_CRITTER, a1->fid & 0xFFF, ANIM_HIT_FROM_BACK, (a1->fid & 0xF000) >> 12, a1->rotation + 1))) {
                        anim = ANIM_HIT_FROM_FRONT;
                    } else {
                        anim = ANIM_HIT_FROM_BACK;
                    }

                    sfx_name = gsnd_build_character_sfx_name(a1, anim, CHARACTER_SOUND_EFFECT_UNUSED);
                    register_object_play_sfx(a1, sfx_name, a10);

                    register_object_animate(a1, anim, 0);
                }
            }
        }
    } else {
        if ((flags & DAM_DEAD) != 0 && (a1->data.critter.combat.results & DAM_DEAD) == 0) {
            anim = action_blood(a1, anim, a10);
        } else {
            return;
        }
    }

    if (weapon != NULL) {
        if ((flags & DAM_EXPLODE) != 0) {
            register_object_must_call(a1, weapon, obj_drop, -1);
            fid = art_id(OBJ_TYPE_MISC, 10, 0, 0, 0);
            register_object_change_fid(weapon, fid, 0);
            register_object_animate_and_hide(weapon, ANIM_STAND, 0);

            sfx_name = gsnd_build_weapon_sfx_name(WEAPON_SOUND_EFFECT_HIT, weapon, HIT_MODE_RIGHT_WEAPON_PRIMARY, a1);
            register_object_play_sfx(weapon, sfx_name, 0);

            register_object_must_erase(weapon);
        } else if ((flags & DAM_DESTROY) != 0) {
            register_object_must_call(a1, weapon, internal_destroy, -1);
        } else if ((flags & DAM_DROP) != 0) {
            register_object_must_call(a1, weapon, obj_drop, -1);
        }
    }

    if ((flags & DAM_DEAD) != 0) {
        // TODO: Get rid of casts.
        register_object_must_call(a1, (void*)anim, (AnimationCallback*)show_death, -1);
    }
}

// 0x410E24
static int show_death(Object* obj, int anim)
{
    Rect v7;
    Rect v8;
    int fid;

    obj_bound(obj, &v8);
    if (anim < 48 && anim > 63) {
        fid = art_id(OBJ_TYPE_CRITTER, obj->fid & 0xFFF, anim + 28, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
        if (obj_change_fid(obj, fid, &v7) == 0) {
            rectUnion(&v8, &v7, &v8);
        }

        if (obj_set_frame(obj, 0, &v7) == 0) {
            rectUnion(&v8, &v7, &v8);
        }
    }

    if (critter_flag_check(obj->pid, CRITTER_FLAT) == 0) {
        obj->flags |= OBJECT_NO_BLOCK;
        if (obj_toggle_flat(obj, &v7) == 0) {
            rectUnion(&v8, &v7, &v8);
        }
    }

    if (obj_turn_off_outline(obj, &v7) == 0) {
        rectUnion(&v8, &v7, &v8);
    }

    if (anim >= 30 && anim <= 31 && critter_flag_check(obj->pid, CRITTER_SPECIAL_DEATH) == 0 && critter_flag_check(obj->pid, CRITTER_NO_DROP) == 0) {
        item_drop_all(obj, obj->tile);
    }

    tileWindowRefreshRect(&v8, obj->elevation);

    return 0;
}

// NOTE: Unused.
//
// 0x410F48
int show_damage_target(Attack* attack)
{
    int frontHit;

    if (FID_TYPE(attack->defender->fid) == OBJ_TYPE_CRITTER) {
        // NOTE: Uninline.
        frontHit = is_hit_from_front(attack->attacker, attack->defender);

        register_begin(ANIMATION_REQUEST_RESERVED);
        register_priority(1);
        show_damage_to_object(attack->defender,
            attack->defenderDamage,
            attack->defenderFlags,
            attack->weapon,
            frontHit,
            attack->defenderKnockback,
            tileGetRotationTo(attack->attacker->tile, attack->defender->tile),
            item_w_anim(attack->attacker, attack->hitMode),
            attack->attacker,
            0);
        register_end();
    }

    return 0;
}

// 0x410FEC
int show_damage_extras(Attack* attack)
{
    int v6;
    int v8;
    int v9;

    for (int index = 0; index < attack->extrasLength; index++) {
        Object* obj = attack->extras[index];
        if (FID_TYPE(obj->fid) == OBJ_TYPE_CRITTER) {
            int delta = attack->attacker->rotation - obj->rotation;
            if (delta < 0) {
                delta = -delta;
            }

            v6 = delta != 0 && delta != 1 && delta != 5;
            register_begin(ANIMATION_REQUEST_RESERVED);
            register_priority(1);
            v8 = item_w_anim(attack->attacker, attack->hitMode);
            v9 = tileGetRotationTo(attack->attacker->tile, obj->tile);
            show_damage_to_object(obj, attack->extrasDamage[index], attack->extrasFlags[index], attack->weapon, v6, attack->extrasKnockback[index], v9, v8, attack->attacker, 0);
            register_end();
        }
    }

    return 0;
}

// 0x4110AC
void show_damage(Attack* attack, int a2, int a3)
{
    int v5;
    int v14;
    int v17;
    int v15;

    v5 = a3;
    for (int index = 0; index < attack->extrasLength; index++) {
        Object* object = attack->extras[index];
        if (FID_TYPE(object->fid) == OBJ_TYPE_CRITTER) {
            register_ping(2, v5);
            v5 = 0;
        }
    }

    if ((attack->attackerFlags & DAM_HIT) == 0) {
        if ((attack->attackerFlags & DAM_CRITICAL) != 0) {
            show_damage_to_object(attack->attacker, attack->attackerDamage, attack->attackerFlags, attack->weapon, 1, 0, 0, a2, attack->attacker, -1);
        } else if ((attack->attackerFlags & DAM_BACKWASH) != 0) {
            show_damage_to_object(attack->attacker, attack->attackerDamage, attack->attackerFlags, attack->weapon, 1, 0, 0, a2, attack->attacker, -1);
        }
    } else {
        if (attack->defender != NULL) {
            // TODO: Looks very similar to show_damage_extras.
            int delta = attack->defender->rotation - attack->attacker->rotation;
            if (delta < 0) {
                delta = -delta;
            }

            v15 = delta != 0 && delta != 1 && delta != 5;

            if (FID_TYPE(attack->defender->fid) == OBJ_TYPE_CRITTER) {
                if (attack->attacker->fid == 33554933) {
                    v14 = tileGetRotationTo(attack->attacker->tile, attack->defender->tile);
                    show_damage_to_object(attack->defender, attack->defenderDamage, attack->defenderFlags, attack->weapon, v15, attack->defenderKnockback, v14, a2, attack->attacker, a3);
                } else {
                    v17 = item_w_anim(attack->attacker, attack->hitMode);
                    v14 = tileGetRotationTo(attack->attacker->tile, attack->defender->tile);
                    show_damage_to_object(attack->defender, attack->defenderDamage, attack->defenderFlags, attack->weapon, v15, attack->defenderKnockback, v14, v17, attack->attacker, a3);
                }
            } else {
                tileGetRotationTo(attack->attacker->tile, attack->defender->tile);
                item_w_anim(attack->attacker, attack->hitMode);
            }
        }

        if ((attack->attackerFlags & DAM_DUD) != 0) {
            show_damage_to_object(attack->attacker, attack->attackerDamage, attack->attackerFlags, attack->weapon, 1, 0, 0, a2, attack->attacker, -1);
        }
    }
}

// 0x411224
int action_attack(Attack* attack)
{
    if (register_clear(attack->attacker) == -2) {
        return -1;
    }

    if (register_clear(attack->defender) == -2) {
        return -1;
    }

    for (int index = 0; index < attack->extrasLength; index++) {
        if (register_clear(attack->extras[index]) == -2) {
            return -1;
        }
    }

    int anim = item_w_anim(attack->attacker, attack->hitMode);
    if (anim < ANIM_FIRE_SINGLE && anim != ANIM_THROW_ANIM) {
        return action_melee(attack, anim);
    } else {
        return action_ranged(attack, anim);
    }
}

// 0x4112B4
static int action_melee(Attack* attack, int anim)
{
    int fid;
    Art* art;
    CacheEntry* cache_entry;
    int v17;
    int v18;
    int delta;
    int flag;
    const char* sfx_name;
    char sfx_name_temp[16];

    register_begin(ANIMATION_REQUEST_RESERVED);
    register_priority(1);

    fid = art_id(OBJ_TYPE_CRITTER, attack->attacker->fid & 0xFFF, anim, (attack->attacker->fid & 0xF000) >> 12, attack->attacker->rotation + 1);
    art = art_ptr_lock(fid, &cache_entry);
    if (art != NULL) {
        v17 = art_frame_action_frame(art);
    } else {
        v17 = 0;
    }
    art_ptr_unlock(cache_entry);

    tileGetTileInDirection(attack->attacker->tile, attack->attacker->rotation, 1);
    register_object_turn_towards(attack->attacker, attack->defender->tile);

    delta = attack->attacker->rotation - attack->defender->rotation;
    if (delta < 0) {
        delta = -delta;
    }
    flag = delta != 0 && delta != 1 && delta != 5;

    if (anim != ANIM_THROW_PUNCH && anim != ANIM_KICK_LEG) {
        sfx_name = gsnd_build_weapon_sfx_name(WEAPON_SOUND_EFFECT_ATTACK, attack->weapon, attack->hitMode, attack->defender);
    } else {
        sfx_name = gsnd_build_character_sfx_name(attack->attacker, anim, CHARACTER_SOUND_EFFECT_UNUSED);
    }

    strcpy(sfx_name_temp, sfx_name);

    combatai_msg(attack->attacker, attack, AI_MESSAGE_TYPE_ATTACK, 0);

    if (attack->attackerFlags & 0x0300) {
        register_object_play_sfx(attack->attacker, sfx_name_temp, 0);
        if (anim != ANIM_THROW_PUNCH && anim != ANIM_KICK_LEG) {
            sfx_name = gsnd_build_weapon_sfx_name(WEAPON_SOUND_EFFECT_HIT, attack->weapon, attack->hitMode, attack->defender);
        } else {
            sfx_name = gsnd_build_character_sfx_name(attack->attacker, anim, CHARACTER_SOUND_EFFECT_CONTACT);
        }

        strcpy(sfx_name_temp, sfx_name);

        register_object_animate(attack->attacker, anim, 0);
        register_object_play_sfx(attack->attacker, sfx_name_temp, v17);
        show_damage(attack, anim, 0);
    } else {
        if (attack->defender->data.critter.combat.results & 0x03) {
            register_object_play_sfx(attack->attacker, sfx_name_temp, -1);
            register_object_animate(attack->attacker, anim, 0);
        } else {
            fid = art_id(OBJ_TYPE_CRITTER, attack->defender->fid & 0xFFF, ANIM_DODGE_ANIM, (attack->defender->fid & 0xF000) >> 12, attack->defender->rotation + 1);
            art = art_ptr_lock(fid, &cache_entry);
            if (art != NULL) {
                v18 = art_frame_action_frame(art);
                art_ptr_unlock(cache_entry);

                if (v18 <= v17) {
                    register_object_play_sfx(attack->attacker, sfx_name_temp, -1);
                    register_object_animate(attack->attacker, anim, 0);

                    sfx_name = gsnd_build_character_sfx_name(attack->defender, ANIM_DODGE_ANIM, CHARACTER_SOUND_EFFECT_UNUSED);
                    register_object_play_sfx(attack->defender, sfx_name, v17 - v18);
                    register_object_animate(attack->defender, ANIM_DODGE_ANIM, 0);
                } else {
                    sfx_name = gsnd_build_character_sfx_name(attack->defender, ANIM_DODGE_ANIM, CHARACTER_SOUND_EFFECT_UNUSED);
                    register_object_play_sfx(attack->defender, sfx_name, -1);
                    register_object_animate(attack->defender, ANIM_DODGE_ANIM, 0);
                    register_object_play_sfx(attack->attacker, sfx_name_temp, v18 - v17);
                    register_object_animate(attack->attacker, anim, 0);
                }
            }
        }
    }

    if ((attack->attackerFlags & DAM_HIT) != 0) {
        if ((attack->defenderFlags & DAM_DEAD) == 0) {
            combatai_msg(attack->attacker, attack, AI_MESSAGE_TYPE_HIT, -1);
        }
    } else {
        combatai_msg(attack->attacker, attack, AI_MESSAGE_TYPE_MISS, -1);
    }

    if (register_end() == -1) {
        return -1;
    }

    show_damage_extras(attack);

    return 0;
}

// NOTE: Unused.
//
// 0x4115CC
int throw_change_fid(Object* object, int fid)
{
    Rect rect;

    debugPrint("\n[throw_change_fid!]: %d", fid);
    obj_change_fid(object, fid, &rect);
    tileWindowRefreshRect(&rect, map_elevation);

    return 0;
}

// 0x411600
static int action_ranged(Attack* attack, int anim)
{
    Object* neighboors[6];
    memset(neighboors, 0, sizeof(neighboors));

    register_begin(ANIMATION_REQUEST_RESERVED);
    register_priority(1);

    Object* projectile = NULL;
    Object* v50 = NULL;
    int weaponFid = -1;

    Proto* weaponProto;
    Object* weapon = attack->weapon;
    proto_ptr(weapon->pid, &weaponProto);

    int fid = art_id(OBJ_TYPE_CRITTER, attack->attacker->fid & 0xFFF, anim, (attack->attacker->fid & 0xF000) >> 12, attack->attacker->rotation + 1);
    CacheEntry* artHandle;
    Art* art = art_ptr_lock(fid, &artHandle);
    int actionFrame = (art != NULL) ? art_frame_action_frame(art) : 0;
    art_ptr_unlock(artHandle);

    item_w_range(attack->attacker, attack->hitMode);

    int damageType = item_w_damage_type(attack->attacker, attack->weapon);

    tileGetTileInDirection(attack->attacker->tile, attack->attacker->rotation, 1);

    register_object_turn_towards(attack->attacker, attack->defender->tile);

    bool isGrenade = false;
    if (anim == ANIM_THROW_ANIM) {
        if (damageType == DAMAGE_TYPE_EXPLOSION || damageType == DAMAGE_TYPE_PLASMA || damageType == DAMAGE_TYPE_EMP) {
            isGrenade = true;
        }
    } else {
        register_object_animate(attack->attacker, ANIM_POINT, -1);
    }

    combatai_msg(attack->attacker, attack, AI_MESSAGE_TYPE_ATTACK, 0);

    const char* sfx;
    if (((attack->attacker->fid & 0xF000) >> 12) != 0) {
        sfx = gsnd_build_weapon_sfx_name(WEAPON_SOUND_EFFECT_ATTACK, weapon, attack->hitMode, attack->defender);
    } else {
        sfx = gsnd_build_character_sfx_name(attack->attacker, anim, CHARACTER_SOUND_EFFECT_UNUSED);
    }
    register_object_play_sfx(attack->attacker, sfx, -1);

    register_object_animate(attack->attacker, anim, 0);

    if (anim != ANIM_FIRE_CONTINUOUS) {
        if ((attack->attackerFlags & DAM_HIT) != 0 || (attack->attackerFlags & DAM_CRITICAL) == 0) {
            bool l56 = false;

            int projectilePid = item_w_proj_pid(weapon);
            Proto* projectileProto;
            if (proto_ptr(projectilePid, &projectileProto) != -1 && projectileProto->fid != -1) {
                if (anim == ANIM_THROW_ANIM) {
                    projectile = weapon;
                    weaponFid = weapon->fid;
                    int weaponFlags = weapon->flags;

                    int leftItemAction;
                    int rightItemAction;
                    intface_get_item_states(&leftItemAction, &rightItemAction);

                    item_remove_mult(attack->attacker, weapon, 1);
                    v50 = item_replace(attack->attacker, weapon, weaponFlags & OBJECT_IN_ANY_HAND);
                    obj_change_fid(projectile, projectileProto->fid, NULL);
                    cAIPrepWeaponItem(attack->attacker, weapon);

                    if (attack->attacker == obj_dude) {
                        if (v50 == NULL) {
                            if ((weaponFlags & OBJECT_IN_LEFT_HAND) != 0) {
                                leftItemAction = INTERFACE_ITEM_ACTION_DEFAULT;
                            } else if ((weaponFlags & OBJECT_IN_RIGHT_HAND) != 0) {
                                rightItemAction = INTERFACE_ITEM_ACTION_DEFAULT;
                            }
                        }
                        intface_update_items(false, leftItemAction, rightItemAction);
                    }

                    obj_connect(weapon, attack->attacker->tile, attack->attacker->elevation, NULL);
                } else {
                    obj_new(&projectile, projectileProto->fid, -1);
                }

                obj_turn_off(projectile, NULL);

                obj_set_light(projectile, 9, projectile->lightIntensity, NULL);

                int projectileOrigin = combat_bullet_start(attack->attacker, attack->defender);
                obj_move_to_tile(projectile, projectileOrigin, attack->attacker->elevation, NULL);

                int projectileRotation = tileGetRotationTo(attack->attacker->tile, attack->defender->tile);
                obj_set_rotation(projectile, projectileRotation, NULL);

                register_object_funset(projectile, OBJECT_HIDDEN, actionFrame);

                const char* sfx = gsnd_build_weapon_sfx_name(WEAPON_SOUND_EFFECT_AMMO_FLYING, weapon, attack->hitMode, attack->defender);
                register_object_play_sfx(projectile, sfx, 0);

                int v24;
                if ((attack->attackerFlags & DAM_HIT) != 0) {
                    register_object_move_straight_to_tile(projectile, attack->defender->tile, attack->defender->elevation, ANIM_WALK, 0);
                    actionFrame = make_straight_path(projectile, projectileOrigin, attack->defender->tile, NULL, NULL, 32) - 1;
                    v24 = attack->defender->tile;
                } else {
                    register_object_move_straight_to_tile(projectile, attack->tile, attack->defender->elevation, ANIM_WALK, 0);
                    actionFrame = 0;
                    v24 = attack->tile;
                }

                if (isGrenade || damageType == DAMAGE_TYPE_EXPLOSION) {
                    if ((attack->attackerFlags & DAM_DROP) == 0) {
                        int explosionFrmId;
                        if (isGrenade) {
                            switch (damageType) {
                            case DAMAGE_TYPE_EMP:
                                explosionFrmId = 2;
                                break;
                            case DAMAGE_TYPE_PLASMA:
                                explosionFrmId = 31;
                                break;
                            default:
                                explosionFrmId = 29;
                                break;
                            }
                        } else {
                            explosionFrmId = 10;
                        }

                        if (isGrenade) {
                            register_object_change_fid(projectile, weaponFid, -1);
                        }

                        int explosionFid = art_id(OBJ_TYPE_MISC, explosionFrmId, 0, 0, 0);
                        register_object_change_fid(projectile, explosionFid, -1);

                        const char* sfx = gsnd_build_weapon_sfx_name(WEAPON_SOUND_EFFECT_HIT, weapon, attack->hitMode, attack->defender);
                        register_object_play_sfx(projectile, sfx, 0);

                        register_object_animate_and_hide(projectile, ANIM_STAND, 0);

                        for (int rotation = 0; rotation < ROTATION_COUNT; rotation++) {
                            if (obj_new(&(neighboors[rotation]), explosionFid, -1) != -1) {
                                obj_turn_off(neighboors[rotation], NULL);

                                int v31 = tileGetTileInDirection(v24, rotation, 1);
                                obj_move_to_tile(neighboors[rotation], v31, projectile->elevation, NULL);

                                int delay;
                                if (rotation != ROTATION_NE) {
                                    delay = 0;
                                } else {
                                    if (damageType == DAMAGE_TYPE_PLASMA) {
                                        delay = 4;
                                    } else {
                                        delay = 2;
                                    }
                                }

                                register_object_funset(neighboors[rotation], OBJECT_HIDDEN, delay);
                                register_object_animate_and_hide(neighboors[rotation], ANIM_STAND, 0);
                            }
                        }

                        l56 = true;
                    }
                } else {
                    if (anim != ANIM_THROW_ANIM) {
                        register_object_must_erase(projectile);
                    }
                }

                if (!l56) {
                    const char* sfx = gsnd_build_weapon_sfx_name(WEAPON_SOUND_EFFECT_HIT, weapon, attack->hitMode, attack->defender);
                    register_object_play_sfx(weapon, sfx, actionFrame);
                }

                actionFrame = 0;
            } else {
                if ((attack->attackerFlags & DAM_HIT) == 0) {
                    Object* defender = attack->defender;
                    if ((defender->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_KNOCKED_DOWN)) == 0) {
                        register_object_animate(defender, ANIM_DODGE_ANIM, actionFrame);
                        l56 = true;
                    }
                }
            }
        }
    }

    show_damage(attack, anim, actionFrame);

    if ((attack->attackerFlags & DAM_HIT) == 0) {
        combatai_msg(attack->defender, attack, AI_MESSAGE_TYPE_MISS, -1);
    } else {
        if ((attack->defenderFlags & DAM_DEAD) == 0) {
            combatai_msg(attack->defender, attack, AI_MESSAGE_TYPE_HIT, -1);
        }
    }

    if (projectile != NULL && (isGrenade || damageType == DAMAGE_TYPE_EXPLOSION)) {
        register_object_must_erase(projectile);
    } else if (anim == ANIM_THROW_ANIM && projectile != NULL) {
        register_object_change_fid(projectile, weaponFid, -1);
    }

    for (int rotation = 0; rotation < ROTATION_COUNT; rotation++) {
        if (neighboors[rotation] != NULL) {
            register_object_must_erase(neighboors[rotation]);
        }
    }

    if ((attack->attackerFlags & (DAM_KNOCKED_OUT | DAM_KNOCKED_DOWN | DAM_DEAD)) == 0) {
        if (anim == ANIM_THROW_ANIM) {
            bool l9 = false;
            if (v50 != NULL) {
                int v38 = item_w_anim_code(v50);
                if (v38 != 0) {
                    register_object_take_out(attack->attacker, v38, -1);
                    l9 = true;
                }
            }

            if (!l9) {
                int fid = art_id(OBJ_TYPE_CRITTER, attack->attacker->fid & 0xFFF, ANIM_STAND, 0, attack->attacker->rotation + 1);
                register_object_change_fid(attack->attacker, fid, -1);
            }
        } else {
            register_object_animate(attack->attacker, ANIM_UNPOINT, -1);
        }
    }

    if (register_end() == -1) {
        debugPrint("Something went wrong with a ranged attack sequence!\n");
        if (projectile != NULL && (isGrenade || damageType == DAMAGE_TYPE_EXPLOSION || anim != ANIM_THROW_ANIM)) {
            obj_erase_object(projectile, NULL);
        }

        for (int rotation = 0; rotation < ROTATION_COUNT; rotation++) {
            if (neighboors[rotation] != NULL) {
                obj_erase_object(neighboors[rotation], NULL);
            }
        }

        return -1;
    }

    show_damage_extras(attack);

    return 0;
}

// 0x411D68
static int is_next_to(Object* a1, Object* a2)
{
    if (obj_dist(a1, a2) > 1) {
        if (a2 == obj_dude) {
            MessageListItem messageListItem;
            // You cannot get there.
            messageListItem.num = 2000;
            if (message_search(&misc_message_file, &messageListItem)) {
                display_print(messageListItem.text);
            }
        }
        return -1;
    }

    return 0;
}

// 0x411DB4
static int action_climb_ladder(Object* a1, Object* a2)
{
    if (a1 == obj_dude) {
        int anim = FID_ANIM_TYPE(obj_dude->fid);
        if (anim == ANIM_WALK || anim == ANIM_RUNNING) {
            register_clear(obj_dude);
        }
    }

    int animationRequestOptions;
    int actionPoints;
    if (isInCombat()) {
        animationRequestOptions = ANIMATION_REQUEST_RESERVED;
        actionPoints = a1->data.critter.combat.ap;
    } else {
        animationRequestOptions = ANIMATION_REQUEST_UNRESERVED;
        actionPoints = -1;
    }

    if (a1 == obj_dude) {
        animationRequestOptions = ANIMATION_REQUEST_RESERVED;
    }

    animationRequestOptions |= ANIMATION_REQUEST_NO_STAND;
    register_begin(animationRequestOptions);

    int tile = tileGetTileInDirection(a2->tile, ROTATION_SE, 1);
    if (actionPoints != -1 || obj_dist(a1, a2) < 5) {
        register_object_move_to_tile(a1, tile, a2->elevation, actionPoints, 0);
    } else {
        register_object_run_to_tile(a1, tile, a2->elevation, actionPoints, 0);
    }

    register_object_must_call(a1, a2, is_next_to, -1);
    register_object_turn_towards(a1, a2->tile);
    register_object_must_call(a1, a2, check_scenery_ap_cost, -1);

    int weaponAnimationCode = (a1->fid & 0xF000) >> 12;
    if (weaponAnimationCode != 0) {
        const char* puttingAwaySfx = gsnd_build_character_sfx_name(a1, ANIM_PUT_AWAY, CHARACTER_SOUND_EFFECT_UNUSED);
        register_object_play_sfx(a1, puttingAwaySfx, -1);
        register_object_animate(a1, ANIM_PUT_AWAY, 0);
    }

    const char* climbingSfx = gsnd_build_character_sfx_name(a1, ANIM_CLIMB_LADDER, CHARACTER_SOUND_EFFECT_UNUSED);
    register_object_play_sfx(a1, climbingSfx, -1);
    register_object_animate(a1, ANIM_CLIMB_LADDER, 0);
    register_object_call(a1, a2, obj_use, -1);

    if (weaponAnimationCode != 0) {
        register_object_take_out(a1, weaponAnimationCode, -1);
    }

    return register_end();
}

// 0x411F2C
int a_use_obj(Object* a1, Object* a2, Object* a3)
{
    Proto* proto = NULL;
    int type = FID_TYPE(a2->fid);
    int sceneryType = -1;
    if (type == OBJ_TYPE_SCENERY) {
        if (proto_ptr(a2->pid, &proto) == -1) {
            return -1;
        }

        sceneryType = proto->scenery.type;
    }

    if (sceneryType != SCENERY_TYPE_LADDER_UP || a3 != NULL) {
        if (a1 == obj_dude) {
            int anim = FID_ANIM_TYPE(obj_dude->fid);
            if (anim == ANIM_WALK || anim == ANIM_RUNNING) {
                register_clear(obj_dude);
            }
        }

        int animationRequestOptions;
        int actionPoints;
        if (isInCombat()) {
            animationRequestOptions = ANIMATION_REQUEST_RESERVED;
            actionPoints = a1->data.critter.combat.ap;
        } else {
            animationRequestOptions = ANIMATION_REQUEST_UNRESERVED;
            actionPoints = -1;
        }

        if (a1 == obj_dude) {
            animationRequestOptions = ANIMATION_REQUEST_RESERVED;
        }

        register_begin(animationRequestOptions);

        if (actionPoints != -1 || obj_dist(a1, a2) < 5) {
            register_object_move_to_object(a1, a2, actionPoints, 0);
        } else {
            register_object_run_to_object(a1, a2, -1, 0);
        }

        register_object_must_call(a1, a2, is_next_to, -1);

        if (a3 == NULL) {
            register_object_call(a1, a2, check_scenery_ap_cost, -1);
        }

        int a2a = (a1->fid & 0xF000) >> 12;
        if (a2a != 0) {
            const char* sfx = gsnd_build_character_sfx_name(a1, ANIM_PUT_AWAY, CHARACTER_SOUND_EFFECT_UNUSED);
            register_object_play_sfx(a1, sfx, -1);
            register_object_animate(a1, ANIM_PUT_AWAY, 0);
        }

        int anim;
        int objectType = FID_TYPE(a2->fid);
        if (objectType == OBJ_TYPE_CRITTER && critter_is_prone(a2)) {
            anim = ANIM_MAGIC_HANDS_GROUND;
        } else if (objectType == OBJ_TYPE_SCENERY && (proto->scenery.extendedFlags & 0x01) != 0) {
            anim = ANIM_MAGIC_HANDS_GROUND;
        } else {
            anim = ANIM_MAGIC_HANDS_MIDDLE;
        }

        if (sceneryType != SCENERY_TYPE_STAIRS && a3 == NULL) {
            register_object_animate(a1, anim, -1);
        }

        if (a3 != NULL) {
            // TODO: Get rid of cast.
            register_object_call3(a1, a2, a3, obj_use_item_on, -1);
        } else {
            register_object_call(a1, a2, obj_use, -1);
        }

        if (a2a != 0) {
            register_object_take_out(a1, a2a, -1);
        }

        return register_end();
    }

    return action_climb_ladder(a1, a2);
}

// 0x411F2C
int action_use_an_item_on_object(Object* a1, Object* a2, Object* a3)
{
    return a_use_obj(a1, a2, a3);
}

// 0x412114
int action_use_an_object(Object* a1, Object* a2)
{
    return a_use_obj(a1, a2, NULL);
}

// NOTE: Unused.
//
// 0x412120
int get_an_object(Object* item)
{
    return action_get_an_object(obj_dude, item);
}

// 0x412134
int action_get_an_object(Object* critter, Object* item)
{
    if (FID_TYPE(item->fid) != OBJ_TYPE_ITEM) {
        return -1;
    }

    if (critter == obj_dude) {
        int animationCode = FID_ANIM_TYPE(obj_dude->fid);
        if (animationCode == ANIM_WALK || animationCode == ANIM_RUNNING) {
            register_clear(obj_dude);
        }
    }

    if (isInCombat()) {
        register_begin(ANIMATION_REQUEST_RESERVED);
        register_object_move_to_object(critter, item, critter->data.critter.combat.ap, 0);
    } else {
        register_begin(critter == obj_dude ? ANIMATION_REQUEST_RESERVED : ANIMATION_REQUEST_UNRESERVED);
        if (obj_dist(critter, item) >= 5) {
            register_object_run_to_object(critter, item, -1, 0);
        } else {
            register_object_move_to_object(critter, item, -1, 0);
        }
    }

    register_object_must_call(critter, item, is_next_to, -1);
    register_object_call(critter, item, check_scenery_ap_cost, -1);

    Proto* itemProto;
    proto_ptr(item->pid, &itemProto);

    if (itemProto->item.type != ITEM_TYPE_CONTAINER || proto_action_can_pickup(item->pid)) {
        register_object_animate(critter, ANIM_MAGIC_HANDS_GROUND, 0);

        int fid = art_id(OBJ_TYPE_CRITTER, critter->fid & 0xFFF, ANIM_MAGIC_HANDS_GROUND, (critter->fid & 0xF000) >> 12, critter->rotation + 1);

        int actionFrame;
        CacheEntry* cacheEntry;
        Art* art = art_ptr_lock(fid, &cacheEntry);
        if (art != NULL) {
            actionFrame = art_frame_action_frame(art);
        } else {
            actionFrame = -1;
        }

        char sfx[16];
        if (art_get_base_name(FID_TYPE(item->fid), item->fid & 0xFFF, sfx) == 0) {
            // NOTE: looks like they copy sfx one more time, what for?
            register_object_play_sfx(item, sfx, actionFrame);
        }

        register_object_call(critter, item, obj_pickup, actionFrame);
    } else {
        int weaponAnimationCode = (critter->fid & 0xF000) >> 12;
        if (weaponAnimationCode != 0) {
            const char* sfx = gsnd_build_character_sfx_name(critter, ANIM_PUT_AWAY, CHARACTER_SOUND_EFFECT_UNUSED);
            register_object_play_sfx(critter, sfx, -1);
            register_object_animate(critter, ANIM_PUT_AWAY, -1);
        }

        // ground vs middle animation
        int anim = (itemProto->item.data.container.openFlags & 0x01) == 0
            ? ANIM_MAGIC_HANDS_MIDDLE
            : ANIM_MAGIC_HANDS_GROUND;
        register_object_animate(critter, anim, 0);

        int fid = art_id(OBJ_TYPE_CRITTER, critter->fid & 0xFFF, anim, 0, critter->rotation + 1);

        int actionFrame;
        CacheEntry* cacheEntry;
        Art* art = art_ptr_lock(fid, &cacheEntry);
        if (art == NULL) {
            actionFrame = art_frame_action_frame(art);
            art_ptr_unlock(cacheEntry);
        } else {
            actionFrame = -1;
        }

        if (item->frame != 1) {
            register_object_call(critter, item, obj_use_container, actionFrame);
        }

        if (weaponAnimationCode != 0) {
            register_object_take_out(critter, weaponAnimationCode, -1);
        }

        if (item->frame == 0 || item->frame == 1) {
            register_object_call(critter, item, scriptsRequestLooting, -1);
        }
    }

    return register_end();
}

// TODO: Looks like the name is a little misleading, container can only be a
// critter, which is enforced by this function as well as at the call sites.
// Used to loot corpses, so probably should be something like actionLootCorpse.
// Check if it can be called with a living critter.
//
// 0x4123E8
int action_loot_container(Object* critter, Object* container)
{
    if (FID_TYPE(container->fid) != OBJ_TYPE_CRITTER) {
        return -1;
    }

    if (critter == obj_dude) {
        int anim = FID_ANIM_TYPE(obj_dude->fid);
        if (anim == ANIM_WALK || anim == ANIM_RUNNING) {
            register_clear(obj_dude);
        }
    }

    if (isInCombat()) {
        register_begin(ANIMATION_REQUEST_RESERVED);
        register_object_move_to_object(critter, container, critter->data.critter.combat.ap, 0);
    } else {
        register_begin(critter == obj_dude ? ANIMATION_REQUEST_RESERVED : ANIMATION_REQUEST_UNRESERVED);

        if (obj_dist(critter, container) < 5) {
            register_object_move_to_object(critter, container, -1, 0);
        } else {
            register_object_run_to_object(critter, container, -1, 0);
        }
    }

    register_object_must_call(critter, container, is_next_to, -1);
    register_object_call(critter, container, check_scenery_ap_cost, -1);
    register_object_call(critter, container, scriptsRequestLooting, -1);
    return register_end();
}

// 0x4124E0
int action_skill_use(int skill)
{
    if (skill == SKILL_SNEAK) {
        register_clear(obj_dude);
        pc_flag_toggle(DUDE_STATE_SNEAKING);
        return 0;
    }

    return -1;
}

// NOTE: Inlined.
//
// 0x412500
int action_use_skill_in_combat_error(Object* critter)
{
    MessageListItem messageListItem;

    if (critter == obj_dude) {
        messageListItem.num = 902;
        if (message_search(&proto_main_msg_file, &messageListItem) == 1) {
            display_print(messageListItem.text);
        }
    }

    return -1;
}

// skill_use
// 0x41255C
int action_use_skill_on(Object* a1, Object* a2, int skill)
{
    switch (skill) {
    case SKILL_FIRST_AID:
    case SKILL_DOCTOR:
        if (isInCombat()) {
            // NOTE: Uninline.
            return action_use_skill_in_combat_error(a1);
        }

        if (PID_TYPE(a2->pid) != OBJ_TYPE_CRITTER) {
            return -1;
        }
        break;
    case SKILL_LOCKPICK:
        if (isInCombat()) {
            // NOTE: Uninline.
            return action_use_skill_in_combat_error(a1);
        }

        if (PID_TYPE(a2->pid) != OBJ_TYPE_ITEM && PID_TYPE(a2->pid) != OBJ_TYPE_SCENERY) {
            return -1;
        }

        break;
    case SKILL_STEAL:
        if (isInCombat()) {
            // NOTE: Uninline.
            return action_use_skill_in_combat_error(a1);
        }

        if (PID_TYPE(a2->pid) != OBJ_TYPE_ITEM && PID_TYPE(a2->pid) != OBJ_TYPE_CRITTER) {
            return -1;
        }

        if (a2 == a1) {
            return -1;
        }

        break;
    case SKILL_TRAPS:
        if (isInCombat()) {
            // NOTE: Uninline.
            return action_use_skill_in_combat_error(a1);
        }

        if (PID_TYPE(a2->pid) == OBJ_TYPE_CRITTER) {
            return -1;
        }

        break;
    case SKILL_SCIENCE:
    case SKILL_REPAIR:
        if (isInCombat()) {
            // NOTE: Uninline.
            return action_use_skill_in_combat_error(a1);
        }

        if (PID_TYPE(a2->pid) != OBJ_TYPE_CRITTER) {
            break;
        }

        if (critterGetKillType(a2) == KILL_TYPE_ROBOT) {
            break;
        }

        if (critterGetKillType(a2) == KILL_TYPE_BRAHMIN
            && skill == SKILL_SCIENCE) {
            break;
        }

        return -1;
    case SKILL_SNEAK:
        pc_flag_toggle(DUDE_STATE_SNEAKING);
        return 0;
    default:
        debugPrint("\nskill_use: invalid skill used.");
    }

    // Performer is either dude, or party member who's best at the specified
    // skill in entire party, and this skill is his/her own best.
    Object* performer = obj_dude;

    if (a1 == obj_dude) {
        Object* partyMember = partyMemberWithHighestSkill(skill);

        if (partyMember == obj_dude) {
            partyMember = NULL;
        }

        // Only dude can perform stealing.
        if (skill == SKILL_STEAL) {
            partyMember = NULL;
        }

        if (partyMember != NULL) {
            if (partyMemberSkill(partyMember) != skill) {
                partyMember = NULL;
            }
        }

        if (partyMember != NULL) {
            performer = partyMember;
            int anim = FID_ANIM_TYPE(partyMember->fid);
            if (anim != ANIM_WALK && anim != ANIM_RUNNING) {
                if (anim != ANIM_STAND) {
                    performer = obj_dude;
                    partyMember = NULL;
                }
            } else {
                register_clear(partyMember);
            }
        }

        if (partyMember != NULL) {
            bool v32 = false;
            if (obj_dist(obj_dude, a2) <= 1) {
                v32 = true;
            }

            char* msg = skillsGetGenericResponse(partyMember, v32);

            Rect rect;
            if (textObjectAdd(partyMember, msg, 101, colorTable[32747], colorTable[0], &rect) == 0) {
                tileWindowRefreshRect(&rect, map_elevation);
            }

            if (v32) {
                performer = obj_dude;
                partyMember = NULL;
            }
        }

        if (partyMember == NULL) {
            int anim = FID_ANIM_TYPE(performer->fid);
            if (anim == ANIM_WALK || anim == ANIM_RUNNING) {
                register_clear(performer);
            }
        }
    }

    if (isInCombat()) {
        register_begin(ANIMATION_REQUEST_RESERVED);
        register_object_move_to_object(performer, a2, performer->data.critter.combat.ap, 0);
    } else {
        register_begin(a1 == obj_dude ? ANIMATION_REQUEST_RESERVED : ANIMATION_REQUEST_UNRESERVED);
        if (a2 != obj_dude) {
            if (obj_dist(performer, a2) >= 5) {
                register_object_run_to_object(performer, a2, -1, 0);
            } else {
                register_object_move_to_object(performer, a2, -1, 0);
            }
        }
    }

    register_object_must_call(performer, a2, is_next_to, -1);

    int anim = (FID_TYPE(a2->fid) == OBJ_TYPE_CRITTER && critter_is_prone(a2)) ? ANIM_MAGIC_HANDS_GROUND : ANIM_MAGIC_HANDS_MIDDLE;
    int fid = art_id(OBJ_TYPE_CRITTER, performer->fid & 0xFFF, anim, 0, performer->rotation + 1);

    CacheEntry* artHandle;
    Art* art = art_ptr_lock(fid, &artHandle);
    if (art != NULL) {
        art_frame_action_frame(art);
        art_ptr_unlock(artHandle);
    }

    register_object_animate(performer, anim, -1);
    // TODO: Get rid of casts.
    register_object_call3(performer, a2, (void*)skill, (AnimationCallback3*)obj_use_skill_on, -1);
    return register_end();
}

// NOTE: Unused.
//
// 0x4129CC
Object* pick_object(int objectType, bool a2)
{
    Object* foundObject;
    int mouseEvent;
    int keyCode;

    foundObject = NULL;

    do {
        _get_input();
    } while ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0);

    gmouse_set_cursor(MOUSE_CURSOR_PLUS);
    gmouse_3d_off();

    do {
        if (_get_input() == -2) {
            mouseEvent = mouse_get_buttons();
            if ((mouseEvent & MOUSE_EVENT_LEFT_BUTTON_UP) != 0) {
                keyCode = 0;
                foundObject = object_under_mouse(objectType, a2, map_elevation);
                break;
            }

            if ((mouseEvent & MOUSE_EVENT_RIGHT_BUTTON_DOWN) != 0) {
                keyCode = KEY_ESCAPE;
                break;
            }
        }
    } while (game_user_wants_to_quit == 0);

    gmouse_set_cursor(MOUSE_CURSOR_ARROW);
    gmouse_3d_on();

    if (keyCode == KEY_ESCAPE) {
        return NULL;
    }

    return foundObject;
}

// NOTE: Unused.
//
// 0x412A54
int pick_hex()
{
    int elevation;
    int inputEvent;
    int tile;
    Rect rect;

    elevation = map_elevation;

    while (1) {
        inputEvent = _get_input();
        if (inputEvent == -2) {
            break;
        }

        if (inputEvent == KEY_CTRL_ARROW_RIGHT) {
            rotation++;
            if (rotation > 5) {
                rotation = 0;
            }

            obj_set_rotation(obj_mouse, rotation, &rect);
            tileWindowRefreshRect(&rect, obj_mouse->elevation);
        }

        if (inputEvent == KEY_CTRL_ARROW_LEFT) {
            rotation--;
            if (rotation == -1) {
                rotation = 5;
            }

            obj_set_rotation(obj_mouse, rotation, &rect);
            tileWindowRefreshRect(&rect, obj_mouse->elevation);
        }

        if (inputEvent == KEY_PAGE_UP || inputEvent == KEY_PAGE_DOWN) {
            if (inputEvent == KEY_PAGE_UP) {
                map_set_elevation(map_elevation + 1);
            } else {
                map_set_elevation(map_elevation - 1);
            }

            rect.left = 30;
            rect.top = 62;
            rect.right = 50;
            rect.bottom = 88;
        }

        if (game_user_wants_to_quit != 0) {
            return -1;
        }
    }

    if ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_DOWN) == 0) {
        return -1;
    }

    if (!mouse_click_in(0, 0, _scr_size.right - _scr_size.left, _scr_size.bottom - _scr_size.top - 100)) {
        return -1;
    }

    mouse_get_position(&(rect.left), &(rect.top));

    tile = tileFromScreenXY(rect.left, rect.top, elevation);
    if (tile == -1) {
        return -1;
    }

    return tile;
}

// 0x412BC4
bool is_hit_from_front(Object* a1, Object* a2)
{
    int diff = a1->rotation - a2->rotation;
    if (diff < 0) {
        diff = -diff;
    }

    return diff != 0 && diff != 1 && diff != 5;
}

// 0x412BEC
bool can_see(Object* a1, Object* a2)
{
    int diff;

    diff = a1->rotation - tileGetRotationTo(a1->tile, a2->tile);
    if (diff < 0) {
        diff = -diff;
    }

    return diff == 0 || diff == 1 || diff == 5;
}

// looks like it tries to change fall animation depending on object's current rotation
// 0x412C1C
int pick_fall(Object* obj, int anim)
{
    int i;
    int rotation;
    int tile_num;
    int fid;

    if (anim == ANIM_FALL_FRONT) {
        rotation = obj->rotation;
        for (i = 1; i < 3; i++) {
            tile_num = tileGetTileInDirection(obj->tile, rotation, i);
            if (obj_blocking_at(obj, tile_num, obj->elevation) != NULL) {
                anim = ANIM_FALL_BACK;
                break;
            }
        }
    } else if (anim == ANIM_FALL_BACK) {
        rotation = (obj->rotation + 3) % ROTATION_COUNT;
        for (i = 1; i < 3; i++) {
            tile_num = tileGetTileInDirection(obj->tile, rotation, i);
            if (obj_blocking_at(obj, tile_num, obj->elevation) != NULL) {
                anim = ANIM_FALL_FRONT;
                break;
            }
        }
    }

    if (anim == ANIM_FALL_FRONT) {
        fid = art_id(OBJ_TYPE_CRITTER, obj->fid & 0xFFF, ANIM_FALL_FRONT, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
        if (!art_exists(fid)) {
            anim = ANIM_FALL_BACK;
        }
    }

    return anim;
}

// 0x412CE4
bool action_explode_running()
{
    return action_in_explode;
}

// action_explode
// 0x412CF4
int action_explode(int tile, int elevation, int minDamage, int maxDamage, Object* a5, bool a6)
{
    if (a6 && action_in_explode) {
        return -2;
    }

    Attack* attack = (Attack*)internal_malloc(sizeof(*attack));
    if (attack == NULL) {
        return -1;
    }

    Object* explosion;
    int fid = art_id(OBJ_TYPE_MISC, 10, 0, 0, 0);
    if (obj_new(&explosion, fid, -1) == -1) {
        internal_free(attack);
        return -1;
    }

    obj_turn_off(explosion, NULL);
    explosion->flags |= OBJECT_TEMPORARY;

    obj_move_to_tile(explosion, tile, elevation, NULL);

    Object* adjacentExplosions[ROTATION_COUNT];
    for (int rotation = 0; rotation < ROTATION_COUNT; rotation++) {
        int fid = art_id(OBJ_TYPE_MISC, 10, 0, 0, 0);
        if (obj_new(&(adjacentExplosions[rotation]), fid, -1) == -1) {
            while (--rotation >= 0) {
                obj_erase_object(adjacentExplosions[rotation], NULL);
            }

            obj_erase_object(explosion, NULL);
            internal_free(attack);
            return -1;
        }

        obj_turn_off(adjacentExplosions[rotation], NULL);
        adjacentExplosions[rotation]->flags |= OBJECT_TEMPORARY;

        int adjacentTile = tileGetTileInDirection(tile, rotation, 1);
        obj_move_to_tile(adjacentExplosions[rotation], adjacentTile, elevation, NULL);
    }

    Object* critter = obj_blocking_at(NULL, tile, elevation);
    if (critter != NULL) {
        if (FID_TYPE(critter->fid) != OBJ_TYPE_CRITTER || (critter->data.critter.combat.results & DAM_DEAD) != 0) {
            critter = NULL;
        }
    }

    combat_ctd_init(attack, explosion, critter, HIT_MODE_PUNCH, HIT_LOCATION_TORSO);

    attack->tile = tile;
    attack->attackerFlags = DAM_HIT;

    game_ui_disable(1);

    if (critter != NULL) {
        if (register_clear(critter) == -2) {
            debugPrint("Cannot clear target's animation for action_explode!\n");
        }
        attack->defenderDamage = compute_explosion_damage(minDamage, maxDamage, critter, &(attack->defenderKnockback));
    }

    compute_explosion_on_extras(attack, 0, 0, 1);

    for (int index = 0; index < attack->extrasLength; index++) {
        Object* critter = attack->extras[index];
        if (register_clear(critter) == -2) {
            debugPrint("Cannot clear extra's animation for action_explode!\n");
        }

        attack->extrasDamage[index] = compute_explosion_damage(minDamage, maxDamage, critter, &(attack->extrasKnockback[index]));
    }

    death_checks(attack);

    if (a6) {
        action_in_explode = true;

        register_begin(ANIMATION_REQUEST_RESERVED);
        register_priority(1);
        register_object_play_sfx(explosion, "whn1xxx1", 0);
        register_object_funset(explosion, OBJECT_HIDDEN, 0);
        register_object_animate_and_hide(explosion, ANIM_STAND, 0);
        show_damage(attack, 0, 1);

        for (int rotation = 0; rotation < ROTATION_COUNT; rotation++) {
            register_object_funset(adjacentExplosions[rotation], OBJECT_HIDDEN, 0);
            register_object_animate_and_hide(adjacentExplosions[rotation], ANIM_STAND, 0);
        }

        register_object_must_call(explosion, 0, combat_explode_scenery, -1);
        register_object_must_erase(explosion);

        for (int rotation = 0; rotation < ROTATION_COUNT; rotation++) {
            register_object_must_erase(adjacentExplosions[rotation]);
        }

        register_object_must_call(attack, a5, report_explosion, -1);
        register_object_must_call(NULL, NULL, finished_explosion, -1);
        if (register_end() == -1) {
            action_in_explode = false;

            obj_erase_object(explosion, NULL);

            for (int rotation = 0; rotation < ROTATION_COUNT; rotation++) {
                obj_erase_object(adjacentExplosions[rotation], NULL);
            }

            internal_free(attack);

            game_ui_enable();
            return -1;
        }

        show_damage_extras(attack);
    } else {
        if (critter != NULL) {
            if ((attack->defenderFlags & DAM_DEAD) != 0) {
                critter_kill(critter, -1, false);
            }
        }

        for (int index = 0; index < attack->extrasLength; index++) {
            if ((attack->extrasFlags[index] & DAM_DEAD) != 0) {
                critter_kill(attack->extras[index], -1, false);
            }
        }

        report_explosion(attack, a5);

        combat_explode_scenery(explosion, NULL);

        obj_erase_object(explosion, NULL);

        for (int rotation = 0; rotation < ROTATION_COUNT; rotation++) {
            obj_erase_object(adjacentExplosions[rotation], NULL);
        }
    }

    return 0;
}

// 0x413144
static int report_explosion(Attack* attack, Object* a2)
{
    bool mainTargetWasDead;
    if (attack->defender != NULL) {
        mainTargetWasDead = (attack->defender->data.critter.combat.results & DAM_DEAD) != 0;
    } else {
        mainTargetWasDead = false;
    }

    bool extrasWasDead[6];
    for (int index = 0; index < attack->extrasLength; index++) {
        extrasWasDead[index] = (attack->extras[index]->data.critter.combat.results & DAM_DEAD) != 0;
    }

    death_checks(attack);
    combat_display(attack);
    apply_damage(attack, false);

    Object* anyDefender = NULL;
    int xp = 0;
    if (a2 != NULL) {
        if (attack->defender != NULL && attack->defender != a2) {
            if ((attack->defender->data.critter.combat.results & DAM_DEAD) != 0) {
                if (a2 == obj_dude && !mainTargetWasDead) {
                    xp += critter_kill_exps(attack->defender);
                }
            } else {
                critter_set_who_hit_me(attack->defender, a2);
                anyDefender = attack->defender;
            }
        }

        for (int index = 0; index < attack->extrasLength; index++) {
            Object* critter = attack->extras[index];
            if (critter != a2) {
                if ((critter->data.critter.combat.results & DAM_DEAD) != 0) {
                    if (a2 == obj_dude && !extrasWasDead[index]) {
                        xp += critter_kill_exps(critter);
                    }
                } else {
                    critter_set_who_hit_me(critter, a2);

                    if (anyDefender == NULL) {
                        anyDefender = critter;
                    }
                }
            }
        }

        if (anyDefender != NULL) {
            if (!isInCombat()) {
                STRUCT_664980 combat;
                combat.attacker = anyDefender;
                combat.defender = a2;
                combat.actionPointsBonus = 0;
                combat.accuracyBonus = 0;
                combat.damageBonus = 0;
                combat.minDamage = 0;
                combat.maxDamage = INT_MAX;
                combat.field_1C = 0;
                scriptsRequestCombat(&combat);
            }
        }
    }

    internal_free(attack);
    game_ui_enable();

    if (a2 == obj_dude) {
        combat_give_exps(xp);
    }

    return 0;
}

// 0x4132C0
static int finished_explosion(Object* a1, Object* a2)
{
    action_in_explode = false;
    return 0;
}

// calculate explosion damage applying threshold and resistances
// 0x4132CC
static int compute_explosion_damage(int min, int max, Object* a3, int* a4)
{
    int v5 = roll_random(min, max);
    int v7 = v5 - critterGetStat(a3, STAT_DAMAGE_THRESHOLD_EXPLOSION);
    if (v7 > 0) {
        v7 -= critterGetStat(a3, STAT_DAMAGE_RESISTANCE_EXPLOSION) * v7 / 100;
    }

    if (v7 < 0) {
        v7 = 0;
    }

    if (a4 != NULL) {
        if ((a3->flags & OBJECT_MULTIHEX) == 0) {
            *a4 = v7 / 10;
        }
    }

    return v7;
}

// 0x413330
int action_talk_to(Object* a1, Object* a2)
{
    if (a1 != obj_dude) {
        return -1;
    }

    if (FID_TYPE(a2->fid) != OBJ_TYPE_CRITTER) {
        return -1;
    }

    int anim = FID_ANIM_TYPE(obj_dude->fid);
    if (anim == ANIM_WALK || anim == ANIM_RUNNING) {
        register_clear(obj_dude);
    }

    if (isInCombat()) {
        register_begin(ANIMATION_REQUEST_RESERVED);
        register_object_move_to_object(a1, a2, a1->data.critter.combat.ap, 0);
    } else {
        register_begin(a1 == obj_dude ? ANIMATION_REQUEST_RESERVED : ANIMATION_REQUEST_UNRESERVED);

        if (obj_dist(a1, a2) >= 9 || combat_is_shot_blocked(a1, a1->tile, a2->tile, a2, NULL)) {
            register_object_run_to_object(a1, a2, -1, 0);
        }
    }

    register_object_must_call(a1, a2, can_talk_to, -1);
    register_object_call(a1, a2, talk_to, -1);
    return register_end();
}

// 0x413420
static int can_talk_to(Object* a1, Object* a2)
{
    if (combat_is_shot_blocked(a1, a1->tile, a2->tile, a2, NULL) || obj_dist(a1, a2) >= 9) {
        if (a1 == obj_dude) {
            // You cannot get there. (used in actions.c)
            MessageListItem messageListItem;
            messageListItem.num = 2000;
            if (message_search(&misc_message_file, &messageListItem)) {
                display_print(messageListItem.text);
            }
        }

        return -1;
    }

    return 0;
}

// 0x413488
static int talk_to(Object* a1, Object* a2)
{
    scriptsRequestDialog(a2);
    return 0;
}

// 0x413494
void action_dmg(int tile, int elevation, int minDamage, int maxDamage, int damageType, bool animated, bool bypassArmor)
{
    Attack* attack = (Attack*)internal_malloc(sizeof(*attack));
    if (attack == NULL) {
        return;
    }

    Object* attacker;
    if (obj_new(&attacker, FID_0x20001F5, -1) == -1) {
        internal_free(attack);
        return;
    }

    obj_turn_off(attacker, NULL);

    attacker->flags |= OBJECT_TEMPORARY;

    obj_move_to_tile(attacker, tile, elevation, NULL);

    Object* defender = obj_blocking_at(NULL, tile, elevation);
    combat_ctd_init(attack, attacker, defender, HIT_MODE_PUNCH, HIT_LOCATION_TORSO);
    attack->tile = tile;
    attack->attackerFlags = DAM_HIT;
    game_ui_disable(1);

    if (defender != NULL) {
        register_clear(defender);

        int damage;
        if (bypassArmor) {
            damage = maxDamage;
        } else {
            damage = compute_dmg_damage(minDamage, maxDamage, defender, &(attack->defenderKnockback), damageType);
        }

        attack->defenderDamage = damage;
    }

    death_checks(attack);

    if (animated) {
        register_begin(ANIMATION_REQUEST_RESERVED);
        register_object_play_sfx(attacker, "whc1xxx1", 0);
        show_damage(attack, death_3[damageType], 0);
        register_object_must_call(attack, NULL, report_dmg, 0);
        register_object_must_erase(attacker);

        if (register_end() == -1) {
            obj_erase_object(attacker, NULL);
            internal_free(attack);
            return;
        }
    } else {
        if (defender != NULL) {
            if ((attack->defenderFlags & DAM_DEAD) != 0) {
                critter_kill(defender, -1, 1);
            }
        }

        // NOTE: Uninline.
        report_dmg(attack, NULL);

        obj_erase_object(attacker, NULL);
    }

    game_ui_enable();
}

// 0x41363C
static int report_dmg(Attack* attack, Object* a2)
{
    combat_display(attack);
    apply_damage(attack, false);
    internal_free(attack);
    game_ui_enable();
    return 0;
}

// Calculate damage by applying threshold and resistances.
//
// 0x413660
static int compute_dmg_damage(int min, int max, Object* obj, int* a4, int damageType)
{
    if (!critter_flag_check(obj->pid, CRITTER_NO_KNOCKBACK)) {
        a4 = NULL;
    }

    int v8 = roll_random(min, max);
    int v10 = v8 - critterGetStat(obj, STAT_DAMAGE_THRESHOLD + damageType);
    if (v10 > 0) {
        v10 -= critterGetStat(obj, STAT_DAMAGE_RESISTANCE + damageType) * v10 / 100;
    }

    if (v10 < 0) {
        v10 = 0;
    }

    if (a4 != NULL) {
        if ((obj->flags & OBJECT_MULTIHEX) == 0 && damageType != DAMAGE_TYPE_ELECTRICAL) {
            *a4 = v10 / 10;
        }
    }

    return v10;
}

// 0x4136EC
bool action_can_be_pushed(Object* a1, Object* a2)
{
    // Cannot push anything but critters.
    if (FID_TYPE(a2->fid) != OBJ_TYPE_CRITTER) {
        return false;
    }

    // Cannot push itself.
    if (a1 == a2) {
        return false;
    }

    // Cannot push inactive critters.
    if (!critter_is_active(a2)) {
        return false;
    }

    if (action_can_talk_to(a1, a2) != 0) {
        return false;
    }

    // Can only push critters that have push handler.
    if (!scriptHasProc(a2->sid, SCRIPT_PROC_PUSH)) {
        return false;
    }

    if (isInCombat()) {
        if (a2->data.critter.combat.team == a1->data.critter.combat.team
            && a2 == a1->data.critter.combat.whoHitMe) {
            return false;
        }

        // TODO: Check.
        Object* whoHitMe = a2->data.critter.combat.whoHitMe;
        if (whoHitMe != NULL
            && whoHitMe->data.critter.combat.team == a1->data.critter.combat.team) {
            return false;
        }
    }

    return true;
}

// 0x413790
int action_push_critter(Object* a1, Object* a2)
{
    if (!action_can_be_pushed(a1, a2)) {
        return -1;
    }

    int sid;
    if (obj_sid(a2, &sid) == 0) {
        scriptSetObjects(sid, a1, a2);
        scriptExecProc(sid, SCRIPT_PROC_PUSH);

        bool scriptOverrides = false;

        Script* scr;
        if (scriptGetScript(sid, &scr) != -1) {
            scriptOverrides = scr->scriptOverrides;
        }

        if (scriptOverrides) {
            return -1;
        }
    }

    int rotation = tileGetRotationTo(a1->tile, a2->tile);
    int tile;
    do {
        tile = tileGetTileInDirection(a2->tile, rotation, 1);
        if (obj_blocking_at(a2, tile, a2->elevation) == NULL) {
            break;
        }

        tile = tileGetTileInDirection(a2->tile, (rotation + 1) % ROTATION_COUNT, 1);
        if (obj_blocking_at(a2, tile, a2->elevation) == NULL) {
            break;
        }

        tile = tileGetTileInDirection(a2->tile, (rotation + 5) % ROTATION_COUNT, 1);
        if (obj_blocking_at(a2, tile, a2->elevation) == NULL) {
            break;
        }

        tile = tileGetTileInDirection(a2->tile, (rotation + 2) % ROTATION_COUNT, 1);
        if (obj_blocking_at(a2, tile, a2->elevation) == NULL) {
            break;
        }

        tile = tileGetTileInDirection(a2->tile, (rotation + 4) % ROTATION_COUNT, 1);
        if (obj_blocking_at(a2, tile, a2->elevation) == NULL) {
            break;
        }

        tile = tileGetTileInDirection(a2->tile, (rotation + 3) % ROTATION_COUNT, 1);
        if (obj_blocking_at(a2, tile, a2->elevation) == NULL) {
            break;
        }

        return -1;
    } while (0);

    int actionPoints;
    if (isInCombat()) {
        actionPoints = a2->data.critter.combat.ap;
    } else {
        actionPoints = -1;
    }

    register_begin(ANIMATION_REQUEST_RESERVED);
    register_object_turn_towards(a2, tile);
    register_object_move_to_tile(a2, tile, a2->elevation, actionPoints, 0);
    return register_end();
}

// Returns -1 if can't see there (can't find a path there)
// Returns -2 if it's too far (> 12 tiles).
//
// 0x413970
int action_can_talk_to(Object* a1, Object* a2)
{
    if (make_path_func(a1, a1->tile, a2->tile, NULL, 0, obj_sight_blocking_at) == 0) {
        return -1;
    }

    if (tileDistanceBetween(a1->tile, a2->tile) > 12) {
        return -2;
    }

    return 0;
}
