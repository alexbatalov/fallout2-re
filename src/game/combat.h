#ifndef FALLOUT_GAME_COMBAT_H_
#define FALLOUT_GAME_COMBAT_H_

#include "game/anim.h"
#include "game/combat_defs.h"
#include "db.h"
#include "game/message.h"
#include "obj_types.h"
#include "party_member.h"
#include "proto_types.h"

extern int combatNumTurns;
extern unsigned int combat_state;
extern STRUCT_664980* gcsd;
extern bool combat_call_display;
extern int cf_table[WEAPON_CRITICAL_FAILURE_TYPE_COUNT][WEAPON_CRITICAL_FAILURE_EFFECT_COUNT];

extern MessageList combat_message_file;
extern Object* combat_turn_obj;
extern int combat_exps;
extern int combat_free_move;

int combat_init();
void combat_reset();
void combat_exit();
int find_cid(int a1, int a2, Object** a3, int a4);
int combat_load(File* stream);
int combat_save(File* stream);
bool combat_safety_invalidate_weapon(Object* attacker, Object* weapon, int hitMode, Object* defender, int* safeDistancePtr);
bool combat_safety_invalidate_weapon_func(Object* attacker, Object* weapon, int hitMode, Object* defender, int* safeDistancePtr, Object* attackerFriend);
bool combatTestIncidentalHit(Object* attacker, Object* defender, Object* attackerFriend, Object* weapon);
Object* combat_whose_turn();
void combat_data_init(Object* obj);
Object* combatAIInfoGetFriendlyDead(Object* obj);
int combatAIInfoSetFriendlyDead(Object* a1, Object* a2);
Object* combatAIInfoGetLastTarget(Object* obj);
int combatAIInfoSetLastTarget(Object* a1, Object* a2);
Object* combatAIInfoGetLastItem(Object* obj);
int combatAIInfoSetLastItem(Object* obj, Object* a2);
int combatAIInfoGetLastMove(Object* object);
int combatAIInfoSetLastMove(Object* object, int move);
void combat_update_critter_outline_for_los(Object* critter, bool a2);
void combat_over_from_load();
void combat_give_exps(int exp_points);
int combat_in_range(Object* critter);
void combat_end();
void combat_turn_run();
void combat_end_turn();
void combat(STRUCT_664980* attack);
void combat_ctd_init(Attack* attack, Object* a2, Object* a3, int a4, int a5);
int combat_attack(Object* a1, Object* a2, int a3, int a4);
int combat_bullet_start(const Object* a1, const Object* a2);
void compute_explosion_on_extras(Attack* attack, int a2, bool isGrenade, int a4);
int determine_to_hit(Object* a1, Object* a2, int hitLocation, int hitMode);
int determine_to_hit_no_range(Object* a1, Object* a2, int a3, int a4, unsigned char* a5);
int determine_to_hit_from_tile(Object* a1, int a2, Object* a3, int a4, int a5);
void death_checks(Attack* attack);
void apply_damage(Attack* attack, bool animated);
void combat_display(Attack* attack);
void combat_anim_begin();
void combat_anim_finished();
int combat_check_bad_shot(Object* attacker, Object* defender, int hitMode, bool aiming);
bool combat_to_hit(Object* target, int* accuracy);
void combat_attack_this(Object* a1);
void combat_outline_on();
void combat_outline_off();
void combat_highlight_change();
bool combat_is_shot_blocked(Object* a1, int from, int to, Object* a4, int* a5);
int combat_player_knocked_out_by();
int combat_explode_scenery(Object* a1, Object* a2);
void combat_delete_critter(Object* obj);
void combatKillCritterOutsideCombat(Object* critter_obj, char* msg);

static inline bool isInCombat()
{
    return (combat_state & COMBAT_STATE_0x01) != 0;
}

#endif /* FALLOUT_GAME_COMBAT_H_ */
