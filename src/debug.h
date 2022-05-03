#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

typedef int(DebugPrintProc)(char* string);

extern FILE* off_51DEF8;
extern int dword_51DEFC;
extern int dword_51DF00;
extern DebugPrintProc* gDebugPrintProc;

void sub_4C6CD0();
void sub_4C6CDC();
void sub_4C6D18(const char* fileName, const char* mode);
void sub_4C6D5C();
void sub_4C6D90();
void sub_4C6F18(DebugPrintProc* proc);
int debugPrint(const char* format, ...);
int sub_4C6F94(char* string);
void sub_4C6FAC();
int sub_4C7004(char* string);
int sub_4C7028(char* string);
int sub_4C7068(char* string);
void sub_4C709C();
void sub_4C71E8(void);

#endif /* DEBUG_H */
