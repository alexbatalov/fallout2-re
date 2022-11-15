#ifndef FALLOUT_GAME_COMBATAI_H_
#define FALLOUT_GAME_COMBATAI_H_

#include <stdbool.h>

#include "game/combatai_defs.h"
#include "game/combat_defs.h"
#include "db.h"
#include "game/message.h"
#include "game/object_types.h"
#include "party_member.h"

#define AI_PACKET_CHEM_PRIMARY_DESIRE_COUNT (3)

typedef enum AiMessageType {
    AI_MESSAGE_TYPE_RUN,
    AI_MESSAGE_TYPE_MOVE,
    AI_MESSAGE_TYPE_ATTACK,
    AI_MESSAGE_TYPE_MISS,
    AI_MESSAGE_TYPE_HIT,
} AiMessageType;

typedef struct AiMessageRange {
    int start;
    int end;
} AiMessageRange;

typedef struct AiPacket {
    char* name;
    int packet_num;
    int max_dist;
    int min_to_hit;
    int min_hp;
    int aggression;
    int hurt_too_much;
    int secondary_freq;
    int called_freq;
    int font;
    int color;
    int outline_color;
    int chance;
    AiMessageRange run;
    AiMessageRange move;
    AiMessageRange attack;
    AiMessageRange miss;
    AiMessageRange hit[HIT_LOCATION_SPECIFIC_COUNT];
    int area_attack_mode;
    int run_away_mode;
    int best_weapon;
    int distance;
    int attack_who;
    int chem_use;
    int chem_primary_desire[AI_PACKET_CHEM_PRIMARY_DESIRE_COUNT];
    int disposition;
    char* body_type;
    char* general_type;
} AiPacket;

typedef struct AiRetargetData {
    Object* source;
    Object* target;
    Object* critterList[100];
    int ratingList[100];
    int critterCount;
    int sourceTeam;
    int sourceRating;
    bool notSameTile;
    int* tiles;
    int currentTileIndex;
    int sourceIntelligence;
} AiRetargetData;

extern const char* area_attack_mode_strs[AREA_ATTACK_MODE_COUNT];
extern const char* attack_who_mode_strs[ATTACK_WHO_COUNT];
extern const char* weapon_pref_strs[BEST_WEAPON_COUNT];
extern const char* chem_use_mode_strs[CHEM_USE_COUNT];
extern const char* distance_pref_strs[DISTANCE_COUNT];
extern const char* run_away_mode_strs[RUN_AWAY_MODE_COUNT];
extern const char* disposition_strs[DISPOSITION_COUNT];

int combat_ai_init();
void combat_ai_reset();
int combat_ai_exit();
int combat_ai_load(File* stream);
int combat_ai_save(File* stream);
int combat_ai_num();
char* combat_ai_name(int packetNum);
int ai_get_burst_value(Object* obj);
int ai_get_run_away_value(Object* obj);
int ai_get_weapon_pref_value(Object* obj);
int ai_get_distance_pref_value(Object* obj);
int ai_get_attack_who_value(Object* obj);
int ai_get_chem_use_value(Object* obj);
int ai_set_burst_value(Object* critter, int areaAttackMode);
int ai_set_run_away_value(Object* obj, int run_away_mode);
int ai_set_weapon_pref_value(Object* critter, int bestWeapon);
int ai_set_distance_pref_value(Object* critter, int distance);
int ai_set_attack_who_value(Object* critter, int attackWho);
int ai_set_chem_use_value(Object* critter, int chemUse);
int ai_get_disposition(Object* obj);
int ai_set_disposition(Object* obj, int a2);
int compare_strength(const void* p1, const void* p2);
int compare_weakness(const void* p1, const void* p2);
Object* ai_danger_source(Object* a1);
int caiSetupTeamCombat(Object* a1, Object* a2);
int caiTeamCombatInit(Object** a1, int a2);
void caiTeamCombatExit();
Object* ai_search_inven_weap(Object* critter, int a2, Object* a3);
Object* ai_search_inven_armor(Object* critter);
int cAIPrepWeaponItem(Object* critter, Object* item);
void cai_attempt_w_reload(Object* critter_obj, int a2);
void combat_ai_begin(int a1, void* a2);
void combat_ai_over();
void combat_ai(Object* a1, Object* a2);
bool combatai_want_to_join(Object* a1);
bool combatai_want_to_stop(Object* a1);
int combatai_switch_team(Object* obj, int team);
int combat_ai_set_ai_packet(Object* object, int aiPacket);
int combatai_msg(Object* a1, Attack* attack, int a3, int a4);
Object* combat_ai_random_target(Attack* attack);
int combatai_check_retaliation(Object* a1, Object* a2);
bool is_within_perception(Object* a1, Object* a2);
void combatai_refresh_messages();
void combatai_notify_onlookers(Object* a1);
void combatai_notify_friends(Object* a1);
void combatai_delete_critter(Object* obj);

#endif /* FALLOUT_GAME_COMBATAI_H_ */
