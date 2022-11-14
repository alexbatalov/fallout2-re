#ifndef NEVS_H
#define NEVS_H

#include <stdbool.h>

#include "int/intrpret.h"

#define NEVS_COUNT (40)

typedef enum NevsType {
    NEVS_TYPE_EVENT = 0,
    NEVS_TYPE_HANDLER = 1,
} NevsType;

typedef struct Nevs {
    bool used;
    char name[32];
    Program* program;
    int proc;
    int type;
    int hits;
    bool busy;
    void (*field_38)();
} Nevs;

extern Nevs* _nevs;

extern int _anyhits;

Nevs* _nevs_alloc();
void _nevs_reset(Nevs* nevs);
void _nevs_close();
void _nevs_removeprogramreferences(Program* program);
void _nevs_initonce();
Nevs* _nevs_find(const char* name);
int _nevs_addevent(const char* name, Program* program, int proc, int a4);
int _nevs_clearevent(const char* name);
int _nevs_signal(const char* name);
void _nevs_update();

#endif /* NEVS_H */
