#include "widget.h"

#include <string.h>

#include "datafile.h"
#include "debug.h"
#include "draw.h"
#include "geometry.h"
#include "memory_manager.h"
#include "sound.h"
#include "text_font.h"
#include "window.h"
#include "window_manager.h"

// 0x66E6A0
UpdateRegion* _updateRegions[WIDGET_UPDATE_REGIONS_CAPACITY];

// 0x66E720
StatusBar _statusBar;

// 0x66E750
TextInputRegion* _textInputRegions;

// 0x66E754
int _numTextInputRegions;

// 0x66E758
TextRegion* _textRegions;

// 0x66E75C
int _statusBarActive;

// 0x66E760
int _numTextRegions;

// 0x4B45A0
void _deleteChar(char* string, int pos, int length)
{
    if (length > pos) {
        memcpy(string + pos, string + pos + 1, length - pos);
    }
}

// 0x4B45C8
void _insertChar(char* string, char ch, int pos, int length)
{
    if (length >= pos) {
        if (length > pos) {
            memmove(string + pos + 1, string + pos, length - pos);
        }
        string[pos] = ch;
    }
}

// 0x4B4788
void _textInputRegionDispatch(int btn, int inputEvent)
{
    // TODO: Incomplete.
}

// 0x4B51D4
int _win_add_text_input_region(int textRegionId, char* text, int a3, int a4)
{
    int textInputRegionIndex;
    int oldFont;
    int btn;

    if (textRegionId <= 0 || textRegionId > _numTextRegions) {
        return 0;
    }

    if (_textRegions[textRegionId - 1].isUsed == 0) {
        return 0;
    }

    for (textInputRegionIndex = 0; textInputRegionIndex < _numTextInputRegions; textInputRegionIndex++) {
        if (_textInputRegions[textInputRegionIndex].isUsed == 0) {
            break;
        }
    }

    if (textInputRegionIndex == _numTextInputRegions) {
        if (_textInputRegions == NULL) {
            _textInputRegions = (TextInputRegion*)internal_malloc_safe(sizeof(*_textInputRegions), __FILE__, __LINE__);
        } else {
            _textInputRegions = (TextInputRegion*)internal_realloc_safe(_textInputRegions, sizeof(*_textInputRegions) * (_numTextInputRegions + 1), __FILE__, __LINE__);
        }
        _numTextInputRegions++;
    }

    _textInputRegions[textInputRegionIndex].field_28 = a4;
    _textInputRegions[textInputRegionIndex].textRegionId = textRegionId;
    _textInputRegions[textInputRegionIndex].isUsed = 1;
    _textInputRegions[textInputRegionIndex].field_8 = a3;
    _textInputRegions[textInputRegionIndex].field_C = 0;
    _textInputRegions[textInputRegionIndex].text = text;
    _textInputRegions[textInputRegionIndex].field_10 = strlen(text);
    _textInputRegions[textInputRegionIndex].deleteFunc = NULL;
    _textInputRegions[textInputRegionIndex].deleteFuncUserData = NULL;

    oldFont = fontGetCurrent();
    fontSetCurrent(_textRegions[textRegionId - 1].font);

    btn = buttonCreate(_textRegions[textRegionId - 1].win,
        _textRegions[textRegionId - 1].x,
        _textRegions[textRegionId - 1].y,
        _textRegions[textRegionId - 1].width,
        fontGetLineHeight(),
        -1,
        -1,
        -1,
        (textInputRegionIndex + 1) | 0x400,
        NULL,
        NULL,
        NULL,
        0);
    buttonSetMouseCallbacks(btn, NULL, NULL, NULL, _textInputRegionDispatch);

    // NOTE: Uninline.
    _win_print_text_region(textRegionId, text);

    _textInputRegions[textInputRegionIndex].btn = btn;

    fontSetCurrent(oldFont);

    return textInputRegionIndex + 1;
}

// NOTE: Unused.
//
// 0x4B53A8
void _windowSelectTextInputRegion(int textInputRegionId)
{
    _textInputRegionDispatch(_textInputRegions[textInputRegionId - 1].btn, textInputRegionId | 0x400);
}

// 0x4B53D0
int _win_delete_all_text_input_regions(int win)
{
    int index;

    for (index = 0; index < _numTextInputRegions; index++) {
        if (_textRegions[_textInputRegions[index].textRegionId - 1].win == win) {
            _win_delete_text_input_region(index + 1);
        }
    }

    return 1;
}

// 0x4B541C
int _win_delete_text_input_region(int textInputRegionId)
{
    int textInputRegionIndex;

    textInputRegionIndex = textInputRegionId - 1;
    if (textInputRegionIndex >= 0 && textInputRegionIndex < _numTextInputRegions) {
        if (_textInputRegions[textInputRegionIndex].isUsed != 0) {
            if (_textInputRegions[textInputRegionIndex].deleteFunc != NULL) {
                _textInputRegions[textInputRegionIndex].deleteFunc(_textInputRegions[textInputRegionIndex].text, _textInputRegions[textInputRegionIndex].deleteFuncUserData);
            }

            // NOTE: Uninline.
            _win_delete_text_region(_textInputRegions[textInputRegionIndex].textRegionId);

            return 1;
        }
    }

    return 0;
}

// 0x4B54C8
int _win_set_text_input_delete_func(int textInputRegionId, TextInputRegionDeleteFunc* deleteFunc, void* userData)
{
    int textInputRegionIndex;

    textInputRegionIndex = textInputRegionId - 1;
    if (textInputRegionIndex >= 0 && textInputRegionIndex < _numTextInputRegions) {
        if (_textInputRegions[textInputRegionIndex].isUsed != 0) {
            _textInputRegions[textInputRegionIndex].deleteFunc = deleteFunc;
            _textInputRegions[textInputRegionIndex].deleteFuncUserData = userData;
            return 1;
        }
    }

    return 0;
}

// 0x4B5508
int _win_add_text_region(int win, int x, int y, int width, int font, int textAlignment, int textFlags, int backgroundColor)
{
    int textRegionIndex;
    int oldFont;
    int height;

    for (textRegionIndex = 0; textRegionIndex < _numTextRegions; textRegionIndex++) {
        if (_textRegions[textRegionIndex].isUsed == 0) {
            break;
        }
    }

    if (textRegionIndex == _numTextRegions) {
        if (_textRegions == NULL) {
            _textRegions = (TextRegion*)internal_malloc_safe(sizeof(*_textRegions), __FILE__, __LINE__); // "..\int\WIDGET.C", 615
        } else {
            _textRegions = (TextRegion*)internal_realloc_safe(_textRegions, sizeof(*_textRegions) * (_numTextRegions + 1), __FILE__, __LINE__); // "..\int\WIDGET.C", 616
        }
        _numTextRegions++;
    }

    oldFont = fontGetCurrent();
    fontSetCurrent(font);

    height = fontGetLineHeight();

    fontSetCurrent(oldFont);

    if ((textFlags & FONT_SHADOW) != 0) {
        width++;
        height++;
    }

    _textRegions[textRegionIndex].isUsed = 1;
    _textRegions[textRegionIndex].win = win;
    _textRegions[textRegionIndex].x = x;
    _textRegions[textRegionIndex].y = y;
    _textRegions[textRegionIndex].width = width;
    _textRegions[textRegionIndex].height = height;
    _textRegions[textRegionIndex].font = font;
    _textRegions[textRegionIndex].textAlignment = textAlignment;
    _textRegions[textRegionIndex].textFlags = textFlags;
    _textRegions[textRegionIndex].backgroundColor = backgroundColor;

    return textRegionIndex + 1;
}

// 0x4B5634
int _win_print_text_region(int textRegionId, char* string)
{
    int textRegionIndex;
    int oldFont;

    textRegionIndex = textRegionId - 1;
    if (textRegionIndex >= 0 && textRegionIndex <= _numTextRegions) {
        if (_textRegions[textRegionIndex].isUsed != 0) {
            oldFont = fontGetCurrent();
            fontSetCurrent(_textRegions[textRegionIndex].font);

            windowFill(_textRegions[textRegionIndex].win,
                _textRegions[textRegionIndex].x,
                _textRegions[textRegionIndex].y,
                _textRegions[textRegionIndex].width,
                _textRegions[textRegionIndex].height,
                _textRegions[textRegionIndex].backgroundColor);

            _windowPrintBuf(_textRegions[textRegionIndex].win,
                string,
                strlen(string),
                _textRegions[textRegionIndex].width,
                windowGetHeight(_textRegions[textRegionIndex].win),
                _textRegions[textRegionIndex].x,
                _textRegions[textRegionIndex].y,
                _textRegions[textRegionIndex].textFlags | 0x2000000,
                _textRegions[textRegionIndex].textAlignment);

            fontSetCurrent(oldFont);

            return 1;
        }
    }

    return 0;
}

// 0x4B5714
int _win_print_substr_region(int textRegionId, char* string, int stringLength)
{
    int textRegionIndex;
    int oldFont;

    textRegionIndex = textRegionId - 1;
    if (textRegionIndex >= 0 && textRegionIndex <= _numTextRegions) {
        if (_textRegions[textRegionIndex].isUsed != 0) {
            oldFont = fontGetCurrent();
            fontSetCurrent(_textRegions[textRegionIndex].font);

            windowFill(_textRegions[textRegionIndex].win,
                _textRegions[textRegionIndex].x,
                _textRegions[textRegionIndex].y,
                _textRegions[textRegionIndex].width,
                _textRegions[textRegionIndex].height,
                _textRegions[textRegionIndex].backgroundColor);

            _windowPrintBuf(_textRegions[textRegionIndex].win,
                string,
                stringLength,
                _textRegions[textRegionIndex].width,
                windowGetHeight(_textRegions[textRegionIndex].win),
                _textRegions[textRegionIndex].x,
                _textRegions[textRegionIndex].y,
                _textRegions[textRegionIndex].textFlags | 0x2000000,
                _textRegions[textRegionIndex].textAlignment);

            fontSetCurrent(oldFont);

            return 1;
        }
    }

    return 0;
}

// 0x4B57E4
int _win_update_text_region(int textRegionId)
{
    int textRegionIndex;
    Rect rect;

    textRegionIndex = textRegionId - 1;
    if (textRegionIndex >= 0 && textRegionIndex <= _numTextRegions) {
        if (_textRegions[textRegionIndex].isUsed != 0) {
            rect.left = _textRegions[textRegionIndex].x;
            rect.top = _textRegions[textRegionIndex].y;
            rect.right = _textRegions[textRegionIndex].x + _textRegions[textRegionIndex].width;
            rect.bottom = _textRegions[textRegionIndex].y + fontGetLineHeight();
            win_draw_rect(_textRegions[textRegionIndex].win, &rect);
            return 1;
        }
    }

    return 0;
}

// 0x4B5864
int _win_delete_text_region(int textRegionId)
{
    int textRegionIndex;

    textRegionIndex = textRegionId - 1;
    if (textRegionIndex >= 0 && textRegionIndex <= _numTextRegions) {
        if (_textRegions[textRegionIndex].isUsed != 0) {
            _textRegions[textRegionIndex].isUsed = 0;
            return 1;
        }
    }

    return 0;
}

// 0x4B58A0
int _win_delete_all_update_regions(int win)
{
    int index;

    for (index = 0; index < WIDGET_UPDATE_REGIONS_CAPACITY; index++) {
        if (_updateRegions[index] != NULL) {
            if (win == _updateRegions[index]->win) {
                internal_free_safe(_updateRegions[index], __FILE__, __LINE__); // "..\int\WIDGET.C", 722
                _updateRegions[index] = NULL;
            }
        }
    }

    return 1;
}

// 0x4B58E8
int _win_text_region_style(int textRegionId, int font, int textAlignment, int textFlags, int backgroundColor)
{
    int textRegionIndex;
    int oldFont;
    int height;

    textRegionIndex = textRegionId - 1;
    if (textRegionIndex >= 0 && textRegionIndex <= _numTextRegions) {
        if (_textRegions[textRegionIndex].isUsed != 0) {
            _textRegions[textRegionIndex].font = font;
            _textRegions[textRegionIndex].textAlignment = textAlignment;

            oldFont = fontGetCurrent();
            fontSetCurrent(font);

            height = fontGetLineHeight();

            fontSetCurrent(oldFont);

            if ((_textRegions[textRegionIndex].textFlags & FONT_SHADOW) == 0
                && (textFlags & FONT_SHADOW) != 0) {
                height++;
                _textRegions[textRegionIndex].width++;
            }

            _textRegions[textRegionIndex].height = height;
            _textRegions[textRegionIndex].textFlags = textFlags;
            _textRegions[textRegionIndex].backgroundColor = backgroundColor;

            return 1;
        }
    }

    return 0;
}

// 0x4B5998
void _win_delete_widgets(int win)
{
    int index;

    _win_delete_all_text_input_regions(win);

    for (index = 0; index < _numTextRegions; index++) {
        if (_textRegions[index].win == win) {
            // NOTE: Uninline.
            _win_delete_text_region(index + 1);
        }
    }

    _win_delete_all_update_regions(win);
}

// 0x4B5A04
int _widgetDoInput()
{
    int index;

    for (index = 0; index < WIDGET_UPDATE_REGIONS_CAPACITY; index++) {
        if (_updateRegions[index] != NULL) {
            _showRegion(_updateRegions[index]);
        }
    }

    return 0;
}

// 0x4B5A2C
int _win_center_str(int win, char* string, int y, int a4)
{
    int windowWidth;
    int stringWidth;

    windowWidth = windowGetWidth(win);
    stringWidth = fontGetStringWidth(string);
    windowDrawText(win, string, 0, (windowWidth - stringWidth) / 2, y, a4);

    return 1;
}

// 0x4B5A64
void _showRegion(UpdateRegion* updateRegion)
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
        windowDrawText(updateRegion->win,
            (char*)updateRegion->value,
            0,
            updateRegion->x,
            updateRegion->y,
            updateRegion->field_10);
        return;
    case 0x10:
        break;
    default:
        debugPrint("Invalid input type given to win_register_update\n");
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
        debugPrint("Invalid output type given to win_register_update\n");
        return;
    }

    windowDrawText(updateRegion->win,
        stringBuffer,
        0,
        updateRegion->x,
        updateRegion->y,
        updateRegion->field_10 | 0x1000000);
}

// 0x4B5BE8
int _draw_widgets()
{
    int index;

    for (index = 0; index < WIDGET_UPDATE_REGIONS_CAPACITY; index++) {
        if (_updateRegions[index] != NULL) {
            if ((_updateRegions[index]->type & 0xFF00) == 0x800) {
                _updateRegions[index]->drawFunc(_updateRegions[index]->value);
            }
        }
    }

    return 1;
}

// 0x4B5C24
int _update_widgets()
{
    for (int index = 0; index < WIDGET_UPDATE_REGIONS_CAPACITY; index++) {
        if (_updateRegions[index] != NULL) {
            _showRegion(_updateRegions[index]);
        }
    }

    return 1;
}

// 0x4B5C4C
int _win_register_update(int win, int x, int y, UpdateRegionShowFunc* showFunc, UpdateRegionDrawFunc* drawFunc, void* value, unsigned int type, int a8)
{
    int updateRegionIndex;

    for (updateRegionIndex = 0; updateRegionIndex < WIDGET_UPDATE_REGIONS_CAPACITY; updateRegionIndex++) {
        if (_updateRegions[updateRegionIndex] == NULL) {
            break;
        }
    }

    if (updateRegionIndex == WIDGET_UPDATE_REGIONS_CAPACITY) {
        return -1;
    }

    _updateRegions[updateRegionIndex] = (UpdateRegion*)internal_malloc_safe(sizeof(*_updateRegions), __FILE__, __LINE__); // "..\int\WIDGET.C", 859
    _updateRegions[updateRegionIndex]->win = win;
    _updateRegions[updateRegionIndex]->x = x;
    _updateRegions[updateRegionIndex]->y = y;
    _updateRegions[updateRegionIndex]->type = type;
    _updateRegions[updateRegionIndex]->field_10 = a8;
    _updateRegions[updateRegionIndex]->value = value;
    _updateRegions[updateRegionIndex]->showFunc = showFunc;
    _updateRegions[updateRegionIndex]->drawFunc = drawFunc;

    return updateRegionIndex;
}

// 0x4B5D0C
int _win_delete_update_region(int updateRegionIndex)
{
    if (updateRegionIndex >= 0 && updateRegionIndex < WIDGET_UPDATE_REGIONS_CAPACITY) {
        if (_updateRegions[updateRegionIndex] == NULL) {
            internal_free_safe(_updateRegions[updateRegionIndex], __FILE__, __LINE__); // "..\int\WIDGET.C", 875
            _updateRegions[updateRegionIndex] = NULL;
            return 1;
        }
    }

    return 0;
}

// 0x4B5D54
void _win_do_updateregions()
{
    for (int index = 0; index < WIDGET_UPDATE_REGIONS_CAPACITY; index++) {
        if (_updateRegions[index] != NULL) {
            _showRegion(_updateRegions[index]);
        }
    }
}

// 0x4B5D78
void _freeStatusBar()
{
    if (_statusBar.field_0 != NULL) {
        internal_free_safe(_statusBar.field_0, __FILE__, __LINE__); // "..\int\WIDGET.C", 891
        _statusBar.field_0 = NULL;
    }

    if (_statusBar.field_4 != NULL) {
        internal_free_safe(_statusBar.field_4, __FILE__, __LINE__); // "..\int\WIDGET.C", 892
        _statusBar.field_4 = NULL;
    }

    memset(&_statusBar, 0, sizeof(_statusBar));

    _statusBarActive = 0;
}

// 0x4B5DE4
void _initWidgets()
{
    int updateRegionIndex;

    for (updateRegionIndex = 0; updateRegionIndex < WIDGET_UPDATE_REGIONS_CAPACITY; updateRegionIndex++) {
        _updateRegions[updateRegionIndex] = NULL;
    }

    _textRegions = NULL;
    _numTextRegions = 0;

    _textInputRegions = NULL;
    _numTextInputRegions = 0;

    _freeStatusBar();
}

// 0x4B5E1C
void _widgetsClose()
{
    if (_textRegions != NULL) {
        internal_free_safe(_textRegions, __FILE__, __LINE__); // "..\int\WIDGET.C", 908
    }
    _textRegions = NULL;
    _numTextRegions = 0;

    if (_textInputRegions != NULL) {
        internal_free_safe(_textInputRegions, __FILE__, __LINE__); // "..\int\WIDGET.C", 912
    }
    _textInputRegions = NULL;
    _numTextInputRegions = 0;

    _freeStatusBar();
}

// 0x4B5E7C
void _drawStatusBar()
{
    Rect rect;
    unsigned char* dest;

    if (_statusBarActive) {
        dest = windowGetBuffer(_statusBar.win) + _statusBar.y * windowGetWidth(_statusBar.win) + _statusBar.x;

        blitBufferToBuffer(_statusBar.field_0,
            _statusBar.width,
            _statusBar.height,
            _statusBar.width,
            dest,
            windowGetWidth(_statusBar.win));

        blitBufferToBuffer(_statusBar.field_4,
            _statusBar.field_1C,
            _statusBar.height,
            _statusBar.width,
            dest,
            windowGetWidth(_statusBar.win));

        rect.left = _statusBar.x;
        rect.top = _statusBar.y;
        rect.right = _statusBar.x + _statusBar.width;
        rect.bottom = _statusBar.y + _statusBar.height;
        win_draw_rect(_statusBar.win, &rect);
    }
}

// 0x4B5F5C
void _real_win_set_status_bar(int a1, int a2, int a3)
{
    if (_statusBarActive) {
        _statusBar.field_1C = a2;
        _statusBar.field_20 = a2;
        _statusBar.field_24 = a3;
        _drawStatusBar();
    }
}

// 0x4B5F80
void _real_win_update_status_bar(float a1, float a2)
{
    if (_statusBarActive) {
        _statusBar.field_1C = (int)(a1 * _statusBar.width);
        _statusBar.field_20 = (int)(a1 * _statusBar.width);
        _statusBar.field_24 = (int)(a2 * _statusBar.width);
        _drawStatusBar();
        soundContinueAll();
    }
}

// 0x4B5FD4
void _real_win_increment_status_bar(float a1)
{
    if (_statusBarActive) {
        _statusBar.field_1C = _statusBar.field_20 + (int)(a1 * (_statusBar.field_24 - _statusBar.field_20));
        _drawStatusBar();
        soundContinueAll();
    }
}

// 0x4B6020
void _real_win_add_status_bar(int win, int a2, char* a3, char* a4, int x, int y)
{
    int imageWidth1;
    int imageHeight1;
    int imageWidth2;
    int imageHeight2;

    _freeStatusBar();

    _statusBar.field_0 = datafileReadRaw(a4, &imageWidth1, &imageHeight1);
    _statusBar.field_4 = datafileReadRaw(a3, &imageWidth2, &imageHeight2);

    if (imageWidth2 == imageWidth1 && imageHeight2 == imageHeight1) {
        _statusBar.x = x;
        _statusBar.y = y;
        _statusBar.width = imageWidth1;
        _statusBar.height = imageHeight1;
        _statusBar.win = win;
        _real_win_set_status_bar(a2, 0, 0);
        _statusBarActive = 1;
    } else {
        _freeStatusBar();
        debugPrint("status bar dimensions not the same\n");
    }
}

// 0x4B60CC
void _real_win_get_status_info(int a1, int* a2, int* a3, int* a4)
{
    if (_statusBarActive) {
        *a2 = _statusBar.field_1C;
        *a3 = _statusBar.field_20;
        *a4 = _statusBar.field_24;
    } else {
        *a2 = -1;
        *a3 = -1;
        *a4 = -1;
    }
}

// 0x4B6100
void _real_win_modify_status_info(int a1, int a2, int a3, int a4)
{
    if (_statusBarActive) {
        _statusBar.field_1C = a2;
        _statusBar.field_20 = a3;
        _statusBar.field_24 = a4;
    }
}
