#ifndef FALLOUT_GAME_ITEM_H_
#define FALLOUT_GAME_ITEM_H_

#include <stdbool.h>

#include "db.h"
#include "obj_types.h"

#define ADDICTION_COUNT 9

typedef enum AttackType {
    ATTACK_TYPE_NONE,
    ATTACK_TYPE_UNARMED,
    ATTACK_TYPE_MELEE,
    ATTACK_TYPE_THROW,
    ATTACK_TYPE_RANGED,
    ATTACK_TYPE_COUNT,
} AttackType;

typedef struct DrugDescription {
    int drugPid;
    int gvar;
    int field_8;
} DrugDescription;

extern DrugDescription drugInfoList[ADDICTION_COUNT];

int item_init();
void item_reset();
void item_exit();
int item_load(File* stream);
int item_save(File* stream);
int item_add_mult(Object* owner, Object* itemToAdd, int quantity);
int item_add_force(Object* owner, Object* itemToAdd, int quantity);
int item_remove_mult(Object* a1, Object* a2, int quantity);
int item_move(Object* a1, Object* a2, Object* a3, int quantity);
int item_move_force(Object* a1, Object* a2, Object* a3, int quantity);
void item_move_all(Object* a1, Object* a2);
int item_move_all_hidden(Object* a1, Object* a2);
int item_destroy_all_hidden(Object* a1);
int item_drop_all(Object* critter, int tile);
char* item_name(Object* obj);
char* item_description(Object* obj);
int item_get_type(Object* item);
int item_material(Object* item);
int item_size(Object* obj);
int item_weight(Object* item);
int item_cost(Object* obj);
int item_total_cost(Object* obj);
int item_total_weight(Object* obj);
bool item_grey(Object* item_obj);
int item_inv_fid(Object* obj);
Object* item_hit_with(Object* critter, int hitMode);
int item_mp_cost(Object* obj, int hitMode, bool aiming);
int item_count(Object* obj, Object* a2);
int item_queued(Object* obj);
Object* item_replace(Object* a1, Object* a2, int a3);
int item_is_hidden(Object* obj);
int item_w_subtype(Object* a1, int a2);
int item_w_skill(Object* a1, int a2);
int item_w_skill_level(Object* a1, int a2);
int item_w_damage_min_max(Object* weapon, int* minDamagePtr, int* maxDamagePtr);
int item_w_damage(Object* critter, int hitMode);
int item_w_damage_type(Object* critter, Object* weapon);
int item_w_is_2handed(Object* weapon);
int item_w_anim(Object* critter, int hitMode);
int item_w_anim_weap(Object* weapon, int hitMode);
int item_w_max_ammo(Object* ammoOrWeapon);
int item_w_curr_ammo(Object* ammoOrWeapon);
int item_w_caliber(Object* ammoOrWeapon);
void item_w_set_curr_ammo(Object* ammoOrWeapon, int quantity);
int item_w_try_reload(Object* critter, Object* weapon);
bool item_w_can_reload(Object* weapon, Object* ammo);
int item_w_reload(Object* weapon, Object* ammo);
int item_w_range(Object* critter, int hitMode);
int item_w_mp_cost(Object* critter, int hitMode, bool aiming);
int item_w_min_st(Object* weapon);
int item_w_crit_fail(Object* weapon);
int item_w_perk(Object* weapon);
int item_w_rounds(Object* weapon);
int item_w_anim_code(Object* weapon);
int item_w_proj_pid(Object* weapon);
int item_w_ammo_pid(Object* weapon);
char item_w_sound_id(Object* weapon);
int item_w_called_shot(Object* critter, int hitMode);
int item_w_can_unload(Object* weapon);
Object* item_w_unload(Object* weapon);
int item_w_primary_mp_cost(Object* weapon);
int item_w_secondary_mp_cost(Object* weapon);
int item_w_compute_ammo_cost(Object* obj, int* inout_a2);
bool item_w_is_grenade(Object* weapon);
int item_w_area_damage_radius(Object* weapon, int hitMode);
int item_w_grenade_dmg_radius(Object* weapon);
int item_w_rocket_dmg_radius(Object* weapon);
int item_w_ac_adjust(Object* weapon);
int item_w_dr_adjust(Object* weapon);
int item_w_dam_mult(Object* weapon);
int item_w_dam_div(Object* weapon);
int item_ar_ac(Object* armor);
int item_ar_dr(Object* armor, int damageType);
int item_ar_dt(Object* armor, int damageType);
int item_ar_perk(Object* armor);
int item_ar_male_fid(Object* armor);
int item_ar_female_fid(Object* armor);
int item_m_max_charges(Object* miscItem);
int item_m_curr_charges(Object* miscItem);
int item_m_set_charges(Object* miscItem, int charges);
int item_m_cell(Object* miscItem);
int item_m_cell_pid(Object* miscItem);
bool item_m_uses_charges(Object* obj);
int item_m_use_charged_item(Object* critter, Object* item);
int item_m_dec_charges(Object* miscItem);
int item_m_trickle(Object* item_obj, void* data);
bool item_m_on(Object* obj);
int item_m_turn_on(Object* item_obj);
int item_m_turn_off(Object* item_obj);
int item_m_turn_off_from_queue(Object* obj, void* data);
int item_c_max_size(Object* container);
int item_c_curr_size(Object* container);
int item_a_ac_adjust(Object* armor);
int item_a_dr_adjust(Object* armor);
int item_a_dam_mult(Object* armor);
int item_a_dam_div(Object* armor);
int item_d_take_drug(Object* critter_obj, Object* item_obj);
int item_d_clear(Object* obj, void* data);
int item_d_process(Object* obj, void* data);
int item_d_load(File* stream, void** dataPtr);
int item_d_save(File* stream, void* data);
int item_wd_clear(Object* obj, void* a2);
int item_wd_process(Object* obj, void* data);
int item_wd_load(File* stream, void** dataPtr);
int item_wd_save(File* stream, void* data);
void item_d_set_addict(int drugPid);
void item_d_unset_addict(int drugPid);
bool item_d_check_addict(int drugPid);
int item_caps_total(Object* obj);
int item_caps_adjust(Object* obj, int amount);
int item_caps_get_amount(Object* obj);
int item_caps_set_amount(Object* obj, int a2);

#endif /* FALLOUT_GAME_ITEM_H_ */
