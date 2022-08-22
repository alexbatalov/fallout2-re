#ifndef WIDGET_H
#define WIDGET_H

#define WIDGET_UPDATE_REGIONS_CAPACITY 32

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

typedef void(UpdateRegionDrawFunc)(int a1);

typedef struct UpdateRegion {
    int field_0;
    int field_4;
    int field_8;
    int field_C;
    int field_10;
    int field_14;
    int field_18;
    UpdateRegionDrawFunc* drawFunc;
} UpdateRegion;

typedef void(TextInputRegionDeleteFunc)(int a1, void* userData);

typedef struct TextInputRegion {
    int textRegionId;
    int field_4;
    int field_8;
    int field_C;
    int field_10;
    int field_14;
    int field_18;
    int field_1C;
    int field_20;
    TextInputRegionDeleteFunc* deleteFunc;
    int field_28;
    void* deleteFuncUserData;
} TextInputRegion;

typedef struct TextRegion {
    int win;
    int isUsed;
    int x;
    int y;
    int width;
    int height;
    int textAlignment;
    int textFlags;
    int backgroundColor;
    int font;
} TextRegion;

extern UpdateRegion* _updateRegions[WIDGET_UPDATE_REGIONS_CAPACITY];
extern StatusBar _statusBar;
extern TextInputRegion* _textInputRegions;
extern int _numTextInputRegions;
extern TextRegion* _textRegions;
extern int _statusBarActive;
extern int _numTextRegions;

void _deleteChar(char* string, int pos, int length);
void _insertChar(char* string, char ch, int pos, int length);
int _win_delete_all_text_input_regions(int win);
int _win_delete_text_input_region(int textInputRegionId);
int _win_set_text_input_delete_func(int textInputRegionId, TextInputRegionDeleteFunc* deleteFunc, void* userData);
int _win_add_text_region(int win, int x, int y, int width, int font, int textAlignment, int textFlags, int backgroundColor);
int _win_print_text_region(int textRegionId, char* string);
int _win_print_substr_region(int textRegionId, char* string, int stringLength);
int _win_update_text_region(int textRegionId);
int _win_delete_text_region(int textRegionId);
int _win_delete_all_update_regions(int a1);
int _win_text_region_style(int textRegionId, int font, int textAlignment, int textFlags, int backgroundColor);
int _widgetDoInput();
int _win_center_str(int win, char* string, int y, int a4);
void _showRegion(UpdateRegion* updateRegion);
int _draw_widgets();
int _update_widgets();
int _win_register_update(int a1, int a2, int a3, int a4, UpdateRegionDrawFunc* drawFunc, int a6, int a7, int a8);
int _win_delete_update_region(int updateRegionIndex);
void _win_do_updateregions();
void _freeStatusBar();
void _initWidgets();
void _widgetsClose();
void _drawStatusBar();
void _real_win_set_status_bar(int a1, int a2, int a3);
void _real_win_update_status_bar(float a1, float a2);
void _real_win_increment_status_bar(float a1);
void _real_win_add_status_bar(int win, int a2, char* a3, char* a4, int x, int y);
void _real_win_get_status_info(int a1, int* a2, int* a3, int* a4);
void _real_win_modify_status_info(int a1, int a2, int a3, int a4);
void sub_4B5998(int win);

#endif /* WIDGET_H */
