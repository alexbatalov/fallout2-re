#ifndef DBOX_H
#define DBOX_H

typedef enum DialogBoxOptions {
    DIALOG_BOX_LARGE = 0x01,
    DIALOG_BOX_MEDIUM = 0x02,
    DIALOG_BOX_NO_HORIZONTAL_CENTERING = 0x04,
    DIALOG_BOX_NO_VERTICAL_CENTERING = 0x08,
    DIALOG_BOX_YES_NO = 0x10,
    DIALOG_BOX_0x20 = 0x20,
} DialogBoxOptions;

typedef enum DialogType {
    DIALOG_TYPE_MEDIUM,
    DIALOG_TYPE_LARGE,
    DIALOG_TYPE_COUNT,
} DialogType;

extern const int gDialogBoxBackgroundFrmIds[DIALOG_TYPE_COUNT];
extern const int dword_5108D0[DIALOG_TYPE_COUNT];
extern const int dword_5108D8[DIALOG_TYPE_COUNT];
extern const int dword_5108E0[DIALOG_TYPE_COUNT];
extern const int dword_5108E8[DIALOG_TYPE_COUNT];
extern const int dword_5108F0[DIALOG_TYPE_COUNT];
extern int dword_510900[7];
extern int dword_51091C[7];

int showDialogBox(const char* title, const char** body, int bodyLength, int x, int y, int titleColor, const char* a8, int bodyColor, int flags);
int sub_41EA78(char* a1, char** fileList, char* fileName, int fileListLength, int x, int y, int flags);

#endif /* DBOX_H */