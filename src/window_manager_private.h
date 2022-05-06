#ifndef WINDOW_MANAGER_PRIVATE_H
#define WINDOW_MANAGER_PRIVATE_H

#include <stdbool.h>

typedef struct struc_177 struc_177;

typedef struct STRUCT_6B2340 {
    int field_0;
    int field_4;
} STRUCT_6B2340;

typedef struct STRUCT_6B2370 {
    int field_0;
    // win
    int field_4;
    int field_8;
} STRUCT_6B2370;

extern int dword_51E414;
extern int dword_51E418;
extern bool dword_51E41C;

extern STRUCT_6B2340 stru_6B2340[5];
extern int dword_6B2368;
extern int dword_6B236C;
extern STRUCT_6B2370 stru_6B2370[5];
extern int dword_6B23AC;
extern int dword_6B23B0;
extern int dword_6B23B4;
extern int dword_6B23B8;
extern int dword_6B23BC;
extern int dword_6B23C0;
extern int dword_6B23C4;
extern char gProgramWindowTitle[256];

int sub_4DC30C(char* a1);
void sub_4DC65C();
int sub_4DC674(int win, int x, int y, int width, int height, int a6, int a7);
int sub_4DC768(int win, int x, char* str, int a4);
int sub_4DCA30(char** fileNameList, int fileNameListLength);
int sub_4DC930(struc_177* ptr, int i);
int sub_4DD03C(int a1, int a2);
void sub_4DD3EC();
void sub_4DD4A4();
void sub_4DD66C();
void sub_4DD6C0();
void sub_4DD744(int a1);
void sub_4DD82C(int btn);
int sub_4DD870(int a1);

#endif /* WINDOW_MANAGER_PRIVATE_H */
