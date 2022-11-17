#ifndef FALLOUT_GAME_EDITOR_H_
#define FALLOUT_GAME_EDITOR_H_

#include "plib/db/db.h"

#define TOWN_REPUTATION_COUNT 19
#define ADDICTION_REPUTATION_COUNT 8

typedef struct TownReputationEntry {
    int gvar;
    int city;
} TownReputationEntry;

extern int character_points;
extern TownReputationEntry town_rep_info[TOWN_REPUTATION_COUNT];
extern int addiction_vars[ADDICTION_REPUTATION_COUNT];
extern int addiction_pics[ADDICTION_REPUTATION_COUNT];

int editor_design(bool isCreationMode);
void CharEditInit();
int get_input_str(int win, int cancelKeyCode, char* text, int maxLength, int x, int y, int textColor, int backgroundColor, int flags);
bool isdoschar(int ch);
char* strmfe(char* dest, const char* name, const char* ext);
bool db_access(const char* fname);
char* AddSpaces(char* string, int length);
char* itostndn(int value, char* dest);
int editor_save(File* stream);
int editor_load(File* stream);
void editor_reset();
void RedrwDMPrk();

#endif /* FALLOUT_GAME_EDITOR_H_ */
