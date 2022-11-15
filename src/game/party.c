#include "game/party.h"

#include <stdio.h>
#include <string.h>

#include "game/anim.h"
#include "color.h"
#include "game/combatai.h"
#include "game/config.h"
#include "game/critter.h"
#include "debug.h"
#include "game/display.h"
#include "game/game.h"
#include "game/gdialog.h"
#include "game/item.h"
#include "game/loadsave.h"
#include "game/map.h"
#include "memory.h"
#include "game/message.h"
#include "game/object.h"
#include "proto.h"
#include "proto_instance.h"
#include "queue.h"
#include "random.h"
#include "scripts.h"
#include "skill.h"
#include "stat.h"
#include "string_parsers.h"
#include "text_object.h"
#include "tile.h"
#include "window_manager.h"

typedef struct PartyMemberDescription {
    bool areaAttackMode[AREA_ATTACK_MODE_COUNT];
    bool runAwayMode[RUN_AWAY_MODE_COUNT];
    bool bestWeapon[BEST_WEAPON_COUNT];
    bool distanceMode[DISTANCE_COUNT];
    bool attackWho[ATTACK_WHO_COUNT];
    bool chemUse[CHEM_USE_COUNT];
    bool disposition[DISPOSITION_COUNT];
    int level_minimum;
    int level_up_every;
    int level_pids_num;
    int level_pids[5];
} PartyMemberDescription;

typedef struct STRU_519DBC {
    int field_0;
    int field_4; // party member level
    int field_8; // early what?
} STRU_519DBC;

typedef struct STRUCT_519DA8 {
    Object* object;
    Script* script;
    int* vars;
    struct STRUCT_519DA8* next;
} STRUCT_519DA8;

static int partyMemberGetAIOptions(Object* object, PartyMemberDescription** partyMemberDescriptionPtr);
static void partyMemberAISlotInit(PartyMemberDescription* partyMemberDescription);
static int partyMemberSlotInit(int index);
static int partyMemberPrepLoadInstance(STRUCT_519DA8* a1);
static int partyMemberRecoverLoadInstance(STRUCT_519DA8* a1);
static int partyMemberNewObjID();
static int partyMemberNewObjIDRecurseFind(Object* object, int objectId);
static int partyMemberPrepItemSave(Object* object);
static int partyMemberItemSave(Object* object);
static int partyMemberItemRecover(STRUCT_519DA8* a1);
static int partyMemberClearItemList();
static int partyFixMultipleMembers();
static int partyMemberCopyLevelInfo(Object* object, int a2);

// 0x519D9C
int partyMemberMaxCount = 0;

// 0x519DA0
int* partyMemberPidList = NULL;

//
STRUCT_519DA8* itemSaveListHead = NULL;

// List of party members, it's length is [partyMemberMaxCount] + 20.
//
// 0x519DA8
static STRUCT_519DA8* partyMemberList = NULL;

// Number of critters added to party.
//
// 0x519DAC
static int partyMemberCount = 0;

// 0x519DB0
static int partyMemberItemCount = 20000;

// 0x519DB4
static int partyStatePrepped = 0;

// 0x519DB8
static PartyMemberDescription* partyMemberAIOptions = NULL;

// 0x519DBC
static STRU_519DBC* partyMemberLevelUpInfoList = NULL;

// 0x493BC0
int partyMember_init()
{
    Config config;

    partyMemberMaxCount = 0;

    if (!config_init(&config)) {
        return -1;
    }

    if (!config_load(&config, "data\\party.txt", true)) {
        goto err;
    }

    char section[50];
    sprintf(section, "Party Member %d", partyMemberMaxCount);

    int partyMemberPid;
    while (config_get_value(&config, section, "party_member_pid", &partyMemberPid)) {
        partyMemberMaxCount++;
        sprintf(section, "Party Member %d", partyMemberMaxCount);
    }

    partyMemberPidList = (int*)internal_malloc(sizeof(*partyMemberPidList) * partyMemberMaxCount);
    if (partyMemberPidList == NULL) {
        goto err;
    }

    memset(partyMemberPidList, 0, sizeof(*partyMemberPidList) * partyMemberMaxCount);

    partyMemberList = (STRUCT_519DA8*)internal_malloc(sizeof(*partyMemberList) * (partyMemberMaxCount + 20));
    if (partyMemberList == NULL) {
        goto err;
    }

    memset(partyMemberList, 0, sizeof(*partyMemberList) * (partyMemberMaxCount + 20));

    partyMemberAIOptions = (PartyMemberDescription*)internal_malloc(sizeof(*partyMemberAIOptions) * partyMemberMaxCount);
    if (partyMemberAIOptions == NULL) {
        goto err;
    }

    memset(partyMemberAIOptions, 0, sizeof(*partyMemberAIOptions) * partyMemberMaxCount);

    partyMemberLevelUpInfoList = (STRU_519DBC*)internal_malloc(sizeof(*partyMemberLevelUpInfoList) * partyMemberMaxCount);
    if (partyMemberLevelUpInfoList == NULL) goto err;

    memset(partyMemberLevelUpInfoList, 0, sizeof(*partyMemberLevelUpInfoList) * partyMemberMaxCount);

    for (int index = 0; index < partyMemberMaxCount; index++) {
        sprintf(section, "Party Member %d", index);

        if (!config_get_value(&config, section, "party_member_pid", &partyMemberPid)) {
            break;
        }

        PartyMemberDescription* partyMemberDescription = &(partyMemberAIOptions[index]);

        partyMemberPidList[index] = partyMemberPid;

        partyMemberAISlotInit(partyMemberDescription);

        char* string;

        if (config_get_string(&config, section, "area_attack_mode", &string)) {
            while (*string != '\0') {
                int areaAttackMode;
                strParseStrFromList(&string, &areaAttackMode, area_attack_mode_strs, AREA_ATTACK_MODE_COUNT);
                partyMemberDescription->areaAttackMode[areaAttackMode] = true;
            }
        }

        if (config_get_string(&config, section, "attack_who", &string)) {
            while (*string != '\0') {
                int attachWho;
                strParseStrFromList(&string, &attachWho, attack_who_mode_strs, ATTACK_WHO_COUNT);
                partyMemberDescription->attackWho[attachWho] = true;
            }
        }

        if (config_get_string(&config, section, "best_weapon", &string)) {
            while (*string != '\0') {
                int bestWeapon;
                strParseStrFromList(&string, &bestWeapon, weapon_pref_strs, BEST_WEAPON_COUNT);
                partyMemberDescription->bestWeapon[bestWeapon] = true;
            }
        }

        if (config_get_string(&config, section, "chem_use", &string)) {
            while (*string != '\0') {
                int chemUse;
                strParseStrFromList(&string, &chemUse, chem_use_mode_strs, CHEM_USE_COUNT);
                partyMemberDescription->chemUse[chemUse] = true;
            }
        }

        if (config_get_string(&config, section, "distance", &string)) {
            while (*string != '\0') {
                int distanceMode;
                strParseStrFromList(&string, &distanceMode, distance_pref_strs, DISTANCE_COUNT);
                partyMemberDescription->distanceMode[distanceMode] = true;
            }
        }

        if (config_get_string(&config, section, "run_away_mode", &string)) {
            while (*string != '\0') {
                int runAwayMode;
                strParseStrFromList(&string, &runAwayMode, run_away_mode_strs, RUN_AWAY_MODE_COUNT);
                partyMemberDescription->runAwayMode[runAwayMode] = true;
            }
        }

        if (config_get_string(&config, section, "disposition", &string)) {
            while (*string != '\0') {
                int disposition;
                strParseStrFromList(&string, &disposition, disposition_strs, DISPOSITION_COUNT);
                partyMemberDescription->disposition[disposition] = true;
            }
        }

        int levelUpEvery;
        if (config_get_value(&config, section, "level_up_every", &levelUpEvery)) {
            partyMemberDescription->level_up_every = levelUpEvery;

            int levelMinimum;
            if (config_get_value(&config, section, "level_minimum", &levelMinimum)) {
                partyMemberDescription->level_minimum = levelMinimum;
            }

            if (config_get_string(&config, section, "level_pids", &string)) {
                while (*string != '\0' && partyMemberDescription->level_pids_num < 5) {
                    int levelPid;
                    strParseInt(&string, &levelPid);
                    partyMemberDescription->level_pids[partyMemberDescription->level_pids_num] = levelPid;
                    partyMemberDescription->level_pids_num++;
                }
            }
        }
    }

    config_exit(&config);

    return 0;

err:

    config_exit(&config);

    return -1;
}

// 0x4940E4
void partyMember_reset()
{
    for (int index = 0; index < partyMemberMaxCount; index++) {
        // NOTE: Uninline.
        partyMemberSlotInit(index);
    }
}

// 0x494134
void partyMember_exit()
{
    for (int index = 0; index < partyMemberMaxCount; index++) {
        // NOTE: Uninline.
        partyMemberSlotInit(index);
    }

    partyMemberMaxCount = 0;

    if (partyMemberPidList != NULL) {
        internal_free(partyMemberPidList);
        partyMemberPidList = NULL;
    }

    if (partyMemberList != NULL) {
        internal_free(partyMemberList);
        partyMemberList = NULL;
    }

    if (partyMemberAIOptions != NULL) {
        internal_free(partyMemberAIOptions);
        partyMemberAIOptions = NULL;
    }

    if (partyMemberLevelUpInfoList != NULL) {
        internal_free(partyMemberLevelUpInfoList);
        partyMemberLevelUpInfoList = NULL;
    }
}

// 0x4941F0
static int partyMemberGetAIOptions(Object* object, PartyMemberDescription** partyMemberDescriptionPtr)
{
    for (int index = 1; index < partyMemberMaxCount; index++) {
        if (partyMemberPidList[index] == object->pid) {
            *partyMemberDescriptionPtr = &(partyMemberAIOptions[index]);
            return 0;
        }
    }

    return -1;
}

// 0x49425C
static void partyMemberAISlotInit(PartyMemberDescription* partyMemberDescription)
{
    for (int index = 0; index < AREA_ATTACK_MODE_COUNT; index++) {
        partyMemberDescription->areaAttackMode[index] = 0;
    }

    for (int index = 0; index < RUN_AWAY_MODE_COUNT; index++) {
        partyMemberDescription->runAwayMode[index] = 0;
    }

    for (int index = 0; index < BEST_WEAPON_COUNT; index++) {
        partyMemberDescription->bestWeapon[index] = 0;
    }

    for (int index = 0; index < DISTANCE_COUNT; index++) {
        partyMemberDescription->distanceMode[index] = 0;
    }

    for (int index = 0; index < ATTACK_WHO_COUNT; index++) {
        partyMemberDescription->attackWho[index] = 0;
    }

    for (int index = 0; index < CHEM_USE_COUNT; index++) {
        partyMemberDescription->chemUse[index] = 0;
    }

    for (int index = 0; index < DISPOSITION_COUNT; index++) {
        partyMemberDescription->disposition[index] = 0;
    }

    partyMemberDescription->level_minimum = 0;
    partyMemberDescription->level_up_every = 0;
    partyMemberDescription->level_pids_num = 0;

    partyMemberDescription->level_pids[0] = -1;

    for (int index = 0; index < partyMemberMaxCount; index++) {
        // NOTE: Uninline.
        partyMemberSlotInit(index);
    }
}

// NOTE: Inlined.
//
// 0x494340
static int partyMemberSlotInit(int index)
{
    if (index >= partyMemberMaxCount) {
        return -1;
    }

    partyMemberLevelUpInfoList[index].field_0 = 0;
    partyMemberLevelUpInfoList[index].field_4 = 0;
    partyMemberLevelUpInfoList[index].field_8 = 0;

    return 0;
}

// 0x494378
int partyMemberAdd(Object* object)
{
    if (partyMemberCount >= partyMemberMaxCount + 20) {
        return -1;
    }

    for (int index = 0; index < partyMemberCount; index++) {
        STRUCT_519DA8* partyMember = &(partyMemberList[index]);
        if (partyMember->object == object || partyMember->object->pid == object->pid) {
            return 0;
        }
    }

    if (partyStatePrepped) {
        debugPrint("\npartyMemberAdd DENIED: %s\n", critter_name(object));
        return -1;
    }

    STRUCT_519DA8* partyMember = &(partyMemberList[partyMemberCount]);
    partyMember->object = object;
    partyMember->script = NULL;
    partyMember->vars = NULL;

    object->id = (object->pid & 0xFFFFFF) + 18000;
    object->flags |= (OBJECT_FLAG_0x400 | OBJECT_TEMPORARY);

    partyMemberCount++;

    Script* script;
    if (scriptGetScript(object->sid, &script) != -1) {
        script->flags |= (SCRIPT_FLAG_0x08 | SCRIPT_FLAG_0x10);
        script->field_1C = object->id;

        object->sid = ((object->pid & 0xFFFFFF) + 18000) | (object->sid & 0xFF000000);
        script->sid = object->sid;
    }

    combatai_switch_team(object, 0);
    queueRemoveEventsByType(object, EVENT_TYPE_SCRIPT);

    if (gdialogActive()) {
        if (object == dialog_target) {
            gdialogUpdatePartyStatus();
        }
    }

    return 0;
}

// 0x4944DC
int partyMemberRemove(Object* object)
{
    if (partyMemberCount == 0) {
        return -1;
    }

    if (object == NULL) {
        return -1;
    }

    int index;
    for (index = 1; index < partyMemberCount; index++) {
        STRUCT_519DA8* partyMember = &(partyMemberList[index]);
        if (partyMember->object == object) {
            break;
        }
    }

    if (index == partyMemberCount) {
        return -1;
    }

    if (partyStatePrepped) {
        debugPrint("\npartyMemberRemove DENIED: %s\n", critter_name(object));
        return -1;
    }

    if (index < partyMemberCount - 1) {
        partyMemberList[index].object = partyMemberList[partyMemberCount - 1].object;
    }

    object->flags &= ~(OBJECT_FLAG_0x400 | OBJECT_TEMPORARY);

    partyMemberCount--;

    Script* script;
    if (scriptGetScript(object->sid, &script) != -1) {
        script->flags &= ~(SCRIPT_FLAG_0x08 | SCRIPT_FLAG_0x10);
    }

    queueRemoveEventsByType(object, EVENT_TYPE_SCRIPT);

    if (gdialogActive()) {
        if (object == dialog_target) {
            gdialogUpdatePartyStatus();
        }
    }

    return 0;
}

// 0x49460C
int partyMemberPrepSave()
{
    partyStatePrepped = 1;

    for (int index = 0; index < partyMemberCount; index++) {
        STRUCT_519DA8* ptr = &(partyMemberList[index]);

        if (index > 0) {
            ptr->object->flags &= ~(OBJECT_FLAG_0x400 | OBJECT_TEMPORARY);
        }

        Script* script;
        if (scriptGetScript(ptr->object->sid, &script) != -1) {
            script->flags &= ~(SCRIPT_FLAG_0x08 | SCRIPT_FLAG_0x10);
        }
    }

    return 0;
}

// 0x49466C
int partyMemberUnPrepSave()
{
    for (int index = 0; index < partyMemberCount; index++) {
        STRUCT_519DA8* ptr = &(partyMemberList[index]);

        if (index > 0) {
            ptr->object->flags |= (OBJECT_FLAG_0x400 | OBJECT_TEMPORARY);
        }

        Script* script;
        if (scriptGetScript(ptr->object->sid, &script) != -1) {
            script->flags |= (SCRIPT_FLAG_0x08 | SCRIPT_FLAG_0x10);
        }
    }

    partyStatePrepped = 0;

    return 0;
}

// 0x4946CC
int partyMemberSave(File* stream)
{
    if (fileWriteInt32(stream, partyMemberCount) == -1) return -1;
    if (fileWriteInt32(stream, partyMemberItemCount) == -1) return -1;

    for (int index = 1; index < partyMemberCount; index++) {
        STRUCT_519DA8* partyMember = &(partyMemberList[index]);
        if (fileWriteInt32(stream, partyMember->object->id) == -1) return -1;
    }

    for (int index = 1; index < partyMemberMaxCount; index++) {
        STRU_519DBC* ptr = &(partyMemberLevelUpInfoList[index]);
        if (fileWriteInt32(stream, ptr->field_0) == -1) return -1;
        if (fileWriteInt32(stream, ptr->field_4) == -1) return -1;
        if (fileWriteInt32(stream, ptr->field_8) == -1) return -1;
    }

    return 0;
}

// 0x4947AC
int partyMemberPrepLoad()
{
    if (partyStatePrepped) {
        return -1;
    }

    partyStatePrepped = 1;

    for (int index = 0; index < partyMemberCount; index++) {
        STRUCT_519DA8* ptr_519DA8 = &(partyMemberList[index]);
        if (partyMemberPrepLoadInstance(ptr_519DA8) != 0) {
            return -1;
        }
    }

    return 0;
}

// 0x49480C
static int partyMemberPrepLoadInstance(STRUCT_519DA8* a1)
{
    Object* obj = a1->object;

    if (obj == NULL) {
        debugPrint("\n  Error!: partyMemberPrepLoadInstance: No Critter Object!");
        a1->script = NULL;
        a1->vars = NULL;
        a1->next = NULL;
        return 0;
    }

    if (PID_TYPE(obj->pid) == OBJ_TYPE_CRITTER) {
        obj->data.critter.combat.whoHitMe = NULL;
    }

    Script* script;
    if (scriptGetScript(obj->sid, &script) == -1) {
        debugPrint("\n  Error!: partyMemberPrepLoadInstance: Can't find script!");
        debugPrint("\n          partyMemberPrepLoadInstance: script was: (%s)", critter_name(obj));
        a1->script = NULL;
        a1->vars = NULL;
        a1->next = NULL;
        return 0;
    }

    a1->script = (Script*)internal_malloc(sizeof(*script));
    if (a1->script == NULL) {
        showMesageBox("\n  Error!: partyMemberPrepLoad: Out of memory!");
        exit(1);
    }

    memcpy(a1->script, script, sizeof(*script));

    if (script->localVarsCount != 0 && script->localVarsOffset != -1) {
        a1->vars = (int*)internal_malloc(sizeof(*a1->vars) * script->localVarsCount);
        if (a1->vars == NULL) {
            showMesageBox("\n  Error!: partyMemberPrepLoad: Out of memory!");
            exit(1);
        }

        if (map_local_vars != NULL) {
            memcpy(a1->vars, map_local_vars + script->localVarsOffset, sizeof(int) * script->localVarsCount);
        } else {
            debugPrint("\nWarning: partyMemberPrepLoadInstance: No map_local_vars found, but script references them!");
            memset(a1->vars, 0, sizeof(int) * script->localVarsCount);
        }
    }

    Inventory* inventory = &(obj->data.inventory);
    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        partyMemberItemSave(inventoryItem->item);
    }

    script->flags &= ~(SCRIPT_FLAG_0x08 | SCRIPT_FLAG_0x10);

    scriptRemove(script->sid);

    if (PID_TYPE(obj->pid) == OBJ_TYPE_CRITTER) {
        dude_stand(obj, obj->rotation, -1);
    }

    return 0;
}

// 0x4949C4
int partyMemberRecoverLoad()
{
    if (partyStatePrepped != 1) {
        debugPrint("\npartyMemberRecoverLoad DENIED");
        return -1;
    }

    debugPrint("\n");

    for (int index = 0; index < partyMemberCount; index++) {
        if (partyMemberRecoverLoadInstance(&(partyMemberList[index])) != 0) {
            return -1;
        }

        debugPrint("[Party Member %d]: %s\n", index, critter_name(partyMemberList[index].object));
    }

    STRUCT_519DA8* v6 = itemSaveListHead;
    while (v6 != NULL) {
        itemSaveListHead = v6->next;

        partyMemberItemRecover(v6);
        internal_free(v6);

        v6 = itemSaveListHead;
    }

    partyStatePrepped = 0;

    if (!isLoadingGame()) {
        partyFixMultipleMembers();
    }

    return 0;
}

// 0x494A88
static int partyMemberRecoverLoadInstance(STRUCT_519DA8* a1)
{
    if (a1->script == NULL) {
        showMesageBox("\n  Error!: partyMemberRecoverLoadInstance: No script!");
        return 0;
    }

    int scriptType = SCRIPT_TYPE_CRITTER;
    if (PID_TYPE(a1->object->pid) != OBJ_TYPE_CRITTER) {
        scriptType = SCRIPT_TYPE_ITEM;
    }

    int v1 = -1;
    if (scriptAdd(&v1, scriptType) == -1) {
        showMesageBox("\n  Error!: partyMemberRecoverLoad: Can't create script!");
        exit(1);
    }

    Script* script;
    if (scriptGetScript(v1, &script) == -1) {
        showMesageBox("\n  Error!: partyMemberRecoverLoad: Can't find script!");
        exit(1);
    }

    memcpy(script, a1->script, sizeof(*script));

    int sid = (scriptType << 24) | ((a1->object->pid & 0xFFFFFF) + 18000);
    a1->object->sid = sid;
    script->sid = sid;

    script->flags &= ~(SCRIPT_FLAG_0x01 | SCRIPT_FLAG_0x04);

    internal_free(a1->script);
    a1->script = NULL;

    script->flags |= (SCRIPT_FLAG_0x08 | SCRIPT_FLAG_0x10);

    if (a1->vars != NULL) {
        script->localVarsOffset = map_malloc_local_var(script->localVarsCount);
        memcpy(map_local_vars + script->localVarsOffset, a1->vars, sizeof(int) * script->localVarsCount);
    }

    return 0;
}

// 0x494BBC
int partyMemberLoad(File* stream)
{
    int* partyMemberObjectIds = (int*)internal_malloc(sizeof(*partyMemberObjectIds) * (partyMemberMaxCount + 20));
    if (partyMemberObjectIds == NULL) {
        return -1;
    }

    // FIXME: partyMemberObjectIds is never free'd in this function, obviously memory leak.

    if (fileReadInt32(stream, &partyMemberCount) == -1) return -1;
    if (fileReadInt32(stream, &partyMemberItemCount) == -1) return -1;

    partyMemberList->object = obj_dude;

    if (partyMemberCount != 0) {
        for (int index = 1; index < partyMemberCount; index++) {
            if (fileReadInt32(stream, &(partyMemberObjectIds[index])) == -1) return -1;
        }

        for (int index = 1; index < partyMemberCount; index++) {
            int objectId = partyMemberObjectIds[index];

            Object* object = obj_find_first();
            while (object != NULL) {
                if (object->id == objectId) {
                    break;
                }
                object = obj_find_next();
            }

            if (object != NULL) {
                partyMemberList[index].object = object;
            } else {
                debugPrint("Couldn't find party member on map...trying to load anyway.\n");
                if (index + 1 >= partyMemberCount) {
                    partyMemberObjectIds[index] = 0;
                } else {
                    memcpy(&(partyMemberObjectIds[index]), &(partyMemberObjectIds[index + 1]), sizeof(*partyMemberObjectIds) * (partyMemberCount - (index + 1)));
                }

                index--;
                partyMemberCount--;
            }
        }

        if (partyMemberUnPrepSave() == -1) {
            return -1;
        }
    }

    partyFixMultipleMembers();

    for (int index = 1; index < partyMemberMaxCount; index++) {
        STRU_519DBC* ptr_519DBC = &(partyMemberLevelUpInfoList[index]);

        if (fileReadInt32(stream, &(ptr_519DBC->field_0)) == -1) return -1;
        if (fileReadInt32(stream, &(ptr_519DBC->field_4)) == -1) return -1;
        if (fileReadInt32(stream, &(ptr_519DBC->field_8)) == -1) return -1;
    }

    return 0;
}

// 0x494D7C
void partyMemberClear()
{
    if (partyStatePrepped) {
        partyMemberUnPrepSave();
    }

    for (int index = partyMemberCount; index > 1; index--) {
        partyMemberRemove(partyMemberList[1].object);
    }

    partyMemberCount = 1;

    _scr_remove_all();
    partyMemberClearItemList();

    partyStatePrepped = 0;
}

// 0x494DD0
int partyMemberSyncPosition()
{
    int clockwiseRotation = (obj_dude->rotation + 2) % ROTATION_COUNT;
    int counterClockwiseRotation = (obj_dude->rotation + 4) % ROTATION_COUNT;

    int n = 0;
    int distance = 2;
    for (int index = 1; index < partyMemberCount; index++) {
        STRUCT_519DA8* partyMember = &(partyMemberList[index]);
        Object* partyMemberObj = partyMember->object;
        if ((partyMemberObj->flags & OBJECT_HIDDEN) == 0 && PID_TYPE(partyMemberObj->pid) == OBJ_TYPE_CRITTER) {
            int rotation;
            if ((n % 2) != 0) {
                rotation = clockwiseRotation;
            } else {
                rotation = counterClockwiseRotation;
            }

            int tile = tileGetTileInDirection(obj_dude->tile, rotation, distance / 2);
            _objPMAttemptPlacement(partyMemberObj, tile, obj_dude->elevation);

            distance++;
            n++;
        }
    }

    return 0;
}

// Heals party members according to their healing rate.
//
// 0x494EB8
int partyMemberRestingHeal(int a1)
{
    int v1 = a1 / 3;
    if (v1 == 0) {
        return 0;
    }

    for (int index = 0; index < partyMemberCount; index++) {
        STRUCT_519DA8* partyMember = &(partyMemberList[index]);
        if (PID_TYPE(partyMember->object->pid) == OBJ_TYPE_CRITTER) {
            int healingRate = critterGetStat(partyMember->object, STAT_HEALING_RATE);
            critter_adjust_hits(partyMember->object, v1 * healingRate);
        }
    }

    return 1;
}

// 0x494F24
Object* partyMemberFindObjFromPid(int pid)
{
    for (int index = 0; index < partyMemberCount; index++) {
        Object* object = partyMemberList[index].object;
        if (object->pid == pid) {
            return object;
        }
    }

    return NULL;
}

// 0x494F64
bool isPotentialPartyMember(Object* object)
{
    for (int index = 0; index < partyMemberCount; index++) {
        STRUCT_519DA8* partyMember = &(partyMemberList[index]);
        if (partyMember->object->pid == partyMemberPidList[index]) {
            return true;
        }
    }

    return false;
}

// Returns `true` if specified object is a party member.
//
// 0x494FC4
bool isPartyMember(Object* object)
{
    if (object == NULL) {
        return false;
    }

    if (object->id < 18000) {
        return false;
    }

    bool isPartyMember = false;

    for (int index = 0; index < partyMemberCount; index++) {
        if (partyMemberList[index].object == object) {
            isPartyMember = true;
            break;
        }
    }

    return isPartyMember;
}

// Returns number of active critters in the party.
//
// 0x495010
int getPartyMemberCount()
{
    int count = partyMemberCount;

    for (int index = 1; index < partyMemberCount; index++) {
        Object* object = partyMemberList[index].object;

        if (PID_TYPE(object->pid) != OBJ_TYPE_CRITTER || critter_is_dead(object) || (object->flags & OBJECT_HIDDEN) != 0) {
            count--;
        }
    }

    return count;
}

// 0x495070
static int partyMemberNewObjID()
{
    // 0x519DC0
    static int curID = 20000;

    Object* object;

    do {
        curID++;

        object = obj_find_first();
        while (object != NULL) {
            if (object->id == curID) {
                break;
            }

            Inventory* inventory = &(object->data.inventory);

            int index;
            for (index = 0; index < inventory->length; index++) {
                InventoryItem* inventoryItem = &(inventory->items[index]);
                Object* item = inventoryItem->item;
                if (item->id == curID) {
                    break;
                }

                if (partyMemberNewObjIDRecurseFind(item, curID)) {
                    break;
                }
            }

            if (index < inventory->length) {
                break;
            }

            object = obj_find_next();
        }
    } while (object != NULL);

    curID++;

    return curID;
}

// 0x4950F4
static int partyMemberNewObjIDRecurseFind(Object* obj, int objectId)
{
    Inventory* inventory = &(obj->data.inventory);
    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        if (inventoryItem->item->id == objectId) {
            return 1;
        }

        if (partyMemberNewObjIDRecurseFind(inventoryItem->item, objectId)) {
            return 1;
        }
    }

    return 0;
}

// 0x495140
int partyMemberPrepItemSaveAll()
{
    for (int partyMemberIndex = 0; partyMemberIndex < partyMemberCount; partyMemberIndex++) {
        STRUCT_519DA8* partyMember = &(partyMemberList[partyMemberIndex]);

        Inventory* inventory = &(partyMember->object->data.inventory);
        for (int inventoryItemIndex = 0; inventoryItemIndex < inventory->length; inventoryItemIndex++) {
            InventoryItem* inventoryItem = &(inventory->items[inventoryItemIndex]);
            partyMemberPrepItemSave(inventoryItem->item);
        }
    }

    return 0;
}

// partyMemberPrepItemSaveAll
static int partyMemberPrepItemSave(Object* object)
{
    if (object->sid != -1) {
        Script* script;
        if (scriptGetScript(object->sid, &script) == -1) {
            showMesageBox("\n  Error!: partyMemberPrepItemSaveAll: Can't find script!");
            exit(1);
        }

        script->flags |= (SCRIPT_FLAG_0x08 | SCRIPT_FLAG_0x10);
    }

    Inventory* inventory = &(object->data.inventory);
    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        partyMemberPrepItemSave(inventoryItem->item);
    }

    return 0;
}

// 0x495234
static int partyMemberItemSave(Object* object)
{
    if (object->sid != -1) {
        Script* script;
        if (scriptGetScript(object->sid, &script) == -1) {
            showMesageBox("\n  Error!: partyMemberItemSave: Can't find script!");
            exit(1);
        }

        if (object->id < 20000) {
            script->field_1C = partyMemberNewObjID();
            object->id = script->field_1C;
        }

        STRUCT_519DA8* node = (STRUCT_519DA8*)internal_malloc(sizeof(*node));
        if (node == NULL) {
            showMesageBox("\n  Error!: partyMemberItemSave: Out of memory!");
            exit(1);
        }

        node->object = object;

        node->script = (Script*)internal_malloc(sizeof(*script));
        if (node->script == NULL) {
            showMesageBox("\n  Error!: partyMemberItemSave: Out of memory!");
            exit(1);
        }

        memcpy(node->script, script, sizeof(*script));

        if (script->localVarsCount != 0 && script->localVarsOffset != -1) {
            node->vars = (int*)internal_malloc(sizeof(*node->vars) * script->localVarsCount);
            if (node->vars == NULL) {
                showMesageBox("\n  Error!: partyMemberItemSave: Out of memory!");
                exit(1);
            }

            memcpy(node->vars, map_local_vars + script->localVarsOffset, sizeof(int) * script->localVarsCount);
        } else {
            node->vars = NULL;
        }

        STRUCT_519DA8* temp = itemSaveListHead;
        itemSaveListHead = node;
        node->next = temp;
    }

    Inventory* inventory = &(object->data.inventory);
    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        partyMemberItemSave(inventoryItem->item);
    }

    return 0;
}

// partyMemberItemRecover
// 0x495388
static int partyMemberItemRecover(STRUCT_519DA8* a1)
{
    int sid = -1;
    if (scriptAdd(&sid, SCRIPT_TYPE_ITEM) == -1) {
        showMesageBox("\n  Error!: partyMemberItemRecover: Can't create script!");
        exit(1);
    }

    Script* script;
    if (scriptGetScript(sid, &script) == -1) {
        showMesageBox("\n  Error!: partyMemberItemRecover: Can't find script!");
        exit(1);
    }

    memcpy(script, a1->script, sizeof(*script));

    a1->object->sid = partyMemberItemCount | (SCRIPT_TYPE_ITEM << 24);
    script->sid = partyMemberItemCount | (SCRIPT_TYPE_ITEM << 24);

    script->program = NULL;
    script->flags &= ~(SCRIPT_FLAG_0x01 | SCRIPT_FLAG_0x04 | SCRIPT_FLAG_0x08 | SCRIPT_FLAG_0x10);

    partyMemberItemCount++;

    internal_free(a1->script);
    a1->script = NULL;

    if (a1->vars != NULL) {
        script->localVarsOffset = map_malloc_local_var(script->localVarsCount);
        memcpy(map_local_vars + script->localVarsOffset, a1->vars, sizeof(int) * script->localVarsCount);
    }

    return 0;
}

// 0x4954C4
static int partyMemberClearItemList()
{
    while (itemSaveListHead != NULL) {
        STRUCT_519DA8* node = itemSaveListHead;
        itemSaveListHead = itemSaveListHead->next;

        if (node->script != NULL) {
            internal_free(node->script);
        }

        if (node->vars != NULL) {
            internal_free(node->vars);
        }

        internal_free(node);
    }

    partyMemberItemCount = 20000;

    return 0;
}

// Returns best skill of the specified party member.
//
// 0x495520
int partyMemberSkill(Object* object)
{
    int bestSkill = SKILL_SMALL_GUNS;

    if (object == NULL) {
        return bestSkill;
    }

    if (PID_TYPE(object->pid) != OBJ_TYPE_CRITTER) {
        return bestSkill;
    }

    int bestValue = 0;
    for (int skill = 0; skill < SKILL_COUNT; skill++) {
        int value = skillGetValue(object, skill);
        if (value > bestValue) {
            bestSkill = skill;
            bestValue = value;
        }
    }

    return bestSkill;
}

// Returns party member with highest skill level.
//
// 0x495560
Object* partyMemberWithHighestSkill(int skill)
{
    int bestValue = 0;
    Object* bestPartyMember = NULL;

    for (int index = 0; index < partyMemberCount; index++) {
        Object* object = partyMemberList[index].object;
        if ((object->flags & OBJECT_HIDDEN) == 0 && PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
            int value = skillGetValue(object, skill);
            if (value > bestValue) {
                bestValue = value;
                bestPartyMember = object;
            }
        }
    }

    return bestPartyMember;
}

// Returns highest skill level in party.
//
// 0x4955C8
int partyMemberHighestSkillLevel(int skill)
{
    int bestValue = 0;

    for (int index = 0; index < partyMemberCount; index++) {
        Object* object = partyMemberList[index].object;
        if ((object->flags & OBJECT_HIDDEN) == 0 && PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
            int value = skillGetValue(object, skill);
            if (value > bestValue) {
                bestValue = value;
            }
        }
    }

    return bestValue;
}

// 0x495620
static int partyFixMultipleMembers()
{
    debugPrint("\n\n\n[Party Members]:");

    int critterCount = 0;
    for (Object* obj = obj_find_first(); obj != NULL; obj = obj_find_next()) {
        if (PID_TYPE(obj->pid) == OBJ_TYPE_CRITTER) {
            critterCount++;
        }

        bool isPartyMember = false;
        for (int index = 1; index < partyMemberMaxCount; index++) {
            if (obj->pid == partyMemberPidList[index]) {
                isPartyMember = true;
                break;
            }
        }

        if (!isPartyMember) {
            continue;
        }

        debugPrint("\n   PM: %s", critter_name(obj));

        bool v19 = false;
        if (obj->sid == -1) {
            v19 = true;
        } else {
            Object* v7 = NULL;
            for (int i = 0; i < partyMemberCount; i++) {
                if (obj->pid == partyMemberList[i].object->pid) {
                    v7 = partyMemberList[i].object;
                    break;
                }
            }

            if (v7 != NULL && obj != v7) {
                if (v7->sid == obj->sid) {
                    obj->sid = -1;
                }
                v19 = true;
            }
        }

        if (!v19) {
            continue;
        }

        Object* v10 = NULL;
        for (int i = 0; i < partyMemberCount; i++) {
            if (obj->pid == partyMemberList[i].object->pid) {
                v10 = partyMemberList[i].object;
            }
        }

        // TODO: Probably wrong.
        if (obj == v10) {
            debugPrint("\nError: Attempting to destroy evil critter doppleganger FAILED!");
            continue;
        }

        debugPrint("\nDestroying evil critter doppleganger!");

        if (obj->sid != -1) {
            scriptRemove(obj->sid);
            obj->sid = -1;
        } else {
            if (queueRemoveEventsByType(obj, EVENT_TYPE_SCRIPT) == -1) {
                debugPrint("\nERROR Removing Timed Events on FIX remove!!\n");
            }
        }

        obj_erase_object(obj, NULL);
    }

    for (int index = 0; index < partyMemberCount; index++) {
        STRUCT_519DA8* partyMember = &(partyMemberList[index]);

        Script* script;
        if (scriptGetScript(partyMember->object->sid, &script) != -1) {
            script->owner = partyMember->object;
        } else {
            debugPrint("\nError: Failed to fix party member critter scripts!");
        }
    }

    debugPrint("\nTotal Critter Count: %d\n\n", critterCount);

    return 0;
}

// 0x495870
void partyMemberSaveProtos()
{
    for (int index = 1; index < partyMemberMaxCount; index++) {
        int pid = partyMemberPidList[index];
        if (pid != -1) {
            _proto_save_pid(pid);
        }
    }
}

// 0x4958B0
bool partyMemberHasAIDisposition(Object* critter, int disposition)
{
    if (critter == NULL) {
        return false;
    }

    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        return false;
    }

    if (disposition == -1 || disposition > 5) {
        return false;
    }

    PartyMemberDescription* partyMemberDescription;
    if (partyMemberGetAIOptions(critter, &partyMemberDescription) == -1) {
        return false;
    }

    return partyMemberDescription->disposition[disposition + 1];
}

// 0x495920
bool partyMemberHasAIBurstValue(Object* object, int areaAttackMode)
{
    if (object == NULL) {
        return false;
    }

    if (PID_TYPE(object->pid) != OBJ_TYPE_CRITTER) {
        return false;
    }

    if (areaAttackMode >= AREA_ATTACK_MODE_COUNT) {
        return false;
    }

    PartyMemberDescription* partyMemberDescription;
    if (partyMemberGetAIOptions(object, &partyMemberDescription) == -1) {
        return false;
    }

    return partyMemberDescription->areaAttackMode[areaAttackMode];
}

// 0x495980
bool partyMemberHasAIRunAwayValue(Object* object, int runAwayMode)
{
    if (object == NULL) {
        return false;
    }

    if (PID_TYPE(object->pid) != OBJ_TYPE_CRITTER) {
        return false;
    }

    if (runAwayMode >= RUN_AWAY_MODE_COUNT) {
        return false;
    }

    PartyMemberDescription* partyMemberDescription;
    if (partyMemberGetAIOptions(object, &partyMemberDescription) == -1) {
        return false;
    }

    return partyMemberDescription->runAwayMode[runAwayMode + 1];
}

// 0x4959E0
bool partyMemberHasAIWeaponPrefValue(Object* object, int bestWeapon)
{
    if (object == NULL) {
        return false;
    }

    if (PID_TYPE(object->pid) != OBJ_TYPE_CRITTER) {
        return false;
    }

    if (bestWeapon >= BEST_WEAPON_COUNT) {
        return false;
    }

    PartyMemberDescription* partyMemberDescription;
    if (partyMemberGetAIOptions(object, &partyMemberDescription) == -1) {
        return false;
    }

    return partyMemberDescription->bestWeapon[bestWeapon];
}

// 0x495A40
bool partyMemberHasAIDistancePrefValue(Object* object, int distanceMode)
{
    if (object == NULL) {
        return false;
    }

    if (PID_TYPE(object->pid) != OBJ_TYPE_CRITTER) {
        return false;
    }

    if (distanceMode >= DISTANCE_COUNT) {
        return false;
    }

    PartyMemberDescription* partyMemberDescription;
    if (partyMemberGetAIOptions(object, &partyMemberDescription) == -1) {
        return false;
    }

    return partyMemberDescription->distanceMode[distanceMode];
}

// 0x495AA0
bool partyMemberHasAIAttackWhoValue(Object* object, int attackWho)
{
    if (object == NULL) {
        return false;
    }

    if (PID_TYPE(object->pid) != OBJ_TYPE_CRITTER) {
        return false;
    }

    if (attackWho >= ATTACK_WHO_COUNT) {
        return false;
    }

    PartyMemberDescription* partyMemberDescription;
    if (partyMemberGetAIOptions(object, &partyMemberDescription) == -1) {
        return false;
    }

    return partyMemberDescription->attackWho[attackWho];
}

// 0x495B00
bool partyMemberHasAIChemUseValue(Object* object, int chemUse)
{
    if (object == NULL) {
        return false;
    }

    if (PID_TYPE(object->pid) != OBJ_TYPE_CRITTER) {
        return false;
    }

    if (chemUse >= CHEM_USE_COUNT) {
        return false;
    }

    PartyMemberDescription* partyMemberDescription;
    if (partyMemberGetAIOptions(object, &partyMemberDescription) == -1) {
        return false;
    }

    return partyMemberDescription->chemUse[chemUse];
}

// partyMemberIncLevels
// 0x495B60
int partyMemberIncLevels()
{
    int i;
    STRUCT_519DA8* ptr;
    Object* obj;
    PartyMemberDescription* party_member;
    const char* name;
    int j;
    int v0;
    STRU_519DBC* ptr_519DBC;
    int v24;
    char* text;
    MessageListItem msg;
    char str[260];
    Rect v19;

    v0 = -1;
    for (i = 1; i < partyMemberCount; i++) {
        ptr = &(partyMemberList[i]);
        obj = ptr->object;

        if (partyMemberGetAIOptions(obj, &party_member) == -1) {
            break;
        }

        if (PID_TYPE(obj->pid) != OBJ_TYPE_CRITTER) {
            continue;
        }

        name = critter_name(obj);
        debugPrint("\npartyMemberIncLevels: %s", name);

        if (party_member->level_up_every == 0) {
            continue;
        }

        for (j = 1; j < partyMemberMaxCount; j++) {
            if (partyMemberPidList[j] == obj->pid) {
                v0 = j;
            }
        }

        if (v0 == -1) {
            continue;
        }

        if (pcGetStat(PC_STAT_LEVEL) < party_member->level_minimum) {
            continue;
        }

        ptr_519DBC = &(partyMemberLevelUpInfoList[v0]);

        if (ptr_519DBC->field_0 >= party_member->level_pids_num) {
            continue;
        }

        ptr_519DBC->field_4++;

        v24 = ptr_519DBC->field_4 % party_member->level_pids_num;
        debugPrint("pm: levelMod: %d, Lvl: %d, Early: %d, Every: %d", v24, ptr_519DBC->field_4, ptr_519DBC->field_8, party_member->level_up_every);

        if (v24 != 0 || ptr_519DBC->field_8 == 0) {
            if (ptr_519DBC->field_8 == 0) {
                if (v24 == 0 || randomBetween(0, 100) <= 100 * v24 / party_member->level_up_every) {
                    ptr_519DBC->field_0++;
                    if (v24 != 0) {
                        ptr_519DBC->field_8 = 1;
                    }

                    if (partyMemberCopyLevelInfo(obj, party_member->level_pids[ptr_519DBC->field_0]) == -1) {
                        return -1;
                    }

                    name = critter_name(obj);
                    // %s has gained in some abilities.
                    text = getmsg(&misc_message_file, &msg, 9000);
                    sprintf(str, text, name);
                    display_print(str);

                    debugPrint(str);

                    // Individual message
                    msg.num = 9000 + 10 * v0 + ptr_519DBC->field_0 - 1;
                    if (message_search(&misc_message_file, &msg)) {
                        name = critter_name(obj);
                        sprintf(str, msg.text, name);
                        textObjectAdd(obj, str, 101, colorTable[0x7FFF], colorTable[0], &v19);
                        tileWindowRefreshRect(&v19, obj->elevation);
                    }
                }
            }
        } else {
            ptr_519DBC->field_8 = 0;
        }
    }

    return 0;
}

// 0x495EA8
static int partyMemberCopyLevelInfo(Object* critter, int a2)
{
    if (critter == NULL) {
        return -1;
    }

    if (a2 == -1) {
        return -1;
    }

    Proto* proto1;
    if (protoGetProto(critter->pid, &proto1) == -1) {
        return -1;
    }

    Proto* proto2;
    if (protoGetProto(a2, &proto2) == -1) {
        return -1;
    }

    Object* item2 = inven_right_hand(critter);
    invenUnwieldFunc(critter, 1, 0);

    Object* armor = inven_worn(critter);
    adjust_ac(critter, armor, NULL);
    item_remove_mult(critter, armor, 1);

    int maxHp = critterGetStat(critter, STAT_MAXIMUM_HIT_POINTS);
    critter_adjust_hits(critter, maxHp);

    for (int stat = 0; stat < SPECIAL_STAT_COUNT; stat++) {
        proto1->critter.data.baseStats[stat] = proto2->critter.data.baseStats[stat];
    }

    for (int stat = 0; stat < SPECIAL_STAT_COUNT; stat++) {
        proto1->critter.data.bonusStats[stat] = proto2->critter.data.bonusStats[stat];
    }

    for (int skill = 0; skill < SKILL_COUNT; skill++) {
        proto1->critter.data.skills[skill] = proto2->critter.data.skills[skill];
    }

    critter->data.critter.hp = critterGetStat(critter, STAT_MAXIMUM_HIT_POINTS);

    if (armor != NULL) {
        item_add_force(critter, armor, 1);
        inven_wield(critter, armor, 0);
    }

    if (item2 != NULL) {
        invenWieldFunc(critter, item2, 0, false);
    }

    return 0;
}

// Returns `true` if any party member that can be healed thru the rest is
// wounded.
//
// This function is used to determine if any party member needs healing thru
// the "Rest until party healed", therefore it excludes robots in the party
// (they cannot be healed by resting) and dude (he/she has it's own "Rest
// until healed" option).
//
// 0x496058
bool partyMemberNeedsHealing()
{
    for (int index = 1; index < partyMemberCount; index++) {
        STRUCT_519DA8* ptr = &(partyMemberList[index]);
        Object* object = ptr->object;

        if (PID_TYPE(object->pid) != OBJ_TYPE_CRITTER) continue;
        if (critter_is_dead(object)) continue;
        if ((object->flags & OBJECT_HIDDEN) != 0) continue;
        if (critterGetKillType(object) == KILL_TYPE_ROBOT) continue;

        int currentHp = critter_get_hits(object);
        int maximumHp = critterGetStat(object, STAT_MAXIMUM_HIT_POINTS);
        if (currentHp < maximumHp) {
            return true;
        }
    }

    return false;
}

// Returns maximum amount of damage of any party member that can be healed thru
// the rest.
//
// 0x4960DC
int partyMemberMaxHealingNeeded()
{
    int maxWound = 0;

    for (int index = 1; index < partyMemberCount; index++) {
        STRUCT_519DA8* ptr = &(partyMemberList[index]);
        Object* object = ptr->object;

        if (PID_TYPE(object->pid) != OBJ_TYPE_CRITTER) continue;
        if (critter_is_dead(object)) continue;
        if ((object->flags & OBJECT_HIDDEN) != 0) continue;
        if (critterGetKillType(object) == KILL_TYPE_ROBOT) continue;

        int currentHp = critter_get_hits(object);
        int maximumHp = critterGetStat(object, STAT_MAXIMUM_HIT_POINTS);
        int wound = maximumHp - currentHp;
        if (wound > 0) {
            if (wound > maxWound) {
                maxWound = wound;
            }
        }
    }

    return maxWound;
}