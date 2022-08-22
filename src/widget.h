#ifndef WIDGET_H
#define WIDGET_H

extern const float flt_50EB1C;
extern const float flt_50EB20;

extern int _updateRegions[32];

void _showRegion(int a1);
int _update_widgets();
int windowGetFont();
int windowSetFont(int a1);
int windowGetTextFlags();
int windowSetTextFlags(int a1);
unsigned char windowGetTextColor();
unsigned char windowGetHighlightColor();
int windowSetTextColor(float a1, float a2, float a3);
int windowSetHighlightColor(float a1, float a2, float a3);
void sub_4B5998(int win);

#endif /* WIDGET_H */
