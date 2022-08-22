#include "widget.h"

#include <string.h>

#include "draw.h"
#include "geometry.h"
#include "memory_manager.h"
#include "sound.h"
#include "window_manager.h"

// 0x66E6A0
int _updateRegions[32];

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

// 0x4B5A64
void _showRegion(int a1)
{
    // TODO: Incomplete.
}

// 0x4B5C24
int _update_widgets()
{
    for (int index = 0; index < 32; index++) {
        if (_updateRegions[index]) {
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
    memset(_updateRegions, 0, sizeof(_updateRegions));

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
