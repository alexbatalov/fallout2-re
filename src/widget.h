#ifndef WIDGET_H
#define WIDGET_H

typedef struct StatusBar {
    void* field_0;
    void* field_4;
    int field_8;
    int field_C;
    int field_10;
    int field_14;
    int field_18;
    int field_1C;
    int field_20;
    int field_24;
} StatusBar;

extern int _updateRegions[32];
extern StatusBar _statusBar;
extern int _statuBarActive;

void _deleteChar(char* string, int pos, int length);
void _insertChar(char* string, char ch, int pos, int length);
void _showRegion(int a1);
int _update_widgets();
void _freeStatusBar();
void sub_4B5998(int win);

#endif /* WIDGET_H */
