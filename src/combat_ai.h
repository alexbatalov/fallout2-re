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

void sub_426F00(char* str, int* out_value);
int sub_426FA4(const char* str, const char** list, int count, int* out_value);
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
int sub_428398(Object* a1, Object* a2, int num);
int sub_428480(Object* critter);
void sub_428868(Object* a1, Object* a2);
int sub_42899C(Object* a1, Object* a2, int a3);
bool sub_428AC4(Object* a1, int a2, int a3);
int sub_428B1C(const void* a1, const void* a2);
int sub_428B8C(const void* p1, const void* p2);
int sub_428BE4(const void* p1, const void* p2);
Object* sub_428C3C(Object* a1, Object* a2, int a3);
Object* sub_428CF4(Object* a1, Object* a2, int a3);
int sub_428DB0(Object* a1, Object** a2, Object** a3, Object** a4);
Object* sub_428F4C(Object* a1);
int sub_4291C4(Object* a1, Object* a2);
int sub_429210(Object** a1, int a2);
void sub_4292C0();
int sub_4292D4(Object* critter_obj, Object* weapon_obj, Object** out_ammo_obj);
bool sub_42938C(AiPacket* ai, int attackType);
Object* sub_4293BC(Object* a1, Object* a2, Object* a3, Object* a4);
bool sub_4298EC(Object* critter, Object* weapon, int hitMode);
Object* sub_4299A0(Object* critter, int a2, Object* a3);
Object* sub_429A6C(Object* critter);
bool aiCanUseItem(Object* obj, Object* a2);
Object* sub_429C18(Object* critter, int itemType);
Object* sub_429D60(Object* a1, Object* a2);
int sub_429DB4(Object* a1, Object* a2, Object* a3);
int sub_429FC8(Object* a1, Object* a2, int actionPoints, int a4);
int sub_42A1D4(Object* a1, Object* a2, int* a3);
int sub_42A410(STRUCT_832* a1, int a2);
bool sub_42A518(Object* a1, Object* a2, Object* a3, int tile, int* distance);
int sub_42A5B8(Object* a1, int* hitMode, Object** weapon, Object* a4);
int sub_42A670(Object* a1, Object* a2, int a3);
int sub_42A748(Object* a1, Object* a2, int a3);
int sub_42A7D8(Object* a1, Object* a2);
int sub_42AE90(Object* critter, Object* item);
void sub_42AECC(Object* critter_obj, int a2);
void sub_42AF78(int a1, void* a2);
void sub_42AFBC();
int sub_42AFDC(Object* a1, Object* a2);
int sub_42B100(AiPacket* ai);
void sub_42B130(Object* a1, Object* a2);
bool sub_42B3FC(Object* a1);
bool sub_42B4A8(Object* a1);
int critterSetTeam(Object* obj, int team);
int critterSetAiPacket(Object* object, int aiPacket);
int sub_42B634(Object* a1, Attack* attack, int a3, int a4);
int sub_42B80C(Object* critter, int type);
Object* sub_42B868(Attack* attack);
int sub_42B90C(Object* obj);
int sub_42B9D4(Object* a1, Object* a2);
bool objectCanHearObject(Object* a1, Object* a2);
int aiMessageListInit();
int aiMessageListFree();
void aiMessageListReloadIfNeeded();
void sub_42BC60(Object* a1);
void sub_42BCD4(Object* a1);
void sub_42BD28(Object* obj);

#endif /* COMBAT_AI_H */
