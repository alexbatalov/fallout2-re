#ifndef PIPBOY_H
#define PIPBOY_H

#include "art.h"
#include "db.h"
#include "geometry.h"
#include "message.h"

#include <stdbool.h>

#define PIPBOY_WINDOW_WIDTH (640)
#define PIPBOY_WINDOW_HEIGHT (480)

#define PIPBOY_WINDOW_DAY_X (20)
#define PIPBOY_WINDOW_DAY_Y (17)

#define PIPBOY_WINDOW_MONTH_X (46)
#define PIPBOY_WINDOW_MONTH_Y (18)

#define PIPBOY_WINDOW_YEAR_X (83)
#define PIPBOY_WINDOW_YEAR_Y (17)

#define PIPBOY_WINDOW_TIME_X (155)
#define PIPBOY_WINDOW_TIME_Y (17)

#define PIPBOY_HOLODISK_LINES_MAX (35)

#define PIPBOY_WINDOW_CONTENT_VIEW_X (254)
#define PIPBOY_WINDOW_CONTENT_VIEW_Y (46)
#define PIPBOY_WINDOW_CONTENT_VIEW_WIDTH (374)
#define PIPBOY_WINDOW_CONTENT_VIEW_HEIGHT (410)

#define PIPBOY_IDLE_TIMEOUT (120000)

#define PIPBOY_BOMB_COUNT (16)

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

typedef void PipboyRenderProc(int a1);

extern const Rect gPipboyWindowContentRect;
extern const int gPipboyFrmIds[PIPBOY_FRM_COUNT];

extern QuestDescription* gQuestDescriptions;
extern int gQuestsCount;
extern HolodiskDescription* gHolodiskDescriptions;
extern int gHolodisksCount;
extern int gPipboyRestOptionsCount;
extern bool gPipboyWindowIsoWasEnabled;
extern const HolidayDescription gHolidayDescriptions[HOLIDAY_COUNT];
extern PipboyRenderProc* off_51C170[5];

extern Size gPipboyFrmSizes[PIPBOY_FRM_COUNT];
extern MessageListItem gPipboyMessageListItem;
extern MessageList gPipboyMessageList;
extern STRUCT_664350 stru_664350[24];
extern MessageList gQuestsMessageList;
extern int gPipboyQuestLocationsCount;
extern unsigned char* gPipboyWindowBuffer;
extern unsigned char* gPipboyFrmData[PIPBOY_FRM_COUNT];
extern int gPipboyWindowHolodisksCount;
extern int gPipboyMouseY;
extern int gPipboyMouseX;
extern unsigned int gPipboyLastEventTimestamp;
extern int gPipboyHolodiskLastPage;
extern int dword_664460[22];
extern int dword_6644B8;
extern int gPipboyPreviousMouseX;
extern int gPipboyPreviousMouseY;
extern int gPipboyWindow;
extern CacheEntry* gPipboyFrmHandles[PIPBOY_FRM_COUNT];
extern int dword_6644F4;
extern int gPipboyWindowButtonCount;
extern int gPipboyWindowOldFont;
extern bool dword_664500;
extern int dword_664504;
extern int gPipboyTab;
extern int dword_66450C;
extern int gPipboyWindowButtonStart;
extern int gPipboyCurrentLine;
extern int dword_664518;
extern int dword_66451C;
extern int dword_664520;
extern int gPipboyLinesCount;
extern unsigned char byte_664528;
extern unsigned char byte_664529;
extern unsigned char byte_66452A;

int pipboyOpen(bool forceRest);
int pipboyWindowInit(bool forceRest);
void pipboyWindowFree();
void sub_497918();
void pipboyInit();
void pipboyReset();
void pipboyDrawNumber(int value, int digits, int x, int y);
void pipboyDrawDate();
void pipboyDrawText(const char* text, int a2, int a3);
void pipboyDrawBackButton(int a1);
int sub_497BD4(File* stream);
int pipboySave(File* stream);
int pipboyLoad(File* stream);
void pipboyWindowHandleStatus(int a1);
void pipboyWindowRenderQuestLocationList(int a1);
void pipboyRenderHolodiskText();
int pipboyWindowRenderHolodiskList(int a1);
int sub_498D34(const void* a1, const void* a2);
void pipboyWindowHandleAutomaps(int a1);
int sub_498F30(int a1);
int sub_499150(int a1);
void pipboyHandleVideoArchive(int a1);
int pipboyRenderVideoArchive(int a1);
void pipboyHandleAlarmClock(int a1);
void pipboyWindowRenderRestOptions(int a1);
void pipboyDrawHitPoints();
void pipboyWindowCreateButtons(int a1, int a2, bool a3);
void pipboyWindowDestroyButtons();
bool pipboyRest(int hours, int minutes, int kind);
bool sub_499FCC(int a1);
bool sub_49A008();
void sub_49A03C(int* hours, int* minutes, int wakeUpHour);
int pipboyRenderScreensaver();
int questInit();
void questFree();
int questDescriptionCompare(const void* a1, const void* a2);
int holodiskInit();
void holodiskFree();

#endif /* PIPBOY_H */