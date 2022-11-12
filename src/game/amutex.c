#include "game/amutex.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// 0x530010
static HANDLE autorun_mutex;

// 0x4139C0
bool autorun_mutex_create()
{
    autorun_mutex = CreateMutexA(NULL, FALSE, "InterplayGenericAutorunMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(autorun_mutex);
        return false;
    }

    return true;
}

// 0x413A00
void autorun_mutex_destroy()
{
    if (autorun_mutex != NULL) {
        CloseHandle(autorun_mutex);
    }
}
