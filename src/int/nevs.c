#include "int/nevs.h"

#include <stdlib.h>
#include <string.h>

#include "plib/gnw/debug.h"
#include "int/intlib.h"
#include "int/memdbg.h"

#define NEVS_COUNT 40

typedef struct Nevs {
    bool used;
    char name[32];
    Program* program;
    int proc;
    int type;
    int hits;
    bool busy;
    NevsCallback* callback;
} Nevs;

static_assert(sizeof(Nevs) == 60, "wrong size");

static Nevs* nevs_alloc();
static void nevs_free(Nevs* nevs);
static void nevs_removeprogramreferences(Program* program);
static Nevs* nevs_find(const char* name);

// 0x6391C8
static Nevs* nevs;

// 0x6391CC
static int anyhits;

// 0x488340
static Nevs* nevs_alloc()
{
    int index;
    Nevs* entry;

    if (nevs == NULL) {
        debug_printf("nevs_alloc(): nevs_initonce() not called!");
        exit(99);
    }

    for (index = 0; index < NEVS_COUNT; index++) {
        entry = &(nevs[index]);
        if (!entry->used) {
            // NOTE: Uninline.
            nevs_free(entry);
            return entry;
        }
    }

    return NULL;
}

// NOTE: Inlined.
//
// 0x488394
static void nevs_free(Nevs* entry)
{
    entry->used = false;
    memset(entry, 0, sizeof(*entry));
}

// 0x4883AC
void nevs_close()
{
    if (nevs != NULL) {
        myfree(nevs, __FILE__, __LINE__); // "..\\int\\NEVS.C", 97
        nevs = NULL;
    }
}

// 0x4883D4
static void nevs_removeprogramreferences(Program* program)
{
    int index;
    Nevs* entry;

    if (nevs != NULL) {
        for (index = 0; index < NEVS_COUNT; index++) {
            entry = &(nevs[index]);
            if (entry->used && entry->program == program) {
                // NOTE: Uninline.
                nevs_free(entry);
            }
        }
    }
}

// 0x488418
void nevs_initonce()
{
    interpretRegisterProgramDeleteCallback(nevs_removeprogramreferences);

    if (nevs == NULL) {
        nevs = (Nevs*)mycalloc(sizeof(Nevs), NEVS_COUNT, __FILE__, __LINE__); // "..\\int\\NEVS.C", 131
        if (nevs == NULL) {
            debug_printf("nevs_initonce(): out of memory");
            exit(99);
        }
    }
}

// 0x48846C
static Nevs* nevs_find(const char* name)
{
    int index;
    Nevs* entry;

    if (nevs == NULL) {
        debug_printf("nevs_find(): nevs_initonce() not called!");
        exit(99);
    }

    for (index = 0; index < NEVS_COUNT; index++) {
        entry = &(nevs[index]);
        if (entry->used && stricmp(entry->name, name) == 0) {
            return entry;
        }
    }

    return NULL;
}

// 0x4884C8
int nevs_addevent(const char* name, Program* program, int proc, int type)
{
    Nevs* entry;

    entry = nevs_find(name);
    if (entry == NULL) {
        entry = nevs_alloc();
    }

    if (entry == NULL) {
        return 1;
    }

    entry->used = true;
    strcpy(entry->name, name);
    entry->program = program;
    entry->proc = proc;
    entry->type = type;
    entry->callback = NULL;

    return 0;
}

// NOTE: Unused.
//
// 0x488528
int nevs_addCevent(const char* name, NevsCallback* callback, int type)
{
    Nevs* entry;

    debug_printf("nevs_addCevent( '%s', %p);\n", name, callback);

    entry = nevs_find(name);
    if (entry == NULL) {
        entry = nevs_alloc();
    }

    if (entry == NULL) {
        return 1;
    }

    entry->used = true;
    strcpy(entry->name, name);
    entry->program = NULL;
    entry->proc = 0;
    entry->type = type;
    entry->callback = NULL;

    return 0;
}

// 0x48859C
int nevs_clearevent(const char* a1)
{
    Nevs* entry;

    debug_printf("nevs_clearevent( '%s');\n", a1);

    entry = nevs_find(a1);
    if (entry != NULL) {
        // NOTE: Uninline.
        nevs_free(entry);
        return 0;
    }

    return 1;
}

// 0x48862C
int nevs_signal(const char* name)
{
    Nevs* entry;

    debug_printf("nevs_signal( '%s');\n", name);

    entry = nevs_find(name);
    if (entry == NULL) {
        return 1;
    }

    debug_printf("nep: %p,  used = %u, prog = %p, proc = %d", entry, entry->used, entry->program, entry->proc);

    if (entry->used
        && ((entry->program != NULL && entry->proc != 0) || entry->callback != NULL)
        && !entry->busy) {
        entry->hits++;
        anyhits++;
        return 0;
    }

    return 1;
}

// 0x4886AC
void nevs_update()
{
    int index;
    Nevs* entry;

    if (anyhits == 0) {
        return;
    }

    debug_printf("nevs_update(): we have anyhits = %u\n", anyhits);

    anyhits = 0;

    for (index = 0; index < NEVS_COUNT; index++) {
        entry = &(nevs[index]);
        if (entry->used
            && ((entry->program != NULL && entry->proc != 0) || entry->callback != NULL)
            && !entry->busy) {
            if (entry->hits > 0) {
                entry->busy = true;

                entry->hits -= 1;
                anyhits += entry->hits;

                if (entry->callback == NULL) {
                    executeProc(entry->program, entry->proc);
                } else {
                    entry->callback(entry->name);
                }

                entry->busy = false;

                if (entry->type == NEVS_TYPE_EVENT) {
                    // NOTE: Uninline.
                    nevs_free(entry);
                }
            }
        }
    }
}
