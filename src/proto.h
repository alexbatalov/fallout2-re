#ifndef PROTO_H
#define PROTO_H

#include "db.h"
#include "message.h"
#include "obj_types.h"
#include "perk_defs.h"
#include "proto_types.h"
#include "skill_defs.h"
#include "stat_defs.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef enum ItemDataMember {
    ITEM_DATA_MEMBER_PID = 0,
    ITEM_DATA_MEMBER_NAME = 1,
    ITEM_DATA_MEMBER_DESCRIPTION = 2,
    ITEM_DATA_MEMBER_FID = 3,
    ITEM_DATA_MEMBER_LIGHT_DISTANCE = 4,
    ITEM_DATA_MEMBER_LIGHT_INTENSITY = 5,
    ITEM_DATA_MEMBER_FLAGS = 6,
    ITEM_DATA_MEMBER_EXTENDED_FLAGS = 7,
    ITEM_DATA_MEMBER_SID = 8,
    ITEM_DATA_MEMBER_TYPE = 9,
    ITEM_DATA_MEMBER_MATERIAL = 11,
    ITEM_DATA_MEMBER_SIZE = 12,
    ITEM_DATA_MEMBER_WEIGHT = 13,
    ITEM_DATA_MEMBER_COST = 14,
    ITEM_DATA_MEMBER_INVENTORY_FID = 15,
    ITEM_DATA_MEMBER_WEAPON_RANGE = 555,
} ItemDataMember;

typedef enum CritterDataMember {
    CRITTER_DATA_MEMBER_PID = 0,
    CRITTER_DATA_MEMBER_NAME = 1,
    CRITTER_DATA_MEMBER_DESCRIPTION = 2,
    CRITTER_DATA_MEMBER_FID = 3,
    CRITTER_DATA_MEMBER_LIGHT_DISTANCE = 4,
    CRITTER_DATA_MEMBER_LIGHT_INTENSITY = 5,
    CRITTER_DATA_MEMBER_FLAGS = 6,
    CRITTER_DATA_MEMBER_EXTENDED_FLAGS = 7,
    CRITTER_DATA_MEMBER_SID = 8,
    CRITTER_DATA_MEMBER_DATA = 9,
    CRITTER_DATA_MEMBER_HEAD_FID = 10,
    CRITTER_DATA_MEMBER_BODY_TYPE = 11,
} CritterDataMember;

typedef enum SceneryDataMember {
    SCENERY_DATA_MEMBER_PID = 0,
    SCENERY_DATA_MEMBER_NAME = 1,
    SCENERY_DATA_MEMBER_DESCRIPTION = 2,
    SCENERY_DATA_MEMBER_FID = 3,
    SCENERY_DATA_MEMBER_LIGHT_DISTANCE = 4,
    SCENERY_DATA_MEMBER_LIGHT_INTENSITY = 5,
    SCENERY_DATA_MEMBER_FLAGS = 6,
    SCENERY_DATA_MEMBER_EXTENDED_FLAGS = 7,
    SCENERY_DATA_MEMBER_SID = 8,
    SCENERY_DATA_MEMBER_TYPE = 9,
    SCENERY_DATA_MEMBER_DATA = 10,
    SCENERY_DATA_MEMBER_MATERIAL = 11,
} SceneryDataMember;

typedef enum WallDataMember {
    WALL_DATA_MEMBER_PID = 0,
    WALL_DATA_MEMBER_NAME = 1,
    WALL_DATA_MEMBER_DESCRIPTION = 2,
    WALL_DATA_MEMBER_FID = 3,
    WALL_DATA_MEMBER_LIGHT_DISTANCE = 4,
    WALL_DATA_MEMBER_LIGHT_INTENSITY = 5,
    WALL_DATA_MEMBER_FLAGS = 6,
    WALL_DATA_MEMBER_EXTENDED_FLAGS = 7,
    WALL_DATA_MEMBER_SID = 8,
    WALL_DATA_MEMBER_MATERIAL = 9,
} WallDataMember;

typedef enum MiscDataMember {
    MISC_DATA_MEMBER_PID = 0,
    MISC_DATA_MEMBER_NAME = 1,
    MISC_DATA_MEMBER_DESCRIPTION = 2,
    MISC_DATA_MEMBER_FID = 3,
    MISC_DATA_MEMBER_LIGHT_DISTANCE = 4,
    MISC_DATA_MEMBER_LIGHT_INTENSITY = 5,
    MISC_DATA_MEMBER_FLAGS = 6,
    MISC_DATA_MEMBER_EXTENDED_FLAGS = 7,
} MiscDataMember;

typedef enum ProtoDataMemberType {
    PROTO_DATA_MEMBER_TYPE_INT = 1,
    PROTO_DATA_MEMBER_TYPE_STRING = 2,
} ProtoDataMemberType;

typedef enum PrototypeMessage {
    PROTOTYPE_MESSAGE_NAME,
    PROTOTYPE_MESSAGE_DESCRIPTION,
} PrototypeMesage;

extern char byte_50CF3C[];

extern char byte_51C18C[MAX_PATH];
extern ProtoList stru_51C290[11];
extern const size_t dword_51C340[11];
extern int dword_51C36C;
extern CritterProto gDudeProto;
extern char* off_51C534;
extern int dword_51C538;
extern int dword_51C53C;

extern char* off_66452C;
extern char* off_664530[PERK_COUNT];
extern char* off_66470C;
extern char* off_664710;
extern char* off_664714[STAT_COUNT];
extern MessageList stru_6647AC[6];
extern char* gRaceTypeNames[2];
extern char* gSceneryTypeNames[6];
extern MessageList gProtoMessageList;
extern char* gMaterialTypeNames[8];
extern char* off_664824;
extern char* gBodyTypeNames[3];
extern char* gItemTypeNames[7];
extern char* gDamageTypeNames[7];
extern char* gCaliberTypeNames[19];

extern char** off_6648B8;
extern char** off_6648BC;

int sub_49E758(int pid, char* proto_path);
bool sub_49E99C(int pid);
bool sub_49E9DC(int pid);
bool sub_49EA24(int pid);
int sub_49EA5C(int pid);
char* protoGetMessage(int pid, int message);
char* protoGetName(int pid);
char* protoGetDescription(int pid);
int sub_49EDB4(Proto* a1, int a2);
void objectDataReset(Object* obj);
int objectCritterCombatDataRead(CritterCombatData* data, File* stream);
int objectCritterCombatDataWrite(CritterCombatData* data, File* stream);
int objectDataRead(Object* obj, File* stream);
int objectDataWrite(Object* obj, File* stream);
int sub_49F73C(Object* obj);
int sub_49F8A0(Object* obj);
int sub_49F984();
int sub_49FA64(const char* path);
int sub_49FFD8(int pid, int member, int* value);
int protoInit();
void protoReset();
void protoExit();
int sub_4A08E0();
int protoItemDataRead(ItemProtoData* item_data, int type, File* stream);
int protoSceneryDataRead(SceneryProtoData* scenery_data, int type, File* stream);
int protoRead(Proto* buf, File* stream);
int protoItemDataWrite(ItemProtoData* item_data, int type, File* stream);
int protoSceneryDataWrite(SceneryProtoData* scenery_data, int type, File* stream);
int protoWrite(Proto* buf, File* stream);
int sub_4A1B30(int pid);
int sub_4A1C3C(int pid, Proto** out_proto);
int sub_4A1D98(int type, Proto** out_ptr);
void sub_4A2040(int type);
void sub_4A2094(int type);
void sub_4A20F4();
int protoGetProto(int pid, Proto** out_proto);
int sub_4A21DC(int a1);
int sub_4A2214(int a1);
int sub_4A22C0();

#endif /* PROTO_H */