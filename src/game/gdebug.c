#include "game/gdebug.h"

#include <stdlib.h>

#include "plib/gnw/debug.h"
#include "window_manager.h"

// 0x444C90
void fatal_error(const char* format, const char* message, const char* file, int line)
{
    char stringBuffer[260];

    debugPrint("\n");
    debugPrint(format, message, file, line);

    windowManagerExit();

    printf("\n\n\n\n\n   ");
    printf(format, message, file, line);
    printf("\n\n\n\n\n");

    sprintf(stringBuffer, format, message, file, line);
    showMesageBox(stringBuffer);

    exit(1);
}
