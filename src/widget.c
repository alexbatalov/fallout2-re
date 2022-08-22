#include "widget.h"

#include <string.h>

#include "datafile.h"
#include "debug.h"
#include "draw.h"
#include "geometry.h"
#include "memory_manager.h"
#include "sound.h"
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

// 0x4B58A0
int _win_delete_all_update_regions(int a1)
{
    int index;

    for (index = 0; index < WIDGET_UPDATE_REGIONS_CAPACITY; index++) {
        if (_updateRegions[index] != NULL) {
            if (a1 == _updateRegions[index]->field_0) {
                internal_free_safe(_updateRegions[index], __FILE__, __LINE__); // "..\int\WIDGET.C", 722
                _updateRegions[index] = NULL;
            }
        }
    }

    return 1;
}

// 0x4B5A64
void _showRegion(UpdateRegion* updateRegion)
{
    // TODO: Incomplete.
}

// 0x4B5BE8
int _draw_widgets()
{
    int index;

    for (index = 0; index < WIDGET_UPDATE_REGIONS_CAPACITY; index++) {
        if (_updateRegions[index] != NULL) {
            if ((_updateRegions[index]->field_C & 0xFF00) == 0x800) {
                _updateRegions[index]->drawFunc(_updateRegions[index]->field_14);
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

// 0x4B5998
void sub_4B5998(int win)
{
    // TODO: Incomplete.
}

// 0x4B5C4C
int _win_register_update(int a1, int a2, int a3, int a4, UpdateRegionDrawFunc* drawFunc, int a6, int a7, int a8)
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
    _updateRegions[updateRegionIndex]->field_0 = a1;
    _updateRegions[updateRegionIndex]->field_4 = a2;
    _updateRegions[updateRegionIndex]->field_8 = a3;
    _updateRegions[updateRegionIndex]->field_C = a7;
    _updateRegions[updateRegionIndex]->field_10 = a8;
    _updateRegions[updateRegionIndex]->field_14 = a6;
    _updateRegions[updateRegionIndex]->field_18 = a4;
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
        windowRefreshRect(_statusBar.win, &rect);
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
