#include "character_editor.h"

#include "art.h"
#include "color.h"
#include "core.h"
#include "critter.h"
#include "cycle.h"
#include "db.h"
#include "dbox.h"
#include "debug.h"
#include "draw.h"
#include "game.h"
#include "game_mouse.h"
#include "game_palette.h"
#include "game_sound.h"
#include "interface.h"
#include "item.h"
#include "map.h"
#include "memory.h"
#include "object.h"
#include "palette.h"
#include "perk.h"
#include "proto.h"
#include "scripts.h"
#include "skill.h"
#include "stat.h"
#include "text_font.h"
#include "trait.h"
#include "window_manager.h"
#include "word_wrap.h"
#include "world_map.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define RENDER_ALL_STATS 7

#define EDITOR_WIN_WIDTH 640
#define EDITOR_WIN_HEIGHT 480

#define NAME_BUTTON_X 9
#define NAME_BUTTON_Y 0

#define TAG_SKILLS_BUTTON_X 347
#define TAG_SKILLS_BUTTON_Y 26
#define TAG_SKILLS_BUTTON_CODE 536

#define PRINT_BTN_X 363
#define PRINT_BTN_Y 454

#define DONE_BTN_X 475
#define DONE_BTN_Y 454

#define CANCEL_BTN_X 571
#define CANCEL_BTN_Y 454

#define NAME_BTN_CODE 517
#define AGE_BTN_CODE 519
#define SEX_BTN_CODE 520

#define OPTIONAL_TRAITS_LEFT_BTN_X 23
#define OPTIONAL_TRAITS_RIGHT_BTN_X 298
#define OPTIONAL_TRAITS_BTN_Y 352

#define OPTIONAL_TRAITS_BTN_CODE 555

#define OPTIONAL_TRAITS_BTN_SPACE 2

#define SPECIAL_STATS_BTN_X 149

#define PERK_WINDOW_X 33
#define PERK_WINDOW_Y 91
#define PERK_WINDOW_WIDTH 573
#define PERK_WINDOW_HEIGHT 230

#define ANIMATE 0x01
#define RED_NUMBERS 0x02
#define BIG_NUM_WIDTH 14
#define BIG_NUM_HEIGHT 24
#define BIG_NUM_ANIMATION_DELAY 123

// 0x431C40
int dword_431C40[EDITOR_GRAPHIC_COUNT] = {
    170,
    175,
    176,
    181,
    182,
    183,
    184,
    185,
    186,
    187,
    188,
    189,
    190,
    191,
    192,
    193,
    194,
    195,
    196,
    197,
    198,
    199,
    200,
    8,
    9,
    204,
    205,
    206,
    207,
    208,
    209,
    210,
    211,
    212,
    213,
    214,
    122,
    123,
    124,
    125,
    219,
    220,
    221,
    222,
    178,
    179,
    180,
    38,
    215,
    216,
};

// flags to preload fid
const unsigned char byte_431D08[EDITOR_GRAPHIC_COUNT] = {
    0,
    0,
    1,
    0,
    0,
    0,
    1,
    1,
    0,
    0,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
    0,
    0,
};

// graphic ids for derived stats panel
// NOTE: the type originally short
const int word_431D3A[EDITOR_DERIVED_STAT_COUNT] = {
    18,
    19,
    20,
    21,
    22,
    23,
    83,
    24,
    25,
    26,
};

// y offsets for stats +/- buttons
const int dword_431D50[7] = {
    37,
    70,
    103,
    136,
    169,
    202,
    235,
};

// stat ids for derived stats panel
// NOTE: the type is originally short
const int word_431D6C[EDITOR_DERIVED_STAT_COUNT] = {
    STAT_ARMOR_CLASS,
    STAT_MAXIMUM_ACTION_POINTS,
    STAT_CARRY_WEIGHT,
    STAT_MELEE_DAMAGE,
    STAT_DAMAGE_RESISTANCE,
    STAT_POISON_RESISTANCE,
    STAT_RADIATION_RESISTANCE,
    STAT_SEQUENCE,
    STAT_HEALING_RATE,
    STAT_CRITICAL_CHANCE,
};

//
char byte_431D93[64];

const int dword_431DD4[7] = {
    1000000,
    100000,
    10000,
    1000,
    100,
    10,
    1,
};

char byte_5016E4[] = "------";
const double dbl_50170B = 14.4;
const double dbl_501713 = 19.2;
const double dbl_5018F0 = 19.2;
const double dbl_5019BE = 14.4;

bool dword_518528 = false;
int dword_51852C = 0;
int dword_518534 = 27;

// 0x518538
int characterEditorRemainingCharacterPoints = 0;

// 0x51853C
KarmaEntry* gKarmaEntries = NULL;
// 0x518540
int gKarmaEntriesLength = 0;

// 0x518544
GenericReputationEntry* gGenericReputationEntries = NULL;
// 0x518548
int gGenericReputationEntriesLength = 0;

// 0x51854C
const TownReputationEntry gTownReputationEntries[TOWN_REPUTATION_COUNT] = {
    { GVAR_TOWN_REP_ARROYO, CITY_ARROYO },
    { GVAR_TOWN_REP_KLAMATH, CITY_KLAMATH },
    { GVAR_TOWN_REP_THE_DEN, CITY_DEN },
    { GVAR_TOWN_REP_VAULT_CITY, CITY_VAULT_CITY },
    { GVAR_TOWN_REP_GECKO, CITY_GECKO },
    { GVAR_TOWN_REP_MODOC, CITY_MODOC },
    { GVAR_TOWN_REP_SIERRA_BASE, CITY_SIERRA_ARMY_BASE },
    { GVAR_TOWN_REP_BROKEN_HILLS, CITY_BROKEN_HILLS },
    { GVAR_TOWN_REP_NEW_RENO, CITY_NEW_RENO },
    { GVAR_TOWN_REP_REDDING, CITY_REDDING },
    { GVAR_TOWN_REP_NCR, CITY_NEW_CALIFORNIA_REPUBLIC },
    { GVAR_TOWN_REP_VAULT_13, CITY_VAULT_13 },
    { GVAR_TOWN_REP_SAN_FRANCISCO, CITY_SAN_FRANCISCO },
    { GVAR_TOWN_REP_ABBEY, CITY_ABBEY },
    { GVAR_TOWN_REP_EPA, CITY_ENV_PROTECTION_AGENCY },
    { GVAR_TOWN_REP_PRIMITIVE_TRIBE, CITY_PRIMITIVE_TRIBE },
    { GVAR_TOWN_REP_RAIDERS, CITY_RAIDERS },
    { GVAR_TOWN_REP_VAULT_15, CITY_VAULT_15 },
    { GVAR_TOWN_REP_GHOST_FARM, CITY_MODOC_GHOST_TOWN },
};

// 0x5185E4
const int gAddictionReputationVars[ADDICTION_REPUTATION_COUNT] = {
    GVAR_NUKA_COLA_ADDICT,
    GVAR_BUFF_OUT_ADDICT,
    GVAR_MENTATS_ADDICT,
    GVAR_PSYCHO_ADDICT,
    GVAR_RADAWAY_ADDICT,
    GVAR_ALCOHOL_ADDICT,
    GVAR_ADDICT_JET,
    GVAR_ADDICT_TRAGIC,
};

// 0x518604
const int gAddictionReputationFrmIds[ADDICTION_REPUTATION_COUNT] = {
    142,
    126,
    140,
    144,
    145,
    52,
    136,
    149,
};

// btn
int dword_518624 = -1;

// btn
int dword_518628 = -1;

char byte_56FB60[256];
int dword_56FC60[SKILL_COUNT];

// 0x56FCA8
MessageList editorMessageList;

STRUCT_56FCB0 stru_56FCB0[DIALOG_PICKER_NUM_OPTIONS];

// buttons for selecting traits
int dword_5700A8[TRAIT_COUNT];

// 0x5700E8
MessageListItem editorMessageListItem;

char byte_5700F8[48];
char byte_570128[48];

// buttons for tagging skills
int dword_570158[SKILL_COUNT];

// pc name
char byte_5701A0[32];

Size stru_5701C0[EDITOR_GRAPHIC_COUNT];

CacheEntry* dword_570350[EDITOR_GRAPHIC_COUNT];
unsigned char* dword_570418[EDITOR_GRAPHIC_COUNT];
unsigned char* dword_5704E0[EDITOR_GRAPHIC_COUNT];

int dword_5705A8;
int dword_5705AC;
int dword_5705B0;
int dword_5705B4;
char* off_5705B8;
char* off_5705BC;
int dword_5705C0;
int dword_5705C4;
int dword_5705C8;
char* off_5705CC;
int dword_5705D0;
int dword_5705D4;
int dword_5705D8;

// 0x5705DC
unsigned char* gEditorPerkBackgroundBuffer;

// 0x5705E0
int gEditorPerkWindow;

int dword_5705E4;
int dword_5705E8;

// - stats buttons
int dword_5705EC[7];

// 0x570608
unsigned char* characterEditorWindowBuf;

// 0x57060C
int characterEditorWindowHandle;

// + stats buttons
int dword_570610[7];

// 0x57062C
unsigned char* gEditorPerkWindowBuffer;

CritterProtoData stru_570630;

// 0x5707A4
unsigned char* characterEditorWindowBackgroundBuf;

int dword_5707A8;
int dword_5707AC;

// unspent hit points
int dword_5707B0;

int dword_5707B4;

// 0x5707B8
int characterEditorWindowOldFont;

int dword_5707BC;

// character editor background
CacheEntry* off_5707C0;

// current hit points
int dword_5707C4;

int dword_5707C8; // mouse y
int dword_5707CC; // mouse x

// 0x5707D0
int characterEditorSelectedItem;

// 0x5707D4
int characterEditorWindowSelectedFolder;

int dword_5707D8;
int dword_5707DC;
int dword_5707E0;
int dword_5707E4[PERK_COUNT];
unsigned int dword_5709C0;
unsigned int dword_5709C4;
int dword_5709C8;
int dword_5709CC;
int dword_5709E8;
int dword_5709EC;
// 0x5709D0
bool gCharacterEditorIsCreationMode;
int dword_5709D4[NUM_TAGGED_SKILLS];
int dword_5709F0[2];
// current index for selecting new trait
int dword_5709FC;
int dword_570A00;
int dword_570A04[TRAITS_MAX_SELECTED_COUNT];
int dword_570A10;
int dword_570A14[NUM_TAGGED_SKILLS];
char byte_570A28;
unsigned char byte_570A29;
unsigned char byte_570A2A;

//
int _editor_design(bool isCreationMode)
{
    char* messageListItemText;
    char line1[128];
    char line2[128];
    const char* lines[] = { line2 };

    gCharacterEditorIsCreationMode = isCreationMode;

    _SavePlayer();

    if (characterEditorWindowInit() == -1) {
        debugPrint("\n ** Error loading character editor data! **\n");
        return -1;
    }

    if (!gCharacterEditorIsCreationMode) {
        if (_UpdateLevel()) {
            critterUpdateDerivedStats(gDude);
            characterEditorWindowRenderTraits();
            editorRenderSkills(0);
            editorRenderPrimaryStat(RENDER_ALL_STATS, 0, 0);
            editorRenderSecondaryStats();
            editorRenderDetails();
        }
    }

    int rc = -1;
    while (rc == -1) {
        dword_5709C4 = _get_time();
        int keyCode = _get_input();

        bool done = false;
        if (keyCode == 500) {
            done = true;
        }

        if (keyCode == KEY_RETURN || keyCode == KEY_UPPERCASE_D || keyCode == KEY_LOWERCASE_D) {
            done = true;
            soundPlayFile("ib1p1xx1");
        }

        if (done) {
            if (gCharacterEditorIsCreationMode) {
                if (characterEditorRemainingCharacterPoints != 0) {
                    soundPlayFile("iisxxxx1");

                    // You must use all character points
                    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 118);
                    strcpy(line1, messageListItemText);

                    // before starting the game!
                    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 119);
                    strcpy(line2, messageListItemText);

                    showDialogBox(line1, lines, 1, 192, 126, byte_6A38D0[32328], 0, byte_6A38D0[32328], 0);
                    windowRefresh(characterEditorWindowHandle);

                    rc = -1;
                    continue;
                }

                if (dword_570A10 > 0) {
                    soundPlayFile("iisxxxx1");

                    // You must select all tag skills
                    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 142);
                    strcpy(line1, messageListItemText);

                    // before starting the game!
                    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 143);
                    strcpy(line2, messageListItemText);

                    showDialogBox(line1, lines, 1, 192, 126, byte_6A38D0[32328], 0, byte_6A38D0[32328], 0);
                    windowRefresh(characterEditorWindowHandle);

                    rc = -1;
                    continue;
                }

                if (_is_supper_bonus()) {
                    soundPlayFile("iisxxxx1");

                    // All stats must be between 1 and 10
                    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 157);
                    strcpy(line1, messageListItemText);

                    // before starting the game!
                    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 158);
                    strcpy(line2, messageListItemText);

                    showDialogBox(line1, lines, 1, 192, 126, byte_6A38D0[32328], 0, byte_6A38D0[32328], 0);
                    windowRefresh(characterEditorWindowHandle);

                    rc = -1;
                    continue;
                }

                if (stricmp(critterGetName(gDude), "None") == 0) {
                    soundPlayFile("iisxxxx1");

                    // Warning: You haven't changed your player
                    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 160);
                    strcpy(line1, messageListItemText);

                    // name. Use this character any way?
                    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 161);
                    strcpy(line2, messageListItemText);

                    if (showDialogBox(line1, lines, 1, 192, 126, byte_6A38D0[32328], 0, byte_6A38D0[32328], DIALOG_BOX_YES_NO) == 0) {
                        windowRefresh(characterEditorWindowHandle);

                        rc = -1;
                        continue;
                    }
                }
            }

            windowRefresh(characterEditorWindowHandle);
            rc = 0;
        } else if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
            showQuitConfirmationDialog();
            windowRefresh(characterEditorWindowHandle);
        } else if (keyCode == 502 || keyCode == KEY_ESCAPE || keyCode == KEY_UPPERCASE_C || keyCode == KEY_LOWERCASE_C || dword_5186CC != 0) {
            windowRefresh(characterEditorWindowHandle);
            rc = 1;
        } else if (gCharacterEditorIsCreationMode && (keyCode == 517 || keyCode == KEY_UPPERCASE_N || keyCode == KEY_LOWERCASE_N)) {
            characterEditorEditName();
            windowRefresh(characterEditorWindowHandle);
        } else if (gCharacterEditorIsCreationMode && (keyCode == 519 || keyCode == KEY_UPPERCASE_A || keyCode == KEY_LOWERCASE_A)) {
            characterEditorRunEditAgeDialog();
            windowRefresh(characterEditorWindowHandle);
        } else if (gCharacterEditorIsCreationMode && (keyCode == 520 || keyCode == KEY_UPPERCASE_S || keyCode == KEY_LOWERCASE_S)) {
            characterEditorEditGender();
            windowRefresh(characterEditorWindowHandle);
        } else if (gCharacterEditorIsCreationMode && (keyCode >= 503 && keyCode < 517)) {
            characterEditorHandleIncDecPrimaryStat(keyCode);
            windowRefresh(characterEditorWindowHandle);
        } else if ((gCharacterEditorIsCreationMode && (keyCode == 501 || keyCode == KEY_UPPERCASE_O || keyCode == KEY_LOWERCASE_O))
            || (!gCharacterEditorIsCreationMode && (keyCode == 501 || keyCode == KEY_UPPERCASE_P || keyCode == KEY_LOWERCASE_P))) {
            // _OptionWindow();
            windowRefresh(characterEditorWindowHandle);
        } else if (keyCode >= 525 && keyCode < 535) {
            _InfoButton(keyCode);
            windowRefresh(characterEditorWindowHandle);
        } else {
            switch (keyCode) {
            case KEY_TAB:
                if (characterEditorSelectedItem >= 0 && characterEditorSelectedItem < 7) {
                    characterEditorSelectedItem = gCharacterEditorIsCreationMode ? 82 : 7;
                } else if (characterEditorSelectedItem >= 7 && characterEditorSelectedItem < 9) {
                    if (gCharacterEditorIsCreationMode) {
                        characterEditorSelectedItem = 82;
                    } else {
                        characterEditorSelectedItem = 10;
                        characterEditorWindowSelectedFolder = 0;
                    }
                } else if (characterEditorSelectedItem >= 10 && characterEditorSelectedItem < 43) {
                    switch (characterEditorWindowSelectedFolder) {
                    case EDITOR_FOLDER_PERKS:
                        characterEditorSelectedItem = 10;
                        characterEditorWindowSelectedFolder = EDITOR_FOLDER_KARMA;
                        break;
                    case EDITOR_FOLDER_KARMA:
                        characterEditorSelectedItem = 10;
                        characterEditorWindowSelectedFolder = EDITOR_FOLDER_KILLS;
                        break;
                    case EDITOR_FOLDER_KILLS:
                        characterEditorSelectedItem = 43;
                        break;
                    }
                } else if (characterEditorSelectedItem >= 43 && characterEditorSelectedItem < 51) {
                    characterEditorSelectedItem = 51;
                } else if (characterEditorSelectedItem >= 51 && characterEditorSelectedItem < 61) {
                    characterEditorSelectedItem = 61;
                } else if (characterEditorSelectedItem >= 61 && characterEditorSelectedItem < 82) {
                    characterEditorSelectedItem = 0;
                } else if (characterEditorSelectedItem >= 82 && characterEditorSelectedItem < 98) {
                    characterEditorSelectedItem = 43;
                }
                editorRenderPrimaryStat(RENDER_ALL_STATS, 0, 0);
                characterEditorWindowRenderTraits();
                editorRenderSkills(0);
                editorRenderPcStats();
                editorRenderFolders();
                editorRenderSecondaryStats();
                editorRenderDetails();
                windowRefresh(characterEditorWindowHandle);
                break;
            case KEY_ARROW_LEFT:
            case KEY_MINUS:
            case KEY_UPPERCASE_J:
                if (characterEditorSelectedItem >= 0 && characterEditorSelectedItem < 7) {
                    if (gCharacterEditorIsCreationMode) {
                        _win_button_press_and_release(dword_5705EC[characterEditorSelectedItem]);
                        windowRefresh(characterEditorWindowHandle);
                    }
                } else if (characterEditorSelectedItem >= 61 && characterEditorSelectedItem < 79) {
                    if (gCharacterEditorIsCreationMode) {
                        _win_button_press_and_release(dword_570158[gCharacterEditorIsCreationMode - 61]);
                    }
                } else if (characterEditorSelectedItem >= 82 && characterEditorSelectedItem < 98) {
                    if (gCharacterEditorIsCreationMode) {
                        _win_button_press_and_release(dword_5700A8[gCharacterEditorIsCreationMode - 82]);
                        windowRefresh(characterEditorWindowHandle);
                    }
                }
                break;
            case KEY_ARROW_RIGHT:
            case KEY_PLUS:
            case KEY_UPPERCASE_N:
                if (characterEditorSelectedItem >= 0 && characterEditorSelectedItem < 7) {
                    if (gCharacterEditorIsCreationMode) {
                        _win_button_press_and_release(dword_570610[characterEditorSelectedItem]);
                        windowRefresh(characterEditorWindowHandle);
                    }
                } else if (characterEditorSelectedItem >= 61 && characterEditorSelectedItem < 79) {
                    if (gCharacterEditorIsCreationMode) {
                        _win_button_press_and_release(dword_570158[gCharacterEditorIsCreationMode - 61]);
                    }
                } else if (characterEditorSelectedItem >= 82 && characterEditorSelectedItem < 98) {
                    if (gCharacterEditorIsCreationMode) {
                        _win_button_press_and_release(dword_5700A8[gCharacterEditorIsCreationMode - 82]);
                        windowRefresh(characterEditorWindowHandle);
                    }
                }
                break;
            case KEY_ARROW_UP:
                if (characterEditorSelectedItem >= 10 && characterEditorSelectedItem < 43) {
                    if (characterEditorSelectedItem == 10) {
                        if (dword_5705B4 <= 0) {
                            _folder_scroll(-1);
                        }
                    }
                    characterEditorSelectedItem -= 1;
                    editorRenderFolders();
                    editorRenderDetails();
                    windowRefresh(characterEditorWindowHandle);
                } else {
                    switch (characterEditorSelectedItem) {
                    case 0:
                        characterEditorSelectedItem = 6;
                        break;
                    case 7:
                        characterEditorSelectedItem = 9;
                        break;
                    case 43:
                        characterEditorSelectedItem = 50;
                        break;
                    case 51:
                        characterEditorSelectedItem = 60;
                        break;
                    case 61:
                        characterEditorSelectedItem = 78;
                        break;
                    case 82:
                        characterEditorSelectedItem = 97;
                        break;
                    default:
                        characterEditorSelectedItem -= 1;
                        break;
                    }

                    if (characterEditorSelectedItem >= 61 && characterEditorSelectedItem < 79) {
                        dword_51852C = characterEditorSelectedItem - 61;
                    }

                    editorRenderPrimaryStat(RENDER_ALL_STATS, 0, 0);
                    characterEditorWindowRenderTraits();
                    editorRenderSkills(0);
                    editorRenderPcStats();
                    editorRenderFolders();
                    editorRenderSecondaryStats();
                    editorRenderDetails();
                    windowRefresh(characterEditorWindowHandle);
                }
                break;
            case KEY_ARROW_DOWN:
                if (characterEditorSelectedItem >= 10 && characterEditorSelectedItem < 43) {
                    if (characterEditorSelectedItem - 10 < dword_5705AC - dword_5705B4) {
                        if (characterEditorSelectedItem - 10 == dword_5705A8 - 1) {
                            _folder_scroll(1);
                        }

                        characterEditorSelectedItem++;

                        editorRenderFolders();
                        editorRenderDetails();
                    }

                    windowRefresh(characterEditorWindowHandle);
                } else {
                    switch (characterEditorSelectedItem) {
                    case 6:
                        characterEditorSelectedItem = 0;
                        break;
                    case 9:
                        characterEditorSelectedItem = 7;
                        break;
                    case 50:
                        characterEditorSelectedItem = 43;
                        break;
                    case 60:
                        characterEditorSelectedItem = 51;
                        break;
                    case 78:
                        characterEditorSelectedItem = 61;
                        break;
                    case 97:
                        characterEditorSelectedItem = 82;
                        break;
                    default:
                        characterEditorSelectedItem += 1;
                        break;
                    }

                    if (characterEditorSelectedItem >= 61 && characterEditorSelectedItem < 79) {
                        dword_51852C = characterEditorSelectedItem - 61;
                    }

                    editorRenderPrimaryStat(RENDER_ALL_STATS, 0, 0);
                    characterEditorWindowRenderTraits();
                    editorRenderSkills(0);
                    editorRenderPcStats();
                    editorRenderFolders();
                    editorRenderSecondaryStats();
                    editorRenderDetails();
                    windowRefresh(characterEditorWindowHandle);
                }
                break;
            case 521:
            case 523:
                editorAdjustSkill(keyCode);
                windowRefresh(characterEditorWindowHandle);
                break;
            case 535:
                _FldrButton();
                windowRefresh(characterEditorWindowHandle);
                break;
            case 17000:
                _folder_scroll(-1);
                windowRefresh(characterEditorWindowHandle);
                break;
            case 17001:
                _folder_scroll(1);
                windowRefresh(characterEditorWindowHandle);
                break;
            default:
                if (gCharacterEditorIsCreationMode && (keyCode >= 536 && keyCode < 554)) {
                    characterEditorToggleTaggedSkill(keyCode - 536);
                    windowRefresh(characterEditorWindowHandle);
                } else if (gCharacterEditorIsCreationMode && (keyCode >= 555 && keyCode < 571)) {
                    characterEditorToggleOptionalTrait(keyCode - 555);
                    windowRefresh(characterEditorWindowHandle);
                } else {
                    if (keyCode == 390) {
                        takeScreenshot();
                    }

                    windowRefresh(characterEditorWindowHandle);
                }
            }
        }
    }

    if (rc == 0) {
        if (isCreationMode) {
            _proto_dude_update_gender();
            paletteFadeTo(gPaletteBlack);
        }
    }

    characterEditorWindowFree();

    if (rc == 1) {
        _RestorePlayer();
    }

    if (dudeHasState(0x03)) {
        dudeDisableState(0x03);
    }

    interfaceRenderHitPoints(false);

    return rc;
}


// 0x4329EC
int characterEditorWindowInit()
{
    int i;
    int v1;
    int v3;
    char path[MAX_PATH];
    int fid;
    char* str;
    int len;
    int btn;
    int x;
    int y;
    char perks[32];
    char karma[32];
    char kills[32];

    characterEditorWindowOldFont = fontGetCurrent();
    dword_5709C8 = 0;
    dword_518528 = 0;
    dword_5709E8 = -1;
    dword_5709EC = -1;
    dword_5707E0 = 0;
    dword_5707D8 = 0;
    byte_570A2A = 1;
    byte_570128[0] = '\0';
    byte_5700F8[0] = '\0';

    fontSetCurrent(101);

    dword_518534 = dword_51852C * (fontGetLineHeight() + 1) + 27;

    // skills
    skillsGetTagged(dword_570A14, NUM_TAGGED_SKILLS);

    v1 = 0;
    for (i = 3; i >= 0; i--) {
        if (dword_570A14[i] != -1) {
            break;
        }

        v1++;
    }

    if (gCharacterEditorIsCreationMode) {
        v1--;
    }

    dword_570A10 = v1;

    // traits
    traitsGetSelected(&(dword_570A04[0]), &(dword_570A04[1]));

    v3 = 0;
    for (i = 1; i >= 0; i--) {
        if (dword_570A04[i] != -1) {
            break;
        }

        v3++;
    }

    dword_5709FC = v3;

    if (!gCharacterEditorIsCreationMode) {
        dword_518528 = isoDisable();
    }

    colorCycleDisable();
    gameMouseSetCursor(MOUSE_CURSOR_ARROW);

    if (!messageListInit(&editorMessageList)) {
        return -1;
    }

    sprintf(path, "%s%s", asc_5186C8, "editor.msg");

    if (!messageListLoad(&editorMessageList, path)) {
        return -1;
    }

    fid = buildFid(6, (gCharacterEditorIsCreationMode ? 169 : 177), 0, 0, 0);
    characterEditorWindowBackgroundBuf = artLockFrameDataReturningSize(fid, &off_5707C0, &(stru_5701C0[0].width), &(stru_5701C0[0].height));
    if (characterEditorWindowBackgroundBuf == NULL) {
        messageListFree(&editorMessageList);
        return -1;
    }

    if (karmaInit() == -1) {
        return -1;
    }

    if (genericReputationInit() == -1) {
        return -1;
    }

    soundContinueAll();

    for (i = 0; i < EDITOR_GRAPHIC_COUNT; i++) {
        fid = buildFid(6, dword_431C40[i], 0, 0, 0);
        dword_5704E0[i] = artLockFrameDataReturningSize(fid, &(dword_570350[i]), &(stru_5701C0[i].width), &(stru_5701C0[i].height));
        if (dword_5704E0[i] == NULL) {
            break;
        }
    }

    if (i != EDITOR_GRAPHIC_COUNT) {
        while (--i >= 0) {
            artUnlock(dword_570350[i]);
        }
        return -1;

        artUnlock(off_5707C0);

        messageListFree(&editorMessageList);

        if (dword_518528) {
            isoEnable();
        }

        colorCycleEnable();
        gameMouseSetCursor(MOUSE_CURSOR_ARROW);
        return -1;
    }

    soundContinueAll();

    for (i = 0; i < EDITOR_GRAPHIC_COUNT; i++) {
        if (byte_431D08[i]) {
            dword_570418[i] = internal_malloc(stru_5701C0[i].width * stru_5701C0[i].height);
            if (dword_570418[i] == NULL) {
                break;
            }
            memcpy(dword_570418[i], dword_5704E0[i], stru_5701C0[i].width * stru_5701C0[i].height);
        } else {
            dword_570418[i] = (unsigned char*)-1;
        }
    }

    if (i != EDITOR_GRAPHIC_COUNT) {
        while (--i >= 0) {
            if (byte_431D08[i]) {
                internal_free(dword_570418[i]);
            }
        }

        for (i = 0; i < EDITOR_GRAPHIC_COUNT; i++) {
            artUnlock(dword_570350[i]);
        }

        artUnlock(off_5707C0);

        messageListFree(&editorMessageList);
        if (dword_518528) {
            isoEnable();
        }

        colorCycleEnable();
        gameMouseSetCursor(MOUSE_CURSOR_ARROW);

        return -1;
    }

    characterEditorWindowHandle = windowCreate(0, 0, 640, 480, 256, 18);
    if (characterEditorWindowHandle == -1) {
        for (i = 0; i < EDITOR_GRAPHIC_COUNT; i++) {
            if (byte_431D08[i]) {
                internal_free(dword_570418[i]);
            }
            artUnlock(dword_570350[i]);
        }

        artUnlock(off_5707C0);

        messageListFree(&editorMessageList);
        if (dword_518528) {
            isoEnable();
        }

        colorCycleEnable();
        gameMouseSetCursor(MOUSE_CURSOR_ARROW);

        return -1;
    }

    characterEditorWindowBuf = windowGetBuffer(characterEditorWindowHandle);
    memcpy(characterEditorWindowBuf, characterEditorWindowBackgroundBuf, 640 * 480);

    if (gCharacterEditorIsCreationMode) {
        fontSetCurrent(103);

        // CHAR POINTS
        str = getmsg(&editorMessageList, &editorMessageListItem, 116);
        fontDrawText(characterEditorWindowBuf + (286 * 640) + 14, str, 640, 640, byte_6A38D0[18979]);
        characterEditorRenderBigNumber(126, 282, 0, characterEditorRemainingCharacterPoints, 0, characterEditorWindowHandle);

        // OPTIONS
        str = getmsg(&editorMessageList, &editorMessageListItem, 101);
        fontDrawText(characterEditorWindowBuf + (454 * 640) + 363, str, 640, 640, byte_6A38D0[18979]);

        // OPTIONAL TRAITS
        str = getmsg(&editorMessageList, &editorMessageListItem, 139);
        fontDrawText(characterEditorWindowBuf + (326 * 640) + 52, str, 640, 640, byte_6A38D0[18979]);
        characterEditorRenderBigNumber(522, 228, 0, dword_570A00, 0, characterEditorWindowHandle);

        // TAG SKILLS
        str = getmsg(&editorMessageList, &editorMessageListItem, 138);
        fontDrawText(characterEditorWindowBuf + (233 * 640) + 422, str, 640, 640, byte_6A38D0[18979]);
        characterEditorRenderBigNumber(522, 228, 0, dword_570A10, 0, characterEditorWindowHandle);
    } else {
        fontSetCurrent(103);

        str = getmsg(&editorMessageList, &editorMessageListItem, 109);
        strcpy(perks, str);

        str = getmsg(&editorMessageList, &editorMessageListItem, 110);
        strcpy(karma, str);

        str = getmsg(&editorMessageList, &editorMessageListItem, 111);
        strcpy(kills, str);

        // perks selected
        len = fontGetStringWidth(perks);
        fontDrawText(
            dword_570418[46] + 5 * stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 61 - len / 2,
            perks,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            byte_6A38D0[18979]);

        len = fontGetStringWidth(karma);
        fontDrawText(dword_570418[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED] + 5 * stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 159 - len / 2,
            karma,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            byte_6A38D0[14723]);

        len = fontGetStringWidth(kills);
        fontDrawText(dword_570418[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED] + 5 * stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 257 - len / 2,
            kills,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            byte_6A38D0[14723]);

        // karma selected
        len = fontGetStringWidth(perks);
        fontDrawText(dword_570418[EDITOR_GRAPHIC_KARMA_FOLDER_SELECTED] + 5 * stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 61 - len / 2,
            perks,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            byte_6A38D0[14723]);

        len = fontGetStringWidth(karma);
        fontDrawText(dword_570418[EDITOR_GRAPHIC_KARMA_FOLDER_SELECTED] + 5 * stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 159 - len / 2,
            karma,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            byte_6A38D0[18979]);

        len = fontGetStringWidth(kills);
        fontDrawText(dword_570418[EDITOR_GRAPHIC_KARMA_FOLDER_SELECTED] + 5 * stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 257 - len / 2,
            kills,
            stru_5701C0[46].width,
            stru_5701C0[46].width,
            byte_6A38D0[14723]);

        // kills selected
        len = fontGetStringWidth(perks);
        fontDrawText(dword_570418[EDITOR_GRAPHIC_KILLS_FOLDER_SELECTED] + 5 * stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 61 - len / 2,
            perks,
            stru_5701C0[46].width,
            stru_5701C0[46].width,
            byte_6A38D0[14723]);

        len = fontGetStringWidth(karma);
        fontDrawText(dword_570418[EDITOR_GRAPHIC_KILLS_FOLDER_SELECTED] + 5 * stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 159 - len / 2,
            karma,
            stru_5701C0[46].width,
            stru_5701C0[46].width,
            byte_6A38D0[14723]);

        len = fontGetStringWidth(kills);
        fontDrawText(dword_570418[EDITOR_GRAPHIC_KILLS_FOLDER_SELECTED] + 5 * stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 257 - len / 2,
            kills,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            byte_6A38D0[18979]);

        editorRenderFolders();

        fontSetCurrent(103);

        // PRINT
        str = getmsg(&editorMessageList, &editorMessageListItem, 103);
        fontDrawText(characterEditorWindowBuf + (EDITOR_WIN_WIDTH * PRINT_BTN_Y) + PRINT_BTN_X, str, EDITOR_WIN_WIDTH, EDITOR_WIN_WIDTH, byte_6A38D0[18979]);

        editorRenderPcStats();
        _folder_init();
    }

    fontSetCurrent(103);

    // CANCEL
    str = getmsg(&editorMessageList, &editorMessageListItem, 102);
    fontDrawText(characterEditorWindowBuf + (EDITOR_WIN_WIDTH * CANCEL_BTN_Y) + CANCEL_BTN_X, str, EDITOR_WIN_WIDTH, EDITOR_WIN_WIDTH, byte_6A38D0[18979]);

    // DONE
    str = getmsg(&editorMessageList, &editorMessageListItem, 100);
    fontDrawText(characterEditorWindowBuf + (EDITOR_WIN_WIDTH * DONE_BTN_Y) + DONE_BTN_X, str, EDITOR_WIN_WIDTH, EDITOR_WIN_WIDTH, byte_6A38D0[18979]);

    editorRenderPrimaryStat(RENDER_ALL_STATS, 0, 0);
    editorRenderSecondaryStats();

    if (!gCharacterEditorIsCreationMode) {
        dword_5705E4 = buttonCreate(
            characterEditorWindowHandle,
            614,
            20,
            stru_5701C0[EDITOR_GRAPHIC_SLIDER_PLUS_ON].width,
            stru_5701C0[EDITOR_GRAPHIC_SLIDER_PLUS_ON].height,
            -1,
            522,
            521,
            522,
            dword_5704E0[EDITOR_GRAPHIC_SLIDER_PLUS_OFF],
            dword_5704E0[EDITOR_GRAPHIC_SLIDER_PLUS_ON],
            0,
            96);
        dword_5705E8 = buttonCreate(
            characterEditorWindowHandle,
            614,
            20 + stru_5701C0[EDITOR_GRAPHIC_SLIDER_MINUS_ON].height - 1,
            stru_5701C0[EDITOR_GRAPHIC_SLIDER_MINUS_ON].width,
            stru_5701C0[EDITOR_GRAPHIC_SLIDER_MINUS_OFF].height,
            -1,
            524,
            523,
            524,
            dword_5704E0[EDITOR_GRAPHIC_SLIDER_MINUS_OFF],
            dword_5704E0[EDITOR_GRAPHIC_SLIDER_MINUS_ON],
            0,
            96);
        buttonSetCallbacks(dword_5705E4, _gsound_red_butt_press, NULL);
        buttonSetCallbacks(dword_5705E8, _gsound_red_butt_press, NULL);
    }

    editorRenderSkills(0);
    editorRenderDetails();
    soundContinueAll();
    editorRenderName();
    editorRenderAge();
    editorRenderGender();

    if (gCharacterEditorIsCreationMode) {
        x = NAME_BUTTON_X;
        btn = buttonCreate(
            characterEditorWindowHandle,
            x,
            NAME_BUTTON_Y,
            stru_5701C0[EDITOR_GRAPHIC_NAME_ON].width,
            stru_5701C0[EDITOR_GRAPHIC_NAME_ON].height,
            -1,
            -1,
            -1,
            NAME_BTN_CODE,
            dword_570418[EDITOR_GRAPHIC_NAME_OFF],
            dword_570418[EDITOR_GRAPHIC_NAME_ON],
            0,
            32);
        if (btn != -1) {
            buttonSetMask(btn, dword_5704E0[EDITOR_GRAPHIC_NAME_MASK]);
            buttonSetCallbacks(btn, _gsound_lrg_butt_press, NULL);
        }

        x += stru_5701C0[EDITOR_GRAPHIC_NAME_ON].width;
        btn = buttonCreate(
            characterEditorWindowHandle,
            x,
            NAME_BUTTON_Y,
            stru_5701C0[EDITOR_GRAPHIC_AGE_ON].width,
            stru_5701C0[EDITOR_GRAPHIC_AGE_ON].height,
            -1,
            -1,
            -1,
            AGE_BTN_CODE,
            dword_570418[EDITOR_GRAPHIC_AGE_OFF],
            dword_570418[EDITOR_GRAPHIC_AGE_ON],
            0,
            32);
        if (btn != -1) {
            buttonSetMask(btn, dword_5704E0[EDITOR_GRAPHIC_AGE_MASK]);
            buttonSetCallbacks(btn, _gsound_lrg_butt_press, NULL);
        }

        x += stru_5701C0[EDITOR_GRAPHIC_AGE_ON].width;
        btn = buttonCreate(
            characterEditorWindowHandle,
            x,
            NAME_BUTTON_Y,
            stru_5701C0[EDITOR_GRAPHIC_SEX_ON].width,
            stru_5701C0[EDITOR_GRAPHIC_SEX_ON].height,
            -1,
            -1,
            -1,
            SEX_BTN_CODE,
            dword_570418[EDITOR_GRAPHIC_SEX_OFF],
            dword_570418[EDITOR_GRAPHIC_SEX_ON],
            0,
            32);
        if (btn != -1) {
            buttonSetMask(btn, dword_5704E0[EDITOR_GRAPHIC_SEX_MASK]);
            buttonSetCallbacks(btn, _gsound_lrg_butt_press, NULL);
        }

        y = TAG_SKILLS_BUTTON_Y;
        for (i = 0; i < SKILL_COUNT; i++) {
            dword_570158[i] = buttonCreate(
                characterEditorWindowHandle,
                TAG_SKILLS_BUTTON_X,
                y,
                stru_5701C0[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].width,
                stru_5701C0[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height,
                -1,
                -1,
                -1,
                TAG_SKILLS_BUTTON_CODE + i,
                dword_5704E0[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_OFF],
                dword_5704E0[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON],
                NULL,
                32);
            y += stru_5701C0[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height;
        }

        y = OPTIONAL_TRAITS_BTN_Y;
        for (i = 0; i < TRAIT_COUNT / 2; i++) {
            dword_5700A8[i] = buttonCreate(
                characterEditorWindowHandle,
                OPTIONAL_TRAITS_LEFT_BTN_X,
                y,
                stru_5701C0[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].width,
                stru_5701C0[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height,
                -1,
                -1,
                -1,
                OPTIONAL_TRAITS_BTN_CODE + i,
                dword_5704E0[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_OFF],
                dword_5704E0[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON],
                NULL,
                32);
            y += stru_5701C0[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height + OPTIONAL_TRAITS_BTN_SPACE;
        }

        y = OPTIONAL_TRAITS_BTN_Y;
        for (i = TRAIT_COUNT / 2; i < TRAIT_COUNT; i++) {
            dword_5700A8[i] = buttonCreate(
                characterEditorWindowHandle,
                OPTIONAL_TRAITS_RIGHT_BTN_X,
                y,
                stru_5701C0[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].width,
                stru_5701C0[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height,
                -1,
                -1,
                -1,
                OPTIONAL_TRAITS_BTN_CODE + i,
                dword_5704E0[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_OFF],
                dword_5704E0[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON],
                NULL,
                32);
            y += stru_5701C0[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height + OPTIONAL_TRAITS_BTN_SPACE;
        }

        characterEditorWindowRenderTraits();
    } else {
        x = NAME_BUTTON_X;
        blitBufferToBufferTrans(dword_570418[EDITOR_GRAPHIC_NAME_OFF],
            stru_5701C0[EDITOR_GRAPHIC_NAME_ON].width,
            stru_5701C0[EDITOR_GRAPHIC_NAME_ON].height,
            stru_5701C0[EDITOR_GRAPHIC_NAME_ON].width,
            characterEditorWindowBuf + (EDITOR_WIN_WIDTH * NAME_BUTTON_Y) + x,
            EDITOR_WIN_WIDTH);

        x += stru_5701C0[EDITOR_GRAPHIC_NAME_ON].width;
        blitBufferToBufferTrans(dword_570418[EDITOR_GRAPHIC_AGE_OFF],
            stru_5701C0[EDITOR_GRAPHIC_AGE_ON].width,
            stru_5701C0[EDITOR_GRAPHIC_AGE_ON].height,
            stru_5701C0[EDITOR_GRAPHIC_AGE_ON].width,
            characterEditorWindowBuf + (EDITOR_WIN_WIDTH * NAME_BUTTON_Y) + x,
            EDITOR_WIN_WIDTH);

        x += stru_5701C0[EDITOR_GRAPHIC_AGE_ON].width;
        blitBufferToBufferTrans(dword_570418[EDITOR_GRAPHIC_SEX_OFF],
            stru_5701C0[EDITOR_GRAPHIC_SEX_ON].width,
            stru_5701C0[EDITOR_GRAPHIC_SEX_ON].height,
            stru_5701C0[EDITOR_GRAPHIC_SEX_ON].width,
            characterEditorWindowBuf + (EDITOR_WIN_WIDTH * NAME_BUTTON_Y) + x,
            EDITOR_WIN_WIDTH);

        btn = buttonCreate(characterEditorWindowHandle,
            11,
            327,
            stru_5701C0[EDITOR_GRAPHIC_FOLDER_MASK].width,
            stru_5701C0[EDITOR_GRAPHIC_FOLDER_MASK].height,
            -1,
            -1,
            -1,
            535,
            NULL,
            NULL,
            NULL,
            BUTTON_FLAG_TRANSPARENT);
        if (btn != -1) {
            buttonSetMask(btn, dword_5704E0[EDITOR_GRAPHIC_FOLDER_MASK]);
        }
    }

    if (gCharacterEditorIsCreationMode) {
        // +/- buttons for stats
        for (i = 0; i < 7; i++) {
            dword_570610[i] = buttonCreate(characterEditorWindowHandle,
                SPECIAL_STATS_BTN_X,
                dword_431D50[i],
                stru_5701C0[EDITOR_GRAPHIC_SLIDER_PLUS_ON].width,
                stru_5701C0[EDITOR_GRAPHIC_SLIDER_PLUS_ON].height,
                -1,
                518,
                503 + i,
                518,
                dword_5704E0[EDITOR_GRAPHIC_SLIDER_PLUS_OFF],
                dword_5704E0[EDITOR_GRAPHIC_SLIDER_PLUS_ON],
                NULL,
                32);
            if (dword_570610[i] != -1) {
                buttonSetCallbacks(dword_570610[i], _gsound_red_butt_press, NULL);
            }

            dword_5705EC[i] = buttonCreate(characterEditorWindowHandle,
                SPECIAL_STATS_BTN_X,
                dword_431D50[i] + stru_5701C0[EDITOR_GRAPHIC_SLIDER_PLUS_ON].height - 1,
                stru_5701C0[EDITOR_GRAPHIC_SLIDER_MINUS_ON].width,
                stru_5701C0[EDITOR_GRAPHIC_SLIDER_MINUS_ON].height,
                -1,
                518,
                510 + i,
                518,
                dword_5704E0[EDITOR_GRAPHIC_SLIDER_MINUS_OFF],
                dword_5704E0[EDITOR_GRAPHIC_SLIDER_MINUS_ON],
                NULL,
                32);
            if (dword_5705EC[i] != -1) {
                buttonSetCallbacks(dword_5705EC[i], _gsound_red_butt_press, NULL);
            }
        }
    }

    _RegInfoAreas();
    soundContinueAll();

    btn = buttonCreate(
        characterEditorWindowHandle,
        343,
        454,
        stru_5701C0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        stru_5701C0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        501,
        dword_5704E0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        dword_5704E0[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    btn = buttonCreate(
        characterEditorWindowHandle,
        552,
        454,
        stru_5701C0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        stru_5701C0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        502,
        dword_5704E0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        dword_5704E0[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        0,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    btn = buttonCreate(
        characterEditorWindowHandle,
        455,
        454,
        stru_5701C0[23].width,
        stru_5701C0[23].height,
        -1,
        -1,
        -1,
        500,
        dword_5704E0[23],
        dword_5704E0[24],
        0,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    windowRefresh(characterEditorWindowHandle);
    indicatorBarHide();

    return 0;
}

// 0x433AA8
void characterEditorWindowFree()
{
    if (dword_518628 != -1) {
        buttonDestroy(dword_518628);
        dword_518628 = -1;
    }

    if (dword_518624 != -1) {
        buttonDestroy(dword_518624);
        dword_518624 = -1;
    }

    windowDestroy(characterEditorWindowHandle);

    for (int index = 0; index < EDITOR_GRAPHIC_COUNT; index++) {
        artUnlock(dword_570350[index]);

        if (byte_431D08[index]) {
            internal_free(dword_570418[index]);
        }
    }

    artUnlock(off_5707C0);

    // NOTE: Uninline.
    genericReputationFree();

    // NOTE: Uninline.
    karmaFree();

    messageListFree(&editorMessageList);

    interfaceBarRefresh();

    if (dword_518528) {
        isoEnable();
    }

    colorCycleEnable();
    gameMouseSetCursor(MOUSE_CURSOR_ARROW);

    fontSetCurrent(characterEditorWindowOldFont);

    if (gCharacterEditorIsCreationMode == 1) {
        skillsSetTagged(dword_570A14, 3);
        traitsSetSelected(dword_570A04[0], dword_570A04[1]);
        characterEditorSelectedItem = 0;
        critterAdjustHitPoints(gDude, 1000);
    }

    indicatorBarShow();
}

// CharEditInit
// 0x433C0C
void _CharEditInit()
{
    int i;

    characterEditorSelectedItem = 0;
    dword_51852C = 0;
    dword_518534 = 27;
    byte_570A29 = 0;
    characterEditorWindowSelectedFolder = EDITOR_FOLDER_PERKS;
    
    for (i = 0; i < 2; i++) {
        dword_570A04[i] = -1;
        dword_5709F0[i] = -1;
    }

    characterEditorRemainingCharacterPoints = 5;
    dword_5707B4 = 1;
}

// handle name input
int _get_input_str(int win, int cancelKeyCode, char* text, int maxLength, int x, int y, int textColor, int backgroundColor, int flags)
{
    int cursorWidth = fontGetStringWidth("_") - 4;
    int windowWidth = windowGetWidth(win);
    int v60 = fontGetLineHeight();
    unsigned char* windowBuffer = windowGetBuffer(win);
    if (maxLength > 255) {
        maxLength = 255;
    }

    char copy[257];
    strcpy(copy, text);

    int nameLength = strlen(text);
    copy[nameLength] = ' ';
    copy[nameLength + 1] = '\0';

    int nameWidth = fontGetStringWidth(copy);

    bufferFill(windowBuffer + windowWidth * y + x, nameWidth, fontGetLineHeight(), windowWidth, backgroundColor);
    fontDrawText(windowBuffer + windowWidth * y + x, copy, windowWidth, windowWidth, textColor);

    windowRefresh(win);

    int blinkingCounter = 3;
    bool blink = false;

    int rc = 1;
    while (rc == 1) {
        dword_5709C4 = _get_time();

        int keyCode = _get_input();
        if (keyCode == cancelKeyCode) {
            rc = 0;
        } else if (keyCode == KEY_RETURN) {
            soundPlayFile("ib1p1xx1");
            rc = 0;
        } else if (keyCode == KEY_ESCAPE || dword_5186CC != 0) {
            rc = -1;
        } else {
            if ((keyCode == KEY_DELETE || keyCode == KEY_BACKSPACE) && nameLength >= 1) {
                bufferFill(windowBuffer + windowWidth * y + x, fontGetStringWidth(copy), v60, windowWidth, backgroundColor);
                copy[nameLength - 1] = ' ';
                copy[nameLength] = '\0';
                fontDrawText(windowBuffer + windowWidth * y + x, copy, windowWidth, windowWidth, textColor);
                nameLength--;

                windowRefresh(win);
            } else if ((keyCode >= KEY_FIRST_INPUT_CHARACTER && keyCode <= KEY_LAST_INPUT_CHARACTER) && nameLength < maxLength) {
                if ((flags & 0x01) != 0) {
                    if (!_isdoschar(keyCode)) {
                        break;
                    }
                }

                bufferFill(windowBuffer + windowWidth * y + x, fontGetStringWidth(copy), v60, windowWidth, backgroundColor);

                copy[nameLength] = keyCode & 0xFF;
                copy[nameLength + 1] = ' ';
                copy[nameLength + 2] = '\0';
                fontDrawText(windowBuffer + windowWidth * y + x, copy, windowWidth, windowWidth, textColor);
                nameLength++;

                windowRefresh(win);
            }
        }

        blinkingCounter -= 1;
        if (blinkingCounter == 0) {
            blinkingCounter = 3;

            int color = blink ? backgroundColor : textColor;
            blink = !blink;

            bufferFill(windowBuffer + windowWidth * y + x + fontGetStringWidth(copy) - cursorWidth, cursorWidth, v60 - 2, windowWidth, color);
        }

        windowRefresh(win);

        while (getTicksSince(dword_5709C4) < 1000 / 24) { }
    }

    if (rc == 0 || nameLength > 0) {
        copy[nameLength] = '\0';
        strcpy(text, copy);
    }

    return rc;
}

bool _isdoschar(int ch)
{
    const char* punctuations = "#@!$`'~^&()-_=[]{}";

    if (isalnum(ch)) {
        return true;
    }

    int length = strlen(punctuations);
    for (int index = 0; index < length; index++) {
        if (punctuations[index] == ch) {
            return true;
        }
    }

    return false;
}

// copy filename replacing extension
char* _strmfe(char* dest, const char* name, const char* ext)
{
    char* save = dest;

    while (*name != '\0' && *name != '.') {
        *dest++ = *name++;
    }

    *dest++ = '.';

    strcpy(dest, ext);

    return save;
}

// 0x43410C
void editorRenderFolders()
{
    if (gCharacterEditorIsCreationMode) {
        return;
    }

    blitBufferToBuffer(characterEditorWindowBackgroundBuf + (360 * 640) + 34, 280, 120, 640, characterEditorWindowBuf + (360 * 640) + 34, 640);

    fontSetCurrent(101);

    switch (characterEditorWindowSelectedFolder) {
    case EDITOR_FOLDER_PERKS:
        blitBufferToBuffer(dword_570418[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED],
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].height,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            characterEditorWindowBuf + (327 * 640) + 11,
            640);
        editorRenderPerks();
        break;
    case EDITOR_FOLDER_KARMA:
        blitBufferToBuffer(dword_570418[EDITOR_GRAPHIC_KARMA_FOLDER_SELECTED],
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].height,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            characterEditorWindowBuf + (327 * 640) + 11,
            640);
        editorRenderKarma();
        break;
    case EDITOR_FOLDER_KILLS:
        blitBufferToBuffer(dword_570418[EDITOR_GRAPHIC_KILLS_FOLDER_SELECTED],
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].height,
            stru_5701C0[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            characterEditorWindowBuf + (327 * 640) + 11,
            640);
        dword_5707BC = editorRenderKills();
        break;
    default:
        debugPrint("\n ** Unknown folder type! **\n");
        break;
    }
}

// 0x434238
void editorRenderPerks()
{
    const char* string;
    char perkName[80];
    int perk;
    int perkLevel;
    bool hasContent = false;

    _folder_clear();

    if (dword_570A04[0] != -1) {
        // TRAITS
        string = getmsg(&editorMessageList, &editorMessageListItem, 156);
        if (_folder_print_seperator(string)) {
            dword_5705B0 = 54;
            // Optional Traits
            off_5705B8 = getmsg(&editorMessageList, &editorMessageListItem, 146);
            off_5705BC = NULL;
            // Optional traits describe your character in more detail. All traits will have positive and negative effects. You may choose up to two traits during creation.
            off_5705CC = getmsg(&editorMessageList, &editorMessageListItem, 147);
            hasContent = true;
        }

        if (dword_570A04[0] != -1) {
            string = traitGetName(dword_570A04[0]);
            if (_folder_print_line(string)) {
                dword_5705B0 = traitGetFrmId(dword_570A04[0]);
                off_5705B8 = traitGetName(dword_570A04[0]);
                off_5705BC = NULL;
                off_5705CC = traitGetDescription(dword_570A04[0]);
                hasContent = true;
            }
        }

        if (dword_570A04[1] != -1) {
            string = traitGetName(dword_570A04[1]);
            if (_folder_print_line(string)) {
                dword_5705B0 = traitGetFrmId(dword_570A04[1]);
                off_5705B8 = traitGetName(dword_570A04[1]);
                off_5705BC = NULL;
                off_5705CC = traitGetDescription(dword_570A04[1]);
                hasContent = true;
            }
        }
    }

    for (perk = 0; perk < PERK_COUNT; perk++) {
        if (perkGetRank(gDude, perk) != 0) {
            break;
        }
    }

    if (perk != PERK_COUNT) {
        // PERKS
        string = getmsg(&editorMessageList, &editorMessageListItem, 109);
        _folder_print_seperator(string);

        for (perk = 0; perk < PERK_COUNT; perk++) {
            perkLevel = perkGetRank(gDude, perk);
            if (perkLevel != 0) {
                string = perkGetName(perk);

                if (perkLevel == 1) {
                    strcpy(perkName, string);
                } else {
                    sprintf(perkName, "%s (%d)", string, perkLevel);
                }

                if (_folder_print_line(perkName)) {
                    dword_5705B0 = perkGetFrmId(perk);
                    off_5705B8 = perkGetName(perk);
                    off_5705BC = NULL;
                    off_5705CC = perkGetDescription(perk);
                    hasContent = true;
                }
            }
        }
    }

    if (!hasContent) {
        dword_5705B0 = 71;
        // Perks
        off_5705B8 = getmsg(&editorMessageList, &editorMessageListItem, 124);
        off_5705BC = NULL;
        // Perks add additional abilities. Every third experience level, you can choose one perk.
        off_5705CC = getmsg(&editorMessageList, &editorMessageListItem, 127);
    }
}

int _kills_list_comp(const KillInfo* a, const KillInfo* b)
{
    return stricmp(a->name, b->name);
}

// 0x4344A4
int editorRenderKills()
{
    int i;
    int killsCount;
    KillInfo kills[19];
    int usedKills = 0;
    bool hasContent = false;

    _folder_clear();

    for (i = 0; i < KILL_TYPE_COUNT; i++) {
        killsCount = killsGetByType(i);
        if (killsCount != 0) {
            KillInfo* killInfo = &(kills[usedKills]);
            killInfo->name = killTypeGetName(i);
            killInfo->killTypeId = i;
            killInfo->kills = killsCount;
            usedKills++;
        }
    }

    if (usedKills != 0) {
        qsort(kills, usedKills, 12, (int (*)(const void*, const void*))_kills_list_comp);

        for (i = 0; i < usedKills; i++) {
            KillInfo* killInfo = &(kills[i]);
            if (editorDrawKillsEntry(killInfo->name, killInfo->kills)) {
                dword_5705B0 = 46;
                off_5705B8 = byte_56FB60;
                off_5705BC = NULL;
                off_5705CC = killTypeGetDescription(kills[i].killTypeId);
                sprintf(byte_56FB60, "%s %s", killInfo->name, getmsg(&editorMessageList, &editorMessageListItem, 126));
                hasContent = true;
            }
        }
    }

    if (!hasContent) {
        dword_5705B0 = 46;
        off_5705B8 = getmsg(&editorMessageList, &editorMessageListItem, 126);
        off_5705BC = NULL;
        off_5705CC = getmsg(&editorMessageList, &editorMessageListItem, 129);
    }

    return usedKills;
}

// 0x4345DC
void characterEditorRenderBigNumber(int x, int y, int flags, int value, int previousValue, int windowHandle)
{
    Rect rect;
    int windowWidth;
    unsigned char* windowBuf;
    int tens;
    int ones;
    unsigned char* tensBufferPtr;
    unsigned char* onesBufferPtr;
    unsigned char* numbersGraphicBufferPtr;

    windowWidth = windowGetWidth(windowHandle);
    windowBuf = windowGetBuffer(windowHandle);

    rect.left = x;
    rect.top = y;
    rect.right = x + BIG_NUM_WIDTH * 2;
    rect.bottom = y + BIG_NUM_HEIGHT;

    numbersGraphicBufferPtr = dword_5704E0[0];

    if (flags & RED_NUMBERS) {
        // First half of the bignum.frm is white,
        // second half is red.
        numbersGraphicBufferPtr += stru_5701C0[EDITOR_GRAPHIC_BIG_NUMBERS].width / 2;
    }

    tensBufferPtr = windowBuf + windowWidth * y + x;
    onesBufferPtr = tensBufferPtr + BIG_NUM_WIDTH;

    if (value >= 0 && value <= 99 && previousValue >= 0 && previousValue <= 99) {
        tens = value / 10;
        ones = value % 10;

        if (flags & ANIMATE) {
            if (previousValue % 10 != ones) {
                dword_5709C4 = _get_time();
                blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * 11,
                    BIG_NUM_WIDTH,
                    BIG_NUM_HEIGHT,
                    stru_5701C0[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                    onesBufferPtr,
                    windowWidth);
                windowRefreshRect(windowHandle, &rect);
                while (getTicksSince(dword_5709C4) < BIG_NUM_ANIMATION_DELAY)
                    ;
            }

            blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * ones,
                BIG_NUM_WIDTH,
                BIG_NUM_HEIGHT,
                stru_5701C0[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                onesBufferPtr,
                windowWidth);
            windowRefreshRect(windowHandle, &rect);

            if (previousValue / 10 != tens) {
                dword_5709C4 = _get_time();
                blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * 11,
                    BIG_NUM_WIDTH,
                    BIG_NUM_HEIGHT,
                    stru_5701C0[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                    tensBufferPtr,
                    windowWidth);
                windowRefreshRect(windowHandle, &rect);
                while (getTicksSince(dword_5709C4) < BIG_NUM_ANIMATION_DELAY)
                    ;
            }

            blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * tens,
                BIG_NUM_WIDTH,
                BIG_NUM_HEIGHT,
                stru_5701C0[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                tensBufferPtr,
                windowWidth);
            windowRefreshRect(windowHandle, &rect);
        } else {
            blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * tens,
                BIG_NUM_WIDTH,
                BIG_NUM_HEIGHT,
                stru_5701C0[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                tensBufferPtr,
                windowWidth);
            blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * ones,
                BIG_NUM_WIDTH,
                BIG_NUM_HEIGHT,
                stru_5701C0[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                onesBufferPtr,
                windowWidth);
        }
    } else {

        blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * 9,
            BIG_NUM_WIDTH,
            BIG_NUM_HEIGHT,
            stru_5701C0[EDITOR_GRAPHIC_BIG_NUMBERS].width,
            tensBufferPtr,
            windowWidth);
        blitBufferToBuffer(numbersGraphicBufferPtr + BIG_NUM_WIDTH * 9,
            BIG_NUM_WIDTH,
            BIG_NUM_HEIGHT,
            stru_5701C0[EDITOR_GRAPHIC_BIG_NUMBERS].width,
            onesBufferPtr,
            windowWidth);
    }
}

// 0x434920
void editorRenderPcStats()
{
    int color;
    int y;
    char* formattedValue;
    char formattedValueBuffer[8];
    char stringBuffer[128];

    if (gCharacterEditorIsCreationMode == 1) {
        return;
    }

    fontSetCurrent(101);

    blitBufferToBuffer(characterEditorWindowBackgroundBuf + 640 * 280 + 32, 124, 32, 640, characterEditorWindowBuf + 640 * 280 + 32, 640);

    // LEVEL
    y = 280;
    if (characterEditorSelectedItem != 7) {
        color = byte_6A38D0[992];
    } else {
        color = byte_6A38D0[32747];
    }

    int level = pcGetStat(PC_STAT_LEVEL);
    sprintf(stringBuffer, "%s %d",
        getmsg(&editorMessageList, &editorMessageListItem, 113),
        level);
    fontDrawText(characterEditorWindowBuf + 640 * y + 32, stringBuffer, 640, 640, color);

    // EXPERIENCE
    y += fontGetLineHeight() + 1;
    if (characterEditorSelectedItem != 8) {
        color = byte_6A38D0[992];
    } else {
        color = byte_6A38D0[32747];
    }

    int exp = pcGetStat(PC_STAT_EXPERIENCE);
    sprintf(stringBuffer, "%s %s",
        getmsg(&editorMessageList, &editorMessageListItem, 114),
        _itostndn(exp, formattedValueBuffer));
    fontDrawText(characterEditorWindowBuf + 640 * y + 32, stringBuffer, 640, 640, color);

    // EXP NEEDED TO NEXT LEVEL
    y += fontGetLineHeight() + 1;
    if (characterEditorSelectedItem != 9) {
        color = byte_6A38D0[992];
    } else {
        color = byte_6A38D0[32747];
    }

    int expToNextLevel = pcGetExperienceForNextLevel();
    int expMsgId;
    if (expToNextLevel == -1) {
        expMsgId = 115;
        formattedValue = byte_5016E4;
    } else {
        expMsgId = 115;
        if (expToNextLevel > 999999) {
            expMsgId = 175;
        }
        formattedValue = _itostndn(expToNextLevel, formattedValueBuffer);
    }

    sprintf(stringBuffer, "%s %s",
        getmsg(&editorMessageList, &editorMessageListItem, expMsgId),
        formattedValue);
    fontDrawText(characterEditorWindowBuf + 640 * y + 32, stringBuffer, 640, 640, color);
}

// 0x434B38
void editorRenderPrimaryStat(int stat, bool animate, int previousValue)
{
    int off;
    int color;
    const char* description;
    int value;
    int flags;
    int messageListItemId;

    fontSetCurrent(101);

    if (stat == RENDER_ALL_STATS) {
        // NOTE: Original code is different, looks like tail recursion
        // optimization.
        for (stat = 0; stat < 7; stat++) {
            editorRenderPrimaryStat(stat, 0, 0);
        }
        return;
    }

    if (characterEditorSelectedItem == stat) {
        color = byte_6A38D0[32747];
    } else {
        color = byte_6A38D0[992];
    }

    off = 640 * (dword_431D50[stat] + 8) + 103;

    // TODO: The original code is different.
    if (gCharacterEditorIsCreationMode) {
        value = critterGetBaseStatWithTraitModifier(gDude, stat) + critterGetBonusStat(gDude, stat);

        flags = 0;

        if (animate) {
            flags |= ANIMATE;
        }

        if (value > 10) {
            flags |= RED_NUMBERS;
        }

        characterEditorRenderBigNumber(58, dword_431D50[stat], flags, value, previousValue, characterEditorWindowHandle);

        blitBufferToBuffer(characterEditorWindowBackgroundBuf + off, 40, fontGetLineHeight(), 640, characterEditorWindowBuf + off, 640);

        messageListItemId = critterGetStat(gDude, stat) + 199;
        if (messageListItemId > 210) {
            messageListItemId = 210;
        }

        description = getmsg(&editorMessageList, &editorMessageListItem, messageListItemId);
        fontDrawText(characterEditorWindowBuf + 640 * (dword_431D50[stat] + 8) + 103, description, 640, 640, color);
    } else {
        value = critterGetStat(gDude, stat);
        characterEditorRenderBigNumber(58, dword_431D50[stat], 0, value, 0, characterEditorWindowHandle);
        blitBufferToBuffer(characterEditorWindowBackgroundBuf + off, 40, fontGetLineHeight(), 640, characterEditorWindowBuf + off, 640);

        value = critterGetStat(gDude, stat);
        if (value > 10) {
            value = 10;
        }

        description = statGetValueDescription(value);
        fontDrawText(characterEditorWindowBuf + off, description, 640, 640, color);
    }
}

// 0x434F18
void editorRenderGender()
{
    int gender;
    char* str;
    char text[32];
    int x, width;

    fontSetCurrent(103);

    gender = critterGetStat(gDude, STAT_GENDER);
    str = getmsg(&editorMessageList, &editorMessageListItem, 107 + gender);

    strcpy(text, str);

    width = stru_5701C0[EDITOR_GRAPHIC_SEX_ON].width;
    x = (width / 2) - (fontGetStringWidth(text) / 2);

    memcpy(dword_570418[11],
        dword_5704E0[EDITOR_GRAPHIC_SEX_ON],
        width * stru_5701C0[EDITOR_GRAPHIC_SEX_ON].height);
    memcpy(dword_570418[EDITOR_GRAPHIC_SEX_OFF],
        dword_5704E0[10],
        width * stru_5701C0[EDITOR_GRAPHIC_SEX_OFF].height);

    x += 6 * width;
    fontDrawText(dword_570418[EDITOR_GRAPHIC_SEX_ON] + x, text, width, width, byte_6A38D0[14723]);
    fontDrawText(dword_570418[EDITOR_GRAPHIC_SEX_OFF] + x, text, width, width, byte_6A38D0[18979]);
}

// 0x43501C
void editorRenderAge()
{
    int age;
    char* str;
    char text[32];
    int x, width;

    fontSetCurrent(103);

    age = critterGetStat(gDude, STAT_AGE);
    str = getmsg(&editorMessageList, &editorMessageListItem, 104);

    sprintf(text, "%s %d", str, age);

    width = stru_5701C0[EDITOR_GRAPHIC_AGE_ON].width;
    x = (width / 2) + 1 - (fontGetStringWidth(text) / 2);

    memcpy(dword_570418[EDITOR_GRAPHIC_AGE_ON],
        dword_5704E0[EDITOR_GRAPHIC_AGE_ON],
        width * stru_5701C0[EDITOR_GRAPHIC_AGE_ON].height);
    memcpy(dword_570418[EDITOR_GRAPHIC_AGE_OFF],
        dword_5704E0[EDITOR_GRAPHIC_AGE_OFF],
        width * stru_5701C0[EDITOR_GRAPHIC_AGE_ON].height);

    x += 6 * width;
    fontDrawText(dword_570418[EDITOR_GRAPHIC_AGE_ON] + x, text, width, width, byte_6A38D0[14723]);
    fontDrawText(dword_570418[EDITOR_GRAPHIC_AGE_OFF] + x, text, width, width, byte_6A38D0[18979]);
}

// 0x435118
void editorRenderName()
{
    char* str;
    char text[32];
    int x, width;
    char *pch, tmp;
    BOOL has_space;

    fontSetCurrent(103);

    str = critterGetName(gDude);
    strcpy(text, str);

    if (fontGetStringWidth(text) > 100) {
        pch = text;
        has_space = FALSE;
        while (*pch != '\0') {
            tmp = *pch;
            *pch = '\0';
            if (tmp == ' ') {
                has_space = TRUE;
            }

            if (fontGetStringWidth(text) > 100) {
                break;
            }

            *pch = tmp;
            pch++;
        }

        if (has_space) {
            pch = text + strlen(text);
            while (pch != text && *pch != ' ') {
                *pch = '\0';
                pch--;
            }
        }
    }

    width = stru_5701C0[EDITOR_GRAPHIC_NAME_ON].width;
    x = (width / 2) + 3 - (fontGetStringWidth(text) / 2);

    memcpy(dword_570418[EDITOR_GRAPHIC_NAME_ON],
        dword_5704E0[EDITOR_GRAPHIC_NAME_ON],
        stru_5701C0[EDITOR_GRAPHIC_NAME_ON].width * stru_5701C0[EDITOR_GRAPHIC_NAME_ON].height);
    memcpy(dword_570418[EDITOR_GRAPHIC_NAME_OFF],
        dword_5704E0[EDITOR_GRAPHIC_NAME_OFF],
        stru_5701C0[EDITOR_GRAPHIC_NAME_OFF].width * stru_5701C0[EDITOR_GRAPHIC_NAME_OFF].height);

    x += 6 * width;
    fontDrawText(dword_570418[EDITOR_GRAPHIC_NAME_ON] + x, text, width, width, byte_6A38D0[14723]);
    fontDrawText(dword_570418[EDITOR_GRAPHIC_NAME_OFF] + x, text, width, width, byte_6A38D0[18979]);
}

// 0x43527C
void editorRenderSecondaryStats()
{
    int conditions;
    int color;
    const char* messageListItemText;
    char t[420]; // TODO: Size is wrong.
    int y;

    conditions = gDude->data.critter.combat.results;

    fontSetCurrent(101);

    y = 46;

    blitBufferToBuffer(characterEditorWindowBackgroundBuf + 640 * y + 194, 118, 108, 640, characterEditorWindowBuf + 640 * y + 194, 640);

    // Hit Points
    if (characterEditorSelectedItem == EDITOR_HIT_POINTS) {
        color = byte_6A38D0[32747];
    } else {
        color = byte_6A38D0[992];
    }

    int currHp;
    int maxHp;
    if (gCharacterEditorIsCreationMode) {
        maxHp = critterGetStat(gDude, STAT_MAXIMUM_HIT_POINTS);
        currHp = maxHp;
    } else {
        maxHp = critterGetStat(gDude, STAT_MAXIMUM_HIT_POINTS);
        currHp = critterGetHitPoints(gDude);
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 300);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    sprintf(t, "%d/%d", currHp, maxHp);
    fontDrawText(characterEditorWindowBuf + 640 * y + 263, t, 640, 640, color);

    // Poisoned
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_POISONED) {
        if (critterGetPoison(gDude) != 0) {
            color = byte_6A38D0[15845];
        } else {
            color = byte_6A38D0[32747];
        }
    } else {
        if (critterGetPoison(gDude) != 0) {
            color = byte_6A38D0[992];
        } else {
            color = byte_6A38D0[1313];
        }
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 312);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    // Radiated
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_RADIATED) {
        if (critterGetRadiation(gDude) != 0) {
            color = byte_6A38D0[15845];
        } else {
            color = byte_6A38D0[32747];
        }
    } else {
        if (critterGetRadiation(gDude) != 0) {
            color = byte_6A38D0[992];
        } else {
            color = byte_6A38D0[1313];
        }
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 313);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    // Eye Damage
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_EYE_DAMAGE) {
        color = (conditions & DAM_BLIND) ? byte_6A38D0[15845] : byte_6A38D0[32747];
    } else {
        color = (conditions & DAM_BLIND) ? byte_6A38D0[992] : byte_6A38D0[1313];
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 314);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    // Crippled Right Arm
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_CRIPPLED_RIGHT_ARM) {
        color = (conditions & DAM_CRIP_ARM_RIGHT) ? byte_6A38D0[15845] : byte_6A38D0[32747];
    } else {
        color = (conditions & DAM_CRIP_ARM_RIGHT) ? byte_6A38D0[992] : byte_6A38D0[1313];
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 315);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    // Crippled Left Arm
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_CRIPPLED_LEFT_ARM) {
        color = (conditions & DAM_CRIP_ARM_LEFT) ? byte_6A38D0[15845] : byte_6A38D0[32747];
    } else {
        color = (conditions & DAM_CRIP_ARM_LEFT) ? byte_6A38D0[992] : byte_6A38D0[1313];
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 316);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    // Crippled Right Leg
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_CRIPPLED_RIGHT_LEG) {
        color = (conditions & DAM_CRIP_LEG_RIGHT) ? byte_6A38D0[15845] : byte_6A38D0[32747];
    } else {
        color = (conditions & DAM_CRIP_LEG_RIGHT) ? byte_6A38D0[992] : byte_6A38D0[1313];
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 317);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    // Crippled Left Leg
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_CRIPPLED_LEFT_LEG) {
        color = (conditions & DAM_CRIP_LEG_LEFT) ? byte_6A38D0[15845] : byte_6A38D0[32747];
    } else {
        color = (conditions & DAM_CRIP_LEG_LEFT) ? byte_6A38D0[992] : byte_6A38D0[1313];
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 318);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    y = 179;

    blitBufferToBuffer(characterEditorWindowBackgroundBuf + 640 * y + 194, 116, 130, 640, characterEditorWindowBuf + 640 * y + 194, 640);

    // Armor Class
    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_ARMOR_CLASS) {
        color = byte_6A38D0[32747];
    } else {
        color = byte_6A38D0[992];
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 302);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(gDude, STAT_ARMOR_CLASS), t, 10);
    fontDrawText(characterEditorWindowBuf + 640 * y + 288, t, 640, 640, color);

    // Action Points
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_ACTION_POINTS) {
        color = byte_6A38D0[32747];
    } else {
        color = byte_6A38D0[992];
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 301);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(gDude, STAT_MAXIMUM_ACTION_POINTS), t, 10);
    fontDrawText(characterEditorWindowBuf + 640 * y + 288, t, 640, 640, color);

    // Carry Weight
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_CARRY_WEIGHT) {
        color = byte_6A38D0[32747];
    } else {
        color = byte_6A38D0[992];
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 311);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(gDude, STAT_CARRY_WEIGHT), t, 10);
    fontDrawText(characterEditorWindowBuf + 640 * y + 288, t, 640, 640, color);

    // Melee Damage
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_MELEE_DAMAGE) {
        color = byte_6A38D0[32747];
    } else {
        color = byte_6A38D0[992];
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 304);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(gDude, STAT_MELEE_DAMAGE), t, 10);
    fontDrawText(characterEditorWindowBuf + 640 * y + 288, t, 640, 640, color);

    // Damage Resistance
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_DAMAGE_RESISTANCE) {
        color = byte_6A38D0[32747];
    } else {
        color = byte_6A38D0[992];
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 305);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    sprintf(t, "%d%%", critterGetStat(gDude, STAT_DAMAGE_RESISTANCE));
    fontDrawText(characterEditorWindowBuf + 640 * y + 288, t, 640, 640, color);

    // Poison Resistance
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_POISON_RESISTANCE) {
        color = byte_6A38D0[32747];
    } else {
        color = byte_6A38D0[992];
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 306);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    sprintf(t, "%d%%", critterGetStat(gDude, STAT_POISON_RESISTANCE));
    fontDrawText(characterEditorWindowBuf + 640 * y + 288, t, 640, 640, color);

    // Radiation Resistance
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_RADIATION_RESISTANCE) {
        color = byte_6A38D0[32747];
    } else {
        color = byte_6A38D0[992];
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 307);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    sprintf(t, "%d%%", critterGetStat(gDude, STAT_RADIATION_RESISTANCE));
    fontDrawText(characterEditorWindowBuf + 640 * y + 288, t, 640, 640, color);

    // Sequence
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_SEQUENCE) {
        color = byte_6A38D0[32747];
    } else {
        color = byte_6A38D0[992];
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 308);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(gDude, STAT_SEQUENCE), t, 10);
    fontDrawText(characterEditorWindowBuf + 640 * y + 288, t, 640, 640, color);

    // Healing Rate
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_HEALING_RATE) {
        color = byte_6A38D0[32747];
    } else {
        color = byte_6A38D0[992];
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 309);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(gDude, STAT_HEALING_RATE), t, 10);
    fontDrawText(characterEditorWindowBuf + 640 * y + 288, t, 640, 640, color);

    // Critical Chance
    y += fontGetLineHeight() + 3;

    if (characterEditorSelectedItem == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_CRITICAL_CHANCE) {
        color = byte_6A38D0[32747];
    } else {
        color = byte_6A38D0[992];
    }

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 309);
    sprintf(t, "%s", messageListItemText);
    fontDrawText(characterEditorWindowBuf + 640 * y + 194, t, 640, 640, color);

    sprintf(t, "%d%%", critterGetStat(gDude, STAT_CRITICAL_CHANCE));
    fontDrawText(characterEditorWindowBuf + 640 * y + 288, t, 640, 640, color);
}

// 0x436154
void editorRenderSkills(int a1)
{
    int selectedSkill = -1;
    const char* str;
    int i;
    int color;
    int y;
    int value;
    char valueString[12]; // TODO: Size might be wrong.

    if (characterEditorSelectedItem >= EDITOR_FIRST_SKILL && characterEditorSelectedItem < 79) {
        selectedSkill = characterEditorSelectedItem - EDITOR_FIRST_SKILL;
    }

    if (gCharacterEditorIsCreationMode == 0 && a1 == 0) {
        buttonDestroy(dword_5705E4);
        buttonDestroy(dword_5705E8);
        dword_5705E8 = -1;
        dword_5705E4 = -1;
    }

    blitBufferToBuffer(characterEditorWindowBackgroundBuf + 370, 270, 252, 640, characterEditorWindowBuf + 370, 640);

    fontSetCurrent(103);

    // SKILLS
    str = getmsg(&editorMessageList, &editorMessageListItem, 117);
    fontDrawText(characterEditorWindowBuf + 640 * 5 + 380, str, 640, 640, byte_6A38D0[18979]);

    if (!gCharacterEditorIsCreationMode) {
        // SKILL POINTS
        str = getmsg(&editorMessageList, &editorMessageListItem, 112);
        fontDrawText(characterEditorWindowBuf + 640 * 233 + 400, str, 640, 640, byte_6A38D0[18979]);

        value = pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS);
        characterEditorRenderBigNumber(522, 228, 0, value, 0, characterEditorWindowHandle);
    } else {
        // TAG SKILLS
        str = getmsg(&editorMessageList, &editorMessageListItem, 138);
        fontDrawText(characterEditorWindowBuf + 640 * 233 + 422, str, 640, 640, byte_6A38D0[18979]);

        // TODO: Check.
        if (a1 == 2 && !byte_570A2A) {
            characterEditorRenderBigNumber(522, 228, ANIMATE, dword_570A10, dword_5709C8, characterEditorWindowHandle);
        } else {
            characterEditorRenderBigNumber(522, 228, 0, dword_570A10, 0, characterEditorWindowHandle);
            byte_570A2A = 0;
        }
    }

    skillsSetTagged(dword_570A14, NUM_TAGGED_SKILLS);

    fontSetCurrent(101);

    y = 27;
    for (i = 0; i < SKILL_COUNT; i++) {
        if (i == selectedSkill) {
            if (i != dword_570A14[0] && i != dword_570A14[1] && i != dword_570A14[2] && i != dword_570A14[3]) {
                color = byte_6A38D0[32747];
            } else {
                color = byte_6A38D0[32767];
            }
        } else {
            if (i != dword_570A14[0] && i != dword_570A14[1] && i != dword_570A14[2] && i != dword_570A14[3]) {
                color = byte_6A38D0[992];
            } else {
                color = byte_6A38D0[21140];
            }
        }

        str = skillGetName(i);
        fontDrawText(characterEditorWindowBuf + 640 * y + 380, str, 640, 640, color);

        value = skillGetValue(gDude, i);
        sprintf(valueString, "%d%%", value);

        // TODO: Check text position.
        fontDrawText(characterEditorWindowBuf + 640 * y + 573, valueString, 640, 640, color);

        y += fontGetLineHeight() + 1;
    }

    if (!gCharacterEditorIsCreationMode) {
        y = dword_51852C * (fontGetLineHeight() + 1);
        dword_518534 = y + 27;

        blitBufferToBufferTrans(
            dword_5704E0[EDITOR_GRAPHIC_SLIDER],
            stru_5701C0[EDITOR_GRAPHIC_SLIDER].width,
            stru_5701C0[EDITOR_GRAPHIC_SLIDER].height,
            stru_5701C0[EDITOR_GRAPHIC_SLIDER].width,
            characterEditorWindowBuf + 640 * (y + 16) + 592,
            640);

        if (a1 == 0) {
            if (dword_5705E4 == -1) {
                dword_5705E4 = buttonCreate(
                    characterEditorWindowHandle,
                    614,
                    dword_518534 - 7,
                    stru_5701C0[EDITOR_GRAPHIC_SLIDER_PLUS_ON].width,
                    stru_5701C0[EDITOR_GRAPHIC_SLIDER_PLUS_ON].height,
                    -1,
                    522,
                    521,
                    522,
                    dword_5704E0[EDITOR_GRAPHIC_SLIDER_PLUS_OFF],
                    dword_5704E0[EDITOR_GRAPHIC_SLIDER_PLUS_ON],
                    NULL,
                    96);
                buttonSetCallbacks(dword_5705E4, _gsound_red_butt_press, NULL);
            }

            if (dword_5705E8 == -1) {
                dword_5705E8 = buttonCreate(
                    characterEditorWindowHandle,
                    614,
                    dword_518534 + 4 - 12 + stru_5701C0[EDITOR_GRAPHIC_SLIDER_MINUS_ON].height,
                    stru_5701C0[EDITOR_GRAPHIC_SLIDER_MINUS_ON].width,
                    stru_5701C0[EDITOR_GRAPHIC_SLIDER_MINUS_OFF].height,
                    -1,
                    524,
                    523,
                    524,
                    dword_5704E0[EDITOR_GRAPHIC_SLIDER_MINUS_OFF],
                    dword_5704E0[EDITOR_GRAPHIC_SLIDER_MINUS_ON],
                    NULL,
                    96);
                buttonSetCallbacks(dword_5705E8, _gsound_red_butt_press, NULL);
            }
        }
    }
}

// 0x4365AC
void editorRenderDetails()
{
    int graphicId;
    char* title;
    char* description;

    if (characterEditorSelectedItem < 0 || characterEditorSelectedItem >= 98) {
        return;
    }

    blitBufferToBuffer(characterEditorWindowBackgroundBuf + (640 * 267) + 345, 277, 170, 640, characterEditorWindowBuf + (267 * 640) + 345, 640);

    if (characterEditorSelectedItem >= 0 && characterEditorSelectedItem < 7) {
        description = statGetDescription(characterEditorSelectedItem);
        title = statGetName(characterEditorSelectedItem);
        graphicId = statGetFrmId(characterEditorSelectedItem);
        _DrawCard(graphicId, title, NULL, description);
    } else if (characterEditorSelectedItem >= 7 && characterEditorSelectedItem < 10) {
        if (gCharacterEditorIsCreationMode) {
            switch (characterEditorSelectedItem) {
            case 7:
                // Character Points
                description = getmsg(&editorMessageList, &editorMessageListItem, 121);
                title = getmsg(&editorMessageList, &editorMessageListItem, 120);
                _DrawCard(7, title, NULL, description);
                break;
            }
        } else {
            switch (characterEditorSelectedItem) {
            case 7:
                description = pcStatGetDescription(PC_STAT_LEVEL);
                title = pcStatGetName(PC_STAT_LEVEL);
                _DrawCard(7, title, NULL, description);
                break;
            case 8:
                description = pcStatGetDescription(PC_STAT_EXPERIENCE);
                title = pcStatGetName(PC_STAT_EXPERIENCE);
                _DrawCard(8, title, NULL, description);
                break;
            case 9:
                // Next Level
                description = getmsg(&editorMessageList, &editorMessageListItem, 123);
                title = getmsg(&editorMessageList, &editorMessageListItem, 122);
                _DrawCard(9, title, NULL, description);
                break;
            }
        }
    } else if ((characterEditorSelectedItem >= 10 && characterEditorSelectedItem < 43) || (characterEditorSelectedItem >= 82 && characterEditorSelectedItem < 98)) {
        _DrawCard(dword_5705B0, off_5705B8, off_5705BC, off_5705CC);
    } else if (characterEditorSelectedItem >= 43 && characterEditorSelectedItem < 51) {
        switch (characterEditorSelectedItem) {
        case EDITOR_HIT_POINTS:
            description = statGetDescription(STAT_MAXIMUM_HIT_POINTS);
            title = getmsg(&editorMessageList, &editorMessageListItem, 300);
            graphicId = statGetFrmId(STAT_MAXIMUM_HIT_POINTS);
            _DrawCard(graphicId, title, NULL, description);
            break;
        case EDITOR_POISONED:
            description = getmsg(&editorMessageList, &editorMessageListItem, 400);
            title = getmsg(&editorMessageList, &editorMessageListItem, 312);
            _DrawCard(11, title, NULL, description);
            break;
        case EDITOR_RADIATED:
            description = getmsg(&editorMessageList, &editorMessageListItem, 401);
            title = getmsg(&editorMessageList, &editorMessageListItem, 313);
            _DrawCard(12, title, NULL, description);
            break;
        case EDITOR_EYE_DAMAGE:
            description = getmsg(&editorMessageList, &editorMessageListItem, 402);
            title = getmsg(&editorMessageList, &editorMessageListItem, 314);
            _DrawCard(13, title, NULL, description);
            break;
        case EDITOR_CRIPPLED_RIGHT_ARM:
            description = getmsg(&editorMessageList, &editorMessageListItem, 403);
            title = getmsg(&editorMessageList, &editorMessageListItem, 315);
            _DrawCard(14, title, NULL, description);
            break;
        case EDITOR_CRIPPLED_LEFT_ARM:
            description = getmsg(&editorMessageList, &editorMessageListItem, 404);
            title = getmsg(&editorMessageList, &editorMessageListItem, 316);
            _DrawCard(15, title, NULL, description);
            break;
        case EDITOR_CRIPPLED_RIGHT_LEG:
            description = getmsg(&editorMessageList, &editorMessageListItem, 405);
            title = getmsg(&editorMessageList, &editorMessageListItem, 317);
            _DrawCard(16, title, NULL, description);
            break;
        case EDITOR_CRIPPLED_LEFT_LEG:
            description = getmsg(&editorMessageList, &editorMessageListItem, 406);
            title = getmsg(&editorMessageList, &editorMessageListItem, 318);
            _DrawCard(17, title, NULL, description);
            break;
        }
    } else if (characterEditorSelectedItem >= EDITOR_FIRST_DERIVED_STAT && characterEditorSelectedItem < 61) {
        int derivedStatIndex = characterEditorSelectedItem - 51;
        int stat = word_431D6C[derivedStatIndex];
        description = statGetDescription(stat);
        title = statGetName(stat);
        graphicId = word_431D3A[derivedStatIndex];
        _DrawCard(graphicId, title, NULL, description);
    } else if (characterEditorSelectedItem >= EDITOR_FIRST_SKILL && characterEditorSelectedItem < 79) {
        int skill = characterEditorSelectedItem - 61;
        const char* attributesDescription = skillGetAttributes(skill);

        char formatted[150]; // TODO: Size is probably wrong.
        const char* base = getmsg(&editorMessageList, &editorMessageListItem, 137);
        int defaultValue = skillGetDefaultValue(skill);
        sprintf(formatted, "%s %d%% %s", base, defaultValue, attributesDescription);

        graphicId = skillGetFrmId(skill);
        title = skillGetName(skill);
        description = skillGetDescription(skill);
        _DrawCard(graphicId, title, formatted, description);
    } else if (characterEditorSelectedItem >= 79 && characterEditorSelectedItem < 82) {
        switch (characterEditorSelectedItem) {
        case EDITOR_TAG_SKILL:
            if (gCharacterEditorIsCreationMode) {
                // Tag Skill
                description = getmsg(&editorMessageList, &editorMessageListItem, 145);
                title = getmsg(&editorMessageList, &editorMessageListItem, 144);
                _DrawCard(27, title, NULL, description);
            } else {
                // Skill Points
                description = getmsg(&editorMessageList, &editorMessageListItem, 131);
                title = getmsg(&editorMessageList, &editorMessageListItem, 130);
                _DrawCard(27, title, NULL, description);
            }
            break;
        case EDITOR_SKILLS:
            // Skills
            description = getmsg(&editorMessageList, &editorMessageListItem, 151);
            title = getmsg(&editorMessageList, &editorMessageListItem, 150);
            _DrawCard(27, title, NULL, description);
            break;
        case EDITOR_OPTIONAL_TRAITS:
            // Optional Traits
            description = getmsg(&editorMessageList, &editorMessageListItem, 147);
            title = getmsg(&editorMessageList, &editorMessageListItem, 146);
            _DrawCard(27, title, NULL, description);
            break;
        }
    }
}

// 0x436C4C
int characterEditorEditName()
{
    char* text;

    int windowWidth = stru_5701C0[EDITOR_GRAPHIC_CHARWIN].width;
    int windowHeight = stru_5701C0[EDITOR_GRAPHIC_CHARWIN].height;

    int win = windowCreate(17, 0, windowWidth, windowHeight, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);
    if (win == -1) {
        return -1;
    }

    unsigned char* windowBuf = windowGetBuffer(win);

    // Copy background
    memcpy(windowBuf, dword_5704E0[EDITOR_GRAPHIC_CHARWIN], windowWidth * windowHeight);

    blitBufferToBufferTrans(
        dword_5704E0[EDITOR_GRAPHIC_NAME_BOX],
        stru_5701C0[EDITOR_GRAPHIC_NAME_BOX].width,
        stru_5701C0[EDITOR_GRAPHIC_NAME_BOX].height,
        stru_5701C0[EDITOR_GRAPHIC_NAME_BOX].width,
        windowBuf + windowWidth * 13 + 13,
        windowWidth);
    blitBufferToBufferTrans(dword_5704E0[EDITOR_GRAPHIC_DONE_BOX],
        stru_5701C0[EDITOR_GRAPHIC_DONE_BOX].width,
        stru_5701C0[EDITOR_GRAPHIC_DONE_BOX].height,
        stru_5701C0[EDITOR_GRAPHIC_DONE_BOX].width,
        windowBuf + windowWidth * 40 + 13,
        windowWidth);

    fontSetCurrent(103);

    text = getmsg(&editorMessageList, &editorMessageListItem, 100);
    fontDrawText(windowBuf + windowWidth * 44 + 50, text, windowWidth, windowWidth, byte_6A38D0[18979]);

    int doneBtn = buttonCreate(win,
        26,
        44,
        stru_5701C0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        stru_5701C0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        500,
        dword_5704E0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        dword_5704E0[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (doneBtn != -1) {
        buttonSetCallbacks(doneBtn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    windowRefresh(win);

    fontSetCurrent(101);

    char name[64];
    strcpy(name, critterGetName(gDude));

    if (strcmp(name, "None") == 0) {
        name[0] = '\0';
    }

    // NOTE: I don't understand the nameCopy, not sure what it is used for. It's
    // definitely there, but I just don' get it.
    char nameCopy[64];
    strcpy(nameCopy, name);

    if (_get_input_str(win, 500, nameCopy, 11, 23, 19, byte_6A38D0[992], 100, 0) != -1) {
        if (nameCopy[0] != '\0') {
            dudeSetName(nameCopy);
            editorRenderName();
            windowDestroy(win);
            return 0;
        }
    }

    // NOTE: original code is a bit different, the following chunk of code written two times.

    fontSetCurrent(101);
    blitBufferToBuffer(dword_5704E0[EDITOR_GRAPHIC_NAME_BOX],
        stru_5701C0[EDITOR_GRAPHIC_NAME_BOX].width,
        stru_5701C0[EDITOR_GRAPHIC_NAME_BOX].height,
        stru_5701C0[EDITOR_GRAPHIC_NAME_BOX].width,
        windowBuf + stru_5701C0[EDITOR_GRAPHIC_CHARWIN].width * 13 + 13,
        stru_5701C0[EDITOR_GRAPHIC_CHARWIN].width);

    _PrintName(windowBuf, stru_5701C0[EDITOR_GRAPHIC_CHARWIN].width);

    strcpy(nameCopy, name);

    windowDestroy(win);

    return 0;
}

//
void _PrintName(unsigned char* buf, int a2)
{
    char str[64];
    char* v4;

    memcpy(str, byte_431D93, 64);

    fontSetCurrent(101);

    v4 = critterGetName(gDude);

    // TODO: Check.
    strcpy(str, v4);

    fontDrawText(buf + 19 * a2 + 21, str, a2, a2, byte_6A38D0[992]);
}

// 0x436FEC
int characterEditorRunEditAgeDialog()
{
    int win;
    unsigned char* windowBuf;
    int windowWidth;
    int windowHeight;
    const char* messageListItemText;
    int previousAge;
    int age;
    int doneBtn;
    int prevBtn;
    int nextBtn;
    int keyCode;
    int change;
    int flags;

    int savedAge = critterGetStat(gDude, STAT_AGE);

    windowWidth = stru_5701C0[EDITOR_GRAPHIC_CHARWIN].width;
    windowHeight = stru_5701C0[EDITOR_GRAPHIC_CHARWIN].height;

    win = windowCreate(stru_5701C0[EDITOR_GRAPHIC_NAME_ON].width + 9, 0, windowWidth, windowHeight, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);
    if (win == -1) {
        return -1;
    }

    windowBuf = windowGetBuffer(win);

    memcpy(windowBuf, dword_5704E0[EDITOR_GRAPHIC_CHARWIN], windowWidth * windowHeight);

    blitBufferToBufferTrans(
        dword_5704E0[EDITOR_GRAPHIC_AGE_BOX],
        stru_5701C0[EDITOR_GRAPHIC_AGE_BOX].width,
        stru_5701C0[EDITOR_GRAPHIC_AGE_BOX].height,
        stru_5701C0[EDITOR_GRAPHIC_AGE_BOX].width,
        windowBuf + windowWidth * 7 + 8,
        windowWidth);
    blitBufferToBufferTrans(
        dword_5704E0[EDITOR_GRAPHIC_DONE_BOX],
        stru_5701C0[EDITOR_GRAPHIC_DONE_BOX].width,
        stru_5701C0[EDITOR_GRAPHIC_DONE_BOX].height,
        stru_5701C0[EDITOR_GRAPHIC_DONE_BOX].width,
        windowBuf + windowWidth * 40 + 13,
        stru_5701C0[EDITOR_GRAPHIC_CHARWIN].width);

    fontSetCurrent(103);

    messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 100);
    fontDrawText(windowBuf + windowWidth * 44 + 50, messageListItemText, windowWidth, windowWidth, byte_6A38D0[18979]);

    age = critterGetStat(gDude, STAT_AGE);
    characterEditorRenderBigNumber(55, 10, 0, age, 0, win);

    doneBtn = buttonCreate(win,
        26,
        44,
        stru_5701C0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        stru_5701C0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        500,
        dword_5704E0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        dword_5704E0[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (doneBtn != -1) {
        buttonSetCallbacks(doneBtn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    nextBtn = buttonCreate(win,
        105,
        13,
        stru_5701C0[EDITOR_GRAPHIC_LEFT_ARROW_DOWN].width,
        stru_5701C0[EDITOR_GRAPHIC_LEFT_ARROW_DOWN].height,
        -1,
        503,
        501,
        503,
        dword_5704E0[EDITOR_GRAPHIC_RIGHT_ARROW_UP],
        dword_5704E0[EDITOR_GRAPHIC_RIGHT_ARROW_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (nextBtn != -1) {
        buttonSetCallbacks(nextBtn, _gsound_med_butt_press, NULL);
    }

    prevBtn = buttonCreate(win,
        19,
        13,
        stru_5701C0[EDITOR_GRAPHIC_RIGHT_ARROW_DOWN].width,
        stru_5701C0[EDITOR_GRAPHIC_RIGHT_ARROW_DOWN].height,
        -1,
        504,
        502,
        504,
        dword_5704E0[EDITOR_GRAPHIC_LEFT_ARROW_UP],
        dword_5704E0[EDITOR_GRAPHIC_LEFT_ARROW_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (prevBtn != -1) {
        buttonSetCallbacks(prevBtn, _gsound_med_butt_press, NULL);
    }

    while (true) {
        dword_5709C4 = _get_time();
        change = 0;
        flags = 0;
        int v32 = 0;

        keyCode = _get_input();

        if (keyCode == KEY_RETURN || keyCode == 500) {
            if (keyCode != 500) {
                soundPlayFile("ib1p1xx1");
            }

            windowDestroy(win);
            return 0;
        } else if (keyCode == KEY_ESCAPE || dword_5186CC != 0) {
            break;
        } else if (keyCode == 501) {
            age = critterGetStat(gDude, STAT_AGE);
            if (age < 35) {
                change = 1;
            }
        } else if (keyCode == 502) {
            age = critterGetStat(gDude, STAT_AGE);
            if (age > 16) {
                change = -1;
            }
        } else if (keyCode == KEY_PLUS || keyCode == KEY_UPPERCASE_N || keyCode == KEY_ARROW_UP) {
            previousAge = critterGetStat(gDude, STAT_AGE);
            if (previousAge < 35) {
                flags = ANIMATE;
                if (critterIncBaseStat(gDude, STAT_AGE) != 0) {
                    flags = 0;
                }
                age = critterGetStat(gDude, STAT_AGE);
                characterEditorRenderBigNumber(55, 10, flags, age, previousAge, win);
            }
        } else if (keyCode == KEY_MINUS || keyCode == KEY_UPPERCASE_J || keyCode == KEY_ARROW_DOWN) {
            previousAge = critterGetStat(gDude, STAT_AGE);
            if (previousAge > 16) {
                flags = ANIMATE;
                if (critterDecBaseStat(gDude, STAT_AGE) != 0) {
                    flags = 0;
                }
                age = critterGetStat(gDude, STAT_AGE);

                characterEditorRenderBigNumber(55, 10, flags, age, previousAge, win);
            }
        }

        if (flags == ANIMATE) {
            editorRenderAge();
            editorRenderPrimaryStat(RENDER_ALL_STATS, 0, 0);
            editorRenderSecondaryStats();
            windowRefresh(characterEditorWindowHandle);
            windowRefresh(win);
        }

        if (change != 0) {
            int v33 = 0;

            dword_5709C0 = 4;

            while (true) {
                dword_5709C4 = _get_time();

                v33++;

                if ((!v32 && v33 == 1) || (v32 && v33 > dbl_50170B)) {
                    v32 = true;

                    if (v33 > dbl_50170B) {
                        dword_5709C0++;
                        if (dword_5709C0 > 24) {
                            dword_5709C0 = 24;
                        }
                    }

                    flags = ANIMATE;
                    previousAge = critterGetStat(gDude, STAT_AGE);

                    if (change == 1) {
                        if (previousAge < 35) {
                            if (critterIncBaseStat(gDude, STAT_AGE) != 0) {
                                flags = 0;
                            }
                        }
                    } else {
                        if (previousAge >= 16) {
                            if (critterDecBaseStat(gDude, STAT_AGE) != 0) {
                                flags = 0;
                            }
                        }
                    }

                    age = critterGetStat(gDude, STAT_AGE);
                    characterEditorRenderBigNumber(55, 10, flags, age, previousAge, win);
                    if (flags == ANIMATE) {
                        editorRenderAge();
                        editorRenderPrimaryStat(RENDER_ALL_STATS, 0, 0);
                        editorRenderSecondaryStats();
                        windowRefresh(characterEditorWindowHandle);
                        windowRefresh(win);
                    }
                }

                if (v33 > dbl_50170B) {
                    while (getTicksSince(dword_5709C4) < 1000 / dword_5709C0)
                        ;
                } else {
                    while (getTicksSince(dword_5709C4) < 1000 / 24)
                        ;
                }

                keyCode = _get_input();
                if (keyCode == 503 || keyCode == 504 || dword_5186CC != 0) {
                    break;
                }
            }
        } else {
            windowRefresh(win);

            while (getTicksSince(dword_5709C4) < 1000 / 24)
                ;
        }
    }

    critterSetBaseStat(gDude, STAT_AGE, savedAge);
    editorRenderAge();
    editorRenderPrimaryStat(RENDER_ALL_STATS, 0, 0);
    editorRenderSecondaryStats();
    windowRefresh(characterEditorWindowHandle);
    windowRefresh(win);
    windowDestroy(win);
    return 0;
}

// 0x437664
void characterEditorEditGender()
{
    char* text;

    int windowWidth = stru_5701C0[EDITOR_GRAPHIC_CHARWIN].width;
    int windowHeight = stru_5701C0[EDITOR_GRAPHIC_CHARWIN].height;

    int x = 9;
    x += stru_5701C0[EDITOR_GRAPHIC_NAME_ON].width;
    x += stru_5701C0[EDITOR_GRAPHIC_AGE_ON].width;
    int win = windowCreate(x, 0, windowWidth, windowHeight, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);

    if (win == -1) {
        return;
    }

    unsigned char* windowBuf = windowGetBuffer(win);

    // Copy background
    memcpy(windowBuf, dword_5704E0[EDITOR_GRAPHIC_CHARWIN], windowWidth * windowHeight);

    blitBufferToBufferTrans(dword_5704E0[EDITOR_GRAPHIC_DONE_BOX],
        stru_5701C0[EDITOR_GRAPHIC_DONE_BOX].width,
        stru_5701C0[EDITOR_GRAPHIC_DONE_BOX].height,
        stru_5701C0[EDITOR_GRAPHIC_DONE_BOX].width,
        windowBuf + windowWidth * 44 + 15,
        windowWidth);

    fontSetCurrent(103);

    text = getmsg(&editorMessageList, &editorMessageListItem, 100);
    fontDrawText(windowBuf + windowWidth * 48 + 52, text, windowWidth, windowWidth, byte_6A38D0[18979]);

    int doneBtn = buttonCreate(win,
        28,
        48,
        stru_5701C0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        stru_5701C0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        500,
        dword_5704E0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        dword_5704E0[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (doneBtn != -1) {
        buttonSetCallbacks(doneBtn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    int btns[2];
    btns[0] = buttonCreate(win,
        22,
        2,
        stru_5701C0[EDITOR_GRAPHIC_MALE_ON].width,
        stru_5701C0[EDITOR_GRAPHIC_MALE_ON].height,
        -1,
        -1,
        -1,
        500,
        dword_5704E0[EDITOR_GRAPHIC_MALE_OFF],
        dword_5704E0[EDITOR_GRAPHIC_MALE_ON],
        NULL,
        BUTTON_FLAG_TRANSPARENT | BUTTON_FLAG_0x04 | BUTTON_FLAG_0x02 | BUTTON_FLAG_0x01);
    if (btns[0] != -1) {
        buttonSetCallbacks(doneBtn, _gsound_red_butt_press, NULL);
    }

    btns[1] = buttonCreate(win,
        71,
        3,
        stru_5701C0[EDITOR_GRAPHIC_FEMALE_ON].width,
        stru_5701C0[EDITOR_GRAPHIC_FEMALE_ON].height,
        -1,
        -1,
        -1,
        500,
        dword_5704E0[EDITOR_GRAPHIC_FEMALE_OFF],
        dword_5704E0[EDITOR_GRAPHIC_FEMALE_ON],
        NULL,
        BUTTON_FLAG_TRANSPARENT | BUTTON_FLAG_0x04 | BUTTON_FLAG_0x02 | BUTTON_FLAG_0x01);
    if (btns[1] != -1) {
        _win_group_radio_buttons(2, btns);
        buttonSetCallbacks(doneBtn, _gsound_red_butt_press, NULL);
    }

    int savedGender = critterGetStat(gDude, STAT_GENDER);
    _win_set_button_rest_state(btns[savedGender], 1, 0);

    while (true) {
        dword_5709C4 = _get_time();

        int eventCode = _get_input();

        if (eventCode == KEY_RETURN || eventCode == 500) {
            if (eventCode == KEY_RETURN) {
                soundPlayFile("ib1p1xx1");
            }
            break;
        }

        if (eventCode == KEY_ESCAPE || dword_5186CC != 0) {
            critterSetBaseStat(gDude, STAT_GENDER, savedGender);
            editorRenderPrimaryStat(RENDER_ALL_STATS, 0, 0);
            editorRenderSecondaryStats();
            windowRefresh(characterEditorWindowHandle);
            break;
        }

        switch (eventCode) {
        case KEY_ARROW_LEFT:
        case KEY_ARROW_RIGHT:
            _win_set_button_rest_state(btns[0], 1 - _win_button_down(btns[0]), 1);
            _win_set_button_rest_state(btns[1], _win_button_down(btns[0]), 1);
            break;
        case 501:
        case 502:
            // TODO: Original code is slightly different.
            critterSetBaseStat(gDude, STAT_GENDER, eventCode - 501);
            editorRenderPrimaryStat(RENDER_ALL_STATS, 0, 0);
            editorRenderSecondaryStats();
            break;
        }

        windowRefresh(win);

        while (getTicksSince(dword_5709C4) < 41)
            ;
    }

    editorRenderGender();
    windowDestroy(win);
}

// 0x4379BC
void characterEditorHandleIncDecPrimaryStat(int eventCode)
{
    dword_5709C0 = 4;

    int savedRemainingCharacterPoints = characterEditorRemainingCharacterPoints;

    if (!gCharacterEditorIsCreationMode) {
        return;
    }

    int incrementingStat = eventCode - 503;
    int decrementingStat = eventCode - 510;

    int v11 = 0;

    bool cont = true;
    do {
        dword_5709C4 = _get_time();
        if (v11 <= 19.2) {
            v11++;
        }

        if (v11 == 1 || v11 > 19.2) {
            if (v11 > 19.2) {
                dword_5709C0++;
                if (dword_5709C0 > 24) {
                    dword_5709C0 = 24;
                }
            }

            if (eventCode >= 510) {
                int previousValue = critterGetStat(gDude, decrementingStat);
                if (critterDecBaseStat(gDude, decrementingStat) == 0) {
                    characterEditorRemainingCharacterPoints++;
                } else {
                    cont = false;
                }

                editorRenderPrimaryStat(decrementingStat, cont ? ANIMATE : 0, previousValue);
                characterEditorRenderBigNumber(126, 282, cont ? ANIMATE : 0, characterEditorRemainingCharacterPoints, savedRemainingCharacterPoints, characterEditorWindowHandle);
                critterUpdateDerivedStats(gDude);
                editorRenderSecondaryStats();
                editorRenderSkills(0);
                characterEditorSelectedItem = decrementingStat;
            } else {
                int previousValue = critterGetBaseStatWithTraitModifier(gDude, incrementingStat);
                previousValue += critterGetBonusStat(gDude, incrementingStat);
                if (characterEditorRemainingCharacterPoints > 0 && previousValue < 10 && critterIncBaseStat(gDude, incrementingStat) == 0) {
                    characterEditorRemainingCharacterPoints--;
                } else {
                    cont = false;
                }

                editorRenderPrimaryStat(incrementingStat, cont ? ANIMATE : 0, previousValue);
                characterEditorRenderBigNumber(126, 282, cont ? ANIMATE : 0, characterEditorRemainingCharacterPoints, savedRemainingCharacterPoints, characterEditorWindowHandle);
                critterUpdateDerivedStats(gDude);
                editorRenderSecondaryStats();
                editorRenderSkills(0);
                characterEditorSelectedItem = incrementingStat;
            }

            windowRefresh(characterEditorWindowHandle);
        }

        if (v11 >= 19.2) {
            unsigned int delay = 1000 / dword_5709C0;
            while (getTicksSince(dword_5709C4) < delay) {
            }
        } else {
            while (getTicksSince(dword_5709C4) < 1000 / 24) {
            }
        }
    } while (_get_input() != 518 && cont);

    editorRenderDetails();
}

// handle options dialog
int _OptionWindow()
{
    int width = stru_5701C0[43].width;
    int height = stru_5701C0[43].height;

    if (gCharacterEditorIsCreationMode) {
        int win = windowCreate(238, 90, stru_5701C0[41].width, stru_5701C0[41].height, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);
        if (win == -1) {
            return -1;
        }

        unsigned char* windowBuffer = windowGetBuffer(win);
        memcpy(windowBuffer, dword_5704E0[41], stru_5701C0[41].width * stru_5701C0[41].height);

        fontSetCurrent(103);

        int err = 0;
        unsigned char* down[5];
        unsigned char* up[5];
        int size = width * height;
        int y = 17;
        int index;

        for (index = 0; index < 5; index++) {
            if (err != 0) {
                break;
            }

            do {
                down[index] = internal_malloc(size);
                if (down[index] == NULL) {
                    err = 1;
                    break;
                }

                up[index] = internal_malloc(size);
                if (up[index] == NULL) {
                    err = 2;
                    break;
                }

                memcpy(down[index], dword_5704E0[43], size);
                memcpy(up[index], dword_5704E0[42], size);

                const char* msg = getmsg(&editorMessageList, &editorMessageListItem, 600 + index);

                char dest[512];
                strcpy(dest, msg);

                int length = fontGetStringWidth(dest);
                int v60 = width / 2 - length / 2;
                fontDrawText(up[index] + v60, dest, width, width, byte_6A38D0[18979]);
                fontDrawText(down[index] + v60, dest, width, width, byte_6A38D0[14723]);

                int btn = buttonCreate(win, 13, y, width, height, -1, -1, -1, 500 + index, up[index], down[index], NULL, BUTTON_FLAG_TRANSPARENT);
                if (btn != -1) {
                    buttonSetCallbacks(btn, _gsound_lrg_butt_press, NULL);
                }
            } while (0);

            y += height + 3;
        }

        if (err != 0) {
            if (err == 2) {
                internal_free(down[index]);
            }

            while (--index >= 0) {
                internal_free(up[index]);
                internal_free(down[index]);
            }

            return -1;
        }

        fontSetCurrent(101);

        int rc = 0;
        while (rc != 0) {
            int keyCode = _get_input();

            if (dword_5186CC != 0) {
                rc = 2;
            } else if (keyCode == 504) {
                rc = 2;
            } else if (keyCode == KEY_RETURN || keyCode == KEY_UPPERCASE_D || keyCode == KEY_LOWERCASE_D) {
                // DONE
                rc = 2;
                soundPlayFile("ib1p1xx1");
            } else if (keyCode == VK_ESCAPE) {
                rc = 2;
            } else if (keyCode == 503 || keyCode == KEY_UPPERCASE_E || keyCode == KEY_LOWERCASE_E) {
                // ERASE
                char line1[512];
                strcpy(line1, getmsg(&editorMessageList, &editorMessageListItem, 605));

                char line2[512];
                strcpy(line2, getmsg(&editorMessageList, &editorMessageListItem, 606));

                const char* lines[] = { line1, line2 };
                if (showDialogBox(NULL, lines, 2, 169, 126, byte_6A38D0[992], NULL, byte_6A38D0[992], 0x10) != 0) {
                    _ResetPlayer();
                    skillsGetTagged(dword_570A14, NUM_TAGGED_SKILLS);

                    int v224 = 3;
                    int v225 = 0;
                    do {
                        if (dword_570A14[v224] != -1) {
                            break;
                        }
                        --v224;
                        ++v225;
                    } while (v224 > -1);

                    if (gCharacterEditorIsCreationMode) {
                        v225--;
                    }

                    dword_570A10 = v225;

                    traitsGetSelected(&dword_570A04[0], &dword_570A04[1]);

                    int v226 = 1;
                    int v227 = 0;
                    do {
                        if (dword_570A04[v226] != -1) {
                            break;
                        }
                        --v226;
                        ++v227;
                    } while (v226 > -1);

                    dword_5709FC = v227;
                    critterUpdateDerivedStats(gDude);
                    _ResetScreen();
                }
            } else if (keyCode == 502 || keyCode == KEY_UPPERCASE_P || keyCode == KEY_LOWERCASE_P) {
                // PRINT TO FILE
                char dest[512];
                dest[0] = '\0';

                strcat(dest, "*.TXT");

                char** fileList;
                int fileListLength = fileNameListInit(dest, &fileList, 0, 0);
                if (fileListLength != -1) {
                    char v236[512];

                    // PRINT
                    strcpy(v236, getmsg(&editorMessageList, &editorMessageListItem, 616));

                    // PRINT TO FILE
                    strcpy(dest, getmsg(&editorMessageList, &editorMessageListItem, 602));

                    if (_save_file_dialog(dest, fileList, v236, fileListLength, 168, 80, 0) == 0) {
                        strcat(v236, ".TXT");

                        dest[0] = '\0';

                        if (!characterFileExists(dest)) {
                            // already exists
                            sprintf(dest,
                                "%s %s",
                                strupr(v236),
                                getmsg(&editorMessageList, &editorMessageListItem, 609));

                            char v240[512];
                            strcpy(v240, getmsg(&editorMessageList, &editorMessageListItem, 610));

                            const char* lines[] = { v240 };
                            if (showDialogBox(dest, lines, 1, 169, 126, byte_6A38D0[32328], NULL, byte_6A38D0[32328], 0x10) != 0) {
                                rc = 1;
                            } else {
                                rc = 0;
                            }
                        } else {
                            rc = 1;
                        }

                        if (rc != 0) {
                            dest[0] = '\0';
                            strcat(dest, v236);

                            if (characterPrintToFile(dest) == 0) {
                                sprintf(dest,
                                    "%s%s",
                                    strupr(v236),
                                    getmsg(&editorMessageList, &editorMessageListItem, 607));
                                showDialogBox(dest, NULL, 0, 169, 126, byte_6A38D0[992], NULL, byte_6A38D0[992], 0);
                            } else {
                                soundPlayFile("iisxxxx1");

                                sprintf(dest,
                                    "%s%s%s",
                                    getmsg(&editorMessageList, &editorMessageListItem, 611),
                                    strupr(v236),
                                    "!");
                                showDialogBox(dest, NULL, 0, 169, 126, byte_6A38D0[32328], NULL, byte_6A38D0[992], 0x01);
                            }
                        }
                    }

                    fileNameListFree(&fileList, 0);
                } else {
                    soundPlayFile("iisxxxx1");

                    strcpy(dest, getmsg(&editorMessageList, &editorMessageListItem, 615));
                    showDialogBox(dest, NULL, 0, 169, 126, byte_6A38D0[32328], NULL, byte_6A38D0[32328], 0);

                    rc = 0;
                }
            } else if (keyCode == 501 || keyCode == KEY_UPPERCASE_L || keyCode == KEY_LOWERCASE_L) {
                // LOAD
                char path[MAX_PATH];
                path[0] = '\0';
                strcat(path, "*.");
                strcat(path, "GCD");

                char** fileNames;
                int filesCount = fileNameListInit(path, &fileNames, 0, 0);
                if (filesCount != -1) {

                } else {
                    soundPlayFile("iisxxxx1");

                    // Error reading file list!
                    strcpy(path, getmsg(&editorMessageList, &editorMessageListItem, 615));
                    rc = 0;

                    showDialogBox(path, NULL, 0, 169, 126, byte_6A38D0[32328], NULL, byte_6A38D0[32328], 0);
                }
            } else if (keyCode == 500 || keyCode == KEY_UPPERCASE_S || keyCode == KEY_LOWERCASE_S) {
                // TODO: Incomplete.
            }

            windowRefresh(win);
        }

        windowDestroy(win);

        for (index = 0; index < 5; index++) {
            internal_free(up[index]);
            internal_free(down[index]);
        }

        return 0;
    }

    // Character Editor is not in creation mode - this button is only for
    // printing character details.

    char pattern[512];
    strcpy(pattern, "*.TXT");

    char** fileNames;
    int filesCount = fileNameListInit(pattern, &fileNames, 0, 0);
    if (filesCount == -1) {
        soundPlayFile("iisxxxx1");

        // Error reading file list!
        strcpy(pattern, getmsg(&editorMessageList, &editorMessageListItem, 615));
        showDialogBox(pattern, NULL, 0, 169, 126, byte_6A38D0[32328], NULL, byte_6A38D0[32328], 0);
        return 0;
    }

    // PRINT
    char fileName[512];
    strcpy(fileName, getmsg(&editorMessageList, &editorMessageListItem, 616));

    char title[512];
    strcpy(title, getmsg(&editorMessageList, &editorMessageListItem, 602));

    if (_save_file_dialog(title, fileNames, fileName, filesCount, 168, 80, 0) == 0) {
        strcat(fileName, ".TXT");

        title[0] = '\0';
        strcat(title, fileName);

        int v42 = 0;
        if (characterFileExists(title)) {
            sprintf(title,
                "%s %s",
                strupr(fileName),
                getmsg(&editorMessageList, &editorMessageListItem, 609));

            char line2[512];
            strcpy(line2, getmsg(&editorMessageList, &editorMessageListItem, 610));

            const char* lines[] = { line2 };
            v42 = showDialogBox(title, lines, 1, 169, 126, byte_6A38D0[32328], NULL, byte_6A38D0[32328], 0x10);
            if (v42) {
                v42 = 1;
            }
        } else {
            v42 = 1;
        }

        if (v42) {
            title[0] = '\0';
            strcpy(title, fileName);

            if (characterPrintToFile(title) != 0) {
                soundPlayFile("iisxxxx1");

                sprintf(title,
                    "%s%s%s",
                    getmsg(&editorMessageList, &editorMessageListItem, 611),
                    strupr(fileName),
                    "!");
                showDialogBox(title, NULL, 0, 169, 126, byte_6A38D0[32328], NULL, byte_6A38D0[32328], 1);
            }
        }
    }

    fileNameListFree(&fileNames, 0);

    return 0;
}

// 0x4390B4
bool characterFileExists(const char* fname)
{
    File* stream = fileOpen(fname, "rb");
    if (stream == NULL) {
        return false;
    }

    fileClose(stream);
    return true;
}

// 0x4390D0
int characterPrintToFile(const char* fileName)
{
    File* stream = fileOpen(fileName, "wt");
    if (stream == NULL) {
        return -1;
    }

    fileWriteString("\n", stream);
    fileWriteString("\n", stream);

    char title1[256];
    char title2[256];
    char title3[256];
    char padding[256];

    // FALLOUT
    strcpy(title1, getmsg(&editorMessageList, &editorMessageListItem, 620));

    // NOTE: Uninline.
    padding[0] = '\0';
    _AddSpaces(padding, (80 - strlen(title1)) / 2 - 2);

    strcat(padding, title1);
    strcat(padding, "\n");
    fileWriteString(padding, stream);

    // VAULT-13 PERSONNEL RECORD
    strcpy(title1, getmsg(&editorMessageList, &editorMessageListItem, 621));

    // NOTE: Uninline.
    padding[0] = '\0';
    _AddSpaces(padding, (80 - strlen(title1)) / 2 - 2);

    strcat(padding, title1);
    strcat(padding, "\n");
    fileWriteString(padding, stream);

    int month;
    int day;
    int year;
    gameTimeGetDate(&month, &day, &year);

    sprintf(title1, "%.2d %s %d  %.4d %s",
        day,
        getmsg(&editorMessageList, &editorMessageListItem, 500 + month - 1),
        year,
        gameTimeGetHour(),
        getmsg(&editorMessageList, &editorMessageListItem, 622));

    // NOTE: Uninline.
    padding[0] = '\0';
    _AddSpaces(padding, (80 - strlen(title1)) / 2 - 2);

    strcat(padding, title1);
    strcat(padding, "\n");
    fileWriteString(padding, stream);

    // Blank line
    fileWriteString("\n", stream);

    // Name
    sprintf(title1,
        "%s %s",
        getmsg(&editorMessageList, &editorMessageListItem, 642),
        critterGetName(gDude));

    int paddingLength = 27 - strlen(title1);
    if (paddingLength > 0) {
        // NOTE: Uninline.
        padding[0] = '\0';
        _AddSpaces(padding, paddingLength);

        strcat(title1, padding);
    }

    // Age
    sprintf(title2,
        "%s%s %d",
        title1,
        getmsg(&editorMessageList, &editorMessageListItem, 643),
        critterGetStat(gDude, STAT_AGE));

    // Gender
    sprintf(title3,
        "%s%s %s",
        title2,
        getmsg(&editorMessageList, &editorMessageListItem, 644),
        getmsg(&editorMessageList, &editorMessageListItem, 645 + critterGetStat(gDude, STAT_GENDER)));

    fileWriteString(title3, stream);
    fileWriteString("\n", stream);

    sprintf(title1,
        "%s %.2d %s %s ",
        getmsg(&editorMessageList, &editorMessageListItem, 647),
        pcGetStat(PC_STAT_LEVEL),
        getmsg(&editorMessageList, &editorMessageListItem, 648),
        _itostndn(pcGetStat(PC_STAT_EXPERIENCE), title3));

    paddingLength = 12 - strlen(title3);
    if (paddingLength > 0) {
        // NOTE: Uninline.
        padding[0] = '\0';
        _AddSpaces(padding, paddingLength);

        strcat(title1, padding);
    }

    sprintf(title2,
        "%s%s %s",
        title1,
        getmsg(&editorMessageList, &editorMessageListItem, 649),
        _itostndn(pcGetExperienceForNextLevel(), title3));
    fileWriteString(title2, stream);
    fileWriteString("\n", stream);
    fileWriteString("\n", stream);

    // Statistics
    sprintf(title1, "%s\n", getmsg(&editorMessageList, &editorMessageListItem, 623));

    // Strength / Hit Points / Sequence
    //
    // FIXME: There is bug - it shows strength instead of sequence.
    sprintf(title1,
        "%s %.2d %s %.3d/%.3d %s %.2d",
        getmsg(&editorMessageList, &editorMessageListItem, 624),
        critterGetStat(gDude, STAT_STRENGTH),
        getmsg(&editorMessageList, &editorMessageListItem, 625),
        critterGetHitPoints(gDude),
        critterGetStat(gDude, STAT_MAXIMUM_HIT_POINTS),
        getmsg(&editorMessageList, &editorMessageListItem, 626),
        critterGetStat(gDude, STAT_STRENGTH));
    fileWriteString(title1, stream);
    fileWriteString("\n", stream);

    // Perception / Armor Class / Healing Rate
    sprintf(title1,
        "%s %.2d %s %.3d %s %.2d",
        getmsg(&editorMessageList, &editorMessageListItem, 627),
        critterGetStat(gDude, STAT_PERCEPTION),
        getmsg(&editorMessageList, &editorMessageListItem, 628),
        critterGetStat(gDude, STAT_ARMOR_CLASS),
        getmsg(&editorMessageList, &editorMessageListItem, 629),
        critterGetStat(gDude, STAT_HEALING_RATE));
    fileWriteString(title1, stream);
    fileWriteString("\n", stream);

    // Endurance / Action Points / Critical Chance
    sprintf(title1,
        "%s %.2d %s %.2d %s %.3d%%",
        getmsg(&editorMessageList, &editorMessageListItem, 630),
        critterGetStat(gDude, STAT_ENDURANCE),
        getmsg(&editorMessageList, &editorMessageListItem, 631),
        critterGetStat(gDude, STAT_MAXIMUM_ACTION_POINTS),
        getmsg(&editorMessageList, &editorMessageListItem, 632),
        critterGetStat(gDude, STAT_CRITICAL_CHANCE));
    fileWriteString(title1, stream);
    fileWriteString("\n", stream);

    // Charisma / Melee Damage / Carry Weight
    sprintf(title1,
        "%s %.2d %s %.2d %s %.3d lbs.",
        getmsg(&editorMessageList, &editorMessageListItem, 633),
        critterGetStat(gDude, STAT_CHARISMA),
        getmsg(&editorMessageList, &editorMessageListItem, 634),
        critterGetStat(gDude, STAT_MELEE_DAMAGE),
        getmsg(&editorMessageList, &editorMessageListItem, 635),
        critterGetStat(gDude, STAT_CARRY_WEIGHT));
    fileWriteString(title1, stream);
    fileWriteString("\n", stream);

    // Intelligence / Damage Resistance
    sprintf(title1,
        "%s %.2d %s %.3d%%",
        getmsg(&editorMessageList, &editorMessageListItem, 636),
        critterGetStat(gDude, STAT_INTELLIGENCE),
        getmsg(&editorMessageList, &editorMessageListItem, 637),
        critterGetStat(gDude, STAT_DAMAGE_RESISTANCE));
    fileWriteString(title1, stream);
    fileWriteString("\n", stream);

    // Agility / Radiation Resistance
    sprintf(title1,
        "%s %.2d %s %.3d%%",
        getmsg(&editorMessageList, &editorMessageListItem, 638),
        critterGetStat(gDude, STAT_AGILITY),
        getmsg(&editorMessageList, &editorMessageListItem, 639),
        critterGetStat(gDude, STAT_RADIATION_RESISTANCE));
    fileWriteString(title1, stream);
    fileWriteString("\n", stream);

    // Luck / Poison Resistance
    sprintf(title1,
        "%s %.2d %s %.3d%%",
        getmsg(&editorMessageList, &editorMessageListItem, 640),
        critterGetStat(gDude, STAT_LUCK),
        getmsg(&editorMessageList, &editorMessageListItem, 641),
        critterGetStat(gDude, STAT_POISON_RESISTANCE));
    fileWriteString(title1, stream);
    fileWriteString("\n", stream);

    fileWriteString("\n", stream);
    fileWriteString("\n", stream);

    if (dword_570A04[0] != -1) {
        // ::: Traits :::
        sprintf(title1, "%s\n", getmsg(&editorMessageList, &editorMessageListItem, 650));
        fileWriteString(title1, stream);

        // NOTE: The original code does not use loop, or it was optimized away.
        for (int index = 0; index < TRAITS_MAX_SELECTED_COUNT; index++) {
            if (dword_570A04[index] != -1) {
                sprintf(title1, "  %s", traitGetName(dword_570A04[index]));
                fileWriteString(title1, stream);
                fileWriteString("\n", stream);
            }
        }
    }

    int perk = 0;
    for (; perk < PERK_COUNT; perk++) {
        if (perkGetRank(gDude, perk) != 0) {
            break;
        }
    }

    if (perk < PERK_COUNT) {
        // ::: Perks :::
        sprintf(title1, "%s\n", getmsg(&editorMessageList, &editorMessageListItem, 651));
        fileWriteString(title1, stream);

        for (perk = 0; perk < PERK_COUNT; perk++) {
            int rank = perkGetRank(gDude, perk);
            if (rank != 0) {
                if (rank == 1) {
                    sprintf(title1, "  %s", perkGetName(perk));
                } else {
                    sprintf(title1, "  %s (%d)", perkGetName(perk), rank);
                }

                fileWriteString(title1, stream);
                fileWriteString("\n", stream);
            }
        }
    }

    fileWriteString("\n", stream);

    // ::: Karma :::
    sprintf(title1, "%s\n", getmsg(&editorMessageList, &editorMessageListItem, 652));
    fileWriteString(title1, stream);

    for (int index = 0; index < gKarmaEntriesLength; index++) {
        KarmaEntry* karmaEntry = &(gKarmaEntries[index]);
        if (karmaEntry->gvar == GVAR_PLAYER_REPUTATION) {
            int reputation = 0;
            for (; reputation < gGenericReputationEntriesLength; reputation++) {
                GenericReputationEntry* reputationDescription = &(gGenericReputationEntries[reputation]);
                if (gGameGlobalVars[GVAR_PLAYER_REPUTATION] >= reputationDescription->threshold) {
                    break;
                }
            }

            if (reputation < gGenericReputationEntriesLength) {
                GenericReputationEntry* reputationDescription = &(gGenericReputationEntries[reputation]);
                sprintf(title1,
                    "  %s: %s (%s)",
                    getmsg(&editorMessageList, &editorMessageListItem, 125),
                    itoa(gGameGlobalVars[GVAR_PLAYER_REPUTATION], title2, 10),
                    getmsg(&editorMessageList, &editorMessageListItem, reputationDescription->name));
                fileWriteString(title1, stream);
                fileWriteString("\n", stream);
            }
        } else {
            if (gGameGlobalVars[karmaEntry->gvar] != 0) {
                sprintf(title1, "  %s", getmsg(&editorMessageList, &editorMessageListItem, karmaEntry->name));
                fileWriteString(title1, stream);
                fileWriteString("\n", stream);
            }
        }
    }

    bool hasTownReputationHeading = false;
    for (int index = 0; index < TOWN_REPUTATION_COUNT; index++) {
        const TownReputationEntry* pair = &(gTownReputationEntries[index]);
        if (_wmAreaIsKnown(pair->city)) {
            if (!hasTownReputationHeading) {
                fileWriteString("\n", stream);

                // ::: Reputation :::
                sprintf(title1, "%s\n", getmsg(&editorMessageList, &editorMessageListItem, 657));
                fileWriteString(title1, stream);
                hasTownReputationHeading = true;
            }

            _wmGetAreaIdxName(pair->city, title2);

            int townReputation = gGameGlobalVars[pair->gvar];

            int townReputationMessageId;

            if (townReputation < -30) {
                townReputationMessageId = 2006; // Vilified
            } else if (townReputation < -15) {
                townReputationMessageId = 2005; // Hated
            } else if (townReputation < 0) {
                townReputationMessageId = 2004; // Antipathy
            } else if (townReputation == 0) {
                townReputationMessageId = 2003; // Neutral
            } else if (townReputation < 15) {
                townReputationMessageId = 2002; // Accepted
            } else if (townReputation < 30) {
                townReputationMessageId = 2001; // Liked
            } else {
                townReputationMessageId = 2000; // Idolized
            }

            sprintf(title1,
                "  %s: %s",
                title2,
                getmsg(&editorMessageList, &editorMessageListItem, townReputationMessageId));
            fileWriteString(title1, stream);
            fileWriteString("\n", stream);
        }
    }

    bool hasAddictionsHeading = false;
    for (int index = 0; index < ADDICTION_REPUTATION_COUNT; index++) {
        if (gGameGlobalVars[gAddictionReputationVars[index]] != 0) {
            if (!hasAddictionsHeading) {
                fileWriteString("\n", stream);

                // ::: Addictions :::
                sprintf(title1, "%s\n", getmsg(&editorMessageList, &editorMessageListItem, 656));
                fileWriteString(title1, stream);
                hasAddictionsHeading = true;
            }

            sprintf(title1,
                "  %s",
                getmsg(&editorMessageList, &editorMessageListItem, 1004 + index));
            fileWriteString(title1, stream);
            fileWriteString("\n", stream);
        }
    }

    fileWriteString("\n", stream);

    // ::: Skills ::: / ::: Kills :::
    sprintf(title1, "%s\n", getmsg(&editorMessageList, &editorMessageListItem, 653));
    fileWriteString(title1, stream);

    int killType = 0;
    for (int skill = 0; skill < SKILL_COUNT; skill++) {
        sprintf(title1, "%s ", skillGetName(skill));

        // NOTE: Uninline.
        _AddDots(title1 + strlen(title1), 16 - strlen(title1));

        bool hasKillType = false;

        for (; killType < KILL_TYPE_COUNT; killType++) {
            int killsCount = killsGetByType(killType);
            if (killsCount > 0) {
                sprintf(title2, "%s ", killTypeGetName(killType));

                // NOTE: Uninline.
                _AddDots(title2 + strlen(title2), 16 - strlen(title2));

                sprintf(title3,
                    "  %s %.3d%%        %s %.3d\n",
                    title1,
                    skillGetValue(gDude, skill),
                    title2,
                    killsCount);
                hasKillType = true;
                break;
            }
        }

        if (!hasKillType) {
            sprintf(title3,
                "  %s %.3d%%\n",
                title1,
                skillGetValue(gDude, skill));
        }
    }

    fileWriteString("\n", stream);
    fileWriteString("\n", stream);

    // ::: Inventory :::
    sprintf(title1, "%s\n", getmsg(&editorMessageList, &editorMessageListItem, 654));
    fileWriteString(title1, stream);

    Inventory* inventory = &(gDude->data.inventory);
    for (int index = 0; index < inventory->length; index += 3) {
        title1[0] = '\0';

        for (int column = 0; column < 3; column++) {
            int inventoryItemIndex = index + column;
            if (inventoryItemIndex >= inventory->length) {
                break;
            }

            InventoryItem* inventoryItem = &(inventory->items[inventoryItemIndex]);

            sprintf(title2,
                "  %sx %s",
                _itostndn(inventoryItem->quantity, title3),
                objectGetName(inventoryItem->item));

            int length = 25 - strlen(title2);
            if (length < 0) {
                length = 0;
            }

            _AddSpaces(title2, length);

            strcat(title1, title2);
        }

        strcat(title1, "\n");
        fileWriteString(title1, stream);
    }

    fileWriteString("\n", stream);

    // Total Weight:
    sprintf(title1,
        "%s %d lbs.",
        getmsg(&editorMessageList, &editorMessageListItem, 655),
        objectGetInventoryWeight(gDude));
    fileWriteString(title1, stream);

    fileWriteString("\n", stream);
    fileWriteString("\n", stream);
    fileWriteString("\n", stream);
    fileClose(stream);

    return 0;
}

char* _AddSpaces(char* string, int length)
{
    char* pch = string + strlen(string);

    for (int index = 0; index < length; index++) {
        *pch++ = ' ';
    }

    *pch = '\0';

    return string;
}

// NOTE: Inlined.
char* _AddDots(char* string, int length)
{
    char* pch = string + strlen(string);

    for (int index = 0; index < length; index++) {
        *pch++ = '.';
    }

    *pch = '\0';

    return string;
}

void _ResetScreen()
{
    characterEditorSelectedItem = 0;
    dword_51852C = 0;
    dword_518534 = 27;
    characterEditorWindowSelectedFolder = 0;

    if (gCharacterEditorIsCreationMode) {
        characterEditorRenderBigNumber(126, 282, 0, characterEditorRemainingCharacterPoints, 0, characterEditorWindowHandle);
    } else {
        editorRenderFolders();
        editorRenderPcStats();
    }

    editorRenderName();
    editorRenderAge();
    editorRenderGender();
    characterEditorWindowRenderTraits();
    editorRenderSkills(0);
    editorRenderPrimaryStat(7, 0, 0);
    editorRenderSecondaryStats();
    editorRenderDetails();
    windowRefresh(characterEditorWindowHandle);
}

//
void _RegInfoAreas()
{
    buttonCreate(characterEditorWindowHandle, 19, 38, 125, 227, -1, -1, 525, -1, NULL, NULL, NULL, 0);
    buttonCreate(characterEditorWindowHandle, 28, 280, 124, 32, -1, -1, 526, -1, NULL, NULL, NULL, 0);

    if (gCharacterEditorIsCreationMode) {
        buttonCreate(characterEditorWindowHandle, 52, 324, 169, 20, -1, -1, 533, -1, NULL, NULL, NULL, 0);
        buttonCreate(characterEditorWindowHandle, 47, 353, 245, 100, -1, -1, 534, -1, NULL, NULL, NULL, 0);
    } else {
        buttonCreate(characterEditorWindowHandle, 28, 363, 283, 105, -1, -1, 527, -1, NULL, NULL, NULL, 0);
    }

    buttonCreate(characterEditorWindowHandle, 191, 41, 122, 110, -1, -1, 528, -1, NULL, NULL, NULL, 0);
    buttonCreate(characterEditorWindowHandle, 191, 175, 122, 135, -1, -1, 529, -1, NULL, NULL, NULL, 0);
    buttonCreate(characterEditorWindowHandle, 376, 5, 223, 20, -1, -1, 530, -1, NULL, NULL, NULL, 0);
    buttonCreate(characterEditorWindowHandle, 370, 27, 223, 195, -1, -1, 531, -1, NULL, NULL, NULL, 0);
    buttonCreate(characterEditorWindowHandle, 396, 228, 171, 25, -1, -1, 532, -1, NULL, NULL, NULL, 0);
}

// copy character to editor
void _SavePlayer()
{
    Proto* proto;
    protoGetProto(gDude->pid, &proto);
    critterProtoDataCopy(&stru_570630, &(proto->critter.data));

    dword_5707C4 = critterGetHitPoints(gDude);

    strncpy(byte_5701A0, critterGetName(gDude), 32);

    dword_5709CC = dword_5707B4;
    for (int perk = 0; perk < PERK_COUNT; perk++) {
        dword_5707E4[perk] = perkGetRank(gDude, perk);
    }

    byte_570A28 = byte_570A29;

    dword_5707B0 = pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS);

    skillsGetTagged(dword_5709D4, NUM_TAGGED_SKILLS);

    traitsGetSelected(&(dword_5709F0[0]), &(dword_5709F0[1]));

    for (int skill = 0; skill < SKILL_COUNT; skill++) {
        dword_56FC60[skill] = skillGetValue(gDude, skill);
    }
}

// copy editor to character
void _RestorePlayer()
{
    Proto* proto;
    int i;
    int v3;
    int cur_hp;

    _pop_perks();

    protoGetProto(gDude->pid, &proto);
    critterProtoDataCopy(&(proto->critter.data), &stru_570630);

    dudeSetName(byte_5701A0);

    dword_5707B4 = dword_5709CC;
    byte_570A29 = byte_570A28;

    pcSetStat(PC_STAT_UNSPENT_SKILL_POINTS, dword_5707B0);

    skillsSetTagged(dword_5709D4, NUM_TAGGED_SKILLS);

    traitsSetSelected(dword_5709F0[0], dword_5709F0[1]);

    skillsGetTagged(dword_570A14, NUM_TAGGED_SKILLS);

    i = 4;
    v3 = 0;
    for (v3 = 0; v3 < 4; v3++) {
        if (dword_570A14[--i] != -1) {
            break;
        }
    }

    if (gCharacterEditorIsCreationMode == 1) {
        v3 -= gCharacterEditorIsCreationMode;
    }

    dword_570A10 = v3;

    traitsGetSelected(&(dword_570A04[0]), &(dword_570A04[1]));

    i = 2;
    v3 = 0;
    for (v3 = 0; v3 < 2; v3++) {
        if (dword_570A04[v3] != -1) {
            break;
        }
    }

    dword_5709FC = v3;

    critterUpdateDerivedStats(gDude);

    cur_hp = critterGetHitPoints(gDude);
    critterAdjustHitPoints(gDude, dword_5707C4 - cur_hp);
}

char* _itostndn(int value, char* dest)
{
    int v16[7];
    static_assert(sizeof(v16) == sizeof(dword_431DD4), "wrong size");
    memcpy(v16, dword_431DD4, sizeof(v16));

    char* savedDest = dest;

    if (value != 0) {
        *dest = '\0';

        bool v3 = false;
        for (int index = 0; index < 7; index++) {
            int v18 = value / v16[index];
            if (v18 > 0 || v3) {
                char temp[64]; // TODO: Size is probably wrong.
                itoa(v18, temp, 10);
                strcat(dest, temp);

                v3 = true;

                value -= v16[index] * v18;

                if (index == 0 || index == 3) {
                    strcat(dest, ",");
                }
            }
        }
    } else {
        strcpy(dest, "0");
    }

    return savedDest;
}

// 0x43AAEC
int _DrawCard(int graphicId, const char* name, const char* attributes, char* description)
{
    CacheEntry* graphicHandle;
    Size size;
    int fid;
    unsigned char* buf;
    unsigned char* ptr;
    int v9;
    int x;
    int y;
    short beginnings[WORD_WRAP_MAX_COUNT];
    short beginningsCount;

    fid = buildFid(10, graphicId, 0, 0, 0);
    buf = artLockFrameDataReturningSize(fid, &graphicHandle, &(size.width), &(size.height));
    if (buf == NULL) {
        return -1;
    }

    blitBufferToBuffer(buf, size.width, size.height, size.width, characterEditorWindowBuf + 640 * 309 + 484, 640);

    v9 = 150;
    ptr = buf;
    for (y = 0; y < size.height; y++) {
        for (x = 0; x < size.width; x++) {
            if (sub_44EBC0(*ptr) < 2 && v9 >= x) {
                v9 = x;
            }
            ptr++;
        }
    }

    v9 -= 8;
    if (v9 < 0) {
        v9 = 0;
    }

    fontSetCurrent(102);

    fontDrawText(characterEditorWindowBuf + 640 * 272 + 348, name, 640, 640, byte_6A38D0[0]);
    int nameFontLineHeight = fontGetLineHeight();
    if (attributes != NULL) {
        int nameWidth = fontGetStringWidth(name);

        fontSetCurrent(101);
        int attributesFontLineHeight = fontGetLineHeight();
        fontDrawText(characterEditorWindowBuf + 640 * (268 + nameFontLineHeight - attributesFontLineHeight) + 348 + nameWidth + 8, attributes, 640, 640, byte_6A38D0[0]);
    }

    y = nameFontLineHeight;
    windowDrawLine(characterEditorWindowHandle, 348, y + 272, 613, y + 272, byte_6A38D0[0]);
    windowDrawLine(characterEditorWindowHandle, 348, y + 273, 613, y + 273, byte_6A38D0[0]);

    fontSetCurrent(101);

    int descriptionFontLineHeight = fontGetLineHeight();

    if (wordWrap(description, v9 + 136, beginnings, &beginningsCount) != 0) {
        // TODO: Leaking graphic handle.
        return -1;
    }

    y = 315;
    for (short i = 0; i < beginningsCount - 1; i++) {
        short beginning = beginnings[i];
        short ending = beginnings[i + 1];
        char c = description[ending];
        description[ending] = '\0';
        fontDrawText(characterEditorWindowBuf + 640 * y + 348, description + beginning, 640, 640, byte_6A38D0[0]);
        description[ending] = c;
        y += descriptionFontLineHeight;
    }

    if ((graphicId != dword_5709EC || strcmp(name, byte_5700F8) != 0) && dword_5707D8) {
        soundPlayFile("isdxxxx1");
    }

    strcpy(byte_5700F8, name);

    dword_5709EC = graphicId;
    dword_5707D8 = 1;

    artUnlock(graphicHandle);

    return 0;
}

void _FldrButton()
{
    mouseGetPosition(&dword_5707CC, &dword_5707C8);
    soundPlayFile("ib3p1xx1");

    if (dword_5707CC >= 208) {
        characterEditorSelectedItem = 41;
        characterEditorWindowSelectedFolder = EDITOR_FOLDER_KILLS;
    } else if (dword_5707CC > 110) {
        characterEditorSelectedItem = 42;
        characterEditorWindowSelectedFolder = EDITOR_FOLDER_KARMA;
    } else {
        characterEditorSelectedItem = 40;
        characterEditorWindowSelectedFolder = EDITOR_FOLDER_PERKS;
    }

    editorRenderFolders();
    editorRenderDetails();
}

void _InfoButton(int eventCode)
{
    mouseGetPosition(&dword_5707CC, &dword_5707C8);

    switch (eventCode) {
    case 525:
        if (1) {
            // TODO: Original code is slightly different.
            double mouseY = dword_5707C8;
            for (int index = 0; index < 7; index++) {
                double buttonTop = dword_431D50[index];
                double buttonBottom = dword_431D50[index] + 22;
                double allowance = 5.0 - index * 0.25;
                if (mouseY >= buttonTop - allowance && mouseY <= buttonBottom + allowance) {
                    characterEditorSelectedItem = index;
                    break;
                }
            }
        }
        break;
    case 526:
        if (gCharacterEditorIsCreationMode) {
            characterEditorSelectedItem = 7;
        } else {
            int offset = dword_5707C8 - 280;
            if (offset < 0) {
                offset = 0;
            }

            characterEditorSelectedItem = offset / 10 + 7;
        }
        break;
    case 527:
        if (!gCharacterEditorIsCreationMode) {
            fontSetCurrent(101);
            int offset = dword_5707C8 - 364;
            if (offset < 0) {
                offset = 0;
            }
            characterEditorSelectedItem = offset / (fontGetLineHeight() + 1) + 10;
        }
        break;
    case 528:
        if (1) {
            int offset = dword_5707C8 - 41;
            if (offset < 0) {
                offset = 0;
            }

            characterEditorSelectedItem = offset / 13 + 43;
        }
        break;
    case 529: {
        int offset = dword_5707C8 - 175;
        if (offset < 0) {
            offset = 0;
        }

        characterEditorSelectedItem = offset / 13 + 51;
        break;
    }
    case 530:
        characterEditorSelectedItem = 80;
        break;
    case 531:
        if (1) {
            int offset = dword_5707C8 - 27;
            if (offset < 0) {
                offset = 0;
            }

            dword_51852C = offset * 0.092307694;
            if (dword_51852C >= 18) {
                dword_51852C = 17;
            }

            characterEditorSelectedItem = dword_51852C + 61;
        }
        break;
    case 532:
        characterEditorSelectedItem = 79;
        break;
    case 533:
        characterEditorSelectedItem = 81;
        break;
    case 534:
        if (1) {
            fontSetCurrent(101);

            // TODO: Original code is slightly different.
            double mouseY = dword_5707C8;
            double fontLineHeight = fontGetLineHeight();
            double y = 353.0;
            double step = fontGetLineHeight() + 3 + 0.56;
            int index;
            for (index = 0; index < 8; index++) {
                if (mouseY >= y - 4.0 && mouseY <= y + fontLineHeight) {
                    break;
                }
                y += step;
            }

            if (index == 8) {
                index = 7;
            }

            characterEditorSelectedItem = index + 82;
            if (dword_5707CC >= 169) {
                characterEditorSelectedItem += 8;
            }
        }
        break;
    }

    editorRenderPrimaryStat(RENDER_ALL_STATS, 0, 0);
    characterEditorWindowRenderTraits();
    editorRenderSkills(0);
    editorRenderPcStats();
    editorRenderFolders();
    editorRenderSecondaryStats();
    editorRenderDetails();
}

// 0x43B230
void editorAdjustSkill(int keyCode)
{
    if (gCharacterEditorIsCreationMode) {
        return;
    }

    int sp = pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS);
    dword_5709C0 = 4;

    bool v40 = false;
    int v3 = 0;

    switch (keyCode) {
    case '+':
    case 'N':
    case 333:
        v40 = true;
        keyCode = 521;
        break;
    case '-':
    case 'J':
    case 331:
        v40 = true;
        keyCode = 523;
        break;
    }

    char title[64];
    char body1[64];
    char body2[64];

    const char* body[] = {
        body1,
        body2,
    };

    int v43 = 0;
    for (;;) {
        dword_5709C4 = _get_time();
        if (v43 <= dbl_5018F0) {
            v43++;
        }

        if (v43 == 1 || v43 > dbl_5018F0) {
            if (v43 > dbl_5018F0) {
                dword_5709C0++;
                if (dword_5709C0 > 24) {
                    dword_5709C0 = 24;
                }
            }

            v3 = 1;
            if (keyCode == 521) {
                if (pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS) > 0) {
                    if (skillAdd(gDude, dword_51852C) == -3) {
                        soundPlayFile("iisxxxx1");

                        sprintf(title, "%s:", skillGetName(dword_51852C));
                        // At maximum level.
                        strcpy(body1, getmsg(&editorMessageList, &editorMessageListItem, 132));
                        // Unable to increment it.
                        strcpy(body2, getmsg(&editorMessageList, &editorMessageListItem, 133));
                        showDialogBox(title, body, 2, 192, 126, byte_6A38D0[32328], NULL, byte_6A38D0[32328], DIALOG_BOX_LARGE);
                        v3 = -1;
                    }
                }
                else {
                    soundPlayFile("iisxxxx1");

                    // Not enough skill points available.
                    strcpy(title, getmsg(&editorMessageList, &editorMessageListItem, 136));
                    showDialogBox(title, NULL, 0, 192, 126, byte_6A38D0[32328], NULL, byte_6A38D0[32328], DIALOG_BOX_LARGE);
                    v3 = -1;
                }
            } else if (keyCode == 523) {
                if (skillGetValue(gDude, dword_51852C) <= dword_56FC60[dword_51852C]) {
                    v3 = 0;
                }
                else {
                    if (skillSub(gDude, dword_51852C) == -2) {
                        v3 = 0;
                    }
                }

                if (v3 == 0) {
                    soundPlayFile("iisxxxx1");

                    sprintf(title, "%s:", skillGetName(dword_51852C));
                    // At minimum level.
                    strcpy(body1, getmsg(&editorMessageList, &editorMessageListItem, 134));
                    // Unable to decrement it.
                    strcpy(body2, getmsg(&editorMessageList, &editorMessageListItem, 135));
                    showDialogBox(title, body, 2, 192, 126, byte_6A38D0[32328], NULL, byte_6A38D0[32328], DIALOG_BOX_LARGE);
                    v3 = -1;
                }
            }

            characterEditorSelectedItem = dword_51852C + 61;
            editorRenderDetails();
            editorRenderSkills(1);

            int flags;
            if (v3 == 1) {
                flags = ANIMATE;
            } else {
                flags = 0;
            }

            characterEditorRenderBigNumber(522, 228, flags, pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS), sp, characterEditorWindowHandle);

            windowRefresh(characterEditorWindowHandle);
        }

        if (!v40) {
            debugPrint("v43: %d\n", v43);
            sp = pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS);
            if (v43 >= dbl_5018F0) {
                while (getTicksSince(dword_5709C4) < 1000 / dword_5709C0) {
                }
            }
            else {
                while (getTicksSince(dword_5709C4) < 1000 / 24) {
                }
            }

            int keyCode = _get_input();
            if (keyCode == 522 || keyCode == 524 || v3 == -1) {
                break;
            }
        }
    }
}

// 0x43B67C
void characterEditorToggleTaggedSkill(int skill)
{
    int insertionIndex;

    insertionIndex = 0;
    for (int index = 3; index >= 0; index--) {
        if (dword_570A14[index] != -1) {
            break;
        }
        insertionIndex++;
    }

    if (gCharacterEditorIsCreationMode) {
        insertionIndex -= 1;
    }

    dword_5709C8 = insertionIndex;

    if (skill == dword_570A14[0] || skill == dword_570A14[1] || skill == dword_570A14[2] || skill == dword_570A14[3]) {
        if (skill == dword_570A14[0]) {
            dword_570A14[0] = dword_570A14[1];
            dword_570A14[1] = dword_570A14[2];
            dword_570A14[2] = -1;
        } else if (skill == dword_570A14[1]) {
            dword_570A14[1] = dword_570A14[2];
            dword_570A14[2] = -1;
        } else {
            dword_570A14[2] = -1;
        }
    } else {
        if (dword_570A10 > 0) {
            insertionIndex = 0;
            for (int index = 0; index < 3; index++) {
                if (dword_570A14[index] == -1) {
                    break;
                }
                insertionIndex++;
            }
            dword_570A14[insertionIndex] = skill;
        } else {
            soundPlayFile("iisxxxx1");

            char line1[128];
            strcpy(line1, getmsg(&editorMessageList, &editorMessageListItem, 140));

            char line2[128];
            strcpy(line2, getmsg(&editorMessageList, &editorMessageListItem, 141));

            const char* lines[] = { line2 };
            showDialogBox(line1, lines, 1, 192, 126, byte_6A38D0[32328], 0, byte_6A38D0[32328], 0);
        }
    }

    insertionIndex = 0;
    for (int index = 3; index >= 0; index--) {
        if (dword_570A14[index] != -1) {
            break;
        }
        insertionIndex++;
    }

    if (gCharacterEditorIsCreationMode) {
        insertionIndex -= 1;
    }

    dword_570A10 = insertionIndex;

    characterEditorSelectedItem = skill + 61;
    editorRenderPrimaryStat(RENDER_ALL_STATS, 0, 0);
    editorRenderSecondaryStats();
    editorRenderSkills(2);
    editorRenderDetails();
    windowRefresh(characterEditorWindowHandle);
}

// 0x43B8A8
void characterEditorWindowRenderTraits()
{
    int v0 = -1;
    int i;
    int color;
    const char* traitName;
    double step;
    double y;

    if (gCharacterEditorIsCreationMode != 1) {
        return;
    }

    if (characterEditorSelectedItem >= 82 && characterEditorSelectedItem < 98) {
        v0 = characterEditorSelectedItem - 82;
    }

    blitBufferToBuffer(characterEditorWindowBackgroundBuf + 640 * 353 + 47, 245, 100, 640, characterEditorWindowBuf + 640 * 353 + 47, 640);

    fontSetCurrent(101);

    traitsSetSelected(dword_570A04[0], dword_570A04[1]);

    step = fontGetLineHeight() + 3 + 0.56;
    y = 353;
    for (i = 0; i < 8; i++) {
        if (i == v0) {
            if (i != dword_570A04[0] && i != dword_570A04[1]) {
                color = byte_6A38D0[32747];
            } else {
                color = byte_6A38D0[32767];
            }

            dword_5705B0 = traitGetFrmId(i);
            off_5705B8 = traitGetName(i);
            off_5705BC = NULL;
            off_5705CC = traitGetDescription(i);
        } else {
            if (i != dword_570A04[0] && i != dword_570A04[1]) {
                color = byte_6A38D0[992];
            } else {
                color = byte_6A38D0[21140];
            }
        }

        traitName = traitGetName(i);
        fontDrawText(characterEditorWindowBuf + 640 * (int)y + 47, traitName, 640, 640, color);
        y += step;
    }

    y = 353;
    for (i = 8; i < 16; i++) {
        if (i == v0) {
            if (i != dword_570A04[0] && i != dword_570A04[1]) {
                color = byte_6A38D0[32747];
            } else {
                color = byte_6A38D0[32767];
            }

            dword_5705B0 = traitGetFrmId(i);
            off_5705B8 = traitGetName(i);
            off_5705BC = NULL;
            off_5705CC = traitGetDescription(i);
        } else {
            if (i != dword_570A04[0] && i != dword_570A04[1]) {
                color = byte_6A38D0[992];
            } else {
                color = byte_6A38D0[21140];
            }
        }

        traitName = traitGetName(i);
        fontDrawText(characterEditorWindowBuf + 640 * (int)y + 199, traitName, 640, 640, color);
        y += step;
    }
}

// 0x43BB0C
void characterEditorToggleOptionalTrait(int trait)
{
    if (trait == dword_570A04[0] || trait == dword_570A04[1]) {
        if (trait == dword_570A04[0]) {
            dword_570A04[0] = dword_570A04[1];
            dword_570A04[1] = -1;
        } else {
            dword_570A04[1] = -1;
        }
    } else {
        if (dword_5709FC == 0) {
            soundPlayFile("iisxxxx1");

            char line1[128];
            strcpy(line1, getmsg(&editorMessageList, &editorMessageListItem, 148));

            char line2[128];
            strcpy(line2, getmsg(&editorMessageList, &editorMessageListItem, 149));

            const char* lines = { line2 };
            showDialogBox(line1, &lines, 1, 192, 126, byte_6A38D0[32328], 0, byte_6A38D0[32328], 0);
        } else {
            for (int index = 0; index < 2; index++) {
                if (dword_570A04[index] == -1) {
                    dword_570A04[index] = trait;
                    break;
                }
            }
        }
    }

    dword_5709FC = 0;
    for (int index = 1; index != 0; index--) {
        if (dword_570A04[index] != -1) {
            break;
        }
        dword_5709FC++;
    }

    characterEditorSelectedItem = trait + EDITOR_FIRST_TRAIT;

    characterEditorWindowRenderTraits();
    editorRenderSkills(0);
    critterUpdateDerivedStats(gDude);
    characterEditorRenderBigNumber(126, 282, 0, characterEditorRemainingCharacterPoints, 0, characterEditorWindowHandle);
    editorRenderPrimaryStat(RENDER_ALL_STATS, false, 0);
    editorRenderSecondaryStats();
    editorRenderDetails();
    windowRefresh(characterEditorWindowHandle);
}

// 0x43BCE0
void editorRenderKarma()
{
    char* msg;
    char formattedText[256];

    _folder_clear();

    bool hasSelection = false;
    for (int index = 0; index < gKarmaEntriesLength; index++) {
        KarmaEntry* karmaDescription = &(gKarmaEntries[index]);
        if (karmaDescription->gvar == GVAR_PLAYER_REPUTATION) {
            int reputation;
            for (reputation = 0; reputation < gGenericReputationEntriesLength; reputation++) {
                GenericReputationEntry* reputationDescription = &(gGenericReputationEntries[reputation]);
                if (gGameGlobalVars[GVAR_PLAYER_REPUTATION] >= reputationDescription->threshold) {
                    break;
                }
            }

            if (reputation != gGenericReputationEntriesLength) {
                GenericReputationEntry* reputationDescription = &(gGenericReputationEntries[reputation]);

                char reputationValue[32];
                itoa(gGameGlobalVars[GVAR_PLAYER_REPUTATION], reputationValue, 10);

                sprintf(formattedText,
                    "%s: %s (%s)",
                    getmsg(&editorMessageList, &editorMessageListItem, 125),
                    reputationValue,
                    getmsg(&editorMessageList, &editorMessageListItem, reputationDescription->name));

                if (_folder_print_line(formattedText)) {
                    dword_5705B0 = karmaDescription->art_num;
                    off_5705B8 = getmsg(&editorMessageList, &editorMessageListItem, 125);
                    off_5705BC = NULL;
                    off_5705CC = getmsg(&editorMessageList, &editorMessageListItem, karmaDescription->description);
                    hasSelection = true;
                }
            }
        } else {
            if (gGameGlobalVars[karmaDescription->gvar] != 0) {
                msg = getmsg(&editorMessageList, &editorMessageListItem, karmaDescription->name);
                if (_folder_print_line(msg)) {
                    dword_5705B0 = karmaDescription->art_num;
                    off_5705B8 = getmsg(&editorMessageList, &editorMessageListItem, karmaDescription->name);
                    off_5705BC = NULL;
                    off_5705CC = getmsg(&editorMessageList, &editorMessageListItem, karmaDescription->description);
                    hasSelection = true;
                }
            }
        }
    }

    bool hasTownReputationHeading = false;
    for (int index = 0; index < TOWN_REPUTATION_COUNT; index++) {
        const TownReputationEntry* pair = &(gTownReputationEntries[index]);
        if (_wmAreaIsKnown(pair->city)) {
            if (!hasTownReputationHeading) {
                msg = getmsg(&editorMessageList, &editorMessageListItem, 4000);
                if (_folder_print_seperator(msg)) {
                    dword_5705B0 = 48;
                    off_5705B8 = getmsg(&editorMessageList, &editorMessageListItem, 4000);
                    off_5705BC = NULL;
                    off_5705CC = getmsg(&editorMessageList, &editorMessageListItem, 4100);
                }
                hasTownReputationHeading = true;
            }

            char cityShortName[40];
            _wmGetAreaIdxName(pair->city, cityShortName);

            int townReputation = gGameGlobalVars[pair->gvar];

            int townReputationGraphicId;
            int townReputationBaseMessageId;

            if (townReputation < -30) {
                townReputationGraphicId = 150;
                townReputationBaseMessageId = 2006; // Vilified
            } else if (townReputation < -15) {
                townReputationGraphicId = 153;
                townReputationBaseMessageId = 2005; // Hated
            } else if (townReputation < 0) {
                townReputationGraphicId = 153;
                townReputationBaseMessageId = 2004; // Antipathy
            } else if (townReputation == 0) {
                townReputationGraphicId = 141;
                townReputationBaseMessageId = 2003; // Neutral
            } else if (townReputation < 15) {
                townReputationGraphicId = 137;
                townReputationBaseMessageId = 2002; // Accepted
            } else if (townReputation < 30) {
                townReputationGraphicId = 137;
                townReputationBaseMessageId = 2001; // Liked
            } else {
                townReputationGraphicId = 135;
                townReputationBaseMessageId = 2000; // Idolized
            }

            msg = getmsg(&editorMessageList, &editorMessageListItem, townReputationBaseMessageId);
            sprintf(formattedText,
                "%s: %s",
                cityShortName,
                msg);

            if (_folder_print_line(formattedText)) {
                dword_5705B0 = townReputationGraphicId;
                off_5705B8 = getmsg(&editorMessageList, &editorMessageListItem, townReputationBaseMessageId);
                off_5705BC = NULL;
                off_5705CC = getmsg(&editorMessageList, &editorMessageListItem, townReputationBaseMessageId + 100);
                hasSelection = 1;
            }
        }
    }

    bool hasAddictionsHeading = false;
    for (int index = 0; index < ADDICTION_REPUTATION_COUNT; index++) {
        if (gGameGlobalVars[gAddictionReputationVars[index]] != 0) {
            if (!hasAddictionsHeading) {
                // Addictions
                msg = getmsg(&editorMessageList, &editorMessageListItem, 4001);
                if (_folder_print_seperator(msg)) {
                    dword_5705B0 = 53;
                    off_5705B8 = getmsg(&editorMessageList, &editorMessageListItem, 4001);
                    off_5705BC = NULL;
                    off_5705CC = getmsg(&editorMessageList, &editorMessageListItem, 4101);
                    hasSelection = 1;
                }
                hasAddictionsHeading = true;
            }

            msg = getmsg(&editorMessageList, &editorMessageListItem, 1004 + index);
            if (_folder_print_line(msg)) {
                dword_5705B0 = gAddictionReputationFrmIds[index];
                off_5705B8 = getmsg(&editorMessageList, &editorMessageListItem, 1004 + index);
                off_5705BC = NULL;
                off_5705CC = getmsg(&editorMessageList, &editorMessageListItem, 1104 + index);
                hasSelection = 1;
            }
        }
    }

    if (!hasSelection) {
        dword_5705B0 = 47;
        off_5705B8 = getmsg(&editorMessageList, &editorMessageListItem, 125);
        off_5705BC = NULL;
        off_5705CC = getmsg(&editorMessageList, &editorMessageListItem, 128);
    }
}

//
int _editor_save(File* stream)
{
    if (fileWriteInt32(stream, dword_5707B4) == -1)
        return -1;
    if (fileWriteUInt8(stream, byte_570A29) == -1)
        return -1;

    return 0;
}

//
int _editor_load(File* stream)
{
    if (fileReadInt32(stream, &dword_5707B4) == -1)
        return -1;
    if (fileReadUInt8(stream, &byte_570A29) == -1)
        return -1;

    return 0;
}

//
void _editor_reset()
{
    characterEditorRemainingCharacterPoints = 5;
    dword_5707B4 = 1;
}

// level up if needed
int _UpdateLevel()
{
    int level = pcGetStat(PC_STAT_LEVEL);
    if (level != dword_5707B4 && level <= PC_LEVEL_MAX) {
        for (int nextLevel = dword_5707B4 + 1; nextLevel <= level; nextLevel++) {
            int sp = pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS);
            sp += 5;
            sp += critterGetBaseStatWithTraitModifier(gDude, STAT_INTELLIGENCE) * 2;
            sp += perkGetRank(gDude, PERK_EDUCATED) * 2;
            sp += traitIsSelected(TRAIT_SKILLED) * 5;
            if (traitIsSelected(TRAIT_GIFTED)) {
                sp -= 5;
                if (sp < 0) {
                    sp = 0;
                }
            }
            if (sp > 99) {
                sp = 99;
            }

            pcSetStat(PC_STAT_UNSPENT_SKILL_POINTS, sp);

            int selectedPerksCount = 0;
            for (int perk = 0; perk < PERK_COUNT; perk++) {
                if (perkGetRank(gDude, perk) != 0) {
                    selectedPerksCount += 1;
                    if (selectedPerksCount >= 37) {
                        break;
                    }
                }
            }

            if (selectedPerksCount < 37) {
                int progression = 3;
                if (traitIsSelected(TRAIT_SKILLED)) {
                    progression += 1;
                }

                if (nextLevel % progression == 0) {
                    byte_570A29 = 1;
                }
            }
        }
    }

    if (byte_570A29 != 0) {
        characterEditorWindowSelectedFolder = 0;
        editorRenderFolders();
        windowRefresh(characterEditorWindowHandle);

        int rc = editorSelectPerk();
        if (rc == -1) {
            debugPrint("\n *** Error running perks dialog! ***\n");
            return -1;
        } else if (rc == 0) {
            editorRenderFolders();
        } else if (rc == 1) {
            editorRenderFolders();
            byte_570A29 = 0;
        }
    }

    dword_5707B4 = level;

    return 1;
}

//
void _RedrwDPrks()
{
    blitBufferToBuffer(
        gEditorPerkBackgroundBuffer + 280,
        293,
        PERK_WINDOW_HEIGHT,
        PERK_WINDOW_WIDTH,
        gEditorPerkWindowBuffer + 280,
        PERK_WINDOW_WIDTH);

    _ListDPerks();

    // NOTE: Original code is slightly different, but basically does the same thing.
    int perk = stru_56FCB0[dword_5707DC + dword_5707A8].field_0;
    int perkFrmId = perkGetFrmId(perk);
    char* perkName = perkGetName(perk);
    char* perkDescription = perkGetDescription(perk);
    char* perkRank = NULL;
    char perkRankBuffer[32];

    int rank = perkGetRank(gDude, perk);
    if (rank != 0) {
        sprintf(perkRankBuffer, "(%d)", rank);
        perkRank = perkRankBuffer;
    }

    _DrawCard2(perkFrmId, perkName, perkRank, perkDescription);

    windowRefresh(gEditorPerkWindow);
}

// 0x43C4F0
int editorSelectPerk()
{
    dword_5707DC = 0;
    dword_5707A8 = 0;
    dword_5709E8 = -1;
    byte_570128[0] = '\0';
    dword_5707E0 = 0;

    CacheEntry* backgroundFrmHandle;
    int backgroundWidth;
    int backgroundHeight;
    int fid = buildFid(6, 86, 0, 0, 0);
    gEditorPerkBackgroundBuffer = artLockFrameDataReturningSize(fid, &backgroundFrmHandle, &backgroundWidth, &backgroundHeight);
    if (gEditorPerkBackgroundBuffer == NULL) {
        debugPrint("\n *** Error running perks dialog window ***\n");
        return -1;
    }

    gEditorPerkWindow = windowCreate(PERK_WINDOW_X, PERK_WINDOW_Y, PERK_WINDOW_WIDTH, PERK_WINDOW_HEIGHT, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);
    if (gEditorPerkWindow == -1) {
        artUnlock(backgroundFrmHandle);
        debugPrint("\n *** Error running perks dialog window ***\n");
        return -1;
    }

    gEditorPerkWindowBuffer = windowGetBuffer(gEditorPerkWindow);
    memcpy(gEditorPerkWindowBuffer, gEditorPerkBackgroundBuffer, PERK_WINDOW_WIDTH * PERK_WINDOW_HEIGHT);

    int btn;

    btn = buttonCreate(gEditorPerkWindow,
        48,
        186,
        stru_5701C0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        stru_5701C0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        500,
        dword_5704E0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        dword_5704E0[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    btn = buttonCreate(gEditorPerkWindow,
        153,
        186,
        stru_5701C0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        stru_5701C0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        502,
        dword_5704E0[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        dword_5704E0[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, _gsound_red_butt_press, _gsound_red_butt_release);
    }

    btn = buttonCreate(gEditorPerkWindow,
        25,
        46,
        stru_5701C0[EDITOR_GRAPHIC_UP_ARROW_ON].width,
        stru_5701C0[EDITOR_GRAPHIC_UP_ARROW_ON].height,
        -1,
        574,
        572,
        574,
        dword_5704E0[EDITOR_GRAPHIC_UP_ARROW_OFF],
        dword_5704E0[EDITOR_GRAPHIC_UP_ARROW_ON],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, _gsound_red_butt_press, NULL);
    }

    btn = buttonCreate(gEditorPerkWindow,
        25,
        47 + stru_5701C0[EDITOR_GRAPHIC_UP_ARROW_ON].height,
        stru_5701C0[EDITOR_GRAPHIC_UP_ARROW_ON].width,
        stru_5701C0[EDITOR_GRAPHIC_UP_ARROW_ON].height,
        -1,
        575,
        573,
        575,
        dword_5704E0[EDITOR_GRAPHIC_DOWN_ARROW_OFF],
        dword_5704E0[EDITOR_GRAPHIC_DOWN_ARROW_ON],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, _gsound_red_butt_press, NULL);
    }

    buttonCreate(gEditorPerkWindow,
        45,
        43,
        192,
        129,
        -1,
        -1,
        -1,
        501,
        NULL,
        NULL,
        NULL,
        BUTTON_FLAG_TRANSPARENT);

    fontSetCurrent(103);

    const char* msg;

    // PICK A NEW PERK
    msg = getmsg(&editorMessageList, &editorMessageListItem, 152);
    fontDrawText(gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * 16 + 49, msg, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, byte_6A38D0[18979]);

    // DONE
    msg = getmsg(&editorMessageList, &editorMessageListItem, 100);
    fontDrawText(gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * 186 + 69, msg, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, byte_6A38D0[18979]);

    // CANCEL
    msg = getmsg(&editorMessageList, &editorMessageListItem, 102);
    fontDrawText(gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * 186 + 171, msg, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, byte_6A38D0[18979]);

    int count = _ListDPerks();
    
    // NOTE: Original code is slightly different, but does the same thing.
    int perk = stru_56FCB0[dword_5707DC + dword_5707A8].field_0;
    int perkFrmId = perkGetFrmId(perk);
    char* perkName = perkGetName(perk);
    char* perkDescription = perkGetDescription(perk);
    char* perkRank = NULL;
    char perkRankBuffer[32];

    int rank = perkGetRank(gDude, perk);
    if (rank != 0) {
        sprintf(perkRankBuffer, "(%d)", rank);
        perkRank = perkRankBuffer;
    }
    
    _DrawCard2(perkFrmId, perkName, perkRank, perkDescription);
    windowRefresh(gEditorPerkWindow);

    int rc = _InputPDLoop(count, _RedrwDPrks);

    if (rc == 1) {
        if (perkAdd(gDude, stru_56FCB0[dword_5707DC + dword_5707A8].field_0) == -1) {
            debugPrint("\n*** Unable to add perk! ***\n");
            rc = 2;
        }
    }

    rc &= 1;

    if (rc != 0) {
        if (perkGetRank(gDude, PERK_TAG) != 0 && dword_5707E4[PERK_TAG] == 0) {
            if (!editorHandleTag()) {
                perkRemove(gDude, PERK_TAG);
            }
        } else if (perkGetRank(gDude, PERK_MUTATE) != 0 && dword_5707E4[PERK_MUTATE] == 0) {
            if (!editorHandleMutate()) {
                perkRemove(gDude, PERK_MUTATE);
            }
        } else if (perkGetRank(gDude, PERK_LIFEGIVER) != dword_5707E4[PERK_LIFEGIVER]) {
            int maxHp = critterGetBonusStat(gDude, STAT_MAXIMUM_HIT_POINTS);
            critterSetBonusStat(gDude, STAT_MAXIMUM_HIT_POINTS, maxHp + 4);
            critterAdjustHitPoints(gDude, 4);
        } else if (perkGetRank(gDude, PERK_EDUCATED) != dword_5707E4[PERK_EDUCATED]) {
            int sp = pcGetStat(PC_STAT_UNSPENT_SKILL_POINTS);
            pcSetStat(PC_STAT_UNSPENT_SKILL_POINTS, sp + 2);
        }
    }

    editorRenderSkills(0);
    editorRenderPrimaryStat(RENDER_ALL_STATS, 0, 0);
    editorRenderPcStats();
    editorRenderSecondaryStats();
    editorRenderFolders();
    editorRenderDetails();
    windowRefresh(characterEditorWindowHandle);
    
    artUnlock(backgroundFrmHandle);

    windowDestroy(gEditorPerkWindow);

    return rc;
}

int _InputPDLoop(int count, void(*refreshProc)())
{
    fontSetCurrent(101);

    int v3 = count - 11;

    int height = fontGetLineHeight();
    dword_5707AC = -2;
    int v16 = height + 2;

    int v7 = 0;

    int rc = 0;
    while (rc == 0) {
        int keyCode = _get_input();
        int v19 = 0;

        if (keyCode == 500) {
            rc = 1;
        } else if (keyCode == KEY_RETURN) {
            soundPlayFile("ib1p1xx1");
            rc = 1;
        } else if (keyCode == 501) {
            mouseGetPosition(&dword_5707CC, &dword_5707C8);
            dword_5707A8 = (dword_5707C8 - 134) / v16;
            if ((dword_5707C8 - 134) / v16 >= 0) {
                if (count - 1 < (dword_5707C8 - 134) / v16)
                    dword_5707A8 = count - 1;
            } else {
                dword_5707A8 = 0;
            }

            if (dword_5707A8 == dword_5707AC) {
                soundPlayFile("ib1p1xx1");
                rc = 1;
            }
            dword_5707AC = dword_5707A8;
            refreshProc();
        }
        else if (keyCode == 502 || keyCode == KEY_ESCAPE || dword_5186CC != 0) {
            rc = 2;
        }
        else {
            switch (keyCode) {
            case KEY_ARROW_UP:
                dword_5707AC = -2;

                dword_5707DC--;
                if (dword_5707DC < 0) {
                    dword_5707DC = 0;

                    dword_5707A8--;
                    if (dword_5707A8 < 0) {
                        dword_5707A8 = 0;
                    }
                }

                refreshProc();
                break;
            case KEY_PAGE_UP:
                dword_5707AC = -2;

                for (int index = 0; index < 11; index++) {
                    dword_5707DC--;
                    if (dword_5707DC < 0) {
                        dword_5707DC = 0;

                        dword_5707A8--;
                        if (dword_5707A8 < 0) {
                            dword_5707A8 = 0;
                        }
                    }
                }

                refreshProc();
                break;
            case KEY_ARROW_DOWN:
                dword_5707AC = -2;

                if (count > 11) {
                    dword_5707DC++;
                    if (dword_5707DC > count - 11) {
                        dword_5707DC = count - 11;

                        dword_5707A8++;
                        if (dword_5707A8 > 10) {
                            dword_5707A8 = 10;
                        }
                    }
                }
                else {
                    dword_5707A8++;
                    if (dword_5707A8 > count - 1) {
                        dword_5707A8 = count - 1;
                    }
                }

                refreshProc();
                break;
            case KEY_PAGE_DOWN:
                dword_5707AC = -2;

                for (int index = 0; index < 11; index++) {
                    if (count > 11) {
                        dword_5707DC++;
                        if (dword_5707DC > count - 11) {
                            dword_5707DC = count - 11;

                            dword_5707A8++;
                            if (dword_5707A8 > 10) {
                                dword_5707A8 = 10;
                            }
                        }
                    }
                    else {
                        dword_5707A8++;
                        if (dword_5707A8 > count - 1) {
                            dword_5707A8 = count - 1;
                        }
                    }
                }

                refreshProc();
                break;
            case 572:
                dword_5709C0 = 4;
                dword_5707AC = -2;

                do {
                    dword_5709C4 = _get_time();
                    if (v19 <= dbl_5019BE) {
                        v19++;
                    }

                    if (v19 == 1 || v19 > dbl_5019BE) {
                        if (v19 > dbl_5019BE) {
                            dword_5709C0++;
                            if (dword_5709C0 > 24) {
                                dword_5709C0 = 24;
                            }
                        }

                        dword_5707DC--;
                        if (dword_5707DC < 0) {
                            dword_5707DC = 0;

                            dword_5707A8--;
                            if (dword_5707A8 < 0) {
                                dword_5707A8 = 0;
                            }
                        }
                        refreshProc();
                    }

                    if (v19 < dbl_5019BE) {
                        while (getTicksSince(dword_5709C4) < 1000 / 24) {
                        }
                    }
                    else {
                        while (getTicksSince(dword_5709C4) < 1000 / dword_5709C0) {
                        }
                    }
                } while (_get_input() != 574);

                break;
            case 573:
                dword_5707AC = -2;
                dword_5709C0 = 4;

                if (count > 11) {
                    do {
                        dword_5709C4 = _get_time();
                        if (v19 <= dbl_5019BE) {
                            v19++;
                        }

                        if (v19 == 1 || v19 > dbl_5019BE) {
                            if (v19 > dbl_5019BE) {
                                dword_5709C0++;
                                if (dword_5709C0 > 24) {
                                    dword_5709C0 = 24;
                                }
                            }

                            dword_5707DC++;
                            if (dword_5707DC > count - 11) {
                                dword_5707DC = count - 11;

                                dword_5707A8++;
                                if (dword_5707A8 > 10) {
                                    dword_5707A8 = 10;
                                }
                            }

                            refreshProc();
                        }

                        if (v19 < dbl_5019BE) {
                            while (getTicksSince(dword_5709C4) < 1000 / 24) {
                            }
                        }
                        else {
                            while (getTicksSince(dword_5709C4) < 1000 / dword_5709C0) {
                            }
                        }
                    } while (_get_input() != 575);
                }
                else {
                    do {
                        dword_5709C4 = _get_time();
                        if (v19 <= dbl_5019BE) {
                            v19++;
                        }

                        if (v19 == 1 || v19 > dbl_5019BE) {
                            if (v19 > dbl_5019BE) {
                                dword_5709C0++;
                                if (dword_5709C0 > 24) {
                                    dword_5709C0 = 24;
                                }
                            }

                            dword_5707A8++;
                            if (dword_5707A8 > count - 1) {
                                dword_5707A8 = count - 1;
                            }

                            refreshProc();
                        }

                        if (v19 < dbl_5019BE) {
                            while (getTicksSince(dword_5709C4) < 1000 / 24) {
                            }
                        } else {
                            while (getTicksSince(dword_5709C4) < 1000 / dword_5709C0) {
                            }
                        }
                    } while (_get_input() != 575);
                }
                break;
            case KEY_HOME:
                dword_5707DC = 0;
                dword_5707A8 = 0;
                dword_5707AC = -2;
                refreshProc();
                break;
            case KEY_END:
                dword_5707AC = -2;
                if (count > 11) {
                    dword_5707DC = count - 11;
                    dword_5707A8 = 10;
                }
                else {
                    dword_5707A8 = count - 1;
                }
                refreshProc();
                break;
            default:
                if (getTicksSince(dword_5709C4) > 700) {
                    dword_5709C4 = _get_time();
                    dword_5707AC = -2;
                }
                break;
            }
        }
    }

    return rc;
}

//
int _ListDPerks()
{
    blitBufferToBuffer(
        gEditorPerkBackgroundBuffer + PERK_WINDOW_WIDTH * 43 + 45,
        192,
        129,
        PERK_WINDOW_WIDTH,
        gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * 43 + 45,
        PERK_WINDOW_WIDTH);

    fontSetCurrent(101);

    int perks[PERK_COUNT];
    int count = perkGetAvailablePerks(gDude, perks);
    if (count == 0) {
        return 0;
    }

    for (int perk = 0; perk < PERK_COUNT; perk++) {
        stru_56FCB0[perk].field_0 = 0;
        stru_56FCB0[perk].field_4 = NULL;
    }

    for (int index = 0; index < count; index++) {
        stru_56FCB0[index].field_0 = perks[index];
        stru_56FCB0[index].field_4 = perkGetName(perks[index]);
    }

    qsort(stru_56FCB0, count, sizeof(*stru_56FCB0), _name_sort_comp);

    int v16 = count - dword_5707DC;
    if (v16 > 11) {
        v16 = 11;
    }

    v16 += dword_5707DC;

    int y = 43;
    int yStep = fontGetLineHeight() + 2;
    for (int index = dword_5707DC; index < v16; index++) {
        int color;
        if (index == dword_5707DC + dword_5707A8) {
            color = byte_6A38D0[32747];
        } else {
            color = byte_6A38D0[992];
        }

        fontDrawText(gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * y + 45, stru_56FCB0[index].field_4, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, color);

        if (perkGetRank(gDude, stru_56FCB0[index].field_0) != 0) {
            char rankString[256];
            sprintf(rankString, "(%d)", perkGetRank(gDude, stru_56FCB0[index].field_0));
            fontDrawText(gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * y + 207, rankString, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, color);
        }

        y += yStep;
    }

    return count;
}

//
void _RedrwDMPrk()
{
    blitBufferToBuffer(gEditorPerkBackgroundBuffer + 280, 293, PERK_WINDOW_HEIGHT, PERK_WINDOW_WIDTH, gEditorPerkWindowBuffer + 280, PERK_WINDOW_WIDTH);

    _ListMyTraits(dword_570A00);

    char* traitName = stru_56FCB0[dword_5707DC + dword_5707A8].field_4;
    char* tratDescription = traitGetDescription(stru_56FCB0[dword_5707DC + dword_5707A8].field_0);
    int frmId = traitGetFrmId(stru_56FCB0[dword_5707DC + dword_5707A8].field_0);
    _DrawCard2(frmId, traitName, NULL, tratDescription);

    windowRefresh(gEditorPerkWindow);
}

// 0x43D38C
bool editorHandleMutate()
{
    dword_5709E8 = -1;
    byte_570128[0] = '\0';
    dword_5707E0 = 0;

    int traitCount = TRAITS_MAX_SELECTED_COUNT - 1;
    int traitIndex = 0;
    while (traitCount >= 0) {
        if (dword_570A04[traitIndex] != -1) {
            break;
        }
        traitCount--;
        traitIndex++;
    }

    dword_5709FC = TRAITS_MAX_SELECTED_COUNT - traitIndex;

    bool result = true;
    if (dword_5709FC >= 1) {
        fontSetCurrent(103);

        blitBufferToBuffer(gEditorPerkBackgroundBuffer + PERK_WINDOW_WIDTH * 14 + 49, 206, fontGetLineHeight() + 2, PERK_WINDOW_WIDTH, gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * 15 + 49, PERK_WINDOW_WIDTH);

        // LOSE A TRAIT
        char* msg = getmsg(&editorMessageList, &editorMessageListItem, 154);
        fontDrawText(gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * 16 + 49, msg, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, byte_6A38D0[18979]);

        dword_570A00 = 0;
        dword_5707A8 = 0;
        dword_5707DC = 0;
        _RedrwDMPrk();

        int rc = _InputPDLoop(dword_5709FC, _RedrwDMPrk);
        if (rc == 1) {
            if (dword_5707A8 == 0) {
                if (dword_5709FC == 1) {
                    dword_570A04[0] = -1;
                    dword_570A04[1] = -1;
                }
                else {
                    if (stru_56FCB0[0].field_0 == dword_570A04[0]) {
                        dword_570A04[0] = dword_570A04[1];
                        dword_570A04[1] = -1;
                    }
                    else {
                        dword_570A04[1] = -1;
                    }
                }
            }
            else {
                if (stru_56FCB0[0].field_0 == dword_570A04[0]) {
                    dword_570A04[1] = -1;
                }
                else {
                    dword_570A04[0] = dword_570A04[1];
                    dword_570A04[1] = -1;
                }
            }
        }
        else {
            result = false;
        }
    }

    if (result) {
        fontSetCurrent(103);

        blitBufferToBuffer(gEditorPerkBackgroundBuffer + PERK_WINDOW_WIDTH * 14 + 49, 206, fontGetLineHeight() + 2, PERK_WINDOW_WIDTH, gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * 15 + 49, PERK_WINDOW_WIDTH);

        // PICK A NEW TRAIT
        char* msg = getmsg(&editorMessageList, &editorMessageListItem, 153);
        fontDrawText(gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * 16 + 49, msg, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, byte_6A38D0[18979]);

        dword_5707A8 = 0;
        dword_5707DC = 0;
        dword_570A00 = 1;

        _RedrwDMPrk();

        int count = 16 - dword_5709FC;
        if (count > 16) {
            count = 16;
        }

        int rc = _InputPDLoop(count, _RedrwDMPrk);
        if (rc == 1) {
            if (dword_5709FC != 0) {
                dword_570A04[1] = stru_56FCB0[dword_5707A8 + dword_5707DC].field_0;
            }
            else {
                dword_570A04[0] = stru_56FCB0[dword_5707A8 + dword_5707DC].field_0;
                dword_570A04[1] = -1;
            }

            traitsSetSelected(dword_570A04[0], dword_570A04[1]);
        }
        else {
            result = false;
        }
    }

    if (!result) {
        memcpy(dword_570A04, dword_5709F0, sizeof(dword_570A04));
    }

    return result;
}

//
void _RedrwDMTagSkl()
{
    blitBufferToBuffer(gEditorPerkBackgroundBuffer + 280, 293, PERK_WINDOW_HEIGHT, PERK_WINDOW_WIDTH, gEditorPerkWindowBuffer + 280, PERK_WINDOW_WIDTH);

    _ListNewTagSkills();

    char* name = stru_56FCB0[dword_5707DC + dword_5707A8].field_4;
    char* description = skillGetDescription(stru_56FCB0[dword_5707DC + dword_5707A8].field_0);
    int frmId = skillGetFrmId(stru_56FCB0[dword_5707DC + dword_5707A8].field_0);
    _DrawCard2(frmId, name, NULL, description);

    windowRefresh(gEditorPerkWindow);
}

// 0x43D6F8
bool editorHandleTag()
{
    fontSetCurrent(103);

    blitBufferToBuffer(gEditorPerkBackgroundBuffer + 573 * 14 + 49, 206, fontGetLineHeight() + 2, 573, gEditorPerkWindowBuffer + 573 * 15 + 49, 573);

    // PICK A NEW TAG SKILL
    char* messageListItemText = getmsg(&editorMessageList, &editorMessageListItem, 155);
    fontDrawText(gEditorPerkWindowBuffer + 573 * 16 + 49, messageListItemText, 573, 573, byte_6A38D0[18979]);

    dword_5707A8 = 0;
    dword_5707DC = 0;
    dword_5709E8 = -1;
    byte_570128[0] = '\0';
    dword_5707E0 = 0;
    _RedrwDMTagSkl();

    int rc = _InputPDLoop(dword_570A00, _RedrwDMTagSkl);
    if (rc != 1) {
        memcpy(dword_570A14, dword_5709D4, sizeof(dword_570A14));
        skillsSetTagged(dword_5709D4, NUM_TAGGED_SKILLS);
        return false;
    }
    
    dword_570A14[3] = stru_56FCB0[dword_5707DC + dword_5707A8].field_0;
    skillsSetTagged(dword_570A14, NUM_TAGGED_SKILLS);

    return true;
}

void _ListNewTagSkills()
{
    blitBufferToBuffer(gEditorPerkBackgroundBuffer + PERK_WINDOW_WIDTH * 43 + 45, 192, 129, PERK_WINDOW_WIDTH, gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * 43 + 45, PERK_WINDOW_WIDTH);

    fontSetCurrent(101);

    dword_570A00 = 0;

    int y = 43;
    int yStep = fontGetLineHeight() + 2;

    for (int skill = 0; skill < SKILL_COUNT; skill++) {
        if (skill != dword_570A14[0] && skill != dword_570A14[1] && skill != dword_570A14[2] && skill != dword_570A14[3]) {
            stru_56FCB0[dword_570A00].field_0 = skill;
            stru_56FCB0[dword_570A00].field_4 = skillGetName(skill);
            dword_570A00++;
        }
    }

    qsort(stru_56FCB0, dword_570A00, sizeof(*stru_56FCB0), _name_sort_comp);

    for (int index = dword_5707DC; index < dword_5707DC + 11; index++) {
        int color;
        if (index == dword_5707A8 + dword_5707DC) {
            color = byte_6A38D0[32747];
        }
        else {
            color = byte_6A38D0[992];
        }

        fontDrawText(gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * y + 45, stru_56FCB0[index].field_4, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, color);
        y += yStep;
    }
}

int _ListMyTraits(int a1)
{
    blitBufferToBuffer(gEditorPerkBackgroundBuffer + PERK_WINDOW_WIDTH * 43 + 45, 192, 129, PERK_WINDOW_WIDTH, gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * 43 + 45, PERK_WINDOW_WIDTH);

    fontSetCurrent(101);

    int y = 43;
    int yStep = fontGetLineHeight() + 2;

    if (a1 != 0) {
        int count = 0;
        for (int trait = 0; trait < TRAIT_COUNT; trait++) {
            if (trait != dword_5709F0[0] && trait != dword_5709F0[1]) {
                stru_56FCB0[count].field_0 = trait;
                stru_56FCB0[count].field_4 = traitGetName(trait);
                count++;
            }
        }

        qsort(stru_56FCB0, count, sizeof(*stru_56FCB0), _name_sort_comp);

        for (int index = dword_5707DC; index < dword_5707DC + 11; index++) {
            int color;
            if (index == dword_5707A8 + dword_5707DC) {
                color = byte_6A38D0[32747];
            }
            else {
                color = byte_6A38D0[992];
            }

            fontDrawText(gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * y + 45, stru_56FCB0[index].field_4, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, color);
            y += yStep;
        }
    }
    else {
        // NOTE: Original code does not use loop.
        for (int index = 0; index < TRAITS_MAX_SELECTED_COUNT; index++) {
            stru_56FCB0[index].field_0 = dword_570A04[index];
            stru_56FCB0[index].field_4 = traitGetName(dword_570A04[index]);
        }

        if (dword_5709FC > 1) {
            qsort(stru_56FCB0, dword_5709FC, sizeof(*stru_56FCB0), _name_sort_comp);
        }

        for (int index = 0; index < dword_5709FC; index++) {
            int color;
            if (index == dword_5707A8) {
                color = byte_6A38D0[32747];
            }
            else {
                color = byte_6A38D0[992];
            }

            fontDrawText(gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * y + 45, stru_56FCB0[index].field_4, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, color);
            y += yStep;
        }
    }
    return 0;
}

int _name_sort_comp(const void* a1, const void* a2)
{
    STRUCT_56FCB0* v1 = (STRUCT_56FCB0*)a1;
    STRUCT_56FCB0* v2 = (STRUCT_56FCB0*)a2;
    return strcmp(v1->field_4, v2->field_4);
}

int _DrawCard2(int frmId, const char* name, const char* rank, char* description)
{
    int fid = buildFid(10, frmId, 0, 0, 0);

    CacheEntry* handle;
    int width;
    int height;
    unsigned char* data = artLockFrameDataReturningSize(fid, &handle, &width, &height);
    if (data == NULL) {
        return -1;
    }

    blitBufferToBuffer(data, width, height, width, gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * 64 + 413, PERK_WINDOW_WIDTH);

    // Calculate width of transparent pixels on the left side of the image. This
    // space will be occupied by description (in addition to fixed width).
    int extraDescriptionWidth = 150;
    for (int y = 0; y < height; y++) {
        unsigned char* stride = data;
        for (int x = 0; x < width; x++) {
            if (sub_44EBC0(*stride) < 2) {
                if (extraDescriptionWidth > x) {
                    extraDescriptionWidth = x;
                }
            }
            stride++;
        }
        data += width;
    }

    // Add gap between description and image.
    extraDescriptionWidth -= 8;
    if (extraDescriptionWidth < 0) {
        extraDescriptionWidth = 0;
    }

    fontSetCurrent(102);
    int nameHeight = fontGetLineHeight();

    fontDrawText(gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * 27 + 280, name, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, byte_6A38D0[0]);

    if (rank != NULL) {
        int rankX = fontGetStringWidth(name) + 280 + 8;
        fontSetCurrent(101);

        int rankHeight = fontGetLineHeight();
        fontDrawText(gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * (23 + nameHeight - rankHeight) + rankX, rank, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, byte_6A38D0[0]);
    }

    windowDrawLine(gEditorPerkWindow, 280, 27 + nameHeight, 545, 27 + nameHeight, byte_6A38D0[0]);
    windowDrawLine(gEditorPerkWindow, 280, 28 + nameHeight, 545, 28 + nameHeight, byte_6A38D0[0]);

    fontSetCurrent(101);

    int yStep = fontGetLineHeight() + 1;
    int y = 70;

    short beginnings[WORD_WRAP_MAX_COUNT];
    short count;
    if (wordWrap(description, 133 + extraDescriptionWidth, beginnings, &count) != 0) {
        // FIXME: Leaks handle.
        return -1;
    }

    for (int index = 0; index < count - 1; index++) {
        char* beginning = description + beginnings[index];
        char* ending = description + beginnings[index + 1];

        char ch = *ending;
        *ending = '\0';

        fontDrawText(gEditorPerkWindowBuffer + PERK_WINDOW_WIDTH * y + 280, beginning, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, byte_6A38D0[0]);

        *ending = ch;

        y += yStep;
    }

    if (frmId != dword_5709E8 || strcmp(byte_570128, name) != 0) {
        if (dword_5707E0) {
            soundPlayFile("isdxxxx1");
        }
    }

    strcpy(byte_570128, name);

    dword_5709E8 = frmId;
    dword_5707E0 = 1;
    artUnlock(handle);

    return 0;
}

// copy editor perks to character
void _pop_perks()
{
    for (int perk = 0; perk < PERK_COUNT; perk++) {
        for (;;) {
            int rank = perkGetRank(gDude, perk);
            if (rank <= dword_5707E4[perk]) {
                break;
            }

            perkRemove(gDude, perk);
        }
    }

    for (int i = 0; i < PERK_COUNT; i++) {
        for(;;) {
            int rank = perkGetRank(gDude, i);
            if (rank >= dword_5707E4[i]) {
                break;
            }

            perkAdd(gDude, i);
        }
    }
}

// validate SPECIAL stats are <= 10
int _is_supper_bonus()
{
    for (int stat = 0; stat < 7; stat++) {
        int v1 = critterGetBaseStatWithTraitModifier(gDude, stat);
        int v2 = critterGetBonusStat(gDude, stat);
        if (v1 + v2 > 10) {
            return 1;
        }
    }

    return 0;
}

//
int _folder_init()
{
    dword_5705C4 = 0;
    dword_5705D8 = 0;
    dword_5705D4 = 0;

    if (dword_518624 == -1) {
        dword_518624 = buttonCreate(characterEditorWindowHandle, 317, 364, stru_5701C0[22].width, stru_5701C0[22].height, -1, -1, -1, 17000, dword_5704E0[21], dword_5704E0[22], NULL, 32);
        if (dword_518624 == -1) {
            return -1;
        }

        buttonSetCallbacks(dword_518624, _gsound_red_butt_press, NULL);
    }

    if (dword_518628 == -1) {
        dword_518628 = buttonCreate(characterEditorWindowHandle,
            317,
            365 + stru_5701C0[22].height,
            stru_5701C0[4].width,
            stru_5701C0[4].height,
            dword_518628,
            dword_518628,
            dword_518628,
            17001,
            dword_5704E0[3],
            dword_5704E0[4],
            0,
            32);
        if (dword_518628 == -1) {
            buttonDestroy(dword_518624);
            return -1;
        }

        buttonSetCallbacks(dword_518628, _gsound_red_butt_press, NULL);
    }

    return 0;
}

void _folder_scroll(int direction)
{
    int* v1;

    switch (characterEditorWindowSelectedFolder) {
    case EDITOR_FOLDER_PERKS:
        v1 = &dword_5705D8;
        break;
    case EDITOR_FOLDER_KARMA:
        v1 = &dword_5705C4;
        break;
    case EDITOR_FOLDER_KILLS:
        v1 = &dword_5705D4;
        break;
    default:
        return;
    }

    if (direction >= 0) {
        if (dword_5705A8 + dword_5705B4 <= dword_5705AC) {
            dword_5705B4++;
            if (characterEditorSelectedItem >= 10 && characterEditorSelectedItem < 43 && characterEditorSelectedItem != 10) {
                characterEditorSelectedItem--;
            }
        }
    } else {
        if (dword_5705B4 > 0) {
            dword_5705B4--;
            if (characterEditorSelectedItem >= 10 && characterEditorSelectedItem < 43 && dword_5705A8 + 9 > characterEditorSelectedItem) {
                characterEditorSelectedItem++;
            }
        }
    }

    *v1 = dword_5705B4;
    editorRenderFolders();

    if (characterEditorSelectedItem >= 10 && characterEditorSelectedItem < 43) {
        blitBufferToBuffer(
            characterEditorWindowBackgroundBuf + 640 * 267 + 345,
            277,
            170,
            640,
            characterEditorWindowBuf + 640 * 267 + 345,
            640);
        _DrawCard(dword_5705B0, off_5705B8, off_5705BC, off_5705CC);
    }
}

//
void _folder_clear()
{
    int v0;

    dword_5705AC = 0;
    dword_5705D0 = 364;

    v0 = fontGetLineHeight();

    dword_5705A8 = 9;
    dword_5705C0 = v0 + 1;

    if (characterEditorSelectedItem < 10 || characterEditorSelectedItem >= 43)
        dword_5705C8 = -1;
    else
        dword_5705C8 = characterEditorSelectedItem - 10;

    if (characterEditorWindowSelectedFolder < 1) {
        if (characterEditorWindowSelectedFolder)
            return;

        dword_5705B4 = dword_5705D8;
    } else if (characterEditorWindowSelectedFolder == 1) {
        dword_5705B4 = dword_5705C4;
    } else if (characterEditorWindowSelectedFolder == 2) {
        dword_5705B4 = dword_5705D4;
    }
}

// render heading string with line
int _folder_print_seperator(const char* string)
{
    int lineHeight;
    int x;
    int y;
    int lineLen;
    int gap;
    int v8 = 0;

    if (dword_5705A8 + dword_5705B4 > dword_5705AC) {
        if (dword_5705AC >= dword_5705B4) {
            if (dword_5705AC - dword_5705B4 == dword_5705C8) {
                v8 = 1;
            }
            lineHeight = fontGetLineHeight();
            x = 280;
            y = dword_5705D0 + lineHeight / 2;
            if (string != NULL) {
                gap = fontGetLetterSpacing();
                // TODO: Not sure about this.
                lineLen = fontGetStringWidth(string) + gap * 4;
                x = (x - lineLen) / 2;
                fontDrawText(characterEditorWindowBuf + 640 * dword_5705D0 + 34 + x + gap * 2, string, 640, 640, byte_6A38D0[992]);
                windowDrawLine(characterEditorWindowHandle, 34 + x + lineLen, y, 34 + 280, y, byte_6A38D0[992]);
            }
            windowDrawLine(characterEditorWindowHandle, 34, y, 34 + x, y, byte_6A38D0[992]);
            dword_5705D0 += dword_5705C0;
        }
        dword_5705AC++;
        return v8;
    } else {
        return 0;
    }
}

bool _folder_print_line(const char* string)
{
    bool success = false;
    int color;

    if (dword_5705A8 + dword_5705B4 > dword_5705AC) {
        if (dword_5705AC >= dword_5705B4) {
            if (dword_5705AC - dword_5705B4 == dword_5705C8) {
                success = true;
                color = byte_6A38D0[32747];
            } else {
                color = byte_6A38D0[992];
            }

            fontDrawText(characterEditorWindowBuf + 640 * dword_5705D0 + 34, string, 640, 640, color);
            dword_5705D0 += dword_5705C0;
        }

        dword_5705AC++;
    }

    return success;
}

// 0x43E470
bool editorDrawKillsEntry(const char* name, int kills)
{
    char killsString[8];
    int color;
    int gap;

    bool success = false;
    if (dword_5705A8 + dword_5705B4 > dword_5705AC) {
        if (dword_5705AC >= dword_5705B4) {
            if (dword_5705AC - dword_5705B4 == dword_5705C8) {
                color = byte_6A38D0[32747];
                success = true;
            } else {
                color = byte_6A38D0[992];
            }

            itoa(kills, killsString, 10);
            int v6 = fontGetStringWidth(killsString);

            // TODO: Check.
            gap = fontGetLetterSpacing();
            int v11 = dword_5705D0 + fontGetLineHeight() / 2;

            fontDrawText(characterEditorWindowBuf + 640 * dword_5705D0 + 34, name, 640, 640, color);

            int v12 = fontGetStringWidth(name);
            windowDrawLine(characterEditorWindowHandle, 34 + v12 + gap, v11, 314 - v6 - gap, v11, color);

            fontDrawText(characterEditorWindowBuf + 640 * dword_5705D0 + 314 - v6, killsString, 640, 640, color);
            dword_5705D0 += dword_5705C0;
        }

        dword_5705AC++;
    }

    return success;
}

// 0x43E5C4
int karmaInit()
{
    const char* delim = " \t,";
    
    if (gKarmaEntries != NULL) {
        internal_free(gKarmaEntries);
        gKarmaEntries = NULL;
    }

    gKarmaEntriesLength = 0;

    File* stream = fileOpen("data\\karmavar.txt", "rt");
    if (stream == NULL) {
        return -1;
    }

    char string[256];
    while (fileReadString(string, 256, stream)) {
        KarmaEntry entry;

        char* pch = string;
        while (isspace(*pch & 0xFF)) {
            pch++;
        }

        if (*pch == '#') {
            continue;
        }

        char* tok = strtok(pch, delim);
        if (tok == NULL) {
            continue;
        }

        entry.gvar = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.art_num = atoi(tok);

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

        KarmaEntry* entries = internal_realloc(gKarmaEntries, sizeof(*entries) * (gKarmaEntriesLength + 1));
        if (entries == NULL) {
            fileClose(stream);

            return -1;
        }

        memcpy(&(entries[gKarmaEntriesLength]), &entry, sizeof(entry));

        gKarmaEntries = entries;
        gKarmaEntriesLength++;
    }

    qsort(gKarmaEntries, gKarmaEntriesLength, sizeof(*gKarmaEntries), karmaEntryCompare);

    fileClose(stream);

    return 0;
}

// NOTE: Inlined.
//
// 0x43E764
void karmaFree()
{
    if (gKarmaEntries != NULL) {
        internal_free(gKarmaEntries);
        gKarmaEntries = NULL;
    }

    gKarmaEntriesLength = 0;
}

// 0x43E78C
int karmaEntryCompare(const void* a1, const void* a2)
{
    KarmaEntry* v1 = (KarmaEntry*)a1;
    KarmaEntry* v2 = (KarmaEntry*)a2;
    return v1->gvar - v2->gvar;
}

// 0x43E798
int genericReputationInit()
{
    const char* delim = " \t,";
    
    if (gGenericReputationEntries != NULL) {
        internal_free(gGenericReputationEntries);
        gGenericReputationEntries = NULL;
    }

    gGenericReputationEntriesLength = 0;

    File* stream = fileOpen("data\\genrep.txt", "rt");
    if (stream == NULL) {
        return -1;
    }

    char string[256];
    while (fileReadString(string, 256, stream)) {
        GenericReputationEntry entry;

        char* pch = string;
        while (isspace(*pch & 0xFF)) {
            pch++;
        }

        if (*pch == '#') {
            continue;
        }

        char* tok = strtok(pch, delim);
        if (tok == NULL) {
            continue;
        }

        entry.threshold = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.name = atoi(tok);

        GenericReputationEntry* entries = internal_realloc(gGenericReputationEntries, sizeof(*entries) * (gGenericReputationEntriesLength + 1));
        if (entries == NULL) {
            fileClose(stream);

            return -1;
        }

        memcpy(&(entries[gGenericReputationEntriesLength]), &entry, sizeof(entry));

        gGenericReputationEntries = entries;
        gGenericReputationEntriesLength++;
    }

    qsort(gGenericReputationEntries, gGenericReputationEntriesLength, sizeof(*gGenericReputationEntries), genericReputationCompare);

    fileClose(stream);

    return 0;
}

// NOTE: Inlined.
//
// 0x43E914
void genericReputationFree()
{
    if (gGenericReputationEntries != NULL) {
        internal_free(gGenericReputationEntries);
        gGenericReputationEntries = NULL;
    }

    gGenericReputationEntriesLength = 0;
}

// 0x43E93C
int genericReputationCompare(const void* a1, const void* a2)
{
    GenericReputationEntry* v1 = (GenericReputationEntry*)a1;
    GenericReputationEntry* v2 = (GenericReputationEntry*)a2;

    if (v2->threshold > v1->threshold) {
        return 1;
    } else if (v2->threshold < v1->threshold) {
        return -1;
    }
    return 0;
}
