#ifndef FALLOUT_GAME_QUEUE_H_
#define FALLOUT_GAME_QUEUE_H_

#include <stdbool.h>

#include "plib/db/db.h"
#include "game/object_types.h"

typedef enum EventType {
    EVENT_TYPE_DRUG = 0,
    EVENT_TYPE_KNOCKOUT = 1,
    EVENT_TYPE_WITHDRAWAL = 2,
    EVENT_TYPE_SCRIPT = 3,
    EVENT_TYPE_GAME_TIME = 4,
    EVENT_TYPE_POISON = 5,
    EVENT_TYPE_RADIATION = 6,
    EVENT_TYPE_FLARE = 7,
    EVENT_TYPE_EXPLOSION = 8,
    EVENT_TYPE_ITEM_TRICKLE = 9,
    EVENT_TYPE_SNEAK = 10,
    EVENT_TYPE_EXPLOSION_FAILURE = 11,
    EVENT_TYPE_MAP_UPDATE_EVENT = 12,
    EVENT_TYPE_GSOUND_SFX_EVENT = 13,
    EVENT_TYPE_COUNT,
} EventType;

typedef struct DrugEffectEvent {
    int drugPid;
    int stats[3];
    int modifiers[3];
} DrugEffectEvent;

typedef struct WithdrawalEvent {
    int field_0;
    int pid;
    int perk;
} WithdrawalEvent;

typedef struct ScriptEvent {
    int sid;
    int fixedParam;
} ScriptEvent;

typedef struct RadiationEvent {
    int radiationLevel;
    int isHealing;
} RadiationEvent;

typedef struct AmbientSoundEffectEvent {
    int ambientSoundEffectIndex;
} AmbientSoundEffectEvent;

typedef int QueueEventHandler(Object* owner, void* data);
typedef void QueueEventDataFreeProc(void* data);
typedef int QueueEventDataReadProc(File* stream, void** dataPtr);
typedef int QueueEventDataWriteProc(File* stream, void* data);

typedef struct EventTypeDescription {
    QueueEventHandler* handlerProc;
    QueueEventDataFreeProc* freeProc;
    QueueEventDataReadProc* readProc;
    QueueEventDataWriteProc* writeProc;
    bool field_10;
    QueueEventHandler* field_14;
} EventTypeDescription;

extern EventTypeDescription q_func[EVENT_TYPE_COUNT];

void queue_init();
int queue_reset();
int queue_exit();
int queue_load(File* stream);
int queue_save(File* stream);
int queue_add(int delay, Object* owner, void* data, int eventType);
int queue_remove(Object* owner);
int queue_remove_this(Object* owner, int eventType);
bool queue_find(Object* owner, int eventType);
int queue_process();
void queue_clear();
void queue_clear_type(int eventType, QueueEventHandler* fn);
int queue_next_time();
void queue_leaving_map();
bool queue_is_empty();
void* queue_find_first(Object* owner, int eventType);
void* queue_find_next(Object* owner, int eventType);

#endif /* FALLOUT_GAME_QUEUE_H_ */
