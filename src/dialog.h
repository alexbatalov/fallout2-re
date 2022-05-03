#ifndef DIALOG_H
#define DIALOG_H

#include "interpreter.h"

typedef void DialogFunc1(int win);
typedef void DialogFunc2(int win);

typedef struct STRUCT_56DAE0_FIELD_4_FIELD_C {
    char* field_0;
    char* field_4;
    int field_8;
    int field_C;
    int field_10;
    int field_14;
    short field_18;
    short field_1A;
} STRUCT_56DAE0_FIELD_4_FIELD_C;

typedef struct STRUCT_56DAE0_FIELD_4 {
    void* field_0;
    char* field_4;
    void* field_8;
    STRUCT_56DAE0_FIELD_4_FIELD_C* field_C;
    int field_10;
    int field_14;
    int field_18; // probably font number
} STRUCT_56DAE0_FIELD_4;

typedef struct STRUCT_56DAE0 {
    Program* field_0;
    STRUCT_56DAE0_FIELD_4* field_4;
    int field_8;
    int field_C;
    int field_10;
    int field_14;
    int field_18;
} STRUCT_56DAE0;

extern const float flt_501623;
extern const float flt_501627;

extern int dword_5184B4;
extern int dword_5184B8;
extern int dword_5184BC;
extern DialogFunc1* off_5184E4;
extern DialogFunc2* off_5184E8;
extern int gDialogBorderX;
extern int gDialogBorderY;
extern int gDialogOptionSpacing;
extern int dword_5184F8;
extern int dword_5184FC;
extern int dword_518500;
extern int dword_518504;
extern int dword_518508;

extern STRUCT_56DAE0 stru_56DAE0[4];
extern short word_56DB60;
extern int dword_56DB64;
extern int dword_56DB68;
extern int dword_56DB6C;
extern int dword_56DB70;
extern int dword_56DB74;
extern int dword_56DB7C;
extern int dword_56DB80;
extern int dword_56DB84;
extern int dword_56DB88;
extern int dword_56DB8C;
extern int dword_56DB90;
extern int dword_56DB94;
extern int gDialogReplyColorG;
extern int gDialogReplyColorB;
extern int gDialogOptionColorG;
extern int gDialogReplyColorR;
extern int gDialogOptionColorB;
extern int gDialogOptionColorR;
extern int dword_56DBB4;
extern int dword_56DBB8;
extern int dword_56DBBC;
extern void* off_56DBC0;
extern void* off_56DBC4;
extern void* off_56DBC8;
extern void* off_56DBCC;
extern char* gDialogReplyTitle;
extern int dword_56DBD4;
extern int dword_56DBD8;
extern int dword_56DBDC;
extern void* off_56DBE0;
extern void* off_56DBE4;
extern void* off_56DBE8;
extern void* off_56DBEC;

STRUCT_56DAE0_FIELD_4* sub_42F434();
void sub_42F4C0(const char* a1, const char* a2, int a3);
void sub_42F624(const char* a1, const char* a2, int a3);
void sub_42F714(STRUCT_56DAE0_FIELD_4_FIELD_C* a1);
void sub_42F754();
int sub_42FB94();
void sub_42FC70(int win, char** strings, int strings_num, int a4, int a5, int a6, int a7, int a8, int a9);
void sub_42FCF0(int win, char* a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
int sub_430104(int a1);
void sub_430180();
void sub_4301E8(int win, char* a2, int font, int width, int height, int left, int top, int a8, int a9, int a10);
int sub_430D40(Program* a1);
int sub_430DB8();
int sub_430DE4(const char* a1);
int dialogSetReplyTitle(const char* a1);
int sub_430EFC(const char* a1, const char* a2);
int sub_430F04(const char* a1, const char* a2);
int sub_430F38(const char* a1, const char* a2);
int sub_431184();
int sub_431198();
int dialogSetOptionWindow(int a1, int a2, int a3, int a4, int a5);
int dialogSetReplyWindow(int a1, int a2, int a3, int a4, int a5);
int dialogSetBorder(int a1, int a2);
int sub_431218(int a1, int a2, void* a3, void* a4, void* a5, void* a6, int a7);
int sub_4312C0(int a1, int a2, void* a3, void* a4, void* a5, void* a6, int a7);
int dialogSetOptionSpacing(int value);
int dialogSetOptionColor(float a1, float a2, float a3);
int dialogSetReplyColor(float a1, float a2, float a3);
int sub_431420(int flags);
void sub_431434();
int sub_431518();
void sub_431520(DialogFunc1* a1, DialogFunc2* a2);
int sub_431530(int a1);
int sub_431554();

#endif /* DIALOG_H */
