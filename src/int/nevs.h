#ifndef FALLOUT_INT_NEVS_H_
#define FALLOUT_INT_NEVS_H_

#include <stdbool.h>

#include "int/intrpret.h"

typedef enum NevsType {
    NEVS_TYPE_EVENT = 0,
    NEVS_TYPE_HANDLER = 1,
} NevsType;

void nevs_close();
void nevs_initonce();
int nevs_addevent(const char* name, Program* program, int proc, int a4);
int nevs_clearevent(const char* name);
int nevs_signal(const char* name);
void nevs_update();

#endif /* FALLOUT_INT_NEVS_H_ */
