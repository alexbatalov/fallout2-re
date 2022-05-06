#ifndef COMBAT_AI_H
#define COMBAT_AI_H

#include "combat_ai_defs.h"
#include "combat_defs.h"
#include "db.h"
#include "message.h"
#include "obj_types.h"
#include "party_member.h"

#include <stdbool.h>

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

typedef struct STRUCT_832 {
    Object* field_0;
    Object* field_4;
    Object* field_8[100];
    int field_198[100];
    int field_328;
    int field_32C;
    int field_330;
    int field_334;
    int* field_338;
    int field_33C;
    int field_340;
} STRUCT_832;

extern Object* off_51805C;
extern int gAiPacketsLength;
extern AiPacket* gAiPackets;
extern int dword_518068;
extern const char* gAreaAttackModeKeys[AREA_ATTACK_MODE_COUNT];
extern const char* gAttackWhoKeys[ATTACK_WHO_COUNT];
extern const char* gBestWeaponKeys[BEST_WEAPON_COUNT];
extern const char* gChemUseKeys[CHEM_USE_COUNT];
extern const char* gDistanceModeKeys[DISTANCE_COUNT];
extern const char* gRunAwayModeKeys[RUN_AWAY_MODE_COUNT];
extern const char* gDispositionKeys[DISPOSITION_COUNT];
extern const char* gHurtTooMuchKeys[HURT_COUNT];
extern const int dword_518124[5];
extern const int dword_518138[6];
extern Object* off_518150;
extern Object* off_518154;
extern const int dword_518158[BEST_WEAPON_COUNT + 1][5];
extern const int dword_51820C[DISTANCE_COUNT];
extern int gLanguageFilter;

extern MessageList gCombatAiMessageList;
extern char byte_56D518[260];
extern int dword_56D61C;
extern Object** off_56D620;
extern char byte_56D624[268];

void parse_hurt_str(char* str, int* out_value);
int cai_match_str_to_list(const char* str, const char** list, int count, int* out_value);
void aiPacketInit(AiPacket* ai);
int aiInit();
void aiReset();
int aiExit();
int aiLoad(File* stream);
int aiSave(File* stream);
int aiPacketRead(File* stream, AiPacket* ai);
int aiPacketWrite(File* stream, AiPacket* ai);
AiPacket* aiGetPacket(Object* obj);
AiPacket* aiGetPacketByNum(int aiPacketNum);
int aiGetAreaAttackMode(Object* obj);
int aiGetRunAwayMode(Object* obj);
int aiGetBestWeapon(Object* obj);
int aiGetDistance(Object* obj);
int aiGetAttackWho(Object* obj);
int aiGetChemUse(Object* obj);
int aiSetAreaAttackMode(Object* critter, int areaAttackMode);
int aiSetRunAwayMode(Object* obj, int run_away_mode);
int aiSetBestWeapon(Object* critter, int bestWeapon);
int aiSetDistance(Object* critter, int distance);
int aiSetAttackWho(Object* critter, int attackWho);
int aiSetChemUse(Object* critter, int chemUse);
int aiGetDisposition(Object* obj);
int aiSetDisposition(Object* obj, int a2);
int ai_magic_hands(Object* a1, Object* a2, int num);
int ai_check_drugs(Object* critter);
void ai_run_away(Object* a1, Object* a2);
int ai_move_away(Object* a1, Object* a2, int a3);
bool ai_find_friend(Object* a1, int a2, int a3);
int compare_nearer(const void* a1, const void* a2);
int compare_strength(const void* p1, const void* p2);
int compare_weakness(const void* p1, const void* p2);
Object* ai_find_nearest_team(Object* a1, Object* a2, int a3);
Object* ai_find_nearest_team_in_combat(Object* a1, Object* a2, int a3);
int ai_find_attackers(Object* a1, Object** a2, Object** a3, Object** a4);
Object* ai_danger_source(Object* a1);
int caiSetupTeamCombat(Object* a1, Object* a2);
int caiTeamCombatInit(Object** a1, int a2);
void caiTeamCombatExit();
int ai_have_ammo(Object* critter_obj, Object* weapon_obj, Object** out_ammo_obj);
bool caiHasWeapPrefType(AiPacket* ai, int attackType);
Object* ai_best_weapon(Object* a1, Object* a2, Object* a3, Object* a4);
bool ai_can_use_weapon(Object* critter, Object* weapon, int hitMode);
Object* ai_search_inven_weap(Object* critter, int a2, Object* a3);
Object* ai_search_inven_armor(Object* critter);
bool aiCanUseItem(Object* obj, Object* a2);
Object* ai_search_environ(Object* critter, int itemType);
Object* ai_retrieve_object(Object* a1, Object* a2);
int ai_pick_hit_mode(Object* a1, Object* a2, Object* a3);
int ai_move_steps_closer(Object* a1, Object* a2, int actionPoints, int a4);
int cai_retargetTileFromFriendlyFire(Object* a1, Object* a2, int* a3);
int cai_retargetTileFromFriendlyFireSubFunc(STRUCT_832* a1, int a2);
bool cai_attackWouldIntersect(Object* a1, Object* a2, Object* a3, int tile, int* distance);
int ai_switch_weapons(Object* a1, int* hitMode, Object** weapon, Object* a4);
int ai_called_shot(Object* a1, Object* a2, int a3);
int ai_attack(Object* a1, Object* a2, int a3);
int ai_try_attack(Object* a1, Object* a2);
int cAIPrepWeaponItem(Object* critter, Object* item);
void cai_attempt_w_reload(Object* critter_obj, int a2);
void combat_ai_begin(int a1, void* a2);
void combat_ai_over();
int cai_perform_distance_prefs(Object* a1, Object* a2);
int cai_get_min_hp(AiPacket* ai);
void combat_ai(Object* a1, Object* a2);
bool combatai_want_to_join(Object* a1);
bool combatai_want_to_stop(Object* a1);
int critterSetTeam(Object* obj, int team);
int critterSetAiPacket(Object* object, int aiPacket);
int combatai_msg(Object* a1, Attack* attack, int a3, int a4);
int ai_print_msg(Object* critter, int type);
Object* combat_ai_random_target(Attack* attack);
int combatai_rating(Object* obj);
int combatai_check_retaliation(Object* a1, Object* a2);
bool objectCanHearObject(Object* a1, Object* a2);
int aiMessageListInit();
int aiMessageListFree();
void aiMessageListReloadIfNeeded();
void combatai_notify_onlookers(Object* a1);
void combatai_notify_friends(Object* a1);
void combatai_delete_critter(Object* obj);

#endif /* COMBAT_AI_H */
