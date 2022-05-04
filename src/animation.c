#include "animation.h"

#include "color.h"
#include "combat.h"
#include "combat_ai.h"
#include "core.h"
#include "critter.h"
#include "debug.h"
#include "display_monitor.h"
#include "game.h"
#include "game_config.h"
#include "game_mouse.h"
#include "game_sound.h"
#include "geometry.h"
#include "interface.h"
#include "item.h"
#include "map.h"
#include "object.h"
#include "perk.h"
#include "proto.h"
#include "proto_instance.h"
#include "random.h"
#include "scripts.h"
#include "stat.h"
#include "text_object.h"
#include "tile.h"
#include "trait.h"

#include <stdio.h>

// 0x510718
int dword_510718 = 0;

// 0x51071C
int gAnimationSequenceCurrentIndex = -1;

// 0x510720
int dword_510720 = 0;

// 0x510724
bool dword_510724 = false;

// 0x510728
bool dword_510728 = false;

// 0x51072C
int dword_51072C = -2;

// 0x510730
unsigned int dword_510730 = 0;

// 0x510734
unsigned int dword_510734 = 0;

// 0x530014
STRUCT_530014 stru_530014[24];

// 0x542FD4
PathNode gClosedPathNodeList[2000];

// 0x54CC14
AnimationSequence gAnimationSequences[32];

// 0x561814
unsigned char gPathfinderProcessedTiles[5000];

// 0x562B9C
PathNode gOpenPathNodeList[2000];

// 0x56C7DC
int gAnimationDescriptionCurrentIndex;

// 0x56C7E0
Object* dword_56C7E0[100];

// anim_init
// 0x413A20
void animationInit()
{
    dword_510720 = 1;
    animationReset();
    dword_510720 = 0;
}

// 0x413A40
void animationReset()
{
    if (!dword_510720) {
        // NOTE: Uninline.
        sub_4186CC();
    }

    dword_510718 = 0;
    gAnimationSequenceCurrentIndex = -1;

    for (int index = 0; index < ANIMATION_SEQUENCE_LIST_CAPACITY; index++) {
        gAnimationSequences[index].field_0 = -1000;
        gAnimationSequences[index].flags = 0;
    }
}

// 0x413AB8
void animationExit()
{
    // NOTE: Uninline.
    sub_4186CC();
}

// 0x413AF4
int reg_anim_begin(int flags)
{
    if (gAnimationSequenceCurrentIndex != -1) {
        return -1;
    }

    if (dword_510724) {
        return -1;
    }

    int v1 = sub_413B80(flags);
    if (v1 == -1) {
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[v1]);
    animationSequence->flags |= 0x08;

    if (flags & 0x02) {
        animationSequence->flags |= 0x04;
    }

    if (flags & 0x0200) {
        animationSequence->flags |= 0x40;
    }

    if (flags & 0x04) {
        animationSequence->flags |= 0x80;
    }

    gAnimationSequenceCurrentIndex = v1;

    gAnimationDescriptionCurrentIndex = 0;

    return 0;
}

// 0x413B80
int sub_413B80(int flags)
{
    int v1 = -1;
    int v2 = 0;
    for (int index = 0; index < ANIMATION_SEQUENCE_LIST_CAPACITY; index++) {
        // TODO: Check.
        if (gAnimationSequences[index].field_0 != -1000 || gAnimationSequences[index].flags & 8 || gAnimationSequences[index].flags & 0x20) {
            if (!(gAnimationSequences[index].flags & 4)) {
                v2++;
            }
        } else if (v1 == -1 && (!((flags >> 8) & 1) || !(gAnimationSequences[index].flags & 0x10))) {
            v1 = index;
        }
    }

    if (v1 == -1) {
        if (flags & 0x02) {
            debugPrint("Unable to begin reserved animation!\n");
        }

        return -1;
    } else if (flags & 0x02 || v2 < 20) {
        return v1;
    }

    return -1;
}

// 0x413C20
int sub_413C20(int a1)
{
    if (gAnimationSequenceCurrentIndex == -1) {
        return -1;
    }

    if (a1 == 0) {
        return -1;
    }

    gAnimationSequences[gAnimationSequenceCurrentIndex].flags |= 0x01;

    return 0;
}

// 0x413C4C
int reg_anim_clear(Object* a1)
{
    for (int animationSequenceIndex = 0; animationSequenceIndex < ANIMATION_SEQUENCE_LIST_CAPACITY; animationSequenceIndex++) {
        if (gAnimationSequences[animationSequenceIndex].field_0 == -1000) {
            continue;
        }

        int animationDescriptionIndex;
        for (animationDescriptionIndex = 0; animationDescriptionIndex < gAnimationSequences[animationSequenceIndex].length; animationDescriptionIndex++) {
            if (a1 != gAnimationSequences[animationSequenceIndex].animations[animationDescriptionIndex].owner || gAnimationSequences[animationSequenceIndex].animations[animationDescriptionIndex].type == 11) {
                continue;
            }

            break;
        }

        if (animationDescriptionIndex == gAnimationSequences[animationSequenceIndex].length) {
            continue;
        }

        if (gAnimationSequences[animationSequenceIndex].flags & 0x01) {
            return -2;
        }

        sub_415B9C(animationSequenceIndex);

        return 0;
    }

    return -1;
}

// 0x413CCC
int reg_anim_end()
{
    if (gAnimationSequenceCurrentIndex == -1) {
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    animationSequence->field_0 = 0;
    animationSequence->length = gAnimationDescriptionCurrentIndex;
    animationSequence->animationIndex = -1;
    animationSequence->flags &= ~0x08;
    animationSequence->animations[0].delay = 0;
    if (isInCombat()) {
        sub_425E3C();
        animationSequence->flags |= 0x02;
    }

    int v1 = gAnimationSequenceCurrentIndex;
    gAnimationSequenceCurrentIndex = -1;

    if (!(animationSequence->flags & 0x10)) {
        sub_415B44(v1, 1);
    }

    return 0;
}

// 0x413D98
void sub_413D98()
{
    if (gAnimationSequenceCurrentIndex == -1) {
        return;
    }

    for (int index = 0; index < ANIMATION_SEQUENCE_LIST_CAPACITY; index++) {
        gAnimationSequences[index].flags &= ~0x18;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    for (int index = 0; index < gAnimationDescriptionCurrentIndex; index++) {
        AnimationDescription* animationDescription = &(animationSequence->animations[index]);
        if (animationDescription->field_2C != NULL) {
            artUnlock(animationDescription->field_2C);
        }

        if (animationDescription->type == ANIM_KIND_EXEC && animationDescription->soundProc == sub_4514F0) {
            soundEffectDelete(animationDescription->sound);
        }
    }

    gAnimationSequenceCurrentIndex = -1;
}

// 0x413E2C
int sub_413E2C(Object* obj)
{
    if (gAnimationSequenceCurrentIndex == -1) {
        return -1;
    }

    if (gAnimationDescriptionCurrentIndex >= 55) {
        return -1;
    }

    if (obj == NULL) {
        return 0;
    }

    for (int animationSequenceIndex = 0; animationSequenceIndex < ANIMATION_SEQUENCE_LIST_CAPACITY; animationSequenceIndex++) {
        AnimationSequence* animationSequence = &(gAnimationSequences[animationSequenceIndex]);

        if (animationSequenceIndex != gAnimationSequenceCurrentIndex && animationSequence->field_0 != -1000) {
            for (int animationDescriptionIndex = 0; animationDescriptionIndex < animationSequence->length; animationDescriptionIndex++) {
                AnimationDescription* animationDescription = &(animationSequence->animations[animationDescriptionIndex]);
                if (obj == animationDescription->owner && animationDescription->type != 11) {
                    if (!(animationSequence->flags & 0x40)) {
                        return -1;
                    }

                    sub_415B9C(animationSequenceIndex);
                }
            }
        }
    }

    return 0;
}

// Returns -1 if object is playing some animation.
//
// 0x413EC8
int animationIsBusy(Object* a1)
{
    if (gAnimationDescriptionCurrentIndex >= ANIMATION_DESCRIPTION_LIST_CAPACITY || a1 == NULL) {
        return 0;
    }

    for (int animationSequenceIndex = 0; animationSequenceIndex < ANIMATION_SEQUENCE_LIST_CAPACITY; animationSequenceIndex++) {
        AnimationSequence* animationSequence = &(gAnimationSequences[animationSequenceIndex]);
        if (animationSequenceIndex != gAnimationSequenceCurrentIndex && animationSequence->field_0 != -1000) {
            for (int animationDescriptionIndex = 0; animationDescriptionIndex < animationSequence->length; animationDescriptionIndex++) {
                AnimationDescription* animationDescription = &(animationSequence->animations[animationDescriptionIndex]);
                if (a1 != animationDescription->owner) {
                    continue;
                }

                if (animationDescription->type == ANIM_KIND_EXEC) {
                    continue;
                }

                if (animationSequence->length == 1 && animationDescription->anim == ANIM_STAND) {
                    continue;
                }

                return -1;
            }
        }
    }

    return 0;
}

// 0x413F5C
int reg_anim_obj_move_to_obj(Object* a1, Object* a2, int actionPoints, int delay)
{
    if (sub_413E2C(a1) == -1 || actionPoints == 0) {
        sub_413D98();
        return -1;
    }

    if (a1->tile == a2->tile && a1->elevation == a2->elevation) {
        return 0;
    }

    AnimationDescription* animationDescription = &(gAnimationSequences[gAnimationSequenceCurrentIndex].animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_OBJ_MOVE_TO_OBJ;
    animationDescription->anim = ANIM_WALK;
    animationDescription->owner = a1;
    animationDescription->destinationObj = a2;
    animationDescription->field_28 = actionPoints;
    animationDescription->delay = delay;

    int fid = buildFid((a1->fid & 0xF000000) >> 24, a1->fid & 0xFFF, animationDescription->anim, (a1->fid & 0xF000) >> 12, a1->rotation + 1);

    if (artLock(fid, &(animationDescription->field_2C)) == NULL) {
        sub_413D98();
        return -1;
    }

    artUnlock(animationDescription->field_2C);
    animationDescription->field_2C = NULL;

    gAnimationDescriptionCurrentIndex++;

    return reg_anim_set_rotation_to_tile(a1, a2->tile);
}

// 0x41405C
int reg_anim_obj_run_to_obj(Object* owner, Object* destination, int actionPoints, int delay)
{
    MessageListItem msg;
    const char* text;
    const char* name;
    char formatted_text[90]; // TODO: Size is probably wrong.

    if (sub_413E2C(owner) == -1 || actionPoints == 0) {
        sub_413D98();
        return -1;
    }

    if (owner->tile == destination->tile && owner->elevation == destination->elevation) {
        return 0;
    }

    if (critterIsEncumbered(owner)) {
        if (objectIsPartyMember(owner)) {
            if (owner == gDude) {
                // You are overloaded.
                text = getmsg(&gMiscMessageList, &msg, 8000);
                strcpy(formatted_text, text);
            } else {
                // %s is overloaded.
                name = critterGetName(owner);
                text = getmsg(&gMiscMessageList, &msg, 8001);
                sprintf(formatted_text, text, name);
            }
            displayMonitorAddMessage(formatted_text);
        }
        return reg_anim_obj_move_to_obj(owner, destination, actionPoints, delay);
    }

    AnimationDescription* animationDescription = &(gAnimationSequences[gAnimationSequenceCurrentIndex].animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_OBJ_MOVE_TO_OBJ;
    animationDescription->owner = owner;
    animationDescription->destinationObj = destination;

    if ((owner->fid & 0xF000000) >> 24 == 1 && (owner->data.critter.combat.results & (DAM_CRIP_LEG_LEFT | DAM_CRIP_LEG_RIGHT))
        || owner == gDude && dudeHasState(0) && !perkGetRank(gDude, PERK_SILENT_RUNNING)
        || !artExists(buildFid((owner->fid & 0xF000000) >> 24, owner->fid & 0xFFF, ANIM_RUNNING, 0, owner->rotation + 1))) {
        animationDescription->anim = ANIM_WALK;
    } else {
        animationDescription->anim = ANIM_RUNNING;
    }

    animationDescription->field_28 = actionPoints;
    animationDescription->delay = delay;

    int fid = buildFid((owner->fid & 0xF000000) >> 24, owner->fid & 0xFFF, animationDescription->anim, (owner->fid & 0xF000) >> 12, owner->rotation + 1);

    animationDescription->field_2C = NULL;
    if (artLock(fid, &(animationDescription->field_2C)) == NULL) {
        sub_413D98();
        return -1;
    }

    artUnlock(animationDescription->field_2C);
    animationDescription->field_2C = NULL;

    gAnimationDescriptionCurrentIndex++;
    return reg_anim_set_rotation_to_tile(owner, destination->tile);
}

// 0x414294
int reg_anim_obj_move_to_tile(Object* obj, int tile_num, int elev, int actionPoints, int delay)
{
    AnimationDescription* ptr;
    int fid;

    if (sub_413E2C(obj) == -1 || actionPoints == 0) {
        sub_413D98();
        return -1;
    }

    if (tile_num == obj->tile && elev == obj->elevation) {
        return 0;
    }

    ptr = &(gAnimationSequences[gAnimationSequenceCurrentIndex].animations[gAnimationDescriptionCurrentIndex]);
    ptr->type = ANIM_KIND_OBJ_MOVE_TO_TILE;
    ptr->anim = ANIM_WALK;
    ptr->owner = obj;
    ptr->tile = tile_num;
    ptr->elevation = elev;
    ptr->field_28 = actionPoints;
    ptr->delay = delay;

    ptr->field_2C = NULL;
    fid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, ptr->anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    if (artLock(fid, &(ptr->field_2C)) == NULL) {
        sub_413D98();
        return -1;
    }

    artUnlock(ptr->field_2C);
    ptr->field_2C = NULL;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x414394
int reg_anim_obj_run_to_tile(Object* obj, int tile_num, int elev, int actionPoints, int delay)
{
    MessageListItem msg;
    const char* text;
    const char* name;
    char str[72]; // TODO: Size is probably wrong.
    AnimationDescription* animationDescription;
    int fid;

    if (sub_413E2C(obj) == -1) {
        sub_413D98();
        return -1;
    }

    if (actionPoints == 0) {
        sub_413D98();
        return -1;
    }

    if (tile_num == obj->tile && elev == obj->elevation) {
        return 0;
    }

    if (critterIsEncumbered(obj)) {
        if (objectIsPartyMember(obj)) {
            if (obj == gDude) {
                // You are overloaded.
                text = getmsg(&gMiscMessageList, &msg, 8000);
                strcpy(str, text);
            } else {
                // %s is overloaded.
                name = critterGetName(obj);
                text = getmsg(&gMiscMessageList, &msg, 8001);
                sprintf(str, text, name);
            }

            displayMonitorAddMessage(str);
        }

        return reg_anim_obj_move_to_tile(obj, tile_num, elev, actionPoints, delay);
    }

    animationDescription = &(gAnimationSequences[gAnimationSequenceCurrentIndex].animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_OBJ_MOVE_TO_TILE;
    animationDescription->owner = obj;
    animationDescription->tile = tile_num;
    animationDescription->elevation = elev;

    // TODO: Check.
    if ((obj->fid & 0xF000000) >> 24 == 1 && (obj->data.critter.combat.results & (DAM_CRIP_LEG_LEFT | DAM_CRIP_LEG_RIGHT))
        || obj == gDude && dudeHasState(0) && !perkGetRank(gDude, PERK_SILENT_RUNNING)
        || !artExists(buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, ANIM_RUNNING, 0, obj->rotation + 1))) {
        animationDescription->anim = ANIM_WALK;
    } else {
        animationDescription->anim = ANIM_RUNNING;
    }

    animationDescription->field_28 = actionPoints;
    animationDescription->delay = delay;

    fid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, animationDescription->anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);

    animationDescription->field_2C = NULL;
    if (artLock(fid, &(animationDescription->field_2C)) == NULL) {
        sub_413D98();
        return -1;
    }

    artUnlock(animationDescription->field_2C);
    animationDescription->field_2C = NULL;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x4145D0
int reg_anim_2(Object* obj, int tile_num, int elev, int anim, int delay)
{
    if (sub_413E2C(obj) == -1) {
        sub_413D98();
        return -1;
    }

    if (tile_num == obj->tile && elev == obj->elevation) {
        return 0;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);

    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_2;
    animationDescription->owner = obj;
    animationDescription->tile = tile_num;
    animationDescription->elevation = elev;
    animationDescription->anim = anim;
    animationDescription->delay = delay;
    animationDescription->field_2C = NULL;

    int fid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, animationDescription->anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    if (artLock(fid, &(animationDescription->field_2C)) == NULL) {
        sub_413D98();
        return -1;
    }

    artUnlock(animationDescription->field_2C);
    animationDescription->field_2C = NULL;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x4146C4
int reg_anim_knockdown(Object* obj, int tile, int elev, int anim, int delay)
{
    if (sub_413E2C(obj) == -1) {
        sub_413D98();
        return -1;
    }

    if (tile == obj->tile && elev == obj->elevation) {
        return 0;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_KNOCKDOWN;
    animationDescription->owner = obj;
    animationDescription->tile = tile;
    animationDescription->elevation = elev;
    animationDescription->anim = anim;
    animationDescription->delay = delay;
    animationDescription->field_2C = NULL;

    int fid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, animationDescription->anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    if (artLock(fid, &(animationDescription->field_2C)) == NULL) {
        sub_413D98();
        return -1;
    }

    artUnlock(animationDescription->field_2C);
    animationDescription->field_2C = NULL;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x4149D0
int reg_anim_animate(Object* obj, int anim, int delay)
{
    if (sub_413E2C(obj) == -1) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_ANIMATE;
    animationDescription->owner = obj;
    animationDescription->anim = anim;
    animationDescription->delay = delay;
    animationDescription->field_2C = NULL;

    int fid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, animationDescription->anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    if (artLock(fid, &(animationDescription->field_2C)) == NULL) {
        sub_413D98();
        return -1;
    }

    artUnlock(animationDescription->field_2C);
    animationDescription->field_2C = NULL;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x414AA8
int reg_anim_animate_reverse(Object* obj, int anim, int delay)
{
    if (sub_413E2C(obj) == -1) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_ANIMATE_REVERSE;
    animationDescription->owner = obj;
    animationDescription->anim = anim;
    animationDescription->delay = delay;
    animationDescription->field_2C = NULL;

    int fid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, animationDescription->anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    if (artLock(fid, &(animationDescription->field_2C)) == NULL) {
        sub_413D98();
        return -1;
    }

    artUnlock(animationDescription->field_2C);
    animationDescription->field_2C = NULL;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x414B7C
int reg_anim_6(Object* obj, int anim, int delay)
{
    if (sub_413E2C(obj) == -1) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_6;
    animationDescription->owner = obj;
    animationDescription->anim = anim;
    animationDescription->delay = delay;
    animationDescription->field_2C = NULL;

    int fid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    if (artLock(fid, &(animationDescription->field_2C)) == NULL) {
        sub_413D98();
        return -1;
    }

    artUnlock(animationDescription->field_2C);
    animationDescription->field_2C = NULL;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x414C50
int reg_anim_set_rotation_to_tile(Object* owner, int tile)
{
    if (sub_413E2C(owner) == -1) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_SET_ROTATION_TO_TILE;
    animationDescription->delay = -1;
    animationDescription->field_2C = NULL;
    animationDescription->owner = owner;
    animationDescription->tile = tile;
    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x414CC8
int reg_anim_rotate_clockwise(Object* obj)
{
    if (sub_413E2C(obj) == -1) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_ROTATE_CLOCKWISE;
    animationDescription->delay = -1;
    animationDescription->field_2C = NULL;
    animationDescription->owner = obj;
    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x414D38
int reg_anim_rotate_counter_clockwise(Object* obj)
{
    if (sub_413E2C(obj) == -1) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_ROTATE_COUNTER_CLOCKWISE;
    animationDescription->delay = -1;
    animationDescription->field_2C = NULL;
    animationDescription->owner = obj;
    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x414E20
int reg_anim_hide(Object* obj)
{
    if (sub_413E2C(obj) == -1) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_HIDE;
    animationDescription->delay = -1;
    animationDescription->field_2C = NULL;
    animationDescription->field_24 = 1;
    animationDescription->owner = obj;
    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x414E98
int reg_anim_11_0(Object* a1, Object* a2, AnimationProc* proc, int delay)
{
    if (sub_413E2C(NULL) == -1 || proc == NULL) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_EXEC;
    animationDescription->field_24 = 0;
    animationDescription->field_2C = NULL;
    animationDescription->owner = a2;
    animationDescription->destinationObj = a1;
    animationDescription->proc = proc;
    animationDescription->delay = delay;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x414F20
int reg_anim_12(Object* a1, Object* a2, void* a3, AnimationProc2* proc, int delay)
{
    if (sub_413E2C(NULL) == -1 || !proc) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_EXEC_2;
    animationDescription->field_24 = 0;
    animationDescription->field_2C = NULL;
    animationDescription->owner = a2;
    animationDescription->destinationObj = a1;
    animationDescription->field_20 = proc;
    animationDescription->field_28_void = a3;
    animationDescription->delay = delay;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x414FAC
int reg_anim_11_1(Object* a1, Object* a2, AnimationProc* proc, int delay)
{
    if (sub_413E2C(NULL) == -1 || proc == NULL) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_EXEC;
    animationDescription->field_24 = 1;
    animationDescription->field_2C = NULL;
    animationDescription->owner = a2;
    animationDescription->destinationObj = a1;
    animationDescription->proc = proc;
    animationDescription->delay = delay;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x4150A8
int reg_anim_15(Object* obj, int a2, int delay)
{
    if (sub_413E2C(obj) == -1) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_15;
    animationDescription->field_2C = NULL;
    animationDescription->owner = obj;
    animationDescription->field_24 = a2;
    animationDescription->delay = delay;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x41518C
int reg_anim_17(Object* obj, int fid, int delay)
{
    if (sub_413E2C(obj) == -1) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_17;
    animationDescription->owner = obj;
    animationDescription->fid = fid;
    animationDescription->delay = delay;
    animationDescription->field_2C = NULL;

    if (artLock(fid, &(animationDescription->field_2C)) == NULL) {
        sub_413D98();
        return -1;
    }

    artUnlock(animationDescription->field_2C);
    animationDescription->field_2C = NULL;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x415238
int reg_anim_18(Object* obj, int weaponAnimationCode, int delay)
{
    const char* sfx = sfxBuildCharName(obj, ANIM_TAKE_OUT, weaponAnimationCode);
    if (reg_anim_play_sfx(obj, sfx, delay) == -1) {
        return -1;
    }

    if (sub_413E2C(obj) == -1) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_18;
    animationDescription->anim = ANIM_TAKE_OUT;
    animationDescription->delay = 0;
    animationDescription->owner = obj;
    animationDescription->weaponAnimationCode = weaponAnimationCode;

    int fid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, ANIM_TAKE_OUT, weaponAnimationCode, obj->rotation + 1);
    if (artLock(fid, &(animationDescription->field_2C)) == NULL) {
        sub_413D98();
        return -1;
    }

    artUnlock(animationDescription->field_2C);
    animationDescription->field_2C = NULL;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x415334
int reg_anim_update_light(Object* obj, int lightDistance, int delay)
{
    if (sub_413E2C(obj) == -1) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_19;
    animationDescription->field_2C = NULL;
    animationDescription->owner = obj;
    animationDescription->lightDistance = lightDistance;
    animationDescription->delay = delay;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x41541C
int reg_anim_play_sfx(Object* obj, const char* soundEffectName, int delay)
{
    if (sub_413E2C(obj) == -1) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_EXEC;
    animationDescription->owner = obj;
    if (soundEffectName != NULL) {
        int volume = sub_451534(obj);
        animationDescription->sound = soundEffectLoadWithVolume(soundEffectName, obj, volume);
        if (animationDescription->sound != NULL) {
            animationDescription->soundProc = sub_4514F0;
        } else {
            animationDescription->type = ANIM_KIND_28;
        }
    } else {
        animationDescription->type = ANIM_KIND_28;
    }

    animationDescription->field_2C = NULL;
    animationDescription->delay = delay;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x4154C4
int reg_anim_animate_forever(Object* obj, int anim, int delay)
{
    if (sub_413E2C(obj) == -1) {
        sub_413D98();
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[gAnimationSequenceCurrentIndex]);
    AnimationDescription* animationDescription = &(animationSequence->animations[gAnimationDescriptionCurrentIndex]);
    animationDescription->type = ANIM_KIND_ANIMATE_FOREVER;
    animationDescription->owner = obj;
    animationDescription->anim = anim;
    animationDescription->delay = delay;
    animationDescription->field_2C = NULL;

    int fid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    if (artLock(fid, &(animationDescription->field_2C)) == NULL) {
        sub_413D98();
        return -1;
    }

    artUnlock(animationDescription->field_2C);
    animationDescription->field_2C = NULL;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x415598
int reg_anim_26(int a1, int delay)
{
    AnimationDescription* ptr;
    int v5;

    if (sub_413E2C(NULL) == -1) {
        sub_413D98();
        return -1;
    }

    v5 = sub_413B80(a1 | 0x01);
    if (v5 == -1) {
        return -1;
    }

    gAnimationSequences[v5].flags = 16;

    ptr = &(gAnimationSequences[gAnimationSequenceCurrentIndex].animations[gAnimationDescriptionCurrentIndex]);
    ptr->owner = NULL;
    ptr->type = ANIM_KIND_26;
    ptr->field_2C = NULL;
    ptr->field_28 = v5;
    ptr->delay = delay;

    gAnimationDescriptionCurrentIndex++;

    return 0;
}

// 0x4156A8
int animationRunSequence(int animationSequenceIndex)
{
    if (animationSequenceIndex == -1) {
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[animationSequenceIndex]);
    if (animationSequence->field_0 == -1000) {
        return -1;
    }

    while (1) {
        if (animationSequence->field_0 >= animationSequence->length) {
            return 0;
        }

        if (animationSequence->field_0 > animationSequence->animationIndex) {
            AnimationDescription* animationDescription = &(animationSequence->animations[animationSequence->field_0]);
            if (animationDescription->delay < 0) {
                return 0;
            }

            if (animationDescription->delay > 0) {
                animationDescription->delay--;
                return 0;
            }
        }

        AnimationDescription* animationDescription = &(animationSequence->animations[animationSequence->field_0++]);

        int rc;
        Rect rect;
        switch (animationDescription->type) {
        case ANIM_KIND_OBJ_MOVE_TO_OBJ:
            rc = animateMoveObjectToObject(animationDescription->owner, animationDescription->destinationObj, animationDescription->field_28, animationDescription->anim, animationSequenceIndex);
            break;
        case ANIM_KIND_OBJ_MOVE_TO_TILE:
            rc = animateMoveObjectToTile(animationDescription->owner, animationDescription->tile, animationDescription->elevation, animationDescription->field_28, animationDescription->anim, animationSequenceIndex);
            break;
        case ANIM_KIND_2:
            rc = sub_416F54(animationDescription->owner, animationDescription->tile, animationDescription->elevation, animationDescription->anim, animationSequenceIndex, 0x00);
            break;
        case ANIM_KIND_KNOCKDOWN:
            rc = sub_416F54(animationDescription->owner, animationDescription->tile, animationDescription->elevation, animationDescription->anim, animationSequenceIndex, 0x10);
            break;
        case ANIM_KIND_ANIMATE:
            rc = sub_4179B8(animationDescription->owner, animationDescription->anim, animationSequenceIndex, 0);
            break;
        case ANIM_KIND_ANIMATE_REVERSE:
            rc = sub_4179B8(animationDescription->owner, animationDescription->anim, animationSequenceIndex, 0x01);
            break;
        case ANIM_KIND_6:
            rc = sub_4179B8(animationDescription->owner, animationDescription->anim, animationSequenceIndex, 0x40);
            if (rc == -1) {
                Rect rect;
                if (objectHide(animationDescription->owner, &rect) == 0) {
                    tileWindowRefreshRect(&rect, animationDescription->elevation);
                }

                if (animationSequenceIndex != -1) {
                    sub_415B44(animationSequenceIndex, 0);
                }
                rc = 0;
            }
            break;
        case ANIM_KIND_ANIMATE_FOREVER:
            rc = sub_4179B8(animationDescription->owner, animationDescription->anim, animationSequenceIndex, 0x80);
            break;
        case ANIM_KIND_SET_ROTATION_TO_TILE:
            if (!sub_42DD80(animationDescription->owner)) {
                int rotation = tileGetRotationTo(animationDescription->owner->tile, animationDescription->tile);
                sub_418378(animationDescription->owner, rotation, -1);
            }
            sub_415B44(animationSequenceIndex, 0);
            rc = 0;
            break;
        case ANIM_KIND_ROTATE_CLOCKWISE:
            rc = actionRotate(animationDescription->owner, 1, animationSequenceIndex);
            break;
        case ANIM_KIND_ROTATE_COUNTER_CLOCKWISE:
            rc = actionRotate(animationDescription->owner, -1, animationSequenceIndex);
            break;
        case ANIM_KIND_HIDE:
            if (objectHide(animationDescription->owner, &rect) == 0) {
                tileWindowRefreshRect(&rect, animationDescription->owner->elevation);
            }
            if (animationSequenceIndex != -1) {
                sub_415B44(animationSequenceIndex, 0);
            }
            rc = 0;
            break;
        case ANIM_KIND_EXEC:
            rc = animationDescription->proc(animationDescription->destinationObj, animationDescription->owner);
            if (rc == 0) {
                rc = sub_415B44(animationSequenceIndex, 0);
            }
            break;
        case ANIM_KIND_EXEC_2:
            rc = animationDescription->field_20(animationDescription->destinationObj, animationDescription->owner, animationDescription->field_28_obj);
            if (rc == 0) {
                rc = sub_415B44(animationSequenceIndex, 0);
            }
            break;
        case ANIM_KIND_14:
            if (animationDescription->field_24 == 32) {
                if (sub_48AD48(animationDescription->owner, &rect) == 0) {
                    tileWindowRefreshRect(&rect, animationDescription->owner->elevation);
                }
            } else if (animationDescription->field_24 == 1) {
                if (objectHide(animationDescription->owner, &rect) == 0) {
                    tileWindowRefreshRect(&rect, animationDescription->owner->elevation);
                }
            } else {
                animationDescription->owner->flags |= animationDescription->field_24;
            }

            rc = sub_415B44(animationSequenceIndex, 0);
            break;
        case ANIM_KIND_15:
            if (animationDescription->field_24 == 32) {
                if (sub_48AD9C(animationDescription->owner, &rect) == 0) {
                    tileWindowRefreshRect(&rect, animationDescription->owner->elevation);
                }
            } else if (animationDescription->field_24 == 1) {
                if (objectShow(animationDescription->owner, &rect) == 0) {
                    tileWindowRefreshRect(&rect, animationDescription->owner->elevation);
                }
            } else {
                animationDescription->owner->flags &= ~animationDescription->field_24;
            }

            rc = sub_415B44(animationSequenceIndex, 0);
            break;
        case ANIM_KIND_16:
            if (sub_48AF2C(animationDescription->owner, &rect) == 0) {
                tileWindowRefreshRect(&rect, animationDescription->owner->elevation);
            }
            rc = sub_415B44(animationSequenceIndex, 0);
            break;
        case ANIM_KIND_17:
            rc = sub_418660(animationDescription->owner, animationSequenceIndex, animationDescription->fid);
            break;
        case ANIM_KIND_18:
            rc = sub_4179B8(animationDescription->owner, ANIM_TAKE_OUT, animationSequenceIndex, animationDescription->tile);
            break;
        case ANIM_KIND_19:
            objectSetLight(animationDescription->owner, animationDescription->lightDistance, animationDescription->owner->lightIntensity, &rect);
            tileWindowRefreshRect(&rect, animationDescription->owner->elevation);
            rc = sub_415B44(animationSequenceIndex, 0);
            break;
        case ANIM_KIND_20:
            rc = sub_41712C(animationDescription->owner, animationDescription->tile, animationDescription->elevation, animationDescription->anim, animationSequenceIndex);
            break;
        case ANIM_KIND_23:
            rc = sub_417248(animationDescription->owner, animationDescription->anim, animationSequenceIndex);
            break;
        case ANIM_KIND_24:
            if (animationDescription->tile) {
                if (objectEnableOutline(animationDescription->owner, &rect) == 0) {
                    tileWindowRefreshRect(&rect, animationDescription->owner->elevation);
                }
            } else {
                if (objectDisableOutline(animationDescription->owner, &rect) == 0) {
                    tileWindowRefreshRect(&rect, animationDescription->owner->elevation);
                }
            }
            rc = sub_415B44(animationSequenceIndex, 0);
            break;
        case ANIM_KIND_26:
            gAnimationSequences[animationDescription->field_28].flags &= ~0x10;
            rc = sub_415B44(animationDescription->field_28, 1);
            if (rc != -1) {
                rc = sub_415B44(animationSequenceIndex, 0);
            }
            break;
        case ANIM_KIND_28:
            rc = sub_415B44(animationSequenceIndex, 0);
            break;
        default:
            rc = -1;
            break;
        }

        if (rc == -1) {
            sub_415B9C(animationSequenceIndex);
        }

        if (animationSequence->field_0 == -1000) {
            return -1;
        }
    }
}

// 0x415B44
int sub_415B44(int animationSequenceIndex, int a2)
{
    if (animationSequenceIndex == -1) {
        return -1;
    }

    AnimationSequence* animationSequence = &(gAnimationSequences[animationSequenceIndex]);
    if (animationSequence->field_0 == -1000) {
        return -1;
    }

    animationSequence->animationIndex++;
    if (animationSequence->animationIndex == animationSequence->length) {
        return sub_415B9C(animationSequenceIndex);
    } else {
        if (a2) {
            return animationRunSequence(animationSequenceIndex);
        }
    }

    return 0;
}

// 0x415B9C
int sub_415B9C(int animationSequenceIndex)
{
    AnimationSequence* animationSequence;
    AnimationDescription* animationDescription;
    STRUCT_530014* ptr_530014;
    int i;
    Rect v27;

    if (animationSequenceIndex == -1) {
        return -1;
    }

    animationSequence = &(gAnimationSequences[animationSequenceIndex]);
    if (animationSequence->field_0 == -1000) {
        return -1;
    }

    for (i = 0; i < dword_510718; i++) {
        ptr_530014 = &(stru_530014[i]);
        if (ptr_530014->animationSequenceIndex == animationSequenceIndex) {
            ptr_530014->field_20 = -1000;
        }
    }

    for (i = 0; i < animationSequence->length; i++) {
        animationDescription = &(animationSequence->animations[i]);
        if (animationDescription->type == ANIM_KIND_HIDE && ((i < animationSequence->animationIndex) || (animationDescription->field_24 & 0x01))) {
            objectDestroy(animationDescription->owner, &v27);
            tileWindowRefreshRect(&v27, animationDescription->owner->elevation);
        }
    }

    for (i = 0; i < animationSequence->length; i++) {
        animationDescription = &(animationSequence->animations[i]);
        if (animationDescription->field_2C) {
            artUnlock(animationDescription->field_2C);
        }

        if (animationDescription->type != 11 && animationDescription->type != 12) {
            // TODO: Check.
            if (animationDescription->type != ANIM_KIND_26) {
                Object* owner = animationDescription->owner;
                if (((owner->fid & 0xF000000) >> 24) == OBJ_TYPE_CRITTER) {
                    int j = 0;
                    for (; j < i; j++) {
                        AnimationDescription* ad = &(animationSequence->animations[j]);
                        if (owner == ad->owner) {
                            if (ad->type != 11 && ad->type != 12) {
                                break;
                            }
                        }
                    }

                    if (i == j) {
                        int k = 0;
                        for (; k < animationSequence->animationIndex; k++) {
                            AnimationDescription* ad = &(animationSequence->animations[k]);
                            if (ad->type == 10 && ad->owner == owner) {
                                break;
                            }
                        }

                        if (k == animationSequence->animationIndex) {
                            for (int m = 0; m < dword_510718; m++) {
                                if (stru_530014[m].obj == owner) {
                                    stru_530014[m].field_20 = -1000;
                                    break;
                                }
                            }

                            if ((animationSequence->flags & 0x80) == 0 && !sub_42DD80(owner)) {
                                sub_418378(owner, owner->rotation, -1);
                            }
                        }
                    }
                }
            }
        } else if (i > animationSequence->field_0) {
            if (animationDescription->field_24 & 0x01) {
                animationDescription->proc(animationDescription->destinationObj, animationDescription->owner);
            } else {
                if (animationDescription->type == ANIM_KIND_EXEC && animationDescription->soundProc == sub_4514F0) {
                    soundEffectDelete(animationDescription->sound);
                }
            }
        }
    }

    animationSequence->animationIndex = -1;
    animationSequence->field_0 = -1000;
    if ((animationSequence->flags & 0x02) != 0) {
        sub_425E80();
    }

    if (dword_510728) {
        animationSequence->flags = 0x20;
    } else {
        animationSequence->flags = 0;
    }

    return 0;
}

// 0x415E24
int sub_415E24(Object* a1, Object* a2)
{
    int body_type;
    Proto* proto;

    // TODO: Check.
    if (a1 == gDude) {
        if (sub_48B2A8(a2) == 0) {
            return 0;
        }
    }

    if ((a1->fid & 0xF000000) >> 24 != 1) {
        return 0;
    }

    if ((a2->fid & 0xF000000) >> 24 != 2) {
        return 0;
    }

    body_type = critterGetBodyType(a1);
    if (body_type != BODY_TYPE_BIPED && body_type != BODY_TYPE_ROBOTIC) {
        return 0;
    }

    if (protoGetProto(a2->pid, &proto) == -1) {
        return 0;
    }

    if (proto->scenery.type != SCENERY_TYPE_DOOR) {
        return 0;
    }

    if (objectIsLocked(a1)) {
        return 0;
    }

    return critterGetKillType(a1) != KILL_TYPE_GECKO;
}

// 0x415EE8
int sub_415EE8(Object* object, int from, int to, unsigned char* rotations, int a5)
{
    return pathfinderFindPath(object, from, to, rotations, a5, sub_48B848);
}

// 0x415EFC
int pathfinderFindPath(Object* object, int from, int to, unsigned char* rotations, int a5, PathBuilderCallback* callback)
{
    if (a5) {
        if (callback(object, to, object->elevation) != NULL) {
            return 0;
        }
    }

    bool isCritter = false;
    int kt = 0;
    if ((object->pid >> 24) == OBJ_TYPE_CRITTER) {
        isCritter = true;
        kt = critterGetKillType(object);
    }

    bool isNotInCombat = !isInCombat();

    memset(gPathfinderProcessedTiles, 0, sizeof(gPathfinderProcessedTiles));

    gPathfinderProcessedTiles[from / 8] |= 1 << (from & 7);

    gOpenPathNodeList[0].tile = from;
    gOpenPathNodeList[0].from = -1;
    gOpenPathNodeList[0].rotation = 0;
    gOpenPathNodeList[0].field_C = sub_416360(from, to);
    gOpenPathNodeList[0].field_10 = 0;

    for (int index = 1; index < 2000; index += 1) {
        gOpenPathNodeList[index].tile = -1;
    }

    int toScreenX;
    int toScreenY;
    tileToScreenXY(to, &toScreenX, &toScreenY, object->elevation);

    int closedPathNodeListLength = 0;
    int openPathNodeListLength = 1;
    PathNode temp;

    while (1) {
        int v63 = -1;

        PathNode* prev = NULL;
        int v12 = 0;
        for (int index = 0; v12 < openPathNodeListLength; index += 1) {
            PathNode* curr = &(gOpenPathNodeList[index]);
            if (curr->tile != -1) {
                v12++;
                if (v63 == -1 || (curr->field_C + curr->field_10) < (prev->field_C + prev->field_10)) {
                    prev = curr;
                    v63 = index;
                }
            }
        }

        PathNode* curr = &(gOpenPathNodeList[v63]);

        memcpy(&temp, curr, sizeof(temp));

        openPathNodeListLength -= 1;

        curr->tile = -1;

        if (temp.tile == to) {
            if (openPathNodeListLength == 0) {
                openPathNodeListLength = 1;
            }
            break;
        }

        PathNode* curr1 = &(gClosedPathNodeList[closedPathNodeListLength]);
        memcpy(curr1, &temp, sizeof(temp));

        closedPathNodeListLength += 1;

        if (closedPathNodeListLength == 2000) {
            return 0;
        }

        for (int rotation = 0; rotation < ROTATION_COUNT; rotation++) {
            int tile = tileGetTileInDirection(temp.tile, rotation, 1);
            int bit = 1 << (tile & 7);
            if ((gPathfinderProcessedTiles[tile / 8] & bit) != 0) {
                continue;
            }

            if (tile != to) {
                Object* v24 = callback(object, tile, object->elevation);
                if (v24 != NULL) {
                    if (sub_415E24(object, v24) == 0) {
                        continue;
                    }
                }
            }

            int v25 = 0;
            for (; v25 < 2000; v25++) {
                if (gOpenPathNodeList[v25].tile == -1) {
                    break;
                }
            }

            openPathNodeListLength += 1;

            if (openPathNodeListLength == 2000) {
                return 0;
            }

            gPathfinderProcessedTiles[tile / 8] |= bit;

            PathNode* v27 = &(gOpenPathNodeList[v25]);
            v27->tile = tile;
            v27->from = temp.tile;
            v27->rotation = rotation;

            int newX;
            int newY;
            tileToScreenXY(tile, &newX, &newY, object->elevation);

            v27->field_C = sub_41633C(newX, newY, toScreenX, toScreenY);
            v27->field_10 = temp.field_10 + 50;

            if (isNotInCombat && temp.rotation != rotation) {
                v27->field_10 += 10;
            }

            if (isCritter) {
                Object* o = objectFindFirstAtLocation(object->elevation, v27->tile);
                while (o != NULL) {
                    if (o->pid >= 0x20003D9 && o->pid <= 0x20003DC) {
                        break;
                    }
                    o = objectFindNextAtLocation();
                }

                if (o != NULL) {
                    if (kt == KILL_TYPE_GECKO) {
                        v27->field_10 += 100;
                    } else {
                        v27->field_10 += 400;
                    }
                }
            }
        }

        if (openPathNodeListLength == 0) {
            break;
        }
    }

    if (openPathNodeListLength != 0) {
        unsigned char* v39 = rotations;
        int index = 0;
        for (; index < 800; index++) {
            if (temp.tile == from) {
                break;
            }

            if (v39 != NULL) {
                *v39 = temp.rotation & 0xFF;
                v39 += 1;
            }

            int j = 0;
            while (gClosedPathNodeList[j].tile != temp.from) {
                j++;
            }

            PathNode* v36 = &(gClosedPathNodeList[j]);
            memcpy(&temp, v36, sizeof(temp));
        }

        if (rotations != NULL) {
            // Looks like array resevering, probably because A* finishes it's path from end to start,
            // this probably reverses it start-to-end.
            unsigned char* beginning = rotations;
            unsigned char* ending = rotations + index - 1;
            int middle = index / 2;
            for (int index = 0; index < middle; index++) {
                unsigned char rotation = *ending;
                *ending = *beginning;
                *beginning = rotation;

                ending -= 1;
                beginning += 1;
            }
        }

        return index;
    }

    return 0;
}

// 0x41633C
int sub_41633C(int x1, int y1, int x2, int y2)
{
    int dx = x2 - x1;
    if (dx < 0) {
        dx = -dx;
    }

    int dy = y2 - y1;
    if (dy < 0) {
        dy = -dy;
    }

    int dm = (dx <= dy) ? dx : dy;

    return dx + dy - (dm / 2);
}

// 0x416360
int sub_416360(int tile1, int tile2)
{
    int x1;
    int y1;
    tileToScreenXY(tile1, &x1, &y1, gElevation);

    int x2;
    int y2;
    tileToScreenXY(tile2, &x2, &y2, gElevation);

    return sub_41633C(x1, y1, x2, y2);
}

// 0x4163AC
int sub_4163AC(Object* a1, int from, int to, STRUCT_530014_28* pathNodes, Object** a5, int a6)
{
    return sub_4163C8(a1, from, to, pathNodes, a5, a6, sub_48B848);
}

// TODO: Rather complex, but understandable, needs testing.
//
// 0x4163C8
int sub_4163C8(Object* a1, int from, int to, STRUCT_530014_28* a4, Object** a5, int a6, Object* (*a7)(Object*, int, int))
{
    if (a5 != NULL) {
        Object* v11 = a7(a1, from, a1->elevation);
        if (v11 != NULL) {
            if (v11 != *a5 && (a6 != 32 || (v11->flags & 0x80000000) == 0)) {
                *a5 = v11;
                return 0;
            }
        }
    }

    int fromX;
    int fromY;
    tileToScreenXY(from, &fromX, &fromY, a1->elevation);
    fromX += 16;
    fromY += 8;

    int toX;
    int toY;
    tileToScreenXY(to, &toX, &toY, a1->elevation);
    toX += 16;
    toY += 8;

    int stepX;
    int deltaX = toX - fromX;
    if (deltaX > 0)
        stepX = 1;
    else if (deltaX < 0)
        stepX = -1;
    else
        stepX = 0;

    int stepY;
    int deltaY = toY - fromY;
    if (deltaY > 0)
        stepY = 1;
    else if (deltaY < 0)
        stepY = -1;
    else
        stepY = 0;

    int v48 = 2 * abs(toX - fromX);
    int v47 = 2 * abs(toY - fromY);

    int tileX = fromX;
    int tileY = fromY;

    int pathNodeIndex = 0;
    int v50 = from;
    int v22 = 0;
    int tile;

    if (v48 <= v47) {
        int middle = v48 - v47 / 2;
        while (true) {
            tile = tileFromScreenXY(tileX, tileY, a1->elevation);

            v22 += 1;
            if (v22 == a6) {
                if (pathNodeIndex >= 200) {
                    return 0;
                }

                if (a4 != NULL) {
                    STRUCT_530014_28* pathNode = &(a4[pathNodeIndex]);
                    pathNode->tile = tile;
                    pathNode->elevation = a1->elevation;

                    tileToScreenXY(tile, &fromX, &fromY, a1->elevation);
                    pathNode->x = tileX - fromX - 16;
                    pathNode->y = tileY - fromY - 8;
                }

                v22 = 0;
                pathNodeIndex++;
            }

            if (tileY == toY) {
                if (a5 != NULL) {
                    *a5 = NULL;
                }
                break;
            }

            if (middle >= 0) {
                tileX += stepX;
                middle -= v47;
            }

            tileY += stepY;
            middle += v48;

            if (tile != v50) {
                if (a5 != NULL) {
                    Object* obj = a7(a1, tile, a1->elevation);
                    if (obj != NULL) {
                        if (obj != *a5 && (a6 != 32 || (obj->flags & 0x80000000) == 0)) {
                            *a5 = obj;
                            break;
                        }
                    }
                }
                v50 = tile;
            }
        }
    } else {
        int middle = v47 - v48 / 2;
        while (true) {
            tile = tileFromScreenXY(tileX, tileY, a1->elevation);

            v22 += 1;
            if (v22 == a6) {
                if (pathNodeIndex >= 200) {
                    return 0;
                }

                if (a4 != NULL) {
                    STRUCT_530014_28* pathNode = &(a4[pathNodeIndex]);
                    pathNode->tile = tile;
                    pathNode->elevation = a1->elevation;

                    tileToScreenXY(tile, &fromX, &fromY, a1->elevation);
                    pathNode->x = tileX - fromX - 16;
                    pathNode->y = tileY - fromY - 8;
                }

                v22 = 0;
                pathNodeIndex++;
            }

            if (tileX == toX) {
                if (a5 != NULL) {
                    *a5 = NULL;
                }
                break;
            }

            if (middle >= 0) {
                tileY += stepY;
                middle -= v48;
            }

            tileX += stepX;
            middle += v47;

            if (tile != v50) {
                if (a5 != NULL) {
                    Object* obj = a7(a1, tile, a1->elevation);
                    if (obj != NULL) {
                        if (obj != *a5 && (a6 != 32 || (obj->flags & 0x80000000) == 0)) {
                            *a5 = obj;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (v22 != 0) {
        if (pathNodeIndex >= 200) {
            return 0;
        }

        if (a4 != NULL) {
            STRUCT_530014_28* pathNode = &(a4[pathNodeIndex]);
            pathNode->tile = tile;
            pathNode->elevation = a1->elevation;

            tileToScreenXY(tile, &fromX, &fromY, a1->elevation);
            pathNode->x = tileX - fromX - 16;
            pathNode->y = tileY - fromY - 8;
        }

        pathNodeIndex += 1;
    } else {
        if (pathNodeIndex > 0 && a4 != NULL) {
            a4[pathNodeIndex - 1].elevation = a1->elevation;
        }
    }

    return pathNodeIndex;
}

// 0x4167F8
int animateMoveObjectToObject(Object* a1, Object* a2, int a3, int anim, int animationSequenceIndex)
{
    int v8;
    int v10;
    int v13;
    STRUCT_530014* ptr;

    v8 = a2->flags & 0x01;
    a2->flags |= 0x01;

    v10 = sub_416DFC(a1, a2->tile, a2->elevation, -1, anim, 0, animationSequenceIndex);

    if (v8 == 0) {
        a2->flags &= ~0x01;
    }

    if (v10 == -1) {
        return -1;
    }

    ptr = &(stru_530014[v10]);
    v13 = (((a1->flags & 0x0800) != 0) + 1); // TODO: What the hell is this?
    ptr->field_1C -= v13;
    if (ptr->field_1C <= 0) {
        ptr->field_20 = -1000;
        sub_415B44(animationSequenceIndex, 0);
    }

    if (v13) {
        ptr->field_24 = tileGetTileInDirection(a2->tile, ptr->field_24 + v13 + ptr->field_1C + 3, 1);
    }

    if (v13 == 2) {
        ptr->field_24 = tileGetTileInDirection(ptr->field_24, ptr->rotations[ptr->field_1C], 1);
    }

    if (a3 != -1 && a3 < ptr->field_1C) {
        ptr->field_1C = a3;
    }

    return 0;
}

// 0x416CFC
int animateMoveObjectToTile(Object* obj, int tile, int elev, int a4, int anim, int animationSequenceIndex)
{
    STRUCT_530014* ptr;
    int v1;

    v1 = sub_416DFC(obj, tile, elev, -1, anim, 0, animationSequenceIndex);
    if (v1 == -1) {
        return -1;
    }

    if (sub_48B848(obj, tile, elev)) {
        ptr = &(stru_530014[v1]);
        ptr->field_1C--;
        if (ptr->field_1C <= 0) {
            ptr->field_20 = -1000;
            sub_415B44(animationSequenceIndex, 0);
        }

        ptr->field_24 = tileGetTileInDirection(tile, ptr->rotations[ptr->field_1C], 1);
        if (a4 != -1 && a4 < ptr->field_1C) {
            ptr->field_1C = a4;
        }
    }

    return 0;
}

// 0x416DFC
int sub_416DFC(Object* obj, int tile, int elev, int a3, int anim, int a5, int animationSequenceIndex)
{
    STRUCT_530014* ptr;

    if (dword_510718 == 24) {
        return -1;
    }

    ptr = &(stru_530014[dword_510718]);
    ptr->obj = obj;

    if (a5) {
        ptr->flags = 0x20;
    } else {
        ptr->flags = 0;
    }

    ptr->field_20 = -2000;
    ptr->fid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    ptr->field_10 = 0;
    ptr->field_14 = sub_418794(obj, ptr->fid);
    ptr->field_24 = tile;
    ptr->animationSequenceIndex = animationSequenceIndex;
    ptr->field_C = anim;

    ptr->field_1C = sub_415EE8(obj, obj->tile, tile, ptr->rotations, a5);
    if (ptr->field_1C == 0) {
        ptr->field_20 = -1000;
        return -1;
    }

    if (a3 != -1 && ptr->field_1C > a3) {
        ptr->field_1C = a3;
    }

    return dword_510718++;
}

// 0x416F54
int sub_416F54(Object* obj, int tile, int elevation, int anim, int animationSequenceIndex, int flags)
{
    if (dword_510718 == 24) {
        return -1;
    }

    STRUCT_530014* ptr = &(stru_530014[dword_510718]);
    ptr->obj = obj;
    ptr->flags = flags | 0x02;
    if (anim == -1) {
        ptr->fid = obj->fid;
        ptr->flags |= 0x04;
    } else {
        ptr->fid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    }
    ptr->field_20 = -2000;
    ptr->field_10 = 0;
    ptr->field_14 = sub_418794(obj, ptr->fid);
    ptr->animationSequenceIndex = animationSequenceIndex;

    int v15;
    if (((obj->fid & 0xF000000) >> 24) == OBJ_TYPE_CRITTER) {
        if (((obj->fid & 0xFF0000) >> 16) == ANIM_JUMP_BEGIN)
            v15 = 16;
        else
            v15 = 4;
    } else {
        v15 = 32;
    }

    ptr->field_1C = sub_4163AC(obj, obj->tile, tile, ptr->field_28, NULL, v15);
    if (ptr->field_1C == 0) {
        ptr->field_20 = -1000;
        return -1;
    }

    dword_510718++;

    return 0;
}

// 0x41712C
int sub_41712C(Object* obj, int tile, int elevation, int anim, int animationSequenceIndex)
{
    STRUCT_530014* ptr;

    if (dword_510718 == 24) {
        return -1;
    }

    ptr = &(stru_530014[dword_510718]);
    ptr->flags = 0x02;
    ptr->obj = obj;
    if (anim == -1) {
        ptr->fid = obj->fid;
        ptr->flags |= 0x04;
    } else {
        ptr->fid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    }
    ptr->field_20 = -2000;
    ptr->field_10 = 0;
    ptr->field_14 = sub_418794(obj, ptr->fid);
    ptr->animationSequenceIndex = animationSequenceIndex;
    // TODO: Incomplete.
    // ptr->field_1C = sub_41695C(obj, obj->tile_index, obj->elevation, tile, elevation, ptr->field_28, 0);
    if (ptr->field_1C == 0) {
        ptr->field_20 = -1000;
        return -1;
    }

    dword_510718++;

    return 0;
}

// 0x417248
int sub_417248(Object* obj, int anim, int a3)
{
    STRUCT_530014* ptr;

    if (dword_510718 == 24) {
        return -1;
    }

    if (sub_418708(obj->tile, obj->elevation) == obj->elevation) {
        return -1;
    }

    ptr = &(stru_530014[dword_510718]);
    ptr->flags = 0x02;
    ptr->obj = obj;
    if (anim == -1) {
        ptr->fid = obj->fid;
        ptr->flags |= 0x04;
    } else {
        ptr->fid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    }
    ptr->field_20 = -2000;
    ptr->field_10 = 0;
    ptr->field_14 = sub_418794(obj, ptr->fid);
    ptr->animationSequenceIndex = a3;
    ptr->field_1C = sub_4163C8(obj, obj->tile, obj->tile, ptr->field_28, 0, 16, sub_48B848);
    if (ptr->field_1C == 0) {
        ptr->field_20 = -1000;
        return -1;
    }

    dword_510718++;

    return 0;
}

// 0x417360
void sub_417360(int index)
{
    STRUCT_530014* p530014 = &(stru_530014[index]);
    Object* object = p530014->obj;

    Rect dirty;
    Rect temp;

    if (p530014->field_20 == -2000) {
        objectSetLocation(object, object->tile, object->elevation, &dirty);

        objectSetFrame(object, 0, &temp);
        rectUnion(&dirty, &temp, &dirty);

        objectSetRotation(object, p530014->rotations[0], &temp);
        rectUnion(&dirty, &temp, &dirty);

        int fid = buildFid((object->fid & 0xF000000) >> 24, object->fid & 0xFFF, p530014->field_C, (object->fid & 0xF000) >> 12, object->rotation + 1);
        objectSetFid(object, fid, &temp);
        rectUnion(&dirty, &temp, &dirty);

        p530014->field_20 = 0;
    } else {
        objectSetNextFrame(object, &dirty);
    }

    int frameX;
    int frameY;

    CacheEntry* cacheHandle;
    Art* art = artLock(object->fid, &cacheHandle);
    if (art != NULL) {
        artGetFrameOffsets(art, object->frame, object->rotation, &frameX, &frameY);
        artUnlock(cacheHandle);
    } else {
        frameX = 0;
        frameY = 0;
    }

    sub_489FF8(object, frameX, frameY, &temp);
    rectUnion(&dirty, &temp, &dirty);

    int rotation = p530014->rotations[p530014->field_20];
    int y = dword_51D984[rotation];
    int x = dword_51D96C[rotation];
    if (x > 0 && x <= object->x || x < 0 && x >= object->x || y > 0 && y <= object->y || y < 0 && y >= object->y) {
        x = object->x - x;
        y = object->y - y;

        int v10 = tileGetTileInDirection(object->tile, rotation, 1);
        Object* v12 = sub_48B848(object, v10, object->elevation);
        if (v12 != NULL) {
            if (sub_415E24(object, v12) == 0) {
                p530014->field_1C = sub_415EE8(object, object->tile, p530014->field_24, p530014->rotations, 1);
                if (p530014->field_1C != 0) {
                    objectSetLocation(object, object->tile, object->elevation, &temp);
                    rectUnion(&dirty, &temp, &dirty);

                    objectSetFrame(object, 0, &temp);
                    rectUnion(&dirty, &temp, &dirty);

                    objectSetRotation(object, p530014->rotations[0], &temp);
                    rectUnion(&dirty, &temp, &dirty);

                    p530014->field_20 = 0;
                } else {
                    p530014->field_20 = -1000;
                }
                v10 = -1;
            } else {
                sub_49CCB8(object, v12, 0);
            }
        }

        if (v10 != -1) {
            objectSetLocation(object, v10, object->elevation, &temp);
            rectUnion(&dirty, &temp, &dirty);

            int v17 = 0;
            if (isInCombat() && ((object->fid & 0xF000000) >> 24) == OBJ_TYPE_CRITTER) {
                int v18 = critterGetMovementPointCostAdjustedForCrippledLegs(object, 1);
                if (dword_56D39C < v18) {
                    int ap = object->data.critter.combat.ap;
                    int v20 = v18 - dword_56D39C;
                    dword_56D39C = 0;
                    if (v20 > ap) {
                        object->data.critter.combat.ap = 0;
                    } else {
                        object->data.critter.combat.ap = ap - v20;
                    }
                } else {
                    dword_56D39C -= v18;
                }

                if (object == gDude) {
                    interfaceRenderActionPoints(gDude->data.critter.combat.ap, dword_56D39C);
                }

                v17 = (object->data.critter.combat.ap + dword_56D39C) <= 0;
            }

            p530014->field_20 += 1;

            if (p530014->field_20 == p530014->field_1C || v17) {
                p530014->field_20 = -1000;
            } else {
                objectSetRotation(object, p530014->rotations[p530014->field_20], &temp);
                rectUnion(&dirty, &temp, &dirty);

                sub_489FF8(object, x, y, &temp);
                rectUnion(&dirty, &temp, &dirty);
            }
        }
    }

    tileWindowRefreshRect(&dirty, object->elevation);
    if (p530014->field_20 == -1000) {
        sub_415B44(p530014->animationSequenceIndex, 1);
    }
}

// 0x4177C0
void sub_4177C0(int index)
{
    STRUCT_530014* p530014 = &(stru_530014[index]);
    Object* object = p530014->obj;

    Rect dirtyRect;
    Rect temp;

    if (p530014->field_20 == -2000) {
        objectSetFid(object, p530014->fid, &dirtyRect);
        p530014->field_20 = 0;
    } else {
        objectGetRect(object, &dirtyRect);
    }

    CacheEntry* cacheHandle;
    Art* art = artLock(object->fid, &cacheHandle);
    if (art != NULL) {
        int lastFrame = artGetFrameCount(art) - 1;
        artUnlock(cacheHandle);

        if ((p530014->flags & 0x04) == 0 && ((p530014->flags & 0x10) == 0 || lastFrame > object->frame)) {
            objectSetNextFrame(object, &temp);
            rectUnion(&dirtyRect, &temp, &dirtyRect);
        }

        if (p530014->field_20 < p530014->field_1C) {
            STRUCT_530014_28* v12 = &(p530014->field_28[p530014->field_20]);

            objectSetLocation(object, v12->tile, v12->elevation, &temp);
            rectUnion(&dirtyRect, &temp, &dirtyRect);

            sub_489FF8(object, v12->x, v12->y, &temp);
            rectUnion(&dirtyRect, &temp, &dirtyRect);

            p530014->field_20++;
        }

        if (p530014->field_20 == p530014->field_1C && ((p530014->flags & 0x10) == 0 || object->frame == lastFrame)) {
            p530014->field_20 = -1000;
        }

        tileWindowRefreshRect(&dirtyRect, p530014->obj->elevation);

        if (p530014->field_20 == -1000) {
            sub_415B44(p530014->animationSequenceIndex, 1);
        }
    }
}

// 0x4179B8
int sub_4179B8(Object* obj, int anim, int animationSequenceIndex, int flags)
{
    if (dword_510718 == 24) {
        return -1;
    }

    STRUCT_530014* ptr = &(stru_530014[dword_510718]);

    int fid;
    if (anim == ANIM_TAKE_OUT) {
        ptr->flags = 0;
        fid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, ANIM_TAKE_OUT, flags, obj->rotation + 1);
    } else {
        ptr->flags = flags;
        fid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    }

    if (!artExists(fid)) {
        return -1;
    }

    ptr->obj = obj;
    ptr->fid = fid;
    ptr->animationSequenceIndex = animationSequenceIndex;
    ptr->field_10 = 0;
    ptr->field_14 = sub_418794(obj, ptr->fid);
    ptr->field_20 = 0;
    ptr->field_1C = 0;

    dword_510718++;

    return 0;
}

// 0x417B30
void sub_417B30()
{
    if (dword_510718 == 0) {
        return;
    }

    dword_510728 = 1;

    for (int index = 0; index < dword_510718; index++) {
        STRUCT_530014* p530014 = &(stru_530014[index]);
        if (p530014->field_20 == -1000) {
            continue;
        }

        Object* object = p530014->obj;

        int time = sub_4C9370();
        if (getTicksBetween(time, p530014->field_10) < p530014->field_14) {
            continue;
        }

        p530014->field_10 = time;

        if (animationRunSequence(p530014->animationSequenceIndex) == -1) {
            continue;
        }

        if (p530014->field_1C > 0) {
            if ((p530014->flags & 0x02) != 0) {
                sub_4177C0(index);
            } else {
                int savedTile = object->tile;
                sub_417360(index);
                if (savedTile != object->tile) {
                    scriptsExecSpatialProc(object, object->tile, object->elevation);
                }
            }
            continue;
        }

        if (p530014->field_20 == 0) {
            for (int index = 0; index < dword_510718; index++) {
                STRUCT_530014* other530014 = &(stru_530014[index]);
                if (object == other530014->obj && other530014->field_20 == -2000) {
                    other530014->field_20 = -1000;
                    sub_415B44(other530014->animationSequenceIndex, 1);
                }
            }
            p530014->field_20 = -2000;
        }

        Rect dirtyRect;
        Rect tempRect;

        objectGetRect(object, &dirtyRect);

        if (object->fid == p530014->fid) {
            if ((p530014->flags & 0x01) == 0) {
                CacheEntry* cacheHandle;
                Art* art = artLock(object->fid, &cacheHandle);
                if (art != NULL) {
                    if ((p530014->flags & 0x80) == 0 && object->frame == artGetFrameCount(art) - 1) {
                        p530014->field_20 = -1000;
                        artUnlock(cacheHandle);

                        if ((p530014->flags & 0x40) != 0 && objectHide(object, &tempRect) == 0) {
                            tileWindowRefreshRect(&tempRect, object->elevation);
                        }

                        sub_415B44(p530014->animationSequenceIndex, 1);
                        continue;
                    } else {
                        objectSetNextFrame(object, &tempRect);
                        rectUnion(&dirtyRect, &tempRect, &dirtyRect);

                        int frameX;
                        int frameY;
                        artGetFrameOffsets(art, object->frame, object->rotation, &frameX, &frameY);

                        sub_489FF8(object, frameX, frameY, &tempRect);
                        rectUnion(&dirtyRect, &tempRect, &dirtyRect);

                        artUnlock(cacheHandle);
                    }
                }

                tileWindowRefreshRect(&dirtyRect, gElevation);

                continue;
            }

            if ((p530014->flags & 0x80) != 0 || object->frame != 0) {
                int x;
                int y;

                CacheEntry* cacheHandle;
                Art* art = artLock(object->fid, &cacheHandle);
                if (art != NULL) {
                    artGetFrameOffsets(art, object->frame, object->rotation, &x, &y);
                    artUnlock(cacheHandle);
                }

                objectSetPrevFrame(object, &tempRect);
                rectUnion(&dirtyRect, &tempRect, &dirtyRect);

                sub_489FF8(object, -x, -y, &tempRect);
                rectUnion(&dirtyRect, &tempRect, &dirtyRect);

                tileWindowRefreshRect(&dirtyRect, gElevation);
                continue;
            }

            p530014->field_20 = -1000;
            sub_415B44(p530014->animationSequenceIndex, 1);
        } else {
            int x;
            int y;

            CacheEntry* cacheHandle;
            Art* art = artLock(object->fid, &cacheHandle);
            if (art != NULL) {
                artGetRotationOffsets(art, object->rotation, &x, &y);
                artUnlock(cacheHandle);
            } else {
                x = 0;
                y = 0;
            }

            Rect v29;
            objectSetFid(object, p530014->fid, &v29);
            rectUnion(&dirtyRect, &v29, &dirtyRect);

            art = artLock(object->fid, &cacheHandle);
            if (art != NULL) {
                int frame;
                if ((p530014->flags & 0x01) != 0) {
                    frame = artGetFrameCount(art) - 1;
                } else {
                    frame = 0;
                }

                objectSetFrame(object, frame, &v29);
                rectUnion(&dirtyRect, &v29, &dirtyRect);

                int frameX;
                int frameY;
                artGetFrameOffsets(art, object->frame, object->rotation, &frameX, &frameY);

                Rect v19;
                sub_489FF8(object, x + frameX, y + frameY, &v19);
                rectUnion(&dirtyRect, &v19, &dirtyRect);

                artUnlock(cacheHandle);
            } else {
                objectSetFrame(object, 0, &v29);
                rectUnion(&dirtyRect, &v29, &dirtyRect);
            }

            tileWindowRefreshRect(&dirtyRect, gElevation);
        }
    }

    dword_510728 = 0;

    sub_417F18();
}

// 0x417F18
void sub_417F18()
{
    for (int index = 0; index < ANIMATION_SEQUENCE_LIST_CAPACITY; index++) {
        AnimationSequence* animationSequence = &(gAnimationSequences[index]);
        if ((animationSequence->flags & 0x20) != 0) {
            animationSequence->flags = 0;
        }
    }

    int index = 0;
    for (; index < dword_510718; index++) {
        if (stru_530014[index].field_20 == -1000) {
            int v2 = index + 1;
            for (; v2 < dword_510718; v2++) {
                if (stru_530014[v2].field_20 != -1000) {
                    break;
                }
            }

            if (v2 == dword_510718) {
                break;
            }

            if (index != v2) {
                memcpy(&(stru_530014[index]), &(stru_530014[v2]), sizeof(STRUCT_530014));
                stru_530014[v2].field_20 = -1000;
                stru_530014[v2].flags = 0;
            }
        }
    }
    dword_510718 = index;
}

// 0x417FFC
int sub_417FFC(int* a1)
{
    int x;
    int y;
    mouseGetPosition(&x, &y);

    int tile = tileFromScreenXY(x, y, gElevation);
    if (tile == -1) {
        return -1;
    }

    if (isInCombat()) {
        if (*a1 != -1) {
            if (gPressedPhysicalKeys[DIK_LCONTROL] || gPressedPhysicalKeys[DIK_RCONTROL]) {
                int hitMode;
                bool aiming;
                interfaceGetCurrentHitMode(&hitMode, &aiming);

                int v6 = sub_478040(gDude, hitMode, aiming);
                *a1 = *a1 - v6;
                if (*a1 <= 0) {
                    return -1;
                }
            }
        }
    } else {
        bool interruptWalk;
        configGetBool(&gGameConfig, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_INTERRUPT_WALK_KEY, &interruptWalk);
        if (interruptWalk) {
            reg_anim_clear(gDude);
        }
    }

    return tile;
}

// 0x4180B4
int sub_4180B4(int a1)
{
    int v1;
    int tile = sub_417FFC(&v1);
    if (tile == -1) {
        return -1;
    }

    if (dword_51072C == tile) {
        return sub_41810C(a1);
    }

    dword_51072C = tile;

    reg_anim_begin(2);

    reg_anim_obj_move_to_tile(gDude, tile, gDude->elevation, a1, 0);

    return reg_anim_end();
}

// 0x41810C
int sub_41810C(int a1)
{
    int a4;
    int tile_num;

    a4 = a1;
    tile_num = sub_417FFC(&a4);
    if (tile_num == -1) {
        return -1;
    }

    if (!perkGetRank(gDude, PERK_SILENT_RUNNING)) {
        dudeDisableState(0);
    }

    reg_anim_begin(2);

    reg_anim_obj_run_to_tile(gDude, tile_num, gDude->elevation, a4, 0);

    return reg_anim_end();
}

// 0x418168
void sub_418168()
{
    if (dword_5186CC != 0) {
        return;
    }

    if (isInCombat()) {
        return;
    }

    if (sub_4D2918() != 2) {
        return;
    }

    if ((gDude->flags & 0x01) != 0) {
        return;
    }

    unsigned int v0 = sub_4C9410();
    if (getTicksBetween(v0, dword_510730) <= dword_510734) {
        return;
    }

    dword_510730 = v0;

    int v5 = 0;
    Object* object = objectFindFirstAtElevation(gDude->elevation);
    while (object != NULL) {
        if (v5 >= 100) {
            break;
        }

        if ((object->flags & 0x01) == 0 && ((object->fid & 0xF000000) >> 24) == OBJ_TYPE_CRITTER && ((object->fid & 0xFF0000) >> 16) == 0 && !critterIsDead(object)) {
            Rect rect;
            objectGetRect(object, &rect);

            Rect intersection;
            if (rectIntersection(&rect, &stru_6AC9F0, &intersection) == 0 && (gMapHeader.field_34 != 97 || object->pid != 0x10000FA)) {
                dword_56C7E0[v5++] = object;
            }
        }

        object = objectFindNextAtElevation();
    }

    int v13;
    if (v5 != 0) {
        int r = randomBetween(0, v5 - 1);
        Object* object = dword_56C7E0[r];

        reg_anim_begin(0x201);

        bool v8 = false;
        if (object == gDude) {
            v8 = true;
        } else {
            char v15[16];
            v15[0] = '\0';
            artCopyFileName(1, object->fid & 0xFFF, v15);
            if (v15[0] == 'm' || v15[0] == 'M') {
                if (objectGetDistanceBetween(object, gDude) < critterGetStat(gDude, STAT_PERCEPTION) * 2) {
                    v8 = true;
                }
            }
        }

        if (v8) {
            const char* sfx = sfxBuildCharName(object, ANIM_STAND, CHARACTER_SOUND_EFFECT_UNUSED);
            reg_anim_play_sfx(object, sfx, 0);
        }

        reg_anim_animate(object, 0, 0);
        reg_anim_end();

        v13 = 20 / v5;
    } else {
        v13 = 7;
    }

    if (v13 < 1) {
        v13 = 1;
    } else if (v13 > 7) {
        v13 = 7;
    }

    dword_510734 = randomBetween(0, 3000) + 1000 * v13;
}

// 0x418378
void sub_418378(Object* obj, int rotation, int fid)
{
    Rect rect;

    objectSetRotation(obj, rotation, &rect);

    int x = 0;
    int y = 0;

    int v4 = (obj->fid & 0xF000) >> 12;
    if (v4 != 0) {
        if (fid == -1) {
            int takeOutFid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, ANIM_TAKE_OUT, v4, obj->rotation + 1);
            CacheEntry* takeOutFrmHandle;
            Art* takeOutFrm = artLock(takeOutFid, &takeOutFrmHandle);
            if (takeOutFrm != NULL) {
                int frameCount = artGetFrameCount(takeOutFrm);
                for (int frame = 0; frame < frameCount; frame++) {
                    int offsetX;
                    int offsetY;
                    artGetFrameOffsets(takeOutFrm, frame, obj->rotation, &offsetX, &offsetY);
                    x += offsetX;
                    y += offsetY;
                }
                artUnlock(takeOutFrmHandle);

                CacheEntry* standFrmHandle;
                int standFid = buildFid((obj->fid & 0xF000000) >> 24, obj->fid & 0xFFF, ANIM_STAND, 0, obj->rotation + 1);
                Art* standFrm = artLock(standFid, &standFrmHandle);
                if (standFrm != NULL) {
                    int offsetX;
                    int offsetY;
                    if (artGetRotationOffsets(standFrm, obj->rotation, &offsetX, &offsetY) == 0) {
                        x += offsetX;
                        y += offsetY;
                    }
                    artUnlock(standFrmHandle);
                }
            }
        }
    }

    if (fid == -1) {
        int anim;
        if (((obj->fid & 0xFF0000) >> 16) == ANIM_FIRE_DANCE) {
            anim = ANIM_FIRE_DANCE;
        } else {
            anim = ANIM_STAND;
        }
        fid = buildFid((obj->fid & 0xF000000) >> 24, (obj->fid & 0xFFF), anim, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    }

    Rect temp;
    objectSetFid(obj, fid, &temp);
    rectUnion(&rect, &temp, &rect);

    objectSetLocation(obj, obj->tile, obj->elevation, &temp);
    rectUnion(&rect, &temp, &rect);

    objectSetFrame(obj, 0, &temp);
    rectUnion(&rect, &temp, &rect);

    sub_489FF8(obj, x, y, &temp);
    rectUnion(&rect, &temp, &rect);

    tileWindowRefreshRect(&rect, obj->elevation);
}

// 0x418574
void sub_418574(Object* a1)
{
    reg_anim_begin(2);

    int anim;
    if ((a1->fid & 0xFF0000) >> 16 == ANIM_FALL_BACK) {
        anim = ANIM_BACK_TO_STANDING;
    } else {
        anim = ANIM_PRONE_TO_STANDING;
    }

    reg_anim_animate(a1, anim, 0);
    reg_anim_end();
    a1->data.critter.combat.results &= ~DAM_KNOCKED_DOWN;
}

// 0x4185EC
int actionRotate(Object* obj, int delta, int animationSequenceIndex)
{
    if (!sub_42DD80(obj)) {
        int rotation = obj->rotation + delta;
        if (rotation >= ROTATION_COUNT) {
            rotation = ROTATION_NE;
        } else if (rotation < 0) {
            rotation = ROTATION_NW;
        }

        sub_418378(obj, rotation, -1);
    }

    sub_415B44(animationSequenceIndex, 0);

    return 0;
}

// 0x418660
int sub_418660(Object* obj, int animationSequenceIndex, int fid)
{
    Rect rect;
    Rect v7;

    if ((fid & 0xFF0000) >> 16) {
        objectSetFid(obj, fid, &rect);
        objectSetFrame(obj, 0, &v7);
        rectUnion(&rect, &v7, &rect);
        tileWindowRefreshRect(&rect, obj->elevation);
    } else {
        sub_418378(obj, obj->rotation, fid);
    }

    sub_415B44(animationSequenceIndex, 0);

    return 0;
}

// 0x4186CC
void sub_4186CC()
{
    dword_510724 = 1;
    gAnimationSequenceCurrentIndex = -1;

    for (int index = 0; index < ANIMATION_SEQUENCE_LIST_CAPACITY; index++) {
        sub_415B9C(index);
    }

    dword_510724 = 0;
    dword_510718 = 0;
}

// 0x418708
int sub_418708(int tile, int elevation)
{
    for (; elevation > 0; elevation--) {
        int x;
        int y;
        tileToScreenXY(tile, &x, &y, elevation);

        int v4 = sub_4B1F04(x + 2, y + 8, elevation);
        int fid = buildFid(4, dword_631E40[elevation]->field_0[v4] & 0xFFF, 0, 0, 0);
        if (fid != buildFid(4, 1, 0, 0, 0)) {
            break;
        }
    }
    return elevation;
}

// 0x418794
unsigned int sub_418794(Object* object, int fid)
{
    int fps;

    CacheEntry* handle;
    Art* frm = artLock(fid, &handle);
    if (frm != NULL) {
        fps = artGetFramesPerSecond(frm);
        artUnlock(handle);
    } else {
        fps = 10;
    }

    if (isInCombat()) {
        if (((fid & 0xFF0000) >> 16) == 1) {
            int playerSpeedup = 0;
            configGetInt(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_PLAYER_SPEEDUP_KEY, &playerSpeedup);

            if (object != gDude || playerSpeedup == 1) {
                int combatSpeed = 0;
                configGetInt(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_COMBAT_SPEED_KEY, &combatSpeed);
                fps += combatSpeed;
            }
        }
    }

    return 1000 / fps;
}