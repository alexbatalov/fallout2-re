#ifndef COMBAT_H
#define COMBAT_H

#include "animation.h"
#include "db.h"
#include "combat_defs.h"
#include "message.h"
#include "obj_types.h"
#include "party_member.h"
#include "proto_types.h"

#define CALLED_SHOW_WINDOW_X (68)
#define CALLED_SHOW_WINDOW_Y (20)
#define CALLED_SHOW_WINDOW_WIDTH (504)
#define CALLED_SHOW_WINDOW_HEIGHT (309)

extern char byte_500B50[];

extern int dword_51093C;
extern int dword_510940;
extern unsigned int gCombatState;
extern STRUCT_510948* off_510948;
extern STRUCT_664980* off_51094C;
extern bool dword_510950;
extern const int dword_510954[HIT_LOCATION_COUNT];
extern CriticalHitDescription gCriticalHitTables[KILL_TYPE_COUNT][HIT_LOCATION_COUNT][CRTICIAL_EFFECT_COUNT];
extern CriticalHitDescription gPlayerCriticalHitTable[HIT_LOCATION_COUNT][CRTICIAL_EFFECT_COUNT];
extern int dword_517F98;
extern bool dword_517F9C;
extern const int dword_517FA0[WEAPON_CRITICAL_FAILURE_TYPE_COUNT][WEAPON_CRITICAL_FAILURE_EFFECT_COUNT];
extern const int dword_51802C[4];
extern const int dword_51803C[4];
extern const int dword_51804C[4];

extern Attack stru_56D2B0;
extern MessageList gCombatMessageList;
extern Object* gCalledShotCritter;
extern int gCalledShotWindow;
extern int dword_56D378;
extern int dword_56D37C;
extern Object* off_56D380;
extern int dword_56D384;
extern Object* off_56D388;
extern int dword_56D38C;
extern Object** off_56D390;
extern int dword_56D394;
extern int dword_56D398;
extern int dword_56D39C;
extern Attack stru_56D3A0;
extern Attack stru_56D458;

int combatInit();
void combatReset();
void combatExit();
int find_cid(int a1, int a2, Object** a3, int a4);
int combatLoad(File* stream);
int combatSave(File* stream);
bool combat_safety_invalidate_weapon(Object* a1, Object* a2, int hitMode, Object* a4, int* a5);
bool combat_safety_invalidate_weapon_func(Object* critter, Object* weapon, int hitMode, Object* a4, int* a5, Object* a6);
bool combatTestIncidentalHit(Object* a1, Object* a2, Object* a3, Object* a4);
Object* combat_whose_turn();
void combat_data_init(Object* obj);
int combatCopyAIInfo(int a1, int a2);
Object* combatAIInfoGetFriendlyDead(Object* obj);
int combatAIInfoSetFriendlyDead(Object* a1, Object* a2);
Object* combatAIInfoGetLastTarget(Object* obj);
int combatAIInfoSetLastTarget(Object* a1, Object* a2);
Object* combatAIInfoGetLastItem(Object* obj);
int combatAIInfoSetLastItem(Object* obj, Object* a2);
void combat_begin(Object* a1);
void combat_begin_extra(Object* a1);
void combat_update_critter_outline_for_los(Object* critter, bool a2);
void combat_over();
void combat_over_from_load();
void combat_give_exps(int exp_points);
void combat_add_noncoms();
int compare_faster(const void* a1, const void* a2);
void combat_sequence_init(Object* a1, Object* a2);
void combat_sequence();
void combatAttemptEnd();
void combat_turn_run();
int combat_input();
void combat_set_move_all();
int combat_turn(Object* a1, bool a2);
bool combat_should_end();
void combat(STRUCT_664980* attack);
void attackInit(Attack* attack, Object* a2, Object* a3, int a4, int a5);
int combat_attack(Object* a1, Object* a2, int a3, int a4);
int combat_bullet_start(const Object* a1, const Object* a2);
bool check_ranged_miss(Attack* attack);
int shoot_along_path(Attack* attack, int a2, int a3, int anim);
int compute_spray(Attack* attack, int accuracy, int* a3, int* a4, int anim);
int attackComputeEnhancedKnockout(Attack* attack);
int attackCompute(Attack* attack);
void compute_explosion_on_extras(Attack* attack, int a2, int a3, int a4);
int attackComputeCriticalHit(Attack* a1);
int attackFindInvalidFlags(Object* a1, Object* a2);
int attackComputeCriticalFailure(Attack* attack);
int determine_to_hit(Object* a1, Object* a2, int hitLocation, int hitMode);
int determine_to_hit_no_range(Object* a1, Object* a2, int a3, int a4, unsigned char* a5);
int determine_to_hit_from_tile(Object* a1, int a2, Object* a3, int a4, int a5);
int attackDetermineToHit(Object* attacker, int tile, Object* defender, int hitLocation, int hitMode, int a6);
void attackComputeDamage(Attack* attack, int ammoQuantity, int a3);
void attackComputeDeathFlags(Attack* attack);
void apply_damage(Attack* attack, bool animated);
void check_for_death(Object* a1, int a2, int* a3);
void set_new_results(Object* a1, int a2);
void damage_object(Object* a1, int damage, bool animated, int a4, Object* a5);
void combat_display(Attack* attack);
void combatCopyDamageAmountDescription(char* dest, Object* critter_obj, int damage);
void combatAddDamageFlagsDescription(char* a1, int flags, Object* a3);
void combat_anim_begin();
void combat_anim_finished();
void combat_standup(Object* a1);
void print_tohit(unsigned char* dest, int dest_pitch, int a3);
char* hitLocationGetName(Object* critter, int hitLocation);
void draw_loc_off(int a1, int a2);
void sub_4261C0(int a1, int a2);
void sub_4261CC(int eventCode, int color);
int calledShotSelectHitLocation(Object* critter, int* hitLocation, int hitMode);
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
    return (gCombatState & COMBAT_STATE_0x01) != 0;
}

#endif /* COMBAT_H */
