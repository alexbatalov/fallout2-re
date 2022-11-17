#include "game/editor.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "game/art.h"
#include "plib/color/color.h"
#include "plib/gnw/input.h"
#include "game/critter.h"
#include "game/cycle.h"
#include "plib/db/db.h"
#include "game/bmpdlog.h"
#include "plib/gnw/button.h"
#include "plib/gnw/debug.h"
#include "plib/gnw/grbuf.h"
#include "game/game.h"
#include "game/gmouse.h"
#include "game/graphlib.h"
#include "game/gsound.h"
#include "plib/gnw/rect.h"
#include "game/intface.h"
#include "game/item.h"
#include "game/map.h"
#include "plib/gnw/memory.h"
#include "game/message.h"
#include "game/object.h"
#include "game/palette.h"
#include "game/perk.h"
#include "game/proto.h"
#include "game/scripts.h"
#include "game/skill.h"
#include "game/stat.h"
#include "plib/gnw/text.h"
#include "game/trait.h"
#include "plib/gnw/gnw.h"
#include "game/wordwrap.h"
#include "game/worldmap.h"

#define RENDER_ALL_STATS 7

#define EDITOR_WINDOW_X 0
#define EDITOR_WINDOW_Y 0
#define EDITOR_WINDOW_WIDTH 640
#define EDITOR_WINDOW_HEIGHT 480

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

#define PERK_WINDOW_LIST_X 45
#define PERK_WINDOW_LIST_Y 43
#define PERK_WINDOW_LIST_WIDTH 192
#define PERK_WINDOW_LIST_HEIGHT 129

#define ANIMATE 0x01
#define RED_NUMBERS 0x02
#define BIG_NUM_WIDTH 14
#define BIG_NUM_HEIGHT 24
#define BIG_NUM_ANIMATION_DELAY 123

// TODO: Should be MAX(PERK_COUNT, TRAIT_COUNT).
#define DIALOG_PICKER_NUM_OPTIONS PERK_COUNT

typedef enum EditorFolder {
    EDITOR_FOLDER_PERKS,
    EDITOR_FOLDER_KARMA,
    EDITOR_FOLDER_KILLS,
} EditorFolder;

enum {
    EDITOR_DERIVED_STAT_ARMOR_CLASS,
    EDITOR_DERIVED_STAT_ACTION_POINTS,
    EDITOR_DERIVED_STAT_CARRY_WEIGHT,
    EDITOR_DERIVED_STAT_MELEE_DAMAGE,
    EDITOR_DERIVED_STAT_DAMAGE_RESISTANCE,
    EDITOR_DERIVED_STAT_POISON_RESISTANCE,
    EDITOR_DERIVED_STAT_RADIATION_RESISTANCE,
    EDITOR_DERIVED_STAT_SEQUENCE,
    EDITOR_DERIVED_STAT_HEALING_RATE,
    EDITOR_DERIVED_STAT_CRITICAL_CHANCE,
    EDITOR_DERIVED_STAT_COUNT,
};

enum {
    EDITOR_FIRST_PRIMARY_STAT,
    EDITOR_HIT_POINTS = 43,
    EDITOR_POISONED,
    EDITOR_RADIATED,
    EDITOR_EYE_DAMAGE,
    EDITOR_CRIPPLED_RIGHT_ARM,
    EDITOR_CRIPPLED_LEFT_ARM,
    EDITOR_CRIPPLED_RIGHT_LEG,
    EDITOR_CRIPPLED_LEFT_LEG,
    EDITOR_FIRST_DERIVED_STAT,
    EDITOR_FIRST_SKILL = EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_COUNT,
    EDITOR_TAG_SKILL = EDITOR_FIRST_SKILL + SKILL_COUNT,
    EDITOR_SKILLS,
    EDITOR_OPTIONAL_TRAITS,
    EDITOR_FIRST_TRAIT,
    EDITOR_BUTTONS_COUNT = EDITOR_FIRST_TRAIT + TRAIT_COUNT,
};

enum {
    EDITOR_GRAPHIC_BIG_NUMBERS,
    EDITOR_GRAPHIC_AGE_MASK,
    EDITOR_GRAPHIC_AGE_OFF,
    EDITOR_GRAPHIC_DOWN_ARROW_OFF,
    EDITOR_GRAPHIC_DOWN_ARROW_ON,
    EDITOR_GRAPHIC_NAME_MASK,
    EDITOR_GRAPHIC_NAME_ON,
    EDITOR_GRAPHIC_NAME_OFF,
    EDITOR_GRAPHIC_FOLDER_MASK, // mask for all three folders
    EDITOR_GRAPHIC_SEX_MASK,
    EDITOR_GRAPHIC_SEX_OFF,
    EDITOR_GRAPHIC_SEX_ON,
    EDITOR_GRAPHIC_SLIDER, // image containing small plus/minus buttons appeared near selected skill
    EDITOR_GRAPHIC_SLIDER_MINUS_OFF,
    EDITOR_GRAPHIC_SLIDER_MINUS_ON,
    EDITOR_GRAPHIC_SLIDER_PLUS_OFF,
    EDITOR_GRAPHIC_SLIDER_PLUS_ON,
    EDITOR_GRAPHIC_SLIDER_TRANS_MINUS_OFF,
    EDITOR_GRAPHIC_SLIDER_TRANS_MINUS_ON,
    EDITOR_GRAPHIC_SLIDER_TRANS_PLUS_OFF,
    EDITOR_GRAPHIC_SLIDER_TRANS_PLUS_ON,
    EDITOR_GRAPHIC_UP_ARROW_OFF,
    EDITOR_GRAPHIC_UP_ARROW_ON,
    EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP,
    EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN,
    EDITOR_GRAPHIC_AGE_ON,
    EDITOR_GRAPHIC_AGE_BOX, // image containing right and left buttons with age stepper in the middle
    EDITOR_GRAPHIC_ATTRIBOX, // ??? black image with two little arrows (up and down) in the right-top corner
    EDITOR_GRAPHIC_ATTRIBWN, // ??? not sure where and when it's used
    EDITOR_GRAPHIC_CHARWIN, // ??? looks like metal plate
    EDITOR_GRAPHIC_DONE_BOX, // metal plate holding DONE button
    EDITOR_GRAPHIC_FEMALE_OFF,
    EDITOR_GRAPHIC_FEMALE_ON,
    EDITOR_GRAPHIC_MALE_OFF,
    EDITOR_GRAPHIC_MALE_ON,
    EDITOR_GRAPHIC_NAME_BOX, // placeholder for name
    EDITOR_GRAPHIC_LEFT_ARROW_UP,
    EDITOR_GRAPHIC_LEFT_ARROW_DOWN,
    EDITOR_GRAPHIC_RIGHT_ARROW_UP,
    EDITOR_GRAPHIC_RIGHT_ARROW_DOWN,
    EDITOR_GRAPHIC_BARARRWS, // ??? two arrows up/down with some strange knob at the top, probably for scrollbar
    EDITOR_GRAPHIC_OPTIONS_BASE, // options metal plate
    EDITOR_GRAPHIC_OPTIONS_BUTTON_OFF,
    EDITOR_GRAPHIC_OPTIONS_BUTTON_ON,
    EDITOR_GRAPHIC_KARMA_FOLDER_SELECTED, // all three folders with middle folder selected (karma)
    EDITOR_GRAPHIC_KILLS_FOLDER_SELECTED, // all theee folders with right folder selected (kills)
    EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED, // all three folders with left folder selected (perks)
    EDITOR_GRAPHIC_KARMAFDR_PLACEOLDER, // ??? placeholder for traits folder image <- this is comment from intrface.lst
    EDITOR_GRAPHIC_TAG_SKILL_BUTTON_OFF,
    EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON,
    EDITOR_GRAPHIC_COUNT,
};

typedef struct KarmaEntry {
    int gvar;
    int art_num;
    int name;
    int description;
} KarmaEntry;

typedef struct GenericReputationEntry {
    int threshold;
    int name;
} GenericReputationEntry;

typedef struct PerkDialogOption {
    // Depending on the current mode this value is the id of either
    // perk, trait (handling Mutate perk), or skill (handling Tag perk).
    int value;
    char* name;
} PerkDialogOption;

// TODO: Field order is probably wrong.
typedef struct KillInfo {
    const char* name;
    int killTypeId;
    int kills;
} KillInfo;

static int CharEditStart();
static void CharEditEnd();
static void RstrBckgProc();
static void DrawFolder();
static void list_perks();
static int kills_list_comp(const void* a1, const void* a2);
static int ListKills();
static void PrintBigNum(int x, int y, int flags, int value, int previousValue, int windowHandle);
static void PrintLevelWin();
static void PrintBasicStat(int stat, bool animate, int previousValue);
static void PrintGender();
static void PrintAgeBig();
static void PrintBigname();
static void ListDrvdStats();
static void ListSkills(int a1);
static void DrawInfoWin();
static int NameWindow();
static void PrintName(unsigned char* buf, int pitch);
static int AgeWindow();
static void SexWindow();
static void StatButton(int eventCode);
static int OptionWindow();
static int Save_as_ASCII(const char* fileName);
static char* AddDots(char* string, int length);
static void ResetScreen();
static void RegInfoAreas();
static int CheckValidPlayer();
static void SavePlayer();
static void RestorePlayer();
static int DrawCard(int graphicId, const char* name, const char* attributes, char* description);
static void FldrButton();
static void InfoButton(int eventCode);
static void SliderBtn(int a1);
static int tagskl_free();
static void TagSkillSelect(int skill);
static void ListTraits();
static int get_trait_count();
static void TraitSelect(int trait);
static void list_karma();
static int UpdateLevel();
static void RedrwDPrks();
static int perks_dialog();
static int InputPDLoop(int count, void (*refreshProc)());
static int ListDPerks();
static bool GetMutateTrait();
static void RedrwDMTagSkl();
static bool Add4thTagSkill();
static void ListNewTagSkills();
static int ListMyTraits(int a1);
static int name_sort_comp(const void* a1, const void* a2);
static int DrawCard2(int frmId, const char* name, const char* rank, char* description);
static void push_perks();
static void pop_perks();
static int PerkCount();
static int is_supper_bonus();
static int folder_init();
static void folder_exit();
static void folder_scroll(int direction);
static void folder_clear();
static int folder_print_seperator(const char* string);
static bool folder_print_line(const char* string);
static bool folder_print_kill(const char* name, int kills);
static int karma_vars_init();
static void karma_vars_exit();
static int karma_vars_qsort_compare(const void* a1, const void* a2);
static int general_reps_init();
static void general_reps_exit();
static int general_reps_qsort_compare(const void* a1, const void* a2);

// 0x431C40
static const int grph_id[EDITOR_GRAPHIC_COUNT] = {
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
//
// 0x431D08
static const unsigned char copyflag[EDITOR_GRAPHIC_COUNT] = {
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
//
// 0x431D3A
static const short ndrvd[EDITOR_DERIVED_STAT_COUNT] = {
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
//
// 0x431D50
static const int StatYpos[7] = {
    37,
    70,
    103,
    136,
    169,
    202,
    235,
};

// stat ids for derived stats panel
//
// 0x431D6
static const short ndinfoxlt[EDITOR_DERIVED_STAT_COUNT] = {
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

// TODO: Remove.
// 0x431D93
char byte_431D93[64];

// TODO: Remove
// 0x5016E4
char byte_5016E4[] = "------";

// 0x518528
static bool bk_enable = false;

// 0x51852C
static int skill_cursor = 0;

// 0x518534
static int slider_y = 27;

// 0x518538
int character_points = 0;

// 0x51853C
static KarmaEntry* karma_vars = NULL;

// 0x518540
static int karma_vars_count = 0;

// 0x518544
static GenericReputationEntry* general_reps = NULL;

// 0x518548
static int general_reps_count = 0;

// 0x51854C
TownReputationEntry town_rep_info[TOWN_REPUTATION_COUNT] = {
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
int addiction_vars[ADDICTION_REPUTATION_COUNT] = {
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
int addiction_pics[ADDICTION_REPUTATION_COUNT] = {
    142,
    126,
    140,
    144,
    145,
    52,
    136,
    149,
};

// 0x518624
static int folder_up_button = -1;

// 0x518628
static int folder_down_button = -1;

// 0x56FB60
static char folder_card_string[256];

// 0x56FC60
static int skillsav[SKILL_COUNT];

// 0x56FCA8
static MessageList editor_message_file;

// 0x56FCB0
static PerkDialogOption name_sort_list[DIALOG_PICKER_NUM_OPTIONS];

// buttons for selecting traits
//
// 0x5700A8
static int trait_bids[TRAIT_COUNT];

// 0x5700E8
static MessageListItem mesg;

// 0x5700F8
static char old_str1[48];

// 0x570128
static char old_str2[48];

// buttons for tagging skills
//
// 0x570158
static int tag_bids[SKILL_COUNT];

// pc name
//
// 0x5701A0
static char name_save[32];

// 0x5701C0
static Size GInfo[EDITOR_GRAPHIC_COUNT];

// 0x570350
static CacheEntry* grph_key[EDITOR_GRAPHIC_COUNT];

// 0x570418
static unsigned char* grphcpy[EDITOR_GRAPHIC_COUNT];

// 0x5704E0
static unsigned char* grphbmp[EDITOR_GRAPHIC_COUNT];

// 0x5705A8
static int folder_max_lines;

// 0x5705AC
static int folder_line;

// 0x5705B0
static int folder_card_fid;

// 0x5705B4
static int folder_top_line;

// 0x5705B8
static char* folder_card_title;

// 0x5705BC
static char* folder_card_title2;

// 0x5705C0
static int folder_yoffset;

// 0x5705C4
static int folder_karma_top_line;

// 0x5705C8
static int folder_highlight_line;

// 0x5705CC
static char* folder_card_desc;

// 0x5705D0
static int folder_ypos;

// 0x5705D4
static int folder_kills_top_line;

// 0x5705D8
static int folder_perk_top_line;

// 0x5705DC
static unsigned char* pbckgnd;

// 0x5705E0
static int pwin;

// 0x5705E4
static int SliderPlusID;

// 0x5705E8
static int SliderNegID;

// - stats buttons
//
// 0x5705EC
static int stat_bids_minus[7];

// 0x570608
static unsigned char* win_buf;

// 0x57060C
static int edit_win;

// + stats buttons
//
// 0x570610
static int stat_bids_plus[7];

// 0x57062C
static unsigned char* pwin_buf;

// 0x570630
static CritterProtoData dude_data;

// 0x5707A4
static unsigned char* bckgnd;

// 0x5707A8
static int cline;

// 0x5707AC
static int oldsline;

// unspent skill points
//
// 0x5707B0
static int upsent_points_back;

// 0x5707B4
static int last_level;

// 0x5707B8
static int fontsave;

// 0x5707BC
static int kills_count;

// character editor background
//
// 0x5707C0
static CacheEntry* bck_key;

// current hit points
//
// 0x5707C4
static int hp_back;

// 0x5707C8
static int mouse_ypos; // mouse y

// 0x5707CC
static int mouse_xpos; // mouse x

// 0x5707D0
static int info_line;

// 0x5707D4
static int folder;

// 0x5707D8
static bool frstc_draw1;

// 0x5707DC
static int crow;

// 0x5707E0
static bool frstc_draw2;

// 0x5707E4
static int perk_back[PERK_COUNT];

// 0x5709C0
static unsigned int _repFtime;

// 0x5709C4
static unsigned int _frame_time;

// 0x5709C8
static int old_tags;

// 0x5709CC
static int last_level_back;

// 0x5709E8
static int old_fid2;

// 0x5709EC
static int old_fid1;

// 0x5709D0
static bool glblmode;

// 0x5709D4
static int tag_skill_back[NUM_TAGGED_SKILLS];

// 0x5709F0
static int trait_back[3];

// current index for selecting new trait
//
// 0x5709FC
static int trait_count;

// 0x570A00
static int optrt_count;

// 0x570A04
static int temp_trait[3];

// 0x570A10
static int tagskill_count;

// 0x570A14
static int temp_tag_skill[NUM_TAGGED_SKILLS];

// 0x570A28
static char free_perk_back;

// 0x570A29
static unsigned char free_perk;

// 0x570A2A
static unsigned char first_skill_list;

// 0x431DF8
int editor_design(bool isCreationMode)
{
    char* messageListItemText;
    char line1[128];
    char line2[128];
    const char* lines[] = { line2 };

    glblmode = isCreationMode;

    SavePlayer();

    if (CharEditStart() == -1) {
        debug_printf("\n ** Error loading character editor data! **\n");
        return -1;
    }

    if (!glblmode) {
        if (UpdateLevel()) {
            stat_recalc_derived(obj_dude);
            ListTraits();
            ListSkills(0);
            PrintBasicStat(RENDER_ALL_STATS, 0, 0);
            ListDrvdStats();
            DrawInfoWin();
        }
    }

    int rc = -1;
    while (rc == -1) {
        _frame_time = get_time();
        int keyCode = get_input();

        bool done = false;
        if (keyCode == 500) {
            done = true;
        }

        if (keyCode == KEY_RETURN || keyCode == KEY_UPPERCASE_D || keyCode == KEY_LOWERCASE_D) {
            done = true;
            gsound_play_sfx_file("ib1p1xx1");
        }

        if (done) {
            if (glblmode) {
                if (character_points != 0) {
                    gsound_play_sfx_file("iisxxxx1");

                    // You must use all character points
                    messageListItemText = getmsg(&editor_message_file, &mesg, 118);
                    strcpy(line1, messageListItemText);

                    // before starting the game!
                    messageListItemText = getmsg(&editor_message_file, &mesg, 119);
                    strcpy(line2, messageListItemText);

                    dialog_out(line1, lines, 1, 192, 126, colorTable[32328], 0, colorTable[32328], 0);
                    win_draw(edit_win);

                    rc = -1;
                    continue;
                }

                if (tagskill_count > 0) {
                    gsound_play_sfx_file("iisxxxx1");

                    // You must select all tag skills
                    messageListItemText = getmsg(&editor_message_file, &mesg, 142);
                    strcpy(line1, messageListItemText);

                    // before starting the game!
                    messageListItemText = getmsg(&editor_message_file, &mesg, 143);
                    strcpy(line2, messageListItemText);

                    dialog_out(line1, lines, 1, 192, 126, colorTable[32328], 0, colorTable[32328], 0);
                    win_draw(edit_win);

                    rc = -1;
                    continue;
                }

                if (is_supper_bonus()) {
                    gsound_play_sfx_file("iisxxxx1");

                    // All stats must be between 1 and 10
                    messageListItemText = getmsg(&editor_message_file, &mesg, 157);
                    strcpy(line1, messageListItemText);

                    // before starting the game!
                    messageListItemText = getmsg(&editor_message_file, &mesg, 158);
                    strcpy(line2, messageListItemText);

                    dialog_out(line1, lines, 1, 192, 126, colorTable[32328], 0, colorTable[32328], 0);
                    win_draw(edit_win);

                    rc = -1;
                    continue;
                }

                if (stricmp(critter_name(obj_dude), "None") == 0) {
                    gsound_play_sfx_file("iisxxxx1");

                    // Warning: You haven't changed your player
                    messageListItemText = getmsg(&editor_message_file, &mesg, 160);
                    strcpy(line1, messageListItemText);

                    // name. Use this character any way?
                    messageListItemText = getmsg(&editor_message_file, &mesg, 161);
                    strcpy(line2, messageListItemText);

                    if (dialog_out(line1, lines, 1, 192, 126, colorTable[32328], 0, colorTable[32328], DIALOG_BOX_YES_NO) == 0) {
                        win_draw(edit_win);

                        rc = -1;
                        continue;
                    }
                }
            }

            win_draw(edit_win);
            rc = 0;
        } else if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
            game_quit_with_confirm();
            win_draw(edit_win);
        } else if (keyCode == 502 || keyCode == KEY_ESCAPE || keyCode == KEY_UPPERCASE_C || keyCode == KEY_LOWERCASE_C || game_user_wants_to_quit != 0) {
            win_draw(edit_win);
            rc = 1;
        } else if (glblmode && (keyCode == 517 || keyCode == KEY_UPPERCASE_N || keyCode == KEY_LOWERCASE_N)) {
            NameWindow();
            win_draw(edit_win);
        } else if (glblmode && (keyCode == 519 || keyCode == KEY_UPPERCASE_A || keyCode == KEY_LOWERCASE_A)) {
            AgeWindow();
            win_draw(edit_win);
        } else if (glblmode && (keyCode == 520 || keyCode == KEY_UPPERCASE_S || keyCode == KEY_LOWERCASE_S)) {
            SexWindow();
            win_draw(edit_win);
        } else if (glblmode && (keyCode >= 503 && keyCode < 517)) {
            StatButton(keyCode);
            win_draw(edit_win);
        } else if ((glblmode && (keyCode == 501 || keyCode == KEY_UPPERCASE_O || keyCode == KEY_LOWERCASE_O))
            || (!glblmode && (keyCode == 501 || keyCode == KEY_UPPERCASE_P || keyCode == KEY_LOWERCASE_P))) {
            OptionWindow();
            win_draw(edit_win);
        } else if (keyCode >= 525 && keyCode < 535) {
            InfoButton(keyCode);
            win_draw(edit_win);
        } else {
            switch (keyCode) {
            case KEY_TAB:
                if (info_line >= 0 && info_line < 7) {
                    info_line = glblmode ? 82 : 7;
                } else if (info_line >= 7 && info_line < 9) {
                    if (glblmode) {
                        info_line = 82;
                    } else {
                        info_line = 10;
                        folder = 0;
                    }
                } else if (info_line >= 10 && info_line < 43) {
                    switch (folder) {
                    case EDITOR_FOLDER_PERKS:
                        info_line = 10;
                        folder = EDITOR_FOLDER_KARMA;
                        break;
                    case EDITOR_FOLDER_KARMA:
                        info_line = 10;
                        folder = EDITOR_FOLDER_KILLS;
                        break;
                    case EDITOR_FOLDER_KILLS:
                        info_line = 43;
                        break;
                    }
                } else if (info_line >= 43 && info_line < 51) {
                    info_line = 51;
                } else if (info_line >= 51 && info_line < 61) {
                    info_line = 61;
                } else if (info_line >= 61 && info_line < 82) {
                    info_line = 0;
                } else if (info_line >= 82 && info_line < 98) {
                    info_line = 43;
                }
                PrintBasicStat(RENDER_ALL_STATS, 0, 0);
                ListTraits();
                ListSkills(0);
                PrintLevelWin();
                DrawFolder();
                ListDrvdStats();
                DrawInfoWin();
                win_draw(edit_win);
                break;
            case KEY_ARROW_LEFT:
            case KEY_MINUS:
            case KEY_UPPERCASE_J:
                if (info_line >= 0 && info_line < 7) {
                    if (glblmode) {
                        win_button_press_and_release(stat_bids_minus[info_line]);
                        win_draw(edit_win);
                    }
                } else if (info_line >= 61 && info_line < 79) {
                    if (glblmode) {
                        win_button_press_and_release(tag_bids[glblmode - 61]);
                        win_draw(edit_win);
                    } else {
                        SliderBtn(keyCode);
                        win_draw(edit_win);
                    }
                } else if (info_line >= 82 && info_line < 98) {
                    if (glblmode) {
                        win_button_press_and_release(trait_bids[glblmode - 82]);
                        win_draw(edit_win);
                    }
                }
                break;
            case KEY_ARROW_RIGHT:
            case KEY_PLUS:
            case KEY_UPPERCASE_N:
                if (info_line >= 0 && info_line < 7) {
                    if (glblmode) {
                        win_button_press_and_release(stat_bids_plus[info_line]);
                        win_draw(edit_win);
                    }
                } else if (info_line >= 61 && info_line < 79) {
                    if (glblmode) {
                        win_button_press_and_release(tag_bids[glblmode - 61]);
                        win_draw(edit_win);
                    } else {
                        SliderBtn(keyCode);
                        win_draw(edit_win);
                    }
                } else if (info_line >= 82 && info_line < 98) {
                    if (glblmode) {
                        win_button_press_and_release(trait_bids[glblmode - 82]);
                        win_draw(edit_win);
                    }
                }
                break;
            case KEY_ARROW_UP:
                if (info_line >= 10 && info_line < 43) {
                    if (info_line == 10) {
                        if (folder_top_line > 0) {
                            folder_scroll(-1);
                            info_line--;
                            DrawFolder();
                            DrawInfoWin();
                        }
                    } else {
                        info_line--;
                        DrawFolder();
                        DrawInfoWin();
                    }

                    win_draw(edit_win);
                } else {
                    switch (info_line) {
                    case 0:
                        info_line = 6;
                        break;
                    case 7:
                        info_line = 9;
                        break;
                    case 43:
                        info_line = 50;
                        break;
                    case 51:
                        info_line = 60;
                        break;
                    case 61:
                        info_line = 78;
                        break;
                    case 82:
                        info_line = 97;
                        break;
                    default:
                        info_line -= 1;
                        break;
                    }

                    if (info_line >= 61 && info_line < 79) {
                        skill_cursor = info_line - 61;
                    }

                    PrintBasicStat(RENDER_ALL_STATS, 0, 0);
                    ListTraits();
                    ListSkills(0);
                    PrintLevelWin();
                    DrawFolder();
                    ListDrvdStats();
                    DrawInfoWin();
                    win_draw(edit_win);
                }
                break;
            case KEY_ARROW_DOWN:
                if (info_line >= 10 && info_line < 43) {
                    if (info_line - 10 < folder_line - folder_top_line) {
                        if (info_line - 10 == folder_max_lines - 1) {
                            folder_scroll(1);
                        }

                        info_line++;

                        DrawFolder();
                        DrawInfoWin();
                    }

                    win_draw(edit_win);
                } else {
                    switch (info_line) {
                    case 6:
                        info_line = 0;
                        break;
                    case 9:
                        info_line = 7;
                        break;
                    case 50:
                        info_line = 43;
                        break;
                    case 60:
                        info_line = 51;
                        break;
                    case 78:
                        info_line = 61;
                        break;
                    case 97:
                        info_line = 82;
                        break;
                    default:
                        info_line += 1;
                        break;
                    }

                    if (info_line >= 61 && info_line < 79) {
                        skill_cursor = info_line - 61;
                    }

                    PrintBasicStat(RENDER_ALL_STATS, 0, 0);
                    ListTraits();
                    ListSkills(0);
                    PrintLevelWin();
                    DrawFolder();
                    ListDrvdStats();
                    DrawInfoWin();
                    win_draw(edit_win);
                }
                break;
            case 521:
            case 523:
                SliderBtn(keyCode);
                win_draw(edit_win);
                break;
            case 535:
                FldrButton();
                win_draw(edit_win);
                break;
            case 17000:
                folder_scroll(-1);
                win_draw(edit_win);
                break;
            case 17001:
                folder_scroll(1);
                win_draw(edit_win);
                break;
            default:
                if (glblmode && (keyCode >= 536 && keyCode < 554)) {
                    TagSkillSelect(keyCode - 536);
                    win_draw(edit_win);
                } else if (glblmode && (keyCode >= 555 && keyCode < 571)) {
                    TraitSelect(keyCode - 555);
                    win_draw(edit_win);
                } else {
                    if (keyCode == 390) {
                        dump_screen();
                    }

                    win_draw(edit_win);
                }
            }
        }
    }

    if (rc == 0) {
        if (isCreationMode) {
            proto_dude_update_gender();
            palette_fade_to(black_palette);
        }
    }

    CharEditEnd();

    if (rc == 1) {
        RestorePlayer();
    }

    if (is_pc_flag(DUDE_STATE_LEVEL_UP_AVAILABLE)) {
        pc_flag_off(DUDE_STATE_LEVEL_UP_AVAILABLE);
    }

    intface_update_hit_points(false);

    return rc;
}

// 0x4329EC
static int CharEditStart()
{
    int i;
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

    fontsave = text_curr();
    old_tags = 0;
    bk_enable = 0;
    old_fid2 = -1;
    old_fid1 = -1;
    frstc_draw2 = false;
    frstc_draw1 = false;
    first_skill_list = 1;
    old_str2[0] = '\0';
    old_str1[0] = '\0';

    text_font(101);

    slider_y = skill_cursor * (text_height() + 1) + 27;

    // skills
    skill_get_tags(temp_tag_skill, NUM_TAGGED_SKILLS);

    // NOTE: Uninline.
    tagskill_count = tagskl_free();

    // traits
    trait_get(&(temp_trait[0]), &(temp_trait[1]));

    // NOTE: Uninline.
    trait_count = get_trait_count();

    if (!glblmode) {
        bk_enable = map_disable_bk_processes();
    }

    cycle_disable();
    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    if (!message_init(&editor_message_file)) {
        return -1;
    }

    sprintf(path, "%s%s", msg_path, "editor.msg");

    if (!message_load(&editor_message_file, path)) {
        return -1;
    }

    fid = art_id(OBJ_TYPE_INTERFACE, (glblmode ? 169 : 177), 0, 0, 0);
    bckgnd = art_lock(fid, &bck_key, &(GInfo[0].width), &(GInfo[0].height));
    if (bckgnd == NULL) {
        message_exit(&editor_message_file);
        return -1;
    }

    if (karma_vars_init() == -1) {
        return -1;
    }

    if (general_reps_init() == -1) {
        return -1;
    }

    soundContinueAll();

    for (i = 0; i < EDITOR_GRAPHIC_COUNT; i++) {
        fid = art_id(OBJ_TYPE_INTERFACE, grph_id[i], 0, 0, 0);
        grphbmp[i] = art_lock(fid, &(grph_key[i]), &(GInfo[i].width), &(GInfo[i].height));
        if (grphbmp[i] == NULL) {
            break;
        }
    }

    if (i != EDITOR_GRAPHIC_COUNT) {
        while (--i >= 0) {
            art_ptr_unlock(grph_key[i]);
        }
        return -1;

        art_ptr_unlock(bck_key);

        message_exit(&editor_message_file);

        // NOTE: Uninline.
        RstrBckgProc();

        return -1;
    }

    soundContinueAll();

    for (i = 0; i < EDITOR_GRAPHIC_COUNT; i++) {
        if (copyflag[i]) {
            grphcpy[i] = (unsigned char*)mem_malloc(GInfo[i].width * GInfo[i].height);
            if (grphcpy[i] == NULL) {
                break;
            }
            memcpy(grphcpy[i], grphbmp[i], GInfo[i].width * GInfo[i].height);
        } else {
            grphcpy[i] = (unsigned char*)-1;
        }
    }

    if (i != EDITOR_GRAPHIC_COUNT) {
        while (--i >= 0) {
            if (copyflag[i]) {
                mem_free(grphcpy[i]);
            }
        }

        for (i = 0; i < EDITOR_GRAPHIC_COUNT; i++) {
            art_ptr_unlock(grph_key[i]);
        }

        art_ptr_unlock(bck_key);

        message_exit(&editor_message_file);

        // NOTE: Uninline.
        RstrBckgProc();

        return -1;
    }

    int editorWindowX = EDITOR_WINDOW_X;
    int editorWindowY = EDITOR_WINDOW_Y;
    edit_win = win_add(editorWindowX,
        editorWindowY,
        EDITOR_WINDOW_WIDTH,
        EDITOR_WINDOW_HEIGHT,
        256,
        WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);
    if (edit_win == -1) {
        for (i = 0; i < EDITOR_GRAPHIC_COUNT; i++) {
            if (copyflag[i]) {
                mem_free(grphcpy[i]);
            }
            art_ptr_unlock(grph_key[i]);
        }

        art_ptr_unlock(bck_key);

        message_exit(&editor_message_file);

        // NOTE: Uninline.
        RstrBckgProc();

        return -1;
    }

    win_buf = win_get_buf(edit_win);
    memcpy(win_buf, bckgnd, 640 * 480);

    if (glblmode) {
        text_font(103);

        // CHAR POINTS
        str = getmsg(&editor_message_file, &mesg, 116);
        text_to_buf(win_buf + (286 * 640) + 14, str, 640, 640, colorTable[18979]);
        PrintBigNum(126, 282, 0, character_points, 0, edit_win);

        // OPTIONS
        str = getmsg(&editor_message_file, &mesg, 101);
        text_to_buf(win_buf + (454 * 640) + 363, str, 640, 640, colorTable[18979]);

        // OPTIONAL TRAITS
        str = getmsg(&editor_message_file, &mesg, 139);
        text_to_buf(win_buf + (326 * 640) + 52, str, 640, 640, colorTable[18979]);
        PrintBigNum(522, 228, 0, optrt_count, 0, edit_win);

        // TAG SKILLS
        str = getmsg(&editor_message_file, &mesg, 138);
        text_to_buf(win_buf + (233 * 640) + 422, str, 640, 640, colorTable[18979]);
        PrintBigNum(522, 228, 0, tagskill_count, 0, edit_win);
    } else {
        text_font(103);

        str = getmsg(&editor_message_file, &mesg, 109);
        strcpy(perks, str);

        str = getmsg(&editor_message_file, &mesg, 110);
        strcpy(karma, str);

        str = getmsg(&editor_message_file, &mesg, 111);
        strcpy(kills, str);

        // perks selected
        len = text_width(perks);
        text_to_buf(
            grphcpy[46] + 5 * GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 61 - len / 2,
            perks,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            colorTable[18979]);

        len = text_width(karma);
        text_to_buf(grphcpy[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED] + 5 * GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 159 - len / 2,
            karma,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            colorTable[14723]);

        len = text_width(kills);
        text_to_buf(grphcpy[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED] + 5 * GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 257 - len / 2,
            kills,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            colorTable[14723]);

        // karma selected
        len = text_width(perks);
        text_to_buf(grphcpy[EDITOR_GRAPHIC_KARMA_FOLDER_SELECTED] + 5 * GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 61 - len / 2,
            perks,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            colorTable[14723]);

        len = text_width(karma);
        text_to_buf(grphcpy[EDITOR_GRAPHIC_KARMA_FOLDER_SELECTED] + 5 * GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 159 - len / 2,
            karma,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            colorTable[18979]);

        len = text_width(kills);
        text_to_buf(grphcpy[EDITOR_GRAPHIC_KARMA_FOLDER_SELECTED] + 5 * GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 257 - len / 2,
            kills,
            GInfo[46].width,
            GInfo[46].width,
            colorTable[14723]);

        // kills selected
        len = text_width(perks);
        text_to_buf(grphcpy[EDITOR_GRAPHIC_KILLS_FOLDER_SELECTED] + 5 * GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 61 - len / 2,
            perks,
            GInfo[46].width,
            GInfo[46].width,
            colorTable[14723]);

        len = text_width(karma);
        text_to_buf(grphcpy[EDITOR_GRAPHIC_KILLS_FOLDER_SELECTED] + 5 * GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 159 - len / 2,
            karma,
            GInfo[46].width,
            GInfo[46].width,
            colorTable[14723]);

        len = text_width(kills);
        text_to_buf(grphcpy[EDITOR_GRAPHIC_KILLS_FOLDER_SELECTED] + 5 * GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width + 257 - len / 2,
            kills,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            colorTable[18979]);

        DrawFolder();

        text_font(103);

        // PRINT
        str = getmsg(&editor_message_file, &mesg, 103);
        text_to_buf(win_buf + (EDITOR_WINDOW_WIDTH * PRINT_BTN_Y) + PRINT_BTN_X, str, EDITOR_WINDOW_WIDTH, EDITOR_WINDOW_WIDTH, colorTable[18979]);

        PrintLevelWin();
        folder_init();
    }

    text_font(103);

    // CANCEL
    str = getmsg(&editor_message_file, &mesg, 102);
    text_to_buf(win_buf + (EDITOR_WINDOW_WIDTH * CANCEL_BTN_Y) + CANCEL_BTN_X, str, EDITOR_WINDOW_WIDTH, EDITOR_WINDOW_WIDTH, colorTable[18979]);

    // DONE
    str = getmsg(&editor_message_file, &mesg, 100);
    text_to_buf(win_buf + (EDITOR_WINDOW_WIDTH * DONE_BTN_Y) + DONE_BTN_X, str, EDITOR_WINDOW_WIDTH, EDITOR_WINDOW_WIDTH, colorTable[18979]);

    PrintBasicStat(RENDER_ALL_STATS, 0, 0);
    ListDrvdStats();

    if (!glblmode) {
        SliderPlusID = win_register_button(
            edit_win,
            614,
            20,
            GInfo[EDITOR_GRAPHIC_SLIDER_PLUS_ON].width,
            GInfo[EDITOR_GRAPHIC_SLIDER_PLUS_ON].height,
            -1,
            522,
            521,
            522,
            grphbmp[EDITOR_GRAPHIC_SLIDER_PLUS_OFF],
            grphbmp[EDITOR_GRAPHIC_SLIDER_PLUS_ON],
            0,
            96);
        SliderNegID = win_register_button(
            edit_win,
            614,
            20 + GInfo[EDITOR_GRAPHIC_SLIDER_MINUS_ON].height - 1,
            GInfo[EDITOR_GRAPHIC_SLIDER_MINUS_ON].width,
            GInfo[EDITOR_GRAPHIC_SLIDER_MINUS_OFF].height,
            -1,
            524,
            523,
            524,
            grphbmp[EDITOR_GRAPHIC_SLIDER_MINUS_OFF],
            grphbmp[EDITOR_GRAPHIC_SLIDER_MINUS_ON],
            0,
            96);
        win_register_button_sound_func(SliderPlusID, gsound_red_butt_press, NULL);
        win_register_button_sound_func(SliderNegID, gsound_red_butt_press, NULL);
    }

    ListSkills(0);
    DrawInfoWin();
    soundContinueAll();
    PrintBigname();
    PrintAgeBig();
    PrintGender();

    if (glblmode) {
        x = NAME_BUTTON_X;
        btn = win_register_button(
            edit_win,
            x,
            NAME_BUTTON_Y,
            GInfo[EDITOR_GRAPHIC_NAME_ON].width,
            GInfo[EDITOR_GRAPHIC_NAME_ON].height,
            -1,
            -1,
            -1,
            NAME_BTN_CODE,
            grphcpy[EDITOR_GRAPHIC_NAME_OFF],
            grphcpy[EDITOR_GRAPHIC_NAME_ON],
            0,
            32);
        if (btn != -1) {
            win_register_button_mask(btn, grphbmp[EDITOR_GRAPHIC_NAME_MASK]);
            win_register_button_sound_func(btn, gsound_lrg_butt_press, NULL);
        }

        x += GInfo[EDITOR_GRAPHIC_NAME_ON].width;
        btn = win_register_button(
            edit_win,
            x,
            NAME_BUTTON_Y,
            GInfo[EDITOR_GRAPHIC_AGE_ON].width,
            GInfo[EDITOR_GRAPHIC_AGE_ON].height,
            -1,
            -1,
            -1,
            AGE_BTN_CODE,
            grphcpy[EDITOR_GRAPHIC_AGE_OFF],
            grphcpy[EDITOR_GRAPHIC_AGE_ON],
            0,
            32);
        if (btn != -1) {
            win_register_button_mask(btn, grphbmp[EDITOR_GRAPHIC_AGE_MASK]);
            win_register_button_sound_func(btn, gsound_lrg_butt_press, NULL);
        }

        x += GInfo[EDITOR_GRAPHIC_AGE_ON].width;
        btn = win_register_button(
            edit_win,
            x,
            NAME_BUTTON_Y,
            GInfo[EDITOR_GRAPHIC_SEX_ON].width,
            GInfo[EDITOR_GRAPHIC_SEX_ON].height,
            -1,
            -1,
            -1,
            SEX_BTN_CODE,
            grphcpy[EDITOR_GRAPHIC_SEX_OFF],
            grphcpy[EDITOR_GRAPHIC_SEX_ON],
            0,
            32);
        if (btn != -1) {
            win_register_button_mask(btn, grphbmp[EDITOR_GRAPHIC_SEX_MASK]);
            win_register_button_sound_func(btn, gsound_lrg_butt_press, NULL);
        }

        y = TAG_SKILLS_BUTTON_Y;
        for (i = 0; i < SKILL_COUNT; i++) {
            tag_bids[i] = win_register_button(
                edit_win,
                TAG_SKILLS_BUTTON_X,
                y,
                GInfo[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].width,
                GInfo[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height,
                -1,
                -1,
                -1,
                TAG_SKILLS_BUTTON_CODE + i,
                grphbmp[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_OFF],
                grphbmp[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON],
                NULL,
                32);
            y += GInfo[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height;
        }

        y = OPTIONAL_TRAITS_BTN_Y;
        for (i = 0; i < TRAIT_COUNT / 2; i++) {
            trait_bids[i] = win_register_button(
                edit_win,
                OPTIONAL_TRAITS_LEFT_BTN_X,
                y,
                GInfo[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].width,
                GInfo[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height,
                -1,
                -1,
                -1,
                OPTIONAL_TRAITS_BTN_CODE + i,
                grphbmp[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_OFF],
                grphbmp[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON],
                NULL,
                32);
            y += GInfo[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height + OPTIONAL_TRAITS_BTN_SPACE;
        }

        y = OPTIONAL_TRAITS_BTN_Y;
        for (i = TRAIT_COUNT / 2; i < TRAIT_COUNT; i++) {
            trait_bids[i] = win_register_button(
                edit_win,
                OPTIONAL_TRAITS_RIGHT_BTN_X,
                y,
                GInfo[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].width,
                GInfo[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height,
                -1,
                -1,
                -1,
                OPTIONAL_TRAITS_BTN_CODE + i,
                grphbmp[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_OFF],
                grphbmp[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON],
                NULL,
                32);
            y += GInfo[EDITOR_GRAPHIC_TAG_SKILL_BUTTON_ON].height + OPTIONAL_TRAITS_BTN_SPACE;
        }

        ListTraits();
    } else {
        x = NAME_BUTTON_X;
        trans_buf_to_buf(grphcpy[EDITOR_GRAPHIC_NAME_OFF],
            GInfo[EDITOR_GRAPHIC_NAME_ON].width,
            GInfo[EDITOR_GRAPHIC_NAME_ON].height,
            GInfo[EDITOR_GRAPHIC_NAME_ON].width,
            win_buf + (EDITOR_WINDOW_WIDTH * NAME_BUTTON_Y) + x,
            EDITOR_WINDOW_WIDTH);

        x += GInfo[EDITOR_GRAPHIC_NAME_ON].width;
        trans_buf_to_buf(grphcpy[EDITOR_GRAPHIC_AGE_OFF],
            GInfo[EDITOR_GRAPHIC_AGE_ON].width,
            GInfo[EDITOR_GRAPHIC_AGE_ON].height,
            GInfo[EDITOR_GRAPHIC_AGE_ON].width,
            win_buf + (EDITOR_WINDOW_WIDTH * NAME_BUTTON_Y) + x,
            EDITOR_WINDOW_WIDTH);

        x += GInfo[EDITOR_GRAPHIC_AGE_ON].width;
        trans_buf_to_buf(grphcpy[EDITOR_GRAPHIC_SEX_OFF],
            GInfo[EDITOR_GRAPHIC_SEX_ON].width,
            GInfo[EDITOR_GRAPHIC_SEX_ON].height,
            GInfo[EDITOR_GRAPHIC_SEX_ON].width,
            win_buf + (EDITOR_WINDOW_WIDTH * NAME_BUTTON_Y) + x,
            EDITOR_WINDOW_WIDTH);

        btn = win_register_button(edit_win,
            11,
            327,
            GInfo[EDITOR_GRAPHIC_FOLDER_MASK].width,
            GInfo[EDITOR_GRAPHIC_FOLDER_MASK].height,
            -1,
            -1,
            -1,
            535,
            NULL,
            NULL,
            NULL,
            BUTTON_FLAG_TRANSPARENT);
        if (btn != -1) {
            win_register_button_mask(btn, grphbmp[EDITOR_GRAPHIC_FOLDER_MASK]);
        }
    }

    if (glblmode) {
        // +/- buttons for stats
        for (i = 0; i < 7; i++) {
            stat_bids_plus[i] = win_register_button(edit_win,
                SPECIAL_STATS_BTN_X,
                StatYpos[i],
                GInfo[EDITOR_GRAPHIC_SLIDER_PLUS_ON].width,
                GInfo[EDITOR_GRAPHIC_SLIDER_PLUS_ON].height,
                -1,
                518,
                503 + i,
                518,
                grphbmp[EDITOR_GRAPHIC_SLIDER_PLUS_OFF],
                grphbmp[EDITOR_GRAPHIC_SLIDER_PLUS_ON],
                NULL,
                32);
            if (stat_bids_plus[i] != -1) {
                win_register_button_sound_func(stat_bids_plus[i], gsound_red_butt_press, NULL);
            }

            stat_bids_minus[i] = win_register_button(edit_win,
                SPECIAL_STATS_BTN_X,
                StatYpos[i] + GInfo[EDITOR_GRAPHIC_SLIDER_PLUS_ON].height - 1,
                GInfo[EDITOR_GRAPHIC_SLIDER_MINUS_ON].width,
                GInfo[EDITOR_GRAPHIC_SLIDER_MINUS_ON].height,
                -1,
                518,
                510 + i,
                518,
                grphbmp[EDITOR_GRAPHIC_SLIDER_MINUS_OFF],
                grphbmp[EDITOR_GRAPHIC_SLIDER_MINUS_ON],
                NULL,
                32);
            if (stat_bids_minus[i] != -1) {
                win_register_button_sound_func(stat_bids_minus[i], gsound_red_butt_press, NULL);
            }
        }
    }

    RegInfoAreas();
    soundContinueAll();

    btn = win_register_button(
        edit_win,
        343,
        454,
        GInfo[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        GInfo[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        501,
        grphbmp[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        grphbmp[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        win_register_button_sound_func(btn, gsound_red_butt_press, gsound_red_butt_release);
    }

    btn = win_register_button(
        edit_win,
        552,
        454,
        GInfo[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        GInfo[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        502,
        grphbmp[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        grphbmp[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        0,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        win_register_button_sound_func(btn, gsound_red_butt_press, gsound_red_butt_release);
    }

    btn = win_register_button(
        edit_win,
        455,
        454,
        GInfo[23].width,
        GInfo[23].height,
        -1,
        -1,
        -1,
        500,
        grphbmp[23],
        grphbmp[24],
        0,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        win_register_button_sound_func(btn, gsound_red_butt_press, gsound_red_butt_release);
    }

    win_draw(edit_win);
    disable_box_bar_win();

    return 0;
}

// 0x433AA8
static void CharEditEnd()
{
    // NOTE: Uninline.
    folder_exit();

    win_delete(edit_win);

    for (int index = 0; index < EDITOR_GRAPHIC_COUNT; index++) {
        art_ptr_unlock(grph_key[index]);

        if (copyflag[index]) {
            mem_free(grphcpy[index]);
        }
    }

    art_ptr_unlock(bck_key);

    // NOTE: Uninline.
    general_reps_exit();

    // NOTE: Uninline.
    karma_vars_exit();

    message_exit(&editor_message_file);

    intface_redraw();

    // NOTE: Uninline.
    RstrBckgProc();

    text_font(fontsave);

    if (glblmode == 1) {
        skill_set_tags(temp_tag_skill, 3);
        trait_set(temp_trait[0], temp_trait[1]);
        info_line = 0;
        critter_adjust_hits(obj_dude, 1000);
    }

    enable_box_bar_win();
}

// NOTE: Inlined.
//
// 0x433BEC
static void RstrBckgProc()
{
    if (bk_enable) {
        map_enable_bk_processes();
    }

    cycle_enable();

    gmouse_set_cursor(MOUSE_CURSOR_ARROW);
}

// CharEditInit
// 0x433C0C
void CharEditInit()
{
    int i;

    info_line = 0;
    skill_cursor = 0;
    slider_y = 27;
    free_perk = 0;
    folder = EDITOR_FOLDER_PERKS;

    for (i = 0; i < 2; i++) {
        temp_trait[i] = -1;
        trait_back[i] = -1;
    }

    character_points = 5;
    last_level = 1;
}

// handle name input
int get_input_str(int win, int cancelKeyCode, char* text, int maxLength, int x, int y, int textColor, int backgroundColor, int flags)
{
    int cursorWidth = text_width("_") - 4;
    int windowWidth = win_width(win);
    int v60 = text_height();
    unsigned char* windowBuffer = win_get_buf(win);
    if (maxLength > 255) {
        maxLength = 255;
    }

    char copy[257];
    strcpy(copy, text);

    int nameLength = strlen(text);
    copy[nameLength] = ' ';
    copy[nameLength + 1] = '\0';

    int nameWidth = text_width(copy);

    buf_fill(windowBuffer + windowWidth * y + x, nameWidth, text_height(), windowWidth, backgroundColor);
    text_to_buf(windowBuffer + windowWidth * y + x, copy, windowWidth, windowWidth, textColor);

    win_draw(win);

    int blinkingCounter = 3;
    bool blink = false;

    int rc = 1;
    while (rc == 1) {
        _frame_time = get_time();

        int keyCode = get_input();
        if (keyCode == cancelKeyCode) {
            rc = 0;
        } else if (keyCode == KEY_RETURN) {
            gsound_play_sfx_file("ib1p1xx1");
            rc = 0;
        } else if (keyCode == KEY_ESCAPE || game_user_wants_to_quit != 0) {
            rc = -1;
        } else {
            if ((keyCode == KEY_DELETE || keyCode == KEY_BACKSPACE) && nameLength >= 1) {
                buf_fill(windowBuffer + windowWidth * y + x, text_width(copy), v60, windowWidth, backgroundColor);
                copy[nameLength - 1] = ' ';
                copy[nameLength] = '\0';
                text_to_buf(windowBuffer + windowWidth * y + x, copy, windowWidth, windowWidth, textColor);
                nameLength--;

                win_draw(win);
            } else if ((keyCode >= KEY_FIRST_INPUT_CHARACTER && keyCode <= KEY_LAST_INPUT_CHARACTER) && nameLength < maxLength) {
                if ((flags & 0x01) != 0) {
                    if (!isdoschar(keyCode)) {
                        break;
                    }
                }

                buf_fill(windowBuffer + windowWidth * y + x, text_width(copy), v60, windowWidth, backgroundColor);

                copy[nameLength] = keyCode & 0xFF;
                copy[nameLength + 1] = ' ';
                copy[nameLength + 2] = '\0';
                text_to_buf(windowBuffer + windowWidth * y + x, copy, windowWidth, windowWidth, textColor);
                nameLength++;

                win_draw(win);
            }
        }

        blinkingCounter -= 1;
        if (blinkingCounter == 0) {
            blinkingCounter = 3;

            int color = blink ? backgroundColor : textColor;
            blink = !blink;

            buf_fill(windowBuffer + windowWidth * y + x + text_width(copy) - cursorWidth, cursorWidth, v60 - 2, windowWidth, color);
        }

        win_draw(win);

        while (elapsed_time(_frame_time) < 1000 / 24) { }
    }

    if (rc == 0 || nameLength > 0) {
        copy[nameLength] = '\0';
        strcpy(text, copy);
    }

    return rc;
}

// 0x434060
bool isdoschar(int ch)
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
//
// 0x4340D0
char* strmfe(char* dest, const char* name, const char* ext)
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
static void DrawFolder()
{
    if (glblmode) {
        return;
    }

    buf_to_buf(bckgnd + (360 * 640) + 34, 280, 120, 640, win_buf + (360 * 640) + 34, 640);

    text_font(101);

    switch (folder) {
    case EDITOR_FOLDER_PERKS:
        buf_to_buf(grphcpy[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED],
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].height,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            win_buf + (327 * 640) + 11,
            640);
        list_perks();
        break;
    case EDITOR_FOLDER_KARMA:
        buf_to_buf(grphcpy[EDITOR_GRAPHIC_KARMA_FOLDER_SELECTED],
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].height,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            win_buf + (327 * 640) + 11,
            640);
        list_karma();
        break;
    case EDITOR_FOLDER_KILLS:
        buf_to_buf(grphcpy[EDITOR_GRAPHIC_KILLS_FOLDER_SELECTED],
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].height,
            GInfo[EDITOR_GRAPHIC_PERKS_FOLDER_SELECTED].width,
            win_buf + (327 * 640) + 11,
            640);
        kills_count = ListKills();
        break;
    default:
        debug_printf("\n ** Unknown folder type! **\n");
        break;
    }
}

// 0x434238
static void list_perks()
{
    const char* string;
    char perkName[80];
    int perk;
    int perkLevel;
    bool hasContent = false;

    folder_clear();

    if (temp_trait[0] != -1) {
        // TRAITS
        string = getmsg(&editor_message_file, &mesg, 156);
        if (folder_print_seperator(string)) {
            folder_card_fid = 54;
            // Optional Traits
            folder_card_title = getmsg(&editor_message_file, &mesg, 146);
            folder_card_title2 = NULL;
            // Optional traits describe your character in more detail. All traits will have positive and negative effects. You may choose up to two traits during creation.
            folder_card_desc = getmsg(&editor_message_file, &mesg, 147);
            hasContent = true;
        }

        if (temp_trait[0] != -1) {
            string = trait_name(temp_trait[0]);
            if (folder_print_line(string)) {
                folder_card_fid = trait_pic(temp_trait[0]);
                folder_card_title = trait_name(temp_trait[0]);
                folder_card_title2 = NULL;
                folder_card_desc = trait_description(temp_trait[0]);
                hasContent = true;
            }
        }

        if (temp_trait[1] != -1) {
            string = trait_name(temp_trait[1]);
            if (folder_print_line(string)) {
                folder_card_fid = trait_pic(temp_trait[1]);
                folder_card_title = trait_name(temp_trait[1]);
                folder_card_title2 = NULL;
                folder_card_desc = trait_description(temp_trait[1]);
                hasContent = true;
            }
        }
    }

    for (perk = 0; perk < PERK_COUNT; perk++) {
        if (perk_level(obj_dude, perk) != 0) {
            break;
        }
    }

    if (perk != PERK_COUNT) {
        // PERKS
        string = getmsg(&editor_message_file, &mesg, 109);
        folder_print_seperator(string);

        for (perk = 0; perk < PERK_COUNT; perk++) {
            perkLevel = perk_level(obj_dude, perk);
            if (perkLevel != 0) {
                string = perk_name(perk);

                if (perkLevel == 1) {
                    strcpy(perkName, string);
                } else {
                    sprintf(perkName, "%s (%d)", string, perkLevel);
                }

                if (folder_print_line(perkName)) {
                    folder_card_fid = perk_skilldex_fid(perk);
                    folder_card_title = perk_name(perk);
                    folder_card_title2 = NULL;
                    folder_card_desc = perk_description(perk);
                    hasContent = true;
                }
            }
        }
    }

    if (!hasContent) {
        folder_card_fid = 71;
        // Perks
        folder_card_title = getmsg(&editor_message_file, &mesg, 124);
        folder_card_title2 = NULL;
        // Perks add additional abilities. Every third experience level, you can choose one perk.
        folder_card_desc = getmsg(&editor_message_file, &mesg, 127);
    }
}

// 0x434498
static int kills_list_comp(const void* a1, const void* a2)
{
    const KillInfo* v1 = (const KillInfo*)a1;
    const KillInfo* v2 = (const KillInfo*)a2;
    return stricmp(v1->name, v2->name);
}

// 0x4344A4
static int ListKills()
{
    int i;
    int killsCount;
    KillInfo kills[19];
    int usedKills = 0;
    bool hasContent = false;

    folder_clear();

    for (i = 0; i < KILL_TYPE_COUNT; i++) {
        killsCount = critter_kill_count(i);
        if (killsCount != 0) {
            KillInfo* killInfo = &(kills[usedKills]);
            killInfo->name = critter_kill_name(i);
            killInfo->killTypeId = i;
            killInfo->kills = killsCount;
            usedKills++;
        }
    }

    if (usedKills != 0) {
        qsort(kills, usedKills, sizeof(*kills), kills_list_comp);

        for (i = 0; i < usedKills; i++) {
            KillInfo* killInfo = &(kills[i]);
            if (folder_print_kill(killInfo->name, killInfo->kills)) {
                folder_card_fid = 46;
                folder_card_title = folder_card_string;
                folder_card_title2 = NULL;
                folder_card_desc = critter_kill_info(kills[i].killTypeId);
                sprintf(folder_card_string, "%s %s", killInfo->name, getmsg(&editor_message_file, &mesg, 126));
                hasContent = true;
            }
        }
    }

    if (!hasContent) {
        folder_card_fid = 46;
        folder_card_title = getmsg(&editor_message_file, &mesg, 126);
        folder_card_title2 = NULL;
        folder_card_desc = getmsg(&editor_message_file, &mesg, 129);
    }

    return usedKills;
}

// 0x4345DC
static void PrintBigNum(int x, int y, int flags, int value, int previousValue, int windowHandle)
{
    Rect rect;
    int windowWidth;
    unsigned char* windowBuf;
    int tens;
    int ones;
    unsigned char* tensBufferPtr;
    unsigned char* onesBufferPtr;
    unsigned char* numbersGraphicBufferPtr;

    windowWidth = win_width(windowHandle);
    windowBuf = win_get_buf(windowHandle);

    rect.ulx = x;
    rect.uly = y;
    rect.lrx = x + BIG_NUM_WIDTH * 2;
    rect.lry = y + BIG_NUM_HEIGHT;

    numbersGraphicBufferPtr = grphbmp[0];

    if (flags & RED_NUMBERS) {
        // First half of the bignum.frm is white,
        // second half is red.
        numbersGraphicBufferPtr += GInfo[EDITOR_GRAPHIC_BIG_NUMBERS].width / 2;
    }

    tensBufferPtr = windowBuf + windowWidth * y + x;
    onesBufferPtr = tensBufferPtr + BIG_NUM_WIDTH;

    if (value >= 0 && value <= 99 && previousValue >= 0 && previousValue <= 99) {
        tens = value / 10;
        ones = value % 10;

        if (flags & ANIMATE) {
            if (previousValue % 10 != ones) {
                _frame_time = get_time();
                buf_to_buf(numbersGraphicBufferPtr + BIG_NUM_WIDTH * 11,
                    BIG_NUM_WIDTH,
                    BIG_NUM_HEIGHT,
                    GInfo[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                    onesBufferPtr,
                    windowWidth);
                win_draw_rect(windowHandle, &rect);
                while (elapsed_time(_frame_time) < BIG_NUM_ANIMATION_DELAY)
                    ;
            }

            buf_to_buf(numbersGraphicBufferPtr + BIG_NUM_WIDTH * ones,
                BIG_NUM_WIDTH,
                BIG_NUM_HEIGHT,
                GInfo[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                onesBufferPtr,
                windowWidth);
            win_draw_rect(windowHandle, &rect);

            if (previousValue / 10 != tens) {
                _frame_time = get_time();
                buf_to_buf(numbersGraphicBufferPtr + BIG_NUM_WIDTH * 11,
                    BIG_NUM_WIDTH,
                    BIG_NUM_HEIGHT,
                    GInfo[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                    tensBufferPtr,
                    windowWidth);
                win_draw_rect(windowHandle, &rect);
                while (elapsed_time(_frame_time) < BIG_NUM_ANIMATION_DELAY)
                    ;
            }

            buf_to_buf(numbersGraphicBufferPtr + BIG_NUM_WIDTH * tens,
                BIG_NUM_WIDTH,
                BIG_NUM_HEIGHT,
                GInfo[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                tensBufferPtr,
                windowWidth);
            win_draw_rect(windowHandle, &rect);
        } else {
            buf_to_buf(numbersGraphicBufferPtr + BIG_NUM_WIDTH * tens,
                BIG_NUM_WIDTH,
                BIG_NUM_HEIGHT,
                GInfo[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                tensBufferPtr,
                windowWidth);
            buf_to_buf(numbersGraphicBufferPtr + BIG_NUM_WIDTH * ones,
                BIG_NUM_WIDTH,
                BIG_NUM_HEIGHT,
                GInfo[EDITOR_GRAPHIC_BIG_NUMBERS].width,
                onesBufferPtr,
                windowWidth);
        }
    } else {

        buf_to_buf(numbersGraphicBufferPtr + BIG_NUM_WIDTH * 9,
            BIG_NUM_WIDTH,
            BIG_NUM_HEIGHT,
            GInfo[EDITOR_GRAPHIC_BIG_NUMBERS].width,
            tensBufferPtr,
            windowWidth);
        buf_to_buf(numbersGraphicBufferPtr + BIG_NUM_WIDTH * 9,
            BIG_NUM_WIDTH,
            BIG_NUM_HEIGHT,
            GInfo[EDITOR_GRAPHIC_BIG_NUMBERS].width,
            onesBufferPtr,
            windowWidth);
    }
}

// 0x434920
static void PrintLevelWin()
{
    int color;
    int y;
    char* formattedValue;
    // NOTE: The length of this buffer is 8 bytes, which is enough to display
    // 999,999 (7 bytes NULL-terminated) experience points. Usually a player
    // will never gain that much during normal gameplay.
    //
    // However it's possible to use one of the F2 modding tools and savegame
    // editors to receive rediculous amount of experience points. Vanilla is
    // able to handle it, because `stringBuffer` acts as continuation of
    // `formattedValueBuffer`. This is not the case with MSVC, where
    // insufficient space for xp greater then 999,999 ruins the stack. In order
    // to fix the `formattedValueBuffer` is expanded to 16 bytes, so it should
    // be possible to store max 32-bit integer (4,294,967,295).
    char formattedValueBuffer[16];
    char stringBuffer[128];

    if (glblmode == 1) {
        return;
    }

    text_font(101);

    buf_to_buf(bckgnd + 640 * 280 + 32, 124, 32, 640, win_buf + 640 * 280 + 32, 640);

    // LEVEL
    y = 280;
    if (info_line != 7) {
        color = colorTable[992];
    } else {
        color = colorTable[32747];
    }

    int level = stat_pc_get(PC_STAT_LEVEL);
    sprintf(stringBuffer, "%s %d",
        getmsg(&editor_message_file, &mesg, 113),
        level);
    text_to_buf(win_buf + 640 * y + 32, stringBuffer, 640, 640, color);

    // EXPERIENCE
    y += text_height() + 1;
    if (info_line != 8) {
        color = colorTable[992];
    } else {
        color = colorTable[32747];
    }

    int exp = stat_pc_get(PC_STAT_EXPERIENCE);
    sprintf(stringBuffer, "%s %s",
        getmsg(&editor_message_file, &mesg, 114),
        itostndn(exp, formattedValueBuffer));
    text_to_buf(win_buf + 640 * y + 32, stringBuffer, 640, 640, color);

    // EXP NEEDED TO NEXT LEVEL
    y += text_height() + 1;
    if (info_line != 9) {
        color = colorTable[992];
    } else {
        color = colorTable[32747];
    }

    int expToNextLevel = stat_pc_min_exp();
    int expMsgId;
    if (expToNextLevel == -1) {
        expMsgId = 115;
        formattedValue = byte_5016E4;
    } else {
        expMsgId = 115;
        if (expToNextLevel > 999999) {
            expMsgId = 175;
        }
        formattedValue = itostndn(expToNextLevel, formattedValueBuffer);
    }

    sprintf(stringBuffer, "%s %s",
        getmsg(&editor_message_file, &mesg, expMsgId),
        formattedValue);
    text_to_buf(win_buf + 640 * y + 32, stringBuffer, 640, 640, color);
}

// 0x434B38
static void PrintBasicStat(int stat, bool animate, int previousValue)
{
    int off;
    int color;
    const char* description;
    int value;
    int flags;
    int messageListItemId;

    text_font(101);

    if (stat == RENDER_ALL_STATS) {
        // NOTE: Original code is different, looks like tail recursion
        // optimization.
        for (stat = 0; stat < 7; stat++) {
            PrintBasicStat(stat, 0, 0);
        }
        return;
    }

    if (info_line == stat) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    off = 640 * (StatYpos[stat] + 8) + 103;

    // TODO: The original code is different.
    if (glblmode) {
        value = stat_get_base(obj_dude, stat) + stat_get_bonus(obj_dude, stat);

        flags = 0;

        if (animate) {
            flags |= ANIMATE;
        }

        if (value > 10) {
            flags |= RED_NUMBERS;
        }

        PrintBigNum(58, StatYpos[stat], flags, value, previousValue, edit_win);

        buf_to_buf(bckgnd + off, 40, text_height(), 640, win_buf + off, 640);

        messageListItemId = critterGetStat(obj_dude, stat) + 199;
        if (messageListItemId > 210) {
            messageListItemId = 210;
        }

        description = getmsg(&editor_message_file, &mesg, messageListItemId);
        text_to_buf(win_buf + 640 * (StatYpos[stat] + 8) + 103, description, 640, 640, color);
    } else {
        value = critterGetStat(obj_dude, stat);
        PrintBigNum(58, StatYpos[stat], 0, value, 0, edit_win);
        buf_to_buf(bckgnd + off, 40, text_height(), 640, win_buf + off, 640);

        value = critterGetStat(obj_dude, stat);
        if (value > 10) {
            value = 10;
        }

        description = stat_level_description(value);
        text_to_buf(win_buf + off, description, 640, 640, color);
    }
}

// 0x434F18
static void PrintGender()
{
    int gender;
    char* str;
    char text[32];
    int x, width;

    text_font(103);

    gender = critterGetStat(obj_dude, STAT_GENDER);
    str = getmsg(&editor_message_file, &mesg, 107 + gender);

    strcpy(text, str);

    width = GInfo[EDITOR_GRAPHIC_SEX_ON].width;
    x = (width / 2) - (text_width(text) / 2);

    memcpy(grphcpy[11],
        grphbmp[EDITOR_GRAPHIC_SEX_ON],
        width * GInfo[EDITOR_GRAPHIC_SEX_ON].height);
    memcpy(grphcpy[EDITOR_GRAPHIC_SEX_OFF],
        grphbmp[10],
        width * GInfo[EDITOR_GRAPHIC_SEX_OFF].height);

    x += 6 * width;
    text_to_buf(grphcpy[EDITOR_GRAPHIC_SEX_ON] + x, text, width, width, colorTable[14723]);
    text_to_buf(grphcpy[EDITOR_GRAPHIC_SEX_OFF] + x, text, width, width, colorTable[18979]);
}

// 0x43501C
static void PrintAgeBig()
{
    int age;
    char* str;
    char text[32];
    int x, width;

    text_font(103);

    age = critterGetStat(obj_dude, STAT_AGE);
    str = getmsg(&editor_message_file, &mesg, 104);

    sprintf(text, "%s %d", str, age);

    width = GInfo[EDITOR_GRAPHIC_AGE_ON].width;
    x = (width / 2) + 1 - (text_width(text) / 2);

    memcpy(grphcpy[EDITOR_GRAPHIC_AGE_ON],
        grphbmp[EDITOR_GRAPHIC_AGE_ON],
        width * GInfo[EDITOR_GRAPHIC_AGE_ON].height);
    memcpy(grphcpy[EDITOR_GRAPHIC_AGE_OFF],
        grphbmp[EDITOR_GRAPHIC_AGE_OFF],
        width * GInfo[EDITOR_GRAPHIC_AGE_ON].height);

    x += 6 * width;
    text_to_buf(grphcpy[EDITOR_GRAPHIC_AGE_ON] + x, text, width, width, colorTable[14723]);
    text_to_buf(grphcpy[EDITOR_GRAPHIC_AGE_OFF] + x, text, width, width, colorTable[18979]);
}

// 0x435118
static void PrintBigname()
{
    char* str;
    char text[32];
    int x, width;
    char *pch, tmp;
    bool has_space;

    text_font(103);

    str = critter_name(obj_dude);
    strcpy(text, str);

    if (text_width(text) > 100) {
        pch = text;
        has_space = false;
        while (*pch != '\0') {
            tmp = *pch;
            *pch = '\0';
            if (tmp == ' ') {
                has_space = true;
            }

            if (text_width(text) > 100) {
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

    width = GInfo[EDITOR_GRAPHIC_NAME_ON].width;
    x = (width / 2) + 3 - (text_width(text) / 2);

    memcpy(grphcpy[EDITOR_GRAPHIC_NAME_ON],
        grphbmp[EDITOR_GRAPHIC_NAME_ON],
        GInfo[EDITOR_GRAPHIC_NAME_ON].width * GInfo[EDITOR_GRAPHIC_NAME_ON].height);
    memcpy(grphcpy[EDITOR_GRAPHIC_NAME_OFF],
        grphbmp[EDITOR_GRAPHIC_NAME_OFF],
        GInfo[EDITOR_GRAPHIC_NAME_OFF].width * GInfo[EDITOR_GRAPHIC_NAME_OFF].height);

    x += 6 * width;
    text_to_buf(grphcpy[EDITOR_GRAPHIC_NAME_ON] + x, text, width, width, colorTable[14723]);
    text_to_buf(grphcpy[EDITOR_GRAPHIC_NAME_OFF] + x, text, width, width, colorTable[18979]);
}

// 0x43527C
static void ListDrvdStats()
{
    int conditions;
    int color;
    const char* messageListItemText;
    char t[420]; // TODO: Size is wrong.
    int y;

    conditions = obj_dude->data.critter.combat.results;

    text_font(101);

    y = 46;

    buf_to_buf(bckgnd + 640 * y + 194, 118, 108, 640, win_buf + 640 * y + 194, 640);

    // Hit Points
    if (info_line == EDITOR_HIT_POINTS) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    int currHp;
    int maxHp;
    if (glblmode) {
        maxHp = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);
        currHp = maxHp;
    } else {
        maxHp = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);
        currHp = critter_get_hits(obj_dude);
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 300);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    sprintf(t, "%d/%d", currHp, maxHp);
    text_to_buf(win_buf + 640 * y + 263, t, 640, 640, color);

    // Poisoned
    y += text_height() + 3;

    if (info_line == EDITOR_POISONED) {
        color = critter_get_poison(obj_dude) != 0 ? colorTable[32747] : colorTable[15845];
    } else {
        color = critter_get_poison(obj_dude) != 0 ? colorTable[992] : colorTable[1313];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 312);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    // Radiated
    y += text_height() + 3;

    if (info_line == EDITOR_RADIATED) {
        color = critter_get_rads(obj_dude) != 0 ? colorTable[32747] : colorTable[15845];
    } else {
        color = critter_get_rads(obj_dude) != 0 ? colorTable[992] : colorTable[1313];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 313);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    // Eye Damage
    y += text_height() + 3;

    if (info_line == EDITOR_EYE_DAMAGE) {
        color = (conditions & DAM_BLIND) ? colorTable[32747] : colorTable[15845];
    } else {
        color = (conditions & DAM_BLIND) ? colorTable[992] : colorTable[1313];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 314);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    // Crippled Right Arm
    y += text_height() + 3;

    if (info_line == EDITOR_CRIPPLED_RIGHT_ARM) {
        color = (conditions & DAM_CRIP_ARM_RIGHT) ? colorTable[32747] : colorTable[15845];
    } else {
        color = (conditions & DAM_CRIP_ARM_RIGHT) ? colorTable[992] : colorTable[1313];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 315);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    // Crippled Left Arm
    y += text_height() + 3;

    if (info_line == EDITOR_CRIPPLED_LEFT_ARM) {
        color = (conditions & DAM_CRIP_ARM_LEFT) ? colorTable[32747] : colorTable[15845];
    } else {
        color = (conditions & DAM_CRIP_ARM_LEFT) ? colorTable[992] : colorTable[1313];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 316);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    // Crippled Right Leg
    y += text_height() + 3;

    if (info_line == EDITOR_CRIPPLED_RIGHT_LEG) {
        color = (conditions & DAM_CRIP_LEG_RIGHT) ? colorTable[32747] : colorTable[15845];
    } else {
        color = (conditions & DAM_CRIP_LEG_RIGHT) ? colorTable[992] : colorTable[1313];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 317);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    // Crippled Left Leg
    y += text_height() + 3;

    if (info_line == EDITOR_CRIPPLED_LEFT_LEG) {
        color = (conditions & DAM_CRIP_LEG_LEFT) ? colorTable[32747] : colorTable[15845];
    } else {
        color = (conditions & DAM_CRIP_LEG_LEFT) ? colorTable[992] : colorTable[1313];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 318);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    y = 179;

    buf_to_buf(bckgnd + 640 * y + 194, 116, 130, 640, win_buf + 640 * y + 194, 640);

    // Armor Class
    if (info_line == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_ARMOR_CLASS) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 302);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(obj_dude, STAT_ARMOR_CLASS), t, 10);
    text_to_buf(win_buf + 640 * y + 288, t, 640, 640, color);

    // Action Points
    y += text_height() + 3;

    if (info_line == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_ACTION_POINTS) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 301);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(obj_dude, STAT_MAXIMUM_ACTION_POINTS), t, 10);
    text_to_buf(win_buf + 640 * y + 288, t, 640, 640, color);

    // Carry Weight
    y += text_height() + 3;

    if (info_line == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_CARRY_WEIGHT) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 311);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(obj_dude, STAT_CARRY_WEIGHT), t, 10);
    text_to_buf(win_buf + 640 * y + 288, t, 640, 640, critterIsOverloaded(obj_dude) ? colorTable[31744] : color);

    // Melee Damage
    y += text_height() + 3;

    if (info_line == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_MELEE_DAMAGE) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 304);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(obj_dude, STAT_MELEE_DAMAGE), t, 10);
    text_to_buf(win_buf + 640 * y + 288, t, 640, 640, color);

    // Damage Resistance
    y += text_height() + 3;

    if (info_line == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_DAMAGE_RESISTANCE) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 305);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    sprintf(t, "%d%%", critterGetStat(obj_dude, STAT_DAMAGE_RESISTANCE));
    text_to_buf(win_buf + 640 * y + 288, t, 640, 640, color);

    // Poison Resistance
    y += text_height() + 3;

    if (info_line == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_POISON_RESISTANCE) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 306);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    sprintf(t, "%d%%", critterGetStat(obj_dude, STAT_POISON_RESISTANCE));
    text_to_buf(win_buf + 640 * y + 288, t, 640, 640, color);

    // Radiation Resistance
    y += text_height() + 3;

    if (info_line == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_RADIATION_RESISTANCE) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 307);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    sprintf(t, "%d%%", critterGetStat(obj_dude, STAT_RADIATION_RESISTANCE));
    text_to_buf(win_buf + 640 * y + 288, t, 640, 640, color);

    // Sequence
    y += text_height() + 3;

    if (info_line == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_SEQUENCE) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 308);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(obj_dude, STAT_SEQUENCE), t, 10);
    text_to_buf(win_buf + 640 * y + 288, t, 640, 640, color);

    // Healing Rate
    y += text_height() + 3;

    if (info_line == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_HEALING_RATE) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 309);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    itoa(critterGetStat(obj_dude, STAT_HEALING_RATE), t, 10);
    text_to_buf(win_buf + 640 * y + 288, t, 640, 640, color);

    // Critical Chance
    y += text_height() + 3;

    if (info_line == EDITOR_FIRST_DERIVED_STAT + EDITOR_DERIVED_STAT_CRITICAL_CHANCE) {
        color = colorTable[32747];
    } else {
        color = colorTable[992];
    }

    messageListItemText = getmsg(&editor_message_file, &mesg, 310);
    sprintf(t, "%s", messageListItemText);
    text_to_buf(win_buf + 640 * y + 194, t, 640, 640, color);

    sprintf(t, "%d%%", critterGetStat(obj_dude, STAT_CRITICAL_CHANCE));
    text_to_buf(win_buf + 640 * y + 288, t, 640, 640, color);
}

// 0x436154
static void ListSkills(int a1)
{
    int selectedSkill = -1;
    const char* str;
    int i;
    int color;
    int y;
    int value;
    char valueString[32];

    if (info_line >= EDITOR_FIRST_SKILL && info_line < 79) {
        selectedSkill = info_line - EDITOR_FIRST_SKILL;
    }

    if (glblmode == 0 && a1 == 0) {
        win_delete_button(SliderPlusID);
        win_delete_button(SliderNegID);
        SliderNegID = -1;
        SliderPlusID = -1;
    }

    buf_to_buf(bckgnd + 370, 270, 252, 640, win_buf + 370, 640);

    text_font(103);

    // SKILLS
    str = getmsg(&editor_message_file, &mesg, 117);
    text_to_buf(win_buf + 640 * 5 + 380, str, 640, 640, colorTable[18979]);

    if (!glblmode) {
        // SKILL POINTS
        str = getmsg(&editor_message_file, &mesg, 112);
        text_to_buf(win_buf + 640 * 233 + 400, str, 640, 640, colorTable[18979]);

        value = stat_pc_get(PC_STAT_UNSPENT_SKILL_POINTS);
        PrintBigNum(522, 228, 0, value, 0, edit_win);
    } else {
        // TAG SKILLS
        str = getmsg(&editor_message_file, &mesg, 138);
        text_to_buf(win_buf + 640 * 233 + 422, str, 640, 640, colorTable[18979]);

        if (a1 == 2 && !first_skill_list) {
            PrintBigNum(522, 228, ANIMATE, tagskill_count, old_tags, edit_win);
        } else {
            PrintBigNum(522, 228, 0, tagskill_count, 0, edit_win);
            first_skill_list = 0;
        }
    }

    skill_set_tags(temp_tag_skill, NUM_TAGGED_SKILLS);

    text_font(101);

    y = 27;
    for (i = 0; i < SKILL_COUNT; i++) {
        if (i == selectedSkill) {
            if (i != temp_tag_skill[0] && i != temp_tag_skill[1] && i != temp_tag_skill[2] && i != temp_tag_skill[3]) {
                color = colorTable[32747];
            } else {
                color = colorTable[32767];
            }
        } else {
            if (i != temp_tag_skill[0] && i != temp_tag_skill[1] && i != temp_tag_skill[2] && i != temp_tag_skill[3]) {
                color = colorTable[992];
            } else {
                color = colorTable[21140];
            }
        }

        str = skill_name(i);
        text_to_buf(win_buf + 640 * y + 380, str, 640, 640, color);

        value = skill_level(obj_dude, i);
        sprintf(valueString, "%d%%", value);

        text_to_buf(win_buf + 640 * y + 573, valueString, 640, 640, color);

        y += text_height() + 1;
    }

    if (!glblmode) {
        y = skill_cursor * (text_height() + 1);
        slider_y = y + 27;

        trans_buf_to_buf(
            grphbmp[EDITOR_GRAPHIC_SLIDER],
            GInfo[EDITOR_GRAPHIC_SLIDER].width,
            GInfo[EDITOR_GRAPHIC_SLIDER].height,
            GInfo[EDITOR_GRAPHIC_SLIDER].width,
            win_buf + 640 * (y + 16) + 592,
            640);

        if (a1 == 0) {
            if (SliderPlusID == -1) {
                SliderPlusID = win_register_button(
                    edit_win,
                    614,
                    slider_y - 7,
                    GInfo[EDITOR_GRAPHIC_SLIDER_PLUS_ON].width,
                    GInfo[EDITOR_GRAPHIC_SLIDER_PLUS_ON].height,
                    -1,
                    522,
                    521,
                    522,
                    grphbmp[EDITOR_GRAPHIC_SLIDER_PLUS_OFF],
                    grphbmp[EDITOR_GRAPHIC_SLIDER_PLUS_ON],
                    NULL,
                    96);
                win_register_button_sound_func(SliderPlusID, gsound_red_butt_press, NULL);
            }

            if (SliderNegID == -1) {
                SliderNegID = win_register_button(
                    edit_win,
                    614,
                    slider_y + 4 - 12 + GInfo[EDITOR_GRAPHIC_SLIDER_MINUS_ON].height,
                    GInfo[EDITOR_GRAPHIC_SLIDER_MINUS_ON].width,
                    GInfo[EDITOR_GRAPHIC_SLIDER_MINUS_OFF].height,
                    -1,
                    524,
                    523,
                    524,
                    grphbmp[EDITOR_GRAPHIC_SLIDER_MINUS_OFF],
                    grphbmp[EDITOR_GRAPHIC_SLIDER_MINUS_ON],
                    NULL,
                    96);
                win_register_button_sound_func(SliderNegID, gsound_red_butt_press, NULL);
            }
        }
    }
}

// 0x4365AC
static void DrawInfoWin()
{
    int graphicId;
    char* title;
    char* description;

    if (info_line < 0 || info_line >= 98) {
        return;
    }

    buf_to_buf(bckgnd + (640 * 267) + 345, 277, 170, 640, win_buf + (267 * 640) + 345, 640);

    if (info_line >= 0 && info_line < 7) {
        description = stat_description(info_line);
        title = stat_name(info_line);
        graphicId = stat_picture(info_line);
        DrawCard(graphicId, title, NULL, description);
    } else if (info_line >= 7 && info_line < 10) {
        if (glblmode) {
            switch (info_line) {
            case 7:
                // Character Points
                description = getmsg(&editor_message_file, &mesg, 121);
                title = getmsg(&editor_message_file, &mesg, 120);
                DrawCard(7, title, NULL, description);
                break;
            }
        } else {
            switch (info_line) {
            case 7:
                description = stat_pc_description(PC_STAT_LEVEL);
                title = stat_pc_name(PC_STAT_LEVEL);
                DrawCard(7, title, NULL, description);
                break;
            case 8:
                description = stat_pc_description(PC_STAT_EXPERIENCE);
                title = stat_pc_name(PC_STAT_EXPERIENCE);
                DrawCard(8, title, NULL, description);
                break;
            case 9:
                // Next Level
                description = getmsg(&editor_message_file, &mesg, 123);
                title = getmsg(&editor_message_file, &mesg, 122);
                DrawCard(9, title, NULL, description);
                break;
            }
        }
    } else if ((info_line >= 10 && info_line < 43) || (info_line >= 82 && info_line < 98)) {
        DrawCard(folder_card_fid, folder_card_title, folder_card_title2, folder_card_desc);
    } else if (info_line >= 43 && info_line < 51) {
        switch (info_line) {
        case EDITOR_HIT_POINTS:
            description = stat_description(STAT_MAXIMUM_HIT_POINTS);
            title = getmsg(&editor_message_file, &mesg, 300);
            graphicId = stat_picture(STAT_MAXIMUM_HIT_POINTS);
            DrawCard(graphicId, title, NULL, description);
            break;
        case EDITOR_POISONED:
            description = getmsg(&editor_message_file, &mesg, 400);
            title = getmsg(&editor_message_file, &mesg, 312);
            DrawCard(11, title, NULL, description);
            break;
        case EDITOR_RADIATED:
            description = getmsg(&editor_message_file, &mesg, 401);
            title = getmsg(&editor_message_file, &mesg, 313);
            DrawCard(12, title, NULL, description);
            break;
        case EDITOR_EYE_DAMAGE:
            description = getmsg(&editor_message_file, &mesg, 402);
            title = getmsg(&editor_message_file, &mesg, 314);
            DrawCard(13, title, NULL, description);
            break;
        case EDITOR_CRIPPLED_RIGHT_ARM:
            description = getmsg(&editor_message_file, &mesg, 403);
            title = getmsg(&editor_message_file, &mesg, 315);
            DrawCard(14, title, NULL, description);
            break;
        case EDITOR_CRIPPLED_LEFT_ARM:
            description = getmsg(&editor_message_file, &mesg, 404);
            title = getmsg(&editor_message_file, &mesg, 316);
            DrawCard(15, title, NULL, description);
            break;
        case EDITOR_CRIPPLED_RIGHT_LEG:
            description = getmsg(&editor_message_file, &mesg, 405);
            title = getmsg(&editor_message_file, &mesg, 317);
            DrawCard(16, title, NULL, description);
            break;
        case EDITOR_CRIPPLED_LEFT_LEG:
            description = getmsg(&editor_message_file, &mesg, 406);
            title = getmsg(&editor_message_file, &mesg, 318);
            DrawCard(17, title, NULL, description);
            break;
        }
    } else if (info_line >= EDITOR_FIRST_DERIVED_STAT && info_line < 61) {
        int derivedStatIndex = info_line - 51;
        int stat = ndinfoxlt[derivedStatIndex];
        description = stat_description(stat);
        title = stat_name(stat);
        graphicId = ndrvd[derivedStatIndex];
        DrawCard(graphicId, title, NULL, description);
    } else if (info_line >= EDITOR_FIRST_SKILL && info_line < 79) {
        int skill = info_line - 61;
        const char* attributesDescription = skill_attribute(skill);

        char formatted[150]; // TODO: Size is probably wrong.
        const char* base = getmsg(&editor_message_file, &mesg, 137);
        int defaultValue = skill_base(skill);
        sprintf(formatted, "%s %d%% %s", base, defaultValue, attributesDescription);

        graphicId = skill_pic(skill);
        title = skill_name(skill);
        description = skill_description(skill);
        DrawCard(graphicId, title, formatted, description);
    } else if (info_line >= 79 && info_line < 82) {
        switch (info_line) {
        case EDITOR_TAG_SKILL:
            if (glblmode) {
                // Tag Skill
                description = getmsg(&editor_message_file, &mesg, 145);
                title = getmsg(&editor_message_file, &mesg, 144);
                DrawCard(27, title, NULL, description);
            } else {
                // Skill Points
                description = getmsg(&editor_message_file, &mesg, 131);
                title = getmsg(&editor_message_file, &mesg, 130);
                DrawCard(27, title, NULL, description);
            }
            break;
        case EDITOR_SKILLS:
            // Skills
            description = getmsg(&editor_message_file, &mesg, 151);
            title = getmsg(&editor_message_file, &mesg, 150);
            DrawCard(27, title, NULL, description);
            break;
        case EDITOR_OPTIONAL_TRAITS:
            // Optional Traits
            description = getmsg(&editor_message_file, &mesg, 147);
            title = getmsg(&editor_message_file, &mesg, 146);
            DrawCard(27, title, NULL, description);
            break;
        }
    }
}

// 0x436C4C
static int NameWindow()
{
    char* text;

    int windowWidth = GInfo[EDITOR_GRAPHIC_CHARWIN].width;
    int windowHeight = GInfo[EDITOR_GRAPHIC_CHARWIN].height;

    int nameWindowX = 17;
    int nameWindowY = 0;
    int win = win_add(nameWindowX, nameWindowY, windowWidth, windowHeight, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);
    if (win == -1) {
        return -1;
    }

    unsigned char* windowBuf = win_get_buf(win);

    // Copy background
    memcpy(windowBuf, grphbmp[EDITOR_GRAPHIC_CHARWIN], windowWidth * windowHeight);

    trans_buf_to_buf(
        grphbmp[EDITOR_GRAPHIC_NAME_BOX],
        GInfo[EDITOR_GRAPHIC_NAME_BOX].width,
        GInfo[EDITOR_GRAPHIC_NAME_BOX].height,
        GInfo[EDITOR_GRAPHIC_NAME_BOX].width,
        windowBuf + windowWidth * 13 + 13,
        windowWidth);
    trans_buf_to_buf(grphbmp[EDITOR_GRAPHIC_DONE_BOX],
        GInfo[EDITOR_GRAPHIC_DONE_BOX].width,
        GInfo[EDITOR_GRAPHIC_DONE_BOX].height,
        GInfo[EDITOR_GRAPHIC_DONE_BOX].width,
        windowBuf + windowWidth * 40 + 13,
        windowWidth);

    text_font(103);

    text = getmsg(&editor_message_file, &mesg, 100);
    text_to_buf(windowBuf + windowWidth * 44 + 50, text, windowWidth, windowWidth, colorTable[18979]);

    int doneBtn = win_register_button(win,
        26,
        44,
        GInfo[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        GInfo[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        500,
        grphbmp[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        grphbmp[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (doneBtn != -1) {
        win_register_button_sound_func(doneBtn, gsound_red_butt_press, gsound_red_butt_release);
    }

    win_draw(win);

    text_font(101);

    char name[64];
    strcpy(name, critter_name(obj_dude));

    if (strcmp(name, "None") == 0) {
        name[0] = '\0';
    }

    // NOTE: I don't understand the nameCopy, not sure what it is used for. It's
    // definitely there, but I just don' get it.
    char nameCopy[64];
    strcpy(nameCopy, name);

    if (get_input_str(win, 500, nameCopy, 11, 23, 19, colorTable[992], 100, 0) != -1) {
        if (nameCopy[0] != '\0') {
            critter_pc_set_name(nameCopy);
            PrintBigname();
            win_delete(win);
            return 0;
        }
    }

    // NOTE: original code is a bit different, the following chunk of code written two times.

    text_font(101);
    buf_to_buf(grphbmp[EDITOR_GRAPHIC_NAME_BOX],
        GInfo[EDITOR_GRAPHIC_NAME_BOX].width,
        GInfo[EDITOR_GRAPHIC_NAME_BOX].height,
        GInfo[EDITOR_GRAPHIC_NAME_BOX].width,
        windowBuf + GInfo[EDITOR_GRAPHIC_CHARWIN].width * 13 + 13,
        GInfo[EDITOR_GRAPHIC_CHARWIN].width);

    PrintName(windowBuf, GInfo[EDITOR_GRAPHIC_CHARWIN].width);

    strcpy(nameCopy, name);

    win_delete(win);

    return 0;
}

// 0x436F70
static void PrintName(unsigned char* buf, int pitch)
{
    char str[64];
    char* v4;

    memcpy(str, byte_431D93, 64);

    text_font(101);

    v4 = critter_name(obj_dude);

    // TODO: Check.
    strcpy(str, v4);

    text_to_buf(buf + 19 * pitch + 21, str, pitch, pitch, colorTable[992]);
}

// 0x436FEC
static int AgeWindow()
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

    int savedAge = critterGetStat(obj_dude, STAT_AGE);

    windowWidth = GInfo[EDITOR_GRAPHIC_CHARWIN].width;
    windowHeight = GInfo[EDITOR_GRAPHIC_CHARWIN].height;

    int ageWindowX = GInfo[EDITOR_GRAPHIC_NAME_ON].width + 9;
    int ageWindowY = 0;
    win = win_add(ageWindowX, ageWindowY, windowWidth, windowHeight, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);
    if (win == -1) {
        return -1;
    }

    windowBuf = win_get_buf(win);

    memcpy(windowBuf, grphbmp[EDITOR_GRAPHIC_CHARWIN], windowWidth * windowHeight);

    trans_buf_to_buf(
        grphbmp[EDITOR_GRAPHIC_AGE_BOX],
        GInfo[EDITOR_GRAPHIC_AGE_BOX].width,
        GInfo[EDITOR_GRAPHIC_AGE_BOX].height,
        GInfo[EDITOR_GRAPHIC_AGE_BOX].width,
        windowBuf + windowWidth * 7 + 8,
        windowWidth);
    trans_buf_to_buf(
        grphbmp[EDITOR_GRAPHIC_DONE_BOX],
        GInfo[EDITOR_GRAPHIC_DONE_BOX].width,
        GInfo[EDITOR_GRAPHIC_DONE_BOX].height,
        GInfo[EDITOR_GRAPHIC_DONE_BOX].width,
        windowBuf + windowWidth * 40 + 13,
        GInfo[EDITOR_GRAPHIC_CHARWIN].width);

    text_font(103);

    messageListItemText = getmsg(&editor_message_file, &mesg, 100);
    text_to_buf(windowBuf + windowWidth * 44 + 50, messageListItemText, windowWidth, windowWidth, colorTable[18979]);

    age = critterGetStat(obj_dude, STAT_AGE);
    PrintBigNum(55, 10, 0, age, 0, win);

    doneBtn = win_register_button(win,
        26,
        44,
        GInfo[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        GInfo[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        500,
        grphbmp[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        grphbmp[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (doneBtn != -1) {
        win_register_button_sound_func(doneBtn, gsound_red_butt_press, gsound_red_butt_release);
    }

    nextBtn = win_register_button(win,
        105,
        13,
        GInfo[EDITOR_GRAPHIC_LEFT_ARROW_DOWN].width,
        GInfo[EDITOR_GRAPHIC_LEFT_ARROW_DOWN].height,
        -1,
        503,
        501,
        503,
        grphbmp[EDITOR_GRAPHIC_RIGHT_ARROW_UP],
        grphbmp[EDITOR_GRAPHIC_RIGHT_ARROW_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (nextBtn != -1) {
        win_register_button_sound_func(nextBtn, gsound_med_butt_press, NULL);
    }

    prevBtn = win_register_button(win,
        19,
        13,
        GInfo[EDITOR_GRAPHIC_RIGHT_ARROW_DOWN].width,
        GInfo[EDITOR_GRAPHIC_RIGHT_ARROW_DOWN].height,
        -1,
        504,
        502,
        504,
        grphbmp[EDITOR_GRAPHIC_LEFT_ARROW_UP],
        grphbmp[EDITOR_GRAPHIC_LEFT_ARROW_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (prevBtn != -1) {
        win_register_button_sound_func(prevBtn, gsound_med_butt_press, NULL);
    }

    while (true) {
        _frame_time = get_time();
        change = 0;
        flags = 0;
        int v32 = 0;

        keyCode = get_input();

        if (keyCode == KEY_RETURN || keyCode == 500) {
            if (keyCode != 500) {
                gsound_play_sfx_file("ib1p1xx1");
            }

            win_delete(win);
            return 0;
        } else if (keyCode == KEY_ESCAPE || game_user_wants_to_quit != 0) {
            break;
        } else if (keyCode == 501) {
            age = critterGetStat(obj_dude, STAT_AGE);
            if (age < 35) {
                change = 1;
            }
        } else if (keyCode == 502) {
            age = critterGetStat(obj_dude, STAT_AGE);
            if (age > 16) {
                change = -1;
            }
        } else if (keyCode == KEY_PLUS || keyCode == KEY_UPPERCASE_N || keyCode == KEY_ARROW_UP) {
            previousAge = critterGetStat(obj_dude, STAT_AGE);
            if (previousAge < 35) {
                flags = ANIMATE;
                if (inc_stat(obj_dude, STAT_AGE) != 0) {
                    flags = 0;
                }
                age = critterGetStat(obj_dude, STAT_AGE);
                PrintBigNum(55, 10, flags, age, previousAge, win);
            }
        } else if (keyCode == KEY_MINUS || keyCode == KEY_UPPERCASE_J || keyCode == KEY_ARROW_DOWN) {
            previousAge = critterGetStat(obj_dude, STAT_AGE);
            if (previousAge > 16) {
                flags = ANIMATE;
                if (dec_stat(obj_dude, STAT_AGE) != 0) {
                    flags = 0;
                }
                age = critterGetStat(obj_dude, STAT_AGE);

                PrintBigNum(55, 10, flags, age, previousAge, win);
            }
        }

        if (flags == ANIMATE) {
            PrintAgeBig();
            PrintBasicStat(RENDER_ALL_STATS, 0, 0);
            ListDrvdStats();
            win_draw(edit_win);
            win_draw(win);
        }

        if (change != 0) {
            int v33 = 0;

            _repFtime = 4;

            while (true) {
                _frame_time = get_time();

                v33++;

                if ((!v32 && v33 == 1) || (v32 && v33 > 14.4)) {
                    v32 = true;

                    if (v33 > 14.4) {
                        _repFtime++;
                        if (_repFtime > 24) {
                            _repFtime = 24;
                        }
                    }

                    flags = ANIMATE;
                    previousAge = critterGetStat(obj_dude, STAT_AGE);

                    if (change == 1) {
                        if (previousAge < 35) {
                            if (inc_stat(obj_dude, STAT_AGE) != 0) {
                                flags = 0;
                            }
                        }
                    } else {
                        if (previousAge >= 16) {
                            if (dec_stat(obj_dude, STAT_AGE) != 0) {
                                flags = 0;
                            }
                        }
                    }

                    age = critterGetStat(obj_dude, STAT_AGE);
                    PrintBigNum(55, 10, flags, age, previousAge, win);
                    if (flags == ANIMATE) {
                        PrintAgeBig();
                        PrintBasicStat(RENDER_ALL_STATS, 0, 0);
                        ListDrvdStats();
                        win_draw(edit_win);
                        win_draw(win);
                    }
                }

                if (v33 > 14.4) {
                    while (elapsed_time(_frame_time) < 1000 / _repFtime)
                        ;
                } else {
                    while (elapsed_time(_frame_time) < 1000 / 24)
                        ;
                }

                keyCode = get_input();
                if (keyCode == 503 || keyCode == 504 || game_user_wants_to_quit != 0) {
                    break;
                }
            }
        } else {
            win_draw(win);

            while (elapsed_time(_frame_time) < 1000 / 24)
                ;
        }
    }

    stat_set_base(obj_dude, STAT_AGE, savedAge);
    PrintAgeBig();
    PrintBasicStat(RENDER_ALL_STATS, 0, 0);
    ListDrvdStats();
    win_draw(edit_win);
    win_draw(win);
    win_delete(win);
    return 0;
}

// 0x437664
static void SexWindow()
{
    char* text;

    int windowWidth = GInfo[EDITOR_GRAPHIC_CHARWIN].width;
    int windowHeight = GInfo[EDITOR_GRAPHIC_CHARWIN].height;

    int genderWindowX = 9
        + GInfo[EDITOR_GRAPHIC_NAME_ON].width
        + GInfo[EDITOR_GRAPHIC_AGE_ON].width;
    int genderWindowY = 0;
    int win = win_add(genderWindowX, genderWindowY, windowWidth, windowHeight, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);

    if (win == -1) {
        return;
    }

    unsigned char* windowBuf = win_get_buf(win);

    // Copy background
    memcpy(windowBuf, grphbmp[EDITOR_GRAPHIC_CHARWIN], windowWidth * windowHeight);

    trans_buf_to_buf(grphbmp[EDITOR_GRAPHIC_DONE_BOX],
        GInfo[EDITOR_GRAPHIC_DONE_BOX].width,
        GInfo[EDITOR_GRAPHIC_DONE_BOX].height,
        GInfo[EDITOR_GRAPHIC_DONE_BOX].width,
        windowBuf + windowWidth * 44 + 15,
        windowWidth);

    text_font(103);

    text = getmsg(&editor_message_file, &mesg, 100);
    text_to_buf(windowBuf + windowWidth * 48 + 52, text, windowWidth, windowWidth, colorTable[18979]);

    int doneBtn = win_register_button(win,
        28,
        48,
        GInfo[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        GInfo[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        500,
        grphbmp[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        grphbmp[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (doneBtn != -1) {
        win_register_button_sound_func(doneBtn, gsound_red_butt_press, gsound_red_butt_release);
    }

    int btns[2];
    btns[0] = win_register_button(win,
        22,
        2,
        GInfo[EDITOR_GRAPHIC_MALE_ON].width,
        GInfo[EDITOR_GRAPHIC_MALE_ON].height,
        -1,
        -1,
        501,
        -1,
        grphbmp[EDITOR_GRAPHIC_MALE_OFF],
        grphbmp[EDITOR_GRAPHIC_MALE_ON],
        NULL,
        BUTTON_FLAG_TRANSPARENT | BUTTON_FLAG_0x04 | BUTTON_FLAG_0x02 | BUTTON_FLAG_0x01);
    if (btns[0] != -1) {
        win_register_button_sound_func(doneBtn, gsound_red_butt_press, NULL);
    }

    btns[1] = win_register_button(win,
        71,
        3,
        GInfo[EDITOR_GRAPHIC_FEMALE_ON].width,
        GInfo[EDITOR_GRAPHIC_FEMALE_ON].height,
        -1,
        -1,
        502,
        -1,
        grphbmp[EDITOR_GRAPHIC_FEMALE_OFF],
        grphbmp[EDITOR_GRAPHIC_FEMALE_ON],
        NULL,
        BUTTON_FLAG_TRANSPARENT | BUTTON_FLAG_0x04 | BUTTON_FLAG_0x02 | BUTTON_FLAG_0x01);
    if (btns[1] != -1) {
        win_group_radio_buttons(2, btns);
        win_register_button_sound_func(doneBtn, gsound_red_butt_press, NULL);
    }

    int savedGender = critterGetStat(obj_dude, STAT_GENDER);
    win_set_button_rest_state(btns[savedGender], 1, 0);

    while (true) {
        _frame_time = get_time();

        int eventCode = get_input();

        if (eventCode == KEY_RETURN || eventCode == 500) {
            if (eventCode == KEY_RETURN) {
                gsound_play_sfx_file("ib1p1xx1");
            }
            break;
        }

        if (eventCode == KEY_ESCAPE || game_user_wants_to_quit != 0) {
            stat_set_base(obj_dude, STAT_GENDER, savedGender);
            PrintBasicStat(RENDER_ALL_STATS, 0, 0);
            ListDrvdStats();
            win_draw(edit_win);
            break;
        }

        switch (eventCode) {
        case KEY_ARROW_LEFT:
        case KEY_ARROW_RIGHT:
            if (1) {
                bool wasMale = win_button_down(btns[0]);
                win_set_button_rest_state(btns[0], !wasMale, 1);
                win_set_button_rest_state(btns[1], wasMale, 1);
            }
            break;
        case 501:
        case 502:
            // TODO: Original code is slightly different.
            stat_set_base(obj_dude, STAT_GENDER, eventCode - 501);
            PrintBasicStat(RENDER_ALL_STATS, 0, 0);
            ListDrvdStats();
            break;
        }

        win_draw(win);

        while (elapsed_time(_frame_time) < 41)
            ;
    }

    PrintGender();
    win_delete(win);
}

// 0x4379BC
static void StatButton(int eventCode)
{
    _repFtime = 4;

    int savedRemainingCharacterPoints = character_points;

    if (!glblmode) {
        return;
    }

    int incrementingStat = eventCode - 503;
    int decrementingStat = eventCode - 510;

    int v11 = 0;

    bool cont = true;
    do {
        _frame_time = get_time();
        if (v11 <= 19.2) {
            v11++;
        }

        if (v11 == 1 || v11 > 19.2) {
            if (v11 > 19.2) {
                _repFtime++;
                if (_repFtime > 24) {
                    _repFtime = 24;
                }
            }

            if (eventCode >= 510) {
                int previousValue = critterGetStat(obj_dude, decrementingStat);
                if (dec_stat(obj_dude, decrementingStat) == 0) {
                    character_points++;
                } else {
                    cont = false;
                }

                PrintBasicStat(decrementingStat, cont ? ANIMATE : 0, previousValue);
                PrintBigNum(126, 282, cont ? ANIMATE : 0, character_points, savedRemainingCharacterPoints, edit_win);
                stat_recalc_derived(obj_dude);
                ListDrvdStats();
                ListSkills(0);
                info_line = decrementingStat;
            } else {
                int previousValue = stat_get_base(obj_dude, incrementingStat);
                previousValue += stat_get_bonus(obj_dude, incrementingStat);
                if (character_points > 0 && previousValue < 10 && inc_stat(obj_dude, incrementingStat) == 0) {
                    character_points--;
                } else {
                    cont = false;
                }

                PrintBasicStat(incrementingStat, cont ? ANIMATE : 0, previousValue);
                PrintBigNum(126, 282, cont ? ANIMATE : 0, character_points, savedRemainingCharacterPoints, edit_win);
                stat_recalc_derived(obj_dude);
                ListDrvdStats();
                ListSkills(0);
                info_line = incrementingStat;
            }

            win_draw(edit_win);
        }

        if (v11 >= 19.2) {
            unsigned int delay = 1000 / _repFtime;
            while (elapsed_time(_frame_time) < delay) {
            }
        } else {
            while (elapsed_time(_frame_time) < 1000 / 24) {
            }
        }
    } while (get_input() != 518 && cont);

    DrawInfoWin();
}

// handle options dialog
//
// 0x437C08
static int OptionWindow()
{
    int width = GInfo[43].width;
    int height = GInfo[43].height;

    // NOTE: The following is a block of general purpose string buffers used in
    // this function. They are either store path, or strings from .msg files. I
    // don't know if such usage was intentional in the original code or it's a
    // result of some kind of compiler optimization.
    char string1[512];
    char string2[512];
    char string3[512];
    char string4[512];
    char string5[512];

    // Only two of the these blocks are used as a dialog body. Depending on the
    // dialog either 1 or 2 strings used from this array.
    const char* dialogBody[2] = {
        string5,
        string2,
    };

    if (glblmode) {
        int optionsWindowX = 238;
        int optionsWindowY = 90;
        int win = win_add(optionsWindowX, optionsWindowY, GInfo[41].width, GInfo[41].height, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);
        if (win == -1) {
            return -1;
        }

        unsigned char* windowBuffer = win_get_buf(win);
        memcpy(windowBuffer, grphbmp[41], GInfo[41].width * GInfo[41].height);

        text_font(103);

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
                down[index] = (unsigned char*)mem_malloc(size);
                if (down[index] == NULL) {
                    err = 1;
                    break;
                }

                up[index] = (unsigned char*)mem_malloc(size);
                if (up[index] == NULL) {
                    err = 2;
                    break;
                }

                memcpy(down[index], grphbmp[43], size);
                memcpy(up[index], grphbmp[42], size);

                strcpy(string4, getmsg(&editor_message_file, &mesg, 600 + index));

                int offset = width * 7 + width / 2 - text_width(string4) / 2;
                text_to_buf(up[index] + offset, string4, width, width, colorTable[18979]);
                text_to_buf(down[index] + offset, string4, width, width, colorTable[14723]);

                int btn = win_register_button(win, 13, y, width, height, -1, -1, -1, 500 + index, up[index], down[index], NULL, BUTTON_FLAG_TRANSPARENT);
                if (btn != -1) {
                    win_register_button_sound_func(btn, gsound_lrg_butt_press, NULL);
                }
            } while (0);

            y += height + 3;
        }

        if (err != 0) {
            if (err == 2) {
                mem_free(down[index]);
            }

            while (--index >= 0) {
                mem_free(up[index]);
                mem_free(down[index]);
            }

            return -1;
        }

        text_font(101);

        int rc = 0;
        while (rc == 0) {
            int keyCode = get_input();

            if (game_user_wants_to_quit != 0) {
                rc = 2;
            } else if (keyCode == 504) {
                rc = 2;
            } else if (keyCode == KEY_RETURN || keyCode == KEY_UPPERCASE_D || keyCode == KEY_LOWERCASE_D) {
                // DONE
                rc = 2;
                gsound_play_sfx_file("ib1p1xx1");
            } else if (keyCode == KEY_ESCAPE) {
                rc = 2;
            } else if (keyCode == 503 || keyCode == KEY_UPPERCASE_E || keyCode == KEY_LOWERCASE_E) {
                // ERASE
                strcpy(string5, getmsg(&editor_message_file, &mesg, 605));
                strcpy(string2, getmsg(&editor_message_file, &mesg, 606));

                if (dialog_out(NULL, dialogBody, 2, 169, 126, colorTable[992], NULL, colorTable[992], DIALOG_BOX_YES_NO) != 0) {
                    ResetPlayer();
                    skill_get_tags(temp_tag_skill, NUM_TAGGED_SKILLS);

                    // NOTE: Uninline.
                    tagskill_count = tagskl_free();

                    trait_get(&temp_trait[0], &temp_trait[1]);

                    // NOTE: Uninline.
                    trait_count = get_trait_count();
                    stat_recalc_derived(obj_dude);
                    ResetScreen();
                }
            } else if (keyCode == 502 || keyCode == KEY_UPPERCASE_P || keyCode == KEY_LOWERCASE_P) {
                // PRINT TO FILE
                string4[0] = '\0';

                strcat(string4, "*.");
                strcat(string4, "TXT");

                char** fileList;
                int fileListLength = db_get_file_list(string4, &fileList, 0, 0);
                if (fileListLength != -1) {
                    // PRINT
                    strcpy(string1, getmsg(&editor_message_file, &mesg, 616));

                    // PRINT TO FILE
                    strcpy(string4, getmsg(&editor_message_file, &mesg, 602));

                    if (save_file_dialog(string4, fileList, string1, fileListLength, 168, 80, 0) == 0) {
                        strcat(string1, ".");
                        strcat(string1, "TXT");

                        string4[0] = '\0';
                        strcat(string4, string1);

                        if (!db_access(string4)) {
                            // already exists
                            sprintf(string4,
                                "%s %s",
                                strupr(string1),
                                getmsg(&editor_message_file, &mesg, 609));

                            strcpy(string5, getmsg(&editor_message_file, &mesg, 610));

                            if (dialog_out(string4, dialogBody, 1, 169, 126, colorTable[32328], NULL, colorTable[32328], 0x10) != 0) {
                                rc = 1;
                            } else {
                                rc = 0;
                            }
                        } else {
                            rc = 1;
                        }

                        if (rc != 0) {
                            string4[0] = '\0';
                            strcat(string4, string1);

                            if (Save_as_ASCII(string4) == 0) {
                                sprintf(string4,
                                    "%s%s",
                                    strupr(string1),
                                    getmsg(&editor_message_file, &mesg, 607));
                                dialog_out(string4, NULL, 0, 169, 126, colorTable[992], NULL, colorTable[992], 0);
                            } else {
                                gsound_play_sfx_file("iisxxxx1");

                                sprintf(string4,
                                    "%s%s%s",
                                    getmsg(&editor_message_file, &mesg, 611),
                                    strupr(string1),
                                    "!");
                                dialog_out(string4, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[992], 0x01);
                            }
                        }
                    }

                    db_free_file_list(&fileList, 0);
                } else {
                    gsound_play_sfx_file("iisxxxx1");

                    strcpy(string4, getmsg(&editor_message_file, &mesg, 615));
                    dialog_out(string4, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[32328], 0);

                    rc = 0;
                }
            } else if (keyCode == 501 || keyCode == KEY_UPPERCASE_L || keyCode == KEY_LOWERCASE_L) {
                // LOAD
                string4[0] = '\0';
                strcat(string4, "*.");
                strcat(string4, "GCD");

                char** fileNameList;
                int fileNameListLength = db_get_file_list(string4, &fileNameList, 0, 0);
                if (fileNameListLength != -1) {
                    // NOTE: This value is not copied as in save dialog.
                    char* title = getmsg(&editor_message_file, &mesg, 601);
                    int loadFileDialogRc = file_dialog(title, fileNameList, string3, fileNameListLength, 168, 80, 0);
                    if (loadFileDialogRc == -1) {
                        db_free_file_list(&fileNameList, 0);
                        // FIXME: This branch ignores cleanup at the end of the loop.
                        return -1;
                    }

                    if (loadFileDialogRc == 0) {
                        string4[0] = '\0';
                        strcat(string4, string3);

                        int oldRemainingCharacterPoints = character_points;

                        ResetPlayer();

                        if (pc_load_data(string4) == 0) {
                            // NOTE: Uninline.
                            CheckValidPlayer();

                            skill_get_tags(temp_tag_skill, 4);

                            // NOTE: Uninline.
                            tagskill_count = tagskl_free();

                            trait_get(&(temp_trait[0]), &(temp_trait[1]));

                            // NOTE: Uninline.
                            trait_count = get_trait_count();

                            stat_recalc_derived(obj_dude);

                            critter_adjust_hits(obj_dude, 1000);

                            rc = 1;
                        } else {
                            RestorePlayer();
                            character_points = oldRemainingCharacterPoints;
                            critter_adjust_hits(obj_dude, 1000);
                            gsound_play_sfx_file("iisxxxx1");

                            strcpy(string4, getmsg(&editor_message_file, &mesg, 612));
                            strcat(string4, string3);
                            strcat(string4, "!");

                            dialog_out(string4, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[32328], 0);
                        }

                        ResetScreen();
                    }

                    db_free_file_list(&fileNameList, 0);
                } else {
                    gsound_play_sfx_file("iisxxxx1");

                    // Error reading file list!
                    strcpy(string4, getmsg(&editor_message_file, &mesg, 615));
                    rc = 0;

                    dialog_out(string4, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[32328], 0);
                }
            } else if (keyCode == 500 || keyCode == KEY_UPPERCASE_S || keyCode == KEY_LOWERCASE_S) {
                // SAVE
                string4[0] = '\0';
                strcat(string4, "*.");
                strcat(string4, "GCD");

                char** fileNameList;
                int fileNameListLength = db_get_file_list(string4, &fileNameList, 0, 0);
                if (fileNameListLength != -1) {
                    strcpy(string1, getmsg(&editor_message_file, &mesg, 617));
                    strcpy(string4, getmsg(&editor_message_file, &mesg, 600));

                    if (save_file_dialog(string4, fileNameList, string1, fileNameListLength, 168, 80, 0) == 0) {
                        strcat(string1, ".");
                        strcat(string1, "GCD");

                        string4[0] = '\0';
                        strcat(string4, string1);

                        bool shouldSave;
                        if (db_access(string4)) {
                            sprintf(string4, "%s %s",
                                strupr(string1),
                                getmsg(&editor_message_file, &mesg, 609));
                            strcpy(string5, getmsg(&editor_message_file, &mesg, 610));

                            if (dialog_out(string4, dialogBody, 1, 169, 126, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_YES_NO) != 0) {
                                shouldSave = true;
                            } else {
                                shouldSave = false;
                            }
                        } else {
                            shouldSave = true;
                        }

                        if (shouldSave) {
                            skill_set_tags(temp_tag_skill, 4);
                            trait_set(temp_trait[0], temp_trait[1]);

                            string4[0] = '\0';
                            strcat(string4, string1);

                            if (pc_save_data(string4) != 0) {
                                gsound_play_sfx_file("iisxxxx1");
                                sprintf(string4, "%s%s!",
                                    strupr(string1),
                                    getmsg(&editor_message_file, &mesg, 611));
                                dialog_out(string4, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_LARGE);
                                rc = 0;
                            } else {
                                sprintf(string4, "%s%s",
                                    strupr(string1),
                                    getmsg(&editor_message_file, &mesg, 607));
                                dialog_out(string4, NULL, 0, 169, 126, colorTable[992], NULL, colorTable[992], DIALOG_BOX_LARGE);
                                rc = 1;
                            }
                        }
                    }

                    db_free_file_list(&fileNameList, 0);
                } else {
                    gsound_play_sfx_file("iisxxxx1");

                    // Error reading file list!
                    char* msg = getmsg(&editor_message_file, &mesg, 615);
                    dialog_out(msg, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[32328], 0);

                    rc = 0;
                }
            }

            win_draw(win);
        }

        win_delete(win);

        for (index = 0; index < 5; index++) {
            mem_free(up[index]);
            mem_free(down[index]);
        }

        return 0;
    }

    // Character Editor is not in creation mode - this button is only for
    // printing character details.

    char pattern[512];
    strcpy(pattern, "*.TXT");

    char** fileNames;
    int filesCount = db_get_file_list(pattern, &fileNames, 0, 0);
    if (filesCount == -1) {
        gsound_play_sfx_file("iisxxxx1");

        // Error reading file list!
        strcpy(pattern, getmsg(&editor_message_file, &mesg, 615));
        dialog_out(pattern, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[32328], 0);
        return 0;
    }

    // PRINT
    char fileName[512];
    strcpy(fileName, getmsg(&editor_message_file, &mesg, 616));

    char title[512];
    strcpy(title, getmsg(&editor_message_file, &mesg, 602));

    if (save_file_dialog(title, fileNames, fileName, filesCount, 168, 80, 0) == 0) {
        strcat(fileName, ".TXT");

        title[0] = '\0';
        strcat(title, fileName);

        int v42 = 0;
        if (db_access(title)) {
            sprintf(title,
                "%s %s",
                strupr(fileName),
                getmsg(&editor_message_file, &mesg, 609));

            char line2[512];
            strcpy(line2, getmsg(&editor_message_file, &mesg, 610));

            const char* lines[] = { line2 };
            v42 = dialog_out(title, lines, 1, 169, 126, colorTable[32328], NULL, colorTable[32328], 0x10);
            if (v42) {
                v42 = 1;
            }
        } else {
            v42 = 1;
        }

        if (v42) {
            title[0] = '\0';
            strcpy(title, fileName);

            if (Save_as_ASCII(title) != 0) {
                gsound_play_sfx_file("iisxxxx1");

                sprintf(title,
                    "%s%s%s",
                    getmsg(&editor_message_file, &mesg, 611),
                    strupr(fileName),
                    "!");
                dialog_out(title, NULL, 0, 169, 126, colorTable[32328], NULL, colorTable[32328], 1);
            }
        }
    }

    db_free_file_list(&fileNames, 0);

    return 0;
}

// 0x4390B4
bool db_access(const char* fname)
{
    File* stream = db_fopen(fname, "rb");
    if (stream == NULL) {
        return false;
    }

    db_fclose(stream);
    return true;
}

// 0x4390D0
static int Save_as_ASCII(const char* fileName)
{
    File* stream = db_fopen(fileName, "wt");
    if (stream == NULL) {
        return -1;
    }

    db_fputs("\n", stream);
    db_fputs("\n", stream);

    char title1[256];
    char title2[256];
    char title3[256];
    char padding[256];

    // FALLOUT
    strcpy(title1, getmsg(&editor_message_file, &mesg, 620));

    // NOTE: Uninline.
    padding[0] = '\0';
    AddSpaces(padding, (80 - strlen(title1)) / 2 - 2);

    strcat(padding, title1);
    strcat(padding, "\n");
    db_fputs(padding, stream);

    // VAULT-13 PERSONNEL RECORD
    strcpy(title1, getmsg(&editor_message_file, &mesg, 621));

    // NOTE: Uninline.
    padding[0] = '\0';
    AddSpaces(padding, (80 - strlen(title1)) / 2 - 2);

    strcat(padding, title1);
    strcat(padding, "\n");
    db_fputs(padding, stream);

    int month;
    int day;
    int year;
    game_time_date(&month, &day, &year);

    sprintf(title1, "%.2d %s %d  %.4d %s",
        day,
        getmsg(&editor_message_file, &mesg, 500 + month - 1),
        year,
        game_time_hour(),
        getmsg(&editor_message_file, &mesg, 622));

    // NOTE: Uninline.
    padding[0] = '\0';
    AddSpaces(padding, (80 - strlen(title1)) / 2 - 2);

    strcat(padding, title1);
    strcat(padding, "\n");
    db_fputs(padding, stream);

    // Blank line
    db_fputs("\n", stream);

    // Name
    sprintf(title1,
        "%s %s",
        getmsg(&editor_message_file, &mesg, 642),
        critter_name(obj_dude));

    int paddingLength = 27 - strlen(title1);
    if (paddingLength > 0) {
        // NOTE: Uninline.
        padding[0] = '\0';
        AddSpaces(padding, paddingLength);

        strcat(title1, padding);
    }

    // Age
    sprintf(title2,
        "%s%s %d",
        title1,
        getmsg(&editor_message_file, &mesg, 643),
        critterGetStat(obj_dude, STAT_AGE));

    // Gender
    sprintf(title3,
        "%s%s %s",
        title2,
        getmsg(&editor_message_file, &mesg, 644),
        getmsg(&editor_message_file, &mesg, 645 + critterGetStat(obj_dude, STAT_GENDER)));

    db_fputs(title3, stream);
    db_fputs("\n", stream);

    sprintf(title1,
        "%s %.2d %s %s ",
        getmsg(&editor_message_file, &mesg, 647),
        stat_pc_get(PC_STAT_LEVEL),
        getmsg(&editor_message_file, &mesg, 648),
        itostndn(stat_pc_get(PC_STAT_EXPERIENCE), title3));

    paddingLength = 12 - strlen(title3);
    if (paddingLength > 0) {
        // NOTE: Uninline.
        padding[0] = '\0';
        AddSpaces(padding, paddingLength);

        strcat(title1, padding);
    }

    sprintf(title2,
        "%s%s %s",
        title1,
        getmsg(&editor_message_file, &mesg, 649),
        itostndn(stat_pc_min_exp(), title3));
    db_fputs(title2, stream);
    db_fputs("\n", stream);
    db_fputs("\n", stream);

    // Statistics
    sprintf(title1, "%s\n", getmsg(&editor_message_file, &mesg, 623));

    // Strength / Hit Points / Sequence
    //
    // FIXME: There is bug - it shows strength instead of sequence.
    sprintf(title1,
        "%s %.2d %s %.3d/%.3d %s %.2d",
        getmsg(&editor_message_file, &mesg, 624),
        critterGetStat(obj_dude, STAT_STRENGTH),
        getmsg(&editor_message_file, &mesg, 625),
        critter_get_hits(obj_dude),
        critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS),
        getmsg(&editor_message_file, &mesg, 626),
        critterGetStat(obj_dude, STAT_STRENGTH));
    db_fputs(title1, stream);
    db_fputs("\n", stream);

    // Perception / Armor Class / Healing Rate
    sprintf(title1,
        "%s %.2d %s %.3d %s %.2d",
        getmsg(&editor_message_file, &mesg, 627),
        critterGetStat(obj_dude, STAT_PERCEPTION),
        getmsg(&editor_message_file, &mesg, 628),
        critterGetStat(obj_dude, STAT_ARMOR_CLASS),
        getmsg(&editor_message_file, &mesg, 629),
        critterGetStat(obj_dude, STAT_HEALING_RATE));
    db_fputs(title1, stream);
    db_fputs("\n", stream);

    // Endurance / Action Points / Critical Chance
    sprintf(title1,
        "%s %.2d %s %.2d %s %.3d%%",
        getmsg(&editor_message_file, &mesg, 630),
        critterGetStat(obj_dude, STAT_ENDURANCE),
        getmsg(&editor_message_file, &mesg, 631),
        critterGetStat(obj_dude, STAT_MAXIMUM_ACTION_POINTS),
        getmsg(&editor_message_file, &mesg, 632),
        critterGetStat(obj_dude, STAT_CRITICAL_CHANCE));
    db_fputs(title1, stream);
    db_fputs("\n", stream);

    // Charisma / Melee Damage / Carry Weight
    sprintf(title1,
        "%s %.2d %s %.2d %s %.3d lbs.",
        getmsg(&editor_message_file, &mesg, 633),
        critterGetStat(obj_dude, STAT_CHARISMA),
        getmsg(&editor_message_file, &mesg, 634),
        critterGetStat(obj_dude, STAT_MELEE_DAMAGE),
        getmsg(&editor_message_file, &mesg, 635),
        critterGetStat(obj_dude, STAT_CARRY_WEIGHT));
    db_fputs(title1, stream);
    db_fputs("\n", stream);

    // Intelligence / Damage Resistance
    sprintf(title1,
        "%s %.2d %s %.3d%%",
        getmsg(&editor_message_file, &mesg, 636),
        critterGetStat(obj_dude, STAT_INTELLIGENCE),
        getmsg(&editor_message_file, &mesg, 637),
        critterGetStat(obj_dude, STAT_DAMAGE_RESISTANCE));
    db_fputs(title1, stream);
    db_fputs("\n", stream);

    // Agility / Radiation Resistance
    sprintf(title1,
        "%s %.2d %s %.3d%%",
        getmsg(&editor_message_file, &mesg, 638),
        critterGetStat(obj_dude, STAT_AGILITY),
        getmsg(&editor_message_file, &mesg, 639),
        critterGetStat(obj_dude, STAT_RADIATION_RESISTANCE));
    db_fputs(title1, stream);
    db_fputs("\n", stream);

    // Luck / Poison Resistance
    sprintf(title1,
        "%s %.2d %s %.3d%%",
        getmsg(&editor_message_file, &mesg, 640),
        critterGetStat(obj_dude, STAT_LUCK),
        getmsg(&editor_message_file, &mesg, 641),
        critterGetStat(obj_dude, STAT_POISON_RESISTANCE));
    db_fputs(title1, stream);
    db_fputs("\n", stream);

    db_fputs("\n", stream);
    db_fputs("\n", stream);

    if (temp_trait[0] != -1) {
        // ::: Traits :::
        sprintf(title1, "%s\n", getmsg(&editor_message_file, &mesg, 650));
        db_fputs(title1, stream);

        // NOTE: The original code does not use loop, or it was optimized away.
        for (int index = 0; index < TRAITS_MAX_SELECTED_COUNT; index++) {
            if (temp_trait[index] != -1) {
                sprintf(title1, "  %s", trait_name(temp_trait[index]));
                db_fputs(title1, stream);
                db_fputs("\n", stream);
            }
        }
    }

    int perk = 0;
    for (; perk < PERK_COUNT; perk++) {
        if (perk_level(obj_dude, perk) != 0) {
            break;
        }
    }

    if (perk < PERK_COUNT) {
        // ::: Perks :::
        sprintf(title1, "%s\n", getmsg(&editor_message_file, &mesg, 651));
        db_fputs(title1, stream);

        for (perk = 0; perk < PERK_COUNT; perk++) {
            int rank = perk_level(obj_dude, perk);
            if (rank != 0) {
                if (rank == 1) {
                    sprintf(title1, "  %s", perk_name(perk));
                } else {
                    sprintf(title1, "  %s (%d)", perk_name(perk), rank);
                }

                db_fputs(title1, stream);
                db_fputs("\n", stream);
            }
        }
    }

    db_fputs("\n", stream);

    // ::: Karma :::
    sprintf(title1, "%s\n", getmsg(&editor_message_file, &mesg, 652));
    db_fputs(title1, stream);

    for (int index = 0; index < karma_vars_count; index++) {
        KarmaEntry* karmaEntry = &(karma_vars[index]);
        if (karmaEntry->gvar == GVAR_PLAYER_REPUTATION) {
            int reputation = 0;
            for (; reputation < general_reps_count; reputation++) {
                GenericReputationEntry* reputationDescription = &(general_reps[reputation]);
                if (game_global_vars[GVAR_PLAYER_REPUTATION] >= reputationDescription->threshold) {
                    break;
                }
            }

            if (reputation < general_reps_count) {
                GenericReputationEntry* reputationDescription = &(general_reps[reputation]);
                sprintf(title1,
                    "  %s: %s (%s)",
                    getmsg(&editor_message_file, &mesg, 125),
                    itoa(game_global_vars[GVAR_PLAYER_REPUTATION], title2, 10),
                    getmsg(&editor_message_file, &mesg, reputationDescription->name));
                db_fputs(title1, stream);
                db_fputs("\n", stream);
            }
        } else {
            if (game_global_vars[karmaEntry->gvar] != 0) {
                sprintf(title1, "  %s", getmsg(&editor_message_file, &mesg, karmaEntry->name));
                db_fputs(title1, stream);
                db_fputs("\n", stream);
            }
        }
    }

    bool hasTownReputationHeading = false;
    for (int index = 0; index < TOWN_REPUTATION_COUNT; index++) {
        const TownReputationEntry* pair = &(town_rep_info[index]);
        if (wmAreaIsKnown(pair->city)) {
            if (!hasTownReputationHeading) {
                db_fputs("\n", stream);

                // ::: Reputation :::
                sprintf(title1, "%s\n", getmsg(&editor_message_file, &mesg, 657));
                db_fputs(title1, stream);
                hasTownReputationHeading = true;
            }

            wmGetAreaIdxName(pair->city, title2);

            int townReputation = game_global_vars[pair->gvar];

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
                getmsg(&editor_message_file, &mesg, townReputationMessageId));
            db_fputs(title1, stream);
            db_fputs("\n", stream);
        }
    }

    bool hasAddictionsHeading = false;
    for (int index = 0; index < ADDICTION_REPUTATION_COUNT; index++) {
        if (game_global_vars[addiction_vars[index]] != 0) {
            if (!hasAddictionsHeading) {
                db_fputs("\n", stream);

                // ::: Addictions :::
                sprintf(title1, "%s\n", getmsg(&editor_message_file, &mesg, 656));
                db_fputs(title1, stream);
                hasAddictionsHeading = true;
            }

            sprintf(title1,
                "  %s",
                getmsg(&editor_message_file, &mesg, 1004 + index));
            db_fputs(title1, stream);
            db_fputs("\n", stream);
        }
    }

    db_fputs("\n", stream);

    // ::: Skills ::: / ::: Kills :::
    sprintf(title1, "%s\n", getmsg(&editor_message_file, &mesg, 653));
    db_fputs(title1, stream);

    int killType = 0;
    for (int skill = 0; skill < SKILL_COUNT; skill++) {
        sprintf(title1, "%s ", skill_name(skill));

        // NOTE: Uninline.
        AddDots(title1 + strlen(title1), 16 - strlen(title1));

        bool hasKillType = false;

        for (; killType < KILL_TYPE_COUNT; killType++) {
            int killsCount = critter_kill_count(killType);
            if (killsCount > 0) {
                sprintf(title2, "%s ", critter_kill_name(killType));

                // NOTE: Uninline.
                AddDots(title2 + strlen(title2), 16 - strlen(title2));

                sprintf(title3,
                    "  %s %.3d%%        %s %.3d\n",
                    title1,
                    skill_level(obj_dude, skill),
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
                skill_level(obj_dude, skill));
        }
    }

    db_fputs("\n", stream);
    db_fputs("\n", stream);

    // ::: Inventory :::
    sprintf(title1, "%s\n", getmsg(&editor_message_file, &mesg, 654));
    db_fputs(title1, stream);

    Inventory* inventory = &(obj_dude->data.inventory);
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
                itostndn(inventoryItem->quantity, title3),
                object_name(inventoryItem->item));

            int length = 25 - strlen(title2);
            if (length < 0) {
                length = 0;
            }

            AddSpaces(title2, length);

            strcat(title1, title2);
        }

        strcat(title1, "\n");
        db_fputs(title1, stream);
    }

    db_fputs("\n", stream);

    // Total Weight:
    sprintf(title1,
        "%s %d lbs.",
        getmsg(&editor_message_file, &mesg, 655),
        item_total_weight(obj_dude));
    db_fputs(title1, stream);

    db_fputs("\n", stream);
    db_fputs("\n", stream);
    db_fputs("\n", stream);
    db_fclose(stream);

    return 0;
}

// 0x43A55C
char* AddSpaces(char* string, int length)
{
    char* pch = string + strlen(string);

    for (int index = 0; index < length; index++) {
        *pch++ = ' ';
    }

    *pch = '\0';

    return string;
}

// NOTE: Inlined.
//
// 0x43A58C
static char* AddDots(char* string, int length)
{
    char* pch = string + strlen(string);

    for (int index = 0; index < length; index++) {
        *pch++ = '.';
    }

    *pch = '\0';

    return string;
}

// 0x43A4BC
static void ResetScreen()
{
    info_line = 0;
    skill_cursor = 0;
    slider_y = 27;
    folder = 0;

    if (glblmode) {
        PrintBigNum(126, 282, 0, character_points, 0, edit_win);
    } else {
        DrawFolder();
        PrintLevelWin();
    }

    PrintBigname();
    PrintAgeBig();
    PrintGender();
    ListTraits();
    ListSkills(0);
    PrintBasicStat(7, 0, 0);
    ListDrvdStats();
    DrawInfoWin();
    win_draw(edit_win);
}

// 0x43A5BC
static void RegInfoAreas()
{
    win_register_button(edit_win, 19, 38, 125, 227, -1, -1, 525, -1, NULL, NULL, NULL, 0);
    win_register_button(edit_win, 28, 280, 124, 32, -1, -1, 526, -1, NULL, NULL, NULL, 0);

    if (glblmode) {
        win_register_button(edit_win, 52, 324, 169, 20, -1, -1, 533, -1, NULL, NULL, NULL, 0);
        win_register_button(edit_win, 47, 353, 245, 100, -1, -1, 534, -1, NULL, NULL, NULL, 0);
    } else {
        win_register_button(edit_win, 28, 363, 283, 105, -1, -1, 527, -1, NULL, NULL, NULL, 0);
    }

    win_register_button(edit_win, 191, 41, 122, 110, -1, -1, 528, -1, NULL, NULL, NULL, 0);
    win_register_button(edit_win, 191, 175, 122, 135, -1, -1, 529, -1, NULL, NULL, NULL, 0);
    win_register_button(edit_win, 376, 5, 223, 20, -1, -1, 530, -1, NULL, NULL, NULL, 0);
    win_register_button(edit_win, 370, 27, 223, 195, -1, -1, 531, -1, NULL, NULL, NULL, 0);
    win_register_button(edit_win, 396, 228, 171, 25, -1, -1, 532, -1, NULL, NULL, NULL, 0);
}

// NOTE: Inlined.
//
// 0x43A79C
static int CheckValidPlayer()
{
    int stat;

    stat_recalc_derived(obj_dude);
    stat_pc_set_defaults();

    for (stat = 0; stat < SAVEABLE_STAT_COUNT; stat++) {
        stat_set_bonus(obj_dude, stat, 0);
    }

    perk_reset();
    stat_recalc_derived(obj_dude);

    return 1;
}

// copy character to editor
//
// 0x43A7DC
static void SavePlayer()
{
    Proto* proto;
    proto_ptr(obj_dude->pid, &proto);
    critter_copy(&dude_data, &(proto->critter.data));

    hp_back = critter_get_hits(obj_dude);

    strncpy(name_save, critter_name(obj_dude), 32);

    last_level_back = last_level;

    // NOTE: Uninline.
    push_perks();

    free_perk_back = free_perk;

    upsent_points_back = stat_pc_get(PC_STAT_UNSPENT_SKILL_POINTS);

    skill_get_tags(tag_skill_back, NUM_TAGGED_SKILLS);

    trait_get(&(trait_back[0]), &(trait_back[1]));

    for (int skill = 0; skill < SKILL_COUNT; skill++) {
        skillsav[skill] = skill_level(obj_dude, skill);
    }
}

// copy editor to character
//
// 0x43A8BC
static void RestorePlayer()
{
    Proto* proto;
    int cur_hp;

    pop_perks();

    proto_ptr(obj_dude->pid, &proto);
    critter_copy(&(proto->critter.data), &dude_data);

    critter_pc_set_name(name_save);

    last_level = last_level_back;
    free_perk = free_perk_back;

    stat_pc_set(PC_STAT_UNSPENT_SKILL_POINTS, upsent_points_back);

    skill_set_tags(tag_skill_back, NUM_TAGGED_SKILLS);

    trait_set(trait_back[0], trait_back[1]);

    skill_get_tags(temp_tag_skill, NUM_TAGGED_SKILLS);

    // NOTE: Uninline.
    tagskill_count = tagskl_free();

    trait_get(&(temp_trait[0]), &(temp_trait[1]));

    // NOTE: Uninline.
    trait_count = get_trait_count();

    stat_recalc_derived(obj_dude);

    cur_hp = critter_get_hits(obj_dude);
    critter_adjust_hits(obj_dude, hp_back - cur_hp);
}

// 0x43A9CC
char* itostndn(int value, char* dest)
{
    // 0x431DD4
    static const int v16[7] = {
        1000000,
        100000,
        10000,
        1000,
        100,
        10,
        1,
    };

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
static int DrawCard(int graphicId, const char* name, const char* attributes, char* description)
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

    fid = art_id(OBJ_TYPE_SKILLDEX, graphicId, 0, 0, 0);
    buf = art_lock(fid, &graphicHandle, &(size.width), &(size.height));
    if (buf == NULL) {
        return -1;
    }

    buf_to_buf(buf, size.width, size.height, size.width, win_buf + 640 * 309 + 484, 640);

    v9 = 150;
    ptr = buf;
    for (y = 0; y < size.height; y++) {
        for (x = 0; x < size.width; x++) {
            if (HighRGB(*ptr) < 2 && v9 >= x) {
                v9 = x;
            }
            ptr++;
        }
    }

    v9 -= 8;
    if (v9 < 0) {
        v9 = 0;
    }

    text_font(102);

    text_to_buf(win_buf + 640 * 272 + 348, name, 640, 640, colorTable[0]);
    int nameFontLineHeight = text_height();
    if (attributes != NULL) {
        int nameWidth = text_width(name);

        text_font(101);
        int attributesFontLineHeight = text_height();
        text_to_buf(win_buf + 640 * (268 + nameFontLineHeight - attributesFontLineHeight) + 348 + nameWidth + 8, attributes, 640, 640, colorTable[0]);
    }

    y = nameFontLineHeight;
    win_line(edit_win, 348, y + 272, 613, y + 272, colorTable[0]);
    win_line(edit_win, 348, y + 273, 613, y + 273, colorTable[0]);

    text_font(101);

    int descriptionFontLineHeight = text_height();

    if (word_wrap(description, v9 + 136, beginnings, &beginningsCount) != 0) {
        // TODO: Leaking graphic handle.
        return -1;
    }

    y = 315;
    for (short i = 0; i < beginningsCount - 1; i++) {
        short beginning = beginnings[i];
        short ending = beginnings[i + 1];
        char c = description[ending];
        description[ending] = '\0';
        text_to_buf(win_buf + 640 * y + 348, description + beginning, 640, 640, colorTable[0]);
        description[ending] = c;
        y += descriptionFontLineHeight;
    }

    if (graphicId != old_fid1 || strcmp(name, old_str1) != 0) {
        if (frstc_draw1) {
            gsound_play_sfx_file("isdxxxx1");
        }
    }

    strcpy(old_str1, name);
    old_fid1 = graphicId;
    frstc_draw1 = true;

    art_ptr_unlock(graphicHandle);

    return 0;
}

// 0x43AE84
static void FldrButton()
{
    mouse_get_position(&mouse_xpos, &mouse_ypos);
    gsound_play_sfx_file("ib3p1xx1");

    if (mouse_xpos >= 208) {
        info_line = 41;
        folder = EDITOR_FOLDER_KILLS;
    } else if (mouse_xpos > 110) {
        info_line = 42;
        folder = EDITOR_FOLDER_KARMA;
    } else {
        info_line = 40;
        folder = EDITOR_FOLDER_PERKS;
    }

    DrawFolder();
    DrawInfoWin();
}

// 0x43AF40
static void InfoButton(int eventCode)
{
    mouse_get_position(&mouse_xpos, &mouse_ypos);

    switch (eventCode) {
    case 525:
        if (1) {
            // TODO: Original code is slightly different.
            double mouseY = mouse_ypos;
            for (int index = 0; index < 7; index++) {
                double buttonTop = StatYpos[index];
                double buttonBottom = StatYpos[index] + 22;
                double allowance = 5.0 - index * 0.25;
                if (mouseY >= buttonTop - allowance && mouseY <= buttonBottom + allowance) {
                    info_line = index;
                    break;
                }
            }
        }
        break;
    case 526:
        if (glblmode) {
            info_line = 7;
        } else {
            int offset = mouse_ypos - 280;
            if (offset < 0) {
                offset = 0;
            }

            info_line = offset / 10 + 7;
        }
        break;
    case 527:
        if (!glblmode) {
            text_font(101);
            int offset = mouse_ypos - 364;
            if (offset < 0) {
                offset = 0;
            }
            info_line = offset / (text_height() + 1) + 10;
        }
        break;
    case 528:
        if (1) {
            int offset = mouse_ypos - 41;
            if (offset < 0) {
                offset = 0;
            }

            info_line = offset / 13 + 43;
        }
        break;
    case 529: {
        int offset = mouse_ypos - 175;
        if (offset < 0) {
            offset = 0;
        }

        info_line = offset / 13 + 51;
        break;
    }
    case 530:
        info_line = 80;
        break;
    case 531:
        if (1) {
            int offset = mouse_ypos - 27;
            if (offset < 0) {
                offset = 0;
            }

            skill_cursor = (int)(offset * 0.092307694);
            if (skill_cursor >= 18) {
                skill_cursor = 17;
            }

            info_line = skill_cursor + 61;
        }
        break;
    case 532:
        info_line = 79;
        break;
    case 533:
        info_line = 81;
        break;
    case 534:
        if (1) {
            text_font(101);

            // TODO: Original code is slightly different.
            double mouseY = mouse_ypos;
            double fontLineHeight = text_height();
            double y = 353.0;
            double step = text_height() + 3 + 0.56;
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

            info_line = index + 82;
            if (mouse_xpos >= 169) {
                info_line += 8;
            }
        }
        break;
    }

    PrintBasicStat(RENDER_ALL_STATS, 0, 0);
    ListTraits();
    ListSkills(0);
    PrintLevelWin();
    DrawFolder();
    ListDrvdStats();
    DrawInfoWin();
}

// 0x43B230
static void SliderBtn(int keyCode)
{
    if (glblmode) {
        return;
    }

    int unspentSp = stat_pc_get(PC_STAT_UNSPENT_SKILL_POINTS);
    _repFtime = 4;

    bool isUsingKeyboard = false;
    int rc = 0;

    switch (keyCode) {
    case KEY_PLUS:
    case KEY_UPPERCASE_N:
    case KEY_ARROW_RIGHT:
        isUsingKeyboard = true;
        keyCode = 521;
        break;
    case KEY_MINUS:
    case KEY_UPPERCASE_J:
    case KEY_ARROW_LEFT:
        isUsingKeyboard = true;
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

    int repeatDelay = 0;
    for (;;) {
        _frame_time = get_time();
        if (repeatDelay <= 19.2) {
            repeatDelay++;
        }

        if (repeatDelay == 1 || repeatDelay > 19.2) {
            if (repeatDelay > 19.2) {
                _repFtime++;
                if (_repFtime > 24) {
                    _repFtime = 24;
                }
            }

            rc = 1;
            if (keyCode == 521) {
                if (stat_pc_get(PC_STAT_UNSPENT_SKILL_POINTS) > 0) {
                    if (skill_inc_point(obj_dude, skill_cursor) == -3) {
                        gsound_play_sfx_file("iisxxxx1");

                        sprintf(title, "%s:", skill_name(skill_cursor));
                        // At maximum level.
                        strcpy(body1, getmsg(&editor_message_file, &mesg, 132));
                        // Unable to increment it.
                        strcpy(body2, getmsg(&editor_message_file, &mesg, 133));
                        dialog_out(title, body, 2, 192, 126, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_LARGE);
                        rc = -1;
                    }
                } else {
                    gsound_play_sfx_file("iisxxxx1");

                    // Not enough skill points available.
                    strcpy(title, getmsg(&editor_message_file, &mesg, 136));
                    dialog_out(title, NULL, 0, 192, 126, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_LARGE);
                    rc = -1;
                }
            } else if (keyCode == 523) {
                if (skill_level(obj_dude, skill_cursor) <= skillsav[skill_cursor]) {
                    rc = 0;
                } else {
                    if (skill_dec_point(obj_dude, skill_cursor) == -2) {
                        rc = 0;
                    }
                }

                if (rc == 0) {
                    gsound_play_sfx_file("iisxxxx1");

                    sprintf(title, "%s:", skill_name(skill_cursor));
                    // At minimum level.
                    strcpy(body1, getmsg(&editor_message_file, &mesg, 134));
                    // Unable to decrement it.
                    strcpy(body2, getmsg(&editor_message_file, &mesg, 135));
                    dialog_out(title, body, 2, 192, 126, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_LARGE);
                    rc = -1;
                }
            }

            info_line = skill_cursor + 61;
            DrawInfoWin();
            ListSkills(1);

            int flags;
            if (rc == 1) {
                flags = ANIMATE;
            } else {
                flags = 0;
            }

            PrintBigNum(522, 228, flags, stat_pc_get(PC_STAT_UNSPENT_SKILL_POINTS), unspentSp, edit_win);

            win_draw(edit_win);
        }

        if (!isUsingKeyboard) {
            unspentSp = stat_pc_get(PC_STAT_UNSPENT_SKILL_POINTS);
            if (repeatDelay >= 19.2) {
                while (elapsed_time(_frame_time) < 1000 / _repFtime) {
                }
            } else {
                while (elapsed_time(_frame_time) < 1000 / 24) {
                }
            }

            int keyCode = get_input();
            if (keyCode != 522 && keyCode != 524 && rc != -1) {
                continue;
            }
        }
        return;
    }
}

// 0x43B64C
static int tagskl_free()
{
    int taggedSkillCount;
    int index;

    taggedSkillCount = 0;
    for (index = 3; index >= 0; index--) {
        if (temp_tag_skill[index] != -1) {
            break;
        }

        taggedSkillCount++;
    }

    if (glblmode == 1) {
        taggedSkillCount--;
    }

    return taggedSkillCount;
}

// 0x43B67C
static void TagSkillSelect(int skill)
{
    int insertionIndex;

    // NOTE: Uninline.
    old_tags = tagskl_free();

    if (skill == temp_tag_skill[0] || skill == temp_tag_skill[1] || skill == temp_tag_skill[2] || skill == temp_tag_skill[3]) {
        if (skill == temp_tag_skill[0]) {
            temp_tag_skill[0] = temp_tag_skill[1];
            temp_tag_skill[1] = temp_tag_skill[2];
            temp_tag_skill[2] = -1;
        } else if (skill == temp_tag_skill[1]) {
            temp_tag_skill[1] = temp_tag_skill[2];
            temp_tag_skill[2] = -1;
        } else {
            temp_tag_skill[2] = -1;
        }
    } else {
        if (tagskill_count > 0) {
            insertionIndex = 0;
            for (int index = 0; index < 3; index++) {
                if (temp_tag_skill[index] == -1) {
                    break;
                }
                insertionIndex++;
            }
            temp_tag_skill[insertionIndex] = skill;
        } else {
            gsound_play_sfx_file("iisxxxx1");

            char line1[128];
            strcpy(line1, getmsg(&editor_message_file, &mesg, 140));

            char line2[128];
            strcpy(line2, getmsg(&editor_message_file, &mesg, 141));

            const char* lines[] = { line2 };
            dialog_out(line1, lines, 1, 192, 126, colorTable[32328], 0, colorTable[32328], 0);
        }
    }

    // NOTE: Uninline.
    tagskill_count = tagskl_free();

    info_line = skill + 61;
    PrintBasicStat(RENDER_ALL_STATS, 0, 0);
    ListDrvdStats();
    ListSkills(2);
    DrawInfoWin();
    win_draw(edit_win);
}

// 0x43B8A8
static void ListTraits()
{
    int v0 = -1;
    int i;
    int color;
    const char* traitName;
    double step;
    double y;

    if (glblmode != 1) {
        return;
    }

    if (info_line >= 82 && info_line < 98) {
        v0 = info_line - 82;
    }

    buf_to_buf(bckgnd + 640 * 353 + 47, 245, 100, 640, win_buf + 640 * 353 + 47, 640);

    text_font(101);

    trait_set(temp_trait[0], temp_trait[1]);

    step = text_height() + 3 + 0.56;
    y = 353;
    for (i = 0; i < 8; i++) {
        if (i == v0) {
            if (i != temp_trait[0] && i != temp_trait[1]) {
                color = colorTable[32747];
            } else {
                color = colorTable[32767];
            }

            folder_card_fid = trait_pic(i);
            folder_card_title = trait_name(i);
            folder_card_title2 = NULL;
            folder_card_desc = trait_description(i);
        } else {
            if (i != temp_trait[0] && i != temp_trait[1]) {
                color = colorTable[992];
            } else {
                color = colorTable[21140];
            }
        }

        traitName = trait_name(i);
        text_to_buf(win_buf + 640 * (int)y + 47, traitName, 640, 640, color);
        y += step;
    }

    y = 353;
    for (i = 8; i < 16; i++) {
        if (i == v0) {
            if (i != temp_trait[0] && i != temp_trait[1]) {
                color = colorTable[32747];
            } else {
                color = colorTable[32767];
            }

            folder_card_fid = trait_pic(i);
            folder_card_title = trait_name(i);
            folder_card_title2 = NULL;
            folder_card_desc = trait_description(i);
        } else {
            if (i != temp_trait[0] && i != temp_trait[1]) {
                color = colorTable[992];
            } else {
                color = colorTable[21140];
            }
        }

        traitName = trait_name(i);
        text_to_buf(win_buf + 640 * (int)y + 199, traitName, 640, 640, color);
        y += step;
    }
}

// NOTE: Inlined.
//
// 0x43BAE8
static int get_trait_count()
{
    int traitCount;
    int index;

    traitCount = 0;
    for (index = 1; index >= 0; index--) {
        if (temp_trait[index] != -1) {
            break;
        }

        traitCount++;
    }

    return traitCount;
}

// 0x43BB0C
static void TraitSelect(int trait)
{
    if (trait == temp_trait[0] || trait == temp_trait[1]) {
        if (trait == temp_trait[0]) {
            temp_trait[0] = temp_trait[1];
            temp_trait[1] = -1;
        } else {
            temp_trait[1] = -1;
        }
    } else {
        if (trait_count == 0) {
            gsound_play_sfx_file("iisxxxx1");

            char line1[128];
            strcpy(line1, getmsg(&editor_message_file, &mesg, 148));

            char line2[128];
            strcpy(line2, getmsg(&editor_message_file, &mesg, 149));

            const char* lines = { line2 };
            dialog_out(line1, &lines, 1, 192, 126, colorTable[32328], 0, colorTable[32328], 0);
        } else {
            for (int index = 0; index < 2; index++) {
                if (temp_trait[index] == -1) {
                    temp_trait[index] = trait;
                    break;
                }
            }
        }
    }

    // NOTE: Uninline.
    trait_count = get_trait_count();

    info_line = trait + EDITOR_FIRST_TRAIT;

    ListTraits();
    ListSkills(0);
    stat_recalc_derived(obj_dude);
    PrintBigNum(126, 282, 0, character_points, 0, edit_win);
    PrintBasicStat(RENDER_ALL_STATS, false, 0);
    ListDrvdStats();
    DrawInfoWin();
    win_draw(edit_win);
}

// 0x43BCE0
static void list_karma()
{
    char* msg;
    char formattedText[256];

    folder_clear();

    bool hasSelection = false;
    for (int index = 0; index < karma_vars_count; index++) {
        KarmaEntry* karmaDescription = &(karma_vars[index]);
        if (karmaDescription->gvar == GVAR_PLAYER_REPUTATION) {
            int reputation;
            for (reputation = 0; reputation < general_reps_count; reputation++) {
                GenericReputationEntry* reputationDescription = &(general_reps[reputation]);
                if (game_global_vars[GVAR_PLAYER_REPUTATION] >= reputationDescription->threshold) {
                    break;
                }
            }

            if (reputation != general_reps_count) {
                GenericReputationEntry* reputationDescription = &(general_reps[reputation]);

                char reputationValue[32];
                itoa(game_global_vars[GVAR_PLAYER_REPUTATION], reputationValue, 10);

                sprintf(formattedText,
                    "%s: %s (%s)",
                    getmsg(&editor_message_file, &mesg, 125),
                    reputationValue,
                    getmsg(&editor_message_file, &mesg, reputationDescription->name));

                if (folder_print_line(formattedText)) {
                    folder_card_fid = karmaDescription->art_num;
                    folder_card_title = getmsg(&editor_message_file, &mesg, 125);
                    folder_card_title2 = NULL;
                    folder_card_desc = getmsg(&editor_message_file, &mesg, karmaDescription->description);
                    hasSelection = true;
                }
            }
        } else {
            if (game_global_vars[karmaDescription->gvar] != 0) {
                msg = getmsg(&editor_message_file, &mesg, karmaDescription->name);
                if (folder_print_line(msg)) {
                    folder_card_fid = karmaDescription->art_num;
                    folder_card_title = getmsg(&editor_message_file, &mesg, karmaDescription->name);
                    folder_card_title2 = NULL;
                    folder_card_desc = getmsg(&editor_message_file, &mesg, karmaDescription->description);
                    hasSelection = true;
                }
            }
        }
    }

    bool hasTownReputationHeading = false;
    for (int index = 0; index < TOWN_REPUTATION_COUNT; index++) {
        const TownReputationEntry* pair = &(town_rep_info[index]);
        if (wmAreaIsKnown(pair->city)) {
            if (!hasTownReputationHeading) {
                msg = getmsg(&editor_message_file, &mesg, 4000);
                if (folder_print_seperator(msg)) {
                    folder_card_fid = 48;
                    folder_card_title = getmsg(&editor_message_file, &mesg, 4000);
                    folder_card_title2 = NULL;
                    folder_card_desc = getmsg(&editor_message_file, &mesg, 4100);
                }
                hasTownReputationHeading = true;
            }

            char cityShortName[40];
            wmGetAreaIdxName(pair->city, cityShortName);

            int townReputation = game_global_vars[pair->gvar];

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

            msg = getmsg(&editor_message_file, &mesg, townReputationBaseMessageId);
            sprintf(formattedText,
                "%s: %s",
                cityShortName,
                msg);

            if (folder_print_line(formattedText)) {
                folder_card_fid = townReputationGraphicId;
                folder_card_title = getmsg(&editor_message_file, &mesg, townReputationBaseMessageId);
                folder_card_title2 = NULL;
                folder_card_desc = getmsg(&editor_message_file, &mesg, townReputationBaseMessageId + 100);
                hasSelection = 1;
            }
        }
    }

    bool hasAddictionsHeading = false;
    for (int index = 0; index < ADDICTION_REPUTATION_COUNT; index++) {
        if (game_global_vars[addiction_vars[index]] != 0) {
            if (!hasAddictionsHeading) {
                // Addictions
                msg = getmsg(&editor_message_file, &mesg, 4001);
                if (folder_print_seperator(msg)) {
                    folder_card_fid = 53;
                    folder_card_title = getmsg(&editor_message_file, &mesg, 4001);
                    folder_card_title2 = NULL;
                    folder_card_desc = getmsg(&editor_message_file, &mesg, 4101);
                    hasSelection = 1;
                }
                hasAddictionsHeading = true;
            }

            msg = getmsg(&editor_message_file, &mesg, 1004 + index);
            if (folder_print_line(msg)) {
                folder_card_fid = addiction_pics[index];
                folder_card_title = getmsg(&editor_message_file, &mesg, 1004 + index);
                folder_card_title2 = NULL;
                folder_card_desc = getmsg(&editor_message_file, &mesg, 1104 + index);
                hasSelection = 1;
            }
        }
    }

    if (!hasSelection) {
        folder_card_fid = 47;
        folder_card_title = getmsg(&editor_message_file, &mesg, 125);
        folder_card_title2 = NULL;
        folder_card_desc = getmsg(&editor_message_file, &mesg, 128);
    }
}

// 0x43C1B0
int editor_save(File* stream)
{
    if (db_fwriteInt(stream, last_level) == -1)
        return -1;
    if (db_fwriteByte(stream, free_perk) == -1)
        return -1;

    return 0;
}

// 0x43C1E0
int editor_load(File* stream)
{
    if (db_freadInt(stream, &last_level) == -1)
        return -1;
    if (db_freadByte(stream, &free_perk) == -1)
        return -1;

    return 0;
}

// 0x43C20C
void editor_reset()
{
    character_points = 5;
    last_level = 1;
}

// level up if needed
//
// 0x43C228
static int UpdateLevel()
{
    int level = stat_pc_get(PC_STAT_LEVEL);
    if (level != last_level && level <= PC_LEVEL_MAX) {
        for (int nextLevel = last_level + 1; nextLevel <= level; nextLevel++) {
            int sp = stat_pc_get(PC_STAT_UNSPENT_SKILL_POINTS);
            sp += 5;
            sp += stat_get_base(obj_dude, STAT_INTELLIGENCE) * 2;
            sp += perk_level(obj_dude, PERK_EDUCATED) * 2;
            sp += trait_level(TRAIT_SKILLED) * 5;
            if (trait_level(TRAIT_GIFTED)) {
                sp -= 5;
                if (sp < 0) {
                    sp = 0;
                }
            }
            if (sp > 99) {
                sp = 99;
            }

            stat_pc_set(PC_STAT_UNSPENT_SKILL_POINTS, sp);

            // NOTE: Uninline.
            int selectedPerksCount = PerkCount();

            if (selectedPerksCount < 37) {
                int progression = 3;
                if (trait_level(TRAIT_SKILLED)) {
                    progression += 1;
                }

                if (nextLevel % progression == 0) {
                    free_perk = 1;
                }
            }
        }
    }

    if (free_perk != 0) {
        folder = 0;
        DrawFolder();
        win_draw(edit_win);

        int rc = perks_dialog();
        if (rc == -1) {
            debug_printf("\n *** Error running perks dialog! ***\n");
            return -1;
        } else if (rc == 0) {
            DrawFolder();
        } else if (rc == 1) {
            DrawFolder();
            free_perk = 0;
        }
    }

    last_level = level;

    return 1;
}

// 0x43C398
static void RedrwDPrks()
{
    buf_to_buf(
        pbckgnd + 280,
        293,
        PERK_WINDOW_HEIGHT,
        PERK_WINDOW_WIDTH,
        pwin_buf + 280,
        PERK_WINDOW_WIDTH);

    ListDPerks();

    // NOTE: Original code is slightly different, but basically does the same thing.
    int perk = name_sort_list[crow + cline].value;
    int perkFrmId = perk_skilldex_fid(perk);
    char* perkName = perk_name(perk);
    char* perkDescription = perk_description(perk);
    char* perkRank = NULL;
    char perkRankBuffer[32];

    int rank = perk_level(obj_dude, perk);
    if (rank != 0) {
        sprintf(perkRankBuffer, "(%d)", rank);
        perkRank = perkRankBuffer;
    }

    DrawCard2(perkFrmId, perkName, perkRank, perkDescription);

    win_draw(pwin);
}

// 0x43C4F0
static int perks_dialog()
{
    crow = 0;
    cline = 0;
    old_fid2 = -1;
    old_str2[0] = '\0';
    frstc_draw2 = false;

    CacheEntry* backgroundFrmHandle;
    int backgroundWidth;
    int backgroundHeight;
    int fid = art_id(OBJ_TYPE_INTERFACE, 86, 0, 0, 0);
    pbckgnd = art_lock(fid, &backgroundFrmHandle, &backgroundWidth, &backgroundHeight);
    if (pbckgnd == NULL) {
        debug_printf("\n *** Error running perks dialog window ***\n");
        return -1;
    }

    int perkWindowX = PERK_WINDOW_X;
    int perkWindowY = PERK_WINDOW_Y;
    pwin = win_add(perkWindowX, perkWindowY, PERK_WINDOW_WIDTH, PERK_WINDOW_HEIGHT, 256, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x02);
    if (pwin == -1) {
        art_ptr_unlock(backgroundFrmHandle);
        debug_printf("\n *** Error running perks dialog window ***\n");
        return -1;
    }

    pwin_buf = win_get_buf(pwin);
    memcpy(pwin_buf, pbckgnd, PERK_WINDOW_WIDTH * PERK_WINDOW_HEIGHT);

    int btn;

    btn = win_register_button(pwin,
        48,
        186,
        GInfo[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        GInfo[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        500,
        grphbmp[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        grphbmp[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        win_register_button_sound_func(btn, gsound_red_butt_press, gsound_red_butt_release);
    }

    btn = win_register_button(pwin,
        153,
        186,
        GInfo[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].width,
        GInfo[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP].height,
        -1,
        -1,
        -1,
        502,
        grphbmp[EDITOR_GRAPHIC_LITTLE_RED_BUTTON_UP],
        grphbmp[EDITOR_GRAPHIC_LILTTLE_RED_BUTTON_DOWN],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        win_register_button_sound_func(btn, gsound_red_butt_press, gsound_red_butt_release);
    }

    btn = win_register_button(pwin,
        25,
        46,
        GInfo[EDITOR_GRAPHIC_UP_ARROW_ON].width,
        GInfo[EDITOR_GRAPHIC_UP_ARROW_ON].height,
        -1,
        574,
        572,
        574,
        grphbmp[EDITOR_GRAPHIC_UP_ARROW_OFF],
        grphbmp[EDITOR_GRAPHIC_UP_ARROW_ON],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        win_register_button_sound_func(btn, gsound_red_butt_press, NULL);
    }

    btn = win_register_button(pwin,
        25,
        47 + GInfo[EDITOR_GRAPHIC_UP_ARROW_ON].height,
        GInfo[EDITOR_GRAPHIC_UP_ARROW_ON].width,
        GInfo[EDITOR_GRAPHIC_UP_ARROW_ON].height,
        -1,
        575,
        573,
        575,
        grphbmp[EDITOR_GRAPHIC_DOWN_ARROW_OFF],
        grphbmp[EDITOR_GRAPHIC_DOWN_ARROW_ON],
        NULL,
        BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        win_register_button_sound_func(btn, gsound_red_butt_press, NULL);
    }

    win_register_button(pwin,
        PERK_WINDOW_LIST_X,
        PERK_WINDOW_LIST_Y,
        PERK_WINDOW_LIST_WIDTH,
        PERK_WINDOW_LIST_HEIGHT,
        -1,
        -1,
        -1,
        501,
        NULL,
        NULL,
        NULL,
        BUTTON_FLAG_TRANSPARENT);

    text_font(103);

    const char* msg;

    // PICK A NEW PERK
    msg = getmsg(&editor_message_file, &mesg, 152);
    text_to_buf(pwin_buf + PERK_WINDOW_WIDTH * 16 + 49, msg, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[18979]);

    // DONE
    msg = getmsg(&editor_message_file, &mesg, 100);
    text_to_buf(pwin_buf + PERK_WINDOW_WIDTH * 186 + 69, msg, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[18979]);

    // CANCEL
    msg = getmsg(&editor_message_file, &mesg, 102);
    text_to_buf(pwin_buf + PERK_WINDOW_WIDTH * 186 + 171, msg, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[18979]);

    int count = ListDPerks();

    // NOTE: Original code is slightly different, but does the same thing.
    int perk = name_sort_list[crow + cline].value;
    int perkFrmId = perk_skilldex_fid(perk);
    char* perkName = perk_name(perk);
    char* perkDescription = perk_description(perk);
    char* perkRank = NULL;
    char perkRankBuffer[32];

    int rank = perk_level(obj_dude, perk);
    if (rank != 0) {
        sprintf(perkRankBuffer, "(%d)", rank);
        perkRank = perkRankBuffer;
    }

    DrawCard2(perkFrmId, perkName, perkRank, perkDescription);
    win_draw(pwin);

    int rc = InputPDLoop(count, RedrwDPrks);

    if (rc == 1) {
        if (perk_add(obj_dude, name_sort_list[crow + cline].value) == -1) {
            debug_printf("\n*** Unable to add perk! ***\n");
            rc = 2;
        }
    }

    rc &= 1;

    if (rc != 0) {
        if (perk_level(obj_dude, PERK_TAG) != 0 && perk_back[PERK_TAG] == 0) {
            if (!Add4thTagSkill()) {
                perk_sub(obj_dude, PERK_TAG);
            }
        } else if (perk_level(obj_dude, PERK_MUTATE) != 0 && perk_back[PERK_MUTATE] == 0) {
            if (!GetMutateTrait()) {
                perk_sub(obj_dude, PERK_MUTATE);
            }
        } else if (perk_level(obj_dude, PERK_LIFEGIVER) != perk_back[PERK_LIFEGIVER]) {
            int maxHp = stat_get_bonus(obj_dude, STAT_MAXIMUM_HIT_POINTS);
            stat_set_bonus(obj_dude, STAT_MAXIMUM_HIT_POINTS, maxHp + 4);
            critter_adjust_hits(obj_dude, 4);
        } else if (perk_level(obj_dude, PERK_EDUCATED) != perk_back[PERK_EDUCATED]) {
            int sp = stat_pc_get(PC_STAT_UNSPENT_SKILL_POINTS);
            stat_pc_set(PC_STAT_UNSPENT_SKILL_POINTS, sp + 2);
        }
    }

    ListSkills(0);
    PrintBasicStat(RENDER_ALL_STATS, 0, 0);
    PrintLevelWin();
    ListDrvdStats();
    DrawFolder();
    DrawInfoWin();
    win_draw(edit_win);

    art_ptr_unlock(backgroundFrmHandle);

    win_delete(pwin);

    return rc;
}

// 0x43CACC
static int InputPDLoop(int count, void (*refreshProc)())
{
    text_font(101);

    int v3 = count - 11;

    int height = text_height();
    oldsline = -2;
    int v16 = height + 2;

    int v7 = 0;

    int rc = 0;
    while (rc == 0) {
        int keyCode = get_input();
        int v19 = 0;

        if (keyCode == 500) {
            rc = 1;
        } else if (keyCode == KEY_RETURN) {
            gsound_play_sfx_file("ib1p1xx1");
            rc = 1;
        } else if (keyCode == 501) {
            mouse_get_position(&mouse_xpos, &mouse_ypos);
            cline = (mouse_ypos - (PERK_WINDOW_Y + PERK_WINDOW_LIST_Y)) / v16;
            if (cline >= 0) {
                if (count - 1 < cline)
                    cline = count - 1;
            } else {
                cline = 0;
            }

            if (cline == oldsline) {
                gsound_play_sfx_file("ib1p1xx1");
                rc = 1;
            }
            oldsline = cline;
            refreshProc();
        } else if (keyCode == 502 || keyCode == KEY_ESCAPE || game_user_wants_to_quit != 0) {
            rc = 2;
        } else {
            switch (keyCode) {
            case KEY_ARROW_UP:
                oldsline = -2;

                crow--;
                if (crow < 0) {
                    crow = 0;

                    cline--;
                    if (cline < 0) {
                        cline = 0;
                    }
                }

                refreshProc();
                break;
            case KEY_PAGE_UP:
                oldsline = -2;

                for (int index = 0; index < 11; index++) {
                    crow--;
                    if (crow < 0) {
                        crow = 0;

                        cline--;
                        if (cline < 0) {
                            cline = 0;
                        }
                    }
                }

                refreshProc();
                break;
            case KEY_ARROW_DOWN:
                oldsline = -2;

                if (count > 11) {
                    crow++;
                    if (crow > count - 11) {
                        crow = count - 11;

                        cline++;
                        if (cline > 10) {
                            cline = 10;
                        }
                    }
                } else {
                    cline++;
                    if (cline > count - 1) {
                        cline = count - 1;
                    }
                }

                refreshProc();
                break;
            case KEY_PAGE_DOWN:
                oldsline = -2;

                for (int index = 0; index < 11; index++) {
                    if (count > 11) {
                        crow++;
                        if (crow > count - 11) {
                            crow = count - 11;

                            cline++;
                            if (cline > 10) {
                                cline = 10;
                            }
                        }
                    } else {
                        cline++;
                        if (cline > count - 1) {
                            cline = count - 1;
                        }
                    }
                }

                refreshProc();
                break;
            case 572:
                _repFtime = 4;
                oldsline = -2;

                do {
                    _frame_time = get_time();
                    if (v19 <= 14.4) {
                        v19++;
                    }

                    if (v19 == 1 || v19 > 14.4) {
                        if (v19 > 14.4) {
                            _repFtime++;
                            if (_repFtime > 24) {
                                _repFtime = 24;
                            }
                        }

                        crow--;
                        if (crow < 0) {
                            crow = 0;

                            cline--;
                            if (cline < 0) {
                                cline = 0;
                            }
                        }
                        refreshProc();
                    }

                    if (v19 < 14.4) {
                        while (elapsed_time(_frame_time) < 1000 / 24) {
                        }
                    } else {
                        while (elapsed_time(_frame_time) < 1000 / _repFtime) {
                        }
                    }
                } while (get_input() != 574);

                break;
            case 573:
                oldsline = -2;
                _repFtime = 4;

                if (count > 11) {
                    do {
                        _frame_time = get_time();
                        if (v19 <= 14.4) {
                            v19++;
                        }

                        if (v19 == 1 || v19 > 14.4) {
                            if (v19 > 14.4) {
                                _repFtime++;
                                if (_repFtime > 24) {
                                    _repFtime = 24;
                                }
                            }

                            crow++;
                            if (crow > count - 11) {
                                crow = count - 11;

                                cline++;
                                if (cline > 10) {
                                    cline = 10;
                                }
                            }

                            refreshProc();
                        }

                        if (v19 < 14.4) {
                            while (elapsed_time(_frame_time) < 1000 / 24) {
                            }
                        } else {
                            while (elapsed_time(_frame_time) < 1000 / _repFtime) {
                            }
                        }
                    } while (get_input() != 575);
                } else {
                    do {
                        _frame_time = get_time();
                        if (v19 <= 14.4) {
                            v19++;
                        }

                        if (v19 == 1 || v19 > 14.4) {
                            if (v19 > 14.4) {
                                _repFtime++;
                                if (_repFtime > 24) {
                                    _repFtime = 24;
                                }
                            }

                            cline++;
                            if (cline > count - 1) {
                                cline = count - 1;
                            }

                            refreshProc();
                        }

                        if (v19 < 14.4) {
                            while (elapsed_time(_frame_time) < 1000 / 24) {
                            }
                        } else {
                            while (elapsed_time(_frame_time) < 1000 / _repFtime) {
                            }
                        }
                    } while (get_input() != 575);
                }
                break;
            case KEY_HOME:
                crow = 0;
                cline = 0;
                oldsline = -2;
                refreshProc();
                break;
            case KEY_END:
                oldsline = -2;
                if (count > 11) {
                    crow = count - 11;
                    cline = 10;
                } else {
                    cline = count - 1;
                }
                refreshProc();
                break;
            default:
                if (elapsed_time(_frame_time) > 700) {
                    _frame_time = get_time();
                    oldsline = -2;
                }
                break;
            }
        }
    }

    return rc;
}

// 0x43D0BC
static int ListDPerks()
{
    buf_to_buf(
        pbckgnd + PERK_WINDOW_WIDTH * 43 + 45,
        192,
        129,
        PERK_WINDOW_WIDTH,
        pwin_buf + PERK_WINDOW_WIDTH * 43 + 45,
        PERK_WINDOW_WIDTH);

    text_font(101);

    int perks[PERK_COUNT];
    int count = perk_make_list(obj_dude, perks);
    if (count == 0) {
        return 0;
    }

    for (int perk = 0; perk < PERK_COUNT; perk++) {
        name_sort_list[perk].value = 0;
        name_sort_list[perk].name = NULL;
    }

    for (int index = 0; index < count; index++) {
        name_sort_list[index].value = perks[index];
        name_sort_list[index].name = perk_name(perks[index]);
    }

    qsort(name_sort_list, count, sizeof(*name_sort_list), name_sort_comp);

    int v16 = count - crow;
    if (v16 > 11) {
        v16 = 11;
    }

    v16 += crow;

    int y = 43;
    int yStep = text_height() + 2;
    for (int index = crow; index < v16; index++) {
        int color;
        if (index == crow + cline) {
            color = colorTable[32747];
        } else {
            color = colorTable[992];
        }

        text_to_buf(pwin_buf + PERK_WINDOW_WIDTH * y + 45, name_sort_list[index].name, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, color);

        if (perk_level(obj_dude, name_sort_list[index].value) != 0) {
            char rankString[256];
            sprintf(rankString, "(%d)", perk_level(obj_dude, name_sort_list[index].value));
            text_to_buf(pwin_buf + PERK_WINDOW_WIDTH * y + 207, rankString, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, color);
        }

        y += yStep;
    }

    return count;
}

// 0x43D2F8
void RedrwDMPrk()
{
    buf_to_buf(pbckgnd + 280, 293, PERK_WINDOW_HEIGHT, PERK_WINDOW_WIDTH, pwin_buf + 280, PERK_WINDOW_WIDTH);

    ListMyTraits(optrt_count);

    char* traitName = name_sort_list[crow + cline].name;
    char* tratDescription = trait_description(name_sort_list[crow + cline].value);
    int frmId = trait_pic(name_sort_list[crow + cline].value);
    DrawCard2(frmId, traitName, NULL, tratDescription);

    win_draw(pwin);
}

// 0x43D38C
static bool GetMutateTrait()
{
    old_fid2 = -1;
    old_str2[0] = '\0';
    frstc_draw2 = false;

    // NOTE: Uninline.
    trait_count = TRAITS_MAX_SELECTED_COUNT - get_trait_count();

    bool result = true;
    if (trait_count >= 1) {
        text_font(103);

        buf_to_buf(pbckgnd + PERK_WINDOW_WIDTH * 14 + 49, 206, text_height() + 2, PERK_WINDOW_WIDTH, pwin_buf + PERK_WINDOW_WIDTH * 15 + 49, PERK_WINDOW_WIDTH);

        // LOSE A TRAIT
        char* msg = getmsg(&editor_message_file, &mesg, 154);
        text_to_buf(pwin_buf + PERK_WINDOW_WIDTH * 16 + 49, msg, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[18979]);

        optrt_count = 0;
        cline = 0;
        crow = 0;
        RedrwDMPrk();

        int rc = InputPDLoop(trait_count, RedrwDMPrk);
        if (rc == 1) {
            if (cline == 0) {
                if (trait_count == 1) {
                    temp_trait[0] = -1;
                    temp_trait[1] = -1;
                } else {
                    if (name_sort_list[0].value == temp_trait[0]) {
                        temp_trait[0] = temp_trait[1];
                        temp_trait[1] = -1;
                    } else {
                        temp_trait[1] = -1;
                    }
                }
            } else {
                if (name_sort_list[0].value == temp_trait[0]) {
                    temp_trait[1] = -1;
                } else {
                    temp_trait[0] = temp_trait[1];
                    temp_trait[1] = -1;
                }
            }
        } else {
            result = false;
        }
    }

    if (result) {
        text_font(103);

        buf_to_buf(pbckgnd + PERK_WINDOW_WIDTH * 14 + 49, 206, text_height() + 2, PERK_WINDOW_WIDTH, pwin_buf + PERK_WINDOW_WIDTH * 15 + 49, PERK_WINDOW_WIDTH);

        // PICK A NEW TRAIT
        char* msg = getmsg(&editor_message_file, &mesg, 153);
        text_to_buf(pwin_buf + PERK_WINDOW_WIDTH * 16 + 49, msg, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[18979]);

        cline = 0;
        crow = 0;
        optrt_count = 1;

        RedrwDMPrk();

        int count = 16 - trait_count;
        if (count > 16) {
            count = 16;
        }

        int rc = InputPDLoop(count, RedrwDMPrk);
        if (rc == 1) {
            if (trait_count != 0) {
                temp_trait[1] = name_sort_list[cline + crow].value;
            } else {
                temp_trait[0] = name_sort_list[cline + crow].value;
                temp_trait[1] = -1;
            }

            trait_set(temp_trait[0], temp_trait[1]);
        } else {
            result = false;
        }
    }

    if (!result) {
        memcpy(temp_trait, trait_back, sizeof(temp_trait));
    }

    return result;
}

// 0x43D668
static void RedrwDMTagSkl()
{
    buf_to_buf(pbckgnd + 280, 293, PERK_WINDOW_HEIGHT, PERK_WINDOW_WIDTH, pwin_buf + 280, PERK_WINDOW_WIDTH);

    ListNewTagSkills();

    char* name = name_sort_list[crow + cline].name;
    char* description = skill_description(name_sort_list[crow + cline].value);
    int frmId = skill_pic(name_sort_list[crow + cline].value);
    DrawCard2(frmId, name, NULL, description);

    win_draw(pwin);
}

// 0x43D6F8
static bool Add4thTagSkill()
{
    text_font(103);

    buf_to_buf(pbckgnd + 573 * 14 + 49, 206, text_height() + 2, 573, pwin_buf + 573 * 15 + 49, 573);

    // PICK A NEW TAG SKILL
    char* messageListItemText = getmsg(&editor_message_file, &mesg, 155);
    text_to_buf(pwin_buf + 573 * 16 + 49, messageListItemText, 573, 573, colorTable[18979]);

    cline = 0;
    crow = 0;
    old_fid2 = -1;
    old_str2[0] = '\0';
    frstc_draw2 = false;
    RedrwDMTagSkl();

    int rc = InputPDLoop(optrt_count, RedrwDMTagSkl);
    if (rc != 1) {
        memcpy(temp_tag_skill, tag_skill_back, sizeof(temp_tag_skill));
        skill_set_tags(tag_skill_back, NUM_TAGGED_SKILLS);
        return false;
    }

    temp_tag_skill[3] = name_sort_list[crow + cline].value;
    skill_set_tags(temp_tag_skill, NUM_TAGGED_SKILLS);

    return true;
}

// 0x43D81C
static void ListNewTagSkills()
{
    buf_to_buf(pbckgnd + PERK_WINDOW_WIDTH * 43 + 45, 192, 129, PERK_WINDOW_WIDTH, pwin_buf + PERK_WINDOW_WIDTH * 43 + 45, PERK_WINDOW_WIDTH);

    text_font(101);

    optrt_count = 0;

    int y = 43;
    int yStep = text_height() + 2;

    for (int skill = 0; skill < SKILL_COUNT; skill++) {
        if (skill != temp_tag_skill[0] && skill != temp_tag_skill[1] && skill != temp_tag_skill[2] && skill != temp_tag_skill[3]) {
            name_sort_list[optrt_count].value = skill;
            name_sort_list[optrt_count].name = skill_name(skill);
            optrt_count++;
        }
    }

    qsort(name_sort_list, optrt_count, sizeof(*name_sort_list), name_sort_comp);

    for (int index = crow; index < crow + 11; index++) {
        int color;
        if (index == cline + crow) {
            color = colorTable[32747];
        } else {
            color = colorTable[992];
        }

        text_to_buf(pwin_buf + PERK_WINDOW_WIDTH * y + 45, name_sort_list[index].name, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, color);
        y += yStep;
    }
}

// 0x43D960
static int ListMyTraits(int a1)
{
    buf_to_buf(pbckgnd + PERK_WINDOW_WIDTH * 43 + 45, 192, 129, PERK_WINDOW_WIDTH, pwin_buf + PERK_WINDOW_WIDTH * 43 + 45, PERK_WINDOW_WIDTH);

    text_font(101);

    int y = 43;
    int yStep = text_height() + 2;

    if (a1 != 0) {
        int count = 0;
        for (int trait = 0; trait < TRAIT_COUNT; trait++) {
            if (trait != trait_back[0] && trait != trait_back[1]) {
                name_sort_list[count].value = trait;
                name_sort_list[count].name = trait_name(trait);
                count++;
            }
        }

        qsort(name_sort_list, count, sizeof(*name_sort_list), name_sort_comp);

        for (int index = crow; index < crow + 11; index++) {
            int color;
            if (index == cline + crow) {
                color = colorTable[32747];
            } else {
                color = colorTable[992];
            }

            text_to_buf(pwin_buf + PERK_WINDOW_WIDTH * y + 45, name_sort_list[index].name, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, color);
            y += yStep;
        }
    } else {
        // NOTE: Original code does not use loop.
        for (int index = 0; index < TRAITS_MAX_SELECTED_COUNT; index++) {
            name_sort_list[index].value = temp_trait[index];
            name_sort_list[index].name = trait_name(temp_trait[index]);
        }

        if (trait_count > 1) {
            qsort(name_sort_list, trait_count, sizeof(*name_sort_list), name_sort_comp);
        }

        for (int index = 0; index < trait_count; index++) {
            int color;
            if (index == cline) {
                color = colorTable[32747];
            } else {
                color = colorTable[992];
            }

            text_to_buf(pwin_buf + PERK_WINDOW_WIDTH * y + 45, name_sort_list[index].name, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, color);
            y += yStep;
        }
    }
    return 0;
}

// 0x43DB48
static int name_sort_comp(const void* a1, const void* a2)
{
    PerkDialogOption* v1 = (PerkDialogOption*)a1;
    PerkDialogOption* v2 = (PerkDialogOption*)a2;
    return strcmp(v1->name, v2->name);
}

// 0x43DB54
static int DrawCard2(int frmId, const char* name, const char* rank, char* description)
{
    int fid = art_id(OBJ_TYPE_SKILLDEX, frmId, 0, 0, 0);

    CacheEntry* handle;
    int width;
    int height;
    unsigned char* data = art_lock(fid, &handle, &width, &height);
    if (data == NULL) {
        return -1;
    }

    buf_to_buf(data, width, height, width, pwin_buf + PERK_WINDOW_WIDTH * 64 + 413, PERK_WINDOW_WIDTH);

    // Calculate width of transparent pixels on the left side of the image. This
    // space will be occupied by description (in addition to fixed width).
    int extraDescriptionWidth = 150;
    for (int y = 0; y < height; y++) {
        unsigned char* stride = data;
        for (int x = 0; x < width; x++) {
            if (HighRGB(*stride) < 2) {
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

    text_font(102);
    int nameHeight = text_height();

    text_to_buf(pwin_buf + PERK_WINDOW_WIDTH * 27 + 280, name, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[0]);

    if (rank != NULL) {
        int rankX = text_width(name) + 280 + 8;
        text_font(101);

        int rankHeight = text_height();
        text_to_buf(pwin_buf + PERK_WINDOW_WIDTH * (23 + nameHeight - rankHeight) + rankX, rank, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[0]);
    }

    win_line(pwin, 280, 27 + nameHeight, 545, 27 + nameHeight, colorTable[0]);
    win_line(pwin, 280, 28 + nameHeight, 545, 28 + nameHeight, colorTable[0]);

    text_font(101);

    int yStep = text_height() + 1;
    int y = 70;

    short beginnings[WORD_WRAP_MAX_COUNT];
    short count;
    if (word_wrap(description, 133 + extraDescriptionWidth, beginnings, &count) != 0) {
        // FIXME: Leaks handle.
        return -1;
    }

    for (int index = 0; index < count - 1; index++) {
        char* beginning = description + beginnings[index];
        char* ending = description + beginnings[index + 1];

        char ch = *ending;
        *ending = '\0';

        text_to_buf(pwin_buf + PERK_WINDOW_WIDTH * y + 280, beginning, PERK_WINDOW_WIDTH, PERK_WINDOW_WIDTH, colorTable[0]);

        *ending = ch;

        y += yStep;
    }

    if (frmId != old_fid2 || strcmp(old_str2, name) != 0) {
        if (frstc_draw2) {
            gsound_play_sfx_file("isdxxxx1");
        }
    }

    strcpy(old_str2, name);
    old_fid2 = frmId;
    frstc_draw2 = true;

    art_ptr_unlock(handle);

    return 0;
}

// 0x43DE94
static void push_perks()
{
    int perk;

    for (perk = 0; perk < PERK_COUNT; perk++) {
        perk_back[perk] = perk_level(obj_dude, perk);
    }
}

// copy editor perks to character
//
// 0x43DEBC
static void pop_perks()
{
    for (int perk = 0; perk < PERK_COUNT; perk++) {
        for (;;) {
            int rank = perk_level(obj_dude, perk);
            if (rank <= perk_back[perk]) {
                break;
            }

            perk_sub(obj_dude, perk);
        }
    }

    for (int i = 0; i < PERK_COUNT; i++) {
        for (;;) {
            int rank = perk_level(obj_dude, i);
            if (rank >= perk_back[i]) {
                break;
            }

            perk_add(obj_dude, i);
        }
    }
}

// NOTE: Inlined.
//
// 0x43DF24
static int PerkCount()
{
    int perk;
    int perkCount;

    perkCount = 0;
    for (perk = 0; perk < PERK_COUNT; perk++) {
        if (perk_level(obj_dude, perk) > 0) {
            perkCount++;
            if (perkCount >= 37) {
                break;
            }
        }
    }

    return perkCount;
}

// validate SPECIAL stats are <= 10
//
// 0x43DF50
static int is_supper_bonus()
{
    for (int stat = 0; stat < 7; stat++) {
        int v1 = stat_get_base(obj_dude, stat);
        int v2 = stat_get_bonus(obj_dude, stat);
        if (v1 + v2 > 10) {
            return 1;
        }
    }

    return 0;
}

// 0x43DF8C
static int folder_init()
{
    folder_karma_top_line = 0;
    folder_perk_top_line = 0;
    folder_kills_top_line = 0;

    if (folder_up_button == -1) {
        folder_up_button = win_register_button(edit_win, 317, 364, GInfo[22].width, GInfo[22].height, -1, -1, -1, 17000, grphbmp[21], grphbmp[22], NULL, 32);
        if (folder_up_button == -1) {
            return -1;
        }

        win_register_button_sound_func(folder_up_button, gsound_red_butt_press, NULL);
    }

    if (folder_down_button == -1) {
        folder_down_button = win_register_button(edit_win,
            317,
            365 + GInfo[22].height,
            GInfo[4].width,
            GInfo[4].height,
            folder_down_button,
            folder_down_button,
            folder_down_button,
            17001,
            grphbmp[3],
            grphbmp[4],
            0,
            32);
        if (folder_down_button == -1) {
            win_delete_button(folder_up_button);
            return -1;
        }

        win_register_button_sound_func(folder_down_button, gsound_red_butt_press, NULL);
    }

    return 0;
}

// NOTE: Inlined.
//
// 0x43E090
static void folder_exit()
{
    if (folder_down_button != -1) {
        win_delete_button(folder_down_button);
        folder_down_button = -1;
    }

    if (folder_up_button != -1) {
        win_delete_button(folder_up_button);
        folder_up_button = -1;
    }
}

// 0x43E0D4
static void folder_scroll(int direction)
{
    int* v1;

    switch (folder) {
    case EDITOR_FOLDER_PERKS:
        v1 = &folder_perk_top_line;
        break;
    case EDITOR_FOLDER_KARMA:
        v1 = &folder_karma_top_line;
        break;
    case EDITOR_FOLDER_KILLS:
        v1 = &folder_kills_top_line;
        break;
    default:
        return;
    }

    if (direction >= 0) {
        if (folder_max_lines + folder_top_line <= folder_line) {
            folder_top_line++;
            if (info_line >= 10 && info_line < 43 && info_line != 10) {
                info_line--;
            }
        }
    } else {
        if (folder_top_line > 0) {
            folder_top_line--;
            if (info_line >= 10 && info_line < 43 && folder_max_lines + 9 > info_line) {
                info_line++;
            }
        }
    }

    *v1 = folder_top_line;
    DrawFolder();

    if (info_line >= 10 && info_line < 43) {
        buf_to_buf(
            bckgnd + 640 * 267 + 345,
            277,
            170,
            640,
            win_buf + 640 * 267 + 345,
            640);
        DrawCard(folder_card_fid, folder_card_title, folder_card_title2, folder_card_desc);
    }
}

// 0x43E200
static void folder_clear()
{
    int v0;

    folder_line = 0;
    folder_ypos = 364;

    v0 = text_height();

    folder_max_lines = 9;
    folder_yoffset = v0 + 1;

    if (info_line < 10 || info_line >= 43)
        folder_highlight_line = -1;
    else
        folder_highlight_line = info_line - 10;

    if (folder < 1) {
        if (folder)
            return;

        folder_top_line = folder_perk_top_line;
    } else if (folder == 1) {
        folder_top_line = folder_karma_top_line;
    } else if (folder == 2) {
        folder_top_line = folder_kills_top_line;
    }
}

// render heading string with line
//
// 0x43E28C
static int folder_print_seperator(const char* string)
{
    int lineHeight;
    int x;
    int y;
    int lineLen;
    int gap;
    int v8 = 0;

    if (folder_max_lines + folder_top_line > folder_line) {
        if (folder_line >= folder_top_line) {
            if (folder_line - folder_top_line == folder_highlight_line) {
                v8 = 1;
            }
            lineHeight = text_height();
            x = 280;
            y = folder_ypos + lineHeight / 2;
            if (string != NULL) {
                gap = text_spacing();
                // TODO: Not sure about this.
                lineLen = text_width(string) + gap * 4;
                x = (x - lineLen) / 2;
                text_to_buf(win_buf + 640 * folder_ypos + 34 + x + gap * 2, string, 640, 640, colorTable[992]);
                win_line(edit_win, 34 + x + lineLen, y, 34 + 280, y, colorTable[992]);
            }
            win_line(edit_win, 34, y, 34 + x, y, colorTable[992]);
            folder_ypos += folder_yoffset;
        }
        folder_line++;
        return v8;
    } else {
        return 0;
    }
}

// 0x43E3D8
static bool folder_print_line(const char* string)
{
    bool success = false;
    int color;

    if (folder_max_lines + folder_top_line > folder_line) {
        if (folder_line >= folder_top_line) {
            if (folder_line - folder_top_line == folder_highlight_line) {
                success = true;
                color = colorTable[32747];
            } else {
                color = colorTable[992];
            }

            text_to_buf(win_buf + 640 * folder_ypos + 34, string, 640, 640, color);
            folder_ypos += folder_yoffset;
        }

        folder_line++;
    }

    return success;
}

// 0x43E470
static bool folder_print_kill(const char* name, int kills)
{
    char killsString[8];
    int color;
    int gap;

    bool success = false;
    if (folder_max_lines + folder_top_line > folder_line) {
        if (folder_line >= folder_top_line) {
            if (folder_line - folder_top_line == folder_highlight_line) {
                color = colorTable[32747];
                success = true;
            } else {
                color = colorTable[992];
            }

            itoa(kills, killsString, 10);
            int v6 = text_width(killsString);

            // TODO: Check.
            gap = text_spacing();
            int v11 = folder_ypos + text_height() / 2;

            text_to_buf(win_buf + 640 * folder_ypos + 34, name, 640, 640, color);

            int v12 = text_width(name);
            win_line(edit_win, 34 + v12 + gap, v11, 314 - v6 - gap, v11, color);

            text_to_buf(win_buf + 640 * folder_ypos + 314 - v6, killsString, 640, 640, color);
            folder_ypos += folder_yoffset;
        }

        folder_line++;
    }

    return success;
}

// 0x43E5C4
static int karma_vars_init()
{
    const char* delim = " \t,";

    if (karma_vars != NULL) {
        mem_free(karma_vars);
        karma_vars = NULL;
    }

    karma_vars_count = 0;

    File* stream = db_fopen("data\\karmavar.txt", "rt");
    if (stream == NULL) {
        return -1;
    }

    char string[256];
    while (db_fgets(string, 256, stream)) {
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

        KarmaEntry* entries = (KarmaEntry*)mem_realloc(karma_vars, sizeof(*entries) * (karma_vars_count + 1));
        if (entries == NULL) {
            db_fclose(stream);

            return -1;
        }

        memcpy(&(entries[karma_vars_count]), &entry, sizeof(entry));

        karma_vars = entries;
        karma_vars_count++;
    }

    qsort(karma_vars, karma_vars_count, sizeof(*karma_vars), karma_vars_qsort_compare);

    db_fclose(stream);

    return 0;
}

// NOTE: Inlined.
//
// 0x43E764
static void karma_vars_exit()
{
    if (karma_vars != NULL) {
        mem_free(karma_vars);
        karma_vars = NULL;
    }

    karma_vars_count = 0;
}

// 0x43E78C
static int karma_vars_qsort_compare(const void* a1, const void* a2)
{
    KarmaEntry* v1 = (KarmaEntry*)a1;
    KarmaEntry* v2 = (KarmaEntry*)a2;
    return v1->gvar - v2->gvar;
}

// 0x43E798
static int general_reps_init()
{
    const char* delim = " \t,";

    if (general_reps != NULL) {
        mem_free(general_reps);
        general_reps = NULL;
    }

    general_reps_count = 0;

    File* stream = db_fopen("data\\genrep.txt", "rt");
    if (stream == NULL) {
        return -1;
    }

    char string[256];
    while (db_fgets(string, 256, stream)) {
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

        GenericReputationEntry* entries = (GenericReputationEntry*)mem_realloc(general_reps, sizeof(*entries) * (general_reps_count + 1));
        if (entries == NULL) {
            db_fclose(stream);

            return -1;
        }

        memcpy(&(entries[general_reps_count]), &entry, sizeof(entry));

        general_reps = entries;
        general_reps_count++;
    }

    qsort(general_reps, general_reps_count, sizeof(*general_reps), general_reps_qsort_compare);

    db_fclose(stream);

    return 0;
}

// NOTE: Inlined.
//
// 0x43E914
static void general_reps_exit()
{
    if (general_reps != NULL) {
        mem_free(general_reps);
        general_reps = NULL;
    }

    general_reps_count = 0;
}

// 0x43E93C
static int general_reps_qsort_compare(const void* a1, const void* a2)
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
