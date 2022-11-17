#include "int/window.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plib/db/db.h"
#include "plib/color/color.h"
#include "plib/gnw/input.h"
#include "int/datafile.h"
#include "plib/gnw/grbuf.h"
#include "game/game.h"
#include "int/intlib.h"
#include "int/memdbg.h"
#include "int/mousemgr.h"
#include "int/movie.h"
#include "plib/gnw/button.h"
#include "plib/gnw/text.h"
#include "plib/gnw/svga.h"

typedef enum ManagedButtonMouseEvent {
    MANAGED_BUTTON_MOUSE_EVENT_BUTTON_DOWN,
    MANAGED_BUTTON_MOUSE_EVENT_BUTTON_UP,
    MANAGED_BUTTON_MOUSE_EVENT_ENTER,
    MANAGED_BUTTON_MOUSE_EVENT_EXIT,
    MANAGED_BUTTON_MOUSE_EVENT_COUNT,
} ManagedButtonMouseEvent;

typedef enum ManagedButtonRightMouseEvent {
    MANAGED_BUTTON_RIGHT_MOUSE_EVENT_BUTTON_DOWN,
    MANAGED_BUTTON_RIGHT_MOUSE_EVENT_BUTTON_UP,
    MANAGED_BUTTON_RIGHT_MOUSE_EVENT_COUNT,
} ManagedButtonRightMouseEvent;

typedef struct ManagedButton {
    int btn;
    int width;
    int height;
    int x;
    int y;
    int flags;
    int field_18;
    char name[32];
    Program* program;
    unsigned char* pressed;
    unsigned char* normal;
    unsigned char* hover;
    void* field_4C;
    void* field_50;
    int procs[MANAGED_BUTTON_MOUSE_EVENT_COUNT];
    int rightProcs[MANAGED_BUTTON_RIGHT_MOUSE_EVENT_COUNT];
    ManagedButtonMouseEventCallback* mouseEventCallback;
    ManagedButtonMouseEventCallback* rightMouseEventCallback;
    void* mouseEventCallbackUserData;
    void* rightMouseEventCallbackUserData;
} ManagedButton;

typedef struct ManagedWindow {
    char name[32];
    int window;
    int width;
    int height;
    Region** regions;
    int currentRegionIndex;
    int regionsLength;
    int field_38;
    ManagedButton* buttons;
    int buttonsLength;
    int field_44;
    int field_48;
    int field_4C;
    int field_50;
    float field_54;
    float field_58;
} ManagedWindow;

static_assert(sizeof(ManagedButton) == 0x7C, "wrong size");

static bool checkRegion(int windowIndex, int mouseX, int mouseY, int mouseEvent);
static bool checkAllRegions();
static void doRegionRightFunc(Region* region, int a2);
static void doRegionFunc(Region* region, int a2);
static void doButtonOn(int btn, int keyCode);
static void doButtonProc(int btn, int mouseEvent);
static void doButtonOff(int btn, int keyCode);
static void doButtonPress(int btn, int keyCode);
static void doButtonRelease(int btn, int keyCode);
static void doRightButtonPress(int btn, int keyCode);
static void doRightButtonProc(int btn, int mouseEvent);
static void doRightButtonRelease(int btn, int keyCode);
static void setButtonGFX(int width, int height, unsigned char* normal, unsigned char* pressed, unsigned char* a5);
static void redrawButton(ManagedButton* button);
static void removeProgramReferences(Program* program);

// 0x51DCAC
static int holdTime = 250;

// 0x51DCB0
static int checkRegionEnable = 1;

// 0x51DCB4
static int winTOS = -1;

// 051DCB8
static int currentWindow = -1;

// 0x51DCBC
static VideoSystemInitProc* gfx_init[12] = {
    init_mode_320_200,
    init_mode_640_480,
    init_mode_640_480_16,
    init_mode_320_400,
    init_mode_640_480_16,
    init_mode_640_400,
    init_mode_640_480_16,
    init_mode_800_600,
    init_mode_640_480_16,
    init_mode_1024_768,
    init_mode_640_480_16,
    init_mode_1280_1024,
};

// 0x51DD1C
static Size sizes[12] = {
    { 320, 200 },
    { 640, 480 },
    { 640, 240 },
    { 320, 400 },
    { 640, 200 },
    { 640, 400 },
    { 800, 300 },
    { 800, 600 },
    { 1024, 384 },
    { 1024, 768 },
    { 1280, 512 },
    { 1280, 1024 },
};

// 0x51DD7C
static int numInputFunc = 0;

// 0x51DD80
int _lastWin = -1;

// 0x51DD84
int _said_quit = 1;

// 0x66E770
static int winStack[MANAGED_WINDOW_COUNT];

// 0x66E7B0
static char alphaBlendTable[64 * 256];

// 0x6727B0
static ManagedWindow windows[MANAGED_WINDOW_COUNT];

// 0x672D70
static WindowInputHandler** inputFunc;

// 0x672D74
static ManagedWindowCreateCallback* createWindowFunc;

// 0x672D78
static ManagedWindowSelectFunc* selectWindowFunc;

// 0x672D7C
static int xres;

// 0x672D80
static DisplayInWindowCallback* displayFunc;

// 0x672D84
static WindowDeleteCallback* deleteWindowFunc;

// 0x672D88
static int yres;

// Highlight color (maybe r).
//
// 0x672D8C
static int currentHighlightColorR;

// 0x672D90
static int currentFont;

// 0x672D94
static ButtonCallback* soundDisableFunc;

// 0x672D98
static ButtonCallback* soundPressFunc;

// 0x672D9C
static ButtonCallback* soundReleaseFunc;

// Text color (maybe g).
//
// 0x672DA0
static int currentTextColorG;

// text color (maybe b).
//
// 0x672DA4
static int currentTextColorB;

// 0x672DA8
static int currentTextFlags;

// Text color (maybe r)
//
// 0x672DAC
static int currentTextColorR;

// highlight color (maybe g)
//
// 0x672DB0
static int currentHighlightColorG;

// Highlight color (maybe b).
//
// 0x672DB4
static int currentHighlightColorB;

// 0x4B6120
int windowGetFont()
{
    return currentFont;
}

// 0x4B6128
int windowSetFont(int a1)
{
    currentFont = a1;
    text_font(a1);
    return 1;
}

// NOTE: Unused.
//
// 0x4B6138
void windowResetTextAttributes()
{
    // NOTE: Uninline.
    windowSetTextColor(1.0, 1.0, 1.0);

    // NOTE: Uninline.
    windowSetTextFlags(0x2000000 | 0x10000);
}

// 0x4B6160
int windowGetTextFlags()
{
    return currentTextFlags;
}

// 0x4B6168
int windowSetTextFlags(int a1)
{
    currentTextFlags = a1;
    return 1;
}

// 0x4B6174
unsigned char windowGetTextColor()
{
    return colorTable[currentTextColorB | (currentTextColorG << 5) | (currentTextColorR << 10)];
}

// 0x4B6198
unsigned char windowGetHighlightColor()
{
    return colorTable[currentHighlightColorB | (currentHighlightColorG << 5) | (currentHighlightColorR << 10)];
}

// 0x4B61BC
int windowSetTextColor(float r, float g, float b)
{
    currentTextColorR = (int)(r * 31.0);
    currentTextColorG = (int)(g * 31.0);
    currentTextColorB = (int)(b * 31.0);

    return 1;
}

// 0x4B6208
int windowSetHighlightColor(float r, float g, float b)
{
    currentHighlightColorR = (int)(r * 31.0);
    currentHighlightColorG = (int)(g * 31.0);
    currentHighlightColorB = (int)(b * 31.0);

    return 1;
}

// 0x4B62E4
static bool checkRegion(int windowIndex, int mouseX, int mouseY, int mouseEvent)
{
    // TODO: Incomplete.
    return false;
}

// 0x4B6858
bool windowCheckRegion(int windowIndex, int mouseX, int mouseY, int mouseEvent)
{
    bool rc = checkRegion(windowIndex, mouseX, mouseY, mouseEvent);

    ManagedWindow* managedWindow = &(windows[windowIndex]);
    int v1 = managedWindow->field_38;

    for (int index = 0; index < managedWindow->regionsLength; index++) {
        Region* region = managedWindow->regions[index];
        if (region != NULL) {
            if (region->field_6C != 0) {
                region->field_6C = 0;
                rc = true;

                if (region->mouseEventCallback != NULL) {
                    region->mouseEventCallback(region, region->mouseEventCallbackUserData, 2);
                    if (v1 != managedWindow->field_38) {
                        return true;
                    }
                }

                if (region->rightMouseEventCallback != NULL) {
                    region->rightMouseEventCallback(region, region->rightMouseEventCallbackUserData, 2);
                    if (v1 != managedWindow->field_38) {
                        return true;
                    }
                }

                if (region->program != NULL && region->procs[2] != 0) {
                    executeProc(region->program, region->procs[2]);
                    if (v1 != managedWindow->field_38) {
                        return true;
                    }
                }
            }
        }
    }

    return rc;
}

// 0x4B69BC
bool windowRefreshRegions()
{
    int mouseX;
    int mouseY;
    mouse_get_position(&mouseX, &mouseY);

    int win = win_get_top_win(mouseX, mouseY);

    for (int windowIndex = 0; windowIndex < MANAGED_WINDOW_COUNT; windowIndex++) {
        ManagedWindow* managedWindow = &(windows[windowIndex]);
        if (managedWindow->window == win) {
            for (int regionIndex = 0; regionIndex < managedWindow->regionsLength; regionIndex++) {
                Region* region = managedWindow->regions[regionIndex];
                region->rightProcs[3] = 0;
            }

            int mouseEvent = mouse_get_buttons();
            return windowCheckRegion(windowIndex, mouseX, mouseY, mouseEvent);
        }
    }

    return false;
}

// 0x4B6A54
static bool checkAllRegions()
{
    if (!checkRegionEnable) {
        return false;
    }

    int mouseX;
    int mouseY;
    mouse_get_position(&mouseX, &mouseY);

    int mouseEvent = mouse_get_buttons();
    int win = win_get_top_win(mouseX, mouseY);

    for (int windowIndex = 0; windowIndex < MANAGED_WINDOW_COUNT; windowIndex++) {
        ManagedWindow* managedWindow = &(windows[windowIndex]);
        if (managedWindow->window != -1 && managedWindow->window == win) {
            if (_lastWin != -1 && _lastWin != windowIndex && windows[_lastWin].window != -1) {
                ManagedWindow* managedWindow = &(windows[_lastWin]);
                int v1 = managedWindow->field_38;

                for (int regionIndex = 0; regionIndex < managedWindow->regionsLength; regionIndex++) {
                    Region* region = managedWindow->regions[regionIndex];
                    if (region != NULL && region->rightProcs[3] != 0) {
                        region->rightProcs[3] = 0;
                        if (region->mouseEventCallback != NULL) {
                            region->mouseEventCallback(region, region->mouseEventCallbackUserData, 3);
                            if (v1 != managedWindow->field_38) {
                                return true;
                            }
                        }

                        if (region->rightMouseEventCallback != NULL) {
                            region->rightMouseEventCallback(region, region->rightMouseEventCallbackUserData, 3);
                            if (v1 != managedWindow->field_38) {
                                return true;
                            }
                        }

                        if (region->program != NULL && region->procs[3] != 0) {
                            executeProc(region->program, region->procs[3]);
                            if (v1 != managedWindow->field_38) {
                                return 1;
                            }
                        }
                    }
                }
                _lastWin = -1;
            } else {
                _lastWin = windowIndex;
            }

            return windowCheckRegion(windowIndex, mouseX, mouseY, mouseEvent);
        }
    }

    return false;
}

// 0x4B6C48
void windowAddInputFunc(WindowInputHandler* handler)
{
    int index;
    for (index = 0; index < numInputFunc; index++) {
        if (inputFunc[index] == NULL) {
            break;
        }
    }

    if (index == numInputFunc) {
        if (inputFunc != NULL) {
            inputFunc = (WindowInputHandler**)myrealloc(inputFunc, sizeof(*inputFunc) * (numInputFunc + 1), __FILE__, __LINE__); // "..\\int\\WINDOW.C", 521
        } else {
            inputFunc = (WindowInputHandler**)mymalloc(sizeof(*inputFunc), __FILE__, __LINE__); // "..\\int\\WINDOW.C", 523
        }
    }

    inputFunc[numInputFunc] = handler;
    numInputFunc++;
}

// 0x4B6CE8
static void doRegionRightFunc(Region* region, int a2)
{
    int v1 = windows[currentWindow].field_38;
    if (region->rightMouseEventCallback != NULL) {
        region->rightMouseEventCallback(region, region->rightMouseEventCallbackUserData, a2);
        if (v1 != windows[currentWindow].field_38) {
            return;
        }
    }

    if (a2 < 4) {
        if (region->program != NULL && region->rightProcs[a2] != 0) {
            executeProc(region->program, region->rightProcs[a2]);
        }
    }
}

// 0x4B6D68
static void doRegionFunc(Region* region, int a2)
{
    int v1 = windows[currentWindow].field_38;
    if (region->mouseEventCallback != NULL) {
        region->mouseEventCallback(region, region->mouseEventCallbackUserData, a2);
        if (v1 != windows[currentWindow].field_38) {
            return;
        }
    }

    if (a2 < 4) {
        if (region->program != NULL && region->rightProcs[a2] != 0) {
            executeProc(region->program, region->rightProcs[a2]);
        }
    }
}

// 0x4B6DE8
bool windowActivateRegion(const char* regionName, int a2)
{
    if (currentWindow == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);

    if (a2 <= 4) {
        for (int index = 0; index < managedWindow->regionsLength; index++) {
            Region* region = managedWindow->regions[index];
            if (stricmp(regionGetName(region), regionName) == 0) {
                doRegionFunc(region, a2);
                return true;
            }
        }
    } else {
        for (int index = 0; index < managedWindow->regionsLength; index++) {
            Region* region = managedWindow->regions[index];
            if (stricmp(regionGetName(region), regionName) == 0) {
                doRegionRightFunc(region, a2 - 5);
                return true;
            }
        }
    }

    return false;
}

// 0x4B6ED0
int getInput()
{
    int keyCode = get_input();
    if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
        game_quit_with_confirm();
    }

    if (game_user_wants_to_quit != 0) {
        _said_quit = 1 - _said_quit;
        if (_said_quit) {
            return -1;
        }

        return KEY_ESCAPE;
    }

    for (int index = 0; index < numInputFunc; index++) {
        WindowInputHandler* handler = inputFunc[index];
        if (handler != NULL) {
            if (handler(keyCode) != 0) {
                return -1;
            }
        }
    }

    return keyCode;
}

// 0x4B6F60
static void doButtonOn(int btn, int keyCode)
{
    doButtonProc(btn, MANAGED_BUTTON_MOUSE_EVENT_ENTER);
}

// 0x4B6F68
static void doButtonProc(int btn, int mouseEvent)
{
    int win = win_last_button_winID();
    if (win == -1) {
        return;
    }

    for (int windowIndex = 0; windowIndex < MANAGED_WINDOW_COUNT; windowIndex++) {
        ManagedWindow* managedWindow = &(windows[windowIndex]);
        if (managedWindow->window == win) {
            for (int buttonIndex = 0; buttonIndex < managedWindow->buttonsLength; buttonIndex++) {
                ManagedButton* managedButton = &(managedWindow->buttons[buttonIndex]);
                if (managedButton->btn == btn) {
                    if ((managedButton->flags & 0x02) != 0) {
                        win_set_button_rest_state(managedButton->btn, 0, 0);
                    } else {
                        if (managedButton->program != NULL && managedButton->procs[mouseEvent] != 0) {
                            executeProc(managedButton->program, managedButton->procs[mouseEvent]);
                        }

                        if (managedButton->mouseEventCallback != NULL) {
                            managedButton->mouseEventCallback(managedButton->mouseEventCallbackUserData, mouseEvent);
                        }
                    }
                }
            }
        }
    }
}

// 0x4B7028
static void doButtonOff(int btn, int keyCode)
{
    doButtonProc(btn, MANAGED_BUTTON_MOUSE_EVENT_EXIT);
}

// 0x4B7034
static void doButtonPress(int btn, int keyCode)
{
    doButtonProc(btn, MANAGED_BUTTON_MOUSE_EVENT_BUTTON_DOWN);
}

// 0x4B703C
static void doButtonRelease(int btn, int keyCode)
{
    doButtonProc(btn, MANAGED_BUTTON_MOUSE_EVENT_BUTTON_UP);
}

// NOTE: Unused.
//
// 0x4B7048
static void doRightButtonPress(int btn, int keyCode)
{
    doRightButtonProc(btn, MANAGED_BUTTON_RIGHT_MOUSE_EVENT_BUTTON_DOWN);
}

// NOTE: Unused.
//
// 0x4B704C
static void doRightButtonProc(int btn, int mouseEvent)
{
    int win = win_last_button_winID();
    if (win == -1) {
        return;
    }

    for (int windowIndex = 0; windowIndex < MANAGED_WINDOW_COUNT; windowIndex++) {
        ManagedWindow* managedWindow = &(windows[windowIndex]);
        if (managedWindow->window == win) {
            for (int buttonIndex = 0; buttonIndex < managedWindow->buttonsLength; buttonIndex++) {
                ManagedButton* managedButton = &(managedWindow->buttons[buttonIndex]);
                if (managedButton->btn == btn) {
                    if ((managedButton->flags & 0x02) != 0) {
                        win_set_button_rest_state(managedButton->btn, 0, 0);
                    } else {
                        if (managedButton->program != NULL && managedButton->rightProcs[mouseEvent] != 0) {
                            executeProc(managedButton->program, managedButton->rightProcs[mouseEvent]);
                        }

                        if (managedButton->rightMouseEventCallback != NULL) {
                            managedButton->rightMouseEventCallback(managedButton->rightMouseEventCallbackUserData, mouseEvent);
                        }
                    }
                }
            }
        }
    }
}

// NOTE: Unused.
//
// 0x4B710C
static void doRightButtonRelease(int btn, int keyCode)
{
    doRightButtonProc(btn, MANAGED_BUTTON_RIGHT_MOUSE_EVENT_BUTTON_UP);
}

// 0x4B7118
static void setButtonGFX(int width, int height, unsigned char* normal, unsigned char* pressed, unsigned char* a5)
{
    if (normal != NULL) {
        buf_fill(normal, width, height, width, colorTable[0]);
        buf_fill(normal + width + 1, width - 2, height - 2, width, intensityColorTable[colorTable[32767]][89]);
        draw_line(normal, width, 1, 1, width - 2, 1, colorTable[32767]);
        draw_line(normal, width, 2, 2, width - 3, 2, colorTable[32767]);
        draw_line(normal, width, 1, height - 2, width - 2, height - 2, intensityColorTable[colorTable[32767]][44]);
        draw_line(normal, width, 2, height - 3, width - 3, height - 3, intensityColorTable[colorTable[32767]][44]);
        draw_line(normal, width, width - 2, 1, width - 3, 2, intensityColorTable[colorTable[32767]][89]);
        draw_line(normal, width, 1, 2, 1, height - 3, colorTable[32767]);
        draw_line(normal, width, 2, 3, 2, height - 4, colorTable[32767]);
        draw_line(normal, width, width - 2, 2, width - 2, height - 3, intensityColorTable[colorTable[32767]][44]);
        draw_line(normal, width, width - 3, 3, width - 3, height - 4, intensityColorTable[colorTable[32767]][44]);
        draw_line(normal, width, 1, height - 2, 2, height - 3, intensityColorTable[colorTable[32767]][89]);
    }

    if (pressed != NULL) {
        buf_fill(pressed, width, height, width, colorTable[0]);
        buf_fill(pressed + width + 1, width - 2, height - 2, width, intensityColorTable[colorTable[32767]][89]);
        draw_line(pressed, width, 1, 1, width - 2, 1, colorTable[32767] + 44);
        draw_line(pressed, width, 1, 1, 1, height - 2, colorTable[32767] + 44);
    }

    if (a5 != NULL) {
        buf_fill(a5, width, height, width, colorTable[0]);
        buf_fill(a5 + width + 1, width - 2, height - 2, width, intensityColorTable[colorTable[32767]][89]);
        draw_line(a5, width, 1, 1, width - 2, 1, colorTable[32767]);
        draw_line(a5, width, 2, 2, width - 3, 2, colorTable[32767]);
        draw_line(a5, width, 1, height - 2, width - 2, height - 2, intensityColorTable[colorTable[32767]][44]);
        draw_line(a5, width, 2, height - 3, width - 3, height - 3, intensityColorTable[colorTable[32767]][44]);
        draw_line(a5, width, width - 2, 1, width - 3, 2, intensityColorTable[colorTable[32767]][89]);
        draw_line(a5, width, 1, 2, 1, height - 3, colorTable[32767]);
        draw_line(a5, width, 2, 3, 2, height - 4, colorTable[32767]);
        draw_line(a5, width, width - 2, 2, width - 2, height - 3, intensityColorTable[colorTable[32767]][44]);
        draw_line(a5, width, width - 3, 3, width - 3, height - 4, intensityColorTable[colorTable[32767]][44]);
        draw_line(a5, width, 1, height - 2, 2, height - 3, intensityColorTable[colorTable[32767]][89]);
    }
}

// NOTE: Unused.
//
// 0x4B75F4
static void redrawButton(ManagedButton* button)
{
    win_register_button_image(button->btn, button->normal, button->pressed, button->hover, 0);
}

// NOTE: Unused.
//
// 0x4B7610
int windowHide()
{
    if (windows[currentWindow].window == -1) {
        return 0;
    }

    win_hide(windows[currentWindow].window);

    return 1;
}

// NOTE: Unused.
//
// 0x4B7648
int windowShow()
{
    if (windows[currentWindow].window == -1) {
        return 0;
    }

    win_show(windows[currentWindow].window);

    return 1;
}

// 0x4B7680
int windowDraw()
{
    ManagedWindow* managedWindow = &(windows[currentWindow]);
    if (managedWindow->window == -1) {
        return 0;
    }

    win_draw(managedWindow->window);

    return 1;
}

// NOTE: Unused.
//
// 0x4B76B8
int windowDrawRect(int left, int top, int right, int bottom)
{
    Rect rect;

    rect.ulx = left;
    rect.uly = top;
    rect.lrx = right;
    rect.lry = bottom;
    win_draw_rect(windows[currentWindow].window, &rect);

    return 1;
}

// NOTE: Unused.
//
// 0x4B76F8
int windowDrawRectID(int windowId, int left, int top, int right, int bottom)
{
    Rect rect;

    rect.ulx = left;
    rect.uly = top;
    rect.lrx = right;
    rect.lry = bottom;
    win_draw_rect(windows[windowId].window, &rect);

    return 1;
}

// 0x4B7734
int windowWidth()
{
    return windows[currentWindow].width;
}

// 0x4B7754
int windowHeight()
{
    return windows[currentWindow].height;
}

// NOTE: Unused.
//
// 0x4B7774
int windowSX()
{
    Rect rect;

    win_get_rect(windows[currentWindow].window, &rect);

    return rect.ulx;
}

// NOTE: Unused.
//
// 0x4B77A4
int windowSY()
{
    Rect rect;

    win_get_rect(windows[currentWindow].window, &rect);

    return rect.uly;
}

// NOTE: Unused.
//
// 0x4B77D4
int pointInWindow(int x, int y)
{
    Rect rect;

    win_get_rect(windows[currentWindow].window, &rect);

    return x >= rect.ulx && x <= rect.lrx && y >= rect.uly && y <= rect.lry;
}

// NOTE: Unused.
//
// 0x4B7828
int windowGetRect(Rect* rect)
{
    return win_get_rect(windows[currentWindow].window, rect);
}

// NOTE: Unused.
//
// 0x4B7854
int windowGetID()
{
    return currentWindow;
}

// NOTE: Inlined.
//
// 0x4B785C
int windowGetGNWID()
{
    return windows[currentWindow].window;
}

// NOTE: Unused.
//
// 0x4B787C
int windowGetSpecificGNWID(int windowIndex)
{
    if (windowIndex >= 0 && windowIndex < MANAGED_WINDOW_COUNT) {
        return windows[windowIndex].window;
    }

    return -1;
}

// 0x4B78A4
bool deleteWindow(const char* windowName)
{
    int index;
    for (index = 0; index < MANAGED_WINDOW_COUNT; index++) {
        ManagedWindow* managedWindow = &(windows[index]);
        if (stricmp(managedWindow->name, windowName) == 0) {
            break;
        }
    }

    if (index == MANAGED_WINDOW_COUNT) {
        return false;
    }

    if (deleteWindowFunc != NULL) {
        deleteWindowFunc(index, windowName);
    }

    ManagedWindow* managedWindow = &(windows[index]);
    win_delete_widgets(managedWindow->window);
    win_delete(managedWindow->window);
    managedWindow->window = -1;
    managedWindow->name[0] = '\0';

    if (managedWindow->buttons != NULL) {
        for (int index = 0; index < managedWindow->buttonsLength; index++) {
            ManagedButton* button = &(managedWindow->buttons[index]);
            if (button->hover != NULL) {
                myfree(button->hover, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 802
            }

            if (button->field_4C != NULL) {
                myfree(button->field_4C, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 804
            }

            if (button->pressed != NULL) {
                myfree(button->pressed, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 806
            }

            if (button->normal != NULL) {
                myfree(button->normal, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 808
            }
        }

        myfree(managedWindow->buttons, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 810
    }

    if (managedWindow->regions != NULL) {
        for (int index = 0; index < managedWindow->regionsLength; index++) {
            Region* region = managedWindow->regions[index];
            if (region != NULL) {
                regionDelete(region);
            }
        }

        myfree(managedWindow->regions, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 818
        managedWindow->regions = NULL;
    }

    return true;
}

// 0x4B7AC4
int resizeWindow(const char* windowName, int x, int y, int width, int height)
{
    // TODO: Incomplete.
    return -1;
}

// 0x4B7E7C
int scaleWindow(const char* windowName, int x, int y, int width, int height)
{
    // TODO: Incomplete.
    return -1;
}

// 0x4B7F3C
int createWindow(const char* windowName, int x, int y, int width, int height, int a6, int flags)
{
    int windowIndex = -1;

    // NOTE: Original code is slightly different.
    for (int index = 0; index < MANAGED_WINDOW_COUNT; index++) {
        ManagedWindow* managedWindow = &(windows[index]);
        if (managedWindow->window == -1) {
            windowIndex = index;
            break;
        } else {
            if (stricmp(managedWindow->name, windowName) == 0) {
                deleteWindow(windowName);
                windowIndex = index;
                break;
            }
        }
    }

    if (windowIndex == -1) {
        return -1;
    }

    ManagedWindow* managedWindow = &(windows[windowIndex]);
    strncpy(managedWindow->name, windowName, 32);
    managedWindow->field_54 = 1.0;
    managedWindow->field_58 = 1.0;
    managedWindow->field_38 = 0;
    managedWindow->regions = NULL;
    managedWindow->regionsLength = 0;
    managedWindow->width = width;
    managedWindow->height = height;
    managedWindow->buttons = NULL;
    managedWindow->buttonsLength = 0;

    flags |= 0x101;
    if (createWindowFunc != NULL) {
        createWindowFunc(windowIndex, managedWindow->name, &flags);
    }

    managedWindow->window = win_add(x, y, width, height, a6, flags);
    managedWindow->field_48 = 0;
    managedWindow->field_44 = 0;
    managedWindow->field_4C = a6;
    managedWindow->field_50 = flags;

    return windowIndex;
}

// 0x4B80A4
int windowOutput(char* string)
{
    if (currentWindow == -1) {
        return 0;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);

    int x = (int)(managedWindow->field_44 * managedWindow->field_54);
    int y = (int)(managedWindow->field_48 * managedWindow->field_58);
    // NOTE: Uses `add` at 0x4B810E, not bitwise `or`.
    int flags = windowGetTextColor() + windowGetTextFlags();
    win_print(managedWindow->window, string, 0, x, y, flags);

    return 1;
}

// 0x4B814C
bool windowGotoXY(int x, int y)
{
    if (currentWindow == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    managedWindow->field_44 = (int)(x * managedWindow->field_54);
    managedWindow->field_48 = (int)(y * managedWindow->field_58);

    return true;
}

// 0x4B81C4
bool selectWindowID(int index)
{
    if (index < 0 || index >= MANAGED_WINDOW_COUNT) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[index]);
    if (managedWindow->window == -1) {
        return false;
    }

    currentWindow = index;

    if (selectWindowFunc != NULL) {
        selectWindowFunc(index, managedWindow->name);
    }

    return true;
}

// 0x4B821C
int selectWindow(const char* windowName)
{
    if (currentWindow != -1) {
        ManagedWindow* managedWindow = &(windows[currentWindow]);
        if (stricmp(managedWindow->name, windowName) == 0) {
            return currentWindow;
        }
    }

    int index;
    for (index = 0; index < MANAGED_WINDOW_COUNT; index++) {
        ManagedWindow* managedWindow = &(windows[index]);
        if (managedWindow->window != -1) {
            if (stricmp(managedWindow->name, windowName) == 0) {
                break;
            }
        }
    }

    if (selectWindowID(index)) {
        return index;
    }

    return -1;
}

// NOTE: Unused.
//
// 0x4B82A0
int windowGetDefined(const char* name)
{
    int index;

    for (index = 0; index < MANAGED_WINDOW_COUNT; index++) {
        if (windows[index].window != -1 && stricmp(windows[index].name, name) == 0) {
            return 1;
        }
    }

    return 0;
}

// 0x4B82DC
unsigned char* windowGetBuffer()
{
    if (currentWindow != -1) {
        ManagedWindow* managedWindow = &(windows[currentWindow]);
        return win_get_buf(managedWindow->window);
    }

    return NULL;
}

// NOTE: Unused.
//
// 0x4B8308
char* windowGetName()
{
    if (currentWindow != -1) {
        return windows[currentWindow].name;
    }

    return NULL;
}

// 0x4B8330
int pushWindow(const char* windowName)
{
    if (winTOS >= MANAGED_WINDOW_COUNT) {
        return -1;
    }

    int oldCurrentWindowIndex = currentWindow;

    int windowIndex = selectWindow(windowName);
    if (windowIndex == -1) {
        return -1;
    }

    // TODO: Check.
    for (int index = 0; index < winTOS; index++) {
        if (winStack[index] == oldCurrentWindowIndex) {
            memcpy(&(winStack[index]), &(winStack[index + 1]), sizeof(*winStack) * (winTOS - index));
            break;
        }
    }

    winTOS++;
    winStack[winTOS] = oldCurrentWindowIndex;

    return windowIndex;
}

// 0x4B83D4
int popWindow()
{
    if (winTOS == -1) {
        return -1;
    }

    int windowIndex = winStack[winTOS];
    ManagedWindow* managedWindow = &(windows[windowIndex]);
    winTOS--;

    return selectWindow(managedWindow->name);
}

// 0x4B8414
void windowPrintBuf(int win, char* string, int stringLength, int width, int maxY, int x, int y, int flags, int textAlignment)
{
    if (y + text_height() > maxY) {
        return;
    }

    if (stringLength > 255) {
        stringLength = 255;
    }

    char* stringCopy = (char*)mymalloc(stringLength + 1, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1078
    strncpy(stringCopy, string, stringLength);
    stringCopy[stringLength] = '\0';

    int stringWidth = text_width(stringCopy);
    int stringHeight = text_height();
    if (stringWidth == 0 || stringHeight == 0) {
        myfree(stringCopy, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1085
        return;
    }

    if ((flags & FONT_SHADOW) != 0) {
        stringWidth++;
        stringHeight++;
    }

    unsigned char* backgroundBuffer = (unsigned char*)mycalloc(stringWidth, stringHeight, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1093
    unsigned char* backgroundBufferPtr = backgroundBuffer;
    text_to_buf(backgroundBuffer, stringCopy, stringWidth, stringWidth, flags);

    switch (textAlignment) {
    case TEXT_ALIGNMENT_LEFT:
        if (stringWidth < width) {
            width = stringWidth;
        }
        break;
    case TEXT_ALIGNMENT_RIGHT:
        if (stringWidth <= width) {
            x += (width - stringWidth);
            width = stringWidth;
        } else {
            backgroundBufferPtr = backgroundBuffer + stringWidth - width;
        }
        break;
    case TEXT_ALIGNMENT_CENTER:
        if (stringWidth <= width) {
            x += (width - stringWidth) / 2;
            width = stringWidth;
        } else {
            backgroundBufferPtr = backgroundBuffer + (stringWidth - width) / 2;
        }
        break;
    }

    if (stringHeight + y > win_height(win)) {
        stringHeight = win_height(win) - y;
    }

    if ((flags & 0x2000000) != 0) {
        trans_buf_to_buf(backgroundBufferPtr, width, stringHeight, stringWidth, win_get_buf(win) + win_width(win) * y + x, win_width(win));
    } else {
        buf_to_buf(backgroundBufferPtr, width, stringHeight, stringWidth, win_get_buf(win) + win_width(win) * y + x, win_width(win));
    }

    myfree(backgroundBuffer, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1130
    myfree(stringCopy, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1131
}

// 0x4B8638
char** windowWordWrap(char* string, int maxLength, int a3, int* substringListLengthPtr)
{
    if (string == NULL) {
        *substringListLengthPtr = 0;
        return NULL;
    }

    char** substringList = NULL;
    int substringListLength = 0;

    char* start = string;
    char* pch = string;
    int v1 = a3;
    while (*pch != '\0') {
        v1 += text_char_width(*pch & 0xFF);
        if (*pch != '\n' && v1 <= maxLength) {
            v1 += text_spacing();
            pch++;
        } else {
            while (v1 > maxLength) {
                v1 -= text_char_width(*pch);
                pch--;
            }

            if (*pch != '\n') {
                while (pch != start && *pch != ' ') {
                    pch--;
                }
            }

            if (substringList != NULL) {
                substringList = (char**)myrealloc(substringList, sizeof(*substringList) * (substringListLength + 1), __FILE__, __LINE__); // "..\int\WINDOW.C", 1166
            } else {
                substringList = (char**)mymalloc(sizeof(*substringList), __FILE__, __LINE__); // "..\int\WINDOW.C", 1167
            }

            char* substring = (char*)mymalloc(pch - start + 1, __FILE__, __LINE__); // "..\int\WINDOW.C", 1169
            strncpy(substring, start, pch - start);
            substring[pch - start] = '\0';

            substringList[substringListLength] = substring;

            while (*pch == ' ') {
                pch++;
            }

            v1 = 0;
            start = pch;
            substringListLength++;
        }
    }

    if (start != pch) {
        if (substringList != NULL) {
            substringList = (char**)myrealloc(substringList, sizeof(*substringList) * (substringListLength + 1), __FILE__, __LINE__); // "..\int\WINDOW.C", 1184
        } else {
            substringList = (char**)mymalloc(sizeof(*substringList), __FILE__, __LINE__); // "..\int\WINDOW.C", 1185
        }

        char* substring = (char*)mymalloc(pch - start + 1, __FILE__, __LINE__); // "..\int\WINDOW.C", 1169
        strncpy(substring, start, pch - start);
        substring[pch - start] = '\0';

        substringList[substringListLength] = substring;
        substringListLength++;
    }

    *substringListLengthPtr = substringListLength;

    return substringList;
}

// 0x4B880C
void windowFreeWordList(char** substringList, int substringListLength)
{
    if (substringList == NULL) {
        return;
    }

    for (int index = 0; index < substringListLength; index++) {
        myfree(substringList[index], __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1200
    }

    myfree(substringList, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1201
}

// Renders multiline string in the specified bounding box.
//
// 0x4B8854
void windowWrapLineWithSpacing(int win, char* string, int width, int height, int x, int y, int flags, int textAlignment, int a9)
{
    if (string == NULL) {
        return;
    }

    int substringListLength;
    char** substringList = windowWordWrap(string, width, 0, &substringListLength);

    for (int index = 0; index < substringListLength; index++) {
        int v1 = y + index * (a9 + text_height());
        windowPrintBuf(win, substringList[index], strlen(substringList[index]), width, height + y, x, v1, flags, textAlignment);
    }

    windowFreeWordList(substringList, substringListLength);
}

// Renders multiline string in the specified bounding box.
//
// 0x4B88FC
void windowWrapLine(int win, char* string, int width, int height, int x, int y, int flags, int textAlignment)
{
    windowWrapLineWithSpacing(win, string, width, height, x, y, flags, textAlignment, 0);
}

// 0x4B8920
bool windowPrintRect(char* string, int a2, int textAlignment)
{
    if (currentWindow == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    int width = (int)(a2 * managedWindow->field_54);
    int height = win_height(managedWindow->window);
    int x = managedWindow->field_44;
    int y = managedWindow->field_48;
    int flags = windowGetTextColor() | 0x2000000;

    // NOTE: Uninline.
    windowWrapLine(managedWindow->window, string, width, height, x, y, flags, textAlignment);

    return true;
}

// 0x4B89B0
bool windowFormatMessage(char* string, int x, int y, int width, int height, int textAlignment)
{
    ManagedWindow* managedWindow = &(windows[currentWindow]);
    int flags = windowGetTextColor() | 0x2000000;

    // NOTE: Uninline.
    windowWrapLine(managedWindow->window, string, width, height, x, y, flags, textAlignment);

    return true;
}

// NOTE: Unused.
//
// 0x4B8A14
int windowFormatMessageColor(char* string, int x, int y, int width, int height, int textAlignment, int flags)
{
    windowWrapLine(windows[currentWindow].window, string, width, height, x, y, flags, textAlignment);

    return 1;
}

// 0x4B8A60
bool windowPrint(char* string, int a2, int x, int y, int a5)
{
    ManagedWindow* managedWindow = &(windows[currentWindow]);
    x = (int)(x * managedWindow->field_54);
    y = (int)(y * managedWindow->field_58);

    win_print(managedWindow->window, string, a2, x, y, a5);

    return true;
}

// NOTE: Unused.
//
// 0x4B8ADC
int windowPrintFont(char* string, int a2, int x, int y, int a5, int font)
{
    int oldFont;

    oldFont = text_curr();
    text_font(font);

    windowPrint(string, a2, x, y, a5);

    text_font(oldFont);

    return 1;
}

// 0x4B8B10
void displayInWindow(unsigned char* data, int width, int height, int pitch)
{
    if (displayFunc != NULL) {
        // NOTE: The second parameter is unclear as there is no distinction
        // between address of entire window struct and it's name (since it's the
        // first field). I bet on name since it matches WindowDeleteCallback,
        // which accepts window index and window name as seen at 0x4B7927).
        displayFunc(currentWindow,
            windows[currentWindow].name,
            data,
            width,
            height);
    }

    if (width == pitch) {
        // NOTE: Uninline.
        if (pitch == windowWidth() && height == windowHeight()) {
            // NOTE: Uninline.
            unsigned char* windowBuffer = windowGetBuffer();
            memcpy(windowBuffer, data, height * width);
        } else {
            // NOTE: Uninline.
            unsigned char* windowBuffer = windowGetBuffer();
            drawScaledBuf(windowBuffer, windowWidth(), windowHeight(), data, width, height);
        }
    } else {
        // NOTE: Uninline.
        unsigned char* windowBuffer = windowGetBuffer();
        drawScaled(windowBuffer,
            windowWidth(),
            windowHeight(),
            windowWidth(),
            data,
            width,
            height,
            pitch);
    }
}

// 0x4B8C68
void displayFile(char* fileName)
{
    int width;
    int height;
    unsigned char* data = loadDataFile(fileName, &width, &height);
    if (data != NULL) {
        displayInWindow(data, width, height, width);
        myfree(data, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1294
    }
}

// 0x4B8CA8
void displayFileRaw(char* fileName)
{
    int width;
    int height;
    unsigned char* data = loadRawDataFile(fileName, &width, &height);
    if (data != NULL) {
        displayInWindow(data, width, height, width);
        myfree(data, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1305
    }
}

// NOTE: Unused.
//
// 0x4B8E0C
int windowDisplayRaw(char* fileName)
{
    int imageWidth;
    int imageHeight;
    unsigned char* imageData;

    imageData = loadDataFile(fileName, &imageWidth, &imageHeight);
    if (imageData == NULL) {
        return 0;
    }

    displayInWindow(imageData, imageWidth, imageHeight, imageWidth);

    myfree(imageData, __FILE__, __LINE__); // "..\int\WINDOW.C", 1363

    return 1;
}

// 0x4B8E50
bool windowDisplay(char* fileName, int x, int y, int width, int height)
{
    int imageWidth;
    int imageHeight;
    unsigned char* imageData = loadDataFile(fileName, &imageWidth, &imageHeight);
    if (imageData == NULL) {
        return false;
    }

    windowDisplayBuf(imageData, imageWidth, imageHeight, x, y, width, height);

    myfree(imageData, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1376

    return true;
}

// NOTE: Unused
//
// 0x4B8EA0
int windowDisplayScaled(char* fileName, int x, int y, int width, int height)
{
    int imageWidth;
    int imageHeight;
    unsigned char* imageData;

    imageData = loadDataFile(fileName, &imageWidth, &imageHeight);
    if (imageData == NULL) {
        return 0;
    }

    windowDisplayBufScaled(imageData, imageWidth, imageHeight, x, y, width, height);

    myfree(imageData, __FILE__, __LINE__); // "..\int\WINDOW.C", 1389

    return 1;
}

// 0x4B8EF0
bool windowDisplayBuf(unsigned char* src, int srcWidth, int srcHeight, int destX, int destY, int destWidth, int destHeight)
{
    ManagedWindow* managedWindow = &(windows[currentWindow]);
    unsigned char* windowBuffer = win_get_buf(managedWindow->window);

    buf_to_buf(src,
        destWidth,
        destHeight,
        srcWidth,
        windowBuffer + managedWindow->width * destY + destX,
        managedWindow->width);

    return true;
}

// NOTE: Unused.
//
// 0x4B8F64
int windowDisplayTransBuf(unsigned char* src, int srcWidth, int srcHeight, int destX, int destY, int destWidth, int destHeight)
{
    unsigned char* windowBuffer;

    windowBuffer = win_get_buf(windows[currentWindow].window);

    trans_buf_to_buf(src,
        destWidth,
        destHeight,
        srcWidth,
        windowBuffer + destY * windows[currentWindow].width + destX,
        windows[currentWindow].width);

    return 1;
}

// NOTE: Unused.
//
// 0x4B8FD8
int windowDisplayBufScaled(unsigned char* src, int srcWidth, int srcHeight, int destX, int destY, int destWidth, int destHeight)
{
    unsigned char* windowBuffer;

    windowBuffer = win_get_buf(windows[currentWindow].window);
    drawScaled(windowBuffer + destY * windows[currentWindow].width + destX,
        destWidth,
        destHeight,
        windows[currentWindow].width,
        src,
        srcWidth,
        srcHeight,
        srcWidth);

    return 1;
}

// 0x4B9048
int windowGetXres()
{
    return xres;
}

// 0x4B9050
int windowGetYres()
{
    return yres;
}

// 0x4B9058
static void removeProgramReferences(Program* program)
{
    for (int index = 0; index < MANAGED_WINDOW_COUNT; index++) {
        ManagedWindow* managedWindow = &(windows[index]);
        if (managedWindow->window != -1) {
            for (int index = 0; index < managedWindow->buttonsLength; index++) {
                ManagedButton* managedButton = &(managedWindow->buttons[index]);
                if (program == managedButton->program) {
                    managedButton->program = NULL;
                    managedButton->procs[MANAGED_BUTTON_MOUSE_EVENT_ENTER] = 0;
                    managedButton->procs[MANAGED_BUTTON_MOUSE_EVENT_EXIT] = 0;
                    managedButton->procs[MANAGED_BUTTON_MOUSE_EVENT_BUTTON_DOWN] = 0;
                    managedButton->procs[MANAGED_BUTTON_MOUSE_EVENT_BUTTON_UP] = 0;
                }
            }

            for (int index = 0; index < managedWindow->regionsLength; index++) {
                Region* region = managedWindow->regions[index];
                if (region != NULL) {
                    if (program == region->program) {
                        region->program = NULL;
                        region->procs[1] = 0;
                        region->procs[0] = 0;
                        region->procs[3] = 0;
                        region->procs[2] = 0;
                    }
                }
            }
        }
    }
}

// 0x4B9190
void initWindow(int resolution, int a2)
{
    char err[MAX_PATH];
    int rc;
    int i, j;

    interpretRegisterProgramDeleteCallback(removeProgramReferences);

    currentTextColorR = 0;
    currentTextColorG = 0;
    currentTextColorB = 0;
    currentHighlightColorR = 0;
    currentHighlightColorG = 0;
    currentTextFlags = 0x2010000;

    yres = sizes[resolution].height; // screen height
    currentHighlightColorB = 0;
    xres = sizes[resolution].width; // screen width

    for (int i = 0; i < MANAGED_WINDOW_COUNT; i++) {
        windows[i].window = -1;
    }

    rc = win_init(gfx_init[resolution], GNW95_reset_mode, a2);
    if (rc != WINDOW_MANAGER_OK) {
        switch (rc) {
        case WINDOW_MANAGER_ERR_INITIALIZING_VIDEO_MODE:
            sprintf(err, "Error initializing video mode %dx%d\n", xres, yres);
            GNWSystemError(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_NO_MEMORY:
            sprintf(err, "Not enough memory to initialize video mode\n");
            GNWSystemError(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_INITIALIZING_TEXT_FONTS:
            sprintf(err, "Couldn't find/load text fonts\n");
            GNWSystemError(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_WINDOW_SYSTEM_ALREADY_INITIALIZED:
            sprintf(err, "Attempt to initialize window system twice\n");
            GNWSystemError(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_WINDOW_SYSTEM_NOT_INITIALIZED:
            sprintf(err, "Window system not initialized\n");
            GNWSystemError(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_CURRENT_WINDOWS_TOO_BIG:
            sprintf(err, "Current windows are too big for new resolution\n");
            GNWSystemError(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_INITIALIZING_DEFAULT_DATABASE:
            sprintf(err, "Error initializing default database.\n");
            GNWSystemError(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_8:
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_ALREADY_RUNNING:
            sprintf(err, "Program already running.\n");
            GNWSystemError(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_TITLE_NOT_SET:
            sprintf(err, "Program title not set.\n");
            GNWSystemError(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_INITIALIZING_INPUT:
            sprintf(err, "Failure initializing input devices.\n");
            GNWSystemError(err);
            exit(1);
            break;
        default:
            sprintf(err, "Unknown error code %d\n", rc);
            GNWSystemError(err);
            exit(1);
            break;
        }
    }

    currentFont = 100;
    text_font(100);

    initMousemgr();

    mousemgrSetNameMangler(interpretMangleName);

    for (i = 0; i < 64; i++) {
        for (j = 0; j < 256; j++) {
            alphaBlendTable[(i << 8) + j] = ((i * j) >> 9);
        }
    }
}

// NOTE: Unused.
//
// 0x4B9454
void windowSetWindowFuncs(ManagedWindowCreateCallback* createCallback, ManagedWindowSelectFunc* selectCallback, WindowDeleteCallback* deleteCallback, DisplayInWindowCallback* displayCallback)
{
    if (createCallback != NULL) {
        createWindowFunc = createCallback;
    }

    if (selectCallback != NULL) {
        selectWindowFunc = selectCallback;
    }

    if (deleteCallback != NULL) {
        deleteWindowFunc = deleteCallback;
    }

    if (displayCallback != NULL) {
        displayFunc = displayCallback;
    }
}

// 0x4B947C
void windowClose()
{
    for (int index = 0; index < MANAGED_WINDOW_COUNT; index++) {
        ManagedWindow* managedWindow = &(windows[index]);
        if (managedWindow->window != -1) {
            deleteWindow(managedWindow->name);
        }
    }

    if (inputFunc != NULL) {
        myfree(inputFunc, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1573
    }

    mousemgrClose();
    db_exit();
    win_exit();
}

// Deletes button with the specified name or all buttons if it's NULL.
//
// 0x4B9548
bool windowDeleteButton(const char* buttonName)
{
    if (currentWindow != -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    if (managedWindow->buttonsLength == 0) {
        return false;
    }

    if (buttonName == NULL) {
        for (int index = 0; index < managedWindow->buttonsLength; index++) {
            ManagedButton* managedButton = &(managedWindow->buttons[index]);
            win_delete_button(managedButton->btn);

            if (managedButton->hover != NULL) {
                myfree(managedButton->hover, __FILE__, __LINE__); // "..\int\WINDOW.C", 1648
                managedButton->hover = NULL;
            }

            if (managedButton->field_4C != NULL) {
                myfree(managedButton->field_4C, __FILE__, __LINE__); // "..\int\WINDOW.C", 1649
                managedButton->field_4C = NULL;
            }

            if (managedButton->pressed != NULL) {
                myfree(managedButton->pressed, __FILE__, __LINE__); // "..\int\WINDOW.C", 1650
                managedButton->pressed = NULL;
            }

            if (managedButton->normal != NULL) {
                myfree(managedButton->normal, __FILE__, __LINE__); // "..\int\WINDOW.C", 1651
                managedButton->normal = NULL;
            }

            if (managedButton->field_50 != NULL) {
                myfree(managedButton->normal, __FILE__, __LINE__); // "..\int\WINDOW.C", 1652
                managedButton->field_50 = NULL;
            }
        }

        myfree(managedWindow->buttons, __FILE__, __LINE__); // "..\int\WINDOW.C", 1654
        managedWindow->buttons = NULL;
        managedWindow->buttonsLength = 0;

        return true;
    }

    for (int index = 0; index < managedWindow->buttonsLength; index++) {
        ManagedButton* managedButton = &(managedWindow->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            win_delete_button(managedButton->btn);

            if (managedButton->hover != NULL) {
                myfree(managedButton->hover, __FILE__, __LINE__); // "..\int\WINDOW.C", 1665
                managedButton->hover = NULL;
            }

            if (managedButton->field_4C != NULL) {
                myfree(managedButton->field_4C, __FILE__, __LINE__); // "..\int\WINDOW.C", 1666
                managedButton->field_4C = NULL;
            }

            if (managedButton->pressed != NULL) {
                myfree(managedButton->pressed, __FILE__, __LINE__); // "..\int\WINDOW.C", 1667
                managedButton->pressed = NULL;
            }

            if (managedButton->normal != NULL) {
                myfree(managedButton->normal, __FILE__, __LINE__); // "..\int\WINDOW.C", 1668
                managedButton->normal = NULL;
            }

            // FIXME: Probably leaking field_50. It's freed when deleting all
            // buttons, but not the specific button.

            if (index != managedWindow->buttonsLength - 1) {
                // Move remaining buttons up. The last item is not reclaimed.
                memcpy(managedWindow->buttons + index, managedWindow->buttons + index + 1, sizeof(*(managedWindow->buttons)) * (managedWindow->buttonsLength - index - 1));
            }

            managedWindow->buttonsLength--;
            if (managedWindow->buttonsLength == 0) {
                myfree(managedWindow->buttons, __FILE__, __LINE__); // "..\int\WINDOW.C", 1672
                managedWindow->buttons = NULL;
            }

            return true;
        }
    }

    return false;
}

// NOTE: Unused.
//
// 0x4B97F8
void windowEnableButton(const char* buttonName, int enabled)
{
    int index;

    for (index = 0; index < windows[currentWindow].buttonsLength; index++) {
        if (stricmp(windows[currentWindow].buttons[index].name, buttonName) == 0) {
            if (enabled) {
                if (soundPressFunc != NULL || soundReleaseFunc != NULL) {
                    win_register_button_sound_func(windows[currentWindow].buttons[index].btn, soundPressFunc, soundReleaseFunc);
                }

                windows[currentWindow].buttons[index].flags &= ~0x02;
            } else {
                if (soundDisableFunc != NULL) {
                    win_register_button_sound_func(windows[currentWindow].buttons[index].btn, soundDisableFunc, NULL);
                }

                windows[currentWindow].buttons[index].flags |= 0x02;
            }
        }
    }
}

// NOTE: Unused.
//
// 0x4B98C4
int windowGetButtonID(const char* buttonName)
{
    int index;

    for (index = 0; index < windows[currentWindow].buttonsLength; index++) {
        if (stricmp(windows[currentWindow].buttons[index].name, buttonName) == 0) {
            return windows[currentWindow].buttons[index].btn;
        }
    }

    return -1;
}

// 0x4B9928
bool windowSetButtonFlag(const char* buttonName, int value)
{
    if (currentWindow != -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    if (managedWindow->buttons == NULL) {
        return false;
    }

    for (int index = 0; index < managedWindow->buttonsLength; index++) {
        ManagedButton* managedButton = &(managedWindow->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            managedButton->flags |= value;
            return true;
        }
    }

    return false;
}

// NOTE: Unused.
//
// 0x4B99B4
void windowRegisterButtonSoundFunc(ButtonCallback* soundPressFunc, ButtonCallback* soundReleaseFunc, ButtonCallback* soundDisableFunc)
{
    soundPressFunc = soundPressFunc;
    soundReleaseFunc = soundReleaseFunc;
    soundDisableFunc = soundDisableFunc;
}

// 0x4B99C8
bool windowAddButton(const char* buttonName, int x, int y, int width, int height, int flags)
{
    if (currentWindow == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    int index;
    for (index = 0; index < managedWindow->buttonsLength; index++) {
        ManagedButton* managedButton = &(managedWindow->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            win_delete_button(managedButton->btn);

            if (managedButton->hover != NULL) {
                myfree(managedButton->hover, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1748
                managedButton->hover = NULL;
            }

            if (managedButton->field_4C != NULL) {
                myfree(managedButton->field_4C, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1749
                managedButton->field_4C = NULL;
            }

            if (managedButton->pressed != NULL) {
                myfree(managedButton->pressed, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1750
                managedButton->pressed = NULL;
            }

            if (managedButton->normal != NULL) {
                myfree(managedButton->normal, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1751
                managedButton->normal = NULL;
            }

            break;
        }
    }

    if (index == managedWindow->buttonsLength) {
        if (managedWindow->buttons == NULL) {
            managedWindow->buttons = (ManagedButton*)mymalloc(sizeof(*managedWindow->buttons), __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1758
        } else {
            managedWindow->buttons = (ManagedButton*)myrealloc(managedWindow->buttons, sizeof(*managedWindow->buttons) * (managedWindow->buttonsLength + 1), __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1761
        }
        managedWindow->buttonsLength += 1;
    }

    x = (int)(x * managedWindow->field_54);
    y = (int)(y * managedWindow->field_58);
    width = (int)(width * managedWindow->field_54);
    height = (int)(height * managedWindow->field_58);

    ManagedButton* managedButton = &(managedWindow->buttons[index]);
    strncpy(managedButton->name, buttonName, 31);
    managedButton->program = NULL;
    managedButton->flags = 0;
    managedButton->procs[MANAGED_BUTTON_MOUSE_EVENT_BUTTON_UP] = 0;
    managedButton->rightProcs[MANAGED_BUTTON_RIGHT_MOUSE_EVENT_BUTTON_UP] = 0;
    managedButton->mouseEventCallback = NULL;
    managedButton->rightMouseEventCallback = NULL;
    managedButton->field_50 = 0;
    managedButton->procs[MANAGED_BUTTON_MOUSE_EVENT_BUTTON_DOWN] = 0;
    managedButton->procs[MANAGED_BUTTON_MOUSE_EVENT_EXIT] = 0;
    managedButton->procs[MANAGED_BUTTON_MOUSE_EVENT_ENTER] = 0;
    managedButton->rightProcs[MANAGED_BUTTON_RIGHT_MOUSE_EVENT_BUTTON_DOWN] = 0;
    managedButton->width = width;
    managedButton->height = height;
    managedButton->x = x;
    managedButton->y = y;

    unsigned char* normal = (unsigned char*)mymalloc(width * height, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1792
    unsigned char* pressed = (unsigned char*)mymalloc(width * height, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1793

    if ((flags & BUTTON_FLAG_TRANSPARENT) != 0) {
        memset(normal, 0, width * height);
        memset(pressed, 0, width * height);
    } else {
        setButtonGFX(width, height, normal, pressed, NULL);
    }

    managedButton->btn = win_register_button(
        managedWindow->window,
        x,
        y,
        width,
        height,
        -1,
        -1,
        -1,
        -1,
        normal,
        pressed,
        NULL,
        flags);

    if (soundPressFunc != NULL || soundReleaseFunc != NULL) {
        win_register_button_sound_func(managedButton->btn, soundPressFunc, soundReleaseFunc);
    }

    managedButton->hover = NULL;
    managedButton->pressed = pressed;
    managedButton->normal = normal;
    managedButton->field_18 = flags;
    managedButton->field_4C = NULL;
    win_register_button_func(managedButton->btn, doButtonOn, doButtonOff, doButtonPress, doButtonRelease);
    windowSetButtonFlag(buttonName, 1);

    if ((flags & BUTTON_FLAG_TRANSPARENT) != 0) {
        win_register_button_mask(managedButton->btn, normal);
    }

    return true;
}

// 0x4B9DD0
bool windowAddButtonGfx(const char* buttonName, char* pressedFileName, char* normalFileName, char* hoverFileName)
{
    ManagedWindow* managedWindow = &(windows[currentWindow]);
    for (int index = 0; index < managedWindow->buttonsLength; index++) {
        ManagedButton* managedButton = &(managedWindow->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            int width;
            int height;

            if (pressedFileName != NULL) {
                unsigned char* pressed = loadDataFile(pressedFileName, &width, &height);
                if (pressed != NULL) {
                    drawScaledBuf(managedButton->pressed, managedButton->width, managedButton->height, pressed, width, height);
                    myfree(pressed, __FILE__, __LINE__); // "..\\int\\WINDOW.C, 1834
                }
            }

            if (normalFileName != NULL) {
                unsigned char* normal = loadDataFile(normalFileName, &width, &height);
                if (normal != NULL) {
                    drawScaledBuf(managedButton->normal, managedButton->width, managedButton->height, normal, width, height);
                    myfree(normal, __FILE__, __LINE__); // "..\\int\\WINDOW.C, 1842
                }
            }

            if (hoverFileName != NULL) {
                unsigned char* hover = loadDataFile(normalFileName, &width, &height);
                if (hover != NULL) {
                    if (managedButton->hover == NULL) {
                        managedButton->hover = (unsigned char*)mymalloc(managedButton->height * managedButton->width, __FILE__, __LINE__); // "..\\int\\WINDOW.C, 1849
                    }

                    drawScaledBuf(managedButton->hover, managedButton->width, managedButton->height, hover, width, height);
                    myfree(hover, __FILE__, __LINE__); // "..\\int\\WINDOW.C, 1853
                }
            }

            if ((managedButton->field_18 & 0x20) != 0) {
                win_register_button_mask(managedButton->btn, managedButton->normal);
            }

            win_register_button_image(managedButton->btn, managedButton->normal, managedButton->pressed, managedButton->hover, 0);

            return true;
        }
    }

    return false;
}

// NOTE: Unused.
//
// 0x4B9F40
int windowAddButtonMask(const char* buttonName, unsigned char* buffer)
{
    int index;
    ManagedButton* button;
    unsigned char* copy;

    for (index = 0; index < windows[currentWindow].buttonsLength; index++) {
        button = &(windows[currentWindow].buttons[index]);
        if (stricmp(button->name, buttonName) == 0) {
            copy = (unsigned char*)mymalloc(button->width * button->height, __FILE__, __LINE__); // "..\\int\\WINDOW.C, 1871
            memcpy(copy, buffer, button->width * button->height);
            win_register_button_mask(button->btn, copy);
            button->field_50 = copy;
            return 1;
        }
    }

    return 0;
}

// NOTE: Unused.
//
// 0x4B9FC8
int windowAddButtonBuf(const char* buttonName, unsigned char* normal, unsigned char* pressed, unsigned char* hover, int width, int height, int pitch)
{
    int index;
    ManagedButton* button;

    for (index = 0; index < windows[currentWindow].buttonsLength; index++) {
        button = &(windows[currentWindow].buttons[index]);
        if (stricmp(button->name, buttonName) == 0) {
            if (normal != NULL) {
                memset(button->normal, 0, button->width * button->height);
                drawScaled(button->normal,
                    button->width,
                    button->height,
                    button->width,
                    normal,
                    width,
                    height,
                    pitch);
            }

            if (pressed != NULL) {
                memset(button->pressed, 0, button->width * button->height);
                drawScaled(button->pressed,
                    button->width,
                    button->height,
                    button->width,
                    pressed,
                    width,
                    height,
                    pitch);
            }

            if (hover != NULL) {
                memset(button->hover, 0, button->width * button->height);
                drawScaled(button->hover,
                    button->width,
                    button->height,
                    button->width,
                    hover,
                    width,
                    height,
                    pitch);
            }

            if ((button->field_18 & 0x20) != 0) {
                win_register_button_mask(button->btn, button->normal);
            }

            win_register_button_image(button->btn, button->normal, button->pressed, button->hover, 0);

            return 1;
        }
    }

    return 0;
}

// 0x4BA11C
bool windowAddButtonProc(const char* buttonName, Program* program, int mouseEnterProc, int mouseExitProc, int mouseDownProc, int mouseUpProc)
{
    if (currentWindow == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    if (managedWindow->buttons == NULL) {
        return false;
    }

    for (int index = 0; index < managedWindow->buttonsLength; index++) {
        ManagedButton* managedButton = &(managedWindow->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            managedButton->procs[MANAGED_BUTTON_MOUSE_EVENT_ENTER] = mouseEnterProc;
            managedButton->procs[MANAGED_BUTTON_MOUSE_EVENT_EXIT] = mouseExitProc;
            managedButton->procs[MANAGED_BUTTON_MOUSE_EVENT_BUTTON_DOWN] = mouseDownProc;
            managedButton->procs[MANAGED_BUTTON_MOUSE_EVENT_BUTTON_UP] = mouseUpProc;
            managedButton->program = program;
            return true;
        }
    }

    return false;
}

// 0x4BA1B4
bool windowAddButtonRightProc(const char* buttonName, Program* program, int rightMouseDownProc, int rightMouseUpProc)
{
    if (currentWindow != -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    if (managedWindow->buttons == NULL) {
        return false;
    }

    for (int index = 0; index < managedWindow->buttonsLength; index++) {
        ManagedButton* managedButton = &(managedWindow->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            managedButton->rightProcs[MANAGED_BUTTON_RIGHT_MOUSE_EVENT_BUTTON_UP] = rightMouseUpProc;
            managedButton->rightProcs[MANAGED_BUTTON_RIGHT_MOUSE_EVENT_BUTTON_DOWN] = rightMouseDownProc;
            managedButton->program = program;
            return true;
        }
    }

    return false;
}

// NOTE: Unused.
//
// 0x4BA238
bool windowAddButtonCfunc(const char* buttonName, ManagedButtonMouseEventCallback* callback, void* userData)
{
    if (currentWindow != -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    if (managedWindow->buttons == NULL) {
        return false;
    }

    for (int index = 0; index < managedWindow->buttonsLength; index++) {
        ManagedButton* managedButton = &(managedWindow->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            managedButton->mouseEventCallbackUserData = userData;
            managedButton->mouseEventCallback = callback;
            return true;
        }
    }

    return false;
}

// NOTE: Unused.
//
// 0x4BA2B4
bool windowAddButtonRightCfunc(const char* buttonName, ManagedButtonMouseEventCallback* callback, void* userData)
{
    if (currentWindow != -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    if (managedWindow->buttons == NULL) {
        return false;
    }

    for (int index = 0; index < managedWindow->buttonsLength; index++) {
        ManagedButton* managedButton = &(managedWindow->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            managedButton->rightMouseEventCallback = callback;
            managedButton->rightMouseEventCallbackUserData = userData;
            win_register_right_button(managedButton->btn, -1, -1, doRightButtonPress, doRightButtonRelease);
            return true;
        }
    }

    return false;
}

// 0x4BA34C
bool windowAddButtonText(const char* buttonName, const char* text)
{
    return windowAddButtonTextWithOffsets(buttonName, text, 2, 2, 0, 0);
}

// 0x4BA364
bool windowAddButtonTextWithOffsets(const char* buttonName, const char* text, int pressedImageOffsetX, int pressedImageOffsetY, int normalImageOffsetX, int normalImageOffsetY)
{
    if (currentWindow == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    if (managedWindow->buttons == NULL) {
        return false;
    }

    for (int index = 0; index < managedWindow->buttonsLength; index++) {
        ManagedButton* managedButton = &(managedWindow->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            int normalImageHeight = text_height() + 1;
            int normalImageWidth = text_width(text) + 1;
            unsigned char* buffer = (unsigned char*)mymalloc(normalImageHeight * normalImageWidth, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 2010

            int normalImageX = (managedButton->width - normalImageWidth) / 2 + normalImageOffsetX;
            int normalImageY = (managedButton->height - normalImageHeight) / 2 + normalImageOffsetY;

            if (normalImageX < 0) {
                normalImageWidth -= normalImageX;
                normalImageX = 0;
            }

            if (normalImageX + normalImageWidth >= managedButton->width) {
                normalImageWidth = managedButton->width - normalImageX;
            }

            if (normalImageY < 0) {
                normalImageHeight -= normalImageY;
                normalImageY = 0;
            }

            if (normalImageY + normalImageHeight >= managedButton->height) {
                normalImageHeight = managedButton->height - normalImageY;
            }

            if (managedButton->normal != NULL) {
                buf_to_buf(managedButton->normal + managedButton->width * normalImageY + normalImageX,
                    normalImageWidth,
                    normalImageHeight,
                    managedButton->width,
                    buffer,
                    normalImageWidth);
            } else {
                memset(buffer, 0, normalImageHeight * normalImageWidth);
            }

            text_to_buf(buffer,
                text,
                normalImageWidth,
                normalImageWidth,
                windowGetTextColor() + windowGetTextFlags());

            trans_buf_to_buf(buffer,
                normalImageWidth,
                normalImageHeight,
                normalImageWidth,
                managedButton->normal + managedButton->width * normalImageY + normalImageX,
                managedButton->width);

            int pressedImageWidth = text_width(text) + 1;
            int pressedImageHeight = text_height() + 1;

            int pressedImageX = (managedButton->width - pressedImageWidth) / 2 + pressedImageOffsetX;
            int pressedImageY = (managedButton->height - pressedImageHeight) / 2 + pressedImageOffsetY;

            if (pressedImageX < 0) {
                pressedImageWidth -= pressedImageX;
                pressedImageX = 0;
            }

            if (pressedImageX + pressedImageWidth >= managedButton->width) {
                pressedImageWidth = managedButton->width - pressedImageX;
            }

            if (pressedImageY < 0) {
                pressedImageHeight -= pressedImageY;
                pressedImageY = 0;
            }

            if (pressedImageY + pressedImageHeight >= managedButton->height) {
                pressedImageHeight = managedButton->height - pressedImageY;
            }

            if (managedButton->pressed != NULL) {
                buf_to_buf(managedButton->pressed + managedButton->width * pressedImageY + pressedImageX,
                    pressedImageWidth,
                    pressedImageHeight,
                    managedButton->width,
                    buffer,
                    pressedImageWidth);
            } else {
                memset(buffer, 0, pressedImageHeight * pressedImageWidth);
            }

            text_to_buf(buffer,
                text,
                pressedImageWidth,
                pressedImageWidth,
                windowGetTextColor() + windowGetTextFlags());

            trans_buf_to_buf(buffer,
                pressedImageWidth,
                normalImageHeight,
                normalImageWidth,
                managedButton->pressed + managedButton->width * pressedImageY + pressedImageX,
                managedButton->width);

            myfree(buffer, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 2078

            if ((managedButton->field_18 & 0x20) != 0) {
                win_register_button_mask(managedButton->btn, managedButton->normal);
            }

            win_register_button_image(managedButton->btn, managedButton->normal, managedButton->pressed, managedButton->hover, 0);

            return true;
        }
    }

    return false;
}

// 0x4BA694
bool windowFill(float r, float g, float b)
{
    int colorIndex;
    int wid;

    colorIndex = ((int)(r * 31.0) << 10) | ((int)(g * 31.0) << 5) | (int)(b * 31.0);

    // NOTE: Uninline.
    wid = windowGetGNWID();

    win_fill(wid,
        0,
        0,
        windowWidth(),
        windowHeight(),
        colorTable[colorIndex]);

    return true;
}

// 0x4BA738
bool windowFillRect(int x, int y, int width, int height, float r, float g, float b)
{
    ManagedWindow* managedWindow;
    int colorIndex;
    int wid;

    managedWindow = &(windows[currentWindow]);
    x = (int)(x * managedWindow->field_54);
    y = (int)(y * managedWindow->field_58);
    width = (int)(width * managedWindow->field_54);
    height = (int)(height * managedWindow->field_58);

    colorIndex = ((int)(r * 31.0) << 10) | ((int)(g * 31.0) << 5) | (int)(b * 31.0);

    // NOTE: Uninline.
    wid = windowGetGNWID();

    win_fill(wid,
        x,
        y,
        width,
        height,
        colorTable[colorIndex]);

    return true;
}

// TODO: There is a value returned, not sure which one - could be either
// currentRegionIndex or points array. For now it can be safely ignored since
// the only caller of this function is op_addregion, which ignores the returned
// value.
//
// 0x4BA844
void windowEndRegion()
{
    ManagedWindow* managedWindow = &(windows[currentWindow]);
    Region* region = managedWindow->regions[managedWindow->currentRegionIndex];
    windowAddRegionPoint(region->points->x, region->points->y, false);
    regionSetBound(region);
}

// NOTE: Unused.
//
// 0x4BA8A4
void* windowRegionGetUserData(const char* windowRegionName)
{
    int index;
    char* regionName;

    if (currentWindow == -1) {
        return NULL;
    }

    for (index = 0; index < windows[currentWindow].regionsLength; index++) {
        regionName = windows[currentWindow].regions[index]->name;
        if (stricmp(regionName, windowRegionName) == 0) {
            return regionGetUserData(windows[currentWindow].regions[index]);
        }
    }

    return NULL;
}

// NOTE: Unused.
//
// 0x4BA914
void windowRegionSetUserData(const char* windowRegionName, void* userData)
{
    int index;
    char* regionName;

    if (currentWindow == -1) {
        return;
    }

    for (index = 0; index < windows[currentWindow].regionsLength; index++) {
        regionName = windows[currentWindow].regions[index]->name;
        if (stricmp(regionName, windowRegionName) == 0) {
            regionSetUserData(windows[currentWindow].regions[index], userData);
            return;
        }
    }
}

// 0x4BA988
bool windowCheckRegionExists(const char* regionName)
{
    if (currentWindow == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    if (managedWindow->window == -1) {
        return false;
    }

    for (int index = 0; index < managedWindow->regionsLength; index++) {
        Region* region = managedWindow->regions[index];
        if (region != NULL) {
            if (stricmp(regionGetName(region), regionName) == 0) {
                return true;
            }
        }
    }

    return false;
}

// 0x4BA9FC
bool windowStartRegion(int initialCapacity)
{
    if (currentWindow == -1) {
        return false;
    }

    int newRegionIndex;
    ManagedWindow* managedWindow = &(windows[currentWindow]);
    if (managedWindow->regions == NULL) {
        managedWindow->regions = (Region**)mymalloc(sizeof(&(managedWindow->regions)), __FILE__, __LINE__); // "..\int\WINDOW.C", 2167
        managedWindow->regionsLength = 1;
        newRegionIndex = 0;
    } else {
        newRegionIndex = 0;
        for (int index = 0; index < managedWindow->regionsLength; index++) {
            if (managedWindow->regions[index] == NULL) {
                break;
            }
            newRegionIndex++;
        }

        if (newRegionIndex == managedWindow->regionsLength) {
            managedWindow->regions = (Region**)myrealloc(managedWindow->regions, sizeof(&(managedWindow->regions)) * (managedWindow->regionsLength + 1), __FILE__, __LINE__); // "..\int\WINDOW.C", 2178
            managedWindow->regionsLength++;
        }
    }

    Region* newRegion;
    if (initialCapacity != 0) {
        newRegion = allocateRegion(initialCapacity + 1);
    } else {
        newRegion = NULL;
    }

    managedWindow->regions[newRegionIndex] = newRegion;
    managedWindow->currentRegionIndex = newRegionIndex;

    return true;
}

// 0x4BAB68
bool windowAddRegionPoint(int x, int y, bool a3)
{
    if (currentWindow == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    Region* region = managedWindow->regions[managedWindow->currentRegionIndex];
    if (region == NULL) {
        region = managedWindow->regions[managedWindow->currentRegionIndex] = allocateRegion(1);
    }

    if (a3) {
        x = (int)(x * managedWindow->field_54);
        y = (int)(y * managedWindow->field_58);
    }

    regionAddPoint(region, x, y);

    return true;
}

// NOTE: Unused.
//
// 0x4BAC5
int windowAddRegionRect(int a1, int a2, int a3, int a4, int a5)
{
    windowAddRegionPoint(a1, a2, a5);
    windowAddRegionPoint(a3, a2, a5);
    windowAddRegionPoint(a3, a4, a5);
    windowAddRegionPoint(a1, a4, a5);

    return 0;
}

// NOTE: Unused.
//
// 0x4BACA0
int windowAddRegionCfunc(const char* regionName, RegionMouseEventCallback* callback, void* userData)
{
    int index;
    Region* region;

    if (currentWindow == -1) {
        return 0;
    }

    for (index = 0; index < windows[currentWindow].regionsLength; index++) {
        region = windows[currentWindow].regions[index];
        if (region != NULL && stricmp(region->name, regionName) == 0) {
            region->mouseEventCallback = callback;
            region->mouseEventCallbackUserData = userData;
            return 1;
        }
    }

    return 0;
}

// NOTE: Unused.
//
// 0x4BAD30
int windowAddRegionRightCfunc(const char* regionName, RegionMouseEventCallback* callback, void* userData)
{
    int index;
    Region* region;

    if (currentWindow == -1) {
        return 0;
    }

    for (index = 0; index < windows[currentWindow].regionsLength; index++) {
        region = windows[currentWindow].regions[index];
        if (region != NULL && stricmp(region->name, regionName) == 0) {
            region->rightMouseEventCallback = callback;
            region->rightMouseEventCallbackUserData = userData;
            return 1;
        }
    }

    return 0;
}

// 0x4BADC0
bool windowAddRegionProc(const char* regionName, Program* program, int a3, int a4, int a5, int a6)
{
    if (currentWindow == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    for (int index = 0; index < managedWindow->regionsLength; index++) {
        Region* region = managedWindow->regions[index];
        if (region != NULL) {
            if (stricmp(region->name, regionName) == 0) {
                region->procs[2] = a3;
                region->procs[3] = a4;
                region->procs[0] = a5;
                region->procs[1] = a6;
                region->program = program;
                return true;
            }
        }
    }

    return false;
}

// 0x4BAE8C
bool windowAddRegionRightProc(const char* regionName, Program* program, int a3, int a4)
{
    if (currentWindow == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    for (int index = 0; index < managedWindow->regionsLength; index++) {
        Region* region = managedWindow->regions[index];
        if (region != NULL) {
            if (stricmp(region->name, regionName) == 0) {
                region->rightProcs[0] = a3;
                region->rightProcs[1] = a4;
                region->program = program;
                return true;
            }
        }
    }

    return false;
}

// 0x4BAF2C
bool windowSetRegionFlag(const char* regionName, int value)
{
    if (currentWindow != -1) {
        ManagedWindow* managedWindow = &(windows[currentWindow]);
        for (int index = 0; index < managedWindow->regionsLength; index++) {
            Region* region = managedWindow->regions[index];
            if (region != NULL) {
                if (stricmp(region->name, regionName) == 0) {
                    regionSetFlag(region, value);
                    return true;
                }
            }
        }
    }

    return false;
}

// 0x4BAFA8
bool windowAddRegionName(const char* regionName)
{
    if (currentWindow == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    Region* region = managedWindow->regions[managedWindow->currentRegionIndex];
    if (region == NULL) {
        return false;
    }

    for (int index = 0; index < managedWindow->regionsLength; index++) {
        if (index != managedWindow->currentRegionIndex) {
            Region* other = managedWindow->regions[index];
            if (other != NULL) {
                if (stricmp(regionGetName(other), regionName) == 0) {
                    regionDelete(other);
                    managedWindow->regions[index] = NULL;
                    break;
                }
            }
        }
    }

    regionAddName(region, regionName);

    return true;
}

// Delete region with the specified name or all regions if it's NULL.
//
// 0x4BB0A8
bool windowDeleteRegion(const char* regionName)
{
    if (currentWindow == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(windows[currentWindow]);
    if (managedWindow->window == -1) {
        return false;
    }

    if (regionName != NULL) {
        for (int index = 0; index < managedWindow->regionsLength; index++) {
            Region* region = managedWindow->regions[index];
            if (region != NULL) {
                if (stricmp(regionGetName(region), regionName) == 0) {
                    regionDelete(region);
                    managedWindow->regions[index] = NULL;
                    managedWindow->field_38++;
                    return true;
                }
            }
        }
        return false;
    }

    managedWindow->field_38++;

    if (managedWindow->regions != NULL) {
        for (int index = 0; index < managedWindow->regionsLength; index++) {
            Region* region = managedWindow->regions[index];
            if (region != NULL) {
                regionDelete(region);
            }
        }

        myfree(managedWindow->regions, __FILE__, __LINE__); // "..\int\WINDOW.C", 2353

        managedWindow->regions = NULL;
        managedWindow->regionsLength = 0;
    }

    return true;
}

// 0x4BB220
void updateWindows()
{
    movieUpdate();
    mousemgrUpdate();
    checkAllRegions();
    update_widgets();
}

// 0x4BB234
int windowMoviePlaying()
{
    return moviePlaying();
}

// 0x4BB23C
bool windowSetMovieFlags(int flags)
{
    if (movieSetFlags(flags) != 0) {
        return false;
    }

    return true;
}

// 0x4BB24C
bool windowPlayMovie(char* filePath)
{
    int wid;

    // NOTE: Uninline.
    wid = windowGetGNWID();

    if (movieRun(wid, filePath) != 0) {
        return false;
    }

    return true;
}

// 0x4BB280
bool windowPlayMovieRect(char* filePath, int a2, int a3, int a4, int a5)
{
    int wid;

    // NOTE: Uninline.
    wid = windowGetGNWID();

    if (movieRunRect(wid, filePath, a2, a3, a4, a5) != 0) {
        return false;
    }

    return true;
}

// 0x4BB2C4
void windowStopMovie()
{
    movieStop();
}

// 0x4BB3A8
void drawScaled(unsigned char* dest, int destWidth, int destHeight, int destPitch, unsigned char* src, int srcWidth, int srcHeight, int srcPitch)
{
    if (destWidth == srcWidth && destHeight == srcHeight) {
        buf_to_buf(src, srcWidth, srcHeight, srcPitch, dest, destPitch);
        return;
    }

    int incrementX = (srcWidth << 16) / destWidth;
    int incrementY = (srcHeight << 16) / destHeight;
    int stepX = incrementX >> 16;
    int stepY = incrementY >> 16;
    int destSkip = destPitch - destWidth;
    int srcSkip = stepY * srcPitch;

    if (srcSkip != 0) {
        // Downscaling.
        int srcPosY = 0;
        for (int y = 0; y < destHeight; y++) {
            int srcPosX = 0;
            int offset = 0;
            for (int x = 0; x < destWidth; x++) {
                *dest++ = src[offset];
                offset += stepX;

                srcPosX += incrementX;
                if (srcPosX >= 0x10000) {
                    srcPosX &= 0xFFFF;
                }
            }

            dest += destSkip;
            src += srcSkip;

            srcPosY += stepY;
            if (srcPosY >= 0x10000) {
                srcPosY &= 0xFFFF;
                src += srcPitch;
            }
        }
    } else {
        // Upscaling.
        int y = 0;
        int srcPosY = 0;
        while (y < destHeight) {
            unsigned char* destPtr = dest;

            int srcPosX = 0;
            int offset = 0;
            for (int x = 0; x < destWidth; x++) {
                *dest++ = src[offset];
                offset += stepX;

                srcPosX += stepX;
                if (srcPosX >= 0x10000) {
                    offset++;
                    srcPosX &= 0xFFFF;
                }
            }

            y++;
            if (y < destHeight) {
                dest += destSkip;
                srcPosY += incrementY;

                while (y < destHeight && srcPosY < 0x10000) {
                    memcpy(dest, destPtr, destWidth);
                    dest += destWidth;
                    srcPosY += incrementY;
                    y++;
                }

                srcPosY &= 0xFFFF;
                src += srcPitch;
            }
        }
    }
}

// 0x4BB5D0
void drawScaledBuf(unsigned char* dest, int destWidth, int destHeight, unsigned char* src, int srcWidth, int srcHeight)
{
    if (destWidth == srcWidth && destHeight == srcHeight) {
        memcpy(dest, src, srcWidth * srcHeight);
        return;
    }

    int incrementX = (srcWidth << 16) / destWidth;
    int incrementY = (srcHeight << 16) / destHeight;
    int stepX = incrementX >> 16;
    int stepY = incrementY >> 16;
    int srcSkip = stepY * srcWidth;

    if (srcSkip != 0) {
        // Downscaling.
        int srcPosY = 0;
        for (int y = 0; y < destHeight; y++) {
            int srcPosX = 0;
            int offset = 0;
            for (int x = 0; x < destWidth; x++) {
                *dest++ = src[offset];
                offset += stepX;

                srcPosX += incrementX;
                if (srcPosX >= 0x10000) {
                    srcPosX &= 0xFFFF;
                }
            }

            src += srcSkip;

            srcPosY += stepY;
            if (srcPosY >= 0x10000) {
                srcPosY &= 0xFFFF;
                src += srcWidth;
            }
        }
    } else {
        // Upscaling.
        int y = 0;
        int srcPosY = 0;
        while (y < destHeight) {
            unsigned char* destPtr = dest;

            int srcPosX = 0;
            int offset = 0;
            for (int x = 0; x < destWidth; x++) {
                *dest++ = src[offset];
                offset += stepX;

                srcPosX += stepX;
                if (srcPosX >= 0x10000) {
                    offset++;
                    srcPosX &= 0xFFFF;
                }
            }

            y++;
            if (y < destHeight) {
                srcPosY += incrementY;

                while (y < destHeight && srcPosY < 0x10000) {
                    memcpy(dest, destPtr, destWidth);
                    dest += destWidth;
                    srcPosY += incrementY;
                    y++;
                }

                srcPosY &= 0xFFFF;
                src += srcWidth;
            }
        }
    }
}

// 0x4BB7D8
void alphaBltBuf(unsigned char* src, int srcWidth, int srcHeight, int srcPitch, unsigned char* alphaWindowBuffer, unsigned char* alphaBuffer, unsigned char* dest, int destPitch)
{
    for (int y = 0; y < srcHeight; y++) {
        for (int x = 0; x < srcWidth; x++) {
            int rle = (alphaBuffer[0] << 8) + alphaBuffer[1];
            alphaBuffer += 2;
            if ((rle & 0x8000) != 0) {
                rle &= ~0x8000;
            } else if ((rle & 0x4000) != 0) {
                rle &= ~0x4000;
                memcpy(dest, src, rle);
            } else {
                unsigned char* destPtr = dest;
                unsigned char* srcPtr = src;
                unsigned char* alphaWindowBufferPtr = alphaWindowBuffer;
                unsigned char* alphaBufferPtr = alphaBuffer;
                for (int index = 0; index < rle; index++) {
                    // TODO: Check.
                    unsigned char* v1 = &(cmap[*srcPtr * 3]);
                    unsigned char* v2 = &(cmap[*alphaWindowBufferPtr * 3]);
                    unsigned char alpha = *alphaBufferPtr;

                    // NOTE: Original code is slightly different.
                    unsigned int r = alphaBlendTable[(v1[0] << 8) | alpha] + alphaBlendTable[(v2[0] << 8) | alpha];
                    unsigned int g = alphaBlendTable[(v1[1] << 8) | alpha] + alphaBlendTable[(v2[1] << 8) | alpha];
                    unsigned int b = alphaBlendTable[(v1[2] << 8) | alpha] + alphaBlendTable[(v2[2] << 8) | alpha];
                    unsigned int colorIndex = (r << 10) | (g << 5) | b;

                    *destPtr = colorTable[colorIndex];

                    destPtr++;
                    srcPtr++;
                    alphaWindowBufferPtr++;
                    alphaBufferPtr++;
                }

                alphaBuffer += rle;
                if ((rle & 1) != 0) {
                    alphaBuffer++;
                }
            }

            src += rle;
            dest += rle;
            alphaWindowBuffer += rle;
        }

        src += srcPitch - srcWidth;
        dest += destPitch - srcWidth;
    }
}

// 0x4BBFC4
void alphaBltBufRect(unsigned char* src, int srcWidth, int srcHeight, unsigned char* dest, int destWidth, int destHeight)
{
    int chunkWidth = srcWidth / 3;
    int chunkHeight = srcHeight / 3;

    // Middle Middle
    unsigned char* ptr = src + srcWidth * chunkHeight + chunkWidth;
    for (int x = 0; x < destWidth; x += chunkWidth) {
        for (int y = 0; y < destHeight; y += chunkHeight) {
            int middleWidth;
            if (x + chunkWidth >= destWidth) {
                middleWidth = destWidth - x;
            } else {
                middleWidth = chunkWidth;
            }
            int middleY = y + chunkHeight;
            if (middleY >= destHeight) {
                middleY = destHeight;
            }
            buf_to_buf(ptr,
                middleWidth,
                middleY - y,
                srcWidth,
                dest + destWidth * y + x,
                destWidth);
        }
    }

    // Middle Column
    for (int x = 0; x < destWidth; x += chunkWidth) {
        // Top Middle
        int topMiddleX = chunkWidth + x;
        if (topMiddleX >= destWidth) {
            topMiddleX = destWidth;
        }
        int topMiddleHeight = chunkHeight;
        if (topMiddleHeight >= destHeight) {
            topMiddleHeight = destHeight;
        }
        buf_to_buf(src + chunkWidth,
            topMiddleX - x,
            topMiddleHeight,
            srcWidth,
            dest + x,
            destWidth);

        // Bottom Middle
        int bottomMiddleX = chunkWidth + x;
        if (bottomMiddleX >= destWidth) {
            bottomMiddleX = destWidth;
        }
        buf_to_buf(src + srcWidth * 2 * chunkHeight + chunkWidth,
            bottomMiddleX - x,
            destHeight - (destHeight - chunkHeight),
            srcWidth,
            dest + destWidth * (destHeight - chunkHeight) + x,
            destWidth);
    }

    // Middle Row
    for (int y = 0; y < destHeight; y += chunkHeight) {
        // Middle Left
        int middleLeftWidth = chunkWidth;
        if (middleLeftWidth >= destWidth) {
            middleLeftWidth = destWidth;
        }
        int middleLeftY = chunkHeight + y;
        if (middleLeftY >= destHeight) {
            middleLeftY = destHeight;
        }
        buf_to_buf(src + srcWidth * chunkHeight,
            middleLeftWidth,
            middleLeftY - y,
            srcWidth,
            dest + destWidth * y,
            destWidth);

        // Middle Right
        int middleRightY = chunkHeight + y;
        if (middleRightY >= destHeight) {
            middleRightY = destHeight;
        }
        buf_to_buf(src + 2 * chunkWidth + srcWidth * chunkHeight,
            destWidth - (destWidth - chunkWidth),
            middleRightY - y,
            srcWidth,
            dest + destWidth * y + destWidth - chunkWidth,
            destWidth);
    }

    // Top Left
    int topLeftWidth = chunkWidth;
    if (topLeftWidth >= destWidth) {
        topLeftWidth = destWidth;
    }
    int topLeftHeight = chunkHeight;
    if (topLeftHeight >= destHeight) {
        topLeftHeight = destHeight;
    }
    buf_to_buf(src,
        topLeftWidth,
        topLeftHeight,
        srcWidth,
        dest,
        destWidth);

    // Bottom Left
    int bottomLeftHeight = chunkHeight;
    if (chunkHeight >= destHeight) {
        bottomLeftHeight = destHeight;
    }
    buf_to_buf(src + chunkWidth * 2,
        destWidth - (destWidth - chunkWidth),
        bottomLeftHeight,
        srcWidth,
        dest + destWidth - chunkWidth,
        destWidth);

    // Top Right
    int topRightWidth = chunkWidth;
    if (chunkWidth >= destWidth) {
        topRightWidth = destWidth;
    }
    buf_to_buf(src + srcWidth * 2 * chunkHeight,
        topRightWidth,
        destHeight - (destHeight - chunkHeight),
        srcWidth,
        dest + destWidth * (destHeight - chunkHeight),
        destWidth);

    // Bottom Right
    buf_to_buf(src + 2 * chunkWidth + srcWidth * 2 * chunkHeight,
        destWidth - (destWidth - chunkWidth),
        destHeight - (destHeight - chunkHeight),
        srcWidth,
        dest + destWidth * (destHeight - chunkHeight) + (destWidth - chunkWidth),
        destWidth);
}

// NOTE: Unused.
//
// 0x4BC5E0
int windowEnableCheckRegion()
{
    checkRegionEnable = 1;
    return 1;
}

// NOTE: Unused.
//
// 0x4BC5F0
int windowDisableCheckRegion()
{
    checkRegionEnable = 0;
    return 1;
}

// NOTE: Unused.
//
// 0x4BC600
int windowSetHoldTime(int value)
{
    holdTime = value;
    return 1;
}

// NOTE: Unused.
//
// 0x4BC60C
int windowAddTextRegion(int x, int y, int width, int font, int textAlignment, int textFlags, int backgroundColor)
{
    if (currentWindow == -1) {
        return -1;
    }

    if (windows[currentWindow].window == -1) {
        return -1;
    }

    return win_add_text_region(windows[currentWindow].window,
        x,
        y,
        width,
        font,
        textAlignment,
        textFlags,
        backgroundColor);
}

// NOTE: Unused.
//
// 0x4BC668
int windowPrintTextRegion(int textRegionId, char* string)
{
    return win_print_text_region(textRegionId, string);
}

// NOTE: Unused.
//
// 0x4BC670
int windowUpdateTextRegion(int textRegionId)
{
    return win_update_text_region(textRegionId);
}

// NOTE: Unused.
//
// 0x4BC678
int windowDeleteTextRegion(int textRegionId)
{
    return win_delete_text_region(textRegionId);
}

// NOTE: Unused.
//
// 0x4BC680
int windowTextRegionStyle(int textRegionId, int font, int textAlignment, int textFlags, int backgroundColor)
{
    return win_text_region_style(textRegionId, font, textAlignment, textFlags, backgroundColor);
}

// NOTE: Unused.
//
// 0x4BC698
int windowAddTextInputRegion(int textRegionId, char* text, int a3, int a4)
{
    return win_add_text_input_region(textRegionId, text, a3, a4);
}

// NOTE: Unused.
//
// 0x4BC6A0
int windowDeleteTextInputRegion(int textInputRegionId)
{
    if (textInputRegionId != -1) {
        return win_delete_text_input_region(textInputRegionId);
    }

    if (currentWindow == -1) {
        return 0;
    }

    if (windows[currentWindow].window == -1) {
        return 0;
    }

    return win_delete_all_text_input_regions(windows[currentWindow].window);
}

// NOTE: Unused.
//
// 0x4BC6E4
int windowSetTextInputDeleteFunc(int textInputRegionId, TextInputRegionDeleteFunc* deleteFunc, void* userData)
{
    return win_set_text_input_delete_func(textInputRegionId, deleteFunc, userData);
}
