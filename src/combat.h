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
int sub_420E24(int a1, int a2, Object** a3, int a4);
int combatLoad(File* stream);
int combatSave(File* stream);
bool sub_4213E8(Object* a1, Object* a2, int hitMode, Object* a4, int* a5);
bool sub_4213FC(Object* critter, Object* weapon, int hitMode, Object* a4, int* a5, Object* a6);
bool sub_4217BC(Object* a1, Object* a2, Object* a3, Object* a4);
Object* sub_4217D4();
void sub_4217E8(Object* obj);
int sub_421850(int a1, int a2);
Object* sub_421880(Object* obj);
int sub_4218AC(Object* a1, Object* a2);
Object* sub_4218EC(Object* obj);
int sub_421918(Object* a1, Object* a2);
Object* sub_42196C(Object* obj);
int sub_421998(Object* obj, Object* a2);
void sub_421A34(Object* a1);
void sub_421C8C(Object* a1);
void sub_421D50(Object* critter, bool a2);
void sub_421EFC();
void sub_422194();
void sub_4221B4(int exp_points);
void sub_4222A8();
int sub_4223C8(const void* a1, const void* a2);
void sub_42243C(Object* a1, Object* a2);
void sub_422580();
void combatAttemptEnd();
void sub_4227DC();
int sub_4227F4();
void sub_422914();
int sub_42299C(Object* a1, bool a2);
bool sub_422C60();
void sub_422D2C(STRUCT_664980* attack);
void attackInit(Attack* attack, Object* a2, Object* a3, int a4, int a5);
int sub_422F3C(Object* a1, Object* a2, int a3, int a4);
int sub_423104(const Object* a1, const Object* a2);
bool sub_423128(Attack* attack);
int sub_423284(Attack* attack, int a2, int a3, int anim);
int sub_423488(Attack* attack, int accuracy, int* a3, int* a4, int anim);
int attackComputeEnhancedKnockout(Attack* attack);
int attackCompute(Attack* attack);
void sub_423C10(Attack* attack, int a2, int a3, int a4);
int attackComputeCriticalHit(Attack* a1);
int sub_424088(Object* a1, Object* a2);
int attackComputeCriticalFailure(Attack* attack);
int sub_42436C(Object* a1, Object* a2, int hitLocation, int hitMode);
int sub_424380(Object* a1, Object* a2, int a3, int a4, unsigned char* a5);
int sub_424394(Object* a1, int a2, Object* a3, int a4, int a5);
int attackDetermineToHit(Object* attacker, int tile, Object* defender, int hitLocation, int hitMode, int a6);
void attackComputeDamage(Attack* attack, int ammoQuantity, int a3);
void attackComputeDeathFlags(Attack* attack);
void sub_424C04(Attack* attack, bool animated);
void sub_424EE8(Object* a1, int a2, int* a3);
void sub_424F2C(Object* a1, int a2);
void sub_425020(Object* a1, int damage, bool animated, int a4, Object* a5);
void sub_425170(Attack* attack);
void combatCopyDamageAmountDescription(char* dest, Object* critter_obj, int damage);
void combatAddDamageFlagsDescription(char* a1, int flags, Object* a3);
void sub_425E3C();
void sub_425E80();
void sub_425FBC(Object* a1);
void sub_42603C(unsigned char* dest, int dest_pitch, int a3);
char* hitLocationGetName(Object* critter, int hitLocation);
void sub_4261B4(int a1, int a2);
void sub_4261C0(int a1, int a2);
void sub_4261CC(int eventCode, int color);
int calledShotSelectHitLocation(Object* critter, int* hitLocation, int hitMode);
int sub_426614(Object* attacker, Object* defender, int hitMode, bool aiming);
bool sub_426744(Object* target, int* accuracy);
void sub_4267CC(Object* a1);
void sub_426AA8();
void sub_426BC0();
void sub_426C64();
bool sub_426CC4(Object* a1, int from, int to, Object* a4, int* a5);
int sub_426D94();
int sub_426DB8(Object* a1, Object* a2);
void sub_426DDC(Object* obj);
void sub_426EC4(Object* critter_obj, char* msg);

static inline bool isInCombat()
{
    return (gCombatState & COMBAT_STATE_0x01) != 0;
}

#endif /* COMBAT_H */
