#ifndef GAME_MOUSE_H
#define GAME_MOUSE_H

#include "art.h"
#include "geometry.h"
#include "obj_types.h"

#include <stdbool.h>

typedef enum ScrollableDirections {
    SCROLLABLE_W = 0x01,
    SCROLLABLE_E = 0x02,
    SCROLLABLE_N = 0x04,
    SCROLLABLE_S = 0x08,
} ScrollableDirections;

typedef enum GameMouseMode {
    GAME_MOUSE_MODE_MOVE,
    GAME_MOUSE_MODE_ARROW,
    GAME_MOUSE_MODE_CROSSHAIR,
    GAME_MOUSE_MODE_USE_CROSSHAIR,
    GAME_MOUSE_MODE_USE_FIRST_AID,
    GAME_MOUSE_MODE_USE_DOCTOR,
    GAME_MOUSE_MODE_USE_LOCKPICK,
    GAME_MOUSE_MODE_USE_STEAL,
    GAME_MOUSE_MODE_USE_TRAPS,
    GAME_MOUSE_MODE_USE_SCIENCE,
    GAME_MOUSE_MODE_USE_REPAIR,
    GAME_MOUSE_MODE_COUNT,
    FIRST_GAME_MOUSE_MODE_SKILL = GAME_MOUSE_MODE_USE_FIRST_AID,
    GAME_MOUSE_MODE_SKILL_COUNT = GAME_MOUSE_MODE_COUNT - FIRST_GAME_MOUSE_MODE_SKILL,
} GameMouseMode;

typedef enum GameMouseActionMenuItem {
    GAME_MOUSE_ACTION_MENU_ITEM_CANCEL = 0,
    GAME_MOUSE_ACTION_MENU_ITEM_DROP = 1,
    GAME_MOUSE_ACTION_MENU_ITEM_INVENTORY = 2,
    GAME_MOUSE_ACTION_MENU_ITEM_LOOK = 3,
    GAME_MOUSE_ACTION_MENU_ITEM_ROTATE = 4,
    GAME_MOUSE_ACTION_MENU_ITEM_TALK = 5,
    GAME_MOUSE_ACTION_MENU_ITEM_USE = 6,
    GAME_MOUSE_ACTION_MENU_ITEM_UNLOAD = 7,
    GAME_MOUSE_ACTION_MENU_ITEM_USE_SKILL = 8,
    GAME_MOUSE_ACTION_MENU_ITEM_PUSH = 9,
    GAME_MOUSE_ACTION_MENU_ITEM_COUNT,
} GameMouseActionMenuItem;

typedef enum MouseCursorType {
    MOUSE_CURSOR_NONE,
    MOUSE_CURSOR_ARROW,
    MOUSE_CURSOR_SMALL_ARROW_UP,
    MOUSE_CURSOR_SMALL_ARROW_DOWN,
    MOUSE_CURSOR_SCROLL_NW,
    MOUSE_CURSOR_SCROLL_N,
    MOUSE_CURSOR_SCROLL_NE,
    MOUSE_CURSOR_SCROLL_E,
    MOUSE_CURSOR_SCROLL_SE,
    MOUSE_CURSOR_SCROLL_S,
    MOUSE_CURSOR_SCROLL_SW,
    MOUSE_CURSOR_SCROLL_W,
    MOUSE_CURSOR_SCROLL_NW_INVALID,
    MOUSE_CURSOR_SCROLL_N_INVALID,
    MOUSE_CURSOR_SCROLL_NE_INVALID,
    MOUSE_CURSOR_SCROLL_E_INVALID,
    MOUSE_CURSOR_SCROLL_SE_INVALID,
    MOUSE_CURSOR_SCROLL_S_INVALID,
    MOUSE_CURSOR_SCROLL_SW_INVALID,
    MOUSE_CURSOR_SCROLL_W_INVALID,
    MOUSE_CURSOR_CROSSHAIR,
    MOUSE_CURSOR_PLUS,
    MOUSE_CURSOR_DESTROY,
    MOUSE_CURSOR_USE_CROSSHAIR,
    MOUSE_CURSOR_WATCH,
    MOUSE_CURSOR_WAIT_PLANET,
    MOUSE_CURSOR_WAIT_WATCH,
    MOUSE_CURSOR_TYPE_COUNT,
    FIRST_GAME_MOUSE_ANIMATED_CURSOR = MOUSE_CURSOR_WAIT_PLANET,
} MouseCursorType;

extern bool gGameMouseInitialized;
extern int dword_518BFC;
extern int dword_518C00;
extern int dword_518C04;
extern int dword_518C08;
extern int gGameMouseCursor;
extern CacheEntry* gGameMouseCursorFrmHandle;
extern const int gGameMouseCursorFrmIds[MOUSE_CURSOR_TYPE_COUNT];
extern bool gGameMouseObjectsInitialized;
extern bool dword_518C84;
extern unsigned int dword_518C88;
extern Art* gGameMouseActionMenuFrm;
extern CacheEntry* gGameMouseActionMenuFrmHandle;
extern int gGameMouseActionMenuFrmWidth;
extern int gGameMouseActionMenuFrmHeight;
extern int gGameMouseActionMenuFrmDataSize;
extern int dword_518CA0;
extern int dword_518CA4;
extern unsigned char* gGameMouseActionMenuFrmData;
extern Art* gGameMouseActionPickFrm;
extern CacheEntry* gGameMouseActionPickFrmHandle;
extern int gGameMouseActionPickFrmWidth;
extern int gGameMouseActionPickFrmHeight;
extern int gGameMouseActionPickFrmDataSize;
extern int dword_518CC0;
extern int dword_518CC4;
extern unsigned char* gGameMouseActionPickFrmData;
extern Art* gGameMouseActionHitFrm;
extern CacheEntry* gGameMouseActionHitFrmHandle;
extern int gGameMouseActionHitFrmWidth;
extern int gGameMouseActionHitFrmHeight;
extern int gGameMouseActionHitFrmDataSize;
extern unsigned char* gGameMouseActionHitFrmData;
extern Art* gGameMouseBouncingCursorFrm;
extern CacheEntry* gGameMouseBouncingCursorFrmHandle;
extern int gGameMouseBouncingCursorFrmWidth;
extern int gGameMouseBouncingCursorFrmHeight;
extern int gGameMouseBouncingCursorFrmDataSize;
extern unsigned char* gGameMouseBouncingCursorFrmData;
extern Art* gGameMouseHexCursorFrm;
extern CacheEntry* gGameMouseHexCursorFrmHandle;
extern int gGameMouseHexCursorFrmWidth;
extern int gGameMouseHexCursorHeight;
extern int gGameMouseHexCursorDataSize;
extern unsigned char* gGameMouseHexCursorFrmData;
extern unsigned char gGameMouseActionMenuItemsLength;
extern unsigned char* off_518D18;
extern unsigned char gGameMouseActionMenuHighlightedItemIndex;
extern const short gGameMouseActionMenuItemFrmIds[GAME_MOUSE_ACTION_MENU_ITEM_COUNT];
extern int dword_518D34;
extern int gGameMouseMode;
extern int gGameMouseModeFrmIds[GAME_MOUSE_MODE_COUNT];
extern const int gGameMouseModeSkills[GAME_MOUSE_MODE_SKILL_COUNT];
extern int gGameMouseAnimatedCursorNextFrame;
extern unsigned int gGameMouseAnimatedCursorLastUpdateTimestamp;
extern int dword_518D8C;
extern bool gGameMouseItemHighlightEnabled;
extern Object* gGameMouseHighlightedItem;
extern bool dword_518D98;
extern int dword_518D9C;

extern int gGameMouseActionMenuItems[GAME_MOUSE_ACTION_MENU_ITEM_COUNT];
extern int gGameMouseLastX;
extern int gGameMouseLastY;
extern Object* gGameMouseBouncingCursor;
extern Object* gGameMouseHexCursor;
extern Object* gGameMousePointedObject;

int gameMouseInit();
int gameMouseReset();
void gameMouseExit();
void sub_44B454();
void sub_44B48C(int a1);
void sub_44B4CC();
void sub_44B4D8();
int sub_44B504();
int sub_44B54C();
void gameMouseRefresh();
void sub_44BFA8(int mouseX, int mouseY, int mouseState);
int gameMouseSetCursor(int cursor);
int gameMouseGetCursor();
void sub_44C9F8();
void gameMouseSetMode(int a1);
int gameMouseGetMode();
void sub_44CB74();
void sub_44CBD0();
int gameMouseSetBouncingCursorFid(int fid);
void gameMouseResetBouncingCursorFid();
void gameMouseObjectsShow();
void gameMouseObjectsHide();
bool gameMouseObjectsIsVisible();
Object* gameMouseGetObjectUnderCursor(int objectType, bool a2, int elevation);
int gameMouseRenderPrimaryAction(int x, int y, int menuItem, int width, int height);
int sub_44D200(int* a1, int* a2);
int gameMouseRenderActionMenuItems(int x, int y, const int* menuItems, int menuItemsCount, int width, int height);
int gameMouseHighlightActionMenuItemAtIndex(int menuItemIndex);
int gameMouseRenderAccuracy(const char* string, int color);
int gameMouseRenderActionPoints(const char* string, int color);
void gameMouseLoadItemHighlight();
int gameMouseObjectsInit();
int gameMouseObjectsReset();
void gameMouseObjectsFree();
int gameMouseActionMenuInit();
void gameMouseActionMenuFree();
int gameMouseUpdateHexCursorFid(Rect* rect);
int sub_44DF94(int x, int y, int elevation, Rect* a4);
int gameMouseHandleScrolling(int x, int y, int cursor);
void sub_44E544(Object* object);
int objectIsDoor(Object* object);

#endif /* GAME_MOUSE_H */
