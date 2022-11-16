#include "plib/gnw/text.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "plib/color/color.h"
#include "db.h"
#include "plib/gnw/memory.h"

// The maximum number of text fonts.
#define TEXT_FONT_MAX 10

// The maximum number of font managers.
#define FONT_MANAGER_MAX 10

static int load_font(int n);
static void GNW_text_font(int font_num);
static bool text_font_exists(int font_num, FontMgrPtr* mgr);
static void GNW_text_to_buf(unsigned char* buf, const char* str, int swidth, int fullw, int color);
static int GNW_text_height();
static int GNW_text_width(const char* str);
static int GNW_text_char_width(int c);
static int GNW_text_mono_width(const char* str);
static int GNW_text_spacing();
static int GNW_text_size(const char* str);
static int GNW_text_max();

// 0x51E3B0
static int curr_font_num = -1;

// 0x51E3B4
static int total_managers = 0;

// 0x51E3B8
text_to_buf_func* text_to_buf = NULL;

// 0x51E3BC
text_height_func* text_height = NULL;

// 0x51E3C0
text_width_func* text_width = NULL;

// 0x51E3C4
text_char_width_func* text_char_width = NULL;

// 0x51E3C8
text_mono_width_func* text_mono_width = NULL;

// 0x51E3CC
text_spacing_func* text_spacing = NULL;

// 0x51E3D0
text_size_func* text_size = NULL;

// 0x51E3D4
text_max_func* text_max = NULL;

// 0x6ADB08
static Font font[TEXT_FONT_MAX];

// 0x6ADBD0
static FontMgr font_managers[FONT_MANAGER_MAX];

// 0x6ADD88
static Font* curr_font;

// 0x4D555C
int GNW_text_init()
{
    // 0x4D5530
    static FontMgr GNW_font_mgr = {
        0,
        9,
        GNW_text_font,
        GNW_text_to_buf,
        GNW_text_height,
        GNW_text_width,
        GNW_text_char_width,
        GNW_text_mono_width,
        GNW_text_spacing,
        GNW_text_size,
        GNW_text_max,
    };

    int i;
    int first_font;

    first_font = -1;

    for (i = 0; i < TEXT_FONT_MAX; i++) {
        if (load_font(i) == -1) {
            font[i].num = 0;
        } else {
            if (first_font == -1) {
                first_font = i;
            }
        }
    }

    if (first_font == -1) {
        return -1;
    }

    if (text_add_manager(&GNW_font_mgr) == -1) {
        return -1;
    }

    text_font(first_font);

    return 0;
}

// 0x4D55CC
void GNW_text_exit()
{
    int i;

    for (i = 0; i < TEXT_FONT_MAX; i++) {
        if (font[i].num != 0) {
            mem_free(font[i].info);
            mem_free(font[i].data);
        }
    }
}

// 0x4D55FC
static int load_font(int n)
{
    int rc = -1;

    char path[MAX_PATH];
    sprintf(path, "font%d.fon", n);

    // NOTE: Original code is slightly different. It uses deep nesting and
    // unwinds everything from the point of failure.
    Font* textFontDescriptor = &(font[n]);
    textFontDescriptor->data = NULL;
    textFontDescriptor->info = NULL;

    File* stream = fileOpen(path, "rb");
    int dataSize;
    if (stream == NULL) {
        goto out;
    }

    if (fileRead(textFontDescriptor, sizeof(Font), 1, stream) != 1) {
        goto out;
    }

    textFontDescriptor->info = (FontInfo*)mem_malloc(textFontDescriptor->num * sizeof(FontInfo));
    if (textFontDescriptor->info == NULL) {
        goto out;
    }

    if (fileRead(textFontDescriptor->info, sizeof(FontInfo), textFontDescriptor->num, stream) != textFontDescriptor->num) {
        goto out;
    }

    dataSize = textFontDescriptor->height * ((textFontDescriptor->info[textFontDescriptor->num - 1].width + 7) >> 3) + textFontDescriptor->info[textFontDescriptor->num - 1].offset;
    textFontDescriptor->data = (unsigned char*)mem_malloc(dataSize);
    if (textFontDescriptor->data == NULL) {
        goto out;
    }

    if (fileRead(textFontDescriptor->data, 1, dataSize, stream) != dataSize) {
        goto out;
    }

    rc = 0;

out:

    if (rc != 0) {
        if (textFontDescriptor->data != NULL) {
            mem_free(textFontDescriptor->data);
            textFontDescriptor->data = NULL;
        }

        if (textFontDescriptor->info != NULL) {
            mem_free(textFontDescriptor->info);
            textFontDescriptor->info = NULL;
        }
    }

    if (stream != NULL) {
        fileClose(stream);
    }

    return rc;
}

// 0x4D5780
int text_add_manager(FontMgrPtr mgr)
{
    int k;

    if (mgr == NULL) {
        return -1;
    }

    if (total_managers >= FONT_MANAGER_MAX) {
        return -1;
    }

    // Check if a font manager exists for any font in the specified range.
    for (k = mgr->low_font_num; k < mgr->high_font_num; k++) {
        FontMgrPtr mgr;
        if (text_font_exists(k, &mgr)) {
            return -1;
        }
    }

    memcpy(&(font_managers[total_managers]), mgr, sizeof(*mgr));
    total_managers++;

    return 0;
}

// 0x4D58AC
static void GNW_text_font(int font_num)
{
    if (font_num >= TEXT_FONT_MAX) {
        return;
    }

    if (font[font_num].num == 0) {
        return;
    }

    curr_font = &(font[font_num]);
}

// 0x4D58D4
int text_curr()
{
    return curr_font_num;
}

// 0x4D58DC
void text_font(int font_num)
{
    FontMgrPtr mgr;

    if (text_font_exists(font_num, &mgr)) {
        text_to_buf = mgr->text_to_buf;
        text_height = mgr->text_height;
        text_width = mgr->text_width;
        text_char_width = mgr->text_char_width;
        text_mono_width = mgr->text_mono_width;
        text_spacing = mgr->text_spacing;
        text_size = mgr->text_size;
        text_max = mgr->text_max;

        curr_font_num = font_num;

        mgr->text_font(font_num);
    }
}

// 0x4D595C
static bool text_font_exists(int font_num, FontMgrPtr* mgr)
{
    int k;

    for (k = 0; k < total_managers; k++) {
        if (font_num >= font_managers[k].low_font_num && font_num <= font_managers[k].high_font_num) {
            *mgr = &(font_managers[k]);
            return true;
        }
    }

    return false;
}

// 0x4D59B0
void GNW_text_to_buf(unsigned char* buf, const char* str, int swidth, int fullw, int color)
{
    if ((color & FONT_SHADOW) != 0) {
        color &= ~FONT_SHADOW;
        text_to_buf(buf + fullw + 1, str, swidth, fullw, colorTable[0]);
    }

    int monospacedCharacterWidth;
    if ((color & FONT_MONO) != 0) {
        monospacedCharacterWidth = text_max();
    }

    unsigned char* ptr = buf;
    while (*str != '\0') {
        char ch = *str++;
        if (ch < curr_font->num) {
            FontInfo* glyph = &(curr_font->info[ch & 0xFF]);

            unsigned char* end;
            if ((color & FONT_MONO) != 0) {
                end = ptr + monospacedCharacterWidth;
                ptr += (monospacedCharacterWidth - curr_font->spacing - glyph->width) / 2;
            } else {
                end = ptr + glyph->width + curr_font->spacing;
            }

            if (end - buf > swidth) {
                break;
            }

            unsigned char* glyphData = curr_font->data + glyph->offset;
            for (int y = 0; y < curr_font->height; y++) {
                int bits = 0x80;
                for (int x = 0; x < glyph->width; x++) {
                    if (bits == 0) {
                        bits = 0x80;
                        glyphData++;
                    }

                    if ((*glyphData & bits) != 0) {
                        *ptr = color & 0xFF;
                    }

                    bits >>= 1;
                    ptr++;
                }
                glyphData++;
                ptr += fullw - glyph->width;
            }

            ptr = end;
        }
    }

    if ((color & FONT_UNDERLINE) != 0) {
        // TODO: Probably additional -1 present, check.
        int length = ptr - buf;
        unsigned char* underlinePtr = buf + fullw * (curr_font->height - 1);
        for (int pix = 0; pix < length; pix++) {
            *underlinePtr++ = color & 0xFF;
        }
    }
}

// 0x4D5B54
static int GNW_text_height()
{
    return curr_font->height;
}

// 0x4D5B60
static int GNW_text_width(const char* str)
{
    int i;
    int len;
    FontInfo* fi;

    len = 0;

    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] < curr_font->num) {
            fi = &(curr_font->info[str[i]]);
            len += curr_font->spacing + fi->width;
        }
    }

    return len;
}

// 0x4D5BA4
static int GNW_text_char_width(int c)
{
    return curr_font->info[c].width;
}

// 0x4D5BB8
static int GNW_text_mono_width(const char* str)
{
    return text_max() * strlen(str);
}

// 0x4D5BD8
static int GNW_text_spacing()
{
    return curr_font->spacing;
}

// 0x4D5BE4
static int GNW_text_size(const char* str)
{
    return text_width(str) * text_height();
}

// 0x4D5BF8
static int GNW_text_max()
{
    int i;
    int len;
    FontInfo* fi;

    len = 0;

    for (i = 0; i < curr_font->num; i++) {
        fi = &(curr_font->info[i]);
        if (len < fi->width) {
            len = fi->width;
        }
    }

    return len + curr_font->spacing;
}
