#include "game/mainmenu.h"

#include <ctype.h>
#include <limits.h>
#include <string.h>

#include "game/art.h"
#include "plib/color/color.h"
#include "core.h"
#include "plib/gnw/grbuf.h"
#include "game/game.h"
#include "game/gsound.h"
#include "game/options.h"
#include "game/palette.h"
#include "plib/gnw/button.h"
#include "plib/gnw/text.h"
#include "game/version.h"

#define MAIN_MENU_WINDOW_WIDTH 640
#define MAIN_MENU_WINDOW_HEIGHT 480

typedef enum MainMenuButton {
    MAIN_MENU_BUTTON_INTRO,
    MAIN_MENU_BUTTON_NEW_GAME,
    MAIN_MENU_BUTTON_LOAD_GAME,
    MAIN_MENU_BUTTON_OPTIONS,
    MAIN_MENU_BUTTON_CREDITS,
    MAIN_MENU_BUTTON_EXIT,
    MAIN_MENU_BUTTON_COUNT,
} MainMenuButton;

static int main_menu_fatal_error();
static void main_menu_play_sound(const char* fileName);

// 0x5194F0
static int main_window = -1;

// 0x5194F4
static unsigned char* main_window_buf = NULL;

// 0x5194F8
static unsigned char* background_data = NULL;

// 0x5194FC
static unsigned char* button_up_data = NULL;

// 0x519500
static unsigned char* button_down_data = NULL;

// 0x519504
bool in_main_menu = false;

// 0x519508
static bool main_menu_created = false;

// 0x51950C
static unsigned int main_menu_timeout = 120000;

// 0x519510
static int button_values[MAIN_MENU_BUTTON_COUNT] = {
    KEY_LOWERCASE_I, // intro
    KEY_LOWERCASE_N, // new game
    KEY_LOWERCASE_L, // load game
    KEY_LOWERCASE_O, // options
    KEY_LOWERCASE_C, // credits
    KEY_LOWERCASE_E, // exit
};

// 0x519528
static int return_values[MAIN_MENU_BUTTON_COUNT] = {
    MAIN_MENU_INTRO,
    MAIN_MENU_NEW_GAME,
    MAIN_MENU_LOAD_GAME,
    MAIN_MENU_OPTIONS,
    MAIN_MENU_CREDITS,
    MAIN_MENU_EXIT,
};

// 0x614840
static int buttons[MAIN_MENU_BUTTON_COUNT];

// 0x614858
static bool main_menu_is_hidden;

// 0x61485C
static CacheEntry* button_up_key;

// 0x614860
static CacheEntry* button_down_key;

// 0x614864
static CacheEntry* background_key;

// 0x481650
int main_menu_create()
{
    int fid;
    MessageListItem msg;
    int len;

    if (main_menu_created) {
        return 0;
    }

    loadColorTable("color.pal");

    int mainMenuWindowX = 0;
    int mainMenuWindowY = 0;
    main_window = win_add(mainMenuWindowX,
        mainMenuWindowY,
        MAIN_MENU_WINDOW_WIDTH,
        MAIN_MENU_WINDOW_HEIGHT,
        0,
        WINDOW_HIDDEN | WINDOW_FLAG_0x04);
    if (main_window == -1) {
        // NOTE: Uninline.
        return main_menu_fatal_error();
    }

    main_window_buf = win_get_buf(main_window);

    // mainmenu.frm
    int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 140, 0, 0, 0);
    background_data = art_ptr_lock_data(backgroundFid, 0, 0, &background_key);
    if (background_data == NULL) {
        // NOTE: Uninline.
        return main_menu_fatal_error();
    }

    blitBufferToBuffer(background_data, 640, 480, 640, main_window_buf, 640);
    art_ptr_unlock(background_key);

    int oldFont = text_curr();
    text_font(100);

    // Copyright.
    msg.num = 20;
    if (message_search(&misc_message_file, &msg)) {
        win_print(main_window, msg.text, 0, 15, 460, colorTable[21091] | 0x6000000);
    }

    // Version.
    char version[VERSION_MAX];
    getverstr(version);
    len = text_width(version);
    win_print(main_window, version, 0, 615 - len, 460, colorTable[21091] | 0x6000000);

    // menuup.frm
    fid = art_id(OBJ_TYPE_INTERFACE, 299, 0, 0, 0);
    button_up_data = art_ptr_lock_data(fid, 0, 0, &button_up_key);
    if (button_up_data == NULL) {
        // NOTE: Uninline.
        return main_menu_fatal_error();
    }

    // menudown.frm
    fid = art_id(OBJ_TYPE_INTERFACE, 300, 0, 0, 0);
    button_down_data = art_ptr_lock_data(fid, 0, 0, &button_down_key);
    if (button_down_data == NULL) {
        // NOTE: Uninline.
        return main_menu_fatal_error();
    }

    for (int index = 0; index < MAIN_MENU_BUTTON_COUNT; index++) {
        buttons[index] = -1;
    }

    for (int index = 0; index < MAIN_MENU_BUTTON_COUNT; index++) {
        buttons[index] = win_register_button(main_window, 30, 19 + index * 42 - index, 26, 26, -1, -1, 1111, button_values[index], button_up_data, button_down_data, 0, 32);
        if (buttons[index] == -1) {
            // NOTE: Uninline.
            return main_menu_fatal_error();
        }

        win_register_button_mask(buttons[index], button_up_data);
    }

    text_font(104);

    for (int index = 0; index < MAIN_MENU_BUTTON_COUNT; index++) {
        msg.num = 9 + index;
        if (message_search(&misc_message_file, &msg)) {
            len = text_width(msg.text);
            text_to_buf(main_window_buf + 640 * (42 * index - index + 20) + 126 - (len / 2), msg.text, 640 - (126 - (len / 2)) - 1, 640, colorTable[21091]);
        }
    }

    text_font(oldFont);

    main_menu_created = true;
    main_menu_is_hidden = true;

    return 0;
}

// 0x481968
void main_menu_destroy()
{
    if (!main_menu_created) {
        return;
    }

    for (int index = 0; index < MAIN_MENU_BUTTON_COUNT; index++) {
        // FIXME: Why it tries to free only invalid buttons?
        if (buttons[index] == -1) {
            win_delete_button(buttons[index]);
        }
    }

    if (button_down_data) {
        art_ptr_unlock(button_down_key);
        button_down_key = NULL;
        button_down_data = NULL;
    }

    if (button_up_data) {
        art_ptr_unlock(button_up_key);
        button_up_key = NULL;
        button_up_data = NULL;
    }

    if (main_window != -1) {
        win_delete(main_window);
    }

    main_menu_created = false;
}

// 0x481A00
void main_menu_hide(bool animate)
{
    if (!main_menu_created) {
        return;
    }

    if (main_menu_is_hidden) {
        return;
    }

    soundContinueAll();

    if (animate) {
        palette_fade_to(black_palette);
        soundContinueAll();
    }

    win_hide(main_window);

    main_menu_is_hidden = true;
}

// 0x481A48
void main_menu_show(bool animate)
{
    if (!main_menu_created) {
        return;
    }

    if (!main_menu_is_hidden) {
        return;
    }

    win_show(main_window);

    if (animate) {
        loadColorTable("color.pal");
        palette_fade_to(cmap);
    }

    main_menu_is_hidden = false;
}

// NOTE: Unused.
//
// 0x481A8C
int main_menu_is_shown()
{
    return main_menu_created ? main_menu_is_hidden == 0 : 0;
}

// 0x481AA8
int main_menu_is_enabled()
{
    return 1;
}

// NOTE: Unused.
//
// 0x481AB0
void main_menu_set_timeout(unsigned int timeout)
{
    main_menu_timeout = 60000 * timeout;
}

// NOTE: Unused.
//
// 0x481AD0
unsigned int main_menu_get_timeout()
{
    return main_menu_timeout / 1000 / 60;
}

// 0x481AEC
int main_menu_loop()
{
    in_main_menu = true;

    bool oldCursorIsHidden = mouse_hidden();
    if (oldCursorIsHidden) {
        mouse_show();
    }

    unsigned int tick = _get_time();

    int rc = -1;
    while (rc == -1) {
        int keyCode = _get_input();

        for (int buttonIndex = 0; buttonIndex < MAIN_MENU_BUTTON_COUNT; buttonIndex++) {
            if (keyCode == button_values[buttonIndex] || keyCode == toupper(button_values[buttonIndex])) {
                // NOTE: Uninline.
                main_menu_play_sound("nmselec1");

                rc = return_values[buttonIndex];

                if (buttonIndex == MAIN_MENU_BUTTON_CREDITS && (keys[DIK_RSHIFT] != KEY_STATE_UP || keys[DIK_LSHIFT] != KEY_STATE_UP)) {
                    rc = MAIN_MENU_QUOTES;
                }

                break;
            }
        }

        if (rc == -1) {
            if (keyCode == KEY_CTRL_R) {
                rc = MAIN_MENU_SELFRUN;
                continue;
            } else if (keyCode == KEY_PLUS || keyCode == KEY_EQUAL) {
                IncGamma();
            } else if (keyCode == KEY_MINUS || keyCode == KEY_UNDERSCORE) {
                DecGamma();
            } else if (keyCode == KEY_UPPERCASE_D || keyCode == KEY_LOWERCASE_D) {
                rc = MAIN_MENU_SCREENSAVER;
                continue;
            } else if (keyCode == 1111) {
                if (!(mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_REPEAT)) {
                    // NOTE: Uninline.
                    main_menu_play_sound("nmselec0");
                }
                continue;
            }
        }

        if (keyCode == KEY_ESCAPE || game_user_wants_to_quit == 3) {
            rc = MAIN_MENU_EXIT;

            // NOTE: Uninline.
            main_menu_play_sound("nmselec1");
            break;
        } else if (game_user_wants_to_quit == 2) {
            game_user_wants_to_quit = 0;
        } else {
            if (getTicksSince(tick) >= main_menu_timeout) {
                rc = MAIN_MENU_TIMEOUT;
            }
        }
    }

    if (oldCursorIsHidden) {
        mouse_hide();
    }

    in_main_menu = false;

    return rc;
}

// NOTE: Inlined.
//
// 0x481C88
static int main_menu_fatal_error()
{
    main_menu_destroy();

    return -1;
}

// NOTE: Inlined.
//
// 0x481C94
static void main_menu_play_sound(const char* fileName)
{
    gsound_play_sfx_file(fileName);
}
