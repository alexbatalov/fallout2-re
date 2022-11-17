#include "game/pipboy.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "game/automap.h"
#include "plib/color/color.h"
#include "game/combat.h"
#include "game/config.h"
#include "plib/gnw/input.h"
#include "game/critter.h"
#include "game/cycle.h"
#include "game/bmpdlog.h"
#include "plib/gnw/button.h"
#include "plib/gnw/debug.h"
#include "plib/gnw/grbuf.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/gmouse.h"
#include "game/gmovie.h"
#include "game/gsound.h"
#include "game/intface.h"
#include "game/map.h"
#include "plib/gnw/memory.h"
#include "game/object.h"
#include "game/queue.h"
#include "game/roll.h"
#include "game/scripts.h"
#include "game/stat.h"
#include "plib/gnw/text.h"
#include "plib/gnw/gnw.h"
#include "game/wordwrap.h"
#include "game/worldmap.h"

#define PIPBOY_RAND_MAX 32767

#define PIPBOY_WINDOW_WIDTH 640
#define PIPBOY_WINDOW_HEIGHT 480

#define PIPBOY_WINDOW_DAY_X 20
#define PIPBOY_WINDOW_DAY_Y 17

#define PIPBOY_WINDOW_MONTH_X 46
#define PIPBOY_WINDOW_MONTH_Y 18

#define PIPBOY_WINDOW_YEAR_X 83
#define PIPBOY_WINDOW_YEAR_Y 17

#define PIPBOY_WINDOW_TIME_X 155
#define PIPBOY_WINDOW_TIME_Y 17

#define PIPBOY_HOLODISK_LINES_MAX 35

#define PIPBOY_WINDOW_CONTENT_VIEW_X 254
#define PIPBOY_WINDOW_CONTENT_VIEW_Y 46
#define PIPBOY_WINDOW_CONTENT_VIEW_WIDTH 374
#define PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT 410

#define PIPBOY_IDLE_TIMEOUT 120000

#define PIPBOY_BOMB_COUNT 16

#define BACK_BUTTON_INDEX 22

typedef enum Holiday {
    HOLIDAY_NEW_YEAR,
    HOLIDAY_VALENTINES_DAY,
    HOLIDAY_FOOLS_DAY,
    HOLIDAY_SHIPPING_DAY,
    HOLIDAY_INDEPENDENCE_DAY,
    HOLIDAY_HALLOWEEN,
    HOLIDAY_THANKSGIVING_DAY,
    HOLIDAY_CRISTMAS,
    HOLIDAY_COUNT,
} Holiday;

// Options used to render Pipboy texts.
typedef enum PipboyTextOptions {
    // Specifies that text should be rendered in the center of the Pipboy
    // monitor.
    //
    // This option is mutually exclusive with other alignment options.
    PIPBOY_TEXT_ALIGNMENT_CENTER = 0x02,

    // Specifies that text should be rendered in the beginning of the right
    // column in two-column layout.
    //
    // This option is mutually exclusive with other alignment options.
    PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN = 0x04,

    // Specifies that text should be rendered in the center of the left column.
    //
    // This option is mutually exclusive with other alignment options.
    PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER = 0x10,

    // Specifies that text should be rendered in the center of the right
    // column.
    //
    // This option is mutually exclusive with other alignment options.
    PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN_CENTER = 0x20,

    // Specifies that text should rendered with underline.
    PIPBOY_TEXT_STYLE_UNDERLINE = 0x08,

    // Specifies that text should rendered with strike-through line.
    PIPBOY_TEXT_STYLE_STRIKE_THROUGH = 0x40,

    // Specifies that text should be rendered with no (minimal) indentation.
    PIPBOY_TEXT_NO_INDENT = 0x80,
} PipboyTextOptions;

typedef enum PipboyRestDuration {
    PIPBOY_REST_DURATION_TEN_MINUTES,
    PIPBOY_REST_DURATION_THIRTY_MINUTES,
    PIPBOY_REST_DURATION_ONE_HOUR,
    PIPBOY_REST_DURATION_TWO_HOURS,
    PIPBOY_REST_DURATION_THREE_HOURS,
    PIPBOY_REST_DURATION_FOUR_HOURS,
    PIPBOY_REST_DURATION_FIVE_HOURS,
    PIPBOY_REST_DURATION_SIX_HOURS,
    PIPBOY_REST_DURATION_UNTIL_MORNING,
    PIPBOY_REST_DURATION_UNTIL_NOON,
    PIPBOY_REST_DURATION_UNTIL_EVENING,
    PIPBOY_REST_DURATION_UNTIL_MIDNIGHT,
    PIPBOY_REST_DURATION_UNTIL_HEALED,
    PIPBOY_REST_DURATION_UNTIL_PARTY_HEALED,
    PIPBOY_REST_DURATION_COUNT,
    PIPBOY_REST_DURATION_COUNT_WITHOUT_PARTY = PIPBOY_REST_DURATION_COUNT - 1,
} PipboyRestDuration;

typedef enum PipboyFrm {
    PIPBOY_FRM_LITTLE_RED_BUTTON_UP,
    PIPBOY_FRM_LITTLE_RED_BUTTON_DOWN,
    PIPBOY_FRM_NUMBERS,
    PIPBOY_FRM_BACKGROUND,
    PIPBOY_FRM_NOTE,
    PIPBOY_FRM_MONTHS,
    PIPBOY_FRM_NOTE_NUMBERS,
    PIPBOY_FRM_ALARM_DOWN,
    PIPBOY_FRM_ALARM_UP,
    PIPBOY_FRM_LOGO,
    PIPBOY_FRM_BOMB,
    PIPBOY_FRM_COUNT,
} PipboyFrm;

// Provides metadata information on quests.
//
// Loaded from `data/quest.txt`.
typedef struct QuestDescription {
    int location;
    int description;
    int gvar;
    int displayThreshold;
    int completedThreshold;
} QuestDescription;

// Provides metadata information on holodisks.
//
// Loaded from `data/holodisk.txt`.
typedef struct HolodiskDescription {
    int gvar;
    int name;
    int description;
} HolodiskDescription;

typedef struct HolidayDescription {
    short month;
    short day;
    short textId;
} HolidayDescription;

typedef struct STRUCT_664350 {
    char* name;
    short field_4;
    short field_6;
} STRUCT_664350;

typedef struct PipboyBomb {
    int x;
    int y;
    float field_8;
    float field_C;
    unsigned char field_10;
} PipboyBomb;

static int StartPipboy(int intent);
static void EndPipboy();
static void pip_num(int value, int digits, int x, int y);
static void pip_date();
static void pip_print(const char* text, int a2, int a3);
static void pip_back(int a1);
static void PipStatus(int a1);
static void ListStatLines(int a1);
static void ShowHoloDisk();
static int ListHoloDiskTitles(int a1);
static int qscmp(const void* a1, const void* a2);
static void PipAutomaps(int a1);
static int PrintAMelevList(int a1);
static int PrintAMList(int a1);
static void PipArchives(int a1);
static int ListArchive(int a1);
static void PipAlarm(int a1);
static void DrawAlarmText(int a1);
static void DrawAlrmHitPnts();
static void AddHotLines(int a1, int a2, bool add_back_button);
static void NixHotLines();
static bool TimedRest(int hours, int minutes, int kind);
static bool Check4Health(int a1);
static bool AddHealth();
static void ClacTime(int* hours, int* minutes, int wakeUpHour);
static int ScreenSaver();
static int quest_init();
static void quest_exit();
static int quest_qsort_compare(const void* a1, const void* a2);
static int holodisks_init();
static void holodisks_exit();

// 0x496FC0
static const Rect pip_rect = {
    PIPBOY_WINDOW_CONTENT_VIEW_X,
    PIPBOY_WINDOW_CONTENT_VIEW_Y,
    PIPBOY_WINDOW_CONTENT_VIEW_X + PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
    PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
};

// 0x496FD0
static const int pipgrphs[PIPBOY_FRM_COUNT] = {
    8,
    9,
    82,
    127,
    128,
    129,
    130,
    131,
    132,
    133,
    226,
};

// 0x51C128
static QuestDescription* quests = NULL;

// 0x51C12C
static int quest_count = 0;

// 0x51C130
static HolodiskDescription* holodisks = NULL;

// 0x51C134
static int holodisks_count = 0;

// Number of rest options available.
//
// 0x51C138
static int currentAlarmTypeCount = PIPBOY_REST_DURATION_COUNT;

// 0x51C13C
static bool bk_enable = false;

// 0x51C140
static HolidayDescription SpclDate[HOLIDAY_COUNT] = {
    { 1, 1, 100 },
    { 2, 14, 101 },
    { 4, 1, 102 },
    { 7, 4, 104 },
    { 10, 6, 103 },
    { 10, 31, 105 },
    { 11, 28, 106 },
    { 12, 25, 107 },
};

// 0x51C170
PipboyRenderProc* PipFnctn[5] = {
    PipStatus,
    PipAutomaps,
    PipArchives,
    PipAlarm,
    PipAlarm,
};

// 0x6642E0
static Size ginfo[PIPBOY_FRM_COUNT];

// 0x664338
static MessageListItem pipmesg;

// pipboy.msg
//
// 0x664348
static MessageList pipboy_message_file;

// 0x664350
static STRUCT_664350 sortlist[24];

// quests.msg
//
// 0x664410
static MessageList quest_message_file;

// 0x664418
static int statcount;

// 0x66441C
static unsigned char* scrn_buf;

// 0x664420
static unsigned char* pipbmp[PIPBOY_FRM_COUNT];

// 0x66444C
static int holocount;

// 0x664450
static int mouse_y;

// 0x664454
static int mouse_x;

// 0x664458
static unsigned int wait_time;

// Index of the last page when rendering holodisk content.
//
// 0x66445C
static int holopages;

// 0x664460
static int HotLines[23];

// 0x6644BC
static int old_mouse_x;

// 0x6644C0
static int old_mouse_y;

// 0x6644C4
static int pip_win;

// 0x6644C8
static CacheEntry* grphkey[PIPBOY_FRM_COUNT];

// 0x6644F4
static int holodisk;

// 0x6644F8
static int hot_line_count;

// 0x6644FC
static int savefont;

// 0x664500
static bool proc_bail_flag;

// 0x664504
static int amlst_mode;

// 0x664508
static int crnt_func;

// 0x66450C
static int actcnt;

// 0x664510
static int hot_line_start;

// 0x664514
static int cursor_line;

// 0x664518
static int rest_time;

// 0x66451C
static int amcty_indx;

// 0x664520
static int view_page;

// 0x664524
static int bottom_line;

// 0x664528
static unsigned char hot_back_line;

// 0x664529
static unsigned char holo_flag;

// 0x66452A
static unsigned char stat_flag;

// 0x497004
int pipboy(int intent)
{
    if (!wmMapPipboyActive()) {
        // You aren't wearing the pipboy!
        const char* text = getmsg(&misc_message_file, &pipmesg, 7000);
        dialog_out(text, NULL, 0, 192, 135, colorTable[32328], NULL, colorTable[32328], 1);
        return 0;
    }

    intent = StartPipboy(intent);
    if (intent == -1) {
        return -1;
    }

    mouse_get_position(&old_mouse_x, &old_mouse_y);
    wait_time = get_time();

    while (true) {
        int keyCode = get_input();

        if (intent == PIPBOY_OPEN_INTENT_REST) {
            keyCode = 504;
            intent = PIPBOY_OPEN_INTENT_UNSPECIFIED;
        }

        mouse_get_position(&mouse_x, &mouse_y);

        if (keyCode != -1 || mouse_x != old_mouse_x || mouse_y != old_mouse_y) {
            wait_time = get_time();
            old_mouse_x = mouse_x;
            old_mouse_y = mouse_y;
        } else {
            if (get_time() - wait_time > PIPBOY_IDLE_TIMEOUT) {
                ScreenSaver();

                wait_time = get_time();
                mouse_get_position(&old_mouse_x, &old_mouse_y);
            }
        }

        if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
            game_quit_with_confirm();
            break;
        }

        if (keyCode == 503 || keyCode == KEY_ESCAPE || keyCode == KEY_RETURN || keyCode == KEY_UPPERCASE_P || keyCode == KEY_LOWERCASE_P || game_user_wants_to_quit != 0) {
            break;
        }

        if (keyCode == KEY_F12) {
            dump_screen();
        } else if (keyCode >= 500 && keyCode <= 504) {
            crnt_func = keyCode - 500;
            PipFnctn[crnt_func](1024);
        } else if (keyCode >= 505 && keyCode <= 527) {
            PipFnctn[crnt_func](keyCode - 506);
        } else if (keyCode == 528) {
            PipFnctn[crnt_func](1025);
        } else if (keyCode == KEY_PAGE_DOWN) {
            PipFnctn[crnt_func](1026);
        } else if (keyCode == KEY_PAGE_UP) {
            PipFnctn[crnt_func](1027);
        }

        if (proc_bail_flag) {
            break;
        }
    }

    EndPipboy();

    return 0;
}

// 0x497228
static int StartPipboy(int intent)
{
    bk_enable = map_disable_bk_processes();

    cycle_disable();
    gmouse_3d_off();
    disable_box_bar_win();
    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    currentAlarmTypeCount = PIPBOY_REST_DURATION_COUNT_WITHOUT_PARTY;

    if (getPartyMemberCount() > 1 && partyMemberNeedsHealing()) {
        currentAlarmTypeCount = PIPBOY_REST_DURATION_COUNT;
    }

    savefont = text_curr();
    text_font(101);

    proc_bail_flag = 0;
    rest_time = 0;
    cursor_line = 0;
    hot_line_count = 0;
    bottom_line = PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT / text_height() - 1;
    hot_line_start = 0;
    hot_back_line = 0;

    if (holodisks_init() == -1) {
        return -1;
    }

    if (!message_init(&pipboy_message_file)) {
        return -1;
    }

    char path[MAX_PATH];
    sprintf(path, "%s%s", msg_path, "pipboy.msg");

    if (!(message_load(&pipboy_message_file, path))) {
        return -1;
    }

    int index;
    for (index = 0; index < PIPBOY_FRM_COUNT; index++) {
        int fid = art_id(OBJ_TYPE_INTERFACE, pipgrphs[index], 0, 0, 0);
        pipbmp[index] = art_lock(fid, &(grphkey[index]), &(ginfo[index].width), &(ginfo[index].height));
        if (pipbmp[index] == NULL) {
            break;
        }
    }

    if (index != PIPBOY_FRM_COUNT) {
        debug_printf("\n** Error loading pipboy graphics! **\n");

        while (--index >= 0) {
            art_ptr_unlock(grphkey[index]);
        }

        return -1;
    }

    int pipboyWindowX = 0;
    int pipboyWindowY = 0;
    pip_win = win_add(pipboyWindowX, pipboyWindowY, PIPBOY_WINDOW_WIDTH, PIPBOY_WINDOW_HEIGHT, colorTable[0], WINDOW_FLAG_0x10);
    if (pip_win == -1) {
        debug_printf("\n** Error opening pipboy window! **\n");
        for (int index = 0; index < PIPBOY_FRM_COUNT; index++) {
            art_ptr_unlock(grphkey[index]);
        }
        return -1;
    }

    scrn_buf = win_get_buf(pip_win);
    memcpy(scrn_buf, pipbmp[PIPBOY_FRM_BACKGROUND], PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_HEIGHT);

    pip_num(game_time_hour(), 4, PIPBOY_WINDOW_TIME_X, PIPBOY_WINDOW_TIME_Y);
    pip_date();

    int alarmButton = win_register_button(pip_win,
        124,
        13,
        ginfo[PIPBOY_FRM_ALARM_UP].width,
        ginfo[PIPBOY_FRM_ALARM_UP].height,
        -1,
        -1,
        -1,
        504,
        pipbmp[PIPBOY_FRM_ALARM_UP],
        pipbmp[PIPBOY_FRM_ALARM_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (alarmButton != -1) {
        win_register_button_sound_func(alarmButton, gsound_med_butt_press, gsound_med_butt_release);
    }

    int y = 341;
    int eventCode = 500;
    for (int index = 0; index < 5; index += 1) {
        if (index != 1) {
            int btn = win_register_button(pip_win,
                53,
                y,
                ginfo[PIPBOY_FRM_LITTLE_RED_BUTTON_UP].width,
                ginfo[PIPBOY_FRM_LITTLE_RED_BUTTON_UP].height,
                -1,
                -1,
                -1,
                eventCode,
                pipbmp[PIPBOY_FRM_LITTLE_RED_BUTTON_UP],
                pipbmp[PIPBOY_FRM_LITTLE_RED_BUTTON_DOWN],
                NULL,
                BUTTON_FLAG_TRANSPARENT);
            if (btn != -1) {
                win_register_button_sound_func(btn, gsound_red_butt_press, gsound_red_butt_release);
            }

            eventCode += 1;
        }

        y += 27;
    }

    if (intent == PIPBOY_OPEN_INTENT_REST) {
        if (!critter_can_obj_dude_rest()) {
            trans_buf_to_buf(
                pipbmp[PIPBOY_FRM_LOGO],
                ginfo[PIPBOY_FRM_LOGO].width,
                ginfo[PIPBOY_FRM_LOGO].height,
                ginfo[PIPBOY_FRM_LOGO].width,
                scrn_buf + PIPBOY_WINDOW_WIDTH * 156 + 323,
                PIPBOY_WINDOW_WIDTH);

            int month;
            int day;
            int year;
            game_time_date(&month, &day, &year);

            int holiday = 0;
            for (; holiday < HOLIDAY_COUNT; holiday += 1) {
                const HolidayDescription* holidayDescription = &(SpclDate[holiday]);
                if (holidayDescription->month == month && holidayDescription->day == day) {
                    break;
                }
            }

            if (holiday != HOLIDAY_COUNT) {
                const HolidayDescription* holidayDescription = &(SpclDate[holiday]);
                const char* holidayName = getmsg(&pipboy_message_file, &pipmesg, holidayDescription->textId);
                char holidayNameCopy[256];
                strcpy(holidayNameCopy, holidayName);

                int len = text_width(holidayNameCopy);
                text_to_buf(scrn_buf + PIPBOY_WINDOW_WIDTH * (ginfo[PIPBOY_FRM_LOGO].height + 174) + 6 + ginfo[PIPBOY_FRM_LOGO].width / 2 + 323 - len / 2,
                    holidayNameCopy,
                    350,
                    PIPBOY_WINDOW_WIDTH,
                    colorTable[992]);
            }

            win_draw(pip_win);

            gsound_play_sfx_file("iisxxxx1");

            const char* text = getmsg(&pipboy_message_file, &pipmesg, 215);
            dialog_out(text, NULL, 0, 192, 135, colorTable[32328], 0, colorTable[32328], DIALOG_BOX_LARGE);

            intent = PIPBOY_OPEN_INTENT_UNSPECIFIED;
        }
    } else {
        trans_buf_to_buf(
            pipbmp[PIPBOY_FRM_LOGO],
            ginfo[PIPBOY_FRM_LOGO].width,
            ginfo[PIPBOY_FRM_LOGO].height,
            ginfo[PIPBOY_FRM_LOGO].width,
            scrn_buf + PIPBOY_WINDOW_WIDTH * 156 + 323,
            PIPBOY_WINDOW_WIDTH);

        int month;
        int day;
        int year;
        game_time_date(&month, &day, &year);

        int holiday;
        for (holiday = 0; holiday < HOLIDAY_COUNT; holiday += 1) {
            const HolidayDescription* holidayDescription = &(SpclDate[holiday]);
            if (holidayDescription->month == month && holidayDescription->day == day) {
                break;
            }
        }

        if (holiday != HOLIDAY_COUNT) {
            const HolidayDescription* holidayDescription = &(SpclDate[holiday]);
            const char* holidayName = getmsg(&pipboy_message_file, &pipmesg, holidayDescription->textId);
            char holidayNameCopy[256];
            strcpy(holidayNameCopy, holidayName);

            int length = text_width(holidayNameCopy);
            text_to_buf(scrn_buf + PIPBOY_WINDOW_WIDTH * (ginfo[PIPBOY_FRM_LOGO].height + 174) + 6 + ginfo[PIPBOY_FRM_LOGO].width / 2 + 323 - length / 2,
                holidayNameCopy,
                350,
                PIPBOY_WINDOW_WIDTH,
                colorTable[992]);
        }

        win_draw(pip_win);
    }

    if (quest_init() == -1) {
        return -1;
    }

    gsound_play_sfx_file("pipon");
    win_draw(pip_win);

    return intent;
}

// 0x497828
static void EndPipboy()
{
    bool showScriptMessages = false;
    configGetBool(&game_config, GAME_CONFIG_DEBUG_KEY, GAME_CONFIG_SHOW_SCRIPT_MESSAGES_KEY, &showScriptMessages);

    if (showScriptMessages) {
        debug_printf("\nScript <Map Update>");
    }

    scr_exec_map_update_scripts();

    win_delete(pip_win);

    message_exit(&pipboy_message_file);

    // NOTE: Uninline.
    holodisks_exit();

    for (int index = 0; index < PIPBOY_FRM_COUNT; index++) {
        art_ptr_unlock(grphkey[index]);
    }

    NixHotLines();

    text_font(savefont);

    if (bk_enable) {
        map_enable_bk_processes();
    }

    cycle_enable();
    enable_box_bar_win();
    gmouse_set_cursor(MOUSE_CURSOR_ARROW);
    intface_redraw();

    // NOTE: Uninline.
    quest_exit();
}

// 0x497918
void pip_init()
{
}

// 0x49791C
static void pip_num(int value, int digits, int x, int y)
{
    int offset = PIPBOY_WINDOW_WIDTH * y + x + 9 * (digits - 1);

    for (int index = 0; index < digits; index++) {
        buf_to_buf(pipbmp[PIPBOY_FRM_NUMBERS] + 9 * (value % 10), 9, 17, 360, scrn_buf + offset, PIPBOY_WINDOW_WIDTH);
        offset -= 9;
        value /= 10;
    }
}

// 0x4979B4
static void pip_date()
{
    int day;
    int month;
    int year;

    game_time_date(&month, &day, &year);
    pip_num(day, 2, PIPBOY_WINDOW_DAY_X, PIPBOY_WINDOW_DAY_Y);

    buf_to_buf(pipbmp[PIPBOY_FRM_MONTHS] + 435 * (month - 1), 29, 14, 29, scrn_buf + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_MONTH_Y + PIPBOY_WINDOW_MONTH_X, PIPBOY_WINDOW_WIDTH);

    pip_num(year, 4, PIPBOY_WINDOW_YEAR_X, PIPBOY_WINDOW_YEAR_Y);
}

// 0x497A40
static void pip_print(const char* text, int flags, int color)
{
    if ((flags & PIPBOY_TEXT_STYLE_UNDERLINE) != 0) {
        color |= FONT_UNDERLINE;
    }

    int left = 8;
    if ((flags & PIPBOY_TEXT_NO_INDENT) != 0) {
        left -= 7;
    }

    int length = text_width(text);

    if ((flags & PIPBOY_TEXT_ALIGNMENT_CENTER) != 0) {
        left = (350 - length) / 2;
    } else if ((flags & PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN) != 0) {
        left += 175;
    } else if ((flags & PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER) != 0) {
        left += 86 - length + 16;
    } else if ((flags & PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN_CENTER) != 0) {
        left += 260 - length;
    }

    text_to_buf(scrn_buf + PIPBOY_WINDOW_WIDTH * (cursor_line * text_height() + PIPBOY_WINDOW_CONTENT_VIEW_Y) + PIPBOY_WINDOW_CONTENT_VIEW_X + left, text, PIPBOY_WINDOW_WIDTH, PIPBOY_WINDOW_WIDTH, color);

    if ((flags & PIPBOY_TEXT_STYLE_STRIKE_THROUGH) != 0) {
        int top = cursor_line * text_height() + 49;
        draw_line(scrn_buf, PIPBOY_WINDOW_WIDTH, PIPBOY_WINDOW_CONTENT_VIEW_X + left, top, PIPBOY_WINDOW_CONTENT_VIEW_X + left + length, top, color);
    }

    if (cursor_line < bottom_line) {
        cursor_line += 1;
    }
}

// 0x497B64
static void pip_back(int color)
{
    if (bottom_line >= 0) {
        cursor_line = bottom_line;
    }

    buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * 436 + 254, 350, 20, PIPBOY_WINDOW_WIDTH, scrn_buf + PIPBOY_WINDOW_WIDTH * 436 + 254, PIPBOY_WINDOW_WIDTH);

    // BACK
    const char* text = getmsg(&pipboy_message_file, &pipmesg, 201);
    pip_print(text, PIPBOY_TEXT_ALIGNMENT_CENTER, color);
}

// NOTE: Collapsed.
//
// 0x497BD4
int _save_pipboy(File* stream)
{
    return 0;
}

// NOTE: Uncollapsed 0x497BD4.
int save_pipboy(File* stream)
{
    return _save_pipboy(stream);
}

// NOTE: Uncollapsed 0x497BD4.
int load_pipboy(File* stream)
{
    return _save_pipboy(stream);
}

// 0x497BD8
static void PipStatus(int a1)
{
    if (a1 == 1024) {
        NixHotLines();
        buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
            PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
            PIPBOY_WINDOW_WIDTH,
            scrn_buf + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_WIDTH);
        if (bottom_line >= 0) {
            cursor_line = 0;
        }

        holo_flag = 0;
        holodisk = -1;
        holocount = 0;
        view_page = 0;
        stat_flag = 0;

        for (int index = 0; index < holodisks_count; index += 1) {
            HolodiskDescription* holodisk_entry = &(holodisks[index]);
            if (game_global_vars[holodisk_entry->gvar] != 0) {
                holocount += 1;
                break;
            }
        }

        ListStatLines(-1);

        if (statcount == 0) {
            const char* text = getmsg(&pipboy_message_file, &pipmesg, 203);
            pip_print(text, 0, colorTable[992]);
        }

        holocount = ListHoloDiskTitles(-1);

        win_draw_rect(pip_win, &pip_rect);
        AddHotLines(2, statcount + holocount + 1, false);
        win_draw(pip_win);
        return;
    }

    if (stat_flag == 0 && holo_flag == 0) {
        if (statcount != 0 && mouse_x < 429) {
            gsound_play_sfx_file("ib1p1xx1");
            buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
                PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
                PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
                PIPBOY_WINDOW_WIDTH,
                scrn_buf + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
                PIPBOY_WINDOW_WIDTH);
            ListStatLines(a1);
            ListHoloDiskTitles(-1);
            win_draw_rect(pip_win, &pip_rect);
            pause_for_tocks(200);
            stat_flag = 1;
        } else {
            if (holocount != 0 && holocount >= a1 && mouse_x > 429) {
                gsound_play_sfx_file("ib1p1xx1");
                holodisk = 0;

                int index = 0;
                for (; index < holodisks_count; index += 1) {
                    HolodiskDescription* holodisk_entry = &(holodisks[index]);
                    if (game_global_vars[holodisk_entry->gvar] > 0) {
                        if (a1 - 1 == holodisk) {
                            break;
                        }
                        holodisk += 1;
                    }
                }
                holodisk = index;

                buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
                    PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
                    PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
                    PIPBOY_WINDOW_WIDTH,
                    scrn_buf + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
                    PIPBOY_WINDOW_WIDTH);
                ListHoloDiskTitles(holodisk);
                ListStatLines(-1);
                win_draw_rect(pip_win, &pip_rect);
                pause_for_tocks(200);
                NixHotLines();
                ShowHoloDisk();
                AddHotLines(0, 0, true);
                holo_flag = 1;
            }
        }
    }

    if (stat_flag == 0) {
        if (holo_flag == 0 || a1 < 1025 || a1 > 1027) {
            return;
        }

        if ((mouse_x > 459 && a1 != 1027) || a1 == 1026) {
            if (holopages <= view_page) {
                if (a1 != 1026) {
                    gsound_play_sfx_file("ib1p1xx1");
                    buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * 436 + 254, 350, 20, PIPBOY_WINDOW_WIDTH, scrn_buf + PIPBOY_WINDOW_WIDTH * 436 + 254, PIPBOY_WINDOW_WIDTH);

                    if (bottom_line >= 0) {
                        cursor_line = bottom_line;
                    }

                    // Back
                    const char* text1 = getmsg(&pipboy_message_file, &pipmesg, 201);
                    pip_print(text1, PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER, colorTable[992]);

                    if (bottom_line >= 0) {
                        cursor_line = bottom_line;
                    }

                    // Done
                    const char* text2 = getmsg(&pipboy_message_file, &pipmesg, 214);
                    pip_print(text2, PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN_CENTER, colorTable[992]);

                    win_draw_rect(pip_win, &pip_rect);
                    pause_for_tocks(200);
                    PipStatus(1024);
                }
            } else {
                gsound_play_sfx_file("ib1p1xx1");
                buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * 436 + 254, 350, 20, PIPBOY_WINDOW_WIDTH, scrn_buf + PIPBOY_WINDOW_WIDTH * 436 + 254, PIPBOY_WINDOW_WIDTH);

                if (bottom_line >= 0) {
                    cursor_line = bottom_line;
                }

                // Back
                const char* text1 = getmsg(&pipboy_message_file, &pipmesg, 201);
                pip_print(text1, PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER, colorTable[992]);

                if (bottom_line >= 0) {
                    cursor_line = bottom_line;
                }

                // More
                const char* text2 = getmsg(&pipboy_message_file, &pipmesg, 200);
                pip_print(text2, PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN_CENTER, colorTable[992]);

                win_draw_rect(pip_win, &pip_rect);
                pause_for_tocks(200);

                view_page += 1;

                ShowHoloDisk();
            }
            return;
        }

        if (a1 == 1027) {
            gsound_play_sfx_file("ib1p1xx1");
            buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * 436 + 254, 350, 20, PIPBOY_WINDOW_WIDTH, scrn_buf + PIPBOY_WINDOW_WIDTH * 436 + 254, PIPBOY_WINDOW_WIDTH);

            if (bottom_line >= 0) {
                cursor_line = bottom_line;
            }

            // Back
            const char* text1 = getmsg(&pipboy_message_file, &pipmesg, 201);
            pip_print(text1, PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER, colorTable[992]);

            if (bottom_line >= 0) {
                cursor_line = bottom_line;
            }

            // More
            const char* text2 = getmsg(&pipboy_message_file, &pipmesg, 200);
            pip_print(text2, PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN_CENTER, colorTable[992]);

            win_draw_rect(pip_win, &pip_rect);
            pause_for_tocks(200);

            view_page -= 1;

            if (view_page < 0) {
                PipStatus(1024);
                return;
            }
        } else {
            if (mouse_x > 395) {
                return;
            }

            gsound_play_sfx_file("ib1p1xx1");
            buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * 436 + 254, 350, 20, PIPBOY_WINDOW_WIDTH, scrn_buf + PIPBOY_WINDOW_WIDTH * 436 + 254, PIPBOY_WINDOW_WIDTH);

            if (bottom_line >= 0) {
                cursor_line = bottom_line;
            }

            // Back
            const char* text1 = getmsg(&pipboy_message_file, &pipmesg, 201);
            pip_print(text1, PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER, colorTable[992]);

            if (bottom_line >= 0) {
                cursor_line = bottom_line;
            }

            // More
            const char* text2 = getmsg(&pipboy_message_file, &pipmesg, 200);
            pip_print(text2, PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN_CENTER, colorTable[992]);

            win_draw_rect(pip_win, &pip_rect);
            pause_for_tocks(200);

            if (view_page <= 0) {
                PipStatus(1024);
                return;
            }

            view_page -= 1;
        }

        ShowHoloDisk();
        return;
    }

    if (a1 == 1025) {
        gsound_play_sfx_file("ib1p1xx1");
        pip_back(colorTable[32747]);
        win_draw_rect(pip_win, &pip_rect);
        pause_for_tocks(200);
        PipStatus(1024);
    }

    if (a1 <= statcount) {
        gsound_play_sfx_file("ib1p1xx1");

        int v13 = 0;
        int index = 0;
        for (; index < quest_count; index++) {
            QuestDescription* questDescription = &(quests[index]);
            if (questDescription->displayThreshold <= game_global_vars[questDescription->gvar]) {
                if (v13 == a1 - 1) {
                    break;
                }

                v13 += 1;

                // Skip quests in the same location.
                //
                // FIXME: This code should be identical to the one in the
                // `ListStatLines`. See buffer overread
                // bug involved.
                for (; index < quest_count; index++) {
                    if (quests[index].location != quests[index + 1].location) {
                        break;
                    }
                }
            }
        }

        NixHotLines();
        buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
            PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
            PIPBOY_WINDOW_WIDTH,
            scrn_buf + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_WIDTH);
        if (bottom_line >= 0) {
            cursor_line = 0;
        }

        if (bottom_line >= 1) {
            cursor_line = 1;
        }

        AddHotLines(0, 0, true);

        QuestDescription* questDescription = &(quests[index]);

        const char* text1 = getmsg(&pipboy_message_file, &pipmesg, 210);
        const char* text2 = getmsg(&map_msg_file, &pipmesg, questDescription->location);
        char formattedText[1024];
        sprintf(formattedText, "%s %s", text2, text1);
        pip_print(formattedText, PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);

        if (bottom_line >= 3) {
            cursor_line = 3;
        }

        int number = 1;
        for (; index < quest_count; index++) {
            QuestDescription* questDescription = &(quests[index]);
            if (game_global_vars[questDescription->gvar] >= questDescription->displayThreshold) {
                const char* text = getmsg(&quest_message_file, &pipmesg, questDescription->description);
                char formattedText[1024];
                sprintf(formattedText, "%d. %s", number, text);
                number += 1;

                short beginnings[WORD_WRAP_MAX_COUNT];
                short count;
                if (word_wrap(formattedText, 350, beginnings, &count) == 0) {
                    for (int line = 0; line < count - 1; line += 1) {
                        char* beginning = formattedText + beginnings[line];
                        char* ending = formattedText + beginnings[line + 1];
                        char c = *ending;
                        *ending = '\0';

                        int flags;
                        int color;
                        if (game_global_vars[questDescription->gvar] < questDescription->completedThreshold) {
                            flags = 0;
                            color = colorTable[992];
                        } else {
                            flags = PIPBOY_TEXT_STYLE_STRIKE_THROUGH;
                            color = colorTable[8804];
                        }

                        pip_print(beginning, flags, color);

                        *ending = c;
                        cursor_line += 1;
                    }
                } else {
                    debug_printf("\n ** Word wrap error in pipboy! **\n");
                }
            }

            if (index != quest_count - 1) {
                QuestDescription* nextQuestDescription = &(quests[index + 1]);
                if (questDescription->location != nextQuestDescription->location) {
                    break;
                }
            }
        }

        pip_back(colorTable[992]);
        win_draw_rect(pip_win, &pip_rect);
        stat_flag = 1;
    }
}

// [a1] is likely selected location, or -1 if nothing is selected
//
// 0x498734
static void ListStatLines(int a1)
{
    if (bottom_line >= 0) {
        cursor_line = 0;
    }

    int flags = holocount != 0 ? PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER : PIPBOY_TEXT_ALIGNMENT_CENTER;
    flags |= PIPBOY_TEXT_STYLE_UNDERLINE;

    // STATUS
    const char* statusText = getmsg(&pipboy_message_file, &pipmesg, 202);
    pip_print(statusText, flags, colorTable[992]);

    if (bottom_line >= 2) {
        cursor_line = 2;
    }

    statcount = 0;

    for (int index = 0; index < quest_count; index += 1) {
        QuestDescription* quest = &(quests[index]);
        if (quest->displayThreshold > game_global_vars[quest->gvar]) {
            continue;
        }

        int color = (cursor_line - 1) / 2 == (a1 - 1) ? colorTable[32747] : colorTable[992];

        // Render location.
        const char* questLocation = getmsg(&map_msg_file, &pipmesg, quest->location);
        pip_print(questLocation, 0, color);

        cursor_line += 1;
        statcount += 1;

        // Skip quests in the same location.
        //
        // FIXME: There is a buffer overread bug at the end of the loop. It does
        // not manifest because dynamically allocated memory blocks have special
        // footer guard. Location field is the first in the struct and matches
        // size of the guard. So on the final iteration it compares location of
        // the last quest with this special guard (0xBEEFCAFE).
        for (; index < quest_count; index++) {
            if (quests[index].location != quests[index + 1].location) {
                break;
            }
        }
    }
}

// 0x4988A0
static void ShowHoloDisk()
{
    buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
        PIPBOY_WINDOW_WIDTH,
        scrn_buf + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_WIDTH);

    if (bottom_line >= 0) {
        cursor_line = 0;
    }

    HolodiskDescription* holodisk_entry = &(holodisks[holodisk]);

    int holodiskTextId;
    int linesCount = 0;

    holopages = 0;

    for (holodiskTextId = holodisk_entry->description; holodiskTextId < holodisk_entry->description + 500; holodiskTextId += 1) {
        const char* text = getmsg(&pipboy_message_file, &pipmesg, holodiskTextId);
        if (strcmp(text, "**END-DISK**") == 0) {
            break;
        }

        linesCount += 1;
        if (linesCount >= PIPBOY_HOLODISK_LINES_MAX) {
            linesCount = 0;
            holopages += 1;
        }
    }

    if (holodiskTextId >= holodisk_entry->description + 500) {
        debug_printf("\nPIPBOY: #1 Holodisk text end not found!\n");
    }

    holodiskTextId = holodisk_entry->description;

    if (view_page != 0) {
        int page = 0;
        int numberOfLines = 0;
        for (; holodiskTextId < holodiskTextId + 500; holodiskTextId += 1) {
            const char* line = getmsg(&pipboy_message_file, &pipmesg, holodiskTextId);
            if (strcmp(line, "**END-DISK**") == 0) {
                debug_printf("\nPIPBOY: Premature page end in holodisk page search!\n");
                break;
            }

            numberOfLines += 1;
            if (numberOfLines >= PIPBOY_HOLODISK_LINES_MAX) {
                page += 1;
                if (page >= view_page) {
                    break;
                }

                numberOfLines = 0;
            }
        }

        holodiskTextId += 1;

        if (holodiskTextId >= holodisk_entry->description + 500) {
            debug_printf("\nPIPBOY: #2 Holodisk text end not found!\n");
        }
    } else {
        const char* name = getmsg(&pipboy_message_file, &pipmesg, holodisk_entry->name);
        pip_print(name, PIPBOY_TEXT_ALIGNMENT_CENTER | PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);
    }

    if (holopages != 0) {
        // of
        const char* of = getmsg(&pipboy_message_file, &pipmesg, 212);
        char formattedText[60]; // TODO: Size is probably wrong.
        sprintf(formattedText, "%d %s %d", view_page + 1, of, holopages + 1);

        int len = text_width(of);
        text_to_buf(scrn_buf + PIPBOY_WINDOW_WIDTH * 47 + 616 + 604 - len, formattedText, 350, PIPBOY_WINDOW_WIDTH, colorTable[992]);
    }

    if (bottom_line >= 3) {
        cursor_line = 3;
    }

    for (int line = 0; line < PIPBOY_HOLODISK_LINES_MAX; line += 1) {
        const char* text = getmsg(&pipboy_message_file, &pipmesg, holodiskTextId);
        if (strcmp(text, "**END-DISK**") == 0) {
            break;
        }

        if (strcmp(text, "**END-PAR**") == 0) {
            cursor_line += 1;
        } else {
            pip_print(text, PIPBOY_TEXT_NO_INDENT, colorTable[992]);
        }

        holodiskTextId += 1;
    }

    int moreOrDoneTextId;
    if (holopages <= view_page) {
        if (bottom_line >= 0) {
            cursor_line = bottom_line;
        }

        const char* back = getmsg(&pipboy_message_file, &pipmesg, 201);
        pip_print(back, PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER, colorTable[992]);

        if (bottom_line >= 0) {
            cursor_line = bottom_line;
        }

        moreOrDoneTextId = 214;
    } else {
        if (bottom_line >= 0) {
            cursor_line = bottom_line;
        }

        const char* back = getmsg(&pipboy_message_file, &pipmesg, 201);
        pip_print(back, PIPBOY_TEXT_ALIGNMENT_LEFT_COLUMN_CENTER, colorTable[992]);

        if (bottom_line >= 0) {
            cursor_line = bottom_line;
        }

        moreOrDoneTextId = 200;
    }

    const char* moreOrDoneText = getmsg(&pipboy_message_file, &pipmesg, moreOrDoneTextId);
    pip_print(moreOrDoneText, PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN_CENTER, colorTable[992]);
    win_draw(pip_win);
}

// 0x498C40
static int ListHoloDiskTitles(int a1)
{
    if (bottom_line >= 2) {
        cursor_line = 2;
    }

    int knownHolodisksCount = 0;
    for (int index = 0; index < holodisks_count; index++) {
        HolodiskDescription* holodisk_entry = &(holodisks[index]);
        if (game_global_vars[holodisk_entry->gvar] != 0) {
            int color;
            if ((cursor_line - 2) / 2 == a1) {
                color = colorTable[32747];
            } else {
                color = colorTable[992];
            }

            const char* text = getmsg(&pipboy_message_file, &pipmesg, holodisk_entry->name);
            pip_print(text, PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN, color);

            cursor_line++;
            knownHolodisksCount++;
        }
    }

    if (knownHolodisksCount != 0) {
        if (bottom_line >= 0) {
            cursor_line = 0;
        }

        const char* text = getmsg(&pipboy_message_file, &pipmesg, 211); // DATA
        pip_print(text, PIPBOY_TEXT_ALIGNMENT_RIGHT_COLUMN_CENTER | PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);
    }

    return knownHolodisksCount;
}

// 0x498D34
static int qscmp(const void* a1, const void* a2)
{
    STRUCT_664350* v1 = (STRUCT_664350*)a1;
    STRUCT_664350* v2 = (STRUCT_664350*)a2;

    return strcmp(v1->name, v2->name);
}

// 0x498D40
static void PipAutomaps(int a1)
{
    if (a1 == 1024) {
        NixHotLines();
        buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
            PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
            PIPBOY_WINDOW_WIDTH,
            scrn_buf + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_WIDTH);

        if (bottom_line >= 0) {
            cursor_line = 0;
        }

        const char* title = getmsg(&pipboy_message_file, &pipmesg, 205);
        pip_print(title, PIPBOY_TEXT_ALIGNMENT_CENTER | PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);

        actcnt = PrintAMList(-1);

        AddHotLines(2, actcnt, 0);

        win_draw_rect(pip_win, &pip_rect);
        amlst_mode = 0;
        return;
    }

    if (amlst_mode != 0) {
        if (a1 == 1025 || a1 <= -1) {
            PipAutomaps(1024);
            gsound_play_sfx_file("ib1p1xx1");
        }

        if (a1 >= 1 && a1 <= actcnt + 3) {
            gsound_play_sfx_file("ib1p1xx1");
            PrintAMelevList(a1);
            draw_top_down_map_pipboy(pip_win, sortlist[a1 - 1].field_6, sortlist[a1 - 1].field_4);
            win_draw_rect(pip_win, &pip_rect);
        }

        return;
    }

    if (a1 > 0 && a1 <= actcnt) {
        gsound_play_sfx_file("ib1p1xx1");
        NixHotLines();
        PrintAMList(a1);
        win_draw_rect(pip_win, &pip_rect);
        amcty_indx = sortlist[a1 - 1].field_4;
        actcnt = PrintAMelevList(1);
        AddHotLines(0, actcnt + 2, 1);
        draw_top_down_map_pipboy(pip_win, sortlist[0].field_6, sortlist[0].field_4);
        win_draw_rect(pip_win, &pip_rect);
        amlst_mode = 1;
    }
}

// 0x498F30
static int PrintAMelevList(int a1)
{
    AutomapHeader* automapHeader;
    if (ReadAMList(&automapHeader) == -1) {
        return -1;
    }

    int v4 = 0;
    for (int elevation = 0; elevation < ELEVATION_COUNT; elevation++) {
        if (automapHeader->offsets[amcty_indx][elevation] > 0) {
            sortlist[v4].name = map_get_elev_idx(amcty_indx, elevation);
            sortlist[v4].field_4 = elevation;
            sortlist[v4].field_6 = amcty_indx;
            v4++;
        }
    }

    int mapCount = wmMapMaxCount();
    for (int map = 0; map < mapCount; map++) {
        if (map == amcty_indx) {
            continue;
        }

        if (get_map_idx_same(amcty_indx, map) == -1) {
            continue;
        }

        for (int elevation = 0; elevation < ELEVATION_COUNT; elevation++) {
            if (automapHeader->offsets[map][elevation] > 0) {
                sortlist[v4].name = map_get_elev_idx(map, elevation);
                sortlist[v4].field_4 = elevation;
                sortlist[v4].field_6 = map;
                v4++;
            }
        }
    }

    buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
        PIPBOY_WINDOW_WIDTH,
        scrn_buf + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_WIDTH);

    if (bottom_line >= 0) {
        cursor_line = 0;
    }

    const char* msg = getmsg(&pipboy_message_file, &pipmesg, 205);
    pip_print(msg, PIPBOY_TEXT_ALIGNMENT_CENTER | PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);

    if (bottom_line >= 2) {
        cursor_line = 2;
    }

    const char* name = map_get_description_idx(amcty_indx);
    pip_print(name, PIPBOY_TEXT_ALIGNMENT_CENTER, colorTable[992]);

    if (bottom_line >= 4) {
        cursor_line = 4;
    }

    int selectedPipboyLine = (a1 - 1) * 2;

    for (int index = 0; index < v4; index++) {
        int color;
        if (cursor_line - 4 == selectedPipboyLine) {
            color = colorTable[32747];
        } else {
            color = colorTable[992];
        }

        pip_print(sortlist[index].name, 0, color);
        cursor_line++;
    }

    pip_back(colorTable[992]);

    return v4;
}

// 0x499150
static int PrintAMList(int a1)
{
    AutomapHeader* automapHeader;
    if (ReadAMList(&automapHeader) == -1) {
        return -1;
    }

    int count = 0;
    int index = 0;

    int mapCount = wmMapMaxCount();
    for (int map = 0; map < mapCount; map++) {
        int elevation;
        for (elevation = 0; elevation < ELEVATION_COUNT; elevation++) {
            if (automapHeader->offsets[map][elevation] > 0) {
                if (automapDisplayMap(map) == 0) {
                    break;
                }
            }
        }

        if (elevation < ELEVATION_COUNT) {
            int v7;
            if (count != 0) {
                v7 = 0;
                for (int index = 0; index < count; index++) {
                    if (is_map_idx_same(map, sortlist[index].field_4)) {
                        break;
                    }

                    v7++;
                }
            } else {
                v7 = 0;
            }

            if (v7 == count) {
                sortlist[count].name = map_get_short_name(map);
                sortlist[count].field_4 = map;
                count++;
            }
        }
    }

    if (count != 0) {
        if (count > 1) {
            qsort(sortlist, count, sizeof(*sortlist), qscmp);
        }

        buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
            PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
            PIPBOY_WINDOW_WIDTH,
            scrn_buf + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
            PIPBOY_WINDOW_WIDTH);

        if (bottom_line >= 0) {
            cursor_line = 0;
        }

        const char* msg = getmsg(&pipboy_message_file, &pipmesg, 205);
        pip_print(msg, PIPBOY_TEXT_ALIGNMENT_CENTER | PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);

        if (bottom_line >= 2) {
            cursor_line = 2;
        }

        for (int index = 0; index < count; index++) {
            int color;
            if (cursor_line - 1 == a1) {
                color = colorTable[32747];
            } else {
                color = colorTable[992];
            }

            pip_print(sortlist[index].name, 0, color);
            cursor_line++;
        }
    }

    return count;
}

// 0x49932C
static void PipArchives(int a1)
{
    if (a1 == 1024) {
        NixHotLines();
        view_page = ListArchive(-1);
        AddHotLines(2, view_page, false);
    } else if (a1 >= 0 && a1 <= view_page) {
        gsound_play_sfx_file("ib1p1xx1");

        ListArchive(a1);

        int movie;
        for (movie = 2; movie < 16; movie++) {
            if (gmovie_has_been_played(movie)) {
                a1--;
                if (a1 <= 0) {
                    break;
                }
            }
        }

        if (movie <= MOVIE_COUNT) {
            gmovie_play(movie, GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC);
        } else {
            debug_printf("\n ** Selected movie not found in list! **\n");
        }

        text_font(101);

        wait_time = get_time();
        ListArchive(-1);
    }
}

// 0x4993DC
static int ListArchive(int a1)
{
    const char* text;
    int i;
    int v12;
    int msg_num;
    int v5;
    int v8;
    int v9;

    buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
        PIPBOY_WINDOW_WIDTH,
        scrn_buf + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_WIDTH);

    if (bottom_line >= 0) {
        cursor_line = 0;
    }

    // VIDEO ARCHIVES
    text = getmsg(&pipboy_message_file, &pipmesg, 206);
    pip_print(text, PIPBOY_TEXT_ALIGNMENT_CENTER | PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);

    if (bottom_line >= 2) {
        cursor_line = 2;
    }

    v5 = 0;
    v12 = a1 - 1;

    // 502 - Elder Speech
    // ...
    // 516 - Credits
    msg_num = 502;

    for (i = 2; i < 16; i++) {
        if (gmovie_has_been_played(i)) {
            v8 = v5++;
            if (v8 == v12) {
                v9 = colorTable[32747];
            } else {
                v9 = colorTable[992];
            }

            text = getmsg(&pipboy_message_file, &pipmesg, msg_num);
            pip_print(text, 0, v9);

            cursor_line++;
        }

        msg_num++;
    }

    win_draw_rect(pip_win, &pip_rect);

    return v5;
}

// 0x499518
static void PipAlarm(int a1)
{
    if (a1 == 1024) {
        if (critter_can_obj_dude_rest()) {
            NixHotLines();
            DrawAlarmText(0);
            AddHotLines(5, currentAlarmTypeCount, false);
        } else {
            gsound_play_sfx_file("iisxxxx1");

            // You cannot rest at this location!
            const char* text = getmsg(&pipboy_message_file, &pipmesg, 215);
            dialog_out(text, NULL, 0, 192, 135, colorTable[32328], 0, colorTable[32328], DIALOG_BOX_LARGE);
        }
    } else if (a1 >= 4 && a1 <= 17) {
        gsound_play_sfx_file("ib1p1xx1");

        DrawAlarmText(a1 - 3);

        int duration = a1 - 4;
        int minutes = 0;
        int hours = 0;
        int v10 = 0;

        switch (duration) {
        case PIPBOY_REST_DURATION_TEN_MINUTES:
            TimedRest(0, 10, 0);
            break;
        case PIPBOY_REST_DURATION_THIRTY_MINUTES:
            TimedRest(0, 30, 0);
            break;
        case PIPBOY_REST_DURATION_ONE_HOUR:
        case PIPBOY_REST_DURATION_TWO_HOURS:
        case PIPBOY_REST_DURATION_THREE_HOURS:
        case PIPBOY_REST_DURATION_FOUR_HOURS:
        case PIPBOY_REST_DURATION_FIVE_HOURS:
        case PIPBOY_REST_DURATION_SIX_HOURS:
            TimedRest(duration - 1, 0, 0);
            break;
        case PIPBOY_REST_DURATION_UNTIL_MORNING:
            ClacTime(&hours, &minutes, 8);
            TimedRest(hours, minutes, 0);
            break;
        case PIPBOY_REST_DURATION_UNTIL_NOON:
            ClacTime(&hours, &minutes, 12);
            TimedRest(hours, minutes, 0);
            break;
        case PIPBOY_REST_DURATION_UNTIL_EVENING:
            ClacTime(&hours, &minutes, 18);
            TimedRest(hours, minutes, 0);
            break;
        case PIPBOY_REST_DURATION_UNTIL_MIDNIGHT:
            ClacTime(&hours, &minutes, 0);
            if (TimedRest(hours, minutes, 0) == 0) {
                pip_num(0, 4, PIPBOY_WINDOW_TIME_X, PIPBOY_WINDOW_TIME_Y);
            }
            win_draw(pip_win);
            break;
        case PIPBOY_REST_DURATION_UNTIL_HEALED:
        case PIPBOY_REST_DURATION_UNTIL_PARTY_HEALED:
            TimedRest(0, 0, duration);
            break;
        }

        gsound_play_sfx_file("ib2lu1x1");

        DrawAlarmText(0);
    }
}

// 0x4996B4
static void DrawAlarmText(int a1)
{
    const char* text;

    buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
        PIPBOY_WINDOW_WIDTH,
        scrn_buf + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_WIDTH);

    if (bottom_line >= 0) {
        cursor_line = 0;
    }

    // ALARM CLOCK
    text = getmsg(&pipboy_message_file, &pipmesg, 300);
    pip_print(text, PIPBOY_TEXT_ALIGNMENT_CENTER | PIPBOY_TEXT_STYLE_UNDERLINE, colorTable[992]);

    if (bottom_line >= 5) {
        cursor_line = 5;
    }

    DrawAlrmHitPnts();

    // NOTE: I don't know if this +1 was a result of compiler optimization or it
    // was written like this in the first place.
    for (int option = 1; option < currentAlarmTypeCount + 1; option++) {
        // 302 - Rest for ten minutes
        // ...
        // 315 - Rest until party is healed
        text = getmsg(&pipboy_message_file, &pipmesg, 302 + option - 1);
        int color = option == a1 ? colorTable[32747] : colorTable[992];

        pip_print(text, 0, color);

        cursor_line++;
    }

    win_draw_rect(pip_win, &pip_rect);
}

// 0x4997B8
static void DrawAlrmHitPnts()
{
    int max_hp;
    int cur_hp;
    char* text;
    char msg[64];
    int len;

    buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + 66 * PIPBOY_WINDOW_WIDTH + 254,
        350,
        10,
        PIPBOY_WINDOW_WIDTH,
        scrn_buf + 66 * PIPBOY_WINDOW_WIDTH + 254,
        PIPBOY_WINDOW_WIDTH);

    max_hp = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);
    cur_hp = critter_get_hits(obj_dude);
    text = getmsg(&pipboy_message_file, &pipmesg, 301); // Hit Points
    sprintf(msg, "%s %d/%d", text, cur_hp, max_hp);
    len = text_width(msg);
    text_to_buf(scrn_buf + 66 * PIPBOY_WINDOW_WIDTH + 254 + (350 - len) / 2, msg, PIPBOY_WINDOW_WIDTH, PIPBOY_WINDOW_WIDTH, colorTable[992]);
}

// 0x4998C0
static void AddHotLines(int start, int count, bool add_back_button)
{
    text_font(101);

    int height = text_height();

    hot_line_start = start;
    hot_line_count = count;

    if (count != 0) {
        int y = start * height + PIPBOY_WINDOW_CONTENT_VIEW_Y;
        int eventCode = start + 505;
        for (int index = start; index < 22; index++) {
            if (hot_line_count + hot_line_start <= index) {
                break;
            }

            HotLines[index] = win_register_button(pip_win, 254, y, 350, height, -1, -1, -1, eventCode, NULL, NULL, NULL, BUTTON_FLAG_TRANSPARENT);
            y += height * 2;
            eventCode += 1;
        }
    }

    if (add_back_button) {
        HotLines[BACK_BUTTON_INDEX] = win_register_button(pip_win, 254, height * bottom_line + PIPBOY_WINDOW_CONTENT_VIEW_Y, 350, height, -1, -1, -1, 528, NULL, NULL, NULL, BUTTON_FLAG_TRANSPARENT);
    }
}

// 0x4999C0
static void NixHotLines()
{
    if (hot_line_count != 0) {
        // NOTE: There is a buffer overread bug. In original binary it leads to
        // reading continuation (from 0x6644B8 onwards), which finally destroys
        // button in `pip_win` (id #3), which corresponds to Skilldex
        // button. Other buttons can be destroyed depending on the last mouse
        // position. I was not able to replicate this exact behaviour with MSVC.
        // So here is a small fix, which is an exception to "Do not fix vanilla
        // bugs" strategy.
        //
        // TODO: Reevaluate after merging back button into `HotLines`.
        int end = hot_line_start + hot_line_count;
        if (end > 22) {
            end = 22;
        }

        for (int index = hot_line_start; index < end; index++) {
            win_delete_button(HotLines[index]);
        }
    }

    if (hot_back_line) {
        win_delete_button(HotLines[BACK_BUTTON_INDEX]);
    }

    hot_line_count = 0;
    hot_back_line = 0;
}

// 0x499A24
static bool TimedRest(int hours, int minutes, int duration)
{
    gmouse_set_cursor(MOUSE_CURSOR_WAIT_WATCH);

    bool rc = false;

    if (duration == 0) {
        int hoursInMinutes = hours * 60;
        double v1 = (double)hoursInMinutes + (double)minutes;
        double v2 = v1 * (1.0 / 1440.0) * 3.5 + 0.25;
        double v3 = (double)minutes / v1 * v2;
        if (minutes != 0) {
            int gameTime = game_time();

            double v4 = v3 * 20.0;
            int v5 = 0;
            for (int v5 = 0; v5 < (int)v4; v5++) {
                if (rc) {
                    break;
                }

                unsigned int start = get_time();

                unsigned int v6 = (unsigned int)((double)v5 / v4 * ((double)minutes * 600.0) + (double)gameTime);
                unsigned int nextEventTime = queue_next_time();
                if (v6 >= nextEventTime) {
                    gameTimeSetTime(nextEventTime + 1);
                    if (queue_process()) {
                        rc = true;
                        debug_printf("PIPBOY: Returning from Queue trigger...\n");
                        proc_bail_flag = 1;
                        break;
                    }

                    if (game_user_wants_to_quit != 0) {
                        rc = true;
                    }
                }

                if (!rc) {
                    gameTimeSetTime(v6);
                    if (get_input() == KEY_ESCAPE || game_user_wants_to_quit != 0) {
                        rc = true;
                    }

                    pip_num(game_time_hour(), 4, PIPBOY_WINDOW_TIME_X, PIPBOY_WINDOW_TIME_Y);
                    pip_date();
                    win_draw(pip_win);

                    while (elapsed_time(start) < 50) {
                    }
                }
            }

            if (!rc) {
                gameTimeSetTime(gameTime + 600 * minutes);

                if (Check4Health(minutes)) {
                    // NOTE: Uninline.
                    AddHealth();
                }
            }

            pip_num(game_time_hour(), 4, PIPBOY_WINDOW_TIME_X, PIPBOY_WINDOW_TIME_Y);
            pip_date();
            DrawAlrmHitPnts();
            win_draw(pip_win);
        }

        if (hours != 0 && !rc) {
            int gameTime = game_time();
            double v7 = (v2 - v3) * 20.0;

            for (int hour = 0; hour < (int)v7; hour++) {
                if (rc) {
                    break;
                }

                unsigned int start = get_time();

                if (get_input() == KEY_ESCAPE || game_user_wants_to_quit != 0) {
                    rc = true;
                }

                unsigned int v8 = (unsigned int)((double)hour / v7 * (hours * GAME_TIME_TICKS_PER_HOUR) + gameTime);
                unsigned int nextEventTime = queue_next_time();
                if (!rc && v8 >= nextEventTime) {
                    gameTimeSetTime(nextEventTime + 1);

                    if (queue_process()) {
                        rc = true;
                        debug_printf("PIPBOY: Returning from Queue trigger...\n");
                        proc_bail_flag = 1;
                        break;
                    }

                    if (game_user_wants_to_quit != 0) {
                        rc = true;
                    }
                }

                if (!rc) {
                    gameTimeSetTime(v8);

                    int healthToAdd = (int)((double)hoursInMinutes / v7);
                    if (Check4Health(healthToAdd)) {
                        // NOTE: Uninline.
                        AddHealth();
                    }

                    pip_num(game_time_hour(), 4, PIPBOY_WINDOW_TIME_X, PIPBOY_WINDOW_TIME_Y);
                    pip_date();
                    DrawAlrmHitPnts();
                    win_draw(pip_win);

                    while (elapsed_time(start) < 50) {
                    }
                }
            }

            if (!rc) {
                gameTimeSetTime(gameTime + GAME_TIME_TICKS_PER_HOUR * hours);
            }

            pip_num(game_time_hour(), 4, PIPBOY_WINDOW_TIME_X, PIPBOY_WINDOW_TIME_Y);
            pip_date();
            DrawAlrmHitPnts();
            win_draw(pip_win);
        }
    } else if (duration == PIPBOY_REST_DURATION_UNTIL_HEALED || duration == PIPBOY_REST_DURATION_UNTIL_PARTY_HEALED) {
        int currentHp = critter_get_hits(obj_dude);
        int maxHp = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);
        if (currentHp != maxHp
            || (duration == PIPBOY_REST_DURATION_UNTIL_PARTY_HEALED && partyMemberNeedsHealing())) {
            // First pass - healing dude is the top priority.
            int hpToHeal = maxHp - currentHp;
            int healingRate = critterGetStat(obj_dude, STAT_HEALING_RATE);
            int hoursToHeal = (int)((double)hpToHeal / (double)healingRate * 3.0);
            while (!rc && hoursToHeal != 0) {
                if (hoursToHeal <= 24) {
                    rc = TimedRest(hoursToHeal, 0, 0);
                    hoursToHeal = 0;
                } else {
                    rc = TimedRest(24, 0, 0);
                    hoursToHeal -= 24;
                }
            }

            // Second pass - attempt to heal delayed damage to dude (via poison
            // or radiation), and remaining party members. This process is
            // performed in 3 hour increments.
            currentHp = critter_get_hits(obj_dude);
            maxHp = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);
            hpToHeal = maxHp - currentHp;

            if (duration == PIPBOY_REST_DURATION_UNTIL_PARTY_HEALED) {
                int partyHpToHeal = partyMemberMaxHealingNeeded();
                if (partyHpToHeal > hpToHeal) {
                    hpToHeal = partyHpToHeal;
                }
            }

            while (!rc && hpToHeal != 0) {
                currentHp = critter_get_hits(obj_dude);
                maxHp = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);
                hpToHeal = maxHp - currentHp;

                if (duration == PIPBOY_REST_DURATION_UNTIL_PARTY_HEALED) {
                    int partyHpToHeal = partyMemberMaxHealingNeeded();
                    if (partyHpToHeal > hpToHeal) {
                        hpToHeal = partyHpToHeal;
                    }
                }

                rc = TimedRest(3, 0, 0);
            }
        } else {
            // No one needs healing.
            gmouse_set_cursor(MOUSE_CURSOR_ARROW);
            return rc;
        }
    }

    int gameTime = game_time();
    int nextEventGameTime = queue_next_time();
    if (gameTime > nextEventGameTime) {
        if (queue_process()) {
            debug_printf("PIPBOY: Returning from Queue trigger...\n");
            proc_bail_flag = 1;
            rc = true;
        }
    }

    pip_num(game_time_hour(), 4, PIPBOY_WINDOW_TIME_X, PIPBOY_WINDOW_TIME_Y);
    pip_date();
    win_draw(pip_win);

    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    return rc;
}

// 0x499FCC
static bool Check4Health(int a1)
{
    rest_time += a1;

    if (rest_time < 180) {
        return false;
    }

    debug_printf("\n health added!\n");
    rest_time = 0;

    return true;
}

// NOTE: Inlined.
//
// 0x49A008
static bool AddHealth()
{
    partyMemberRestingHeal(3);

    int currentHp = critter_get_hits(obj_dude);
    int maxHp = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);
    return currentHp == maxHp;
}

// Returns [hours] and [minutes] needed to rest until [wakeUpHour].
//
// 0x49A03C
static void ClacTime(int* hours, int* minutes, int wakeUpHour)
{
    int gameTimeHour = game_time_hour();

    *hours = gameTimeHour / 100;
    *minutes = gameTimeHour % 100;

    if (*hours != wakeUpHour || *minutes != 0) {
        *hours = wakeUpHour - *hours;
        if (*hours < 0) {
            *hours += 24;
            if (*minutes != 0) {
                *hours -= 1;
                *minutes = 60 - *minutes;
            }
        } else {
            if (*minutes != 0) {
                *hours -= 1;
                *minutes = 60 - *minutes;
                if (*hours < 0) {
                    *hours = 23;
                }
            }
        }
    } else {
        *hours = 24;
    }
}

// 0x49A0C8
static int ScreenSaver()
{
    PipboyBomb bombs[PIPBOY_BOMB_COUNT];

    mouse_get_position(&old_mouse_x, &old_mouse_y);

    for (int index = 0; index < PIPBOY_BOMB_COUNT; index += 1) {
        bombs[index].field_10 = 0;
    }

    gmouse_disable(0);

    unsigned char* buf = (unsigned char*)mem_malloc(412 * 374);
    if (buf == NULL) {
        return -1;
    }

    buf_to_buf(scrn_buf + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
        PIPBOY_WINDOW_WIDTH,
        buf,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH);

    buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
        PIPBOY_WINDOW_WIDTH,
        scrn_buf + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_WIDTH);

    int v31 = 50;
    while (true) {
        unsigned int time = get_time();

        mouse_get_position(&mouse_x, &mouse_y);
        if (get_input() != -1 || old_mouse_x != mouse_x || old_mouse_y != mouse_y) {
            break;
        }

        double random = roll_random(0, PIPBOY_RAND_MAX);

        // TODO: Figure out what this constant means. Probably somehow related
        // to PIPBOY_RAND_MAX.
        if (random < 3047.3311) {
            int index = 0;
            for (; index < PIPBOY_BOMB_COUNT; index += 1) {
                if (bombs[index].field_10 == 0) {
                    break;
                }
            }

            if (index < PIPBOY_BOMB_COUNT) {
                PipboyBomb* bomb = &(bombs[index]);
                int v27 = (350 - ginfo[PIPBOY_FRM_BOMB].width / 4) + (406 - ginfo[PIPBOY_FRM_BOMB].height / 4);
                int v5 = (int)((double)roll_random(0, PIPBOY_RAND_MAX) / (double)PIPBOY_RAND_MAX * (double)v27);
                int v6 = ginfo[PIPBOY_FRM_BOMB].height / 4;
                if (PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT - v6 >= v5) {
                    bomb->x = 602;
                    bomb->y = v5 + 48;
                } else {
                    bomb->x = v5 - (PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT - v6) + PIPBOY_WINDOW_CONTENT_VIEW_X + ginfo[PIPBOY_FRM_BOMB].width / 4;
                    bomb->y = PIPBOY_WINDOW_CONTENT_VIEW_Y - ginfo[PIPBOY_FRM_BOMB].height + 2;
                }

                bomb->field_10 = 1;
                bomb->field_8 = (float)((double)roll_random(0, PIPBOY_RAND_MAX) * (2.75 / PIPBOY_RAND_MAX) + 0.15);
                bomb->field_C = 0;
            }
        }

        if (v31 == 0) {
            buf_to_buf(pipbmp[PIPBOY_FRM_BACKGROUND] + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
                PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
                PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
                PIPBOY_WINDOW_WIDTH,
                scrn_buf + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
                PIPBOY_WINDOW_WIDTH);
        }

        for (int index = 0; index < PIPBOY_BOMB_COUNT; index++) {
            PipboyBomb* bomb = &(bombs[index]);
            if (bomb->field_10 != 1) {
                continue;
            }

            int srcWidth = ginfo[PIPBOY_FRM_BOMB].width;
            int srcHeight = ginfo[PIPBOY_FRM_BOMB].height;
            int destX = bomb->x;
            int destY = bomb->y;
            int srcY = 0;
            int srcX = 0;

            if (destX >= PIPBOY_WINDOW_CONTENT_VIEW_X) {
                if (destX + ginfo[PIPBOY_FRM_BOMB].width >= 604) {
                    srcWidth = 604 - destX;
                    if (srcWidth < 1) {
                        bomb->field_10 = 0;
                    }
                }
            } else {
                srcX = PIPBOY_WINDOW_CONTENT_VIEW_X - destX;
                if (srcX >= ginfo[PIPBOY_FRM_BOMB].width) {
                    bomb->field_10 = 0;
                }
                destX = PIPBOY_WINDOW_CONTENT_VIEW_X;
                srcWidth = ginfo[PIPBOY_FRM_BOMB].width - srcX;
            }

            if (destY >= PIPBOY_WINDOW_CONTENT_VIEW_Y) {
                if (destY + ginfo[PIPBOY_FRM_BOMB].height >= 452) {
                    srcHeight = 452 - destY;
                    if (srcHeight < 1) {
                        bomb->field_10 = 0;
                    }
                }
            } else {
                if (destY + ginfo[PIPBOY_FRM_BOMB].height < PIPBOY_WINDOW_CONTENT_VIEW_Y) {
                    bomb->field_10 = 0;
                }

                srcY = PIPBOY_WINDOW_CONTENT_VIEW_Y - destY;
                srcHeight = ginfo[PIPBOY_FRM_BOMB].height - srcY;
                destY = PIPBOY_WINDOW_CONTENT_VIEW_Y;
            }

            if (bomb->field_10 == 1 && v31 == 0) {
                trans_buf_to_buf(
                    pipbmp[PIPBOY_FRM_BOMB] + ginfo[PIPBOY_FRM_BOMB].width * srcY + srcX,
                    srcWidth,
                    srcHeight,
                    ginfo[PIPBOY_FRM_BOMB].width,
                    scrn_buf + PIPBOY_WINDOW_WIDTH * destY + destX,
                    PIPBOY_WINDOW_WIDTH);
            }

            bomb->field_C += bomb->field_8;
            if (bomb->field_C >= 1.0) {
                bomb->x = (int)((float)bomb->x - bomb->field_C);
                bomb->y = (int)((float)bomb->y + bomb->field_C);
                bomb->field_C = 0.0;
            }
        }

        if (v31 != 0) {
            v31 -= 1;
        } else {
            win_draw_rect(pip_win, &pip_rect);
            while (elapsed_time(time) < 50) {
            }
        }
    }

    buf_to_buf(buf,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT,
        PIPBOY_WINDOW_CONTENT_VIEW_WIDTH,
        scrn_buf + PIPBOY_WINDOW_WIDTH * PIPBOY_WINDOW_CONTENT_VIEW_Y + PIPBOY_WINDOW_CONTENT_VIEW_X,
        PIPBOY_WINDOW_WIDTH);

    mem_free(buf);

    win_draw_rect(pip_win, &pip_rect);
    gmouse_enable();

    return 0;
}

// 0x49A5D4
static int quest_init()
{
    // NOTE: Uninline.
    quest_exit();

    if (!message_init(&quest_message_file)) {
        return -1;
    }

    if (!message_load(&quest_message_file, "game\\quests.msg")) {
        return -1;
    }

    File* stream = db_fopen("data\\quests.txt", "rt");
    if (stream == NULL) {
        return -1;
    }

    char string[256];
    while (db_fgets(string, 256, stream)) {
        const char* delim = " \t,";
        char* tok;
        QuestDescription entry;

        char* pch = string;
        while (isspace(*pch)) {
            pch++;
        }

        if (*pch == '#') {
            continue;
        }

        tok = strtok(pch, delim);
        if (tok == NULL) {
            continue;
        }

        entry.location = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.description = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.gvar = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.displayThreshold = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.completedThreshold = atoi(tok);

        QuestDescription* entries = (QuestDescription*)mem_realloc(quests, sizeof(*quests) * (quest_count + 1));
        if (entries == NULL) {
            goto err;
        }

        memcpy(&(entries[quest_count]), &entry, sizeof(entry));

        quests = entries;
        quest_count++;
    }

    qsort(quests, quest_count, sizeof(*quests), quest_qsort_compare);

    db_fclose(stream);

    return 0;

err:

    db_fclose(stream);

    return -1;
}

// 0x49A7E4
static void quest_exit()
{
    if (quests != NULL) {
        mem_free(quests);
        quests = NULL;
    }

    quest_count = 0;

    message_exit(&quest_message_file);
}

// 0x49A818
static int quest_qsort_compare(const void* a1, const void* a2)
{
    QuestDescription* quest1 = (QuestDescription*)a1;
    QuestDescription* quest2 = (QuestDescription*)a2;
    return quest1->location - quest2->location;
}

// 0x49A824
static int holodisks_init()
{
    // NOTE: Uninline.
    holodisks_exit();

    File* stream = db_fopen("data\\holodisk.txt", "rt");
    if (stream == NULL) {
        return -1;
    }

    char str[256];
    while (db_fgets(str, sizeof(str), stream)) {
        const char* delim = " \t,";
        char* tok;
        HolodiskDescription entry;

        char* ch = str;
        while (isspace(*ch)) {
            ch++;
        }

        if (*ch == '#') {
            continue;
        }

        tok = strtok(ch, delim);
        if (tok == NULL) {
            continue;
        }

        entry.gvar = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.name = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.description = atoi(tok);

        HolodiskDescription* entries = (HolodiskDescription*)mem_realloc(holodisks, sizeof(*holodisks) * (holodisks_count + 1));
        if (entries == NULL) {
            goto err;
        }

        memcpy(&(entries[holodisks_count]), &entry, sizeof(*holodisks));

        holodisks = entries;
        holodisks_count++;
    }

    db_fclose(stream);

    return 0;

err:

    db_fclose(stream);

    return -1;
}

// 0x49A968
static void holodisks_exit()
{
    if (holodisks != NULL) {
        mem_free(holodisks);
        holodisks = NULL;
    }

    holodisks_count = 0;
}
