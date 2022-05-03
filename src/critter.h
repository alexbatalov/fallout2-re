#ifndef CRITTER_H
#define CRITTER_H

#include "db.h"
#include "message.h"
#include "obj_types.h"
#include "proto_types.h"

#include <stdbool.h>

// Maximum length of dude's name length.
#define DUDE_NAME_MAX_LENGTH (32)

// The number of effects caused by radiation.
//
// A radiation effect is an identifier and does not have it's own name. It's
// stat is specified in [gRadiationEffectStats], and it's amount is specified
// in [gRadiationEffectPenalties] for every [RadiationLevel].
#define RADIATION_EFFECT_COUNT 8

// Radiation levels.
//
// The names of levels are taken from Fallout 3, comments from Fallout 2.
typedef enum RadiationLevel {
    // Very nauseous.
    RADIATION_LEVEL_NONE,

    // Slightly fatigued.
    RADIATION_LEVEL_MINOR,

    // Vomiting does not stop.
    RADIATION_LEVEL_ADVANCED,

    // Hair is falling out.
    RADIATION_LEVEL_CRITICAL,

    // Skin is falling off.
    RADIATION_LEVEL_DEADLY,

    // Intense agony.
    RADIATION_LEVEL_FATAL,

    // The number of radiation levels.
    RADIATION_LEVEL_COUNT,
} RadiationLevel;

typedef enum DudeState {
    DUDE_STATE_SNEAKING = 0,
    DUDE_STATE_LEVEL_UP_AVAILABLE = 3,
    DUDE_STATE_ADDICTED = 4,
} DudeState;

extern char byte_50141C[];
extern char byte_501494[];

extern char* off_51833C;
extern const int gRadiationEnduranceModifiers[RADIATION_LEVEL_COUNT];
extern const int gRadiationEffectStats[RADIATION_EFFECT_COUNT];
extern const int gRadiationEffectPenalties[RADIATION_LEVEL_COUNT][RADIATION_EFFECT_COUNT];
extern Object* off_518438;

extern MessageList gCritterMessageList;
extern char gDudeName[DUDE_NAME_MAX_LENGTH];
extern int dword_56D77C;
extern int gKillsByType[KILL_TYPE_COUNT];
extern int dword_56D7CC;

int critterInit();
void critterReset();
void critterExit();
int critterLoad(File* stream);
int critterSave(File* stream);
char* critterGetName(Object* obj);
void critterProtoDataCopy(CritterProtoData* dest, CritterProtoData* src);
int dudeSetName(const char* name);
void dudeResetName();
int critterGetHitPoints(Object* critter);
int critterAdjustHitPoints(Object* critter, int amount);
int critterGetPoison(Object* critter);
int critterAdjustPoison(Object* obj, int amount);
int poisonEventProcess(Object* obj, void* data);
int critterGetRadiation(Object* critter);
int critterAdjustRadiation(Object* obj, int amount);
int sub_42D4F4(Object* critter);
int sub_42D618(Object* obj, void* data);
int sub_42D624(Object* obj, void* data);
void sub_42D63C(Object* obj, int radiationLevel, bool direction);
int radiationEventProcess(Object* obj, void* data);
int radiationEventRead(File* stream, void** dataPtr);
int radiationEventWrite(File* stream, void* data);
int critterGetDamageType(Object* critter);
int killsIncByType(int killType);
int killsGetByType(int killType);
int killsLoad(File* stream);
int killsSave(File* stream);
int critterGetKillType(Object* critter);
char* killTypeGetName(int killType);
char* killTypeGetDescription(int killType);
int sub_42D9F4(Object* obj, int a2);
int sub_42DA54(Object* obj, void* data);
void critterKill(Object* critter, int anim, bool a3);
int critterGetExp(Object* critter);
bool critterIsActive(Object* critter);
bool critterIsDead(Object* critter);
bool critterIsCrippled(Object* critter);
bool sub_42DD80(Object* critter);
int critterGetBodyType(Object* critter);
int gcdLoad(const char* path);
int protoCritterDataRead(File* stream, CritterProtoData* critterData);
int gcdSave(const char* path);
int protoCritterDataWrite(File* stream, CritterProtoData* critterData);
void dudeDisableState(int state);
void dudeEnableState(int state);
void dudeToggleState(int state);
bool dudeHasState(int state);
int sneakEventProcess(Object* obj, void* data);
int sub_42E3E4(Object* obj, void* data);
bool dudeIsSneaking();
int knockoutEventProcess(Object* obj, void* data);
int sub_42E460(Object* obj, void* data);
int sub_42E4C0(Object* a1, Object* a2);
bool sub_42E564();
int critterGetMovementPointCostAdjustedForCrippledLegs(Object* critter, int a2);
bool critterIsEncumbered(Object* critter);
bool critterIsFleeing(Object* a1);
bool sub_42E6AC(int pid, int flag);

#endif /* CRITTER_H */
