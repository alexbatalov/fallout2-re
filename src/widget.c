#include "widget.h"

#include <string.h>

// 0x66E6A0
int _updateRegions[32];

// 0x4B45A0
void _deleteChar(char* string, int pos, int length)
{
    if (length > pos) {
        memcpy(string + pos, string + pos + 1, length - pos);
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
