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
int action_blood(Object* obj, int anim, int delay);
int pick_death(Object* attacker, Object* defender, Object* weapon, int damage, int anim, bool isFallingBack);
int check_death(Object* obj, int anim, int minViolenceLevel, bool isFallingBack);
int internal_destroy(Object* a1, Object* a2);
void show_damage_to_object(Object* a1, int damage, int flags, Object* weapon, bool isFallingBack, int knockbackDistance, int knockbackRotation, int a8, Object* a9, int a10);
int show_death(Object* obj, int anim);
int show_damage_extras(Attack* attack);
void show_damage(Attack* attack, int a2, int a3);
int action_attack(Attack* attack);
int action_melee(Attack* attack, int a2);
int action_ranged(Attack* attack, int a2);
int is_next_to(Object* a1, Object* a2);
int action_climb_ladder(Object* a1, Object* a2);
int action_use_an_item_on_object(Object* a1, Object* a2, Object* a3);
int action_use_an_object(Object* a1, Object* a2);
int actionPickUp(Object* critter, Object* item);
int action_loot_container(Object* a1, Object* a2);
int action_skill_use(int a1);
int actionUseSkill(Object* a1, Object* a2, int skill);
bool is_hit_from_front(Object* a1, Object* a2);
bool can_see(Object* a1, Object* a2);
int pick_fall(Object* obj, int anim);
bool action_explode_running();
int actionExplode(int tile, int elevation, int minDamage, int maxDamage, Object* a5, bool a6);
int report_explosion(Attack* attack, Object* a2);
int finished_explosion(Object* a1, Object* a2);
int compute_explosion_damage(int min, int max, Object* a3, int* a4);
int actionTalk(Object* a1, Object* a2);
int can_talk_to(Object* a1, Object* a2);
int talk_to(Object* a1, Object* a2);
void action_dmg(int tile, int elevation, int minDamage, int maxDamage, int damageType, bool animated, bool bypassArmor);
int report_dmg(Attack* attack, Object* a2);
int compute_dmg_damage(int min, int max, Object* obj, int* a4, int damage_type);
bool actionCheckPush(Object* a1, Object* a2);
int actionPush(Object* a1, Object* a2);
int action_can_talk_to(Object* a1, Object* a2);

#endif /* ACTIONS_H */
