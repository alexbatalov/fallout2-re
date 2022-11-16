#include "game/trap.h"

#include <stdlib.h>

#include "plib/gnw/debug.h"
#include "plib/gnw/gnw.h"

typedef struct TrapEntry {
    int address;
    int name;
    int field_8;
    int field_C;
} TrapEntry;

typedef struct DuplicateEntry {
    int address;
    int field_4;
    int size;
    int name;
    int field_10;
} DuplicateEntry;

static void duplicate_report(int trap, int offset, const char* file, int line);
static void heap_report(int trap, int address, const char* file, int line);

// NOTE: Probably |trap_list_data|.
//
// 0x51DC44
static TrapEntry* off_51DC44 = NULL;

// NOTE: Probably |trap_list_duplicate|.
//
// 0x66BE48
static DuplicateEntry stru_66BE48[512];

// 0x4B4190
void trap_exit()
{
}

// 0x4B4190
void trap_init()
{
}

// 0x4B43A4
static void trap_report(int trap, int address, const char* file, int line)
{
    TrapEntry* entry;

    entry = &(off_51DC44[trap]);
    debug_printf("TRAPPED A STOMP ERROR:\n");
    debug_printf("Stomp caught by check on line %d", line);
    debug_printf(" of module %s.\n", file);
    debug_printf("Data stomped was in trap %s", entry->name);
    debug_printf(" at address %p.\n", entry->address);
    debug_printf("See comment in trap.c for suggestions on better");
    debug_printf(" isolating the stomp bug.\n");
    showMesageBox("STOMPED!");
    exit(1);
}

// 0x4B4430
static void duplicate_report(int trap, int offset, const char* file, int line)
{
    DuplicateEntry* entry;

    entry = &(stru_66BE48[trap]);
    debug_printf("TRAPPED A STOMP ERROR:\n");
    debug_printf("Stomp caught by check on line %d", line);
    debug_printf(" of module %s.\n", file);
    debug_printf("Data stomped was in trap %s", entry->name);
    debug_printf(" at address %p.\n", entry->address + offset);
    debug_printf("This is duplicate trap number %d", trap);
    debug_printf(" at an internal offset of %d.\n", offset);
    debug_printf("Trap size is %d.\n", entry->size);
    debug_printf("See comment in trap.c for suggestions on better");
    debug_printf(" isolating the stomp bug.\n");
    showMesageBox("STOMPED!");
    exit(1);
}

// 0x4B44F0
static void heap_report(int trap, int address, const char* file, int line)
{
    debug_printf("TRAPPED A STOMP ERROR:\n");
    debug_printf("Stomp caught by check on line %d", line);
    debug_printf(" of module %s.\n", file);
    debug_printf("Data stomped was in heap trap number %d", trap);
    debug_printf(" at address %p.\n", address);
    debug_printf("See comment in trap.c for suggestions on better");
    debug_printf(" isolating the stomp bug.\n");
    showMesageBox("STOMPED!");
    exit(1);
}
