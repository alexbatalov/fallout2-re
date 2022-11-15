#ifndef FALLOUT_GAME_INTFACE_H_
#define FALLOUT_GAME_INTFACE_H_

#include <stdbool.h>

#include "db.h"
#include "game/object_types.h"

#define INTERFACE_BAR_WIDTH 640
#define INTERFACE_BAR_HEIGHT 100

typedef enum Hand {
    // Item1 (Punch)
    HAND_LEFT,
    // Item2 (Kick)
    HAND_RIGHT,
    HAND_COUNT,
} Hand;

typedef enum InterfaceItemAction {
    INTERFACE_ITEM_ACTION_DEFAULT = -1,
    INTERFACE_ITEM_ACTION_USE,
    INTERFACE_ITEM_ACTION_PRIMARY,
    INTERFACE_ITEM_ACTION_PRIMARY_AIMING,
    INTERFACE_ITEM_ACTION_SECONDARY,
    INTERFACE_ITEM_ACTION_SECONDARY_AIMING,
    INTERFACE_ITEM_ACTION_RELOAD,
    INTERFACE_ITEM_ACTION_COUNT,
} InterfaceItemAction;

extern int interfaceWindow;
extern int bar_window;

int intface_init();
void intface_reset();
void intface_exit();
int intface_load(File* stream);
int intface_save(File* stream);
void intface_hide();
void intface_show();
int intface_is_hidden();
void intface_enable();
void intface_disable();
bool intface_is_enabled();
void intface_redraw();
void intface_update_hit_points(bool animate);
void intface_update_ac(bool animate);
void intface_update_move_points(int actionPointsLeft, int bonusActionPoints);
int intface_get_attack(int* hitMode, bool* aiming);
int intface_update_items(bool animated, int leftItemAction, int rightItemAction);
int intface_toggle_items(bool animated);
int intface_get_item_states(int* leftItemAction, int* rightItemAction);
int intface_toggle_item_state();
void intface_use_item();
int intface_is_item_right_hand();
int intface_get_current_item(Object** itemPtr);
int intface_update_ammo_lights();
void intface_end_window_open(bool animated);
void intface_end_window_close(bool animated);
void intface_end_buttons_enable();
void intface_end_buttons_disable();
int refresh_box_bar_win();
bool enable_box_bar_win();
bool disable_box_bar_win();

#endif /* FALLOUT_GAME_INTFACE_H_ */
