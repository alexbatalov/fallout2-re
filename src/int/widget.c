#include "int/widget.h"

#include <stdio.h>
#include <string.h>

#include "int/datafile.h"
#include "plib/gnw/debug.h"
#include "draw.h"
#include "plib/gnw/rect.h"
#include "int/memdbg.h"
#include "int/sound.h"
#include "plib/gnw/text.h"
#include "window.h"
#include "plib/gnw/gnw.h"

#define WIDGET_UPDATE_REGIONS_CAPACITY 32

typedef struct StatusBar {
    unsigned char* field_0;
    unsigned char* field_4;
    int win;
    int x;
    int y;
    int width;
    int height;
    int field_1C;
    int field_20;
    int field_24;
} StatusBar;

typedef struct UpdateRegion {
    int win;
    int x;
    int y;
    unsigned int type;
    int field_10;
    void* value;
    UpdateRegionShowFunc* showFunc;
    UpdateRegionDrawFunc* drawFunc;
} UpdateRegion;


typedef struct TextInputRegion {
    int textRegionId;
    int isUsed;
    int field_8;
    int field_C;
    int field_10;
    char* text;
    int field_18;
    int field_1C;
    int btn;
    TextInputRegionDeleteFunc* deleteFunc;
    int field_28;
    void* deleteFuncUserData;
} TextInputRegion;

typedef struct TextRegion {
    int win;
    int isUsed;
    int x;
    int y;
    int width;
    int height;
    int textAlignment;
    int textFlags;
    int backgroundColor;
    int font;
} TextRegion;

static void deleteChar(char* string, int pos, int length);
static void insertChar(char* string, char ch, int pos, int length);
static void textInputRegionDispatch(int btn, int inputEvent);
static void showRegion(UpdateRegion* updateRegion);
static void freeStatusBar();
static void drawStatusBar();

// 0x66E6A0
static UpdateRegion* updateRegions[WIDGET_UPDATE_REGIONS_CAPACITY];

// 0x66E720
static StatusBar statusBar;

// 0x66E750
static TextInputRegion* textInputRegions;

// 0x66E754
static int numTextInputRegions;

// 0x66E758
static TextRegion* textRegions;

// 0x66E75C
static int statusBarActive;

// 0x66E760
static int numTextRegions;

// 0x4B45A0
static void deleteChar(char* string, int pos, int length)
{
    if (length > pos) {
        memcpy(string + pos, string + pos + 1, length - pos);
    }
}

// 0x4B45C8
static void insertChar(char* string, char ch, int pos, int length)
{
    if (length >= pos) {
        if (length > pos) {
            memmove(string + pos + 1, string + pos, length - pos);
        }
        string[pos] = ch;
    }
}

// 0x4B4788
static void textInputRegionDispatch(int btn, int inputEvent)
{
    // TODO: Incomplete.
}

// 0x4B51D4
int win_add_text_input_region(int textRegionId, char* text, int a3, int a4)
{
    int textInputRegionIndex;
    int oldFont;
    int btn;

    if (textRegionId <= 0 || textRegionId > numTextRegions) {
        return 0;
    }

    if (textRegions[textRegionId - 1].isUsed == 0) {
        return 0;
    }

    for (textInputRegionIndex = 0; textInputRegionIndex < numTextInputRegions; textInputRegionIndex++) {
        if (textInputRegions[textInputRegionIndex].isUsed == 0) {
            break;
        }
    }

    if (textInputRegionIndex == numTextInputRegions) {
        if (textInputRegions == NULL) {
            textInputRegions = (TextInputRegion*)mymalloc(sizeof(*textInputRegions), __FILE__, __LINE__);
        } else {
            textInputRegions = (TextInputRegion*)myrealloc(textInputRegions, sizeof(*textInputRegions) * (numTextInputRegions + 1), __FILE__, __LINE__);
        }
        numTextInputRegions++;
    }

    textInputRegions[textInputRegionIndex].field_28 = a4;
    textInputRegions[textInputRegionIndex].textRegionId = textRegionId;
    textInputRegions[textInputRegionIndex].isUsed = 1;
    textInputRegions[textInputRegionIndex].field_8 = a3;
    textInputRegions[textInputRegionIndex].field_C = 0;
    textInputRegions[textInputRegionIndex].text = text;
    textInputRegions[textInputRegionIndex].field_10 = strlen(text);
    textInputRegions[textInputRegionIndex].deleteFunc = NULL;
    textInputRegions[textInputRegionIndex].deleteFuncUserData = NULL;

    oldFont = text_curr();
    text_font(textRegions[textRegionId - 1].font);

    btn = win_register_button(textRegions[textRegionId - 1].win,
        textRegions[textRegionId - 1].x,
        textRegions[textRegionId - 1].y,
        textRegions[textRegionId - 1].width,
        text_height(),
        -1,
        -1,
        -1,
        (textInputRegionIndex + 1) | 0x400,
        NULL,
        NULL,
        NULL,
        0);
    win_register_button_func(btn, NULL, NULL, NULL, textInputRegionDispatch);

    // NOTE: Uninline.
    win_print_text_region(textRegionId, text);

    textInputRegions[textInputRegionIndex].btn = btn;

    text_font(oldFont);

    return textInputRegionIndex + 1;
}

// NOTE: Unused.
//
// 0x4B53A8
void windowSelectTextInputRegion(int textInputRegionId)
{
    textInputRegionDispatch(textInputRegions[textInputRegionId - 1].btn, textInputRegionId | 0x400);
}

// 0x4B53D0
int win_delete_all_text_input_regions(int win)
{
    int index;

    for (index = 0; index < numTextInputRegions; index++) {
        if (textRegions[textInputRegions[index].textRegionId - 1].win == win) {
            win_delete_text_input_region(index + 1);
        }
    }

    return 1;
}

// 0x4B541C
int win_delete_text_input_region(int textInputRegionId)
{
    int textInputRegionIndex;

    textInputRegionIndex = textInputRegionId - 1;
    if (textInputRegionIndex >= 0 && textInputRegionIndex < numTextInputRegions) {
        if (textInputRegions[textInputRegionIndex].isUsed != 0) {
            if (textInputRegions[textInputRegionIndex].deleteFunc != NULL) {
                textInputRegions[textInputRegionIndex].deleteFunc(textInputRegions[textInputRegionIndex].text, textInputRegions[textInputRegionIndex].deleteFuncUserData);
            }

            // NOTE: Uninline.
            win_delete_text_region(textInputRegions[textInputRegionIndex].textRegionId);

            return 1;
        }
    }

    return 0;
}

// 0x4B54C8
int win_set_text_input_delete_func(int textInputRegionId, TextInputRegionDeleteFunc* deleteFunc, void* userData)
{
    int textInputRegionIndex;

    textInputRegionIndex = textInputRegionId - 1;
    if (textInputRegionIndex >= 0 && textInputRegionIndex < numTextInputRegions) {
        if (textInputRegions[textInputRegionIndex].isUsed != 0) {
            textInputRegions[textInputRegionIndex].deleteFunc = deleteFunc;
            textInputRegions[textInputRegionIndex].deleteFuncUserData = userData;
            return 1;
        }
    }

    return 0;
}

// 0x4B5508
int win_add_text_region(int win, int x, int y, int width, int font, int textAlignment, int textFlags, int backgroundColor)
{
    int textRegionIndex;
    int oldFont;
    int height;

    for (textRegionIndex = 0; textRegionIndex < numTextRegions; textRegionIndex++) {
        if (textRegions[textRegionIndex].isUsed == 0) {
            break;
        }
    }

    if (textRegionIndex == numTextRegions) {
        if (textRegions == NULL) {
            textRegions = (TextRegion*)mymalloc(sizeof(*textRegions), __FILE__, __LINE__); // "..\int\WIDGET.C", 615
        } else {
            textRegions = (TextRegion*)myrealloc(textRegions, sizeof(*textRegions) * (numTextRegions + 1), __FILE__, __LINE__); // "..\int\WIDGET.C", 616
        }
        numTextRegions++;
    }

    oldFont = text_curr();
    text_font(font);

    height = text_height();

    text_font(oldFont);

    if ((textFlags & FONT_SHADOW) != 0) {
        width++;
        height++;
    }

    textRegions[textRegionIndex].isUsed = 1;
    textRegions[textRegionIndex].win = win;
    textRegions[textRegionIndex].x = x;
    textRegions[textRegionIndex].y = y;
    textRegions[textRegionIndex].width = width;
    textRegions[textRegionIndex].height = height;
    textRegions[textRegionIndex].font = font;
    textRegions[textRegionIndex].textAlignment = textAlignment;
    textRegions[textRegionIndex].textFlags = textFlags;
    textRegions[textRegionIndex].backgroundColor = backgroundColor;

    return textRegionIndex + 1;
}

// 0x4B5634
int win_print_text_region(int textRegionId, char* string)
{
    int textRegionIndex;
    int oldFont;

    textRegionIndex = textRegionId - 1;
    if (textRegionIndex >= 0 && textRegionIndex <= numTextRegions) {
        if (textRegions[textRegionIndex].isUsed != 0) {
            oldFont = text_curr();
            text_font(textRegions[textRegionIndex].font);

            win_fill(textRegions[textRegionIndex].win,
                textRegions[textRegionIndex].x,
                textRegions[textRegionIndex].y,
                textRegions[textRegionIndex].width,
                textRegions[textRegionIndex].height,
                textRegions[textRegionIndex].backgroundColor);

            _windowPrintBuf(textRegions[textRegionIndex].win,
                string,
                strlen(string),
                textRegions[textRegionIndex].width,
                win_height(textRegions[textRegionIndex].win),
                textRegions[textRegionIndex].x,
                textRegions[textRegionIndex].y,
                textRegions[textRegionIndex].textFlags | 0x2000000,
                textRegions[textRegionIndex].textAlignment);

            text_font(oldFont);

            return 1;
        }
    }

    return 0;
}

// 0x4B5714
int win_print_substr_region(int textRegionId, char* string, int stringLength)
{
    int textRegionIndex;
    int oldFont;

    textRegionIndex = textRegionId - 1;
    if (textRegionIndex >= 0 && textRegionIndex <= numTextRegions) {
        if (textRegions[textRegionIndex].isUsed != 0) {
            oldFont = text_curr();
            text_font(textRegions[textRegionIndex].font);

            win_fill(textRegions[textRegionIndex].win,
                textRegions[textRegionIndex].x,
                textRegions[textRegionIndex].y,
                textRegions[textRegionIndex].width,
                textRegions[textRegionIndex].height,
                textRegions[textRegionIndex].backgroundColor);

            _windowPrintBuf(textRegions[textRegionIndex].win,
                string,
                stringLength,
                textRegions[textRegionIndex].width,
                win_height(textRegions[textRegionIndex].win),
                textRegions[textRegionIndex].x,
                textRegions[textRegionIndex].y,
                textRegions[textRegionIndex].textFlags | 0x2000000,
                textRegions[textRegionIndex].textAlignment);

            text_font(oldFont);

            return 1;
        }
    }

    return 0;
}

// 0x4B57E4
int win_update_text_region(int textRegionId)
{
    int textRegionIndex;
    Rect rect;

    textRegionIndex = textRegionId - 1;
    if (textRegionIndex >= 0 && textRegionIndex <= numTextRegions) {
        if (textRegions[textRegionIndex].isUsed != 0) {
            rect.ulx = textRegions[textRegionIndex].x;
            rect.uly = textRegions[textRegionIndex].y;
            rect.lrx = textRegions[textRegionIndex].x + textRegions[textRegionIndex].width;
            rect.lry = textRegions[textRegionIndex].y + text_height();
            win_draw_rect(textRegions[textRegionIndex].win, &rect);
            return 1;
        }
    }

    return 0;
}

// 0x4B5864
int win_delete_text_region(int textRegionId)
{
    int textRegionIndex;

    textRegionIndex = textRegionId - 1;
    if (textRegionIndex >= 0 && textRegionIndex <= numTextRegions) {
        if (textRegions[textRegionIndex].isUsed != 0) {
            textRegions[textRegionIndex].isUsed = 0;
            return 1;
        }
    }

    return 0;
}

// 0x4B58A0
int win_delete_all_update_regions(int win)
{
    int index;

    for (index = 0; index < WIDGET_UPDATE_REGIONS_CAPACITY; index++) {
        if (updateRegions[index] != NULL) {
            if (win == updateRegions[index]->win) {
                myfree(updateRegions[index], __FILE__, __LINE__); // "..\int\WIDGET.C", 722
                updateRegions[index] = NULL;
            }
        }
    }

    return 1;
}

// 0x4B58E8
int win_text_region_style(int textRegionId, int font, int textAlignment, int textFlags, int backgroundColor)
{
    int textRegionIndex;
    int oldFont;
    int height;

    textRegionIndex = textRegionId - 1;
    if (textRegionIndex >= 0 && textRegionIndex <= numTextRegions) {
        if (textRegions[textRegionIndex].isUsed != 0) {
            textRegions[textRegionIndex].font = font;
            textRegions[textRegionIndex].textAlignment = textAlignment;

            oldFont = text_curr();
            text_font(font);

            height = text_height();

            text_font(oldFont);

            if ((textRegions[textRegionIndex].textFlags & FONT_SHADOW) == 0
                && (textFlags & FONT_SHADOW) != 0) {
                height++;
                textRegions[textRegionIndex].width++;
            }

            textRegions[textRegionIndex].height = height;
            textRegions[textRegionIndex].textFlags = textFlags;
            textRegions[textRegionIndex].backgroundColor = backgroundColor;

            return 1;
        }
    }

    return 0;
}

// 0x4B5998
void win_delete_widgets(int win)
{
    int index;

    win_delete_all_text_input_regions(win);

    for (index = 0; index < numTextRegions; index++) {
        if (textRegions[index].win == win) {
            // NOTE: Uninline.
            win_delete_text_region(index + 1);
        }
    }

    win_delete_all_update_regions(win);
}

// 0x4B5A04
int widgetDoInput()
{
    int index;

    for (index = 0; index < WIDGET_UPDATE_REGIONS_CAPACITY; index++) {
        if (updateRegions[index] != NULL) {
            showRegion(updateRegions[index]);
        }
    }

    return 0;
}

// 0x4B5A2C
int win_center_str(int win, char* string, int y, int a4)
{
    int windowWidth;
    int stringWidth;

    windowWidth = win_width(win);
    stringWidth = text_width(string);
    win_print(win, string, 0, (windowWidth - stringWidth) / 2, y, a4);

    return 1;
}

// 0x4B5A64
static void showRegion(UpdateRegion* updateRegion)
{
    float value;
    char stringBuffer[80];

    switch (updateRegion->type & 0xFF) {
    case 1:
        value = (float)(*(int*)updateRegion->value);
        break;
    case 2:
        value = *(float*)updateRegion->value;
        break;
    case 4:
        value = *(float*)updateRegion->value / 65636.0f;
        break;
    case 8:
        win_print(updateRegion->win,
            (char*)updateRegion->value,
            0,
            updateRegion->x,
            updateRegion->y,
            updateRegion->field_10);
        return;
    case 0x10:
        break;
    default:
        debug_printf("Invalid input type given to win_register_update\n");
        return;
    }

    switch (updateRegion->type & 0xFF00) {
    case 0x100:
        sprintf(stringBuffer, " %d ", (int)value);
        break;
    case 0x200:
        sprintf(stringBuffer, " %f ", value);
        break;
    case 0x400:
        sprintf(stringBuffer, " %6.2f%% ", value * 100.0f);
        break;
    case 0x800:
        if (updateRegion->showFunc != NULL) {
            updateRegion->showFunc(updateRegion->value);
        }
        return;
    default:
        debug_printf("Invalid output type given to win_register_update\n");
        return;
    }

    win_print(updateRegion->win,
        stringBuffer,
        0,
        updateRegion->x,
        updateRegion->y,
        updateRegion->field_10 | 0x1000000);
}

// 0x4B5BE8
int draw_widgets()
{
    int index;

    for (index = 0; index < WIDGET_UPDATE_REGIONS_CAPACITY; index++) {
        if (updateRegions[index] != NULL) {
            if ((updateRegions[index]->type & 0xFF00) == 0x800) {
                updateRegions[index]->drawFunc(updateRegions[index]->value);
            }
        }
    }

    return 1;
}

// 0x4B5C24
int update_widgets()
{
    for (int index = 0; index < WIDGET_UPDATE_REGIONS_CAPACITY; index++) {
        if (updateRegions[index] != NULL) {
            showRegion(updateRegions[index]);
        }
    }

    return 1;
}

// 0x4B5C4C
int win_register_update(int win, int x, int y, UpdateRegionShowFunc* showFunc, UpdateRegionDrawFunc* drawFunc, void* value, unsigned int type, int a8)
{
    int updateRegionIndex;

    for (updateRegionIndex = 0; updateRegionIndex < WIDGET_UPDATE_REGIONS_CAPACITY; updateRegionIndex++) {
        if (updateRegions[updateRegionIndex] == NULL) {
            break;
        }
    }

    if (updateRegionIndex == WIDGET_UPDATE_REGIONS_CAPACITY) {
        return -1;
    }

    updateRegions[updateRegionIndex] = (UpdateRegion*)mymalloc(sizeof(*updateRegions), __FILE__, __LINE__); // "..\int\WIDGET.C", 859
    updateRegions[updateRegionIndex]->win = win;
    updateRegions[updateRegionIndex]->x = x;
    updateRegions[updateRegionIndex]->y = y;
    updateRegions[updateRegionIndex]->type = type;
    updateRegions[updateRegionIndex]->field_10 = a8;
    updateRegions[updateRegionIndex]->value = value;
    updateRegions[updateRegionIndex]->showFunc = showFunc;
    updateRegions[updateRegionIndex]->drawFunc = drawFunc;

    return updateRegionIndex;
}

// 0x4B5D0C
int win_delete_update_region(int updateRegionIndex)
{
    if (updateRegionIndex >= 0 && updateRegionIndex < WIDGET_UPDATE_REGIONS_CAPACITY) {
        if (updateRegions[updateRegionIndex] == NULL) {
            myfree(updateRegions[updateRegionIndex], __FILE__, __LINE__); // "..\int\WIDGET.C", 875
            updateRegions[updateRegionIndex] = NULL;
            return 1;
        }
    }

    return 0;
}

// 0x4B5D54
void win_do_updateregions()
{
    for (int index = 0; index < WIDGET_UPDATE_REGIONS_CAPACITY; index++) {
        if (updateRegions[index] != NULL) {
            showRegion(updateRegions[index]);
        }
    }
}

// 0x4B5D78
static void freeStatusBar()
{
    if (statusBar.field_0 != NULL) {
        myfree(statusBar.field_0, __FILE__, __LINE__); // "..\int\WIDGET.C", 891
        statusBar.field_0 = NULL;
    }

    if (statusBar.field_4 != NULL) {
        myfree(statusBar.field_4, __FILE__, __LINE__); // "..\int\WIDGET.C", 892
        statusBar.field_4 = NULL;
    }

    memset(&statusBar, 0, sizeof(statusBar));

    statusBarActive = 0;
}

// 0x4B5DE4
void initWidgets()
{
    int updateRegionIndex;

    for (updateRegionIndex = 0; updateRegionIndex < WIDGET_UPDATE_REGIONS_CAPACITY; updateRegionIndex++) {
        updateRegions[updateRegionIndex] = NULL;
    }

    textRegions = NULL;
    numTextRegions = 0;

    textInputRegions = NULL;
    numTextInputRegions = 0;

    freeStatusBar();
}

// 0x4B5E1C
void widgetsClose()
{
    if (textRegions != NULL) {
        myfree(textRegions, __FILE__, __LINE__); // "..\int\WIDGET.C", 908
    }
    textRegions = NULL;
    numTextRegions = 0;

    if (textInputRegions != NULL) {
        myfree(textInputRegions, __FILE__, __LINE__); // "..\int\WIDGET.C", 912
    }
    textInputRegions = NULL;
    numTextInputRegions = 0;

    freeStatusBar();
}

// 0x4B5E7C
static void drawStatusBar()
{
    Rect rect;
    unsigned char* dest;

    if (statusBarActive) {
        dest = win_get_buf(statusBar.win) + statusBar.y * win_width(statusBar.win) + statusBar.x;

        blitBufferToBuffer(statusBar.field_0,
            statusBar.width,
            statusBar.height,
            statusBar.width,
            dest,
            win_width(statusBar.win));

        blitBufferToBuffer(statusBar.field_4,
            statusBar.field_1C,
            statusBar.height,
            statusBar.width,
            dest,
            win_width(statusBar.win));

        rect.ulx = statusBar.x;
        rect.uly = statusBar.y;
        rect.lrx = statusBar.x + statusBar.width;
        rect.lry = statusBar.y + statusBar.height;
        win_draw_rect(statusBar.win, &rect);
    }
}

// 0x4B5F5C
void real_win_set_status_bar(int a1, int a2, int a3)
{
    if (statusBarActive) {
        statusBar.field_1C = a2;
        statusBar.field_20 = a2;
        statusBar.field_24 = a3;
        drawStatusBar();
    }
}

// 0x4B5F80
void real_win_update_status_bar(float a1, float a2)
{
    if (statusBarActive) {
        statusBar.field_1C = (int)(a1 * statusBar.width);
        statusBar.field_20 = (int)(a1 * statusBar.width);
        statusBar.field_24 = (int)(a2 * statusBar.width);
        drawStatusBar();
        soundContinueAll();
    }
}

// 0x4B5FD4
void real_win_increment_status_bar(float a1)
{
    if (statusBarActive) {
        statusBar.field_1C = statusBar.field_20 + (int)(a1 * (statusBar.field_24 - statusBar.field_20));
        drawStatusBar();
        soundContinueAll();
    }
}

// 0x4B6020
void real_win_add_status_bar(int win, int a2, char* a3, char* a4, int x, int y)
{
    int imageWidth1;
    int imageHeight1;
    int imageWidth2;
    int imageHeight2;

    freeStatusBar();

    statusBar.field_0 = loadRawDataFile(a4, &imageWidth1, &imageHeight1);
    statusBar.field_4 = loadRawDataFile(a3, &imageWidth2, &imageHeight2);

    if (imageWidth2 == imageWidth1 && imageHeight2 == imageHeight1) {
        statusBar.x = x;
        statusBar.y = y;
        statusBar.width = imageWidth1;
        statusBar.height = imageHeight1;
        statusBar.win = win;
        real_win_set_status_bar(a2, 0, 0);
        statusBarActive = 1;
    } else {
        freeStatusBar();
        debug_printf("status bar dimensions not the same\n");
    }
}

// 0x4B60CC
void real_win_get_status_info(int a1, int* a2, int* a3, int* a4)
{
    if (statusBarActive) {
        *a2 = statusBar.field_1C;
        *a3 = statusBar.field_20;
        *a4 = statusBar.field_24;
    } else {
        *a2 = -1;
        *a3 = -1;
        *a4 = -1;
    }
}

// 0x4B6100
void real_win_modify_status_info(int a1, int a2, int a3, int a4)
{
    if (statusBarActive) {
        statusBar.field_1C = a2;
        statusBar.field_20 = a3;
        statusBar.field_24 = a4;
    }
}
