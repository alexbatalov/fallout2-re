#include "dialog.h"

#include "core.h"
#include "memory_manager.h"
#include "movie.h"
#include "text_font.h"
#include "widget.h"
#include "window_manager.h"

#include <string.h>

// 0x501623
const float flt_501623 = 31.0;

// 0x501627
const float flt_501627 = 31.0;

// 0x5184B4
int dword_5184B4 = -1;

// 0x5184B8
int dword_5184B8 = 0;

// 0x5184BC
int dword_5184BC = 0;

// 0x5184E4
DialogFunc1* off_5184E4 = NULL;

// 0x5184E8
DialogFunc2* off_5184E8 = NULL;

// 0x5184EC
int gDialogBorderX = 7;

// 0x5184F0
int gDialogBorderY = 7;

// 0x5184F4
int gDialogOptionSpacing = 5;

// 0x5184F8
int dword_5184F8 = 0;

// 0x5184FC
int dword_5184FC = 0;

// 0x518500
int dword_518500 = 0;

// 0x518504
int dword_518504 = 0;

// 0x518508
int dword_518508 = 2;

// 0x56DAE0
STRUCT_56DAE0 stru_56DAE0[4];

// Reply flags.
//
// 0x56DB60
short word_56DB60;

// 0x56DB64
int dword_56DB64;

// 0x56DB68
int dword_56DB68;

// 0x56DB6C
int dword_56DB6C;

// 0x56DB70
int dword_56DB70;

// 0x56DB74
int dword_56DB74;

// 0x56DB7C
int dword_56DB7C;

// 0x56DB80
int dword_56DB80;

// 0x56DB84
int dword_56DB84;

// 0x56DB88
int dword_56DB88;

// 0x56DB8C
int dword_56DB8C;

// 0x56DB90
int dword_56DB90;

// 0x56DB94
int dword_56DB94 = -1;

// 0x56DB98
int gDialogReplyColorG;

// 0x56DB9C
int gDialogReplyColorB;

// 0x56DBA4
int gDialogOptionColorG;

// 0x56DBA8
int gDialogReplyColorR;

// 0x56DBAC
int gDialogOptionColorB;

// 0x56DBB0
int gDialogOptionColorR;

// 0x56DBB4
int dword_56DBB4;

// 0x56DBB8
int dword_56DBB8;

// 0x56DBBC
int dword_56DBBC;

// 0x56DBC0
void* off_56DBC0;

// 0x56DBC4
void* off_56DBC4;

// 0x56DBC8
void* off_56DBC8;

// 0x56DBCC
void* off_56DBCC;

// 0x56DBD0
char* gDialogReplyTitle;

// 0x56DBD4
int dword_56DBD4;

// 0x56DBD8
int dword_56DBD8;

// 0x56DBDC
int dword_56DBDC;

// 0x56DBE0
void* off_56DBE0;

// 0x56DBE4
void* off_56DBE4;

// 0x56DBE8
void* off_56DBE8;

// 0x56DBEC
void* off_56DBEC;

// 0x42F434
STRUCT_56DAE0_FIELD_4* getReply()
{
    STRUCT_56DAE0_FIELD_4* v0;
    STRUCT_56DAE0_FIELD_4_FIELD_C* v1;

    v0 = &(stru_56DAE0[dword_5184B4].field_4[stru_56DAE0[dword_5184B4].field_C]);
    if (v0->field_C == NULL) {
        v0->field_14 = 1;
        v1 = internal_malloc_safe(sizeof(STRUCT_56DAE0_FIELD_4_FIELD_C), __FILE__, __LINE__); // "..\\int\\DIALOG.C", 789
    } else {
        v0->field_14++;
        v1 = internal_realloc_safe(v0->field_C, sizeof(STRUCT_56DAE0_FIELD_4_FIELD_C) * v0->field_14, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 793
    }
    v0->field_C = v1;

    return v0;
}

// 0x42F4C0
void replyAddOption(const char* a1, const char* a2, int a3)
{
    STRUCT_56DAE0_FIELD_4* v18;
    int v17;
    char* v14;
    char* v15;

    v18 = getReply();
    v17 = v18->field_14 - 1;
    v18->field_C[v17].field_8 = 2;

    if (a1 != NULL) {
        v14 = internal_malloc_safe(strlen(a1) + 1, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 805
        strcpy(v14, a1);
        v18->field_C[v17].field_0 = v14;
    } else {
        v18->field_C[v17].field_0 = NULL;
    }

    if (a2 != NULL) {
        v15 = internal_malloc_safe(strlen(a2) + 1, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 810
        strcpy(v15, a2);
        v18->field_C[v17].field_4 = v15;
    } else {
        v18->field_C[v17].field_4 = NULL;
    }

    v18->field_C[v17].field_18 = widgetGetFont();
    v18->field_C[v17].field_1A = word_56DB60;
    v18->field_C[v17].field_14 = a3;
}

// 0x42F624
void replyAddOptionProc(const char* a1, const char* a2, int a3)
{
    STRUCT_56DAE0_FIELD_4* v5;
    int v13;
    char* v11;

    v5 = getReply();
    v13 = v5->field_14 - 1;

    v5->field_C[v13].field_8 = 1;

    if (a1 != NULL) {
        v11 = internal_malloc_safe(strlen(a1) + 1, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 830
        strcpy(v11, a1);
        v5->field_C[v13].field_0 = v11;
    } else {
        v5->field_C[v13].field_0 = NULL;
    }

    v5->field_C[v13].field_4 = (char*)a2;

    v5->field_C[v13].field_18 = widgetGetFont();
    v5->field_C[v13].field_1A = word_56DB60;
    v5->field_C[v13].field_14 = a3;
}

// 0x42F714
void optionFree(STRUCT_56DAE0_FIELD_4_FIELD_C* a1)
{
    if (a1->field_0 != NULL) {
        internal_free_safe(a1->field_0, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 844
    }

    if (a1->field_8 == 2) {
        if (a1->field_4 != NULL) {
            internal_free_safe(a1->field_4, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 846
        }
    }
}

// 0x42F754
void replyFree()
{
    int i;
    int j;
    STRUCT_56DAE0* ptr;
    STRUCT_56DAE0_FIELD_4* v6;

    ptr = &(stru_56DAE0[dword_5184B4]);
    for (i = 0; i < ptr->field_8; i++) {
        v6 = &(stru_56DAE0[dword_5184B4].field_4[i]);

        if (v6->field_C != NULL) {
            for (j = 0; j < v6->field_14; j++) {
                optionFree(&(v6->field_C[j]));
            }

            internal_free_safe(v6->field_C, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 857
        }

        if (v6->field_8 != NULL) {
            internal_free_safe(v6->field_8, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 860
        }

        if (v6->field_4 != NULL) {
            internal_free_safe(v6->field_4, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 862
        }

        if (v6->field_0 != NULL) {
            internal_free_safe(v6->field_0, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 864
        }
    }

    if (ptr->field_4 != NULL) {
        internal_free_safe(ptr->field_4, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 867
    }
}

// 0x42FB94
int endDialog()
{
    if (dword_5184B4 == -1) {
        return -1;
    }

    dword_5184BC = stru_56DAE0[dword_5184B4].field_10;
    replyFree();

    if (gDialogReplyTitle != NULL) {
        internal_free_safe(gDialogReplyTitle, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 986
        gDialogReplyTitle = NULL;
    }

    --dword_5184B4;

    return 0;
}

// 0x42FC70
void printLine(int win, char** strings, int strings_num, int a4, int a5, int a6, int a7, int a8, int a9)
{
    int i;
    int v11;

    for (i = 0; i < strings_num; i++) {
        v11 = a7 + i * fontGetLineHeight();
        windowPrintBuf(win, strings[i], strlen(strings[i]), a4, a5 + a7, a6, v11, a8, a9);
    }
}

// 0x42FCF0
void printStr(int win, char* a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
    char** strings;
    int strings_num;

    strings = windowWordWrap(a2, a3, 0, &strings_num);
    printLine(win, strings, strings_num, a3, a4, a5, a6, a7, a8);
    windowFreeWordList(strings, strings_num);
}

// 0x430104
int abortReply(int a1)
{
    int result;
    int y;
    int x;

    if (dword_56DB90 == 2) {
        return moviePlaying() == 0;
    } else if (dword_56DB90 == 3) {
        return 1;
    }

    result = 1;
    if (a1) {
        if (dword_56DB94 != -1) {
            if (!(mouseGetEvent() & 0x10)) {
                result = 0;
            } else {
                mouseGetPosition(&x, &y);

                if (windowGetAtPoint(x, y) != dword_56DB94) {
                    result = 0;
                }
            }
        }
    }
    return result;
}

// 0x430180
void endReply()
{
    if (dword_56DB90 != 2) {
        if (dword_56DB90 == 1) {
            if (!(dword_518508 & 2) && dword_56DB94 != -1) {
                windowDestroy(dword_56DB94);
                dword_56DB94 = -1;
            }
        } else if (dword_56DB90 != 3 && dword_56DB94 != -1) {
            windowDestroy(dword_56DB94);
            dword_56DB94 = -1;
        }
    }
}

// 0x4301E8
void drawStr(int win, char* str, int font, int width, int height, int left, int top, int a8, int a9, int a10)
{
    int old_font;
    Rect rect;

    old_font = widgetGetFont();
    widgetSetFont(font);

    printStr(win, str, width, height, left, top, a8, a9, a10);

    rect.left = left;
    rect.top = top;
    rect.right = width + left;
    rect.bottom = height + top;
    windowRefreshRect(win, &rect);
    widgetSetFont(old_font);
}

// 0x430D40
int dialogStart(Program* a1)
{
    STRUCT_56DAE0* ptr;

    if (dword_5184B4 == 3) {
        return 1;
    }

    ptr = &(stru_56DAE0[dword_5184B4]);
    ptr->field_0 = a1;
    ptr->field_4 = 0;
    ptr->field_8 = 0;
    ptr->field_C = -1;
    ptr->field_10 = -1;
    ptr->field_14 = 1;
    ptr->field_10 = 1;

    dword_5184B4++;

    return 0;
}

// 0x430DB8
int dialogRestart()
{
    if (dword_5184B4 == -1) {
        return 1;
    }

    stru_56DAE0[dword_5184B4].field_10 = 0;

    return 0;
}

// 0x430DE4
int dialogGotoReply(const char* a1)
{
    STRUCT_56DAE0* ptr;
    STRUCT_56DAE0_FIELD_4* v5;
    int i;

    if (dword_5184B4 == -1) {
        return 1;
    }

    if (a1 != NULL) {
        ptr = &(stru_56DAE0[dword_5184B4]);
        for (i = 0; i < ptr->field_8; i++) {
            v5 = &(ptr->field_4[i]);
            if (v5->field_4 != NULL && stricmp(v5->field_4, a1) == 0) {
                ptr->field_10 = i;
                return 0;
            }
        }

        return 1;
    }

    stru_56DAE0[dword_5184B4].field_10 = 0;

    return 0;
}

// 0x430E84
int dialogSetReplyTitle(const char* a1)
{
    if (gDialogReplyTitle != NULL) {
        internal_free_safe(gDialogReplyTitle, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2561
    }

    if (a1 != NULL) {
        gDialogReplyTitle = internal_malloc_safe(strlen(a1) + 1, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2564
        strcpy(gDialogReplyTitle, a1);
    } else {
        gDialogReplyTitle = NULL;
    }

    return 0;
}

// 0x430EFC
int dialogReply(const char* a1, const char* a2)
{
    // TODO: Incomplete.
    // replyAddNew(a1, a2);
    return 0;
}

// 0x430F04
int dialogOption(const char* a1, const char* a2)
{
    if (stru_56DAE0[dword_5184B4].field_C == -1) {
        return 0;
    }

    replyAddOption(a1, a2, 0);

    return 0;
}

// 0x430F38
int dialogOptionProc(const char* a1, const char* a2)
{
    if (stru_56DAE0[dword_5184B4].field_C == -1) {
        return 1;
    }

    replyAddOptionProc(a1, a2, 0);

    return 0;
}

// 0x431184
int dialogGetExitPoint()
{
    return dword_5184B8 + (dword_5184BC << 16);
}

// 0x431198
int dialogQuit()
{
    if (dword_518504) {
        dword_518500 = 1;
    } else {
        endDialog();
    }

    return 0;
}

// 0x4311B8
int dialogSetOptionWindow(int a1, int a2, int a3, int a4, int a5)
{
    dword_56DB6C = a1;
    dword_56DB70 = a2;
    dword_56DB64 = a3;
    dword_56DB68 = a4;
    dword_56DB74 = a5;
    return 0;
}

// 0x4311E0
int dialogSetReplyWindow(int a1, int a2, int a3, int a4, int a5)
{
    dword_56DB84 = a1;
    dword_56DB88 = a2;
    dword_56DB7C = a3;
    dword_56DB80 = a4;
    dword_56DB8C = a5;

    return 0;
}

// 0x431208
int dialogSetBorder(int a1, int a2)
{
    gDialogBorderX = a1;
    gDialogBorderY = a2;

    return 0;
}

// 0x431218
int dialogSetScrollUp(int a1, int a2, void* a3, void* a4, void* a5, void* a6, int a7)
{
    dword_56DBD4 = a1;
    dword_56DBD8 = a2;

    if (off_56DBE0 != NULL) {
        internal_free_safe(off_56DBE0, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2750
    }
    off_56DBE0 = a3;

    if (off_56DBE4 != NULL) {
        internal_free_safe(off_56DBE4, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2752
    }
    off_56DBE4 = a4;

    if (off_56DBE8 != NULL) {
        internal_free_safe(off_56DBE8, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2754
    }
    off_56DBE8 = a5;

    if (off_56DBEC != NULL) {
        internal_free_safe(off_56DBEC, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2756
    }
    off_56DBEC = a5;

    dword_56DBDC = a7;

    return 0;
}

// 0x4312C0
int dialogSetScrollDown(int a1, int a2, void* a3, void* a4, void* a5, void* a6, int a7)
{
    dword_56DBB4 = a1;
    dword_56DBB8 = a2;

    if (off_56DBC0 != NULL) {
        internal_free_safe(off_56DBC0, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2765
    }
    off_56DBC0 = a3;

    if (off_56DBC4 != NULL) {
        internal_free_safe(off_56DBC4, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2767
    }
    off_56DBC4 = a4;

    if (off_56DBC8 != NULL) {
        internal_free_safe(off_56DBC8, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2769
    }
    off_56DBC8 = a5;

    if (off_56DBCC != NULL) {
        internal_free_safe(off_56DBCC, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2771
    }
    off_56DBCC = a6;

    dword_56DBBC = a7;

    return 0;
}

// 0x431368
int dialogSetOptionSpacing(int value)
{
    gDialogOptionSpacing = value;

    return 0;
}

// 0x431370
int dialogSetOptionColor(float a1, float a2, float a3)
{
    gDialogOptionColorR = (int)(a1 * flt_501623);
    gDialogOptionColorG = (int)(a2 * flt_501623);
    gDialogOptionColorB = (int)(a3 * flt_501623);

    dword_5184FC = 1;

    return 0;
}

// 0x4313C8
int dialogSetReplyColor(float a1, float a2, float a3)
{
    gDialogReplyColorR = (int)(a1 * flt_501627);
    gDialogReplyColorG = (int)(a2 * flt_501627);
    gDialogReplyColorB = (int)(a3 * flt_501627);

    dword_5184F8 = 1;

    return 0;
}

// 0x431420
int dialogSetOptionFlags(int flags)
{
    word_56DB60 = flags & 0xFFFF;

    return 1;
}

// 0x431434
void dialogClose()
{
    if (off_56DBE0) {
        internal_free_safe(off_56DBE0, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2818
    }

    if (off_56DBE4) {
        internal_free_safe(off_56DBE4, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2819
    }

    if (off_56DBE8) {
        internal_free_safe(off_56DBE8, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2820
    }

    if (off_56DBEC) {
        internal_free_safe(off_56DBEC, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2821
    }

    if (off_56DBC0) {
        internal_free_safe(off_56DBC0, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2823
    }

    if (off_56DBC4) {
        internal_free_safe(off_56DBC4, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2824
    }

    if (off_56DBC8) {
        internal_free_safe(off_56DBC8, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2825
    }

    if (off_56DBCC) {
        internal_free_safe(off_56DBCC, __FILE__, __LINE__); // "..\\int\\DIALOG.C", 2826
    }
}

// 0x431518
int dialogGetDialogDepth()
{
    return dword_5184B4;
}

// 0x431520
void dialogRegisterWinDrawCallbacks(DialogFunc1* a1, DialogFunc2* a2)
{
    off_5184E4 = a1;
    off_5184E8 = a2;
}

// 0x431530
int dialogToggleMediaFlag(int a1)
{
    if ((a1 & dword_518508) == a1) {
        dword_518508 &= ~a1;
    } else {
        dword_518508 |= a1;
    }

    return dword_518508;
}

// 0x431554
int dialogGetMediaFlag()
{
    return dword_518508;
}
