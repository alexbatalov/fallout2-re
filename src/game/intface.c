#include "game/intface.h"

#include <stdio.h>
#include <string.h>

#include "game/art.h"
#include "game/anim.h"
#include "plib/color/color.h"
#include "game/combat.h"
#include "game/config.h"
#include "plib/gnw/input.h"
#include "game/critter.h"
#include "game/cycle.h"
#include "plib/gnw/debug.h"
#include "game/display.h"
#include "plib/gnw/grbuf.h"
#include "game/endgame.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/gmouse.h"
#include "game/gsound.h"
#include "plib/gnw/rect.h"
#include "game/item.h"
#include "plib/gnw/memory.h"
#include "game/object.h"
#include "game/proto.h"
#include "game/protinst.h"
#include "game/proto_types.h"
#include "game/skill.h"
#include "game/stat.h"
#include "plib/gnw/text.h"
#include "game/tile.h"
#include "plib/gnw/button.h"
#include "plib/gnw/gnw.h"

#define INDICATOR_BAR_X 0
#define INDICATOR_BAR_Y 358

// The width of connectors in the indicator box.
//
// There are male connectors on the left, and female connectors on the right.
// When displaying series of boxes they appear to be plugged into a chain.
#define INDICATOR_BOX_CONNECTOR_WIDTH 3

// Minimum radiation amount to display RADIATED indicator.
#define RADATION_INDICATOR_THRESHOLD 65

// Minimum poison amount to display POISONED indicator.
#define POISON_INDICATOR_THRESHOLD 0

// The values of it's members are offsets to beginning of numbers in
// numbers.frm.
typedef enum InterfaceNumbersColor {
    INTERFACE_NUMBERS_COLOR_WHITE = 0,
    INTERFACE_NUMBERS_COLOR_YELLOW = 120,
    INTERFACE_NUMBERS_COLOR_RED = 240,
} InterfaceNumbersColor;

#define INDICATOR_BOX_WIDTH 130
#define INDICATOR_BOX_HEIGHT 21

// The maximum number of indicator boxes the indicator bar can display.
//
// For unknown reason this number is 6, even though there are only 5 different
// indicator types. In addition to that, default screen width 640px cannot hold
// 6 boxes 130px each.
#define INDICATOR_SLOTS_COUNT (6)

// Available indicators.
//
// Indicator boxes in the bar are displayed according to the order of this enum.
typedef enum Indicator {
    INDICATOR_ADDICT,
    INDICATOR_SNEAK,
    INDICATOR_LEVEL,
    INDICATOR_POISONED,
    INDICATOR_RADIATED,
    INDICATOR_COUNT,
} Indicator;

// Provides metadata about indicator boxes.
typedef struct IndicatorDescription {
    // An identifier of title in `intrface.msg`.
    int title;

    // A flag denoting this box represents something harmful to the player. It
    // affects color of the title.
    bool isBad;

    // Prerendered indicator data.
    //
    // This value is provided at runtime during indicator box initialization.
    // It includes indicator box background with it's title positioned in the
    // center and is green colored if indicator is good, or red otherwise, as
    // denoted by [isBad] property.
    unsigned char* data;
} IndicatorDescription;

typedef struct InterfaceItemState {
    Object* item;
    unsigned char isDisabled;
    unsigned char isWeapon;
    int primaryHitMode;
    int secondaryHitMode;
    int action;
    int itemFid;
} InterfaceItemState;

static int intface_init_items();
static int intface_redraw_items();
static int intface_redraw_items_callback(Object* a1, Object* a2);
static int intface_change_fid_callback(Object* a1, Object* a2);
static void intface_change_fid_animate(int previousWeaponAnimationCode, int weaponAnimationCode);
static int intface_create_end_turn_button();
static int intface_destroy_end_turn_button();
static int intface_create_end_combat_button();
static int intface_destroy_end_combat_button();
static void intface_draw_ammo_lights(int x, int ratio);
static int intface_item_reload();
static void intface_rotate_numbers(int x, int y, int previousValue, int value, int offset, int delay);
static int intface_fatal_error(int rc);
static int construct_box_bar_win();
static void deconstruct_box_bar_win();
static void reset_box_bar_win();
static int bbox_comp(const void* a, const void* b);
static void draw_bboxes(int count);
static bool add_bar_box(int indicator);

// 0x518F08
static bool insideInit = false;

// 0x518F0C
static bool intface_fid_is_changing = false;

// 0x518F10
static bool intfaceEnabled = false;

// 0x518F14
static bool intfaceHidden = false;

// 0x518F18
static int inventoryButton = -1;

// 0x518F1C
static CacheEntry* inventoryButtonUpKey = NULL;

// 0x518F20
static CacheEntry* inventoryButtonDownKey = NULL;

// 0x518F24
static int optionsButton = -1;

// 0x518F28
static CacheEntry* optionsButtonUpKey = NULL;

// 0x518F2C
static CacheEntry* optionsButtonDownKey = NULL;

// 0x518F30
static int skilldexButton = -1;

// 0x518F34
static CacheEntry* skilldexButtonUpKey = NULL;

// 0x518F38
static CacheEntry* skilldexButtonDownKey = NULL;

// 0x518F3C
static CacheEntry* skilldexButtonMaskKey = NULL;

// 0x518F40
static int automapButton = -1;

// 0x518F44
static CacheEntry* automapButtonUpKey = NULL;

// 0x518F48
static CacheEntry* automapButtonDownKey = NULL;

// 0x518F4C
static CacheEntry* automapButtonMaskKey = NULL;

// 0x518F50
static int pipboyButton = -1;

// 0x518F54
static CacheEntry* pipboyButtonUpKey = NULL;

// 0x518F58
static CacheEntry* pipboyButtonDownKey = NULL;

// 0x518F5C
static int characterButton = -1;

// 0x518F60
static CacheEntry* characterButtonUpKey = NULL;

// 0x518F64
static CacheEntry* characterButtonDownKey = NULL;

// 0x518F68
static int itemButton = -1;

// 0x518F6C
static CacheEntry* itemButtonUpKey = NULL;

// 0x518F70
static CacheEntry* itemButtonDownKey = NULL;

// 0x518F74
static CacheEntry* itemButtonDisabledKey = NULL;

// 0x518F78
static int itemCurrentItem = HAND_LEFT;

// 0x518F7C
static Rect itemButtonRect = { 267, 26, 455, 93 };

// 0x518F8C
static int toggleButton = -1;

// 0x518F90
static CacheEntry* toggleButtonUpKey = NULL;

// 0x518F94
static CacheEntry* toggleButtonDownKey = NULL;

// 0x518F98
static CacheEntry* toggleButtonMaskKey = NULL;

// 0x518F9C
static bool endWindowOpen = false;

// Combat mode curtains rect.
//
// 0x518FA0
static Rect endWindowRect = { 580, 38, 637, 96 };

// 0x518FB0
static int endTurnButton = -1;

// 0x518FB4
static CacheEntry* endTurnButtonUpKey = NULL;

// 0x518FB8
static CacheEntry* endTurnButtonDownKey = NULL;

// 0x518FBC
static int endCombatButton = -1;

// 0x518FC0
static CacheEntry* endCombatButtonUpKey = NULL;

// 0x518FC4
static CacheEntry* endCombatButtonDownKey = NULL;

// 0x518FC8
static unsigned char* moveLightGreen = NULL;

// 0x518FCC
static unsigned char* moveLightYellow = NULL;

// 0x518FD0
static unsigned char* moveLightRed = NULL;

// 0x518FD4
static Rect movePointRect = { 316, 14, 406, 19 };

// 0x518FE4
static unsigned char* numbersBuffer = NULL;

// 0x518FE8
static IndicatorDescription bbox[INDICATOR_COUNT] = {
    { 102, true, NULL }, // ADDICT
    { 100, false, NULL }, // SNEAK
    { 101, false, NULL }, // LEVEL
    { 103, true, NULL }, // POISONED
    { 104, true, NULL }, // RADIATED
};

// 0x519024
int interfaceWindow = -1;

// 0x519028
int bar_window = -1;

// Each slot contains one of indicators or -1 if slot is empty.
//
// 0x5970E0
static int bboxslot[INDICATOR_SLOTS_COUNT];

// 0x5970F8
static InterfaceItemState itemButtonItems[HAND_COUNT];

// 0x597128
static CacheEntry* moveLightYellowKey;

// 0x59712C
static CacheEntry* moveLightRedKey;

// 0x597130
static CacheEntry* numbersKey;

// 0x597138
static bool box_status_flag;

// 0x59713C
static unsigned char* toggleButtonUp;

// 0x597140
static CacheEntry* moveLightGreenKey;

// 0x597144
static unsigned char* endCombatButtonUp;

// 0x597148
static unsigned char* endCombatButtonDown;

// 0x59714C
static unsigned char* toggleButtonDown;

// 0x597150
static unsigned char* endTurnButtonDown;

// 0x597154
static unsigned char itemButtonDown[188 * 67];

// 0x59A288
static unsigned char* endTurnButtonUp;

// 0x59A28C
static unsigned char* toggleButtonMask;

// 0x59A290
static unsigned char* characterButtonUp;

// 0x59A294
static unsigned char* itemButtonUpBlank;

// 0x59A298
static unsigned char* itemButtonDisabled;

// 0x59A29C
static unsigned char* automapButtonDown;

// 0x59A2A0
static unsigned char* pipboyButtonUp;

// 0x59A2A4
static unsigned char* characterButtonDown;

// 0x59A2A8
static unsigned char* itemButtonDownBlank;

// 0x59A2AC
static unsigned char* pipboyButtonDown;

// 0x59A2B0
static unsigned char* automapButtonMask;

// 0x59A2B4
static unsigned char itemButtonUp[188 * 67];

// 0x59D3E8
static unsigned char* automapButtonUp;

// 0x59D3EC
static unsigned char* skilldexButtonMask;

// 0x59D3F0
static unsigned char* skilldexButtonDown;

// 0x59D3F4
static unsigned char* interfaceBuffer;

// 0x59D3F8
static unsigned char* inventoryButtonUp;

// 0x59D3FC
static unsigned char* optionsButtonUp;

// 0x59D400
static unsigned char* optionsButtonDown;

// 0x59D404
static unsigned char* skilldexButtonUp;

// 0x59D408
static unsigned char* inventoryButtonDown;

// A slice of main interface background containing 10 shadowed action point
// dots. In combat mode individual colored dots are rendered on top of this
// background.
//
// This buffer is initialized once and does not change throughout the game.
//
// 0x59D40C
static unsigned char movePointBackground[90 * 5];

// 0x45D880
int intface_init()
{
    int fid;
    CacheEntry* backgroundFrmHandle;
    unsigned char* backgroundFrmData;

    if (interfaceWindow != -1) {
        return -1;
    }

    insideInit = 1;

    int interfaceBarWindowX = 0;
    int interfaceBarWindowY = 480 - INTERFACE_BAR_HEIGHT - 1;

    interfaceWindow = win_add(interfaceBarWindowX, interfaceBarWindowY, INTERFACE_BAR_WIDTH, INTERFACE_BAR_HEIGHT, colorTable[0], WINDOW_HIDDEN);
    if (interfaceWindow == -1) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    interfaceBuffer = win_get_buf(interfaceWindow);
    if (interfaceBuffer == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 16, 0, 0, 0);
    backgroundFrmData = art_ptr_lock_data(fid, 0, 0, &backgroundFrmHandle);
    if (backgroundFrmData == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    buf_to_buf(backgroundFrmData, INTERFACE_BAR_WIDTH, INTERFACE_BAR_HEIGHT - 1, INTERFACE_BAR_WIDTH, interfaceBuffer, 640);
    art_ptr_unlock(backgroundFrmHandle);

    fid = art_id(OBJ_TYPE_INTERFACE, 47, 0, 0, 0);
    inventoryButtonUp = art_ptr_lock_data(fid, 0, 0, &inventoryButtonUpKey);
    if (inventoryButtonUp == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 46, 0, 0, 0);
    inventoryButtonDown = art_ptr_lock_data(fid, 0, 0, &inventoryButtonDownKey);
    if (inventoryButtonDown == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    inventoryButton = win_register_button(interfaceWindow, 211, 41, 32, 21, -1, -1, -1, KEY_LOWERCASE_I, inventoryButtonUp, inventoryButtonDown, NULL, 0);
    if (inventoryButton == -1) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    win_register_button_sound_func(inventoryButton, gsound_med_butt_press, gsound_med_butt_release);

    fid = art_id(OBJ_TYPE_INTERFACE, 18, 0, 0, 0);
    optionsButtonUp = art_ptr_lock_data(fid, 0, 0, &optionsButtonUpKey);
    if (optionsButtonUp == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 17, 0, 0, 0);
    optionsButtonDown = art_ptr_lock_data(fid, 0, 0, &optionsButtonDownKey);
    if (optionsButtonDown == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    optionsButton = win_register_button(interfaceWindow, 210, 62, 34, 34, -1, -1, -1, KEY_LOWERCASE_O, optionsButtonUp, optionsButtonDown, NULL, 0);
    if (optionsButton == -1) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    win_register_button_sound_func(optionsButton, gsound_med_butt_press, gsound_med_butt_release);

    fid = art_id(OBJ_TYPE_INTERFACE, 6, 0, 0, 0);
    skilldexButtonUp = art_ptr_lock_data(fid, 0, 0, &skilldexButtonUpKey);
    if (skilldexButtonUp == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 7, 0, 0, 0);
    skilldexButtonDown = art_ptr_lock_data(fid, 0, 0, &skilldexButtonDownKey);
    if (skilldexButtonDown == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 6, 0, 0, 0);
    skilldexButtonMask = art_ptr_lock_data(fid, 0, 0, &skilldexButtonMaskKey);
    if (skilldexButtonMask == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    skilldexButton = win_register_button(interfaceWindow, 523, 6, 22, 21, -1, -1, -1, KEY_LOWERCASE_S, skilldexButtonUp, skilldexButtonDown, NULL, BUTTON_FLAG_TRANSPARENT);
    if (skilldexButton == -1) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    win_register_button_mask(skilldexButton, skilldexButtonMask);
    win_register_button_sound_func(skilldexButton, gsound_med_butt_press, gsound_med_butt_release);

    fid = art_id(OBJ_TYPE_INTERFACE, 13, 0, 0, 0);
    automapButtonUp = art_ptr_lock_data(fid, 0, 0, &automapButtonUpKey);
    if (automapButtonUp == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 10, 0, 0, 0);
    automapButtonDown = art_ptr_lock_data(fid, 0, 0, &automapButtonDownKey);
    if (automapButtonDown == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 13, 0, 0, 0);
    automapButtonMask = art_ptr_lock_data(fid, 0, 0, &automapButtonMaskKey);
    if (automapButtonMask == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    automapButton = win_register_button(interfaceWindow, 526, 40, 41, 19, -1, -1, -1, KEY_TAB, automapButtonUp, automapButtonDown, NULL, BUTTON_FLAG_TRANSPARENT);
    if (automapButton == -1) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    win_register_button_mask(automapButton, automapButtonMask);
    win_register_button_sound_func(automapButton, gsound_med_butt_press, gsound_med_butt_release);

    fid = art_id(OBJ_TYPE_INTERFACE, 59, 0, 0, 0);
    pipboyButtonUp = art_ptr_lock_data(fid, 0, 0, &pipboyButtonUpKey);
    if (pipboyButtonUp == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 58, 0, 0, 0);
    pipboyButtonDown = art_ptr_lock_data(fid, 0, 0, &pipboyButtonDownKey);
    if (pipboyButtonDown == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    pipboyButton = win_register_button(interfaceWindow, 526, 78, 41, 19, -1, -1, -1, KEY_LOWERCASE_P, pipboyButtonUp, pipboyButtonDown, NULL, 0);
    if (pipboyButton == -1) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    win_register_button_mask(pipboyButton, automapButtonMask);
    win_register_button_sound_func(pipboyButton, gsound_med_butt_press, gsound_med_butt_release);

    fid = art_id(OBJ_TYPE_INTERFACE, 57, 0, 0, 0);
    characterButtonUp = art_ptr_lock_data(fid, 0, 0, &characterButtonUpKey);
    if (characterButtonUp == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 56, 0, 0, 0);
    characterButtonDown = art_ptr_lock_data(fid, 0, 0, &characterButtonDownKey);
    if (characterButtonDown == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    characterButton = win_register_button(interfaceWindow, 526, 59, 41, 19, -1, -1, -1, KEY_LOWERCASE_C, characterButtonUp, characterButtonDown, NULL, 0);
    if (characterButton == -1) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    win_register_button_mask(characterButton, automapButtonMask);
    win_register_button_sound_func(characterButton, gsound_med_butt_press, gsound_med_butt_release);

    fid = art_id(OBJ_TYPE_INTERFACE, 32, 0, 0, 0);
    itemButtonUpBlank = art_ptr_lock_data(fid, 0, 0, &itemButtonUpKey);
    if (itemButtonUpBlank == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 31, 0, 0, 0);
    itemButtonDownBlank = art_ptr_lock_data(fid, 0, 0, &itemButtonDownKey);
    if (itemButtonDownBlank == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 73, 0, 0, 0);
    itemButtonDisabled = art_ptr_lock_data(fid, 0, 0, &itemButtonDisabledKey);
    if (itemButtonDisabled == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    memcpy(itemButtonUp, itemButtonUpBlank, sizeof(itemButtonUp));
    memcpy(itemButtonDown, itemButtonDownBlank, sizeof(itemButtonDown));

    itemButton = win_register_button(interfaceWindow, 267, 26, 188, 67, -1, -1, -1, -20, itemButtonUp, itemButtonDown, NULL, BUTTON_FLAG_TRANSPARENT);
    if (itemButton == -1) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    win_register_right_button(itemButton, -1, KEY_LOWERCASE_N, NULL, NULL);
    win_register_button_sound_func(itemButton, gsound_lrg_butt_press, gsound_lrg_butt_release);

    fid = art_id(OBJ_TYPE_INTERFACE, 6, 0, 0, 0);
    toggleButtonUp = art_ptr_lock_data(fid, 0, 0, &toggleButtonUpKey);
    if (toggleButtonUp == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 7, 0, 0, 0);
    toggleButtonDown = art_ptr_lock_data(fid, 0, 0, &toggleButtonDownKey);
    if (toggleButtonDown == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 6, 0, 0, 0);
    toggleButtonMask = art_ptr_lock_data(fid, 0, 0, &toggleButtonMaskKey);
    if (toggleButtonMask == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    // Swap hands button
    toggleButton = win_register_button(interfaceWindow, 218, 6, 22, 21, -1, -1, -1, KEY_LOWERCASE_B, toggleButtonUp, toggleButtonDown, NULL, BUTTON_FLAG_TRANSPARENT);
    if (toggleButton == -1) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    win_register_button_mask(toggleButton, toggleButtonMask);
    win_register_button_sound_func(toggleButton, gsound_med_butt_press, gsound_med_butt_release);

    fid = art_id(OBJ_TYPE_INTERFACE, 82, 0, 0, 0);
    numbersBuffer = art_ptr_lock_data(fid, 0, 0, &numbersKey);
    if (numbersBuffer == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 83, 0, 0, 0);
    moveLightGreen = art_ptr_lock_data(fid, 0, 0, &moveLightGreenKey);
    if (moveLightGreen == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 84, 0, 0, 0);
    moveLightYellow = art_ptr_lock_data(fid, 0, 0, &moveLightYellowKey);
    if (moveLightYellow == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 85, 0, 0, 0);
    moveLightRed = art_ptr_lock_data(fid, 0, 0, &moveLightRedKey);
    if (moveLightRed == NULL) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    buf_to_buf(interfaceBuffer + 640 * 14 + 316, 90, 5, 640, movePointBackground, 90);

    if (construct_box_bar_win() == -1) {
        // NOTE: Uninline.
        return intface_fatal_error(-1);
    }

    itemCurrentItem = HAND_LEFT;

    // NOTE: Uninline.
    intface_init_items();

    display_init();

    intfaceEnabled = true;
    insideInit = false;
    intfaceHidden = 1;

    return 0;
}

// 0x45E3D0
void intface_reset()
{
    intface_enable();

    // NOTE: Uninline.
    intface_hide();

    display_reset();

    // NOTE: Uninline.
    reset_box_bar_win();

    itemCurrentItem = 0;
}

// 0x45E440
void intface_exit()
{
    if (interfaceWindow != -1) {
        display_exit();

        if (moveLightRed != NULL) {
            art_ptr_unlock(moveLightRedKey);
            moveLightRed = NULL;
        }

        if (moveLightYellow != NULL) {
            art_ptr_unlock(moveLightYellowKey);
            moveLightYellow = NULL;
        }

        if (moveLightGreen != NULL) {
            art_ptr_unlock(moveLightGreenKey);
            moveLightGreen = NULL;
        }

        if (numbersBuffer != NULL) {
            art_ptr_unlock(numbersKey);
            numbersBuffer = NULL;
        }

        if (toggleButton != -1) {
            win_delete_button(toggleButton);
            toggleButton = -1;
        }

        if (toggleButtonMask != NULL) {
            art_ptr_unlock(toggleButtonMaskKey);
            toggleButtonMaskKey = NULL;
            toggleButtonMask = NULL;
        }

        if (toggleButtonDown != NULL) {
            art_ptr_unlock(toggleButtonDownKey);
            toggleButtonDownKey = NULL;
            toggleButtonDown = NULL;
        }

        if (toggleButtonUp != NULL) {
            art_ptr_unlock(toggleButtonUpKey);
            toggleButtonUpKey = NULL;
            toggleButtonUp = NULL;
        }

        if (itemButton != -1) {
            win_delete_button(itemButton);
            itemButton = -1;
        }

        if (itemButtonDisabled != NULL) {
            art_ptr_unlock(itemButtonDisabledKey);
            itemButtonDisabledKey = NULL;
            itemButtonDisabled = NULL;
        }

        if (itemButtonDownBlank != NULL) {
            art_ptr_unlock(itemButtonDownKey);
            itemButtonDownKey = NULL;
            itemButtonDownBlank = NULL;
        }

        if (itemButtonUpBlank != NULL) {
            art_ptr_unlock(itemButtonUpKey);
            itemButtonUpKey = NULL;
            itemButtonUpBlank = NULL;
        }

        if (characterButton != -1) {
            win_delete_button(characterButton);
            characterButton = -1;
        }

        if (characterButtonDown != NULL) {
            art_ptr_unlock(characterButtonDownKey);
            characterButtonDownKey = NULL;
            characterButtonDown = NULL;
        }

        if (characterButtonUp != NULL) {
            art_ptr_unlock(characterButtonUpKey);
            characterButtonUpKey = NULL;
            characterButtonUp = NULL;
        }

        if (pipboyButton != -1) {
            win_delete_button(pipboyButton);
            pipboyButton = -1;
        }

        if (pipboyButtonDown != NULL) {
            art_ptr_unlock(pipboyButtonDownKey);
            pipboyButtonDownKey = NULL;
            pipboyButtonDown = NULL;
        }

        if (pipboyButtonUp != NULL) {
            art_ptr_unlock(pipboyButtonUpKey);
            pipboyButtonUpKey = NULL;
            pipboyButtonUp = NULL;
        }

        if (automapButton != -1) {
            win_delete_button(automapButton);
            automapButton = -1;
        }

        if (automapButtonMask != NULL) {
            art_ptr_unlock(automapButtonMaskKey);
            automapButtonMaskKey = NULL;
            automapButtonMask = NULL;
        }

        if (automapButtonDown != NULL) {
            art_ptr_unlock(automapButtonDownKey);
            automapButtonDownKey = NULL;
            automapButtonDown = NULL;
        }

        if (automapButtonUp != NULL) {
            art_ptr_unlock(automapButtonUpKey);
            automapButtonUpKey = NULL;
            automapButtonUp = NULL;
        }

        if (skilldexButton != -1) {
            win_delete_button(skilldexButton);
            skilldexButton = -1;
        }

        if (skilldexButtonMask != NULL) {
            art_ptr_unlock(skilldexButtonMaskKey);
            skilldexButtonMaskKey = NULL;
            skilldexButtonMask = NULL;
        }

        if (skilldexButtonDown != NULL) {
            art_ptr_unlock(skilldexButtonDownKey);
            skilldexButtonDownKey = NULL;
            skilldexButtonDown = NULL;
        }

        if (skilldexButtonUp != NULL) {
            art_ptr_unlock(skilldexButtonUpKey);
            skilldexButtonUpKey = NULL;
            skilldexButtonUp = NULL;
        }

        if (optionsButton != -1) {
            win_delete_button(optionsButton);
            optionsButton = -1;
        }

        if (optionsButtonDown != NULL) {
            art_ptr_unlock(optionsButtonDownKey);
            optionsButtonDownKey = NULL;
            optionsButtonDown = NULL;
        }

        if (optionsButtonUp != NULL) {
            art_ptr_unlock(optionsButtonUpKey);
            optionsButtonUpKey = NULL;
            optionsButtonUp = NULL;
        }

        if (inventoryButton != -1) {
            win_delete_button(inventoryButton);
            inventoryButton = -1;
        }

        if (inventoryButtonDown != NULL) {
            art_ptr_unlock(inventoryButtonDownKey);
            inventoryButtonDownKey = NULL;
            inventoryButtonDown = NULL;
        }

        if (inventoryButtonUp != NULL) {
            art_ptr_unlock(inventoryButtonUpKey);
            inventoryButtonUpKey = NULL;
            inventoryButtonUp = NULL;
        }

        if (interfaceWindow != -1) {
            win_delete(interfaceWindow);
            interfaceWindow = -1;
        }
    }

    deconstruct_box_bar_win();
}

// 0x45E860
int intface_load(File* stream)
{
    if (interfaceWindow == -1) {
        if (intface_init() == -1) {
            return -1;
        }
    }

    int interfaceBarEnabled;
    if (db_freadInt(stream, &interfaceBarEnabled) == -1) return -1;

    int v2;
    if (db_freadInt(stream, &v2) == -1) return -1;

    int interfaceCurrentHand;
    if (db_freadInt(stream, &interfaceCurrentHand) == -1) return -1;

    bool interfaceBarEndButtonsIsVisible;
    if (fileReadBool(stream, &interfaceBarEndButtonsIsVisible) == -1) return -1;

    if (!intfaceEnabled) {
        intface_enable();
    }

    if (v2) {
        // NOTE: Uninline.
        intface_hide();
    } else {
        intface_show();
    }

    intface_update_hit_points(false);
    intface_update_ac(false);

    itemCurrentItem = interfaceCurrentHand;

    intface_update_items(false, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);

    if (interfaceBarEndButtonsIsVisible != endWindowOpen) {
        if (interfaceBarEndButtonsIsVisible) {
            intface_end_window_open(false);
        } else {
            intface_end_window_close(false);
        }
    }

    if (!interfaceBarEnabled) {
        intface_disable();
    }

    refresh_box_bar_win();

    win_draw(interfaceWindow);

    return 0;
}

// 0x45E988
int intface_save(File* stream)
{
    if (interfaceWindow == -1) {
        return -1;
    }

    if (db_fwriteInt(stream, intfaceEnabled) == -1) return -1;
    if (db_fwriteInt(stream, intfaceHidden) == -1) return -1;
    if (db_fwriteInt(stream, itemCurrentItem) == -1) return -1;
    if (db_fwriteInt(stream, endWindowOpen) == -1) return -1;

    return 0;
}

// NOTE: Inlined.
//
// 0x45E9E0
void intface_hide()
{
    if (interfaceWindow != -1) {
        if (!intfaceHidden) {
            win_hide(interfaceWindow);
            intfaceHidden = 1;
        }
    }
    refresh_box_bar_win();
}

// 0x45EA10
void intface_show()
{
    if (interfaceWindow != -1) {
        if (intfaceHidden) {
            intface_update_items(false, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);
            intface_update_hit_points(false);
            intface_update_ac(false);
            win_show(interfaceWindow);
            intfaceHidden = false;
        }
    }
    refresh_box_bar_win();
}

// NOTE: Unused.
//
// 0x45EA5C
int intface_is_hidden()
{
    return intfaceHidden;
}

// 0x45EA64
void intface_enable()
{
    if (!intfaceEnabled) {
        win_enable_button(inventoryButton);
        win_enable_button(optionsButton);
        win_enable_button(skilldexButton);
        win_enable_button(automapButton);
        win_enable_button(pipboyButton);
        win_enable_button(characterButton);

        if (itemButtonItems[itemCurrentItem].isDisabled == 0) {
            win_enable_button(itemButton);
        }

        win_enable_button(endTurnButton);
        win_enable_button(endCombatButton);
        display_enable();

        intfaceEnabled = true;
    }
}

// 0x45EAFC
void intface_disable()
{
    if (intfaceEnabled) {
        display_disable();
        win_disable_button(inventoryButton);
        win_disable_button(optionsButton);
        win_disable_button(skilldexButton);
        win_disable_button(automapButton);
        win_disable_button(pipboyButton);
        win_disable_button(characterButton);
        if (itemButtonItems[itemCurrentItem].isDisabled == 0) {
            win_disable_button(itemButton);
        }
        win_disable_button(endTurnButton);
        win_disable_button(endCombatButton);
        intfaceEnabled = false;
    }
}

// 0x45EB90
bool intface_is_enabled()
{
    return intfaceEnabled;
}

// 0x45EB98
void intface_redraw()
{
    if (interfaceWindow != -1) {
        intface_update_items(false, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);
        intface_update_hit_points(false);
        intface_update_ac(false);
        refresh_box_bar_win();
        win_draw(interfaceWindow);
    }
    refresh_box_bar_win();
}

// Render hit points.
//
// 0x45EBD8
void intface_update_hit_points(bool animate)
{
    // Last hit points rendered in interface.
    //
    // Used to animate changes.
    //
    // 0x51902C
    static int last_points = 0;

    // Last color used to render hit points in interface.
    //
    // Used to animate changes.
    //
    // 0x519030
    static int last_points_color = INTERFACE_NUMBERS_COLOR_RED;

    if (interfaceWindow == -1) {
        return;
    }

    int hp = critter_get_hits(obj_dude);
    int maxHp = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);

    int red = (int)((double)maxHp * 0.25);
    int yellow = (int)((double)maxHp * 0.5);

    int color;
    if (hp < red) {
        color = INTERFACE_NUMBERS_COLOR_RED;
    } else if (hp < yellow) {
        color = INTERFACE_NUMBERS_COLOR_YELLOW;
    } else {
        color = INTERFACE_NUMBERS_COLOR_WHITE;
    }

    int v1[4];
    int v2[3];
    int count = 1;

    v1[0] = last_points;
    v2[0] = last_points_color;

    if (last_points_color != color) {
        if (hp >= last_points) {
            if (last_points < red && hp >= red) {
                v1[count] = red;
                v2[count] = INTERFACE_NUMBERS_COLOR_YELLOW;
                count += 1;
            }

            if (last_points < yellow && hp >= yellow) {
                v1[count] = yellow;
                v2[count] = INTERFACE_NUMBERS_COLOR_WHITE;
                count += 1;
            }
        } else {
            if (last_points >= yellow && hp < yellow) {
                v1[count] = yellow;
                v2[count] = INTERFACE_NUMBERS_COLOR_YELLOW;
                count += 1;
            }

            if (last_points >= red && hp < red) {
                v1[count] = red;
                v2[count] = INTERFACE_NUMBERS_COLOR_RED;
                count += 1;
            }
        }
    }

    v1[count] = hp;

    if (animate) {
        int delay = 250 / (abs(last_points - hp) + 1);
        for (int index = 0; index < count; index++) {
            intface_rotate_numbers(473, 40, v1[index], v1[index + 1], v2[index], delay);
        }
    } else {
        intface_rotate_numbers(473, 40, last_points, hp, color, 0);
    }

    last_points = hp;
    last_points_color = color;
}

// Render armor class.
//
// 0x45EDA8
void intface_update_ac(bool animate)
{
    // Last armor class rendered in interface.
    //
    // Used to animate changes.
    //
    // 0x519034
    static int last_ac = 0;

    int armorClass = critterGetStat(obj_dude, STAT_ARMOR_CLASS);

    int delay = 0;
    if (animate) {
        delay = 250 / (abs(last_ac - armorClass) + 1);
    }

    intface_rotate_numbers(473, 75, last_ac, armorClass, 0, delay);

    last_ac = armorClass;
}

// 0x45EE0C
void intface_update_move_points(int actionPointsLeft, int bonusActionPoints)
{
    unsigned char* frmData;

    if (interfaceWindow == -1) {
        return;
    }

    buf_to_buf(movePointBackground, 90, 5, 90, interfaceBuffer + 14 * 640 + 316, 640);

    if (actionPointsLeft == -1) {
        frmData = moveLightRed;
        actionPointsLeft = 10;
        bonusActionPoints = 0;
    } else {
        frmData = moveLightGreen;

        if (actionPointsLeft < 0) {
            actionPointsLeft = 0;
        }

        if (actionPointsLeft > 10) {
            actionPointsLeft = 10;
        }

        if (bonusActionPoints >= 0) {
            if (actionPointsLeft + bonusActionPoints > 10) {
                bonusActionPoints = 10 - actionPointsLeft;
            }
        } else {
            bonusActionPoints = 0;
        }
    }

    int index;
    for (index = 0; index < actionPointsLeft; index++) {
        buf_to_buf(frmData, 5, 5, 5, interfaceBuffer + 14 * 640 + 316 + index * 9, 640);
    }

    for (; index < (actionPointsLeft + bonusActionPoints); index++) {
        buf_to_buf(moveLightYellow, 5, 5, 5, interfaceBuffer + 14 * 640 + 316 + index * 9, 640);
    }

    if (!insideInit) {
        win_draw_rect(interfaceWindow, &movePointRect);
    }
}

// 0x45EF6C
int intface_get_attack(int* hitMode, bool* aiming)
{
    if (interfaceWindow == -1) {
        return -1;
    }

    *aiming = false;

    switch (itemButtonItems[itemCurrentItem].action) {
    case INTERFACE_ITEM_ACTION_PRIMARY_AIMING:
        *aiming = true;
        // FALLTHROUGH
    case INTERFACE_ITEM_ACTION_PRIMARY:
        *hitMode = itemButtonItems[itemCurrentItem].primaryHitMode;
        return 0;
    case INTERFACE_ITEM_ACTION_SECONDARY_AIMING:
        *aiming = true;
        // FALLTHROUGH
    case INTERFACE_ITEM_ACTION_SECONDARY:
        *hitMode = itemButtonItems[itemCurrentItem].secondaryHitMode;
        return 0;
    }

    return -1;
}

// 0x45EFEC
int intface_update_items(bool animated, int leftItemAction, int rightItemAction)
{
    if (map_bk_processes_are_disabled()) {
        animated = false;
    }

    if (interfaceWindow == -1) {
        return -1;
    }

    Object* oldCurrentItem = itemButtonItems[itemCurrentItem].item;

    InterfaceItemState* leftItemState = &(itemButtonItems[HAND_LEFT]);
    Object* item1 = inven_left_hand(obj_dude);
    if (item1 == leftItemState->item && leftItemState->item != NULL) {
        if (leftItemState->item != NULL) {
            leftItemState->isDisabled = item_grey(item1);
            leftItemState->itemFid = item_inv_fid(item1);
        }
    } else {
        leftItemState->item = item1;
        if (item1 != NULL) {
            leftItemState->isDisabled = item_grey(item1);
            leftItemState->primaryHitMode = HIT_MODE_LEFT_WEAPON_PRIMARY;
            leftItemState->secondaryHitMode = HIT_MODE_LEFT_WEAPON_SECONDARY;
            leftItemState->isWeapon = item_get_type(item1) == ITEM_TYPE_WEAPON;

            if (leftItemAction == INTERFACE_ITEM_ACTION_DEFAULT) {
                if (leftItemState->isWeapon != 0) {
                    leftItemState->action = INTERFACE_ITEM_ACTION_PRIMARY;
                } else {
                    leftItemState->action = INTERFACE_ITEM_ACTION_USE;
                }
            } else {
                leftItemState->action = leftItemAction;
            }

            leftItemState->itemFid = item_inv_fid(item1);
        } else {
            leftItemState->isDisabled = 0;
            leftItemState->isWeapon = 1;
            leftItemState->action = INTERFACE_ITEM_ACTION_PRIMARY;
            leftItemState->itemFid = -1;

            int unarmed = skill_level(obj_dude, SKILL_UNARMED);
            int agility = critterGetStat(obj_dude, STAT_AGILITY);
            int strength = critterGetStat(obj_dude, STAT_STRENGTH);
            int level = stat_pc_get(PC_STAT_LEVEL);

            if (unarmed > 99 && agility > 6 && strength > 4 && level > 8) {
                leftItemState->primaryHitMode = HIT_MODE_HAYMAKER;
            } else if (unarmed > 74 && agility > 5 && strength > 4 && level > 5) {
                leftItemState->primaryHitMode = HIT_MODE_HAMMER_PUNCH;
            } else if (unarmed > 54 && agility > 5) {
                leftItemState->primaryHitMode = HIT_MODE_STRONG_PUNCH;
            } else {
                leftItemState->primaryHitMode = HIT_MODE_PUNCH;
            }

            if (unarmed > 129 && agility > 6 && strength > 4 && level > 15) {
                leftItemState->secondaryHitMode = HIT_MODE_PIERCING_STRIKE;
            } else if (unarmed > 114 && agility > 6 && strength > 4 && level > 11) {
                leftItemState->secondaryHitMode = HIT_MODE_PALM_STRIKE;
            } else if (unarmed > 74 && agility > 6 && strength > 4 && level > 4) {
                leftItemState->secondaryHitMode = HIT_MODE_JAB;
            } else {
                leftItemState->secondaryHitMode = HIT_MODE_PUNCH;
            }
        }
    }

    InterfaceItemState* rightItemState = &(itemButtonItems[HAND_RIGHT]);

    Object* item2 = inven_right_hand(obj_dude);
    if (item2 == rightItemState->item && rightItemState->item != NULL) {
        if (rightItemState->item != NULL) {
            rightItemState->isDisabled = item_grey(rightItemState->item);
            rightItemState->itemFid = item_inv_fid(rightItemState->item);
        }
    } else {
        rightItemState->item = item2;

        if (item2 != NULL) {
            rightItemState->isDisabled = item_grey(item2);
            rightItemState->primaryHitMode = HIT_MODE_RIGHT_WEAPON_PRIMARY;
            rightItemState->secondaryHitMode = HIT_MODE_RIGHT_WEAPON_SECONDARY;
            rightItemState->isWeapon = item_get_type(item2) == ITEM_TYPE_WEAPON;

            if (rightItemAction == INTERFACE_ITEM_ACTION_DEFAULT) {
                if (rightItemState->isWeapon != 0) {
                    rightItemState->action = INTERFACE_ITEM_ACTION_PRIMARY;
                } else {
                    rightItemState->action = INTERFACE_ITEM_ACTION_USE;
                }
            } else {
                rightItemState->action = rightItemAction;
            }
            rightItemState->itemFid = item_inv_fid(item2);
        } else {
            rightItemState->isDisabled = 0;
            rightItemState->isWeapon = 1;
            rightItemState->action = INTERFACE_ITEM_ACTION_PRIMARY;
            rightItemState->itemFid = -1;

            int unarmed = skill_level(obj_dude, SKILL_UNARMED);
            int agility = critterGetStat(obj_dude, STAT_AGILITY);
            int strength = critterGetStat(obj_dude, STAT_STRENGTH);
            int level = stat_pc_get(PC_STAT_LEVEL);

            if (unarmed > 79 && agility > 5 && strength > 5 && level > 8) {
                rightItemState->primaryHitMode = HIT_MODE_POWER_KICK;
            } else if (unarmed > 59 && agility > 5 && level > 5) {
                rightItemState->primaryHitMode = HIT_MODE_SNAP_KICK;
            } else if (unarmed > 39 && agility > 5) {
                rightItemState->primaryHitMode = HIT_MODE_STRONG_KICK;
            } else {
                rightItemState->primaryHitMode = HIT_MODE_KICK;
            }

            if (unarmed > 124 && agility > 7 && strength > 5 && level > 14) {
                rightItemState->secondaryHitMode = HIT_MODE_PIERCING_KICK;
            } else if (unarmed > 99 && agility > 6 && strength > 5 && level > 11) {
                rightItemState->secondaryHitMode = HIT_MODE_HOOK_KICK;
            } else if (unarmed > 59 && agility > 6 && strength > 5 && level > 5) {
                rightItemState->secondaryHitMode = HIT_MODE_HIP_KICK;
            } else {
                rightItemState->secondaryHitMode = HIT_MODE_KICK;
            }
        }
    }

    if (animated) {
        Object* newCurrentItem = itemButtonItems[itemCurrentItem].item;
        if (newCurrentItem != oldCurrentItem) {
            int animationCode = 0;
            if (newCurrentItem != NULL) {
                if (item_get_type(newCurrentItem) == ITEM_TYPE_WEAPON) {
                    animationCode = item_w_anim_code(newCurrentItem);
                }
            }

            intface_change_fid_animate((obj_dude->fid & 0xF000) >> 12, animationCode);

            return 0;
        }
    }

    intface_redraw_items();

    return 0;
}

// 0x45F404
int intface_toggle_items(bool animated)
{
    if (interfaceWindow == -1) {
        return -1;
    }

    itemCurrentItem = 1 - itemCurrentItem;

    if (animated) {
        Object* item = itemButtonItems[itemCurrentItem].item;
        int animationCode = 0;
        if (item != NULL) {
            if (item_get_type(item) == ITEM_TYPE_WEAPON) {
                animationCode = item_w_anim_code(item);
            }
        }

        intface_change_fid_animate((obj_dude->fid & 0xF000) >> 12, animationCode);
    } else {
        intface_redraw_items();
    }

    int mode = gmouse_3d_get_mode();
    if (mode == GAME_MOUSE_MODE_CROSSHAIR || mode == GAME_MOUSE_MODE_USE_CROSSHAIR) {
        gmouse_3d_set_mode(GAME_MOUSE_MODE_MOVE);
    }

    return 0;
}

// 0x45F4B4
int intface_get_item_states(int* leftItemAction, int* rightItemAction)
{
    *leftItemAction = itemButtonItems[HAND_LEFT].action;
    *rightItemAction = itemButtonItems[HAND_RIGHT].action;
    return 0;
}

// 0x45F4E0
int intface_toggle_item_state()
{
    if (interfaceWindow == -1) {
        return -1;
    }

    InterfaceItemState* itemState = &(itemButtonItems[itemCurrentItem]);

    int oldAction = itemState->action;
    if (itemState->isWeapon != 0) {
        bool done = false;
        while (!done) {
            itemState->action++;
            switch (itemState->action) {
            case INTERFACE_ITEM_ACTION_PRIMARY:
                done = true;
                break;
            case INTERFACE_ITEM_ACTION_PRIMARY_AIMING:
                if (item_w_called_shot(obj_dude, itemState->primaryHitMode)) {
                    done = true;
                }
                break;
            case INTERFACE_ITEM_ACTION_SECONDARY:
                if (itemState->secondaryHitMode != HIT_MODE_PUNCH
                    && itemState->secondaryHitMode != HIT_MODE_KICK
                    && item_w_subtype(itemState->item, itemState->secondaryHitMode) != ATTACK_TYPE_NONE) {
                    done = true;
                }
                break;
            case INTERFACE_ITEM_ACTION_SECONDARY_AIMING:
                if (itemState->secondaryHitMode != HIT_MODE_PUNCH
                    && itemState->secondaryHitMode != HIT_MODE_KICK
                    && item_w_subtype(itemState->item, itemState->secondaryHitMode) != ATTACK_TYPE_NONE
                    && item_w_called_shot(obj_dude, itemState->secondaryHitMode)) {
                    done = true;
                }
                break;
            case INTERFACE_ITEM_ACTION_RELOAD:
                if (item_w_max_ammo(itemState->item) != item_w_curr_ammo(itemState->item)) {
                    done = true;
                }
                break;
            case INTERFACE_ITEM_ACTION_COUNT:
                itemState->action = INTERFACE_ITEM_ACTION_USE;
                break;
            }
        }
    }

    if (oldAction != itemState->action) {
        intface_redraw_items();
    }

    return 0;
}

// 0x45F5EC
void intface_use_item()
{
    if (interfaceWindow == -1) {
        return;
    }

    InterfaceItemState* ptr = &(itemButtonItems[itemCurrentItem]);

    if (ptr->isWeapon != 0) {
        if (ptr->action == INTERFACE_ITEM_ACTION_RELOAD) {
            if (isInCombat()) {
                int hitMode = itemCurrentItem == HAND_LEFT
                    ? HIT_MODE_LEFT_WEAPON_RELOAD
                    : HIT_MODE_RIGHT_WEAPON_RELOAD;

                int actionPointsRequired = item_mp_cost(obj_dude, hitMode, false);
                if (actionPointsRequired <= obj_dude->data.critter.combat.ap) {
                    if (intface_item_reload() == 0) {
                        if (actionPointsRequired > obj_dude->data.critter.combat.ap) {
                            obj_dude->data.critter.combat.ap = 0;
                        } else {
                            obj_dude->data.critter.combat.ap -= actionPointsRequired;
                        }
                        intface_update_move_points(obj_dude->data.critter.combat.ap, combat_free_move);
                    }
                }
            } else {
                intface_item_reload();
            }
        } else {
            gmouse_set_cursor(MOUSE_CURSOR_CROSSHAIR);
            gmouse_3d_set_mode(GAME_MOUSE_MODE_CROSSHAIR);
            if (!isInCombat()) {
                combat(NULL);
            }
        }
    } else if (proto_action_can_use_on(ptr->item->pid)) {
        gmouse_set_cursor(MOUSE_CURSOR_USE_CROSSHAIR);
        gmouse_3d_set_mode(GAME_MOUSE_MODE_USE_CROSSHAIR);
    } else if (obj_action_can_use(ptr->item)) {
        if (isInCombat()) {
            int actionPointsRequired = item_mp_cost(obj_dude, ptr->secondaryHitMode, false);
            if (actionPointsRequired <= obj_dude->data.critter.combat.ap) {
                obj_use_item(obj_dude, ptr->item);
                intface_update_items(false, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);
                if (actionPointsRequired > obj_dude->data.critter.combat.ap) {
                    obj_dude->data.critter.combat.ap = 0;
                } else {
                    obj_dude->data.critter.combat.ap -= actionPointsRequired;
                }

                intface_update_move_points(obj_dude->data.critter.combat.ap, combat_free_move);
            }
        } else {
            obj_use_item(obj_dude, ptr->item);
            intface_update_items(false, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);
        }
    }
}

// 0x45F7FC
int intface_is_item_right_hand()
{
    return itemCurrentItem;
}

// 0x45F804
int intface_get_current_item(Object** itemPtr)
{
    if (interfaceWindow == -1) {
        return -1;
    }

    *itemPtr = itemButtonItems[itemCurrentItem].item;

    return 0;
}

// 0x45F838
int intface_update_ammo_lights()
{
    if (interfaceWindow == -1) {
        return -1;
    }

    InterfaceItemState* p = &(itemButtonItems[itemCurrentItem]);

    int ratio = 0;

    if (p->isWeapon != 0) {
        // calls sub_478674 twice, probably because if min/max kind macro
        int maximum = item_w_max_ammo(p->item);
        if (maximum > 0) {
            int current = item_w_curr_ammo(p->item);
            ratio = (int)((double)current / (double)maximum * 70.0);
        }
    } else {
        if (item_get_type(p->item) == ITEM_TYPE_MISC) {
            // calls sub_4793D0 twice, probably because if min/max kind macro
            int maximum = item_m_max_charges(p->item);
            if (maximum > 0) {
                int current = item_m_curr_charges(p->item);
                ratio = (int)((double)current / (double)maximum * 70.0);
            }
        }
    }

    intface_draw_ammo_lights(463, ratio);

    return 0;
}

// 0x45F96C
void intface_end_window_open(bool animated)
{
    if (interfaceWindow == -1) {
        return;
    }

    if (endWindowOpen) {
        return;
    }

    int fid = art_id(OBJ_TYPE_INTERFACE, 104, 0, 0, 0);
    CacheEntry* handle;
    Art* art = art_ptr_lock(fid, &handle);
    if (art == NULL) {
        return;
    }

    int frameCount = art_frame_max_frame(art);
    gsound_play_sfx_file("iciboxx1");

    if (animated) {
        unsigned int delay = 1000 / art_frame_fps(art);
        int time = 0;
        int frame = 0;
        while (frame < frameCount) {
            if (elapsed_time(time) >= delay) {
                unsigned char* src = art_frame_data(art, frame, 0);
                if (src != NULL) {
                    buf_to_buf(src, 57, 58, 57, interfaceBuffer + 640 * 38 + 580, 640);
                    win_draw_rect(interfaceWindow, &endWindowRect);
                }

                time = get_time();
                frame++;
            }
            gmouse_bk_process();
        }
    } else {
        unsigned char* src = art_frame_data(art, frameCount - 1, 0);
        buf_to_buf(src, 57, 58, 57, interfaceBuffer + 640 * 38 + 580, 640);
        win_draw_rect(interfaceWindow, &endWindowRect);
    }

    art_ptr_unlock(handle);

    endWindowOpen = true;
    intface_create_end_turn_button();
    intface_create_end_combat_button();
    intface_end_buttons_disable();
}

// 0x45FAC0
void intface_end_window_close(bool animated)
{
    if (interfaceWindow == -1) {
        return;
    }

    if (!endWindowOpen) {
        return;
    }

    int fid = art_id(OBJ_TYPE_INTERFACE, 104, 0, 0, 0);
    CacheEntry* handle;
    Art* art = art_ptr_lock(fid, &handle);
    if (art == NULL) {
        return;
    }

    intface_destroy_end_turn_button();
    intface_destroy_end_combat_button();
    gsound_play_sfx_file("icibcxx1");

    if (animated) {
        unsigned int delay = 1000 / art_frame_fps(art);
        unsigned int time = 0;
        int frame = art_frame_max_frame(art);

        while (frame != 0) {
            if (elapsed_time(time) >= delay) {
                unsigned char* src = art_frame_data(art, frame - 1, 0);
                unsigned char* dest = interfaceBuffer + 640 * 38 + 580;
                if (src != NULL) {
                    buf_to_buf(src, 57, 58, 57, dest, 640);
                    win_draw_rect(interfaceWindow, &endWindowRect);
                }

                time = get_time();
                frame--;
            }
            gmouse_bk_process();
        }
    } else {
        unsigned char* dest = interfaceBuffer + 640 * 38 + 580;
        unsigned char* src = art_frame_data(art, 0, 0);
        buf_to_buf(src, 57, 58, 57, dest, 640);
        win_draw_rect(interfaceWindow, &endWindowRect);
    }

    art_ptr_unlock(handle);
    endWindowOpen = false;
}

// 0x45FC04
void intface_end_buttons_enable()
{
    if (endWindowOpen) {
        win_enable_button(endTurnButton);
        win_enable_button(endCombatButton);

        // endltgrn.frm - green lights around end turn/combat window
        int lightsFid = art_id(OBJ_TYPE_INTERFACE, 109, 0, 0, 0);
        CacheEntry* lightsFrmHandle;
        unsigned char* lightsFrmData = art_ptr_lock_data(lightsFid, 0, 0, &lightsFrmHandle);
        if (lightsFrmData == NULL) {
            return;
        }

        gsound_play_sfx_file("icombat2");
        trans_buf_to_buf(lightsFrmData, 57, 58, 57, interfaceBuffer + 38 * 640 + 580, 640);
        win_draw_rect(interfaceWindow, &endWindowRect);

        art_ptr_unlock(lightsFrmHandle);
    }
}

// 0x45FC98
void intface_end_buttons_disable()
{
    if (endWindowOpen) {
        win_disable_button(endTurnButton);
        win_disable_button(endCombatButton);

        CacheEntry* lightsFrmHandle;
        // endltred.frm - red lights around end turn/combat window
        int lightsFid = art_id(OBJ_TYPE_INTERFACE, 110, 0, 0, 0);
        unsigned char* lightsFrmData = art_ptr_lock_data(lightsFid, 0, 0, &lightsFrmHandle);
        if (lightsFrmData == NULL) {
            return;
        }

        gsound_play_sfx_file("icombat1");
        trans_buf_to_buf(lightsFrmData, 57, 58, 57, interfaceBuffer + 38 * 640 + 580, 640);
        win_draw_rect(interfaceWindow, &endWindowRect);

        art_ptr_unlock(lightsFrmHandle);
    }
}

// NOTE: Inlined.
//
// 0x45FD2C
static int intface_init_items()
{
    // FIXME: For unknown reason these values initialized with -1. It's never
    // checked for -1, so I have no explanation for this.
    itemButtonItems[HAND_LEFT].item = (Object*)-1;
    itemButtonItems[HAND_RIGHT].item = (Object*)-1;

    return 0;
}

// 0x45FD88
static int intface_redraw_items()
{
    if (interfaceWindow == -1) {
        return -1;
    }

    win_enable_button(itemButton);

    InterfaceItemState* itemState = &(itemButtonItems[itemCurrentItem]);
    int actionPoints = -1;

    if (itemState->isDisabled == 0) {
        memcpy(itemButtonUp, itemButtonUpBlank, sizeof(itemButtonUp));
        memcpy(itemButtonDown, itemButtonDownBlank, sizeof(itemButtonDown));

        if (itemState->isWeapon == 0) {
            int fid;
            if (proto_action_can_use_on(itemState->item->pid)) {
                // USE ON
                fid = art_id(OBJ_TYPE_INTERFACE, 294, 0, 0, 0);
            } else if (obj_action_can_use(itemState->item)) {
                // USE
                fid = art_id(OBJ_TYPE_INTERFACE, 292, 0, 0, 0);
            } else {
                fid = -1;
            }

            if (fid != -1) {
                CacheEntry* useTextFrmHandle;
                Art* useTextFrm = art_ptr_lock(fid, &useTextFrmHandle);
                if (useTextFrm != NULL) {
                    int width = art_frame_width(useTextFrm, 0, 0);
                    int height = art_frame_length(useTextFrm, 0, 0);
                    unsigned char* data = art_frame_data(useTextFrm, 0, 0);
                    trans_buf_to_buf(data, width, height, width, itemButtonUp + 188 * 7 + 181 - width, 188);
                    dark_trans_buf_to_buf(data, width, height, width, itemButtonDown, 181 - width + 1, 5, 188, 59641);
                    art_ptr_unlock(useTextFrmHandle);
                }

                actionPoints = item_mp_cost(obj_dude, itemState->primaryHitMode, false);
            }
        } else {
            int primaryFid = -1;
            int bullseyeFid = -1;
            int hitMode = -1;

            // NOTE: This value is decremented at 0x45FEAC, probably to build
            // jump table.
            switch (itemState->action) {
            case INTERFACE_ITEM_ACTION_PRIMARY_AIMING:
                bullseyeFid = art_id(OBJ_TYPE_INTERFACE, 288, 0, 0, 0);
                // FALLTHROUGH
            case INTERFACE_ITEM_ACTION_PRIMARY:
                hitMode = itemState->primaryHitMode;
                break;
            case INTERFACE_ITEM_ACTION_SECONDARY_AIMING:
                bullseyeFid = art_id(OBJ_TYPE_INTERFACE, 288, 0, 0, 0);
                // FALLTHROUGH
            case INTERFACE_ITEM_ACTION_SECONDARY:
                hitMode = itemState->secondaryHitMode;
                break;
            case INTERFACE_ITEM_ACTION_RELOAD:
                actionPoints = item_mp_cost(obj_dude, itemCurrentItem == HAND_LEFT ? HIT_MODE_LEFT_WEAPON_RELOAD : HIT_MODE_RIGHT_WEAPON_RELOAD, false);
                primaryFid = art_id(OBJ_TYPE_INTERFACE, 291, 0, 0, 0);
                break;
            }

            if (bullseyeFid != -1) {
                CacheEntry* bullseyeFrmHandle;
                Art* bullseyeFrm = art_ptr_lock(bullseyeFid, &bullseyeFrmHandle);
                if (bullseyeFrm != NULL) {
                    int width = art_frame_width(bullseyeFrm, 0, 0);
                    int height = art_frame_length(bullseyeFrm, 0, 0);
                    unsigned char* data = art_frame_data(bullseyeFrm, 0, 0);
                    trans_buf_to_buf(data, width, height, width, itemButtonUp + 188 * (60 - height) + (181 - width), 188);

                    int v9 = 60 - height - 2;
                    if (v9 < 0) {
                        v9 = 0;
                        height -= 2;
                    }

                    dark_trans_buf_to_buf(data, width, height, width, itemButtonDown, 181 - width + 1, v9, 188, 59641);
                    art_ptr_unlock(bullseyeFrmHandle);
                }
            }

            if (hitMode != -1) {
                actionPoints = item_w_mp_cost(obj_dude, hitMode, bullseyeFid != -1);

                int id;
                int anim = item_w_anim(obj_dude, hitMode);
                switch (anim) {
                case ANIM_THROW_PUNCH:
                    switch (hitMode) {
                    case HIT_MODE_STRONG_PUNCH:
                        id = 432; // strong punch
                        break;
                    case HIT_MODE_HAMMER_PUNCH:
                        id = 425; // hammer punch
                        break;
                    case HIT_MODE_HAYMAKER:
                        id = 428; // lightning punch
                        break;
                    case HIT_MODE_JAB:
                        id = 421; // chop punch
                        break;
                    case HIT_MODE_PALM_STRIKE:
                        id = 423; // dragon punch
                        break;
                    case HIT_MODE_PIERCING_STRIKE:
                        id = 424; // force punch
                        break;
                    default:
                        id = 42; // punch
                        break;
                    }
                    break;
                case ANIM_KICK_LEG:
                    switch (hitMode) {
                    case HIT_MODE_STRONG_KICK:
                        id = 430; // skick.frm - strong kick text
                        break;
                    case HIT_MODE_SNAP_KICK:
                        id = 431; // snapkick.frm - snap kick text
                        break;
                    case HIT_MODE_POWER_KICK:
                        id = 429; // cm_pwkck.frm - roundhouse kick text
                        break;
                    case HIT_MODE_HIP_KICK:
                        id = 426; // hipk.frm - kip kick text
                        break;
                    case HIT_MODE_HOOK_KICK:
                        id = 427; // cm_hookk.frm - jump kick text
                        break;
                    case HIT_MODE_PIERCING_KICK: // cm_prckk.frm - death blossom kick text
                        id = 422;
                        break;
                    default:
                        id = 41; // kick.frm - kick text
                        break;
                    }
                    break;
                case ANIM_THROW_ANIM:
                    id = 117; // throw
                    break;
                case ANIM_THRUST_ANIM:
                    id = 45; // thrust
                    break;
                case ANIM_SWING_ANIM:
                    id = 44; // swing
                    break;
                case ANIM_FIRE_SINGLE:
                    id = 43; // single
                    break;
                case ANIM_FIRE_BURST:
                case ANIM_FIRE_CONTINUOUS:
                    id = 40; // burst
                    break;
                }

                primaryFid = art_id(OBJ_TYPE_INTERFACE, id, 0, 0, 0);
            }

            if (primaryFid != -1) {
                CacheEntry* primaryFrmHandle;
                Art* primaryFrm = art_ptr_lock(primaryFid, &primaryFrmHandle);
                if (primaryFrm != NULL) {
                    int width = art_frame_width(primaryFrm, 0, 0);
                    int height = art_frame_length(primaryFrm, 0, 0);
                    unsigned char* data = art_frame_data(primaryFrm, 0, 0);
                    trans_buf_to_buf(data, width, height, width, itemButtonUp + 188 * 7 + 181 - width, 188);
                    dark_trans_buf_to_buf(data, width, height, width, itemButtonDown, 181 - width + 1, 5, 188, 59641);
                    art_ptr_unlock(primaryFrmHandle);
                }
            }
        }
    }

    if (actionPoints >= 0 && actionPoints < 10) {
        // movement point text
        int fid = art_id(OBJ_TYPE_INTERFACE, 289, 0, 0, 0);

        CacheEntry* handle;
        Art* art = art_ptr_lock(fid, &handle);
        if (art != NULL) {
            int width = art_frame_width(art, 0, 0);
            int height = art_frame_length(art, 0, 0);
            unsigned char* data = art_frame_data(art, 0, 0);

            trans_buf_to_buf(data, width, height, width, itemButtonUp + 188 * (60 - height) + 7, 188);

            int v29 = 60 - height - 2;
            if (v29 < 0) {
                v29 = 0;
                height -= 2;
            }

            dark_trans_buf_to_buf(data, width, height, width, itemButtonDown, 7 + 1, v29, 188, 59641);
            art_ptr_unlock(handle);

            int offset = width + 7;

            // movement point numbers - ten numbers 0 to 9, each 10 pixels wide.
            fid = art_id(OBJ_TYPE_INTERFACE, 290, 0, 0, 0);
            art = art_ptr_lock(fid, &handle);
            if (art != NULL) {
                width = art_frame_width(art, 0, 0);
                height = art_frame_length(art, 0, 0);
                data = art_frame_data(art, 0, 0);

                trans_buf_to_buf(data + actionPoints * 10, 10, height, width, itemButtonUp + 188 * (60 - height) + 7 + offset, 188);

                int v40 = 60 - height - 2;
                if (v40 < 0) {
                    v40 = 0;
                    height -= 2;
                }
                dark_trans_buf_to_buf(data + actionPoints * 10, 10, height, width, itemButtonDown, offset + 7 + 1, v40, 188, 59641);

                art_ptr_unlock(handle);
            }
        }
    } else {
        memcpy(itemButtonUp, itemButtonDisabled, sizeof(itemButtonUp));
        memcpy(itemButtonDown, itemButtonDisabled, sizeof(itemButtonDown));
    }

    if (itemState->itemFid != -1) {
        CacheEntry* itemFrmHandle;
        Art* itemFrm = art_ptr_lock(itemState->itemFid, &itemFrmHandle);
        if (itemFrm != NULL) {
            int width = art_frame_width(itemFrm, 0, 0);
            int height = art_frame_length(itemFrm, 0, 0);
            unsigned char* data = art_frame_data(itemFrm, 0, 0);

            int v46 = (188 - width) / 2;
            int v47 = (67 - height) / 2 - 2;

            trans_buf_to_buf(data, width, height, width, itemButtonUp + 188 * ((67 - height) / 2) + v46, 188);

            if (v47 < 0) {
                v47 = 0;
                height -= 2;
            }

            dark_trans_buf_to_buf(data, width, height, width, itemButtonDown, v46 + 1, v47, 188, 63571);
            art_ptr_unlock(itemFrmHandle);
        }
    }

    if (!insideInit) {
        intface_update_ammo_lights();

        win_draw_rect(interfaceWindow, &itemButtonRect);

        if (itemState->isDisabled != 0) {
            win_disable_button(itemButton);
        } else {
            win_enable_button(itemButton);
        }
    }

    return 0;
}

// 0x460658
static int intface_redraw_items_callback(Object* a1, Object* a2)
{
    intface_redraw_items();
    return 0;
}

// 0x460660
static int intface_change_fid_callback(Object* a1, Object* a2)
{
    intface_fid_is_changing = false;
    return 0;
}

// 0x46066C
static void intface_change_fid_animate(int previousWeaponAnimationCode, int weaponAnimationCode)
{
    intface_fid_is_changing = true;

    register_clear(obj_dude);
    register_begin(ANIMATION_REQUEST_RESERVED);
    register_object_light(obj_dude, 4, 0);

    if (previousWeaponAnimationCode != 0) {
        const char* sfx = gsnd_build_character_sfx_name(obj_dude, ANIM_PUT_AWAY, CHARACTER_SOUND_EFFECT_UNUSED);
        register_object_play_sfx(obj_dude, sfx, 0);
        register_object_animate(obj_dude, ANIM_PUT_AWAY, 0);
    }

    register_object_must_call(NULL, NULL, intface_redraw_items_callback, -1);

    Object* item = itemButtonItems[itemCurrentItem].item;
    if (item != NULL && item->lightDistance > 4) {
        register_object_light(obj_dude, item->lightDistance, 0);
    }

    if (weaponAnimationCode != 0) {
        register_object_take_out(obj_dude, weaponAnimationCode, -1);
    } else {
        int fid = art_id(OBJ_TYPE_CRITTER, obj_dude->fid & 0xFFF, ANIM_STAND, 0, obj_dude->rotation + 1);
        register_object_change_fid(obj_dude, fid, -1);
    }

    register_object_must_call(NULL, NULL, intface_change_fid_callback, -1);

    if (register_end() == -1) {
        return;
    }

    bool interfaceBarWasEnabled = intfaceEnabled;

    intface_disable();
    gmouse_disable(0);

    gmouse_set_cursor(MOUSE_CURSOR_WAIT_WATCH);

    while (intface_fid_is_changing) {
        if (game_user_wants_to_quit) {
            break;
        }

        get_input();
    }

    gmouse_set_cursor(MOUSE_CURSOR_NONE);

    gmouse_enable();

    if (interfaceBarWasEnabled) {
        intface_enable();
    }
}

// 0x4607E0
static int intface_create_end_turn_button()
{
    int fid;

    if (interfaceWindow == -1) {
        return -1;
    }

    if (!endWindowOpen) {
        return -1;
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 105, 0, 0, 0);
    endTurnButtonUp = art_ptr_lock_data(fid, 0, 0, &endTurnButtonUpKey);
    if (endTurnButtonUp == NULL) {
        return -1;
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 106, 0, 0, 0);
    endTurnButtonDown = art_ptr_lock_data(fid, 0, 0, &endTurnButtonDownKey);
    if (endTurnButtonDown == NULL) {
        return -1;
    }

    endTurnButton = win_register_button(interfaceWindow, 590, 43, 38, 22, -1, -1, -1, 32, endTurnButtonUp, endTurnButtonDown, NULL, 0);
    if (endTurnButton == -1) {
        return -1;
    }

    win_register_button_disable(endTurnButton, endTurnButtonUp, endTurnButtonUp, endTurnButtonUp);
    win_register_button_sound_func(endTurnButton, gsound_med_butt_press, gsound_med_butt_release);

    return 0;
}

// 0x4608C4
static int intface_destroy_end_turn_button()
{
    if (interfaceWindow == -1) {
        return -1;
    }

    if (endTurnButton != -1) {
        win_delete_button(endTurnButton);
        endTurnButton = -1;
    }

    if (endTurnButtonDown) {
        art_ptr_unlock(endTurnButtonDownKey);
        endTurnButtonDownKey = NULL;
        endTurnButtonDown = NULL;
    }

    if (endTurnButtonUp) {
        art_ptr_unlock(endTurnButtonUpKey);
        endTurnButtonUpKey = NULL;
        endTurnButtonUp = NULL;
    }

    return 0;
}

// 0x460940
static int intface_create_end_combat_button()
{
    int fid;

    if (interfaceWindow == -1) {
        return -1;
    }

    if (!endWindowOpen) {
        return -1;
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 107, 0, 0, 0);
    endCombatButtonUp = art_ptr_lock_data(fid, 0, 0, &endCombatButtonUpKey);
    if (endCombatButtonUp == NULL) {
        return -1;
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 108, 0, 0, 0);
    endCombatButtonDown = art_ptr_lock_data(fid, 0, 0, &endCombatButtonDownKey);
    if (endCombatButtonDown == NULL) {
        return -1;
    }

    endCombatButton = win_register_button(interfaceWindow, 590, 65, 38, 22, -1, -1, -1, 13, endCombatButtonUp, endCombatButtonDown, NULL, 0);
    if (endCombatButton == -1) {
        return -1;
    }

    win_register_button_disable(endCombatButton, endCombatButtonUp, endCombatButtonUp, endCombatButtonUp);
    win_register_button_sound_func(endCombatButton, gsound_med_butt_press, gsound_med_butt_release);

    return 0;
}

// 0x460A24
static int intface_destroy_end_combat_button()
{
    if (interfaceWindow == -1) {
        return -1;
    }

    if (endCombatButton != -1) {
        win_delete_button(endCombatButton);
        endCombatButton = -1;
    }

    if (endCombatButtonDown != NULL) {
        art_ptr_unlock(endCombatButtonDownKey);
        endCombatButtonDownKey = NULL;
        endCombatButtonDown = NULL;
    }

    if (endCombatButtonUp != NULL) {
        art_ptr_unlock(endCombatButtonUpKey);
        endCombatButtonUpKey = NULL;
        endCombatButtonUp = NULL;
    }

    return 0;
}

// 0x460AA0
static void intface_draw_ammo_lights(int x, int ratio)
{
    if ((ratio & 1) != 0) {
        ratio -= 1;
    }

    unsigned char* dest = interfaceBuffer + 640 * 26 + x;

    for (int index = 70; index > ratio; index--) {
        *dest = 14;
        dest += 640;
    }

    while (ratio > 0) {
        *dest = 196;
        dest += 640;

        *dest = 14;
        dest += 640;

        ratio -= 2;
    }

    if (!insideInit) {
        Rect rect;
        rect.ulx = x;
        rect.uly = 26;
        rect.lrx = x + 1;
        rect.lry = 26 + 70;
        win_draw_rect(interfaceWindow, &rect);
    }
}

// 0x460B20
static int intface_item_reload()
{
    if (interfaceWindow == -1) {
        return -1;
    }

    bool v0 = false;
    while (item_w_try_reload(obj_dude, itemButtonItems[itemCurrentItem].item) != -1) {
        v0 = true;
    }

    intface_toggle_item_state();
    intface_update_items(false, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);

    if (!v0) {
        return -1;
    }

    const char* sfx = gsnd_build_weapon_sfx_name(WEAPON_SOUND_EFFECT_READY, itemButtonItems[itemCurrentItem].item, HIT_MODE_RIGHT_WEAPON_PRIMARY, NULL);
    gsound_play_sfx_file(sfx);

    return 0;
}

// Renders hit points.
//
// [delay] is an animation delay.
// [previousValue] is only meaningful for animation.
// [offset] = 0 - grey, 120 - yellow, 240 - red.
//
// 0x460BA0
static void intface_rotate_numbers(int x, int y, int previousValue, int value, int offset, int delay)
{
    if (value > 999) {
        value = 999;
    } else if (value < -999) {
        value = -999;
    }

    unsigned char* numbers = numbersBuffer + offset;
    unsigned char* dest = interfaceBuffer + 640 * y;

    unsigned char* downSrc = numbers + 90;
    unsigned char* upSrc = numbers + 99;
    unsigned char* minusSrc = numbers + 108;
    unsigned char* plusSrc = numbers + 114;

    unsigned char* signDest = dest + x;
    unsigned char* hundredsDest = dest + x + 6;
    unsigned char* tensDest = dest + x + 6 + 9;
    unsigned char* onesDest = dest + x + 6 + 9 * 2;

    int normalizedSign;
    int normalizedValue;
    if (insideInit || delay == 0) {
        normalizedSign = value >= 0 ? 1 : -1;
        normalizedValue = abs(value);
    } else {
        normalizedSign = previousValue >= 0 ? 1 : -1;
        normalizedValue = previousValue;
    }

    int ones = normalizedValue % 10;
    int tens = (normalizedValue / 10) % 10;
    int hundreds = normalizedValue / 100;

    buf_to_buf(numbers + 9 * hundreds, 9, 17, 360, hundredsDest, 640);
    buf_to_buf(numbers + 9 * tens, 9, 17, 360, tensDest, 640);
    buf_to_buf(numbers + 9 * ones, 9, 17, 360, onesDest, 640);
    buf_to_buf(normalizedSign >= 0 ? plusSrc : minusSrc, 6, 17, 360, signDest, 640);

    if (!insideInit) {
        Rect numbersRect = { x, y, x + 33, y + 17 };
        win_draw_rect(interfaceWindow, &numbersRect);
        if (delay != 0) {
            int change = value - previousValue >= 0 ? 1 : -1;
            int v14 = previousValue >= 0 ? 1 : -1;
            int v49 = change * v14;
            while (previousValue != value) {
                if ((hundreds | tens | ones) == 0) {
                    v49 = 1;
                }

                buf_to_buf(upSrc, 9, 17, 360, onesDest, 640);
                mouse_info();
                gmouse_bk_process();
                block_for_tocks(delay);
                win_draw_rect(interfaceWindow, &numbersRect);

                ones += v49;

                if (ones > 9 || ones < 0) {
                    buf_to_buf(upSrc, 9, 17, 360, tensDest, 640);
                    mouse_info();
                    gmouse_bk_process();
                    block_for_tocks(delay);
                    win_draw_rect(interfaceWindow, &numbersRect);

                    tens += v49;
                    ones -= 10 * v49;
                    if (tens == 10 || tens == -1) {
                        buf_to_buf(upSrc, 9, 17, 360, hundredsDest, 640);
                        mouse_info();
                        gmouse_bk_process();
                        block_for_tocks(delay);
                        win_draw_rect(interfaceWindow, &numbersRect);

                        hundreds += v49;
                        tens -= 10 * v49;
                        if (hundreds == 10 || hundreds == -1) {
                            hundreds -= 10 * v49;
                        }

                        buf_to_buf(downSrc, 9, 17, 360, hundredsDest, 640);
                        mouse_info();
                        gmouse_bk_process();
                        block_for_tocks(delay);
                        win_draw_rect(interfaceWindow, &numbersRect);
                    }

                    buf_to_buf(downSrc, 9, 17, 360, tensDest, 640);
                    block_for_tocks(delay);
                    win_draw_rect(interfaceWindow, &numbersRect);
                }

                buf_to_buf(downSrc, 9, 17, 360, onesDest, 640);
                mouse_info();
                gmouse_bk_process();
                block_for_tocks(delay);
                win_draw_rect(interfaceWindow, &numbersRect);

                previousValue += change;

                buf_to_buf(numbers + 9 * hundreds, 9, 17, 360, hundredsDest, 640);
                buf_to_buf(numbers + 9 * tens, 9, 17, 360, tensDest, 640);
                buf_to_buf(numbers + 9 * ones, 9, 17, 360, onesDest, 640);

                buf_to_buf(previousValue >= 0 ? plusSrc : minusSrc, 6, 17, 360, signDest, 640);
                mouse_info();
                gmouse_bk_process();
                block_for_tocks(delay);
                win_draw_rect(interfaceWindow, &numbersRect);
            }
        }
    }
}

// NOTE: Inlined.
//
// 0x461128
static int intface_fatal_error(int rc)
{
    intface_exit();

    return rc;
}

// 0x461134
static int construct_box_bar_win()
{
    int oldFont = text_curr();

    if (bar_window != -1) {
        return 0;
    }

    MessageList messageList;
    MessageListItem messageListItem;
    int rc = 0;
    if (!message_init(&messageList)) {
        rc = -1;
    }

    char path[MAX_PATH];
    sprintf(path, "%s%s", msg_path, "intrface.msg");

    if (rc != -1) {
        if (!message_load(&messageList, path)) {
            rc = -1;
        }
    }

    if (rc == -1) {
        debug_printf("\nINTRFACE: Error indicator box messages! **\n");
        return -1;
    }

    CacheEntry* indicatorBoxFrmHandle;
    int width;
    int height;
    int indicatorBoxFid = art_id(OBJ_TYPE_INTERFACE, 126, 0, 0, 0);
    unsigned char* indicatorBoxFrmData = art_lock(indicatorBoxFid, &indicatorBoxFrmHandle, &width, &height);
    if (indicatorBoxFrmData == NULL) {
        debug_printf("\nINTRFACE: Error initializing indicator box graphics! **\n");
        message_exit(&messageList);
        return -1;
    }

    for (int index = 0; index < INDICATOR_COUNT; index++) {
        IndicatorDescription* indicatorDescription = &(bbox[index]);

        indicatorDescription->data = (unsigned char*)mem_malloc(INDICATOR_BOX_WIDTH * INDICATOR_BOX_HEIGHT);
        if (indicatorDescription->data == NULL) {
            debug_printf("\nINTRFACE: Error initializing indicator box graphics! **");

            while (--index >= 0) {
                mem_free(bbox[index].data);
            }

            message_exit(&messageList);
            art_ptr_unlock(indicatorBoxFrmHandle);

            return -1;
        }
    }

    text_font(101);

    for (int index = 0; index < INDICATOR_COUNT; index++) {
        IndicatorDescription* indicator = &(bbox[index]);

        char text[1024];
        strcpy(text, getmsg(&messageList, &messageListItem, indicator->title));

        int color = indicator->isBad ? colorTable[31744] : colorTable[992];

        memcpy(indicator->data, indicatorBoxFrmData, INDICATOR_BOX_WIDTH * INDICATOR_BOX_HEIGHT);

        // NOTE: For unknown reason it uses 24 as a height of the box to center
        // the title. One explanation is that these boxes were redesigned, but
        // this value was not changed. On the other hand 24 is
        // [INDICATOR_BOX_HEIGHT] + [INDICATOR_BOX_CONNECTOR_WIDTH]. Maybe just
        // a coincidence. I guess we'll never find out.
        int y = (24 - text_height()) / 2;
        int x = (INDICATOR_BOX_WIDTH - text_width(text)) / 2;
        text_to_buf(indicator->data + INDICATOR_BOX_WIDTH * y + x, text, INDICATOR_BOX_WIDTH, INDICATOR_BOX_WIDTH, color);
    }

    box_status_flag = true;
    refresh_box_bar_win();

    message_exit(&messageList);
    art_ptr_unlock(indicatorBoxFrmHandle);
    text_font(oldFont);

    return 0;
}

// 0x461454
static void deconstruct_box_bar_win()
{
    if (bar_window != -1) {
        win_delete(bar_window);
        bar_window = -1;
    }

    for (int index = 0; index < INDICATOR_COUNT; index++) {
        IndicatorDescription* indicatorBoxDescription = &(bbox[index]);
        if (indicatorBoxDescription->data != NULL) {
            mem_free(indicatorBoxDescription->data);
            indicatorBoxDescription->data = NULL;
        }
    }
}

// NOTE: Inlined.
//
// 0x4614A0
static void reset_box_bar_win()
{
    if (bar_window != -1) {
        win_delete(bar_window);
        bar_window = -1;
    }

    box_status_flag = true;
}

// Updates indicator bar.
//
// 0x4614CC
int refresh_box_bar_win()
{
    if (interfaceWindow != -1 && box_status_flag && !intfaceHidden) {
        for (int index = 0; index < INDICATOR_SLOTS_COUNT; index++) {
            bboxslot[index] = -1;
        }

        int count = 0;

        if (is_pc_flag(DUDE_STATE_SNEAKING)) {
            if (add_bar_box(INDICATOR_SNEAK)) {
                ++count;
            }
        }

        if (is_pc_flag(DUDE_STATE_LEVEL_UP_AVAILABLE)) {
            if (add_bar_box(INDICATOR_LEVEL)) {
                ++count;
            }
        }

        if (is_pc_flag(DUDE_STATE_ADDICTED)) {
            if (add_bar_box(INDICATOR_ADDICT)) {
                ++count;
            }
        }

        if (critter_get_poison(obj_dude) > POISON_INDICATOR_THRESHOLD) {
            if (add_bar_box(INDICATOR_POISONED)) {
                ++count;
            }
        }

        if (critter_get_rads(obj_dude) > RADATION_INDICATOR_THRESHOLD) {
            if (add_bar_box(INDICATOR_RADIATED)) {
                ++count;
            }
        }

        if (count > 1) {
            qsort(bboxslot, count, sizeof(*bboxslot), bbox_comp);
        }

        if (bar_window != -1) {
            win_delete(bar_window);
            bar_window = -1;
        }

        if (count != 0) {
            bar_window = win_add(INDICATOR_BAR_X,
                INDICATOR_BAR_Y,
                (INDICATOR_BOX_WIDTH - INDICATOR_BOX_CONNECTOR_WIDTH) * count,
                INDICATOR_BOX_HEIGHT,
                colorTable[0],
                0);
            draw_bboxes(count);
            win_draw(bar_window);
        }

        return count;
    }

    if (bar_window != -1) {
        win_delete(bar_window);
        bar_window = -1;
    }

    return 0;
}

// 0x461624
static int bbox_comp(const void* a, const void* b)
{
    int indicatorBox1 = *(int*)a;
    int indicatorBox2 = *(int*)b;

    if (indicatorBox1 == indicatorBox2) {
        return 0;
    } else if (indicatorBox1 < indicatorBox2) {
        return -1;
    } else {
        return 1;
    }
}

// Renders indicator boxes into the indicator bar window.
//
// 0x461648
static void draw_bboxes(int count)
{
    if (bar_window == -1) {
        return;
    }

    if (count == 0) {
        return;
    }

    int windowWidth = win_width(bar_window);
    unsigned char* windowBuffer = win_get_buf(bar_window);

    // The initial number of connections is 2 - one is first box to the screen
    // boundary, the other is female socket (initially empty). Every displayed
    // box adds one more connection (it is "plugged" into previous box and
    // exposes it's own empty female socket).
    int connections = 2;

    // The width of displayed indicator boxes as if there were no connections.
    int unconnectedIndicatorsWidth = 0;

    // The X offset to display next box.
    int x = 0;

    // The first box is connected to the screen boundary, so we have to clamp
    // male connectors on the left.
    int connectorWidthCompensation = INDICATOR_BOX_CONNECTOR_WIDTH;

    for (int index = 0; index < count; index++) {
        int indicator = bboxslot[index];
        IndicatorDescription* indicatorDescription = &(bbox[indicator]);

        trans_buf_to_buf(indicatorDescription->data + connectorWidthCompensation,
            INDICATOR_BOX_WIDTH - connectorWidthCompensation,
            INDICATOR_BOX_HEIGHT,
            INDICATOR_BOX_WIDTH,
            windowBuffer + x, windowWidth);

        connectorWidthCompensation = 0;

        unconnectedIndicatorsWidth += INDICATOR_BOX_WIDTH;
        x = unconnectedIndicatorsWidth - INDICATOR_BOX_CONNECTOR_WIDTH * connections;
        connections++;
    }
}

// Adds indicator to the indicator bar.
//
// Returns `true` if indicator was added, or `false` if there is no available
// space in the indicator bar.
//
// 0x4616F0
static bool add_bar_box(int indicator)
{
    for (int index = 0; index < INDICATOR_SLOTS_COUNT; index++) {
        if (bboxslot[index] == -1) {
            bboxslot[index] = indicator;
            return true;
        }
    }

    debug_printf("\nINTRFACE: no free bar box slots!\n");

    return false;
}

// 0x461740
bool enable_box_bar_win()
{
    bool oldIsVisible = box_status_flag;
    box_status_flag = true;

    refresh_box_bar_win();

    return oldIsVisible;
}

// 0x461760
bool disable_box_bar_win()
{
    bool oldIsVisible = box_status_flag;
    box_status_flag = false;

    refresh_box_bar_win();

    return oldIsVisible;
}
