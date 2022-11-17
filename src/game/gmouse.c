#include "game/gmouse.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "game/art.h"
#include "game/actions.h"
#include "plib/color/color.h"
#include "game/combat.h"
#include "plib/gnw/input.h"
#include "game/critter.h"
#include "plib/gnw/grbuf.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/gsound.h"
#include "plib/gnw/rect.h"
#include "game/intface.h"
#include "game/item.h"
#include "game/map.h"
#include "game/object.h"
#include "game/proto.h"
#include "game/protinst.h"
#include "game/skilldex.h"
#include "plib/gnw/text.h"
#include "game/tile.h"
#include "plib/gnw/gnw.h"
#include "plib/gnw/svga.h"

typedef enum ScrollableDirections {
    SCROLLABLE_W = 0x01,
    SCROLLABLE_E = 0x02,
    SCROLLABLE_N = 0x04,
    SCROLLABLE_S = 0x08,
} ScrollableDirections;

static int gmouse_3d_init();
static int gmouse_3d_reset();
static void gmouse_3d_exit();
static int gmouse_3d_lock_frames();
static void gmouse_3d_unlock_frames();
static int gmouse_3d_set_flat_fid(int fid, Rect* rect);
static int gmouse_3d_reset_flat_fid(Rect* rect);
static int gmouse_3d_move_to(int x, int y, int elevation, Rect* a4);
static int gmouse_check_scrolling(int x, int y, int cursor);
static int gmObjIsValidTarget(Object* object);

// 0x518BF8
static bool gmouse_initialized = false;

// 0x518BFC
static int gmouse_enabled = 0;

// 0x518C00
static int gmouse_mapper_mode = 0;

// 0x518C04
static int gmouse_click_to_scroll = 0;

// 0x518C08
static int gmouse_scrolling_enabled = 1;

// 0x518C0C
static int gmouse_current_cursor = MOUSE_CURSOR_NONE;

// 0x518C10
static CacheEntry* gmouse_current_cursor_key = INVALID_CACHE_ENTRY;

// 0x518C14
static int gmouse_cursor_nums[MOUSE_CURSOR_TYPE_COUNT] = {
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
static bool gmouse_3d_initialized = false;

// 0x518C84
static bool gmouse_3d_hover_test = false;

// 0x518C88
static unsigned int gmouse_3d_last_move_time = 0;

// actmenu.frm
// 0x518C8C
static Art* gmouse_3d_menu_frame = NULL;

// 0x518C90
static CacheEntry* gmouse_3d_menu_frame_key = INVALID_CACHE_ENTRY;

// 0x518C94
static int gmouse_3d_menu_frame_width = 0;

// 0x518C98
static int gmouse_3d_menu_frame_height = 0;

// 0x518C9C
static int gmouse_3d_menu_frame_size = 0;

// 0x518CA0
static int gmouse_3d_menu_frame_hot_x = 0;

// 0x518CA4
static int gmouse_3d_menu_frame_hot_y = 0;

// 0x518CA8
static unsigned char* gmouse_3d_menu_frame_data = NULL;

// actpick.frm
// 0x518CAC
static Art* gmouse_3d_pick_frame = NULL;

// 0x518CB0
static CacheEntry* gmouse_3d_pick_frame_key = INVALID_CACHE_ENTRY;

// 0x518CB4
static int gmouse_3d_pick_frame_width = 0;

// 0x518CB8
static int gmouse_3d_pick_frame_height = 0;

// 0x518CBC
static int gmouse_3d_pick_frame_size = 0;

// 0x518CC0
static int gmouse_3d_pick_frame_hot_x = 0;

// 0x518CC4
static int gmouse_3d_pick_frame_hot_y = 0;

// 0x518CC8
static unsigned char* gmouse_3d_pick_frame_data = NULL;

// acttohit.frm
// 0x518CCC
static Art* gmouse_3d_to_hit_frame = NULL;

// 0x518CD0
static CacheEntry* gmouse_3d_to_hit_frame_key = INVALID_CACHE_ENTRY;

// 0x518CD4
static int gmouse_3d_to_hit_frame_width = 0;

// 0x518CD8
static int gmouse_3d_to_hit_frame_height = 0;

// 0x518CDC
static int gmouse_3d_to_hit_frame_size = 0;

// 0x518CE0
static unsigned char* gmouse_3d_to_hit_frame_data = NULL;

// blank.frm
// 0x518CE4
static Art* gmouse_3d_hex_base_frame = NULL;

// 0x518CE8
static CacheEntry* gmouse_3d_hex_base_frame_key = INVALID_CACHE_ENTRY;

// 0x518CEC
static int gmouse_3d_hex_base_frame_width = 0;

// 0x518CF0
static int gmouse_3d_hex_base_frame_height = 0;

// 0x518CF4
static int gmouse_3d_hex_base_frame_size = 0;

// 0x518CF8
static unsigned char* gmouse_3d_hex_base_frame_data = NULL;

// msef000.frm
// 0x518CFC
static Art* gmouse_3d_hex_frame = NULL;

// 0x518D00
static CacheEntry* gmouse_3d_hex_frame_key = INVALID_CACHE_ENTRY;

// 0x518D04
static int gmouse_3d_hex_frame_width = 0;

// 0x518D08
static int gmouse_3d_hex_frame_height = 0;

// 0x518D0C
static int gmouse_3d_hex_frame_size = 0;

// 0x518D10
static unsigned char* gmouse_3d_hex_frame_data = NULL;

// 0x518D14
static unsigned char gmouse_3d_menu_available_actions = 0;

// 0x518D18
static unsigned char* gmouse_3d_menu_actions_start = NULL;

// 0x518D1C
static unsigned char gmouse_3d_menu_current_action_index = 0;

// 0x518D1E
static short gmouse_3d_action_nums[GAME_MOUSE_ACTION_MENU_ITEM_COUNT] = {
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
static int gmouse_3d_modes_enabled = 1;

// 0x518D38
static int gmouse_3d_current_mode = GAME_MOUSE_MODE_MOVE;

// 0x518D3C
static int gmouse_3d_mode_nums[GAME_MOUSE_MODE_COUNT] = {
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
static int gmouse_skill_table[GAME_MOUSE_MODE_SKILL_COUNT] = {
    SKILL_FIRST_AID,
    SKILL_DOCTOR,
    SKILL_LOCKPICK,
    SKILL_STEAL,
    SKILL_TRAPS,
    SKILL_SCIENCE,
    SKILL_REPAIR,
};

// 0x518D84
static int gmouse_wait_cursor_frame = 0;

// 0x518D88
static unsigned int gmouse_wait_cursor_time = 0;

// 0x518D8C
static int gmouse_bk_last_cursor = -1;

// 0x518D90
static bool gmouse_3d_item_highlight = true;

// 0x518D94
static Object* outlined_object = NULL;

// 0x518D98
bool gmouse_clicked_on_edge = false;

// 0x596C3C
static int gmouse_3d_menu_frame_actions[GAME_MOUSE_ACTION_MENU_ITEM_COUNT];

// 0x596C64
static int gmouse_3d_last_mouse_x;

// 0x596C68
static int gmouse_3d_last_mouse_y;

// blank.frm
// 0x596C6C
Object* obj_mouse;

// msef000.frm
// 0x596C70
Object* obj_mouse_flat;

// 0x44B2B0
int gmouse_init()
{
    if (gmouse_initialized) {
        return -1;
    }

    if (gmouse_3d_init() != 0) {
        return -1;
    }

    gmouse_initialized = true;
    gmouse_enabled = 1;

    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    return 0;
}

// 0x44B2E8
int gmouse_reset()
{
    if (!gmouse_initialized) {
        return -1;
    }

    // NOTE: Uninline.
    if (gmouse_3d_reset() != 0) {
        return -1;
    }

    // NOTE: Uninline.
    gmouse_enable();

    gmouse_scrolling_enabled = 1;
    gmouse_set_cursor(MOUSE_CURSOR_ARROW);
    gmouse_wait_cursor_frame = 0;
    gmouse_wait_cursor_time = 0;
    gmouse_clicked_on_edge = 0;

    return 0;
}

// 0x44B3B8
void gmouse_exit()
{
    if (!gmouse_initialized) {
        return;
    }

    mouse_hide();

    mouse_set_shape(NULL, 0, 0, 0, 0, 0, 0);

    // NOTE: Uninline.
    gmouse_3d_exit();

    if (gmouse_current_cursor_key != INVALID_CACHE_ENTRY) {
        art_ptr_unlock(gmouse_current_cursor_key);
    }
    gmouse_current_cursor_key = INVALID_CACHE_ENTRY;

    gmouse_enabled = 0;
    gmouse_initialized = false;
    gmouse_current_cursor = -1;
}

// 0x44B454
void gmouse_enable()
{
    if (!gmouse_enabled) {
        gmouse_current_cursor = -1;
        gmouse_set_cursor(MOUSE_CURSOR_NONE);
        gmouse_scrolling_enabled = 1;
        gmouse_enabled = 1;
        gmouse_bk_last_cursor = -1;
    }
}

// 0x44B48C
void gmouse_disable(int a1)
{
    if (gmouse_enabled) {
        gmouse_set_cursor(MOUSE_CURSOR_NONE);
        gmouse_enabled = 0;

        if (a1 & 1) {
            gmouse_scrolling_enabled = 1;
        } else {
            gmouse_scrolling_enabled = 0;
        }
    }
}

// NOTE: Unused.
//
// 0x44B4C4
int gmouse_is_enabled()
{
    return gmouse_enabled;
}

// 0x44B4CC
void gmouse_enable_scrolling()
{
    gmouse_scrolling_enabled = 1;
}

// 0x44B4D8
void gmouse_disable_scrolling()
{
    gmouse_scrolling_enabled = 0;
}

// NOTE: Inlined.
//
// 0x44B4E4
int gmouse_scrolling_is_enabled()
{
    return gmouse_scrolling_enabled;
}

// NOTE: Unused.
//
// 0x44B4EC
void gmouse_set_click_to_scroll(int a1)
{
    if (a1 != gmouse_click_to_scroll) {
        gmouse_click_to_scroll = a1;
        gmouse_clicked_on_edge = 0;
    }
}

// 0x44B504
int gmouse_get_click_to_scroll()
{
    return gmouse_click_to_scroll;
}

// 0x44B54C
int gmouse_is_scrolling()
{
    int v1 = 0;

    if (gmouse_scrolling_enabled) {
        int x;
        int y;
        mouse_get_position(&x, &y);
        if (x == scr_size.ulx || x == scr_size.lrx || y == scr_size.uly || y == scr_size.lry) {
            switch (gmouse_current_cursor) {
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
void gmouse_bk_process()
{
    // 0x596C74
    static Object* last_object;

    // 0x518D9C
    static int last_tile = -1;

    if (!gmouse_initialized) {
        return;
    }

    int mouseX;
    int mouseY;

    if (gmouse_current_cursor >= FIRST_GAME_MOUSE_ANIMATED_CURSOR) {
        mouse_info();

        // NOTE: Uninline.
        if (gmouse_scrolling_is_enabled()) {
            mouse_get_position(&mouseX, &mouseY);
            int oldMouseCursor = gmouse_current_cursor;

            if (gmouse_check_scrolling(mouseX, mouseY, gmouse_current_cursor) == 0) {
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
                    gmouse_bk_last_cursor = oldMouseCursor;
                    break;
                }
                return;
            }

            if (gmouse_bk_last_cursor != -1) {
                gmouse_set_cursor(gmouse_bk_last_cursor);
                gmouse_bk_last_cursor = -1;
                return;
            }
        }

        gmouse_set_cursor(gmouse_current_cursor);
        return;
    }

    if (!gmouse_enabled) {
        // NOTE: Uninline.
        if (gmouse_scrolling_is_enabled()) {
            mouse_get_position(&mouseX, &mouseY);
            int oldMouseCursor = gmouse_current_cursor;

            if (gmouse_check_scrolling(mouseX, mouseY, gmouse_current_cursor) == 0) {
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
                    gmouse_bk_last_cursor = oldMouseCursor;
                    break;
                }

                return;
            }

            if (gmouse_bk_last_cursor != -1) {
                gmouse_set_cursor(gmouse_bk_last_cursor);
                gmouse_bk_last_cursor = -1;
            }
        }

        return;
    }

    mouse_get_position(&mouseX, &mouseY);

    int oldMouseCursor = gmouse_current_cursor;
    if (gmouse_check_scrolling(mouseX, mouseY, MOUSE_CURSOR_NONE) == 0) {
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
            gmouse_bk_last_cursor = oldMouseCursor;
            break;
        }
        return;
    }

    if (gmouse_bk_last_cursor != -1) {
        gmouse_set_cursor(gmouse_bk_last_cursor);
        gmouse_bk_last_cursor = -1;
    }

    if (win_get_top_win(mouseX, mouseY) != display_win) {
        if (gmouse_current_cursor == MOUSE_CURSOR_NONE) {
            gmouse_3d_off();
            gmouse_set_cursor(MOUSE_CURSOR_ARROW);

            if (gmouse_3d_current_mode >= 2 && !isInCombat()) {
                gmouse_3d_set_mode(GAME_MOUSE_MODE_MOVE);
            }
        }
        return;
    }

    // NOTE: Strange set of conditions and jumps. Not sure about this one.
    switch (gmouse_current_cursor) {
    case MOUSE_CURSOR_NONE:
    case MOUSE_CURSOR_ARROW:
    case MOUSE_CURSOR_SMALL_ARROW_UP:
    case MOUSE_CURSOR_SMALL_ARROW_DOWN:
    case MOUSE_CURSOR_CROSSHAIR:
    case MOUSE_CURSOR_USE_CROSSHAIR:
        if (gmouse_current_cursor != MOUSE_CURSOR_NONE) {
            gmouse_set_cursor(MOUSE_CURSOR_NONE);
        }

        if ((obj_mouse_flat->flags & OBJECT_HIDDEN) != 0) {
            gmouse_3d_on();
        }

        break;
    }

    Rect r1;
    if (gmouse_3d_move_to(mouseX, mouseY, map_elevation, &r1) == 0) {
        tile_refresh_rect(&r1, map_elevation);
    }

    if ((obj_mouse_flat->flags & OBJECT_HIDDEN) != 0 || gmouse_mapper_mode != 0) {
        return;
    }

    unsigned int v3 = get_bk_time();
    if (mouseX == gmouse_3d_last_mouse_x && mouseY == gmouse_3d_last_mouse_y) {
        if (gmouse_3d_hover_test || elapsed_tocks(v3, gmouse_3d_last_move_time) < 250) {
            return;
        }

        if (gmouse_3d_current_mode != GAME_MOUSE_MODE_MOVE) {
            if (gmouse_3d_current_mode == GAME_MOUSE_MODE_ARROW) {
                gmouse_3d_last_move_time = v3;
                gmouse_3d_hover_test = true;

                Object* pointedObject = object_under_mouse(-1, true, map_elevation);
                if (pointedObject != NULL) {
                    int primaryAction = -1;

                    switch (FID_TYPE(pointedObject->fid)) {
                    case OBJ_TYPE_ITEM:
                        primaryAction = GAME_MOUSE_ACTION_MENU_ITEM_USE;
                        if (gmouse_3d_item_highlight) {
                            Rect tmp;
                            if (obj_outline_object(pointedObject, OUTLINE_TYPE_ITEM, &tmp) == 0) {
                                tile_refresh_rect(&tmp, map_elevation);
                                outlined_object = pointedObject;
                            }
                        }
                        break;
                    case OBJ_TYPE_CRITTER:
                        if (pointedObject == obj_dude) {
                            primaryAction = GAME_MOUSE_ACTION_MENU_ITEM_ROTATE;
                        } else {
                            if (obj_action_can_talk_to(pointedObject)) {
                                if (isInCombat()) {
                                    primaryAction = GAME_MOUSE_ACTION_MENU_ITEM_LOOK;
                                } else {
                                    primaryAction = GAME_MOUSE_ACTION_MENU_ITEM_TALK;
                                }
                            } else {
                                if (critter_flag_check(pointedObject->pid, CRITTER_NO_STEAL)) {
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
                        if (gmouse_3d_build_pick_frame(mouseX, mouseY, primaryAction, scr_size.lrx - scr_size.ulx + 1, scr_size.lry - scr_size.uly - 99) == 0) {
                            Rect tmp;
                            int fid = art_id(OBJ_TYPE_INTERFACE, 282, 0, 0, 0);
                            // NOTE: Uninline.
                            if (gmouse_3d_set_flat_fid(fid, &tmp) == 0) {
                                tile_refresh_rect(&tmp, map_elevation);
                            }
                        }
                    }

                    if (pointedObject != last_object) {
                        last_object = pointedObject;
                        obj_look_at(obj_dude, last_object);
                    }
                }
            } else if (gmouse_3d_current_mode == GAME_MOUSE_MODE_CROSSHAIR) {
                Object* pointedObject = object_under_mouse(OBJ_TYPE_CRITTER, false, map_elevation);
                if (pointedObject == NULL) {
                    pointedObject = object_under_mouse(-1, false, map_elevation);
                    if (!gmObjIsValidTarget(pointedObject)) {
                        pointedObject = NULL;
                    }
                }

                if (pointedObject != NULL) {
                    bool pointedObjectIsCritter = FID_TYPE(pointedObject->fid) == OBJ_TYPE_CRITTER;

                    int combatLooks = 0;
                    config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_COMBAT_LOOKS_KEY, &combatLooks);
                    if (combatLooks != 0) {
                        if (obj_examine(obj_dude, pointedObject) == -1) {
                            obj_look_at(obj_dude, pointedObject);
                        }
                    }

                    int color;
                    int accuracy;
                    char formattedAccuracy[8];
                    if (combat_to_hit(pointedObject, &accuracy)) {
                        sprintf(formattedAccuracy, "%d%%", accuracy);

                        if (pointedObjectIsCritter) {
                            if (pointedObject->data.critter.combat.team != 0) {
                                color = colorTable[32767];
                            } else {
                                color = colorTable[32495];
                            }
                        } else {
                            color = colorTable[17969];
                        }
                    } else {
                        sprintf(formattedAccuracy, " %c ", 'X');

                        if (pointedObjectIsCritter) {
                            if (pointedObject->data.critter.combat.team != 0) {
                                color = colorTable[31744];
                            } else {
                                color = colorTable[18161];
                            }
                        } else {
                            color = colorTable[32239];
                        }
                    }

                    if (gmouse_3d_build_to_hit_frame(formattedAccuracy, color) == 0) {
                        Rect tmp;
                        int fid = art_id(OBJ_TYPE_INTERFACE, 284, 0, 0, 0);
                        // NOTE: Uninline.
                        if (gmouse_3d_set_flat_fid(fid, &tmp) == 0) {
                            tile_refresh_rect(&tmp, map_elevation);
                        }
                    }

                    if (last_object != pointedObject) {
                        last_object = pointedObject;
                    }
                } else {
                    Rect tmp;
                    if (gmouse_3d_reset_flat_fid(&tmp) == 0) {
                        tile_refresh_rect(&tmp, map_elevation);
                    }
                }

                gmouse_3d_last_move_time = v3;
                gmouse_3d_hover_test = true;
            }
            return;
        }

        char formattedActionPoints[8];
        int color;
        int v6 = make_path(obj_dude, obj_dude->tile, obj_mouse_flat->tile, NULL, 1);
        if (v6) {
            if (!isInCombat()) {
                formattedActionPoints[0] = '\0';
                color = colorTable[31744];
            } else {
                int v7 = critter_compute_ap_from_distance(obj_dude, v6);
                int v8;
                if (v7 - combat_free_move >= 0) {
                    v8 = v7 - combat_free_move;
                } else {
                    v8 = 0;
                }

                if (v8 <= obj_dude->data.critter.combat.ap) {
                    sprintf(formattedActionPoints, "%d", v8);
                    color = colorTable[32767];
                } else {
                    sprintf(formattedActionPoints, "%c", 'X');
                    color = colorTable[31744];
                }
            }
        } else {
            sprintf(formattedActionPoints, "%c", 'X');
            color = colorTable[31744];
        }

        if (gmouse_3d_build_hex_frame(formattedActionPoints, color) == 0) {
            Rect tmp;
            obj_bound(obj_mouse_flat, &tmp);
            tile_refresh_rect(&tmp, 0);
        }

        gmouse_3d_last_move_time = v3;
        gmouse_3d_hover_test = true;
        last_tile = obj_mouse_flat->tile;
        return;
    }

    gmouse_3d_last_move_time = v3;
    gmouse_3d_hover_test = false;
    gmouse_3d_last_mouse_x = mouseX;
    gmouse_3d_last_mouse_y = mouseY;

    if (!gmouse_mapper_mode) {
        int fid = art_id(OBJ_TYPE_INTERFACE, 0, 0, 0, 0);
        gmouse_3d_set_fid(fid);
    }

    int v34 = 0;

    Rect r2;
    Rect r26;
    if (gmouse_3d_reset_flat_fid(&r2) == 0) {
        v34 |= 1;
    }

    if (outlined_object != NULL) {
        if (obj_remove_outline(outlined_object, &r26) == 0) {
            v34 |= 2;
        }
        outlined_object = NULL;
    }

    switch (v34) {
    case 3:
        rect_min_bound(&r2, &r26, &r2);
        // FALLTHROUGH
    case 1:
        tile_refresh_rect(&r2, map_elevation);
        break;
    case 2:
        tile_refresh_rect(&r26, map_elevation);
        break;
    }
}

// 0x44BFA8
void gmouse_handle_event(int mouseX, int mouseY, int mouseState)
{
    if (!gmouse_initialized) {
        return;
    }

    if (gmouse_current_cursor >= MOUSE_CURSOR_WAIT_PLANET) {
        return;
    }

    if (!gmouse_enabled) {
        return;
    }

    if (gmouse_clicked_on_edge) {
        if (gmouse_get_click_to_scroll()) {
            return;
        }
    }

    if (!mouse_click_in(0, 0, scr_size.lrx - scr_size.ulx, scr_size.lry - scr_size.uly - 100)) {
        return;
    }

    if ((mouseState & MOUSE_EVENT_RIGHT_BUTTON_DOWN) != 0) {
        if ((mouseState & MOUSE_EVENT_RIGHT_BUTTON_REPEAT) == 0 && (obj_mouse_flat->flags & OBJECT_HIDDEN) == 0) {
            gmouse_3d_toggle_mode();
        }
        return;
    }

    if ((mouseState & MOUSE_EVENT_LEFT_BUTTON_UP) != 0) {
        if (gmouse_3d_current_mode == GAME_MOUSE_MODE_MOVE) {
            int actionPoints;
            if (isInCombat()) {
                actionPoints = combat_free_move + obj_dude->data.critter.combat.ap;
            } else {
                actionPoints = -1;
            }

            bool running;
            configGetBool(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_RUNNING_KEY, &running);

            if (keys[DIK_LSHIFT] || keys[DIK_RSHIFT]) {
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

        if (gmouse_3d_current_mode == GAME_MOUSE_MODE_ARROW) {
            Object* v5 = object_under_mouse(-1, true, map_elevation);
            if (v5 != NULL) {
                switch (FID_TYPE(v5->fid)) {
                case OBJ_TYPE_ITEM:
                    action_get_an_object(obj_dude, v5);
                    break;
                case OBJ_TYPE_CRITTER:
                    if (v5 == obj_dude) {
                        if (FID_ANIM_TYPE(obj_dude->fid) == ANIM_STAND) {
                            Rect a1;
                            if (obj_inc_rotation(v5, &a1) == 0) {
                                tile_refresh_rect(&a1, v5->elevation);
                            }
                        }
                    } else {
                        if (obj_action_can_talk_to(v5)) {
                            if (isInCombat()) {
                                if (obj_examine(obj_dude, v5) == -1) {
                                    obj_look_at(obj_dude, v5);
                                }
                            } else {
                                action_talk_to(obj_dude, v5);
                            }
                        } else {
                            action_loot_container(obj_dude, v5);
                        }
                    }
                    break;
                case OBJ_TYPE_SCENERY:
                    if (obj_action_can_use(v5)) {
                        action_use_an_object(obj_dude, v5);
                    } else {
                        if (obj_examine(obj_dude, v5) == -1) {
                            obj_look_at(obj_dude, v5);
                        }
                    }
                    break;
                case OBJ_TYPE_WALL:
                    if (obj_examine(obj_dude, v5) == -1) {
                        obj_look_at(obj_dude, v5);
                    }
                    break;
                }
            }
            return;
        }

        if (gmouse_3d_current_mode == GAME_MOUSE_MODE_CROSSHAIR) {
            Object* v7 = object_under_mouse(OBJ_TYPE_CRITTER, false, map_elevation);
            if (v7 == NULL) {
                v7 = object_under_mouse(-1, false, map_elevation);
                if (!gmObjIsValidTarget(v7)) {
                    v7 = NULL;
                }
            }

            if (v7 != NULL) {
                combat_attack_this(v7);
                gmouse_3d_hover_test = true;
                gmouse_3d_last_mouse_y = mouseY;
                gmouse_3d_last_mouse_x = mouseX;
                gmouse_3d_last_move_time = get_time() - 250;
            }
            return;
        }

        if (gmouse_3d_current_mode == GAME_MOUSE_MODE_USE_CROSSHAIR) {
            Object* object = object_under_mouse(-1, true, map_elevation);
            if (object != NULL) {
                Object* weapon;
                if (intface_get_current_item(&weapon) != -1) {
                    if (isInCombat()) {
                        int hitMode = intface_is_item_right_hand()
                            ? HIT_MODE_RIGHT_WEAPON_PRIMARY
                            : HIT_MODE_LEFT_WEAPON_PRIMARY;

                        int actionPointsRequired = item_mp_cost(obj_dude, hitMode, false);
                        if (actionPointsRequired <= obj_dude->data.critter.combat.ap) {
                            if (action_use_an_item_on_object(obj_dude, object, weapon) != -1) {
                                int actionPoints = obj_dude->data.critter.combat.ap;
                                if (actionPointsRequired > actionPoints) {
                                    obj_dude->data.critter.combat.ap = 0;
                                } else {
                                    obj_dude->data.critter.combat.ap -= actionPointsRequired;
                                }
                                intface_update_move_points(obj_dude->data.critter.combat.ap, combat_free_move);
                            }
                        }
                    } else {
                        action_use_an_item_on_object(obj_dude, object, weapon);
                    }
                }
            }
            gmouse_set_cursor(MOUSE_CURSOR_NONE);
            gmouse_3d_set_mode(GAME_MOUSE_MODE_MOVE);
            return;
        }

        if (gmouse_3d_current_mode == GAME_MOUSE_MODE_USE_FIRST_AID
            || gmouse_3d_current_mode == GAME_MOUSE_MODE_USE_DOCTOR
            || gmouse_3d_current_mode == GAME_MOUSE_MODE_USE_LOCKPICK
            || gmouse_3d_current_mode == GAME_MOUSE_MODE_USE_STEAL
            || gmouse_3d_current_mode == GAME_MOUSE_MODE_USE_TRAPS
            || gmouse_3d_current_mode == GAME_MOUSE_MODE_USE_SCIENCE
            || gmouse_3d_current_mode == GAME_MOUSE_MODE_USE_REPAIR) {
            Object* object = object_under_mouse(-1, 1, map_elevation);
            if (object == NULL || action_use_skill_on(obj_dude, object, gmouse_skill_table[gmouse_3d_current_mode - FIRST_GAME_MOUSE_MODE_SKILL]) != -1) {
                gmouse_set_cursor(MOUSE_CURSOR_NONE);
                gmouse_3d_set_mode(GAME_MOUSE_MODE_MOVE);
            }
            return;
        }
    }

    if ((mouseState & MOUSE_EVENT_LEFT_BUTTON_DOWN_REPEAT) == MOUSE_EVENT_LEFT_BUTTON_DOWN_REPEAT && gmouse_3d_current_mode == GAME_MOUSE_MODE_ARROW) {
        Object* v16 = object_under_mouse(-1, true, map_elevation);
        if (v16 != NULL) {
            int actionMenuItemsCount = 0;
            int actionMenuItems[6];
            switch (FID_TYPE(v16->fid)) {
            case OBJ_TYPE_ITEM:
                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_USE;
                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_LOOK;
                if (item_get_type(v16) == ITEM_TYPE_CONTAINER) {
                    actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_INVENTORY;
                    actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_USE_SKILL;
                }
                actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_CANCEL;
                break;
            case OBJ_TYPE_CRITTER:
                if (v16 == obj_dude) {
                    actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_ROTATE;
                } else {
                    if (obj_action_can_talk_to(v16)) {
                        if (!isInCombat()) {
                            actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_TALK;
                        }
                    } else {
                        if (!critter_flag_check(v16->pid, CRITTER_NO_STEAL)) {
                            actionMenuItems[actionMenuItemsCount++] = GAME_MOUSE_ACTION_MENU_ITEM_USE;
                        }
                    }

                    if (action_can_be_pushed(obj_dude, v16)) {
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

            if (gmouse_3d_build_menu_frame(mouseX, mouseY, actionMenuItems, actionMenuItemsCount, scr_size.lrx - scr_size.ulx + 1, scr_size.lry - scr_size.uly - 99) == 0) {
                Rect v43;
                int fid = art_id(OBJ_TYPE_INTERFACE, 283, 0, 0, 0);
                // NOTE: Uninline.
                if (gmouse_3d_set_flat_fid(fid, &v43) == 0 && gmouse_3d_move_to(mouseX, mouseY, map_elevation, &v43) == 0) {
                    tile_refresh_rect(&v43, map_elevation);
                    map_disable_bk_processes();

                    int v33 = mouseY;
                    int actionIndex = 0;
                    while ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_UP) == 0) {
                        get_input();

                        if (game_user_wants_to_quit != 0) {
                            actionMenuItems[actionIndex] = 0;
                        }

                        int v48;
                        int v47;
                        mouse_get_position(&v48, &v47);

                        if (abs(v47 - v33) > 10) {
                            if (v33 >= v47) {
                                actionIndex -= 1;
                            } else {
                                actionIndex += 1;
                            }

                            if (gmouse_3d_highlight_menu_frame(actionIndex) == 0) {
                                tile_refresh_rect(&v43, map_elevation);
                            }
                            v33 = v47;
                        }
                    }

                    map_enable_bk_processes();

                    gmouse_3d_hover_test = false;
                    gmouse_3d_last_mouse_x = mouseX;
                    gmouse_3d_last_mouse_y = mouseY;
                    gmouse_3d_last_move_time = get_time();

                    mouse_set_position(mouseX, v33);

                    if (gmouse_3d_reset_flat_fid(&v43) == 0) {
                        tile_refresh_rect(&v43, map_elevation);
                    }

                    switch (actionMenuItems[actionIndex]) {
                    case GAME_MOUSE_ACTION_MENU_ITEM_INVENTORY:
                        use_inventory_on(v16);
                        break;
                    case GAME_MOUSE_ACTION_MENU_ITEM_LOOK:
                        if (obj_examine(obj_dude, v16) == -1) {
                            obj_look_at(obj_dude, v16);
                        }
                        break;
                    case GAME_MOUSE_ACTION_MENU_ITEM_ROTATE:
                        if (obj_inc_rotation(v16, &v43) == 0) {
                            tile_refresh_rect(&v43, v16->elevation);
                        }
                        break;
                    case GAME_MOUSE_ACTION_MENU_ITEM_TALK:
                        action_talk_to(obj_dude, v16);
                        break;
                    case GAME_MOUSE_ACTION_MENU_ITEM_USE:
                        switch (FID_TYPE(v16->fid)) {
                        case OBJ_TYPE_SCENERY:
                            action_use_an_object(obj_dude, v16);
                            break;
                        case OBJ_TYPE_CRITTER:
                            action_loot_container(obj_dude, v16);
                            break;
                        default:
                            action_get_an_object(obj_dude, v16);
                            break;
                        }
                        break;
                    case GAME_MOUSE_ACTION_MENU_ITEM_USE_SKILL:
                        if (1) {
                            int skill = -1;

                            int rc = skilldex_select();
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
                                action_use_skill_on(obj_dude, v16, skill);
                            }
                        }
                        break;
                    case GAME_MOUSE_ACTION_MENU_ITEM_PUSH:
                        action_push_critter(obj_dude, v16);
                        break;
                    }
                }
            }
        }
    }
}

// 0x44C840
int gmouse_set_cursor(int cursor)
{
    if (!gmouse_initialized) {
        return -1;
    }

    if (cursor != MOUSE_CURSOR_ARROW && cursor == gmouse_current_cursor && (gmouse_current_cursor < 25 || gmouse_current_cursor >= 27)) {
        return -1;
    }

    CacheEntry* mouseCursorFrmHandle;
    int fid = art_id(OBJ_TYPE_INTERFACE, gmouse_cursor_nums[cursor], 0, 0, 0);
    Art* mouseCursorFrm = art_ptr_lock(fid, &mouseCursorFrmHandle);
    if (mouseCursorFrm == NULL) {
        return -1;
    }

    bool shouldUpdate = true;
    int frame = 0;
    if (cursor >= FIRST_GAME_MOUSE_ANIMATED_CURSOR) {
        unsigned int tick = get_time();

        if ((obj_mouse_flat->flags & OBJECT_HIDDEN) == 0) {
            gmouse_3d_off();
        }

        unsigned int delay = 1000 / art_frame_fps(mouseCursorFrm);
        if (elapsed_tocks(tick, gmouse_wait_cursor_time) < delay) {
            shouldUpdate = false;
        } else {
            if (art_frame_max_frame(mouseCursorFrm) <= gmouse_wait_cursor_frame) {
                gmouse_wait_cursor_frame = 0;
            }

            frame = gmouse_wait_cursor_frame;
            gmouse_wait_cursor_time = tick;
            gmouse_wait_cursor_frame++;
        }
    }

    if (!shouldUpdate) {
        return -1;
    }

    int width = art_frame_width(mouseCursorFrm, frame, 0);
    int height = art_frame_length(mouseCursorFrm, frame, 0);

    int offsetX;
    int offsetY;
    art_frame_offset(mouseCursorFrm, 0, &offsetX, &offsetY);

    offsetX = width / 2 - offsetX;
    offsetY = height - 1 - offsetY;

    unsigned char* mouseCursorFrmData = art_frame_data(mouseCursorFrm, frame, 0);
    if (mouse_set_shape(mouseCursorFrmData, width, height, width, offsetX, offsetY, 0) != 0) {
        return -1;
    }

    if (gmouse_current_cursor_key != INVALID_CACHE_ENTRY) {
        art_ptr_unlock(gmouse_current_cursor_key);
    }

    gmouse_current_cursor = cursor;
    gmouse_current_cursor_key = mouseCursorFrmHandle;

    return 0;
}

// 0x44C9E8
int gmouse_get_cursor()
{
    return gmouse_current_cursor;
}

// NOTE: Unused.
//
// 0x44C9F0
void gmouse_set_mapper_mode(int mode)
{
    gmouse_mapper_mode = mode;
}

// 0x44C9F8
void gmouse_3d_enable_modes()
{
    gmouse_3d_modes_enabled = 1;
}

// NOTE: Unused.
//
// 0x44CA04
void gmouse_3d_disable_modes()
{
    gmouse_3d_modes_enabled = 0;
}

// NOTE: Unused.
//
// 0x44CA10
int gmouse_3d_modes_are_enabled()
{
    return gmouse_3d_modes_enabled;
}

// 0x44CA18
void gmouse_3d_set_mode(int mode)
{
    if (!gmouse_initialized) {
        return;
    }

    if (!gmouse_3d_modes_enabled) {
        return;
    }

    if (mode == gmouse_3d_current_mode) {
        return;
    }

    int fid = art_id(OBJ_TYPE_INTERFACE, 0, 0, 0, 0);
    gmouse_3d_set_fid(fid);

    fid = art_id(OBJ_TYPE_INTERFACE, gmouse_3d_mode_nums[mode], 0, 0, 0);

    Rect rect;
    // NOTE: Uninline.
    if (gmouse_3d_set_flat_fid(fid, &rect) == -1) {
        return;
    }

    int mouseX;
    int mouseY;
    mouse_get_position(&mouseX, &mouseY);

    Rect r2;
    if (gmouse_3d_move_to(mouseX, mouseY, map_elevation, &r2) == 0) {
        rect_min_bound(&rect, &r2, &rect);
    }

    int v5 = 0;
    if (gmouse_3d_current_mode == GAME_MOUSE_MODE_CROSSHAIR) {
        v5 = -1;
    }

    if (mode != 0) {
        if (mode == GAME_MOUSE_MODE_CROSSHAIR) {
            v5 = 1;
        }

        if (gmouse_3d_current_mode == 0) {
            if (obj_turn_off_outline(obj_mouse_flat, &r2) == 0) {
                rect_min_bound(&rect, &r2, &rect);
            }
        }
    } else {
        if (obj_turn_on_outline(obj_mouse_flat, &r2) == 0) {
            rect_min_bound(&rect, &r2, &rect);
        }
    }

    gmouse_3d_current_mode = mode;
    gmouse_3d_hover_test = false;
    gmouse_3d_last_move_time = get_time();

    tile_refresh_rect(&rect, map_elevation);

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
int gmouse_3d_get_mode()
{
    return gmouse_3d_current_mode;
}

// 0x44CB74
void gmouse_3d_toggle_mode()
{
    int mode = (gmouse_3d_current_mode + 1) % 3;

    if (isInCombat()) {
        Object* item;
        if (intface_get_current_item(&item) == 0) {
            if (item != NULL && item_get_type(item) != ITEM_TYPE_WEAPON && mode == GAME_MOUSE_MODE_CROSSHAIR) {
                mode = GAME_MOUSE_MODE_MOVE;
            }
        }
    } else {
        if (mode == GAME_MOUSE_MODE_CROSSHAIR) {
            mode = GAME_MOUSE_MODE_MOVE;
        }
    }

    gmouse_3d_set_mode(mode);
}

// 0x44CBD0
void gmouse_3d_refresh()
{
    gmouse_3d_last_mouse_x = -1;
    gmouse_3d_last_mouse_y = -1;
    gmouse_3d_hover_test = false;
    gmouse_3d_last_move_time = 0;
    gmouse_bk_process();
}

// 0x44CBFC
int gmouse_3d_set_fid(int fid)
{
    if (!gmouse_initialized) {
        return -1;
    }

    if (!art_exists(fid)) {
        return -1;
    }

    if (obj_mouse->fid == fid) {
        return -1;
    }

    if (!gmouse_mapper_mode) {
        return obj_change_fid(obj_mouse, fid, NULL);
    }

    int v1 = 0;

    Rect oldRect;
    if (obj_mouse->fid != -1) {
        obj_bound(obj_mouse, &oldRect);
        v1 |= 1;
    }

    int rc = -1;

    Rect rect;
    if (obj_change_fid(obj_mouse, fid, &rect) == 0) {
        rc = 0;
        v1 |= 2;
    }

    if ((obj_mouse_flat->flags & OBJECT_HIDDEN) == 0) {
        if (v1 == 1) {
            tile_refresh_rect(&oldRect, map_elevation);
        } else if (v1 == 2) {
            tile_refresh_rect(&rect, map_elevation);
        } else if (v1 == 3) {
            rect_min_bound(&oldRect, &rect, &oldRect);
            tile_refresh_rect(&oldRect, map_elevation);
        }
    }

    return rc;
}

// NOTE: Unused.
//
// 0x44CCF4
int gmouse_3d_get_fid()
{
    if (gmouse_initialized) {
        return obj_mouse->fid;
    }

    return -1;
}

// 0x44CD0C
void gmouse_3d_reset_fid()
{
    int fid = art_id(OBJ_TYPE_INTERFACE, 0, 0, 0, 0);
    gmouse_3d_set_fid(fid);
}

// 0x44CD2C
void gmouse_3d_on()
{
    if (!gmouse_initialized) {
        return;
    }

    int v2 = 0;

    Rect rect1;
    if (obj_turn_on(obj_mouse, &rect1) == 0) {
        v2 |= 1;
    }

    Rect rect2;
    if (obj_turn_on(obj_mouse_flat, &rect2) == 0) {
        v2 |= 2;
    }

    Rect tmp;
    if (gmouse_3d_current_mode != GAME_MOUSE_MODE_MOVE) {
        if (obj_turn_off_outline(obj_mouse_flat, &tmp) == 0) {
            if ((v2 & 2) != 0) {
                rect_min_bound(&rect2, &tmp, &rect2);
            } else {
                memcpy(&rect2, &tmp, sizeof(rect2));
                v2 |= 2;
            }
        }
    }

    if (gmouse_3d_reset_flat_fid(&tmp) == 0) {
        if ((v2 & 2) != 0) {
            rect_min_bound(&rect2, &tmp, &rect2);
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
            rect_min_bound(&rect1, &rect2, &rect1);
            rect = &rect1;
            break;
        default:
            assert(false && "Should be unreachable");
        }

        tile_refresh_rect(rect, map_elevation);
    }

    gmouse_3d_hover_test = false;
    gmouse_3d_last_move_time = get_time() - 250;
}

// 0x44CE34
void gmouse_3d_off()
{
    if (!gmouse_initialized) {
        return;
    }

    int v1 = 0;

    Rect rect1;
    if (obj_turn_off(obj_mouse, &rect1) == 0) {
        v1 |= 1;
    }

    Rect rect2;
    if (obj_turn_off(obj_mouse_flat, &rect2) == 0) {
        v1 |= 2;
    }

    if (v1 == 1) {
        tile_refresh_rect(&rect1, map_elevation);
    } else if (v1 == 2) {
        tile_refresh_rect(&rect2, map_elevation);
    } else if (v1 == 3) {
        rect_min_bound(&rect1, &rect2, &rect1);
        tile_refresh_rect(&rect1, map_elevation);
    }
}

// 0x44CEB0
bool gmouse_3d_is_on()
{
    return (obj_mouse_flat->flags & OBJECT_HIDDEN) == 0;
}

// 0x44CEC4
Object* object_under_mouse(int objectType, bool a2, int elevation)
{
    int mouseX;
    int mouseY;
    mouse_get_position(&mouseX, &mouseY);

    bool v13 = false;
    if (objectType == -1) {
        if (square_roof_intersect(mouseX, mouseY, elevation)) {
            if (obj_intersects_with(obj_egg, mouseX, mouseY) == 0) {
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
            if (a2 || obj_dude != ptr->object) {
                v4 = ptr->object;
                if ((ptr->flags & 0x01) != 0) {
                    if ((ptr->flags & 0x04) == 0) {
                        if (FID_TYPE(ptr->object->fid) != OBJ_TYPE_CRITTER || (ptr->object->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_DEAD)) == 0) {
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
int gmouse_3d_build_pick_frame(int x, int y, int menuItem, int width, int height)
{
    CacheEntry* menuItemFrmHandle;
    int menuItemFid = art_id(OBJ_TYPE_INTERFACE, gmouse_3d_action_nums[menuItem], 0, 0, 0);
    Art* menuItemFrm = art_ptr_lock(menuItemFid, &menuItemFrmHandle);
    if (menuItemFrm == NULL) {
        return -1;
    }

    CacheEntry* arrowFrmHandle;
    int arrowFid = art_id(OBJ_TYPE_INTERFACE, gmouse_3d_mode_nums[GAME_MOUSE_MODE_ARROW], 0, 0, 0);
    Art* arrowFrm = art_ptr_lock(arrowFid, &arrowFrmHandle);
    if (arrowFrm == NULL) {
        art_ptr_unlock(menuItemFrmHandle);
        // FIXME: Why this is success?
        return 0;
    }

    unsigned char* arrowFrmData = art_frame_data(arrowFrm, 0, 0);
    int arrowFrmWidth = art_frame_width(arrowFrm, 0, 0);
    int arrowFrmHeight = art_frame_length(arrowFrm, 0, 0);

    unsigned char* menuItemFrmData = art_frame_data(menuItemFrm, 0, 0);
    int menuItemFrmWidth = art_frame_width(menuItemFrm, 0, 0);
    int menuItemFrmHeight = art_frame_length(menuItemFrm, 0, 0);

    unsigned char* arrowFrmDest = gmouse_3d_pick_frame_data;
    unsigned char* menuItemFrmDest = gmouse_3d_pick_frame_data;

    gmouse_3d_pick_frame_hot_x = 0;
    gmouse_3d_pick_frame_hot_y = 0;

    gmouse_3d_pick_frame->xOffsets[0] = gmouse_3d_pick_frame_width / 2;
    gmouse_3d_pick_frame->yOffsets[0] = gmouse_3d_pick_frame_height - 1;

    int maxX = x + menuItemFrmWidth + arrowFrmWidth - 1;
    int maxY = y + menuItemFrmHeight - 1;
    int shiftY = maxY - height + 2;

    if (maxX < width) {
        menuItemFrmDest += arrowFrmWidth;
        if (maxY >= height) {
            gmouse_3d_pick_frame_hot_y = shiftY;
            gmouse_3d_pick_frame->yOffsets[0] -= shiftY;
            arrowFrmDest += gmouse_3d_pick_frame_width * shiftY;
        }
    } else {
        art_ptr_unlock(arrowFrmHandle);

        arrowFid = art_id(OBJ_TYPE_INTERFACE, 285, 0, 0, 0);
        arrowFrm = art_ptr_lock(arrowFid, &arrowFrmHandle);
        arrowFrmData = art_frame_data(arrowFrm, 0, 0);
        arrowFrmDest += menuItemFrmWidth;

        gmouse_3d_pick_frame->xOffsets[0] = -gmouse_3d_pick_frame->xOffsets[0];
        gmouse_3d_pick_frame_hot_x += menuItemFrmWidth + arrowFrmWidth;

        if (maxY >= height) {
            gmouse_3d_pick_frame_hot_y += shiftY;
            gmouse_3d_pick_frame->yOffsets[0] -= shiftY;

            arrowFrmDest += gmouse_3d_pick_frame_width * shiftY;
        }
    }

    memset(gmouse_3d_pick_frame_data, 0, gmouse_3d_pick_frame_size);

    buf_to_buf(arrowFrmData, arrowFrmWidth, arrowFrmHeight, arrowFrmWidth, arrowFrmDest, gmouse_3d_pick_frame_width);
    buf_to_buf(menuItemFrmData, menuItemFrmWidth, menuItemFrmHeight, menuItemFrmWidth, menuItemFrmDest, gmouse_3d_pick_frame_width);

    art_ptr_unlock(arrowFrmHandle);
    art_ptr_unlock(menuItemFrmHandle);

    return 0;
}

// 0x44D200
int gmouse_3d_pick_frame_hot(int* a1, int* a2)
{
    *a1 = gmouse_3d_pick_frame_hot_x;
    *a2 = gmouse_3d_pick_frame_hot_y;
    return 0;
}

// 0x44D214
int gmouse_3d_build_menu_frame(int x, int y, const int* menuItems, int menuItemsLength, int width, int height)
{
    gmouse_3d_menu_actions_start = NULL;
    gmouse_3d_menu_current_action_index = 0;
    gmouse_3d_menu_available_actions = 0;

    if (menuItems == NULL) {
        return -1;
    }

    if (menuItemsLength == 0 || menuItemsLength >= GAME_MOUSE_ACTION_MENU_ITEM_COUNT) {
        return -1;
    }

    CacheEntry* menuItemFrmHandles[GAME_MOUSE_ACTION_MENU_ITEM_COUNT];
    Art* menuItemFrms[GAME_MOUSE_ACTION_MENU_ITEM_COUNT];

    for (int index = 0; index < menuItemsLength; index++) {
        int frmId = gmouse_3d_action_nums[menuItems[index]] & 0xFFFF;
        if (index == 0) {
            frmId -= 1;
        }

        int fid = art_id(OBJ_TYPE_INTERFACE, frmId, 0, 0, 0);

        menuItemFrms[index] = art_ptr_lock(fid, &(menuItemFrmHandles[index]));
        if (menuItemFrms[index] == NULL) {
            while (--index >= 0) {
                art_ptr_unlock(menuItemFrmHandles[index]);
            }
            return -1;
        }
    }

    int fid = art_id(OBJ_TYPE_INTERFACE, gmouse_3d_mode_nums[GAME_MOUSE_MODE_ARROW], 0, 0, 0);
    CacheEntry* arrowFrmHandle;
    Art* arrowFrm = art_ptr_lock(fid, &arrowFrmHandle);
    if (arrowFrm == NULL) {
        // FIXME: Unlock arts.
        return -1;
    }

    int arrowWidth = art_frame_width(arrowFrm, 0, 0);
    int arrowHeight = art_frame_length(arrowFrm, 0, 0);

    int menuItemWidth = art_frame_width(menuItemFrms[0], 0, 0);
    int menuItemHeight = art_frame_length(menuItemFrms[0], 0, 0);

    gmouse_3d_menu_frame_hot_x = 0;
    gmouse_3d_menu_frame_hot_y = 0;

    gmouse_3d_menu_frame->xOffsets[0] = gmouse_3d_menu_frame_width / 2;
    gmouse_3d_menu_frame->yOffsets[0] = gmouse_3d_menu_frame_height - 1;

    int v60 = y + menuItemsLength * menuItemHeight - 1;
    int v24 = v60 - height + 2;
    unsigned char* v22 = gmouse_3d_menu_frame_data;
    unsigned char* v58 = v22;

    unsigned char* arrowData;
    if (x + arrowWidth + menuItemWidth - 1 < width) {
        arrowData = art_frame_data(arrowFrm, 0, 0);
        v58 = v22 + arrowWidth;
        if (height <= v60) {
            gmouse_3d_menu_frame_hot_y += v24;
            v22 += gmouse_3d_menu_frame_width * v24;
            gmouse_3d_menu_frame->yOffsets[0] -= v24;
        }
    } else {
        // Mirrored arrow (from left to right).
        fid = art_id(OBJ_TYPE_INTERFACE, 285, 0, 0, 0);
        arrowFrm = art_ptr_lock(fid, &arrowFrmHandle);
        arrowData = art_frame_data(arrowFrm, 0, 0);
        gmouse_3d_menu_frame->xOffsets[0] = -gmouse_3d_menu_frame->xOffsets[0];
        gmouse_3d_menu_frame_hot_x += menuItemWidth + arrowWidth;
        if (v60 >= height) {
            gmouse_3d_menu_frame_hot_y += v24;
            gmouse_3d_menu_frame->yOffsets[0] -= v24;
            v22 += gmouse_3d_menu_frame_width * v24;
        }
    }

    memset(gmouse_3d_menu_frame_data, 0, gmouse_3d_menu_frame_size);
    buf_to_buf(arrowData, arrowWidth, arrowHeight, arrowWidth, v22, gmouse_3d_pick_frame_width);

    unsigned char* v38 = v58;
    for (int index = 0; index < menuItemsLength; index++) {
        unsigned char* data = art_frame_data(menuItemFrms[index], 0, 0);
        buf_to_buf(data, menuItemWidth, menuItemHeight, menuItemWidth, v38, gmouse_3d_pick_frame_width);
        v38 += gmouse_3d_menu_frame_width * menuItemHeight;
    }

    art_ptr_unlock(arrowFrmHandle);

    for (int index = 0; index < menuItemsLength; index++) {
        art_ptr_unlock(menuItemFrmHandles[index]);
    }

    memcpy(gmouse_3d_menu_frame_actions, menuItems, sizeof(*gmouse_3d_menu_frame_actions) * menuItemsLength);
    gmouse_3d_menu_available_actions = menuItemsLength;
    gmouse_3d_menu_actions_start = v58;

    Sound* sound = gsound_load_sound("iaccuxx1", NULL);
    if (sound != NULL) {
        gsound_play_sound(sound);
    }

    return 0;
}

// NOTE: Unused.
//
// 0x44D61C
int gmouse_3d_menu_frame_hot(int* x, int* y)
{
    *x = gmouse_3d_menu_frame_hot_x;
    *y = gmouse_3d_menu_frame_hot_y;
    return 0;
}

// 0x44D630
int gmouse_3d_highlight_menu_frame(int menuItemIndex)
{
    if (menuItemIndex < 0 || menuItemIndex >= gmouse_3d_menu_available_actions) {
        return -1;
    }

    CacheEntry* handle;
    int fid = art_id(OBJ_TYPE_INTERFACE, gmouse_3d_action_nums[gmouse_3d_menu_frame_actions[gmouse_3d_menu_current_action_index]], 0, 0, 0);
    Art* art = art_ptr_lock(fid, &handle);
    if (art == NULL) {
        return -1;
    }

    int width = art_frame_width(art, 0, 0);
    int height = art_frame_length(art, 0, 0);
    unsigned char* data = art_frame_data(art, 0, 0);
    buf_to_buf(data, width, height, width, gmouse_3d_menu_actions_start + gmouse_3d_menu_frame_width * height * gmouse_3d_menu_current_action_index, gmouse_3d_menu_frame_width);
    art_ptr_unlock(handle);

    fid = art_id(OBJ_TYPE_INTERFACE, gmouse_3d_action_nums[gmouse_3d_menu_frame_actions[menuItemIndex]] - 1, 0, 0, 0);
    art = art_ptr_lock(fid, &handle);
    if (art == NULL) {
        return -1;
    }

    data = art_frame_data(art, 0, 0);
    buf_to_buf(data, width, height, width, gmouse_3d_menu_actions_start + gmouse_3d_menu_frame_width * height * menuItemIndex, gmouse_3d_menu_frame_width);
    art_ptr_unlock(handle);

    gmouse_3d_menu_current_action_index = menuItemIndex;

    return 0;
}

// 0x44D774
int gmouse_3d_build_to_hit_frame(const char* string, int color)
{
    CacheEntry* crosshairFrmHandle;
    int fid = art_id(OBJ_TYPE_INTERFACE, gmouse_3d_mode_nums[GAME_MOUSE_MODE_CROSSHAIR], 0, 0, 0);
    Art* crosshairFrm = art_ptr_lock(fid, &crosshairFrmHandle);
    if (crosshairFrm == NULL) {
        return -1;
    }

    memset(gmouse_3d_to_hit_frame_data, 0, gmouse_3d_to_hit_frame_size);

    int crosshairFrmWidth = art_frame_width(crosshairFrm, 0, 0);
    int crosshairFrmHeight = art_frame_length(crosshairFrm, 0, 0);
    unsigned char* crosshairFrmData = art_frame_data(crosshairFrm, 0, 0);
    buf_to_buf(crosshairFrmData,
        crosshairFrmWidth,
        crosshairFrmHeight,
        crosshairFrmWidth,
        gmouse_3d_to_hit_frame_data,
        gmouse_3d_to_hit_frame_width);

    int oldFont = text_curr();
    text_font(101);

    text_to_buf(gmouse_3d_to_hit_frame_data + gmouse_3d_to_hit_frame_width + crosshairFrmWidth + 1,
        string,
        gmouse_3d_to_hit_frame_width - crosshairFrmWidth,
        gmouse_3d_to_hit_frame_width,
        color);

    buf_outline(gmouse_3d_to_hit_frame_data + crosshairFrmWidth,
        gmouse_3d_to_hit_frame_width - crosshairFrmWidth,
        gmouse_3d_to_hit_frame_height,
        gmouse_3d_to_hit_frame_width,
        colorTable[0]);

    text_font(oldFont);

    art_ptr_unlock(crosshairFrmHandle);

    return 0;
}

// 0x44D878
int gmouse_3d_build_hex_frame(const char* string, int color)
{
    memset(gmouse_3d_hex_frame_data, 0, gmouse_3d_hex_frame_width * gmouse_3d_hex_frame_height);

    if (*string == '\0') {
        return 0;
    }

    int oldFont = text_curr();
    text_font(101);

    int length = text_width(string);
    text_to_buf(gmouse_3d_hex_frame_data + gmouse_3d_hex_frame_width * (gmouse_3d_hex_frame_height - text_height()) / 2 + (gmouse_3d_hex_frame_width - length) / 2, string, gmouse_3d_hex_frame_width, gmouse_3d_hex_frame_width, color);

    buf_outline(gmouse_3d_hex_frame_data, gmouse_3d_hex_frame_width, gmouse_3d_hex_frame_height, gmouse_3d_hex_frame_width, colorTable[0]);

    text_font(oldFont);

    int fid = art_id(OBJ_TYPE_INTERFACE, 1, 0, 0, 0);
    gmouse_3d_set_fid(fid);

    return 0;
}

// 0x44D954
void gmouse_3d_synch_item_highlight()
{
    bool itemHighlight;
    if (configGetBool(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_ITEM_HIGHLIGHT_KEY, &itemHighlight)) {
        gmouse_3d_item_highlight = itemHighlight;
    }
}

// 0x44D984
static int gmouse_3d_init()
{
    int fid;

    if (gmouse_3d_initialized) {
        return -1;
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 0, 0, 0, 0);
    if (obj_new(&obj_mouse, fid, -1) != 0) {
        return -1;
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 1, 0, 0, 0);
    if (obj_new(&obj_mouse_flat, fid, -1) != 0) {
        return -1;
    }

    if (obj_outline_object(obj_mouse_flat, OUTLINE_PALETTED | OUTLINE_TYPE_2, NULL) != 0) {
        return -1;
    }

    if (gmouse_3d_lock_frames() != 0) {
        return -1;
    }

    obj_mouse->flags |= OBJECT_LIGHT_THRU;
    obj_mouse->flags |= OBJECT_TEMPORARY;
    obj_mouse->flags |= OBJECT_FLAG_0x400;
    obj_mouse->flags |= OBJECT_SHOOT_THRU;
    obj_mouse->flags |= OBJECT_NO_BLOCK;

    obj_mouse_flat->flags |= OBJECT_FLAG_0x400;
    obj_mouse_flat->flags |= OBJECT_TEMPORARY;
    obj_mouse_flat->flags |= OBJECT_LIGHT_THRU;
    obj_mouse_flat->flags |= OBJECT_SHOOT_THRU;
    obj_mouse_flat->flags |= OBJECT_NO_BLOCK;

    obj_toggle_flat(obj_mouse_flat, NULL);

    int x;
    int y;
    mouse_get_position(&x, &y);

    Rect v9;
    gmouse_3d_move_to(x, y, map_elevation, &v9);

    gmouse_3d_initialized = true;

    gmouse_3d_synch_item_highlight();

    return 0;
}

// NOTE: Inlined.
//
// 0x44DAC0
static int gmouse_3d_reset()
{
    if (!gmouse_3d_initialized) {
        return -1;
    }

    // NOTE: Uninline.
    gmouse_3d_enable_modes();

    // NOTE: Uninline.
    gmouse_3d_reset_fid();

    gmouse_3d_set_mode(GAME_MOUSE_MODE_MOVE);
    gmouse_3d_on();

    gmouse_3d_last_mouse_x = -1;
    gmouse_3d_last_mouse_y = -1;
    gmouse_3d_hover_test = false;
    gmouse_3d_last_move_time = get_time();
    gmouse_3d_synch_item_highlight();

    return 0;
}

// NOTE: Inlined.
//
// 0x44DB34
static void gmouse_3d_exit()
{
    if (gmouse_3d_initialized) {
        gmouse_3d_unlock_frames();

        obj_mouse->flags &= ~OBJECT_TEMPORARY;
        obj_mouse_flat->flags &= ~OBJECT_TEMPORARY;

        obj_erase_object(obj_mouse, NULL);
        obj_erase_object(obj_mouse_flat, NULL);

        gmouse_3d_initialized = false;
    }
}

// 0x44DB78
static int gmouse_3d_lock_frames()
{
    int fid;

    // actmenu.frm - action menu
    fid = art_id(OBJ_TYPE_INTERFACE, 283, 0, 0, 0);
    gmouse_3d_menu_frame = art_ptr_lock(fid, &gmouse_3d_menu_frame_key);
    if (gmouse_3d_menu_frame == NULL) {
        goto err;
    }

    // actpick.frm - action pick
    fid = art_id(OBJ_TYPE_INTERFACE, 282, 0, 0, 0);
    gmouse_3d_pick_frame = art_ptr_lock(fid, &gmouse_3d_pick_frame_key);
    if (gmouse_3d_pick_frame == NULL) {
        goto err;
    }

    // acttohit.frm - action to hit
    fid = art_id(OBJ_TYPE_INTERFACE, 284, 0, 0, 0);
    gmouse_3d_to_hit_frame = art_ptr_lock(fid, &gmouse_3d_to_hit_frame_key);
    if (gmouse_3d_to_hit_frame == NULL) {
        goto err;
    }

    // blank.frm - used be mset000.frm for top of bouncing mouse cursor
    fid = art_id(OBJ_TYPE_INTERFACE, 0, 0, 0, 0);
    gmouse_3d_hex_base_frame = art_ptr_lock(fid, &gmouse_3d_hex_base_frame_key);
    if (gmouse_3d_hex_base_frame == NULL) {
        goto err;
    }

    // msef000.frm - hex mouse cursor
    fid = art_id(OBJ_TYPE_INTERFACE, 1, 0, 0, 0);
    gmouse_3d_hex_frame = art_ptr_lock(fid, &gmouse_3d_hex_frame_key);
    if (gmouse_3d_hex_frame == NULL) {
        goto err;
    }

    gmouse_3d_menu_frame_width = art_frame_width(gmouse_3d_menu_frame, 0, 0);
    gmouse_3d_menu_frame_height = art_frame_length(gmouse_3d_menu_frame, 0, 0);
    gmouse_3d_menu_frame_size = gmouse_3d_menu_frame_width * gmouse_3d_menu_frame_height;
    gmouse_3d_menu_frame_data = art_frame_data(gmouse_3d_menu_frame, 0, 0);

    gmouse_3d_pick_frame_width = art_frame_width(gmouse_3d_pick_frame, 0, 0);
    gmouse_3d_pick_frame_height = art_frame_length(gmouse_3d_pick_frame, 0, 0);
    gmouse_3d_pick_frame_size = gmouse_3d_pick_frame_width * gmouse_3d_pick_frame_height;
    gmouse_3d_pick_frame_data = art_frame_data(gmouse_3d_pick_frame, 0, 0);

    gmouse_3d_to_hit_frame_width = art_frame_width(gmouse_3d_to_hit_frame, 0, 0);
    gmouse_3d_to_hit_frame_height = art_frame_length(gmouse_3d_to_hit_frame, 0, 0);
    gmouse_3d_to_hit_frame_size = gmouse_3d_to_hit_frame_width * gmouse_3d_to_hit_frame_height;
    gmouse_3d_to_hit_frame_data = art_frame_data(gmouse_3d_to_hit_frame, 0, 0);

    gmouse_3d_hex_base_frame_width = art_frame_width(gmouse_3d_hex_base_frame, 0, 0);
    gmouse_3d_hex_base_frame_height = art_frame_length(gmouse_3d_hex_base_frame, 0, 0);
    gmouse_3d_hex_base_frame_size = gmouse_3d_hex_base_frame_width * gmouse_3d_hex_base_frame_height;
    gmouse_3d_hex_base_frame_data = art_frame_data(gmouse_3d_hex_base_frame, 0, 0);

    gmouse_3d_hex_frame_width = art_frame_width(gmouse_3d_hex_frame, 0, 0);
    gmouse_3d_hex_frame_height = art_frame_length(gmouse_3d_hex_frame, 0, 0);
    gmouse_3d_hex_frame_size = gmouse_3d_hex_frame_width * gmouse_3d_hex_frame_height;
    gmouse_3d_hex_frame_data = art_frame_data(gmouse_3d_hex_frame, 0, 0);

    return 0;

err:

    // NOTE: Original code is different. There is no call to this function.
    // Instead it either use deep nesting or bunch of goto's to unwind
    // locked frms from the point of failure.
    gmouse_3d_unlock_frames();

    return -1;
}

// 0x44DE44
static void gmouse_3d_unlock_frames()
{
    if (gmouse_3d_hex_base_frame_key != INVALID_CACHE_ENTRY) {
        art_ptr_unlock(gmouse_3d_hex_base_frame_key);
    }
    gmouse_3d_hex_base_frame = NULL;
    gmouse_3d_hex_base_frame_key = INVALID_CACHE_ENTRY;

    if (gmouse_3d_hex_frame_key != INVALID_CACHE_ENTRY) {
        art_ptr_unlock(gmouse_3d_hex_frame_key);
    }
    gmouse_3d_hex_frame = NULL;
    gmouse_3d_hex_frame_key = INVALID_CACHE_ENTRY;

    if (gmouse_3d_to_hit_frame_key != INVALID_CACHE_ENTRY) {
        art_ptr_unlock(gmouse_3d_to_hit_frame_key);
    }
    gmouse_3d_to_hit_frame = NULL;
    gmouse_3d_to_hit_frame_key = INVALID_CACHE_ENTRY;

    if (gmouse_3d_menu_frame_key != INVALID_CACHE_ENTRY) {
        art_ptr_unlock(gmouse_3d_menu_frame_key);
    }
    gmouse_3d_menu_frame = NULL;
    gmouse_3d_menu_frame_key = INVALID_CACHE_ENTRY;

    if (gmouse_3d_pick_frame_key != INVALID_CACHE_ENTRY) {
        art_ptr_unlock(gmouse_3d_pick_frame_key);
    }

    gmouse_3d_pick_frame = NULL;
    gmouse_3d_pick_frame_key = INVALID_CACHE_ENTRY;

    gmouse_3d_pick_frame_data = NULL;
    gmouse_3d_pick_frame_width = 0;
    gmouse_3d_pick_frame_height = 0;
    gmouse_3d_pick_frame_size = 0;
}

// NOTE: Inlined.
//
// 0x44DF1C
static int gmouse_3d_set_flat_fid(int fid, Rect* rect)
{
    if (obj_change_fid(obj_mouse_flat, fid, rect) == 0) {
        return 0;
    }

    return -1;
}

// 0x44DF40
static int gmouse_3d_reset_flat_fid(Rect* rect)
{
    int fid = art_id(OBJ_TYPE_INTERFACE, gmouse_3d_mode_nums[gmouse_3d_current_mode], 0, 0, 0);
    if (obj_mouse_flat->fid == fid) {
        return -1;
    }

    // NOTE: Uninline.
    return gmouse_3d_set_flat_fid(fid, rect);
}

// 0x44DF94
static int gmouse_3d_move_to(int x, int y, int elevation, Rect* a4)
{
    if (gmouse_mapper_mode == 0) {
        if (gmouse_3d_current_mode != GAME_MOUSE_MODE_MOVE) {
            int offsetX = 0;
            int offsetY = 0;
            CacheEntry* hexCursorFrmHandle;
            Art* hexCursorFrm = art_ptr_lock(obj_mouse_flat->fid, &hexCursorFrmHandle);
            if (hexCursorFrm != NULL) {
                art_frame_offset(hexCursorFrm, 0, &offsetX, &offsetY);

                int frameOffsetX;
                int frameOffsetY;
                art_frame_hot(hexCursorFrm, 0, 0, &frameOffsetX, &frameOffsetY);

                offsetX += frameOffsetX;
                offsetY += frameOffsetY;

                art_ptr_unlock(hexCursorFrmHandle);
            }

            obj_move(obj_mouse_flat, x + offsetX, y + offsetY, elevation, a4);
        } else {
            int tile = tile_num(x, y, 0);
            if (tile != -1) {
                int screenX;
                int screenY;

                bool v1 = false;
                Rect rect1;
                if (tile_coord(tile, &screenX, &screenY, 0) == 0) {
                    if (obj_move(obj_mouse, screenX + 16, screenY + 15, 0, &rect1) == 0) {
                        v1 = true;
                    }
                }

                Rect rect2;
                if (obj_move_to_tile(obj_mouse_flat, tile, elevation, &rect2) == 0) {
                    if (v1) {
                        rect_min_bound(&rect1, &rect2, &rect1);
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

    int fid = obj_mouse->fid;
    if (FID_TYPE(fid) == OBJ_TYPE_TILE) {
        int squareTile = square_num(x, y, elevation);
        if (squareTile == -1) {
            tile = HEX_GRID_WIDTH * (2 * (squareTile / SQUARE_GRID_WIDTH) + 1) + 2 * (squareTile % SQUARE_GRID_WIDTH) + 1;
            x1 = -8;
            y1 = 13;

            char* executable;
            config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_EXECUTABLE_KEY, &executable);
            if (stricmp(executable, "mapper") == 0) {
                if (tile_roof_visible()) {
                    if ((obj_dude->flags & OBJECT_HIDDEN) == 0) {
                        y1 = -83;
                    }
                }
            }
        } else {
            tile = -1;
        }
    } else {
        tile = tile_num(x, y, elevation);
    }

    if (tile != -1) {
        bool v1 = false;

        Rect rect1;
        Rect rect2;

        if (obj_move_to_tile(obj_mouse, tile, elevation, &rect1) == 0) {
            if (x1 != 0 || y1 != 0) {
                if (obj_offset(obj_mouse, x1, y1, &rect2) == 0) {
                    rect_min_bound(&rect1, &rect2, &rect1);
                }
            }
            v1 = true;
        }

        if (gmouse_3d_current_mode != GAME_MOUSE_MODE_MOVE) {
            int offsetX = 0;
            int offsetY = 0;
            CacheEntry* hexCursorFrmHandle;
            Art* hexCursorFrm = art_ptr_lock(obj_mouse_flat->fid, &hexCursorFrmHandle);
            if (hexCursorFrm != NULL) {
                art_frame_offset(hexCursorFrm, 0, &offsetX, &offsetY);

                int frameOffsetX;
                int frameOffsetY;
                art_frame_hot(hexCursorFrm, 0, 0, &frameOffsetX, &frameOffsetY);

                offsetX += frameOffsetX;
                offsetY += frameOffsetY;

                art_ptr_unlock(hexCursorFrmHandle);
            }

            if (obj_move(obj_mouse_flat, x + offsetX, y + offsetY, elevation, &rect2) == 0) {
                if (v1) {
                    rect_min_bound(&rect1, &rect2, &rect1);
                } else {
                    rectCopy(&rect1, &rect2);
                    v1 = true;
                }
            }
        } else {
            if (obj_move_to_tile(obj_mouse_flat, tile, elevation, &rect2) == 0) {
                if (v1) {
                    rect_min_bound(&rect1, &rect2, &rect1);
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
static int gmouse_check_scrolling(int x, int y, int cursor)
{
    if (!gmouse_scrolling_enabled) {
        return -1;
    }

    int flags = 0;

    if (x <= scr_size.ulx) {
        flags |= SCROLLABLE_W;
    }

    if (x >= scr_size.lrx) {
        flags |= SCROLLABLE_E;
    }

    if (y <= scr_size.uly) {
        flags |= SCROLLABLE_N;
    }

    if (y >= scr_size.lry) {
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

    int rc = map_scroll(dx, dy);
    switch (rc) {
    case -1:
        // Scrolling is blocked for whatever reason, upgrade cursor to
        // appropriate blocked version.
        cursor += 8;
        // FALLTHROUGH
    case 0:
        gmouse_set_cursor(cursor);
        break;
    }

    return 0;
}

// 0x44E544
void gmouse_remove_item_outline(Object* object)
{
    if (outlined_object != NULL && outlined_object == object) {
        Rect rect;
        if (obj_remove_outline(object, &rect) == 0) {
            tile_refresh_rect(&rect, map_elevation);
        }
        outlined_object = NULL;
    }
}

// 0x44E580
static int gmObjIsValidTarget(Object* object)
{
    if (object == NULL) {
        return false;
    }

    if (PID_TYPE(object->pid) != OBJ_TYPE_SCENERY) {
        return false;
    }

    Proto* proto;
    if (proto_ptr(object->pid, &proto) == -1) {
        return false;
    }

    return proto->scenery.type == SCENERY_TYPE_DOOR;
}
