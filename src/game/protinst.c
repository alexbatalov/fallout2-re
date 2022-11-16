#include "game/protinst.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "game/anim.h"
#include "plib/color/color.h"
#include "game/combat.h"
#include "game/critter.h"
#include "plib/gnw/debug.h"
#include "game/display.h"
#include "game/game.h"
#include "game/gsound.h"
#include "plib/gnw/rect.h"
#include "game/intface.h"
#include "game/item.h"
#include "game/map.h"
#include "game/message.h"
#include "game/object.h"
#include "game/palette.h"
#include "game/perk.h"
#include "game/proto.h"
#include "game/queue.h"
#include "game/roll.h"
#include "game/scripts.h"
#include "game/skill.h"
#include "game/stat.h"
#include "game/tile.h"
#include "game/worldmap.h"

static int obj_use_book(Object* item_obj);
static int obj_use_flare(Object* critter_obj, Object* item_obj);
static int obj_use_explosive(Object* explosive);
static int obj_use_misc_item(Object* item_obj);
static int protinstTestDroppedExplosive(Object* a1);
static int protinst_default_use_item(Object* a1, Object* a2, Object* item);
static int set_door_state_open(Object* a1, Object* a2);
static int set_door_state_closed(Object* a1, Object* a2);
static int check_door_state(Object* a1, Object* a2);

// 0x49A9A0
int obj_sid(Object* object, int* sidPtr)
{
    *sidPtr = object->sid;
    if (*sidPtr == -1) {
        return -1;
    }

    return 0;
}

// 0x49A9B4
int obj_new_sid(Object* object, int* sidPtr)
{
    *sidPtr = -1;

    Proto* proto;
    if (proto_ptr(object->pid, &proto) == -1) {
        return -1;
    }

    int sid;
    int objectType = PID_TYPE(object->pid);
    if (objectType < OBJ_TYPE_TILE) {
        sid = proto->sid;
    } else if (objectType == OBJ_TYPE_TILE) {
        sid = proto->tile.sid;
    } else if (objectType == OBJ_TYPE_MISC) {
        sid = -1;
    } else {
        assert(false && "Should be unreachable");
    }

    if (sid == -1) {
        return -1;
    }

    int scriptType = SID_TYPE(sid);
    if (scr_new(sidPtr, scriptType) == -1) {
        return -1;
    }

    Script* script;
    if (scr_ptr(*sidPtr, &script) == -1) {
        return -1;
    }

    script->field_14 = sid & 0xFFFFFF;

    if (objectType == OBJ_TYPE_CRITTER) {
        object->field_80 = script->field_14;
    }

    if (scriptType == SCRIPT_TYPE_SPATIAL) {
        script->sp.built_tile = builtTileCreate(object->tile, object->elevation);
        script->sp.radius = 3;
    }

    if (object->id == -1) {
        object->id = new_obj_id();
    }

    script->field_1C = object->id;
    script->owner = object;

    scr_find_str_run_info(sid & 0xFFFFFF, &(script->field_50), *sidPtr);

    return 0;
}

// 0x49AAC0
int obj_new_sid_inst(Object* obj, int scriptType, int a3)
{
    if (a3 == -1) {
        return -1;
    }

    int sid;
    if (scr_new(&sid, scriptType) == -1) {
        return -1;
    }

    Script* script;
    if (scr_ptr(sid, &script) == -1) {
        return -1;
    }

    script->field_14 = a3;
    if (scriptType == SCRIPT_TYPE_SPATIAL) {
        script->sp.built_tile = builtTileCreate(obj->tile, obj->elevation);
        script->sp.radius = 3;
    }

    obj->sid = sid;

    obj->id = new_obj_id();
    script->field_1C = obj->id;

    script->owner = obj;

    scr_find_str_run_info(a3 & 0xFFFFFF, &(script->field_50), sid);

    if (PID_TYPE(obj->pid) == OBJ_TYPE_CRITTER) {
        obj->field_80 = script->field_14;
    }

    return 0;
}

// 0x49AC3C
int obj_look_at(Object* a1, Object* a2)
{
    return obj_look_at_func(a1, a2, display_print);
}

// 0x49AC4C
int obj_look_at_func(Object* a1, Object* a2, void (*a3)(char* string))
{
    if (critter_is_dead(a1)) {
        return -1;
    }

    if (FID_TYPE(a2->fid) == OBJ_TYPE_TILE) {
        return -1;
    }

    Proto* proto;
    if (proto_ptr(a2->pid, &proto) == -1) {
        return -1;
    }

    bool scriptOverrides = false;

    if (a2->sid != -1) {
        scr_set_objs(a2->sid, a1, a2);
        exec_script_proc(a2->sid, SCRIPT_PROC_LOOK_AT);

        Script* script;
        if (scr_ptr(a2->sid, &script) == -1) {
            return -1;
        }

        scriptOverrides = script->scriptOverrides;
    }

    if (!scriptOverrides) {
        MessageListItem messageListItem;

        if (PID_TYPE(a2->pid) == OBJ_TYPE_CRITTER && critter_is_dead(a2)) {
            messageListItem.num = 491 + roll_random(0, 1);
        } else {
            messageListItem.num = 490;
        }

        if (message_search(&proto_main_msg_file, &messageListItem)) {
            const char* objectName = object_name(a2);

            char formattedText[260];
            sprintf(formattedText, messageListItem.text, objectName);

            a3(formattedText);
        }
    }

    return -1;
}

// 0x49AD78
int obj_examine(Object* a1, Object* a2)
{
    return obj_examine_func(a1, a2, display_print);
}

// Performs examine (reading description) action and passes resulting text
// to given callback.
//
// [critter] is a critter who's performing an action. Can be NULL.
// [fn] can be called up to three times when [a2] is an ammo.
//
// 0x49AD88
int obj_examine_func(Object* critter, Object* target, void (*fn)(char* string))
{
    if (critter_is_dead(critter)) {
        return -1;
    }

    if (FID_TYPE(target->fid) == OBJ_TYPE_TILE) {
        return -1;
    }

    bool scriptOverrides = false;
    if (target->sid != -1) {
        scr_set_objs(target->sid, critter, target);
        exec_script_proc(target->sid, SCRIPT_PROC_DESCRIPTION);

        Script* script;
        if (scr_ptr(target->sid, &script) == -1) {
            return -1;
        }

        scriptOverrides = script->scriptOverrides;
    }

    if (!scriptOverrides) {
        char* description = object_description(target);
        if (description != NULL && strcmp(description, proto_none_str) == 0) {
            description = NULL;
        }

        if (description == NULL || *description == '\0') {
            MessageListItem messageListItem;
            messageListItem.num = 493;
            if (!message_search(&proto_main_msg_file, &messageListItem)) {
                debug_printf("\nError: Can't find msg num!");
            }
            fn(messageListItem.text);
        } else {
            if (PID_TYPE(target->pid) != OBJ_TYPE_CRITTER || !critter_is_dead(target)) {
                fn(description);
            }
        }
    }

    if (critter == NULL || critter != obj_dude) {
        return 0;
    }

    char formattedText[260];

    int type = PID_TYPE(target->pid);
    if (type == OBJ_TYPE_CRITTER) {
        if (target != obj_dude && perk_level(obj_dude, PERK_AWARENESS) && !critter_is_dead(target)) {
            MessageListItem hpMessageListItem;

            if (critter_body_type(target) != BODY_TYPE_BIPED) {
                // It has %d/%d hps
                hpMessageListItem.num = 537;
            } else {
                // 535: He has %d/%d hps
                // 536: She has %d/%d hps
                hpMessageListItem.num = 535 + critterGetStat(target, STAT_GENDER);
            }

            Object* item2 = inven_right_hand(target);
            if (item2 != NULL && item_get_type(item2) != ITEM_TYPE_WEAPON) {
                item2 = NULL;
            }

            if (!message_search(&proto_main_msg_file, &hpMessageListItem)) {
                debug_printf("\nError: Can't find msg num!");
                exit(1);
            }

            if (item2 != NULL) {
                MessageListItem weaponMessageListItem;

                if (item_w_caliber(item2) != 0) {
                    weaponMessageListItem.num = 547; // and is wielding a %s with %d/%d shots of %s.
                } else {
                    weaponMessageListItem.num = 546; // and is wielding a %s.
                }

                if (!message_search(&proto_main_msg_file, &weaponMessageListItem)) {
                    debug_printf("\nError: Can't find msg num!");
                    exit(1);
                }

                char format[80];
                sprintf(format, "%s%s", hpMessageListItem.text, weaponMessageListItem.text);

                if (item_w_caliber(item2) != 0) {
                    const int ammoTypePid = item_w_ammo_pid(item2);
                    const char* ammoName = proto_name(ammoTypePid);
                    const int ammoCapacity = item_w_max_ammo(item2);
                    const int ammoQuantity = item_w_curr_ammo(item2);
                    const char* weaponName = object_name(item2);
                    const int maxiumHitPoints = critterGetStat(target, STAT_MAXIMUM_HIT_POINTS);
                    const int currentHitPoints = critterGetStat(target, STAT_CURRENT_HIT_POINTS);
                    sprintf(formattedText,
                        format,
                        currentHitPoints,
                        maxiumHitPoints,
                        weaponName,
                        ammoQuantity,
                        ammoCapacity,
                        ammoName);
                } else {
                    const char* weaponName = object_name(item2);
                    const int maxiumHitPoints = critterGetStat(target, STAT_MAXIMUM_HIT_POINTS);
                    const int currentHitPoints = critterGetStat(target, STAT_CURRENT_HIT_POINTS);
                    sprintf(formattedText,
                        format,
                        currentHitPoints,
                        maxiumHitPoints,
                        weaponName);
                }
            } else {
                MessageListItem endingMessageListItem;

                if (critter_is_crippled(target)) {
                    endingMessageListItem.num = 544; // ,
                } else {
                    endingMessageListItem.num = 545; // .
                }

                if (!message_search(&proto_main_msg_file, &endingMessageListItem)) {
                    debug_printf("\nError: Can't find msg num!");
                    exit(1);
                }

                const int maxiumHitPoints = critterGetStat(target, STAT_MAXIMUM_HIT_POINTS);
                const int currentHitPoints = critterGetStat(target, STAT_CURRENT_HIT_POINTS);
                sprintf(formattedText, hpMessageListItem.text, currentHitPoints, maxiumHitPoints);
                strcat(formattedText, endingMessageListItem.text);
            }
        } else {
            int v12 = 0;
            if (critter_is_crippled(target)) {
                v12 -= 2;
            }

            int v16;

            const int maxiumHitPoints = critterGetStat(target, STAT_MAXIMUM_HIT_POINTS);
            const int currentHitPoints = critterGetStat(target, STAT_CURRENT_HIT_POINTS);
            if (currentHitPoints <= 0 || critter_is_dead(target)) {
                v16 = 0;
            } else if (currentHitPoints == maxiumHitPoints) {
                v16 = 4;
            } else {
                v16 = (currentHitPoints * 3) / maxiumHitPoints + 1;
            }

            MessageListItem hpMessageListItem;
            hpMessageListItem.num = 500 + v16;
            if (!message_search(&proto_main_msg_file, &hpMessageListItem)) {
                debug_printf("\nError: Can't find msg num!");
                exit(1);
            }

            if (v16 > 4) {
                // Error: lookup_val out of range
                hpMessageListItem.num = 550;
                if (!message_search(&proto_main_msg_file, &hpMessageListItem)) {
                    debug_printf("\nError: Can't find msg num!");
                    exit(1);
                }

                debug_printf(hpMessageListItem.text);
                return 0;
            }

            MessageListItem v66;
            if (target == obj_dude) {
                // You look %s
                v66.num = 520 + v12;
                if (!message_search(&proto_main_msg_file, &v66)) {
                    debug_printf("\nError: Can't find msg num!");
                    exit(1);
                }

                sprintf(formattedText, v66.text, hpMessageListItem.text);
            } else {
                // %s %s
                v66.num = 521 + v12;
                if (!message_search(&proto_main_msg_file, &v66)) {
                    debug_printf("\nError: Can't find msg num!");
                    exit(1);
                }

                MessageListItem v63;
                v63.num = 522 + critterGetStat(target, STAT_GENDER);
                if (!message_search(&proto_main_msg_file, &v63)) {
                    debug_printf("\nError: Can't find msg num!");
                    exit(1);
                }

                sprintf(formattedText, v63.text, hpMessageListItem.text);
            }
        }

        if (critter_is_crippled(target)) {
            const int maxiumHitPoints = critterGetStat(target, STAT_MAXIMUM_HIT_POINTS);
            const int currentHitPoints = critterGetStat(target, STAT_CURRENT_HIT_POINTS);

            MessageListItem v63;
            v63.num = maxiumHitPoints >= currentHitPoints ? 531 : 530;

            if (target == obj_dude) {
                v63.num += 2;
            }

            if (!message_search(&proto_main_msg_file, &v63)) {
                debug_printf("\nError: Can't find msg num!");
                exit(1);
            }

            strcat(formattedText, v63.text);
        }

        fn(formattedText);
    } else if (type == OBJ_TYPE_SCENERY) {
        if (target->pid == PROTO_ID_CAR) {
            MessageListItem carMessageListItem;
            carMessageListItem.num = 549; // The car is running at %d%% power.

            int car = game_get_global_var(GVAR_PLAYER_GOT_CAR);
            if (car == 0) {
                carMessageListItem.num = 548; // The car doesn't look like it's working right now.
            }

            if (!message_search(&proto_main_msg_file, &carMessageListItem)) {
                debug_printf("\nError: Can't find msg num!");
                exit(1);
            }

            if (car != 0) {
                sprintf(formattedText, carMessageListItem.text, 100 * wmCarGasAmount() / 80000);
            } else {
                strcpy(formattedText, carMessageListItem.text);
            }

            fn(formattedText);
        }
    } else if (type == OBJ_TYPE_ITEM) {
        int itemType = item_get_type(target);
        if (itemType == ITEM_TYPE_WEAPON) {
            if (item_w_caliber(target) != 0) {
                MessageListItem weaponMessageListItem;
                weaponMessageListItem.num = 526;

                if (!message_search(&proto_main_msg_file, &weaponMessageListItem)) {
                    debug_printf("\nError: Can't find msg num!");
                    exit(1);
                }

                int ammoTypePid = item_w_ammo_pid(target);
                const char* ammoName = proto_name(ammoTypePid);
                int ammoCapacity = item_w_max_ammo(target);
                int ammoQuantity = item_w_curr_ammo(target);
                sprintf(formattedText, weaponMessageListItem.text, ammoQuantity, ammoCapacity, ammoName);
                fn(formattedText);
            }
        } else if (itemType == ITEM_TYPE_AMMO) {
            MessageListItem ammoMessageListItem;
            ammoMessageListItem.num = 510;

            if (!message_search(&proto_main_msg_file, &ammoMessageListItem)) {
                debug_printf("\nError: Can't find msg num!");
                exit(1);
            }

            sprintf(formattedText,
                ammoMessageListItem.text,
                item_a_ac_adjust(target));
            fn(formattedText);

            ammoMessageListItem.num++;
            if (!message_search(&proto_main_msg_file, &ammoMessageListItem)) {
                debug_printf("\nError: Can't find msg num!");
                exit(1);
            }

            sprintf(formattedText,
                ammoMessageListItem.text,
                item_a_dr_adjust(target));
            fn(formattedText);

            ammoMessageListItem.num++;
            if (!message_search(&proto_main_msg_file, &ammoMessageListItem)) {
                debug_printf("\nError: Can't find msg num!");
                exit(1);
            }

            sprintf(formattedText,
                ammoMessageListItem.text,
                item_a_dam_mult(target),
                item_a_dam_div(target));
            fn(formattedText);
        }
    }

    return 0;
}

// 0x49B650
int obj_pickup(Object* critter, Object* item)
{
    bool overriden = false;

    if (item->sid != -1) {
        scr_set_objs(item->sid, critter, item);
        exec_script_proc(item->sid, SCRIPT_PROC_PICKUP);

        Script* script;
        if (scr_ptr(item->sid, &script) == -1) {
            return -1;
        }

        overriden = script->scriptOverrides;
    }

    if (!overriden) {
        int rc;
        if (item->pid == PROTO_ID_MONEY) {
            int amount = item_caps_get_amount(item);
            if (amount <= 0) {
                amount = 1;
            }

            rc = item_add_mult(critter, item, amount);
            if (rc == 0) {
                item_caps_set_amount(item, 0);
            }
        } else {
            rc = item_add_mult(critter, item, 1);
        }

        if (rc == 0) {
            Rect rect;
            obj_disconnect(item, &rect);
            tile_refresh_rect(&rect, item->elevation);
        } else {
            MessageListItem messageListItem;
            // You cannot pick up that item. You are at your maximum weight capacity.
            messageListItem.num = 905;
            if (message_search(&proto_main_msg_file, &messageListItem)) {
                display_print(messageListItem.text);
            }
        }
    }

    return 0;
}

// 0x49B73C
int obj_remove_from_inven(Object* critter, Object* item)
{
    Rect updatedRect;
    int fid;
    int v11 = 0;
    if (inven_right_hand(critter) == item) {
        if (critter != obj_dude || intface_is_item_right_hand()) {
            fid = art_id(OBJ_TYPE_CRITTER, critter->fid & 0xFFF, FID_ANIM_TYPE(critter->fid), 0, critter->rotation);
            obj_change_fid(critter, fid, &updatedRect);
            v11 = 2;
        } else {
            v11 = 1;
        }
    } else if (inven_left_hand(critter) == item) {
        if (critter == obj_dude && !intface_is_item_right_hand()) {
            fid = art_id(OBJ_TYPE_CRITTER, critter->fid & 0xFFF, FID_ANIM_TYPE(critter->fid), 0, critter->rotation);
            obj_change_fid(critter, fid, &updatedRect);
            v11 = 2;
        } else {
            v11 = 1;
        }
    } else if (inven_worn(critter) == item) {
        if (critter == obj_dude) {
            int v5 = 1;

            Proto* proto;
            if (proto_ptr(0x1000000, &proto) != -1) {
                v5 = proto->fid;
            }

            fid = art_id(OBJ_TYPE_CRITTER, v5, FID_ANIM_TYPE(critter->fid), (critter->fid & 0xF000) >> 12, critter->rotation);
            obj_change_fid(critter, fid, &updatedRect);
            v11 = 3;
        }
    }

    int rc = item_remove_mult(critter, item, 1);

    if (v11 >= 2) {
        tile_refresh_rect(&updatedRect, critter->elevation);
    }

    if (v11 <= 2 && critter == obj_dude) {
        intface_update_items(false, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);
    }

    return rc;
}

// 0x49B8B0
int obj_drop(Object* a1, Object* a2)
{
    if (a2 == NULL) {
        return -1;
    }

    bool scriptOverrides = false;
    if (a1->sid != -1) {
        scr_set_objs(a1->sid, a2, NULL);
        exec_script_proc(a1->sid, SCRIPT_PROC_IS_DROPPING);

        Script* scr;
        if (scr_ptr(a1->sid, &scr) == -1) {
            return -1;
        }

        scriptOverrides = scr->scriptOverrides;
    }

    if (scriptOverrides) {
        return 0;
    }

    if (a2->sid != -1) {
        scr_set_objs(a2->sid, a1, a2);
        exec_script_proc(a2->sid, SCRIPT_PROC_DROP);

        Script* scr;
        if (scr_ptr(a2->sid, &scr) == -1) {
            return -1;
        }

        scriptOverrides = scr->scriptOverrides;
    }

    if (scriptOverrides) {
        return 0;
    }

    if (obj_remove_from_inven(a1, a2) == 0) {
        Object* owner = obj_top_environment(a1);
        if (owner == NULL) {
            owner = a1;
        }

        Rect updatedRect;
        obj_connect(a2, owner->tile, owner->elevation, &updatedRect);
        tile_refresh_rect(&updatedRect, owner->elevation);
    }

    return 0;
}

// 0x49B9A0
int obj_destroy(Object* obj)
{
    if (obj == NULL) {
        return -1;
    }

    int elev;
    Object* owner = obj->owner;
    if (owner != NULL) {
        obj_remove_from_inven(owner, obj);
    } else {
        elev = obj->elevation;
    }

    queue_remove(obj);

    Rect rect;
    obj_erase_object(obj, &rect);

    if (owner == NULL) {
        tile_refresh_rect(&rect, elev);
    }

    return 0;
}

// Read a book.
//
// 0x49B9F0
static int obj_use_book(Object* book)
{
    MessageListItem messageListItem;

    int messageId = -1;
    int skill;

    switch (book->pid) {
    case PROTO_ID_BIG_BOOK_OF_SCIENCE:
        // You learn new science information.
        messageId = 802;
        skill = SKILL_SCIENCE;
        break;
    case PROTO_ID_DEANS_ELECTRONICS:
        // You learn a lot about repairing broken electronics.
        messageId = 803;
        skill = SKILL_REPAIR;
        break;
    case PROTO_ID_FIRST_AID_BOOK:
        // You learn new ways to heal injury.
        messageId = 804;
        skill = SKILL_FIRST_AID;
        break;
    case PROTO_ID_SCOUT_HANDBOOK:
        // You learn a lot about wilderness survival.
        messageId = 806;
        skill = SKILL_OUTDOORSMAN;
        break;
    case PROTO_ID_GUNS_AND_BULLETS:
        // You learn how to handle your guns better.
        messageId = 805;
        skill = SKILL_SMALL_GUNS;
        break;
    }

    if (messageId == -1) {
        return -1;
    }

    if (isInCombat()) {
        // You cannot do that in combat.
        messageListItem.num = 902;
        if (message_search(&proto_main_msg_file, &messageListItem)) {
            display_print(messageListItem.text);
        }

        return 0;
    }

    int increase = (100 - skill_level(obj_dude, skill)) / 10;
    if (increase <= 0) {
        messageId = 801;
    } else {
        if (perk_level(obj_dude, PERK_COMPREHENSION)) {
            increase = 150 * increase / 100;
        }

        for (int i = 0; i < increase; i++) {
            skill_inc_point_force(obj_dude, skill);
        }
    }

    palette_fade_to(black_palette);

    int intelligence = critterGetStat(obj_dude, STAT_INTELLIGENCE);
    inc_game_time_in_seconds(3600 * (11 - intelligence));

    scr_exec_map_update_scripts();

    palette_fade_to(cmap);

    // You read the book.
    messageListItem.num = 800;
    if (message_search(&proto_main_msg_file, &messageListItem)) {
        display_print(messageListItem.text);
    }

    messageListItem.num = messageId;
    if (message_search(&proto_main_msg_file, &messageListItem)) {
        display_print(messageListItem.text);
    }

    return 1;
}

// Light a flare.
//
// 0x49BBA8
static int obj_use_flare(Object* critter_obj, Object* flare)
{
    MessageListItem messageListItem;

    if (flare->pid != PROTO_ID_FLARE) {
        return -1;
    }

    if ((flare->flags & OBJECT_USED) != 0) {
        if (critter_obj == obj_dude) {
            // The flare is already lit.
            messageListItem.num = 588;
            if (message_search(&proto_main_msg_file, &messageListItem)) {
                display_print(messageListItem.text);
            }
        }
    } else {
        if (critter_obj == obj_dude) {
            // You light the flare.
            messageListItem.num = 588;
            if (message_search(&proto_main_msg_file, &messageListItem)) {
                display_print(messageListItem.text);
            }
        }

        flare->pid = PROTO_ID_LIT_FLARE;

        obj_set_light(flare, 8, 0x10000, NULL);
        queue_add(72000, flare, NULL, EVENT_TYPE_FLARE);
    }

    return 0;
}

// 0x49BC60
int obj_use_radio(Object* item)
{
    Script* scr;

    if (item->sid == -1) {
        return -1;
    }

    scr_set_objs(item->sid, obj_dude, item);
    exec_script_proc(item->sid, SCRIPT_PROC_USE);

    if (scr_ptr(item->sid, &scr) == -1) {
        return -1;
    }

    return 0;
}

// 0x49BCB4
static int obj_use_explosive(Object* explosive)
{
    MessageListItem messageListItem;

    int pid = explosive->pid;
    if (pid != PROTO_ID_DYNAMITE_I
        && pid != PROTO_ID_PLASTIC_EXPLOSIVES_I
        && pid != PROTO_ID_DYNAMITE_II
        && pid != PROTO_ID_PLASTIC_EXPLOSIVES_II) {
        return -1;
    }

    if ((explosive->flags & OBJECT_USED) != 0) {
        // The timer is already ticking.
        messageListItem.num = 590;
        if (message_search(&proto_main_msg_file, &messageListItem)) {
            display_print(messageListItem.text);
        }
    } else {
        int seconds = inven_set_timer(explosive);
        if (seconds != -1) {
            // You set the timer.
            messageListItem.num = 589;
            if (message_search(&proto_main_msg_file, &messageListItem)) {
                display_print(messageListItem.text);
            }

            if (pid == PROTO_ID_DYNAMITE_I) {
                explosive->pid = PROTO_ID_DYNAMITE_II;
            } else if (pid == PROTO_ID_PLASTIC_EXPLOSIVES_I) {
                explosive->pid = PROTO_ID_PLASTIC_EXPLOSIVES_II;
            }

            int delay = 10 * seconds;

            int roll;
            if (perkHasRank(obj_dude, PERK_DEMOLITION_EXPERT)) {
                roll = ROLL_SUCCESS;
            } else {
                roll = skill_result(obj_dude, SKILL_TRAPS, 0, NULL);
            }

            int eventType;
            switch (roll) {
            case ROLL_CRITICAL_FAILURE:
                delay = 0;
                eventType = EVENT_TYPE_EXPLOSION_FAILURE;
                break;
            case ROLL_FAILURE:
                eventType = EVENT_TYPE_EXPLOSION_FAILURE;
                delay /= 2;
                break;
            default:
                eventType = EVENT_TYPE_EXPLOSION;
                break;
            }

            queue_add(delay, explosive, NULL, eventType);
        }
    }

    return 2;
}

// Recharge car with given item
// Returns -1 when car cannot be recharged with given item.
// Returns 1 when car is recharged.
//
// 0x49BDE8
int obj_use_power_on_car(Object* item)
{
    // 0x49A990
    static MessageListItem messageListItem;

    int messageNum;

    bool isEnergy = false;
    int energyDensity;

    switch (item->pid) {
    case PROTO_ID_SMALL_ENERGY_CELL:
        energyDensity = 16000;
        isEnergy = true;
        break;
    case PROTO_ID_MICRO_FUSION_CELL:
        energyDensity = 40000;
        isEnergy = true;
        break;
    }

    if (!isEnergy) {
        return -1;
    }

    if (wmCarGasAmount() < CAR_FUEL_MAX) {
        int energy = item_w_curr_ammo(item) * energyDensity;
        int capacity = item_w_max_ammo(item);

        // NOTE: that function will never return -1
        if (wmCarFillGas(energy / capacity) == -1) {
            return -1;
        }

        // You charge the car with more power.
        messageNum = 595;
    } else {
        // The car is already full of power.
        messageNum = 596;
    }

    char* text = getmsg(&proto_main_msg_file, &messageListItem, messageNum);
    display_print(text);

    return 1;
}

// 0x49BE88
static int obj_use_misc_item(Object* item)
{

    if (item == NULL) {
        return -1;
    }

    switch (item->pid) {
    case PROTO_ID_RAMIREZ_BOX_CLOSED:
    case PROTO_ID_RAIDERS_MAP:
    case PROTO_ID_CATS_PAW_ISSUE_5:
    case PROTO_ID_PIP_BOY_LINGUAL_ENHANCER:
    case PROTO_ID_SURVEY_MAP:
    case PROTO_ID_PIP_BOY_MEDICAL_ENHANCER:
        if (item->sid == -1) {
            return 1;
        }

        scr_set_objs(item->sid, obj_dude, item);
        exec_script_proc(item->sid, SCRIPT_PROC_USE);

        Script* scr;
        if (scr_ptr(item->sid, &scr) == -1) {
            return -1;
        }

        return 1;
    }

    return -1;
}

// 0x49BF38
int protinst_use_item(Object* critter, Object* item)
{
    int rc;
    MessageListItem messageListItem;

    switch (item_get_type(item)) {
    case ITEM_TYPE_DRUG:
        rc = -1;
        break;
    case ITEM_TYPE_WEAPON:
    case ITEM_TYPE_MISC:
        rc = obj_use_book(item);
        if (rc != -1) {
            break;
        }

        rc = obj_use_flare(critter, item);
        if (rc == 0) {
            break;
        }

        rc = obj_use_misc_item(item);
        if (rc != -1) {
            break;
        }

        rc = obj_use_radio(item);
        if (rc == 0) {
            break;
        }

        rc = obj_use_explosive(item);
        if (rc == 0 || rc == 2) {
            break;
        }

        // TODO: Not sure about these two conditions.
        if (item_m_uses_charges(item)) {
            rc = item_m_use_charged_item(critter, item);
            if (rc == 0) {
                break;
            }
        }
        // FALLTHROUGH
    default:
        // That does nothing
        messageListItem.num = 582;
        if (message_search(&proto_main_msg_file, &messageListItem)) {
            display_print(messageListItem.text);
        }

        rc = -1;
    }

    return rc;
}

// 0x49BFE8
static int protinstTestDroppedExplosive(Object* a1)
{
    if (a1->pid == PROTO_ID_DYNAMITE_II || a1->pid == PROTO_ID_PLASTIC_EXPLOSIVES_II) {
        Attack attack;
        combat_ctd_init(&attack, obj_dude, 0, HIT_MODE_PUNCH, HIT_LOCATION_TORSO);
        attack.attackerFlags = DAM_HIT;
        attack.tile = obj_dude->tile;
        compute_explosion_on_extras(&attack, 0, 0, 1);

        int team = obj_dude->data.critter.combat.team;
        Object* v2 = NULL;
        for (int index = 0; index < attack.extrasLength; index++) {
            Object* v5 = attack.extras[index];
            if (v5 != obj_dude
                && v5->data.critter.combat.team != team
                && stat_result(v5, STAT_PERCEPTION, 0, NULL) >= 2) {
                critter_set_who_hit_me(v5, obj_dude);
                if (v2 == NULL) {
                    v2 = v5;
                }
            }
        }

        if (v2 != NULL && !isInCombat()) {
            STRUCT_664980 attack;
            attack.attacker = v2;
            attack.defender = obj_dude;
            attack.actionPointsBonus = 0;
            attack.accuracyBonus = 0;
            attack.minDamage = 0;
            attack.maxDamage = 99999;
            attack.field_1C = 0;
            scripts_request_combat(&attack);
        }
    }
    return 0;
}

// 0x49C124
int obj_use_item(Object* a1, Object* a2)
{
    int rc = protinst_use_item(a1, a2);
    if (rc == 1 || rc == 2) {
        Object* root = obj_top_environment(a2);
        if (root != NULL) {
            int flags = a2->flags & OBJECT_IN_ANY_HAND;
            item_remove_mult(root, a2, 1);
            Object* v8 = item_replace(root, a2, flags);
            if (root == obj_dude) {
                int leftItemAction;
                int rightItemAction;
                intface_get_item_states(&leftItemAction, &rightItemAction);
                if (v8 == NULL) {
                    if ((flags & OBJECT_IN_LEFT_HAND) != 0) {
                        leftItemAction = INTERFACE_ITEM_ACTION_DEFAULT;
                    } else if ((flags & OBJECT_IN_RIGHT_HAND) != 0) {
                        rightItemAction = INTERFACE_ITEM_ACTION_DEFAULT;
                    } else {
                        leftItemAction = INTERFACE_ITEM_ACTION_DEFAULT;
                        rightItemAction = INTERFACE_ITEM_ACTION_DEFAULT;
                    }
                }
                intface_update_items(false, leftItemAction, rightItemAction);
            }
        }

        if (rc == 1) {
            obj_destroy(a2);
        } else if (rc == 2 && root != NULL) {
            Rect updatedRect;
            obj_connect(a2, root->tile, root->elevation, &updatedRect);
            tile_refresh_rect(&updatedRect, root->elevation);
            protinstTestDroppedExplosive(a2);
        }

        rc = 0;
    }

    scr_exec_map_update_scripts();

    return rc;
}

// 0x49C240
static int protinst_default_use_item(Object* a1, Object* a2, Object* item)
{
    char formattedText[90];
    MessageListItem messageListItem;

    int rc;
    switch (item_get_type(item)) {
    case ITEM_TYPE_DRUG:
        if (PID_TYPE(a2->pid) != OBJ_TYPE_CRITTER) {
            if (a1 == obj_dude) {
                // That does nothing
                messageListItem.num = 582;
                if (message_search(&proto_main_msg_file, &messageListItem)) {
                    display_print(messageListItem.text);
                }
            }
            return -1;
        }

        if (critter_is_dead(a2)) {
            // 583: To your dismay, you realize that it is already dead.
            // 584: As you reach down, you realize that it is already dead.
            // 585: Alas, you are too late.
            // 586: That won't work on the dead.
            messageListItem.num = 583 + roll_random(0, 3);
            if (message_search(&proto_main_msg_file, &messageListItem)) {
                display_print(messageListItem.text);
            }
            return -1;
        }

        rc = item_d_take_drug(a2, item);

        if (a1 == obj_dude && a2 != obj_dude) {
            // TODO: Looks like there is bug in this branch, message 580 will never be shown,
            // as we can only be here when target is not dude.

            // 580: You use the %s.
            // 581: You use the %s on %s.
            messageListItem.num = 580 + (a2 != obj_dude);
            if (!message_search(&proto_main_msg_file, &messageListItem)) {
                return -1;
            }

            sprintf(formattedText, messageListItem.text, object_name(item), object_name(a2));
            display_print(formattedText);
        }

        if (a2 == obj_dude) {
            intface_update_hit_points(true);
        }

        return rc;
    case ITEM_TYPE_AMMO:
        rc = obj_use_power_on_car(item);
        if (rc == 1) {
            return 1;
        }
        break;
    case ITEM_TYPE_WEAPON:
    case ITEM_TYPE_MISC:
        rc = obj_use_flare(a1, item);
        if (rc == 0) {
            return 0;
        }
        break;
    }

    messageListItem.num = 582;
    if (message_search(&proto_main_msg_file, &messageListItem)) {
        sprintf(formattedText, messageListItem.text);
        display_print(formattedText);
    }
    return -1;
}

// 0x49C3CC
int protinst_use_item_on(Object* a1, Object* a2, Object* item)
{
    int messageId = -1;
    int criticalChanceModifier = 0;
    int skill = -1;

    switch (item->pid) {
    case PROTO_ID_DOCTORS_BAG:
        // The supplies in the Doctor's Bag run out.
        messageId = 900;
        criticalChanceModifier = 20;
        skill = SKILL_DOCTOR;
        break;
    case PROTO_ID_FIRST_AID_KIT:
        // The supplies in the First Aid Kit run out.
        messageId = 901;
        criticalChanceModifier = 20;
        skill = SKILL_FIRST_AID;
        break;
    case PROTO_ID_PARAMEDICS_BAG:
        // The supplies in the Paramedic's Bag run out.
        messageId = 910;
        criticalChanceModifier = 40;
        skill = SKILL_DOCTOR;
        break;
    case PROTO_ID_FIELD_MEDIC_FIRST_AID_KIT:
        // The supplies in the Field Medic First Aid Kit run out.
        messageId = 911;
        criticalChanceModifier = 40;
        skill = SKILL_FIRST_AID;
        break;
    }

    if (skill == -1) {
        Script* script;

        if (item->sid == -1) {
            if (a2->sid == -1) {
                return protinst_default_use_item(a1, a2, item);
            }

            scr_set_objs(a2->sid, a1, item);
            exec_script_proc(a2->sid, SCRIPT_PROC_USE_OBJ_ON);

            if (scr_ptr(a2->sid, &script) == -1) {
                return -1;
            }

            if (!script->scriptOverrides) {
                return protinst_default_use_item(a1, a2, item);
            }
        } else {
            scr_set_objs(item->sid, a1, a2);
            exec_script_proc(item->sid, SCRIPT_PROC_USE_OBJ_ON);

            if (scr_ptr(item->sid, &script) == -1) {
                return -1;
            }

            if (script->field_28 == 0) {
                if (a2->sid == -1) {
                    return protinst_default_use_item(a1, a2, item);
                }

                scr_set_objs(a2->sid, a1, item);
                exec_script_proc(a2->sid, SCRIPT_PROC_USE_OBJ_ON);

                Script* script;
                if (scr_ptr(a2->sid, &script) == -1) {
                    return -1;
                }

                if (!script->scriptOverrides) {
                    return protinst_default_use_item(a1, a2, item);
                }
            }
        }

        return script->field_28;
    }

    if (isInCombat()) {
        MessageListItem messageListItem;
        // You cannot do that in combat.
        messageListItem.num = 902;
        if (a1 == obj_dude) {
            if (message_search(&proto_main_msg_file, &messageListItem)) {
                display_print(messageListItem.text);
            }
        }
        return -1;
    }

    if (skill_use(a1, a2, skill, criticalChanceModifier) != 0) {
        return 0;
    }

    if (roll_random(1, 10) != 1) {
        return 0;
    }

    MessageListItem messageListItem;
    messageListItem.num = messageId;
    if (a1 == obj_dude) {
        if (message_search(&proto_main_msg_file, &messageListItem)) {
            display_print(messageListItem.text);
        }
    }

    return 1;
}

// 0x49C5FC
int obj_use_item_on(Object* a1, Object* a2, Object* a3)
{
    int rc = protinst_use_item_on(a1, a2, a3);

    if (rc == 1) {
        if (a1 != NULL) {
            int flags = a3->flags & OBJECT_IN_ANY_HAND;
            item_remove_mult(a1, a3, 1);

            Object* v7 = item_replace(a1, a3, flags);

            int leftItemAction;
            int rightItemAction;
            if (a1 == obj_dude) {
                intface_get_item_states(&leftItemAction, &rightItemAction);
            }

            if (v7 == NULL) {
                if ((flags & OBJECT_IN_LEFT_HAND) != 0) {
                    leftItemAction = INTERFACE_ITEM_ACTION_DEFAULT;
                } else if ((flags & OBJECT_IN_RIGHT_HAND) != 0) {
                    rightItemAction = INTERFACE_ITEM_ACTION_DEFAULT;
                } else {
                    leftItemAction = INTERFACE_ITEM_ACTION_DEFAULT;
                    rightItemAction = INTERFACE_ITEM_ACTION_DEFAULT;
                }
            }

            intface_update_items(false, leftItemAction, rightItemAction);
        }

        obj_destroy(a3);

        rc = 0;
    }

    scr_exec_map_update_scripts();

    return rc;
}

// 0x49C6BC
int check_scenery_ap_cost(Object* obj, Object* a2)
{
    if (!isInCombat()) {
        return 0;
    }

    int actionPoints = obj->data.critter.combat.ap;
    if (actionPoints >= 3) {
        obj->data.critter.combat.ap = actionPoints - 3;

        if (obj == obj_dude) {
            intface_update_move_points(obj_dude->data.critter.combat.ap, combat_free_move);
        }

        return 0;
    }

    MessageListItem messageListItem;
    // You don't have enough action points.
    messageListItem.num = 700;

    if (obj == obj_dude) {
        if (message_search(&proto_main_msg_file, &messageListItem)) {
            display_print(messageListItem.text);
        }
    }

    return -1;
}

// 0x49C740
int obj_use(Object* a1, Object* a2)
{
    int type = FID_TYPE(a2->fid);
    if (a1 == obj_dude) {
        if (type != OBJ_TYPE_SCENERY) {
            return -1;
        }
    } else {
        if (type != OBJ_TYPE_SCENERY) {
            return 0;
        }
    }

    Proto* sceneryProto;
    if (proto_ptr(a2->pid, &sceneryProto) == -1) {
        return -1;
    }

    if (PID_TYPE(a2->pid) == OBJ_TYPE_SCENERY && sceneryProto->scenery.type == SCENERY_TYPE_DOOR) {
        return obj_use_door(a1, a2, 0);
    }

    bool scriptOverrides = false;

    if (a2->sid != -1) {
        scr_set_objs(a2->sid, a1, a2);
        exec_script_proc(a2->sid, SCRIPT_PROC_USE);

        Script* script;
        if (scr_ptr(a2->sid, &script) == -1) {
            return -1;
        }

        scriptOverrides = script->scriptOverrides;
    }

    if (!scriptOverrides) {
        if (PID_TYPE(a2->pid) == OBJ_TYPE_SCENERY) {
            if (sceneryProto->scenery.type == SCENERY_TYPE_LADDER_DOWN) {
                if (obj_use_ladder_top(a1, a2, 0) == 0) {
                    scriptOverrides = true;
                }
            } else if (sceneryProto->scenery.type == SCENERY_TYPE_LADDER_UP) {
                if (obj_use_ladder_bottom(a1, a2, 0) == 0) {
                    scriptOverrides = true;
                }
            } else if (sceneryProto->scenery.type == SCENERY_TYPE_STAIRS) {
                if (obj_use_stairs(a1, a2, 0) == 0) {
                    scriptOverrides = true;
                }
            }
        }
    }

    if (!scriptOverrides) {
        if (a1 == obj_dude) {
            // You see: %s
            MessageListItem messageListItem;
            messageListItem.num = 480;
            if (!message_search(&proto_main_msg_file, &messageListItem)) {
                return -1;
            }

            char formattedText[260];
            const char* name = object_name(a2);
            sprintf(formattedText, messageListItem.text, name);
            display_print(formattedText);
        }
    }

    scr_exec_map_update_scripts();

    return 0;
}

// 0x49C900
int obj_use_ladder_top(Object* a1, Object* ladder, int a3)
{
    int builtTile = ladder->data.scenery.ladder.destinationBuiltTile;
    if (builtTile == -1) {
        return -1;
    }

    int tile = builtTileGetTile(builtTile);
    int elevation = builtTileGetElevation(builtTile);
    if (ladder->data.scenery.ladder.destinationMap != 0) {
        MapTransition transition;
        memset(&transition, 0, sizeof(transition));

        transition.map = ladder->data.scenery.ladder.destinationMap;
        transition.elevation = elevation;
        transition.tile = tile;
        transition.rotation = builtTileGetRotation(builtTile);

        map_leave_map(&transition);

        wmMapMarkMapEntranceState(transition.map, elevation, 1);
    } else {
        Rect updatedRect;
        if (obj_move_to_tile(a1, tile, elevation, &updatedRect) == -1) {
            return -1;
        }

        tile_refresh_rect(&updatedRect, map_elevation);
    }

    return 0;
}

// 0x49C9A4
int obj_use_ladder_bottom(Object* a1, Object* ladder, int a3)
{
    int builtTile = ladder->data.scenery.ladder.destinationBuiltTile;
    if (builtTile == -1) {
        return -1;
    }

    int tile = builtTileGetTile(builtTile);
    int elevation = builtTileGetElevation(builtTile);
    if (ladder->data.scenery.ladder.destinationMap != 0) {
        MapTransition transition;
        memset(&transition, 0, sizeof(transition));

        transition.map = ladder->data.scenery.ladder.destinationMap;
        transition.elevation = elevation;
        transition.tile = tile;
        transition.rotation = builtTileGetRotation(builtTile);

        map_leave_map(&transition);

        wmMapMarkMapEntranceState(transition.map, elevation, 1);
    } else {
        Rect updatedRect;
        if (obj_move_to_tile(a1, tile, elevation, &updatedRect) == -1) {
            return -1;
        }

        tile_refresh_rect(&updatedRect, map_elevation);
    }

    return 0;
}

// 0x49CA48
int obj_use_stairs(Object* a1, Object* stairs, int a3)
{
    int builtTile = stairs->data.scenery.stairs.destinationBuiltTile;
    if (builtTile == -1) {
        return -1;
    }

    int tile = builtTileGetTile(builtTile);
    int elevation = builtTileGetElevation(builtTile);
    if (stairs->data.scenery.stairs.destinationMap > 0) {
        MapTransition transition;
        memset(&transition, 0, sizeof(transition));

        transition.map = stairs->data.scenery.stairs.destinationMap;
        transition.elevation = elevation;
        transition.tile = tile;
        transition.rotation = builtTileGetRotation(builtTile);

        map_leave_map(&transition);

        wmMapMarkMapEntranceState(transition.map, elevation, 1);
    } else {
        Rect updatedRect;
        if (obj_move_to_tile(a1, tile, elevation, &updatedRect) == -1) {
            return -1;
        }

        tile_refresh_rect(&updatedRect, map_elevation);
    }

    return 0;
}

// 0x49CAF4
static int set_door_state_open(Object* a1, Object* a2)
{
    a1->data.scenery.door.openFlags |= 0x01;
    return 0;
}

// 0x49CB04
static int set_door_state_closed(Object* a1, Object* a2)
{
    a1->data.scenery.door.openFlags &= ~0x01;
    return 0;
}

// 0x49CB14
static int check_door_state(Object* a1, Object* a2)
{
    if ((a1->data.scenery.door.openFlags & 0x01) == 0) {
        a1->flags &= ~OBJECT_OPEN_DOOR;

        obj_rebuild_all_light();
        tile_refresh_display();

        if (a1->frame == 0) {
            return 0;
        }

        CacheEntry* artHandle;
        Art* art = art_ptr_lock(a1->fid, &artHandle);
        if (art == NULL) {
            return -1;
        }

        Rect dirty;
        Rect temp;

        obj_bound(a1, &dirty);

        for (int frame = a1->frame - 1; frame >= 0; frame--) {
            int x;
            int y;
            art_frame_hot(art, frame, a1->rotation, &x, &y);
            obj_offset(a1, -x, -y, &temp);
        }

        obj_set_frame(a1, 0, &temp);
        rect_min_bound(&dirty, &temp, &dirty);

        tile_refresh_rect(&dirty, map_elevation);

        art_ptr_unlock(artHandle);
        return 0;
    } else {
        a1->flags |= OBJECT_OPEN_DOOR;

        obj_rebuild_all_light();
        tile_refresh_display();

        CacheEntry* artHandle;
        Art* art = art_ptr_lock(a1->fid, &artHandle);
        if (art == NULL) {
            return -1;
        }

        int frameCount = art_frame_max_frame(art);
        if (a1->frame == frameCount - 1) {
            art_ptr_unlock(artHandle);
            return 0;
        }

        Rect dirty;
        Rect temp;

        obj_bound(a1, &dirty);

        for (int frame = a1->frame + 1; frame < frameCount; frame++) {
            int x;
            int y;
            art_frame_hot(art, frame, a1->rotation, &x, &y);
            obj_offset(a1, x, y, &temp);
        }

        obj_set_frame(a1, frameCount - 1, &temp);
        rect_min_bound(&dirty, &temp, &dirty);

        tile_refresh_rect(&dirty, map_elevation);

        art_ptr_unlock(artHandle);
        return 0;
    }
}

// 0x49CCB8
int obj_use_door(Object* a1, Object* a2, int a3)
{
    if (obj_is_locked(a2)) {
        const char* sfx = gsnd_build_open_sfx_name(a2, SCENERY_SOUND_EFFECT_LOCKED);
        gsound_play_sfx_file(sfx);
    }

    bool scriptOverrides = false;
    if (a2->sid != -1) {
        scr_set_objs(a2->sid, a1, a2);
        exec_script_proc(a2->sid, SCRIPT_PROC_USE);

        Script* script;
        if (scr_ptr(a2->sid, &script) == -1) {
            return -1;
        }

        scriptOverrides = script->scriptOverrides;
    }

    if (!scriptOverrides) {
        int start;
        int end;
        int step;
        if (a2->frame != 0) {
            if (obj_blocking_at(NULL, a2->tile, a2->elevation) != 0) {
                MessageListItem messageListItem;
                char* text = getmsg(&proto_main_msg_file, &messageListItem, 597);
                display_print(text);
                return -1;
            }
            start = 1;
            end = (a3 == 0) - 1;
            step = -1;
        } else {
            if (a2->data.scenery.door.openFlags & 0x01) {
                return -1;
            }

            start = 0;
            end = (a3 != 0) + 1;
            step = 1;
        }

        register_begin(ANIMATION_REQUEST_RESERVED);

        for (int i = start; i != end; i += step) {
            if (i != 0) {
                if (a3 == 0) {
                    register_object_call(a2, a2, set_door_state_closed, -1);
                }

                const char* sfx = gsnd_build_open_sfx_name(a2, SCENERY_SOUND_EFFECT_CLOSED);
                register_object_play_sfx(a2, sfx, -1);

                register_object_animate_reverse(a2, ANIM_STAND, 0);
            } else {
                if (a3 == 0) {
                    register_object_call(a2, a2, set_door_state_open, -1);
                }

                const char* sfx = gsnd_build_open_sfx_name(a2, SCENERY_SOUND_EFFECT_CLOSED);
                register_object_play_sfx(a2, sfx, -1);

                register_object_animate(a2, ANIM_STAND, 0);
            }
        }

        register_object_must_call(a2, a2, check_door_state, -1);

        register_end();
    }

    return 0;
}

// 0x49CE7C
int obj_use_container(Object* critter, Object* item)
{
    if (FID_TYPE(item->fid) != OBJ_TYPE_ITEM) {
        return -1;
    }

    Proto* itemProto;
    if (proto_ptr(item->pid, &itemProto) == -1) {
        return -1;
    }

    if (itemProto->item.type != ITEM_TYPE_CONTAINER) {
        return -1;
    }

    if (obj_is_locked(item)) {
        const char* sfx = gsnd_build_open_sfx_name(item, SCENERY_SOUND_EFFECT_LOCKED);
        gsound_play_sfx_file(sfx);

        if (critter == obj_dude) {
            MessageListItem messageListItem;
            // It is locked.
            messageListItem.num = 487;
            if (!message_search(&proto_main_msg_file, &messageListItem)) {
                return -1;
            }

            display_print(messageListItem.text);
        }

        return -1;
    }

    bool overriden = false;
    if (item->sid != -1) {
        scr_set_objs(item->sid, critter, item);
        exec_script_proc(item->sid, SCRIPT_PROC_USE);

        Script* script;
        if (scr_ptr(item->sid, &script) == -1) {
            return -1;
        }

        overriden = script->scriptOverrides;
    }

    if (overriden) {
        return -1;
    }

    register_begin(ANIMATION_REQUEST_RESERVED);

    if (item->frame == 0) {
        const char* sfx = gsnd_build_open_sfx_name(item, SCENERY_SOUND_EFFECT_OPEN);
        register_object_play_sfx(item, sfx, 0);
        register_object_animate(item, ANIM_STAND, 0);
    } else {
        const char* sfx = gsnd_build_open_sfx_name(item, SCENERY_SOUND_EFFECT_CLOSED);
        register_object_play_sfx(item, sfx, 0);
        register_object_animate_reverse(item, ANIM_STAND, 0);
    }

    register_end();

    if (critter == obj_dude) {
        MessageListItem messageListItem;
        messageListItem.num = item->frame != 0
            ? 486 // You search the %s.
            : 485; // You close the %s.
        if (!message_search(&proto_main_msg_file, &messageListItem)) {
            return -1;
        }

        char formattedText[260];
        const char* objectName = object_name(item);
        sprintf(formattedText, messageListItem.text, objectName);
        display_print(formattedText);
    }

    return 0;
}

// 0x49D078
int obj_use_skill_on(Object* source, Object* target, int skill)
{
    if (obj_lock_is_jammed(target)) {
        if (source == obj_dude) {
            MessageListItem messageListItem;
            messageListItem.num = 2001;
            if (message_search(&misc_message_file, &messageListItem)) {
                display_print(messageListItem.text);
            }
        }
        return -1;
    }

    Proto* proto;
    if (proto_ptr(target->pid, &proto) == -1) {
        return -1;
    }

    bool scriptOverrides = false;
    if (target->sid != -1) {
        scr_set_objs(target->sid, source, target);
        scr_set_action_num(target->sid, skill);
        exec_script_proc(target->sid, SCRIPT_PROC_USE_SKILL_ON);

        Script* script;
        if (scr_ptr(target->sid, &script) == -1) {
            return -1;
        }

        scriptOverrides = script->scriptOverrides;
    }

    if (!scriptOverrides) {
        skill_use(source, target, skill, 0);
    }

    return 0;
}

// NOTE: Unused.
//
// 0x49D140
bool obj_is_a_portal(Object* obj)
{
    if (obj == NULL) {
        return false;
    }

    Proto* proto;
    if (proto_ptr(obj->pid, &proto) == -1) {
        return false;
    }

    return proto->scenery.type == SCENERY_TYPE_DOOR;
}

// 0x49D178
bool obj_is_lockable(Object* obj)
{
    Proto* proto;

    if (obj == NULL) {
        return false;
    }

    if (proto_ptr(obj->pid, &proto) == -1) {
        return false;
    }

    switch (PID_TYPE(obj->pid)) {
    case OBJ_TYPE_ITEM:
        if (proto->item.type == ITEM_TYPE_CONTAINER) {
            return true;
        }
        break;
    case OBJ_TYPE_SCENERY:
        if (proto->scenery.type == SCENERY_TYPE_DOOR) {
            return true;
        }
        break;
    }

    return false;
}

// 0x49D1C8
bool obj_is_locked(Object* obj)
{
    if (obj == NULL) {
        return false;
    }

    ObjectData* data = &(obj->data);
    switch (PID_TYPE(obj->pid)) {
    case OBJ_TYPE_ITEM:
        return data->flags & CONTAINER_FLAG_LOCKED;
    case OBJ_TYPE_SCENERY:
        return data->scenery.door.openFlags & DOOR_FLAG_LOCKED;
    }

    return false;
}

// 0x49D20C
int obj_lock(Object* object)
{
    if (object == NULL) {
        return -1;
    }

    switch (PID_TYPE(object->pid)) {
    case OBJ_TYPE_ITEM:
        object->data.flags |= OBJ_LOCKED;
        break;
    case OBJ_TYPE_SCENERY:
        object->data.scenery.door.openFlags |= OBJ_LOCKED;
        break;
    default:
        return -1;
    }

    return 0;
}

// 0x49D250
int obj_unlock(Object* object)
{
    if (object == NULL) {
        return -1;
    }

    switch (PID_TYPE(object->pid)) {
    case OBJ_TYPE_ITEM:
        object->data.flags &= ~OBJ_LOCKED;
        return 0;
    case OBJ_TYPE_SCENERY:
        object->data.scenery.door.openFlags &= ~OBJ_LOCKED;
        return 0;
    }

    return -1;
}

// 0x49D294
bool obj_is_openable(Object* obj)
{
    Proto* proto;

    if (obj == NULL) {
        return false;
    }

    if (proto_ptr(obj->pid, &proto) == -1) {
        return false;
    }

    switch (PID_TYPE(obj->pid)) {
    case OBJ_TYPE_ITEM:
        if (proto->item.type == ITEM_TYPE_CONTAINER) {
            return true;
        }
        break;
    case OBJ_TYPE_SCENERY:
        if (proto->scenery.type == SCENERY_TYPE_DOOR) {
            return true;
        }
        break;
    }

    return false;
}

// 0x49D2E4
int obj_is_open(Object* object)
{
    return object->frame != 0;
}

// 0x49D2F4
int obj_toggle_open(Object* obj)
{
    if (obj == NULL) {
        return -1;
    }

    if (!obj_is_openable(obj)) {
        return -1;
    }

    if (obj_is_locked(obj)) {
        return -1;
    }

    obj_unjam_lock(obj);

    register_begin(ANIMATION_REQUEST_RESERVED);

    if (obj->frame != 0) {
        register_object_must_call(obj, obj, set_door_state_closed, -1);

        const char* sfx = gsnd_build_open_sfx_name(obj, SCENERY_SOUND_EFFECT_CLOSED);
        register_object_play_sfx(obj, sfx, -1);

        register_object_animate_reverse(obj, ANIM_STAND, 0);
    } else {
        register_object_must_call(obj, obj, set_door_state_open, -1);

        const char* sfx = gsnd_build_open_sfx_name(obj, SCENERY_SOUND_EFFECT_OPEN);
        register_object_play_sfx(obj, sfx, -1);
        register_object_animate(obj, ANIM_STAND, 0);
    }

    register_object_must_call(obj, obj, check_door_state, -1);

    register_end();

    return 0;
}

// 0x49D3D8
int obj_open(Object* obj)
{
    if (obj->frame == 0) {
        obj_toggle_open(obj);
    }

    return 0;
}

// 0x49D3F4
int obj_close(Object* obj)
{
    if (obj->frame != 0) {
        obj_toggle_open(obj);
    }

    return 0;
}

// 0x49D410
bool obj_lock_is_jammed(Object* obj)
{
    if (!obj_is_lockable(obj)) {
        return false;
    }

    if (PID_TYPE(obj->pid) == OBJ_TYPE_SCENERY) {
        if ((obj->data.scenery.door.openFlags & OBJ_JAMMED) != 0) {
            return true;
        }
    } else {
        if ((obj->data.flags & OBJ_JAMMED) != 0) {
            return true;
        }
    }

    return false;
}

// jam_lock
// 0x49D448
int obj_jam_lock(Object* obj)
{
    if (!obj_is_lockable(obj)) {
        return -1;
    }

    ObjectData* data = &(obj->data);
    switch (PID_TYPE(obj->pid)) {
    case OBJ_TYPE_ITEM:
        data->flags |= CONTAINER_FLAG_JAMMED;
        break;
    case OBJ_TYPE_SCENERY:
        data->scenery.door.openFlags |= DOOR_FLAG_JAMMGED;
        break;
    }

    return 0;
}

// 0x49D480
int obj_unjam_lock(Object* obj)
{
    if (!obj_is_lockable(obj)) {
        return -1;
    }

    ObjectData* data = &(obj->data);
    switch (PID_TYPE(obj->pid)) {
    case OBJ_TYPE_ITEM:
        data->flags &= ~CONTAINER_FLAG_JAMMED;
        break;
    case OBJ_TYPE_SCENERY:
        data->scenery.door.openFlags &= ~DOOR_FLAG_JAMMGED;
        break;
    }

    return 0;
}

// 0x49D4B8
int obj_unjam_all_locks()
{
    Object* obj = obj_find_first();
    while (obj != NULL) {
        obj_unjam_lock(obj);
        obj = obj_find_next();
    }

    return 0;
}

// critter_attempt_placement
// 0x49D4D4
int obj_attempt_placement(Object* obj, int tile, int elevation, int a4)
{
    if (tile == -1) {
        return -1;
    }

    int newTile = tile;
    if (obj_blocking_at(NULL, tile, elevation) != NULL) {
        int v6 = a4;
        if (a4 < 1) {
            v6 = 1;
        }

        int attempts = 0;
        while (v6 < 7) {
            attempts++;
            if (attempts >= 100) {
                break;
            }

            for (int rotation = 0; rotation < ROTATION_COUNT; rotation++) {
                newTile = tile_num_in_direction(tile, rotation, v6);
                if (obj_blocking_at(NULL, newTile, elevation) == NULL && v6 > 1 && make_path(obj_dude, obj_dude->tile, newTile, NULL, 0) != 0) {
                    break;
                }
            }

            v6++;
        }

        if (a4 != 1 && v6 > a4 + 2) {
            for (int rotation = 0; rotation < ROTATION_COUNT; rotation++) {
                int candidate = tile_num_in_direction(tile, rotation, 1);
                if (obj_blocking_at(NULL, candidate, elevation) == NULL) {
                    newTile = candidate;
                    break;
                }
            }
        }
    }

    Rect updatedRect;
    obj_turn_on(obj, &updatedRect);

    Rect temp;
    if (obj_move_to_tile(obj, newTile, elevation, &temp) != -1) {
        rect_min_bound(&updatedRect, &temp, &updatedRect);

        if (elevation == map_elevation) {
            tile_refresh_rect(&updatedRect, elevation);
        }
    }

    return 0;
}

// 0x49D628
int objPMAttemptPlacement(Object* obj, int tile, int elevation)
{
    if (obj == NULL) {
        return -1;
    }

    if (tile == -1) {
        return -1;
    }

    int v9 = tile;
    int v7 = 0;
    if (!wmEvalTileNumForPlacement(tile)) {
        v9 = obj_dude->tile;
        for (int v4 = 1; v4 <= 100; v4++) {
            // TODO: Check.
            v7++;
            v9 = tile_num_in_direction(v9, v7 % ROTATION_COUNT, 1);
            if (wmEvalTileNumForPlacement(v9) != 0) {
                break;
            }

            if (tile_dist(obj_dude->tile, v9) > 8) {
                v9 = tile;
                break;
            }
        }
    }

    obj_turn_on(obj, NULL);
    obj_move_to_tile(obj, v9, elevation, NULL);

    return 0;
}
