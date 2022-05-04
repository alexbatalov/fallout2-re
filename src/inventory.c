#include "inventory.h"

#include "actions.h"
#include "animation.h"
#include "art.h"
#include "color.h"
#include "combat.h"
#include "combat_ai.h"
#include "core.h"
#include "critter.h"
#include "dbox.h"
#include "debug.h"
#include "dialog.h"
#include "display_monitor.h"
#include "draw.h"
#include "game.h"
#include "game_dialog.h"
#include "game_mouse.h"
#include "game_sound.h"
#include "interface.h"
#include "item.h"
#include "light.h"
#include "map.h"
#include "object.h"
#include "perk.h"
#include "proto.h"
#include "proto_instance.h"
#include "random.h"
#include "reaction.h"
#include "scripts.h"
#include "skill.h"
#include "stat.h"
#include "text_font.h"
#include "tile.h"
#include "window_manager.h"

#include <intrin.h>
#include <stdio.h>

// 0x46E6D0
const int dword_46E6D0[7] = {
    STAT_CURRENT_HIT_POINTS,
    STAT_ARMOR_CLASS,
    STAT_DAMAGE_THRESHOLD,
    STAT_DAMAGE_THRESHOLD_LASER,
    STAT_DAMAGE_THRESHOLD_FIRE,
    STAT_DAMAGE_THRESHOLD_PLASMA,
    STAT_DAMAGE_THRESHOLD_EXPLOSION,
};

// 0x46E6EC
const int dword_46E6EC[7] = {
    STAT_MAXIMUM_HIT_POINTS,
    -1,
    STAT_DAMAGE_RESISTANCE,
    STAT_DAMAGE_RESISTANCE_LASER,
    STAT_DAMAGE_RESISTANCE_FIRE,
    STAT_DAMAGE_RESISTANCE_PLASMA,
    STAT_DAMAGE_RESISTANCE_EXPLOSION,
};

// 0x46E708
const int gInventoryArrowFrmIds[INVENTORY_ARROW_FRM_COUNT] = {
    122, // left arrow up
    123, // left arrow down
    124, // right arrow up
    125, // right arrow down
};

// The number of items to show in scroller.
//
// 0x519054
int gInventorySlotsCount = 6;

// 0x519058
Object* off_519058 = NULL;

// Probably fid of armor to display in inventory dialog.
//
// 0x51905C
int dword_51905C = -1;

// 0x519060
bool dword_519060 = false;

// 0x519064
int dword_519064 = 1;

// 0x519068
const InventoryWindowDescription gInventoryWindowDescriptions[INVENTORY_WINDOW_TYPE_COUNT] = {
    { 48, 499, 377, 80, 0 },
    { 113, 292, 376, 80, 0 },
    { 114, 537, 376, 80, 0 },
    { 111, 480, 180, 80, 290 },
    { 305, 259, 162, 140, 80 },
    { 305, 259, 162, 140, 80 },
};

// 0x5190E0
bool dword_5190E0 = false;

// 0x5190E4
int gInventoryScrollUpButton = -1;

// 0x5190E8
int gInventoryScrollDownButton = -1;

// 0x5190EC
int gSecondaryInventoryScrollUpButton = -1;

// 0x5190F0
int gSecondaryInventoryScrollDownButton = -1;

// 0x5190F4
unsigned int gInventoryWindowDudeRotationTimestamp = 0;

// 0x5190F8
int gInventoryWindowDudeRotation = 0;

// 0x5190FC
const int gInventoryWindowCursorFrmIds[INVENTORY_WINDOW_CURSOR_COUNT] = {
    286, // pointing hand
    250, // action arrow
    282, // action pick
    283, // action menu
    266, // blank
};

// 0x519110
Object* off_519110 = NULL;

// 0x519114
const int dword_519114[4] = {
    GAME_MOUSE_ACTION_MENU_ITEM_LOOK,
    GAME_MOUSE_ACTION_MENU_ITEM_USE,
    GAME_MOUSE_ACTION_MENU_ITEM_DROP,
    GAME_MOUSE_ACTION_MENU_ITEM_CANCEL,
};

// 0x519124
const int dword_519124[3] = {
    GAME_MOUSE_ACTION_MENU_ITEM_LOOK,
    GAME_MOUSE_ACTION_MENU_ITEM_DROP,
    GAME_MOUSE_ACTION_MENU_ITEM_CANCEL,
};

// 0x519130
const int dword_519130[3] = {
    GAME_MOUSE_ACTION_MENU_ITEM_LOOK,
    GAME_MOUSE_ACTION_MENU_ITEM_USE,
    GAME_MOUSE_ACTION_MENU_ITEM_CANCEL,
};

// 0x51913C
const int dword_51913C[2] = {
    GAME_MOUSE_ACTION_MENU_ITEM_LOOK,
    GAME_MOUSE_ACTION_MENU_ITEM_CANCEL,
};

// 0x519144
const int dword_519144[4] = {
    GAME_MOUSE_ACTION_MENU_ITEM_LOOK,
    GAME_MOUSE_ACTION_MENU_ITEM_UNLOAD,
    GAME_MOUSE_ACTION_MENU_ITEM_DROP,
    GAME_MOUSE_ACTION_MENU_ITEM_CANCEL,
};

// 0x519154
const int dword_519154[3] = {
    GAME_MOUSE_ACTION_MENU_ITEM_LOOK,
    GAME_MOUSE_ACTION_MENU_ITEM_UNLOAD,
    GAME_MOUSE_ACTION_MENU_ITEM_CANCEL,
};

// 0x59E79C
CacheEntry* off_59E79C[8];

// 0x59E7BC
CacheEntry* off_59E7BC[OFF_59E7BC_COUNT];

// 0x59E7EC
int dword_59E7EC[10];

// inventory.msg
//
// 0x59E814
MessageList gInventoryMessageList;

// 0x59E81C
Object* off_59E81C[10];

// 0x59E844
int dword_59E844[10];

// 0x59E86C
Object* off_59E86C[10];

// 0x59E894
int dword_59E894;

// 0x59E898
int dword_59E898;

// 0x59E89C
int dword_59E89C;

// 0x59E8A0
int dword_59E8A0;

// 0x59E8A4
Inventory* off_59E8A4;

// 0x59E8A8
InventoryCursorData gInventoryCursorData[INVENTORY_WINDOW_CURSOR_COUNT];

// 0x59E934
Object* off_59E934;

// 0x59E938
InventoryPrintItemDescriptionHandler* gInventoryPrintItemDescriptionHandler;

// 0x59E93C
int dword_59E93C;

// 0x59E940
int gInventoryCursor;

// 0x59E944
Object* off_59E944;

// 0x59E948
int dword_59E948;

// 0x59E94C
Inventory* off_59E94C;

// 0x59E950
bool dword_59E950;

// 0x59E954
Object* gInventoryArmor;

// 0x59E958
Object* gInventoryLeftHandItem;

// Rotating character's fid.
//
// 0x59E95C
int gInventoryWindowDudeFid;

// 0x59E960
Inventory* off_59E960;

// 0x59E964
int gInventoryWindow;

// item2
// 0x59E968
Object* gInventoryRightHandItem;

// 0x59E96C
int dword_59E96C;

// 0x59E970
int gInventoryWindowMaxY;

// 0x59E974
int gInventoryWindowMaxX;

// 0x59E978
Inventory* off_59E978;

// 0x59E97C
int dword_59E97C;

// 0x46E724
void sub_46E724()
{
    off_519058 = gDude;
    dword_51905C = 0x1000000;
}

// inventory_msg_init
// 0x46E73C
int inventoryMessageListInit()
{
    char path[MAX_PATH];

    if (!messageListInit(&gInventoryMessageList))
        return -1;

    sprintf(path, "%s%s", asc_5186C8, "inventry.msg");
    if (!messageListLoad(&gInventoryMessageList, path))
        return -1;

    return 0;
}

// inventory_msg_free
// 0x46E7A0
int inventoryMessageListFree()
{
    messageListFree(&gInventoryMessageList);
    return 0;
}

// 0x46E7B0
void inventoryOpen()
{
    if (isInCombat()) {
        if (sub_4217D4() != off_519058) {
            return;
        }
    }

    if (inventoryCommonInit() == -1) {
        return;
    }

    if (isInCombat()) {
        if (off_519058 == gDude) {
            int actionPointsRequired = 4 - 2 * perkGetRank(off_519058, PERK_QUICK_POCKETS);
            if (actionPointsRequired > 0 && actionPointsRequired > gDude->data.critter.combat.ap) {
                // You don't have enough action points to use inventory
                MessageListItem messageListItem;
                messageListItem.num = 19;
                if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                    displayMonitorAddMessage(messageListItem.text);
                }

                // NOTE: Uninline.
                inventoryCommonFree();

                return;
            }

            if (actionPointsRequired > 0) {
                if (actionPointsRequired > gDude->data.critter.combat.ap) {
                    gDude->data.critter.combat.ap = 0;
                } else {
                    gDude->data.critter.combat.ap -= actionPointsRequired;
                }
                interfaceRenderActionPoints(gDude->data.critter.combat.ap, dword_56D39C);
            }
        }
    }

    Object* oldArmor = critterGetArmor(off_519058);
    bool isoWasEnabled = sub_46EC90(INVENTORY_WINDOW_TYPE_NORMAL);
    reg_anim_clear(off_519058);
    inventoryRenderSummary();
    sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_NORMAL);
    inventorySetCursor(INVENTORY_WINDOW_CURSOR_HAND);

    for (;;) {
        int keyCode = sub_4C8B78();

        if (keyCode == KEY_ESCAPE) {
            break;
        }

        if (dword_5186CC != 0) {
            break;
        }

        sub_470650(-1, INVENTORY_WINDOW_TYPE_NORMAL);

        if (sub_443E2C() == 5) {
            break;
        }

        if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X) {
            showQuitConfirmationDialog();
        } else if (keyCode == KEY_HOME) {
            dword_59E844[dword_59E96C] = 0;
            sub_46FDF4(0, -1, INVENTORY_WINDOW_TYPE_NORMAL);
        } else if (keyCode == KEY_ARROW_UP) {
            if (dword_59E844[dword_59E96C] > 0) {
                dword_59E844[dword_59E96C] -= 1;
                sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_NORMAL);
            }
        } else if (keyCode == KEY_PAGE_UP) {
            dword_59E844[dword_59E96C] -= gInventorySlotsCount;
            if (dword_59E844[dword_59E96C] < 0) {
                dword_59E844[dword_59E96C] = 0;
            }
            sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_NORMAL);
        } else if (keyCode == KEY_END) {
            dword_59E844[dword_59E96C] = off_59E960->length - gInventorySlotsCount;
            if (dword_59E844[dword_59E96C] < 0) {
                dword_59E844[dword_59E96C] = 0;
            }
            sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_NORMAL);
        } else if (keyCode == KEY_ARROW_DOWN) {
            if (gInventorySlotsCount + dword_59E844[dword_59E96C] < off_59E960->length) {
                dword_59E844[dword_59E96C] += 1;
                sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_NORMAL);
            }
        } else if (keyCode == KEY_PAGE_DOWN) {
            int v12 = gInventorySlotsCount + dword_59E844[dword_59E96C];
            int v13 = v12 + gInventorySlotsCount;
            dword_59E844[dword_59E96C] = v12;
            int v14 = off_59E960->length;
            if (v13 >= off_59E960->length) {
                int v15 = v14 - gInventorySlotsCount;
                dword_59E844[dword_59E96C] = v14 - gInventorySlotsCount;
                if (v15 < 0) {
                    dword_59E844[dword_59E96C] = 0;
                }
            }
            sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_NORMAL);
        } else if (keyCode == 2500) {
            sub_476394(keyCode, INVENTORY_WINDOW_TYPE_NORMAL);
        } else {
            if ((mouseGetEvent() & MOUSE_EVENT_RIGHT_BUTTON_DOWN) != 0) {
                if (gInventoryCursor == INVENTORY_WINDOW_CURSOR_HAND) {
                    inventorySetCursor(INVENTORY_WINDOW_CURSOR_ARROW);
                } else if (gInventoryCursor == INVENTORY_WINDOW_CURSOR_ARROW) {
                    inventorySetCursor(INVENTORY_WINDOW_CURSOR_HAND);
                    inventoryRenderSummary();
                    windowRefresh(gInventoryWindow);
                }
            } else if ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_DOWN) != 0) {
                if (keyCode >= 1000 && keyCode <= 1008) {
                    if (gInventoryCursor == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inventoryWindowOpenContextMenu(keyCode, INVENTORY_WINDOW_TYPE_NORMAL);
                    } else {
                        sub_470DB8(keyCode, dword_59E844[dword_59E96C]);
                    }
                }
            }
        }
    }

    off_519058 = off_59E86C[0];
    sub_4716E8();

    if (off_519058 == gDude) {
        Rect rect;
        objectSetFid(off_519058, gInventoryWindowDudeFid, &rect);
        tileWindowRefreshRect(&rect, off_519058->elevation);
    }

    Object* newArmor = critterGetArmor(off_519058);
    if (off_519058 == gDude) {
        if (oldArmor != newArmor) {
            interfaceRenderArmorClass(true);
        }
    }

    sub_46FBD8(isoWasEnabled);

    // NOTE: Uninline.
    inventoryCommonFree();

    if (off_519058 == gDude) {
        sub_45EFEC(false, -1, -1);
    }
}

// 0x46EC90
bool sub_46EC90(int inventoryWindowType)
{
    dword_5190E0 = 0;
    dword_59E96C = 0;
    dword_59E844[0] = 0;
    gInventorySlotsCount = 6;
    off_59E960 = &(off_519058->data.inventory);
    off_59E86C[0] = off_519058;

    if (inventoryWindowType <= INVENTORY_WINDOW_TYPE_LOOT) {
        const InventoryWindowDescription* windowDescription = &(gInventoryWindowDescriptions[inventoryWindowType]);
        gInventoryWindow = windowCreate(80, 0, windowDescription->width, windowDescription->height, 257, 20);
        gInventoryWindowMaxX = windowDescription->width + 80;
        gInventoryWindowMaxY = windowDescription->height;

        unsigned char* dest = windowGetBuffer(gInventoryWindow);
        int backgroundFid = buildFid(6, windowDescription->field_0, 0, 0, 0);

        CacheEntry* backgroundFrmHandle;
        unsigned char* backgroundFrmData = artLockFrameData(backgroundFid, 0, 0, &backgroundFrmHandle);
        if (backgroundFrmData != NULL) {
            blitBufferToBuffer(backgroundFrmData, windowDescription->width, windowDescription->height, windowDescription->width, dest, windowDescription->width);
            artUnlock(backgroundFrmHandle);
        }

        gInventoryPrintItemDescriptionHandler = displayMonitorAddMessage;
    } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        if (dword_59E97C == -1) {
            exit(1);
        }

        gInventorySlotsCount = 3;

        gInventoryWindow = windowCreate(80, 290, 480, 180, 257, 0);
        gInventoryWindowMaxX = 560;
        gInventoryWindowMaxY = 470;

        unsigned char* dest = windowGetBuffer(gInventoryWindow);
        unsigned char* src = windowGetBuffer(dword_59E97C);
        blitBufferToBuffer(src + 80, 480, 180, stru_6AC9F0.right - stru_6AC9F0.left + 1, dest, 480);

        gInventoryPrintItemDescriptionHandler = gameDialogRenderSupplementaryMessage;
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
        // Create invsibile buttons representing character's inventory item
        // slots.
        for (int index = 0; index < gInventorySlotsCount; index++) {
            int btn = buttonCreate(gInventoryWindow, 176, 48 * (gInventorySlotsCount - index - 1) + 37, 64, 48, 999 + gInventorySlotsCount - index, -1, 999 + gInventorySlotsCount - index, -1, NULL, NULL, NULL, 0);
            if (btn != -1) {
                buttonSetMouseCallbacks(btn, inventoryItemSlotOnMouseEnter, inventoryItemSlotOnMouseExit, NULL, NULL);
            }
        }

        int eventCode = 2005;
        int y = 277;

        // Create invisible buttons representing container's inventory item
        // slots. For unknown reason it loops backwards and it's size is
        // hardcoded at 6 items.
        //
        // Original code is slightly different. It loops until y reaches -11,
        // which is a bit awkward for a loop. Probably result of some
        // optimization.
        for (int index = 0; index < 6; index++) {
            int btn = buttonCreate(gInventoryWindow, 297, y, 64, 48, eventCode, -1, eventCode, -1, NULL, NULL, NULL, 0);
            if (btn != -1) {
                buttonSetMouseCallbacks(btn, inventoryItemSlotOnMouseEnter, inventoryItemSlotOnMouseExit, NULL, NULL);
            }

            eventCode -= 1;
            y -= 48;
        }
    } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        int y1 = 35;
        int y2 = 20;

        for (int index = 0; index < gInventorySlotsCount; index++) {
            int btn;

            // Invsibile button representing left inventory slot.
            btn = buttonCreate(gInventoryWindow, 29, y1, 64, 48, 1000 + index, -1, 1000 + index, -1, NULL, NULL, NULL, 0);
            if (btn != -1) {
                buttonSetMouseCallbacks(btn, inventoryItemSlotOnMouseEnter, inventoryItemSlotOnMouseExit, NULL, NULL);
            }

            // Invisible button representing right inventory slot.
            btn = buttonCreate(gInventoryWindow, 395, y1, 64, 48, 2000 + index, -1, 2000 + index, -1, NULL, NULL, NULL, 0);
            if (btn != -1) {
                buttonSetMouseCallbacks(btn, inventoryItemSlotOnMouseEnter, inventoryItemSlotOnMouseExit, NULL, NULL);
            }

            // Invisible button representing left suggested slot.
            btn = buttonCreate(gInventoryWindow, 165, y2, 64, 48, 2300 + index, -1, 2300 + index, -1, NULL, NULL, NULL, 0);
            if (btn != -1) {
                buttonSetMouseCallbacks(btn, inventoryItemSlotOnMouseEnter, inventoryItemSlotOnMouseExit, NULL, NULL);
            }

            // Invisible button representing right suggested slot.
            btn = buttonCreate(gInventoryWindow, 250, y2, 64, 48, 2400 + index, -1, 2400 + index, -1, NULL, NULL, NULL, 0);
            if (btn != -1) {
                buttonSetMouseCallbacks(btn, inventoryItemSlotOnMouseEnter, inventoryItemSlotOnMouseExit, NULL, NULL);
            }

            y1 += 48;
            y2 += 48;
        }
    } else {
        // Create invisible buttons representing item slots.
        for (int index = 0; index < gInventorySlotsCount; index++) {
            int btn = buttonCreate(gInventoryWindow,
                44,
                48 * (gInventorySlotsCount - index - 1) + 35,
                64,
                48,
                999 + gInventorySlotsCount - index,
                -1,
                999 + gInventorySlotsCount - index,
                -1,
                NULL,
                NULL,
                NULL,
                0);
            if (btn != -1) {
                buttonSetMouseCallbacks(btn, inventoryItemSlotOnMouseEnter, inventoryItemSlotOnMouseExit, NULL, NULL);
            }
        }
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_NORMAL) {
        int btn;

        // Item2 slot
        btn = buttonCreate(gInventoryWindow, 245, 286, 90, 61, 1006, -1, 1006, -1, NULL, NULL, NULL, 0);
        if (btn != -1) {
            buttonSetMouseCallbacks(btn, inventoryItemSlotOnMouseEnter, inventoryItemSlotOnMouseExit, NULL, NULL);
        }

        // Item1 slot
        btn = buttonCreate(gInventoryWindow, 154, 286, 90, 61, 1007, -1, 1007, -1, NULL, NULL, NULL, 0);
        if (btn != -1) {
            buttonSetMouseCallbacks(btn, inventoryItemSlotOnMouseEnter, inventoryItemSlotOnMouseExit, NULL, NULL);
        }

        // Armor slot
        btn = buttonCreate(gInventoryWindow, 154, 183, 90, 61, 1008, -1, 1008, -1, NULL, NULL, NULL, 0);
        if (btn != -1) {
            buttonSetMouseCallbacks(btn, inventoryItemSlotOnMouseEnter, inventoryItemSlotOnMouseExit, NULL, NULL);
        }
    }

    memset(off_59E7BC, 0, sizeof(off_59E7BC));

    int fid;
    int btn;
    unsigned char* buttonUpData;
    unsigned char* buttonDownData;
    unsigned char* buttonDisabledData;

    fid = buildFid(6, 8, 0, 0, 0);
    buttonUpData = artLockFrameData(fid, 0, 0, &(off_59E7BC[0]));

    fid = buildFid(6, 9, 0, 0, 0);
    buttonDownData = artLockFrameData(fid, 0, 0, &(off_59E7BC[1]));

    if (buttonUpData != NULL && buttonDownData != NULL) {
        btn = -1;
        switch (inventoryWindowType) {
        case INVENTORY_WINDOW_TYPE_NORMAL:
            // Done button
            btn = buttonCreate(gInventoryWindow, 437, 329, 15, 16, -1, -1, -1, KEY_ESCAPE, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
            break;
        case INVENTORY_WINDOW_TYPE_USE_ITEM_ON:
            // Cancel button
            btn = buttonCreate(gInventoryWindow, 233, 328, 15, 16, -1, -1, -1, KEY_ESCAPE, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
            break;
        case INVENTORY_WINDOW_TYPE_LOOT:
            // Done button
            btn = buttonCreate(gInventoryWindow, 476, 331, 15, 16, -1, -1, -1, KEY_ESCAPE, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
            break;
        }

        if (btn != -1) {
            buttonSetCallbacks(btn, sub_451970, sub_451978);
        }
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        // TODO: Figure out why it building fid in chain.

        // Large arrow up (normal).
        fid = buildFid(6, 100, 0, 0, 0);
        fid = buildFid(6, fid, 0, 0, 0);
        buttonUpData = artLockFrameData(fid, 0, 0, &(off_59E7BC[2]));

        // Large arrow up (pressed).
        fid = buildFid(6, 101, 0, 0, 0);
        fid = buildFid(6, fid, 0, 0, 0);
        buttonDownData = artLockFrameData(fid, 0, 0, &(off_59E7BC[3]));

        if (buttonUpData != NULL && buttonDownData != NULL) {
            // Left inventory up button.
            btn = buttonCreate(gInventoryWindow, 109, 56, 23, 24, -1, -1, KEY_ARROW_UP, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, sub_451970, sub_451978);
            }

            // Right inventory up button.
            btn = buttonCreate(gInventoryWindow, 342, 56, 23, 24, -1, -1, KEY_CTRL_ARROW_UP, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, sub_451970, sub_451978);
            }
        }
    } else {
        // Large up arrow (normal).
        fid = buildFid(6, 49, 0, 0, 0);
        buttonUpData = artLockFrameData(fid, 0, 0, &(off_59E7BC[2]));

        // Large up arrow (pressed).
        fid = buildFid(6, 50, 0, 0, 0);
        buttonDownData = artLockFrameData(fid, 0, 0, &(off_59E7BC[3]));

        // Large up arrow (disabled).
        fid = buildFid(6, 53, 0, 0, 0);
        buttonDisabledData = artLockFrameData(fid, 0, 0, &(off_59E7BC[4]));

        if (buttonUpData != NULL && buttonDownData != NULL && buttonDisabledData != NULL) {
            if (inventoryWindowType != INVENTORY_WINDOW_TYPE_TRADE) {
                // Left inventory up button.
                gInventoryScrollUpButton = buttonCreate(gInventoryWindow, 128, 39, 22, 23, -1, -1, KEY_ARROW_UP, -1, buttonUpData, buttonDownData, NULL, 0);
                if (gInventoryScrollUpButton != -1) {
                    sub_4D8674(gInventoryScrollUpButton, buttonDisabledData, buttonDisabledData, buttonDisabledData);
                    buttonSetCallbacks(gInventoryScrollUpButton, sub_451970, sub_451978);
                    buttonDisable(gInventoryScrollUpButton);
                }
            }

            if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
                // Right inventory up button.
                gSecondaryInventoryScrollUpButton = buttonCreate(gInventoryWindow, 379, 39, 22, 23, -1, -1, KEY_CTRL_ARROW_UP, -1, buttonUpData, buttonDownData, NULL, 0);
                if (gSecondaryInventoryScrollUpButton != -1) {
                    sub_4D8674(gSecondaryInventoryScrollUpButton, buttonDisabledData, buttonDisabledData, buttonDisabledData);
                    buttonSetCallbacks(gSecondaryInventoryScrollUpButton, sub_451970, sub_451978);
                    buttonDisable(gSecondaryInventoryScrollUpButton);
                }
            }
        }
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        // Large dialog down button (normal)
        fid = buildFid(6, 93, 0, 0, 0);
        fid = buildFid(6, fid, 0, 0, 0);
        buttonUpData = artLockFrameData(fid, 0, 0, &(off_59E7BC[5]));

        // Dialog down button (pressed)
        fid = buildFid(6, 94, 0, 0, 0);
        fid = buildFid(6, fid, 0, 0, 0);
        buttonDownData = artLockFrameData(fid, 0, 0, &(off_59E7BC[6]));

        if (buttonUpData != NULL && buttonDownData != NULL) {
            // Left inventory down button.
            btn = buttonCreate(gInventoryWindow, 109, 82, 24, 25, -1, -1, KEY_ARROW_DOWN, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, sub_451970, sub_451978);
            }

            // Right inventory down button
            btn = buttonCreate(gInventoryWindow, 342, 82, 24, 25, -1, -1, KEY_CTRL_ARROW_DOWN, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, sub_451970, sub_451978);
            }

            // Invisible button representing left character.
            buttonCreate(dword_59E97C, 15, 25, 60, 100, -1, -1, 2500, -1, NULL, NULL, NULL, 0);

            // Invisible button representing right character.
            buttonCreate(dword_59E97C, 560, 25, 60, 100, -1, -1, 2501, -1, NULL, NULL, NULL, 0);
        }
    } else {
        // Large arrow down (normal).
        fid = buildFid(6, 51, 0, 0, 0);
        buttonUpData = artLockFrameData(fid, 0, 0, &(off_59E7BC[5]));

        // Large arrow down (pressed).
        fid = buildFid(6, 52, 0, 0, 0);
        buttonDownData = artLockFrameData(fid, 0, 0, &(off_59E7BC[6]));

        // Large arrow down (disabled).
        fid = buildFid(6, 54, 0, 0, 0);
        buttonDisabledData = artLockFrameData(fid, 0, 0, &(off_59E7BC[7]));

        if (buttonUpData != NULL && buttonDownData != NULL && buttonDisabledData != NULL) {
            // Left inventory down button.
            gInventoryScrollDownButton = buttonCreate(gInventoryWindow, 128, 62, 22, 23, -1, -1, KEY_ARROW_DOWN, -1, buttonUpData, buttonDownData, NULL, 0);
            buttonSetCallbacks(gInventoryScrollDownButton, sub_451970, sub_451978);
            sub_4D8674(gInventoryScrollDownButton, buttonDisabledData, buttonDisabledData, buttonDisabledData);
            buttonDisable(gInventoryScrollDownButton);

            if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
                // Invisible button representing left character.
                buttonCreate(gInventoryWindow, 44, 35, 60, 100, -1, -1, 2500, -1, NULL, NULL, NULL, 0);

                // Right inventory down button.
                gSecondaryInventoryScrollDownButton = buttonCreate(gInventoryWindow, 379, 62, 22, 23, -1, -1, KEY_CTRL_ARROW_DOWN, -1, buttonUpData, buttonDownData, 0, 0);
                if (gSecondaryInventoryScrollDownButton != -1) {
                    buttonSetCallbacks(gSecondaryInventoryScrollDownButton, sub_451970, sub_451978);
                    sub_4D8674(gSecondaryInventoryScrollDownButton, buttonDisabledData, buttonDisabledData, buttonDisabledData);
                    buttonDisable(gSecondaryInventoryScrollDownButton);
                }

                // Invisible button representing right character.
                buttonCreate(gInventoryWindow, 422, 35, 60, 100, -1, -1, 2501, -1, NULL, NULL, NULL, 0);
            } else {
                // Invisible button representing character (in inventory and use on dialogs).
                buttonCreate(gInventoryWindow, 176, 37, 60, 100, -1, -1, 2500, -1, NULL, NULL, NULL, 0);
            }
        }
    }

    if (inventoryWindowType != INVENTORY_WINDOW_TYPE_TRADE) {
        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
            if (!dword_51D430) {
                // Take all button (normal)
                fid = buildFid(6, 436, 0, 0, 0);
                buttonUpData = artLockFrameData(fid, 0, 0, &(off_59E7BC[8]));

                // Take all button (pressed)
                fid = buildFid(6, 437, 0, 0, 0);
                buttonDownData = artLockFrameData(fid, 0, 0, &(off_59E7BC[9]));

                if (buttonUpData != NULL && buttonDownData != NULL) {
                    // Take all button.
                    btn = buttonCreate(gInventoryWindow, 432, 204, 39, 41, -1, -1, KEY_UPPERCASE_A, -1, buttonUpData, buttonDownData, NULL, 0);
                    if (btn != -1) {
                        buttonSetCallbacks(btn, sub_451970, sub_451978);
                    }
                }
            }
        }
    } else {
        // Inventory button up (normal)
        fid = buildFid(6, 49, 0, 0, 0);
        buttonUpData = artLockFrameData(fid, 0, 0, &(off_59E7BC[8]));

        // Inventory button up (pressed)
        fid = buildFid(6, 50, 0, 0, 0);
        buttonDownData = artLockFrameData(fid, 0, 0, &(off_59E7BC[9]));

        if (buttonUpData != NULL && buttonDownData != NULL) {
            // Left offered inventory up button.
            btn = buttonCreate(gInventoryWindow, 128, 113, 22, 23, -1, -1, KEY_PAGE_UP, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, sub_451970, sub_451978);
            }

            // Right offered inventory up button.
            btn = buttonCreate(gInventoryWindow, 333, 113, 22, 23, -1, -1, KEY_CTRL_PAGE_UP, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, sub_451970, sub_451978);
            }
        }

        // Inventory button down (normal)
        fid = buildFid(6, 51, 0, 0, 0);
        buttonUpData = artLockFrameData(fid, 0, 0, &(off_59E7BC[8]));

        // Inventory button down (pressed).
        fid = buildFid(6, 52, 0, 0, 0);
        buttonDownData = artLockFrameData(fid, 0, 0, &(off_59E7BC[9]));

        if (buttonUpData != NULL && buttonDownData != NULL) {
            // Left offered inventory down button.
            btn = buttonCreate(gInventoryWindow, 128, 136, 22, 23, -1, -1, KEY_PAGE_DOWN, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, sub_451970, sub_451978);
            }

            // Right offered inventory down button.
            btn = buttonCreate(gInventoryWindow, 333, 136, 22, 23, -1, -1, KEY_CTRL_PAGE_DOWN, -1, buttonUpData, buttonDownData, NULL, 0);
            if (btn != -1) {
                buttonSetCallbacks(btn, sub_451970, sub_451978);
            }
        }
    }

    gInventoryRightHandItem = NULL;
    gInventoryArmor = NULL;
    gInventoryLeftHandItem = NULL;

    for (int index = 0; index < off_59E960->length; index++) {
        InventoryItem* inventoryItem = &(off_59E960->items[index]);
        Object* item = inventoryItem->item;
        if ((item->flags & 0x01000000) != 0) {
            if ((item->flags & 0x02000000) != 0) {
                gInventoryRightHandItem = item;
            }
            gInventoryLeftHandItem = item;
        } else if ((item->flags & 0x2000000) != 0) {
            gInventoryRightHandItem = item;
        } else if ((item->flags & 0x04000000) != 0) {
            gInventoryArmor = item;
        }
    }

    if (gInventoryLeftHandItem != NULL) {
        itemRemove(off_519058, gInventoryLeftHandItem, 1);
    }

    if (gInventoryRightHandItem != NULL && gInventoryRightHandItem != gInventoryLeftHandItem) {
        itemRemove(off_519058, gInventoryRightHandItem, 1);
    }

    if (gInventoryArmor != NULL) {
        itemRemove(off_519058, gInventoryArmor, 1);
    }

    sub_4716E8();

    bool isoWasEnabled = isoDisable();

    sub_44B48C(0);

    return isoWasEnabled;
}

// 0x46FBD8
void sub_46FBD8(bool shouldEnableIso)
{
    off_519058 = off_59E86C[0];

    if (gInventoryLeftHandItem != NULL) {
        gInventoryLeftHandItem->flags |= 0x01000000;
        if (gInventoryLeftHandItem == gInventoryRightHandItem) {
            gInventoryLeftHandItem->flags |= 0x02000000;
        }

        itemAdd(off_519058, gInventoryLeftHandItem, 1);
    }

    if (gInventoryRightHandItem != NULL && gInventoryRightHandItem != gInventoryLeftHandItem) {
        gInventoryRightHandItem->flags |= 0x02000000;
        itemAdd(off_519058, gInventoryRightHandItem, 1);
    }

    if (gInventoryArmor != NULL) {
        gInventoryArmor->flags |= 0x04000000;
        itemAdd(off_519058, gInventoryArmor, 1);
    }

    gInventoryRightHandItem = NULL;
    gInventoryArmor = NULL;
    gInventoryLeftHandItem = NULL;

    for (int index = 0; index < OFF_59E7BC_COUNT; index++) {
        artUnlock(off_59E7BC[index]);
    }

    if (shouldEnableIso) {
        isoEnable();
    }

    windowDestroy(gInventoryWindow);

    sub_44B454();

    if (dword_5190E0) {
        Attack v1;
        attackInit(&v1, gDude, NULL, HIT_MODE_PUNCH, HIT_LOCATION_TORSO);
        v1.attackerFlags = DAM_HIT;
        v1.tile = gDude->tile;
        sub_423C10(&v1, 0, 0, 1);

        Object* v2 = NULL;
        for (int index = 0; index < v1.extrasLength; index++) {
            Object* critter = v1.extras[index];
            if (critter != gDude
                && critter->data.critter.combat.team != gDude->data.critter.combat.team
                && statRoll(critter, STAT_PERCEPTION, 0, NULL) >= ROLL_SUCCESS) {
                sub_42E4C0(critter, gDude);

                if (v2 == NULL) {
                    v2 = critter;
                }
            }
        }

        if (v2 != NULL) {
            if (!isInCombat()) {
                STRUCT_664980 v3;
                v3.attacker = v2;
                v3.defender = gDude;
                v3.actionPointsBonus = 0;
                v3.accuracyBonus = 0;
                v3.damageBonus = 0;
                v3.minDamage = 0;
                v3.maxDamage = INT_MAX;
                v3.field_1C = 0;
                scriptsRequestCombat(&v3);
            }
        }

        dword_5190E0 = false;
    }
}

// 0x46FDF4
void sub_46FDF4(int a1, int a2, int inventoryWindowType)
{
    unsigned char* windowBuffer = windowGetBuffer(gInventoryWindow);
    int pitch;

    int v49 = 0;
    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_NORMAL) {
        pitch = 499;

        int backgroundFid = buildFid(6, 48, 0, 0, 0);

        CacheEntry* backgroundFrmHandle;
        unsigned char* backgroundFrmData = artLockFrameData(backgroundFid, 0, 0, &backgroundFrmHandle);
        if (backgroundFrmData != NULL) {
            // Clear scroll view background.
            blitBufferToBuffer(backgroundFrmData + pitch * 35 + 44, 64, gInventorySlotsCount * 48, pitch, windowBuffer + pitch * 35 + 44, pitch);

            // Clear armor button background.
            blitBufferToBuffer(backgroundFrmData + pitch * 183 + 154, 90, 61, pitch, windowBuffer + pitch * 183 + 154, pitch);

            if (gInventoryLeftHandItem != NULL && gInventoryLeftHandItem == gInventoryRightHandItem) {
                // Clear item1.
                int itemBackgroundFid = buildFid(6, 32, 0, 0, 0);

                CacheEntry* itemBackgroundFrmHandle;
                Art* itemBackgroundFrm = artLock(itemBackgroundFid, &itemBackgroundFrmHandle);
                if (itemBackgroundFrm != NULL) {
                    unsigned char* data = artGetFrameData(itemBackgroundFrm, 0, 0);
                    int width = artGetWidth(itemBackgroundFrm, 0, 0);
                    int height = artGetHeight(itemBackgroundFrm, 0, 0);
                    blitBufferToBuffer(data, width, height, width, windowBuffer + pitch * 284 + 152, pitch);
                    artUnlock(itemBackgroundFrmHandle);
                }
            } else {
                // Clear both items in one go.
                blitBufferToBuffer(backgroundFrmData + pitch * 286 + 154, 180, 61, pitch, windowBuffer + pitch * 286 + 154, pitch);
            }

            artUnlock(backgroundFrmHandle);
        }
    } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_USE_ITEM_ON) {
        pitch = 292;

        int backgroundFid = buildFid(6, 113, 0, 0, 0);

        CacheEntry* backgroundFrmHandle;
        unsigned char* backgroundFrmData = artLockFrameData(backgroundFid, 0, 0, &backgroundFrmHandle);
        if (backgroundFrmData != NULL) {
            // Clear scroll view background.
            blitBufferToBuffer(backgroundFrmData + pitch * 35 + 44, 64, gInventorySlotsCount * 48, pitch, windowBuffer + pitch * 35 + 44, pitch);
            artUnlock(backgroundFrmHandle);
        }
    } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
        pitch = 537;

        int backgroundFid = buildFid(6, 114, 0, 0, 0);

        CacheEntry* backgroundFrmHandle;
        unsigned char* backgroundFrmData = artLockFrameData(backgroundFid, 0, 0, &backgroundFrmHandle);
        if (backgroundFrmData != NULL) {
            // Clear scroll view background.
            blitBufferToBuffer(backgroundFrmData + pitch * 37 + 176, 64, gInventorySlotsCount * 48, pitch, windowBuffer + pitch * 37 + 176, pitch);
            artUnlock(backgroundFrmHandle);
        }
    } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        pitch = 480;

        windowBuffer = windowGetBuffer(gInventoryWindow);

        blitBufferToBuffer(windowGetBuffer(dword_59E97C) + 35 * (stru_6AC9F0.right - stru_6AC9F0.left + 1) + 100, 64, 48 * gInventorySlotsCount, stru_6AC9F0.right - stru_6AC9F0.left + 1, windowBuffer + pitch * 35 + 20, pitch);
        v49 = -20;
    } else {
        // Should be unreachable.
        __assume(0);
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_NORMAL
        || inventoryWindowType == INVENTORY_WINDOW_TYPE_USE_ITEM_ON
        || inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
        if (gInventoryScrollUpButton != -1) {
            if (a1 <= 0) {
                buttonDisable(gInventoryScrollUpButton);
            } else {
                buttonEnable(gInventoryScrollUpButton);
            }
        }

        if (gInventoryScrollDownButton != -1) {
            if (off_59E960->length - a1 <= gInventorySlotsCount) {
                buttonDisable(gInventoryScrollDownButton);
            } else {
                buttonEnable(gInventoryScrollDownButton);
            }
        }
    }

    int y = 0;
    for (int v19 = 0; v19 + a1 < off_59E960->length && v19 < gInventorySlotsCount; v19 += 1) {
        int v21 = v19 + a1 + 1;

        int width;
        int offset;
        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
            offset = pitch * (y + 39) + 26;
            width = 59;
        } else {
            if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
                offset = pitch * (y + 41) + 180;
            } else {
                offset = pitch * (y + 39) + 48;
            }
            width = 56;
        }

        InventoryItem* inventoryItem = &(off_59E960->items[off_59E960->length - v21]);

        int inventoryFid = itemGetInventoryFid(inventoryItem->item);
        artRender(inventoryFid, windowBuffer + offset, width, 40, pitch);

        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
            offset = pitch * (y + 41) + 180 + v49;
        } else {
            offset = pitch * (y + 39) + 48 + v49;
        }

        sub_4705A0(inventoryItem->item, inventoryItem->quantity, windowBuffer + offset, pitch, v19 == a2);

        y += 48;
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_NORMAL) {
        if (gInventoryRightHandItem != NULL) {
            int width = gInventoryRightHandItem == gInventoryLeftHandItem ? 180 : 90;
            int inventoryFid = itemGetInventoryFid(gInventoryRightHandItem);
            artRender(inventoryFid, windowBuffer + 499 * 286 + 245, width, 61, 499);
        }

        if (gInventoryLeftHandItem != NULL && gInventoryLeftHandItem != gInventoryRightHandItem) {
            int inventoryFid = itemGetInventoryFid(gInventoryLeftHandItem);
            artRender(inventoryFid, windowBuffer + 499 * 286 + 154, 90, 61, 499);
        }

        if (gInventoryArmor != NULL) {
            int inventoryFid = itemGetInventoryFid(gInventoryArmor);
            artRender(inventoryFid, windowBuffer + 499 * 183 + 154, 90, 61, 499);
        }
    }

    windowRefresh(gInventoryWindow);
}

// Render inventory item.
//
// [a1] is likely an index of the first visible item in the scrolling view.
// [a2] is likely an index of selected item or moving item (it decreases displayed number of items in inner functions).
//
// 0x47036C
void sub_47036C(int a1, int a2, Inventory* inventory, int inventoryWindowType)
{
    unsigned char* windowBuffer = windowGetBuffer(gInventoryWindow);

    int pitch;
    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
        pitch = 537;

        int fid = buildFid(6, 114, 0, 0, 0);

        CacheEntry* handle;
        unsigned char* data = artLockFrameData(fid, 0, 0, &handle);
        if (data != NULL) {
            blitBufferToBuffer(data + 537 * 37 + 297, 64, 48 * gInventorySlotsCount, 537, windowBuffer + 537 * 37 + 297, 537);
            artUnlock(handle);
        }
    } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        pitch = 480;

        unsigned char* src = windowGetBuffer(dword_59E97C);
        blitBufferToBuffer(src + (stru_6AC9F0.right - stru_6AC9F0.left + 1) * 35 + 475, 64, 48 * gInventorySlotsCount, stru_6AC9F0.right - stru_6AC9F0.left + 1, windowBuffer + 480 * 35 + 395, 480);
    } else {
        __assume(0);
    }

    int y = 0;
    for (int index = 0; index < gInventorySlotsCount; index++) {
        int v27 = a1 + index;
        if (v27 >= inventory->length) {
            break;
        }

        int offset;
        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
            offset = pitch * (y + 41) + 301;
        } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
            offset = pitch * (y + 39) + 397;
        } else {
            __assume(0);
        }

        InventoryItem* inventoryItem = &(inventory->items[inventory->length - (v27 + 1)]);
        int inventoryFid = itemGetInventoryFid(inventoryItem->item);
        artRender(inventoryFid, windowBuffer + offset, 56, 40, pitch);
        sub_4705A0(inventoryItem->item, inventoryItem->quantity, windowBuffer + offset, pitch, index == a2);

        y += 48;
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
        if (gSecondaryInventoryScrollUpButton != -1) {
            if (a1 <= 0) {
                buttonDisable(gSecondaryInventoryScrollUpButton);
            } else {
                buttonEnable(gSecondaryInventoryScrollUpButton);
            }
        }

        if (gSecondaryInventoryScrollDownButton != -1) {
            if (inventory->length - a1 <= gInventorySlotsCount) {
                buttonDisable(gSecondaryInventoryScrollDownButton);
            } else {
                buttonEnable(gSecondaryInventoryScrollDownButton);
            }
        }
    }
}

// Renders inventory item quantity.
//
// 0x4705A0
void sub_4705A0(Object* item, int quantity, unsigned char* dest, int pitch, bool a5)
{
    int oldFont = fontGetCurrent();
    fontSetCurrent(101);

    char formattedText[12];

    // NOTE: Original code is slightly different and probably used goto.
    bool draw = false;

    if (itemGetType(item) == ITEM_TYPE_AMMO) {
        int ammoQuantity = ammoGetCapacity(item) * (quantity - 1);

        if (!a5) {
            ammoQuantity += ammoGetQuantity(item);
        }

        if (ammoQuantity > 99999) {
            ammoQuantity = 99999;
        }

        sprintf(formattedText, "x%d", ammoQuantity);
        draw = true;
    } else {
        if (quantity > 1) {
            int v9 = quantity;
            if (a5) {
                v9 -= 1;
            }

            // NOTE: Checking for quantity twice probably means inlined function
            // or some macro expansion.
            if (quantity > 1) {
                if (v9 > 99999) {
                    v9 = 99999;
                }

                sprintf(formattedText, "x%d", v9);
                draw = true;
            }
        }
    }

    if (draw) {
        fontDrawText(dest, formattedText, 80, pitch, byte_6A38D0[32767]);
    }

    fontSetCurrent(oldFont);
}

// 0x470650
void sub_470650(int fid, int inventoryWindowType)
{
    if (getTicksSince(gInventoryWindowDudeRotationTimestamp) < INVENTORY_NORMAL_WINDOW_PC_ROTATION_DELAY) {
        return;
    }

    gInventoryWindowDudeRotation += 1;

    if (gInventoryWindowDudeRotation == ROTATION_COUNT) {
        gInventoryWindowDudeRotation = 0;
    }

    int rotations[2];
    if (fid == -1) {
        rotations[0] = gInventoryWindowDudeRotation;
        rotations[1] = ROTATION_SE;
    } else {
        rotations[0] = ROTATION_SW;
        rotations[1] = off_59E81C[dword_59E948]->rotation;
    }

    int fids[2] = {
        gInventoryWindowDudeFid,
        fid,
    };

    for (int index = 0; index < 2; index += 1) {
        int fid = fids[index];
        if (fid == -1) {
            continue;
        }

        CacheEntry* handle;
        Art* art = artLock(fid, &handle);
        if (art == NULL) {
            continue;
        }

        int frame = 0;
        if (index == 1) {
            frame = artGetFrameCount(art) - 1;
        }

        int rotation = rotations[index];

        unsigned char* frameData = artGetFrameData(art, frame, rotation);

        int framePitch = artGetWidth(art, frame, rotation);
        int frameWidth = min(framePitch, 60);

        int frameHeight = artGetHeight(art, frame, rotation);
        if (frameHeight > 100) {
            frameHeight = 100;
        }

        int win;
        Rect rect;
        CacheEntry* backrgroundFrmHandle;
        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
            unsigned char* windowBuffer = windowGetBuffer(dword_59E97C);
            int windowPitch = windowGetWidth(dword_59E97C);

            if (index == 1) {
                rect.left = 560;
                rect.top = 25;
            } else {
                rect.left = 15;
                rect.top = 25;
            }

            rect.right = rect.left + 60 - 1;
            rect.bottom = rect.top + 100 - 1;

            int frmId = gGameDialogSpeakerIsPartyMember ? 420 : 111;
            int backgroundFid = buildFid(6, frmId, 0, 0, 0);

            unsigned char* src = artLockFrameData(backgroundFid, 0, 0, &backrgroundFrmHandle);
            if (src != NULL) {
                blitBufferToBuffer(src + rect.top * (stru_6AC9F0.right - stru_6AC9F0.left + 1) + rect.left,
                    60,
                    100,
                    stru_6AC9F0.right - stru_6AC9F0.left + 1,
                    windowBuffer + windowPitch * rect.top + rect.left,
                    windowPitch);
            }

            blitBufferToBufferTrans(frameData, frameWidth, frameHeight, framePitch,
                windowBuffer + windowPitch * (rect.top + (100 - frameHeight) / 2) + (60 - frameWidth) / 2 + rect.left,
                windowPitch);

            win = dword_59E97C;
        } else {
            unsigned char* windowBuffer = windowGetBuffer(gInventoryWindow);
            int windowPitch = windowGetWidth(gInventoryWindow);

            if (index == 1) {
                if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
                    rect.left = 426;
                    rect.top = 39;
                } else {
                    rect.left = 297;
                    rect.top = 37;
                }
            } else {
                if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT) {
                    rect.left = 48;
                    rect.top = 39;
                } else {
                    rect.left = 176;
                    rect.top = 37;
                }
            }

            rect.right = rect.left + 60 - 1;
            rect.bottom = rect.top + 100 - 1;

            int backgroundFid = buildFid(6, 114, 0, 0, 0);
            unsigned char* src = artLockFrameData(backgroundFid, 0, 0, &backrgroundFrmHandle);
            if (src != NULL) {
                blitBufferToBuffer(src + 537 * rect.top + rect.left,
                    60,
                    100,
                    537,
                    windowBuffer + windowPitch * rect.top + rect.left,
                    windowPitch);
            }

            blitBufferToBufferTrans(frameData, frameWidth, frameHeight, framePitch,
                windowBuffer + windowPitch * (rect.top + (100 - frameHeight) / 2) + (60 - frameWidth) / 2 + rect.left,
                windowPitch);

            win = gInventoryWindow;
        }
        windowRefreshRect(win, &rect);

        artUnlock(backrgroundFrmHandle);
        artUnlock(handle);
    }

    gInventoryWindowDudeRotationTimestamp = sub_4C9370();
}

// 0x470A2C
int inventoryCommonInit()
{
    if (inventoryMessageListInit() == -1) {
        return -1;
    }

    dword_59E950 = gameUiIsDisabled();

    if (dword_59E950) {
        gameUiEnable();
    }

    gameMouseObjectsHide();

    gameMouseSetCursor(MOUSE_CURSOR_ARROW);

    int index;
    for (index = 0; index < INVENTORY_WINDOW_CURSOR_COUNT; index++) {
        InventoryCursorData* cursorData = &(gInventoryCursorData[index]);

        int fid = buildFid(6, gInventoryWindowCursorFrmIds[index], 0, 0, 0);
        Art* frm = artLock(fid, &(cursorData->frmHandle));
        if (frm == NULL) {
            break;
        }

        cursorData->frm = frm;
        cursorData->frmData = artGetFrameData(frm, 0, 0);
        cursorData->width = artGetWidth(frm, 0, 0);
        cursorData->height = artGetHeight(frm, 0, 0);
        artGetFrameOffsets(frm, 0, 0, &(cursorData->offsetX), &(cursorData->offsetY));
    }

    if (index != INVENTORY_WINDOW_CURSOR_COUNT) {
        for (; index >= 0; index--) {
            artUnlock(gInventoryCursorData[index].frmHandle);
        }

        if (dword_59E950) {
            gameUiDisable(0);
        }

        messageListFree(&gInventoryMessageList);

        return -1;
    }

    dword_519060 = true;
    dword_59E93C = -1;

    return 0;
}

// NOTE: Inlined.
//
// 0x470B8C
void inventoryCommonFree()
{
    for (int index = 0; index < INVENTORY_WINDOW_CURSOR_COUNT; index++) {
        artUnlock(gInventoryCursorData[index].frmHandle);
    }

    if (dword_59E950) {
        gameUiDisable(0);
    }

    // NOTE: Uninline.
    inventoryMessageListFree();

    dword_519060 = 0;
}

// 0x470BCC
void inventorySetCursor(int cursor)
{
    gInventoryCursor = cursor;

    if (cursor != INVENTORY_WINDOW_CURSOR_ARROW || dword_59E93C == -1) {
        InventoryCursorData* cursorData = &(gInventoryCursorData[cursor]);
        mouseSetFrame(cursorData->frmData, cursorData->width, cursorData->height, cursorData->width, cursorData->offsetX, cursorData->offsetY, 0);
    } else {
        inventoryItemSlotOnMouseEnter(-1, dword_59E93C);
    }
}

// 0x470C2C
void inventoryItemSlotOnMouseEnter(int btn, int keyCode)
{
    if (gInventoryCursor == INVENTORY_WINDOW_CURSOR_ARROW) {
        int x;
        int y;
        mouseGetPosition(&x, &y);

        Object* a2a = NULL;
        if (sub_472B54(keyCode, &a2a, NULL, NULL) != 0) {
            gameMouseRenderPrimaryAction(x, y, 3, gInventoryWindowMaxX, gInventoryWindowMaxY);

            int v5 = 0;
            int v6 = 0;
            sub_44D200(&v5, &v6);

            InventoryCursorData* cursorData = &(gInventoryCursorData[INVENTORY_WINDOW_CURSOR_PICK]);
            mouseSetFrame(cursorData->frmData, cursorData->width, cursorData->height, cursorData->width, v5, v6, 0);

            if (a2a != off_519110) {
                sub_49AC4C(off_59E86C[0], a2a, gInventoryPrintItemDescriptionHandler);
            }
        } else {
            InventoryCursorData* cursorData = &(gInventoryCursorData[INVENTORY_WINDOW_CURSOR_ARROW]);
            mouseSetFrame(cursorData->frmData, cursorData->width, cursorData->height, cursorData->width, cursorData->offsetX, cursorData->offsetY, 0);
        }

        off_519110 = a2a;
    }

    dword_59E93C = keyCode;
}

// 0x470D1C
void inventoryItemSlotOnMouseExit(int btn, int keyCode)
{
    if (gInventoryCursor == INVENTORY_WINDOW_CURSOR_ARROW) {
        InventoryCursorData* cursorData = &(gInventoryCursorData[INVENTORY_WINDOW_CURSOR_ARROW]);
        mouseSetFrame(cursorData->frmData, cursorData->width, cursorData->height, cursorData->width, cursorData->offsetX, cursorData->offsetY, 0);
    }

    dword_59E93C = -1;
}

// 0x470D5C
void sub_470D5C(Object* a1)
{
    if (gDude == off_519058) {
        int lightDistance;
        if (a1 != NULL && a1->lightDistance > 4) {
            lightDistance = a1->lightDistance;
        } else {
            lightDistance = 4;
        }

        Rect rect;
        objectSetLight(off_519058, lightDistance, 0x10000, &rect);
        tileWindowRefreshRect(&rect, gElevation);
    }
}

// 0x470DB8
void sub_470DB8(int keyCode, int a2)
{
    Object* a1a;
    Object** v29 = NULL;
    int count = sub_472B54(keyCode, &a1a, &v29, NULL);
    if (count == 0) {
        return;
    }

    int v3 = -1;
    Object* v39 = NULL;
    Rect rect;

    switch (keyCode) {
    case 1006:
        rect.left = 245;
        rect.top = 286;
        if (off_519058 == gDude && interfaceGetCurrentHand() != HAND_LEFT) {
            v39 = a1a;
        }
        break;
    case 1007:
        rect.left = 154;
        rect.top = 286;
        if (off_519058 == gDude && interfaceGetCurrentHand() == HAND_LEFT) {
            v39 = a1a;
        }
        break;
    case 1008:
        rect.left = 154;
        rect.top = 183;
        break;
    default:
        // NOTE: Original code a little bit different, this code path
        // is only for key codes below 1006.
        v3 = keyCode - 1000;
        rect.left = 44;
        rect.top = 48 * v3 + 35;
        break;
    }

    if (v3 == -1 || off_59E960->items[a2 + v3].quantity <= 1) {
        unsigned char* windowBuffer = windowGetBuffer(gInventoryWindow);
        if (gInventoryRightHandItem != gInventoryLeftHandItem || a1a != gInventoryLeftHandItem) {
            int height;
            int width;
            if (v3 == -1) {
                height = 61;
                width = 90;
            } else {
                height = 48;
                width = 64;
            }

            CacheEntry* backgroundFrmHandle;
            int backgroundFid = buildFid(6, 48, 0, 0, 0);
            unsigned char* backgroundFrmData = artLockFrameData(backgroundFid, 0, 0, &backgroundFrmHandle);
            if (backgroundFrmData != NULL) {
                blitBufferToBuffer(backgroundFrmData + 499 * rect.top + rect.left, width, height, 499, windowBuffer + 499 * rect.top + rect.left, 499);
                artUnlock(backgroundFrmHandle);
            }

            rect.right = rect.left + width - 1;
            rect.bottom = rect.top + height - 1;
        } else {
            CacheEntry* backgroundFrmHandle;
            int backgroundFid = buildFid(6, 48, 0, 0, 0);
            unsigned char* backgroundFrmData = artLockFrameData(backgroundFid, 0, 0, &backgroundFrmHandle);
            if (backgroundFrmData != NULL) {
                blitBufferToBuffer(backgroundFrmData + 499 * 286 + 154, 180, 61, 499, windowBuffer + 499 * 286 + 154, 499);
                artUnlock(backgroundFrmHandle);
            }

            rect.left = 154;
            rect.top = 286;
            rect.right = rect.left + 180 - 1;
            rect.bottom = rect.top + 61 - 1;
        }
        windowRefreshRect(gInventoryWindow, &rect);
    } else {
        sub_46FDF4(a2, v3, INVENTORY_WINDOW_TYPE_NORMAL);
    }

    CacheEntry* itemInventoryFrmHandle;
    int itemInventoryFid = itemGetInventoryFid(a1a);
    Art* itemInventoryFrm = artLock(itemInventoryFid, &itemInventoryFrmHandle);
    if (itemInventoryFrm != NULL) {
        int width = artGetWidth(itemInventoryFrm, 0, 0);
        int height = artGetHeight(itemInventoryFrm, 0, 0);
        unsigned char* itemInventoryFrmData = artGetFrameData(itemInventoryFrm, 0, 0);
        mouseSetFrame(itemInventoryFrmData, width, height, width, width / 2, height / 2, 0);
        soundPlayFile("ipickup1");
    }

    if (v39 != NULL) {
        sub_470D5C(NULL);
    }

    do {
        sub_4C8B78();
        sub_470650(-1, INVENTORY_WINDOW_TYPE_NORMAL);
    } while ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0);

    if (itemInventoryFrm != NULL) {
        artUnlock(itemInventoryFrmHandle);
        soundPlayFile("iputdown");
    }

    if (sub_4CA934(124, 35, 188, 48 * gInventorySlotsCount + 35)) {
        int x;
        int y;
        mouseGetPosition(&x, &y);

        int v18 = (y - 39) / 48 + a2;
        if (v18 < off_59E960->length) {
            Object* v19 = off_59E960->items[v18].item;
            if (v19 != a1a) {
                // TODO: Needs checking usage of v19
                if (itemGetType(v19) == ITEM_TYPE_CONTAINER) {
                    if (sub_476464(v19, a1a, v3, v29, count) == 0) {
                        v3 = 0;
                    }
                } else {
                    if (sub_47650C(v19, a1a, v29, count, keyCode) == 0) {
                        v3 = 0;
                    }
                }
            }
        }

        if (v3 == -1) {
            // TODO: Holy shit, needs refactoring.
            *v29 = NULL;
            if (itemAdd(off_519058, a1a, 1)) {
                *v29 = a1a;
            } else if (v29 == &gInventoryArmor) {
                sub_4715F8(off_59E86C[0], a1a, NULL);
            } else if (gInventoryRightHandItem == gInventoryLeftHandItem) {
                gInventoryLeftHandItem = NULL;
                gInventoryRightHandItem = NULL;
            }
        }
    } else if (sub_4CA934(234, 286, 324, 347)) {
        if (gInventoryLeftHandItem != NULL && itemGetType(gInventoryLeftHandItem) == ITEM_TYPE_CONTAINER && gInventoryLeftHandItem != a1a) {
            sub_476464(gInventoryLeftHandItem, a1a, v3, v29, count);
        } else if (gInventoryLeftHandItem == NULL || sub_47650C(gInventoryLeftHandItem, a1a, v29, count, keyCode)) {
            sub_4714E0(a1a, &gInventoryLeftHandItem, v29, keyCode);
        }
    } else if (sub_4CA934(325, 286, 415, 347)) {
        if (gInventoryRightHandItem != NULL && itemGetType(gInventoryRightHandItem) == ITEM_TYPE_CONTAINER && gInventoryRightHandItem != a1a) {
            sub_476464(gInventoryRightHandItem, a1a, v3, v29, count);
        } else if (gInventoryRightHandItem == NULL || sub_47650C(gInventoryRightHandItem, a1a, v29, count, keyCode)) {
            sub_4714E0(a1a, &gInventoryRightHandItem, v29, v3);
        }
    } else if (sub_4CA934(234, 183, 324, 244)) {
        if (itemGetType(a1a) == ITEM_TYPE_ARMOR) {
            Object* v21 = gInventoryArmor;
            int v22 = 0;
            if (v3 != -1) {
                itemRemove(off_519058, a1a, 1);
            }

            if (gInventoryArmor != NULL) {
                if (v29 != NULL) {
                    *v29 = gInventoryArmor;
                } else {
                    gInventoryArmor = NULL;
                    v22 = itemAdd(off_519058, v21, 1);
                }
            } else {
                if (v29 != NULL) {
                    *v29 = gInventoryArmor;
                }
            }

            if (v22 != 0) {
                gInventoryArmor = v21;
                if (v3 != -1) {
                    itemAdd(off_519058, a1a, 1);
                }
            } else {
                sub_4715F8(off_59E86C[0], v21, a1a);
                gInventoryArmor = a1a;
            }
        }
    } else if (sub_4CA934(256, 37, 316, 137)) {
        if (dword_59E96C != 0) {
            // TODO: Check this dword_59E96C - 1, not sure.
            sub_476464(off_59E86C[dword_59E96C - 1], a1a, v3, v29, count);
        }
    }

    sub_4716E8();
    inventoryRenderSummary();
    sub_46FDF4(a2, -1, INVENTORY_WINDOW_TYPE_NORMAL);
    inventorySetCursor(INVENTORY_WINDOW_CURSOR_HAND);
    if (off_519058 == gDude) {
        Object* item;
        if (interfaceGetCurrentHand() == HAND_LEFT) {
            item = critterGetItem1(off_519058);
        } else {
            item = critterGetItem2(off_519058);
        }

        if (item != NULL) {
            sub_470D5C(item);
        }
    }
}

// 0x4714E0
void sub_4714E0(Object* a1, Object** a2, Object** a3, int a4)
{
    if (*a2 != NULL) {
        if (itemGetType(*a2) == ITEM_TYPE_WEAPON && itemGetType(a1) == ITEM_TYPE_AMMO) {
            return;
        }

        if (a3 != NULL && (a3 != &gInventoryArmor || itemGetType(a1) == ITEM_TYPE_ARMOR)) {
            if (a3 == &gInventoryArmor) {
                sub_4715F8(off_59E86C[0], gInventoryArmor, *a2);
            }
        } else {
            if (a4 != -1) {
                itemRemove(off_519058, a1, 1);
            }

            Object* itemToAdd = *a2;
            *a2 = NULL;
            if (itemAdd(off_519058, itemToAdd, 1) != 0) {
                itemAdd(off_519058, a1, 1);
                return;
            }

            a4 = -1;
        }
    }

    if (a3 != NULL) {
        if (a3 == &gInventoryArmor) {
            sub_4715F8(off_59E86C[0], gInventoryArmor, NULL);
        }
        *a3 = NULL;
    }

    *a2 = a1;

    if (a4 != -1) {
        itemRemove(off_519058, a1, 1);
    }
}

// This function removes armor bonuses and effects granted by [oldArmor] and
// adds appropriate bonuses and effects granted by [newArmor]. Both [oldArmor]
// and [newArmor] can be NULL.
//
// 0x4715F8
void sub_4715F8(Object* critter, Object* oldArmor, Object* newArmor)
{
    int armorClassBonus = critterGetBonusStat(critter, STAT_ARMOR_CLASS);
    int oldArmorClass = armorGetArmorClass(oldArmor);
    int newArmorClass = armorGetArmorClass(newArmor);
    critterSetBonusStat(critter, STAT_ARMOR_CLASS, armorClassBonus - oldArmorClass + newArmorClass);

    int damageResistanceStat = STAT_DAMAGE_RESISTANCE;
    int damageThresholdStat = STAT_DAMAGE_THRESHOLD;
    for (int damageType = 0; damageType < DAMAGE_TYPE_COUNT; damageType += 1) {
        int damageResistanceBonus = critterGetBonusStat(critter, damageResistanceStat);
        int oldArmorDamageResistance = armorGetDamageResistance(oldArmor, damageType);
        int newArmorDamageResistance = armorGetDamageResistance(newArmor, damageType);
        critterSetBonusStat(critter, damageResistanceStat, damageResistanceBonus - oldArmorDamageResistance + newArmorDamageResistance);

        int damageThresholdBonus = critterGetBonusStat(critter, damageThresholdStat);
        int oldArmorDamageThreshold = armorGetDamageThreshold(oldArmor, damageType);
        int newArmorDamageThreshold = armorGetDamageThreshold(newArmor, damageType);
        critterSetBonusStat(critter, damageThresholdStat, damageThresholdBonus - oldArmorDamageThreshold + newArmorDamageThreshold);

        damageResistanceStat += 1;
        damageThresholdStat += 1;
    }

    if (objectIsPartyMember(critter)) {
        if (oldArmor != NULL) {
            int perk = armorGetPerk(oldArmor);
            perkRemoveEffect(critter, perk);
        }

        if (newArmor != NULL) {
            int perk = armorGetPerk(newArmor);
            perkAddEffect(critter, perk);
        }
    }
}

// 0x4716E8
void sub_4716E8()
{
    int fid;
    if ((off_519058->fid & 0xF000000) >> 24 == OBJ_TYPE_CRITTER) {
        Proto* proto;

        int v0 = dword_5108A4;

        if (protoGetProto(dword_51905C, &proto) == -1) {
            v0 = proto->fid & 0xFFF;
        }

        if (gInventoryArmor != NULL) {
            protoGetProto(gInventoryArmor->pid, &proto);
            if (critterGetStat(off_519058, STAT_GENDER) == GENDER_FEMALE) {
                v0 = proto->item.data.armor.femaleFid;
            } else {
                v0 = proto->item.data.armor.maleFid;
            }

            if (v0 == -1) {
                v0 = dword_5108A4;
            }
        }

        int animationCode = 0;
        if (interfaceGetCurrentHand()) {
            if (gInventoryRightHandItem != NULL) {
                protoGetProto(gInventoryRightHandItem->pid, &proto);
                if (proto->item.type == ITEM_TYPE_WEAPON) {
                    animationCode = proto->item.data.weapon.animationCode;
                }
            }
        } else {
            if (gInventoryLeftHandItem != NULL) {
                protoGetProto(gInventoryLeftHandItem->pid, &proto);
                if (proto->item.type == ITEM_TYPE_WEAPON) {
                    animationCode = proto->item.data.weapon.animationCode;
                }
            }
        }

        fid = buildFid(1, v0, 0, animationCode, 0);
    } else {
        fid = off_519058->fid;
    }

    gInventoryWindowDudeFid = fid;
}

// 0x4717E4
void inventoryOpenUseItemOn(Object* a1)
{
    if (inventoryCommonInit() == -1) {
        return;
    }

    bool isoWasEnabled = sub_46EC90(INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
    sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
    inventorySetCursor(INVENTORY_WINDOW_CURSOR_HAND);
    for (;;) {
        if (dword_5186CC != 0) {
            break;
        }

        sub_470650(-1, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);

        int keyCode = sub_4C8B78();
        switch (keyCode) {
        case KEY_HOME:
            dword_59E844[dword_59E96C] = 0;
            sub_46FDF4(0, -1, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
            break;
        case KEY_ARROW_UP:
            if (dword_59E844[dword_59E96C] > 0) {
                dword_59E844[dword_59E96C] -= 1;
                sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
            }
            break;
        case KEY_PAGE_UP:
            dword_59E844[dword_59E96C] -= gInventorySlotsCount;
            if (dword_59E844[dword_59E96C] < 0) {
                dword_59E844[dword_59E96C] = 0;
                sub_46FDF4(dword_59E844[dword_59E96C], -1, 1);
            }
            break;
        case KEY_END:
            dword_59E844[dword_59E96C] = off_59E960->length - gInventorySlotsCount;
            if (dword_59E844[dword_59E96C] < 0) {
                dword_59E844[dword_59E96C] = 0;
            }
            sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
            break;
        case KEY_ARROW_DOWN:
            if (dword_59E844[dword_59E96C] + gInventorySlotsCount < off_59E960->length) {
                dword_59E844[dword_59E96C] += 1;
                sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
            }
            break;
        case KEY_PAGE_DOWN:
            dword_59E844[dword_59E96C] += gInventorySlotsCount;
            if (dword_59E844[dword_59E96C] + gInventorySlotsCount >= off_59E960->length) {
                dword_59E844[dword_59E96C] = off_59E960->length - gInventorySlotsCount;
                if (dword_59E844[dword_59E96C] < 0) {
                    dword_59E844[dword_59E96C] = 0;
                }
            }
            sub_46FDF4(dword_59E844[dword_59E96C], -1, 1);
            break;
        case 2500:
            sub_476394(keyCode, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
            break;
        default:
            if ((mouseGetEvent() & MOUSE_EVENT_RIGHT_BUTTON_DOWN) != 0) {
                if (gInventoryCursor == INVENTORY_WINDOW_CURSOR_HAND) {
                    inventorySetCursor(INVENTORY_WINDOW_CURSOR_ARROW);
                } else {
                    inventorySetCursor(INVENTORY_WINDOW_CURSOR_HAND);
                }
            } else if ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_DOWN) != 0) {
                if (keyCode >= 1000 && keyCode < 1000 + gInventorySlotsCount) {
                    if (gInventoryCursor == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inventoryWindowOpenContextMenu(keyCode, INVENTORY_WINDOW_TYPE_USE_ITEM_ON);
                    } else {
                        int inventoryItemIndex = off_59E960->length - (dword_59E844[dword_59E96C] + keyCode - 1000 + 1);
                        if (inventoryItemIndex < off_59E960->length) {
                            InventoryItem* inventoryItem = &(off_59E960->items[inventoryItemIndex]);
                            if (isInCombat()) {
                                if (gDude->data.critter.combat.ap >= 2) {
                                    if (sub_411F2C(gDude, a1, inventoryItem->item) != -1) {
                                        int actionPoints = gDude->data.critter.combat.ap;
                                        if (actionPoints < 2) {
                                            gDude->data.critter.combat.ap = 0;
                                        } else {
                                            gDude->data.critter.combat.ap = actionPoints - 2;
                                        }
                                        interfaceRenderActionPoints(gDude->data.critter.combat.ap, dword_56D39C);
                                    }
                                }
                            } else {
                                sub_411F2C(gDude, a1, inventoryItem->item);
                            }
                            keyCode = VK_ESCAPE;
                        } else {
                            keyCode = -1;
                        }
                    }
                }
            }
        }

        if (keyCode == VK_ESCAPE) {
            break;
        }
    }

    sub_46FBD8(isoWasEnabled);

    // NOTE: Uninline.
    inventoryCommonFree();
}

// 0x471B70
Object* critterGetItem2(Object* critter)
{
    int i;
    Inventory* inventory;
    Object* item;

    if (gInventoryRightHandItem != NULL && critter == off_519058) {
        return gInventoryRightHandItem;
    }

    inventory = &(critter->data.inventory);
    for (i = 0; i < inventory->length; i++) {
        item = inventory->items[i].item;
        if (item->flags & 0x02000000) {
            return item;
        }
    }

    return NULL;
}

// 0x471BBC
Object* critterGetItem1(Object* critter)
{
    int i;
    Inventory* inventory;
    Object* item;

    if (gInventoryLeftHandItem != NULL && critter == off_519058) {
        return gInventoryLeftHandItem;
    }

    inventory = &(critter->data.inventory);
    for (i = 0; i < inventory->length; i++) {
        item = inventory->items[i].item;
        if (item->flags & 0x01000000) {
            return item;
        }
    }

    return NULL;
}

// 0x471C08
Object* critterGetArmor(Object* critter)
{
    int i;
    Inventory* inventory;
    Object* item;

    if (gInventoryArmor != NULL && critter == off_519058) {
        return gInventoryArmor;
    }

    inventory = &(critter->data.inventory);
    for (i = 0; i < inventory->length; i++) {
        item = inventory->items[i].item;
        if (item->flags & 0x04000000) {
            return item;
        }
    }

    return NULL;
}

// 0x471CA0
Object* objectGetCarriedObjectByPid(Object* obj, int pid)
{
    Inventory* inventory = &(obj->data.inventory);

    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        if (inventoryItem->item->pid == pid) {
            return inventoryItem->item;
        }

        Object* found = objectGetCarriedObjectByPid(inventoryItem->item, pid);
        if (found != NULL) {
            return found;
        }
    }

    return NULL;
}

// 0x471CDC
int objectGetCarriedQuantityByPid(Object* object, int pid)
{
    int quantity = 0;

    Inventory* inventory = &(object->data.inventory);
    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        if (inventoryItem->item->pid == pid) {
            quantity += inventoryItem->quantity;
        }

        quantity += objectGetCarriedQuantityByPid(inventoryItem->item, pid);
    }

    return quantity;
}

// Renders character's summary of SPECIAL stats, equipped armor bonuses,
// and weapon's damage/range.
//
// 0x471D5C
void inventoryRenderSummary()
{
    int v56[7];
    static_assert(sizeof(v56) == sizeof(dword_46E6D0), "wrong size");
    memcpy(v56, dword_46E6D0, sizeof(v56));

    int v57[7];
    static_assert(sizeof(v57) == sizeof(dword_46E6EC), "wrong size");
    memcpy(v57, dword_46E6EC, sizeof(v57));

    int oldFont = fontGetCurrent();
    fontSetCurrent(101);

    unsigned char* windowBuffer = windowGetBuffer(gInventoryWindow);

    int fid = buildFid(6, 48, 0, 0, 0);

    // #region Background

    CacheEntry* backgroundHandle;
    unsigned char* backgroundData = artLockFrameData(fid, 0, 0, &backgroundHandle);
    if (backgroundData != NULL) {
        blitBufferToBuffer(backgroundData + 499 * 44 + 297, 152, 188, 499, windowBuffer + 499 * 44 + 297, 499);
    }
    artUnlock(backgroundHandle);

    // #endregion

    // Render character name.
    const char* critterName = critterGetName(off_59E86C[0]);
    fontDrawText(windowBuffer + 499 * 44 + 297, critterName, 80, 499, byte_6A38D0[992]);

    bufferDrawLine(windowBuffer,
        499,
        297,
        3 * fontGetLineHeight() / 2 + 44,
        440,
        3 * fontGetLineHeight() / 2 + 44,
        byte_6A38D0[992]);

    MessageListItem messageListItem;

    int offset = 499 * 2 * fontGetLineHeight() + 499 * 44 + 297;
    for (int stat = 0; stat < 7; stat++) {
        messageListItem.num = stat;
        if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
            fontDrawText(windowBuffer + offset, messageListItem.text, 80, 499, byte_6A38D0[992]);
        }

        int value = critterGetStat(off_59E86C[0], stat);
        char valueText[4]; // TODO: Size is probably wrong.
        sprintf(valueText, "%d", value);
        fontDrawText(windowBuffer + offset + 24, valueText, 80, 499, byte_6A38D0[992]);

        offset += 499 * fontGetLineHeight();
    }

    offset -= 499 * 7 * fontGetLineHeight();

    for (int index = 0; index < 7; index += 1) {
        messageListItem.num = 7 + index;
        if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
            fontDrawText(windowBuffer + offset + 40, messageListItem.text, 80, 499, byte_6A38D0[992]);
        }

        char valueText[80]; // TODO: Size is probably wrong.
        if (v57[index] == -1) {
            int value = critterGetStat(off_59E86C[0], v57[index]);
            sprintf(valueText, "   %d", value);
        } else {
            int value1 = critterGetStat(off_59E86C[0], v56[index]);
            int value2 = critterGetStat(off_59E86C[0], v57[index]);
            const char* format = index != 0 ? "%d/%d%%" : "%d/%d";
            sprintf(valueText, format, value1, value2);
        }

        fontDrawText(windowBuffer + offset + 104, valueText, 80, 499, byte_6A38D0[992]);

        offset += 499 * fontGetLineHeight();
    }

    bufferDrawLine(windowBuffer, 499, 297, 18 * fontGetLineHeight() / 2 + 48, 440, 18 * fontGetLineHeight() / 2 + 48, byte_6A38D0[992]);
    bufferDrawLine(windowBuffer, 499, 297, 26 * fontGetLineHeight() / 2 + 48, 440, 26 * fontGetLineHeight() / 2 + 48, byte_6A38D0[992]);

    Object* itemsInHands[2] = {
        gInventoryLeftHandItem,
        gInventoryRightHandItem,
    };

    const int hitModes[2] = {
        HIT_MODE_LEFT_WEAPON_PRIMARY,
        HIT_MODE_RIGHT_WEAPON_PRIMARY,
    };

    offset += 499 * fontGetLineHeight();

    for (int index = 0; index < 2; index += 1) {
        Object* item = itemsInHands[index];
        if (item == NULL) {
            // No item
            messageListItem.num = 14;
            if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                fontDrawText(windowBuffer + offset, messageListItem.text, 120, 499, byte_6A38D0[992]);
            }

            offset += 499 * fontGetLineHeight();

            char formattedText[80]; // TODO: Size is probably wrong.

            // Unarmed dmg:
            messageListItem.num = 24;
            if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                int damage = critterGetStat(off_59E86C[0], STAT_UNARMED_DAMAGE) + 2;
                sprintf(formattedText, "%s 1-%d", messageListItem.text, damage);
            }

            fontDrawText(windowBuffer + offset, formattedText, 120, 499, byte_6A38D0[992]);

            offset += 3 * 499 * fontGetLineHeight();
            continue;
        }

        const char* itemName = itemGetName(item);
        fontDrawText(windowBuffer + offset, itemName, 140, 499, byte_6A38D0[992]);

        offset += 499 * fontGetLineHeight();

        int itemType = itemGetType(item);
        if (itemType != ITEM_TYPE_WEAPON) {
            if (itemType == ITEM_TYPE_ARMOR) {
                // (Not worn)
                messageListItem.num = 18;
                if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                    fontDrawText(windowBuffer + offset, messageListItem.text, 120, 499, byte_6A38D0[992]);
                }
            }

            offset += 3 * 499 * fontGetLineHeight();
            continue;
        }

        int range = sub_478A1C(off_59E86C[0], hitModes[index]);

        int damageMin;
        int damageMax;
        weaponGetDamageMinMax(item, &damageMin, &damageMax);

        int attackType = weaponGetAttackTypeForHitMode(item, hitModes[index]);

        int meleeDamage;
        if (attackType == ATTACK_TYPE_MELEE || attackType == ATTACK_TYPE_UNARMED) {
            meleeDamage = critterGetStat(off_59E86C[0], STAT_MELEE_DAMAGE);
        } else {
            meleeDamage = 0;
        }

        messageListItem.num = 15; // Dmg:
        if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
            char formattedText[80]; // TODO: Size is probably wrong.
            if (attackType != 4 && range <= 1) {
                sprintf(formattedText, "%s %d-%d", messageListItem.text, damageMin, damageMax + meleeDamage);
            } else {
                MessageListItem rangeMessageListItem;
                rangeMessageListItem.num = 16; // Rng:
                if (messageListGetItem(&gInventoryMessageList, &rangeMessageListItem)) {
                    sprintf(formattedText, "%s %d-%d   %s %d", messageListItem.text, damageMin, damageMax + meleeDamage, rangeMessageListItem.text, attackType);
                }
            }

            fontDrawText(windowBuffer + offset, formattedText, 140, 499, byte_6A38D0[992]);
        }

        offset += 499 * fontGetLineHeight();

        if (ammoGetCapacity(item) > 0) {
            char formattedText[80]; // TODO: Size is probably wrong.

            int ammoTypePid = weaponGetAmmoTypePid(item);

            messageListItem.num = 17; // Ammo:
            if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                if (ammoTypePid != -1) {
                    if (ammoGetQuantity(item) != 0) {
                        const char* ammoName = protoGetName(ammoTypePid);
                        int capacity = ammoGetCapacity(item);
                        int quantity = ammoGetQuantity(item);
                        sprintf(formattedText, "%s %d/%d %s", messageListItem.text, quantity, capacity, ammoName);
                    } else {
                        int capacity = ammoGetCapacity(item);
                        int quantity = ammoGetQuantity(item);
                        sprintf(formattedText, "%s %d/%d", messageListItem.text, quantity, capacity);
                    }
                }
            } else {
                int capacity = ammoGetCapacity(item);
                int quantity = ammoGetQuantity(item);
                sprintf(formattedText, "%s %d/%d", messageListItem.text, quantity, capacity);
            }

            fontDrawText(windowBuffer + offset, formattedText, 140, 499, byte_6A38D0[992]);
        }

        offset += 2 * 499 * fontGetLineHeight();
    }

    // Total wt:
    messageListItem.num = 20;
    if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
        if (off_59E86C[0]->pid >> 24 == OBJ_TYPE_CRITTER) {
            int carryWeight = critterGetStat(off_59E86C[0], STAT_CARRY_WEIGHT);
            int inventoryWeight = objectGetInventoryWeight(off_59E86C[0]);

            char formattedText[80]; // TODO: Size is probably wrong.
            sprintf(formattedText, "%s %d/%d", messageListItem.text, inventoryWeight, carryWeight);

            int color = byte_6A38D0[992];
            if (critterIsEncumbered(off_59E86C[0])) {
                color = byte_6A38D0[31744];
            }

            fontDrawText(windowBuffer + offset + 15, formattedText, 120, 499, color);
        } else {
            int inventoryWeight = objectGetInventoryWeight(off_59E86C[0]);

            char formattedText[80]; // TODO: Size is probably wrong.
            sprintf(formattedText, "%s %d", messageListItem.text, inventoryWeight);

            fontDrawText(windowBuffer + offset + 30, formattedText, 80, 499, byte_6A38D0[992]);
        }
    }

    fontSetCurrent(oldFont);
}

// Finds next item of given [itemType] (can be -1 which means any type of
// item).
//
// The [index] is used to control where to continue the search from, -1 - from
// the beginning.
//
// 0x472698
Object* sub_472698(Object* obj, int itemType, int* indexPtr)
{
    int dummy = -1;
    if (indexPtr == NULL) {
        indexPtr = &dummy;
    }

    *indexPtr += 1;

    Inventory* inventory = &(obj->data.inventory);

    // TODO: Refactor with for loop.
    if (*indexPtr >= inventory->length) {
        return NULL;
    }

    while (itemType != -1 && itemGetType(inventory->items[*indexPtr].item) != itemType) {
        *indexPtr += 1;

        if (*indexPtr >= inventory->length) {
            return NULL;
        }
    }

    return inventory->items[*indexPtr].item;
}

// 0x4726EC
Object* sub_4726EC(Object* obj, int id)
{
    if (obj->id == id) {
        return obj;
    }

    Inventory* inventory = &(obj->data.inventory);
    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        Object* item = inventoryItem->item;
        if (item->id == id) {
            return item;
        }

        if (itemGetType(item) == ITEM_TYPE_CONTAINER) {
            item = sub_4726EC(item, id);
            if (item != NULL) {
                return item;
            }
        }
    }

    return NULL;
}

// 0x472740
Object* sub_472740(Object* obj, int a2)
{
    Inventory* inventory;

    inventory = &(obj->data.inventory);

    if (a2 < 0 || a2 >= inventory->length) {
        return NULL;
    }

    return inventory->items[a2].item;
}

// inven_wield
// 0x472758
int sub_472758(Object* a1, Object* a2, int a3)
{
    return sub_472768(a1, a2, a3, true);
}

// 0x472768
int sub_472768(Object* critter, Object* item, int a3, bool a4)
{
    if (a4) {
        if (!isoIsDisabled()) {
            reg_anim_begin(2);
        }
    }

    int itemType = itemGetType(item);
    if (itemType == ITEM_TYPE_ARMOR) {
        Object* armor = critterGetArmor(critter);
        if (armor != NULL) {
            armor->flags = ~0x4000000;
        }

        item->flags |= 0x4000000;

        int baseFrmId;
        if (critterGetStat(critter, STAT_GENDER) == GENDER_FEMALE) {
            baseFrmId = armorGetFemaleFid(item);
        } else {
            baseFrmId = armorGetMaleFid(item);
        }

        if (baseFrmId == -1) {
            baseFrmId = 1;
        }

        if (critter == gDude) {
            if (!isoIsDisabled()) {
                int fid = buildFid(1, baseFrmId, 0, (critter->fid & 0xF000) >> 12, critter->rotation + 1);
                reg_anim_17(critter, fid, 0);
            }
        } else {
            sub_4715F8(critter, armor, item);
        }
    } else {
        int hand;
        if (critter == gDude) {
            hand = interfaceGetCurrentHand();
        } else {
            hand = HAND_RIGHT;
        }

        int weaponAnimationCode = weaponGetAnimationCode(item);
        int hitModeAnimationCode = weaponGetAnimationForHitMode(item, HIT_MODE_RIGHT_WEAPON_PRIMARY);
        int fid = buildFid(1, critter->fid & 0xFFF, hitModeAnimationCode, weaponAnimationCode, critter->rotation + 1);
        if (!artExists(fid)) {
            debugPrint("\ninven_wield failed!  ERROR ERROR ERROR!");
            return -1;
        }

        Object* v17;
        if (a3) {
            v17 = critterGetItem2(critter);
            item->flags |= 0x2000000;
        } else {
            v17 = critterGetItem1(critter);
            item->flags |= 0x1000000;
        }

        Rect rect;
        if (v17 != NULL) {
            v17->flags &= ~(0x1000000 | 0x2000000);

            if (v17->pid == PROTO_ID_LIT_FLARE) {
                int lightIntensity;
                int lightDistance;
                if (critter == gDude) {
                    lightIntensity = LIGHT_LEVEL_MAX;
                    lightDistance = 4;
                } else {
                    Proto* proto;
                    if (protoGetProto(critter->pid, &proto) == -1) {
                        return -1;
                    }

                    lightDistance = proto->lightDistance;
                    lightIntensity = proto->lightIntensity;
                }

                objectSetLight(critter, lightDistance, lightIntensity, &rect);
            }
        }

        if (item->pid == PROTO_ID_LIT_FLARE) {
            int lightDistance = item->lightDistance;
            if (lightDistance < critter->lightDistance) {
                lightDistance = critter->lightDistance;
            }

            int lightIntensity = item->lightIntensity;
            if (lightIntensity < critter->lightIntensity) {
                lightIntensity = critter->lightIntensity;
            }

            objectSetLight(critter, lightDistance, lightIntensity, &rect);
            tileWindowRefreshRect(&rect, gElevation);
        }

        if (itemGetType(item) == ITEM_TYPE_WEAPON) {
            weaponAnimationCode = weaponGetAnimationCode(item);
        } else {
            weaponAnimationCode = 0;
        }

        if (hand == a3) {
            if ((critter->fid & 0xF000) >> 12 != 0) {
                if (a4) {
                    if (!isoIsDisabled()) {
                        const char* soundEffectName = sfxBuildCharName(critter, ANIM_PUT_AWAY, CHARACTER_SOUND_EFFECT_UNUSED);
                        reg_anim_play_sfx(critter, soundEffectName, 0);
                        reg_anim_animate(critter, ANIM_PUT_AWAY, 0);
                    }
                }
            }

            // TODO: Check.
            if (a4 && isoIsDisabled()) {
                if (weaponAnimationCode != 0) {
                    reg_anim_18(critter, weaponAnimationCode, -1);
                } else {
                    int fid = buildFid(1, critter->fid & 0xFFF, 0, 0, critter->rotation + 1);
                    reg_anim_17(critter, fid, -1);
                }
            } else {
                int fid = buildFid(1, critter->fid & 0xFFF, 0, weaponAnimationCode, critter->rotation + 1);
                sub_418378(critter, critter->rotation, fid);
            }
        }
    }

    if (a4) {
        if (!isoIsDisabled()) {
            return reg_anim_end();
        }
    }

    return 0;
}

// inven_unwield
// 0x472A54
int sub_472A54(Object* critter_obj, int a2)
{
    return sub_472A64(critter_obj, a2, 1);
}

// 0x472A64
int sub_472A64(Object* obj, int a2, int a3)
{
    int v6;
    Object* item_obj;
    int fid;

    if (obj == gDude) {
        v6 = interfaceGetCurrentHand();
    } else {
        v6 = 1;
    }

    if (a2) {
        item_obj = critterGetItem2(obj);
    } else {
        item_obj = critterGetItem1(obj);
    }

    if (item_obj) {
        item_obj->flags &= 0x3000000;
    }

    if (v6 == a2 && ((obj->fid & 0xF000) >> 12) != 0) {
        if (a3 && !isoIsDisabled()) {
            reg_anim_begin(2);

            const char* sfx = sfxBuildCharName(obj, ANIM_PUT_AWAY, CHARACTER_SOUND_EFFECT_UNUSED);
            reg_anim_play_sfx(obj, sfx, 0);

            reg_anim_animate(obj, 39, 0);

            fid = buildFid(1, obj->fid & 0xFFF, 0, 0, obj->rotation + 1);
            reg_anim_17(obj, fid, -1);

            return reg_anim_end();
        }

        fid = buildFid(1, obj->fid & 0xFFF, 0, 0, obj->rotation + 1);
        sub_418378(obj, obj->rotation, fid);
    }

    return 0;
}

// 0x472B54
int sub_472B54(int keyCode, Object** a2, Object*** a3, Object** a4)
{
    Object** v6;
    Object* v7;
    Object* v8;
    int quantity = 0;

    switch (keyCode) {
    case 1006:
        v6 = &gInventoryRightHandItem;
        v7 = off_59E86C[0];
        v8 = gInventoryRightHandItem;
        break;
    case 1007:
        v6 = &gInventoryLeftHandItem;
        v7 = off_59E86C[0];
        v8 = gInventoryLeftHandItem;
        break;
    case 1008:
        v6 = &gInventoryArmor;
        v7 = off_59E86C[0];
        v8 = gInventoryArmor;
        break;
    default:
        v6 = NULL;
        v7 = NULL;
        v8 = NULL;

        InventoryItem* inventoryItem;
        if (keyCode < 2000) {
            int index = dword_59E844[dword_59E96C] + keyCode - 1000;
            if (index >= off_59E960->length) {
                break;
            }

            inventoryItem = &(off_59E960->items[off_59E960->length - (index + 1)]);
            v8 = inventoryItem->item;
            v7 = off_59E86C[dword_59E96C];
        } else if (keyCode < 2300) {
            int index = dword_59E7EC[dword_59E948] + keyCode - 2000;
            if (index >= off_59E978->length) {
                break;
            }

            inventoryItem = &(off_59E978->items[off_59E978->length - (index + 1)]);
            v8 = inventoryItem->item;
            v7 = off_59E944;
        } else if (keyCode < 2400) {
            int index = dword_59E8A0 + keyCode - 2300;
            if (index >= off_59E8A4->length) {
                break;
            }

            inventoryItem = &(off_59E8A4->items[off_59E8A4->length - (index + 1)]);
            v8 = inventoryItem->item;
            v7 = off_59E934;
        } else {
            int index = dword_59E89C + keyCode - 2400;
            if (index >= off_59E94C->length) {
                break;
            }

            inventoryItem = &(off_59E94C->items[off_59E94C->length - (index + 1)]);
            v8 = inventoryItem->item;
            v7 = off_59E944;
        }

        quantity = inventoryItem->quantity;
    }

    if (a3 != NULL) {
        *a3 = v6;
    }

    if (a2 != NULL) {
        *a2 = v8;
    }

    if (a4 != NULL) {
        *a4 = v7;
    }

    if (quantity == 0 && v8 != NULL) {
        quantity = 1;
    }

    return quantity;
}

// Displays item description.
//
// The [string] is mutated in the process replacing spaces back and forth
// for word wrapping purposes.
//
// inven_display_msg
// 0x472D24
void inventoryRenderItemDescription(char* string)
{
    int oldFont = fontGetCurrent();
    fontSetCurrent(101);

    unsigned char* windowBuffer = windowGetBuffer(gInventoryWindow);
    windowBuffer += 499 * 44 + 297;

    char* c = string;
    while (c != NULL && *c != '\0') {
        dword_519064 += 1;
        if (dword_519064 > 17) {
            debugPrint("\nError: inven_display_msg: out of bounds!");
            return;
        }

        char* space = NULL;
        if (fontGetStringWidth(c) > 152) {
            // Look for next space.
            space = c + 1;
            while (*space != '\0' && *space != ' ') {
                space += 1;
            }

            if (*space == '\0') {
                // This was the last line containing very long word. Text
                // drawing routine will silently truncate it after reaching
                // desired length.
                fontDrawText(windowBuffer + 499 * dword_519064 * fontGetLineHeight(), c, 152, 499, byte_6A38D0[992]);
                return;
            }

            char* nextSpace = space + 1;
            while (true) {
                while (*nextSpace != '\0' && *nextSpace != ' ') {
                    nextSpace += 1;
                }

                if (*nextSpace == '\0') {
                    break;
                }

                // Break string and measure it.
                *nextSpace = '\0';
                if (fontGetStringWidth(c) >= 152) {
                    // Next space is too far to fit in one line. Restore next
                    // space's character and stop.
                    *nextSpace = ' ';
                    break;
                }

                space = nextSpace;

                // Restore next space's character and continue looping from the
                // next character.
                *nextSpace = ' ';
                nextSpace += 1;
            }

            if (*space == ' ') {
                *space = '\0';
            }
        }

        if (fontGetStringWidth(c) > 152) {
            debugPrint("\nError: inven_display_msg: word too long!");
            return;
        }

        fontDrawText(windowBuffer + 499 * dword_519064 * fontGetLineHeight(), c, 152, 499, byte_6A38D0[992]);

        if (space != NULL) {
            c = space + 1;
            if (*space == '\0') {
                *space = ' ';
            }
        } else {
            c = NULL;
        }
    }

    fontSetCurrent(oldFont);
}

// Examines inventory item.
//
// 0x472EB8
void inventoryExamineItem(Object* critter, Object* item)
{
    int oldFont = fontGetCurrent();
    fontSetCurrent(101);

    unsigned char* windowBuffer = windowGetBuffer(gInventoryWindow);

    // Clear item description area.
    int backgroundFid = buildFid(6, 48, 0, 0, 0);

    CacheEntry* handle;
    unsigned char* backgroundData = artLockFrameData(backgroundFid, 0, 0, &handle);
    if (backgroundData != NULL) {
        blitBufferToBuffer(backgroundData + 499 * 44 + 297, 152, 188, 499, windowBuffer + 499 * 44 + 297, 499);
    }
    artUnlock(handle);

    // Reset item description lines counter.
    dword_519064 = 0;

    // Render item's name.
    char* itemName = objectGetName(item);
    inventoryRenderItemDescription(itemName);

    // Increment line counter to accomodate separator below.
    dword_519064 += 1;

    int lineHeight = fontGetLineHeight();

    // Draw separator.
    bufferDrawLine(windowBuffer,
        499,
        297,
        3 * lineHeight / 2 + 49,
        440,
        3 * lineHeight / 2 + 49,
        byte_6A38D0[992]);

    // Examine item.
    sub_49AD88(critter, item, inventoryRenderItemDescription);

    // Add weight if neccessary.
    int weight = itemGetWeight(item);
    if (weight != 0) {
        MessageListItem messageListItem;
        messageListItem.num = 540;

        if (weight == 1) {
            messageListItem.num = 541;
        }

        if (!messageListGetItem(&gProtoMessageList, &messageListItem)) {
            debugPrint("\nError: Couldn't find message!");
        }

        char formattedText[40];
        sprintf(formattedText, messageListItem.text, weight);
        inventoryRenderItemDescription(formattedText);
    }

    fontSetCurrent(oldFont);
}

// 0x47304C
void inventoryWindowOpenContextMenu(int keyCode, int inventoryWindowType)
{
    Object* item;
    Object** v43;
    Object* v41;

    int v56 = sub_472B54(keyCode, &item, &v43, &v41);
    if (v56 == 0) {
        return;
    }

    int itemType = itemGetType(item);

    int mouseState;
    do {
        sub_4C8B78();

        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_NORMAL) {
            sub_470650(-1, INVENTORY_WINDOW_TYPE_NORMAL);
        }

        mouseState = mouseGetEvent();
        if ((mouseState & MOUSE_EVENT_LEFT_BUTTON_UP) != 0) {
            if (inventoryWindowType != INVENTORY_WINDOW_TYPE_NORMAL) {
                sub_49AC4C(off_59E86C[0], item, gInventoryPrintItemDescriptionHandler);
            } else {
                inventoryExamineItem(off_59E86C[0], item);
            }
            windowRefresh(gInventoryWindow);
            return;
        }
    } while ((mouseState & MOUSE_EVENT_LEFT_BUTTON_DOWN_REPEAT) != MOUSE_EVENT_LEFT_BUTTON_DOWN_REPEAT);

    inventorySetCursor(INVENTORY_WINDOW_CURSOR_BLANK);

    unsigned char* windowBuffer = windowGetBuffer(gInventoryWindow);

    int x;
    int y;
    mouseGetPosition(&x, &y);

    int actionMenuItemsLength;
    const int* actionMenuItems;
    if (itemType == ITEM_TYPE_WEAPON && sub_478EF4(item)) {
        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_NORMAL && objectGetOwner(item) != gDude) {
            actionMenuItemsLength = 3;
            actionMenuItems = dword_519154;
        } else {
            actionMenuItemsLength = 4;
            actionMenuItems = dword_519144;
        }
    } else {
        if (inventoryWindowType != INVENTORY_WINDOW_TYPE_NORMAL) {
            if (objectGetOwner(item) != gDude) {
                if (itemType == ITEM_TYPE_CONTAINER) {
                    actionMenuItemsLength = 3;
                    actionMenuItems = dword_519130;
                } else {
                    actionMenuItemsLength = 2;
                    actionMenuItems = dword_51913C;
                }
            } else {
                if (itemType == ITEM_TYPE_CONTAINER) {
                    actionMenuItemsLength = 4;
                    actionMenuItems = dword_519114;
                } else {
                    actionMenuItemsLength = 3;
                    actionMenuItems = dword_519124;
                }
            }
        } else {
            if (itemType == ITEM_TYPE_CONTAINER && v43 != NULL) {
                actionMenuItemsLength = 3;
                actionMenuItems = dword_519124;
            } else {
                if (sub_48B24C(item) || sub_49E9DC(item->pid)) {
                    actionMenuItemsLength = 4;
                    actionMenuItems = dword_519114;
                } else {
                    actionMenuItemsLength = 3;
                    actionMenuItems = dword_519124;
                }
            }
        }
    }

    const InventoryWindowDescription* windowDescription = &(gInventoryWindowDescriptions[inventoryWindowType]);
    gameMouseRenderActionMenuItems(x, y, actionMenuItems, actionMenuItemsLength,
        windowDescription->width + windowDescription->x,
        windowDescription->height + windowDescription->y);

    InventoryCursorData* cursorData = &(gInventoryCursorData[INVENTORY_WINDOW_CURSOR_MENU]);

    int offsetX;
    int offsetY;
    artGetRotationOffsets(cursorData->frm, 0, &offsetX, &offsetY);

    Rect rect;
    rect.left = x - windowDescription->x - cursorData->width / 2 + offsetX;
    rect.top = y - windowDescription->y - cursorData->height + 1 + offsetY;
    rect.right = rect.left + cursorData->width - 1;
    rect.bottom = rect.top + cursorData->height - 1;

    int menuButtonHeight = cursorData->height;
    if (rect.top + menuButtonHeight > windowDescription->height) {
        menuButtonHeight = windowDescription->height - rect.top;
    }

    int btn = buttonCreate(gInventoryWindow,
        rect.left,
        rect.top,
        cursorData->width,
        menuButtonHeight,
        -1,
        -1,
        -1,
        -1,
        cursorData->frmData,
        cursorData->frmData,
        0,
        BUTTON_FLAG_TRANSPARENT);
    windowRefreshRect(gInventoryWindow, &rect);

    int menuItemIndex = 0;
    int previousMouseY = y;
    while ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_UP) == 0) {
        sub_4C8B78();

        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_NORMAL) {
            sub_470650(-1, INVENTORY_WINDOW_TYPE_NORMAL);
        }

        int x;
        int y;
        mouseGetPosition(&x, &y);
        if (y - previousMouseY > 10 || previousMouseY - y > 10) {
            if (y >= previousMouseY || menuItemIndex <= 0) {
                if (previousMouseY < y && menuItemIndex < actionMenuItemsLength - 1) {
                    menuItemIndex++;
                }
            } else {
                menuItemIndex--;
            }
            gameMouseHighlightActionMenuItemAtIndex(menuItemIndex);
            windowRefreshRect(gInventoryWindow, &rect);
            previousMouseY = y;
        }
    }

    buttonDestroy(btn);

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        unsigned char* src = windowGetBuffer(dword_59E97C);
        int pitch = stru_6AC9F0.right - stru_6AC9F0.left + 1;
        blitBufferToBuffer(src + pitch * rect.top + rect.left + 80,
            cursorData->width,
            menuButtonHeight,
            pitch,
            windowBuffer + windowDescription->width * rect.top + rect.left,
            windowDescription->width);
    } else {
        int backgroundFid = buildFid(6, windowDescription->field_0, 0, 0, 0);
        CacheEntry* backgroundFrmHandle;
        unsigned char* backgroundFrmData = artLockFrameData(backgroundFid, 0, 0, &backgroundFrmHandle);
        blitBufferToBuffer(backgroundFrmData + windowDescription->width * rect.top + rect.left,
            cursorData->width,
            menuButtonHeight,
            windowDescription->width,
            windowBuffer + windowDescription->width * rect.top + rect.left,
            windowDescription->width);
        artUnlock(backgroundFrmHandle);
    }

    sub_4CAA04(x, y);

    sub_46FDF4(dword_59E844[dword_59E96C], -1, inventoryWindowType);

    int actionMenuItem = actionMenuItems[menuItemIndex];
    switch (actionMenuItem) {
    case GAME_MOUSE_ACTION_MENU_ITEM_DROP:
        if (v43 != NULL) {
            if (v43 == &gInventoryArmor) {
                sub_4715F8(off_59E86C[0], item, NULL);
            }
            itemAdd(v41, item, 1);
            v56 = 1;
            *v43 = NULL;
        }

        if (item->pid == PROTO_ID_MONEY) {
            if (v56 > 1) {
                v56 = inventoryQuantitySelect(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, item, v56);
            } else {
                v56 = 1;
            }

            if (v56 > 0) {
                if (v56 == 1) {
                    itemSetMoney(item, 1);
                } else {
                    if (itemRemove(v41, item, v56 - 1) != 0) {
                        Object* a2;
                        if (sub_472B54(keyCode, &a2, &v43, &v41) == 0) {
                            itemSetMoney(a2, v56);
                            sub_49B8B0(a2, v41);
                        } else {
                            itemAdd(v41, item, v56 - 1);
                        }
                    }
                }
            }
        } else if (item->pid == PROTO_ID_DYNAMITE_II || item->pid == PROTO_ID_PLASTIC_EXPLOSIVES_II) {
            dword_5190E0 = 1;
            sub_49B8B0(v41, item);
        } else {
            if (v56 > 1) {
                v56 = inventoryQuantitySelect(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, item, v56);

                for (int index = 0; index < v56; index++) {
                    if (sub_472B54(keyCode, &item, &v43, &v41) != 0) {
                        sub_49B8B0(v41, item);
                    }
                }
            } else {
                sub_49B8B0(v41, item);
            }
        }
        break;
    case GAME_MOUSE_ACTION_MENU_ITEM_LOOK:
        if (inventoryWindowType != INVENTORY_WINDOW_TYPE_NORMAL) {
            sub_49AD88(off_59E86C[0], item, gInventoryPrintItemDescriptionHandler);
        } else {
            inventoryExamineItem(off_59E86C[0], item);
        }
        break;
    case GAME_MOUSE_ACTION_MENU_ITEM_USE:
        switch (itemType) {
        case ITEM_TYPE_CONTAINER:
            sub_47620C(keyCode, inventoryWindowType);
            break;
        case ITEM_TYPE_DRUG:
            if (sub_479F60(off_59E86C[0], item)) {
                if (v43 != NULL) {
                    *v43 = NULL;
                } else {
                    itemRemove(v41, item, 1);
                }

                sub_489EC4(item, gDude->tile, gDude->elevation, NULL);
                sub_49B9A0(item);
            }
            interfaceRenderHitPoints(true);
            break;
        case ITEM_TYPE_WEAPON:
        case ITEM_TYPE_MISC:
            if (v43 == NULL) {
                itemRemove(v41, item, 1);
            }

            int v21;
            if (sub_48B24C(item)) {
                v21 = sub_49BF38(off_59E86C[0], item);
            } else {
                v21 = sub_49C3CC(off_59E86C[0], off_59E86C[0], item);
            }

            if (v21 == 1) {
                if (v43 != NULL) {
                    *v43 = NULL;
                }

                sub_489EC4(item, gDude->tile, gDude->elevation, NULL);
                sub_49B9A0(item);
            } else {
                if (v43 == NULL) {
                    itemAdd(v41, item, 1);
                }
            }
        }
        break;
    case GAME_MOUSE_ACTION_MENU_ITEM_UNLOAD:
        if (v43 == NULL) {
            itemRemove(v41, item, 1);
        }

        for (;;) {
            Object* v21 = sub_478F80(item);
            if (v21 != NULL) {
            }

            Rect rect;
            sub_489F34(v21, &rect);
            itemAdd(v41, v21, 1);
        }

        if (v43 == NULL) {
            itemAdd(v41, item, 1);
        }
        break;
    default:
        break;
    }

    inventorySetCursor(INVENTORY_WINDOW_CURSOR_ARROW);

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_NORMAL && actionMenuItem != GAME_MOUSE_ACTION_MENU_ITEM_LOOK) {
        inventoryRenderSummary();
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_LOOT
        || inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, inventoryWindowType);
    }

    sub_46FDF4(dword_59E844[dword_59E96C], -1, inventoryWindowType);

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_TRADE) {
        inventoryWindowRenderInnerInventories(dword_59E97C, off_59E934, off_59E944, -1);
    }

    sub_4716E8();
}

// 0x473904
int inventoryOpenLooting(Object* a1, Object* a2)
{
    int arrowFrmIds[INVENTORY_ARROW_FRM_COUNT];
    CacheEntry* arrowFrmHandles[INVENTORY_ARROW_FRM_COUNT];
    MessageListItem messageListItem;

    static_assert(sizeof(arrowFrmIds) == sizeof(gInventoryArrowFrmIds), "wrong size");
    memcpy(arrowFrmIds, gInventoryArrowFrmIds, sizeof(gInventoryArrowFrmIds));

    if (a1 != off_519058) {
        return 0;
    }

    if (((a2->fid & 0xF000000) >> 24) == OBJ_TYPE_CRITTER) {
        if (sub_42E6AC(a2->pid, 0x20)) {
            // You can't find anything to take from that.
            messageListItem.num = 50;
            if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                displayMonitorAddMessage(messageListItem.text);
            }
            return 0;
        }
    }

    if (((a2->fid & 0xF000000) >> 24) == OBJ_TYPE_ITEM) {
        if (itemGetType(a2) == ITEM_TYPE_CONTAINER) {
            if (a2->frame == 0) {
                CacheEntry* handle;
                Art* frm = artLock(a2->fid, &handle);
                if (frm != NULL) {
                    int frameCount = artGetFrameCount(frm);
                    artUnlock(handle);
                    if (frameCount > 1) {
                        return 0;
                    }
                }
            }
        }
    }

    int sid = -1;
    if (!dword_51D430) {
        if (sub_49A9A0(a2, &sid) != -1) {
            scriptSetObjects(sid, a1, NULL);
            scriptExecProc(sid, SCRIPT_PROC_PICKUP);

            Script* script;
            if (scriptGetScript(sid, &script) != -1) {
                if (script->scriptOverrides) {
                    return 0;
                }
            }
        }
    }

    if (inventoryCommonInit() == -1) {
        return 0;
    }

    off_59E978 = &(a2->data.inventory);
    dword_59E948 = 0;
    dword_59E7EC[0] = 0;
    off_59E81C[0] = a2;

    Object* a1a = NULL;
    if (objectCreateWithFidPid(&a1a, 0, 467) == -1) {
        return 0;
    }

    sub_4776E0(a2, a1a);

    Object* item1 = NULL;
    Object* item2 = NULL;
    Object* armor = NULL;

    if (dword_51D430) {
        item1 = critterGetItem1(a2);
        if (item1 != NULL) {
            itemRemove(a2, item1, 1);
        }

        item2 = critterGetItem2(a2);
        if (item2 != NULL) {
            itemRemove(a2, item2, 1);
        }

        armor = critterGetArmor(a2);
        if (armor != NULL) {
            itemRemove(a2, armor, 1);
        }
    }

    bool isoWasEnabled = sub_46EC90(INVENTORY_WINDOW_TYPE_LOOT);

    Object** critters = NULL;
    int critterCount = 0;
    int critterIndex = 0;
    if (!dword_51D430) {
        if ((a2->fid & 0xF000000) >> 24 == OBJ_TYPE_CRITTER) {
            critterCount = objectListCreate(a2->tile, a2->elevation, OBJ_TYPE_CRITTER, &critters);
            int endIndex = critterCount - 1;
            for (int index = 0; index < critterCount; index++) {
                Object* critter = critters[index];
                if ((critter->data.critter.combat.results & (DAM_DEAD | DAM_KNOCKED_OUT)) != 0) {
                    critters[index] = critters[endIndex];
                    critters[endIndex] = critter;
                    critterCount--;
                    index--;
                    endIndex--;
                } else {
                    critterIndex++;
                }
            }

            if (critterCount == 1) {
                objectListFree(critters);
                critterCount = 0;
            }

            if (critterCount > 1) {
                int fid;
                unsigned char* buttonUpData;
                unsigned char* buttonDownData;
                int btn;

                __stosd((unsigned long*)arrowFrmHandles, (unsigned long)INVALID_CACHE_ENTRY, INVENTORY_ARROW_FRM_COUNT);

                // Setup left arrow button.
                fid = buildFid(6, arrowFrmIds[INVENTORY_ARROW_FRM_LEFT_ARROW_UP], 0, 0, 0);
                buttonUpData = artLockFrameData(fid, 0, 0, &(arrowFrmHandles[INVENTORY_ARROW_FRM_LEFT_ARROW_UP]));

                fid = buildFid(6, arrowFrmIds[INVENTORY_ARROW_FRM_LEFT_ARROW_DOWN], 0, 0, 0);
                buttonDownData = artLockFrameData(fid, 0, 0, &(arrowFrmHandles[INVENTORY_ARROW_FRM_LEFT_ARROW_DOWN]));

                if (buttonUpData != NULL && buttonDownData != NULL) {
                    btn = buttonCreate(gInventoryWindow, 436, 162, 20, 18, -1, -1, KEY_PAGE_UP, -1, buttonUpData, buttonDownData, NULL, 0);
                    if (btn != -1) {
                        buttonSetCallbacks(btn, sub_451970, sub_451978);
                    }
                }

                // Setup right arrow button.
                fid = buildFid(6, arrowFrmIds[INVENTORY_ARROW_FRM_RIGHT_ARROW_UP], 0, 0, 0);
                buttonUpData = artLockFrameData(fid, 0, 0, &(arrowFrmHandles[INVENTORY_ARROW_FRM_RIGHT_ARROW_UP]));

                fid = buildFid(6, arrowFrmIds[INVENTORY_ARROW_FRM_RIGHT_ARROW_DOWN], 0, 0, 0);
                buttonDownData = artLockFrameData(fid, 0, 0, &(arrowFrmHandles[INVENTORY_ARROW_FRM_RIGHT_ARROW_DOWN]));

                if (buttonUpData != NULL && buttonDownData != NULL) {
                    btn = buttonCreate(gInventoryWindow, 456, 162, 20, 18, -1, -1, KEY_PAGE_DOWN, -1, buttonUpData, buttonDownData, NULL, 0);
                    if (btn != -1) {
                        buttonSetCallbacks(btn, sub_451970, sub_451978);
                    }
                }

                for (int index = 0; index < critterCount; index++) {
                    if (a2 == critters[index]) {
                        critterIndex = index;
                    }
                }
            }
        }
    }

    sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, INVENTORY_WINDOW_TYPE_LOOT);
    sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_LOOT);
    sub_470650(a2->fid, INVENTORY_WINDOW_TYPE_LOOT);
    inventorySetCursor(INVENTORY_WINDOW_CURSOR_HAND);

    bool isCaughtStealing = false;
    int stealingXp = 0;
    for (;;) {
        if (dword_5186CC != 0) {
            break;
        }

        if (isCaughtStealing) {
            break;
        }

        int keyCode = sub_4C8B78();

        if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
            showQuitConfirmationDialog();
        }

        if (dword_5186CC != 0) {
            break;
        }

        if (keyCode == KEY_UPPERCASE_A) {
            if (!dword_51D430) {
                int maxCarryWeight = critterGetStat(a1, STAT_CARRY_WEIGHT);
                int currentWeight = objectGetInventoryWeight(a1);
                int newInventoryWeight = objectGetInventoryWeight(a2);
                if (newInventoryWeight <= maxCarryWeight - currentWeight) {
                    sub_4776AC(a2, a1);
                    sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, INVENTORY_WINDOW_TYPE_LOOT);
                    sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_LOOT);
                } else {
                    // Sorry, you cannot carry that much.
                    messageListItem.num = 31;
                    if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                        showDialogBox(messageListItem.text, NULL, 0, 169, 117, byte_6A38D0[32328], NULL, byte_6A38D0[32328], 0);
                    }
                }
            }
        } else if (keyCode == KEY_ARROW_UP) {
            if (dword_59E844[dword_59E96C] > 0) {
                dword_59E844[dword_59E96C] -= 1;
                sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_LOOT);
            }
        } else if (keyCode == KEY_PAGE_UP) {
            if (critterCount != 0) {
                if (critterIndex > 0) {
                    critterIndex -= 1;
                } else {
                    critterIndex = critterCount - 1;
                }

                off_59E978 = &(a2->data.inventory);
                off_59E81C[0] = a2;
                dword_59E948 = 0;
                dword_59E7EC[0] = 0;
                sub_47036C(0, -1, off_59E978, INVENTORY_WINDOW_TYPE_LOOT);
                sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_LOOT);
                sub_470650(a2->fid, INVENTORY_WINDOW_TYPE_LOOT);
            }
        } else if (keyCode == KEY_ARROW_DOWN) {
            if (dword_59E844[dword_59E96C] + gInventorySlotsCount < off_59E960->length) {
                dword_59E844[dword_59E96C] += 1;
                sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_LOOT);
            }
        } else if (keyCode == KEY_PAGE_DOWN) {
            if (critterCount != 0) {
                if (critterIndex < critterCount - 1) {
                    critterIndex += 1;
                } else {
                    critterIndex = 0;
                }

                off_59E978 = &(a2->data.inventory);
                off_59E81C[0] = a2;
                dword_59E948 = 0;
                dword_59E7EC[0] = 0;
                sub_47036C(0, -1, off_59E978, INVENTORY_WINDOW_TYPE_LOOT);
                sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_LOOT);
                sub_470650(a2->fid, INVENTORY_WINDOW_TYPE_LOOT);
            }
        } else if (keyCode == KEY_CTRL_ARROW_UP) {
            if (dword_59E7EC[dword_59E948] > 0) {
                dword_59E7EC[dword_59E948] -= 1;
                sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, INVENTORY_WINDOW_TYPE_LOOT);
                windowRefresh(gInventoryWindow);
            }
        } else if (keyCode == KEY_CTRL_ARROW_DOWN) {
            if (dword_59E7EC[dword_59E948] + gInventorySlotsCount < off_59E978->length) {
                dword_59E7EC[dword_59E948] += 1;
                sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, INVENTORY_WINDOW_TYPE_LOOT);
                windowRefresh(gInventoryWindow);
            }
        } else if (keyCode >= 2500 && keyCode <= 2501) {
            sub_476394(keyCode, INVENTORY_WINDOW_TYPE_LOOT);
        } else {
            if ((mouseGetEvent() & MOUSE_EVENT_RIGHT_BUTTON_DOWN) != 0) {
                if (gInventoryCursor == INVENTORY_WINDOW_CURSOR_HAND) {
                    inventorySetCursor(INVENTORY_WINDOW_CURSOR_ARROW);
                } else {
                    inventorySetCursor(INVENTORY_WINDOW_CURSOR_HAND);
                }
            } else if ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_DOWN) != 0) {
                if (keyCode >= 1000 && keyCode <= 1000 + gInventorySlotsCount) {
                    if (gInventoryCursor == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inventoryWindowOpenContextMenu(keyCode, INVENTORY_WINDOW_TYPE_LOOT);
                    } else {
                        int v40 = keyCode - 1000;
                        if (v40 + dword_59E844[dword_59E96C] < off_59E960->length) {
                            dword_51D434 += 1;
                            dword_51D438 += itemGetSize(off_59E86C[dword_59E96C]);

                            InventoryItem* inventoryItem = &(off_59E960->items[off_59E960->length - (v40 + dword_59E844[dword_59E96C] + 1)]);
                            int rc = sub_474708(inventoryItem->item, v40, off_59E81C[dword_59E948], true);
                            if (rc == 1) {
                                isCaughtStealing = true;
                            } else if (rc == 2) {
                                stealingXp += 10;
                            }

                            sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, INVENTORY_WINDOW_TYPE_LOOT);
                            sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_LOOT);
                        }

                        keyCode = -1;
                    }
                } else if (keyCode >= 2000 && keyCode <= 2000 + gInventorySlotsCount) {
                    if (gInventoryCursor == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inventoryWindowOpenContextMenu(keyCode, INVENTORY_WINDOW_TYPE_LOOT);
                    } else {
                        int v46 = keyCode - 2000;
                        if (v46 + dword_59E7EC[dword_59E948] < off_59E978->length) {
                            dword_51D434 += 1;
                            dword_51D438 += itemGetSize(off_59E86C[dword_59E96C]);

                            InventoryItem* inventoryItem = &(off_59E978->items[off_59E978->length - (v46 + dword_59E7EC[dword_59E948] + 1)]);
                            int rc = sub_474708(inventoryItem->item, v46, off_59E81C[dword_59E948], false);
                            if (rc == 1) {
                                isCaughtStealing = true;
                            } else if (rc == 2) {
                                stealingXp += 10;
                            }

                            sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, INVENTORY_WINDOW_TYPE_LOOT);
                            sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_LOOT);
                        }
                    }
                }
            }
        }

        if (keyCode == KEY_ESCAPE) {
            break;
        }
    }

    if (critterCount != 0) {
        objectListFree(critters);

        for (int index = 0; index < INVENTORY_ARROW_FRM_COUNT; index++) {
            artUnlock(arrowFrmHandles[index]);
        }
    }

    if (dword_51D430) {
        if (item1 != NULL) {
            item1->flags |= 0x1000000;
            itemAdd(a2, item1, 1);
        }

        if (item2 != NULL) {
            item2->flags |= 0x2000000;
            itemAdd(a2, item2, 1);
        }

        if (armor != NULL) {
            armor->flags |= 0x4000000;
            itemAdd(a2, armor, 1);
        }
    }

    sub_4776AC(a1a, a2);
    objectDestroy(a1a, NULL);

    if (dword_51D430) {
        if (!isCaughtStealing) {
            if (stealingXp > 0) {
                if (!objectIsPartyMember(a2)) {
                    stealingXp = min(300 - skillGetValue(a1, SKILL_STEAL), stealingXp);
                    debugPrint("\n[[[%d]]]", 300 - skillGetValue(a1, SKILL_STEAL));

                    // You gain %d experience points for successfully using your Steal skill.
                    messageListItem.num = 29;
                    if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                        char formattedText[200];
                        sprintf(formattedText, messageListItem.text, stealingXp);
                        displayMonitorAddMessage(formattedText);
                    }

                    pcAddExperience(stealingXp);
                }
            }
        }
    }

    sub_46FBD8(isoWasEnabled);

    // NOTE: Uninline.
    inventoryCommonFree();

    if (dword_51D430) {
        if (isCaughtStealing) {
            if (dword_51D434 > 0) {
                if (sub_49A9A0(a2, &sid) != -1) {
                    scriptSetObjects(sid, a1, NULL);
                    scriptExecProc(sid, SCRIPT_PROC_PICKUP);

                    // TODO: Looks like inlining, script is not used.
                    Script* script;
                    scriptGetScript(sid, &script);
                }
            }
        }
    }

    return 0;
}

// 0x4746A0
int inventoryOpenStealing(Object* a1, Object* a2)
{
    if (a1 == a2) {
        return -1;
    }

    dword_51D430 = (a1->pid >> 24) == OBJ_TYPE_CRITTER && critterIsActive(a2);
    dword_51D434 = 0;
    dword_51D438 = 0;

    int rc = inventoryOpenLooting(a1, a2);

    dword_51D430 = 0;
    dword_51D434 = 0;
    dword_51D438 = 0;

    return rc;
}

// 0x474708
int sub_474708(Object* a1, int a2, Object* a3, bool a4)
{
    bool v38 = true;

    Rect rect;

    int quantity;
    if (a4) {
        rect.left = 176;
        rect.top = 48 * a2 + 37;

        InventoryItem* inventoryItem = &(off_59E960->items[off_59E960->length - (a2 + dword_59E844[dword_59E96C] + 1)]);
        quantity = inventoryItem->quantity;
        if (quantity > 1) {
            sub_46FDF4(dword_59E844[dword_59E96C], a2, INVENTORY_WINDOW_TYPE_LOOT);
            v38 = false;
        }
    } else {
        rect.left = 297;
        rect.top = 48 * a2 + 37;

        InventoryItem* inventoryItem = &(off_59E978->items[off_59E978->length - (a2 + dword_59E7EC[dword_59E948] + 1)]);
        quantity = inventoryItem->quantity;
        if (quantity > 1) {
            sub_47036C(dword_59E7EC[dword_59E948], a2, off_59E978, INVENTORY_WINDOW_TYPE_LOOT);
            windowRefresh(gInventoryWindow);
            v38 = false;
        }
    }

    if (v38) {
        unsigned char* windowBuffer = windowGetBuffer(gInventoryWindow);

        CacheEntry* handle;
        int fid = buildFid(6, 114, 0, 0, 0);
        unsigned char* data = artLockFrameData(fid, 0, 0, &handle);
        if (data != NULL) {
            blitBufferToBuffer(data + 537 * rect.top + rect.left, 64, 48, 537, windowBuffer + 537 * rect.top + rect.left, 537);
            artUnlock(handle);
        }

        rect.right = rect.left + 64 - 1;
        rect.bottom = rect.top + 48 - 1;
        windowRefreshRect(gInventoryWindow, &rect);
    }

    CacheEntry* inventoryFrmHandle;
    int inventoryFid = itemGetInventoryFid(a1);
    Art* inventoryFrm = artLock(inventoryFid, &inventoryFrmHandle);
    if (inventoryFrm != NULL) {
        int width = artGetWidth(inventoryFrm, 0, 0);
        int height = artGetHeight(inventoryFrm, 0, 0);
        unsigned char* data = artGetFrameData(inventoryFrm, 0, 0);
        mouseSetFrame(data, width, height, width, width / 2, height / 2, 0);
        soundPlayFile("ipickup1");
    }

    do {
        sub_4C8B78();
    } while ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0);

    if (inventoryFrm != NULL) {
        artUnlock(inventoryFrmHandle);
        soundPlayFile("iputdown");
    }

    int rc = 0;
    MessageListItem messageListItem;

    if (a4) {
        if (sub_4CA934(377, 37, 441, 48 * gInventorySlotsCount + 37)) {
            int quantityToMove;
            if (quantity > 1) {
                quantityToMove = inventoryQuantitySelect(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, a1, quantity);
            } else {
                quantityToMove = 1;
            }

            if (quantityToMove != -1) {
                if (dword_51D430) {
                    if (skillsPerformStealing(off_519058, a3, a1, true) == 0) {
                        rc = 1;
                    }
                }

                if (rc != 1) {
                    if (sub_47769C(off_519058, a3, a1, quantityToMove) != -1) {
                        rc = 2;
                    } else {
                        // There is no space left for that item.
                        messageListItem.num = 26;
                        if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                            displayMonitorAddMessage(messageListItem.text);
                        }
                    }
                }
            }
        }
    } else {
        if (sub_4CA934(256, 37, 320, 48 * gInventorySlotsCount + 37)) {
            int quantityToMove;
            if (quantity > 1) {
                quantityToMove = inventoryQuantitySelect(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, a1, quantity);
            } else {
                quantityToMove = 1;
            }

            if (quantityToMove != -1) {
                if (dword_51D430) {
                    if (skillsPerformStealing(off_519058, a3, a1, false) == 0) {
                        rc = 1;
                    }
                }

                if (rc != 1) {
                    if (sub_47769C(a3, off_519058, a1, quantityToMove) == 0) {
                        if ((a1->flags & 0x2000000) != 0) {
                            a3->fid = buildFid((a3->fid & 0xF000000) >> 24, a3->fid & 0xFFF, (a3->fid & 0xFF0000) >> 16, 0, a3->rotation + 1);
                        }

                        a3->flags &= ~0x7000000;

                        rc = 2;
                    } else {
                        // You cannot pick that up. You are at your maximum weight capacity.
                        messageListItem.num = 25;
                        if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                            displayMonitorAddMessage(messageListItem.text);
                        }
                    }
                }
            }
        }
    }

    inventorySetCursor(INVENTORY_WINDOW_CURSOR_HAND);

    return rc;
}

// 0x474B2C
int sub_474B2C(Object* a1, Object* a2)
{
    if (gGameDialogSpeakerIsPartyMember) {
        return objectGetInventoryWeight(off_59E944);
    }

    int cost = objectGetCost(off_59E944);
    int caps = itemGetTotalCaps(off_59E944);
    int v14 = cost - caps;

    double bonus = 0.0;
    if (a1 == gDude) {
        if (perkHasRank(gDude, PERK_MASTER_TRADER)) {
            bonus = 25.0;
        }
    }

    int partyBarter = partyGetBestSkillValue(SKILL_BARTER);
    int npcBarter = skillGetValue(a2, SKILL_BARTER);

    // TODO: Check in debugger, complex math, probably uses floats, not doubles.
    double v1 = (dword_59E898 + 100.0 - bonus) * 0.01;
    double v2 = (160.0 + npcBarter) / (160.0 + partyBarter) * (v14 * 2.0);
    if (v1 < 0) {
        // TODO: Probably 0.01 as float.
        v1 = 0.0099999998;
    }

    int rounded = (int)(v1 * v2 + caps);
    return rounded;
}

// 0x474C50
int sub_474C50(Object* a1, Object* a2, Object* a3, Object* a4)
{
    MessageListItem messageListItem;

    int v8 = critterGetStat(a1, STAT_CARRY_WEIGHT) - objectGetInventoryWeight(a1);
    if (objectGetInventoryWeight(a4) > v8) {
        // Sorry, you cannot carry that much.
        messageListItem.num = 31;
        if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
            gameDialogRenderSupplementaryMessage(messageListItem.text);
        }
        return -1;
    }

    if (gGameDialogSpeakerIsPartyMember) {
        int v10 = critterGetStat(a3, STAT_CARRY_WEIGHT) - objectGetInventoryWeight(a3);
        if (objectGetInventoryWeight(a2) > v10) {
            // Sorry, that's too much to carry.
            messageListItem.num = 32;
            if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                gameDialogRenderSupplementaryMessage(messageListItem.text);
            }
            return -1;
        }
    } else {
        bool v11 = false;
        if (a2->data.inventory.length == 0) {
            v11 = true;
        } else {
            if (sub_4780E4(a2)) {
                if (a2->pid != PROTO_ID_GEIGER_COUNTER_I || miscItemTurnOff(a2) == -1) {
                    v11 = true;
                }
            }
        }

        if (!v11) {
            int cost = objectGetCost(a2);
            if (sub_474B2C(a1, a3) > cost) {
                v11 = true;
            }
        }

        if (v11) {
            // No, your offer is not good enough.
            messageListItem.num = 28;
            if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                gameDialogRenderSupplementaryMessage(messageListItem.text);
            }
            return -1;
        }
    }

    sub_4776AC(a4, a1);
    sub_4776AC(a2, a3);
    return 0;
}

// 0x474DAC
void sub_474DAC(Object* a1, int quantity, int a3, int a4, Object* a5, Object* a6, bool a7)
{
    Rect rect;
    if (a7) {
        rect.left = 23;
        rect.top = 48 * a3 + 34;
    } else {
        rect.left = 395;
        rect.top = 48 * a3 + 31;
    }

    if (quantity > 1) {
        if (a7) {
            sub_46FDF4(a4, a3, INVENTORY_WINDOW_TYPE_TRADE);
        } else {
            sub_47036C(a4, a3, off_59E978, INVENTORY_WINDOW_TYPE_TRADE);
        }
    } else {
        unsigned char* dest = windowGetBuffer(gInventoryWindow);
        unsigned char* src = windowGetBuffer(dword_59E97C);

        int pitch = stru_6AC9F0.right - stru_6AC9F0.left + 1;
        blitBufferToBuffer(src + pitch * rect.top + rect.left + 80, 64, 48, pitch, dest + 480 * rect.top + rect.left, 480);

        rect.right = rect.left + 64 - 1;
        rect.bottom = rect.top + 48 - 1;
        windowRefreshRect(gInventoryWindow, &rect);
    }

    CacheEntry* inventoryFrmHandle;
    int inventoryFid = itemGetInventoryFid(a1);
    Art* inventoryFrm = artLock(inventoryFid, &inventoryFrmHandle);
    if (inventoryFrm != NULL) {
        int width = artGetWidth(inventoryFrm, 0, 0);
        int height = artGetHeight(inventoryFrm, 0, 0);
        unsigned char* data = artGetFrameData(inventoryFrm, 0, 0);
        mouseSetFrame(data, width, height, width, width / 2, height / 2, 0);
        soundPlayFile("ipickup1");
    }

    do {
        sub_4C8B78();
    } while ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0);

    if (inventoryFrm != NULL) {
        artUnlock(inventoryFrmHandle);
        soundPlayFile("iputdown");
    }

    MessageListItem messageListItem;

    if (a7) {
        if (sub_4CA934(245, 310, 309, 48 * gInventorySlotsCount + 310)) {
            int quantityToMove = quantity > 1 ? inventoryQuantitySelect(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, a1, quantity) : 1;
            if (quantityToMove != -1) {
                if (sub_4776A4(off_519058, a6, a1, quantityToMove) == -1) {
                    // There is no space left for that item.
                    messageListItem.num = 26;
                    if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                        displayMonitorAddMessage(messageListItem.text);
                    }
                }
            }
        }
    } else {
        if (sub_4CA934(330, 310, 394, 48 * gInventorySlotsCount + 310)) {
            int quantityToMove = quantity > 1 ? inventoryQuantitySelect(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, a1, quantity) : 1;
            if (quantityToMove != -1) {
                if (sub_4776A4(a5, a6, a1, quantityToMove) == -1) {
                    // You cannot pick that up. You are at your maximum weight capacity.
                    messageListItem.num = 25;
                    if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                        displayMonitorAddMessage(messageListItem.text);
                    }
                }
            }
        }
    }

    inventorySetCursor(INVENTORY_WINDOW_CURSOR_HAND);
}

// 0x475070
void sub_475070(Object* a1, int quantity, int a3, Object* a4, Object* a5, bool a6)
{
    Rect rect;
    if (a6) {
        rect.left = 169;
        rect.top = 48 * a3 + 24;
    } else {
        rect.left = 254;
        rect.top = 48 * a3 + 24;
    }

    if (quantity > 1) {
        if (a6) {
            inventoryWindowRenderInnerInventories(dword_59E97C, a5, NULL, a3);
        } else {
            inventoryWindowRenderInnerInventories(dword_59E97C, NULL, a5, a3);
        }
    } else {
        unsigned char* dest = windowGetBuffer(gInventoryWindow);
        unsigned char* src = windowGetBuffer(dword_59E97C);

        int pitch = stru_6AC9F0.right - stru_6AC9F0.left + 1;
        blitBufferToBuffer(src + pitch * rect.top + rect.left + 80, 64, 48, pitch, dest + 480 * rect.top + rect.left, 480);

        rect.right = rect.left + 64 - 1;
        rect.bottom = rect.top + 48 - 1;
        windowRefreshRect(gInventoryWindow, &rect);
    }

    CacheEntry* inventoryFrmHandle;
    int inventoryFid = itemGetInventoryFid(a1);
    Art* inventoryFrm = artLock(inventoryFid, &inventoryFrmHandle);
    if (inventoryFrm != NULL) {
        int width = artGetWidth(inventoryFrm, 0, 0);
        int height = artGetHeight(inventoryFrm, 0, 0);
        unsigned char* data = artGetFrameData(inventoryFrm, 0, 0);
        mouseSetFrame(data, width, height, width, width / 2, height / 2, 0);
        soundPlayFile("ipickup1");
    }

    do {
        sub_4C8B78();
    } while ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0);

    if (inventoryFrm != NULL) {
        artUnlock(inventoryFrmHandle);
        soundPlayFile("iputdown");
    }

    MessageListItem messageListItem;

    if (a6) {
        if (sub_4CA934(80, 310, 144, 48 * gInventorySlotsCount + 310)) {
            int quantityToMove = quantity > 1 ? inventoryQuantitySelect(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, a1, quantity) : 1;
            if (quantityToMove != -1) {
                if (sub_4776A4(a5, off_519058, a1, quantityToMove) == -1) {
                    // There is no space left for that item.
                    messageListItem.num = 26;
                    if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                        displayMonitorAddMessage(messageListItem.text);
                    }
                }
            }
        }
    } else {
        if (sub_4CA934(475, 310, 539, 48 * gInventorySlotsCount + 310)) {
            int quantityToMove = quantity > 1 ? inventoryQuantitySelect(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, a1, quantity) : 1;
            if (quantityToMove != -1) {
                if (sub_4776A4(a5, a4, a1, quantityToMove) == -1) {
                    // You cannot pick that up. You are at your maximum weight capacity.
                    messageListItem.num = 25;
                    if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                        displayMonitorAddMessage(messageListItem.text);
                    }
                }
            }
        }
    }

    inventorySetCursor(INVENTORY_WINDOW_CURSOR_HAND);
}

// 0x475334
void inventoryWindowRenderInnerInventories(int win, Object* a2, Object* a3, int a4)
{
    unsigned char* windowBuffer = windowGetBuffer(gInventoryWindow);

    int oldFont = fontGetCurrent();
    fontSetCurrent(101);

    char formattedText[80];
    int v45 = fontGetLineHeight() + 48 * gInventorySlotsCount;

    if (a2 != NULL) {
        unsigned char* src = windowGetBuffer(win);
        blitBufferToBuffer(src + (stru_6AC9F0.right - stru_6AC9F0.left + 1) * 20 + 249, 64, v45 + 1, stru_6AC9F0.right - stru_6AC9F0.left + 1, windowBuffer + 480 * 20 + 169, 480);

        unsigned char* dest = windowBuffer + 480 * 24 + 169;
        Inventory* inventory = &(a2->data.inventory);
        for (int index = 0; index < gInventorySlotsCount && index + dword_59E8A0 < inventory->length; index++) {
            InventoryItem* inventoryItem = &(inventory->items[inventory->length - (index + dword_59E8A0 + 1)]);
            int inventoryFid = itemGetInventoryFid(inventoryItem->item);
            artRender(inventoryFid, dest, 56, 40, 480);
            sub_4705A0(inventoryItem->item, inventoryItem->quantity, dest, 480, index == a4);

            dest += 480 * 48;
        }

        if (gGameDialogSpeakerIsPartyMember) {
            MessageListItem messageListItem;
            messageListItem.num = 30;

            if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                int weight = objectGetInventoryWeight(a2);
                sprintf(formattedText, "%s %d", messageListItem.text, weight);
            }
        } else {
            int cost = objectGetCost(a2);
            sprintf(formattedText, "$%d", cost);
        }

        fontDrawText(windowBuffer + 480 * (48 * gInventorySlotsCount + 24) + 169, formattedText, 80, 480, byte_6A38D0[32767]);

        Rect rect;
        rect.left = 169;
        rect.top = 24;
        rect.right = 223;
        rect.bottom = rect.top + v45;
        windowRefreshRect(gInventoryWindow, &rect);
    }

    if (a3 != NULL) {
        unsigned char* src = windowGetBuffer(win);
        blitBufferToBuffer(src + (stru_6AC9F0.right - stru_6AC9F0.left + 1) * 20 + 334, 64, v45 + 1, stru_6AC9F0.right - stru_6AC9F0.left + 1, windowBuffer + 480 * 20 + 254, 480);

        unsigned char* dest = windowBuffer + 480 * 24 + 254;
        Inventory* inventory = &(a3->data.inventory);
        for (int index = 0; index < gInventorySlotsCount && index + dword_59E89C < inventory->length; index++) {
            InventoryItem* inventoryItem = &(inventory->items[inventory->length - (index + dword_59E89C + 1)]);
            int inventoryFid = itemGetInventoryFid(inventoryItem->item);
            artRender(inventoryFid, dest, 56, 40, 480);
            sub_4705A0(inventoryItem->item, inventoryItem->quantity, dest, 480, index == a4);

            dest += 480 * 48;
        }

        if (gGameDialogSpeakerIsPartyMember) {
            MessageListItem messageListItem;
            messageListItem.num = 30;

            if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                int weight = sub_474B2C(gDude, off_59E81C[0]);
                sprintf(formattedText, "%s %d", messageListItem.text, weight);
            }
        } else {
            int cost = sub_474B2C(gDude, off_59E81C[0]);
            sprintf(formattedText, "$%d", cost);
        }

        fontDrawText(windowBuffer + 480 * (48 * gInventorySlotsCount + 24) + 254, formattedText, 80, 480, byte_6A38D0[32767]);

        Rect rect;
        rect.left = 254;
        rect.top = 24;
        rect.right = 318;
        rect.bottom = rect.top + v45;
        windowRefreshRect(gInventoryWindow, &rect);
    }

    fontSetCurrent(oldFont);
}

// 0x4757F0
void inventoryOpenTrade(int win, Object* a2, Object* a3, Object* a4, int a5)
{
    dword_59E898 = a5;

    if (inventoryCommonInit() == -1) {
        return;
    }

    Object* armor = critterGetArmor(a2);
    if (armor != NULL) {
        itemRemove(a2, armor, 1);
    }

    Object* item1 = NULL;
    Object* item2 = critterGetItem2(a2);
    if (item2 != NULL) {
        itemRemove(a2, item2, 1);
    } else {
        if (!gGameDialogSpeakerIsPartyMember) {
            item1 = sub_472698(a2, ITEM_TYPE_WEAPON, NULL);
            if (item1 != NULL) {
                itemRemove(a2, item1, 1);
            }
        }
    }

    Object* a1a = NULL;
    if (objectCreateWithFidPid(&a1a, 0, 467) == -1) {
        return;
    }

    off_59E960 = &(off_519058->data.inventory);
    off_59E944 = a4;
    off_59E934 = a3;

    dword_59E8A0 = 0;
    dword_59E89C = 0;

    off_59E8A4 = &(a3->data.inventory);
    off_59E94C = &(a4->data.inventory);

    dword_59E97C = win;
    dword_59E948 = 0;
    off_59E978 = &(a2->data.inventory);

    off_59E81C[0] = a2;
    dword_59E7EC[0] = 0;

    bool isoWasEnabled = sub_46EC90(INVENTORY_WINDOW_TYPE_TRADE);
    sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, INVENTORY_WINDOW_TYPE_TRADE);
    sub_46FDF4(dword_59E844[0], -1, INVENTORY_WINDOW_TYPE_TRADE);
    sub_470650(a2->fid, INVENTORY_WINDOW_TYPE_TRADE);
    windowRefresh(dword_59E97C);
    inventoryWindowRenderInnerInventories(win, a3, a4, -1);

    inventorySetCursor(INVENTORY_WINDOW_CURSOR_HAND);

    int modifier;
    int npcReactionValue = reactionGetValue(a2);
    int npcReactionType = reactionTranslateValue(npcReactionValue);
    switch (npcReactionType) {
    case NPC_REACTION_BAD:
        modifier = 25;
        break;
    case NPC_REACTION_NEUTRAL:
        modifier = 0;
        break;
    case NPC_REACTION_GOOD:
        modifier = -15;
        break;
    default:
        __assume(0);
    }

    int keyCode = -1;
    for (;;) {
        if (keyCode == KEY_ESCAPE || dword_5186CC != 0) {
            break;
        }

        keyCode = sub_4C8B78();
        if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
            showQuitConfirmationDialog();
        }

        if (dword_5186CC != 0) {
            break;
        }

        dword_59E898 = a5 + modifier;

        if (keyCode == KEY_LOWERCASE_T || modifier <= -30) {
            sub_4776AC(a4, a2);
            sub_4776AC(a3, gDude);
            sub_448268();
            break;
        } else if (keyCode == KEY_LOWERCASE_M) {
            if (a3->data.inventory.length != 0 || off_59E944->data.inventory.length != 0) {
                if (sub_474C50(off_519058, a3, a2, a4) == 0) {
                    sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, INVENTORY_WINDOW_TYPE_TRADE);
                    sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_TRADE);
                    inventoryWindowRenderInnerInventories(win, a3, a4, -1);

                    // Ok, that's a good trade.
                    MessageListItem messageListItem;
                    messageListItem.num = 27;
                    if (!gGameDialogSpeakerIsPartyMember) {
                        if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                            gameDialogRenderSupplementaryMessage(messageListItem.text);
                        }
                    }
                }
            }
        } else if (keyCode == KEY_ARROW_UP) {
            if (dword_59E844[dword_59E96C] > 0) {
                dword_59E844[dword_59E96C] -= 1;
                sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_TRADE);
            }
        } else if (keyCode == KEY_PAGE_UP) {
            if (dword_59E8A0 > 0) {
                dword_59E8A0 -= 1;
                inventoryWindowRenderInnerInventories(win, a3, a4, -1);
            }
        } else if (keyCode == KEY_ARROW_DOWN) {
            if (dword_59E844[dword_59E96C] + gInventorySlotsCount < off_59E960->length) {
                dword_59E844[dword_59E96C] += 1;
                sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_TRADE);
            }
        } else if (keyCode == KEY_PAGE_DOWN) {
            if (dword_59E8A0 + gInventorySlotsCount < off_59E8A4->length) {
                dword_59E8A0 += 1;
                inventoryWindowRenderInnerInventories(win, a3, a4, -1);
            }
        } else if (keyCode == KEY_CTRL_PAGE_DOWN) {
            if (dword_59E89C + gInventorySlotsCount < off_59E94C->length) {
                dword_59E89C++;
                inventoryWindowRenderInnerInventories(win, a3, a4, -1);
            }
        } else if (keyCode == KEY_CTRL_PAGE_UP) {
            if (dword_59E89C > 0) {
                dword_59E89C -= 1;
                inventoryWindowRenderInnerInventories(win, a3, a4, -1);
            }
        } else if (keyCode == KEY_CTRL_ARROW_UP) {
            if (dword_59E7EC[dword_59E948] > 0) {
                dword_59E7EC[dword_59E948] -= 1;
                sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, INVENTORY_WINDOW_TYPE_TRADE);
                windowRefresh(gInventoryWindow);
            }
        } else if (keyCode == KEY_CTRL_ARROW_DOWN) {
            if (dword_59E7EC[dword_59E948] + gInventorySlotsCount < off_59E978->length) {
                dword_59E7EC[dword_59E948] += 1;
                sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, INVENTORY_WINDOW_TYPE_TRADE);
                windowRefresh(gInventoryWindow);
            }
        } else if (keyCode >= 2500 && keyCode <= 2501) {
            sub_476394(keyCode, INVENTORY_WINDOW_TYPE_TRADE);
        } else {
            if ((mouseGetEvent() & MOUSE_EVENT_RIGHT_BUTTON_DOWN) != 0) {
                if (gInventoryCursor == INVENTORY_WINDOW_CURSOR_HAND) {
                    inventorySetCursor(INVENTORY_WINDOW_CURSOR_ARROW);
                } else {
                    inventorySetCursor(INVENTORY_WINDOW_CURSOR_HAND);
                }
            } else if ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_DOWN) != 0) {
                if (keyCode >= 1000 && keyCode <= 1000 + gInventorySlotsCount) {
                    if (gInventoryCursor == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inventoryWindowOpenContextMenu(keyCode, INVENTORY_WINDOW_TYPE_TRADE);
                        inventoryWindowRenderInnerInventories(win, a3, NULL, -1);
                    } else {
                        int v30 = keyCode - 1000;
                        if (v30 + dword_59E844[dword_59E96C] < off_59E960->length) {
                            int v31 = dword_59E844[dword_59E96C];
                            InventoryItem* inventoryItem = &(off_59E960->items[off_59E960->length - (v30 + v31 + 1)]);
                            sub_474DAC(inventoryItem->item, inventoryItem->quantity, v30, v31, a2, a3, true);
                            sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, INVENTORY_WINDOW_TYPE_TRADE);
                            sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_TRADE);
                            inventoryWindowRenderInnerInventories(win, a3, NULL, -1);
                        }
                    }

                    keyCode = -1;
                } else if (keyCode >= 2000 && keyCode <= 2000 + gInventorySlotsCount) {
                    if (gInventoryCursor == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inventoryWindowOpenContextMenu(keyCode, INVENTORY_WINDOW_TYPE_TRADE);
                        inventoryWindowRenderInnerInventories(win, NULL, a4, -1);
                    } else {
                        int v35 = keyCode - 2000;
                        if (v35 + dword_59E7EC[dword_59E948] < off_59E978->length) {
                            int v36 = dword_59E7EC[dword_59E948];
                            InventoryItem* inventoryItem = &(off_59E978->items[off_59E978->length - (v35 + v36 + 1)]);
                            sub_474DAC(inventoryItem->item, inventoryItem->quantity, v35, v36, a2, a4, false);
                            sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, INVENTORY_WINDOW_TYPE_TRADE);
                            sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_TRADE);
                            inventoryWindowRenderInnerInventories(win, NULL, a4, -1);
                        }
                    }

                    keyCode = -1;
                } else if (keyCode >= 2300 && keyCode <= 2300 + gInventorySlotsCount) {
                    if (gInventoryCursor == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inventoryWindowOpenContextMenu(keyCode, INVENTORY_WINDOW_TYPE_TRADE);
                        inventoryWindowRenderInnerInventories(win, a3, NULL, -1);
                    } else {
                        int v41 = keyCode - 2300;
                        if (v41 < off_59E8A4->length) {
                            InventoryItem* inventoryItem = &(off_59E8A4->items[off_59E8A4->length - (v41 + dword_59E8A0 + 1)]);
                            sub_475070(inventoryItem->item, inventoryItem->quantity, v41, a2, a3, true);
                            sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, INVENTORY_WINDOW_TYPE_TRADE);
                            sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_TRADE);
                            inventoryWindowRenderInnerInventories(win, a3, NULL, -1);
                        }
                    }

                    keyCode = -1;
                } else if (keyCode >= 2400 && keyCode <= 2400 + gInventorySlotsCount) {
                    if (gInventoryCursor == INVENTORY_WINDOW_CURSOR_ARROW) {
                        inventoryWindowOpenContextMenu(keyCode, INVENTORY_WINDOW_TYPE_TRADE);
                        inventoryWindowRenderInnerInventories(win, NULL, a4, -1);
                    } else {
                        int v45 = keyCode - 2400;
                        if (v45 < off_59E94C->length) {
                            InventoryItem* inventoryItem = &(off_59E94C->items[off_59E94C->length - (v45 + dword_59E89C + 1)]);
                            sub_475070(inventoryItem->item, inventoryItem->quantity, v45, a2, a4, false);
                            sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, INVENTORY_WINDOW_TYPE_TRADE);
                            sub_46FDF4(dword_59E844[dword_59E96C], -1, INVENTORY_WINDOW_TYPE_TRADE);
                            inventoryWindowRenderInnerInventories(win, NULL, a4, -1);
                        }
                    }

                    keyCode = -1;
                }
            }
        }
    }

    sub_4776AC(a1a, a2);
    objectDestroy(a1a, NULL);

    if (armor != NULL) {
        armor->flags |= 0x4000000;
        itemAdd(a2, armor, 1);
    }

    if (item2 != NULL) {
        item2->flags |= 0x2000000;
        itemAdd(a2, item2, 1);
    }

    if (item1 != NULL) {
        itemAdd(a2, item1, 1);
    }

    sub_46FBD8(isoWasEnabled);

    // NOTE: Uninline.
    inventoryCommonFree();
}

// 0x47620C
void sub_47620C(int keyCode, int inventoryWindowType)
{
    if (keyCode >= 2000) {
        int index = off_59E978->length - (dword_59E7EC[dword_59E948] + keyCode - 2000 + 1);
        if (index < off_59E978->length && dword_59E948 < 9) {
            InventoryItem* inventoryItem = &(off_59E978->items[index]);
            Object* item = inventoryItem->item;
            if (itemGetType(item) == ITEM_TYPE_CONTAINER) {
                dword_59E948 += 1;
                off_59E81C[dword_59E948] = item;
                dword_59E7EC[dword_59E948] = 0;

                off_59E978 = &(item->data.inventory);

                sub_470650(item->fid, inventoryWindowType);
                sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, inventoryWindowType);
                windowRefresh(gInventoryWindow);
            }
        }
    } else {
        int index = off_59E960->length - (dword_59E844[dword_59E96C] + keyCode - 1000 + 1);
        if (index < off_59E960->length && dword_59E96C < 9) {
            InventoryItem* inventoryItem = &(off_59E960->items[index]);
            Object* item = inventoryItem->item;
            if (itemGetType(item) == ITEM_TYPE_CONTAINER) {
                dword_59E96C += 1;

                off_59E86C[dword_59E96C] = item;
                dword_59E844[dword_59E96C] = 0;

                off_59E960 = &(item->data.inventory);

                sub_4716E8();
                sub_470650(-1, inventoryWindowType);
                sub_46FDF4(dword_59E844[dword_59E96C], -1, inventoryWindowType);
            }
        }
    }
}

// 0x476394
void sub_476394(int keyCode, int inventoryWindowType)
{
    if (keyCode == 2500) {
        if (dword_59E96C > 0) {
            dword_59E96C -= 1;
            off_519058 = off_59E86C[dword_59E96C];
            off_59E960 = &off_519058->data.inventory;
            sub_4716E8();
            sub_470650(-1, inventoryWindowType);
            sub_46FDF4(dword_59E844[dword_59E96C], -1, inventoryWindowType);
        }
    } else if (keyCode == 2501) {
        if (dword_59E948 > 0) {
            dword_59E948 -= 1;
            Object* v5 = off_59E81C[dword_59E948];
            off_59E978 = &(v5->data.inventory);
            sub_470650(v5->fid, inventoryWindowType);
            sub_47036C(dword_59E7EC[dword_59E948], -1, off_59E978, inventoryWindowType);
            windowRefresh(gInventoryWindow);
        }
    }
}

// 0x476464
int sub_476464(Object* a1, Object* a2, int a3, Object** a4, int quantity)
{
    int quantityToMove;
    if (quantity > 1) {
        quantityToMove = inventoryQuantitySelect(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, a2, quantity);
    } else {
        quantityToMove = 1;
    }

    if (quantityToMove == -1) {
        return -1;
    }

    if (a3 != -1) {
        if (itemRemove(off_519058, a2, quantityToMove) == -1) {
            return -1;
        }
    }

    int rc = itemAttemptAdd(a1, a2, quantityToMove);
    if (rc != 0) {
        if (a3 != -1) {
            itemAttemptAdd(off_519058, a2, quantityToMove);
        }
    } else {
        if (a4 != NULL) {
            if (a4 == &gInventoryArmor) {
                sub_4715F8(off_59E86C[0], gInventoryArmor, NULL);
            }
            *a4 = NULL;
        }
    }

    return rc;
}

// 0x47650C
int sub_47650C(Object* weapon, Object* ammo, Object** a3, int quantity, int keyCode)
{
    if (itemGetType(weapon) != ITEM_TYPE_WEAPON) {
        return -1;
    }

    if (itemGetType(ammo) != ITEM_TYPE_AMMO) {
        return -1;
    }

    if (!weaponCanBeReloadedWith(weapon, ammo)) {
        return -1;
    }

    int quantityToMove;
    if (quantity > 1) {
        quantityToMove = inventoryQuantitySelect(INVENTORY_WINDOW_TYPE_MOVE_ITEMS, ammo, quantity);
    } else {
        quantityToMove = 1;
    }

    if (quantityToMove == -1) {
        return -1;
    }

    Object* v14 = ammo;
    bool v17 = false;
    int rc = itemRemove(off_519058, weapon, 1);
    for (int index = 0; index < quantityToMove; index++) {
        int v11 = sub_478918(weapon, v14);
        if (v11 == 0) {
            if (a3 != NULL) {
                *a3 = NULL;
            }

            sub_49B9A0(v14);

            v17 = true;
            if (sub_472B54(keyCode, &v14, NULL, NULL) == 0) {
                break;
            }
        }
        if (v11 != -1) {
            v17 = true;
        }
        if (v11 != 0) {
            break;
        }
    }

    if (rc != -1) {
        itemAdd(off_519058, weapon, 1);
    }

    if (!v17) {
        return -1;
    }

    const char* sfx = sfxBuildWeaponName(WEAPON_SOUND_EFFECT_READY, weapon, HIT_MODE_RIGHT_WEAPON_PRIMARY, NULL);
    soundPlayFile(sfx);

    return 0;
}

// 0x47664C
void sub_47664C(int value, int inventoryWindowType)
{
    // BIGNUM.frm
    CacheEntry* handle;
    int fid = buildFid(6, 170, 0, 0, 0);
    unsigned char* data = artLockFrameData(fid, 0, 0, &handle);
    if (data == NULL) {
        return;
    }

    Rect rect;

    int windowWidth = windowGetWidth(dword_59E894);
    unsigned char* windowBuffer = windowGetBuffer(dword_59E894);

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
        rect.left = 125;
        rect.top = 45;
        rect.right = 195;
        rect.bottom = 69;

        int ranks[5];
        ranks[4] = value % 10;
        ranks[3] = value / 10 % 10;
        ranks[2] = value / 100 % 10;
        ranks[1] = value / 1000 % 10;
        ranks[0] = value / 10000 % 10;

        windowBuffer += rect.top * windowWidth + rect.left;

        for (int index = 0; index < 5; index++) {
            unsigned char* src = data + 14 * ranks[index];
            blitBufferToBuffer(src, 14, 24, 336, windowBuffer, windowWidth);
            windowBuffer += 14;
        }
    } else {
        rect.left = 133;
        rect.top = 64;
        rect.right = 189;
        rect.bottom = 88;

        windowBuffer += windowWidth * rect.top + rect.left;
        blitBufferToBuffer(data + 14 * (value / 60), 14, 24, 336, windowBuffer, windowWidth);
        blitBufferToBuffer(data + 14 * (value % 60 / 10), 14, 24, 336, windowBuffer + 14 * 2, windowWidth);
        blitBufferToBuffer(data + 14 * (value % 10), 14, 24, 336, windowBuffer + 14 * 3, windowWidth);
    }

    artUnlock(handle);
    windowRefreshRect(dword_59E894, &rect);
}

// 0x47688C
int inventoryQuantitySelect(int inventoryWindowType, Object* item, int max)
{
    inventoryQuantityWindowInit(inventoryWindowType, item);

    int value;
    int min;
    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
        value = 1;
        if (max > 99999) {
            max = 99999;
        }
        min = 1;
    } else {
        value = 60;
        min = 10;
    }

    sub_47664C(value, inventoryWindowType);

    bool v5 = false;
    for (;;) {
        int keyCode = sub_4C8B78();
        if (keyCode == VK_ESCAPE) {
            inventoryQuantityWindowFree(inventoryWindowType);
            return -1;
        }

        if (keyCode == VK_RETURN) {
            if (value >= min && value <= max) {
                if (inventoryWindowType != INVENTORY_WINDOW_TYPE_SET_TIMER || value % 10 == 0) {
                    soundPlayFile("ib1p1xx1");
                    break;
                }
            }

            soundPlayFile("iisxxxx1");
        } else if (keyCode == 5000) {
            v5 = false;
            value = max;
            sub_47664C(value, inventoryWindowType);
        } else if (keyCode == 6000) {
            v5 = false;
            if (value < max) {
                if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
                    if ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0) {
                        sub_4C9370();

                        unsigned int delay = 100;
                        while ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0) {
                            if (value < max) {
                                value++;
                            }

                            sub_47664C(value, inventoryWindowType);
                            sub_4C8B78();

                            if (delay > 1) {
                                delay--;
                                coreDelayProcessingEvents(delay);
                            }
                        }
                    } else {
                        if (value < max) {
                            value++;
                        }
                    }
                } else {
                    value += 10;
                }

                sub_47664C(value, inventoryWindowType);
                continue;
            }
        } else if (keyCode == 7000) {
            v5 = false;
            if (value > min) {
                if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
                    if ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0) {
                        sub_4C9370();

                        unsigned int delay = 100;
                        while ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_REPEAT) != 0) {
                            if (value > min) {
                                value--;
                            }

                            sub_47664C(value, inventoryWindowType);
                            sub_4C8B78();

                            if (delay > 1) {
                                delay--;
                                coreDelayProcessingEvents(delay);
                            }
                        }
                    } else {
                        if (value > min) {
                            value--;
                        }
                    }
                } else {
                    value -= 10;
                }

                sub_47664C(value, inventoryWindowType);
                continue;
            }
        }

        if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
            if (keyCode >= KEY_0 && keyCode <= KEY_9) {
                int number = keyCode - KEY_0;
                if (!v5) {
                    value = 0;
                }

                value = 10 * value % 100000 + number;
                v5 = true;

                sub_47664C(value, inventoryWindowType);
                continue;
            } else if (keyCode == KEY_BACKSPACE) {
                if (!v5) {
                    value = 0;
                }

                value /= 10;
                v5 = true;

                sub_47664C(value, inventoryWindowType);
                continue;
            }
        }
    }

    inventoryQuantityWindowFree(inventoryWindowType);

    return value;
}

// Creates move items/set timer interface.
//
// 0x476AB8
int inventoryQuantityWindowInit(int inventoryWindowType, Object* item)
{
    const int oldFont = fontGetCurrent();
    fontSetCurrent(103);

    for (int index = 0; index < 8; index++) {
        off_59E79C[index] = NULL;
    }

    const InventoryWindowDescription* windowDescription = &(gInventoryWindowDescriptions[inventoryWindowType]);

    dword_59E894 = windowCreate(windowDescription->x, windowDescription->y, windowDescription->width, windowDescription->height, 257, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
    unsigned char* windowBuffer = windowGetBuffer(dword_59E894);

    CacheEntry* backgroundHandle;
    int backgroundFid = buildFid(6, windowDescription->field_0, 0, 0, 0);
    unsigned char* backgroundData = artLockFrameData(backgroundFid, 0, 0, &backgroundHandle);
    if (backgroundData != NULL) {
        blitBufferToBuffer(backgroundData, windowDescription->width, windowDescription->height, windowDescription->width, windowBuffer, windowDescription->width);
        artUnlock(backgroundHandle);
    }

    MessageListItem messageListItem;
    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
        // MOVE ITEMS
        messageListItem.num = 21;
        if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
            int length = fontGetStringWidth(messageListItem.text);
            fontDrawText(windowBuffer + windowDescription->width * 9 + (windowDescription->width - length) / 2, messageListItem.text, 200, windowDescription->width, byte_6A38D0[21091]);
        }
    } else if (inventoryWindowType == INVENTORY_WINDOW_TYPE_SET_TIMER) {
        // SET TIMER
        messageListItem.num = 23;
        if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
            int length = fontGetStringWidth(messageListItem.text);
            fontDrawText(windowBuffer + windowDescription->width * 9 + (windowDescription->width - length) / 2, messageListItem.text, 200, windowDescription->width, byte_6A38D0[21091]);
        }

        // Timer overlay
        CacheEntry* overlayFrmHandle;
        int overlayFid = buildFid(6, 306, 0, 0, 0);
        unsigned char* overlayFrmData = artLockFrameData(overlayFid, 0, 0, &overlayFrmHandle);
        if (overlayFrmData != NULL) {
            blitBufferToBuffer(overlayFrmData, 105, 81, 105, windowBuffer + 34 * windowDescription->width + 113, windowDescription->width);
            artUnlock(overlayFrmHandle);
        }
    }

    int inventoryFid = itemGetInventoryFid(item);
    artRender(inventoryFid, windowBuffer + windowDescription->width * 46 + 16, 90, 61, windowDescription->width);

    int x;
    int y;
    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
        x = 200;
        y = 46;
    } else {
        x = 194;
        y = 64;
    }

    int fid;
    unsigned char* buttonUpData;
    unsigned char* buttonDownData;
    int btn;

    // Plus button
    fid = buildFid(6, 193, 0, 0, 0);
    buttonUpData = artLockFrameData(fid, 0, 0, &(off_59E79C[0]));

    fid = buildFid(6, 194, 0, 0, 0);
    buttonDownData = artLockFrameData(fid, 0, 0, &(off_59E79C[1]));

    if (buttonUpData != NULL && buttonDownData != NULL) {
        btn = buttonCreate(dword_59E894, x, y, 16, 12, -1, -1, 6000, -1, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
        if (btn != -1) {
            buttonSetCallbacks(btn, sub_451970, sub_451978);
        }
    }

    // Minus button
    fid = buildFid(6, 191, 0, 0, 0);
    buttonUpData = artLockFrameData(fid, 0, 0, &(off_59E79C[2]));

    fid = buildFid(6, 192, 0, 0, 0);
    buttonDownData = artLockFrameData(fid, 0, 0, &(off_59E79C[3]));

    if (buttonUpData != NULL && buttonDownData != NULL) {
        btn = buttonCreate(dword_59E894, x, y + 12, 17, 12, -1, -1, 7000, -1, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
        if (btn != -1) {
            buttonSetCallbacks(btn, sub_451970, sub_451978);
        }
    }

    fid = buildFid(6, 8, 0, 0, 0);
    buttonUpData = artLockFrameData(fid, 0, 0, &(off_59E79C[4]));

    fid = buildFid(6, 9, 0, 0, 0);
    buttonDownData = artLockFrameData(fid, 0, 0, &(off_59E79C[5]));

    if (buttonUpData != NULL && buttonDownData != NULL) {
        // Done
        btn = buttonCreate(dword_59E894, 98, 128, 15, 16, -1, -1, -1, KEY_RETURN, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
        if (btn != -1) {
            buttonSetCallbacks(btn, sub_451970, sub_451978);
        }

        // Cancel
        btn = buttonCreate(dword_59E894, 148, 128, 15, 16, -1, -1, -1, KEY_ESCAPE, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
        if (btn != -1) {
            buttonSetCallbacks(btn, sub_451970, sub_451978);
        }
    }

    if (inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS) {
        fid = buildFid(6, 307, 0, 0, 0);
        buttonUpData = artLockFrameData(fid, 0, 0, &(off_59E79C[6]));

        fid = buildFid(6, 308, 0, 0, 0);
        buttonDownData = artLockFrameData(fid, 0, 0, &(off_59E79C[7]));

        if (buttonUpData != NULL && buttonDownData != NULL) {
            // ALL
            messageListItem.num = 22;
            if (messageListGetItem(&gInventoryMessageList, &messageListItem)) {
                int length = fontGetStringWidth(messageListItem.text);

                // TODO: Where is y? Is it hardcoded in to 376?
                fontDrawText(buttonUpData + (94 - length) / 2 + 376, messageListItem.text, 200, 94, byte_6A38D0[21091]);
                fontDrawText(buttonDownData + (94 - length) / 2 + 376, messageListItem.text, 200, 94, byte_6A38D0[18977]);

                btn = buttonCreate(dword_59E894, 120, 80, 94, 33, -1, -1, -1, 5000, buttonUpData, buttonDownData, NULL, BUTTON_FLAG_TRANSPARENT);
                if (btn != -1) {
                    buttonSetCallbacks(btn, sub_451970, sub_451978);
                }
            }
        }
    }

    windowRefresh(dword_59E894);
    inventorySetCursor(INVENTORY_WINDOW_CURSOR_ARROW);
    fontSetCurrent(oldFont);

    return 0;
}

// 0x477030
int inventoryQuantityWindowFree(int inventoryWindowType)
{
    int count = inventoryWindowType == INVENTORY_WINDOW_TYPE_MOVE_ITEMS ? 8 : 6;

    for (int index = 0; index < count; index++) {
        artUnlock(off_59E79C[index]);
    }

    windowDestroy(dword_59E894);

    return 0;
}

// 0x477074
int sub_477074(Object* a1)
{
    bool v1 = dword_519060;

    if (!v1) {
        if (inventoryCommonInit() == -1) {
            return -1;
        }
    }

    int seconds = inventoryQuantitySelect(INVENTORY_WINDOW_TYPE_SET_TIMER, a1, 180);

    if (!v1) {
        // NOTE: Uninline.
        inventoryCommonFree();
    }

    return seconds;
}