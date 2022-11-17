#ifndef FALLOUT_PLIB_GNW_GNW_TYPES_H_
#define FALLOUT_PLIB_GNW_GNW_TYPES_H_

#include "plib/gnw/rect.h"

// The maximum number of buttons in one radio group.
#define RADIO_GROUP_BUTTON_LIST_CAPACITY 64

typedef enum WindowFlags {
    WINDOW_FLAG_0x01 = 0x01,
    WINDOW_FLAG_0x02 = 0x02,
    WINDOW_FLAG_0x04 = 0x04,
    WINDOW_HIDDEN = 0x08,
    WINDOW_FLAG_0x10 = 0x10,
    WINDOW_FLAG_0x20 = 0x20,
    WINDOW_FLAG_0x40 = 0x40,
    WINDOW_FLAG_0x80 = 0x80,
    WINDOW_FLAG_0x0100 = 0x0100,
} WindowFlags;

typedef enum ButtonFlags {
    BUTTON_FLAG_0x01 = 0x01,
    BUTTON_FLAG_0x02 = 0x02,
    BUTTON_FLAG_0x04 = 0x04,
    BUTTON_FLAG_DISABLED = 0x08,
    BUTTON_FLAG_0x10 = 0x10,
    BUTTON_FLAG_TRANSPARENT = 0x20,
    BUTTON_FLAG_0x40 = 0x40,
    BUTTON_FLAG_0x010000 = 0x010000,
    BUTTON_FLAG_0x020000 = 0x020000,
    BUTTON_FLAG_0x040000 = 0x040000,
    BUTTON_FLAG_RIGHT_MOUSE_BUTTON_CONFIGURED = 0x080000,
} ButtonFlags;

typedef struct Button Button;
typedef struct RadioGroup RadioGroup;

typedef void WindowBlitProc(unsigned char* src, int width, int height, int srcPitch, unsigned char* dest, int destPitch);
typedef void ButtonCallback(int btn, int keyCode);

typedef struct MenuPulldown {
    Rect rect;
    int keyCode;
    int itemsLength;
    char** items;
    int field_1C;
    int field_20;
} MenuPulldown;

typedef struct MenuBar {
    int win;
    Rect rect;
    int pulldownsLength;
    MenuPulldown pulldowns[15];
    int borderColor;
    int backgroundColor;
} MenuBar;
static_assert(sizeof(MenuBar) == 572, "wrong size");

typedef struct Window {
    int id;
    int flags;
    Rect rect;
    int width;
    int height;
    int field_20;
    // rand
    int field_24;
    // rand
    int field_28;
    unsigned char* buffer;
    Button* buttonListHead;
    Button* field_34;
    Button* field_38;
    MenuBar* menuBar;
    WindowBlitProc* blitProc;
} Window;
static_assert(sizeof(Window) == 68, "wrong size");

typedef struct Button {
    int id;
    int flags;
    Rect rect;
    int mouseEnterEventCode;
    int mouseExitEventCode;
    int lefMouseDownEventCode;
    int leftMouseUpEventCode;
    int rightMouseDownEventCode;
    int rightMouseUpEventCode;
    unsigned char* mouseUpImage;
    unsigned char* mouseDownImage;
    unsigned char* mouseHoverImage;
    unsigned char* field_3C;
    unsigned char* field_40;
    unsigned char* field_44;
    unsigned char* currentImage;
    unsigned char* mask;
    ButtonCallback* mouseEnterProc;
    ButtonCallback* mouseExitProc;
    ButtonCallback* leftMouseDownProc;
    ButtonCallback* leftMouseUpProc;
    ButtonCallback* rightMouseDownProc;
    ButtonCallback* rightMouseUpProc;
    ButtonCallback* onPressed;
    ButtonCallback* onUnpressed;
    RadioGroup* radioGroup;
    Button* prev;
    Button* next;
} Button;
static_assert(sizeof(Button) == 124, "wrong size");

typedef struct RadioGroup {
    int field_0;
    int field_4;
    void (*field_8)(int);
    int buttonsLength;
    Button* buttons[RADIO_GROUP_BUTTON_LIST_CAPACITY];
} RadioGroup;

#endif /* FALLOUT_PLIB_GNW_GNW_TYPES_H_ */
