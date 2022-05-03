#ifndef PROTOTYPE_INSTANCES_H
#define PROTOTYPE_INSTANCES_H

#include "message.h"
#include "obj_types.h"

#include <stdbool.h>

extern MessageListItem stru_49A990;

int sub_49A9A0(Object* object, int* sidPtr);
int sub_49A9B4(Object* object, int* sidPtr);
int sub_49AAC0(Object* obj, int a2, int a3);
int sub_49AC3C(Object* a1, Object* a2);
int sub_49AC4C(Object* a1, Object* a2, void (*a3)(char* string));
int sub_49AD78(Object* a1, Object* a2);
int sub_49AD88(Object* critter, Object* target, void (*fn)(char* string));
int sub_49B650(Object* critter, Object* item);
int sub_49B73C(Object* critter, Object* item);
int sub_49B8B0(Object* a1, Object* a2);
int sub_49B9A0(Object* obj);
int sub_49B9F0(Object* item_obj);
int sub_49BBA8(Object* critter_obj, Object* item_obj);
int sub_49BC60(Object* item_obj);
int sub_49BCB4(Object* explosive);
int sub_49BDE8(Object* ammo);
int sub_49BE88(Object* item_obj);
int sub_49BF38(Object* a1, Object* a2);
int sub_49BFE8(Object* a1);
int sub_49C124(Object* a1, Object* a2);
int sub_49C240(Object* a1, Object* a2, Object* item);
int sub_49C3CC(Object* a1, Object* a2, Object* item);
int sub_49C5FC(Object* a1, Object* a2, Object* a3);
int sub_49C6BC(Object* obj, Object* a2);
int sub_49C740(Object* a1, Object* a2);
int useLadderDown(Object* a1, Object* ladder, int a3);
int useLadderUp(Object* a1, Object* ladder, int a3);
int useStairs(Object* a1, Object* stairs, int a3);
int sub_49CAF4(Object* a1, Object* a2);
int sub_49CB04(Object* a1, Object* a2);
int sub_49CB14(Object* a1, Object* a2);
int sub_49CCB8(Object* a1, Object* a2, int a3);
int sub_49CE7C(Object* critter, Object* item);
int sub_49D078(Object* a1, Object* a2, int skill);
bool sub_49D178(Object* obj);
bool objectIsLocked(Object* obj);
int objectLock(Object* obj);
int objectUnlock(Object* obj);
bool sub_49D294(Object* obj);
int objectIsOpen(Object* obj);
int objectOpenClose(Object* obj);
int objectOpen(Object* obj);
int objectClose(Object* obj);
bool objectIsJammed(Object* obj);
int objectJamLock(Object* obj);
int objectUnjamLock(Object* obj);
int objectUnjamAll();
int sub_49D4D4(Object* obj, int tile, int elevation, int a4);
int sub_49D628(Object* obj, int tile, int elevation);

#endif /* PROTOTYPE_INSTANCES_H */
