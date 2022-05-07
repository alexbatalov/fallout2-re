#ifndef INVENTORY_H
#define INVENTORY_H

#include "art.h"
#include "cache.h"
#include "message.h"
#include "obj_types.h"

#include <stdbool.h>

#define INVENTORY_NORMAL_WINDOW_PC_ROTATION_DELAY (1000U / ROTATION_COUNT)
#define OFF_59E7BC_COUNT 12

typedef enum InventoryArrowFrm {
    INVENTORY_ARROW_FRM_LEFT_ARROW_UP,
    INVENTORY_ARROW_FRM_LEFT_ARROW_DOWN,
    INVENTORY_ARROW_FRM_RIGHT_ARROW_UP,
    INVENTORY_ARROW_FRM_RIGHT_ARROW_DOWN,
    INVENTORY_ARROW_FRM_COUNT,
} InventoryArrowFrm;

typedef enum InventoryWindowCursor {
    INVENTORY_WINDOW_CURSOR_HAND,
    INVENTORY_WINDOW_CURSOR_ARROW,
    INVENTORY_WINDOW_CURSOR_PICK,
    INVENTORY_WINDOW_CURSOR_MENU,
    INVENTORY_WINDOW_CURSOR_BLANK,
    INVENTORY_WINDOW_CURSOR_COUNT,
} InventoryWindowCursor;

typedef enum InventoryWindowType {
    // Normal inventory window with quick character sheet.
    INVENTORY_WINDOW_TYPE_NORMAL,

    // Narrow inventory window with just an item scroller that's shown when
    // a "Use item on" is selected from context menu.
    INVENTORY_WINDOW_TYPE_USE_ITEM_ON,

    // Looting/strealing interface.
    INVENTORY_WINDOW_TYPE_LOOT,

    // Barter interface.
    INVENTORY_WINDOW_TYPE_TRADE,

    // Supplementary "Move items" window. Used to set quantity of items when
    // moving items between inventories.
    INVENTORY_WINDOW_TYPE_MOVE_ITEMS,

    // Supplementary "Set timer" window. Internally it's implemented as "Move
    // items" window but with timer overlay and slightly different adjustment
    // mechanics.
    INVENTORY_WINDOW_TYPE_SET_TIMER,

    INVENTORY_WINDOW_TYPE_COUNT,
} InventoryWindowType;

typedef struct InventoryWindowConfiguration {
    int field_0; // artId
    int width;
    int height;
    int x;
    int y;
} InventoryWindowDescription;

typedef struct InventoryCursorData {
    Art* frm;
    unsigned char* frmData;
    int width;
    int height;
    int offsetX;
    int offsetY;
    CacheEntry* frmHandle;
} InventoryCursorData;

typedef void InventoryPrintItemDescriptionHandler(char* string);

extern const int dword_46E6D0[7];
extern const int dword_46E6EC[7];
extern const int gInventoryArrowFrmIds[INVENTORY_ARROW_FRM_COUNT];

extern int gInventorySlotsCount;
extern Object* off_519058;
extern int dword_51905C;
extern bool dword_519060;
extern int dword_519064;
extern const InventoryWindowDescription gInventoryWindowDescriptions[INVENTORY_WINDOW_TYPE_COUNT];
extern bool dword_5190E0;
extern int gInventoryScrollUpButton;
extern int gInventoryScrollDownButton;
extern int gSecondaryInventoryScrollUpButton;
extern int gSecondaryInventoryScrollDownButton;
extern unsigned int gInventoryWindowDudeRotationTimestamp;
extern int gInventoryWindowDudeRotation;
extern const int gInventoryWindowCursorFrmIds[INVENTORY_WINDOW_CURSOR_COUNT];
extern Object* off_519110;
extern const int dword_519114[4];
extern const int dword_519124[3];
extern const int dword_519130[3];
extern const int dword_51913C[2];
extern const int dword_519144[4];
extern const int dword_519154[3];

extern CacheEntry* off_59E79C[8];
extern CacheEntry* off_59E7BC[OFF_59E7BC_COUNT];
extern int dword_59E7EC[10];
extern MessageList gInventoryMessageList;
extern Object* off_59E81C[10];
extern int dword_59E844[10];
extern Object* off_59E86C[10];
extern int dword_59E894;
extern int dword_59E898;
extern int dword_59E89C;
extern int dword_59E8A0;
extern Inventory* off_59E8A4;
extern InventoryCursorData gInventoryCursorData[INVENTORY_WINDOW_CURSOR_COUNT];
extern Object* off_59E934;
extern InventoryPrintItemDescriptionHandler* gInventoryPrintItemDescriptionHandler;
extern int dword_59E93C;
extern int gInventoryCursor;
extern Object* off_59E944;
extern int dword_59E948;
extern Inventory* off_59E94C;
extern bool dword_59E950;
extern Object* gInventoryArmor;
extern Object* gInventoryLeftHandItem;
extern int gInventoryWindowDudeFid;
extern Inventory* off_59E960;
extern int gInventoryWindow;
extern Object* gInventoryRightHandItem;
extern int dword_59E96C;
extern int gInventoryWindowMaxY;
extern int gInventoryWindowMaxX;
extern Inventory* off_59E978;
extern int dword_59E97C;

void _inven_reset_dude();
int inventoryMessageListInit();
int inventoryMessageListFree();
void inventoryOpen();
bool _setup_inventory(int inventoryWindowType);
void _exit_inventory(bool a1);
void _display_inventory(int a1, int a2, int inventoryWindowType);
void _display_target_inventory(int a1, int a2, Inventory* a3, int a4);
void _display_inventory_info(Object* item, int quantity, unsigned char* dest, int pitch, bool a5);
void _display_body(int fid, int inventoryWindowType);
int inventoryCommonInit();
void inventoryCommonFree();
void inventorySetCursor(int cursor);
void inventoryItemSlotOnMouseEnter(int btn, int keyCode);
void inventoryItemSlotOnMouseExit(int btn, int keyCode);
void _inven_update_lighting(Object* a1);
void _inven_pickup(int keyCode, int a2);
void _switch_hand(Object* a1, Object** a2, Object** a3, int a4);
void _adjust_ac(Object* critter, Object* oldArmor, Object* newArmor);
void _adjust_fid();
void inventoryOpenUseItemOn(Object* a1);
Object* critterGetItem2(Object* obj);
Object* critterGetItem1(Object* obj);
Object* critterGetArmor(Object* obj);
Object* objectGetCarriedObjectByPid(Object* obj, int pid);
int objectGetCarriedQuantityByPid(Object* obj, int pid);
void inventoryRenderSummary();
Object* _inven_find_type(Object* obj, int a2, int* inout_a3);
Object* _inven_find_id(Object* obj, int a2);
Object* _inven_index_ptr(Object* obj, int a2);
int _inven_wield(Object* a1, Object* a2, int a3);
int _invenWieldFunc(Object* a1, Object* a2, int a3, bool a4);
int _inven_unwield(Object* critter_obj, int a2);
int _invenUnwieldFunc(Object* obj, int a2, int a3);
int _inven_from_button(int a1, Object** a2, Object*** a3, Object** a4);
void inventoryRenderItemDescription(char* string);
void inventoryExamineItem(Object* critter, Object* item);
void inventoryWindowOpenContextMenu(int eventCode, int inventoryWindowType);
int inventoryOpenLooting(Object* a1, Object* a2);
int inventoryOpenStealing(Object* a1, Object* a2);
int _move_inventory(Object* a1, int a2, Object* a3, bool a4);
int _barter_compute_value(Object* a1, Object* a2);
int _barter_attempt_transaction(Object* a1, Object* a2, Object* a3, Object* a4);
void _barter_move_inventory(Object* a1, int quantity, int a3, int a4, Object* a5, Object* a6, bool a7);
void _barter_move_from_table_inventory(Object* a1, int quantity, int a3, Object* a4, Object* a5, bool a6);
void inventoryWindowRenderInnerInventories(int win, Object* a2, Object* a3, int a4);
void inventoryOpenTrade(int win, Object* a2, Object* a3, Object* a4, int a5);
void _container_enter(int a1, int a2);
void _container_exit(int keyCode, int inventoryWindowType);
int _drop_into_container(Object* a1, Object* a2, int a3, Object** a4, int quantity);
int _drop_ammo_into_weapon(Object* weapon, Object* ammo, Object** a3, int quantity, int keyCode);
void _draw_amount(int value, int inventoryWindowType);
int inventoryQuantitySelect(int inventoryWindowType, Object* item, int a3);
int inventoryQuantityWindowInit(int inventoryWindowType, Object* item);
int inventoryQuantityWindowFree(int inventoryWindowType);
int _inven_set_timer(Object* a1);

#endif /* INVENTORY_H */
