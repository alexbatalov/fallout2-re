#ifndef ITEM_H
#define ITEM_H

#include "db.h"
#include "message.h"
#include "obj_types.h"

#include <stdbool.h>

#define ADDICTION_COUNT (9)

typedef enum _WeaponClass {
    ATTACK_TYPE_NONE,
    ATTACK_TYPE_UNARMED, // unarmed
    ATTACK_TYPE_MELEE, // melee
    ATTACK_TYPE_THROW,
    ATTACK_TYPE_RANGED,
    ATTACK_TYPE_COUNT,
} WeaponClass;

typedef struct DrugDescription {
    int drugPid;
    int gvar;
    int field_8;
} DrugDescription;

extern char byte_509FFC[];

extern const int dword_519160[9];
extern const int dword_519184[9];
extern const int dword_5191A8[9];
extern DrugDescription gDrugDescriptions[ADDICTION_COUNT];
extern char* off_519238;

extern MessageList gItemsMessageList;
extern int dword_59E988;
extern Object* off_59E98C;
extern int dword_59E990;

int itemsInit();
void itemsReset();
void itemsExit();
int sub_477154(File* stream);
int itemsLoad(File* stream);
int itemsSave(File* stream);
int itemAttemptAdd(Object* owner, Object* itemToAdd, int quantity);
int itemAdd(Object* owner, Object* itemToAdd, int quantity);
int itemRemove(Object* a1, Object* a2, int quantity);
void sub_4775D8(int inventoryItemIndex, Inventory* inventory);
int sub_477608(Object* a1, Object* a2, Object* a3, int quantity, bool a5);
int sub_47769C(Object* a1, Object* a2, Object* a3, int quantity);
int sub_4776A4(Object* a1, Object* a2, Object* a3, int quantity);
void sub_4776AC(Object* a1, Object* a2);
int sub_4776E0(Object* a1, Object* a2);
int sub_477770(Object* a1);
int sub_477804(Object* critter, int tile);
bool sub_4779F0(Object* a1, Object* a2);
char* itemGetName(Object* obj);
char* itemGetDescription(Object* obj);
int itemGetType(Object* item);
int itemGetMaterial(Object* item);
int itemGetSize(Object* obj);
int itemGetWeight(Object* item);
int itemGetCost(Object* obj);
int objectGetCost(Object* obj);
int objectGetInventoryWeight(Object* obj);
bool sub_477F3C(Object* item_obj);
int itemGetInventoryFid(Object* obj);
Object* critterGetWeaponForHitMode(Object* critter, int hitMode);
int sub_478040(Object* obj, int hitMode, bool aiming);
int sub_47808C(Object* obj, Object* a2);
int sub_4780E4(Object* obj);
Object* sub_478154(Object* a1, Object* a2, int a3);
int weaponIsNatural(Object* obj);
int weaponGetAttackTypeForHitMode(Object* a1, int a2);
int weaponGetSkillForHitMode(Object* a1, int a2);
int sub_478370(Object* a1, int a2);
int weaponGetDamageMinMax(Object* weapon, int* minDamagePtr, int* maxDamagePtr);
int weaponGetMeleeDamage(Object* critter, int hitMode);
int weaponGetDamageType(Object* critter, Object* weapon);
int weaponIsTwoHanded(Object* weapon);
int critterGetAnimationForHitMode(Object* critter, int hitMode);
int weaponGetAnimationForHitMode(Object* weapon, int hitMode);
int ammoGetCapacity(Object* ammoOrWeapon);
int ammoGetQuantity(Object* ammoOrWeapon);
int ammoGetCaliber(Object* ammoOrWeapon);
void ammoSetQuantity(Object* ammoOrWeapon, int quantity);
int sub_478768(Object* critter, Object* weapon);
bool weaponCanBeReloadedWith(Object* weapon, Object* ammo);
int sub_478918(Object* weapon, Object* ammo);
int sub_478A1C(Object* critter, int hitMode);
int sub_478B24(Object* critter, int hitMode, bool aiming);
int weaponGetMinStrengthRequired(Object* weapon);
int weaponGetCriticalFailureType(Object* weapon);
int weaponGetPerk(Object* weapon);
int weaponGetBurstRounds(Object* weapon);
int weaponGetAnimationCode(Object* weapon);
int weaponGetProjectilePid(Object* weapon);
int weaponGetAmmoTypePid(Object* weapon);
char weaponGetSoundId(Object* weapon);
int sub_478E5C(Object* critter, int hitMode);
int sub_478EF4(Object* weapon);
Object* sub_478F80(Object* weapon);
int weaponGetActionPointCost1(Object* weapon);
int weaponGetActionPointCost2(Object* weapon);
int sub_4790AC(Object* obj, int* inout_a2);
bool sub_4790E8(Object* weapon);
int sub_47910C(Object* weapon, int hitMode);
int sub_479180(Object* weapon);
int sub_479188(Object* weapon);
int weaponGetAmmoArmorClassModifier(Object* weapon);
int weaponGetAmmoDamageResistanceModifier(Object* weapon);
int weaponGetAmmoDamageMultiplier(Object* weapon);
int weaponGetAmmoDamageDivisor(Object* weapon);
int armorGetArmorClass(Object* armor);
int armorGetDamageResistance(Object* armor, int damageType);
int armorGetDamageThreshold(Object* armor, int damageType);
int armorGetPerk(Object* armor);
int armorGetMaleFid(Object* armor);
int armorGetFemaleFid(Object* armor);
int miscItemGetMaxCharges(Object* miscItem);
int miscItemGetCharges(Object* miscItem);
int miscItemSetCharges(Object* miscItem, int charges);
int miscItemGetPowerType(Object* miscItem);
int miscItemGetPowerTypePid(Object* miscItem);
bool miscItemIsConsumable(Object* obj);
int sub_4794A4(Object* critter, Object* item);
int miscItemConsumeCharge(Object* miscItem);
int miscItemTrickleEventProcess(Object* item_obj, void* data);
bool miscItemIsOn(Object* obj);
int miscItemTurnOn(Object* item_obj);
int miscItemTurnOff(Object* item_obj);
int sub_479954(Object* obj, void* data);
int stealthBoyTurnOn(Object* object);
int stealthBoyTurnOff(Object* critter, Object* item);
int containerGetMaxSize(Object* container);
int containerGetTotalSize(Object* container);
int ammoGetArmorClassModifier(Object* armor);
int ammoGetDamageResistanceModifier(Object* armor);
int ammoGetDamageMultiplier(Object* armor);
int ammoGetDamageDivisor(Object* armor);
int sub_479B44(Object* critter_obj, Object* item_obj, int a3, int* stats, int* mods);
void sub_479C20(Object* critter_obj, int* stats, int* mods, bool is_immediate);
bool sub_479EE4(Object* critter, int pid);
int sub_479F60(Object* critter_obj, Object* item_obj);
int sub_47A178(Object* obj, void* data);
int drugEffectEventProcess(Object* obj, void* data);
int drugEffectEventRead(File* stream, void** dataPtr);
int drugEffectEventWrite(File* stream, void* data);
int sub_47A290(Object* obj, int a2, int a3, int a4, int a5);
int sub_47A2FC(Object* obj, void* a2);
int sub_47A324(Object* a1, void* data);
int withdrawalEventProcess(Object* obj, void* data);
int withdrawalEventRead(File* stream, void** dataPtr);
int withdrawalEventWrite(File* stream, void* data);
void performWithdrawalStart(Object* obj, int perk, int a3);
void performWithdrawalEnd(Object* obj, int a2);
int drugGetAddictionGvarByPid(int drugPid);
void dudeSetAddiction(int drugPid);
void dudeClearAddiction(int drugPid);
bool dudeIsAddicted(int drugPid);
int itemGetTotalCaps(Object* obj);
int itemCapsAdjust(Object* obj, int amount);
int itemGetMoney(Object* obj);
int itemSetMoney(Object* obj, int a2);

#endif /* ITEM_H */
