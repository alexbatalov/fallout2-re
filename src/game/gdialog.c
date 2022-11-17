#include "game/gdialog.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "int/window.h"
#include "game/actions.h"
#include "plib/color/color.h"
#include "game/combat.h"
#include "game/combatai.h"
#include "plib/gnw/input.h"
#include "game/critter.h"
#include "game/cycle.h"
#include "plib/gnw/debug.h"
#include "int/dialog.h"
#include "game/display.h"
#include "plib/gnw/grbuf.h"
#include "game/game.h"
#include "game/gmouse.h"
#include "game/gsound.h"
#include "plib/gnw/rect.h"
#include "game/intface.h"
#include "game/item.h"
#include "game/lip_sync.h"
#include "plib/gnw/memory.h"
#include "game/message.h"
#include "game/object.h"
#include "game/perk.h"
#include "game/proto.h"
#include "game/roll.h"
#include "game/scripts.h"
#include "game/skill.h"
#include "game/stat.h"
#include "plib/gnw/text.h"
#include "game/textobj.h"
#include "game/tile.h"
#include "plib/gnw/button.h"
#include "plib/gnw/gnw.h"
#include "plib/gnw/svga.h"

// NOTE: Rare case - as a compatibility measure with Community Edition, this
// define is actually an expression as original game used. However leaving it
// in the code produces to too many diffs in this file, which is not good for
// RE<->CE reconciliation.
#define GAME_DIALOG_WINDOW_WIDTH (scr_size.lrx - scr_size.ulx + 1)
#define GAME_DIALOG_WINDOW_HEIGHT 480

#define GAME_DIALOG_REPLY_WINDOW_X 135
#define GAME_DIALOG_REPLY_WINDOW_Y 225
#define GAME_DIALOG_REPLY_WINDOW_WIDTH 379
#define GAME_DIALOG_REPLY_WINDOW_HEIGHT 58

#define GAME_DIALOG_OPTIONS_WINDOW_X 127
#define GAME_DIALOG_OPTIONS_WINDOW_Y 335
#define GAME_DIALOG_OPTIONS_WINDOW_WIDTH 393
#define GAME_DIALOG_OPTIONS_WINDOW_HEIGHT 117

// NOTE: See `GAME_DIALOG_WINDOW_WIDTH`.
#define GAME_DIALOG_REVIEW_WINDOW_WIDTH (scr_size.lrx - scr_size.ulx + 1)
#define GAME_DIALOG_REVIEW_WINDOW_HEIGHT 480

#define DIALOG_REVIEW_ENTRIES_CAPACITY 80

#define DIALOG_OPTION_ENTRIES_CAPACITY 30

typedef enum GameDialogReviewWindowButton {
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_SCROLL_UP,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_SCROLL_DOWN,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_DONE,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_COUNT,
} GameDialogReviewWindowButton;

typedef enum GameDialogReviewWindowButtonFrm {
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_ARROW_UP_NORMAL,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_ARROW_UP_PRESSED,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_ARROW_DOWN_NORMAL,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_ARROW_DOWN_PRESSED,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_DONE_NORMAL,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_DONE_PRESSED,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_COUNT,
} GameDialogReviewWindowButtonFrm;

typedef enum GameDialogReaction {
    GAME_DIALOG_REACTION_GOOD = 49,
    GAME_DIALOG_REACTION_NEUTRAL = 50,
    GAME_DIALOG_REACTION_BAD = 51,
} GameDialogReaction;

typedef struct GameDialogReviewEntry {
    int replyMessageListId;
    int replyMessageId;
    // Can be NULL.
    char* replyText;
    int optionMessageListId;
    int optionMessageId;
    char* optionText;
} GameDialogReviewEntry;

typedef struct GameDialogOptionEntry {
    int messageListId;
    int messageId;
    int reaction;
    int proc;
    int btn;
    int field_14;
    char text[900];
    int field_39C;
} GameDialogOptionEntry;

typedef struct GameDialogBlock {
    Program* program;
    int replyMessageListId;
    int replyMessageId;
    int offset;

    // NOTE: The is something odd about next two members. There are 2700 bytes,
    // which is 3 x 900, but anywhere in the app only 900 characters is used.
    // The length of text in [DialogOptionEntry] is definitely 900 bytes. There
    // are two possible explanations:
    // - it's an array of 3 elements.
    // - there are three separate elements, two of which are not used, therefore
    // they are not referenced anywhere, but they take up their space.
    //
    // See `gdProcessChoice` for more info how this unreferenced range plays
    // important role.
    char replyText[900];
    char field_394[1800];
    GameDialogOptionEntry options[DIALOG_OPTION_ENTRIES_CAPACITY];
} GameDialogBlock;

// Provides button configuration for party member combat control and
// customization interface.
typedef struct GameDialogButtonData {
    int x;
    int y;
    int upFrmId;
    int downFrmId;
    int disabledFrmId;
    CacheEntry* upFrmHandle;
    CacheEntry* downFrmHandle;
    CacheEntry* disabledFrmHandle;
    int keyCode;
    int value;
} GameDialogButtonData;

typedef struct STRUCT_5189E4 {
    int messageId;
    int value;
} STRUCT_5189E4;

typedef enum PartyMemberCustomizationOption {
    PARTY_MEMBER_CUSTOMIZATION_OPTION_AREA_ATTACK_MODE,
    PARTY_MEMBER_CUSTOMIZATION_OPTION_RUN_AWAY_MODE,
    PARTY_MEMBER_CUSTOMIZATION_OPTION_BEST_WEAPON,
    PARTY_MEMBER_CUSTOMIZATION_OPTION_DISTANCE,
    PARTY_MEMBER_CUSTOMIZATION_OPTION_ATTACK_WHO,
    PARTY_MEMBER_CUSTOMIZATION_OPTION_CHEM_USE,
    PARTY_MEMBER_CUSTOMIZATION_OPTION_COUNT,
} PartyMemberCustomizationOption;

static int gdHide();
static int gdUnhide();
static int gdUnhideReply();
static int gdAddOption(int a1, int a2, int a3);
static int gdAddOptionStr(int a1, const char* a2, int a3);
static int gdReviewInit(int* win);
static int gdReviewExit(int* win);
static int gdReview();
static void gdReviewPressed(int btn, int keyCode);
static void gdReviewDisplay(int win, int origin);
static void gdReviewFree();
static int gdAddReviewReply(int messageListId, int messageId);
static int gdAddReviewReplyStr(const char* text);
static int gdAddReviewOptionChosen(int messageListId, int messageId);
static int gdAddReviewOptionChosenStr(const char* text);
static int gdProcessInit();
static void gdProcessCleanup();
static int gdProcessExit();
static void gdUpdateMula();
static int gdProcess();
static int gdProcessChoice(int a1);
static void gdProcessHighlight(int a1);
static void gdProcessUnHighlight(int a1);
static void gdProcessReply();
static void gdProcessUpdate();
static int gdCreateHeadWindow();
static void gdDestroyHeadWindow();
static void gdSetupFidget(int headFid, int reaction);
static void gdWaitForFidget();
static void gdPlayTransition(int a1);
static void reply_arrow_up(int btn, int a2);
static void reply_arrow_down(int btn, int a2);
static void reply_arrow_restore(int btn, int a2);
static void demo_copy_title(int win);
static void demo_copy_options(int win);
static void gDialogRefreshOptionsRect(int win, Rect* drawRect);
static void gdialog_bk();
static void gdialog_scroll_subwin(int a1, int a2, unsigned char* a3, unsigned char* a4, unsigned char* a5, int a6, int a7);
static int text_num_lines(const char* a1, int a2);
static int text_to_rect_wrapped(unsigned char* buffer, Rect* rect, char* string, int* a4, int height, int pitch, int color);
static int text_to_rect_func(unsigned char* buffer, Rect* rect, char* string, int* a4, int height, int pitch, int color, int a7);
static int gdialog_barter_create_win();
static void gdialog_barter_destroy_win();
static void gdialog_barter_cleanup_tables();
static int gdControlCreateWin();
static void gdControlDestroyWin();
static void gdControlUpdateInfo();
static void gdControlPressed(int a1, int a2);
static int gdPickAIUpdateMsg(Object* obj);
static int gdCanBarter();
static void gdControl();
static int gdCustomCreateWin();
static void gdCustomDestroyWin();
static void gdCustom();
static void gdCustomUpdateInfo();
static void gdCustomSelectRedraw(unsigned char* dest, int pitch, int type, int selectedIndex);
static int gdCustomSelect(int a1);
static void gdCustomUpdateSetting(int option, int value);
static void gdialog_barter_pressed(int btn, int a2);
static int gdialog_window_create();
static void gdialog_window_destroy();
static int talk_to_create_background_window();
static int talk_to_refresh_background_window();
static int talkToRefreshDialogWindowRect(Rect* rect);
static void talk_to_translucent_trans_buf_to_buf(unsigned char* src, int srcWidth, int srcHeight, int srcPitch, unsigned char* dest, int x, int y, int destPitch, unsigned char* a9, unsigned char* a10);
static void gdDisplayFrame(Art* art, int frame);
static void gdBlendTableInit();
static void gdBlendTableExit();

// 0x5186D4
static int dialog_state_fix = 0;

// 0x5186D8
static int gdNumOptions = 0;

// 0x5186DC
static int curReviewSlot = 0;

// 0x5186E0
static unsigned char* headWindowBuffer = NULL;

// 0x5186E4
static int gReplyWin = -1;

// 0x5186E8
static int gOptionWin = -1;

// 0x5186EC
static bool gdialog_window_created = false;

// 0x5186F0
static int boxesWereDisabled = 0;

// 0x5186F4
static int fidgetFID = 0;

// 0x5186F8
static CacheEntry* fidgetKey = NULL;

// 0x5186FC
static Art* fidgetFp = NULL;

// 0x518700
static int backgroundIndex = 2;

// 0x518704
static int lipsFID = 0;

// 0x518708
static CacheEntry* lipsKey = NULL;

// 0x51870C
static Art* lipsFp = NULL;

// 0x518710
static bool gdialog_speech_playing = false;

// 0x518714
static int dialogue_state = 0;

// 0x518718
static int dialogue_switch_mode = 0;

// 0x51871C
static int gdialog_state = -1;

// 0x518720
static bool gdDialogWentOff = false;

// 0x518724
static bool gdDialogTurnMouseOff = false;

// 0x518728
static int gdReenterLevel = 0;

// 0x51872C
static bool gdReplyTooBig = false;

// 0x518730
static Object* peon_table_obj = NULL;

// 0x518734
static Object* barterer_table_obj = NULL;

// 0x518738
static Object* barterer_temp_obj = NULL;

// 0x51873C
static int gdBarterMod = 0;

// 0x518740
static int dialogueBackWindow = -1;

// 0x518744
static int dialogueWindow = -1;

// 0x518748
static Rect backgrndRects[8] = {
    { 126, 14, 152, 40 },
    { 488, 14, 514, 40 },
    { 126, 188, 152, 214 },
    { 488, 188, 514, 214 },
    { 152, 14, 488, 24 },
    { 152, 204, 488, 214 },
    { 126, 40, 136, 188 },
    { 504, 40, 514, 188 },
};

// 0x5187C8
static int talk_need_to_center = 1;

// 0x5187CC
static bool can_start_new_fidget = false;

// 0x5187D0
static int gd_replyWin = -1;

// 0x5187D4
static int gd_optionsWin = -1;

// 0x5187D8
static int gDialogMusicVol = -1;

// 0x5187DC
static int gdCenterTile = -1;

// 0x5187E0
static int gdPlayerTile = -1;

// 0x5187E4
unsigned char* light_BlendTable = NULL;

// 0x5187E8
unsigned char* dark_BlendTable = NULL;

// 0x5187EC
static int dialogue_just_started = 0;

// 0x5187F0
static int dialogue_seconds_since_last_input = 0;

// 0x5187F4
static CacheEntry* reviewKeys[GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_COUNT] = {
    INVALID_CACHE_ENTRY,
    INVALID_CACHE_ENTRY,
    INVALID_CACHE_ENTRY,
    INVALID_CACHE_ENTRY,
    INVALID_CACHE_ENTRY,
    INVALID_CACHE_ENTRY,
};

// 0x51880C
static CacheEntry* reviewBackKey = INVALID_CACHE_ENTRY;

// 0x518810
static CacheEntry* reviewDispBackKey = INVALID_CACHE_ENTRY;

// 0x518814
static unsigned char* reviewDispBuf = NULL;

// 0x518818
static int reviewFidWids[GAME_DIALOG_REVIEW_WINDOW_BUTTON_COUNT] = {
    35,
    35,
    82,
};

// 0x518824
static int reviewFidLens[GAME_DIALOG_REVIEW_WINDOW_BUTTON_COUNT] = {
    35,
    37,
    46,
};

// 0x518830
static int reviewFids[GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_COUNT] = {
    89, // di_bgdn1.frm - dialog big down arrow
    90, // di_bgdn2.frm - dialog big down arrow
    87, // di_bgup1.frm - dialog big up arrow
    88, // di_bgup2.frm - dialog big up arrow
    91, // di_done1.frm - dialog big done button up
    92, // di_done2.frm - dialog big done button down
};

// 0x518848
Object* dialog_target = NULL;

// 0x51884C
bool dialog_target_is_party = false;

// 0x518850
int dialogue_head = 0;

// 0x518854
int dialogue_scr_id = -1;

// Maps phoneme to talking head frame.
//
// 0x518858
static int head_phoneme_lookup[PHONEME_COUNT] = {
    0,
    3,
    1,
    1,
    3,
    1,
    1,
    1,
    7,
    8,
    7,
    3,
    1,
    8,
    1,
    7,
    7,
    6,
    6,
    2,
    2,
    2,
    2,
    4,
    4,
    5,
    5,
    2,
    2,
    2,
    2,
    2,
    6,
    2,
    2,
    5,
    8,
    2,
    2,
    2,
    2,
    8,
};

// 0x51890C
static const char* react_strs[3] = {
    "Said Good",
    "Said Neutral",
    "Said Bad",
};

// 0x518918
static int dialogue_subwin_len = 0;

// 0x51891C
static GameDialogButtonData control_button_info[5] = {
    { 438, 37, 397, 395, 396, NULL, NULL, NULL, 2098, 4 },
    { 438, 67, 394, 392, 393, NULL, NULL, NULL, 2103, 3 },
    { 438, 96, 406, 404, 405, NULL, NULL, NULL, 2102, 2 },
    { 438, 126, 400, 398, 399, NULL, NULL, NULL, 2111, 1 },
    { 438, 156, 403, 401, 402, NULL, NULL, NULL, 2099, 0 },
};

// 0x5189E4
static STRUCT_5189E4 custom_settings[PARTY_MEMBER_CUSTOMIZATION_OPTION_COUNT][6] = {
    {
        { 100, AREA_ATTACK_MODE_ALWAYS }, // Always!
        { 101, AREA_ATTACK_MODE_SOMETIMES }, // Sometimes, don't worry about hitting me
        { 102, AREA_ATTACK_MODE_BE_SURE }, // Be sure you won't hit me
        { 103, AREA_ATTACK_MODE_BE_CAREFUL }, // Be careful not to hit me
        { 104, AREA_ATTACK_MODE_BE_ABSOLUTELY_SURE }, // Be absolutely sure you won't hit me
        { -1, 0 },
    },
    {
        { 200, RUN_AWAY_MODE_COWARD - 1 }, // Abject coward
        { 201, RUN_AWAY_MODE_FINGER_HURTS - 1 }, // Your finger hurts
        { 202, RUN_AWAY_MODE_BLEEDING - 1 }, // You're bleeding a bit
        { 203, RUN_AWAY_MODE_NOT_FEELING_GOOD - 1 }, // Not feeling good
        { 204, RUN_AWAY_MODE_TOURNIQUET - 1 }, // You need a tourniquet
        { 205, RUN_AWAY_MODE_NEVER - 1 }, // Never!
    },
    {
        { 300, BEST_WEAPON_NO_PREF }, // None
        { 301, BEST_WEAPON_MELEE }, // Melee
        { 302, BEST_WEAPON_MELEE_OVER_RANGED }, // Melee then ranged
        { 303, BEST_WEAPON_RANGED_OVER_MELEE }, // Ranged then melee
        { 304, BEST_WEAPON_RANGED }, // Ranged
        { 305, BEST_WEAPON_UNARMED }, // Unarmed
    },
    {
        { 400, DISTANCE_STAY_CLOSE }, // Stay close to me
        { 401, DISTANCE_CHARGE }, // Charge!
        { 402, DISTANCE_SNIPE }, // Snipe the enemy
        { 403, DISTANCE_ON_YOUR_OWN }, // On your own
        { 404, DISTANCE_STAY }, // Say where you are
        { -1, 0 },
    },
    {
        { 500, ATTACK_WHO_WHOMEVER_ATTACKING_ME }, // Whomever is attacking me
        { 501, ATTACK_WHO_STRONGEST }, // The strongest
        { 502, ATTACK_WHO_WEAKEST }, // The weakest
        { 503, ATTACK_WHO_WHOMEVER }, // Whomever you want
        { 504, ATTACK_WHO_CLOSEST }, // Whoever is closest
        { -1, 0 },
    },
    {
        { 600, CHEM_USE_CLEAN }, // I'm clean
        { 601, CHEM_USE_STIMS_WHEN_HURT_LITTLE }, // Stimpacks when hurt a bit
        { 602, CHEM_USE_STIMS_WHEN_HURT_LOTS }, // Stimpacks when hurt a lot
        { 603, CHEM_USE_SOMETIMES }, // Any drug some of the time
        { 604, CHEM_USE_ANYTIME }, // Any drug any time
        { -1, 0 },
    },
};

// 0x518B04
static GameDialogButtonData custom_button_info[PARTY_MEMBER_CUSTOMIZATION_OPTION_COUNT] = {
    { 95, 9, 410, 409, -1, NULL, NULL, NULL, 0, 0 },
    { 96, 38, 416, 415, -1, NULL, NULL, NULL, 1, 0 },
    { 96, 68, 418, 417, -1, NULL, NULL, NULL, 2, 0 },
    { 96, 98, 414, 413, -1, NULL, NULL, NULL, 3, 0 },
    { 96, 127, 408, 407, -1, NULL, NULL, NULL, 4, 0 },
    { 96, 157, 412, 411, -1, NULL, NULL, NULL, 5, 0 },
};

// 0x58EA80
static int custom_current_selected[PARTY_MEMBER_CUSTOMIZATION_OPTION_COUNT];

// custom.msg
//
// 0x58EA98
static MessageList custom_msg_file;

// 0x58EAA0
unsigned char light_GrayTable[256];

// 0x58EBA0
unsigned char dark_GrayTable[256];

// 0x58ECA0
static unsigned char* backgrndBufs[8];

// 0x58ECC0
static Rect optionRect;

// 0x58ECD0
static Rect replyRect;

// 0x58ECE0
static GameDialogReviewEntry reviewList[DIALOG_REVIEW_ENTRIES_CAPACITY];

// 0x58F460
static int custom_buttons_start;

// 0x58F464
static int control_buttons_start;

// 0x58F468
static int reviewOldFont;

// 0x58F46C
static CacheEntry* dialog_red_button_up_key;

// 0x58F470
static int gdialog_buttons[9];

// 0x58F494
static CacheEntry* upper_hi_key;

// 0x58F498
static CacheEntry* gdialog_review_up_key;

// 0x58F49C
static int lower_hi_len;

// 0x58F4A0
static CacheEntry* gdialog_review_down_key;

// 0x58F4A4
static unsigned char* dialog_red_button_down_buf;

// 0x58F4A8
static int lower_hi_wid;

// 0x58F4AC
static unsigned char* dialog_red_button_up_buf;

// 0x58F4B0
static int upper_hi_wid;

// Yellow highlight blick effect.
//
// 0x58F4B4
static Art* lower_hi_fp;

// 0x58F4B8
static int upper_hi_len;

// 0x58F4BC
static CacheEntry* dialog_red_button_down_key;

// 0x58F4C0
static CacheEntry* lower_hi_key;

// White highlight blick effect.
//
// This effect appears at the top-right corner on dialog display. Together with
// [gDialogLowerHighlight] it gives an effect of depth of the monitor.
//
// 0x58F4C4
static Art* upper_hi_fp;

// 0x58F4C8
static int oldFont;

// 0x58F4CC
static unsigned int fidgetLastTime;

// 0x58F4D0
static int fidgetAnim;

// 0x58F4D4
static GameDialogBlock dialogBlock;

// 0x596C30
static int talkOldFont;

// 0x596C34
static unsigned int fidgetTocksPerFrame;

// 0x596C38
static int fidgetFrameCounter;

// 0x444D1C
int gdialogInit()
{
    return 0;
}

// 0x444D20
int gdialogReset()
{
    gdialogFreeSpeech();
    return 0;
}

// 0x444D20.
int gdialogExit()
{
    gdialogFreeSpeech();
    return 0;
}

// 0x444D2C
bool gdialogActive()
{
    return dialog_state_fix != 0;
}

// gdialogEnter
// 0x444D3C
void gdialogEnter(Object* a1, int a2)
{
    if (a1 == NULL) {
        debug_printf("\nError: gdialogEnter: target was NULL!");
        return;
    }

    gdDialogWentOff = false;

    if (isInCombat()) {
        return;
    }

    if (a1->sid == -1) {
        return;
    }

    if (PID_TYPE(a1->pid) != OBJ_TYPE_ITEM && SID_TYPE(a1->sid) != SCRIPT_TYPE_SPATIAL) {
        MessageListItem messageListItem;

        int rc = action_can_talk_to(obj_dude, a1);
        if (rc == -1) {
            // You can't see there.
            messageListItem.num = 660;
            if (message_search(&proto_main_msg_file, &messageListItem)) {
                if (a2) {
                    display_print(messageListItem.text);
                } else {
                    debug_printf(messageListItem.text);
                }
            } else {
                debug_printf("\nError: gdialog: Can't find message!");
            }
            return;
        }

        if (rc == -2) {
            // Too far away.
            messageListItem.num = 661;
            if (message_search(&proto_main_msg_file, &messageListItem)) {
                if (a2) {
                    display_print(messageListItem.text);
                } else {
                    debug_printf(messageListItem.text);
                }
            } else {
                debug_printf("\nError: gdialog: Can't find message!");
            }
            return;
        }
    }

    gdCenterTile = tile_center_tile;
    gdBarterMod = 0;
    gdPlayerTile = obj_dude->tile;
    map_disable_bk_processes();

    dialog_state_fix = 1;
    dialog_target = a1;
    dialog_target_is_party = isPartyMember(a1);

    dialogue_just_started = 1;

    if (a1->sid != -1) {
        exec_script_proc(a1->sid, SCRIPT_PROC_TALK);
    }

    Script* script;
    if (scr_ptr(a1->sid, &script) == -1) {
        gmouse_3d_on();
        map_enable_bk_processes();
        scr_exec_map_update_scripts();
        dialog_state_fix = 0;
        return;
    }

    if (script->scriptOverrides || dialogue_state != 4) {
        dialogue_just_started = 0;
        map_enable_bk_processes();
        scr_exec_map_update_scripts();
        dialog_state_fix = 0;
        return;
    }

    gdialogFreeSpeech();

    if (gdialog_state == 1) {
        // TODO: Not sure about these conditions.
        if (dialogue_switch_mode == 2) {
            gdialog_window_destroy();
        } else if (dialogue_switch_mode == 8) {
            gdialog_window_destroy();
        } else if (dialogue_switch_mode == 11) {
            gdialog_window_destroy();
        } else {
            if (dialogue_switch_mode == gdialog_state) {
                gdialog_barter_destroy_win();
            } else if (dialogue_state == gdialog_state) {
                gdialog_window_destroy();
            } else if (dialogue_state == a2) {
                gdialog_barter_destroy_win();
            }
        }
        gdialogExitFromScript();
    }

    gdialog_state = 0;
    dialogue_state = 0;

    int tile = obj_dude->tile;
    if (gdPlayerTile != tile) {
        gdCenterTile = tile;
    }

    if (gdDialogWentOff) {
        tile_scroll_to(gdCenterTile, 2);
    }

    map_enable_bk_processes();
    scr_exec_map_update_scripts();

    dialog_state_fix = 0;
}

// 0x444FE4
void gdialogSystemEnter()
{
    game_state_update();

    gdDialogTurnMouseOff = true;

    soundContinueAll();
    gdialogEnter(dialog_target, 0);
    soundContinueAll();

    if (gdPlayerTile != obj_dude->tile) {
        gdCenterTile = obj_dude->tile;
    }

    if (gdDialogWentOff) {
        tile_scroll_to(gdCenterTile, 2);
    }

    game_state_request(GAME_STATE_2);

    game_state_update();
}

// 0x445050
void gdialogSetupSpeech(const char* audioFileName)
{
    if (audioFileName == NULL) {
        debug_printf("\nGDialog: Bleep!");
        gsound_play_sfx_file("censor");
        return;
    }

    char name[16];
    if (art_get_base_name(OBJ_TYPE_HEAD, dialogue_head & 0xFFF, name) == -1) {
        return;
    }

    if (lips_load_file(audioFileName, name) == -1) {
        return;
    }

    gdialog_speech_playing = true;

    lips_play_speech();

    debug_printf("Starting lipsynch speech");
}

// 0x4450C4
void gdialogFreeSpeech()
{
    if (gdialog_speech_playing) {
        debug_printf("Ending lipsynch system");
        gdialog_speech_playing = false;

        lips_free_speech();
    }
}

// 0x4450EC
int gdialogEnableBK()
{
    add_bk_process(gdialog_bk);
    return 0;
}

// 0x4450FC
int gdialogDisableBK()
{
    remove_bk_process(gdialog_bk);
    return 0;
}

// 0x44510C
int gdialogInitFromScript(int headFid, int reaction)
{
    if (dialogue_state == 1) {
        return -1;
    }

    if (gdialog_state == 1) {
        return 0;
    }

    anim_stop();

    boxesWereDisabled = disable_box_bar_win();
    dialog_target_is_party = isPartyMember(dialog_target);
    oldFont = text_curr();
    text_font(101);
    dialogSetReplyWindow(135, 225, 379, 58, NULL);
    dialogSetReplyColor(0.3f, 0.3f, 0.3f);
    dialogSetOptionWindow(127, 335, 393, 117, NULL);
    dialogSetOptionColor(0.2f, 0.2f, 0.2f);
    dialogTitle(NULL);
    dialogRegisterWinDrawCallbacks(demo_copy_title, demo_copy_options);
    gdBlendTableInit();
    cycle_disable();
    if (gdDialogTurnMouseOff) {
        gmouse_disable(0);
    }
    gmouse_3d_off();
    gmouse_set_cursor(MOUSE_CURSOR_ARROW);
    text_object_reset();

    if (PID_TYPE(dialog_target->pid) != OBJ_TYPE_ITEM) {
        tile_scroll_to(dialog_target->tile, 2);
    }

    talk_need_to_center = 1;

    gdCreateHeadWindow();
    add_bk_process(gdialog_bk);
    gdSetupFidget(headFid, reaction);
    gdialog_state = 1;
    gmouse_disable_scrolling();

    if (headFid == -1) {
        gDialogMusicVol = gsound_background_volume_get_set(gDialogMusicVol / 2);
    } else {
        gDialogMusicVol = -1;
        gsound_background_stop();
    }

    gdDialogWentOff = true;

    return 0;
}

// 0x445298
int gdialogExitFromScript()
{
    if (dialogue_switch_mode == 2 || dialogue_switch_mode == 8 || dialogue_switch_mode == 11) {
        return -1;
    }

    if (gdialog_state == 0) {
        return 0;
    }

    gdialogFreeSpeech();
    gdReviewFree();
    remove_bk_process(gdialog_bk);

    if (PID_TYPE(dialog_target->pid) != OBJ_TYPE_ITEM) {
        if (gdPlayerTile != obj_dude->tile) {
            gdCenterTile = obj_dude->tile;
        }
        tile_scroll_to(gdCenterTile, 2);
    }

    gdDestroyHeadWindow();

    text_font(oldFont);

    if (fidgetFp != NULL) {
        art_ptr_unlock(fidgetKey);
        fidgetFp = NULL;
    }

    if (lipsKey != NULL) {
        if (art_ptr_unlock(lipsKey) == -1) {
            debug_printf("Failure unlocking lips frame!\n");
        }
        lipsKey = NULL;
        lipsFp = NULL;
        lipsFID = 0;
    }

    // NOTE: Uninline.
    gdBlendTableExit();

    gdialog_state = 0;
    dialogue_state = 0;

    cycle_enable();

    if (!game_ui_is_disabled()) {
        gmouse_enable_scrolling();
    }

    if (gDialogMusicVol == -1) {
        gsound_background_restart_last(11);
    } else {
        gsound_background_volume_set(gDialogMusicVol);
    }

    if (boxesWereDisabled) {
        enable_box_bar_win();
    }

    boxesWereDisabled = 0;

    if (gdDialogTurnMouseOff) {
        if (!game_ui_is_disabled()) {
            gmouse_enable();
        }

        gdDialogTurnMouseOff = 0;
    }

    if (!game_ui_is_disabled()) {
        gmouse_3d_on();
    }

    gdDialogWentOff = true;

    return 0;
}

// 0x445438
void gdialogSetBackground(int a1)
{
    if (a1 != -1) {
        backgroundIndex = a1;
    }
}

// Renders supplementary message in reply area of the dialog.
//
// 0x445448
void gdialogDisplayMsg(char* msg)
{
    if (gd_replyWin == -1) {
        debug_printf("\nError: Reply window doesn't exist!");
        return;
    }

    replyRect.ulx = 5;
    replyRect.uly = 10;
    replyRect.lrx = 374;
    replyRect.lry = 58;
    demo_copy_title(gReplyWin);

    unsigned char* windowBuffer = win_get_buf(gReplyWin);
    int lineHeight = text_height();

    int a4 = 0;

    // NOTE: Uninline.
    text_to_rect_wrapped(windowBuffer,
        &replyRect,
        msg,
        &a4,
        lineHeight,
        379,
        colorTable[992] | 0x2000000);

    win_show(gd_replyWin);
    win_draw(gReplyWin);
}

// 0x4454FC
int gdialogStart()
{
    curReviewSlot = 0;
    gdNumOptions = 0;
    return 0;
}

// 0x445510
int gdialogSayMessage()
{
    mouse_show();
    gdialogGo();

    gdNumOptions = 0;
    dialogBlock.replyMessageListId = -1;

    return 0;
}

// NOTE: If you look at the scripts handlers, my best guess that their intention
// was to allow scripters to specify proc names instead of proc addresses. They
// dropped this idea, probably because they've updated their own compiler, or
// maybe there was not enough time to complete it. Any way, [procedure] is the
// identifier of the procedure in the script, but it is silently ignored.
//
// 0x445538
int gdialogOption(int messageListId, int messageId, const char* proc, int reaction)
{
    dialogBlock.options[gdNumOptions].proc = 0;

    return gdAddOption(messageListId, messageId, reaction);
}

// NOTE: If you look at the script handlers, my best guess that their intention
// was to allow scripters to specify proc names instead of proc addresses. They
// dropped this idea, probably because they've updated their own compiler, or
// maybe there was not enough time to complete it. Any way, [procedure] is the
// identifier of the procedure in the script, but it is silently ignored.
//
// 0x445578
int gdialogOptionStr(int messageListId, const char* text, const char* proc, int reaction)
{
    dialogBlock.options[gdNumOptions].proc = 0;

    return gdAddOptionStr(messageListId, text, reaction);
}

// 0x4455B8
int gdialogOptionProc(int messageListId, int messageId, int proc, int reaction)
{
    dialogBlock.options[gdNumOptions].proc = proc;

    return gdAddOption(messageListId, messageId, reaction);
}

// 0x4455FC
int gdialogOptionProcStr(int messageListId, const char* text, int proc, int reaction)
{
    dialogBlock.options[gdNumOptions].proc = proc;

    return gdAddOptionStr(messageListId, text, reaction);
}

// 0x445640
int gdialogReply(Program* program, int messageListId, int messageId)
{
    gdAddReviewReply(messageListId, messageId);

    dialogBlock.program = program;
    dialogBlock.replyMessageListId = messageListId;
    dialogBlock.replyMessageId = messageId;
    dialogBlock.offset = 0;
    dialogBlock.replyText[0] = '\0';
    gdNumOptions = 0;

    return 0;
}

// 0x44567C
int gdialogReplyStr(Program* program, int messageListId, const char* text)
{
    gdAddReviewReplyStr(text);

    dialogBlock.program = program;
    dialogBlock.offset = 0;
    dialogBlock.replyMessageListId = -4;
    dialogBlock.replyMessageId = -4;

    strcpy(dialogBlock.replyText, text);

    gdNumOptions = 0;

    return 0;
}

// 0x4456D8
int gdialogGo()
{
    if (dialogBlock.replyMessageListId == -1) {
        return 0;
    }

    int rc = 0;

    if (gdNumOptions < 1) {
        dialogBlock.options[gdNumOptions].proc = 0;

        if (gdAddOption(-1, -1, 50) == -1) {
            interpretError("Error setting option.");
            rc = -1;
        }
    }

    if (rc != -1) {
        rc = gdProcess();
    }

    gdNumOptions = 0;

    return rc;
}

// 0x445764
void gdialogUpdatePartyStatus()
{
    if (dialogue_state != 1) {
        return;
    }

    bool is_party = isPartyMember(dialog_target);
    if (is_party == dialog_target_is_party) {
        return;
    }

    // NOTE: Uninline.
    gdHide();

    gdialog_window_destroy();

    dialog_target_is_party = is_party;

    gdialog_window_create();

    // NOTE: Uninline.
    gdUnhide();
}

// NOTE: Inlined.
//
// 0x4457EC
static int gdHide()
{
    if (gd_replyWin != -1) {
        win_hide(gd_replyWin);
    }

    if (gd_optionsWin != -1) {
        win_hide(gd_optionsWin);
    }

    return 0;
}

// NOTE: Inlined.
//
// 0x445818
static int gdUnhide()
{
    if (gd_replyWin != -1) {
        win_show(gd_replyWin);
    }

    if (gd_optionsWin != -1) {
        win_show(gd_optionsWin);
    }

    return 0;
}

// NOTE: Unused.
//
// 0x445844
static int gdUnhideReply()
{
    if (gd_replyWin != -1) {
        win_show(gd_replyWin);
    }

    return 0;
}

// 0x44585C
static int gdAddOption(int messageListId, int messageId, int reaction)
{
    if (gdNumOptions >= DIALOG_OPTION_ENTRIES_CAPACITY) {
        debug_printf("\nError: dialog: Ran out of options!");
        return -1;
    }

    GameDialogOptionEntry* optionEntry = &(dialogBlock.options[gdNumOptions]);
    optionEntry->messageListId = messageListId;
    optionEntry->messageId = messageId;
    optionEntry->reaction = reaction;
    optionEntry->btn = -1;
    optionEntry->text[0] = '\0';

    gdNumOptions++;

    return 0;
}

// 0x4458BC
static int gdAddOptionStr(int messageListId, const char* text, int reaction)
{
    if (gdNumOptions >= DIALOG_OPTION_ENTRIES_CAPACITY) {
        debug_printf("\nError: dialog: Ran out of options!");
        return -1;
    }

    GameDialogOptionEntry* optionEntry = &(dialogBlock.options[gdNumOptions]);
    optionEntry->messageListId = -4;
    optionEntry->messageId = -4;
    optionEntry->reaction = reaction;
    optionEntry->btn = -1;
    sprintf(optionEntry->text, "%c %s", '\x95', text);

    gdNumOptions++;

    return 0;
}

// 0x445938
static int gdReviewInit(int* win)
{
    if (gdialog_speech_playing) {
        if (soundPlaying(lip_info.sound)) {
            gdialogFreeSpeech();
        }
    }

    reviewOldFont = text_curr();

    if (win == NULL) {
        return -1;
    }

    int reviewWindowX = 0;
    int reviewWindowY = 0;
    *win = win_add(reviewWindowX,
        reviewWindowY,
        GAME_DIALOG_REVIEW_WINDOW_WIDTH,
        GAME_DIALOG_REVIEW_WINDOW_HEIGHT,
        256,
        WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
    if (*win == -1) {
        return -1;
    }

    int fid = art_id(OBJ_TYPE_INTERFACE, 102, 0, 0, 0);
    unsigned char* backgroundFrmData = art_ptr_lock_data(fid, 0, 0, &reviewBackKey);
    if (backgroundFrmData == NULL) {
        win_delete(*win);
        *win = -1;
        return -1;
    }

    unsigned char* windowBuffer = win_get_buf(*win);
    buf_to_buf(backgroundFrmData,
        GAME_DIALOG_REVIEW_WINDOW_WIDTH,
        GAME_DIALOG_REVIEW_WINDOW_HEIGHT,
        GAME_DIALOG_REVIEW_WINDOW_WIDTH,
        windowBuffer,
        GAME_DIALOG_REVIEW_WINDOW_WIDTH);

    art_ptr_unlock(reviewBackKey);
    reviewBackKey = INVALID_CACHE_ENTRY;

    unsigned char* buttonFrmData[GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_COUNT];

    int index;
    for (index = 0; index < GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_COUNT; index++) {
        int fid = art_id(OBJ_TYPE_INTERFACE, reviewFids[index], 0, 0, 0);
        buttonFrmData[index] = art_ptr_lock_data(fid, 0, 0, &(reviewKeys[index]));
        if (buttonFrmData[index] == NULL) {
            break;
        }
    }

    if (index != GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_COUNT) {
        gdReviewExit(win);
        return -1;
    }

    int upBtn = win_register_button(*win,
        475,
        152,
        reviewFidWids[GAME_DIALOG_REVIEW_WINDOW_BUTTON_SCROLL_UP],
        reviewFidLens[GAME_DIALOG_REVIEW_WINDOW_BUTTON_SCROLL_UP],
        -1,
        -1,
        -1,
        KEY_ARROW_UP,
        buttonFrmData[GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_ARROW_UP_NORMAL],
        buttonFrmData[GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_ARROW_UP_PRESSED],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (upBtn == -1) {
        gdReviewExit(win);
        return -1;
    }

    win_register_button_sound_func(upBtn, gsound_med_butt_press, gsound_med_butt_release);

    int downBtn = win_register_button(*win,
        475,
        191,
        reviewFidWids[GAME_DIALOG_REVIEW_WINDOW_BUTTON_SCROLL_DOWN],
        reviewFidLens[GAME_DIALOG_REVIEW_WINDOW_BUTTON_SCROLL_DOWN],
        -1,
        -1,
        -1,
        KEY_ARROW_DOWN,
        buttonFrmData[GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_ARROW_DOWN_NORMAL],
        buttonFrmData[GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_ARROW_DOWN_PRESSED],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (downBtn == -1) {
        gdReviewExit(win);
        return -1;
    }

    win_register_button_sound_func(downBtn, gsound_med_butt_press, gsound_med_butt_release);

    int doneBtn = win_register_button(*win,
        499,
        398,
        reviewFidWids[GAME_DIALOG_REVIEW_WINDOW_BUTTON_DONE],
        reviewFidLens[GAME_DIALOG_REVIEW_WINDOW_BUTTON_DONE],
        -1,
        -1,
        -1,
        KEY_ESCAPE,
        buttonFrmData[GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_DONE_NORMAL],
        buttonFrmData[GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_DONE_PRESSED],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (doneBtn == -1) {
        gdReviewExit(win);
        return -1;
    }

    win_register_button_sound_func(doneBtn, gsound_red_butt_press, gsound_red_butt_release);

    text_font(101);

    win_draw(*win);

    remove_bk_process(gdialog_bk);

    int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 102, 0, 0, 0);
    reviewDispBuf = art_ptr_lock_data(backgroundFid, 0, 0, &reviewDispBackKey);
    if (reviewDispBuf == NULL) {
        gdReviewExit(win);
        return -1;
    }

    return 0;
}

// 0x445C18
static int gdReviewExit(int* win)
{
    add_bk_process(gdialog_bk);

    for (int index = 0; index < GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_COUNT; index++) {
        if (reviewKeys[index] != INVALID_CACHE_ENTRY) {
            art_ptr_unlock(reviewKeys[index]);
            reviewKeys[index] = INVALID_CACHE_ENTRY;
        }
    }

    if (reviewDispBackKey != INVALID_CACHE_ENTRY) {
        art_ptr_unlock(reviewDispBackKey);
        reviewDispBackKey = INVALID_CACHE_ENTRY;
        reviewDispBuf = NULL;
    }

    text_font(reviewOldFont);

    if (win == NULL) {
        return -1;
    }

    win_delete(*win);
    *win = -1;

    return 0;
}

// 0x445CA0
static int gdReview()
{
    int win;

    if (gdReviewInit(&win) == -1) {
        debug_printf("\nError initializing review window!");
        return -1;
    }

    // probably current top line or something like this, which is used to scroll
    int v1 = 0;
    gdReviewDisplay(win, v1);

    while (true) {
        int keyCode = get_input();
        if (keyCode == 17 || keyCode == 24 || keyCode == 324) {
            game_quit_with_confirm();
        }

        if (game_user_wants_to_quit != 0 || keyCode == KEY_ESCAPE) {
            break;
        }

        // likely scrolling
        if (keyCode == 328) {
            v1 -= 1;
            if (v1 >= 0) {
                gdReviewDisplay(win, v1);
            } else {
                v1 = 0;
            }
        } else if (keyCode == 336) {
            v1 += 1;
            if (v1 <= curReviewSlot - 1) {
                gdReviewDisplay(win, v1);
            } else {
                v1 = curReviewSlot - 1;
            }
        }
    }

    if (gdReviewExit(&win) == -1) {
        return -1;
    }

    return 0;
}

// NOTE: Uncollapsed 0x445CA0 with different signature.
static void gdReviewPressed(int btn, int keyCode)
{
    gdReview();
}

// 0x445D44
static void gdReviewDisplay(int win, int origin)
{
    Rect entriesRect;
    entriesRect.ulx = 113;
    entriesRect.uly = 76;
    entriesRect.lrx = 422;
    entriesRect.lry = 418;

    int v20 = text_height() + 2;
    unsigned char* windowBuffer = win_get_buf(win);
    if (windowBuffer == NULL) {
        debug_printf("\nError: gdialog: review: can't find buffer!");
        return;
    }

    int width = GAME_DIALOG_WINDOW_WIDTH;
    buf_to_buf(
        reviewDispBuf + width * entriesRect.uly + entriesRect.ulx,
        width,
        entriesRect.lry - entriesRect.uly + 15,
        width,
        windowBuffer + width * entriesRect.uly + entriesRect.ulx,
        width);

    int y = 76;
    for (int index = origin; index < curReviewSlot; index++) {
        GameDialogReviewEntry* dialogReviewEntry = &(reviewList[index]);

        char name[60];
        sprintf(name, "%s:", object_name(dialog_target));
        win_print(win, name, 180, 88, y, colorTable[992] | 0x2000000);
        entriesRect.uly += v20;

        char* replyText;
        if (dialogReviewEntry->replyMessageListId <= -3) {
            replyText = dialogReviewEntry->replyText;
        } else {
            replyText = scr_get_msg_str(dialogReviewEntry->replyMessageListId, dialogReviewEntry->replyMessageId);
        }

        if (replyText == NULL) {
            GNWSystemError("\nGDialog::Error Grabbing text message!");
            exit(1);
        }

        // NOTE: Uninline.
        y = text_to_rect_wrapped(windowBuffer + 113,
                &entriesRect,
                replyText,
                NULL,
                text_height(),
                640,
                colorTable[768] | 0x2000000);

        if (dialogReviewEntry->optionMessageListId != -3) {
            sprintf(name, "%s:", object_name(obj_dude));
            win_print(win, name, 180, 88, y, colorTable[21140] | 0x2000000);
            entriesRect.uly += v20;

            char* optionText;
            if (dialogReviewEntry->optionMessageListId <= -3) {
                optionText = dialogReviewEntry->optionText;
            } else {
                optionText = scr_get_msg_str(dialogReviewEntry->optionMessageListId, dialogReviewEntry->optionMessageId);
            }

            if (optionText == NULL) {
                GNWSystemError("\nGDialog::Error Grabbing text message!");
                exit(1);
            }

            // NOTE: Uninline.
            y = text_to_rect_wrapped(windowBuffer + 113,
                    &entriesRect,
                    optionText,
                    NULL,
                    text_height(),
                    640,
                    colorTable[15855] | 0x2000000);
        }

        if (y >= 407) {
            break;
        }
    }

    entriesRect.ulx = 88;
    entriesRect.uly = 76;
    entriesRect.lry += 14;
    entriesRect.lrx = 434;
    win_draw_rect(win, &entriesRect);
}

// 0x445FDC
static void gdReviewFree()
{
    for (int index = 0; index < curReviewSlot; index++) {
        GameDialogReviewEntry* entry = &(reviewList[index]);
        entry->replyMessageListId = 0;
        entry->replyMessageId = 0;

        if (entry->replyText != NULL) {
            mem_free(entry->replyText);
            entry->replyText = NULL;
        }

        entry->optionMessageListId = 0;
        entry->optionMessageId = 0;
    }
}

// 0x446040
static int gdAddReviewReply(int messageListId, int messageId)
{
    if (curReviewSlot >= DIALOG_REVIEW_ENTRIES_CAPACITY) {
        debug_printf("\nError: Ran out of review slots!");
        return -1;
    }

    GameDialogReviewEntry* entry = &(reviewList[curReviewSlot]);
    entry->replyMessageListId = messageListId;
    entry->replyMessageId = messageId;

    // NOTE: I'm not sure why there are two consequtive assignments.
    entry->optionMessageListId = -1;
    entry->optionMessageId = -1;

    entry->optionMessageListId = -3;
    entry->optionMessageId = -3;

    curReviewSlot++;

    return 0;
}

// 0x4460B4
static int gdAddReviewReplyStr(const char* string)
{
    if (curReviewSlot >= DIALOG_REVIEW_ENTRIES_CAPACITY) {
        debug_printf("\nError: Ran out of review slots!");
        return -1;
    }

    GameDialogReviewEntry* entry = &(reviewList[curReviewSlot]);
    entry->replyMessageListId = -4;
    entry->replyMessageId = -4;

    if (entry->replyText != NULL) {
        mem_free(entry->replyText);
        entry->replyText = NULL;
    }

    entry->replyText = (char*)mem_malloc(strlen(string) + 1);
    strcpy(entry->replyText, string);

    entry->optionMessageListId = -3;
    entry->optionMessageId = -3;
    entry->optionText = NULL;

    curReviewSlot++;

    return 0;
}

// 0x4461A4
static int gdAddReviewOptionChosen(int messageListId, int messageId)
{
    if (curReviewSlot >= DIALOG_REVIEW_ENTRIES_CAPACITY) {
        debug_printf("\nError: Ran out of review slots!");
        return -1;
    }

    GameDialogReviewEntry* entry = &(reviewList[curReviewSlot - 1]);
    entry->optionMessageListId = messageListId;
    entry->optionMessageId = messageId;
    entry->optionText = NULL;

    return 0;
}

// 0x4461F0
static int gdAddReviewOptionChosenStr(const char* string)
{
    if (curReviewSlot >= DIALOG_REVIEW_ENTRIES_CAPACITY) {
        debug_printf("\nError: Ran out of review slots!");
        return -1;
    }

    GameDialogReviewEntry* entry = &(reviewList[curReviewSlot - 1]);
    entry->optionMessageListId = -4;
    entry->optionMessageId = -4;

    entry->optionText = (char*)mem_malloc(strlen(string) + 1);
    strcpy(entry->optionText, string);

    return 0;
}

// Creates dialog interface.
//
// 0x446288
static int gdProcessInit()
{
    int upBtn;
    int downBtn;
    int optionsWindowX;
    int optionsWindowY;
    int fid;

    int replyWindowX = GAME_DIALOG_REPLY_WINDOW_X;
    int replyWindowY = GAME_DIALOG_REPLY_WINDOW_Y;
    gReplyWin = win_add(replyWindowX,
        replyWindowY,
        GAME_DIALOG_REPLY_WINDOW_WIDTH,
        GAME_DIALOG_REPLY_WINDOW_HEIGHT,
        256,
        WINDOW_FLAG_0x04);
    if (gReplyWin == -1) {
        goto err;
    }

    // Top part of the reply window - scroll up.
    upBtn = win_register_button(gReplyWin, 1, 1, 377, 28, -1, -1, KEY_ARROW_UP, -1, NULL, NULL, NULL, 32);
    if (upBtn == -1) {
        goto err_1;
    }

    win_register_button_sound_func(upBtn, gsound_red_butt_press, gsound_red_butt_release);
    win_register_button_func(upBtn, reply_arrow_up, reply_arrow_restore, 0, 0);

    // Bottom part of the reply window - scroll down.
    downBtn = win_register_button(gReplyWin, 1, 29, 377, 28, -1, -1, KEY_ARROW_DOWN, -1, NULL, NULL, NULL, 32);
    if (downBtn == -1) {
        goto err_1;
    }

    win_register_button_sound_func(downBtn, gsound_red_butt_press, gsound_red_butt_release);
    win_register_button_func(downBtn, reply_arrow_down, reply_arrow_restore, 0, 0);

    optionsWindowX = GAME_DIALOG_OPTIONS_WINDOW_X;
    optionsWindowY = GAME_DIALOG_OPTIONS_WINDOW_Y;
    gOptionWin = win_add(optionsWindowX, optionsWindowY, GAME_DIALOG_OPTIONS_WINDOW_WIDTH, GAME_DIALOG_OPTIONS_WINDOW_HEIGHT, 256, WINDOW_FLAG_0x04);
    if (gOptionWin == -1) {
        goto err_2;
    }

    // di_rdbt2.frm - dialog red button down
    fid = art_id(OBJ_TYPE_INTERFACE, 96, 0, 0, 0);
    dialog_red_button_up_buf = art_ptr_lock_data(fid, 0, 0, &dialog_red_button_up_key);
    if (dialog_red_button_up_buf == NULL) {
        goto err_3;
    }

    // di_rdbt1.frm - dialog red button up
    fid = art_id(OBJ_TYPE_INTERFACE, 95, 0, 0, 0);
    dialog_red_button_down_buf = art_ptr_lock_data(fid, 0, 0, &dialog_red_button_down_key);
    if (dialog_red_button_down_buf == NULL) {
        goto err_3;
    }

    talkOldFont = text_curr();
    text_font(101);

    return 0;

err_3:

    art_ptr_unlock(dialog_red_button_up_key);
    dialog_red_button_up_key = NULL;

err_2:

    win_delete(gOptionWin);
    gOptionWin = -1;

err_1:

    win_delete(gReplyWin);
    gReplyWin = -1;

err:

    return -1;
}

// RELASE: Rename/comment.
// free dialog option buttons
// 0x446454
static void gdProcessCleanup()
{
    for (int index = 0; index < gdNumOptions; index++) {
        GameDialogOptionEntry* optionEntry = &(dialogBlock.options[index]);

        if (optionEntry->btn != -1) {
            win_delete_button(optionEntry->btn);
            optionEntry->btn = -1;
        }
    }
}

// RELASE: Rename/comment.
// free dialog interface
// 0x446498
static int gdProcessExit()
{
    gdProcessCleanup();

    art_ptr_unlock(dialog_red_button_down_key);
    dialog_red_button_down_key = NULL;
    dialog_red_button_down_buf = NULL;

    art_ptr_unlock(dialog_red_button_up_key);
    dialog_red_button_up_key = NULL;
    dialog_red_button_up_buf = NULL;

    win_delete(gReplyWin);
    gReplyWin = -1;

    win_delete(gOptionWin);
    gOptionWin = -1;

    text_font(talkOldFont);

    return 0;
}

// 0x446504
static void gdUpdateMula()
{
    Rect rect;
    rect.ulx = 5;
    rect.lrx = 70;
    rect.uly = 36;
    rect.lry = text_height() + 36;

    talkToRefreshDialogWindowRect(&rect);

    int oldFont = text_curr();
    text_font(101);

    int caps = item_caps_total(obj_dude);
    char text[20];
    sprintf(text, "$%d", caps);

    int width = text_width(text);
    if (width > 60) {
        width = 60;
    }

    win_print(dialogueWindow, text, width, 38 - width / 2, 36, colorTable[992] | 0x7000000);

    text_font(oldFont);
}

// 0x4465C0
static int gdProcess()
{
    if (gdReenterLevel == 0) {
        if (gdProcessInit() == -1) {
            return -1;
        }
    }

    gdReenterLevel += 1;

    gdProcessUpdate();

    int v18 = 0;
    if (dialogBlock.offset != 0) {
        v18 = 1;
        gdReplyTooBig = 1;
    }

    unsigned int tick = get_time();
    int pageCount = 0;
    int pageIndex = 0;
    int pageOffsets[10];
    pageOffsets[0] = 0;
    for (;;) {
        int keyCode = get_input();

        if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
            game_quit_with_confirm();
        }

        if (game_user_wants_to_quit != 0) {
            break;
        }

        if (keyCode == KEY_CTRL_B && !mouse_click_in(135, 225, 514, 283)) {
            if (gmouse_get_cursor() != MOUSE_CURSOR_ARROW) {
                gmouse_set_cursor(MOUSE_CURSOR_ARROW);
            }
        } else {
            if (dialogue_switch_mode == 3) {
                dialogue_state = 4;
                barter_inventory(dialogueWindow, dialog_target, peon_table_obj, barterer_table_obj, gdBarterMod);
                gdialog_barter_cleanup_tables();

                int v5 = dialogue_state;
                gdialog_barter_destroy_win();
                dialogue_state = v5;

                if (v5 == 4) {
                    dialogue_switch_mode = 1;
                    dialogue_state = 1;
                }
                continue;
            } else if (dialogue_switch_mode == 9) {
                dialogue_state = 10;
                gdControl();
                gdControlDestroyWin();
                continue;
            } else if (dialogue_switch_mode == 12) {
                dialogue_state = 13;
                gdCustom();
                gdCustomDestroyWin();
                continue;
            }

            if (keyCode == KEY_LOWERCASE_B) {
                gdialog_barter_pressed(-1, -1);
            }
        }

        if (gdReplyTooBig) {
            unsigned int v6 = get_bk_time();
            if (v18) {
                if (elapsed_tocks(v6, tick) >= 10000 || keyCode == KEY_SPACE) {
                    pageCount++;
                    pageIndex++;
                    pageOffsets[pageCount] = dialogBlock.offset;
                    gdProcessReply();
                    tick = v6;
                    if (!dialogBlock.offset) {
                        v18 = 0;
                    }
                }
            }

            if (keyCode == KEY_ARROW_UP) {
                if (pageIndex > 0) {
                    pageIndex--;
                    dialogBlock.offset = pageOffsets[pageIndex];
                    v18 = 0;
                    gdProcessReply();
                }
            } else if (keyCode == KEY_ARROW_DOWN) {
                if (pageIndex < pageCount) {
                    pageIndex++;
                    dialogBlock.offset = pageOffsets[pageIndex];
                    v18 = 0;
                    gdProcessReply();
                } else {
                    if (dialogBlock.offset != 0) {
                        tick = v6;
                        pageIndex++;
                        pageCount++;
                        pageOffsets[pageCount] = dialogBlock.offset;
                        v18 = 0;
                        gdProcessReply();
                    }
                }
            }
        }

        if (keyCode != -1) {
            if (keyCode >= 1200 && keyCode <= 1250) {
                gdProcessHighlight(keyCode - 1200);
            } else if (keyCode >= 1300 && keyCode <= 1330) {
                gdProcessUnHighlight(keyCode - 1300);
            } else if (keyCode >= 48 && keyCode <= 57) {
                int v11 = keyCode - 49;
                if (v11 < gdNumOptions) {
                    pageCount = 0;
                    pageIndex = 0;
                    pageOffsets[0] = 0;
                    gdReplyTooBig = 0;

                    if (gdProcessChoice(v11) == -1) {
                        break;
                    }

                    tick = get_time();

                    if (dialogBlock.offset) {
                        v18 = 1;
                        gdReplyTooBig = 1;
                    } else {
                        v18 = 0;
                    }
                }
            }
        }
    }

    gdReenterLevel -= 1;

    if (gdReenterLevel == 0) {
        if (gdProcessExit() == -1) {
            return -1;
        }
    }

    return 0;
}

// 0x4468DC
static int gdProcessChoice(int a1)
{
    // FIXME: There is a buffer underread bug when `a1` is -1 (pressing 0 on the
    // keyboard, see `gdProcess`). When it happens the game looks into unused
    // continuation of `dialogBlock.replyText` (within 0x58F868-0x58FF70 range) which
    // is initialized to 0 according to C spec. I was not able to replicate the
    // same behaviour by extending dialogBlock.replyText to 2700 bytes or introduce
    // new 1800 bytes buffer in between, at least not in debug builds. In order
    // to preserve original behaviour this dummy dialog option entry is used.
    //
    // TODO: Recheck behaviour after introducing |GameDialogBlock|.
    GameDialogOptionEntry dummy;
    memset(&dummy, 0, sizeof(dummy));

    mouse_hide();
    gdProcessCleanup();

    GameDialogOptionEntry* dialogOptionEntry = a1 != -1 ? &(dialogBlock.options[a1]) : &dummy;
    if (dialogOptionEntry->messageListId == -4) {
        gdAddReviewOptionChosenStr(dialogOptionEntry->text);
    } else {
        gdAddReviewOptionChosen(dialogOptionEntry->messageListId, dialogOptionEntry->messageId);
    }

    can_start_new_fidget = false;

    gdialogFreeSpeech();

    int v1 = GAME_DIALOG_REACTION_NEUTRAL;
    switch (dialogOptionEntry->reaction) {
    case GAME_DIALOG_REACTION_GOOD:
        v1 = -1;
        break;
    case GAME_DIALOG_REACTION_NEUTRAL:
        v1 = 0;
        break;
    case GAME_DIALOG_REACTION_BAD:
        v1 = 1;
        break;
    default:
        // See 0x446907 in ecx but this branch should be unreachable. Due to the
        // bug described above, this code is reachable.
        v1 = GAME_DIALOG_REACTION_NEUTRAL;
        debug_printf("\nError: dialog: Empathy Perk: invalid reaction!");
        break;
    }

    demo_copy_title(gReplyWin);
    demo_copy_options(gOptionWin);
    win_draw(gReplyWin);
    win_draw(gOptionWin);

    gdProcessHighlight(a1);
    talk_to_critter_reacts(v1);

    gdNumOptions = 0;

    if (gdReenterLevel < 2) {
        if (dialogOptionEntry->proc != 0) {
            executeProcedure(dialogBlock.program, dialogOptionEntry->proc);
        }
    }

    mouse_show();

    if (gdNumOptions == 0) {
        return -1;
    }

    gdProcessUpdate();

    return 0;
}

// 0x446A18
static void gdProcessHighlight(int index)
{
    // FIXME: See explanation in `gdProcessChoice`.
    GameDialogOptionEntry dummy;
    memset(&dummy, 0, sizeof(dummy));

    GameDialogOptionEntry* dialogOptionEntry = index != -1 ? &(dialogBlock.options[index]) : &dummy;
    if (dialogOptionEntry->btn == 0) {
        return;
    }

    optionRect.ulx = 0;
    optionRect.uly = dialogOptionEntry->field_14;
    optionRect.lrx = 391;
    optionRect.lry = dialogOptionEntry->field_39C;
    gDialogRefreshOptionsRect(gOptionWin, &optionRect);

    optionRect.ulx = 5;
    optionRect.lrx = 388;

    int color = colorTable[32747] | 0x2000000;
    if (perkHasRank(obj_dude, PERK_EMPATHY)) {
        color = colorTable[32747] | 0x2000000;
        switch (dialogOptionEntry->reaction) {
        case GAME_DIALOG_REACTION_GOOD:
            color = colorTable[31775] | 0x2000000;
            break;
        case GAME_DIALOG_REACTION_NEUTRAL:
            break;
        case GAME_DIALOG_REACTION_BAD:
            color = colorTable[32074] | 0x2000000;
            break;
        default:
            debug_printf("\nError: dialog: Empathy Perk: invalid reaction!");
            break;
        }
    }

    // NOTE: Uninline.
    text_to_rect_wrapped(win_get_buf(gOptionWin),
        &optionRect,
        dialogOptionEntry->text,
        NULL,
        text_height(),
        393,
        color);

    optionRect.ulx = 0;
    optionRect.lrx = 391;
    optionRect.uly = dialogOptionEntry->field_14;
    win_draw_rect(gOptionWin, &optionRect);
}

// 0x446B5C
static void gdProcessUnHighlight(int index)
{
    GameDialogOptionEntry* dialogOptionEntry = &(dialogBlock.options[index]);

    optionRect.ulx = 0;
    optionRect.uly = dialogOptionEntry->field_14;
    optionRect.lrx = 391;
    optionRect.lry = dialogOptionEntry->field_39C;
    gDialogRefreshOptionsRect(gOptionWin, &optionRect);

    int color = colorTable[992] | 0x2000000;
    if (perk_level(obj_dude, PERK_EMPATHY) != 0) {
        color = colorTable[32747] | 0x2000000;
        switch (dialogOptionEntry->reaction) {
        case GAME_DIALOG_REACTION_GOOD:
            color = colorTable[31] | 0x2000000;
            break;
        case GAME_DIALOG_REACTION_NEUTRAL:
            color = colorTable[992] | 0x2000000;
            break;
        case GAME_DIALOG_REACTION_BAD:
            color = colorTable[31744] | 0x2000000;
            break;
        default:
            debug_printf("\nError: dialog: Empathy Perk: invalid reaction!");
            break;
        }
    }

    optionRect.ulx = 5;
    optionRect.lrx = 388;

    // NOTE: Uninline.
    text_to_rect_wrapped(win_get_buf(gOptionWin),
        &optionRect,
        dialogOptionEntry->text,
        NULL,
        text_height(),
        393,
        color);

    optionRect.lrx = 391;
    optionRect.uly = dialogOptionEntry->field_14;
    optionRect.ulx = 0;
    win_draw_rect(gOptionWin, &optionRect);
}

// 0x446C94
static void gdProcessReply()
{
    replyRect.ulx = 5;
    replyRect.uly = 10;
    replyRect.lrx = 374;
    replyRect.lry = 58;

    // NOTE: There is an unused if condition.
    perk_level(obj_dude, PERK_EMPATHY);

    demo_copy_title(gReplyWin);

    // NOTE: Uninline.
    text_to_rect_wrapped(win_get_buf(gReplyWin),
        &replyRect,
        dialogBlock.replyText,
        &(dialogBlock.offset),
        text_height(),
        379,
        colorTable[992] | 0x2000000);
    win_draw(gReplyWin);
}

// 0x446D30
static void gdProcessUpdate()
{
    replyRect.ulx = 5;
    replyRect.uly = 10;
    replyRect.lrx = 374;
    replyRect.lry = 58;

    optionRect.ulx = 5;
    optionRect.uly = 5;
    optionRect.lrx = 388;
    optionRect.lry = 112;

    demo_copy_title(gReplyWin);
    demo_copy_options(gOptionWin);

    if (dialogBlock.replyMessageListId > 0) {
        char* s = scr_get_msg_str_speech(dialogBlock.replyMessageListId, dialogBlock.replyMessageId, 1);
        if (s == NULL) {
            GNWSystemError("\n'GDialog::Error Grabbing text message!");
            exit(1);
        }

        strncpy(dialogBlock.replyText, s, sizeof(dialogBlock.replyText) - 1);
        *(dialogBlock.replyText + sizeof(dialogBlock.replyText) - 1) = '\0';
    }

    gdProcessReply();

    int color = colorTable[992] | 0x2000000;

    bool hasEmpathy = perk_level(obj_dude, PERK_EMPATHY) != 0;

    int width = optionRect.lrx - optionRect.ulx - 4;

    MessageListItem messageListItem;

    int v21 = 0;

    for (int index = 0; index < gdNumOptions; index++) {
        GameDialogOptionEntry* dialogOptionEntry = &(dialogBlock.options[index]);

        if (hasEmpathy) {
            switch (dialogOptionEntry->reaction) {
            case GAME_DIALOG_REACTION_GOOD:
                color = colorTable[31] | 0x2000000;
                break;
            case GAME_DIALOG_REACTION_NEUTRAL:
                color = colorTable[992] | 0x2000000;
                break;
            case GAME_DIALOG_REACTION_BAD:
                color = colorTable[31744] | 0x2000000;
                break;
            default:
                debug_printf("\nError: dialog: Empathy Perk: invalid reaction!");
                break;
            }
        }

        if (dialogOptionEntry->messageListId >= 0) {
            char* text = scr_get_msg_str_speech(dialogOptionEntry->messageListId, dialogOptionEntry->messageId, 0);
            if (text == NULL) {
                GNWSystemError("\nGDialog::Error Grabbing text message!");
                exit(1);
            }

            sprintf(dialogOptionEntry->text, "%c ", '\x95');
            strncat(dialogOptionEntry->text, text, 897);
        } else if (dialogOptionEntry->messageListId == -1) {
            if (index == 0) {
                // Go on
                messageListItem.num = 655;
                if (critterGetStat(obj_dude, STAT_INTELLIGENCE) < 4) {
                    if (message_search(&proto_main_msg_file, &messageListItem)) {
                        strcpy(dialogOptionEntry->text, messageListItem.text);
                    } else {
                        debug_printf("\nError...can't find message!");
                        return;
                    }
                }
            } else {
                // TODO: Why only space?
                strcpy(dialogOptionEntry->text, " ");
            }
        } else if (dialogOptionEntry->messageListId == -2) {
            // [Done]
            messageListItem.num = 650;
            if (message_search(&proto_main_msg_file, &messageListItem)) {
                sprintf(dialogOptionEntry->text, "%c %s", '\x95', messageListItem.text);
            } else {
                debug_printf("\nError...can't find message!");
                return;
            }
        }

        int v11 = text_num_lines(dialogOptionEntry->text, optionRect.lrx - optionRect.ulx) * text_height() + optionRect.uly + 2;
        if (v11 < optionRect.lry) {
            int y = optionRect.uly;

            dialogOptionEntry->field_39C = v11;
            dialogOptionEntry->field_14 = y;

            if (index == 0) {
                y = 0;
            }

            // NOTE: Uninline.
            text_to_rect_wrapped(win_get_buf(gOptionWin),
                &optionRect,
                dialogOptionEntry->text,
                NULL,
                text_height(),
                393,
                color);

            optionRect.uly += 2;

            dialogOptionEntry->btn = win_register_button(gOptionWin, 2, y, width, optionRect.uly - y - 4, 1200 + index, 1300 + index, -1, 49 + index, NULL, NULL, NULL, 0);
            if (dialogOptionEntry->btn != -1) {
                win_register_button_sound_func(dialogOptionEntry->btn, gsound_red_butt_press, gsound_red_butt_release);
            } else {
                debug_printf("\nError: Can't create button!");
            }
        } else {
            if (!v21) {
                v21 = 1;
            } else {
                debug_printf("Error: couldn't make button because it went below the window.\n");
            }
        }
    }

    gdUpdateMula();
    win_draw(gReplyWin);
    win_draw(gOptionWin);
}

// 0x44715C
static int gdCreateHeadWindow()
{
    dialogue_state = 1;

    int windowWidth = GAME_DIALOG_WINDOW_WIDTH;

    // NOTE: Uninline.
    talk_to_create_background_window();
    talk_to_refresh_background_window();

    unsigned char* buf = win_get_buf(dialogueBackWindow);

    for (int index = 0; index < 8; index++) {
        soundContinueAll();

        Rect* rect = &(backgrndRects[index]);
        int width = rect->lrx - rect->ulx;
        int height = rect->lry - rect->uly;
        backgrndBufs[index] = (unsigned char*)mem_malloc(width * height);
        if (backgrndBufs[index] == NULL) {
            return -1;
        }

        unsigned char* src = buf;
        src += windowWidth * rect->uly + rect->ulx;

        buf_to_buf(src, width, height, windowWidth, backgrndBufs[index], width);
    }

    gdialog_window_create();

    headWindowBuffer = win_get_buf(dialogueBackWindow) + windowWidth * 14 + 126;

    if (headWindowBuffer == NULL) {
        gdDestroyHeadWindow();
        return -1;
    }

    return 0;
}

// 0x447294
static void gdDestroyHeadWindow()
{
    if (dialogueWindow != -1) {
        headWindowBuffer = NULL;
    }

    if (dialogue_state == 1) {
        gdialog_window_destroy();
    } else if (dialogue_state == 4) {
        gdialog_barter_destroy_win();
    }

    if (dialogueBackWindow != -1) {
        win_delete(dialogueBackWindow);
        dialogueBackWindow = -1;
    }

    for (int index = 0; index < 8; index++) {
        mem_free(backgrndBufs[index]);
    }
}

// 0x447300
static void gdSetupFidget(int headFrmId, int reaction)
{
    // 0x518900
    static int phone_anim = 0;

    fidgetFrameCounter = 0;

    if (headFrmId == -1) {
        fidgetFID = -1;
        fidgetFp = NULL;
        fidgetKey = INVALID_CACHE_ENTRY;
        fidgetAnim = -1;
        fidgetTocksPerFrame = 0;
        fidgetLastTime = 0;
        gdDisplayFrame(NULL, 0);
        lipsFID = 0;
        lipsKey = NULL;
        lipsFp = 0;
        return;
    }

    int anim = HEAD_ANIMATION_NEUTRAL_PHONEMES;
    switch (reaction) {
    case FIDGET_GOOD:
        anim = HEAD_ANIMATION_GOOD_PHONEMES;
        break;
    case FIDGET_BAD:
        anim = HEAD_ANIMATION_BAD_PHONEMES;
        break;
    }

    if (lipsFID != 0) {
        if (anim != phone_anim) {
            if (art_ptr_unlock(lipsKey) == -1) {
                debug_printf("failure unlocking lips frame!\n");
            }
            lipsKey = NULL;
            lipsFp = NULL;
            lipsFID = 0;
        }
    }

    if (lipsFID == 0) {
        phone_anim = anim;
        lipsFID = art_id(OBJ_TYPE_HEAD, headFrmId, anim, 0, 0);
        lipsFp = art_ptr_lock(lipsFID, &lipsKey);
        if (lipsFp == NULL) {
            debug_printf("failure!\n");

            char stats[200];
            cache_stats(&art_cache, stats);
            debug_printf("%s", stats);
        }
    }

    int fid = art_id(OBJ_TYPE_HEAD, headFrmId, reaction, 0, 0);
    int fidgetCount = art_head_fidgets(fid);
    if (fidgetCount == -1) {
        debug_printf("\tError - No available fidgets for given frame id\n");
        return;
    }

    int chance = roll_random(1, 100) + dialogue_seconds_since_last_input / 2;

    int fidget = fidgetCount;
    switch (fidgetCount) {
    case 1:
        fidget = 1;
        break;
    case 2:
        if (chance < 68) {
            fidget = 1;
        } else {
            fidget = 2;
        }
        break;
    case 3:
        dialogue_seconds_since_last_input = 0;
        if (chance < 52) {
            fidget = 1;
        } else if (chance < 77) {
            fidget = 2;
        } else {
            fidget = 3;
        }
        break;
    }

    debug_printf("Choosing fidget %d out of %d\n", fidget, fidgetCount);

    if (fidgetFp != NULL) {
        if (art_ptr_unlock(fidgetKey) == -1) {
            debug_printf("failure!\n");
        }
    }

    fidgetFID = art_id(OBJ_TYPE_HEAD, headFrmId, reaction, fidget, 0);
    fidgetFrameCounter = 0;
    fidgetFp = art_ptr_lock(fidgetFID, &fidgetKey);
    if (fidgetFp == NULL) {
        debug_printf("failure!\n");

        char stats[200];
        cache_stats(&art_cache, stats);
        debug_printf("%s", stats);
    }

    fidgetLastTime = 0;
    fidgetAnim = reaction;
    fidgetTocksPerFrame = 1000 / art_frame_fps(fidgetFp);
}

// 0x447598
static void gdWaitForFidget()
{
    if (fidgetFp == NULL) {
        return;
    }

    if (dialogueWindow == -1) {
        return;
    }

    debug_printf("Waiting for fidget to complete...\n");

    while (art_frame_max_frame(fidgetFp) > fidgetFrameCounter) {
        if (elapsed_time(fidgetLastTime) >= fidgetTocksPerFrame) {
            gdDisplayFrame(fidgetFp, fidgetFrameCounter);
            fidgetLastTime = get_time();
            fidgetFrameCounter++;
        }
    }

    fidgetFrameCounter = 0;
}

// 0x447614
static void gdPlayTransition(int anim)
{
    if (fidgetFp == NULL) {
        return;
    }

    if (dialogueWindow == -1) {
        return;
    }

    mouse_hide();

    debug_printf("Starting transition...\n");

    gdWaitForFidget();

    if (fidgetFp != NULL) {
        if (art_ptr_unlock(fidgetKey) == -1) {
            debug_printf("\tError unlocking fidget in transition func...");
        }
        fidgetFp = NULL;
    }

    CacheEntry* headFrmHandle;
    int headFid = art_id(OBJ_TYPE_HEAD, dialogue_head, anim, 0, 0);
    Art* headFrm = art_ptr_lock(headFid, &headFrmHandle);
    if (headFrm == NULL) {
        debug_printf("\tError locking transition...\n");
    }

    unsigned int delay = 1000 / art_frame_fps(headFrm);

    int frame = 0;
    unsigned int time = 0;
    while (frame < art_frame_max_frame(headFrm)) {
        if (elapsed_time(time) >= delay) {
            gdDisplayFrame(headFrm, frame);
            time = get_time();
            frame++;
        }
    }

    if (art_ptr_unlock(headFrmHandle) == -1) {
        debug_printf("\tError unlocking transition...\n");
    }

    debug_printf("Finished transition...\n");
    mouse_show();
}

// 0x447724
static void reply_arrow_up(int btn, int keyCode)
{
    if (gdReplyTooBig) {
        gmouse_set_cursor(MOUSE_CURSOR_SMALL_ARROW_UP);
    }
}

// 0x447738
static void reply_arrow_down(int btn, int keyCode)
{
    if (gdReplyTooBig) {
        gmouse_set_cursor(MOUSE_CURSOR_SMALL_ARROW_DOWN);
    }
}

// 0x44774C
static void reply_arrow_restore(int btn, int keyCode)
{
    gmouse_set_cursor(MOUSE_CURSOR_ARROW);
}

// demo_copy_title
// 0x447758
static void demo_copy_title(int win)
{
    gd_replyWin = win;

    if (win == -1) {
        debug_printf("\nError: demo_copy_title: win invalid!");
        return;
    }

    int width = win_width(win);
    if (width < 1) {
        debug_printf("\nError: demo_copy_title: width invalid!");
        return;
    }

    int height = win_height(win);
    if (height < 1) {
        debug_printf("\nError: demo_copy_title: length invalid!");
        return;
    }

    if (dialogueBackWindow == -1) {
        debug_printf("\nError: demo_copy_title: dialogueBackWindow wasn't created!");
        return;
    }

    unsigned char* src = win_get_buf(dialogueBackWindow);
    if (src == NULL) {
        debug_printf("\nError: demo_copy_title: couldn't get buffer!");
        return;
    }

    unsigned char* dest = win_get_buf(win);

    buf_to_buf(src + 640 * 225 + 135, width, height, 640, dest, width);
}

// demo_copy_options
// 0x447818
static void demo_copy_options(int win)
{
    gd_optionsWin = win;

    if (win == -1) {
        debug_printf("\nError: demo_copy_options: win invalid!");
        return;
    }

    int width = win_width(win);
    if (width < 1) {
        debug_printf("\nError: demo_copy_options: width invalid!");
        return;
    }

    int height = win_height(win);
    if (height < 1) {
        debug_printf("\nError: demo_copy_options: length invalid!");
        return;
    }

    if (dialogueBackWindow == -1) {
        debug_printf("\nError: demo_copy_options: dialogueBackWindow wasn't created!");
        return;
    }

    Rect windowRect;
    win_get_rect(dialogueWindow, &windowRect);

    unsigned char* src = win_get_buf(dialogueWindow);
    if (src == NULL) {
        debug_printf("\nError: demo_copy_options: couldn't get buffer!");
        return;
    }

    unsigned char* dest = win_get_buf(win);
    buf_to_buf(src + 640 * (335 - windowRect.uly) + 127, width, height, 640, dest, width);
}

// gDialogRefreshOptionsRect
// 0x447914
static void gDialogRefreshOptionsRect(int win, Rect* drawRect)
{
    if (drawRect == NULL) {
        debug_printf("\nError: gDialogRefreshOptionsRect: drawRect NULL!");
        return;
    }

    if (win == -1) {
        debug_printf("\nError: gDialogRefreshOptionsRect: win invalid!");
        return;
    }

    if (dialogueBackWindow == -1) {
        debug_printf("\nError: gDialogRefreshOptionsRect: dialogueBackWindow wasn't created!");
        return;
    }

    Rect windowRect;
    win_get_rect(dialogueWindow, &windowRect);

    unsigned char* src = win_get_buf(dialogueWindow);
    if (src == NULL) {
        debug_printf("\nError: gDialogRefreshOptionsRect: couldn't get buffer!");
        return;
    }

    if (drawRect->uly >= drawRect->lry) {
        debug_printf("\nError: gDialogRefreshOptionsRect: Invalid Rect (too many options)!");
        return;
    }

    if (drawRect->ulx >= drawRect->lrx) {
        debug_printf("\nError: gDialogRefreshOptionsRect: Invalid Rect (too many options)!");
        return;
    }

    int destWidth = win_width(win);
    unsigned char* dest = win_get_buf(win);

    buf_to_buf(
        src + (640 * (335 - windowRect.uly) + 127) + (640 * drawRect->uly + drawRect->ulx),
        drawRect->lrx - drawRect->ulx,
        drawRect->lry - drawRect->uly,
        640,
        dest + destWidth * drawRect->uly,
        destWidth);
}

// 0x447A58
static void gdialog_bk()
{
    // 0x518904
    static int loop_cnt = -1;

    // 0x518908
    static unsigned int tocksWaiting = 10000;

    switch (dialogue_switch_mode) {
    case 2:
        loop_cnt = -1;
        dialogue_switch_mode = 3;
        gdialog_window_destroy();
        gdialog_barter_create_win();
        break;
    case 1:
        loop_cnt = -1;
        dialogue_switch_mode = 0;
        gdialog_barter_destroy_win();
        gdialog_window_create();

        // NOTE: Uninline.
        gdUnhide();

        break;
    case 8:
        loop_cnt = -1;
        dialogue_switch_mode = 9;
        gdialog_window_destroy();
        gdControlCreateWin();
        break;
    case 11:
        loop_cnt = -1;
        dialogue_switch_mode = 12;
        gdialog_window_destroy();
        gdCustomCreateWin();
        break;
    }

    if (fidgetFp == NULL) {
        return;
    }

    if (gdialog_speech_playing) {
        lips_bkg_proc();

        if (lips_draw_head) {
            gdDisplayFrame(lipsFp, head_phoneme_lookup[head_phoneme_current]);
            lips_draw_head = false;
        }

        if (!soundPlaying(lip_info.sound)) {
            gdialogFreeSpeech();
            gdDisplayFrame(lipsFp, 0);
            can_start_new_fidget = true;
            dialogue_seconds_since_last_input = 3;
            fidgetFrameCounter = 0;
        }
        return;
    }

    if (can_start_new_fidget) {
        if (elapsed_time(fidgetLastTime) >= tocksWaiting) {
            can_start_new_fidget = false;
            dialogue_seconds_since_last_input += tocksWaiting / 1000;
            tocksWaiting = 1000 * (roll_random(0, 3) + 4);
            gdSetupFidget(fidgetFID & 0xFFF, (fidgetFID & 0xFF0000) >> 16);
        }
        return;
    }

    if (elapsed_time(fidgetLastTime) >= fidgetTocksPerFrame) {
        if (art_frame_max_frame(fidgetFp) <= fidgetFrameCounter) {
            gdDisplayFrame(fidgetFp, 0);
            can_start_new_fidget = true;
        } else {
            gdDisplayFrame(fidgetFp, fidgetFrameCounter);
            fidgetLastTime = get_time();
            fidgetFrameCounter += 1;
        }
    }
}

// FIXME: Due to the bug in `gdProcessChoice` this function can receive invalid
// reaction value (50 instead of expected -1, 0, 1). It's handled gracefully by
// the game.
//
// 0x447CA0
void talk_to_critter_reacts(int a1)
{
    int v1 = a1 + 1;

    debug_printf("Dialogue Reaction: ");
    if (v1 < 3) {
        debug_printf("%s\n", react_strs[v1]);
    }

    int v3 = a1 + 50;
    dialogue_seconds_since_last_input = 0;

    switch (v3) {
    case GAME_DIALOG_REACTION_GOOD:
        switch (fidgetAnim) {
        case FIDGET_GOOD:
            gdPlayTransition(HEAD_ANIMATION_VERY_GOOD_REACTION);
            gdSetupFidget(dialogue_head, FIDGET_GOOD);
            break;
        case FIDGET_NEUTRAL:
            gdPlayTransition(HEAD_ANIMATION_NEUTRAL_TO_GOOD);
            gdSetupFidget(dialogue_head, FIDGET_GOOD);
            break;
        case FIDGET_BAD:
            gdPlayTransition(HEAD_ANIMATION_BAD_TO_NEUTRAL);
            gdSetupFidget(dialogue_head, FIDGET_NEUTRAL);
            break;
        }
        break;
    case GAME_DIALOG_REACTION_NEUTRAL:
        break;
    case GAME_DIALOG_REACTION_BAD:
        switch (fidgetAnim) {
        case FIDGET_GOOD:
            gdPlayTransition(HEAD_ANIMATION_GOOD_TO_NEUTRAL);
            gdSetupFidget(dialogue_head, FIDGET_NEUTRAL);
            break;
        case FIDGET_NEUTRAL:
            gdPlayTransition(HEAD_ANIMATION_NEUTRAL_TO_BAD);
            gdSetupFidget(dialogue_head, FIDGET_BAD);
            break;
        case FIDGET_BAD:
            gdPlayTransition(HEAD_ANIMATION_VERY_BAD_REACTION);
            gdSetupFidget(dialogue_head, FIDGET_BAD);
            break;
        }
        break;
    }
}

// 0x447D98
static void gdialog_scroll_subwin(int win, int a2, unsigned char* a3, unsigned char* a4, unsigned char* a5, int a6, int a7)
{
    int v7;
    unsigned char* v9;
    Rect rect;
    unsigned int tick;

    v7 = a6;
    v9 = a4;

    if (a2 == 1) {
        rect.ulx = 0;
        rect.lrx = GAME_DIALOG_WINDOW_WIDTH - 1;
        rect.lry = a6 - 1;

        int v18 = a6 / 10;
        if (a7 == -1) {
            rect.uly = 10;
            v18 = 0;
        } else {
            rect.uly = v18 * 10;
            v7 = a6 % 10;
            v9 += (GAME_DIALOG_WINDOW_WIDTH) * rect.uly;
        }

        for (; v18 >= 0; v18--) {
            soundContinueAll();
            buf_to_buf(a3,
                GAME_DIALOG_WINDOW_WIDTH,
                v7,
                GAME_DIALOG_WINDOW_WIDTH,
                v9,
                GAME_DIALOG_WINDOW_WIDTH);
            rect.uly -= 10;
            win_draw_rect(win, &rect);
            v7 += 10;
            v9 -= 10 * (GAME_DIALOG_WINDOW_WIDTH);

            tick = get_time();
            while (elapsed_time(tick) < 33) {
            }
        }
    } else {
        rect.lrx = GAME_DIALOG_WINDOW_WIDTH - 1;
        rect.lry = a6 - 1;
        rect.ulx = 0;
        rect.uly = 0;

        for (int index = a6 / 10; index > 0; index--) {
            soundContinueAll();

            buf_to_buf(a5,
                GAME_DIALOG_WINDOW_WIDTH,
                10,
                GAME_DIALOG_WINDOW_WIDTH,
                v9,
                GAME_DIALOG_WINDOW_WIDTH);

            v9 += 10 * (GAME_DIALOG_WINDOW_WIDTH);
            v7 -= 10;
            a5 += 10 * (GAME_DIALOG_WINDOW_WIDTH);

            buf_to_buf(a3,
                GAME_DIALOG_WINDOW_WIDTH,
                v7,
                GAME_DIALOG_WINDOW_WIDTH,
                v9,
                GAME_DIALOG_WINDOW_WIDTH);

            win_draw_rect(win, &rect);

            rect.uly += 10;

            tick = get_time();
            while (elapsed_time(tick) < 33) {
            }
        }
    }
}

// 0x447F64
static int text_num_lines(const char* a1, int a2)
{
    int width = text_width(a1);

    int v1 = 0;
    while (width > 0) {
        width -= a2;
        v1++;
    }

    return v1;
}

// NOTE: Inlined.
//
// 0x447F80
static int text_to_rect_wrapped(unsigned char* buffer, Rect* rect, char* string, int* a4, int height, int pitch, int color)
{
    return text_to_rect_func(buffer, rect, string, a4, height, pitch, color, 1);
}

// display_msg
// 0x447FA0
static int text_to_rect_func(unsigned char* buffer, Rect* rect, char* string, int* a4, int height, int pitch, int color, int a7)
{
    char* start;
    if (a4 != NULL) {
        start = string + *a4;
    } else {
        start = string;
    }

    int maxWidth = rect->lrx - rect->ulx;
    char* end = NULL;
    while (start != NULL && *start != '\0') {
        if (text_width(start) > maxWidth) {
            end = start + 1;
            while (*end != '\0' && *end != ' ') {
                end++;
            }

            if (*end != '\0') {
                char* lookahead = end + 1;
                while (lookahead != NULL) {
                    while (*lookahead != '\0' && *lookahead != ' ') {
                        lookahead++;
                    }

                    if (*lookahead == '\0') {
                        lookahead = NULL;
                    } else {
                        *lookahead = '\0';
                        if (text_width(start) >= maxWidth) {
                            *lookahead = ' ';
                            lookahead = NULL;
                        } else {
                            end = lookahead;
                            *lookahead = ' ';
                            lookahead++;
                        }
                    }
                }

                if (*end == ' ') {
                    *end = '\0';
                }
            } else {
                if (rect->lry - text_height() < rect->uly) {
                    return rect->uly;
                }

                if (a7 != 1 || start == string) {
                    text_to_buf(buffer + pitch * rect->uly + 10, start, maxWidth, pitch, color);
                } else {
                    text_to_buf(buffer + pitch * rect->uly, start, maxWidth, pitch, color);
                }

                if (a4 != NULL) {
                    *a4 += strlen(start) + 1;
                }

                rect->uly += height;
                return rect->uly;
            }
        }

        if (text_width(start) > maxWidth) {
            debug_printf("\nError: display_msg: word too long!");
            break;
        }

        if (a7 != 0) {
            if (rect->lry - text_height() < rect->uly) {
                if (end != NULL && *end == '\0') {
                    *end = ' ';
                }
                return rect->uly;
            }

            unsigned char* dest;
            if (a7 != 1 || start == string) {
                dest = buffer + 10;
            } else {
                dest = buffer;
            }
            text_to_buf(dest + pitch * rect->uly, start, maxWidth, pitch, color);
        }

        if (a4 != NULL && end != NULL) {
            *a4 += strlen(start) + 1;
        }

        rect->uly += height;

        if (end != NULL) {
            start = end + 1;
            if (*end == '\0') {
                *end = ' ';
            }
            end = NULL;
        } else {
            start = NULL;
        }
    }

    if (a4 != NULL) {
        *a4 = 0;
    }

    return rect->uly;
}

// 0x448214
void gdialogSetBarterMod(int modifier)
{
    gdBarterMod = modifier;
}

// gdialog_barter
// 0x44821C
int gdActivateBarter(int modifier)
{
    if (!dialog_state_fix) {
        return -1;
    }

    gdBarterMod = modifier;
    gdialog_barter_pressed(-1, -1);
    dialogue_state = 4;
    dialogue_switch_mode = 2;

    return 0;
}

// 0x448268
void barter_end_to_talk_to()
{
    dialogQuit();
    dialogClose();
    updatePrograms();
    updateWindows();
    dialogue_state = 1;
    dialogue_switch_mode = 1;
}

// 0x448290
static int gdialog_barter_create_win()
{
    dialogue_state = 4;

    int frmId;
    if (dialog_target_is_party) {
        // trade.frm - party member barter/trade interface
        frmId = 420;
    } else {
        // barter.frm - barter window
        frmId = 111;
    }

    int backgroundFid = art_id(OBJ_TYPE_INTERFACE, frmId, 0, 0, 0);
    CacheEntry* backgroundHandle;
    Art* backgroundFrm = art_ptr_lock(backgroundFid, &backgroundHandle);
    if (backgroundFrm == NULL) {
        return -1;
    }

    unsigned char* backgroundData = art_frame_data(backgroundFrm, 0, 0);
    if (backgroundData == NULL) {
        art_ptr_unlock(backgroundHandle);
        return -1;
    }

    dialogue_subwin_len = art_frame_length(backgroundFrm, 0, 0);

    int barterWindowX = 0;
    int barterWindowY = GAME_DIALOG_WINDOW_HEIGHT - dialogue_subwin_len;
    dialogueWindow = win_add(barterWindowX,
        barterWindowY,
        GAME_DIALOG_WINDOW_WIDTH,
        dialogue_subwin_len,
        256,
        WINDOW_FLAG_0x02);
    if (dialogueWindow == -1) {
        art_ptr_unlock(backgroundHandle);
        return -1;
    }

    int width = GAME_DIALOG_WINDOW_WIDTH;

    unsigned char* windowBuffer = win_get_buf(dialogueWindow);
    unsigned char* backgroundWindowBuffer = win_get_buf(dialogueBackWindow);
    buf_to_buf(backgroundWindowBuffer + width * (480 - dialogue_subwin_len), width, dialogue_subwin_len, width, windowBuffer, width);

    gdialog_scroll_subwin(dialogueWindow, 1, backgroundData, windowBuffer, NULL, dialogue_subwin_len, 0);

    art_ptr_unlock(backgroundHandle);

    // TRADE
    gdialog_buttons[0] = win_register_button(dialogueWindow, 41, 163, 14, 14, -1, -1, -1, KEY_LOWERCASE_M, dialog_red_button_up_buf, dialog_red_button_down_buf, 0, BUTTON_FLAG_TRANSPARENT);
    if (gdialog_buttons[0] != -1) {
        win_register_button_sound_func(gdialog_buttons[0], gsound_med_butt_press, gsound_med_butt_release);

        // TALK
        gdialog_buttons[1] = win_register_button(dialogueWindow, 584, 162, 14, 14, -1, -1, -1, KEY_LOWERCASE_T, dialog_red_button_up_buf, dialog_red_button_down_buf, 0, BUTTON_FLAG_TRANSPARENT);
        if (gdialog_buttons[1] != -1) {
            win_register_button_sound_func(gdialog_buttons[1], gsound_med_butt_press, gsound_med_butt_release);

            if (obj_new(&peon_table_obj, -1, -1) != -1) {
                peon_table_obj->flags |= OBJECT_HIDDEN;

                if (obj_new(&barterer_table_obj, -1, -1) != -1) {
                    barterer_table_obj->flags |= OBJECT_HIDDEN;

                    if (obj_new(&barterer_temp_obj, dialog_target->fid, -1) != -1) {
                        barterer_temp_obj->flags |= OBJECT_HIDDEN | OBJECT_TEMPORARY;
                        barterer_temp_obj->sid = -1;
                        return 0;
                    }

                    obj_erase_object(barterer_table_obj, 0);
                }

                obj_erase_object(peon_table_obj, 0);
            }

            win_delete_button(gdialog_buttons[1]);
            gdialog_buttons[1] = -1;
        }

        win_delete_button(gdialog_buttons[0]);
        gdialog_buttons[0] = -1;
    }

    win_delete(dialogueWindow);
    dialogueWindow = -1;

    return -1;
}

// 0x44854C
static void gdialog_barter_destroy_win()
{
    if (dialogueWindow == -1) {
        return;
    }

    obj_erase_object(barterer_temp_obj, 0);
    obj_erase_object(barterer_table_obj, 0);
    obj_erase_object(peon_table_obj, 0);

    for (int index = 0; index < 9; index++) {
        win_delete_button(gdialog_buttons[index]);
        gdialog_buttons[index] = -1;
    }

    unsigned char* backgroundWindowBuffer = win_get_buf(dialogueBackWindow);
    backgroundWindowBuffer += (GAME_DIALOG_WINDOW_WIDTH) * (480 - dialogue_subwin_len);

    int frmId;
    if (dialog_target_is_party) {
        // trade.frm - party member barter/trade interface
        frmId = 420;
    } else {
        // barter.frm - barter window
        frmId = 111;
    }

    CacheEntry* backgroundFrmHandle;
    int fid = art_id(OBJ_TYPE_INTERFACE, frmId, 0, 0, 0);
    unsigned char* backgroundFrmData = art_ptr_lock_data(fid, 0, 0, &backgroundFrmHandle);
    if (backgroundFrmData != NULL) {
        unsigned char* windowBuffer = win_get_buf(dialogueWindow);
        gdialog_scroll_subwin(dialogueWindow, 0, backgroundFrmData, windowBuffer, backgroundWindowBuffer, dialogue_subwin_len, 0);
        art_ptr_unlock(backgroundFrmHandle);
    }

    win_delete(dialogueWindow);
    dialogueWindow = -1;

    cai_attempt_w_reload(dialog_target, 0);
}

// 0x448660
static void gdialog_barter_cleanup_tables()
{
    Inventory* inventory;
    int length;

    inventory = &(peon_table_obj->data.inventory);
    length = inventory->length;
    for (int index = 0; index < length; index++) {
        Object* item = inventory->items->item;
        int quantity = item_count(peon_table_obj, item);
        item_move_force(peon_table_obj, obj_dude, item, quantity);
    }

    inventory = &(barterer_table_obj->data.inventory);
    length = inventory->length;
    for (int index = 0; index < length; index++) {
        Object* item = inventory->items->item;
        int quantity = item_count(barterer_table_obj, item);
        item_move_force(barterer_table_obj, dialog_target, item, quantity);
    }

    if (barterer_temp_obj != NULL) {
        inventory = &(barterer_temp_obj->data.inventory);
        length = inventory->length;
        for (int index = 0; index < length; index++) {
            Object* item = inventory->items->item;
            int quantity = item_count(barterer_temp_obj, item);
            item_move_force(barterer_temp_obj, dialog_target, item, quantity);
        }
    }
}

// 0x448740
static int gdControlCreateWin()
{
    CacheEntry* backgroundFrmHandle;
    int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 390, 0, 0, 0);
    Art* backgroundFrm = art_ptr_lock(backgroundFid, &backgroundFrmHandle);
    if (backgroundFrm == NULL) {
        return -1;
    }

    unsigned char* backgroundData = art_frame_data(backgroundFrm, 0, 0);
    if (backgroundData == NULL) {
        gdControlDestroyWin();
        return -1;
    }

    dialogue_subwin_len = art_frame_length(backgroundFrm, 0, 0);
    int controlWindowX = 0;
    int controlWindowY = GAME_DIALOG_WINDOW_HEIGHT - dialogue_subwin_len;
    dialogueWindow = win_add(controlWindowX,
        controlWindowY,
        GAME_DIALOG_WINDOW_WIDTH,
        dialogue_subwin_len,
        256,
        WINDOW_FLAG_0x02);
    if (dialogueWindow == -1) {
        gdControlDestroyWin();
        return -1;
    }

    unsigned char* windowBuffer = win_get_buf(dialogueWindow);
    unsigned char* src = win_get_buf(dialogueBackWindow);
    buf_to_buf(src + (GAME_DIALOG_WINDOW_WIDTH) * (GAME_DIALOG_WINDOW_HEIGHT - dialogue_subwin_len), GAME_DIALOG_WINDOW_WIDTH, dialogue_subwin_len, GAME_DIALOG_WINDOW_WIDTH, windowBuffer, GAME_DIALOG_WINDOW_WIDTH);
    gdialog_scroll_subwin(dialogueWindow, 1, backgroundData, windowBuffer, 0, dialogue_subwin_len, 0);
    art_ptr_unlock(backgroundFrmHandle);

    // TALK
    gdialog_buttons[0] = win_register_button(dialogueWindow, 593, 41, 14, 14, -1, -1, -1, KEY_ESCAPE, dialog_red_button_up_buf, dialog_red_button_down_buf, NULL, BUTTON_FLAG_TRANSPARENT);
    if (gdialog_buttons[0] == -1) {
        gdControlDestroyWin();
        return -1;
    }
    win_register_button_sound_func(gdialog_buttons[0], gsound_med_butt_press, gsound_med_butt_release);

    // TRADE
    gdialog_buttons[1] = win_register_button(dialogueWindow, 593, 97, 14, 14, -1, -1, -1, KEY_LOWERCASE_D, dialog_red_button_up_buf, dialog_red_button_down_buf, NULL, BUTTON_FLAG_TRANSPARENT);
    if (gdialog_buttons[1] == -1) {
        gdControlDestroyWin();
        return -1;
    }
    win_register_button_sound_func(gdialog_buttons[1], gsound_med_butt_press, gsound_med_butt_release);

    // USE BEST WEAPON
    gdialog_buttons[2] = win_register_button(dialogueWindow, 236, 15, 14, 14, -1, -1, -1, KEY_LOWERCASE_W, dialog_red_button_up_buf, dialog_red_button_down_buf, NULL, BUTTON_FLAG_TRANSPARENT);
    if (gdialog_buttons[2] == -1) {
        gdControlDestroyWin();
        return -1;
    }
    win_register_button_sound_func(gdialog_buttons[1], gsound_med_butt_press, gsound_med_butt_release);

    // USE BEST ARMOR
    gdialog_buttons[3] = win_register_button(dialogueWindow, 235, 46, 14, 14, -1, -1, -1, KEY_LOWERCASE_A, dialog_red_button_up_buf, dialog_red_button_down_buf, NULL, BUTTON_FLAG_TRANSPARENT);
    if (gdialog_buttons[3] == -1) {
        gdControlDestroyWin();
        return -1;
    }
    win_register_button_sound_func(gdialog_buttons[2], gsound_med_butt_press, gsound_med_butt_release);

    control_buttons_start = 4;

    int v21 = 3;

    for (int index = 0; index < 5; index++) {
        GameDialogButtonData* buttonData = &(control_button_info[index]);
        int fid;

        fid = art_id(OBJ_TYPE_INTERFACE, buttonData->upFrmId, 0, 0, 0);
        Art* upButtonFrm = art_ptr_lock(fid, &(buttonData->upFrmHandle));
        if (upButtonFrm == NULL) {
            gdControlDestroyWin();
            return -1;
        }

        int width = art_frame_width(upButtonFrm, 0, 0);
        int height = art_frame_length(upButtonFrm, 0, 0);
        unsigned char* upButtonFrmData = art_frame_data(upButtonFrm, 0, 0);

        fid = art_id(OBJ_TYPE_INTERFACE, buttonData->downFrmId, 0, 0, 0);
        Art* downButtonFrm = art_ptr_lock(fid, &(buttonData->downFrmHandle));
        if (downButtonFrm == NULL) {
            gdControlDestroyWin();
            return -1;
        }

        unsigned char* downButtonFrmData = art_frame_data(downButtonFrm, 0, 0);

        fid = art_id(OBJ_TYPE_INTERFACE, buttonData->disabledFrmId, 0, 0, 0);
        Art* disabledButtonFrm = art_ptr_lock(fid, &(buttonData->disabledFrmHandle));
        if (disabledButtonFrm == NULL) {
            gdControlDestroyWin();
            return -1;
        }

        unsigned char* disabledButtonFrmData = art_frame_data(disabledButtonFrm, 0, 0);

        v21++;

        gdialog_buttons[v21] = win_register_button(dialogueWindow,
            buttonData->x,
            buttonData->y,
            width,
            height,
            -1,
            -1,
            buttonData->keyCode,
            -1,
            upButtonFrmData,
            downButtonFrmData,
            NULL,
            BUTTON_FLAG_TRANSPARENT | BUTTON_FLAG_0x04 | BUTTON_FLAG_0x01);
        if (gdialog_buttons[v21] == -1) {
            gdControlDestroyWin();
            return -1;
        }

        win_register_button_disable(gdialog_buttons[v21], disabledButtonFrmData, disabledButtonFrmData, disabledButtonFrmData);
        win_register_button_sound_func(gdialog_buttons[v21], gsound_med_butt_press, gsound_med_butt_release);

        if (!partyMemberHasAIDisposition(dialog_target, buttonData->value)) {
            win_disable_button(gdialog_buttons[v21]);
        }
    }

    win_group_radio_buttons(5, &(gdialog_buttons[control_buttons_start]));

    int disposition = ai_get_disposition(dialog_target);
    win_set_button_rest_state(gdialog_buttons[control_buttons_start + 4 - disposition], 1, 0);

    gdControlUpdateInfo();

    dialogue_state = 10;

    win_draw(dialogueWindow);

    return 0;
}

// 0x448C10
static void gdControlDestroyWin()
{
    if (dialogueWindow == -1) {
        return;
    }

    for (int index = 0; index < 9; index++) {
        win_delete_button(gdialog_buttons[index]);
        gdialog_buttons[index] = -1;
    }

    for (int index = 0; index < 5; index++) {
        GameDialogButtonData* buttonData = &(control_button_info[index]);

        if (buttonData->upFrmHandle) {
            art_ptr_unlock(buttonData->upFrmHandle);
            buttonData->upFrmHandle = NULL;
        }

        if (buttonData->downFrmHandle) {
            art_ptr_unlock(buttonData->downFrmHandle);
            buttonData->downFrmHandle = NULL;
        }

        if (buttonData->disabledFrmHandle) {
            art_ptr_unlock(buttonData->disabledFrmHandle);
            buttonData->disabledFrmHandle = NULL;
        }
    }

    // control.frm - party member control interface
    CacheEntry* backgroundFrmHandle;
    int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 390, 0, 0, 0);
    unsigned char* backgroundFrmData = art_ptr_lock_data(backgroundFid, 0, 0, &backgroundFrmHandle);
    if (backgroundFrmData != NULL) {
        gdialog_scroll_subwin(dialogueWindow, 0, backgroundFrmData, win_get_buf(dialogueWindow), win_get_buf(dialogueBackWindow) + (GAME_DIALOG_WINDOW_WIDTH) * (480 - dialogue_subwin_len), dialogue_subwin_len, 0);
        art_ptr_unlock(backgroundFrmHandle);
    }

    win_delete(dialogueWindow);
    dialogueWindow = -1;
}

// 0x448D30
static void gdControlUpdateInfo()
{
    int oldFont = text_curr();
    text_font(101);

    unsigned char* windowBuffer = win_get_buf(dialogueWindow);
    int windowWidth = win_width(dialogueWindow);

    CacheEntry* backgroundHandle;
    int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 390, 0, 0, 0);
    Art* background = art_ptr_lock(backgroundFid, &backgroundHandle);
    if (background != NULL) {
        int width = art_frame_width(background, 0, 0);
        unsigned char* buffer = art_frame_data(background, 0, 0);

        // Clear "Weapon Used:".
        buf_to_buf(buffer + width * 20 + 112, 110, text_height(), width, windowBuffer + windowWidth * 20 + 112, windowWidth);

        // Clear "Armor Used:".
        buf_to_buf(buffer + width * 49 + 112, 110, text_height(), width, windowBuffer + windowWidth * 49 + 112, windowWidth);

        // Clear character preview.
        buf_to_buf(buffer + width * 84 + 8, 70, 98, width, windowBuffer + windowWidth * 84 + 8, windowWidth);

        // Clear ?
        buf_to_buf(buffer + width * 80 + 232, 132, 106, width, windowBuffer + windowWidth * 80 + 232, windowWidth);

        art_ptr_unlock(backgroundHandle);
    }

    MessageListItem messageListItem;
    char* text;
    char formattedText[256];

    // Render item in right hand.
    Object* item2 = inven_right_hand(dialog_target);
    text = item2 != NULL ? item_name(item2) : getmsg(&proto_main_msg_file, &messageListItem, 10);
    sprintf(formattedText, "%s", text);
    text_to_buf(windowBuffer + windowWidth * 20 + 112, formattedText, 110, windowWidth, colorTable[992]);

    // Render armor.
    Object* armor = inven_worn(dialog_target);
    text = armor != NULL ? item_name(armor) : getmsg(&proto_main_msg_file, &messageListItem, 10);
    sprintf(formattedText, "%s", text);
    text_to_buf(windowBuffer + windowWidth * 49 + 112, formattedText, 110, windowWidth, colorTable[992]);

    // Render preview.
    CacheEntry* previewHandle;
    int previewFid = art_id(FID_TYPE(dialog_target->fid), dialog_target->fid & 0xFFF, ANIM_STAND, (dialog_target->fid & 0xF000) >> 12, ROTATION_SW);
    Art* preview = art_ptr_lock(previewFid, &previewHandle);
    if (preview != NULL) {
        int width = art_frame_width(preview, 0, ROTATION_SW);
        int height = art_frame_length(preview, 0, ROTATION_SW);
        unsigned char* buffer = art_frame_data(preview, 0, ROTATION_SW);
        trans_buf_to_buf(buffer, width, height, width, windowBuffer + windowWidth * (132 - height / 2) + 39 - width / 2, windowWidth);
        art_ptr_unlock(previewHandle);
    }

    // Render hit points.
    int maximumHitPoints = critterGetStat(dialog_target, STAT_MAXIMUM_HIT_POINTS);
    int hitPoints = critterGetStat(dialog_target, STAT_CURRENT_HIT_POINTS);
    sprintf(formattedText, "%d/%d", hitPoints, maximumHitPoints);
    text_to_buf(windowBuffer + windowWidth * 96 + 240, formattedText, 115, windowWidth, colorTable[992]);

    // Render best skill.
    int bestSkill = partyMemberSkill(dialog_target);
    text = skill_name(bestSkill);
    sprintf(formattedText, "%s", text);
    text_to_buf(windowBuffer + windowWidth * 113 + 240, formattedText, 115, windowWidth, colorTable[992]);

    // Render weight summary.
    int inventoryWeight = item_total_weight(dialog_target);
    int carryWeight = critterGetStat(dialog_target, STAT_CARRY_WEIGHT);
    sprintf(formattedText, "%d/%d ", inventoryWeight, carryWeight);
    text_to_buf(windowBuffer + windowWidth * 131 + 240, formattedText, 115, windowWidth, critterIsOverloaded(dialog_target) ? colorTable[31744] : colorTable[992]);

    // Render melee damage.
    int meleeDamage = critterGetStat(dialog_target, STAT_MELEE_DAMAGE);
    sprintf(formattedText, "%d", meleeDamage);
    text_to_buf(windowBuffer + windowWidth * 148 + 240, formattedText, 115, windowWidth, colorTable[992]);

    int actionPoints;
    if (isInCombat()) {
        actionPoints = dialog_target->data.critter.combat.ap;
    } else {
        actionPoints = critterGetStat(dialog_target, STAT_MAXIMUM_ACTION_POINTS);
    }
    int maximumActionPoints = critterGetStat(dialog_target, STAT_MAXIMUM_ACTION_POINTS);
    sprintf(formattedText, "%d/%d ", actionPoints, maximumActionPoints);
    text_to_buf(windowBuffer + windowWidth * 167 + 240, formattedText, 115, windowWidth, colorTable[992]);

    text_font(oldFont);
    win_draw(dialogueWindow);
}

// 0x44928C
static void gdControlPressed(int btn, int keyCode)
{
    dialogue_switch_mode = 8;
    dialogue_state = 10;

    // NOTE: Uninline.
    gdHide();
}

// 0x4492D0
static int gdPickAIUpdateMsg(Object* critter)
{
    // TODO: Check.
    // 0x444D10
    static const int pids[3] = {
        0x1000088,
        0x1000156,
        0x1000180,
    };

    for (int index = 0; index < 3; index++) {
        if (critter->pid == pids[index]) {
            return 677 + roll_random(0, 1);
        }
    }

    return 670 + roll_random(0, 4);
}

// 0x449330
static int gdCanBarter()
{
    if (PID_TYPE(dialog_target->pid) != OBJ_TYPE_CRITTER) {
        return 1;
    }

    Proto* proto;
    if (proto_ptr(dialog_target->pid, &proto) == -1) {
        return 1;
    }

    if (proto->critter.data.flags & CRITTER_BARTER) {
        return 1;
    }

    MessageListItem messageListItem;

    // This person will not barter with you.
    messageListItem.num = 903;
    if (dialog_target_is_party) {
        // This critter can't carry anything.
        messageListItem.num = 913;
    }

    if (!message_search(&proto_main_msg_file, &messageListItem)) {
        debug_printf("\nError: gdialog: Can't find message!");
        return 0;
    }

    gdialogDisplayMsg(messageListItem.text);

    return 0;
}

// 0x4493B8
static void gdControl()
{
    MessageListItem messageListItem;

    bool done = false;
    while (!done) {
        int keyCode = get_input();
        if (keyCode != -1) {
            if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
                game_quit_with_confirm();
            }

            if (game_user_wants_to_quit != 0) {
                break;
            }

            if (keyCode == KEY_LOWERCASE_W) {
                inven_unwield(dialog_target, 1);

                Object* weapon = ai_search_inven_weap(dialog_target, 0, NULL);
                if (weapon != NULL) {
                    inven_wield(dialog_target, weapon, 1);
                    cai_attempt_w_reload(dialog_target, 0);

                    int num = gdPickAIUpdateMsg(dialog_target);
                    char* msg = getmsg(&proto_main_msg_file, &messageListItem, num);
                    gdialogDisplayMsg(msg);
                    gdControlUpdateInfo();
                }
            } else if (keyCode == 2098) {
                ai_set_disposition(dialog_target, 4);
            } else if (keyCode == 2099) {
                ai_set_disposition(dialog_target, 0);
                dialogue_state = 13;
                dialogue_switch_mode = 11;
                done = true;
            } else if (keyCode == 2102) {
                ai_set_disposition(dialog_target, 2);
            } else if (keyCode == 2103) {
                ai_set_disposition(dialog_target, 3);
            } else if (keyCode == 2111) {
                ai_set_disposition(dialog_target, 1);
            } else if (keyCode == KEY_ESCAPE) {
                dialogue_switch_mode = 1;
                dialogue_state = 1;
                return;
            } else if (keyCode == KEY_LOWERCASE_A) {
                if (dialog_target->pid != 0x10000A1) {
                    Object* armor = ai_search_inven_armor(dialog_target);
                    if (armor != NULL) {
                        inven_wield(dialog_target, armor, 0);
                    }
                }

                int num = gdPickAIUpdateMsg(dialog_target);
                char* msg = getmsg(&proto_main_msg_file, &messageListItem, num);
                gdialogDisplayMsg(msg);
                gdControlUpdateInfo();
            } else if (keyCode == KEY_LOWERCASE_D) {
                if (gdCanBarter()) {
                    dialogue_switch_mode = 2;
                    dialogue_state = 4;
                    return;
                }
            } else if (keyCode == -2) {
                if (mouse_click_in(441, 451, 540, 470)) {
                    ai_set_disposition(dialog_target, 0);
                    dialogue_state = 13;
                    dialogue_switch_mode = 11;
                    done = true;
                }
            }
        }
    }
}

// 0x4496A0
static int gdCustomCreateWin()
{
    if (!message_init(&custom_msg_file)) {
        return -1;
    }

    if (!message_load(&custom_msg_file, "game\\custom.msg")) {
        return -1;
    }

    CacheEntry* backgroundFrmHandle;
    int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 391, 0, 0, 0);
    Art* backgroundFrm = art_ptr_lock(backgroundFid, &backgroundFrmHandle);
    if (backgroundFrm == NULL) {
        return -1;
    }

    unsigned char* backgroundFrmData = art_frame_data(backgroundFrm, 0, 0);
    if (backgroundFrmData == NULL) {
        // FIXME: Leaking background.
        gdCustomDestroyWin();
        return -1;
    }

    dialogue_subwin_len = art_frame_length(backgroundFrm, 0, 0);

    int customizationWindowX = 0;
    int customizationWindowY = GAME_DIALOG_WINDOW_HEIGHT - dialogue_subwin_len;
    dialogueWindow = win_add(customizationWindowX,
        customizationWindowY,
        GAME_DIALOG_WINDOW_WIDTH,
        dialogue_subwin_len,
        256,
        WINDOW_FLAG_0x02);
    if (dialogueWindow == -1) {
        gdCustomDestroyWin();
        return -1;
    }

    unsigned char* windowBuffer = win_get_buf(dialogueWindow);
    unsigned char* parentWindowBuffer = win_get_buf(dialogueBackWindow);
    buf_to_buf(parentWindowBuffer + (GAME_DIALOG_WINDOW_HEIGHT - dialogue_subwin_len) * GAME_DIALOG_WINDOW_WIDTH,
        GAME_DIALOG_WINDOW_WIDTH,
        dialogue_subwin_len,
        GAME_DIALOG_WINDOW_WIDTH,
        windowBuffer,
        GAME_DIALOG_WINDOW_WIDTH);

    gdialog_scroll_subwin(dialogueWindow, 1, backgroundFrmData, windowBuffer, NULL, dialogue_subwin_len, 0);
    art_ptr_unlock(backgroundFrmHandle);

    gdialog_buttons[0] = win_register_button(dialogueWindow, 593, 101, 14, 14, -1, -1, -1, 13, dialog_red_button_up_buf, dialog_red_button_down_buf, 0, BUTTON_FLAG_TRANSPARENT);
    if (gdialog_buttons[0] == -1) {
        gdCustomDestroyWin();
        return -1;
    }

    win_register_button_sound_func(gdialog_buttons[0], gsound_med_butt_press, gsound_med_butt_release);

    int optionButton = 0;
    custom_buttons_start = 1;

    for (int index = 0; index < PARTY_MEMBER_CUSTOMIZATION_OPTION_COUNT; index++) {
        GameDialogButtonData* buttonData = &(custom_button_info[index]);

        int upButtonFid = art_id(OBJ_TYPE_INTERFACE, buttonData->upFrmId, 0, 0, 0);
        Art* upButtonFrm = art_ptr_lock(upButtonFid, &(buttonData->upFrmHandle));
        if (upButtonFrm == NULL) {
            gdCustomDestroyWin();
            return -1;
        }

        int width = art_frame_width(upButtonFrm, 0, 0);
        int height = art_frame_length(upButtonFrm, 0, 0);
        unsigned char* upButtonFrmData = art_frame_data(upButtonFrm, 0, 0);

        int downButtonFid = art_id(OBJ_TYPE_INTERFACE, buttonData->downFrmId, 0, 0, 0);
        Art* downButtonFrm = art_ptr_lock(downButtonFid, &(buttonData->downFrmHandle));
        if (downButtonFrm == NULL) {
            gdCustomDestroyWin();
            return -1;
        }

        unsigned char* downButtonFrmData = art_frame_data(downButtonFrm, 0, 0);

        optionButton++;
        gdialog_buttons[optionButton] = win_register_button(dialogueWindow,
            buttonData->x,
            buttonData->y,
            width,
            height,
            -1,
            -1,
            -1,
            buttonData->keyCode,
            upButtonFrmData,
            downButtonFrmData,
            NULL,
            BUTTON_FLAG_TRANSPARENT);
        if (gdialog_buttons[optionButton] == -1) {
            gdCustomDestroyWin();
            return -1;
        }

        win_register_button_sound_func(gdialog_buttons[index], gsound_med_butt_press, gsound_med_butt_release);
    }

    custom_current_selected[PARTY_MEMBER_CUSTOMIZATION_OPTION_AREA_ATTACK_MODE] = ai_get_burst_value(dialog_target);
    custom_current_selected[PARTY_MEMBER_CUSTOMIZATION_OPTION_RUN_AWAY_MODE] = ai_get_run_away_value(dialog_target);
    custom_current_selected[PARTY_MEMBER_CUSTOMIZATION_OPTION_BEST_WEAPON] = ai_get_weapon_pref_value(dialog_target);
    custom_current_selected[PARTY_MEMBER_CUSTOMIZATION_OPTION_DISTANCE] = ai_get_distance_pref_value(dialog_target);
    custom_current_selected[PARTY_MEMBER_CUSTOMIZATION_OPTION_ATTACK_WHO] = ai_get_attack_who_value(dialog_target);
    custom_current_selected[PARTY_MEMBER_CUSTOMIZATION_OPTION_CHEM_USE] = ai_get_chem_use_value(dialog_target);

    dialogue_state = 13;

    gdCustomUpdateInfo();

    return 0;
}

// 0x449A10
static void gdCustomDestroyWin()
{
    if (dialogueWindow == -1) {
        return;
    }

    for (int index = 0; index < 9; index++) {
        win_delete_button(gdialog_buttons[index]);
        gdialog_buttons[index] = -1;
    }

    for (int index = 0; index < PARTY_MEMBER_CUSTOMIZATION_OPTION_COUNT; index++) {
        GameDialogButtonData* buttonData = &(custom_button_info[index]);

        if (buttonData->upFrmHandle != NULL) {
            art_ptr_unlock(buttonData->upFrmHandle);
            buttonData->upFrmHandle = NULL;
        }

        if (buttonData->downFrmHandle != NULL) {
            art_ptr_unlock(buttonData->downFrmHandle);
            buttonData->downFrmHandle = NULL;
        }

        if (buttonData->disabledFrmHandle != NULL) {
            art_ptr_unlock(buttonData->disabledFrmHandle);
            buttonData->disabledFrmHandle = NULL;
        }
    }

    CacheEntry* backgroundFrmHandle;
    // custom.frm - party member control interface
    int fid = art_id(OBJ_TYPE_INTERFACE, 391, 0, 0, 0);
    unsigned char* backgroundFrmData = art_ptr_lock_data(fid, 0, 0, &backgroundFrmHandle);
    if (backgroundFrmData != NULL) {
        gdialog_scroll_subwin(dialogueWindow, 0, backgroundFrmData, win_get_buf(dialogueWindow), win_get_buf(dialogueBackWindow) + (GAME_DIALOG_WINDOW_WIDTH) * (480 - dialogue_subwin_len), dialogue_subwin_len, 0);
        art_ptr_unlock(backgroundFrmHandle);
    }

    win_delete(dialogueWindow);
    dialogueWindow = -1;

    message_exit(&custom_msg_file);
}

// 0x449B3C
static void gdCustom()
{
    bool done = false;
    while (!done) {
        unsigned int keyCode = get_input();
        if (keyCode != -1) {
            if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
                game_quit_with_confirm();
            }

            if (game_user_wants_to_quit != 0) {
                break;
            }

            if (keyCode <= 5) {
                gdCustomSelect(keyCode);
                gdCustomUpdateInfo();
            } else if (keyCode == KEY_RETURN || keyCode == KEY_ESCAPE) {
                done = true;
                dialogue_switch_mode = 8;
                dialogue_state = 10;
            }
        }
    }
}

// 0x449BB4
static void gdCustomUpdateInfo()
{
    int oldFont = text_curr();
    text_font(101);

    unsigned char* windowBuffer = win_get_buf(dialogueWindow);
    int windowWidth = win_width(dialogueWindow);

    CacheEntry* backgroundHandle;
    int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 391, 0, 0, 0);
    Art* background = art_ptr_lock(backgroundFid, &backgroundHandle);
    if (background == NULL) {
        return;
    }

    int backgroundWidth = art_frame_width(background, 0, 0);
    int backgroundHeight = art_frame_length(background, 0, 0);
    unsigned char* backgroundData = art_frame_data(background, 0, 0);
    buf_to_buf(backgroundData, backgroundWidth, backgroundHeight, backgroundWidth, windowBuffer, GAME_DIALOG_WINDOW_WIDTH);

    art_ptr_unlock(backgroundHandle);

    MessageListItem messageListItem;
    int num;
    char* msg;

    // BURST
    if (custom_current_selected[PARTY_MEMBER_CUSTOMIZATION_OPTION_AREA_ATTACK_MODE] == -1) {
        // Not Applicable
        num = 99;
    } else {
        debug_printf("\nburst: %d", custom_current_selected[PARTY_MEMBER_CUSTOMIZATION_OPTION_AREA_ATTACK_MODE]);
        num = custom_settings[PARTY_MEMBER_CUSTOMIZATION_OPTION_AREA_ATTACK_MODE][custom_current_selected[PARTY_MEMBER_CUSTOMIZATION_OPTION_AREA_ATTACK_MODE]].messageId;
    }

    msg = getmsg(&custom_msg_file, &messageListItem, num);
    text_to_buf(windowBuffer + windowWidth * 20 + 232, msg, 248, windowWidth, colorTable[992]);

    // RUN AWAY
    msg = getmsg(&custom_msg_file, &messageListItem, custom_settings[PARTY_MEMBER_CUSTOMIZATION_OPTION_RUN_AWAY_MODE][custom_current_selected[PARTY_MEMBER_CUSTOMIZATION_OPTION_RUN_AWAY_MODE]].messageId);
    text_to_buf(windowBuffer + windowWidth * 48 + 232, msg, 248, windowWidth, colorTable[992]);

    // WEAPON PREF
    msg = getmsg(&custom_msg_file, &messageListItem, custom_settings[PARTY_MEMBER_CUSTOMIZATION_OPTION_BEST_WEAPON][custom_current_selected[PARTY_MEMBER_CUSTOMIZATION_OPTION_BEST_WEAPON]].messageId);
    text_to_buf(windowBuffer + windowWidth * 78 + 232, msg, 248, windowWidth, colorTable[992]);

    // DISTANCE
    msg = getmsg(&custom_msg_file, &messageListItem, custom_settings[PARTY_MEMBER_CUSTOMIZATION_OPTION_DISTANCE][custom_current_selected[PARTY_MEMBER_CUSTOMIZATION_OPTION_DISTANCE]].messageId);
    text_to_buf(windowBuffer + windowWidth * 108 + 232, msg, 248, windowWidth, colorTable[992]);

    // ATTACK WHO
    msg = getmsg(&custom_msg_file, &messageListItem, custom_settings[PARTY_MEMBER_CUSTOMIZATION_OPTION_ATTACK_WHO][custom_current_selected[PARTY_MEMBER_CUSTOMIZATION_OPTION_ATTACK_WHO]].messageId);
    text_to_buf(windowBuffer + windowWidth * 137 + 232, msg, 248, windowWidth, colorTable[992]);

    // CHEM USE
    msg = getmsg(&custom_msg_file, &messageListItem, custom_settings[PARTY_MEMBER_CUSTOMIZATION_OPTION_CHEM_USE][custom_current_selected[PARTY_MEMBER_CUSTOMIZATION_OPTION_CHEM_USE]].messageId);
    text_to_buf(windowBuffer + windowWidth * 166 + 232, msg, 248, windowWidth, colorTable[992]);

    win_draw(dialogueWindow);
    text_font(oldFont);
}

// 0x449E64
static void gdCustomSelectRedraw(unsigned char* dest, int pitch, int type, int selectedIndex)
{
    MessageListItem messageListItem;

    text_font(101);

    for (int index = 0; index < 6; index++) {
        STRUCT_5189E4* ptr = &(custom_settings[type][index]);
        if (ptr->messageId != -1) {
            bool enabled = false;
            switch (type) {
            case PARTY_MEMBER_CUSTOMIZATION_OPTION_AREA_ATTACK_MODE:
                enabled = partyMemberHasAIBurstValue(dialog_target, ptr->value);
                break;
            case PARTY_MEMBER_CUSTOMIZATION_OPTION_RUN_AWAY_MODE:
                enabled = partyMemberHasAIRunAwayValue(dialog_target, ptr->value);
                break;
            case PARTY_MEMBER_CUSTOMIZATION_OPTION_BEST_WEAPON:
                enabled = partyMemberHasAIWeaponPrefValue(dialog_target, ptr->value);
                break;
            case PARTY_MEMBER_CUSTOMIZATION_OPTION_DISTANCE:
                enabled = partyMemberHasAIDistancePrefValue(dialog_target, ptr->value);
                break;
            case PARTY_MEMBER_CUSTOMIZATION_OPTION_ATTACK_WHO:
                enabled = partyMemberHasAIAttackWhoValue(dialog_target, ptr->value);
                break;
            case PARTY_MEMBER_CUSTOMIZATION_OPTION_CHEM_USE:
                enabled = partyMemberHasAIChemUseValue(dialog_target, ptr->value);
                break;
            }

            int color;
            if (enabled) {
                if (index == selectedIndex) {
                    color = colorTable[32747];
                } else {
                    color = colorTable[992];
                }
            } else {
                color = colorTable[15855];
            }

            const char* msg = getmsg(&custom_msg_file, &messageListItem, ptr->messageId);
            text_to_buf(dest + pitch * (text_height() * index + 42) + 42, msg, pitch - 84, pitch, color);
        }
    }
}

// 0x449FC0
static int gdCustomSelect(int a1)
{
    int oldFont = text_curr();

    CacheEntry* backgroundFrmHandle;
    int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 419, 0, 0, 0);
    Art* backgroundFrm = art_ptr_lock(backgroundFid, &backgroundFrmHandle);
    if (backgroundFrm == NULL) {
        return -1;
    }

    int backgroundFrmWidth = art_frame_width(backgroundFrm, 0, 0);
    int backgroundFrmHeight = art_frame_length(backgroundFrm, 0, 0);

    int selectWindowX = (640 - backgroundFrmWidth) / 2;
    int selectWindowY = (480 - backgroundFrmHeight) / 2;
    int win = win_add(selectWindowX, selectWindowY, backgroundFrmWidth, backgroundFrmHeight, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
    if (win == -1) {
        art_ptr_unlock(backgroundFrmHandle);
        return -1;
    }

    unsigned char* windowBuffer = win_get_buf(win);
    unsigned char* backgroundFrmData = art_frame_data(backgroundFrm, 0, 0);
    buf_to_buf(backgroundFrmData,
        backgroundFrmWidth,
        backgroundFrmHeight,
        backgroundFrmWidth,
        windowBuffer,
        backgroundFrmWidth);

    art_ptr_unlock(backgroundFrmHandle);

    int btn1 = win_register_button(win, 70, 164, 14, 14, -1, -1, -1, KEY_RETURN, dialog_red_button_up_buf, dialog_red_button_down_buf, NULL, BUTTON_FLAG_TRANSPARENT);
    if (btn1 == -1) {
        win_delete(win);
        return -1;
    }

    int btn2 = win_register_button(win, 176, 163, 14, 14, -1, -1, -1, KEY_ESCAPE, dialog_red_button_up_buf, dialog_red_button_down_buf, NULL, BUTTON_FLAG_TRANSPARENT);
    if (btn2 == -1) {
        win_delete(win);
        return -1;
    }

    text_font(103);

    MessageListItem messageListItem;
    const char* msg;

    msg = getmsg(&custom_msg_file, &messageListItem, a1);
    text_to_buf(windowBuffer + backgroundFrmWidth * 15 + 40, msg, backgroundFrmWidth, backgroundFrmWidth, colorTable[18979]);

    msg = getmsg(&custom_msg_file, &messageListItem, 10);
    text_to_buf(windowBuffer + backgroundFrmWidth * 163 + 88, msg, backgroundFrmWidth, backgroundFrmWidth, colorTable[18979]);

    msg = getmsg(&custom_msg_file, &messageListItem, 11);
    text_to_buf(windowBuffer + backgroundFrmWidth * 162 + 193, msg, backgroundFrmWidth, backgroundFrmWidth, colorTable[18979]);

    int value = custom_current_selected[a1];
    gdCustomSelectRedraw(windowBuffer, backgroundFrmWidth, a1, value);
    win_draw(win);

    int minX = selectWindowX + 42;
    int minY = selectWindowY + 42;
    int maxX = selectWindowX + backgroundFrmWidth - 42;
    int maxY = selectWindowY + backgroundFrmHeight - 42;

    bool done = false;
    unsigned int v53 = 0;
    while (!done) {
        int keyCode = get_input();
        if (keyCode == -1) {
            continue;
        }

        if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
            game_quit_with_confirm();
        }

        if (game_user_wants_to_quit != 0) {
            break;
        }

        if (keyCode == KEY_RETURN) {
            STRUCT_5189E4* ptr = &(custom_settings[a1][value]);
            custom_current_selected[a1] = value;
            gdCustomUpdateSetting(a1, ptr->value);
            done = true;
        } else if (keyCode == KEY_ESCAPE) {
            done = true;
        } else if (keyCode == -2) {
            if ((mouse_get_buttons() & MOUSE_EVENT_LEFT_BUTTON_UP) == 0) {
                continue;
            }

            if (!mouse_click_in(minX, minY, maxX, maxY)) {
                continue;
            }

            int mouseX;
            int mouseY;
            mouse_get_position(&mouseX, &mouseY);

            int lineHeight = text_height();
            int newValue = (mouseY - minY) / lineHeight;
            if (newValue >= 6) {
                continue;
            }

            unsigned int timestamp = get_time();
            if (newValue == value) {
                if (elapsed_tocks(timestamp, v53) < 250) {
                    custom_current_selected[a1] = newValue;
                    gdCustomUpdateSetting(a1, newValue);
                    done = true;
                }
            } else {
                STRUCT_5189E4* ptr = &(custom_settings[a1][newValue]);
                if (ptr->messageId != -1) {
                    bool enabled = false;
                    switch (a1) {
                    case PARTY_MEMBER_CUSTOMIZATION_OPTION_AREA_ATTACK_MODE:
                        enabled = partyMemberHasAIBurstValue(dialog_target, ptr->value);
                        break;
                    case PARTY_MEMBER_CUSTOMIZATION_OPTION_RUN_AWAY_MODE:
                        enabled = partyMemberHasAIRunAwayValue(dialog_target, ptr->value);
                        break;
                    case PARTY_MEMBER_CUSTOMIZATION_OPTION_BEST_WEAPON:
                        enabled = partyMemberHasAIWeaponPrefValue(dialog_target, ptr->value);
                        break;
                    case PARTY_MEMBER_CUSTOMIZATION_OPTION_DISTANCE:
                        enabled = partyMemberHasAIDistancePrefValue(dialog_target, ptr->value);
                        break;
                    case PARTY_MEMBER_CUSTOMIZATION_OPTION_ATTACK_WHO:
                        enabled = partyMemberHasAIAttackWhoValue(dialog_target, ptr->value);
                        break;
                    case PARTY_MEMBER_CUSTOMIZATION_OPTION_CHEM_USE:
                        enabled = partyMemberHasAIChemUseValue(dialog_target, ptr->value);
                        break;
                    }

                    if (enabled) {
                        value = newValue;
                        gdCustomSelectRedraw(windowBuffer, backgroundFrmWidth, a1, newValue);
                        win_draw(win);
                    }
                }
            }
            v53 = timestamp;
        }
    }

    win_delete(win);
    text_font(oldFont);
    return 0;
}

// 0x44A4E0
static void gdCustomUpdateSetting(int option, int value)
{
    switch (option) {
    case PARTY_MEMBER_CUSTOMIZATION_OPTION_AREA_ATTACK_MODE:
        ai_set_burst_value(dialog_target, value);
        break;
    case PARTY_MEMBER_CUSTOMIZATION_OPTION_RUN_AWAY_MODE:
        ai_set_run_away_value(dialog_target, value);
        break;
    case PARTY_MEMBER_CUSTOMIZATION_OPTION_BEST_WEAPON:
        ai_set_weapon_pref_value(dialog_target, value);
        break;
    case PARTY_MEMBER_CUSTOMIZATION_OPTION_DISTANCE:
        ai_set_distance_pref_value(dialog_target, value);
        break;
    case PARTY_MEMBER_CUSTOMIZATION_OPTION_ATTACK_WHO:
        ai_set_attack_who_value(dialog_target, value);
        break;
    case PARTY_MEMBER_CUSTOMIZATION_OPTION_CHEM_USE:
        ai_set_chem_use_value(dialog_target, value);
        break;
    }
}

// 0x44A52C
static void gdialog_barter_pressed(int btn, int keyCode)
{
    if (PID_TYPE(dialog_target->pid) != OBJ_TYPE_CRITTER) {
        return;
    }

    Script* script;
    if (scr_ptr(dialog_target->sid, &script) == -1) {
        return;
    }

    Proto* proto;
    proto_ptr(dialog_target->pid, &proto);
    if (proto->critter.data.flags & CRITTER_BARTER) {
        if (gdialog_speech_playing) {
            if (soundPlaying(lip_info.sound)) {
                gdialogFreeSpeech();
            }
        }

        dialogue_switch_mode = 2;
        dialogue_state = 4;

        // NOTE: Uninline.
        gdHide();
    } else {
        MessageListItem messageListItem;
        // This person will not barter with you.
        messageListItem.num = 903;
        if (dialog_target_is_party) {
            // This critter can't carry anything.
            messageListItem.num = 913;
        }

        if (message_search(&proto_main_msg_file, &messageListItem)) {
            gdialogDisplayMsg(messageListItem.text);
        } else {
            debug_printf("\nError: gdialog: Can't find message!");
        }
    }
}

// 0x44A62C
static int gdialog_window_create()
{
    const int screenWidth = GAME_DIALOG_WINDOW_WIDTH;

    if (gdialog_window_created) {
        return -1;
    }

    for (int index = 0; index < 9; index++) {
        gdialog_buttons[index] = -1;
    }

    CacheEntry* backgroundFrmHandle;
    // 389 - di_talkp.frm - dialog screen subwindow (party members)
    // 99 - di_talk.frm - dialog screen subwindow (NPC's)
    int backgroundFid = art_id(OBJ_TYPE_INTERFACE, dialog_target_is_party ? 389 : 99, 0, 0, 0);
    Art* backgroundFrm = art_ptr_lock(backgroundFid, &backgroundFrmHandle);
    if (backgroundFrm == NULL) {
        return -1;
    }

    unsigned char* backgroundFrmData = art_frame_data(backgroundFrm, 0, 0);
    if (backgroundFrmData != NULL) {
        dialogue_subwin_len = art_frame_length(backgroundFrm, 0, 0);

        int dialogSubwindowX = 0;
        int dialogSubwindowY = 480 - dialogue_subwin_len;
        dialogueWindow = win_add(dialogSubwindowX, dialogSubwindowY, screenWidth, dialogue_subwin_len, 256, WINDOW_FLAG_0x02);
        if (dialogueWindow != -1) {

            unsigned char* v10 = win_get_buf(dialogueWindow);
            unsigned char* v14 = win_get_buf(dialogueBackWindow);
            // TODO: Not sure about offsets.
            buf_to_buf(v14 + screenWidth * (GAME_DIALOG_WINDOW_HEIGHT - dialogue_subwin_len), screenWidth, dialogue_subwin_len, screenWidth, v10, screenWidth);

            if (dialogue_just_started) {
                win_draw(dialogueBackWindow);
                gdialog_scroll_subwin(dialogueWindow, 1, backgroundFrmData, v10, 0, dialogue_subwin_len, -1);
                dialogue_just_started = 0;
            } else {
                gdialog_scroll_subwin(dialogueWindow, 1, backgroundFrmData, v10, 0, dialogue_subwin_len, 0);
            }

            art_ptr_unlock(backgroundFrmHandle);

            // BARTER/TRADE
            gdialog_buttons[0] = win_register_button(dialogueWindow, 593, 41, 14, 14, -1, -1, -1, -1, dialog_red_button_up_buf, dialog_red_button_down_buf, NULL, BUTTON_FLAG_TRANSPARENT);
            if (gdialog_buttons[0] != -1) {
                win_register_button_func(gdialog_buttons[0], NULL, NULL, NULL, gdialog_barter_pressed);
                win_register_button_sound_func(gdialog_buttons[0], gsound_med_butt_press, gsound_med_butt_release);

                // di_rest1.frm - dialog rest button up
                int upFid = art_id(OBJ_TYPE_INTERFACE, 97, 0, 0, 0);
                unsigned char* reviewButtonUpData = art_ptr_lock_data(upFid, 0, 0, &gdialog_review_up_key);
                if (reviewButtonUpData != NULL) {
                    // di_rest2.frm - dialog rest button down
                    int downFid = art_id(OBJ_TYPE_INTERFACE, 98, 0, 0, 0);
                    unsigned char* reivewButtonDownData = art_ptr_lock_data(downFid, 0, 0, &gdialog_review_down_key);
                    if (reivewButtonDownData != NULL) {
                        // REVIEW
                        gdialog_buttons[1] = win_register_button(dialogueWindow, 13, 154, 51, 29, -1, -1, -1, -1, reviewButtonUpData, reivewButtonDownData, NULL, 0);
                        if (gdialog_buttons[1] != -1) {
                            win_register_button_func(gdialog_buttons[1], NULL, NULL, NULL, gdReviewPressed);
                            win_register_button_sound_func(gdialog_buttons[1], gsound_red_butt_press, gsound_red_butt_release);

                            if (!dialog_target_is_party) {
                                gdialog_window_created = true;
                                return 0;
                            }

                            // COMBAT CONTROL
                            gdialog_buttons[2] = win_register_button(dialogueWindow, 593, 116, 14, 14, -1, -1, -1, -1, dialog_red_button_up_buf, dialog_red_button_down_buf, 0, BUTTON_FLAG_TRANSPARENT);
                            if (gdialog_buttons[2] != -1) {
                                win_register_button_func(gdialog_buttons[2], NULL, NULL, NULL, gdControlPressed);
                                win_register_button_sound_func(gdialog_buttons[2], gsound_med_butt_press, gsound_med_butt_release);

                                gdialog_window_created = true;
                                return 0;
                            }

                            win_delete_button(gdialog_buttons[1]);
                            gdialog_buttons[1] = -1;
                        }

                        art_ptr_unlock(gdialog_review_down_key);
                    }

                    art_ptr_unlock(gdialog_review_up_key);
                }

                win_delete_button(gdialog_buttons[0]);
                gdialog_buttons[0] = -1;
            }

            win_delete(dialogueWindow);
            dialogueWindow = -1;
        }
    }

    art_ptr_unlock(backgroundFrmHandle);

    return -1;
}

// 0x44A9D8
static void gdialog_window_destroy()
{
    if (dialogueWindow == -1) {
        return;
    }

    for (int index = 0; index < 9; index++) {
        win_delete_button(gdialog_buttons[index]);
        gdialog_buttons[index] = -1;
    }

    art_ptr_unlock(gdialog_review_down_key);
    art_ptr_unlock(gdialog_review_up_key);

    int offset = (GAME_DIALOG_WINDOW_WIDTH) * (480 - dialogue_subwin_len);
    unsigned char* backgroundWindowBuffer = win_get_buf(dialogueBackWindow) + offset;

    int frmId;
    if (dialog_target_is_party) {
        // di_talkp.frm - dialog screen subwindow (party members)
        frmId = 389;
    } else {
        // di_talk.frm - dialog screen subwindow (NPC's)
        frmId = 99;
    }

    CacheEntry* backgroundFrmHandle;
    int fid = art_id(OBJ_TYPE_INTERFACE, frmId, 0, 0, 0);
    unsigned char* backgroundFrmData = art_ptr_lock_data(fid, 0, 0, &backgroundFrmHandle);
    if (backgroundFrmData != NULL) {
        unsigned char* windowBuffer = win_get_buf(dialogueWindow);
        gdialog_scroll_subwin(dialogueWindow, 0, backgroundFrmData, windowBuffer, backgroundWindowBuffer, dialogue_subwin_len, 0);
        art_ptr_unlock(backgroundFrmHandle);
        win_delete(dialogueWindow);
        gdialog_window_created = 0;
        dialogueWindow = -1;
    }
}

// NOTE: Inlined.
//
// 0x44AAD8
static int talk_to_create_background_window()
{
    dialogueBackWindow = win_add(0,
        0,
        scr_size.lrx - scr_size.ulx + 1,
        GAME_DIALOG_WINDOW_HEIGHT,
        256,
        WINDOW_FLAG_0x02);

    if (dialogueBackWindow != -1) {
        return 0;
    }

    return -1;
}

// 0x44AB18
static int talk_to_refresh_background_window()
{
    CacheEntry* backgroundFrmHandle;
    // alltlk.frm - dialog screen background
    int fid = art_id(OBJ_TYPE_INTERFACE, 103, 0, 0, 0);
    unsigned char* backgroundFrmData = art_ptr_lock_data(fid, 0, 0, &backgroundFrmHandle);
    if (backgroundFrmData == NULL) {
        return -1;
    }

    int windowWidth = GAME_DIALOG_WINDOW_WIDTH;
    unsigned char* windowBuffer = win_get_buf(dialogueBackWindow);
    buf_to_buf(backgroundFrmData, windowWidth, 480, windowWidth, windowBuffer, windowWidth);
    art_ptr_unlock(backgroundFrmHandle);

    if (!dialogue_just_started) {
        win_draw(dialogueBackWindow);
    }

    return 0;
}

// 0x44ABA8
static int talkToRefreshDialogWindowRect(Rect* rect)
{
    int frmId;
    if (dialog_target_is_party) {
        // di_talkp.frm - dialog screen subwindow (party members)
        frmId = 389;
    } else {
        // di_talk.frm - dialog screen subwindow (NPC's)
        frmId = 99;
    }

    CacheEntry* backgroundFrmHandle;
    int fid = art_id(OBJ_TYPE_INTERFACE, frmId, 0, 0, 0);
    unsigned char* backgroundFrmData = art_ptr_lock_data(fid, 0, 0, &backgroundFrmHandle);
    if (backgroundFrmData == NULL) {
        return -1;
    }

    int offset = 640 * rect->uly + rect->ulx;

    unsigned char* windowBuffer = win_get_buf(dialogueWindow);
    buf_to_buf(backgroundFrmData + offset,
        rect->lrx - rect->ulx,
        rect->lry - rect->uly,
        GAME_DIALOG_WINDOW_WIDTH,
        windowBuffer + offset,
        GAME_DIALOG_WINDOW_WIDTH);

    art_ptr_unlock(backgroundFrmHandle);

    win_draw_rect(dialogueWindow, rect);

    return 0;
}

// 0x44AC68
static void talk_to_translucent_trans_buf_to_buf(unsigned char* src, int srcWidth, int srcHeight, int srcPitch, unsigned char* dest, int destX, int destY, int destPitch, unsigned char* a9, unsigned char* a10)
{
    int srcStep = srcPitch - srcWidth;
    int destStep = destPitch - srcWidth;

    dest += destPitch * destY + destX;

    for (int y = 0; y < srcHeight; y++) {
        for (int x = 0; x < srcWidth; x++) {
            unsigned char v1 = *src++;
            if (v1 != 0) {
                v1 = (256 - v1) >> 4;
            }

            unsigned char v15 = *dest;
            *dest++ = a9[256 * v1 + v15];
        }
        src += srcStep;
        dest += destStep;
    }
}

// 0x44ACFC
static void gdDisplayFrame(Art* headFrm, int frame)
{
    // 0x518BF4
    static int totalHotx = 0;

    if (dialogueWindow == -1) {
        return;
    }

    if (headFrm != NULL) {
        if (frame == 0) {
            totalHotx = 0;
        }

        int backgroundFid = art_id(OBJ_TYPE_BACKGROUND, backgroundIndex, 0, 0, 0);

        CacheEntry* backgroundHandle;
        Art* backgroundFrm = art_ptr_lock(backgroundFid, &backgroundHandle);
        if (backgroundFrm == NULL) {
            debug_printf("\tError locking background in display...\n");
        }

        unsigned char* backgroundFrmData = art_frame_data(backgroundFrm, 0, 0);
        if (backgroundFrmData != NULL) {
            buf_to_buf(backgroundFrmData, 388, 200, 388, headWindowBuffer, GAME_DIALOG_WINDOW_WIDTH);
        } else {
            debug_printf("\tError getting background data in display...\n");
        }

        art_ptr_unlock(backgroundHandle);

        int width = art_frame_width(headFrm, frame, 0);
        int height = art_frame_length(headFrm, frame, 0);
        unsigned char* data = art_frame_data(headFrm, frame, 0);

        int a3;
        int v8;
        art_frame_offset(headFrm, 0, &a3, &v8);

        int a4;
        int a5;
        art_frame_hot(headFrm, frame, 0, &a4, &a5);

        totalHotx += a4;
        a3 += totalHotx;

        if (data != NULL) {
            int destWidth = GAME_DIALOG_WINDOW_WIDTH;
            int destOffset = destWidth * (200 - height) + a3 + (388 - width) / 2;
            if (destOffset + width * v8 > 0) {
                destOffset += width * v8;
            }

            trans_buf_to_buf(
                data,
                width,
                height,
                width,
                headWindowBuffer + destOffset,
                destWidth);
        } else {
            debug_printf("\tError getting head data in display...\n");
        }
    } else {
        if (talk_need_to_center == 1) {
            talk_need_to_center = 0;
            tile_refresh_display();
        }

        unsigned char* src = win_get_buf(display_win);
        buf_to_buf(
            src + ((scr_size.lry - scr_size.uly + 1 - 332) / 2) * (GAME_DIALOG_WINDOW_WIDTH) + (GAME_DIALOG_WINDOW_WIDTH - 388) / 2,
            388,
            200,
            scr_size.lrx - scr_size.ulx + 1,
            headWindowBuffer,
            GAME_DIALOG_WINDOW_WIDTH);
    }

    Rect v27;
    v27.ulx = 126;
    v27.uly = 14;
    v27.lrx = 514;
    v27.lry = 214;

    unsigned char* dest = win_get_buf(dialogueBackWindow);

    unsigned char* data1 = art_frame_data(upper_hi_fp, 0, 0);
    talk_to_translucent_trans_buf_to_buf(data1, upper_hi_wid, upper_hi_len, upper_hi_wid, dest, 426, 15, GAME_DIALOG_WINDOW_WIDTH, light_BlendTable, light_GrayTable);

    unsigned char* data2 = art_frame_data(lower_hi_fp, 0, 0);
    talk_to_translucent_trans_buf_to_buf(data2, lower_hi_wid, lower_hi_len, lower_hi_wid, dest, 129, 214 - lower_hi_len - 2, GAME_DIALOG_WINDOW_WIDTH, dark_BlendTable, dark_GrayTable);

    for (int index = 0; index < 8; ++index) {
        Rect* rect = &(backgrndRects[index]);
        int width = rect->lrx - rect->ulx;

        trans_buf_to_buf(backgrndBufs[index],
            width,
            rect->lry - rect->uly,
            width,
            dest + (GAME_DIALOG_WINDOW_WIDTH) * rect->uly + rect->ulx,
            GAME_DIALOG_WINDOW_WIDTH);
    }

    win_draw_rect(dialogueBackWindow, &v27);
}

// 0x44B080
static void gdBlendTableInit()
{
    for (int color = 0; color < 256; color++) {
        int r = (Color2RGB(color) & 0x7C00) >> 10;
        int g = (Color2RGB(color) & 0x3E0) >> 5;
        int b = Color2RGB(color) & 0x1F;
        light_GrayTable[color] = ((r + 2 * g + 2 * b) / 10) >> 2;
        dark_GrayTable[color] = ((r + g + b) / 10) >> 2;
    }

    light_GrayTable[0] = 0;
    dark_GrayTable[0] = 0;

    light_BlendTable = getColorBlendTable(colorTable[17969]);
    dark_BlendTable = getColorBlendTable(colorTable[22187]);

    // hilight1.frm - dialogue upper hilight
    int upperHighlightFid = art_id(OBJ_TYPE_INTERFACE, 115, 0, 0, 0);
    upper_hi_fp = art_ptr_lock(upperHighlightFid, &upper_hi_key);
    upper_hi_wid = art_frame_width(upper_hi_fp, 0, 0);
    upper_hi_len = art_frame_length(upper_hi_fp, 0, 0);

    // hilight2.frm - dialogue lower hilight
    int lowerHighlightFid = art_id(OBJ_TYPE_INTERFACE, 116, 0, 0, 0);
    lower_hi_fp = art_ptr_lock(lowerHighlightFid, &lower_hi_key);
    lower_hi_wid = art_frame_width(lower_hi_fp, 0, 0);
    lower_hi_len = art_frame_length(lower_hi_fp, 0, 0);
}

// NOTE: Inlined.
//
// 0x44B1D4
static void gdBlendTableExit()
{
    freeColorBlendTable(colorTable[17969]);
    freeColorBlendTable(colorTable[22187]);

    art_ptr_unlock(upper_hi_key);
    art_ptr_unlock(lower_hi_key);
}
