#include "game/loadsave.h"

#include <assert.h>
#include <direct.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "game/automap.h"
#include "game/editor.h"
#include "plib/color/color.h"
#include "game/combat.h"
#include "game/combatai.h"
#include "plib/gnw/input.h"
#include "game/critter.h"
#include "game/cycle.h"
#include "game/bmpdlog.h"
#include "plib/gnw/button.h"
#include "plib/gnw/debug.h"
#include "game/display.h"
#include "plib/gnw/grbuf.h"
#include "game/gz.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/gmouse.h"
#include "game/gmovie.h"
#include "game/gsound.h"
#include "game/intface.h"
#include "game/item.h"
#include "game/map.h"
#include "plib/gnw/memory.h"
#include "game/object.h"
#include "game/options.h"
#include "game/perk.h"
#include "game/pipboy.h"
#include "game/proto.h"
#include "game/queue.h"
#include "game/roll.h"
#include "game/scripts.h"
#include "game/skill.h"
#include "game/stat.h"
#include "plib/gnw/text.h"
#include "game/tile.h"
#include "game/trait.h"
#include "game/version.h"
#include "plib/gnw/gnw.h"
#include "game/wordwrap.h"
#include "game/worldmap.h"

#define LOAD_SAVE_SIGNATURE "FALLOUT SAVE FILE"
#define LOAD_SAVE_DESCRIPTION_LENGTH 30
#define LOAD_SAVE_HANDLER_COUNT 27

#define LSGAME_MSG_NAME "LSGAME.MSG"

#define LS_WINDOW_WIDTH 640
#define LS_WINDOW_HEIGHT 480

#define LS_PREVIEW_WIDTH 224
#define LS_PREVIEW_HEIGHT 133
#define LS_PREVIEW_SIZE ((LS_PREVIEW_WIDTH) * (LS_PREVIEW_HEIGHT))

#define LS_COMMENT_WINDOW_X 169
#define LS_COMMENT_WINDOW_Y 116

typedef int LoadGameHandler(File* stream);
typedef int SaveGameHandler(File* stream);

typedef enum LoadSaveWindowType {
    LOAD_SAVE_WINDOW_TYPE_SAVE_GAME,
    LOAD_SAVE_WINDOW_TYPE_PICK_QUICK_SAVE_SLOT,
    LOAD_SAVE_WINDOW_TYPE_LOAD_GAME,
    LOAD_SAVE_WINDOW_TYPE_LOAD_GAME_FROM_MAIN_MENU,
    LOAD_SAVE_WINDOW_TYPE_PICK_QUICK_LOAD_SLOT,
} LoadSaveWindowType;

typedef enum LoadSaveSlotState {
    SLOT_STATE_EMPTY,
    SLOT_STATE_OCCUPIED,
    SLOT_STATE_ERROR,
    SLOT_STATE_UNSUPPORTED_VERSION,
} LoadSaveSlotState;

typedef enum LoadSaveScrollDirection {
    LOAD_SAVE_SCROLL_DIRECTION_NONE,
    LOAD_SAVE_SCROLL_DIRECTION_UP,
    LOAD_SAVE_SCROLL_DIRECTION_DOWN,
} LoadSaveScrollDirection;

typedef struct LoadSaveSlotData {
    char signature[24];
    short versionMinor;
    short versionMajor;
    // TODO: The type is probably char, but it's read with the same function as
    // reading unsigned chars, which in turn probably result of collapsing
    // reading functions.
    unsigned char versionRelease;
    char characterName[32];
    char description[LOAD_SAVE_DESCRIPTION_LENGTH];
    short fileMonth;
    short fileDay;
    short fileYear;
    int fileTime;
    short gameMonth;
    short gameDay;
    short gameYear;
    int gameTime;
    short elevation;
    short map;
    char fileName[16];
} LoadSaveSlotData;

typedef enum LoadSaveFrm {
    LOAD_SAVE_FRM_BACKGROUND,
    LOAD_SAVE_FRM_BOX,
    LOAD_SAVE_FRM_PREVIEW_COVER,
    LOAD_SAVE_FRM_RED_BUTTON_PRESSED,
    LOAD_SAVE_FRM_RED_BUTTON_NORMAL,
    LOAD_SAVE_FRM_ARROW_DOWN_NORMAL,
    LOAD_SAVE_FRM_ARROW_DOWN_PRESSED,
    LOAD_SAVE_FRM_ARROW_UP_NORMAL,
    LOAD_SAVE_FRM_ARROW_UP_PRESSED,
    LOAD_SAVE_FRM_COUNT,
} LoadSaveFrm;

static int QuickSnapShot();
static int LSGameStart(int windowType);
static int LSGameEnd(int windowType);
static int SaveSlot();
static int LoadSlot(int slot);
static int SaveHeader(int slot);
static int LoadHeader(int slot);
static int GetSlotList();
static void ShowSlotList(int a1);
static void DrawInfoBox(int a1);
static int LoadTumbSlot(int a1);
static int GetComment(int a1);
static int get_input_str2(int win, int doneKeyCode, int cancelKeyCode, char* description, int maxLength, int x, int y, int textColor, int backgroundColor, int flags);
static int DummyFunc(File* stream);
static int PrepLoad(File* stream);
static int EndLoad(File* stream);
static int GameMap2Slot(File* stream);
static int SlotMap2Game(File* stream);
static int mygets(char* dest, File* stream);
static int copy_file(const char* a1, const char* a2);
static int SaveBackup();
static int RestoreSave();
static int LoadObjDudeCid(File* stream);
static int SaveObjDudeCid(File* stream);
static int EraseSave();

// 0x47B7C0
static const int lsgrphs[LOAD_SAVE_FRM_COUNT] = {
    237, // lsgame.frm - load/save game
    238, // lsgbox.frm - load/save game
    239, // lscover.frm - load/save game
    9, // lilreddn.frm - little red button down
    8, // lilredup.frm - little red button up
    181, // dnarwoff.frm - character editor
    182, // dnarwon.frm - character editor
    199, // uparwoff.frm - character editor
    200, // uparwon.frm - character editor
};

// 0x5193B8
static int slot_cursor = 0;

// 0x5193BC
static bool quick_done = false;

// 0x5193C0
static bool bk_enable = false;

// 0x5193C4
static int map_backup_count = -1;

// 0x5193C8
static int automap_db_flag = 0;

// 0x5193CC
static char* patches = NULL;

// 0x5193D0
static char emgpath[] = "\\FALLOUT\\CD\\DATA\\SAVEGAME";

// 0x5193EC
static SaveGameHandler* master_save_list[LOAD_SAVE_HANDLER_COUNT] = {
    DummyFunc,
    SaveObjDudeCid,
    scr_game_save,
    GameMap2Slot,
    scr_game_save,
    obj_save_dude,
    critter_save,
    critter_kill_count_save,
    skill_save,
    roll_save,
    perk_save,
    combat_save,
    combat_ai_save,
    stat_save,
    item_save,
    trait_save,
    automap_save,
    save_options,
    editor_save,
    wmWorldMap_save,
    save_pipboy,
    gmovie_save,
    skill_use_slot_save,
    partyMemberSave,
    queue_save,
    intface_save,
    DummyFunc,
};

// 0x519458
static LoadGameHandler* master_load_list[LOAD_SAVE_HANDLER_COUNT] = {
    PrepLoad,
    LoadObjDudeCid,
    scr_game_load,
    SlotMap2Game,
    scr_game_load2,
    obj_load_dude,
    critter_load,
    critter_kill_count_load,
    skill_load,
    roll_load,
    perk_load,
    combat_load,
    combat_ai_load,
    stat_load,
    item_load,
    trait_load,
    automap_load,
    load_options,
    editor_load,
    wmWorldMap_load,
    load_pipboy,
    gmovie_load,
    skill_use_slot_load,
    partyMemberLoad,
    queue_load,
    intface_load,
    EndLoad,
};

// 0x5194C4
static int loadingGame = 0;

// 0x613CE0
static Size ginfo[LOAD_SAVE_FRM_COUNT];

// lsgame.msg
//
// 0x613D28
static MessageList lsgame_msgfl;

// 0x613D30
static LoadSaveSlotData LSData[10];

// 0x614280
static int LSstatus[10];

// 0x6142A8
static unsigned char* thumbnail_image[2];

// 0x6142B0
static MessageListItem lsgmesg;

// 0x6142C0
static int dbleclkcntr;

// 0x6142C4
static int lsgwin;

// 0x6142C8
static unsigned char* lsbmp[LOAD_SAVE_FRM_COUNT];

// 0x6142EC
static unsigned char* snapshot;

// 0x6142F0
static char str2[MAX_PATH];

// 0x6143F4
static char str0[MAX_PATH];

// 0x6144F8
static char str1[MAX_PATH];

// 0x6145FC
static char str[MAX_PATH];

// 0x614700
static unsigned char* lsgbuf;

// 0x614704
static char gmpath[MAX_PATH];

// 0x614808
static File* flptr;

// 0x61480C
static int ls_error_code;

// 0x614810
static int fontsave;

// 0x614814
static CacheEntry* grphkey[LOAD_SAVE_FRM_COUNT];

// 0x47B7E4
void InitLoadSave()
{
    quick_done = false;
    slot_cursor = 0;

    if (!config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_MASTER_PATCHES_KEY, &patches)) {
        debug_printf("\nLOADSAVE: Error reading patches config variable! Using default.\n");
        patches = emgpath;
    }

    MapDirErase("MAPS\\", "SAV");
    MapDirErase("PROTO\\CRITTERS\\", "PRO");
    MapDirErase("PROTO\\ITEMS\\", "PRO");
}

// 0x47B85C
void ResetLoadSave()
{
    MapDirErase("MAPS\\", "SAV");
    MapDirErase("PROTO\\CRITTERS\\", "PRO");
    MapDirErase("PROTO\\ITEMS\\", "PRO");
}

// SaveGame
// 0x47B88C
int SaveGame(int mode)
{
    MessageListItem messageListItem;

    ls_error_code = 0;

    if (!config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_MASTER_PATCHES_KEY, &patches)) {
        debug_printf("\nLOADSAVE: Error reading patches config variable! Using default.\n");
        patches = emgpath;
    }

    if (mode == LOAD_SAVE_MODE_QUICK && quick_done) {
        sprintf(gmpath, "%s\\%s%.2d\\", "SAVEGAME", "SLOT", slot_cursor + 1);
        strcat(gmpath, "SAVE.DAT");

        flptr = db_fopen(gmpath, "rb");
        if (flptr != NULL) {
            LoadHeader(slot_cursor);
            db_fclose(flptr);
        }

        thumbnail_image[1] = NULL;
        int v6 = QuickSnapShot();
        if (v6 == 1) {
            int v7 = SaveSlot();
            if (v7 != -1) {
                v6 = v7;
            }
        }

        if (thumbnail_image[1] != NULL) {
            mem_free(snapshot);
        }

        gmouse_set_cursor(MOUSE_CURSOR_ARROW);

        if (v6 != -1) {
            return 1;
        }

        if (!message_init(&lsgame_msgfl)) {
            return -1;
        }

        char path[MAX_PATH];
        sprintf(path, "%s%s", msg_path, "LSGAME.MSG");
        if (!message_load(&lsgame_msgfl, path)) {
            return -1;
        }

        gsound_play_sfx_file("iisxxxx1");

        // Error saving game!
        strcpy(str0, getmsg(&lsgame_msgfl, &messageListItem, 132));
        // Unable to save game.
        strcpy(str1, getmsg(&lsgame_msgfl, &messageListItem, 133));

        const char* body[] = {
            str1,
        };
        dialog_out(str0, body, 1, 169, 116, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_LARGE);

        message_exit(&lsgame_msgfl);

        return -1;
    }

    quick_done = false;

    int windowType = mode == LOAD_SAVE_MODE_QUICK
        ? LOAD_SAVE_WINDOW_TYPE_PICK_QUICK_SAVE_SLOT
        : LOAD_SAVE_WINDOW_TYPE_SAVE_GAME;
    if (LSGameStart(windowType) == -1) {
        debug_printf("\nLOADSAVE: ** Error loading save game screen data! **\n");
        return -1;
    }

    if (GetSlotList() == -1) {
        win_draw(lsgwin);

        gsound_play_sfx_file("iisxxxx1");

        // Error loading save game list!
        strcpy(str0, getmsg(&lsgame_msgfl, &messageListItem, 106));
        // Save game directory:
        strcpy(str1, getmsg(&lsgame_msgfl, &messageListItem, 107));

        sprintf(str2, "\"%s\\\"", "SAVEGAME");

        // TODO: Check.
        strcpy(str2, getmsg(&lsgame_msgfl, &messageListItem, 108));

        const char* body[] = {
            str1,
            str2,
        };
        dialog_out(str0, body, 2, 169, 116, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_LARGE);

        LSGameEnd(0);

        return -1;
    }

    switch (LSstatus[slot_cursor]) {
    case SLOT_STATE_EMPTY:
    case SLOT_STATE_ERROR:
    case SLOT_STATE_UNSUPPORTED_VERSION:
        buf_to_buf(thumbnail_image[1],
            LS_PREVIEW_WIDTH - 1,
            LS_PREVIEW_HEIGHT - 1,
            LS_PREVIEW_WIDTH,
            lsgbuf + LS_WINDOW_WIDTH * 58 + 366,
            LS_WINDOW_WIDTH);
        break;
    default:
        LoadTumbSlot(slot_cursor);
        buf_to_buf(thumbnail_image[0],
            LS_PREVIEW_WIDTH - 1,
            LS_PREVIEW_HEIGHT - 1,
            LS_PREVIEW_WIDTH,
            lsgbuf + LS_WINDOW_WIDTH * 58 + 366,
            LS_WINDOW_WIDTH);
        break;
    }

    ShowSlotList(0);
    DrawInfoBox(slot_cursor);
    win_draw(lsgwin);

    dbleclkcntr = 24;

    int rc = -1;
    int doubleClickSlot = -1;
    while (rc == -1) {
        unsigned int tick = get_time();
        int keyCode = get_input();
        bool selectionChanged = false;
        int scrollDirection = LOAD_SAVE_SCROLL_DIRECTION_NONE;

        if (keyCode == KEY_ESCAPE || keyCode == 501 || game_user_wants_to_quit != 0) {
            rc = 0;
        } else {
            switch (keyCode) {
            case KEY_ARROW_UP:
                slot_cursor -= 1;
                if (slot_cursor < 0) {
                    slot_cursor = 0;
                }
                selectionChanged = true;
                doubleClickSlot = -1;
                break;
            case KEY_ARROW_DOWN:
                slot_cursor += 1;
                if (slot_cursor > 9) {
                    slot_cursor = 9;
                }
                selectionChanged = true;
                doubleClickSlot = -1;
                break;
            case KEY_HOME:
                slot_cursor = 0;
                selectionChanged = true;
                doubleClickSlot = -1;
                break;
            case KEY_END:
                slot_cursor = 9;
                selectionChanged = true;
                doubleClickSlot = -1;
                break;
            case 506:
                scrollDirection = LOAD_SAVE_SCROLL_DIRECTION_UP;
                break;
            case 504:
                scrollDirection = LOAD_SAVE_SCROLL_DIRECTION_DOWN;
                break;
            case 502:
                if (1) {
                    int mouseX;
                    int mouseY;
                    mouse_get_position(&mouseX, &mouseY);

                    slot_cursor = (mouseY - 79) / (3 * text_height() + 4);
                    if (slot_cursor < 0) {
                        slot_cursor = 0;
                    }
                    if (slot_cursor > 9) {
                        slot_cursor = 9;
                    }

                    selectionChanged = true;

                    if (slot_cursor == doubleClickSlot) {
                        keyCode = 500;
                        gsound_play_sfx_file("ib1p1xx1");
                    }

                    doubleClickSlot = slot_cursor;
                    scrollDirection = LOAD_SAVE_SCROLL_DIRECTION_NONE;
                }
                break;
            case KEY_CTRL_Q:
            case KEY_CTRL_X:
            case KEY_F10:
                game_quit_with_confirm();

                if (game_user_wants_to_quit != 0) {
                    rc = 0;
                }
                break;
            case KEY_PLUS:
            case KEY_EQUAL:
                IncGamma();
                break;
            case KEY_MINUS:
            case KEY_UNDERSCORE:
                DecGamma();
                break;
            case KEY_RETURN:
                keyCode = 500;
                break;
            }
        }

        if (keyCode == 500) {
            if (LSstatus[slot_cursor] == SLOT_STATE_OCCUPIED) {
                rc = 1;
                // Save game already exists, overwrite?
                const char* title = getmsg(&lsgame_msgfl, &lsgmesg, 131);
                if (dialog_out(title, NULL, 0, 169, 131, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_YES_NO) == 0) {
                    rc = -1;
                }
            } else {
                rc = 1;
            }

            selectionChanged = true;
            scrollDirection = LOAD_SAVE_SCROLL_DIRECTION_NONE;
        }

        if (scrollDirection) {
            unsigned int scrollVelocity = 4;
            bool isScrolling = false;
            int scrollCounter = 0;
            do {
                unsigned int start = get_time();
                scrollCounter += 1;

                if ((!isScrolling && scrollCounter == 1) || (isScrolling && scrollCounter > 14.4)) {
                    isScrolling = true;

                    if (scrollCounter > 14.4) {
                        scrollVelocity += 1;
                        if (scrollVelocity > 24) {
                            scrollVelocity = 24;
                        }
                    }

                    if (scrollDirection == LOAD_SAVE_SCROLL_DIRECTION_UP) {
                        slot_cursor -= 1;
                        if (slot_cursor < 0) {
                            slot_cursor = 0;
                        }
                    } else {
                        slot_cursor += 1;
                        if (slot_cursor > 9) {
                            slot_cursor = 9;
                        }
                    }

                    // TODO: Does not check for unsupported version error like
                    // other switches do.
                    switch (LSstatus[slot_cursor]) {
                    case SLOT_STATE_EMPTY:
                    case SLOT_STATE_ERROR:
                        buf_to_buf(thumbnail_image[1],
                            LS_PREVIEW_WIDTH - 1,
                            LS_PREVIEW_HEIGHT - 1,
                            LS_PREVIEW_WIDTH,
                            lsgbuf + LS_WINDOW_WIDTH * 58 + 366,
                            LS_WINDOW_WIDTH);
                        break;
                    default:
                        LoadTumbSlot(slot_cursor);
                        buf_to_buf(thumbnail_image[0],
                            LS_PREVIEW_WIDTH - 1,
                            LS_PREVIEW_HEIGHT - 1,
                            LS_PREVIEW_WIDTH,
                            lsgbuf + LS_WINDOW_WIDTH * 58 + 366,
                            LS_WINDOW_WIDTH);
                        break;
                    }

                    ShowSlotList(LOAD_SAVE_WINDOW_TYPE_SAVE_GAME);
                    DrawInfoBox(slot_cursor);
                    win_draw(lsgwin);
                }

                if (scrollCounter > 14.4) {
                    while (elapsed_time(start) < 1000 / scrollVelocity) { }
                } else {
                    while (elapsed_time(start) < 1000 / 24) { }
                }

                keyCode = get_input();
            } while (keyCode != 505 && keyCode != 503);
        } else {
            if (selectionChanged) {
                switch (LSstatus[slot_cursor]) {
                case SLOT_STATE_EMPTY:
                case SLOT_STATE_ERROR:
                case SLOT_STATE_UNSUPPORTED_VERSION:
                    buf_to_buf(thumbnail_image[1],
                        LS_PREVIEW_WIDTH - 1,
                        LS_PREVIEW_HEIGHT - 1,
                        LS_PREVIEW_WIDTH,
                        lsgbuf + LS_WINDOW_WIDTH * 58 + 366,
                        LS_WINDOW_WIDTH);
                    break;
                default:
                    LoadTumbSlot(slot_cursor);
                    buf_to_buf(thumbnail_image[0],
                        LS_PREVIEW_WIDTH - 1,
                        LS_PREVIEW_HEIGHT - 1,
                        LS_PREVIEW_WIDTH,
                        lsgbuf + LS_WINDOW_WIDTH * 58 + 366,
                        LS_WINDOW_WIDTH);
                    break;
                }

                DrawInfoBox(slot_cursor);
                ShowSlotList(LOAD_SAVE_WINDOW_TYPE_SAVE_GAME);
            }

            win_draw(lsgwin);

            dbleclkcntr -= 1;
            if (dbleclkcntr == 0) {
                dbleclkcntr = 24;
                doubleClickSlot = -1;
            }

            while (elapsed_time(tick) < 1000 / 24) {
            }
        }

        if (rc == 1) {
            int v50 = GetComment(slot_cursor);
            if (v50 == -1) {
                gmouse_set_cursor(MOUSE_CURSOR_ARROW);
                gsound_play_sfx_file("iisxxxx1");
                debug_printf("\nLOADSAVE: ** Error getting save file comment **\n");

                // Error saving game!
                strcpy(str0, getmsg(&lsgame_msgfl, &lsgmesg, 132));
                // Unable to save game.
                strcpy(str1, getmsg(&lsgame_msgfl, &lsgmesg, 133));

                const char* body[1] = {
                    str1,
                };
                dialog_out(str0, body, 1, 169, 116, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_LARGE);
                rc = -1;
            } else if (v50 == 0) {
                gmouse_set_cursor(MOUSE_CURSOR_ARROW);
                rc = -1;
            } else if (v50 == 1) {
                if (SaveSlot() == -1) {
                    gmouse_set_cursor(MOUSE_CURSOR_ARROW);
                    gsound_play_sfx_file("iisxxxx1");

                    // Error saving game!
                    strcpy(str0, getmsg(&lsgame_msgfl, &lsgmesg, 132));
                    // Unable to save game.
                    strcpy(str1, getmsg(&lsgame_msgfl, &lsgmesg, 133));

                    rc = -1;

                    const char* body[1] = {
                        str1,
                    };
                    dialog_out(str0, body, 1, 169, 116, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_LARGE);

                    if (GetSlotList() == -1) {
                        win_draw(lsgwin);
                        gsound_play_sfx_file("iisxxxx1");

                        // Error loading save agme list!
                        strcpy(str0, getmsg(&lsgame_msgfl, &lsgmesg, 106));
                        // Save game directory:
                        strcpy(str1, getmsg(&lsgame_msgfl, &lsgmesg, 107));

                        sprintf(str2, "\"%s\\\"", "SAVEGAME");

                        char text[260];
                        // Doesn't exist or is corrupted.
                        strcpy(text, getmsg(&lsgame_msgfl, &lsgmesg, 107));

                        const char* body[2] = {
                            str1,
                            str2,
                        };
                        dialog_out(str0, body, 2, 169, 116, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_LARGE);

                        LSGameEnd(0);

                        return -1;
                    }

                    switch (LSstatus[slot_cursor]) {
                    case SLOT_STATE_EMPTY:
                    case SLOT_STATE_ERROR:
                    case SLOT_STATE_UNSUPPORTED_VERSION:
                        buf_to_buf(thumbnail_image[1],
                            LS_PREVIEW_WIDTH - 1,
                            LS_PREVIEW_HEIGHT - 1,
                            LS_PREVIEW_WIDTH,
                            lsgbuf + LS_WINDOW_WIDTH * 58 + 366,
                            LS_WINDOW_WIDTH);
                        break;
                    default:
                        LoadTumbSlot(slot_cursor);
                        buf_to_buf(thumbnail_image[0],
                            LS_PREVIEW_WIDTH - 1,
                            LS_PREVIEW_HEIGHT - 1,
                            LS_PREVIEW_WIDTH,
                            lsgbuf + LS_WINDOW_WIDTH * 58 + 366,
                            LS_WINDOW_WIDTH);
                        break;
                    }

                    ShowSlotList(LOAD_SAVE_WINDOW_TYPE_SAVE_GAME);
                    DrawInfoBox(slot_cursor);
                    win_draw(lsgwin);
                    dbleclkcntr = 24;
                }
            }
        }
    }

    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    LSGameEnd(LOAD_SAVE_WINDOW_TYPE_SAVE_GAME);

    tile_refresh_display();

    if (mode == LOAD_SAVE_MODE_QUICK) {
        if (rc == 1) {
            quick_done = true;
        }
    }

    return rc;
}

// 0x47C5B4
static int QuickSnapShot()
{
    snapshot = (unsigned char*)mem_malloc(LS_PREVIEW_SIZE);
    if (snapshot == NULL) {
        return -1;
    }

    bool gameMouseWasVisible = gmouse_3d_is_on();
    if (gameMouseWasVisible) {
        gmouse_3d_off();
    }

    mouse_hide();
    tile_refresh_display();
    mouse_show();

    if (gameMouseWasVisible) {
        gmouse_3d_on();
    }

    unsigned char* windowBuffer = win_get_buf(display_win);
    cscale(windowBuffer, 640, 380, 640, snapshot, LS_PREVIEW_WIDTH, LS_PREVIEW_HEIGHT, LS_PREVIEW_WIDTH);

    thumbnail_image[1] = snapshot;

    return 1;
}

// LoadGame
// 0x47C640
int LoadGame(int mode)
{
    MessageListItem messageListItem;

    const char* body[] = {
        str1,
        str2,
    };

    ls_error_code = 0;

    if (!config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_MASTER_PATCHES_KEY, &patches)) {
        debug_printf("\nLOADSAVE: Error reading patches config variable! Using default.\n");
        patches = emgpath;
    }

    if (mode == LOAD_SAVE_MODE_QUICK && quick_done) {
        int quickSaveWindowX = 0;
        int quickSaveWindowY = 0;
        int window = win_add(quickSaveWindowX,
            quickSaveWindowY,
            LS_WINDOW_WIDTH,
            LS_WINDOW_HEIGHT,
            256,
            WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);
        if (window != -1) {
            unsigned char* windowBuffer = win_get_buf(window);
            buf_fill(windowBuffer, LS_WINDOW_WIDTH, LS_WINDOW_HEIGHT, LS_WINDOW_WIDTH, colorTable[0]);
            win_draw(window);
        }

        if (LoadSlot(slot_cursor) != -1) {
            if (window != -1) {
                win_delete(window);
            }
            gmouse_set_cursor(MOUSE_CURSOR_ARROW);
            return 1;
        }

        if (!message_init(&lsgame_msgfl)) {
            return -1;
        }

        char path[MAX_PATH];
        sprintf(path, "%s\\%s", msg_path, "LSGAME.MSG");
        if (!message_load(&lsgame_msgfl, path)) {
            return -1;
        }

        if (window != -1) {
            win_delete(window);
        }

        gmouse_set_cursor(MOUSE_CURSOR_ARROW);
        gsound_play_sfx_file("iisxxxx1");
        strcpy(str0, getmsg(&lsgame_msgfl, &messageListItem, 134));
        strcpy(str1, getmsg(&lsgame_msgfl, &messageListItem, 135));
        dialog_out(str0, body, 1, 169, 116, colorTable[32328], 0, colorTable[32328], DIALOG_BOX_LARGE);

        message_exit(&lsgame_msgfl);
        map_new_map();
        game_user_wants_to_quit = 2;

        return -1;
    }

    quick_done = false;

    int windowType;
    switch (mode) {
    case LOAD_SAVE_MODE_FROM_MAIN_MENU:
        windowType = LOAD_SAVE_WINDOW_TYPE_LOAD_GAME_FROM_MAIN_MENU;
        break;
    case LOAD_SAVE_MODE_NORMAL:
        windowType = LOAD_SAVE_WINDOW_TYPE_LOAD_GAME;
        break;
    case LOAD_SAVE_MODE_QUICK:
        windowType = LOAD_SAVE_WINDOW_TYPE_PICK_QUICK_LOAD_SLOT;
        break;
    default:
        assert(false && "Should be unreachable");
    }

    if (LSGameStart(windowType) == -1) {
        debug_printf("\nLOADSAVE: ** Error loading save game screen data! **\n");
        return -1;
    }

    if (GetSlotList() == -1) {
        gmouse_set_cursor(MOUSE_CURSOR_ARROW);
        win_draw(lsgwin);
        gsound_play_sfx_file("iisxxxx1");
        strcpy(str0, getmsg(&lsgame_msgfl, &lsgmesg, 106));
        strcpy(str1, getmsg(&lsgame_msgfl, &lsgmesg, 107));
        sprintf(str2, "\"%s\\\"", "SAVEGAME");
        dialog_out(str0, body, 2, 169, 116, colorTable[32328], 0, colorTable[32328], DIALOG_BOX_LARGE);
        LSGameEnd(windowType);
        return -1;
    }

    switch (LSstatus[slot_cursor]) {
    case SLOT_STATE_EMPTY:
    case SLOT_STATE_ERROR:
    case SLOT_STATE_UNSUPPORTED_VERSION:
        buf_to_buf(lsbmp[LOAD_SAVE_FRM_PREVIEW_COVER],
            ginfo[LOAD_SAVE_FRM_PREVIEW_COVER].width,
            ginfo[LOAD_SAVE_FRM_PREVIEW_COVER].height,
            ginfo[LOAD_SAVE_FRM_PREVIEW_COVER].width,
            lsgbuf + LS_WINDOW_WIDTH * 39 + 340,
            LS_WINDOW_WIDTH);
        break;
    default:
        LoadTumbSlot(slot_cursor);
        buf_to_buf(thumbnail_image[0],
            LS_PREVIEW_WIDTH - 1,
            LS_PREVIEW_HEIGHT - 1,
            LS_PREVIEW_WIDTH,
            lsgbuf + LS_WINDOW_WIDTH * 58 + 366,
            LS_WINDOW_WIDTH);
        break;
    }

    ShowSlotList(2);
    DrawInfoBox(slot_cursor);
    win_draw(lsgwin);
    dbleclkcntr = 24;

    int rc = -1;
    int doubleClickSlot = -1;
    while (rc == -1) {
        unsigned int time = get_time();
        int keyCode = get_input();
        bool selectionChanged = false;
        int scrollDirection = 0;

        if (keyCode == KEY_ESCAPE || keyCode == 501 || game_user_wants_to_quit != 0) {
            rc = 0;
        } else {
            switch (keyCode) {
            case KEY_ARROW_UP:
                if (--slot_cursor < 0) {
                    slot_cursor = 0;
                }
                selectionChanged = true;
                doubleClickSlot = -1;
                break;
            case KEY_ARROW_DOWN:
                if (++slot_cursor > 9) {
                    slot_cursor = 9;
                }
                selectionChanged = true;
                doubleClickSlot = -1;
                break;
            case KEY_HOME:
                slot_cursor = 0;
                selectionChanged = true;
                doubleClickSlot = -1;
                break;
            case KEY_END:
                slot_cursor = 9;
                selectionChanged = true;
                doubleClickSlot = -1;
                break;
            case 506:
                scrollDirection = LOAD_SAVE_SCROLL_DIRECTION_UP;
                break;
            case 504:
                scrollDirection = LOAD_SAVE_SCROLL_DIRECTION_DOWN;
                break;
            case 502:
                if (1) {
                    int mouseX;
                    int mouseY;
                    mouse_get_position(&mouseX, &mouseY);

                    int clickedSlot = (mouseY - 79) / (3 * text_height() + 4);
                    if (clickedSlot < 0) {
                        clickedSlot = 0;
                    } else if (clickedSlot > 9) {
                        clickedSlot = 9;
                    }

                    slot_cursor = clickedSlot;
                    if (clickedSlot == doubleClickSlot) {
                        keyCode = 500;
                        gsound_play_sfx_file("ib1p1xx1");
                    }

                    selectionChanged = true;
                    scrollDirection = LOAD_SAVE_SCROLL_DIRECTION_NONE;
                    doubleClickSlot = slot_cursor;
                }
                break;
            case KEY_MINUS:
            case KEY_UNDERSCORE:
                DecGamma();
                break;
            case KEY_EQUAL:
            case KEY_PLUS:
                IncGamma();
                break;
            case KEY_RETURN:
                keyCode = 500;
                break;
            case KEY_CTRL_Q:
            case KEY_CTRL_X:
            case KEY_F10:
                game_quit_with_confirm();
                if (game_user_wants_to_quit != 0) {
                    rc = 0;
                }
                break;
            }
        }

        if (keyCode == 500) {
            if (LSstatus[slot_cursor] != SLOT_STATE_EMPTY) {
                rc = 1;
            } else {
                rc = -1;
            }

            selectionChanged = true;
            scrollDirection = LOAD_SAVE_SCROLL_DIRECTION_NONE;
        }

        if (scrollDirection != LOAD_SAVE_SCROLL_DIRECTION_NONE) {
            unsigned int scrollVelocity = 4;
            bool isScrolling = false;
            int scrollCounter = 0;
            do {
                unsigned int start = get_time();
                scrollCounter += 1;

                if ((!isScrolling && scrollCounter == 1) || (isScrolling && scrollCounter > 14.4)) {
                    isScrolling = true;

                    if (scrollCounter > 14.4) {
                        scrollVelocity += 1;
                        if (scrollVelocity > 24) {
                            scrollVelocity = 24;
                        }
                    }

                    if (scrollDirection == LOAD_SAVE_SCROLL_DIRECTION_UP) {
                        slot_cursor -= 1;
                        if (slot_cursor < 0) {
                            slot_cursor = 0;
                        }
                    } else {
                        slot_cursor += 1;
                        if (slot_cursor > 9) {
                            slot_cursor = 9;
                        }
                    }

                    switch (LSstatus[slot_cursor]) {
                    case SLOT_STATE_EMPTY:
                    case SLOT_STATE_ERROR:
                    case SLOT_STATE_UNSUPPORTED_VERSION:
                        buf_to_buf(lsbmp[LOAD_SAVE_FRM_PREVIEW_COVER],
                            ginfo[LOAD_SAVE_FRM_PREVIEW_COVER].width,
                            ginfo[LOAD_SAVE_FRM_PREVIEW_COVER].height,
                            ginfo[LOAD_SAVE_FRM_PREVIEW_COVER].width,
                            lsgbuf + LS_WINDOW_WIDTH * 39 + 340,
                            LS_WINDOW_WIDTH);
                        break;
                    default:
                        LoadTumbSlot(slot_cursor);
                        buf_to_buf(lsbmp[LOAD_SAVE_FRM_BACKGROUND] + LS_WINDOW_WIDTH * 39 + 340,
                            ginfo[LOAD_SAVE_FRM_PREVIEW_COVER].width,
                            ginfo[LOAD_SAVE_FRM_PREVIEW_COVER].height,
                            LS_WINDOW_WIDTH,
                            lsgbuf + LS_WINDOW_WIDTH * 39 + 340,
                            LS_WINDOW_WIDTH);
                        buf_to_buf(thumbnail_image[0],
                            LS_PREVIEW_WIDTH - 1,
                            LS_PREVIEW_HEIGHT - 1,
                            LS_PREVIEW_WIDTH,
                            lsgbuf + LS_WINDOW_WIDTH * 58 + 366,
                            LS_WINDOW_WIDTH);
                        break;
                    }

                    ShowSlotList(2);
                    DrawInfoBox(slot_cursor);
                    win_draw(lsgwin);
                }

                if (scrollCounter > 14.4) {
                    while (elapsed_time(start) < 1000 / scrollVelocity) { }
                } else {
                    while (elapsed_time(start) < 1000 / 24) { }
                }

                keyCode = get_input();
            } while (keyCode != 505 && keyCode != 503);
        } else {
            if (selectionChanged) {
                switch (LSstatus[slot_cursor]) {
                case SLOT_STATE_EMPTY:
                case SLOT_STATE_ERROR:
                case SLOT_STATE_UNSUPPORTED_VERSION:
                    buf_to_buf(lsbmp[LOAD_SAVE_FRM_PREVIEW_COVER],
                        ginfo[LOAD_SAVE_FRM_PREVIEW_COVER].width,
                        ginfo[LOAD_SAVE_FRM_PREVIEW_COVER].height,
                        ginfo[LOAD_SAVE_FRM_PREVIEW_COVER].width,
                        lsgbuf + LS_WINDOW_WIDTH * 39 + 340,
                        LS_WINDOW_WIDTH);
                    break;
                default:
                    LoadTumbSlot(slot_cursor);
                    buf_to_buf(lsbmp[LOAD_SAVE_FRM_BACKGROUND] + LS_WINDOW_WIDTH * 39 + 340,
                        ginfo[LOAD_SAVE_FRM_PREVIEW_COVER].width,
                        ginfo[LOAD_SAVE_FRM_PREVIEW_COVER].height,
                        LS_WINDOW_WIDTH,
                        lsgbuf + LS_WINDOW_WIDTH * 39 + 340,
                        LS_WINDOW_WIDTH);
                    buf_to_buf(thumbnail_image[0],
                        LS_PREVIEW_WIDTH - 1,
                        LS_PREVIEW_HEIGHT - 1,
                        LS_PREVIEW_WIDTH,
                        lsgbuf + LS_WINDOW_WIDTH * 58 + 366,
                        LS_WINDOW_WIDTH);
                    break;
                }

                DrawInfoBox(slot_cursor);
                ShowSlotList(2);
            }

            win_draw(lsgwin);

            dbleclkcntr -= 1;
            if (dbleclkcntr == 0) {
                dbleclkcntr = 24;
                doubleClickSlot = -1;
            }

            while (elapsed_time(time) < 1000 / 24) { }
        }

        if (rc == 1) {
            switch (LSstatus[slot_cursor]) {
            case SLOT_STATE_UNSUPPORTED_VERSION:
                gsound_play_sfx_file("iisxxxx1");
                strcpy(str0, getmsg(&lsgame_msgfl, &lsgmesg, 134));
                strcpy(str1, getmsg(&lsgame_msgfl, &lsgmesg, 136));
                strcpy(str2, getmsg(&lsgame_msgfl, &lsgmesg, 135));
                dialog_out(str0, body, 2, 169, 116, colorTable[32328], 0, colorTable[32328], DIALOG_BOX_LARGE);
                rc = -1;
                break;
            case SLOT_STATE_ERROR:
                gsound_play_sfx_file("iisxxxx1");
                strcpy(str0, getmsg(&lsgame_msgfl, &lsgmesg, 134));
                strcpy(str1, getmsg(&lsgame_msgfl, &lsgmesg, 136));
                dialog_out(str0, body, 1, 169, 116, colorTable[32328], 0, colorTable[32328], DIALOG_BOX_LARGE);
                rc = -1;
                break;
            default:
                if (LoadSlot(slot_cursor) == -1) {
                    gmouse_set_cursor(MOUSE_CURSOR_ARROW);
                    gsound_play_sfx_file("iisxxxx1");
                    strcpy(str0, getmsg(&lsgame_msgfl, &lsgmesg, 134));
                    strcpy(str1, getmsg(&lsgame_msgfl, &lsgmesg, 135));
                    dialog_out(str0, body, 1, 169, 116, colorTable[32328], 0, colorTable[32328], DIALOG_BOX_LARGE);
                    map_new_map();
                    game_user_wants_to_quit = 2;
                    rc = -1;
                }
                break;
            }
        }
    }

    LSGameEnd(mode == LOAD_SAVE_MODE_FROM_MAIN_MENU
            ? LOAD_SAVE_WINDOW_TYPE_LOAD_GAME_FROM_MAIN_MENU
            : LOAD_SAVE_WINDOW_TYPE_LOAD_GAME);

    if (mode == LOAD_SAVE_MODE_QUICK) {
        if (rc == 1) {
            quick_done = true;
        }
    }

    return rc;
}

// 0x47D2E4
static int LSGameStart(int windowType)
{
    fontsave = text_curr();
    text_font(103);

    bk_enable = false;
    if (!message_init(&lsgame_msgfl)) {
        return -1;
    }

    sprintf(str, "%s%s", msg_path, LSGAME_MSG_NAME);
    if (!message_load(&lsgame_msgfl, str)) {
        return -1;
    }

    snapshot = (unsigned char*)mem_malloc(61632);
    if (snapshot == NULL) {
        message_exit(&lsgame_msgfl);
        text_font(fontsave);
        return -1;
    }

    thumbnail_image[0] = snapshot;
    thumbnail_image[1] = snapshot + LS_PREVIEW_SIZE;

    if (windowType != LOAD_SAVE_WINDOW_TYPE_LOAD_GAME_FROM_MAIN_MENU) {
        bk_enable = map_disable_bk_processes();
    }

    cycle_disable();

    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    if (windowType == LOAD_SAVE_WINDOW_TYPE_SAVE_GAME || windowType == LOAD_SAVE_WINDOW_TYPE_PICK_QUICK_SAVE_SLOT) {
        bool gameMouseWasVisible = gmouse_3d_is_on();
        if (gameMouseWasVisible) {
            gmouse_3d_off();
        }

        mouse_hide();
        tile_refresh_display();
        mouse_show();

        if (gameMouseWasVisible) {
            gmouse_3d_on();
        }

        unsigned char* windowBuf = win_get_buf(display_win);
        cscale(windowBuf, 640, 380, 640, thumbnail_image[1], LS_PREVIEW_WIDTH, LS_PREVIEW_HEIGHT, LS_PREVIEW_WIDTH);
    }

    for (int index = 0; index < LOAD_SAVE_FRM_COUNT; index++) {
        int fid = art_id(OBJ_TYPE_INTERFACE, lsgrphs[index], 0, 0, 0);
        lsbmp[index] = art_lock(fid,
            &(grphkey[index]),
            &(ginfo[index].width),
            &(ginfo[index].height));

        if (lsbmp[index] == NULL) {
            while (--index >= 0) {
                art_ptr_unlock(grphkey[index]);
            }
            mem_free(snapshot);
            message_exit(&lsgame_msgfl);
            text_font(fontsave);

            if (windowType != LOAD_SAVE_WINDOW_TYPE_LOAD_GAME_FROM_MAIN_MENU) {
                if (bk_enable) {
                    map_enable_bk_processes();
                }
            }

            cycle_enable();
            gmouse_set_cursor(MOUSE_CURSOR_ARROW);
            return -1;
        }
    }

    int lsWindowX = 0;
    int lsWindowY = 0;
    lsgwin = win_add(lsWindowX,
        lsWindowY,
        LS_WINDOW_WIDTH,
        LS_WINDOW_HEIGHT,
        256,
        WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
    if (lsgwin == -1) {
        // FIXME: Leaking frms.
        mem_free(snapshot);
        message_exit(&lsgame_msgfl);
        text_font(fontsave);

        if (windowType != LOAD_SAVE_WINDOW_TYPE_LOAD_GAME_FROM_MAIN_MENU) {
            if (bk_enable) {
                map_enable_bk_processes();
            }
        }

        cycle_enable();
        gmouse_set_cursor(MOUSE_CURSOR_ARROW);
        return -1;
    }

    lsgbuf = win_get_buf(lsgwin);
    memcpy(lsgbuf, lsbmp[LOAD_SAVE_FRM_BACKGROUND], LS_WINDOW_WIDTH * LS_WINDOW_HEIGHT);

    int messageId;
    switch (windowType) {
    case LOAD_SAVE_WINDOW_TYPE_SAVE_GAME:
        // SAVE GAME
        messageId = 102;
        break;
    case LOAD_SAVE_WINDOW_TYPE_PICK_QUICK_SAVE_SLOT:
        // PICK A QUICK SAVE SLOT
        messageId = 103;
        break;
    case LOAD_SAVE_WINDOW_TYPE_LOAD_GAME:
    case LOAD_SAVE_WINDOW_TYPE_LOAD_GAME_FROM_MAIN_MENU:
        // LOAD GAME
        messageId = 100;
        break;
    case LOAD_SAVE_WINDOW_TYPE_PICK_QUICK_LOAD_SLOT:
        // PICK A QUICK LOAD SLOT
        messageId = 101;
        break;
    default:
        assert(false && "Should be unreachable");
    }

    char* msg;

    msg = getmsg(&lsgame_msgfl, &lsgmesg, messageId);
    text_to_buf(lsgbuf + LS_WINDOW_WIDTH * 27 + 48, msg, LS_WINDOW_WIDTH, LS_WINDOW_WIDTH, colorTable[18979]);

    // DONE
    msg = getmsg(&lsgame_msgfl, &lsgmesg, 104);
    text_to_buf(lsgbuf + LS_WINDOW_WIDTH * 348 + 410, msg, LS_WINDOW_WIDTH, LS_WINDOW_WIDTH, colorTable[18979]);

    // CANCEL
    msg = getmsg(&lsgame_msgfl, &lsgmesg, 105);
    text_to_buf(lsgbuf + LS_WINDOW_WIDTH * 348 + 515, msg, LS_WINDOW_WIDTH, LS_WINDOW_WIDTH, colorTable[18979]);

    int btn;

    btn = win_register_button(lsgwin,
        391,
        349,
        ginfo[LOAD_SAVE_FRM_RED_BUTTON_PRESSED].width,
        ginfo[LOAD_SAVE_FRM_RED_BUTTON_PRESSED].height,
        -1,
        -1,
        -1,
        500,
        lsbmp[LOAD_SAVE_FRM_RED_BUTTON_NORMAL],
        lsbmp[LOAD_SAVE_FRM_RED_BUTTON_PRESSED],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        win_register_button_sound_func(btn, gsound_red_butt_press, gsound_red_butt_release);
    }

    btn = win_register_button(lsgwin,
        495,
        349,
        ginfo[LOAD_SAVE_FRM_RED_BUTTON_PRESSED].width,
        ginfo[LOAD_SAVE_FRM_RED_BUTTON_PRESSED].height,
        -1,
        -1,
        -1,
        501,
        lsbmp[LOAD_SAVE_FRM_RED_BUTTON_NORMAL],
        lsbmp[LOAD_SAVE_FRM_RED_BUTTON_PRESSED],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        win_register_button_sound_func(btn, gsound_red_butt_press, gsound_red_butt_release);
    }

    btn = win_register_button(lsgwin,
        35,
        58,
        ginfo[LOAD_SAVE_FRM_ARROW_UP_PRESSED].width,
        ginfo[LOAD_SAVE_FRM_ARROW_UP_PRESSED].height,
        -1,
        505,
        506,
        505,
        lsbmp[LOAD_SAVE_FRM_ARROW_UP_NORMAL],
        lsbmp[LOAD_SAVE_FRM_ARROW_UP_PRESSED],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        win_register_button_sound_func(btn, gsound_red_butt_press, gsound_red_butt_release);
    }

    btn = win_register_button(lsgwin,
        35,
        ginfo[LOAD_SAVE_FRM_ARROW_UP_PRESSED].height + 58,
        ginfo[LOAD_SAVE_FRM_ARROW_DOWN_PRESSED].width,
        ginfo[LOAD_SAVE_FRM_ARROW_DOWN_PRESSED].height,
        -1,
        503,
        504,
        503,
        lsbmp[LOAD_SAVE_FRM_ARROW_DOWN_NORMAL],
        lsbmp[LOAD_SAVE_FRM_ARROW_DOWN_PRESSED],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        win_register_button_sound_func(btn, gsound_red_butt_press, gsound_red_butt_release);
    }

    win_register_button(lsgwin, 55, 87, 230, 353, -1, -1, -1, 502, NULL, NULL, NULL, BUTTON_FLAG_TRANSPARENT);
    text_font(101);

    return 0;
}

// 0x47D824
static int LSGameEnd(int windowType)
{
    win_delete(lsgwin);
    text_font(fontsave);
    message_exit(&lsgame_msgfl);

    for (int index = 0; index < LOAD_SAVE_FRM_COUNT; index++) {
        art_ptr_unlock(grphkey[index]);
    }

    mem_free(snapshot);

    if (windowType != LOAD_SAVE_WINDOW_TYPE_LOAD_GAME_FROM_MAIN_MENU) {
        if (bk_enable) {
            map_enable_bk_processes();
        }
    }

    cycle_enable();
    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    return 0;
}

// 0x47D88C
static int SaveSlot()
{
    ls_error_code = 0;
    map_backup_count = -1;
    gmouse_set_cursor(MOUSE_CURSOR_WAIT_PLANET);

    gsound_background_pause();

    sprintf(gmpath, "%s\\%s", patches, "SAVEGAME");
    mkdir(gmpath);

    sprintf(gmpath, "%s\\%s\\%s%.2d", patches, "SAVEGAME", "SLOT", slot_cursor + 1);
    mkdir(gmpath);

    strcat(gmpath, "\\proto");
    mkdir(gmpath);

    char* protoBasePath = gmpath + strlen(gmpath);

    strcpy(protoBasePath, "\\critters");
    mkdir(gmpath);

    strcpy(protoBasePath, "\\items");
    mkdir(gmpath);

    if (SaveBackup() == -1) {
        debug_printf("\nLOADSAVE: Warning, can't backup save file!\n");
    }

    sprintf(gmpath, "%s\\%s%.2d\\", "SAVEGAME", "SLOT", slot_cursor + 1);
    strcat(gmpath, "SAVE.DAT");

    debug_printf("\nLOADSAVE: Save name: %s\n", gmpath);

    flptr = db_fopen(gmpath, "wb");
    if (flptr == NULL) {
        debug_printf("\nLOADSAVE: ** Error opening save game for writing! **\n");
        RestoreSave();
        sprintf(gmpath, "%s\\%s%.2d\\", "SAVEGAME", "SLOT", slot_cursor + 1);
        MapDirErase(gmpath, "BAK");
        partyMemberUnPrepSave();
        gsound_background_unpause();
        return -1;
    }

    long pos = db_ftell(flptr);
    if (SaveHeader(slot_cursor) == -1) {
        debug_printf("\nLOADSAVE: ** Error writing save game header! **\n");
        debug_printf("LOADSAVE: Save file header size written: %d bytes.\n", db_ftell(flptr) - pos);
        db_fclose(flptr);
        RestoreSave();
        sprintf(gmpath, "%s\\%s%.2d\\", "SAVEGAME", "SLOT", slot_cursor + 1);
        MapDirErase(gmpath, "BAK");
        partyMemberUnPrepSave();
        gsound_background_unpause();
        return -1;
    }

    for (int index = 0; index < LOAD_SAVE_HANDLER_COUNT; index++) {
        long pos = db_ftell(flptr);
        SaveGameHandler* handler = master_save_list[index];
        if (handler(flptr) == -1) {
            debug_printf("\nLOADSAVE: ** Error writing save function #%d data! **\n", index);
            db_fclose(flptr);
            RestoreSave();
            sprintf(gmpath, "%s\\%s%.2d\\", "SAVEGAME", "SLOT", slot_cursor + 1);
            MapDirErase(gmpath, "BAK");
            partyMemberUnPrepSave();
            gsound_background_unpause();
            return -1;
        }

        debug_printf("LOADSAVE: Save function #%d data size written: %d bytes.\n", index, db_ftell(flptr) - pos);
    }

    debug_printf("LOADSAVE: Total save data written: %ld bytes.\n", db_ftell(flptr));

    db_fclose(flptr);

    sprintf(gmpath, "%s\\%s%.2d\\", "SAVEGAME", "SLOT", slot_cursor + 1);
    MapDirErase(gmpath, "BAK");

    lsgmesg.num = 140;
    if (message_search(&lsgame_msgfl, &lsgmesg)) {
        display_print(lsgmesg.text);
    } else {
        debug_printf("\nError: Couldn't find LoadSave Message!");
    }

    gsound_background_unpause();

    return 0;
}

// 0x47DC60
int isLoadingGame()
{
    return loadingGame;
}

// 0x47DC68
static int LoadSlot(int slot)
{
    loadingGame = 1;

    if (isInCombat()) {
        intface_end_window_close(false);
        combat_over_from_load();
        gmouse_set_cursor(MOUSE_CURSOR_WAIT_PLANET);
    }

    sprintf(gmpath, "%s\\%s%.2d\\", "SAVEGAME", "SLOT", slot_cursor + 1);
    strcat(gmpath, "SAVE.DAT");

    LoadSaveSlotData* ptr = &(LSData[slot]);
    debug_printf("\nLOADSAVE: Load name: %s\n", ptr->description);

    flptr = db_fopen(gmpath, "rb");
    if (flptr == NULL) {
        debug_printf("\nLOADSAVE: ** Error opening load game file for reading! **\n");
        loadingGame = 0;
        return -1;
    }

    long pos = db_ftell(flptr);
    if (LoadHeader(slot) == -1) {
        debug_printf("\nLOADSAVE: ** Error reading save  game header! **\n");
        db_fclose(flptr);
        game_reset();
        loadingGame = 0;
        return -1;
    }

    debug_printf("LOADSAVE: Load file header size read: %d bytes.\n", db_ftell(flptr) - pos);

    for (int index = 0; index < LOAD_SAVE_HANDLER_COUNT; index += 1) {
        long pos = db_ftell(flptr);
        LoadGameHandler* handler = master_load_list[index];
        if (handler(flptr) == -1) {
            debug_printf("\nLOADSAVE: ** Error reading load function #%d data! **\n", index);
            int v12 = db_ftell(flptr);
            debug_printf("LOADSAVE: Load function #%d data size read: %d bytes.\n", index, db_ftell(flptr) - pos);
            db_fclose(flptr);
            game_reset();
            loadingGame = 0;
            return -1;
        }

        debug_printf("LOADSAVE: Load function #%d data size read: %d bytes.\n", index, db_ftell(flptr) - pos);
    }

    debug_printf("LOADSAVE: Total load data read: %ld bytes.\n", db_ftell(flptr));
    db_fclose(flptr);

    sprintf(str, "%s\\", "MAPS");
    MapDirErase(str, "BAK");
    proto_dude_update_gender();

    // Game Loaded.
    lsgmesg.num = 141;
    if (message_search(&lsgame_msgfl, &lsgmesg) == 1) {
        display_print(lsgmesg.text);
    } else {
        debug_printf("\nError: Couldn't find LoadSave Message!");
    }

    loadingGame = 0;

    return 0;
}

// 0x47DF10
static int SaveHeader(int slot)
{
    ls_error_code = 4;

    LoadSaveSlotData* ptr = &(LSData[slot]);
    strncpy(ptr->signature, LOAD_SAVE_SIGNATURE, 24);

    if (db_fwrite(ptr->signature, 1, 24, flptr) == -1) {
        return -1;
    }

    short temp[3];
    temp[0] = VERSION_MAJOR;
    temp[1] = VERSION_MINOR;

    ptr->versionMinor = temp[0];
    ptr->versionMajor = temp[1];

    if (db_fwriteShortCount(flptr, temp, 2) == -1) {
        return -1;
    }

    ptr->versionRelease = VERSION_RELEASE;
    if (db_fwriteByte(flptr, VERSION_RELEASE) == -1) {
        return -1;
    }

    char* characterName = critter_name(obj_dude);
    strncpy(ptr->characterName, characterName, 32);

    if (db_fwrite(ptr->characterName, 32, 1, flptr) != 1) {
        return -1;
    }

    if (db_fwrite(ptr->description, 30, 1, flptr) != 1) {
        return -1;
    }

    time_t now = time(NULL);
    struct tm* local = localtime(&now);

    temp[0] = local->tm_mday;
    temp[1] = local->tm_mon + 1;
    temp[2] = local->tm_year + 1900;

    ptr->fileDay = temp[0];
    ptr->fileMonth = temp[1];
    ptr->fileYear = temp[2];
    ptr->fileTime = local->tm_hour + local->tm_min;

    if (db_fwriteShortCount(flptr, temp, 3) == -1) {
        return -1;
    }

    if (db_fwriteLong(flptr, ptr->fileTime) == -1) {
        return -1;
    }

    int month;
    int day;
    int year;
    game_time_date(&month, &day, &year);

    temp[0] = month;
    temp[1] = day;
    temp[2] = year;
    ptr->gameTime = game_time();

    if (db_fwriteShortCount(flptr, temp, 3) == -1) {
        return -1;
    }

    if (db_fwriteLong(flptr, ptr->gameTime) == -1) {
        return -1;
    }

    ptr->elevation = map_elevation;
    if (db_fwriteShort(flptr, ptr->elevation) == -1) {
        return -1;
    }

    ptr->map = map_get_index_number();
    if (db_fwriteShort(flptr, ptr->map) == -1) {
        return -1;
    }

    char mapName[128];
    strcpy(mapName, map_data.name);

    char* v1 = strmfe(str, mapName, "sav");
    strncpy(ptr->fileName, v1, 16);
    if (db_fwrite(ptr->fileName, 16, 1, flptr) != 1) {
        return -1;
    }

    if (db_fwrite(thumbnail_image[1], LS_PREVIEW_SIZE, 1, flptr) != 1) {
        return -1;
    }

    memset(mapName, 0, 128);
    if (db_fwrite(mapName, 1, 128, flptr) != 128) {
        return -1;
    }

    ls_error_code = 0;

    return 0;
}

// 0x47E2E4
static int LoadHeader(int slot)
{
    ls_error_code = 3;

    LoadSaveSlotData* ptr = &(LSData[slot]);

    if (db_fread(ptr->signature, 1, 24, flptr) != 24) {
        return -1;
    }

    if (strncmp(ptr->signature, LOAD_SAVE_SIGNATURE, 18) != 0) {
        debug_printf("\nLOADSAVE: ** Invalid save file on load! **\n");
        ls_error_code = 2;
        return -1;
    }

    short v8[3];
    if (db_freadShortCount(flptr, v8, 2) == -1) {
        return -1;
    }

    ptr->versionMinor = v8[0];
    ptr->versionMajor = v8[1];

    if (db_freadByte(flptr, &(ptr->versionRelease)) == -1) {
        return -1;
    }

    if (ptr->versionMinor != 1 || ptr->versionMajor != 2 || ptr->versionRelease != 'R') {
        debug_printf("\nLOADSAVE: Load slot #%d Version: %d.%d%c\n", slot, ptr->versionMinor, ptr->versionMajor, ptr->versionRelease);
        ls_error_code = 1;
        return -1;
    }

    if (db_fread(ptr->characterName, 32, 1, flptr) != 1) {
        return -1;
    }

    if (db_fread(ptr->description, 30, 1, flptr) != 1) {
        return -1;
    }

    if (db_freadShortCount(flptr, v8, 3) == -1) {
        return -1;
    }

    ptr->fileMonth = v8[0];
    ptr->fileDay = v8[1];
    ptr->fileYear = v8[2];

    if (db_freadLong(flptr, &(ptr->fileTime)) == -1) {
        return -1;
    }

    if (db_freadShortCount(flptr, v8, 3) == -1) {
        return -1;
    }

    ptr->gameMonth = v8[0];
    ptr->gameDay = v8[1];
    ptr->gameYear = v8[2];

    if (db_freadLong(flptr, &(ptr->gameTime)) == -1) {
        return -1;
    }

    if (db_freadShort(flptr, &(ptr->elevation)) == -1) {
        return -1;
    }

    if (db_freadShort(flptr, &(ptr->map)) == -1) {
        return -1;
    }

    if (db_fread(ptr->fileName, 1, 16, flptr) != 16) {
        return -1;
    }

    if (db_fseek(flptr, LS_PREVIEW_SIZE, SEEK_CUR) != 0) {
        return -1;
    }

    if (db_fseek(flptr, 128, 1) != 0) {
        return -1;
    }

    ls_error_code = 0;

    return 0;
}

// 0x47E5D0
static int GetSlotList()
{
    int index = 0;
    for (; index < 10; index += 1) {
        sprintf(str, "%s\\%s%.2d\\%s", "SAVEGAME", "SLOT", index + 1, "SAVE.DAT");

        int fileSize;
        if (db_dir_entry(str, &fileSize) != 0) {
            LSstatus[index] = SLOT_STATE_EMPTY;
        } else {
            flptr = db_fopen(str, "rb");

            if (flptr == NULL) {
                debug_printf("\nLOADSAVE: ** Error opening save  game for reading! **\n");
                return -1;
            }

            if (LoadHeader(index) == -1) {
                if (ls_error_code == 1) {
                    debug_printf("LOADSAVE: ** save file #%d is an older version! **\n", slot_cursor);
                    LSstatus[index] = SLOT_STATE_UNSUPPORTED_VERSION;
                } else {
                    debug_printf("LOADSAVE: ** Save file #%d corrupt! **", index);
                    LSstatus[index] = SLOT_STATE_ERROR;
                }
            } else {
                LSstatus[index] = SLOT_STATE_OCCUPIED;
            }

            db_fclose(flptr);
        }
    }
    return index;
}

// 0x47E6D8
static void ShowSlotList(int a1)
{
    buf_fill(lsgbuf + LS_WINDOW_WIDTH * 87 + 55, 230, 353, LS_WINDOW_WIDTH, lsgbuf[LS_WINDOW_WIDTH * 86 + 55] & 0xFF);

    int y = 87;
    for (int index = 0; index < 10; index += 1) {

        int color = index == slot_cursor ? colorTable[32747] : colorTable[992];
        const char* text = getmsg(&lsgame_msgfl, &lsgmesg, a1 != 0 ? 110 : 109);
        sprintf(str, "[   %s %.2d:   ]", text, index + 1);
        text_to_buf(lsgbuf + LS_WINDOW_WIDTH * y + 55, str, LS_WINDOW_WIDTH, LS_WINDOW_WIDTH, color);

        y += text_height();
        switch (LSstatus[index]) {
        case SLOT_STATE_OCCUPIED:
            strcpy(str, LSData[index].description);
            break;
        case SLOT_STATE_EMPTY:
            // - EMPTY -
            text = getmsg(&lsgame_msgfl, &lsgmesg, 111);
            sprintf(str, "       %s", text);
            break;
        case SLOT_STATE_ERROR:
            // - CORRUPT SAVE FILE -
            text = getmsg(&lsgame_msgfl, &lsgmesg, 112);
            sprintf(str, "%s", text);
            color = colorTable[32328];
            break;
        case SLOT_STATE_UNSUPPORTED_VERSION:
            // - OLD VERSION -
            text = getmsg(&lsgame_msgfl, &lsgmesg, 113);
            sprintf(str, " %s", text);
            color = colorTable[32328];
            break;
        }

        text_to_buf(lsgbuf + LS_WINDOW_WIDTH * y + 55, str, LS_WINDOW_WIDTH, LS_WINDOW_WIDTH, color);
        y += 2 * text_height() + 4;
    }
}

// 0x47E8E0
static void DrawInfoBox(int a1)
{
    buf_to_buf(lsbmp[LOAD_SAVE_FRM_BACKGROUND] + LS_WINDOW_WIDTH * 254 + 396, 164, 60, LS_WINDOW_WIDTH, lsgbuf + LS_WINDOW_WIDTH * 254 + 396, 640);

    unsigned char* dest;
    const char* text;
    int color = colorTable[992];

    switch (LSstatus[a1]) {
    case SLOT_STATE_OCCUPIED:
        do {
            LoadSaveSlotData* ptr = &(LSData[a1]);
            text_to_buf(lsgbuf + LS_WINDOW_WIDTH * 254 + 396, ptr->characterName, LS_WINDOW_WIDTH, LS_WINDOW_WIDTH, color);

            int v4 = ptr->gameTime / 600;
            int minutes = v4 % 60;
            int v6 = 25 * (v4 / 60 % 24);
            int time = 4 * v6 + minutes;

            text = getmsg(&lsgame_msgfl, &lsgmesg, 116 + ptr->gameMonth);
            sprintf(str, "%.2d %s %.4d   %.4d", ptr->gameDay, text, ptr->gameYear, time);

            int v2 = text_height();
            text_to_buf(lsgbuf + LS_WINDOW_WIDTH * (256 + v2) + 397, str, LS_WINDOW_WIDTH, LS_WINDOW_WIDTH, color);

            const char* v22 = map_get_elev_idx(ptr->map, ptr->elevation);
            const char* v9 = map_get_short_name(ptr->map);
            sprintf(str, "%s %s", v9, v22);

            int y = v2 + 3 + v2 + 256;
            short beginnings[WORD_WRAP_MAX_COUNT];
            short count;
            if (word_wrap(str, 164, beginnings, &count) == 0) {
                for (int index = 0; index < count - 1; index += 1) {
                    char* beginning = str + beginnings[index];
                    char* ending = str + beginnings[index + 1];
                    char c = *ending;
                    *ending = '\0';
                    text_to_buf(lsgbuf + LS_WINDOW_WIDTH * y + 399, beginning, 164, LS_WINDOW_WIDTH, color);
                    y += v2 + 2;
                }
            }
        } while (0);
        return;
    case SLOT_STATE_EMPTY:
        // Empty.
        text = getmsg(&lsgame_msgfl, &lsgmesg, 114);
        dest = lsgbuf + LS_WINDOW_WIDTH * 262 + 404;
        break;
    case SLOT_STATE_ERROR:
        // Error!
        text = getmsg(&lsgame_msgfl, &lsgmesg, 115);
        dest = lsgbuf + LS_WINDOW_WIDTH * 262 + 404;
        color = colorTable[32328];
        break;
    case SLOT_STATE_UNSUPPORTED_VERSION:
        // Old version.
        text = getmsg(&lsgame_msgfl, &lsgmesg, 116);
        dest = lsgbuf + LS_WINDOW_WIDTH * 262 + 400;
        color = colorTable[32328];
        break;
    default:
        assert(false && "Should be unreachable");
    }

    text_to_buf(dest, text, LS_WINDOW_WIDTH, LS_WINDOW_WIDTH, color);
}

// 0x47EC48
static int LoadTumbSlot(int a1)
{
    File* stream;
    int v2;

    v2 = LSstatus[slot_cursor];
    if (v2 != 0 && v2 != 2 && v2 != 3) {
        sprintf(str, "%s\\%s%.2d\\%s", "SAVEGAME", "SLOT", slot_cursor + 1, "SAVE.DAT");
        debug_printf(" Filename %s\n", str);

        stream = db_fopen(str, "rb");
        if (stream == NULL) {
            debug_printf("\nLOADSAVE: ** (A) Error reading thumbnail #%d! **\n", a1);
            return -1;
        }

        if (db_fseek(stream, 131, SEEK_SET) != 0) {
            debug_printf("\nLOADSAVE: ** (B) Error reading thumbnail #%d! **\n", a1);
            db_fclose(stream);
            return -1;
        }

        if (db_fread(thumbnail_image[0], LS_PREVIEW_SIZE, 1, stream) != 1) {
            debug_printf("\nLOADSAVE: ** (C) Error reading thumbnail #%d! **\n", a1);
            db_fclose(stream);
            return -1;
        }

        db_fclose(stream);
    }

    return 0;
}

// 0x47ED5C
static int GetComment(int a1)
{
    int commentWindowX = LS_COMMENT_WINDOW_X;
    int commentWindowY = LS_COMMENT_WINDOW_Y;
    int window = win_add(commentWindowX,
        commentWindowY,
        ginfo[LOAD_SAVE_FRM_BOX].width,
        ginfo[LOAD_SAVE_FRM_BOX].height,
        256,
        WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
    if (window == -1) {
        return -1;
    }

    unsigned char* windowBuffer = win_get_buf(window);
    memcpy(windowBuffer,
        lsbmp[LOAD_SAVE_FRM_BOX],
        ginfo[LOAD_SAVE_FRM_BOX].height * ginfo[LOAD_SAVE_FRM_BOX].width);

    text_font(103);

    const char* msg;

    // DONE
    msg = getmsg(&lsgame_msgfl, &lsgmesg, 104);
    text_to_buf(windowBuffer + ginfo[LOAD_SAVE_FRM_BOX].width * 57 + 56,
        msg,
        ginfo[LOAD_SAVE_FRM_BOX].width,
        ginfo[LOAD_SAVE_FRM_BOX].width,
        colorTable[18979]);

    // CANCEL
    msg = getmsg(&lsgame_msgfl, &lsgmesg, 105);
    text_to_buf(windowBuffer + ginfo[LOAD_SAVE_FRM_BOX].width * 57 + 181,
        msg,
        ginfo[LOAD_SAVE_FRM_BOX].width,
        ginfo[LOAD_SAVE_FRM_BOX].width,
        colorTable[18979]);

    // DESCRIPTION
    msg = getmsg(&lsgame_msgfl, &lsgmesg, 130);

    char title[260];
    strcpy(title, msg);

    int width = text_width(title);
    text_to_buf(windowBuffer + ginfo[LOAD_SAVE_FRM_BOX].width * 7 + (ginfo[LOAD_SAVE_FRM_BOX].width - width) / 2,
        title,
        ginfo[LOAD_SAVE_FRM_BOX].width,
        ginfo[LOAD_SAVE_FRM_BOX].width,
        colorTable[18979]);

    text_font(101);

    int btn;

    // DONE
    btn = win_register_button(window,
        34,
        58,
        ginfo[LOAD_SAVE_FRM_RED_BUTTON_PRESSED].width,
        ginfo[LOAD_SAVE_FRM_RED_BUTTON_PRESSED].height,
        -1,
        -1,
        -1,
        507,
        lsbmp[LOAD_SAVE_FRM_RED_BUTTON_NORMAL],
        lsbmp[LOAD_SAVE_FRM_RED_BUTTON_PRESSED],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn == -1) {
        win_register_button_sound_func(btn, gsound_red_butt_press, gsound_red_butt_release);
    }

    // CANCEL
    btn = win_register_button(window,
        160,
        58,
        ginfo[LOAD_SAVE_FRM_RED_BUTTON_PRESSED].width,
        ginfo[LOAD_SAVE_FRM_RED_BUTTON_PRESSED].height,
        -1,
        -1,
        -1,
        508,
        lsbmp[LOAD_SAVE_FRM_RED_BUTTON_NORMAL],
        lsbmp[LOAD_SAVE_FRM_RED_BUTTON_PRESSED],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn == -1) {
        win_register_button_sound_func(btn, gsound_red_butt_press, gsound_red_butt_release);
    }

    win_draw(window);

    char description[LOAD_SAVE_DESCRIPTION_LENGTH];
    if (LSstatus[slot_cursor] == SLOT_STATE_OCCUPIED) {
        strncpy(description, LSData[a1].description, LOAD_SAVE_DESCRIPTION_LENGTH);
    } else {
        memset(description, '\0', LOAD_SAVE_DESCRIPTION_LENGTH);
    }

    int rc;

    if (get_input_str2(window, 507, 508, description, LOAD_SAVE_DESCRIPTION_LENGTH - 1, 24, 35, colorTable[992], lsbmp[LOAD_SAVE_FRM_BOX][ginfo[1].width * 35 + 24], 0) == 0) {
        strncpy(LSData[a1].description, description, LOAD_SAVE_DESCRIPTION_LENGTH);
        LSData[a1].description[LOAD_SAVE_DESCRIPTION_LENGTH - 1] = '\0';
        rc = 1;
    } else {
        rc = 0;
    }

    win_delete(window);

    return rc;
}

// 0x47F084
static int get_input_str2(int win, int doneKeyCode, int cancelKeyCode, char* description, int maxLength, int x, int y, int textColor, int backgroundColor, int flags)
{
    int cursorWidth = text_width("_") - 4;
    int windowWidth = win_width(win);
    int lineHeight = text_height();
    unsigned char* windowBuffer = win_get_buf(win);
    if (maxLength > 255) {
        maxLength = 255;
    }

    char text[256];
    strcpy(text, description);

    int textLength = strlen(text);
    text[textLength] = ' ';
    text[textLength + 1] = '\0';

    int nameWidth = text_width(text);

    buf_fill(windowBuffer + windowWidth * y + x, nameWidth, lineHeight, windowWidth, backgroundColor);
    text_to_buf(windowBuffer + windowWidth * y + x, text, windowWidth, windowWidth, textColor);

    win_draw(win);

    int blinkCounter = 3;
    bool blink = false;

    int v1 = 0;

    int rc = 1;
    while (rc == 1) {
        int tick = get_time();

        int keyCode = get_input();
        if ((keyCode & 0x80000000) == 0) {
            v1++;
        }

        if (keyCode == doneKeyCode || keyCode == KEY_RETURN) {
            rc = 0;
        } else if (keyCode == cancelKeyCode || keyCode == KEY_ESCAPE) {
            rc = -1;
        } else {
            if ((keyCode == KEY_DELETE || keyCode == KEY_BACKSPACE) && textLength > 0) {
                buf_fill(windowBuffer + windowWidth * y + x, text_width(text), lineHeight, windowWidth, backgroundColor);

                // TODO: Probably incorrect, needs testing.
                if (v1 == 1) {
                    textLength = 1;
                }

                text[textLength - 1] = ' ';
                text[textLength] = '\0';
                text_to_buf(windowBuffer + windowWidth * y + x, text, windowWidth, windowWidth, textColor);
                textLength--;
            } else if ((keyCode >= KEY_FIRST_INPUT_CHARACTER && keyCode <= KEY_LAST_INPUT_CHARACTER) && textLength < maxLength) {
                if ((flags & 0x01) != 0) {
                    if (!isdoschar(keyCode)) {
                        break;
                    }
                }

                buf_fill(windowBuffer + windowWidth * y + x, text_width(text), lineHeight, windowWidth, backgroundColor);

                text[textLength] = keyCode & 0xFF;
                text[textLength + 1] = ' ';
                text[textLength + 2] = '\0';
                text_to_buf(windowBuffer + windowWidth * y + x, text, windowWidth, windowWidth, textColor);
                textLength++;

                win_draw(win);
            }
        }

        blinkCounter -= 1;
        if (blinkCounter == 0) {
            blinkCounter = 3;
            blink = !blink;

            int color = blink ? backgroundColor : textColor;
            buf_fill(windowBuffer + windowWidth * y + x + text_width(text) - cursorWidth, cursorWidth, lineHeight - 2, windowWidth, color);
            win_draw(win);
        }

        while (elapsed_time(tick) < 1000 / 24) {
        }
    }

    if (rc == 0) {
        text[textLength] = '\0';
        strcpy(description, text);
    }

    return rc;
}

// 0x47F48C
static int DummyFunc(File* stream)
{
    return 0;
}

// 0x47F490
static int PrepLoad(File* stream)
{
    game_reset();
    gmouse_set_cursor(MOUSE_CURSOR_WAIT_PLANET);
    map_data.name[0] = '\0';
    gameTimeSetTime(LSData[slot_cursor].gameTime);
    return 0;
}

// 0x47F4C8
static int EndLoad(File* stream)
{
    wmMapMusicStart();
    critter_pc_set_name(LSData[slot_cursor].characterName);
    intface_redraw();
    refresh_box_bar_win();
    tile_refresh_display();
    if (isInCombat()) {
        scripts_request_combat(NULL);
    }
    return 0;
}

// 0x47F510
static int GameMap2Slot(File* stream)
{
    if (partyMemberPrepSave() == -1) {
        return -1;
    }

    if (map_save_in_game(false) == -1) {
        return -1;
    }

    for (int index = 1; index < partyMemberMaxCount; index += 1) {
        int pid = partyMemberPidList[index];
        if (pid == -2) {
            continue;
        }

        char path[MAX_PATH];
        if (proto_list_str(pid, path) != 0) {
            continue;
        }

        const char* critterItemPath = PID_TYPE(pid) == OBJ_TYPE_CRITTER ? "PROTO\\CRITTERS" : "PROTO\\ITEMS";
        sprintf(str0, "%s\\%s\\%s", patches, critterItemPath, path);
        sprintf(str1, "%s\\%s\\%s%.2d\\%s\\%s", patches, "SAVEGAME", "SLOT", slot_cursor + 1, critterItemPath, path);
        if (gzcompress_file(str0, str1) == -1) {
            return -1;
        }
    }

    sprintf(str0, "%s\\*.%s", "MAPS", "SAV");

    char** fileNameList;
    int fileNameListLength = db_get_file_list(str0, &fileNameList, 0, 0);
    if (fileNameListLength == -1) {
        return -1;
    }

    if (db_fwriteInt(stream, fileNameListLength) == -1) {
        db_free_file_list(&fileNameList, 0);
        return -1;
    }

    if (fileNameListLength == 0) {
        db_free_file_list(&fileNameList, 0);
        return -1;
    }

    sprintf(gmpath, "%s\\%s%.2d\\", "SAVEGAME", "SLOT", slot_cursor + 1);

    if (MapDirErase(gmpath, "SAV") == -1) {
        db_free_file_list(&fileNameList, 0);
        return -1;
    }

    sprintf(gmpath, "%s\\%s\\%s%.2d\\", patches, "SAVEGAME", "SLOT", slot_cursor + 1);
    strmfe(str0, "AUTOMAP.DB", "SAV");
    strcat(gmpath, str0);
    remove(gmpath);

    for (int index = 0; index < fileNameListLength; index += 1) {
        char* string = fileNameList[index];
        if (db_fwrite(string, strlen(string) + 1, 1, stream) == -1) {
            db_free_file_list(&fileNameList, 0);
            return -1;
        }

        sprintf(str0, "%s\\%s\\%s", patches, "MAPS", string);
        sprintf(str1, "%s\\%s\\%s%.2d\\%s", patches, "SAVEGAME", "SLOT", slot_cursor + 1, string);
        if (gzcompress_file(str0, str1) == -1) {
            db_free_file_list(&fileNameList, 0);
            return -1;
        }
    }

    db_free_file_list(&fileNameList, 0);

    strmfe(str0, "AUTOMAP.DB", "SAV");
    sprintf(str1, "%s\\%s\\%s%.2d\\%s", patches, "SAVEGAME", "SLOT", slot_cursor + 1, str0);
    sprintf(str0, "%s\\%s\\%s", patches, "MAPS", "AUTOMAP.DB");

    if (gzcompress_file(str0, str1) == -1) {
        return -1;
    }

    sprintf(str0, "%s\\%s", "MAPS", "AUTOMAP.DB");
    File* inStream = db_fopen(str0, "rb");
    if (inStream == NULL) {
        return -1;
    }

    int fileSize = db_filelength(inStream);
    if (fileSize == -1) {
        db_fclose(inStream);
        return -1;
    }

    db_fclose(inStream);

    if (db_fwriteInt(stream, fileSize) == -1) {
        return -1;
    }

    if (partyMemberUnPrepSave() == -1) {
        return -1;
    }

    return 0;
}

// SlotMap2Game
// 0x47F990
static int SlotMap2Game(File* stream)
{
    debug_printf("LOADSAVE: in SlotMap2Game\n");

    int fileNameListLength;
    if (db_freadInt(stream, &fileNameListLength) == -1) {
        debug_printf("LOADSAVE: returning 1\n");
        return -1;
    }

    if (fileNameListLength == 0) {
        debug_printf("LOADSAVE: returning 2\n");
        return -1;
    }

    sprintf(str0, "%s\\", "PROTO\\CRITTERS");

    if (MapDirErase(str0, "PRO") == -1) {
        debug_printf("LOADSAVE: returning 3\n");
        return -1;
    }

    sprintf(str0, "%s\\", "PROTO\\ITEMS");
    if (MapDirErase(str0, "PRO") == -1) {
        debug_printf("LOADSAVE: returning 4\n");
        return -1;
    }

    sprintf(str0, "%s\\", "MAPS");
    if (MapDirErase(str0, "SAV") == -1) {
        debug_printf("LOADSAVE: returning 5\n");
        return -1;
    }

    sprintf(str0, "%s\\%s\\%s", patches, "MAPS", "AUTOMAP.DB");
    remove(str0);

    for (int index = 1; index < partyMemberMaxCount; index += 1) {
        int pid = partyMemberPidList[index];
        if (pid != -2) {
            char protoPath[MAX_PATH];
            if (proto_list_str(pid, protoPath) == 0) {
                const char* basePath = PID_TYPE(pid) == OBJ_TYPE_CRITTER
                    ? "PROTO\\CRITTERS"
                    : "PROTO\\ITEMS";
                sprintf(str0, "%s\\%s\\%s", patches, basePath, protoPath);
                sprintf(str1, "%s\\%s\\%s%.2d\\%s\\%s", patches, "SAVEGAME", "SLOT", slot_cursor + 1, basePath, protoPath);

                if (gzdecompress_file(str1, str0) == -1) {
                    debug_printf("LOADSAVE: returning 6\n");
                    return -1;
                }
            }
        }
    }

    for (int index = 0; index < fileNameListLength; index += 1) {
        char fileName[MAX_PATH];
        if (mygets(fileName, stream) == -1) {
            break;
        }

        sprintf(str0, "%s\\%s\\%s%.2d\\%s", patches, "SAVEGAME", "SLOT", slot_cursor + 1, fileName);
        sprintf(str1, "%s\\%s\\%s", patches, "MAPS", fileName);

        if (gzdecompress_file(str0, str1) == -1) {
            debug_printf("LOADSAVE: returning 7\n");
            return -1;
        }
    }

    const char* automapFileName = strmfe(str1, "AUTOMAP.DB", "SAV");
    sprintf(str0, "%s\\%s\\%s%.2d\\%s", patches, "SAVEGAME", "SLOT", slot_cursor + 1, automapFileName);
    sprintf(str1, "%s\\%s\\%s", patches, "MAPS", "AUTOMAP.DB");
    if (gzRealUncompressCopyReal_file(str0, str1) == -1) {
        debug_printf("LOADSAVE: returning 8\n");
        return -1;
    }

    sprintf(str1, "%s\\%s", "MAPS", "AUTOMAP.DB");

    int v12;
    if (db_freadInt(stream, &v12) == -1) {
        debug_printf("LOADSAVE: returning 9\n");
        return -1;
    }

    if (map_load_in_game(LSData[slot_cursor].fileName) == -1) {
        debug_printf("LOADSAVE: returning 13\n");
        return -1;
    }

    return 0;
}

// 0x47FE14
static int mygets(char* dest, File* stream)
{
    int index = 14;
    while (true) {
        int c = db_fgetc(stream);
        if (c == -1) {
            return -1;
        }

        index -= 1;

        *dest = c & 0xFF;
        dest += 1;

        if (index == -1 || c == '\0') {
            break;
        }
    }

    if (index == 0) {
        return -1;
    }

    return 0;
}

// 0x47FE58
static int copy_file(const char* a1, const char* a2)
{
    File* stream1;
    File* stream2;
    int length;
    int chunk_length;
    void* buf;
    int result;

    stream1 = NULL;
    stream2 = NULL;
    buf = NULL;
    result = -1;

    stream1 = db_fopen(a1, "rb");
    if (stream1 == NULL) {
        goto out;
    }

    length = db_filelength(stream1);
    if (length == -1) {
        goto out;
    }

    stream2 = db_fopen(a2, "wb");
    if (stream2 == NULL) {
        goto out;
    }

    buf = mem_malloc(0xFFFF);
    if (buf == NULL) {
        goto out;
    }

    while (length != 0) {
        chunk_length = min(length, 0xFFFF);

        if (db_fread(buf, chunk_length, 1, stream1) != 1) {
            break;
        }

        if (db_fwrite(buf, chunk_length, 1, stream2) != 1) {
            break;
        }

        length -= chunk_length;
    }

    if (length != 0) {
        goto out;
    }

    result = 0;

out:

    if (stream1 != NULL) {
        db_fclose(stream1);
    }

    if (stream2 != NULL) {
        db_fclose(stream1);
    }

    if (buf != NULL) {
        mem_free(buf);
    }

    return result;
}

// InitLoadSave
// 0x48000C
void KillOldMaps()
{
    char path[MAX_PATH];
    sprintf(path, "%s\\", "MAPS");
    MapDirErase(path, "SAV");
}

// 0x480040
int MapDirErase(const char* relativePath, const char* extension)
{
    char path[MAX_PATH];
    sprintf(path, "%s*.%s", relativePath, extension);

    char** fileList;
    int fileListLength = db_get_file_list(path, &fileList, 0, 0);
    while (--fileListLength >= 0) {
        sprintf(path, "%s\\%s%s", patches, relativePath, fileList[fileListLength]);
        remove(path);
    }
    db_free_file_list(&fileList, 0);

    return 0;
}

// 0x4800C8
int MapDirEraseFile(const char* a1, const char* a2)
{
    char path[MAX_PATH];

    sprintf(path, "%s\\%s%s", patches, a1, a2);
    if (remove(path) != 0) {
        return -1;
    }

    return 0;
}

// 0x480104
static int SaveBackup()
{
    debug_printf("\nLOADSAVE: Backing up save slot files..\n");

    sprintf(gmpath, "%s\\%s\\%s%.2d\\", patches, "SAVEGAME", "SLOT", slot_cursor + 1);
    strcpy(str0, gmpath);

    strcat(str0, "SAVE.DAT");

    strmfe(str1, str0, "BAK");

    File* stream1 = db_fopen(str0, "rb");
    if (stream1 != NULL) {
        db_fclose(stream1);
        if (rename(str0, str1) != 0) {
            return -1;
        }
    }

    sprintf(gmpath, "%s\\%s%.2d\\", "SAVEGAME", "SLOT", slot_cursor + 1);
    sprintf(str0, "%s*.%s", gmpath, "SAV");

    char** fileList;
    int fileListLength = db_get_file_list(str0, &fileList, 0, 0);
    if (fileListLength == -1) {
        return -1;
    }

    map_backup_count = fileListLength;

    sprintf(gmpath, "%s\\%s\\%s%.2d\\", patches, "SAVEGAME", "SLOT", slot_cursor + 1);
    for (int index = fileListLength - 1; index >= 0; index--) {
        strcpy(str0, gmpath);
        strcat(str0, fileList[index]);

        strmfe(str1, str0, "BAK");
        if (rename(str0, str1) != 0) {
            db_free_file_list(&fileList, 0);
            return -1;
        }
    }

    db_free_file_list(&fileList, 0);

    debug_printf("\nLOADSAVE: %d map files backed up.\n", fileListLength);

    sprintf(gmpath, "%s\\%s%.2d\\", "SAVEGAME", "SLOT", slot_cursor + 1);

    char* v1 = strmfe(str2, "AUTOMAP.DB", "SAV");
    sprintf(str0, "%s\\%s", gmpath, v1);

    char* v2 = strmfe(str2, "AUTOMAP.DB", "BAK");
    sprintf(str1, "%s\\%s", gmpath, v2);

    automap_db_flag = 0;

    File* stream2 = db_fopen(str0, "rb");
    if (stream2 != NULL) {
        db_fclose(stream2);

        if (copy_file(str0, str1) == -1) {
            return -1;
        }

        automap_db_flag = 1;
    }

    return 0;
}

// 0x4803D8
static int RestoreSave()
{
    debug_printf("\nLOADSAVE: Restoring save file backup...\n");

    EraseSave();

    sprintf(gmpath, "%s\\%s\\%s%.2d\\", patches, "SAVEGAME", "SLOT", slot_cursor + 1);
    strcpy(str0, gmpath);
    strcat(str0, "SAVE.DAT");
    strmfe(str1, str0, "BAK");
    remove(str0);

    if (rename(str1, str0) != 0) {
        EraseSave();
        return -1;
    }

    sprintf(gmpath, "%s\\%s%.2d\\", "SAVEGAME", "SLOT", slot_cursor + 1);
    sprintf(str0, "%s*.%s", gmpath, "BAK");

    char** fileList;
    int fileListLength = db_get_file_list(str0, &fileList, 0, 0);
    if (fileListLength == -1) {
        return -1;
    }

    if (fileListLength != map_backup_count) {
        // FIXME: Probably leaks fileList.
        EraseSave();
        return -1;
    }

    sprintf(gmpath, "%s\\%s\\%s%.2d\\", patches, "SAVEGAME", "SLOT", slot_cursor + 1);

    for (int index = fileListLength - 1; index >= 0; index--) {
        strcpy(str0, gmpath);
        strcat(str0, fileList[index]);
        strmfe(str1, str0, "SAV");
        remove(str1);
        if (rename(str0, str1) != 0) {
            // FIXME: Probably leaks fileList.
            EraseSave();
            return -1;
        }
    }

    db_free_file_list(&fileList, 0);

    if (!automap_db_flag) {
        return 0;
    }

    sprintf(gmpath, "%s\\%s\\%s%.2d\\", patches, "SAVEGAME", "SLOT", slot_cursor + 1);
    char* v1 = strmfe(str2, "AUTOMAP.DB", "BAK");
    strcpy(str0, gmpath);
    strcat(str0, v1);

    char* v2 = strmfe(str2, "AUTOMAP.DB", "SAV");
    strcpy(str1, gmpath);
    strcat(str1, v2);

    if (rename(str0, str1) != 0) {
        EraseSave();
        return -1;
    }

    return 0;
}

// 0x480710
static int LoadObjDudeCid(File* stream)
{
    int value;

    if (db_freadInt(stream, &value) == -1) {
        return -1;
    }

    obj_dude->cid = value;

    return 0;
}

// 0x480734
static int SaveObjDudeCid(File* stream)
{
    return db_fwriteInt(stream, obj_dude->cid);
}

// 0x480754
static int EraseSave()
{
    debug_printf("\nLOADSAVE: Erasing save(bad) slot...\n");

    sprintf(gmpath, "%s\\%s\\%s%.2d\\", patches, "SAVEGAME", "SLOT", slot_cursor + 1);
    strcpy(str0, gmpath);
    strcat(str0, "SAVE.DAT");
    remove(str0);

    sprintf(gmpath, "%s\\%s%.2d\\", "SAVEGAME", "SLOT", slot_cursor + 1);
    sprintf(str0, "%s*.%s", gmpath, "SAV");

    char** fileList;
    int fileListLength = db_get_file_list(str0, &fileList, 0, 0);
    if (fileListLength == -1) {
        return -1;
    }

    sprintf(gmpath, "%s\\%s\\%s%.2d\\", patches, "SAVEGAME", "SLOT", slot_cursor + 1);
    for (int index = fileListLength - 1; index >= 0; index--) {
        strcpy(str0, gmpath);
        strcat(str0, fileList[index]);
        remove(str0);
    }

    db_free_file_list(&fileList, 0);

    sprintf(gmpath, "%s\\%s\\%s%.2d\\", patches, "SAVEGAME", "SLOT", slot_cursor + 1);

    char* v1 = strmfe(str1, "AUTOMAP.DB", "SAV");
    strcpy(str0, gmpath);
    strcat(str0, v1);

    remove(str0);

    return 0;
}
