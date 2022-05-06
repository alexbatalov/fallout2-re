#include "game_mouse.h"

#include "actions.h"
#include "color.h"
#include "combat.h"
#include "core.h"
#include "critter.h"
#include "draw.h"
#include "game.h"
#include "game_config.h"
#include "game_sound.h"
#include "interface.h"
#include "item.h"
#include "map.h"
#include "object.h"
#include "proto.h"
#include "proto_instance.h"
#include "skilldex.h"
#include "text_font.h"
#include "tile.h"
#include "window_manager.h"

#include <stdio.h>

// 0x518BF8
bool gGameMouseInitialized = false;

// 0x518BFC
int dword_518BFC = 0;

// 0x518C00
int dword_518C00 = 0;

// 0x518C04
int dword_518C04 = 0;

// 0x518C08
int dword_518C08 = 1;

// 0x518C0C
int gGameMouseCursor = MOUSE_CURSOR_NONE;

// 0x518C10
CacheEntry* gGameMouseCursorFrmHandle = INVALID_CACHE_ENTRY;

// 0x518C14
const int gGameMouseCursorFrmIds[MOUSE_CURSOR_TYPE_COUNT] = {
    266,
    267,
    268,
    269,
    270,
    271,
    272,
    273,
    274,
    275,
    276,
    277,
    330,
    331,
    329,
    328,
    332,
    334,
    333,
    335,
    279,
    280,
    281,
    293,
    310,
    278,
    295,
};

// 0x518C80
bool gGameMouseObjectsInitialized = false;

// 0x518C84
bool dword_518C84 = false;

// 0x518C88
unsigned int dword_518C88 = 0;

// actmenu.frm
// 0x518C8C
Art* gGameMouseActionMenuFrm = NULL;

// 0x518C90
CacheEntry* gGameMouseActionMenuFrmHandle = INVALID_CACHE_ENTRY;

// 0x518C94
int gGameMouseActionMenuFrmWidth = 0;

// 0x518C98
int gGameMouseActionMenuFrmHeight = 0;

// 0x518C9C
int gGameMouseActionMenuFrmDataSize = 0;

// 0x518CA0
int dword_518CA0 = 0;

// 0x518CA4
int dword_518CA4 = 0;

// 0x518CA8
unsigned char* gGameMouseActionMenuFrmData = NULL;

// actpick.frm
// 0x518CAC
Art* gGameMouseActionPickFrm = NULL;

// 0x518CB0
CacheEntry* gGameMouseActionPickFrmHandle = INVALID_CACHE_ENTRY;

// 0x518CB4
int gGameMouseActionPickFrmWidth = 0;

// 0x518CB8
int gGameMouseActionPickFrmHeight = 0;

// 0x518CBC
int gGameMouseActionPickFrmDataSize = 0;

// 0x518CC0
int dword_518CC0 = 0;

// 0x518CC4
int dword_518CC4 = 0;

// 0x518CC8
unsigned char* gGameMouseActionPickFrmData = NULL;

// acttohit.frm
// 0x518CCC
Art* gGameMouseActionHitFrm = NULL;

// 0x518CD0
CacheEntry* gGameMouseActionHitFrmHandle = INVALID_CACHE_ENTRY;

// 0x518CD4
int gGameMouseActionHitFrmWidth = 0;

// 0x518CD8
int gGameMouseActionHitFrmHeight = 0;

// 0x518CDC
int gGameMouseActionHitFrmDataSize = 0;

// 0x518CE0
unsigned char* gGameMouseActionHitFrmData = NULL;

// blank.frm
// 0x518CE4
Art* gGameMouseBouncingCursorFrm = NULL;

// 0x518CE8
CacheEntry* gGameMouseBouncingCursorFrmHandle = INVALID_CACHE_ENTRY;

// 0x518CEC
int gGameMouseBouncingCursorFrmWidth = 0;

// 0x518CF0
int gGameMouseBouncingCursorFrmHeight = 0;

// 0x518CF4
int gGameMouseBouncingCursorFrmDataSize = 0;

// 0x518CF8
unsigned char* gGameMouseBouncingCursorFrmData = NULL;

// msef000.frm
// 0x518CFC
Art* gGameMouseHexCursorFrm = NULL;

// 0x518D00
CacheEntry* gGameMouseHexCursorFrmHandle = INVALID_CACHE_ENTRY;

// 0x518D04
int gGameMouseHexCursorFrmWidth = 0;

// 0x518D08
int gGameMouseHexCursorHeight = 0;

// 0x518D0C
int gGameMouseHexCursorDataSize = 0;

// 0x518D10
unsigned char* gGameMouseHexCursorFrmData = NULL;

// 0x518D14
unsigned char gGameMouseActionMenuItemsLength = 0;

// 0x518D18
unsigned char* off_518D18 = NULL;

// 0x518D1C
unsigned char gGameMouseActionMenuHighlightedItemIndex = 0;

// 0x518D1E
const short gGameMouseActionMenuItemFrmIds[GAME_MOUSE_ACTION_MENU_ITEM_COUNT] = {
    253, // Cancel
    255, // Drop
    257, // Inventory
    259, // Look
    261, // Rotate
    263, // Talk
    265, // Use/Get
    302, // Unload
    304, // Skill
    435, // Push
};

// 0x518D34
int dword_518D34 = 1;

// 0x518D38
int gGameMouseMode = GAME_MOUSE_MODE_MOVE;

// 0x518D3C
int gGameMouseModeFrmIds[GAME_MOUSE_MODE_COUNT] = {
    249,
    250,
    251,
    293,
    293,
    293,
    293,
    293,
    293,
    293,
    293,
};

// 0x518D68
const int gGameMouseModeSkills[GAME_MOUSE_MODE_SKILL_COUNT] = {
    SKILL_FIRST_AID,
    SKILL_DOCTOR,
    SKILL_LOCKPICK,
    SKILL_STEAL,
    SKILL_TRAPS,
    SKILL_SCIENCE,
    SKILL_REPAIR,
};

// 0x518D84
int gGameMouseAnimatedCursorNextFrame = 0;

// 0x518D88
unsigned int gGameMouseAnimatedCursorLastUpdateTimestamp = 0;

// 0x518D8C
int dword_518D8C = -1;

// 0x518D90
bool gGameMouseItemHighlightEnabled = true;

// 0x518D94
Object* gGameMouseHighlightedItem = NULL;

// 0x518D98
bool dword_518D98 = false;

// 0x518D9C
int dword_518D9C = -1;

// 0x596C3C
int gGameMouseActionMenuItems[GAME_MOUSE_ACTION_MENU_ITEM_COUNT];

// 0x596C64
int gGameMouseLastX;

// 0x596C68
int gGameMouseLastY;

// blank.frm
// 0x596C6C
Object* gGameMouseBouncingCursor;

// msef000.frm
// 0x596C70
Object* gGameMouseHexCursor;

// 0x596C74
Object* gGameMousePointedObject;

// 0x44B2B0
int gameMouseInit()
{
    if (gGameMouseInitialized) {
        return -1;
    }

    if (gameMouseObjectsInit() != 0) {
        return -1;
    }

    gGameMouseInitialized = true;
    dword_518BFC = 1;

    gameMouseSetCursor(MOUSE_CURSOR_ARROW);

    return 0;
}

// 0x44B2E8
int gameMouseReset()
{
    if (!gGameMouseInitialized) {
        return -1;
    }

    // NOTE: Uninline.
    if (gameMouseObjectsReset() != 0) {
        return -1;
    }

    // NOTE: Uninline.
    gmouse_enable();

    dword_518C08 = 1;
    gameMouseSetCursor(MOUSE_CURSOR_ARROW);
    gGameMouseAnimatedCursorNextFrame = 0;
    gGameMouseAnimatedCursorLastUpdateTimestamp = 0;
    dword_518D98 = 0;

    return 0;
}

// 0x44B3B8
void gameMouseExit()
{
    if (!gGameMouseInitialized) {
        return;
    }

    mouseHideCursor();

    mouseSetFrame(NULL, 0, 0, 0, 0, 0, 0);

    // NOTE: Uninline.
    gameMouseObjectsFree();

    if (gGameMouseCursorFrmHandle != INVALID_CACHE_ENTRY) {
        artUnlock(gGameMouseCursorFrmHandle);
    }
    gGameMouseCursorFrmHandle = INVALID_CACHE_ENTRY;

    dword_518BFC = 0;
    gGameMouseInitialized = false;
    gGameMouseCursor = -1;
}

// 0x44B454
void gmouse_enable()
{
    if (!dword_518BFC) {
        gGameMouseCursor = -1;
        gameMouseSetCursor(MOUSE_CURSOR_NONE);
        dword_518C08 = 1;
        dword_518BFC = 1;
        dword_518D8C = -1;
    }
}

// 0x44B48C
void gmouse_disable(int a1)
{
    if (dword_518BFC) {
        gameMouseSetCursor(MOUSE_CURSOR_NONE);
        dword_518BFC = 0;

        if (a1 & 1) {
            dword_518C08 = 1;
        } else {
            dword_518C08 = 0;
        }
    }
}

// 0x44B4CC
void gmouse_enable_scrolling()
{
    dword_518C08 = 1;
}

// 0x44B4D8
void gmouse_disable_scrolling()
{
    dword_518C08 = 0;
}

// 0x44B504
int gmouse_get_click_to_scroll()
{
    return dword_518C04;
}

// 0x44B54C
int gmouse_is_scrolling()
{
    int v1 = 0;

    if (dword_518C08) {
        int x;
        int y;
        mouseGetPosition(&x, &y);
        if (x == stru_6AC9F0.left || x == stru_6AC9F0.right || y == stru_6AC9F0.top || y == stru_6AC9F0.bottom) {
            switch (gGameMouseCursor) {
            case MOUSE_CURSOR_SCROLL_NW:
            case MOUSE_CURSOR_SCROLL_N:
            case MOUSE_CURSOR_SCROLL_NE:
            case MOUSE_CURSOR_SCROLL_E:
            case MOUSE_CURSOR_SCROLL_SE:
            case MOUSE_CURSOR_SCROLL_S:
            case MOUSE_CURSOR_SCROLL_SW:
            case MOUSE_CURSOR_SCROLL_W:
            case MOUSE_CURSOR_SCROLL_NW_INVALID:
            case MOUSE_CURSOR_SCROLL_N_INVALID:
            case MOUSE_CURSOR_SCROLL_NE_INVALID:
            case MOUSE_CURSOR_SCROLL_E_INVALID:
            case MOUSE_CURSOR_SCROLL_SE_INVALID:
            case MOUSE_CURSOR_SCROLL_S_INVALID:
            case MOUSE_CURSOR_SCROLL_SW_INVALID:
            case MOUSE_CURSOR_SCROLL_W_INVALID:
                v1 = 1;
                break;
            default:
                return v1;
            }
        }
    }

    return v1;
}

// 0x44B684
void gameMouseRefresh()
{
    if (!gGameMouseInitialized) {
        return;
    }

    int mouseX;
    int mouseY;

    if (gGameMouseCursor >= FIRST_GAME_MOUSE_ANIMATED_CURSOR) {
        mouse_info();

        if (dword_518C08) {
            mouseGetPosition(&mouseX, &mouseY);
            int oldMouseCursor = gGameMouseCursor;

            if (gameMouseHandleScrolling(mouseX, mouseY, gGameMouseCursor) == 0) {
                switch (oldMouseCursor) {
                case MOUSE_CURSOR_SCROLL_NW:
                case MOUSE_CURSOR_SCROLL_N:
                case MOUSE_CURSOR_SCROLL_NE:
                case MOUSE_CURSOR_SCROLL_E:
                case MOUSE_CURSOR_SCROLL_SE:
                case MOUSE_CURSOR_SCROLL_S:
                case MOUSE_CURSOR_SCROLL_SW:
                case MOUSE_CURSOR_SCROLL_W:
                case MOUSE_CURSOR_SCROLL_NW_INVALID:
                case MOUSE_CURSOR_SCROLL_N_INVALID:
                case MOUSE_CURSOR_SCROLL_NE_INVALID:
                case MOUSE_CURSOR_SCROLL_E_INVALID:
                case MOUSE_CURSOR_SCROLL_SE_INVALID:
                case MOUSE_CURSOR_SCROLL_S_INVALID:
                case MOUSE_CURSOR_SCROLL_SW_INVALID:
                case MOUSE_CURSOR_SCROLL_W_INVALID:
                    break;
                default:
                    dword_518D8C = oldMouseCursor;
                    break;
                }
                return;
            }

            if (dword_518D8C != -1) {
                gameMouseSetCursor(dword_518D8C);
                dword_518D8C = -1;
                return;
            }
        }

        gameMouseSetCursor(gGameMouseCursor);
        return;
    }

    if (!dword_518BFC) {
        if (dword_518C08) {
            mouseGetPosition(&mouseX, &mouseY);
            int oldMouseCursor = gGameMouseCursor;

            if (gameMouseHandleScrolling(mouseX, mouseY, gGameMouseCursor) == 0) {
                switch (oldMouseCursor) {
                case MOUSE_CURSOR_SCROLL_NW:
                case MOUSE_CURSOR_SCROLL_N:
                case MOUSE_CURSOR_SCROLL_NE:
                case MOUSE_CURSOR_SCROLL_E:
                case MOUSE_CURSOR_SCROLL_SE:
                case MOUSE_CURSOR_SCROLL_S:
                case MOUSE_CURSOR_SCROLL_SW:
                case MOUSE_CURSOR_SCROLL_W:
                case MOUSE_CURSOR_SCROLL_NW_INVALID:
                case MOUSE_CURSOR_SCROLL_N_INVALID:
                case MOUSE_CURSOR_SCROLL_NE_INVALID:
                case MOUSE_CURSOR_SCROLL_E_INVALID:
                case MOUSE_CURSOR_SCROLL_SE_INVALID:
                case MOUSE_CURSOR_SCROLL_S_INVALID:
                case MOUSE_CURSOR_SCROLL_SW_INVALID:
                case MOUSE_CURSOR_SCROLL_W_INVALID:
                    break;
                default:
                    dword_518D8C = oldMouseCursor;
                    break;
                }

                return;
            }

            if (dword_518D8C != -1) {
                gameMouseSetCursor(dword_518D8C);
                dword_518D8C = -1;
            }
        }

        return;
    }

    mouseGetPosition(&mouseX, &mouseY);

    int oldMouseCursor = gGameMouseCursor;
    if (gameMouseHandleScrolling(mouseX, mouseY, MOUSE_CURSOR_NONE) == 0) {
        switch (oldMouseCursor) {
        case MOUSE_CURSOR_SCROLL_NW:
        case MOUSE_CURSOR_SCROLL_N:
        case MOUSE_CURSOR_SCROLL_NE:
        case MOUSE_CURSOR_SCROLL_E:
        case MOUSE_CURSOR_SCROLL_SE:
        case MOUSE_CURSOR_SCROLL_S:
        case MOUSE_CURSOR_SCROLL_SW:
        case MOUSE_CURSOR_SCROLL_W:
        case MOUSE_CURSOR_SCROLL_NW_INVALID:
        case MOUSE_CURSOR_SCROLL_N_INVALID:
        case MOUSE_CURSOR_SCROLL_NE_INVALID:
        case MOUSE_CURSOR_SCROLL_E_INVALID:
        case MOUSE_CURSOR_SCROLL_SE_INVALID:
        case MOUSE_CURSOR_SCROLL_S_INVALID:
        case MOUSE_CURSOR_SCROLL_SW_INVALID:
        case MOUSE_CURSOR_SCROLL_W_INVALID:
            break;
        default:
            dword_518D8C = oldMouseCursor;
            break;
        }
        return;
    }

    if (dword_518D8C != -1) {
        gameMouseSetCursor(dword_518D8C);
        dword_518D8C = -1;
    }

    if (windowGetAtPoint(mouseX, mouseY) != gIsoWindow) {
        if (gGameMouseCursor == MOUSE_CURSOR_NONE) {
            gameMouseObjectsHide();
            gameMouseSetCursor(MOUSE_CURSOR_ARROW);

            if (gGameMouseMode >= 2 && !isInCombat()) {
                gameMouseSetMode(GAME_MOUSE_MODE_MOVE);
            }
        }
        return;
    }

    // NOTE: Strange set of conditions and jumps. Not sure about this one.
    switch (gGameMouseCursor) {
    case MOUSE_CURSOR_NONE:
    case MOUSE_CURSOR_ARROW:
    case MOUSE_CURSOR_SMALL_ARROW_UP:
    case MOUSE_CURSOR_SMALL_ARROW_DOWN:
    case MOUSE_CURSOR_CROSSHAIR:
    case MOUSE_CURSOR_USE_CROSSHAIR:
        if (gGameMouseCursor != MOUSE_CURSOR_NONE) {
            gameMouseSetCursor(MOUSE_CURSOR_NONE);
        }

        if ((gGameMouseHexCursor->flags & 0x01) != 0) {
            gameMouseObjectsShow();
        }

        break;
    }

    Rect r1;
    if (gmouse_3d_move_to(mouseX, mouseY, gElevation, &r1) == 0) {
        tileWindowRefreshRect(&r1, gElevation);
    }

    if ((gGameMouseHexCursor->flags & 0x01) != 0 || dword_518C00 != 0) {
        return;
    }

    unsigned int v3 = get_bk_time();
    if (mouseX == gGameMouseLastX && mouseY == gGameMouseLastY) {
        if (dword_518C84 || getTicksBetween(v3, dword_518C88) < 250) {
            return;
        }

        if (gGameMouseMode != GAME_MOUSE_MODE_MOVE) {
            if (gGameMouseMode == GAME_MOUSE_MODE_ARROW) {
                dword_518C88 = v3;
                dword_518C84 = true;

                Object* pointedObject = gameMouseGetObjectUnderCursor(-1, true, gElevation);
                if (pointedObject != NULL) {
                    int primaryAction = -1;

                    switch ((pointedObject->fid & 0xF000000) >> 24) {
                    case OBJ_TYPE_ITEM:
                        primaryAction = GAME_MOUSE_ACTION_MENU_ITEM_USE;
                        if (gGameMouseItemHighlightEnabled) {
                            Rect tmp;
                            if (objectSetOutline(pointedObject, OUTLINE_TYPE_ITEM, &tmp) == 0) {
                                tileWindowRefreshRect(&tmp, gElevation);
                                gGameMouseHighlightedItem = pointedObject;
                            }
                        }
                        break;
                    case OBJ_TYPE_CRITTER:
                        if (pointedObject == gDude) {
                            primaryAction = GAME_MOUSE_ACTION_MENU_ITEM_ROTATE;
                        } else {
                            if (obj_action_can_talk_to(pointedObject)) {
                                if (isInCombat()) {
                                    primaryAction = GAME_MOUSE_ACTION_MENU_ITEM_LOOK;
                                } else {
                                    primaryAction = GAME_MOUSE_ACTION_MENU_ITEM_TALK;
                                }
                            } else {
                                if (critter_flag_check(pointedObject->pid, 32)) {
                                    primaryAction = GAME_MOUSE_ACTION_MENU_ITEM_LOOK;
                                } else {
                                    primaryAction = GAME_MOUSE_ACTION_MENU_ITEM_USE;
                                }
                            }
                        }
                        break;
                    case OBJ_TYPE_SCENERY:
                        if (!obj_action_can_use(pointedObject)) {
                            primaryAction = GAME_MOUSE_ACTION_MENU_ITEM_LOOK;
                        } else {
                            primaryAction = GAME_MOUSE_ACTION_MENU_ITEM_USE;
                        }
                        break;
                    case OBJ_TYPE_WALL:
                        primaryAction = GAME_MOUSE_ACTION_MENU_ITEM_LOOK;
                        break;
                    }

                    if (primaryAction != -1) {
                        if (gameMouseRenderPrimaryAction(mouseX, mouseY, primaryAction, stru_6AC9F0.right - stru_6AC9F0.left + 1, stru_6AC9F0.bottom - stru_6AC9F0.top - 99) == 0) {
                            Rect tmp;
                            int fid = buildFid(6, 282, 0, 0, 0);
                            if (objectSetFid(gGameMouseHexCursor, fid, &tmp) == 0) {
                                tileWindowRefreshRect(&tmp, gElevation);
                            }
                        }
                    }

                    if (pointedObject != gGameMousePointedObject) {
                        gGameMousePointedObject = pointedObject;
                        obj_look_at(gDude, gGameMousePointedObject);
                    }
                }
            } else if (gGameMouseMode == GAME_MOUSE_MODE_CROSSHAIR) {
                Object* pointedObject = gameMouseGetObjectUnderCursor(OBJ_TYPE_CRITTER, false, gElevation);
                if (pointedObject == NULL) {
                    pointedObject = gameMouseGetObjectUnderCursor(-1, false, gElevation);
                    if (!objectIsDoor(pointedObject)) {
                        pointedObject = NULL;
                    }
                }

                if (pointedObject != NULL) {
                    bool pointedObjectIsCritter = (pointedObject->fid & 0xF000000) >> 24 == OBJ_TYPE_CRITTER;

                    int combatLooks = 0;
                    configGetInt(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_COMBAT_LOOKS_KEY, &combatLooks);
                    if (combatLooks != 0) {
                        if (obj_examine(gDude, pointedObject) == -1) {
                            obj_look_at(gDude, pointedObject);
                        }
                    }

                    int color;
                    int accuracy;
                    char formattedAccuracy[8];
                    if (combat_to_hit(pointedObject, &accuracy)) {
                        sprintf(formattedAccuracy, "%d%%", accuracy);

                        if (pointedObjectIsCritter) {
                            if (pointedObject->data.critter.combat.team != 0) {
                                color = byte_6A38D0[32767];
                            } else {
                                color = byte_6A38D0[32495];
                            }
                        } else {
                            color = byte_6A38D0[17969];
                        }
                    } else {
                        sprintf(formattedAccuracy, " %c ", 'X');

                        if (pointedObjectIsCritter) {
                            if (pointedObject->data.critter.combat.team != 0) {
                                color = byte_6A38D0[31744];
                            } else {
                                color = byte_6A38D0[18161];
                            }
                        } else {
                            color = byte_6A38D0[32239];
                        }
                    }

                    if (gameMouseRenderAccuracy(formattedAccuracy, color) == 0) {
                        Rect tmp;
                        int fid = buildFid(6, 284, 0, 0, 0);
                        if (objectSetFid(gGameMouseHexCursor, fid, &tmp) == 0) {
                            tileWindowRefreshRect(&tmp, gElevation);
                        }
                    }

                    if (gGameMousePointedObject != pointedObject) {
                        gGameMousePointedObject = pointedObject;
                    }
                } else {
                    Rect tmp;
                    if (gameMouseUpdateHexCursorFid(&tmp) == 0) {
                        tileWindowRefreshRect(&tmp, gElevation);
                    }
                }

                dword_518C88 = v3;
                dword_518C84 = true;
            }
            return;
        }

        char formattedActionPoints[8];
        int color;
        int v6 = make_path(gDude, gDude->tile, gGameMouseHexCursor->tile, NULL, 1);
        if (v6) {
            if (!isInCombat()) {
                formattedActionPoints[0] = '\0';
                color = byte_6A38D0[31744];
            } else {
                int v7 = critterGetMovementPointCostAdjustedForCrippledLegs(gDude, v6);
                int v8;
                if (v7 - dword_56D39C >= 0) {
                    v8 = v7 - dword_56D39C;
                } else {
                    v8 = 0;
                }

                if (v8 <= gDude->data.critter.combat.ap) {
                    sprintf(formattedActionPoints, "%d", v8);
                    color = byte_6A38D0[32767];
                } else {
                    sprintf(formattedActionPoints, "%c", 'X');
                    color = byte_6A38D0[31744];
                }
            }
        } else {
            sprintf(formattedActionPoints, "%c", 'X');
            color = byte_6A38D0[31744];
        }

        if (gameMouseRenderActionPoints(formattedActionPoints, color) == 0) {
            Rect tmp;
            objectGetRect(gGameMouseHexCursor, &tmp);
            tileWindowRefreshRect(&tmp, 0);
        }

        dword_518C88 = v3;
        dword_518C84 = true;
        dword_518D9C = gGameMouseHexCursor->tile;
        return;
    }

    dword_518C88 = v3;
    dword_518C84 = false;
    gGameMouseLastX = mouseX;
    gGameMouseLastY = mouseY;

    if (!dword_518C00) {
        int fid = buildFid(6, 0, 0, 0, 0);
        gameMouseSetBouncingCursorFid(fid);
    }

    int v34 = 0;

    Rect r2;
    Rect r26;
    if (gameMouseUpdateHexCursorFid(&r2) == 0) {
        v34 |= 1;
    }

    if (gGameMouseHighlightedItem != NULL) {
        if (objectClearOutline(gGameMouseHighlightedItem, &r26) == 0) {
            v34 |= 2;
        }
        gGameMouseHighlightedItem = NULL;
    }

    switch (v34) {
    case 3:
        rectUnion(&r2, &r26, &r2);
        // FALLTHROUGH
    case 1:
        tileWindowRefreshRect(&r2, gElevation);
        break;
    case 2:
        tileWindowRefreshRect(&r26, gElevation);
        break;
    }
}

// 0x44BFA8
void gmouse_handle_event(int mouseX, int mouseY, int mouseState)
{
    if (!gGameMouseInitialized) {
        return;
    }

    if (gGameMouseCursor >= MOUSE_CURSOR_WAIT_PLANET) {
        return;
    }

    if (!dword_518BFC) {
        return;
    }

    if (dword_518D98) {
        if (gmouse_get_click_to_scroll()) {
            return;
        }
    }

    if (!mouse_click_in(0, 0, stru_6AC9F0.right - stru_6AC9F0.left, stru_6AC9F0.bottom - stru_6AC9F0.top - 100)) {
        return;
    }

    if ((mouseState & MOUSE_EVENT_RIGHT_BUTTON_DOWN) != 0) {
        if ((mouseState & MOUSE_EVENT_RIGHT_BUTTON_REPEAT) == 0 && (gGameMouseHexCursor->flags & 0x01) == 0) {
            gameMouseCycleMode();
        }
        return;
    }

    if ((mouseState & MOUSE_EVENT_LEFT_BUTTON_UP) != 0) {
        if (gGameMouseMode == GAME_MOUSE_MODE_MOVE) {
            int actionPoints;
            if (isInCombat()) {
                actionPoints = dword_56D39C + gDude->data.critter.combat.ap;
            } else {
                actionPoints = -1;
            }

            bool running;
            configGetBool(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_RUNNING_KEY, &running);

            if (gPressedPhysicalKeys[DIK_LSHIFT] || gPressedPhysicalKeys[DIK_RSHIFT]) {
                if (running) {
                    dude_move(actionPoints);
                    return;
                }
            } else {
                if (!running) {
                    dude_move(actionPoints);
                    return;
                }
            }

            dude_run(actionPoints);
            return;
        }

        if (gGameMouseMode == GAME_MOUSE_MODE_ARROW) {
            Object* v5 = gameMouseGetObjectUnderCursor(-1, true, gElevation);
            if (v5 != NULL) {
                switch ((v5->fid & 0xF000000) >> 24) {
                case OBJ_TYPE_ITEM:
                    actionPickUp(gDude, v5);
                    break;
                case OBJ_TYPE_CRITTER:
                    if (v5 == gDude) {
                        if (((gDude->fid & 0xFF0000) >> 16) == ANIM_STAND) {
                            Rect a1;
                            if (objectRotateClockwise(v5, &a1) == 0) {
                                tileWindowRefreshRect(&a1, v5->elevation);
                            }
                        }
                    } else {
                        if (obj_action_can_talk_to(v5)) {
                            if (isInCombat()) {
                                if (obj_examine(gDude, v5) == -1) {
                                    obj_look_at(gDude, v5);
                                }
                            } else {
                                actionTalk(gDude, v5);
                            }
                        } else {
                            action_loot_container(gDude, v5);
                        }
                    }
                    break;
                case OBJ_TYPE_SCENERY:
                    if (obj_action_can_use(v5)) {
                        action_use_an_object(gDude, v5);
                    } else {
                        if (obj_examine(gDude, v5) == -1) {
                            obj_look_at(gDude, v5);
                        }
                    }
                    break;
                case OBJ_TYPE_WALL:
                    if (obj_examine(gDude, v5) == -1) {
                        obj_look_at(gDude, v5);
                    }
                    break;
                }
            }
            return;
        }

        if (gGameMouseMode == GAME_MOUSE_MODE_CROSSHAIR) {
            Object* v7 = gameMouseGetObjectUnderCursor(OBJ_TYPE_CRITTER, false, gElevation);
            if (v7 == NULL) {
                v7 = gameMouseGetObjectUnderCursor(-1, false, gElevation);
                if (!objectIsDoor(v7)) {
                    v7 = NULL;
                }
            }

            if (v7 != NULL) {
                combat_attack_this(v7);
                dword_518C84 = true;
                gGameMouseLastY = mouseY;
                gGameMouseLastX = mouseX;
                dword_518C88 = get_time() - 250;
            }
            return;
        }

        if (gGameMouseMode == GAME_MOUSE_MODE_USE_CROSSHAIR) {
            Object* object = gameMouseGetObjectUnderCursor(-1, true, gElevation);
            if (object != NULL) {
                Object* weapon;
                if (interfaceGetActiveItem(&weapon) != -1) {
                    if (isInCombat()) {
                        int hitMode = interfaceGetCurrentHand()
                            ? HIT_MODE_RIGHT_WEAPON_PRIMARY
                            : HIT_MODE_LEFT_WEAPON_PRIMARY;

                        int actionPointsRequired = item_mp_cost(gDude, hitMode, false);
                        if (actionPointsRequired <= gDude->data.critter.combat.ap) {
                            if (action_use_an_item_on_object(gDude, object, weapon) != -1) {
                                int actionPoints = gDude->data.critter.combat.ap;
                                if (actionPointsRequired > actionPoints) {
                                    gDude->data.critter.combat.ap = 0;
                                } else {
                                    gDude->data.critter.combat.ap -= actionPointsRequired;
                                }
                                interfaceRenderActionPoints(gDude->data.critter.combat.ap, dword_56D39C);
                            }
                        }
                    } else {
                        action_use_an_item_on_object(gDude, object, weapon);
                    }
                }
            }
            gameMouseSetCursor(MOUSE_CURSOR_NONE);
            gameMouseSetMode(GAME_MOUSE_MODE_MOVE);
            return;
        }

        if (gGameMouseMode == GAME_MOUSE_MODE_USE_FIRST_AID
            || gGameMouseMode == GAME_MOUSE_MODE_USE_DOCTOR
            || gGameMouseMode == GAME_MOUSE_MODE_USE_LOCKPICK
            || gGameMouseMode == GAME_MOUSE_MODE_USE_STEAL
            || gGameMouseMode == GAME_MOUSE_MODE_USE_TRAPS
            || gGameMouseMode == GAME_MOUSE_MODE_USE_SCIENCE
            || gGameMouseMode == GAME_MOUSE_MODE_USE_REPAIR) {
            Object* object = gameMouseGetObjectUnderCursor(-1, 1, gElevation);
            if (object == NULL || actionUseSkill(gDude, object, gGameMouseModeSkills[gGameMouseMode - FIRST_GAME_MOUSE_MODE_SKILL]) != -1) {
                gameMouseSetCursor(MOUSE_CURSOR_NONE);
                gameMouseSetMode(GAME_MOUSE_MODE_MOVE);
            }
            return;
        }
    }

    if ((mouseState & MOUSE_EVENT_LEFT_BUTTON_DOWN_REPEAT) == MOUSE_EVENT_LEFT_BUTTON_DOWN_REPEAT && gGameMouseMode == GAME_MOUSE_MODE_ARROW) {
        Object* v16 = gameMouseGetObjectUnderCursor(-1, true, gElevation);
        if (v16 != NULL) {
            int actionMenuItemsCount = 0;
            int actionMenuItems[6];
            switch ((v16->fid & 0xF000000) >> 24) {
            case OBJ_TYPE_ITEM:
                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_USE;
                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_LOOK;
                if (itemGetType(v16) == ITEM_TYPE_CONTAINER) {
                    actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_INVENTORY;
                    actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_USE_SKILL;
                }
                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_CANCEL;
                break;
            case OBJ_TYPE_CRITTER:
                if (v16 == gDude) {
                    actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_ROTATE;
                } else {
                    if (obj_action_can_talk_to(v16)) {
                        if (!isInCombat()) {
                            actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_TALK;
                        }
                    } else {
                        if (!critter_flag_check(v16->pid, 32)) {
                            actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_USE;
                        }
                    }

                    if (actionCheckPush(gDude, v16)) {
                        actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_PUSH;
                    }
                }

                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_LOOK;
                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_INVENTORY;
                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_USE_SKILL;
                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_CANCEL;
                break;
            case OBJ_TYPE_SCENERY:
                if (obj_action_can_use(v16)) {
                    actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_USE;
                }

                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_LOOK;
                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_INVENTORY;
                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_USE_SKILL;
                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_CANCEL;
                break;
            case OBJ_TYPE_WALL:
                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_LOOK;
                if (obj_action_can_use(v16)) {
                    actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_INVENTORY;
                }
                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_CANCEL;
                break;
            }

            if (gameMouseRenderActionMenuItems(mouseX, mouseY, actionMenuItems, actionMenuItemsCount, stru_6AC9F0.right - stru_6AC9F0.left + 1, stru_6AC9F0.bottom - stru_6AC9F0.top - 99) == 0) {
                Rect v43;
                int fid = buildFid(6, 283, 0, 0, 0);
                if (objectSetFid(gGameMouseHexCursor, fid, &v43) == 0 && gmouse_3d_move_to(mouseX, mouseY, gElevation, &v43) == 0) {
                    tileWindowRefreshRect(&v43, gElevation);
                    isoDisable();

                    int v33 = mouseY;
                    int actionIndex = 0;
                    while ((mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_UP) == 0) {
                        get_input();

                        if (dword_5186CC != 0) {
                            actionMenuItems[actionIndex] = 0;
                        }

                        int v48;
                        int v47;
                        mouseGetPosition(&v48, &v47);

                        if (abs(v47 - v33) > 10) {
                            if (v33 >= v47) {
                                actionIndex -= 1;
                            } else {
                                actionIndex += 1;
                            }

                            if (gameMouseHighlightActionMenuItemAtIndex(actionIndex) == 0) {
                                tileWindowRefreshRect(&v43, gElevation);
                            }
                            v33 = v47;
                        }
                    }

                    isoEnable();

                    dword_518C84 = false;
                    gGameMouseLastX = mouseX;
                    gGameMouseLastY = mouseY;
                    dword_518C88 = get_time();

                    mouse_set_position(mouseX, v33);

                    if (gameMouseUpdateHexCursorFid(&v43) == 0) {
                        tileWindowRefreshRect(&v43, gElevation);
                    }

                    switch (actionMenuItems[actionIndex]) {
                    case GAME_MOUSE_ACTION_MENU_ITEM_INVENTORY:
                        inventoryOpenUseItemOn(v16);
                        break;
                    case GAME_MOUSE_ACTION_MENU_ITEM_LOOK:
                        if (obj_examine(gDude, v16) == -1) {
                            obj_look_at(gDude, v16);
                        }
                        break;
                    case GAME_MOUSE_ACTION_MENU_ITEM_ROTATE:
                        if (objectRotateClockwise(v16, &v43) == 0) {
                            tileWindowRefreshRect(&v43, v16->elevation);
                        }
                        break;
                    case GAME_MOUSE_ACTION_MENU_ITEM_TALK:
                        actionTalk(gDude, v16);
                        break;
                    case GAME_MOUSE_ACTION_MENU_ITEM_USE:
                        switch ((v16->fid & 0xF000000) >> 24) {
                        case OBJ_TYPE_SCENERY:
                            action_use_an_object(gDude, v16);
                            break;
                        case OBJ_TYPE_CRITTER:
                            action_loot_container(gDude, v16);
                            break;
                        default:
                            actionPickUp(gDude, v16);
                            break;
                        }
                        break;
                    case GAME_MOUSE_ACTION_MENU_ITEM_USE_SKILL:
                        if (1) {
                            int skill = -1;

                            int rc = skilldexOpen();
                            switch (rc) {
                            case SKILLDEX_RC_SNEAK:
                                action_skill_use(SKILL_SNEAK);
                                break;
                            case SKILLDEX_RC_LOCKPICK:
                                skill = SKILL_LOCKPICK;
                                break;
                            case SKILLDEX_RC_STEAL:
                                skill = SKILL_STEAL;
                                break;
                            case SKILLDEX_RC_TRAPS:
                                skill = SKILL_TRAPS;
                                break;
                            case SKILLDEX_RC_FIRST_AID:
                                skill = SKILL_FIRST_AID;
                                break;
                            case SKILLDEX_RC_DOCTOR:
                                skill = SKILL_DOCTOR;
                                break;
                            case SKILLDEX_RC_SCIENCE:
                                skill = SKILL_SCIENCE;
                                break;
                            case SKILLDEX_RC_REPAIR:
                                skill = SKILL_REPAIR;
                                break;
                            }

                            if (skill != -1) {
                                actionUseSkill(gDude, v16, skill);
                            }
                        }
                        break;
                    case GAME_MOUSE_ACTION_MENU_ITEM_PUSH:
                        actionPush(gDude, v16);
                        break;
                    }
                }
            }
        }
    }
}

// 0x44C840
int gameMouseSetCursor(int cursor)
{
    if (!gGameMouseInitialized) {
        return -1;
    }

    if (cursor != MOUSE_CURSOR_ARROW && cursor == gGameMouseCursor && (gGameMouseCursor < 25 || gGameMouseCursor >= 27)) {
        return -1;
    }

    CacheEntry* mouseCursorFrmHandle;
    int fid = buildFid(6, gGameMouseCursorFrmIds[cursor], 0, 0, 0);
    Art* mouseCursorFrm = artLock(fid, &mouseCursorFrmHandle);
    if (mouseCursorFrm == NULL) {
        return -1;
    }

    bool shouldUpdate = true;
    int frame = 0;
    if (cursor >= FIRST_GAME_MOUSE_ANIMATED_CURSOR) {
        unsigned int tick = get_time();

        if (!(gGameMouseHexCursor->flags & 1)) {
            gameMouseObjectsHide();
        }

        unsigned int delay = 1000 / artGetFramesPerSecond(mouseCursorFrm);
        if (getTicksBetween(tick, gGameMouseAnimatedCursorLastUpdateTimestamp) < delay) {
            shouldUpdate = false;
        } else {
            if (artGetFrameCount(mouseCursorFrm) <= gGameMouseAnimatedCursorNextFrame) {
                gGameMouseAnimatedCursorNextFrame = 0;
            }

            frame = gGameMouseAnimatedCursorNextFrame;
            gGameMouseAnimatedCursorLastUpdateTimestamp = tick;
            gGameMouseAnimatedCursorNextFrame++;
        }
    }

    if (!shouldUpdate) {
        return -1;
    }

    int width = artGetWidth(mouseCursorFrm, frame, 0);
    int height = artGetHeight(mouseCursorFrm, frame, 0);

    int offsetX;
    int offsetY;
    artGetRotationOffsets(mouseCursorFrm, 0, &offsetX, &offsetY);

    offsetX = width / 2 - offsetX;
    offsetY = height - 1 - offsetY;

    unsigned char* mouseCursorFrmData = artGetFrameData(mouseCursorFrm, frame, 0);
    if (mouseSetFrame(mouseCursorFrmData, width, height, width, offsetX, offsetY, 0) != 0) {
        return -1;
    }

    if (gGameMouseCursorFrmHandle != INVALID_CACHE_ENTRY) {
        artUnlock(gGameMouseCursorFrmHandle);
    }

    gGameMouseCursor = cursor;
    gGameMouseCursorFrmHandle = mouseCursorFrmHandle;

    return 0;
}

// 0x44C9E8
int gameMouseGetCursor()
{
    return gGameMouseCursor;
}

// 0x44C9F8
void gmouse_3d_enable_modes()
{
    dword_518D34 = 1;
}

// 0x44CA18
void gameMouseSetMode(int mode)
{
    if (!gGameMouseInitialized) {
        return;
    }

    if (!dword_518D34) {
        return;
    }

    if (mode == gGameMouseMode) {
        return;
    }

    int fid = buildFid(6, 0, 0, 0, 0);
    gameMouseSetBouncingCursorFid(fid);

    fid = buildFid(6, gGameMouseModeFrmIds[mode], 0, 0, 0);

    Rect rect;
    if (objectSetFid(gGameMouseHexCursor, fid, &rect) == -1) {
        return;
    }

    int mouseX;
    int mouseY;
    mouseGetPosition(&mouseX, &mouseY);

    Rect r2;
    if (gmouse_3d_move_to(mouseX, mouseY, gElevation, &r2) == 0) {
        rectUnion(&rect, &r2, &rect);
    }

    int v5 = 0;
    if (gGameMouseMode == GAME_MOUSE_MODE_CROSSHAIR) {
        v5 = -1;
    }

    if (mode != 0) {
        if (mode == GAME_MOUSE_MODE_CROSSHAIR) {
            v5 = 1;
        }

        if (gGameMouseMode == 0) {
            if (objectDisableOutline(gGameMouseHexCursor, &r2) == 0) {
                rectUnion(&rect, &r2, &rect);
            }
        }
    } else {
        if (objectEnableOutline(gGameMouseHexCursor, &r2) == 0) {
            rectUnion(&rect, &r2, &rect);
        }
    }

    gGameMouseMode = mode;
    dword_518C84 = false;
    dword_518C88 = get_time();

    tileWindowRefreshRect(&rect, gElevation);

    switch (v5) {
    case 1:
        combat_outline_on();
        break;
    case -1:
        combat_outline_off();
        break;
    }
}

// 0x44CB6C
int gameMouseGetMode()
{
    return gGameMouseMode;
}

// 0x44CB74
void gameMouseCycleMode()
{
    int mode = (gGameMouseMode + 1) % 3;

    if (isInCombat()) {
        Object* item;
        if (interfaceGetActiveItem(&item) == 0) {
            if (item != NULL && itemGetType(item) != ITEM_TYPE_WEAPON && mode == GAME_MOUSE_MODE_CROSSHAIR) {
                mode = GAME_MOUSE_MODE_MOVE;
            }
        }
    } else {
        if (mode == GAME_MOUSE_MODE_CROSSHAIR) {
            mode = GAME_MOUSE_MODE_MOVE;
        }
    }

    gameMouseSetMode(mode);
}

// 0x44CBD0
void gmouse_3d_refresh()
{
    gGameMouseLastX = -1;
    gGameMouseLastY = -1;
    dword_518C84 = false;
    dword_518C88 = 0;
    gameMouseRefresh();
}

// 0x44CBFC
int gameMouseSetBouncingCursorFid(int fid)
{
    if (!gGameMouseInitialized) {
        return -1;
    }

    if (!artExists(fid)) {
        return -1;
    }

    if (gGameMouseBouncingCursor->fid == fid) {
        return -1;
    }

    if (!dword_518C00) {
        return objectSetFid(gGameMouseBouncingCursor, fid, NULL);
    }

    int v1 = 0;

    Rect oldRect;
    if (gGameMouseBouncingCursor->fid != -1) {
        objectGetRect(gGameMouseBouncingCursor, &oldRect);
        v1 |= 1;
    }

    int rc = -1;

    Rect rect;
    if (objectSetFid(gGameMouseBouncingCursor, fid, &rect) == 0) {
        rc = 0;
        v1 |= 2;
    }

    if ((gGameMouseHexCursor->flags & OBJECT_IS_INVISIBLE) == 0) {
        if (v1 == 1) {
            tileWindowRefreshRect(&oldRect, gElevation);
        } else if (v1 == 2) {
            tileWindowRefreshRect(&rect, gElevation);
        } else if (v1 == 3) {
            rectUnion(&oldRect, &rect, &oldRect);
            tileWindowRefreshRect(&oldRect, gElevation);
        }
    }

    return rc;
}

// 0x44CD0C
void gameMouseResetBouncingCursorFid()
{
    int fid = buildFid(6, 0, 0, 0, 0);
    gameMouseSetBouncingCursorFid(fid);
}

// 0x44CD2C
void gameMouseObjectsShow()
{
    if (!gGameMouseInitialized) {
        return;
    }

    int v2 = 0;

    Rect rect1;
    if (objectShow(gGameMouseBouncingCursor, &rect1) == 0) {
        v2 |= 1;
    }

    Rect rect2;
    if (objectShow(gGameMouseHexCursor, &rect2) == 0) {
        v2 |= 2;
    }

    Rect tmp;
    if (gGameMouseMode != GAME_MOUSE_MODE_MOVE) {
        if (objectDisableOutline(gGameMouseHexCursor, &tmp) == 0) {
            if ((v2 & 2) != 0) {
                rectUnion(&rect2, &tmp, &rect2);
            } else {
                memcpy(&rect2, &tmp, sizeof(rect2));
                v2 |= 2;
            }
        }
    }

    if (gameMouseUpdateHexCursorFid(&tmp) == 0) {
        if ((v2 & 2) != 0) {
            rectUnion(&rect2, &tmp, &rect2);
        } else {
            memcpy(&rect2, &tmp, sizeof(rect2));
            v2 |= 2;
        }
    }

    if (v2 != 0) {
        Rect* rect;
        switch (v2) {
        case 1:
            rect = &rect1;
            break;
        case 2:
            rect = &rect2;
            break;
        case 3:
            rectUnion(&rect1, &rect2, &rect1);
            rect = &rect1;
            break;
        default:
            // NOTE: Should be unreachable.
            __assume(0);
        }

        tileWindowRefreshRect(rect, gElevation);
    }

    dword_518C84 = false;
    dword_518C88 = get_time() - 250;
}

// 0x44CE34
void gameMouseObjectsHide()
{
    if (!gGameMouseInitialized) {
        return;
    }

    int v1 = 0;

    Rect rect1;
    if (objectHide(gGameMouseBouncingCursor, &rect1) == 0) {
        v1 |= 1;
    }

    Rect rect2;
    if (objectHide(gGameMouseHexCursor, &rect2) == 0) {
        v1 |= 2;
    }

    if (v1 == 1) {
        tileWindowRefreshRect(&rect1, gElevation);
    } else if (v1 == 2) {
        tileWindowRefreshRect(&rect2, gElevation);
    } else if (v1 == 3) {
        rectUnion(&rect1, &rect2, &rect1);
        tileWindowRefreshRect(&rect1, gElevation);
    }
}

// 0x44CEB0
bool gameMouseObjectsIsVisible()
{
    return (gGameMouseHexCursor->flags & OBJECT_IS_INVISIBLE) == 0;
}

// 0x44CEC4
Object* gameMouseGetObjectUnderCursor(int objectType, bool a2, int elevation)
{
    int mouseX;
    int mouseY;
    mouseGetPosition(&mouseX, &mouseY);

    bool v13 = false;
    if (objectType == -1) {
        if (square_roof_intersect(mouseX, mouseY, elevation)) {
            if (obj_intersects_with(gEgg, mouseX, mouseY) == 0) {
                v13 = true;
            }
        }
    }

    Object* v4 = NULL;
    if (!v13) {
        ObjectWithFlags* entries;
        int count = obj_create_intersect_list(mouseX, mouseY, elevation, objectType, &entries);
        for (int index = count - 1; index >= 0; index--) {
            ObjectWithFlags* ptr = &(entries[index]);
            if (a2 || gDude != ptr->object) {
                v4 = ptr->object;
                if ((ptr->flags & 0x01) != 0) {
                    if ((ptr->flags & 0x04) == 0) {
                        if ((ptr->object->fid & 0xF000000) >> 24 != OBJ_TYPE_CRITTER || (ptr->object->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_DEAD)) == 0) {
                            break;
                        }
                    }
                }
            }
        }

        if (count != 0) {
            obj_delete_intersect_list(&entries);
        }
    }
    return v4;
}

// 0x44CFA0
int gameMouseRenderPrimaryAction(int x, int y, int menuItem, int width, int height)
{
    CacheEntry* menuItemFrmHandle;
    int menuItemFid = buildFid(6, gGameMouseActionMenuItemFrmIds[menuItem], 0, 0, 0);
    Art* menuItemFrm = artLock(menuItemFid, &menuItemFrmHandle);
    if (menuItemFrm == NULL) {
        return -1;
    }

    CacheEntry* arrowFrmHandle;
    int arrowFid = buildFid(6, gGameMouseModeFrmIds[GAME_MOUSE_MODE_ARROW], 0, 0, 0);
    Art* arrowFrm = artLock(arrowFid, &arrowFrmHandle);
    if (arrowFrm == NULL) {
        artUnlock(menuItemFrmHandle);
        // FIXME: Why this is success?
        return 0;
    }

    unsigned char* arrowFrmData = artGetFrameData(arrowFrm, 0, 0);
    int arrowFrmWidth = artGetWidth(arrowFrm, 0, 0);
    int arrowFrmHeight = artGetHeight(arrowFrm, 0, 0);

    unsigned char* menuItemFrmData = artGetFrameData(menuItemFrm, 0, 0);
    int menuItemFrmWidth = artGetWidth(menuItemFrm, 0, 0);
    int menuItemFrmHeight = artGetHeight(menuItemFrm, 0, 0);

    unsigned char* arrowFrmDest = gGameMouseActionPickFrmData;
    unsigned char* menuItemFrmDest = gGameMouseActionPickFrmData;

    dword_518CC0 = 0;
    dword_518CC4 = 0;

    gGameMouseActionPickFrm->xOffsets[0] = gGameMouseActionPickFrmWidth / 2;
    gGameMouseActionPickFrm->yOffsets[0] = gGameMouseActionPickFrmHeight - 1;

    int maxX = x + menuItemFrmWidth + arrowFrmWidth - 1;
    int maxY = y + menuItemFrmHeight - 1;
    int shiftY = maxY - height + 2;

    if (maxX < width) {
        menuItemFrmDest += arrowFrmWidth;
        if (maxY >= height) {
            dword_518CC4 = shiftY;
            gGameMouseActionPickFrm->yOffsets[0] -= shiftY;
            arrowFrmDest += gGameMouseActionPickFrmWidth * shiftY;
        }
    } else {
        artUnlock(arrowFrmHandle);

        arrowFid = buildFid(6, 285, 0, 0, 0);
        arrowFrm = artLock(arrowFid, &arrowFrmHandle);
        arrowFrmData = artGetFrameData(arrowFrm, 0, 0);
        arrowFrmDest += menuItemFrmWidth;

        gGameMouseActionPickFrm->xOffsets[0] = -gGameMouseActionPickFrm->xOffsets[0];
        dword_518CC0 += menuItemFrmWidth + arrowFrmWidth;

        if (maxY >= height) {
            dword_518CC4 += shiftY;
            gGameMouseActionPickFrm->yOffsets[0] -= shiftY;

            arrowFrmDest += gGameMouseActionPickFrmWidth * shiftY;
        }
    }

    memset(gGameMouseActionPickFrmData, 0, gGameMouseActionPickFrmDataSize);

    blitBufferToBuffer(arrowFrmData, arrowFrmWidth, arrowFrmHeight, arrowFrmWidth, arrowFrmDest, gGameMouseActionPickFrmWidth);
    blitBufferToBuffer(menuItemFrmData, menuItemFrmWidth, menuItemFrmHeight, menuItemFrmWidth, menuItemFrmDest, gGameMouseActionPickFrmWidth);

    artUnlock(arrowFrmHandle);
    artUnlock(menuItemFrmHandle);

    return 0;
}

// 0x44D200
int gmouse_3d_pick_frame_hot(int* a1, int* a2)
{
    *a1 = dword_518CC0;
    *a2 = dword_518CC4;
    return 0;
}

// 0x44D214
int gameMouseRenderActionMenuItems(int x, int y, const int* menuItems, int menuItemsLength, int width, int height)
{
    off_518D18 = NULL;
    gGameMouseActionMenuHighlightedItemIndex = 0;
    gGameMouseActionMenuItemsLength = 0;

    if (menuItems == NULL) {
        return -1;
    }

    if (menuItemsLength == 0 || menuItemsLength >= GAME_MOUSE_ACTION_MENU_ITEM_COUNT) {
        return -1;
    }

    CacheEntry* menuItemFrmHandles[GAME_MOUSE_ACTION_MENU_ITEM_COUNT];
    Art* menuItemFrms[GAME_MOUSE_ACTION_MENU_ITEM_COUNT];

    for (int index = 0; index < menuItemsLength; index++) {
        int frmId = gGameMouseActionMenuItemFrmIds[menuItems[index]] & 0xFFFF;
        if (index == 0) {
            frmId -= 1;
        }

        int fid = buildFid(6, frmId, 0, 0, 0);

        menuItemFrms[index] = artLock(fid, &(menuItemFrmHandles[index]));
        if (menuItemFrms[index] == NULL) {
            while (--index >= 0) {
                artUnlock(menuItemFrmHandles[index]);
            }
            return -1;
        }
    }

    int fid = buildFid(6, gGameMouseModeFrmIds[GAME_MOUSE_MODE_ARROW], 0, 0, 0);
    CacheEntry* arrowFrmHandle;
    Art* arrowFrm = artLock(fid, &arrowFrmHandle);
    if (arrowFrm == NULL) {
        // FIXME: Unlock arts.
        return -1;
    }

    int arrowWidth = artGetWidth(arrowFrm, 0, 0);
    int arrowHeight = artGetHeight(arrowFrm, 0, 0);

    int menuItemWidth = artGetWidth(menuItemFrms[0], 0, 0);
    int menuItemHeight = artGetHeight(menuItemFrms[0], 0, 0);

    dword_518CA0 = 0;
    dword_518CA4 = 0;

    gGameMouseActionMenuFrm->xOffsets[0] = gGameMouseActionMenuFrmWidth / 2;
    gGameMouseActionMenuFrm->yOffsets[0] = gGameMouseActionMenuFrmHeight - 1;

    int v60 = y + menuItemsLength * menuItemHeight - 1;
    int v24 = v60 - height + 2;
    unsigned char* v22 = gGameMouseActionMenuFrmData;
    unsigned char* v58 = v22;

    unsigned char* arrowData;
    if (x + arrowWidth + menuItemWidth - 1 < width) {
        arrowData = artGetFrameData(arrowFrm, 0, 0);
        v58 = v22 + arrowWidth;
        if (height <= v60) {
            dword_518CA4 += v24;
            v22 += gGameMouseActionMenuFrmWidth * v24;
            gGameMouseActionMenuFrm->yOffsets[0] -= v24;
        }
    } else {
        // Mirrored arrow (from left to right).
        fid = buildFid(6, 285, 0, 0, 0);
        arrowFrm = artLock(fid, &arrowFrmHandle);
        arrowData = artGetFrameData(arrowFrm, 0, 0);
        gGameMouseActionMenuFrm->xOffsets[0] = -gGameMouseActionMenuFrm->xOffsets[0];
        dword_518CA0 += menuItemWidth + arrowWidth;
        if (v60 >= height) {
            dword_518CA4 += v24;
            gGameMouseActionMenuFrm->yOffsets[0] -= v24;
            v22 += gGameMouseActionMenuFrmWidth * v24;
        }
    }

    memset(gGameMouseActionMenuFrmData, 0, gGameMouseActionMenuFrmDataSize);
    blitBufferToBuffer(arrowData, arrowWidth, arrowHeight, arrowWidth, v22, gGameMouseActionPickFrmWidth);

    unsigned char* v38 = v58;
    for (int index = 0; index < menuItemsLength; index++) {
        unsigned char* data = artGetFrameData(menuItemFrms[index], 0, 0);
        blitBufferToBuffer(data, menuItemWidth, menuItemHeight, menuItemWidth, v38, gGameMouseActionPickFrmWidth);
        v38 += gGameMouseActionMenuFrmWidth * menuItemHeight;
    }

    artUnlock(arrowFrmHandle);

    for (int index = 0; index < menuItemsLength; index++) {
        artUnlock(menuItemFrmHandles[index]);
    }

    memcpy(gGameMouseActionMenuItems, menuItems, sizeof(*gGameMouseActionMenuItems) * menuItemsLength);
    gGameMouseActionMenuItemsLength = menuItemsLength;
    off_518D18 = v58;

    Sound* sound = soundEffectLoad("iaccuxx1", NULL);
    if (sound != NULL) {
        soundEffectPlay(sound);
    }

    return 0;
}

// 0x44D630
int gameMouseHighlightActionMenuItemAtIndex(int menuItemIndex)
{
    if (menuItemIndex < 0 || menuItemIndex >= gGameMouseActionMenuItemsLength) {
        return -1;
    }

    CacheEntry* handle;
    int fid = buildFid(6, gGameMouseActionMenuItemFrmIds[gGameMouseActionMenuItems[gGameMouseActionMenuHighlightedItemIndex]], 0, 0, 0);
    Art* art = artLock(fid, &handle);
    if (art == NULL) {
        return -1;
    }

    int width = artGetWidth(art, 0, 0);
    int height = artGetHeight(art, 0, 0);
    unsigned char* data = artGetFrameData(art, 0, 0);
    blitBufferToBuffer(data, width, height, width, off_518D18 + gGameMouseActionMenuFrmWidth * height * gGameMouseActionMenuHighlightedItemIndex, gGameMouseActionMenuFrmWidth);
    artUnlock(handle);

    fid = buildFid(6, gGameMouseActionMenuItemFrmIds[gGameMouseActionMenuItems[menuItemIndex]] - 1, 0, 0, 0);
    art = artLock(fid, &handle);
    if (art == NULL) {
        return -1;
    }

    data = artGetFrameData(art, 0, 0);
    blitBufferToBuffer(data, width, height, width, off_518D18 + gGameMouseActionMenuFrmWidth * height * menuItemIndex, gGameMouseActionMenuFrmWidth);
    artUnlock(handle);

    gGameMouseActionMenuHighlightedItemIndex = menuItemIndex;

    return 0;
}

// 0x44D774
int gameMouseRenderAccuracy(const char* string, int color)
{
    CacheEntry* crosshairFrmHandle;
    int fid = buildFid(6, gGameMouseModeFrmIds[GAME_MOUSE_MODE_CROSSHAIR], 0, 0, 0);
    Art* crosshairFrm = artLock(fid, &crosshairFrmHandle);
    if (crosshairFrm == NULL) {
        return -1;
    }

    memset(gGameMouseActionHitFrmData, 0, gGameMouseActionHitFrmDataSize);

    int crosshairFrmWidth = artGetWidth(crosshairFrm, 0, 0);
    int crosshairFrmHeight = artGetHeight(crosshairFrm, 0, 0);
    unsigned char* crosshairFrmData = artGetFrameData(crosshairFrm, 0, 0);
    blitBufferToBuffer(crosshairFrmData,
        crosshairFrmWidth,
        crosshairFrmHeight,
        crosshairFrmWidth,
        gGameMouseActionHitFrmData,
        gGameMouseActionHitFrmWidth);

    int oldFont = fontGetCurrent();
    fontSetCurrent(101);

    fontDrawText(gGameMouseActionHitFrmData + gGameMouseActionHitFrmWidth + crosshairFrmWidth + 1,
        string,
        gGameMouseActionHitFrmWidth - crosshairFrmWidth,
        gGameMouseActionHitFrmWidth,
        color);

    bufferOutline(gGameMouseActionHitFrmData + crosshairFrmWidth,
        gGameMouseActionHitFrmWidth - crosshairFrmWidth,
        gGameMouseActionHitFrmHeight,
        gGameMouseActionHitFrmWidth,
        byte_6A38D0[0]);

    fontSetCurrent(oldFont);

    artUnlock(crosshairFrmHandle);

    return 0;
}

// 0x44D878
int gameMouseRenderActionPoints(const char* string, int color)
{
    memset(gGameMouseHexCursorFrmData, 0, gGameMouseHexCursorFrmWidth * gGameMouseHexCursorHeight);

    if (*string == '\0') {
        return 0;
    }

    int oldFont = fontGetCurrent();
    fontSetCurrent(101);

    int length = fontGetStringWidth(string);
    fontDrawText(gGameMouseHexCursorFrmData + gGameMouseHexCursorFrmWidth * (gGameMouseHexCursorHeight - fontGetLineHeight()) / 2 + (gGameMouseHexCursorFrmWidth - length) / 2, string, gGameMouseHexCursorFrmWidth, gGameMouseHexCursorFrmWidth, color);

    bufferOutline(gGameMouseHexCursorFrmData, gGameMouseHexCursorFrmWidth, gGameMouseHexCursorHeight, gGameMouseHexCursorFrmWidth, byte_6A38D0[0]);

    fontSetCurrent(oldFont);

    int fid = buildFid(6, 1, 0, 0, 0);
    gameMouseSetBouncingCursorFid(fid);

    return 0;
}

// 0x44D954
void gameMouseLoadItemHighlight()
{
    bool itemHighlight;
    if (configGetBool(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_ITEM_HIGHLIGHT_KEY, &itemHighlight)) {
        gGameMouseItemHighlightEnabled = itemHighlight;
    }
}

// 0x44D984
int gameMouseObjectsInit()
{
    int fid;

    if (gGameMouseObjectsInitialized) {
        return -1;
    }

    fid = buildFid(6, 0, 0, 0, 0);
    if (objectCreateWithFidPid(&gGameMouseBouncingCursor, fid, -1) != 0) {
        return -1;
    }

    fid = buildFid(6, 1, 0, 0, 0);
    if (objectCreateWithFidPid(&gGameMouseHexCursor, fid, -1) != 0) {
        return -1;
    }

    if (objectSetOutline(gGameMouseHexCursor, OUTLINE_PALETTED | OUTLINE_TYPE_2, NULL) != 0) {
        return -1;
    }

    if (gameMouseActionMenuInit() != 0) {
        return -1;
    }

    gGameMouseBouncingCursor->flags |= 0x20000000;
    gGameMouseBouncingCursor->flags |= 0x04;
    gGameMouseBouncingCursor->flags |= 0x0400;
    gGameMouseBouncingCursor->flags |= 0x80000000;
    gGameMouseBouncingCursor->flags |= 0x10;

    gGameMouseHexCursor->flags |= 0x0400;
    gGameMouseHexCursor->flags |= 0x04;
    gGameMouseHexCursor->flags |= 0x20000000;
    gGameMouseHexCursor->flags |= 0x80000000;
    gGameMouseHexCursor->flags |= 0x10;

    obj_toggle_flat(gGameMouseHexCursor, NULL);

    int x;
    int y;
    mouseGetPosition(&x, &y);

    Rect v9;
    gmouse_3d_move_to(x, y, gElevation, &v9);

    gGameMouseObjectsInitialized = true;

    gameMouseLoadItemHighlight();

    return 0;
}

// NOTE: Inlined.
//
// 0x44DAC0
int gameMouseObjectsReset()
{
    if (!gGameMouseObjectsInitialized) {
        return -1;
    }

    // NOTE: Uninline.
    gmouse_3d_enable_modes();

    // NOTE: Uninline.
    gameMouseResetBouncingCursorFid();

    gameMouseSetMode(GAME_MOUSE_MODE_MOVE);
    gameMouseObjectsShow();

    gGameMouseLastX = -1;
    gGameMouseLastY = -1;
    dword_518C84 = false;
    dword_518C88 = get_time();
    gameMouseLoadItemHighlight();

    return 0;
}

// NOTE: Inlined.
//
// 0x44DB34
void gameMouseObjectsFree()
{
    if (gGameMouseObjectsInitialized) {
        gameMouseActionMenuFree();

        gGameMouseBouncingCursor->flags &= ~0x04;
        gGameMouseHexCursor->flags &= ~0x04;

        objectDestroy(gGameMouseBouncingCursor, NULL);
        objectDestroy(gGameMouseHexCursor, NULL);

        gGameMouseObjectsInitialized = false;
    }
}

// 0x44DB78
int gameMouseActionMenuInit()
{
    int fid;

    // actmenu.frm - action menu
    fid = buildFid(6, 283, 0, 0, 0);
    gGameMouseActionMenuFrm = artLock(fid, &gGameMouseActionMenuFrmHandle);
    if (gGameMouseActionMenuFrm == NULL) {
        goto err;
    }

    // actpick.frm - action pick
    fid = buildFid(6, 282, 0, 0, 0);
    gGameMouseActionPickFrm = artLock(fid, &gGameMouseActionPickFrmHandle);
    if (gGameMouseActionPickFrm == NULL) {
        goto err;
    }

    // acttohit.frm - action to hit
    fid = buildFid(6, 284, 0, 0, 0);
    gGameMouseActionHitFrm = artLock(fid, &gGameMouseActionHitFrmHandle);
    if (gGameMouseActionHitFrm == NULL) {
        goto err;
    }

    // blank.frm - used be mset000.frm for top of bouncing mouse cursor
    fid = buildFid(6, 0, 0, 0, 0);
    gGameMouseBouncingCursorFrm = artLock(fid, &gGameMouseBouncingCursorFrmHandle);
    if (gGameMouseBouncingCursorFrm == NULL) {
        goto err;
    }

    // msef000.frm - hex mouse cursor
    fid = buildFid(6, 1, 0, 0, 0);
    gGameMouseHexCursorFrm = artLock(fid, &gGameMouseHexCursorFrmHandle);
    if (gGameMouseHexCursorFrm == NULL) {
        goto err;
    }

    gGameMouseActionMenuFrmWidth = artGetWidth(gGameMouseActionMenuFrm, 0, 0);
    gGameMouseActionMenuFrmHeight = artGetHeight(gGameMouseActionMenuFrm, 0, 0);
    gGameMouseActionMenuFrmDataSize = gGameMouseActionMenuFrmWidth * gGameMouseActionMenuFrmHeight;
    gGameMouseActionMenuFrmData = artGetFrameData(gGameMouseActionMenuFrm, 0, 0);

    gGameMouseActionPickFrmWidth = artGetWidth(gGameMouseActionPickFrm, 0, 0);
    gGameMouseActionPickFrmHeight = artGetHeight(gGameMouseActionPickFrm, 0, 0);
    gGameMouseActionPickFrmDataSize = gGameMouseActionPickFrmWidth * gGameMouseActionPickFrmHeight;
    gGameMouseActionPickFrmData = artGetFrameData(gGameMouseActionPickFrm, 0, 0);

    gGameMouseActionHitFrmWidth = artGetWidth(gGameMouseActionHitFrm, 0, 0);
    gGameMouseActionHitFrmHeight = artGetHeight(gGameMouseActionHitFrm, 0, 0);
    gGameMouseActionHitFrmDataSize = gGameMouseActionHitFrmWidth * gGameMouseActionHitFrmHeight;
    gGameMouseActionHitFrmData = artGetFrameData(gGameMouseActionHitFrm, 0, 0);

    gGameMouseBouncingCursorFrmWidth = artGetWidth(gGameMouseBouncingCursorFrm, 0, 0);
    gGameMouseBouncingCursorFrmHeight = artGetHeight(gGameMouseBouncingCursorFrm, 0, 0);
    gGameMouseBouncingCursorFrmDataSize = gGameMouseBouncingCursorFrmWidth * gGameMouseBouncingCursorFrmHeight;
    gGameMouseBouncingCursorFrmData = artGetFrameData(gGameMouseBouncingCursorFrm, 0, 0);

    gGameMouseHexCursorFrmWidth = artGetWidth(gGameMouseHexCursorFrm, 0, 0);
    gGameMouseHexCursorHeight = artGetHeight(gGameMouseHexCursorFrm, 0, 0);
    gGameMouseHexCursorDataSize = gGameMouseHexCursorFrmWidth * gGameMouseHexCursorHeight;
    gGameMouseHexCursorFrmData = artGetFrameData(gGameMouseHexCursorFrm, 0, 0);

    return 0;

err:

    // NOTE: Original code is different. There is no call to this function.
    // Instead it either use deep nesting or bunch of goto's to unwind
    // locked frms from the point of failure.
    gameMouseActionMenuFree();

    return -1;
}

// 0x44DE44
void gameMouseActionMenuFree()
{
    if (gGameMouseBouncingCursorFrmHandle != INVALID_CACHE_ENTRY) {
        artUnlock(gGameMouseBouncingCursorFrmHandle);
    }
    gGameMouseBouncingCursorFrm = NULL;
    gGameMouseBouncingCursorFrmHandle = INVALID_CACHE_ENTRY;

    if (gGameMouseHexCursorFrmHandle != INVALID_CACHE_ENTRY) {
        artUnlock(gGameMouseHexCursorFrmHandle);
    }
    gGameMouseHexCursorFrm = NULL;
    gGameMouseHexCursorFrmHandle = INVALID_CACHE_ENTRY;

    if (gGameMouseActionHitFrmHandle != INVALID_CACHE_ENTRY) {
        artUnlock(gGameMouseActionHitFrmHandle);
    }
    gGameMouseActionHitFrm = NULL;
    gGameMouseActionHitFrmHandle = INVALID_CACHE_ENTRY;

    if (gGameMouseActionMenuFrmHandle != INVALID_CACHE_ENTRY) {
        artUnlock(gGameMouseActionMenuFrmHandle);
    }
    gGameMouseActionMenuFrm = NULL;
    gGameMouseActionMenuFrmHandle = INVALID_CACHE_ENTRY;

    if (gGameMouseActionPickFrmHandle != INVALID_CACHE_ENTRY) {
        artUnlock(gGameMouseActionPickFrmHandle);
    }

    gGameMouseActionPickFrm = NULL;
    gGameMouseActionPickFrmHandle = INVALID_CACHE_ENTRY;

    gGameMouseActionPickFrmData = NULL;
    gGameMouseActionPickFrmWidth = 0;
    gGameMouseActionPickFrmHeight = 0;
    gGameMouseActionPickFrmDataSize = 0;
}

// 0x44DF40
int gameMouseUpdateHexCursorFid(Rect* rect)
{
    int fid = buildFid(6, gGameMouseModeFrmIds[gGameMouseMode], 0, 0, 0);
    if (gGameMouseHexCursor->fid == fid) {
        return -1;
    }

    return objectSetFid(gGameMouseHexCursor, fid, rect);
}

// 0x44DF94
int gmouse_3d_move_to(int x, int y, int elevation, Rect* a4)
{
    if (dword_518C00 == 0) {
        if (gGameMouseMode != GAME_MOUSE_MODE_MOVE) {
            int offsetX = 0;
            int offsetY = 0;
            CacheEntry* hexCursorFrmHandle;
            Art* hexCursorFrm = artLock(gGameMouseHexCursor->fid, &hexCursorFrmHandle);
            if (hexCursorFrm != NULL) {
                artGetRotationOffsets(hexCursorFrm, 0, &offsetX, &offsetY);

                int frameOffsetX;
                int frameOffsetY;
                artGetFrameOffsets(hexCursorFrm, 0, 0, &frameOffsetX, &frameOffsetY);

                offsetX += frameOffsetX;
                offsetY += frameOffsetY;

                artUnlock(hexCursorFrmHandle);
            }

            obj_move(gGameMouseHexCursor, x + offsetX, y + offsetY, elevation, a4);
        } else {
            int tile = tileFromScreenXY(x, y, 0);
            if (tile != -1) {
                int screenX;
                int screenY;

                bool v1 = false;
                Rect rect1;
                if (tileToScreenXY(tile, &screenX, &screenY, 0) == 0) {
                    if (obj_move(gGameMouseBouncingCursor, screenX + 16, screenY + 15, 0, &rect1) == 0) {
                        v1 = true;
                    }
                }

                Rect rect2;
                if (objectSetLocation(gGameMouseHexCursor, tile, elevation, &rect2) == 0) {
                    if (v1) {
                        rectUnion(&rect1, &rect2, &rect1);
                    } else {
                        rectCopy(&rect1, &rect2);
                    }

                    rectCopy(a4, &rect1);
                }
            }
        }
        return 0;
    }

    int tile;
    int x1 = 0;
    int y1 = 0;

    int fid = gGameMouseBouncingCursor->fid;
    if ((fid & 0xF000000) >> 24 == OBJ_TYPE_TILE) {
        int squareTile = square_num(x, y, elevation);
        if (squareTile == -1) {
            tile = HEX_GRID_WIDTH * (2 * (squareTile / SQUARE_GRID_WIDTH) + 1) + 2 * (squareTile % SQUARE_GRID_WIDTH) + 1;
            x1 = -8;
            y1 = 13;

            char* executable;
            configGetString(&gGameConfig, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_EXECUTABLE_KEY, &executable);
            if (stricmp(executable, "mapper") == 0) {
                if (tile_roof_visible()) {
                    if ((gDude->flags & OBJECT_IS_INVISIBLE) == 0) {
                        y1 = -83;
                    }
                }
            }
        } else {
            tile = -1;
        }
    } else {
        tile = tileFromScreenXY(x, y, elevation);
    }

    if (tile != -1) {
        bool v1 = false;

        Rect rect1;
        Rect rect2;

        if (objectSetLocation(gGameMouseBouncingCursor, tile, elevation, &rect1) == 0) {
            if (x1 != 0 || y1 != 0) {
                if (obj_offset(gGameMouseBouncingCursor, x1, y1, &rect2) == 0) {
                    rectUnion(&rect1, &rect2, &rect1);
                }
            }
            v1 = true;
        }

        if (gGameMouseMode != GAME_MOUSE_MODE_MOVE) {
            int offsetX = 0;
            int offsetY = 0;
            CacheEntry* hexCursorFrmHandle;
            Art* hexCursorFrm = artLock(gGameMouseHexCursor->fid, &hexCursorFrmHandle);
            if (hexCursorFrm != NULL) {
                artGetRotationOffsets(hexCursorFrm, 0, &offsetX, &offsetY);

                int frameOffsetX;
                int frameOffsetY;
                artGetFrameOffsets(hexCursorFrm, 0, 0, &frameOffsetX, &frameOffsetY);

                offsetX += frameOffsetX;
                offsetY += frameOffsetY;

                artUnlock(hexCursorFrmHandle);
            }

            if (obj_move(gGameMouseHexCursor, x + offsetX, y + offsetY, elevation, &rect2) == 0) {
                if (v1) {
                    rectUnion(&rect1, &rect2, &rect1);
                } else {
                    rectCopy(&rect1, &rect2);
                    v1 = true;
                }
            }
        } else {
            if (objectSetLocation(gGameMouseHexCursor, tile, elevation, &rect2) == 0) {
                if (v1) {
                    rectUnion(&rect1, &rect2, &rect1);
                } else {
                    rectCopy(&rect1, &rect2);
                    v1 = true;
                }
            }
        }

        if (v1) {
            rectCopy(a4, &rect1);
        }
    }

    return 0;
}

// 0x44E42C
int gameMouseHandleScrolling(int x, int y, int cursor)
{
    if (!dword_518C08) {
        return -1;
    }

    int flags = 0;

    if (x <= stru_6AC9F0.left) {
        flags |= SCROLLABLE_W;
    }

    if (x >= stru_6AC9F0.right) {
        flags |= SCROLLABLE_E;
    }

    if (y <= stru_6AC9F0.top) {
        flags |= SCROLLABLE_N;
    }

    if (y >= stru_6AC9F0.bottom) {
        flags |= SCROLLABLE_S;
    }

    int dx = 0;
    int dy = 0;

    switch (flags) {
    case SCROLLABLE_W:
        dx = -1;
        cursor = MOUSE_CURSOR_SCROLL_W;
        break;
    case SCROLLABLE_E:
        dx = 1;
        cursor = MOUSE_CURSOR_SCROLL_E;
        break;
    case SCROLLABLE_N:
        dy = -1;
        cursor = MOUSE_CURSOR_SCROLL_N;
        break;
    case SCROLLABLE_N | SCROLLABLE_W:
        dx = -1;
        dy = -1;
        cursor = MOUSE_CURSOR_SCROLL_NW;
        break;
    case SCROLLABLE_N | SCROLLABLE_E:
        dx = 1;
        dy = -1;
        cursor = MOUSE_CURSOR_SCROLL_NE;
        break;
    case SCROLLABLE_S:
        dy = 1;
        cursor = MOUSE_CURSOR_SCROLL_S;
        break;
    case SCROLLABLE_S | SCROLLABLE_W:
        dx = -1;
        dy = 1;
        cursor = MOUSE_CURSOR_SCROLL_SW;
        break;
    case SCROLLABLE_S | SCROLLABLE_E:
        dx = 1;
        dy = 1;
        cursor = MOUSE_CURSOR_SCROLL_SE;
        break;
    }

    if (dx == 0 && dy == 0) {
        return -1;
    }

    int rc = mapScroll(dx, dy);
    switch (rc) {
    case -1:
        // Scrolling is blocked for whatever reason, upgrade cursor to
        // appropriate blocked version.
        cursor += 8;
        // FALLTHROUGH
    case 0:
        gameMouseSetCursor(cursor);
        break;
    }

    return 0;
}

// 0x44E544
void gmouse_remove_item_outline(Object* object)
{
    if (gGameMouseHighlightedItem != NULL && gGameMouseHighlightedItem == object) {
        Rect rect;
        if (objectClearOutline(object, &rect) == 0) {
            tileWindowRefreshRect(&rect, gElevation);
        }
        gGameMouseHighlightedItem = NULL;
    }
}

// 0x44E580
int objectIsDoor(Object* object)
{
    if (object == NULL) {
        return false;
    }

    if ((object->pid >> 24) != OBJ_TYPE_SCENERY) {
        return false;
    }

    Proto* proto;
    if (protoGetProto(object->pid, &proto) == -1) {
        return false;
    }

    return proto->scenery.type == SCENERY_TYPE_DOOR;
}
