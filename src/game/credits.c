#include "game/credits.h"

#include <string.h>

#include "game/art.h"
#include "color.h"
#include "core.h"
#include "game/cycle.h"
#include "db.h"
#include "debug.h"
#include "draw.h"
#include "game/gmouse.h"
#include "memory.h"
#include "game/message.h"
#include "game/palette.h"
#include "sound.h"
#include "text_font.h"
#include "window_manager.h"

#define CREDITS_WINDOW_WIDTH 640
#define CREDITS_WINDOW_HEIGHT 480
#define CREDITS_WINDOW_SCROLLING_DELAY 38

static bool credits_get_next_line(char* dest, int* font, int* color);

// 0x56D740
static File* credits_file;

// 0x56D744
static int name_color;

// 0x56D748
static int title_font;

// 0x56D74C
static int name_font;

// 0x56D750
static int title_color;

// 0x42C860
void credits(const char* filePath, int backgroundFid, bool useReversedStyle)
{
    int oldFont = fontGetCurrent();

    loadColorTable("color.pal");

    if (useReversedStyle) {
        title_color = colorTable[18917];
        name_font = 103;
        title_font = 104;
        name_color = colorTable[13673];
    } else {
        title_color = colorTable[13673];
        name_font = 104;
        title_font = 103;
        name_color = colorTable[18917];
    }

    soundContinueAll();

    char localizedPath[MAX_PATH];
    if (message_make_path(localizedPath, filePath)) {
        credits_file = fileOpen(localizedPath, "rt");
        if (credits_file != NULL) {
            soundContinueAll();

            cycle_disable();
            gmouse_set_cursor(MOUSE_CURSOR_NONE);

            bool cursorWasHidden = mouse_hidden();
            if (cursorWasHidden) {
                mouse_show();
            }

            int creditsWindowX = 0;
            int creditsWindowY = 0;
            int window = windowCreate(creditsWindowX, creditsWindowY, CREDITS_WINDOW_WIDTH, CREDITS_WINDOW_HEIGHT, colorTable[0], 20);
            soundContinueAll();
            if (window != -1) {
                unsigned char* windowBuffer = windowGetBuffer(window);
                if (windowBuffer != NULL) {
                    unsigned char* backgroundBuffer = (unsigned char*)internal_malloc(CREDITS_WINDOW_WIDTH * CREDITS_WINDOW_HEIGHT);
                    if (backgroundBuffer) {
                        soundContinueAll();

                        memset(backgroundBuffer, colorTable[0], CREDITS_WINDOW_WIDTH * CREDITS_WINDOW_HEIGHT);

                        if (backgroundFid != -1) {
                            CacheEntry* backgroundFrmHandle;
                            Art* frm = art_ptr_lock(backgroundFid, &backgroundFrmHandle);
                            if (frm != NULL) {
                                int width = art_frame_width(frm, 0, 0);
                                int height = art_frame_length(frm, 0, 0);
                                unsigned char* backgroundFrmData = art_frame_data(frm, 0, 0);
                                blitBufferToBuffer(backgroundFrmData,
                                    width,
                                    height,
                                    width,
                                    backgroundBuffer + CREDITS_WINDOW_WIDTH * ((CREDITS_WINDOW_HEIGHT - height) / 2) + (CREDITS_WINDOW_WIDTH - width) / 2,
                                    CREDITS_WINDOW_WIDTH);
                                art_ptr_unlock(backgroundFrmHandle);
                            }
                        }

                        unsigned char* intermediateBuffer = (unsigned char*)internal_malloc(CREDITS_WINDOW_WIDTH * CREDITS_WINDOW_HEIGHT);
                        if (intermediateBuffer != NULL) {
                            memset(intermediateBuffer, 0, CREDITS_WINDOW_WIDTH * CREDITS_WINDOW_HEIGHT);

                            fontSetCurrent(title_font);
                            int titleFontLineHeight = fontGetLineHeight();

                            fontSetCurrent(name_font);
                            int nameFontLineHeight = fontGetLineHeight();

                            int lineHeight = nameFontLineHeight + (titleFontLineHeight >= nameFontLineHeight ? titleFontLineHeight - nameFontLineHeight : 0);
                            int stringBufferSize = CREDITS_WINDOW_WIDTH * lineHeight;
                            unsigned char* stringBuffer = (unsigned char*)internal_malloc(stringBufferSize);
                            if (stringBuffer != NULL) {
                                blitBufferToBuffer(backgroundBuffer,
                                    CREDITS_WINDOW_WIDTH,
                                    CREDITS_WINDOW_HEIGHT,
                                    CREDITS_WINDOW_WIDTH,
                                    windowBuffer,
                                    CREDITS_WINDOW_WIDTH);

                                win_draw(window);

                                palette_fade_to(cmap);

                                unsigned char* v40 = intermediateBuffer + CREDITS_WINDOW_WIDTH * CREDITS_WINDOW_HEIGHT - CREDITS_WINDOW_WIDTH;
                                char str[260];
                                int font;
                                int color;
                                unsigned int tick = 0;
                                bool stop = false;
                                while (credits_get_next_line(str, &font, &color)) {
                                    fontSetCurrent(font);

                                    int v19 = fontGetStringWidth(str);
                                    if (v19 >= CREDITS_WINDOW_WIDTH) {
                                        continue;
                                    }

                                    memset(stringBuffer, 0, stringBufferSize);
                                    fontDrawText(stringBuffer, str, CREDITS_WINDOW_WIDTH, CREDITS_WINDOW_WIDTH, color);

                                    unsigned char* dest = intermediateBuffer + CREDITS_WINDOW_WIDTH * CREDITS_WINDOW_HEIGHT - CREDITS_WINDOW_WIDTH + (CREDITS_WINDOW_WIDTH - v19) / 2;
                                    unsigned char* src = stringBuffer;
                                    for (int index = 0; index < lineHeight; index++) {
                                        if (_get_input() != -1) {
                                            stop = true;
                                            break;
                                        }

                                        memmove(intermediateBuffer, intermediateBuffer + CREDITS_WINDOW_WIDTH, CREDITS_WINDOW_WIDTH * CREDITS_WINDOW_HEIGHT - CREDITS_WINDOW_WIDTH);
                                        memcpy(dest, src, v19);

                                        blitBufferToBuffer(backgroundBuffer,
                                            CREDITS_WINDOW_WIDTH,
                                            CREDITS_WINDOW_HEIGHT,
                                            CREDITS_WINDOW_WIDTH,
                                            windowBuffer,
                                            CREDITS_WINDOW_WIDTH);

                                        blitBufferToBufferTrans(intermediateBuffer,
                                            CREDITS_WINDOW_WIDTH,
                                            CREDITS_WINDOW_HEIGHT,
                                            CREDITS_WINDOW_WIDTH,
                                            windowBuffer,
                                            CREDITS_WINDOW_WIDTH);

                                        while (getTicksSince(tick) < CREDITS_WINDOW_SCROLLING_DELAY) {
                                        }

                                        tick = _get_time();

                                        win_draw(window);

                                        src += CREDITS_WINDOW_WIDTH;
                                    }

                                    if (stop) {
                                        break;
                                    }
                                }

                                if (!stop) {
                                    for (int index = 0; index < CREDITS_WINDOW_HEIGHT; index++) {
                                        if (_get_input() != -1) {
                                            break;
                                        }

                                        memmove(intermediateBuffer, intermediateBuffer + CREDITS_WINDOW_WIDTH, CREDITS_WINDOW_WIDTH * CREDITS_WINDOW_HEIGHT - CREDITS_WINDOW_WIDTH);
                                        memset(intermediateBuffer + CREDITS_WINDOW_WIDTH * CREDITS_WINDOW_HEIGHT - CREDITS_WINDOW_WIDTH, 0, CREDITS_WINDOW_WIDTH);

                                        blitBufferToBuffer(backgroundBuffer,
                                            CREDITS_WINDOW_WIDTH,
                                            CREDITS_WINDOW_HEIGHT,
                                            CREDITS_WINDOW_WIDTH,
                                            windowBuffer,
                                            CREDITS_WINDOW_WIDTH);

                                        blitBufferToBufferTrans(intermediateBuffer,
                                            CREDITS_WINDOW_WIDTH,
                                            CREDITS_WINDOW_HEIGHT,
                                            CREDITS_WINDOW_WIDTH,
                                            windowBuffer,
                                            CREDITS_WINDOW_WIDTH);

                                        while (getTicksSince(tick) < CREDITS_WINDOW_SCROLLING_DELAY) {
                                        }

                                        tick = _get_time();

                                        win_draw(window);
                                    }
                                }

                                internal_free(stringBuffer);
                            }
                            internal_free(intermediateBuffer);
                        }
                        internal_free(backgroundBuffer);
                    }
                }

                soundContinueAll();
                palette_fade_to(black_palette);
                soundContinueAll();
                windowDestroy(window);
            }

            if (cursorWasHidden) {
                mouse_hide();
            }

            gmouse_set_cursor(MOUSE_CURSOR_ARROW);
            cycle_enable();
            fileClose(credits_file);
        }
    }

    fontSetCurrent(oldFont);
}

// 0x42CE6C
static bool credits_get_next_line(char* dest, int* font, int* color)
{
    char string[256];
    while (fileReadString(string, 256, credits_file)) {
        char* pch;
        if (string[0] == ';') {
            continue;
        } else if (string[0] == '@') {
            *font = title_font;
            *color = title_color;
            pch = string + 1;
        } else if (string[0] == '#') {
            *font = name_font;
            *color = colorTable[17969];
            pch = string + 1;
        } else {
            *font = name_font;
            *color = name_color;
            pch = string;
        }

        strcpy(dest, pch);

        return true;
    }

    return false;
}
