#include "window.h"

#include "color.h"
#include "core.h"
#include "datafile.h"
#include "draw.h"
#include "interpreter_lib.h"
#include "memory_manager.h"
#include "mouse_manager.h"
#include "movie.h"
#include "text_font.h"
#include "widget.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 0x51DCAC
int _holdTime = 250;

// 0x51DCB0
int _checkRegionEnable = 1;

// 0x51DCB4
int _winTOS = -1;

// 051DCB8
int gCurrentManagedWindowIndex = -1;

// 0x51DCBC
INITVIDEOFN _gfx_init[12] = {
    _init_mode_320_200,
    _init_mode_640_480,
    _init_mode_640_480_16,
    _init_mode_320_400,
    _init_mode_640_480_16,
    _init_mode_640_400,
    _init_mode_640_480_16,
    _init_mode_800_600,
    _init_mode_640_480_16,
    _init_mode_1024_768,
    _init_mode_640_480_16,
    _init_mode_1280_1024,
};

// 0x51DD1C
Size _sizes_x[12] = {
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
int gWindowInputHandlersLength = 0;

// 0x51DD80
int _lastWin = -1;

// 0x51DD84
int _said_quit = 1;

// 0x66E770
int _winStack[MANAGED_WINDOW_COUNT];

// 0x66E7B0
char _alphaBlendTable[64 * 256];

// 0x6727B0
ManagedWindow gManagedWindows[MANAGED_WINDOW_COUNT];

// 0x672D70
WindowInputHandler** gWindowInputHandlers;

// 0x672D74
ManagedWindowCreateCallback* off_672D74;

// NOTE: This value is never set.
//
// 0x672D78
void (*_selectWindowFunc)(int, ManagedWindow*);

// 0x672D7C
int _xres;

// 0x672D80
DisplayInWindowCallback* gDisplayInWindowCallback;

// 0x672D84
WindowDeleteCallback* gWindowDeleteCallback;

// 0x672D88
int _yres;

// Highlight color (maybe r).
//
// 0x672D8C
int _currentHighlightColorR;

// 0x672D90
int gWidgetFont;

// 0x672D98
ButtonCallback* off_672D98;

// 0x672D9C
ButtonCallback* off_672D9C;

// Text color (maybe g).
//
// 0x672DA0
int _currentTextColorG;

// text color (maybe b).
//
// 0x672DA4
int _currentTextColorB;

// 0x672DA8
int gWidgetTextFlags;

// Text color (maybe r)
//
// 0x672DAC
int _currentTextColorR;

// highlight color (maybe g)
//
// 0x672DB0
int _currentHighlightColorG;

// Highlight color (maybe b).
//
// 0x672DB4
int _currentHighlightColorB;

// 0x4B69BC
bool sub_4B69BC()
{
    // TODO: Incomplete.
    return false;
}

// 0x4B6858
bool _windowCheckRegion(int windowIndex, int mouseX, int mouseY, int mouseEvent)
{
    // TODO: Incomplete.
    return false;
}

// 0x4B6A54
bool _checkAllRegions()
{
    if (!_checkRegionEnable) {
        return false;
    }

    int mouseX;
    int mouseY;
    mouseGetPosition(&mouseX, &mouseY);

    int mouseEvent = mouseGetEvent();
    int win = windowGetAtPoint(mouseX, mouseY);

    for (int windowIndex = 0; windowIndex < MANAGED_WINDOW_COUNT; windowIndex++) {
        ManagedWindow* managedWindow = &(gManagedWindows[windowIndex]);
        if (managedWindow->window != -1 && managedWindow->window == win) {
            if (_lastWin != -1 && _lastWin != windowIndex && gManagedWindows[_lastWin].window != -1) {
                ManagedWindow* managedWindow = &(gManagedWindows[_lastWin]);
                int v1 = managedWindow->field_38;

                for (int regionIndex = 0; regionIndex < managedWindow->regionsLength; regionIndex++) {
                    Region* region = managedWindow->regions[regionIndex];
                    if (region != NULL && region->field_64 != 0) {
                        region->field_64 = 0;
                        if (region->field_78 != NULL) {
                            region->field_78(region, region->field_80, 3);
                            if (v1 != managedWindow->field_38) {
                                return true;
                            }
                        }

                        if (region->field_7C != NULL) {
                            region->field_7C(region, region->field_84, 3);
                            if (v1 != managedWindow->field_38) {
                                return true;
                            }
                        }

                        if (region->program != NULL && region->proc != 0) {
                            _executeProc(region->program, region->proc);
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

            return _windowCheckRegion(windowIndex, mouseX, mouseY, mouseEvent);
        }
    }

    return false;
}

// 0x4B6C48
void _windowAddInputFunc(WindowInputHandler* handler)
{
    int index;
    for (index = 0; index < gWindowInputHandlersLength; index++) {
        if (gWindowInputHandlers[index] == NULL) {
            break;
        }
    }

    if (index == gWindowInputHandlersLength) {
        if (gWindowInputHandlers != NULL) {
            gWindowInputHandlers = (WindowInputHandler**)internal_realloc_safe(gWindowInputHandlers, sizeof(*gWindowInputHandlers) * (gWindowInputHandlersLength + 1), __FILE__, __LINE__); // "..\\int\\WINDOW.C", 521
        } else {
            gWindowInputHandlers = (WindowInputHandler**)internal_malloc_safe(sizeof(*gWindowInputHandlers), __FILE__, __LINE__); // "..\\int\\WINDOW.C", 523
        }
    }

    gWindowInputHandlers[gWindowInputHandlersLength] = handler;
    gWindowInputHandlersLength++;

}

// 0x4B6DE8
int sub_4B6DE8(const char* regionName, int a2)
{
    // TODO: Incomplete.
    return 0;
}

// 0x4B6F60
void _doButtonOn(int btn, int keyCode)
{
    sub_4B6F68(btn, MANAGED_BUTTON_MOUSE_EVENT_ENTER);
}

// 0x4B6F68
void sub_4B6F68(int btn, int mouseEvent)
{
    int win = _win_last_button_winID();
    if (win == -1) {
        return;
    }

    for (int windowIndex = 0; windowIndex < MANAGED_WINDOW_COUNT; windowIndex++) {
        ManagedWindow* managedWindow = &(gManagedWindows[windowIndex]);
        if (managedWindow->window == win) {
            for (int buttonIndex = 0; buttonIndex < managedWindow->buttonsLength; buttonIndex++) {
                ManagedButton* managedButton = &(managedWindow->buttons[buttonIndex]);
                if (managedButton->btn == btn) {
                    if ((managedButton->flags & 0x02) != 0) {
                        _win_set_button_rest_state(managedButton->btn, 0, 0);
                    } else {
                        if (managedButton->program != NULL && managedButton->procs[mouseEvent] != 0) {
                            _executeProc(managedButton->program, managedButton->procs[mouseEvent]);
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
void _doButtonOff(int btn, int keyCode)
{
    sub_4B6F68(btn, MANAGED_BUTTON_MOUSE_EVENT_EXIT);
}

// 0x4B7034
void _doButtonPress(int btn, int keyCode)
{
    sub_4B6F68(btn, MANAGED_BUTTON_MOUSE_EVENT_BUTTON_DOWN);
}

// 0x4B703C
void _doButtonRelease(int btn, int keyCode)
{
    sub_4B6F68(btn, MANAGED_BUTTON_MOUSE_EVENT_BUTTON_UP);
}

// NOTE: Unused.
//
// 0x4B7048
void _doRightButtonPress(int btn, int keyCode)
{
    sub_4B704C(btn, MANAGED_BUTTON_RIGHT_MOUSE_EVENT_BUTTON_DOWN);
}

// NOTE: Unused.
//
// 0x4B704C
void sub_4B704C(int btn, int mouseEvent)
{
    int win = _win_last_button_winID();
    if (win == -1) {
        return;
    }

    for (int windowIndex = 0; windowIndex < MANAGED_WINDOW_COUNT; windowIndex++) {
        ManagedWindow* managedWindow = &(gManagedWindows[windowIndex]);
        if (managedWindow->window == win) {
            for (int buttonIndex = 0; buttonIndex < managedWindow->buttonsLength; buttonIndex++) {
                ManagedButton* managedButton = &(managedWindow->buttons[buttonIndex]);
                if (managedButton->btn == btn) {
                    if ((managedButton->flags & 0x02) != 0) {
                        _win_set_button_rest_state(managedButton->btn, 0, 0);
                    } else {
                        if (managedButton->program != NULL && managedButton->rightProcs[mouseEvent] != 0) {
                            _executeProc(managedButton->program, managedButton->rightProcs[mouseEvent]);
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
void _doRightButtonRelease(int btn, int keyCode)
{
    sub_4B704C(btn, MANAGED_BUTTON_RIGHT_MOUSE_EVENT_BUTTON_UP);
}

// 0x4B7118
void _setButtonGFX(int width, int height, unsigned char* normal, unsigned char* pressed, unsigned char* a5)
{
    if (normal != NULL) {
        bufferFill(normal, width, height, width, _colorTable[0]);
        bufferFill(normal + width + 1, width - 2, height - 2, width, _intensityColorTable[(_colorTable[32767] << 8) + 89]);
        bufferDrawLine(normal, width, 1, 1, width - 2, 1, _colorTable[32767]);
        bufferDrawLine(normal, width, 2, 2, width - 3, 2, _colorTable[32767]);
        bufferDrawLine(normal, width, 1, height - 2, width - 2, height - 2, _intensityColorTable[(_colorTable[32767] << 8) + 44]);
        bufferDrawLine(normal, width, 2, height - 3, width - 3, height - 3, _intensityColorTable[(_colorTable[32767] << 8) + 44]);
        bufferDrawLine(normal, width, width - 2, 1, width - 3, 2, _intensityColorTable[(_colorTable[32767] << 8) + 89]);
        bufferDrawLine(normal, width, 1, 2, 1, height - 3, _colorTable[32767]);
        bufferDrawLine(normal, width, 2, 3, 2, height - 4, _colorTable[32767]);
        bufferDrawLine(normal, width, width - 2, 2, width - 2, height - 3, _intensityColorTable[(_colorTable[32767] << 8) + 44]);
        bufferDrawLine(normal, width, width - 3, 3, width - 3, height - 4, _intensityColorTable[(_colorTable[32767] << 8) + 44]);
        bufferDrawLine(normal, width, 1, height - 2, 2, height - 3, _intensityColorTable[(_colorTable[32767] << 8) + 89]);
    }

    if (pressed != NULL) {
        bufferFill(pressed, width, height, width, _colorTable[0]);
        bufferFill(pressed + width + 1, width - 2, height - 2, width, _intensityColorTable[(_colorTable[32767] << 8) + 89]);
        bufferDrawLine(pressed, width, 1, 1, width - 2, 1, _colorTable[32767] + 44);
        bufferDrawLine(pressed, width, 1, 1, 1, height - 2, _colorTable[32767] + 44);
    }

    if (a5 != NULL) {
        bufferFill(a5, width, height, width, _colorTable[0]);
        bufferFill(a5 + width + 1, width - 2, height - 2, width, _intensityColorTable[(_colorTable[32767] << 8) + 89]);
        bufferDrawLine(a5, width, 1, 1, width - 2, 1, _colorTable[32767]);
        bufferDrawLine(a5, width, 2, 2, width - 3, 2, _colorTable[32767]);
        bufferDrawLine(a5, width, 1, height - 2, width - 2, height - 2, _intensityColorTable[(_colorTable[32767] << 8) + 44]);
        bufferDrawLine(a5, width, 2, height - 3, width - 3, height - 3, _intensityColorTable[(_colorTable[32767] << 8) + 44]);
        bufferDrawLine(a5, width, width - 2, 1, width - 3, 2, _intensityColorTable[(_colorTable[32767] << 8) + 89]);
        bufferDrawLine(a5, width, 1, 2, 1, height - 3, _colorTable[32767]);
        bufferDrawLine(a5, width, 2, 3, 2, height - 4, _colorTable[32767]);
        bufferDrawLine(a5, width, width - 2, 2, width - 2, height - 3, _intensityColorTable[(_colorTable[32767] << 8) + 44]);
        bufferDrawLine(a5, width, width - 3, 3, width - 3, height - 4, _intensityColorTable[(_colorTable[32767] << 8) + 44]);
        bufferDrawLine(a5, width, 1, height - 2, 2, height - 3, _intensityColorTable[(_colorTable[32767] << 8) + 89]);
    }
}

// 0x4B7734
int _windowWidth()
{
    return gManagedWindows[gCurrentManagedWindowIndex].width;
}

// 0x4B7754
int _windowHeight()
{
    return gManagedWindows[gCurrentManagedWindowIndex].height;
}

// 0x4B7680
bool _windowDraw()
{
    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    if (managedWindow->window == -1) {
        return false;
    }

    windowRefresh(managedWindow->window);

    return true;
}

// 0x4B78A4
bool _deleteWindow(const char* windowName)
{
    int index;
    for (index = 0; index < MANAGED_WINDOW_COUNT; index++) {
        ManagedWindow* managedWindow = &(gManagedWindows[index]);
        if (stricmp(managedWindow->name, windowName) == 0) {
            break;
        }
    }

    if (index == MANAGED_WINDOW_COUNT) {
        return false;
    }

    if (gWindowDeleteCallback != NULL) {
        gWindowDeleteCallback(index, windowName);
    }

    ManagedWindow* managedWindow = &(gManagedWindows[index]);
    sub_4B5998(managedWindow->window);
    windowDestroy(managedWindow->window);
    managedWindow->window = -1;
    managedWindow->name[0] = '\0';

    if (managedWindow->buttons != NULL) {
        for (int index = 0; index < managedWindow->buttonsLength; index++) {
            ManagedButton* button = &(managedWindow->buttons[index]);
            if (button->hover != NULL) {
                internal_free_safe(button->hover, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 802
            }

            if (button->field_4C != NULL) {
                internal_free_safe(button->field_4C, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 804
            }

            if (button->pressed != NULL) {
                internal_free_safe(button->pressed, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 806
            }

            if (button->normal != NULL) {
                internal_free_safe(button->normal, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 808
            }
        }

        internal_free_safe(managedWindow->buttons, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 810
    }

    if (managedWindow->regions != NULL) {
        for (int index = 0; index < managedWindow->regionsLength; index++) {
            Region* region = managedWindow->regions[index];
            if (region != NULL) {
                regionDelete(region);
            }
        }

        internal_free_safe(managedWindow->regions, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 818
        managedWindow->regions = NULL;
    }

    return true;
}

// 0x4B7AC4
int sub_4B7AC4(const char* windowName, int x, int y, int width, int height)
{
    // TODO: Incomplete.
    return -1;
}

// 0x4B7E7C
int sub_4B7E7C(const char* windowName, int x, int y, int width, int height)
{
    // TODO: Incomplete.
    return -1;
}

// 0x4B7F3C
int _createWindow(const char* windowName, int x, int y, int width, int height, int a6, int flags)
{
    int windowIndex = -1;

    // NOTE: Original code is slightly different.
    for (int index = 0; index < MANAGED_WINDOW_COUNT; index++) {
        ManagedWindow* managedWindow = &(gManagedWindows[index]);
        if (managedWindow->window == -1) {
            windowIndex = index;
            break;
        } else {
            if (stricmp(managedWindow->name, windowName) == 0) {
                _deleteWindow(windowName);
                windowIndex = index;
                break;
            }
        }
    }

    if (windowIndex == -1) {
        return -1;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[windowIndex]);
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
    if (off_672D74 != NULL) {
        off_672D74(windowIndex, managedWindow->name, &flags);
    }

    managedWindow->window = windowCreate(x, y, width, height, a6, flags);
    managedWindow->field_48 = 0;
    managedWindow->field_44 = 0;
    managedWindow->field_4C = a6;
    managedWindow->field_50 = flags;

    return windowIndex;
}

// 0x4B80A4
int _windowOutput(char* string)
{
    if (gCurrentManagedWindowIndex == -1) {
        return 0;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);

    int x = (int)(managedWindow->field_44 * managedWindow->field_54);
    int y = (int)(managedWindow->field_48 * managedWindow->field_58);
    // NOTE: Uses `add` at 0x4B810E, not bitwise `or`.
    int flags = widgetGetTextColor() + widgetGetTextFlags();
    windowDrawText(managedWindow->window, string, 0, x, y, flags);

    return 1;
}

// 0x4B814C
bool _windowGotoXY(int x, int y)
{
    if (gCurrentManagedWindowIndex == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    managedWindow->field_44 = (int)(x * managedWindow->field_54);
    managedWindow->field_48 = (int)(y * managedWindow->field_58);

    return true;
}

// 0x4B81C4
bool _selectWindowID(int index)
{
    if (index < 0 || index >= MANAGED_WINDOW_COUNT) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[index]);
    if (managedWindow->window == -1) {
        return false;
    }

    gCurrentManagedWindowIndex = index;

    if (_selectWindowFunc != NULL) {
        _selectWindowFunc(index, managedWindow);
    }

    return true;
}

// 0x4B821C
int _selectWindow(const char* windowName)
{
    if (gCurrentManagedWindowIndex != -1) {
        ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
        if (stricmp(managedWindow->name, windowName) == 0) {
            return gCurrentManagedWindowIndex;
        }
    }

    int index;
    for (index = 0; index < MANAGED_WINDOW_COUNT; index++) {
        ManagedWindow* managedWindow = &(gManagedWindows[index]);
        if (managedWindow->window != -1) {
            if (stricmp(managedWindow->name, windowName) == 0) {
                break;
            }
        }
    }

    if (!_selectWindowID(index)) {
        return index;
    }

    return -1;
}

// 0x4B82DC
unsigned char* _windowGetBuffer()
{
    if (gCurrentManagedWindowIndex != -1) {
        ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
        return windowGetBuffer(managedWindow->window);
    }

    return NULL;
}

// 0x4B8330
int _pushWindow(const char* windowName)
{
    if (_winTOS >= MANAGED_WINDOW_COUNT) {
        return -1;
    }

    int oldCurrentWindowIndex = gCurrentManagedWindowIndex;

    int windowIndex = _selectWindow(windowName);
    if (windowIndex == -1) {
        return -1;
    }

    // TODO: Check.
    for (int index = 0; index < _winTOS; index++) {
        if (_winStack[index] == oldCurrentWindowIndex) {
            memcpy(&(_winStack[index]), &(_winStack[index + 1]), sizeof(*_winStack) * (_winTOS - index));
            break;
        }
    }

    _winTOS++;
    _winStack[_winTOS] = oldCurrentWindowIndex;

    return windowIndex;
}

// 0x4B83D4
int _popWindow()
{
    if (_winTOS == -1) {
        return -1;
    }

    int windowIndex = _winStack[_winTOS];
    ManagedWindow* managedWindow = &(gManagedWindows[windowIndex]);
    _winTOS--;

    return _selectWindow(managedWindow->name);
}

// 0x4B8414
void _windowPrintBuf(int win, char* string, int stringLength, int width, int maxY, int x, int y, int flags, int textAlignment)
{
    if (y + fontGetLineHeight() > maxY) {
        return;
    }

    if (stringLength > 255) {
        stringLength = 255;
    }

    char* stringCopy = (char*)internal_malloc_safe(stringLength + 1, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1078
    strncpy(stringCopy, string, stringLength);
    stringCopy[stringLength] = '\0';

    int stringWidth = fontGetStringWidth(stringCopy);
    int stringHeight = fontGetLineHeight();
    if (stringWidth == 0 || stringHeight == 0) {
        internal_free_safe(stringCopy, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1085
        return;
    }

    if ((flags & FONT_SHADOW) != 0) {
        stringWidth++;
        stringHeight++;
    }

    unsigned char* backgroundBuffer = (unsigned char*)internal_calloc_safe(stringWidth, stringHeight, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1093
    unsigned char* backgroundBufferPtr = backgroundBuffer;
    fontDrawText(backgroundBuffer, stringCopy, stringWidth, stringWidth, flags);

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

    if (stringHeight + y > windowGetHeight(win)) {
        stringHeight = windowGetHeight(win) - y;
    }

    if ((flags & 0x2000000) != 0) {
        blitBufferToBufferTrans(backgroundBufferPtr, width, stringHeight, stringWidth, windowGetBuffer(win) + windowGetWidth(win) * y + x, windowGetWidth(win));
    } else {
        blitBufferToBuffer(backgroundBufferPtr, width, stringHeight, stringWidth, windowGetBuffer(win) + windowGetWidth(win) * y + x, windowGetWidth(win));
    }

    internal_free_safe(backgroundBuffer, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1130
    internal_free_safe(stringCopy, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1131
}

// 0x4B8638
char** _windowWordWrap(char* string, int maxLength, int a3, int* substringListLengthPtr)
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
        v1 += fontGetCharacterWidth(*pch & 0xFF);
        if (*pch != '\n' && v1 <= maxLength) {
            v1 += fontGetLetterSpacing();
            pch++;
        } else {
            while (v1 > maxLength) {
                v1 -= fontGetCharacterWidth(*pch);
                pch--;
            }

            if (*pch != '\n') {
                while (pch != start && *pch != ' ') {
                    pch--;
                }
            }

            if (substringList != NULL) {
                substringList = (char**)internal_realloc_safe(substringList, sizeof(*substringList) * (substringListLength + 1), __FILE__, __LINE__); // "..\int\WINDOW.C", 1166
            } else {
                substringList = (char**)internal_malloc_safe(sizeof(*substringList), __FILE__, __LINE__); // "..\int\WINDOW.C", 1167
            }

            char* substring = (char*)internal_malloc_safe(pch - start + 1, __FILE__, __LINE__); // "..\int\WINDOW.C", 1169
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
            substringList = (char**)internal_realloc_safe(substringList, sizeof(*substringList) * (substringListLength + 1), __FILE__, __LINE__); // "..\int\WINDOW.C", 1184
        } else {
            substringList = (char**)internal_malloc_safe(sizeof(*substringList), __FILE__, __LINE__); // "..\int\WINDOW.C", 1185
        }

        char* substring = (char*)internal_malloc_safe(pch - start + 1, __FILE__, __LINE__); // "..\int\WINDOW.C", 1169
        strncpy(substring, start, pch - start);
        substring[pch - start] = '\0';

        substringList[substringListLength] = substring;
        substringListLength++;
    }

    *substringListLengthPtr = substringListLength;

    return substringList;
}

// 0x4B880C
void _windowFreeWordList(char** substringList, int substringListLength)
{
    if (substringList == NULL) {
        return;
    }

    for (int index = 0; index < substringListLength; index++) {
        internal_free_safe(substringList[index], __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1200
    }

    internal_free_safe(substringList, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1201
}

// Renders multiline string in the specified bounding box.
//
// 0x4B8854
void _windowWrapLineWithSpacing(int win, char* string, int width, int height, int x, int y, int flags, int textAlignment, int a9)
{
    if (string == NULL) {
        return;
    }

    int substringListLength;
    char** substringList = _windowWordWrap(string, width, 0, &substringListLength);

    for (int index = 0; index < substringListLength; index++) {
        int v1 = y + index * (a9 + fontGetLineHeight());
        _windowPrintBuf(win, substringList[index], strlen(substringList[index]), width, height + y, x, v1, flags, textAlignment);
    }

    _windowFreeWordList(substringList, substringListLength);
}

// Renders multiline string in the specified bounding box.
//
// 0x4B88FC
void _windowWrapLine(int win, char* string, int width, int height, int x, int y, int flags, int textAlignment)
{
    _windowWrapLineWithSpacing(win, string, width, height, x, y, flags, textAlignment, 0);
}

// 0x4B8920
bool _windowPrintRect(char* string, int a2, int textAlignment)
{
    if (gCurrentManagedWindowIndex == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    int width = (int)(a2 * managedWindow->field_54);
    int height = windowGetHeight(managedWindow->window);
    int x = managedWindow->field_44;
    int y = managedWindow->field_48;
    int flags = widgetGetTextColor() | 0x2000000;
    _windowWrapLineWithSpacing(managedWindow->window, string, width, height, x, y, flags, textAlignment, 0);

    return true;
}

// 0x4B89B0
bool _windowFormatMessage(char* string, int x, int y, int width, int height, int textAlignment)
{
    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    int flags = widgetGetTextColor() | 0x2000000;
    _windowWrapLineWithSpacing(managedWindow->window, string, width, height, x, y, flags, textAlignment, 0);

    return true;
}

// 0x4B8A60
bool _windowPrint(char* string, int a2, int x, int y, int a5)
{
    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    x = (int)(x * managedWindow->field_54);
    y = (int)(y * managedWindow->field_58);

    windowDrawText(managedWindow->window, string, a2, x, y, a5);

    return true;
}

// 0x4B8B10
void _displayInWindow(unsigned char* data, int width, int height, int pitch)
{
    if (gDisplayInWindowCallback != NULL) {
        // NOTE: The second parameter is unclear as there is no distinction
        // between address of entire window struct and it's name (since it's the
        // first field). I bet on name since it matches WindowDeleteCallback,
        // which accepts window index and window name as seen at 0x4B7927).
        gDisplayInWindowCallback(gCurrentManagedWindowIndex,
            gManagedWindows[gCurrentManagedWindowIndex].name,
            data,
            width,
            height);
    }

    if (width == pitch) {
        // NOTE: Uninline.
        if (pitch == _windowWidth() && height == _windowHeight()) {
            // NOTE: Uninline.
            unsigned char* windowBuffer = _windowGetBuffer();
            memcpy(windowBuffer, data, height * width);
        } else {
            // NOTE: Uninline.
            unsigned char* windowBuffer = _windowGetBuffer();
            _drawScaledBuf(windowBuffer, _windowWidth(), _windowHeight(), data, width, height);
        }
    } else {
        // NOTE: Uninline.
        unsigned char* windowBuffer = _windowGetBuffer();
        _drawScaled(windowBuffer,
            _windowWidth(),
            _windowHeight(),
            _windowWidth(),
            data,
            width,
            height,
            pitch);
    }
}

// 0x4B8C68
void _displayFile(char* fileName)
{
    int width;
    int height;
    unsigned char* data = datafileRead(fileName, &width, &height);
    if (data != NULL) {
        _displayInWindow(data, width, height, width);
        internal_free_safe(data, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1294
    }
}

// 0x4B8CA8
void _displayFileRaw(char* fileName)
{
    int width;
    int height;
    unsigned char* data = datafileReadRaw(fileName, &width, &height);
    if (data != NULL) {
        _displayInWindow(data, width, height, width);
        internal_free_safe(data, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1305
    }
}

// 0x4B8E50
bool _windowDisplay(char* fileName, int x, int y, int width, int height)
{
    int imageWidth;
    int imageHeight;
    unsigned char* imageData = datafileRead(fileName, &imageWidth, &imageHeight);
    if (imageData == NULL) {
        return false;
    }

    _windowDisplayBuf(imageData, imageWidth, imageHeight, x, y, width, height);

    internal_free_safe(imageData, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1376

    return true;
}

// 0x4B8EF0
bool _windowDisplayBuf(unsigned char* src, int srcWidth, int srcHeight, int destX, int destY, int destWidth, int destHeight)
{
    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    unsigned char* windowBuffer = windowGetBuffer(managedWindow->window);

    blitBufferToBuffer(src,
        destWidth,
        destHeight,
        srcWidth,
        windowBuffer + managedWindow->width * destY + destX,
        managedWindow->width);

    return true;
}

// 0x4B9048
int _windowGetXres()
{
    return _xres;
}

// 0x4B9050
int _windowGetYres()
{
    return _yres;
}

// 0x4B9058
void _removeProgramReferences_3(Program* program)
{
    for (int index = 0; index < MANAGED_WINDOW_COUNT; index++) {
        ManagedWindow* managedWindow = &(gManagedWindows[index]);
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
                        region->field_4C = 0;
                        region->field_48 = 0;
                        region->proc = 0;
                        region->field_50 = 0;
                    }
                }
            }
        }
    }
}

// 0x4B9190
void _initWindow(int resolution, int a2)
{
    char err[MAX_PATH];
    int rc;
    int i, j;

    intLibRegisterProgramDeleteCallback(_removeProgramReferences_3);

    _currentTextColorR = 0;
    _currentTextColorG = 0;
    _currentTextColorB = 0;
    _currentHighlightColorR = 0;
    _currentHighlightColorG = 0;
    gWidgetTextFlags = 0x2010000;

    _yres = _sizes_x[resolution].height; // screen height
    _currentHighlightColorB = 0;
    _xres = _sizes_x[resolution].width; // screen width

    for (int i = 0; i < MANAGED_WINDOW_COUNT; i++) {
        gManagedWindows[i].window = -1;
    }

    rc = windowManagerInit(_gfx_init[resolution], directDrawFree, a2);
    if (rc != WINDOW_MANAGER_OK) {
        switch (rc) {
        case WINDOW_MANAGER_ERR_INITIALIZING_VIDEO_MODE:
            sprintf(err, "Error initializing video mode %dx%d\n", _xres, _yres);
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_NO_MEMORY:
            sprintf(err, "Not enough memory to initialize video mode\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_INITIALIZING_TEXT_FONTS:
            sprintf(err, "Couldn't find/load text fonts\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_WINDOW_SYSTEM_ALREADY_INITIALIZED:
            sprintf(err, "Attempt to initialize window system twice\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_WINDOW_SYSTEM_NOT_INITIALIZED:
            sprintf(err, "Window system not initialized\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_CURRENT_WINDOWS_TOO_BIG:
            sprintf(err, "Current windows are too big for new resolution\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_INITIALIZING_DEFAULT_DATABASE:
            sprintf(err, "Error initializing default database.\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_8:
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_ALREADY_RUNNING:
            sprintf(err, "Program already running.\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_TITLE_NOT_SET:
            sprintf(err, "Program title not set.\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_INITIALIZING_INPUT:
            sprintf(err, "Failure initializing input devices.\n");
            showMesageBox(err);
            exit(1);
            break;
        default:
            sprintf(err, "Unknown error code %d\n", rc);
            showMesageBox(err);
            exit(1);
            break;
        }
    }

    gWidgetFont = 100;
    fontSetCurrent(100);

    _initMousemgr();

    _mousemgrSetNameMangler(_interpretMangleName);

    for (i = 0; i < 64; i++) {
        for (j = 0; j < 256; j++) {
            _alphaBlendTable[(i << 8) + j] = ((i * j) >> 9);
        }
    }
}

// 0x4B947C
void _windowClose()
{
    // TODO: Incomplete, but required for graceful exit.

    for (int index = 0; index < MANAGED_WINDOW_COUNT; index++) {
        ManagedWindow* managedWindow = &(gManagedWindows[index]);
        if (managedWindow->window != -1) {
            // _deleteWindow(managedWindow);
        }
    }

    dbExit();
    windowManagerExit();
}

// Deletes button with the specified name or all buttons if it's NULL.
//
// 0x4B9548
bool _windowDeleteButton(const char* buttonName)
{
    if (gCurrentManagedWindowIndex != -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    if (managedWindow->buttonsLength == 0) {
        return false;
    }

    if (buttonName == NULL) {
        for (int index = 0; index < managedWindow->buttonsLength; index++) {
            ManagedButton* managedButton = &(managedWindow->buttons[index]);
            buttonDestroy(managedButton->btn);

            if (managedButton->hover != NULL) {
                internal_free_safe(managedButton->hover, __FILE__, __LINE__); // "..\int\WINDOW.C", 1648
                managedButton->hover = NULL;
            }

            if (managedButton->field_4C != NULL) {
                internal_free_safe(managedButton->field_4C, __FILE__, __LINE__); // "..\int\WINDOW.C", 1649
                managedButton->field_4C = NULL;
            }

            if (managedButton->pressed != NULL) {
                internal_free_safe(managedButton->pressed, __FILE__, __LINE__); // "..\int\WINDOW.C", 1650
                managedButton->pressed = NULL;
            }

            if (managedButton->normal != NULL) {
                internal_free_safe(managedButton->normal, __FILE__, __LINE__); // "..\int\WINDOW.C", 1651
                managedButton->normal = NULL;
            }

            if (managedButton->field_50 != NULL) {
                internal_free_safe(managedButton->normal, __FILE__, __LINE__); // "..\int\WINDOW.C", 1652
                managedButton->field_50 = NULL;
            }
        }

        internal_free_safe(managedWindow->buttons, __FILE__, __LINE__); // "..\int\WINDOW.C", 1654
        managedWindow->buttons = NULL;
        managedWindow->buttonsLength = 0;

        return true;
    }

    for (int index = 0; index < managedWindow->buttonsLength; index++) {
        ManagedButton* managedButton = &(managedWindow->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            buttonDestroy(managedButton->btn);

            if (managedButton->hover != NULL) {
                internal_free_safe(managedButton->hover, __FILE__, __LINE__); // "..\int\WINDOW.C", 1665
                managedButton->hover = NULL;
            }

            if (managedButton->field_4C != NULL) {
                internal_free_safe(managedButton->field_4C, __FILE__, __LINE__); // "..\int\WINDOW.C", 1666
                managedButton->field_4C = NULL;
            }

            if (managedButton->pressed != NULL) {
                internal_free_safe(managedButton->pressed, __FILE__, __LINE__); // "..\int\WINDOW.C", 1667
                managedButton->pressed = NULL;
            }

            if (managedButton->normal != NULL) {
                internal_free_safe(managedButton->normal, __FILE__, __LINE__); // "..\int\WINDOW.C", 1668
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
                internal_free_safe(managedWindow->buttons, __FILE__, __LINE__); // "..\int\WINDOW.C", 1672
                managedWindow->buttons = NULL;
            }

            return true;
        }
    }

    return false;
}

// 0x4B9928
bool _windowSetButtonFlag(const char* buttonName, int value)
{
    if (gCurrentManagedWindowIndex != -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
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

// 0x4B99C8
bool _windowAddButton(const char* buttonName, int x, int y, int width, int height, int flags)
{
    if (gCurrentManagedWindowIndex == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    int index;
    for (index = 0; index < managedWindow->buttonsLength; index++) {
        ManagedButton* managedButton = &(managedWindow->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            buttonDestroy(managedButton->btn);

            if (managedButton->hover != NULL) {
                internal_free_safe(managedButton->hover, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1748
                managedButton->hover = NULL;
            }

            if (managedButton->field_4C != NULL) {
                internal_free_safe(managedButton->field_4C, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1749
                managedButton->field_4C = NULL;
            }

            if (managedButton->pressed != NULL) {
                internal_free_safe(managedButton->pressed, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1750
                managedButton->pressed = NULL;
            }

            if (managedButton->normal != NULL) {
                internal_free_safe(managedButton->normal, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1751
                managedButton->normal = NULL;
            }

            break;
        }
    }

    if (index == managedWindow->buttonsLength) {
        if (managedWindow->buttons == NULL) {
            managedWindow->buttons = (ManagedButton*)internal_malloc_safe(sizeof(*managedWindow->buttons), __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1758
        } else {
            managedWindow->buttons = (ManagedButton*)internal_realloc_safe(managedWindow->buttons, sizeof(*managedWindow->buttons) * (managedWindow->buttonsLength + 1), __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1761
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

    unsigned char* normal = (unsigned char*)internal_malloc_safe(width * height, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1792
    unsigned char* pressed = (unsigned char*)internal_malloc_safe(width * height, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1793

    if ((flags & BUTTON_FLAG_TRANSPARENT) != 0) {
        memset(normal, 0, width * height);
        memset(pressed, 0, width * height);
    } else {
        _setButtonGFX(width, height, normal, pressed, NULL);
    }

    managedButton->btn = buttonCreate(
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
    
    if (off_672D98 != NULL || off_672D9C != NULL) {
        buttonSetCallbacks(managedButton->btn, off_672D98, off_672D9C);
    }

    managedButton->hover = NULL;
    managedButton->pressed = pressed;
    managedButton->normal = normal;
    managedButton->field_18 = flags;
    managedButton->field_4C = NULL;
    buttonSetMouseCallbacks(managedButton->btn, _doButtonOn, _doButtonOff, _doButtonPress, _doButtonRelease);
    _windowSetButtonFlag(buttonName, 1);

    if ((flags & BUTTON_FLAG_TRANSPARENT) != 0) {
        buttonSetMask(managedButton->btn, normal);
    }

    return true;
}

// 0x4B9DD0
bool _windowAddButtonGfx(const char* buttonName, char* pressedFileName, char* normalFileName, char* hoverFileName)
{
    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    for (int index = 0; index < managedWindow->buttonsLength; index++) {
        ManagedButton* managedButton = &(managedWindow->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            int width;
            int height;

            if (pressedFileName != NULL) {
                unsigned char* pressed = datafileRead(pressedFileName, &width, &height);
                if (pressed != NULL) {
                    _drawScaledBuf(managedButton->pressed, managedButton->width, managedButton->height, pressed, width, height);
                    internal_free_safe(pressed, __FILE__, __LINE__); // "..\\int\\WINDOW.C, 1834
                }
            }

            if (normalFileName != NULL) {
                unsigned char* normal = datafileRead(normalFileName, &width, &height);
                if (normal != NULL) {
                    _drawScaledBuf(managedButton->normal, managedButton->width, managedButton->height, normal, width, height);
                    internal_free_safe(normal, __FILE__, __LINE__); // "..\\int\\WINDOW.C, 1842
                }
            }

            if (hoverFileName != NULL) {
                unsigned char* hover = datafileRead(normalFileName, &width, &height);
                if (hover != NULL) {
                    if (managedButton->hover == NULL) {
                        managedButton->hover = (unsigned char*)internal_malloc_safe(managedButton->height * managedButton->width, __FILE__, __LINE__); // "..\\int\\WINDOW.C, 1849
                    }

                    _drawScaledBuf(managedButton->hover, managedButton->width, managedButton->height, hover, width, height);
                    internal_free_safe(hover, __FILE__, __LINE__); // "..\\int\\WINDOW.C, 1853
                }
            }

            if ((managedButton->field_18 & 0x20) != 0) {
                buttonSetMask(managedButton->btn, managedButton->normal);
            }

            _win_register_button_image(managedButton->btn, managedButton->normal, managedButton->pressed, managedButton->hover, 0);

            return true;
        }
    }

    return false;
}

// 0x4BA11C
bool _windowAddButtonProc(const char* buttonName, Program* program, int mouseEnterProc, int mouseExitProc, int mouseDownProc, int mouseUpProc)
{
    if (gCurrentManagedWindowIndex != -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
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
bool _windowAddButtonRightProc(const char* buttonName, Program* program, int rightMouseDownProc, int rightMouseUpProc)
{
    if (gCurrentManagedWindowIndex != -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
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
bool _windowAddButtonCfunc(const char* buttonName, ManagedButtonMouseEventCallback* callback, void* userData)
{
    if (gCurrentManagedWindowIndex != -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
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
bool _windowAddButtonRightCfunc(const char* buttonName, ManagedButtonMouseEventCallback* callback, void* userData)
{
    if (gCurrentManagedWindowIndex != -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    if (managedWindow->buttons == NULL) {
        return false;
    }

    for (int index = 0; index < managedWindow->buttonsLength; index++) {
        ManagedButton* managedButton = &(managedWindow->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            managedButton->rightMouseEventCallback = callback;
            managedButton->rightMouseEventCallbackUserData = userData;
            buttonSetRightMouseCallbacks(managedButton->btn, -1, -1, _doRightButtonPress, _doRightButtonRelease);
            return true;
        }
    }

    return false;
}

// 0x4BA34C
bool _windowAddButtonText(const char* buttonName, const char* text)
{
    return _windowAddButtonTextWithOffsets(buttonName, text, 2, 2, 0, 0);
}

// 0x4BA364
bool _windowAddButtonTextWithOffsets(const char* buttonName, const char* text, int pressedImageOffsetX, int pressedImageOffsetY, int normalImageOffsetX, int normalImageOffsetY)
{
    if (gCurrentManagedWindowIndex == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    if (managedWindow->buttons == NULL) {
        return false;
    }

    for (int index = 0; index < managedWindow->buttonsLength; index++) {
        ManagedButton* managedButton = &(managedWindow->buttons[index]);
        if (stricmp(managedButton->name, buttonName) == 0) {
            int normalImageHeight = fontGetLineHeight() + 1;
            int normalImageWidth = fontGetStringWidth(text) + 1;
            unsigned char* buffer = (unsigned char*)internal_malloc_safe(normalImageHeight * normalImageWidth, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 2010

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
                blitBufferToBuffer(managedButton->normal + managedButton->width * normalImageY + normalImageX,
                    normalImageWidth,
                    normalImageHeight,
                    managedButton->width,
                    buffer,
                    normalImageWidth);
            } else {
                memset(buffer, 0, normalImageHeight * normalImageWidth);
            }

            fontDrawText(buffer,
                text,
                normalImageWidth,
                normalImageWidth,
                widgetGetTextColor() + widgetGetTextFlags());

            blitBufferToBufferTrans(buffer,
                normalImageWidth,
                normalImageHeight,
                normalImageWidth,
                managedButton->normal + managedButton->width * normalImageY + normalImageX,
                managedButton->width);

            int pressedImageWidth = fontGetStringWidth(text) + 1;
            int pressedImageHeight = fontGetLineHeight() + 1;

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
                blitBufferToBuffer(managedButton->pressed + managedButton->width * pressedImageY + pressedImageX,
                    pressedImageWidth,
                    pressedImageHeight,
                    managedButton->width,
                    buffer,
                    pressedImageWidth);
            } else {
                memset(buffer, 0, pressedImageHeight * pressedImageWidth);
            }

            fontDrawText(buffer,
                text,
                pressedImageWidth,
                pressedImageWidth,
                widgetGetTextColor() + widgetGetTextFlags());

            blitBufferToBufferTrans(buffer,
                pressedImageWidth,
                normalImageHeight,
                normalImageWidth,
                managedButton->pressed + managedButton->width * pressedImageY + pressedImageX,
                managedButton->width);

            internal_free_safe(buffer, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 2078

            if ((managedButton->field_18 & 0x20) != 0) {
                buttonSetMask(managedButton->btn, managedButton->normal);
            }

            _win_register_button_image(managedButton->btn, managedButton->normal, managedButton->pressed, managedButton->hover, 0);

            return true;
        }
    }

    return false;
}

// 0x4BA694
bool _windowFill(float r, float g, float b)
{
    int colorIndex = ((int)(r * 31.0) << 10) | ((int)(g * 31.0) << 5) | (int)(b * 31.0);

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    windowFill(managedWindow->window,
        0,
        0,
        _windowWidth(),
        _windowHeight(),
        _colorTable[colorIndex]);

    return true;
}

// 0x4BA738
bool _windowFillRect(int x, int y, int width, int height, float r, float g, float b)
{
    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);

    x = (int)(x * managedWindow->field_54);
    y = (int)(y * managedWindow->field_58);
    width = (int)(width * managedWindow->field_54);
    height = (int)(height * managedWindow->field_58);

    int colorIndex = ((int)(r * 31.0) << 10) | ((int)(g * 31.0) << 5) | (int)(b * 31.0);

    windowFill(managedWindow->window,
        x,
        y,
        width,
        height,
        _colorTable[colorIndex]);

    return true;
}

// TODO: There is a value returned, not sure which one - could be either
// currentRegionIndex or points array. For now it can be safely ignored since
// the only caller of this function is opAddRegion, which ignores the returned
// value.
//
// 0x4BA844
void _windowEndRegion()
{
    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    Region* region = managedWindow->regions[managedWindow->currentRegionIndex];
    _windowAddRegionPoint(region->points->x, region->points->y, false);
    _regionSetBound(region);
}

// 0x4BA988
bool _windowCheckRegionExists(const char* regionName)
{
    if (gCurrentManagedWindowIndex == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
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
bool _windowStartRegion(int initialCapacity)
{
    if (gCurrentManagedWindowIndex == -1) {
        return false;
    }

    int newRegionIndex;
    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    if (managedWindow->regions == NULL) {
        managedWindow->regions = (Region**)internal_malloc_safe(sizeof(&(managedWindow->regions)), __FILE__, __LINE__); // "..\int\WINDOW.C", 2167
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
            managedWindow->regions = (Region**)internal_realloc_safe(managedWindow->regions, sizeof(&(managedWindow->regions)) * (managedWindow->regionsLength + 1), __FILE__, __LINE__); // "..\int\WINDOW.C", 2178
            managedWindow->regionsLength++;
        }
    }

    Region* newRegion;
    if (initialCapacity != 0) {
        newRegion = regionCreate(initialCapacity + 1);
    } else {
        newRegion = NULL;
    }

    managedWindow->regions[newRegionIndex] = newRegion;
    managedWindow->currentRegionIndex = newRegionIndex;

    return true;
}

// 0x4BAB68
bool _windowAddRegionPoint(int x, int y, bool a3)
{
    if (gCurrentManagedWindowIndex == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    Region* region = managedWindow->regions[managedWindow->currentRegionIndex];
    if (region == NULL) {
        region = managedWindow->regions[managedWindow->currentRegionIndex] = regionCreate(1);
    }

    if (a3) {
        x = (int)(x * managedWindow->field_54);
        y = (int)(y * managedWindow->field_58);
    }

    regionAddPoint(region, x, y);

    return true;
}

// 0x4BADC0
bool _windowAddRegionProc(const char* regionName, Program* program, int a3, int a4, int a5, int a6)
{
    if (gCurrentManagedWindowIndex == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    for (int index = 0; index < managedWindow->regionsLength; index++) {
        Region* region = managedWindow->regions[index];
        if (region != NULL) {
            if (stricmp(region->name, regionName) == 0) {
                region->field_50 = a3;
                region->proc = a4;
                region->field_48 = a5;
                region->field_4C = a6;
                region->program = program;
                return true;
            }
        }
    }

    return false;
}

// 0x4BAE8C
bool _windowAddRegionRightProc(const char* regionName, Program* program, int a3, int a4)
{
    if (gCurrentManagedWindowIndex == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
    for (int index = 0; index < managedWindow->regionsLength; index++) {
        Region* region = managedWindow->regions[index];
        if (region != NULL) {
            if (stricmp(region->name, regionName) == 0) {
                region->field_58 = a3;
                region->field_5C = a4;
                region->program = program;
                return true;
            }
        }
    }

    return false;
}

// 0x4BAF2C
bool _windowSetRegionFlag(const char* regionName, int value)
{
    if (gCurrentManagedWindowIndex != -1) {
        ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
        for (int index = 0; index < managedWindow->regionsLength; index++) {
            Region* region = managedWindow->regions[index];
            if (region != NULL) {
                if (stricmp(region->name, regionName) == 0) {
                    regionAddFlag(region, value);
                    return true;
                }
            }
        }
    }

    return false;
}

// 0x4BAFA8
bool _windowAddRegionName(const char* regionName)
{
    if (gCurrentManagedWindowIndex == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
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

    regionSetName(region, regionName);

    return true;
}

// Delete region with the specified name or all regions if it's NULL.
//
// 0x4BB0A8
bool _windowDeleteRegion(const char* regionName)
{
    if (gCurrentManagedWindowIndex == -1) {
        return false;
    }

    ManagedWindow* managedWindow = &(gManagedWindows[gCurrentManagedWindowIndex]);
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

        internal_free_safe(managedWindow->regions, __FILE__, __LINE__); // "..\int\WINDOW.C", 2353

        managedWindow->regions = NULL;
        managedWindow->regionsLength = 0;
    }

    return true;
}

// 0x4BB220
void _updateWindows()
{
    _movieUpdate();
    // TODO: Incomplete.
    // _mousemgrUpdate();
    _checkAllRegions();
    _update_widgets();
}

// 0x4BB234
int _windowMoviePlaying()
{
    return _moviePlaying();
}

// 0x4BB23C
bool _windowSetMovieFlags(int flags)
{
    if (movieSetFlags(flags) != 0) {
        return false;
    }
    
    return true;
}

// 0x4BB24C
bool _windowPlayMovie(char* filePath)
{
    if (_movieRun(gManagedWindows[gCurrentManagedWindowIndex].window, filePath) != 0) {
        return false;
    }

    return true;
}

// 0x4BB280
bool _windowPlayMovieRect(char* filePath, int a2, int a3, int a4, int a5)
{
    if (_movieRunRect(gManagedWindows[gCurrentManagedWindowIndex].window, filePath, a2, a3, a4, a5) != 0) {
        return false;
    }

    return true;
}

// 0x4BB2C4
void _windowStopMovie()
{
    _movieStop();
}

// 0x4BB3A8
void _drawScaled(unsigned char* dest, int destWidth, int destHeight, int destPitch, unsigned char* src, int srcWidth, int srcHeight, int srcPitch)
{
    // TODO: Incomplete.
}

// 0x4BB5D0
void _drawScaledBuf(unsigned char* dest, int destWidth, int destHeight, unsigned char* src, int srcWidth, int srcHeight)
{
    // TODO: Incomplete.
}

// 0x4BBFC4
void sub_4BBFC4(unsigned char* src, int srcWidth, int srcHeight, unsigned char* dest, int destWidth, int destHeight)
{
    // TODO: Incomplete.
}
