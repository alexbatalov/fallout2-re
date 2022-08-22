#ifndef WIDGET_H
#define WIDGET_H

typedef struct StatusBar {
    unsigned char* field_0;
    unsigned char* field_4;
    int win;
    int x;
    int y;
    int width;
    int height;
    int field_1C;
    int field_20;
    int field_24;
} StatusBar;

typedef struct TextInputRegion {
    int field_0;
    int field_4;
    int field_8;
    int field_C;
    int field_10;
    int field_14;
    int field_18;
    int field_1C;
    int field_20;
    int field_24;
    int field_28;
    int field_2C;
} TextInputRegion;

typedef struct TextRegion {
    int field_0;
    int field_4;
    int field_8;
    int field_C;
    int field_10;
    int field_14;
    int field_18;
    int field_1C;
    int field_20;
    int field_24;
} TextRegion;

extern int _updateRegions[32];
extern StatusBar _statusBar;
extern TextInputRegion* _textInputRegions;
extern int _numTextInputRegions;
extern TextRegion* _textRegions;
extern int _statusBarActive;
extern int _numTextRegions;

void _deleteChar(char* string, int pos, int length);
void _insertChar(char* string, char ch, int pos, int length);
void _showRegion(int a1);
int _update_widgets();
void _freeStatusBar();
void _initWidgets();
void _widgetsClose();
void _drawStatusBar();
void sub_4B5998(int win);

#endif /* WIDGET_H */
