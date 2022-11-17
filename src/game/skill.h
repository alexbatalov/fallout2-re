#ifndef FALLOUT_GAME_SKILL_H_
#define FALLOUT_GAME_SKILL_H_

#include <stdbool.h>

#include "plib/db/db.h"
#include "game/object_types.h"
#include "game/proto_types.h"
#include "game/skill_defs.h"

extern int gIsSteal;
extern int gStealCount;
extern int gStealSize;

int skill_init();
void skill_reset();
void skill_exit();
int skill_load(File* stream);
int skill_save(File* stream);
void skill_set_defaults(CritterProtoData* data);
void skill_set_tags(int* skills, int count);
void skill_get_tags(int* skills, int count);
bool skill_is_tagged(int skill);
int skill_level(Object* critter, int skill);
int skill_base(int skill);
int skill_points(Object* critter, int skill);
int skill_inc_point(Object* critter, int skill);
int skill_inc_point_force(Object* critter, int skill);
int skill_dec_point(Object* critter, int skill);
int skill_dec_point_force(Object* critter, int skill);
int skill_result(Object* critter, int skill, int a3, int* a4);
int skill_contest(Object* attacker, Object* defender, int skill, int attackerModifier, int defenderModifier, int* howMuch);
char* skill_name(int skill);
char* skill_description(int skill);
char* skill_attribute(int skill);
int skill_pic(int skill);
int skill_use(Object* obj, Object* a2, int skill, int a4);
int skill_check_stealing(Object* a1, Object* a2, Object* item, bool isPlanting);
int skill_use_slot_save(File* stream);
int skill_use_slot_load(File* stream);
char* skillGetPartyMemberString(Object* critter, bool isDude);

// Returns true if skill is valid.
static inline bool skillIsValid(int skill)
{
    return skill >= 0 && skill < SKILL_COUNT;
}

#endif /* FALLOUT_GAME_SKILL_H_ */
