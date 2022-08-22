#include "widget.h"

#include <string.h>

#include "memory_manager.h"

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
int _statuBarActive;

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

    _statuBarActive = 0;
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
