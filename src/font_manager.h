#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include "text_font.h"

#include <stdbool.h>

// The maximum number of interface fonts.
#define INTERFACE_FONT_MAX (16)

typedef struct InterfaceFontGlyph {
    short width;
    short height;
    int offset;
} InterfaceFontGlyph;

typedef struct InterfaceFontDescriptor {
    short maxHeight;
    short letterSpacing;
    short wordSpacing;
    short lineSpacing;
    short field_8;
    short field_A;
    InterfaceFontGlyph glyphs[256];
    unsigned char* data;
} InterfaceFontDescriptor;

extern bool gInterfaceFontsInitialized;
extern int gInterfaceFontsLength;
extern FontManager gModernFontManager;

extern InterfaceFontDescriptor gInterfaceFontDescriptors[INTERFACE_FONT_MAX];
extern int gCurrentInterfaceFont;
extern InterfaceFontDescriptor* gCurrentInterfaceFontDescriptor;

int interfaceFontsInit();
void interfaceFontsExit();
int interfaceFontLoad(int font);
void interfaceFontSetCurrentImpl(int font);
int interfaceFontGetLineHeightImpl();
int interfaceFontGetStringWidthImpl(const char* string);
int interfaceFontGetCharacterWidthImpl(int ch);
int interfaceFontGetMonospacedStringWidthImpl(const char* string);
int interfaceFontGetLetterSpacingImpl();
int interfaceFontGetBufferSizeImpl(const char* string);
int interfaceFontGetMonospacedCharacterWidthImpl();
int FMtext_curr();
void interfaceFontDrawImpl(unsigned char* buf, const char* string, int length, int pitch, int color);
void interfaceFontByteSwapUInt32(unsigned int* value);
void interfaceFontByteSwapInt32(int* value);
void interfaceFontByteSwapUInt16(unsigned short* value);
void interfaceFontByteSwapInt16(short* value);

#endif /* FONT_MANAGER_H */
