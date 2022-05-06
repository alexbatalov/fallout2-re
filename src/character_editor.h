#ifndef CHARACTER_EDITOR_H
#define CHARACTER_EDITOR_H

#include "art.h"
#include "db.h"
#include "geometry.h"
#include "message.h"
#include "perk_defs.h"
#include "proto_types.h"
#include "skill_defs.h"
#include "stat_defs.h"
#include "trait_defs.h"

#define DIALOG_PICKER_NUM_OPTIONS max(PERK_COUNT, TRAIT_COUNT)
#define TOWN_REPUTATION_COUNT 19
#define ADDICTION_REPUTATION_COUNT 8

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

typedef struct TownReputationEntry {
    int gvar;
    int city;
} TownReputationEntry;

typedef struct STRUCT_56FCB0 {
    int field_0;
    char* field_4;
} STRUCT_56FCB0;

// TODO: Field order is probably wrong.
typedef struct KillInfo {
    const char* name;
    int killTypeId;
    int kills;
} KillInfo;

extern int dword_431C40[50];
extern const unsigned char byte_431D08[EDITOR_GRAPHIC_COUNT];
extern const int word_431D3A[EDITOR_DERIVED_STAT_COUNT];
extern const int dword_431D50[7];
extern const int word_431D6C[EDITOR_DERIVED_STAT_COUNT];
extern char byte_431D93[64];
extern const int dword_431DD4[7];

extern const double dbl_50170B;
extern const double dbl_501713;
extern const double dbl_5018F0;
extern const double dbl_5019BE;

extern bool dword_518528;
extern int dword_51852C;
extern int dword_518534;
extern int characterEditorRemainingCharacterPoints;
extern KarmaEntry* gKarmaEntries;
extern int gKarmaEntriesLength;
extern GenericReputationEntry* gGenericReputationEntries;
extern int gGenericReputationEntriesLength;
extern const TownReputationEntry gTownReputationEntries[TOWN_REPUTATION_COUNT];
extern const int gAddictionReputationVars[ADDICTION_REPUTATION_COUNT];
extern const int gAddictionReputationFrmIds[ADDICTION_REPUTATION_COUNT];
extern int dword_518624;
extern int dword_518628;

extern char byte_56FB60[256];
extern int dword_56FC60[SKILL_COUNT];
extern MessageList editorMessageList;
extern STRUCT_56FCB0 stru_56FCB0[DIALOG_PICKER_NUM_OPTIONS];
extern int dword_5700A8[TRAIT_COUNT];
extern MessageListItem editorMessageListItem;
extern char byte_5700F8[48];
extern char byte_570128[48];
extern int dword_570158[SKILL_COUNT];
extern char byte_5701A0[32];
extern Size stru_5701C0[EDITOR_GRAPHIC_COUNT];
extern CacheEntry* dword_570350[EDITOR_GRAPHIC_COUNT];
extern unsigned char* dword_570418[EDITOR_GRAPHIC_COUNT];
extern unsigned char* dword_5704E0[EDITOR_GRAPHIC_COUNT];
extern int dword_5705A8;
extern int dword_5705AC;
extern int dword_5705B0;
extern int dword_5705B4;
extern char* off_5705B8;
extern char* off_5705BC;
extern int dword_5705C0;
extern int dword_5705C4;
extern int dword_5705C8;
extern char* off_5705CC;
extern int dword_5705D0;
extern int dword_5705D4;
extern int dword_5705D8;
extern unsigned char* gEditorPerkBackgroundBuffer;
extern int gEditorPerkWindow;
extern int dword_5705E4;
extern int dword_5705E8;
extern int dword_5705EC[7];
extern unsigned char* characterEditorWindowBuf;
extern int characterEditorWindowHandle;
extern int dword_570610[7];
extern unsigned char* gEditorPerkWindowBuffer;
extern CritterProtoData stru_570630;
extern unsigned char* characterEditorWindowBackgroundBuf;
extern int dword_5707A8;
extern int dword_5707AC;
extern int dword_5707B0;
extern int dword_5707B4;
extern int characterEditorWindowOldFont;
extern int dword_5707BC;
extern CacheEntry* off_5707C0;
extern int dword_5707C4;
extern int dword_5707C8;
extern int dword_5707CC;
extern int characterEditorSelectedItem;
extern int characterEditorWindowSelectedFolder;
extern int dword_5707D8;
extern int dword_5707DC;
extern int dword_5707E0;
extern int dword_5707E4[PERK_COUNT];
extern unsigned int dword_5709C0;
extern unsigned int dword_5709C4;
extern int dword_5709C8;
extern int dword_5709CC;
extern bool gCharacterEditorIsCreationMode;
extern int dword_5709D4[NUM_TAGGED_SKILLS];
extern int dword_5709E8;
extern int dword_5709EC;
extern int dword_5709F0[2];
extern int dword_5709FC;
extern int dword_570A00;
extern int dword_570A04[TRAITS_MAX_SELECTED_COUNT];
extern int dword_570A10;
extern int dword_570A14[NUM_TAGGED_SKILLS];
extern char byte_570A28;
extern unsigned char byte_570A29;
extern unsigned char byte_570A2A;

int editor_design(bool isCreationMode);
int characterEditorWindowInit();
void characterEditorWindowFree();
void CharEditInit();
int get_input_str(int win, int cancelKeyCode, char* text, int maxLength, int x, int y, int textColor, int backgroundColor, int flags);
bool isdoschar(int ch);
char* strmfe(char* dest, const char* name, const char* ext);
void editorRenderFolders();
void editorRenderPerks();
int kills_list_comp(const KillInfo* a, const KillInfo* b);
int editorRenderKills();
void characterEditorRenderBigNumber(int x, int y, int flags, int value, int previousValue, int windowHandle);
void editorRenderPcStats();
void editorRenderPrimaryStat(int stat, bool animate, int previousValue);
void editorRenderGender();
void editorRenderAge();
void editorRenderName();
void editorRenderSecondaryStats();
void editorRenderSkills(int a1);
void editorRenderDetails();
int characterEditorEditName();
void PrintName(unsigned char* buf, int a2);
int characterEditorRunEditAgeDialog();
void characterEditorEditGender();
void characterEditorHandleIncDecPrimaryStat(int eventCode);
int OptionWindow();
bool characterFileExists(const char* fname);
int characterPrintToFile(const char* fileName);
char* AddSpaces(char* string, int length);
char* AddDots(char* string, int length);
void ResetScreen();
void RegInfoAreas();
void SavePlayer();
void RestorePlayer();
char* itostndn(int value, char* dest);
int DrawCard(int graphicId, const char* name, const char* attributes, char* description);
void FldrButton();
void InfoButton(int eventCode);
void editorAdjustSkill(int a1);
void characterEditorToggleTaggedSkill(int skill);
void characterEditorWindowRenderTraits();
void characterEditorToggleOptionalTrait(int trait);
void editorRenderKarma();
int editor_save(File* stream);
int editor_load(File* stream);
void editor_reset();
int UpdateLevel();
void RedrwDPrks();
int editorSelectPerk();
int InputPDLoop(int count, void (*refreshProc)());
int ListDPerks();
void RedrwDMPrk();
bool editorHandleMutate();
void RedrwDMTagSkl();
bool editorHandleTag();
void ListNewTagSkills();
int ListMyTraits(int a1);
int name_sort_comp(const void* a1, const void* a2);
int DrawCard2(int frmId, const char* name, const char* rank, char* description);
void pop_perks();
int is_supper_bonus();
int folder_init();
void folder_scroll(int direction);
void folder_clear();
int folder_print_seperator(const char* string);
bool folder_print_line(const char* string);
bool editorDrawKillsEntry(const char* name, int kills);
int karmaInit();
void karmaFree();
int karmaEntryCompare(const void* a1, const void* a2);
int genericReputationInit();
void genericReputationFree();
int genericReputationCompare(const void* a1, const void* a2);

#endif /* CHARACTER_EDITOR_H */
