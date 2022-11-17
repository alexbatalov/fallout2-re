#ifndef FALLOUT_GAME_PARTY_H_
#define FALLOUT_GAME_PARTY_H_

#include <stdbool.h>

#include "plib/db/db.h"
#include "game/object_types.h"

extern int partyMemberMaxCount;
extern int* partyMemberPidList;

int partyMember_init();
void partyMember_reset();
void partyMember_exit();
int partyMemberAdd(Object* object);
int partyMemberRemove(Object* object);
int partyMemberPrepSave();
int partyMemberUnPrepSave();
int partyMemberSave(File* stream);
int partyMemberPrepLoad();
int partyMemberRecoverLoad();
int partyMemberLoad(File* stream);
void partyMemberClear();
int partyMemberSyncPosition();
int partyMemberRestingHeal(int a1);
Object* partyMemberFindObjFromPid(int a1);
bool isPotentialPartyMember(Object* object);
bool isPartyMember(Object* object);
int getPartyMemberCount();
int partyMemberPrepItemSaveAll();
int partyMemberSkill(Object* object);
Object* partyMemberWithHighestSkill(int skill);
int partyMemberHighestSkillLevel(int skill);
void partyMemberSaveProtos();
bool partyMemberHasAIDisposition(Object* object, int disposition);
bool partyMemberHasAIBurstValue(Object* object, int areaAttackMode);
bool partyMemberHasAIRunAwayValue(Object* object, int runAwayMode);
bool partyMemberHasAIWeaponPrefValue(Object* object, int bestWeapon);
bool partyMemberHasAIDistancePrefValue(Object* object, int distanceMode);
bool partyMemberHasAIAttackWhoValue(Object* object, int attackWho);
bool partyMemberHasAIChemUseValue(Object* object, int chemUse);
int partyMemberIncLevels();
bool partyMemberNeedsHealing();
int partyMemberMaxHealingNeeded();

#endif /* FALLOUT_GAME_PARTY_H_ */
