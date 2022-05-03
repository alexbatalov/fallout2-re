#ifndef PARTY_MEMBER_H
#define PARTY_MEMBER_H

#include "combat_ai_defs.h"
#include "db.h"
#include "obj_types.h"
#include "scripts.h"

#include <stdbool.h>

typedef struct PartyMemberDescription {
    bool areaAttackMode[AREA_ATTACK_MODE_COUNT];
    bool runAwayMode[RUN_AWAY_MODE_COUNT];
    bool bestWeapon[BEST_WEAPON_COUNT];
    bool distanceMode[DISTANCE_COUNT];
    bool attackWho[ATTACK_WHO_COUNT];
    bool chemUse[CHEM_USE_COUNT];
    bool disposition[DISPOSITION_COUNT];
    int level_minimum;
    int level_up_every;
    int level_pids_num;
    int level_pids[5];
} PartyMemberDescription;

typedef struct STRU_519DBC {
    int field_0;
    int field_4; // party member level
    int field_8; // early what?
} STRU_519DBC;

typedef struct STRUCT_519DA8 {
    Object* object;
    Script* script;
    int* vars;
    struct STRUCT_519DA8* next;
} STRUCT_519DA8;

extern int gPartyMemberDescriptionsLength;
extern int* gPartyMemberPids;
extern STRUCT_519DA8* off_519DA4;
extern STRUCT_519DA8* gPartyMembers;
extern int gPartyMembersLength;
extern int dword_519DB0;
extern int dword_519DB4;
extern PartyMemberDescription* gPartyMemberDescriptions;
extern STRU_519DBC* off_519DBC;
extern int dword_519DC0;

int partyMembersInit();
void partyMembersReset();
void partyMembersExit();
int partyMemberGetDescription(Object* object, PartyMemberDescription** partyMemberDescriptionPtr);
void partyMemberDescriptionInit(PartyMemberDescription* partyMemberDescription);
int partyMemberAdd(Object* object);
int partyMemberRemove(Object* object);
int sub_49460C();
int sub_49466C();
int partyMembersSave(File* stream);
int sub_4947AC();
int sub_49480C(STRUCT_519DA8* a1);
int sub_4949C4();
int sub_494A88(STRUCT_519DA8* a1);
int partyMembersLoad(File* stream);
void sub_494D7C();
int sub_494DD0();
int sub_494EB8(int a1);
Object* partyMemberFindByPid(int a1);
bool sub_494F64(Object* object);
bool objectIsPartyMember(Object* object);
int sub_495010();
int sub_495070();
int sub_4950F4(Object* object, int objectId);
int sub_495140();
int sub_495198(Object* object);
int sub_495234(Object* object);
int sub_495388(STRUCT_519DA8* a1);
int sub_4954C4();
int partyMemberGetBestSkill(Object* object);
Object* partyMemberGetBestInSkill(int skill);
int partyGetBestSkillValue(int skill);
int sub_495620();
void sub_495870();
bool partyMemberSupportsDisposition(Object* object, int disposition);
bool partyMemberSupportsAreaAttackMode(Object* object, int areaAttackMode);
bool partyMemberSupportsRunAwayMode(Object* object, int runAwayMode);
bool partyMemberSupportsBestWeapon(Object* object, int bestWeapon);
bool partyMemberSupportsDistance(Object* object, int distanceMode);
bool partyMemberSupportsAttackWho(Object* object, int attackWho);
bool partyMemberSupportsChemUse(Object* object, int chemUse);
int sub_495B60();
int sub_495EA8(Object* object, int a2);
bool partyIsAnyoneCanBeHealedByRest();
int partyGetMaxWoundToHealByRest();

#endif /* PARTY_MEMBER_H */
