#include "game/queue.h"

#include "game/actions.h"
#include "game/critter.h"
#include "game/display.h"
#include "game/game.h"
#include "game/gsound.h"
#include "game/item.h"
#include "game/map.h"
#include "plib/gnw/memory.h"
#include "game/message.h"
#include "game/object.h"
#include "game/perk.h"
#include "game/proto.h"
#include "game/protinst.h"
#include "game/scripts.h"

typedef struct QueueListNode {
    // TODO: Make unsigned.
    int time;
    int type;
    Object* owner;
    void* data;
    struct QueueListNode* next;
} QueueListNode;

static int queue_destroy(Object* obj, void* data);
static int queue_explode(Object* obj, void* data);
static int queue_explode_exit(Object* obj, void* data);
static int queue_do_explosion(Object* obj, bool a2);
static int queue_premature(Object* obj, void* data);

// Last queue list node found during [queue_find_first] and
// [queue_find_next] calls.
//
// 0x51C690
static QueueListNode* tmpQNode = NULL;

// 0x6648C0
static QueueListNode* queue;

// 0x51C540
EventTypeDescription q_func[EVENT_TYPE_COUNT] = {
    { item_d_process, mem_free, item_d_load, item_d_save, true, item_d_clear },
    { critter_wake_up, NULL, NULL, NULL, true, critter_wake_clear },
    { item_wd_process, mem_free, item_wd_load, item_wd_save, true, item_wd_clear },
    { script_q_process, mem_free, script_q_load, script_q_save, true, NULL },
    { gtime_q_process, NULL, NULL, NULL, true, NULL },
    { critter_check_poison, NULL, NULL, NULL, false, NULL },
    { critter_process_rads, mem_free, critter_load_rads, critter_save_rads, false, NULL },
    { queue_destroy, NULL, NULL, NULL, true, queue_destroy },
    { queue_explode, NULL, NULL, NULL, true, queue_explode_exit },
    { item_m_trickle, NULL, NULL, NULL, true, item_m_turn_off_from_queue },
    { critter_sneak_check, NULL, NULL, NULL, true, critter_sneak_clear },
    { queue_premature, NULL, NULL, NULL, true, queue_explode_exit },
    { scr_map_q_process, NULL, NULL, NULL, true, NULL },
    { gsound_sfx_q_process, mem_free, NULL, NULL, true, NULL },
};

// 0x4A2320
void queue_init()
{
    queue = NULL;
}

// NOTE: Uncollapsed 0x4A2330.
//
// 0x4A2330
int queue_reset()
{
    queue_clear();
    return 0;
}

// NOTE: Uncollapsed 0x4A2330.
//
// 0x4A2330
int queue_exit()
{
    queue_clear();
    return 0;
}

// 0x4A2338
int queue_load(File* stream)
{
    int count;
    if (fileReadInt32(stream, &count) == -1) {
        return -1;
    }

    QueueListNode* oldListHead = queue;
    queue = NULL;

    QueueListNode** nextPtr = &queue;

    int rc = 0;
    for (int index = 0; index < count; index += 1) {
        QueueListNode* queueListNode = (QueueListNode*)mem_malloc(sizeof(*queueListNode));
        if (queueListNode == NULL) {
            rc = -1;
            break;
        }

        if (fileReadInt32(stream, &(queueListNode->time)) == -1) {
            mem_free(queueListNode);
            rc = -1;
            break;
        }

        if (fileReadInt32(stream, &(queueListNode->type)) == -1) {
            mem_free(queueListNode);
            rc = -1;
            break;
        }

        int objectId;
        if (fileReadInt32(stream, &objectId) == -1) {
            mem_free(queueListNode);
            rc = -1;
            break;
        }

        Object* obj;
        if (objectId == -2) {
            obj = NULL;
        } else {
            obj = obj_find_first();
            while (obj != NULL) {
                obj = inven_find_id(obj, objectId);
                if (obj != NULL) {
                    break;
                }
                obj = obj_find_next();
            }
        }

        queueListNode->owner = obj;

        EventTypeDescription* eventTypeDescription = &(q_func[queueListNode->type]);
        if (eventTypeDescription->readProc != NULL) {
            if (eventTypeDescription->readProc(stream, &(queueListNode->data)) == -1) {
                mem_free(queueListNode);
                rc = -1;
                break;
            }
        } else {
            queueListNode->data = NULL;
        }

        queueListNode->next = NULL;

        *nextPtr = queueListNode;
        nextPtr = &(queueListNode->next);
    }

    if (rc == -1) {
        while (queue != NULL) {
            QueueListNode* next = queue->next;

            EventTypeDescription* eventTypeDescription = &(q_func[queue->type]);
            if (eventTypeDescription->freeProc != NULL) {
                eventTypeDescription->freeProc(queue->data);
            }

            mem_free(queue);

            queue = next;
        }
    }

    if (oldListHead != NULL) {
        QueueListNode** v13 = &queue;
        QueueListNode* v15;
        do {
            while (true) {
                QueueListNode* v14 = *v13;
                if (v14 == NULL) {
                    break;
                }

                if (v14->time > oldListHead->time) {
                    break;
                }

                v13 = &(v14->next);
            }
            v15 = oldListHead->next;
            oldListHead->next = *v13;
            *v13 = oldListHead;
            oldListHead = v15;
        } while (v15 != NULL);
    }

    return rc;
}

// 0x4A24E0
int queue_save(File* stream)
{
    QueueListNode* queueListNode;

    int count = 0;

    queueListNode = queue;
    while (queueListNode != NULL) {
        count += 1;
        queueListNode = queueListNode->next;
    }

    if (db_fwriteInt(stream, count) == -1) {
        return -1;
    }

    queueListNode = queue;
    while (queueListNode != NULL) {
        Object* object = queueListNode->owner;
        int objectId = object != NULL ? object->id : -2;

        if (db_fwriteInt(stream, queueListNode->time) == -1) {
            return -1;
        }

        if (db_fwriteInt(stream, queueListNode->type) == -1) {
            return -1;
        }

        if (db_fwriteInt(stream, objectId) == -1) {
            return -1;
        }

        EventTypeDescription* eventTypeDescription = &(q_func[queueListNode->type]);
        if (eventTypeDescription->writeProc != NULL) {
            if (eventTypeDescription->writeProc(stream, queueListNode->data) == -1) {
                return -1;
            }
        }

        queueListNode = queueListNode->next;
    }

    return 0;
}

// 0x4A258C
int queue_add(int delay, Object* obj, void* data, int eventType)
{
    QueueListNode* newQueueListNode = (QueueListNode*)mem_malloc(sizeof(QueueListNode));
    if (newQueueListNode == NULL) {
        return -1;
    }

    int v1 = game_time();
    int v2 = v1 + delay;
    newQueueListNode->time = v2;
    newQueueListNode->type = eventType;
    newQueueListNode->owner = obj;
    newQueueListNode->data = data;

    if (obj != NULL) {
        obj->flags |= OBJECT_USED;
    }

    QueueListNode** v3 = &queue;

    if (queue != NULL) {
        QueueListNode* v4;

        do {
            v4 = *v3;
            if (v2 < v4->time) {
                break;
            }
            v3 = &(v4->next);
        } while (v4->next != NULL);
    }

    newQueueListNode->next = *v3;
    *v3 = newQueueListNode;

    return 0;
}

// 0x4A25F4
int queue_remove(Object* owner)
{
    QueueListNode* queueListNode = queue;
    QueueListNode** queueListNodePtr = &queue;

    while (queueListNode) {
        if (queueListNode->owner == owner) {
            QueueListNode* temp = queueListNode;

            queueListNode = queueListNode->next;
            *queueListNodePtr = queueListNode;

            EventTypeDescription* eventTypeDescription = &(q_func[temp->type]);
            if (eventTypeDescription->freeProc != NULL) {
                eventTypeDescription->freeProc(temp->data);
            }

            mem_free(temp);
        } else {
            queueListNodePtr = &(queueListNode->next);
            queueListNode = queueListNode->next;
        }
    }

    return 0;
}

// 0x4A264C
int queue_remove_this(Object* owner, int eventType)
{
    QueueListNode* queueListNode = queue;
    QueueListNode** queueListNodePtr = &queue;

    while (queueListNode) {
        if (queueListNode->owner == owner && queueListNode->type == eventType) {
            QueueListNode* temp = queueListNode;

            queueListNode = queueListNode->next;
            *queueListNodePtr = queueListNode;

            EventTypeDescription* eventTypeDescription = &(q_func[temp->type]);
            if (eventTypeDescription->freeProc != NULL) {
                eventTypeDescription->freeProc(temp->data);
            }

            mem_free(temp);
        } else {
            queueListNodePtr = &(queueListNode->next);
            queueListNode = queueListNode->next;
        }
    }

    return 0;
}

// Returns true if there is at least one event of given type scheduled.
//
// 0x4A26A8
bool queue_find(Object* owner, int eventType)
{
    QueueListNode* queueListEvent = queue;
    while (queueListEvent != NULL) {
        if (owner == queueListEvent->owner && eventType == queueListEvent->type) {
            return true;
        }

        queueListEvent = queueListEvent->next;
    }

    return false;
}

// 0x4A26D0
int queue_process()
{
    int time = game_time();
    int v1 = 0;

    while (queue != NULL) {
        QueueListNode* queueListNode = queue;
        if (time < queueListNode->time || v1 != 0) {
            break;
        }

        queue = queueListNode->next;

        EventTypeDescription* eventTypeDescription = &(q_func[queueListNode->type]);
        v1 = eventTypeDescription->handlerProc(queueListNode->owner, queueListNode->data);

        if (eventTypeDescription->freeProc != NULL) {
            eventTypeDescription->freeProc(queueListNode->data);
        }

        mem_free(queueListNode);
    }

    return v1;
}

// 0x4A2748
void queue_clear()
{
    QueueListNode* queueListNode = queue;
    while (queueListNode != NULL) {
        QueueListNode* next = queueListNode->next;

        EventTypeDescription* eventTypeDescription = &(q_func[queueListNode->type]);
        if (eventTypeDescription->freeProc != NULL) {
            eventTypeDescription->freeProc(queueListNode->data);
        }

        mem_free(queueListNode);

        queueListNode = next;
    }

    queue = NULL;
}

// 0x4A2790
void queue_clear_type(int eventType, QueueEventHandler* fn)
{
    QueueListNode** ptr = &queue;
    QueueListNode* curr = *ptr;

    while (curr != NULL) {
        if (eventType == curr->type) {
            QueueListNode* tmp = curr;

            *ptr = curr->next;
            curr = *ptr;

            if (fn != NULL && fn(tmp->owner, tmp->data) != 1) {
                *ptr = tmp;
                ptr = &(tmp->next);
            } else {
                EventTypeDescription* eventTypeDescription = &(q_func[tmp->type]);
                if (eventTypeDescription->freeProc != NULL) {
                    eventTypeDescription->freeProc(tmp->data);
                }

                mem_free(tmp);
            }
        } else {
            ptr = &(curr->next);
            curr = *ptr;
        }
    }
}

// TODO: Make unsigned.
//
// 0x4A2808
int queue_next_time()
{
    if (queue == NULL) {
        return 0;
    }

    return queue->time;
}

// 0x4A281C
static int queue_destroy(Object* obj, void* data)
{
    obj_destroy(obj);
    return 1;
}

// 0x4A2828
static int queue_explode(Object* obj, void* data)
{
    return queue_do_explosion(obj, true);
}

// 0x4A2830
static int queue_explode_exit(Object* obj, void* data)
{
    return queue_do_explosion(obj, false);
}

// 0x4A2834
static int queue_do_explosion(Object* explosive, bool a2)
{
    int tile;
    int elevation;

    Object* owner = obj_top_environment(explosive);
    if (owner) {
        tile = owner->tile;
        elevation = owner->elevation;
    } else {
        tile = explosive->tile;
        elevation = explosive->elevation;
    }

    int maxDamage;
    int minDamage;
    if (explosive->pid == PROTO_ID_DYNAMITE_I || explosive->pid == PROTO_ID_DYNAMITE_II) {
        // Dynamite
        minDamage = 30;
        maxDamage = 50;
    } else {
        // Plastic explosive
        minDamage = 40;
        maxDamage = 80;
    }

    // FIXME: I guess this is a little bit wrong, dude can never be null, I
    // guess it needs to check if owner is dude.
    if (obj_dude != NULL) {
        if (perkHasRank(obj_dude, PERK_DEMOLITION_EXPERT)) {
            maxDamage += 10;
            minDamage += 10;
        }
    }

    if (action_explode(tile, elevation, minDamage, maxDamage, obj_dude, a2) == -2) {
        queue_add(50, explosive, NULL, EVENT_TYPE_EXPLOSION);
    } else {
        obj_destroy(explosive);
    }

    return 1;
}

// 0x4A28E4
static int queue_premature(Object* obj, void* data)
{
    MessageListItem msg;

    // Due to your inept handling, the explosive detonates prematurely.
    msg.num = 4000;
    if (message_search(&misc_message_file, &msg)) {
        display_print(msg.text);
    }

    return queue_do_explosion(obj, true);
}

// 0x4A2920
void queue_leaving_map()
{
    for (int eventType = 0; eventType < EVENT_TYPE_COUNT; eventType++) {
        EventTypeDescription* eventTypeDescription = &(q_func[eventType]);
        if (eventTypeDescription->field_10) {
            queue_clear_type(eventType, eventTypeDescription->field_14);
        }
    }
}

// 0x4A294C
bool queue_is_empty()
{
    return queue == NULL;
}

// 0x4A295C
void* queue_find_first(Object* owner, int eventType)
{
    QueueListNode* queueListNode = queue;
    while (queueListNode != NULL) {
        if (owner == queueListNode->owner && eventType == queueListNode->type) {
            tmpQNode = queueListNode;
            return queueListNode->data;
        }
        queueListNode = queueListNode->next;
    }

    tmpQNode = NULL;
    return NULL;
}

// 0x4A2994
void* queue_find_next(Object* owner, int eventType)
{
    if (tmpQNode != NULL) {
        QueueListNode* queueListNode = tmpQNode->next;
        while (queueListNode != NULL) {
            if (owner == queueListNode->owner && eventType == queueListNode->type) {
                tmpQNode = queueListNode;
                return queueListNode->data;
            }
            queueListNode = queueListNode->next;
        }
    }

    tmpQNode = NULL;

    return NULL;
}
