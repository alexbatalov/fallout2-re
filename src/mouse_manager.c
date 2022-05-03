#include "mouse_manager.h"

#include "core.h"

// 0x5195A8
char* (*off_5195A8)(char*) = sub_485250;

// 0x5195AC
int (*off_5195AC)() = sub_485254;

// 0x5195B0
int (*off_5195B0)() = sub_48525C;

// 0x5195B4
int dword_5195B4 = 1;

// 0x485250
char* sub_485250(char* a1)
{
    return a1;
}

// 0x485254
int sub_485254()
{
    return 1000;
}

// 0x48525C
int sub_48525C()
{
    return sub_4C9370();
}

// 0x485288
void sub_485288(char* (*func)(char*))
{
    off_5195A8 = func;
}

// 0x48568C
void sub_48568C()
{
    mouseSetSensitivity(1.0);
}

// 0x4865C4
void sub_4865C4()
{
    mouseHideCursor();
}

// 0x4865CC
void sub_4865CC()
{
    mouseShowCursor();
}
