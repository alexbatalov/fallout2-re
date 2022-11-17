#include "game/perk.h"

#include <stdio.h>

#include "plib/gnw/debug.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/message.h"
#include "plib/gnw/memory.h"
#include "game/object.h"
#include "game/party.h"
#include "game/skill.h"
#include "game/stat.h"

typedef struct PerkDescription {
    char* name;
    char* description;
    int frmId;
    int maxRank;
    int minLevel;
    int stat;
    int statModifier;
    int param1;
    int value1;
    int field_24;
    int param2;
    int value2;
    int stats[PRIMARY_STAT_COUNT];
} PerkDescription;

static bool perk_can_add(Object* critter, int perk);
static void perk_defaults();

// 0x519DCC
static PerkDescription perk_data[PERK_COUNT] = {
    { NULL, NULL, 72, 1, 3, -1, 0, -1, 0, 0, -1, 0, 0, 5, 0, 0, 0, 0, 0 },
    { NULL, NULL, 73, 1, 15, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 6, 0 },
    { NULL, NULL, 74, 3, 3, 11, 2, -1, 0, 0, -1, 0, 6, 0, 0, 0, 0, 6, 0 },
    { NULL, NULL, 75, 2, 6, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 5, 0 },
    { NULL, NULL, 76, 2, 6, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 6, 6 },
    { NULL, NULL, 77, 1, 15, -1, 0, -1, 0, 0, -1, 0, 0, 6, 0, 0, 6, 7, 0 },
    { NULL, NULL, 78, 3, 3, 13, 2, -1, 0, 0, -1, 0, 0, 6, 0, 0, 0, 0, 0 },
    { NULL, NULL, 79, 3, 3, 14, 2, -1, 0, 0, -1, 0, 0, 0, 6, 0, 0, 0, 0 },
    { NULL, NULL, 80, 3, 6, 15, 5, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 6 },
    { NULL, NULL, 81, 1, 3, -1, 0, -1, 0, 0, -1, 0, 0, 6, 0, 0, 0, 0, 0 },
    { NULL, NULL, 82, 3, 3, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 6, 0, 0, 0 },
    { NULL, NULL, 83, 2, 6, 31, 15, -1, 0, 0, -1, 0, 0, 0, 6, 0, 4, 0, 0 },
    { NULL, NULL, 84, 3, 3, 24, 10, -1, 0, 0, -1, 0, 0, 0, 6, 0, 0, 0, 6 },
    { NULL, NULL, 85, 3, 3, 12, 50, -1, 0, 0, -1, 0, 6, 0, 6, 0, 0, 0, 0 },
    { NULL, NULL, 86, 1, 9, -1, 0, -1, 0, 0, -1, 0, 0, 7, 0, 0, 6, 0, 0 },
    { NULL, NULL, 87, 1, 6, -1, 0, 8, 50, 0, -1, 0, 0, 0, 0, 0, 0, 6, 0 },
    { NULL, NULL, 88, 1, 3, -1, 0, 17, 40, 0, -1, 0, 0, 0, 6, 0, 6, 0, 0 },
    { NULL, NULL, 89, 1, 12, -1, 0, 15, 75, 0, -1, 0, 0, 0, 0, 7, 0, 0, 0 },
    { NULL, NULL, 90, 3, 6, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 6, 0, 0 },
    { NULL, NULL, 91, 2, 3, -1, 0, 6, 40, 0, -1, 0, 0, 7, 0, 0, 5, 6, 0 },
    { NULL, NULL, 92, 1, 6, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 8 },
    { NULL, NULL, 93, 1, 9, 16, 20, -1, 0, 0, -1, 0, 0, 6, 0, 0, 0, 4, 6 },
    { NULL, NULL, 94, 1, 6, -1, 0, -1, 0, 0, -1, 0, 0, 7, 0, 0, 5, 0, 0 },
    { NULL, NULL, 95, 1, 24, -1, 0, 3, 80, 0, -1, 0, 8, 0, 0, 0, 0, 8, 0 },
    { NULL, NULL, 96, 1, 24, -1, 0, 0, 80, 0, -1, 0, 0, 8, 0, 0, 0, 8, 0 },
    { NULL, NULL, 97, 1, 18, -1, 0, 8, 80, 2, 3, 80, 0, 0, 0, 0, 0, 10, 0 },
    { NULL, NULL, 98, 2, 12, 8, 1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 5, 0 },
    { NULL, NULL, 99, 1, 310, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 100, 2, 12, -1, 0, -1, 0, 0, -1, 0, 0, 0, 4, 0, 0, 0, 0 },
    { NULL, NULL, 101, 1, 9, 9, 5, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 6, 0 },
    { NULL, NULL, 102, 2, 6, 32, 25, -1, 0, 0, -1, 0, 0, 0, 3, 0, 0, 0, 0 },
    { NULL, NULL, 103, 1, 12, -1, 0, 13, 40, 1, 12, 40, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 104, 1, 12, -1, 0, 6, 40, 1, 7, 40, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 105, 1, 12, -1, 0, 10, 50, 2, 9, 50, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 106, 1, 9, -1, 0, 14, 50, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 107, 3, 6, -1, 0, -1, 0, 0, -1, 0, -9, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 108, 1, 310, -1, 0, -1, 0, 0, -1, 0, 0, 4, 0, 0, 0, 0, 0 },
    { NULL, NULL, 109, 1, 15, -1, 0, 10, 80, 0, -1, 0, 0, 0, 0, 0, 0, 8, 0 },
    { NULL, NULL, 110, 1, 6, -1, 0, 8, 60, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 111, 1, 12, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 10, 0, 0, 0 },
    { NULL, NULL, 112, 1, 310, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 8 },
    { NULL, NULL, 113, 1, 9, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 114, 1, 310, -1, 0, -1, 0, 0, -1, 0, 0, 0, 5, 0, 0, 0, 0 },
    { NULL, NULL, 115, 2, 6, -1, 0, 17, 40, 0, -1, 0, 0, 0, 6, 0, 0, 0, 0 },
    { NULL, NULL, 116, 1, 310, -1, 0, 17, 25, 0, -1, 0, 0, 0, 0, 0, 5, 0, 0 },
    { NULL, NULL, 117, 1, 3, -1, 0, -1, 0, 0, -1, 0, 0, 7, 0, 0, 0, 0, 0 },
    { NULL, NULL, 118, 1, 9, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 4 },
    { NULL, NULL, 119, 1, 6, -1, 0, -1, 0, 0, -1, 0, 0, 6, 0, 0, 0, 0, 0 },
    { NULL, NULL, 120, 1, 3, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 5, 0 },
    { NULL, NULL, 121, 3, 3, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 4, 0, 0 },
    { NULL, NULL, 122, 3, 3, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 4, 0, 0 },
    { NULL, NULL, 123, 1, 12, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 124, 1, 9, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 125, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 126, -1, 1, -1, 0, -1, 0, 0, -1, 0, -2, 0, -2, 0, 0, -3, 0 },
    { NULL, NULL, 127, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -3, -2, 0 },
    { NULL, NULL, 128, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -2, 0, 0 },
    { NULL, NULL, 129, -1, 1, 31, -20, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 130, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 131, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 132, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 133, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 134, -1, 1, 31, 30, -1, 0, 0, -1, 0, 3, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 135, -1, 1, 31, 20, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 136, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 137, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 138, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 139, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 140, -1, 1, 31, 60, -1, 0, 0, -1, 0, 4, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 141, -1, 1, 31, 75, -1, 0, 0, -1, 0, 4, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 136, -1, 1, 8, -1, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0 },
    { NULL, NULL, 149, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, -2, 0, 0, -1, 0, -1 },
    { NULL, NULL, 154, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 2, 0, 0, 0 },
    { NULL, NULL, 158, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 157, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 157, -1, 1, 3, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 168, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 168, -1, 1, 3, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 172, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 155, 1, 6, -1, 0, -1, 0, 0, -1, 0, -10, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 156, 1, 3, -1, 0, -1, 0, 0, -1, 0, 0, 6, 0, 0, 0, 0, 0 },
    { NULL, NULL, 122, 1, 3, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 6, 0, 0 },
    { NULL, NULL, 39, 1, 9, -1, 0, 11, 75, 0, -1, 0, 0, 0, 0, 0, 0, 4, 0 },
    { NULL, NULL, 44, 1, 6, -1, 0, 16, 50, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 0, 1, 12, -1, 0, -1, 0, 0, -1, 0, -10, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 1, 1, 12, -1, 0, -1, 0, 0, -1, 0, 0, -10, 0, 0, 0, 0, 0 },
    { NULL, NULL, 2, 1, 12, -1, 0, -1, 0, 0, -1, 0, 0, 0, -10, 0, 0, 0, 0 },
    { NULL, NULL, 3, 1, 12, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -10, 0, 0, 0 },
    { NULL, NULL, 4, 1, 12, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -10, 0, 0 },
    { NULL, NULL, 5, 1, 12, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -10, 0 },
    { NULL, NULL, 6, 1, 12, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -10 },
    { NULL, NULL, 160, 1, 6, -1, 0, 10, 50, 2, 0x4000000, 50, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 161, 1, 3, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 159, 1, 12, -1, 0, 3, 75, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 163, 1, 3, -1, 0, -1, 0, 0, -1, 0, 0, 0, 5, 0, 0, 5, 0 },
    { NULL, NULL, 162, 1, 9, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 6, 0, 0, 0 },
    { NULL, NULL, 164, 1, 9, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 5, 5 },
    { NULL, NULL, 165, 1, 12, -1, 0, 7, 60, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 166, 1, 6, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, -10, 0, 0, 0 },
    { NULL, NULL, 43, 1, 6, -1, 0, 15, 50, 2, 14, 50, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 167, 1, 6, 12, 50, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 169, 1, 9, -1, 0, 1, 75, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 170, 1, 6, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 5, 0 },
    { NULL, NULL, 121, 1, 6, -1, 0, 15, 50, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 171, 1, 3, -1, 0, -1, 0, 0, -1, 0, 6, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 38, 1, 3, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 173, 1, 12, -1, 0, -1, 0, 0, -1, 0, -7, 0, 0, 0, 0, 5, 0 },
    { NULL, NULL, 104, -1, 1, -1, 0, 7, 75, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 142, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 142, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 52, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 52, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 104, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 104, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 35, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 35, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 154, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 154, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { NULL, NULL, 64, -1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
};

// An array of perk ranks for each party member.
//
// 0x51C120
static PerkRankData* perkLevelDataList = NULL;

// Amount of experience points granted when player selected "Here and now"
// perk.
//
// 0x51C124
static int hereAndNowExps = 0;

// perk.msg
//
// 0x6642D4
static MessageList perk_message_file;

// 0x4965A0
int perk_init()
{
    perkLevelDataList = (PerkRankData*)mem_malloc(sizeof(*perkLevelDataList) * partyMemberMaxCount);
    if (perkLevelDataList == NULL) {
        return -1;
    }

    perk_defaults();

    if (!message_init(&perk_message_file)) {
        return -1;
    }

    char path[MAX_PATH];
    sprintf(path, "%s%s", msg_path, "perk.msg");

    if (!message_load(&perk_message_file, path)) {
        return -1;
    }

    for (int perk = 0; perk < PERK_COUNT; perk++) {
        MessageListItem messageListItem;

        messageListItem.num = 101 + perk;
        if (message_search(&perk_message_file, &messageListItem)) {
            perk_data[perk].name = messageListItem.text;
        }

        messageListItem.num = 1101 + perk;
        if (message_search(&perk_message_file, &messageListItem)) {
            perk_data[perk].description = messageListItem.text;
        }
    }

    return 0;
}

// 0x4966B0
void perk_reset()
{
    perk_defaults();
}

// 0x4966B8
void perk_exit()
{
    message_exit(&perk_message_file);

    if (perkLevelDataList != NULL) {
        mem_free(perkLevelDataList);
        perkLevelDataList = NULL;
    }
}

// 0x4966E4
int perk_load(File* stream)
{
    for (int index = 0; index < partyMemberMaxCount; index++) {
        PerkRankData* ranksData = &(perkLevelDataList[index]);
        for (int perk = 0; perk < PERK_COUNT; perk++) {
            if (db_freadInt(stream, &(ranksData->ranks[perk])) == -1) {
                return -1;
            }
        }
    }

    return 0;
}

// 0x496738
int perk_save(File* stream)
{
    for (int index = 0; index < partyMemberMaxCount; index++) {
        PerkRankData* ranksData = &(perkLevelDataList[index]);
        for (int perk = 0; perk < PERK_COUNT; perk++) {
            if (db_fwriteInt(stream, ranksData->ranks[perk]) == -1) {
                return -1;
            }
        }
    }

    return 0;
}

// perkGetLevelData
// 0x49678C
PerkRankData* perkGetLevelData(Object* critter)
{
    if (critter == obj_dude) {
        return perkLevelDataList;
    }

    for (int index = 1; index < partyMemberMaxCount; index++) {
        if (critter->pid == partyMemberPidList[index]) {
            return perkLevelDataList + index;
        }
    }

    debug_printf("\nError: perkGetLevelData: Can't find party member match!");

    return perkLevelDataList;
}

// 0x49680C
static bool perk_can_add(Object* critter, int perk)
{
    if (!perkIsValid(perk)) {
        return false;
    }

    PerkDescription* perkDescription = &(perk_data[perk]);

    if (perkDescription->maxRank == -1) {
        return false;
    }

    PerkRankData* ranksData = perkGetLevelData(critter);
    if (ranksData->ranks[perk] >= perkDescription->maxRank) {
        return false;
    }

    if (critter == obj_dude) {
        if (stat_pc_get(PC_STAT_LEVEL) < perkDescription->minLevel) {
            return false;
        }
    }

    bool v1 = true;

    int param1 = perkDescription->param1;
    if (param1 != -1) {
        bool isVariable = false;
        if ((param1 & 0x4000000) != 0) {
            isVariable = true;
            param1 &= ~0x4000000;
        }

        int value1 = perkDescription->value1;
        if (value1 < 0) {
            if (isVariable) {
                if (game_get_global_var(param1) >= value1) {
                    v1 = false;
                }
            } else {
                if (skill_level(critter, param1) >= -value1) {
                    v1 = false;
                }
            }
        } else {
            if (isVariable) {
                if (game_get_global_var(param1) < value1) {
                    v1 = false;
                }
            } else {
                if (skill_level(critter, param1) < value1) {
                    v1 = false;
                }
            }
        }
    }

    if (!v1 || perkDescription->field_24 == 2) {
        if (perkDescription->field_24 == 0) {
            return false;
        }

        if (!v1 && perkDescription->field_24 == 2) {
            return false;
        }

        int param2 = perkDescription->param2;
        bool isVariable = false;
        if (param2 != -1) {
            if ((param2 & 0x4000000) != 0) {
                isVariable = true;
                param2 &= ~0x4000000;
            }
        }

        if (param2 == -1) {
            return false;
        }

        int value2 = perkDescription->value2;
        if (value2 < 0) {
            if (isVariable) {
                if (game_get_global_var(param2) >= value2) {
                    return false;
                }
            } else {
                if (skill_level(critter, param2) >= -value2) {
                    return false;
                }
            }
        } else {
            if (isVariable) {
                if (game_get_global_var(param2) < value2) {
                    return false;
                }
            } else {
                if (skill_level(critter, param2) < value2) {
                    return false;
                }
            }
        }
    }

    for (int stat = 0; stat < PRIMARY_STAT_COUNT; stat++) {
        if (perkDescription->stats[stat] < 0) {
            if (critterGetStat(critter, stat) >= -perkDescription->stats[stat]) {
                return false;
            }
        } else {
            if (critterGetStat(critter, stat) < perkDescription->stats[stat]) {
                return false;
            }
        }
    }

    return true;
}

// Resets party member perks.
//
// 0x496A0C
static void perk_defaults()
{
    for (int index = 0; index < partyMemberMaxCount; index++) {
        PerkRankData* ranksData = &(perkLevelDataList[index]);
        for (int perk = 0; perk < PERK_COUNT; perk++) {
            ranksData->ranks[perk] = 0;
        }
    }
}

// 0x496A5C
int perk_add(Object* critter, int perk)
{
    if (!perkIsValid(perk)) {
        return -1;
    }

    if (!perk_can_add(critter, perk)) {
        return -1;
    }

    PerkRankData* ranksData = perkGetLevelData(critter);
    ranksData->ranks[perk] += 1;

    perk_add_effect(critter, perk);

    return 0;
}

// perk_add_force
// 0x496A9C
int perk_add_force(Object* critter, int perk)
{
    if (!perkIsValid(perk)) {
        return -1;
    }

    PerkRankData* ranksData = perkGetLevelData(critter);
    int value = ranksData->ranks[perk];

    int maxRank = perk_data[perk].maxRank;

    if (maxRank != -1 && value >= maxRank) {
        return -1;
    }

    ranksData->ranks[perk] += 1;

    perk_add_effect(critter, perk);

    return 0;
}

// perk_sub
// 0x496AFC
int perk_sub(Object* critter, int perk)
{
    if (!perkIsValid(perk)) {
        return -1;
    }

    PerkRankData* ranksData = perkGetLevelData(critter);
    int value = ranksData->ranks[perk];

    if (value < 1) {
        return -1;
    }

    ranksData->ranks[perk] -= 1;

    perk_remove_effect(critter, perk);

    return 0;
}

// Returns perks available to pick.
//
// 0x496B44
int perk_make_list(Object* critter, int* perks)
{
    int count = 0;
    for (int perk = 0; perk < PERK_COUNT; perk++) {
        if (perk_can_add(critter, perk)) {
            perks[count] = perk;
            count++;
        }
    }
    return count;
}

// has_perk
// 0x496B78
int perk_level(Object* critter, int perk)
{
    if (!perkIsValid(perk)) {
        return 0;
    }

    PerkRankData* ranksData = perkGetLevelData(critter);
    return ranksData->ranks[perk];
}

// 0x496B90
char* perk_name(int perk)
{
    if (!perkIsValid(perk)) {
        return NULL;
    }
    return perk_data[perk].name;
}

// 0x496BB4
char* perk_description(int perk)
{
    if (!perkIsValid(perk)) {
        return NULL;
    }
    return perk_data[perk].description;
}

// 0x496BD8
int perk_skilldex_fid(int perk)
{
    if (!perkIsValid(perk)) {
        return 0;
    }
    return perk_data[perk].frmId;
}

// perk_add_effect
// 0x496BFC
void perk_add_effect(Object* critter, int perk)
{
    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        debug_printf("\nERROR: perk_add_effect: Was called on non-critter!");
        return;
    }

    if (!perkIsValid(perk)) {
        return;
    }

    PerkDescription* perkDescription = &(perk_data[perk]);

    if (perkDescription->stat != -1) {
        int value = stat_get_bonus(critter, perkDescription->stat);
        stat_set_bonus(critter, perkDescription->stat, value + perkDescription->statModifier);
    }

    if (perk == PERK_HERE_AND_NOW) {
        PerkRankData* ranksData = perkGetLevelData(critter);
        ranksData->ranks[PERK_HERE_AND_NOW] -= 1;

        int level = stat_pc_get(PC_STAT_LEVEL);

        hereAndNowExps = statPcMinExpForLevel(level + 1) - stat_pc_get(PC_STAT_EXPERIENCE);
        statPCAddExperienceCheckPMs(hereAndNowExps, false);

        ranksData->ranks[PERK_HERE_AND_NOW] += 1;
    }

    if (perkDescription->maxRank == -1) {
        for (int stat = 0; stat < PRIMARY_STAT_COUNT; stat++) {
            int value = stat_get_bonus(critter, stat);
            stat_set_bonus(critter, stat, value + perkDescription->stats[stat]);
        }
    }
}

// perk_remove_effect
// 0x496CE0
void perk_remove_effect(Object* critter, int perk)
{
    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        debug_printf("\nERROR: perk_remove_effect: Was called on non-critter!");
        return;
    }

    if (!perkIsValid(perk)) {
        return;
    }

    PerkDescription* perkDescription = &(perk_data[perk]);

    if (perkDescription->stat != -1) {
        int value = stat_get_bonus(critter, perkDescription->stat);
        stat_set_bonus(critter, perkDescription->stat, value - perkDescription->statModifier);
    }

    if (perk == PERK_HERE_AND_NOW) {
        int xp = stat_pc_get(PC_STAT_EXPERIENCE);
        stat_pc_set(PC_STAT_EXPERIENCE, xp - hereAndNowExps);
    }

    if (perkDescription->maxRank == -1) {
        for (int stat = 0; stat < PRIMARY_STAT_COUNT; stat++) {
            int value = stat_get_bonus(critter, stat);
            stat_set_bonus(critter, stat, value - perkDescription->stats[stat]);
        }
    }
}

// Returns modifier to specified skill accounting for perks.
//
// 0x496DD0
int perk_adjust_skill(Object* critter, int skill)
{
    int modifier = 0;

    switch (skill) {
    case SKILL_FIRST_AID:
        if (perkHasRank(critter, PERK_MEDIC)) {
            modifier += 10;
        }

        if (perkHasRank(critter, PERK_VAULT_CITY_TRAINING)) {
            modifier += 5;
        }

        break;
    case SKILL_DOCTOR:
        if (perkHasRank(critter, PERK_MEDIC)) {
            modifier += 10;
        }

        if (perkHasRank(critter, PERK_LIVING_ANATOMY)) {
            modifier += 10;
        }

        if (perkHasRank(critter, PERK_VAULT_CITY_TRAINING)) {
            modifier += 5;
        }

        break;
    case SKILL_SNEAK:
        if (perkHasRank(critter, PERK_GHOST)) {
            int lightIntensity = obj_get_visible_light(obj_dude);
            if (lightIntensity > 45875) {
                modifier += 20;
            }
        }
        // FALLTHROUGH
    case SKILL_LOCKPICK:
    case SKILL_STEAL:
    case SKILL_TRAPS:
        if (perkHasRank(critter, PERK_THIEF)) {
            modifier += 10;
        }

        if (skill == SKILL_LOCKPICK || skill == SKILL_STEAL) {
            if (perkHasRank(critter, PERK_MASTER_THIEF)) {
                modifier += 15;
            }
        }

        if (skill == SKILL_STEAL) {
            if (perkHasRank(critter, PERK_HARMLESS)) {
                modifier += 20;
            }
        }

        break;
    case SKILL_SCIENCE:
    case SKILL_REPAIR:
        if (perkHasRank(critter, PERK_MR_FIXIT)) {
            modifier += 10;
        }

        break;
    case SKILL_SPEECH:
        if (perkHasRank(critter, PERK_SPEAKER)) {
            modifier += 20;
        }

        if (perkHasRank(critter, PERK_EXPERT_EXCREMENT_EXPEDITOR)) {
            modifier += 5;
        }

        // FALLTHROUGH
    case SKILL_BARTER:
        if (perkHasRank(critter, PERK_NEGOTIATOR)) {
            modifier += 10;
        }

        if (skill == SKILL_BARTER) {
            if (perkHasRank(critter, PERK_SALESMAN)) {
                modifier += 20;
            }
        }

        break;
    case SKILL_GAMBLING:
        if (perkHasRank(critter, PERK_GAMBLER)) {
            modifier += 20;
        }

        break;
    case SKILL_OUTDOORSMAN:
        if (perkHasRank(critter, PERK_RANGER)) {
            modifier += 15;
        }

        if (perkHasRank(critter, PERK_SURVIVALIST)) {
            modifier += 25;
        }

        break;
    }

    return modifier;
}
