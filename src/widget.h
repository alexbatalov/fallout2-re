#ifndef WIDGET_H
#define WIDGET_H

extern const float flt_50EB1C;
extern const float flt_50EB20;

extern int dword_66E6A0[32];

void sub_4B5A64(int a1);
int sub_4B5C24();
int widgetGetFont();
int widgetSetFont(int a1);
int widgetGetTextFlags();
int widgetSetTextFlags(int a1);
unsigned char widgetGetTextColor();
unsigned char widgetGetHighlightColor();
int widgetSetTextColor(float a1, float a2, float a3);
int widgetSetHighlightColor(float a1, float a2, float a3);

#endif /* WIDGET_H */
