#include "trap.h"

#include <stdlib.h>

#include "debug.h"
#include "window_manager.h"

static void heap_report(int trap, int address, const char* file, int line);

// NOTE: Likely collapsed trapInit/trapExit.
//
// 0x4B4190
void _trap_init()
{
}

// 0x4B44F0
static void heap_report(int trap, int address, const char* file, int line)
{
    debugPrint("TRAPPED A STOMP ERROR:\n");
    debugPrint("Stomp caught by check on line %d", line);
    debugPrint(" of module %s.\n", file);
    debugPrint("Data stomped was in heap trap number %d", trap);
    debugPrint(" at address %p.\n", address);
    debugPrint("See comment in trap.c for suggestions on better");
    debugPrint(" isolating the stomp bug.\n");
    showMesageBox("STOMPED!");
    exit(1);
}
