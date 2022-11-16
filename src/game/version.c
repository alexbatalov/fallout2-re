#include "game/version.h"

#include <stdio.h>

// 0x4B4580
void getverstr(char* dest)
{
    sprintf(dest, "FALLOUT II %d.%02d", VERSION_MAJOR, VERSION_MINOR);
}
