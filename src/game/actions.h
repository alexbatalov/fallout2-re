#ifndef FALLOUT_GAME_ACTIONS_H_
#define FALLOUT_GAME_ACTIONS_H_

#include <stdbool.h>

#include "combat_defs.h"
#include "obj_types.h"
#include "proto_types.h"

extern unsigned int rotation;
extern int obj_fid;
extern int obj_pid_old;

void switch_dude();
int action_knockback(Object* obj, int* anim, int maxDistance, int rotation, int delay);
int action_blood(Object* obj, int anim, int delay);
void show_damage_to_object(Object* a1, int damage, int flags, Object* weapon, bool isFallingBack, int knockbackDistance, int knockbackRotation, int a8, Object* a9, int a10);
int show_damage_target(Attack* attack);
int show_damage_extras(Attack* attack);
void show_damage(Attack* attack, int a2, int a3);
int action_attack(Attack* attack);
int throw_change_fid(Object* object, int fid);
int a_use_obj(Object* a1, Object* a2, Object* a3);
int action_use_an_item_on_object(Object* a1, Object* a2, Object* a3);
int action_use_an_object(Object* a1, Object* a2);
int get_an_object(Object* item);
int action_get_an_object(Object* critter, Object* item);
int action_loot_container(Object* critter, Object* container);
int action_skill_use(int a1);
int action_use_skill_in_combat_error(Object* critter);
int action_use_skill_on(Object* a1, Object* a2, int skill);
Object* pick_object(int objectType, bool a2);
int pick_hex();
bool is_hit_from_front(Object* a1, Object* a2);
bool can_see(Object* a1, Object* a2);
int pick_fall(Object* obj, int anim);
bool action_explode_running();
int action_explode(int tile, int elevation, int minDamage, int maxDamage, Object* a5, bool a6);
int action_talk_to(Object* a1, Object* a2);
void action_dmg(int tile, int elevation, int minDamage, int maxDamage, int damageType, bool animated, bool bypassArmor);
bool action_can_be_pushed(Object* a1, Object* a2);
int action_push_critter(Object* a1, Object* a2);
int action_can_talk_to(Object* a1, Object* a2);

#endif /* FALLOUT_GAME_ACTIONS_H_ */
