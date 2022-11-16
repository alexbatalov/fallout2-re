#ifndef FALLOUT_PLIB_GNW_DOSCMDLN_H_
#define FALLOUT_PLIB_GNW_DOSCMDLN_H_

#include <stdbool.h>

typedef struct tagDOSCmdLine {
    int numArgs;
    char** args;
} DOSCmdLine;

void DOSCmdLineInit(DOSCmdLine* d);
bool DOSCmdLineCreate(DOSCmdLine* d, char* windowsCmdLine);
void DOSCmdLineDestroy(DOSCmdLine* d);

#endif /* FALLOUT_PLIB_GNW_DOSCMDLN_H_ */
