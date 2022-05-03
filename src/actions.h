#ifndef ACTIONS_H
#define ACTIONS_H

#include "combat_defs.h"
#include "obj_types.h"
#include "proto_types.h"

#include <stdbool.h>

extern int dword_5106D0;
extern const int gNormalDeathAnimations[DAMAGE_TYPE_COUNT];
extern const int gMaximumBloodDeathAnimations[DAMAGE_TYPE_COUNT];

int actionKnockdown(Object* obj, int* anim, int maxDistance, int rotation, int delay);
int sub_410568(Object* obj, int anim, int delay);
int sub_41060C(Object* attacker, Object* defender, Object* weapon, int damage, int anim, bool isFallingBack);
int sub_410814(Object* obj, int anim, int minViolenceLevel, bool isFallingBack);
int sub_4108C8(Object* a1, Object* a2);
void sub_4108D0(Object* a1, int a2, int a3, Object* a4, bool isFallingBack, int a6, int a7, int a8, Object* a9, int a10);
int sub_410E24(Object* obj, int anim);
int sub_410FEC(Attack* attack);
void sub_4110AC(Attack* attack, int a2, int a3);
int sub_411224(Attack* attack);
int sub_4112B4(Attack* attack, int a2);
int sub_411600(Attack* attack, int a2);
int sub_411D68(Object* a1, Object* a2);
int sub_411DB4(Object* a1, Object* a2);
int sub_411F2C(Object* a1, Object* a2, Object* a3);
int sub_412114(Object* a1, Object* a2);
int actionPickUp(Object* critter, Object* item);
int sub_4123E8(Object* a1, Object* a2);
int sub_4124E0(int a1);
int actionUseSkill(Object* a1, Object* a2, int skill);
bool sub_412BC4(Object* a1, Object* a2);
bool sub_412BEC(Object* a1, Object* a2);
int sub_412C1C(Object* obj, int anim);
bool sub_412CE4();
int actionExplode(int tile, int elevation, int minDamage, int maxDamage, Object* a5, bool a6);
int sub_413144(Attack* attack, Object* a2);
int sub_4132C0(Object* a1, Object* a2);
int sub_4132CC(int min, int max, Object* a3, int* a4);
int actionTalk(Object* a1, Object* a2);
int sub_413420(Object* a1, Object* a2);
int sub_413488(Object* a1, Object* a2);
void sub_413494(int tile, int elevation, int minDamage, int maxDamage, int damageType, bool animated, bool bypassArmor);
int sub_41363C(Attack* attack, Object* a2);
int sub_413660(int min, int max, Object* obj, int* a4, int damage_type);
bool actionCheckPush(Object* a1, Object* a2);
int actionPush(Object* a1, Object* a2);
int sub_413970(Object* a1, Object* a2);

#endif /* ACTIONS_H */
