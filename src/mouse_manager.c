#include "mouse_manager.h"

#include "core.h"

// 0x5195A8
char* (*off_5195A8)(char*) = _defaultNameMangler;

// 0x5195AC
int (*off_5195AC)() = _defaultRateCallback;

// 0x5195B0
int (*off_5195B0)() = _defaultTimeCallback;

// 0x5195B4
int dword_5195B4 = 1;

// 0x485250
char* _defaultNameMangler(char* a1)
{
    return a1;
}

// 0x485254
int _defaultRateCallback()
{
    return 1000;
}

// 0x48525C
int _defaultTimeCallback()
{
    return _get_time();
}

// 0x485288
void _mousemgrSetNameMangler(char* (*func)(char*))
{
    off_5195A8 = func;
}

// 0x48568C
void _initMousemgr()
{
    mouseSetSensitivity(1.0);
}

// 0x4865C4
void _mouseHide()
{
    mouseHideCursor();
}

// 0x4865CC
void _mouseShow()
{
    mouseShowCursor();
}
