#include "selfrun.h"

#include "db.h"
#include "game.h"

#include <stdlib.h>

// 0x51C8D8
int dword_51C8D8 = 0;

// 0x4A8BE0
int sub_4A8BE0(char*** fileListPtr, int* fileListLengthPtr)
{
    if (fileListPtr == NULL) {
        return -1;
    }

    if (fileListLengthPtr == NULL) {
        return -1;
    }

    *fileListLengthPtr = fileNameListInit("selfrun\\*.sdf", fileListPtr, 0, 0);

    return 0;
}

// 0x4A8C10
int sub_4A8C10(char*** fileListPtr)
{
    if (fileListPtr == NULL) {
        return -1;
    }

    fileNameListFree(fileListPtr, 0);

    return 0;
}

// 0x4A8E74
void sub_4A8E74()
{
    dword_5186CC = 2;
    dword_51C8D8 = 0;
}