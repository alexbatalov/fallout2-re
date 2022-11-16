#ifndef SKILL_H
#define SKILL_H

#include <stdbool.h>

#include "db.h"
#include "game/message.h"
#include "game/object_types.h"
#include "game/proto_types.h"
#include "game/skill_defs.h"

#define SKILLS_MAX_USES_PER_DAY (3)

#define REPAIRABLE_DAMAGE_FLAGS_LENGTH (5)
#define HEALABLE_DAMAGE_FLAGS_LENGTH (5)

typedef struct SkillDescription {
    char* name;
    char* description;
    char* attributes;
    int frmId;
    int defaultValue;
    int statModifier;
    int stat1;
    int stat2;
    int field_20;
    int experience;
    int field_28;
} SkillDescription;

extern const int gRepairableDamageFlags[REPAIRABLE_DAMAGE_FLAGS_LENGTH];
extern const int gHealableDamageFlags[HEALABLE_DAMAGE_FLAGS_LENGTH];

extern SkillDescription gSkillDescriptions[SKILL_COUNT];
extern int _gIsSteal;
extern int _gStealCount;
extern int _gStealSize;

extern int _timesSkillUsed[SKILL_COUNT][SKILLS_MAX_USES_PER_DAY];
extern int gTaggedSkills[NUM_TAGGED_SKILLS];
extern MessageList gSkillsMessageList;

int skillsInit();
void skillsReset();
void skillsExit();
int skillsLoad(File* stream);
int skillsSave(File* stream);
void protoCritterDataResetSkills(CritterProtoData* data);
void skillsSetTagged(int* skills, int count);
void skillsGetTagged(int* skills, int count);
bool skillIsTagged(int skill);
int skillGetValue(Object* critter, int skill);
int skillGetDefaultValue(int skill);
int skillGetBaseValue(Object* critter, int skill);
int skillAdd(Object* critter, int skill);
int skillAddForce(Object* critter, int skill);
int skillsGetCost(int a1);
int skillSub(Object* critter, int skill);
int skillSubForce(Object* critter, int skill);
int skillRoll(Object* critter, int skill, int a3, int* a4);
int skill_contest(Object* attacker, Object* defender, int skill, int attackerModifier, int defenderModifier, int* howMuch);
char* skillGetName(int skill);
char* skillGetDescription(int skill);
char* skillGetAttributes(int skill);
int skillGetFrmId(int skill);
void _show_skill_use_messages(Object* obj, int skill, Object* a3, int a4, int a5);
int skillUse(Object* obj, Object* a2, int skill, int a4);
int skillsPerformStealing(Object* a1, Object* a2, Object* item, bool isPlanting);
int skillGetGameDifficultyModifier(int skill);
int skillGetFreeUsageSlot(int skill);
int skillUpdateLastUse(int skill);
int skill_use_slot_clear();
int skillsUsageSave(File* stream);
int skillsUsageLoad(File* stream);
char* skillsGetGenericResponse(Object* critter, bool isDude);

// Returns true if skill is valid.
static inline bool skillIsValid(int skill)
{
    return skill >= 0 && skill < SKILL_COUNT;
}

#endif /* SKILL_H */
