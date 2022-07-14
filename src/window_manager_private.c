#include "window_manager_private.h"

#include "color.h"
#include "core.h"
#include "draw.h"
#include "memory.h"
#include "text_font.h"
#include "window_manager.h"

#include <stdio.h>
#include <string.h>

// 0x51E414
int _wd = -1;

// 0x51E418
MenuBar* _curr_menu = NULL;

// 0x51E41C
bool _tm_watch_active = false;

// 0x6B2340
STRUCT_6B2340 _tm_location[5];

// 0x6B2368
int _tm_text_x;

// 0x6B236C
int _tm_h;

// 0x6B2370
STRUCT_6B2370 _tm_queue[5];

// 0x6B23AC
int _tm_persistence;

// 0x6B23B0
int _scr_center_x;

// 0x6B23B4
int _tm_text_y;

// 0x6B23B8
int _tm_kill;

// 0x6B23BC
int _tm_add;

// x
//
// 0x6B23C0
int _curry;

// y
//
// 0x6B23C4
int _currx;

// 0x6B23D0
char gProgramWindowTitle[256];

// 0x4DA6C0
int sub_4DA6C0(const char* title, char** fileList, int fileListLength, int a4, int x, int y, int a7)
{
    return sub_4DA70C(title, fileList, fileListLength, a4, x, y, a7, 0);
}

// 0x4DA70C
int sub_4DA70C(const char* title, char** fileList, int fileListLength, int a4, int x, int y, int a7, int a8)
{
    // TODO: Incomplete.
    return -1;
}

// 0x4DB478
int _win_get_str(char* dest, int length, const char* title, int x, int y)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    int titleWidth = fontGetStringWidth(title) + 12;
    if (titleWidth < fontGetMonospacedCharacterWidth() * length) {
        titleWidth = fontGetMonospacedCharacterWidth() * length;
    }

    int windowWidth = titleWidth + 16;
    if (windowWidth < 160) {
        windowWidth = 160;
    }

    int windowHeight = 5 * fontGetLineHeight() + 16;

    int win = windowCreate(x, y, windowWidth, windowHeight, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
    if (win == -1) {
        return -1;
    }

    windowDrawBorder(win);

    unsigned char* windowBuffer = windowGetBuffer(win);

    bufferFill(windowBuffer + windowWidth * (fontGetLineHeight() + 14) + 14,
        windowWidth - 28,
        fontGetLineHeight() + 2,
        windowWidth,
        _colorTable[_GNW_wcolor[0]]);
    fontDrawText(windowBuffer + windowWidth * 8 + 8, title, windowWidth, windowWidth, _colorTable[_GNW_wcolor[4]]);

    bufferDrawRectShadowed(windowBuffer,
        windowWidth,
        14,
        fontGetLineHeight() + 14,
        windowWidth - 14,
        2 * fontGetLineHeight() + 16,
        _colorTable[_GNW_wcolor[2]],
        _colorTable[_GNW_wcolor[1]]);

    _win_register_text_button(win,
        windowWidth / 2 - 72,
        windowHeight - 8 - fontGetLineHeight() - 6,
        -1,
        -1,
        -1,
        KEY_RETURN,
        "Done",
        0);

    _win_register_text_button(win,
        windowWidth / 2 + 8,
        windowHeight - 8 - fontGetLineHeight() - 6,
        -1,
        -1,
        -1,
        KEY_ESCAPE,
        "Cancel",
        0);

    windowRefresh(win);

    _win_input_str(win,
        dest,
        length,
        16,
        fontGetLineHeight() + 16,
        _colorTable[_GNW_wcolor[3]],
        _colorTable[_GNW_wcolor[0]]);

    windowDestroy(win);

    return 0;
}

// 0x4DBA98
int _win_msg(const char* string, int x, int y, int flags)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    int windowHeight = 3 * fontGetLineHeight() + 16;

    int windowWidth = fontGetStringWidth(string) + 16;
    if (windowWidth < 80) {
        windowWidth = 80;
    }

    windowWidth += 16;

    int win = windowCreate(x, y, windowWidth, windowHeight, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
    if (win == -1) {
        return -1;
    }

    windowDrawBorder(win);

    Window* window = windowGetWindow(win);
    unsigned char* windowBuffer = window->buffer;

    int color;
    if ((flags & 0xFF00) != 0) {
        int index = (flags & 0xFF) - 1;
        color = _colorTable[_GNW_wcolor[index]];
        color |= flags & ~0xFFFF;
    } else {
        color = flags;
    }

    fontDrawText(windowBuffer + windowWidth * 8 + 16, string, windowWidth, windowWidth, color);

    _win_register_text_button(win,
        windowWidth / 2 - 32,
        windowHeight - 8 - fontGetLineHeight() - 6,
        -1,
        -1,
        -1,
        KEY_ESCAPE,
        "Done",
        0);

    windowRefresh(win);

    while (_get_input() != KEY_ESCAPE) {
    }

    windowDestroy(win);

    return 0;
}

// 0x4DBC34
int _create_pull_down(char** stringList, int stringListLength, int x, int y, int a5, int a6, Rect* rect)
{
    int windowHeight = stringListLength * fontGetLineHeight() + 16;
    int windowWidth = _win_width_needed(stringList, stringListLength) + 4;
    if (windowHeight < 2 || windowWidth < 2) {
        return -1;
    }

    int win = windowCreate(x, y, windowWidth, windowHeight, a6, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
    if (win == -1) {
        return -1;
    }

    _win_text(win, stringList, stringListLength, windowWidth - 4, 2, 8, a5);
    windowDrawRect(win, 0, 0, windowWidth - 1, windowHeight - 1, _colorTable[0]);
    windowDrawRect(win, 1, 1, windowWidth - 2, windowHeight - 2, a5);
    windowRefresh(win);
    windowGetRect(win, rect);

    return win;
}

// 0x4DC30C
int _win_debug(char* string)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    int lineHeight = fontGetLineHeight();

    if (_wd == -1) {
        _wd = windowCreate(80, 80, 300, 192, 256, WINDOW_FLAG_0x04);
        if (_wd == -1) {
            return -1;
        }

        windowDrawBorder(_wd);

        Window* window = windowGetWindow(_wd);
        unsigned char* windowBuffer = window->buffer;

        windowFill(_wd, 8, 8, 284, lineHeight, 0x100 | 1);

        windowDrawText(_wd,
            "Debug",
            0,
            (300 - fontGetStringWidth("Debug")) / 2,
            8,
            0x2000000 | 0x100 | 4);

        bufferDrawRectShadowed(windowBuffer,
            300,
            8,
            8,
            291,
            lineHeight + 8,
            _colorTable[_GNW_wcolor[2]],
            _colorTable[_GNW_wcolor[1]]);

        windowFill(_wd, 9, 26, 282, 135, 0x100 | 1);

        bufferDrawRectShadowed(windowBuffer,
            300,
            8,
            25,
            291,
            lineHeight + 145,
            _colorTable[_GNW_wcolor[2]],
            _colorTable[_GNW_wcolor[1]]);

        _currx = 9;
        _curry = 26;

        int btn = _win_register_text_button(_wd,
            (300 - fontGetStringWidth("Close")) / 2,
            192 - 8 - lineHeight - 6,
            -1,
            -1,
            -1,
            -1,
            "Close",
            0);
        buttonSetMouseCallbacks(btn, NULL, NULL, NULL, _win_debug_delete);

        buttonCreate(_wd,
            8,
            8,
            284,
            lineHeight,
            -1,
            -1,
            -1,
            -1,
            NULL,
            NULL,
            NULL,
            BUTTON_FLAG_0x10);
    }

    char temp[2];
    temp[1] = '\0';

    char* pch = string;
    while (*pch != '\0') {
        int characterWidth = fontGetCharacterWidth(*pch);
        if (*pch == '\n' || _currx + characterWidth > 291) {
            _currx = 9;
            _curry += lineHeight;
        }

        while (160 - _curry < lineHeight) {
            Window* window = windowGetWindow(_wd);
            unsigned char* windowBuffer = window->buffer;
            blitBufferToBuffer(windowBuffer + lineHeight * 300 + 300 * 26 + 9,
                282,
                134 - lineHeight - 1,
                300,
                windowBuffer + 300 * 26 + 9,
                300);
            _curry -= lineHeight;
            windowFill(_wd, 9, _curry, 282, lineHeight, 0x100 | 1);
        }

        if (*pch != '\n') {
            temp[0] = *pch;
            windowDrawText(_wd, temp, 0, _currx, _curry, 0x2000000 | 0x100 | 4);
            _currx += characterWidth + fontGetLetterSpacing();
        }

        pch++;
    }

    windowRefresh(_wd);

    return 0;
}

// 0x4DC65C
void _win_debug_delete(int btn, int keyCode)
{
    windowDestroy(_wd);
    _wd = -1;
}

// 0x4DC674
int _win_register_menu_bar(int win, int x, int y, int width, int height, int borderColor, int backgroundColor)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return -1;
    }

    if (window == NULL) {
        return -1;
    }

    if (window->menuBar != NULL) {
        return -1;
    }

    int right = x + width;
    if (right > window->width) {
        return -1;
    }

    int bottom = y + height;
    if (bottom > window->height) {
        return -1;
    }

    MenuBar* menuBar = window->menuBar = (MenuBar*)internal_malloc(sizeof(MenuBar));
    if (menuBar == NULL) {
        return -1;
    }

    menuBar->win = win;
    menuBar->rect.left = x;
    menuBar->rect.top = y;
    menuBar->rect.right = right - 1;
    menuBar->rect.bottom = bottom - 1;
    menuBar->pulldownsLength = 0;
    menuBar->borderColor = borderColor;
    menuBar->backgroundColor = backgroundColor;

    windowFill(win, x, y, width, height, backgroundColor);
    windowDrawRect(win, x, y, right - 1, bottom - 1, borderColor);

    return 0;
}

// 0x4DC768
int _win_register_menu_pulldown(int win, int x, char* str, int a4)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return -1;
    }

    if (window == NULL) {
        return -1;
    }

    MenuBar* menuBar = window->menuBar;
    if (menuBar == NULL) {
        return -1;
    }

    if (window->menuBar->pulldownsLength == 15) {
        return -1;
    }

    int btn = buttonCreate(win,
        menuBar->rect.left + x,
        (menuBar->rect.top + menuBar->rect.bottom - fontGetLineHeight()) / 2,
        fontGetStringWidth(str),
        fontGetLineHeight(),
        -1,
        -1,
        a4,
        -1,
        NULL,
        NULL,
        NULL,
        0);
    if (btn == -1) {
        return -1;
    }

    // TODO: Incomplete.

    return 0;
}

// 0x4DC9F0
int _find_first_letter(int ch, char** stringList, int stringListLength)
{
    if (ch >= 'A' && ch <= 'Z') {
        ch += ' ';
    }

    for (int index = 0; index < stringListLength; index++) {
        char* string = stringList[index];
        if (string[0] == ch || string[0] == ch - ' ') {
            return index;
        }
    }
    
    return -1;
}

// 0x4DCA30
int _win_width_needed(char** fileNameList, int fileNameListLength)
{
    int maxWidth = 0;

    for (int index = 0; index < fileNameListLength; index++) {
        int width = fontGetStringWidth(fileNameList[index]);
        if (width > maxWidth) {
            maxWidth = width;
        }
    }

    return maxWidth;
}

// 0x4DCA5C
int _win_input_str(int win, char* dest, int maxLength, int x, int y, int textColor, int backgroundColor)
{
    Window* window = windowGetWindow(win);
    unsigned char* buffer = window->buffer + window->width * y + x;

    int cursorPos = strlen(dest);
    dest[cursorPos] = '_';
    dest[cursorPos + 1] = '\0';

    int lineHeight = fontGetLineHeight();
    int stringWidth = fontGetStringWidth(dest);
    bufferFill(buffer, stringWidth, lineHeight, window->width, backgroundColor);
    fontDrawText(buffer, dest, stringWidth, window->width, textColor);

    Rect dirtyRect;
    dirtyRect.left = window->rect.left + x;
    dirtyRect.top = window->rect.top + y;
    dirtyRect.right = dirtyRect.left + stringWidth;
    dirtyRect.bottom = dirtyRect.top + lineHeight;
    _GNW_win_refresh(window, &dirtyRect, NULL);

    // NOTE: This loop is slightly different compared to other input handling
    // loops. Cursor position is managed inside an incrementing loop. Cursor is
    // decremented in the loop body when key is not handled.
    bool isFirstKey = true;
    for (; cursorPos <= maxLength; cursorPos++) {
        int keyCode = _get_input();
        if (keyCode != -1) {
            if (keyCode == KEY_ESCAPE) {
                dest[cursorPos] = '\0';
                return -1;
            }

            if (keyCode == KEY_BACKSPACE) {
                if (cursorPos > 0) {
                    stringWidth = fontGetStringWidth(dest);

                    if (isFirstKey) {
                        bufferFill(buffer, stringWidth, lineHeight, window->width, backgroundColor);

                        dirtyRect.left = window->rect.left + x;
                        dirtyRect.top = window->rect.top + y;
                        dirtyRect.right = dirtyRect.left + stringWidth;
                        dirtyRect.bottom = dirtyRect.top + lineHeight;
                        _GNW_win_refresh(window, &dirtyRect, NULL);

                        dest[0] = '_';
                        dest[1] = '\0';
                        cursorPos = 1;
                    } else {
                        dest[cursorPos] = ' ';
                        dest[cursorPos - 1] = '_';
                    }

                    bufferFill(buffer, stringWidth, lineHeight, window->width, backgroundColor);
                    fontDrawText(buffer, dest, stringWidth, window->width, textColor);

                    dirtyRect.left = window->rect.left + x;
                    dirtyRect.top = window->rect.top + y;
                    dirtyRect.right = dirtyRect.left + stringWidth;
                    dirtyRect.bottom = dirtyRect.top + lineHeight;
                    _GNW_win_refresh(window, &dirtyRect, NULL);

                    dest[cursorPos] = '\0';
                    cursorPos -= 2;

                    isFirstKey = false;
                } else {
                    cursorPos--;
                }
            } else if (keyCode == KEY_RETURN) {
                break;
            } else {
                if (cursorPos == maxLength) {
                    cursorPos = maxLength - 1;
                } else {
                    if (keyCode > 0 && keyCode < 256) {
                        dest[cursorPos] = keyCode;
                        dest[cursorPos + 1] = '_';
                        dest[cursorPos + 2] = '\0';

                        int stringWidth = fontGetStringWidth(dest);
                        bufferFill(buffer, stringWidth, lineHeight, window->width, backgroundColor);
                        fontDrawText(buffer, dest, stringWidth, window->width, textColor);

                        dirtyRect.left = window->rect.left + x;
                        dirtyRect.top = window->rect.top + y;
                        dirtyRect.right = dirtyRect.left + stringWidth;
                        dirtyRect.bottom = dirtyRect.top + lineHeight;
                        _GNW_win_refresh(window, &dirtyRect, NULL);

                        isFirstKey = false;
                    } else {
                        cursorPos--;
                    }
                }
            }
        } else {
            cursorPos--;
        }
    }

    dest[cursorPos] = '\0';

    return 0;
}

// 0x4DBD04
int sub_4DBD04(int win, Rect* rect, char** items, int itemsLength, int a5, int a6, MenuBar* menuBar, int pulldownIndex)
{
    // TODO: Incomplete.
    return -1;
}

// 0x4DC930
int _GNW_process_menu(MenuBar* menuBar, int pulldownIndex)
{
    if (_curr_menu != NULL) {
        return -1;
    }

    _curr_menu = menuBar;

    int keyCode;
    Rect rect;
    do {
        MenuPulldown* pulldown = &(menuBar->pulldowns[pulldownIndex]);
        int win = _create_pull_down(pulldown->items,
            pulldown->itemsLength,
            pulldown->rect.left,
            menuBar->rect.bottom + 1,
            pulldown->field_1C,
            pulldown->field_20,
            &rect);
        if (win == -1) {
            _curr_menu = NULL;
            return -1;
        }

        keyCode = sub_4DBD04(win, &rect, pulldown->items, pulldown->itemsLength, pulldown->field_1C, pulldown->field_20, menuBar, pulldownIndex);
        if (keyCode < -1) {
            pulldownIndex = -2 - keyCode;
        }
    } while (keyCode < -1);

    if (keyCode != -1) {
        inputEventQueueReset();
        enqueueInputEvent(keyCode);
        keyCode = menuBar->pulldowns[pulldownIndex].keyCode;
    }

    _curr_menu = NULL;

    return keyCode;
}

// Calculates max length of string needed to represent a1 or a2.
//
// 0x4DD03C
int _calc_max_field_chars_wcursor(int a1, int a2)
{
    char* str = (char*)internal_malloc(17);
    if (str == NULL) {
        return -1;
    }

    sprintf(str, "%d", a1);
    int len1 = strlen(str);

    sprintf(str, "%d", a2);
    int len2 = strlen(str);

    internal_free(str);

    return max(len1, len2) + 1;
}

// 0x4DD3EC
void _GNW_intr_init()
{
    int v1, v2;
    int i;

    _tm_persistence = 3000;
    _tm_add = 0;
    _tm_kill = -1;
    _scr_center_x = _scr_size.right / 2;

    if (_scr_size.bottom >= 479) {
        _tm_text_y = 16;
        _tm_text_x = 16;
    } else {
        _tm_text_y = 10;
        _tm_text_x = 10;
    }

    _tm_h = 2 * _tm_text_y + fontGetLineHeight();

    v1 = _scr_size.bottom >> 3;
    v2 = _scr_size.bottom >> 2;

    for (i = 0; i < 5; i++) {
        _tm_location[i].field_4 = v1 * i + v2;
        _tm_location[i].field_0 = 0;
    }
}

// 0x4DD4A4
void _GNW_intr_exit()
{
    tickersRemove(_tm_watch_msgs);
    while (_tm_kill != -1) {
        _tm_kill_msg();
    }
}

// 0x4DD66C
void _tm_watch_msgs()
{
    if (_tm_watch_active) {
        return;
    }

    _tm_watch_active = 1;
    while (_tm_kill != -1) {
        if (getTicksSince(_tm_queue[_tm_kill].field_0) < _tm_persistence) {
            break;
        }

        _tm_kill_msg();
    }
    _tm_watch_active = 0;
}

// 0x4DD6C0
void _tm_kill_msg()
{
    int v0;

    v0 = _tm_kill;
    if (v0 != -1) {
        windowDestroy(_tm_queue[_tm_kill].field_4);
        _tm_location[_tm_queue[_tm_kill].field_8].field_0 = 0;

        if (v0 == 5) {
            v0 = 0;
        }

        if (v0 == _tm_add) {
            _tm_add = 0;
            _tm_kill = -1;
            tickersRemove(_tm_watch_msgs);
            v0 = _tm_kill;
        }
    }

    _tm_kill = v0;
}

// 0x4DD744
void _tm_kill_out_of_order(int a1)
{
    int v7;
    int v6;

    if (_tm_kill == -1) {
        return;
    }

    if (!_tm_index_active(a1)) {
        return;
    }

    windowDestroy(_tm_queue[a1].field_4);

    _tm_location[_tm_queue[a1].field_8].field_0 = 0;

    if (a1 != _tm_kill) {
        v6 = a1;
        do {
            v7 = v6 - 1;
            if (v7 < 0) {
                v7 = 4;
            }

            memcpy(&(_tm_queue[v6]), &(_tm_queue[v7]), sizeof(STRUCT_6B2370));
            v6 = v7;
        } while (v7 != _tm_kill);
    }

    if (++_tm_kill == 5) {
        _tm_kill = 0;
    }

    if (_tm_add == _tm_kill) {
        _tm_add = 0;
        _tm_kill = -1;
        tickersRemove(_tm_watch_msgs);
    }
}

// 0x4DD82C
void _tm_click_response(int btn)
{
    int win;
    int v3;

    if (_tm_kill == -1) {
        return;
    }

    win = buttonGetWindowId(btn);
    v3 = _tm_kill;
    while (win != _tm_queue[v3].field_4) {
        v3++;
        if (v3 == 5) {
            v3 = 0;
        }

        if (v3 == _tm_kill || !_tm_index_active(v3))
            return;
    }

    _tm_kill_out_of_order(v3);
}

// 0x4DD870
int _tm_index_active(int a1)
{
    if (_tm_kill != _tm_add) {
        if (_tm_kill >= _tm_add) {
            if (a1 >= _tm_add && a1 < _tm_kill)
                return 0;
        } else if (a1 < _tm_kill || a1 >= _tm_add) {
            return 0;
        }
    }
    return 1;
}
