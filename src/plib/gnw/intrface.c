#include "plib/gnw/intrface.h"

#include <stdio.h>
#include <string.h>

#include "plib/color/color.h"
#include "core.h"
#include "draw.h"
#include "plib/gnw/memory.h"
#include "plib/gnw/text.h"
#include "plib/gnw/gnw.h"

static int create_pull_down(char** stringList, int stringListLength, int x, int y, int a5, int a6, Rect* rect);
static void win_debug_delete(int btn, int keyCode);
static int find_first_letter(int ch, char** stringList, int stringListLength);
static int process_pull_down(int win, Rect* rect, char** items, int itemsLength, int a5, int a6, MenuBar* menuBar, int pulldownIndex);
static int calc_max_field_chars_wcursor(int a1, int a2);
static void tm_watch_msgs();
static void tm_kill_msg();
static void tm_kill_out_of_order(int a1);
static void tm_click_response(int btn);
static int tm_index_active(int a1);

// 0x51E414
static int wd = -1;

// 0x51E41C
static bool tm_watch_active = false;

// 0x6B2340
static struct {
    int taken;
    int y;
} tm_location[5];

// 0x6B2368
static int tm_text_x;

// 0x6B236C
static int tm_h;

// 0x6B2370
static struct {
    int created;
    int id;
    int location;
} tm_queue[5];

// 0x6B23AC
static unsigned int tm_persistence;

// 0x6B23B0
static int scr_center_x;

// 0x6B23B4
int tm_text_y;

// 0x6B23B8
int tm_kill;

// 0x6B23BC
int tm_add;

// 0x6B23C0
int curry;

// 0x6B23C4
int currx;

// 0x4DA6C0
int win_list_select(const char* title, char** fileList, int fileListLength, SelectFunc* callback, int x, int y, int a7)
{
    return win_list_select_at(title, fileList, fileListLength, callback, x, y, a7, 0);
}

// 0x4DA70C
int win_list_select_at(const char* title, char** items, int itemsLength, SelectFunc* callback, int x, int y, int a7, int a8)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    int listViewWidth = win_width_needed(items, itemsLength);
    int windowWidth = listViewWidth + 16;

    int titleWidth = text_width(title);
    if (titleWidth > windowWidth) {
        windowWidth = titleWidth;
        listViewWidth = titleWidth - 16;
    }

    windowWidth += 20;

    int win;
    int windowHeight;
    int listViewCapacity = 10;
    for (int heightMultiplier = 13; heightMultiplier > 8; heightMultiplier--) {
        windowHeight = heightMultiplier * text_height() + 22;
        win = win_add(x, y, windowWidth, windowHeight, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
        if (win != -1) {
            break;
        }
        listViewCapacity--;
    }

    if (win == -1) {
        return -1;
    }

    Window* window = GNW_find(win);
    Rect* windowRect = &(window->rect);
    unsigned char* windowBuffer = window->buffer;

    bufferDrawRect(windowBuffer,
        windowWidth,
        0,
        0,
        windowWidth - 1,
        windowHeight - 1,
        colorTable[0]);
    bufferDrawRectShadowed(windowBuffer,
        windowWidth,
        1,
        1,
        windowWidth - 2,
        windowHeight - 2,
        colorTable[GNW_wcolor[1]],
        colorTable[GNW_wcolor[2]]);

    bufferFill(windowBuffer + windowWidth * 5 + 5,
        windowWidth - 11,
        text_height() + 3,
        windowWidth,
        colorTable[GNW_wcolor[0]]);

    text_to_buf(windowBuffer + windowWidth / 2 + 8 * windowWidth - text_width(title) / 2,
        title,
        windowWidth,
        windowWidth,
        colorTable[GNW_wcolor[3]]);

    bufferDrawRectShadowed(windowBuffer,
        windowWidth,
        5,
        5,
        windowWidth - 6,
        text_height() + 8,
        colorTable[GNW_wcolor[2]],
        colorTable[GNW_wcolor[1]]);

    int listViewX = 8;
    int listViewY = text_height() + 16;
    unsigned char* listViewBuffer = windowBuffer + windowWidth * listViewY + listViewX;
    int listViewMaxY = listViewCapacity * text_height() + listViewY;

    bufferFill(listViewBuffer + windowWidth * (-2) + (-3),
        listViewWidth + listViewX - 2,
        listViewCapacity * text_height() + 2,
        windowWidth,
        colorTable[GNW_wcolor[0]]);

    int scrollOffset = a8;
    if (a8 < 0 || a8 >= itemsLength) {
        scrollOffset = 0;
    }

    // Relative to `scrollOffset`.
    int selectedItemIndex;
    if (itemsLength - scrollOffset < listViewCapacity) {
        int newScrollOffset = itemsLength - listViewCapacity;
        if (newScrollOffset < 0) {
            newScrollOffset = 0;
        }
        int oldScrollOffset = scrollOffset;
        scrollOffset = newScrollOffset;
        selectedItemIndex = oldScrollOffset - newScrollOffset;
    } else {
        selectedItemIndex = 0;
    }

    char** itemsTO = items + a8;
    win_text(win,
        items + a8,
        itemsLength < listViewCapacity ? itemsLength : listViewCapacity,
        listViewWidth,
        listViewX,
        listViewY,
        a7 | 0x2000000);

    _lighten_buf(listViewBuffer + windowWidth * selectedItemIndex * text_height(),
        listViewWidth,
        text_height(),
        windowWidth);

    bufferDrawRectShadowed(windowBuffer,
        windowWidth,
        5,
        listViewY - 3,
        listViewWidth + 10,
        listViewMaxY,
        colorTable[GNW_wcolor[2]],
        colorTable[GNW_wcolor[1]]);

    win_register_text_button(win,
        windowWidth - 25,
        listViewY - 3,
        -1,
        -1,
        KEY_ARROW_UP,
        -1,
        "\x18",
        0);

    win_register_text_button(win,
        windowWidth - 25,
        listViewMaxY - text_height() - 5,
        -1,
        -1,
        KEY_ARROW_DOWN,
        -1,
        "\x19",
        0);

    win_register_text_button(win,
        windowWidth / 2 - 32,
        windowHeight - 8 - text_height() - 6,
        -1,
        -1,
        -1,
        KEY_ESCAPE,
        "Done",
        0);

    int scrollbarX = windowWidth - 21;
    int scrollbarY = listViewY + text_height() + 7;
    int scrollbarKnobSize = 14;
    int scrollbarHeight = listViewMaxY - scrollbarY;
    unsigned char* scrollbarBuffer = windowBuffer + windowWidth * scrollbarY + scrollbarX;

    bufferFill(scrollbarBuffer,
        scrollbarKnobSize + 1,
        scrollbarHeight - text_height() - 8,
        windowWidth,
        colorTable[GNW_wcolor[0]]);

    win_register_button(win,
        scrollbarX,
        scrollbarY,
        scrollbarKnobSize + 1,
        scrollbarHeight - text_height() - 8,
        -1,
        -1,
        2048,
        -1,
        NULL,
        NULL,
        NULL,
        0);

    bufferDrawRectShadowed(windowBuffer,
        windowWidth,
        windowWidth - 22,
        scrollbarY - 1,
        scrollbarX + scrollbarKnobSize + 1,
        listViewMaxY - text_height() - 9,
        colorTable[GNW_wcolor[2]],
        colorTable[GNW_wcolor[1]]);
    bufferDrawRectShadowed(windowBuffer,
        windowWidth,
        scrollbarX,
        scrollbarY,
        scrollbarX + scrollbarKnobSize,
        scrollbarY + scrollbarKnobSize,
        colorTable[GNW_wcolor[1]],
        colorTable[GNW_wcolor[2]]);

    _lighten_buf(scrollbarBuffer, scrollbarKnobSize, scrollbarKnobSize, windowWidth);

    for (int index = 0; index < listViewCapacity; index++) {
        win_register_button(win,
            listViewX,
            listViewY + index * text_height(),
            listViewWidth,
            text_height(),
            512 + index,
            -1,
            1024 + index,
            -1,
            NULL,
            NULL,
            NULL,
            0);
    }

    win_register_button(win,
        0,
        0,
        windowWidth,
        text_height() + 8,
        -1,
        -1,
        -1,
        -1,
        NULL,
        NULL,
        NULL,
        BUTTON_FLAG_0x10);

    win_draw(win);

    int absoluteSelectedItemIndex = -1;

    // Relative to `scrollOffset`.
    int previousSelectedItemIndex = -1;
    while (1) {
        int keyCode = _get_input();
        int mouseX;
        int mouseY;
        mouse_get_position(&mouseX, &mouseY);

        if (keyCode == KEY_RETURN || (keyCode >= 1024 && keyCode < listViewCapacity + 1024)) {
            if (selectedItemIndex != -1) {
                absoluteSelectedItemIndex = scrollOffset + selectedItemIndex;
                if (absoluteSelectedItemIndex < itemsLength) {
                    if (callback == NULL) {
                        break;
                    }

                    callback(items, absoluteSelectedItemIndex);
                }
                absoluteSelectedItemIndex = -1;
            }
        } else if (keyCode == 2048) {
            if (window->rect.uly + scrollbarY > mouseY) {
                keyCode = KEY_PAGE_UP;
            } else if (window->rect.uly + scrollbarKnobSize + scrollbarY < mouseY) {
                keyCode = KEY_PAGE_DOWN;
            }
        }

        if (keyCode == KEY_ESCAPE) {
            break;
        }

        if (keyCode >= 512 && keyCode < listViewCapacity + 512) {
            int itemIndex = keyCode - 512;
            if (itemIndex != selectedItemIndex && itemIndex < itemsLength) {
                previousSelectedItemIndex = selectedItemIndex;
                selectedItemIndex = itemIndex;
                keyCode = -3;
            } else {
                continue;
            }
        } else {
            switch (keyCode) {
            case KEY_HOME:
                if (scrollOffset > 0) {
                    keyCode = -4;
                    scrollOffset = 0;
                }
                break;
            case KEY_ARROW_UP:
                if (selectedItemIndex > 0) {
                    keyCode = -3;
                    previousSelectedItemIndex = selectedItemIndex;
                    selectedItemIndex -= 1;
                } else {
                    if (scrollOffset > 0) {
                        keyCode = -4;
                        scrollOffset -= 1;
                    }
                }
                break;
            case KEY_PAGE_UP:
                if (scrollOffset > 0) {
                    scrollOffset -= listViewCapacity;
                    if (scrollOffset < 0) {
                        scrollOffset = 0;
                    }
                    keyCode = -4;
                }
                break;
            case KEY_END:
                if (scrollOffset < itemsLength - listViewCapacity) {
                    keyCode = -4;
                    scrollOffset = itemsLength - listViewCapacity;
                }
                break;
            case KEY_ARROW_DOWN:
                if (selectedItemIndex < listViewCapacity - 1 && selectedItemIndex < itemsLength - 1) {
                    keyCode = -3;
                    previousSelectedItemIndex = selectedItemIndex;
                    selectedItemIndex += 1;
                } else {
                    if (scrollOffset + listViewCapacity < itemsLength) {
                        keyCode = -4;
                        scrollOffset += 1;
                    }
                }
                break;
            case KEY_PAGE_DOWN:
                if (scrollOffset < itemsLength - listViewCapacity) {
                    scrollOffset += listViewCapacity;
                    if (scrollOffset > itemsLength - listViewCapacity) {
                        scrollOffset = itemsLength - listViewCapacity;
                    }
                    keyCode = -4;
                }
                break;
            default:
                if (itemsLength > listViewCapacity) {
                    if ((keyCode >= 'a' && keyCode <= 'z')
                        || (keyCode >= 'A' && keyCode <= 'Z')) {
                        int found = find_first_letter(keyCode, items, itemsLength);
                        if (found != -1) {
                            scrollOffset = found;
                            if (scrollOffset > itemsLength - listViewCapacity) {
                                scrollOffset = itemsLength - listViewCapacity;
                            }
                            keyCode = -4;
                            selectedItemIndex = found - scrollOffset;
                        }
                    }
                }
                break;
            }
        }

        if (keyCode == -4) {
            bufferFill(listViewBuffer,
                listViewWidth,
                listViewMaxY - listViewY,
                windowWidth,
                colorTable[GNW_wcolor[0]]);

            win_text(win,
                items + scrollOffset,
                itemsLength < listViewCapacity ? itemsLength : listViewCapacity,
                listViewWidth,
                listViewX,
                listViewY,
                a7 | 0x2000000);

            _lighten_buf(listViewBuffer + windowWidth * selectedItemIndex * text_height(),
                listViewWidth,
                text_height(),
                windowWidth);

            if (itemsLength > listViewCapacity) {
                bufferFill(windowBuffer + windowWidth * scrollbarY + scrollbarX,
                    scrollbarKnobSize + 1,
                    scrollbarKnobSize + 1,
                    windowWidth,
                    colorTable[GNW_wcolor[0]]);

                scrollbarY = (scrollOffset * (listViewMaxY - listViewY - 2 * text_height() - 16 - scrollbarKnobSize - 1)) / (itemsLength - listViewCapacity)
                    + listViewY + text_height() + 7;

                bufferDrawRectShadowed(windowBuffer,
                    windowWidth,
                    scrollbarX,
                    scrollbarY,
                    scrollbarX + scrollbarKnobSize,
                    scrollbarY + scrollbarKnobSize,
                    colorTable[GNW_wcolor[1]],
                    colorTable[GNW_wcolor[2]]);

                _lighten_buf(windowBuffer + windowWidth * scrollbarY + scrollbarX,
                    scrollbarKnobSize,
                    scrollbarKnobSize,
                    windowWidth);

                GNW_win_refresh(window, windowRect, NULL);
            }
        } else if (keyCode == -3) {
            Rect itemRect;
            itemRect.ulx = windowRect->ulx + listViewX;
            itemRect.lrx = itemRect.ulx + listViewWidth;

            if (previousSelectedItemIndex != -1) {
                itemRect.uly = windowRect->uly + listViewY + previousSelectedItemIndex * text_height();
                itemRect.lry = itemRect.uly + text_height();

                bufferFill(listViewBuffer + windowWidth * previousSelectedItemIndex * text_height(),
                    listViewWidth,
                    text_height(),
                    windowWidth,
                    colorTable[GNW_wcolor[0]]);

                int color;
                if ((a7 & 0xFF00) != 0) {
                    int colorIndex = (a7 & 0xFF) - 1;
                    color = (a7 & ~0xFFFF) | colorTable[GNW_wcolor[colorIndex]];
                } else {
                    color = a7;
                }

                text_to_buf(listViewBuffer + windowWidth * previousSelectedItemIndex * text_height(),
                    items[scrollOffset + previousSelectedItemIndex],
                    windowWidth,
                    windowWidth,
                    color);

                GNW_win_refresh(window, &itemRect, NULL);
            }

            if (selectedItemIndex != -1) {
                itemRect.uly = windowRect->uly + listViewY + selectedItemIndex * text_height();
                itemRect.lry = itemRect.uly + text_height();

                _lighten_buf(listViewBuffer + windowWidth * selectedItemIndex * text_height(),
                    listViewWidth,
                    text_height(),
                    windowWidth);

                GNW_win_refresh(window, &itemRect, NULL);
            }
        }
    }

    win_delete(win);

    return absoluteSelectedItemIndex;
}

// 0x4DB478
int win_get_str(char* dest, int length, const char* title, int x, int y)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    int titleWidth = text_width(title) + 12;
    if (titleWidth < text_max() * length) {
        titleWidth = text_max() * length;
    }

    int windowWidth = titleWidth + 16;
    if (windowWidth < 160) {
        windowWidth = 160;
    }

    int windowHeight = 5 * text_height() + 16;

    int win = win_add(x, y, windowWidth, windowHeight, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
    if (win == -1) {
        return -1;
    }

    win_border(win);

    unsigned char* windowBuffer = win_get_buf(win);

    bufferFill(windowBuffer + windowWidth * (text_height() + 14) + 14,
        windowWidth - 28,
        text_height() + 2,
        windowWidth,
        colorTable[GNW_wcolor[0]]);
    text_to_buf(windowBuffer + windowWidth * 8 + 8, title, windowWidth, windowWidth, colorTable[GNW_wcolor[4]]);

    bufferDrawRectShadowed(windowBuffer,
        windowWidth,
        14,
        text_height() + 14,
        windowWidth - 14,
        2 * text_height() + 16,
        colorTable[GNW_wcolor[2]],
        colorTable[GNW_wcolor[1]]);

    win_register_text_button(win,
        windowWidth / 2 - 72,
        windowHeight - 8 - text_height() - 6,
        -1,
        -1,
        -1,
        KEY_RETURN,
        "Done",
        0);

    win_register_text_button(win,
        windowWidth / 2 + 8,
        windowHeight - 8 - text_height() - 6,
        -1,
        -1,
        -1,
        KEY_ESCAPE,
        "Cancel",
        0);

    win_draw(win);

    win_input_str(win,
        dest,
        length,
        16,
        text_height() + 16,
        colorTable[GNW_wcolor[3]],
        colorTable[GNW_wcolor[0]]);

    win_delete(win);

    return 0;
}

// 0x4DBA98
int win_msg(const char* string, int x, int y, int flags)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    int windowHeight = 3 * text_height() + 16;

    int windowWidth = text_width(string) + 16;
    if (windowWidth < 80) {
        windowWidth = 80;
    }

    windowWidth += 16;

    int win = win_add(x, y, windowWidth, windowHeight, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
    if (win == -1) {
        return -1;
    }

    win_border(win);

    Window* window = GNW_find(win);
    unsigned char* windowBuffer = window->buffer;

    int color;
    if ((flags & 0xFF00) != 0) {
        int index = (flags & 0xFF) - 1;
        color = colorTable[GNW_wcolor[index]];
        color |= flags & ~0xFFFF;
    } else {
        color = flags;
    }

    text_to_buf(windowBuffer + windowWidth * 8 + 16, string, windowWidth, windowWidth, color);

    win_register_text_button(win,
        windowWidth / 2 - 32,
        windowHeight - 8 - text_height() - 6,
        -1,
        -1,
        -1,
        KEY_ESCAPE,
        "Done",
        0);

    win_draw(win);

    while (_get_input() != KEY_ESCAPE) {
    }

    win_delete(win);

    return 0;
}

// 0x4DBBC4
int win_pull_down(char** items, int itemsLength, int x, int y, int a5)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    Rect rect;
    int win = create_pull_down(items, itemsLength, x, y, a5, colorTable[GNW_wcolor[0]], &rect);
    if (win == -1) {
        return -1;
    }

    return process_pull_down(win, &rect, items, itemsLength, a5, colorTable[GNW_wcolor[0]], NULL, -1);
}

// 0x4DBC34
static int create_pull_down(char** stringList, int stringListLength, int x, int y, int a5, int a6, Rect* rect)
{
    int windowHeight = stringListLength * text_height() + 16;
    int windowWidth = win_width_needed(stringList, stringListLength) + 4;
    if (windowHeight < 2 || windowWidth < 2) {
        return -1;
    }

    int win = win_add(x, y, windowWidth, windowHeight, a6, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
    if (win == -1) {
        return -1;
    }

    win_text(win, stringList, stringListLength, windowWidth - 4, 2, 8, a5);
    win_box(win, 0, 0, windowWidth - 1, windowHeight - 1, colorTable[0]);
    win_box(win, 1, 1, windowWidth - 2, windowHeight - 2, a5);
    win_draw(win);
    win_get_rect(win, rect);

    return win;
}

// 0x4DC30C
int win_debug(char* string)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    int lineHeight = text_height();

    if (wd == -1) {
        wd = win_add(80, 80, 300, 192, 256, WINDOW_FLAG_0x04);
        if (wd == -1) {
            return -1;
        }

        win_border(wd);

        Window* window = GNW_find(wd);
        unsigned char* windowBuffer = window->buffer;

        win_fill(wd, 8, 8, 284, lineHeight, 0x100 | 1);

        win_print(wd,
            "Debug",
            0,
            (300 - text_width("Debug")) / 2,
            8,
            0x2000000 | 0x100 | 4);

        bufferDrawRectShadowed(windowBuffer,
            300,
            8,
            8,
            291,
            lineHeight + 8,
            colorTable[GNW_wcolor[2]],
            colorTable[GNW_wcolor[1]]);

        win_fill(wd, 9, 26, 282, 135, 0x100 | 1);

        bufferDrawRectShadowed(windowBuffer,
            300,
            8,
            25,
            291,
            lineHeight + 145,
            colorTable[GNW_wcolor[2]],
            colorTable[GNW_wcolor[1]]);

        currx = 9;
        curry = 26;

        int btn = win_register_text_button(wd,
            (300 - text_width("Close")) / 2,
            192 - 8 - lineHeight - 6,
            -1,
            -1,
            -1,
            -1,
            "Close",
            0);
        win_register_button_func(btn, NULL, NULL, NULL, win_debug_delete);

        win_register_button(wd,
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
        int characterWidth = text_char_width(*pch);
        if (*pch == '\n' || currx + characterWidth > 291) {
            currx = 9;
            curry += lineHeight;
        }

        while (160 - curry < lineHeight) {
            Window* window = GNW_find(wd);
            unsigned char* windowBuffer = window->buffer;
            blitBufferToBuffer(windowBuffer + lineHeight * 300 + 300 * 26 + 9,
                282,
                134 - lineHeight - 1,
                300,
                windowBuffer + 300 * 26 + 9,
                300);
            curry -= lineHeight;
            win_fill(wd, 9, curry, 282, lineHeight, 0x100 | 1);
        }

        if (*pch != '\n') {
            temp[0] = *pch;
            win_print(wd, temp, 0, currx, curry, 0x2000000 | 0x100 | 4);
            currx += characterWidth + text_spacing();
        }

        pch++;
    }

    win_draw(wd);

    return 0;
}

// 0x4DC65C
static void win_debug_delete(int btn, int keyCode)
{
    win_delete(wd);
    wd = -1;
}

// 0x4DC674
int win_register_menu_bar(int win, int x, int y, int width, int height, int borderColor, int backgroundColor)
{
    Window* window = GNW_find(win);

    if (!GNW_win_init_flag) {
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

    MenuBar* menuBar = window->menuBar = (MenuBar*)mem_malloc(sizeof(MenuBar));
    if (menuBar == NULL) {
        return -1;
    }

    menuBar->win = win;
    menuBar->rect.ulx = x;
    menuBar->rect.uly = y;
    menuBar->rect.lrx = right - 1;
    menuBar->rect.lry = bottom - 1;
    menuBar->pulldownsLength = 0;
    menuBar->borderColor = borderColor;
    menuBar->backgroundColor = backgroundColor;

    win_fill(win, x, y, width, height, backgroundColor);
    win_box(win, x, y, right - 1, bottom - 1, borderColor);

    return 0;
}

// 0x4DC768
int win_register_menu_pulldown(int win, int x, char* title, int keyCode, int itemsLength, char** items, int a7, int a8)
{
    Window* window = GNW_find(win);

    if (!GNW_win_init_flag) {
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

    int titleX = menuBar->rect.ulx + x;
    int titleY = (menuBar->rect.uly + menuBar->rect.lry - text_height()) / 2;
    int btn = win_register_button(win,
        titleX,
        titleY,
        text_width(title),
        text_height(),
        -1,
        -1,
        keyCode,
        -1,
        NULL,
        NULL,
        NULL,
        0);
    if (btn == -1) {
        return -1;
    }

    win_print(win, title, 0, titleX, titleY, window->menuBar->borderColor | 0x2000000);

    MenuPulldown* pulldown = &(window->menuBar->pulldowns[window->menuBar->pulldownsLength]);
    pulldown->rect.ulx = titleX;
    pulldown->rect.uly = titleY;
    pulldown->rect.lrx = text_width(title) + titleX - 1;
    pulldown->rect.lry = text_height() + titleY - 1;
    pulldown->keyCode = keyCode;
    pulldown->itemsLength = itemsLength;
    pulldown->items = items;
    pulldown->field_1C = a7;
    pulldown->field_20 = a8;

    window->menuBar->pulldownsLength++;

    return 0;
}

// 0x4DC8D0
void win_delete_menu_bar(int win)
{
    Window* window = GNW_find(win);

    if (!GNW_win_init_flag) {
        return;
    }

    if (window == NULL) {
        return;
    }

    if (window->menuBar == NULL) {
        return;
    }

    win_fill(win,
        window->menuBar->rect.ulx,
        window->menuBar->rect.uly,
        rectGetWidth(&(window->menuBar->rect)),
        rectGetHeight(&(window->menuBar->rect)),
        window->field_20);

    mem_free(window->menuBar);
    window->menuBar = NULL;
}

// 0x4DC9F0
static int find_first_letter(int ch, char** stringList, int stringListLength)
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
int win_width_needed(char** fileNameList, int fileNameListLength)
{
    int maxWidth = 0;

    for (int index = 0; index < fileNameListLength; index++) {
        int width = text_width(fileNameList[index]);
        if (width > maxWidth) {
            maxWidth = width;
        }
    }

    return maxWidth;
}

// 0x4DCA5C
int win_input_str(int win, char* dest, int maxLength, int x, int y, int textColor, int backgroundColor)
{
    Window* window = GNW_find(win);
    unsigned char* buffer = window->buffer + window->width * y + x;

    int cursorPos = strlen(dest);
    dest[cursorPos] = '_';
    dest[cursorPos + 1] = '\0';

    int lineHeight = text_height();
    int stringWidth = text_width(dest);
    bufferFill(buffer, stringWidth, lineHeight, window->width, backgroundColor);
    text_to_buf(buffer, dest, stringWidth, window->width, textColor);

    Rect dirtyRect;
    dirtyRect.ulx = window->rect.ulx + x;
    dirtyRect.uly = window->rect.uly + y;
    dirtyRect.lrx = dirtyRect.ulx + stringWidth;
    dirtyRect.lry = dirtyRect.uly + lineHeight;
    GNW_win_refresh(window, &dirtyRect, NULL);

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
                    stringWidth = text_width(dest);

                    if (isFirstKey) {
                        bufferFill(buffer, stringWidth, lineHeight, window->width, backgroundColor);

                        dirtyRect.ulx = window->rect.ulx + x;
                        dirtyRect.uly = window->rect.uly + y;
                        dirtyRect.lrx = dirtyRect.ulx + stringWidth;
                        dirtyRect.lry = dirtyRect.uly + lineHeight;
                        GNW_win_refresh(window, &dirtyRect, NULL);

                        dest[0] = '_';
                        dest[1] = '\0';
                        cursorPos = 1;
                    } else {
                        dest[cursorPos] = ' ';
                        dest[cursorPos - 1] = '_';
                    }

                    bufferFill(buffer, stringWidth, lineHeight, window->width, backgroundColor);
                    text_to_buf(buffer, dest, stringWidth, window->width, textColor);

                    dirtyRect.ulx = window->rect.ulx + x;
                    dirtyRect.uly = window->rect.uly + y;
                    dirtyRect.lrx = dirtyRect.ulx + stringWidth;
                    dirtyRect.lry = dirtyRect.uly + lineHeight;
                    GNW_win_refresh(window, &dirtyRect, NULL);

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

                        int stringWidth = text_width(dest);
                        bufferFill(buffer, stringWidth, lineHeight, window->width, backgroundColor);
                        text_to_buf(buffer, dest, stringWidth, window->width, textColor);

                        dirtyRect.ulx = window->rect.ulx + x;
                        dirtyRect.uly = window->rect.uly + y;
                        dirtyRect.lrx = dirtyRect.ulx + stringWidth;
                        dirtyRect.lry = dirtyRect.uly + lineHeight;
                        GNW_win_refresh(window, &dirtyRect, NULL);

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
static int process_pull_down(int win, Rect* rect, char** items, int itemsLength, int a5, int a6, MenuBar* menuBar, int pulldownIndex)
{
    // TODO: Incomplete.
    return -1;
}

// 0x4DC930
int GNW_process_menu(MenuBar* menuBar, int pulldownIndex)
{
    // 0x51E418
    static MenuBar* curr_menu = NULL;

    if (curr_menu != NULL) {
        return -1;
    }

    curr_menu = menuBar;

    int keyCode;
    Rect rect;
    do {
        MenuPulldown* pulldown = &(menuBar->pulldowns[pulldownIndex]);
        int win = create_pull_down(pulldown->items,
            pulldown->itemsLength,
            pulldown->rect.ulx,
            menuBar->rect.lry + 1,
            pulldown->field_1C,
            pulldown->field_20,
            &rect);
        if (win == -1) {
            curr_menu = NULL;
            return -1;
        }

        keyCode = process_pull_down(win, &rect, pulldown->items, pulldown->itemsLength, pulldown->field_1C, pulldown->field_20, menuBar, pulldownIndex);
        if (keyCode < -1) {
            pulldownIndex = -2 - keyCode;
        }
    } while (keyCode < -1);

    if (keyCode != -1) {
        inputEventQueueReset();
        enqueueInputEvent(keyCode);
        keyCode = menuBar->pulldowns[pulldownIndex].keyCode;
    }

    curr_menu = NULL;

    return keyCode;
}

// Calculates max length of string needed to represent a1 or a2.
//
// 0x4DD03C
static int calc_max_field_chars_wcursor(int a1, int a2)
{
    char* str = (char*)mem_malloc(17);
    if (str == NULL) {
        return -1;
    }

    sprintf(str, "%d", a1);
    int len1 = strlen(str);

    sprintf(str, "%d", a2);
    int len2 = strlen(str);

    mem_free(str);

    return max(len1, len2) + 1;
}

// 0x4DD3EC
void GNW_intr_init()
{
    int v1, v2;
    int i;

    tm_persistence = 3000;
    tm_add = 0;
    tm_kill = -1;
    scr_center_x = _scr_size.lrx / 2;

    if (_scr_size.lry >= 479) {
        tm_text_y = 16;
        tm_text_x = 16;
    } else {
        tm_text_y = 10;
        tm_text_x = 10;
    }

    tm_h = 2 * tm_text_y + text_height();

    v1 = _scr_size.lry >> 3;
    v2 = _scr_size.lry >> 2;

    for (i = 0; i < 5; i++) {
        tm_location[i].y = v1 * i + v2;
        tm_location[i].taken = 0;
    }
}

// 0x4DD4A4
void GNW_intr_exit()
{
    tickersRemove(tm_watch_msgs);
    while (tm_kill != -1) {
        tm_kill_msg();
    }
}

// 0x4DD66C
static void tm_watch_msgs()
{
    if (tm_watch_active) {
        return;
    }

    tm_watch_active = 1;
    while (tm_kill != -1) {
        if (getTicksSince(tm_queue[tm_kill].created) < tm_persistence) {
            break;
        }

        tm_kill_msg();
    }
    tm_watch_active = 0;
}

// 0x4DD6C0
static void tm_kill_msg()
{
    int v0;

    v0 = tm_kill;
    if (v0 != -1) {
        win_delete(tm_queue[tm_kill].id);
        tm_location[tm_queue[tm_kill].location].taken = 0;

        if (v0 == 5) {
            v0 = 0;
        }

        if (v0 == tm_add) {
            tm_add = 0;
            tm_kill = -1;
            tickersRemove(tm_watch_msgs);
            v0 = tm_kill;
        }
    }

    tm_kill = v0;
}

// 0x4DD744
static void tm_kill_out_of_order(int a1)
{
    int v7;
    int v6;

    if (tm_kill == -1) {
        return;
    }

    if (!tm_index_active(a1)) {
        return;
    }

    win_delete(tm_queue[a1].id);

    tm_location[tm_queue[a1].location].taken = 0;

    if (a1 != tm_kill) {
        v6 = a1;
        do {
            v7 = v6 - 1;
            if (v7 < 0) {
                v7 = 4;
            }

            tm_queue[v6] = tm_queue[v7];
            v6 = v7;
        } while (v7 != tm_kill);
    }

    if (++tm_kill == 5) {
        tm_kill = 0;
    }

    if (tm_add == tm_kill) {
        tm_add = 0;
        tm_kill = -1;
        tickersRemove(tm_watch_msgs);
    }
}

// 0x4DD82C
static void tm_click_response(int btn)
{
    int win;
    int v3;

    if (tm_kill == -1) {
        return;
    }

    win = win_button_winID(btn);
    v3 = tm_kill;
    while (win != tm_queue[v3].id) {
        v3++;
        if (v3 == 5) {
            v3 = 0;
        }

        if (v3 == tm_kill || !tm_index_active(v3))
            return;
    }

    tm_kill_out_of_order(v3);
}

// 0x4DD870
static int tm_index_active(int a1)
{
    if (tm_kill != tm_add) {
        if (tm_kill >= tm_add) {
            if (a1 >= tm_add && a1 < tm_kill)
                return 0;
        } else if (a1 < tm_kill || a1 >= tm_add) {
            return 0;
        }
    }
    return 1;
}
