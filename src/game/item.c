#include "game/item.h"

#include <string.h>

#include "game/anim.h"
#include "game/automap.h"
#include "game/combat.h"
#include "game/critter.h"
#include "debug.h"
#include "game/display.h"
#include "game/game.h"
#include "game/intface.h"
#include "game/inventry.h"
#include "game/light.h"
#include "game/map.h"
#include "memory.h"
#include "game/message.h"
#include "game/object.h"
#include "game/perk.h"
#include "game/proto.h"
#include "game/protinst.h"
#include "game/queue.h"
#include "game/roll.h"
#include "game/skill.h"
#include "game/stat.h"
#include "tile.h"
#include "trait.h"

static void item_compact(int inventoryItemIndex, Inventory* inventory);
static int item_move_func(Object* a1, Object* a2, Object* a3, int quantity, bool a5);
static bool item_identical(Object* a1, Object* a2);
static int item_m_stealth_effect_on(Object* object);
static int item_m_stealth_effect_off(Object* critter, Object* item);
static int insert_drug_effect(Object* critter_obj, Object* item_obj, int a3, int* stats, int* mods);
static void perform_drug_effect(Object* critter_obj, int* stats, int* mods, bool is_immediate);
static bool drug_effect_allowed(Object* critter, int pid);
static int insert_withdrawal(Object* obj, int a2, int a3, int a4, int a5);
static int item_wd_clear_all(Object* a1, void* data);
static void perform_withdrawal_start(Object* obj, int perk, int a3);
static void perform_withdrawal_end(Object* obj, int a2);
static int pid_to_gvar(int drugPid);

// TODO: Remove.
// 0x509FFC
char _aItem_1[] = "<item>";

// Maps weapon extended flags to skill.
//
// 0x519160
static int attack_skill[9] = {
    -1,
    SKILL_UNARMED,
    SKILL_UNARMED,
    SKILL_MELEE_WEAPONS,
    SKILL_MELEE_WEAPONS,
    SKILL_THROWING,
    SKILL_SMALL_GUNS,
    SKILL_SMALL_GUNS,
    SKILL_SMALL_GUNS,
};

// A map of item's extendedFlags to animation.
//
// 0x519184
static int attack_anim[9] = {
    ANIM_STAND,
    ANIM_THROW_PUNCH,
    ANIM_KICK_LEG,
    ANIM_SWING_ANIM,
    ANIM_THRUST_ANIM,
    ANIM_THROW_ANIM,
    ANIM_FIRE_SINGLE,
    ANIM_FIRE_BURST,
    ANIM_FIRE_CONTINUOUS,
};

// Maps weapon extended flags to weapon class
//
// 0x5191A8
static int attack_subtype[9] = {
    ATTACK_TYPE_NONE, // 0 // None
    ATTACK_TYPE_UNARMED, // 1 // Punch // Brass Knuckles, Power First
    ATTACK_TYPE_UNARMED, // 2 // Kick?
    ATTACK_TYPE_MELEE, // 3 // Swing //  Sledgehammer (prim), Club, Knife (prim), Spear (prim), Crowbar
    ATTACK_TYPE_MELEE, // 4 // Thrust // Sledgehammer (sec), Knife (sec), Spear (sec)
    ATTACK_TYPE_THROW, // 5 // Throw // Rock,
    ATTACK_TYPE_RANGED, // 6 // Single // 10mm SMG (prim), Rocket Launcher, Hunting Rifle, Plasma Rifle, Laser Pistol
    ATTACK_TYPE_RANGED, // 7 // Burst // 10mm SMG (sec), Minigun
    ATTACK_TYPE_RANGED, // 8 // Continous // Only: Flamer, Improved Flamer, Flame Breath
};

// 0x5191CC
DrugDescription drugInfoList[ADDICTION_COUNT] = {
    { PROTO_ID_NUKA_COLA, GVAR_NUKA_COLA_ADDICT, 0 },
    { PROTO_ID_BUFF_OUT, GVAR_BUFF_OUT_ADDICT, 4 },
    { PROTO_ID_MENTATS, GVAR_MENTATS_ADDICT, 4 },
    { PROTO_ID_PSYCHO, GVAR_PSYCHO_ADDICT, 4 },
    { PROTO_ID_RADAWAY, GVAR_RADAWAY_ADDICT, 0 },
    { PROTO_ID_BEER, GVAR_ALCOHOL_ADDICT, 0 },
    { PROTO_ID_BOOZE, GVAR_ALCOHOL_ADDICT, 0 },
    { PROTO_ID_JET, GVAR_ADDICT_JET, 4 },
    { PROTO_ID_DECK_OF_TRAGIC_CARDS, GVAR_ADDICT_TRAGIC, 0 },
};

// item.msg
//
// 0x59E980
static MessageList item_message_file;

// 0x59E988
static int wd_onset;

// 0x59E98C
static Object* wd_obj;

// 0x59E990
static int wd_gvar;

// 0x4770E0
int item_init()
{
    if (!message_init(&item_message_file)) {
        return -1;
    }

    char path[MAX_PATH];
    sprintf(path, "%s%s", msg_path, "item.msg");

    if (!message_load(&item_message_file, path)) {
        return -1;
    }

    return 0;
}

// 0x477144
void item_reset()
{
    return;
}

// 0x477148
void item_exit()
{
    message_exit(&item_message_file);
}

// NOTE: Uncollapsed 0x477154.
int item_load(File* stream)
{
    return 0;
}

// NOTE: Uncollapsed 0x477154.
int item_save(File* stream)
{
    return 0;
}

// 0x477158
int item_add_mult(Object* owner, Object* itemToAdd, int quantity)
{
    if (quantity < 1) {
        return -1;
    }

    int parentType = FID_TYPE(owner->fid);
    if (parentType == OBJ_TYPE_ITEM) {
        int itemType = item_get_type(owner);
        if (itemType == ITEM_TYPE_CONTAINER) {
            // NOTE: Uninline.
            int sizeToAdd = item_size(itemToAdd);
            sizeToAdd *= quantity;

            int currentSize = item_c_curr_size(owner);
            int maxSize = item_c_max_size(owner);
            if (currentSize + sizeToAdd >= maxSize) {
                return -6;
            }

            Object* containerOwner = obj_top_environment(owner);
            if (containerOwner != NULL) {
                if (FID_TYPE(containerOwner->fid) == OBJ_TYPE_CRITTER) {
                    int weightToAdd = item_weight(itemToAdd);
                    weightToAdd *= quantity;

                    int currentWeight = item_total_weight(containerOwner);
                    int maxWeight = critterGetStat(containerOwner, STAT_CARRY_WEIGHT);
                    if (currentWeight + weightToAdd > maxWeight) {
                        return -6;
                    }
                }
            }
        } else if (itemType == ITEM_TYPE_MISC) {
            // NOTE: Uninline.
            int powerTypePid = item_m_cell_pid(owner);
            if (powerTypePid != itemToAdd->pid) {
                return -1;
            }
        } else {
            return -1;
        }
    } else if (parentType == OBJ_TYPE_CRITTER) {
        if (critter_body_type(owner) != BODY_TYPE_BIPED) {
            return -5;
        }

        int weightToAdd = item_weight(itemToAdd);
        weightToAdd *= quantity;

        int currentWeight = item_total_weight(owner);
        int maxWeight = critterGetStat(owner, STAT_CARRY_WEIGHT);
        if (currentWeight + weightToAdd > maxWeight) {
            return -6;
        }
    }

    return item_add_force(owner, itemToAdd, quantity);
}

// item_add
// 0x4772B8
int item_add_force(Object* owner, Object* itemToAdd, int quantity)
{
    if (quantity < 1) {
        return -1;
    }

    Inventory* inventory = &(owner->data.inventory);

    int index;
    for (index = 0; index < inventory->length; index++) {
        if (item_identical(inventory->items[index].item, itemToAdd) != 0) {
            break;
        }
    }

    if (index == inventory->length) {
        if (inventory->length == inventory->capacity || inventory->items == NULL) {
            InventoryItem* inventoryItems = (InventoryItem*)internal_realloc(inventory->items, sizeof(InventoryItem) * (inventory->capacity + 10));
            if (inventoryItems == NULL) {
                return -1;
            }

            inventory->items = inventoryItems;
            inventory->capacity += 10;
        }

        inventory->items[inventory->length].item = itemToAdd;
        inventory->items[inventory->length].quantity = quantity;

        if (itemToAdd->pid == PROTO_ID_STEALTH_BOY_II) {
            if ((itemToAdd->flags & OBJECT_IN_ANY_HAND) != 0) {
                // NOTE: Uninline.
                item_m_stealth_effect_on(owner);
            }
        }

        inventory->length++;
        itemToAdd->owner = owner;

        return 0;
    }

    if (itemToAdd == inventory->items[index].item) {
        debugPrint("Warning! Attempt to add same item twice in item_add()\n");
        return 0;
    }

    if (item_get_type(itemToAdd) == ITEM_TYPE_AMMO) {
        // NOTE: Uninline.
        int ammoQuantityToAdd = item_w_curr_ammo(itemToAdd);

        int ammoQuantity = item_w_curr_ammo(inventory->items[index].item);

        // NOTE: Uninline.
        int capacity = item_w_max_ammo(itemToAdd);

        ammoQuantity += ammoQuantityToAdd;
        if (ammoQuantity > capacity) {
            item_w_set_curr_ammo(itemToAdd, ammoQuantity - capacity);
            inventory->items[index].quantity++;
        } else {
            item_w_set_curr_ammo(itemToAdd, ammoQuantity);
        }

        inventory->items[index].quantity += quantity - 1;
    } else {
        inventory->items[index].quantity += quantity;
    }

    obj_erase_object(inventory->items[index].item, NULL);
    inventory->items[index].item = itemToAdd;
    itemToAdd->owner = owner;

    return 0;
}

// 0x477490
int item_remove_mult(Object* owner, Object* itemToRemove, int quantity)
{
    Inventory* inventory = &(owner->data.inventory);
    Object* item1 = inven_left_hand(owner);
    Object* item2 = inven_right_hand(owner);

    int index = 0;
    for (; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        if (inventoryItem->item == itemToRemove) {
            break;
        }

        if (item_get_type(inventoryItem->item) == ITEM_TYPE_CONTAINER) {
            if (item_remove_mult(inventoryItem->item, itemToRemove, quantity) == 0) {
                return 0;
            }
        }
    }

    if (index == inventory->length) {
        return -1;
    }

    InventoryItem* inventoryItem = &(inventory->items[index]);
    if (inventoryItem->quantity < quantity) {
        return -1;
    }

    if (inventoryItem->quantity == quantity) {
        // NOTE: Uninline.
        item_compact(index, inventory);
    } else {
        // TODO: Not sure about this line.
        if (obj_copy(&(inventoryItem->item), itemToRemove) == -1) {
            return -1;
        }

        obj_disconnect(inventoryItem->item, NULL);

        inventoryItem->quantity -= quantity;

        if (item_get_type(itemToRemove) == ITEM_TYPE_AMMO) {
            int capacity = item_w_max_ammo(itemToRemove);
            item_w_set_curr_ammo(inventoryItem->item, capacity);
        }
    }

    if (itemToRemove->pid == PROTO_ID_STEALTH_BOY_I || itemToRemove->pid == PROTO_ID_STEALTH_BOY_II) {
        if (itemToRemove == item1 || itemToRemove == item2) {
            Object* owner = obj_top_environment(itemToRemove);
            if (owner != NULL) {
                item_m_stealth_effect_off(owner, itemToRemove);
            }
        }
    }

    itemToRemove->owner = NULL;
    itemToRemove->flags &= ~OBJECT_EQUIPPED;

    return 0;
}

// NOTE: Inlined.
//
// 0x4775D8
static void item_compact(int inventoryItemIndex, Inventory* inventory)
{
    for (int index = inventoryItemIndex + 1; index < inventory->length; index++) {
        InventoryItem* prev = &(inventory->items[index - 1]);
        InventoryItem* curr = &(inventory->items[index]);
        static_assert(sizeof(*prev) == sizeof(*curr), "wrong size");
        memcpy(prev, curr, sizeof(*prev));
    }
    inventory->length--;
}

// 0x477608
static int item_move_func(Object* a1, Object* a2, Object* a3, int quantity, bool a5)
{
    if (item_remove_mult(a1, a3, quantity) == -1) {
        return -1;
    }

    int rc;
    if (a5) {
        rc = item_add_force(a2, a3, quantity);
    } else {
        rc = item_add_mult(a2, a3, quantity);
    }

    if (rc != 0) {
        if (item_add_force(a1, a3, quantity) != 0) {
            Object* owner = obj_top_environment(a1);
            if (owner == NULL) {
                owner = a1;
            }

            if (owner->tile != -1) {
                Rect updatedRect;
                obj_connect(a3, owner->tile, owner->elevation, &updatedRect);
                tileWindowRefreshRect(&updatedRect, map_elevation);
            }
        }
        return -1;
    }

    a3->owner = a2;

    return 0;
}

// 0x47769C
int item_move(Object* a1, Object* a2, Object* a3, int quantity)
{
    return item_move_func(a1, a2, a3, quantity, false);
}

// 0x4776A4
int item_move_force(Object* a1, Object* a2, Object* a3, int quantity)
{
    return item_move_func(a1, a2, a3, quantity, true);
}

// 0x4776AC
void item_move_all(Object* a1, Object* a2)
{
    Inventory* inventory = &(a1->data.inventory);
    while (inventory->length > 0) {
        InventoryItem* inventoryItem = &(inventory->items[0]);
        item_move_func(a1, a2, inventoryItem->item, inventoryItem->quantity, true);
    }
}

// 0x4776E0
int item_move_all_hidden(Object* a1, Object* a2)
{
    Inventory* inventory = &(a1->data.inventory);
    // TODO: Not sure about two loops.
    for (int i = 0; i < inventory->length;) {
        for (int j = i; j < inventory->length;) {
            bool v5;
            InventoryItem* inventoryItem = &(inventory->items[j]);
            if (PID_TYPE(inventoryItem->item->pid) == OBJ_TYPE_ITEM) {
                Proto* proto;
                if (proto_ptr(inventoryItem->item->pid, &proto) != -1) {
                    v5 = (proto->item.extendedFlags & ItemProtoExtendedFlags_NaturalWeapon) == 0;
                } else {
                    v5 = true;
                }
            } else {
                v5 = true;
            }

            if (!v5) {
                item_move_func(a1, a2, inventoryItem->item, inventoryItem->quantity, true);
            } else {
                i++;
                j++;
            }
        }
    }
    return 0;
}

// 0x477770
int item_destroy_all_hidden(Object* a1)
{
    Inventory* inventory = &(a1->data.inventory);
    // TODO: Not sure about this one. Why two loops?
    for (int i = 0; i < inventory->length;) {
        // TODO: Probably wrong, something with two loops.
        for (int j = i; j < inventory->length;) {
            bool v5;
            InventoryItem* inventoryItem = &(inventory->items[j]);
            if (PID_TYPE(inventoryItem->item->pid) == OBJ_TYPE_ITEM) {
                Proto* proto;
                if (proto_ptr(inventoryItem->item->pid, &proto) != -1) {
                    v5 = (proto->item.extendedFlags & ItemProtoExtendedFlags_NaturalWeapon) == 0;
                } else {
                    v5 = true;
                }
            } else {
                v5 = true;
            }

            if (!v5) {
                item_remove_mult(a1, inventoryItem->item, 1);
                obj_destroy(inventoryItem->item);
            } else {
                i++;
                j++;
            }
        }
    }
    return 0;
}

// 0x477804
int item_drop_all(Object* critter, int tile)
{
    bool hasEquippedItems = false;

    int frmId = critter->fid & 0xFFF;

    Inventory* inventory = &(critter->data.inventory);
    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        Object* item = inventoryItem->item;
        if (item->pid == PROTO_ID_MONEY) {
            if (item_remove_mult(critter, item, inventoryItem->quantity) != 0) {
                return -1;
            }

            if (obj_connect(item, tile, critter->elevation, NULL) != 0) {
                if (item_add_force(critter, item, 1) != 0) {
                    obj_destroy(item);
                }
                return -1;
            }

            item->data.item.misc.charges = inventoryItem->quantity;
        } else {
            if ((item->flags & OBJECT_EQUIPPED) != 0) {
                hasEquippedItems = true;

                if ((item->flags & OBJECT_WORN) != 0) {
                    Proto* proto;
                    if (proto_ptr(critter->pid, &proto) == -1) {
                        return -1;
                    }

                    frmId = proto->fid & 0xFFF;
                    adjust_ac(critter, item, NULL);
                }
            }

            for (int index = 0; index < inventoryItem->quantity; index++) {
                if (item_remove_mult(critter, item, 1) != 0) {
                    return -1;
                }

                if (obj_connect(item, tile, critter->elevation, NULL) != 0) {
                    if (item_add_force(critter, item, 1) != 0) {
                        obj_destroy(item);
                    }
                    return -1;
                }
            }
        }
    }

    if (hasEquippedItems) {
        Rect updatedRect;
        int fid = art_id(OBJ_TYPE_CRITTER, frmId, FID_ANIM_TYPE(critter->fid), 0, (critter->fid & 0x70000000) >> 28);
        obj_change_fid(critter, fid, &updatedRect);
        if (FID_ANIM_TYPE(critter->fid) == ANIM_STAND) {
            tileWindowRefreshRect(&updatedRect, map_elevation);
        }
    }

    return -1;
}

// 0x4779F0
static bool item_identical(Object* a1, Object* a2)
{
    if (a1->pid != a2->pid) {
        return false;
    }

    if (a1->sid != a2->sid) {
        return false;
    }

    if ((a1->flags & (OBJECT_EQUIPPED | OBJECT_USED)) != 0) {
        return false;
    }

    if ((a2->flags & (OBJECT_EQUIPPED | OBJECT_USED)) != 0) {
        return false;
    }

    Proto* proto;
    proto_ptr(a1->pid, &proto);
    if (proto->item.type == ITEM_TYPE_CONTAINER) {
        return false;
    }

    Inventory* inventory1 = &(a1->data.inventory);
    Inventory* inventory2 = &(a2->data.inventory);
    if (inventory1->length != 0 || inventory2->length != 0) {
        return false;
    }

    int v1;
    if (proto->item.type == ITEM_TYPE_AMMO || a1->pid == PROTO_ID_MONEY) {
        v1 = a2->data.item.ammo.quantity;
        a2->data.item.ammo.quantity = a1->data.item.ammo.quantity;
    }

    // NOTE: Probably inlined memcmp, but I'm not sure why it only checks 32
    // bytes.
    int i;
    for (i = 0; i < 8; i++) {
        if (a1->field_2C_array[i] != a2->field_2C_array[i]) {
            break;
        }
    }

    if (proto->item.type == ITEM_TYPE_AMMO || a1->pid == PROTO_ID_MONEY) {
        a2->data.item.ammo.quantity = v1;
    }

    return i == 8;
}

// 0x477AE4
char* item_name(Object* obj)
{
    // 0x519238
    static char* name = _aItem_1;

    name = proto_name(obj->pid);
    return name;
}

// 0x477AF4
char* item_description(Object* obj)
{
    return proto_description(obj->pid);
}

// 0x477AFC
int item_get_type(Object* item)
{
    if (item == NULL) {
        return ITEM_TYPE_MISC;
    }

    if (PID_TYPE(item->pid) != OBJ_TYPE_ITEM) {
        return ITEM_TYPE_MISC;
    }

    if (item->pid == PROTO_ID_SHIV) {
        return ITEM_TYPE_MISC;
    }

    Proto* proto;
    proto_ptr(item->pid, &proto);

    return proto->item.type;
}

// NOTE: Unused.
//
// 0x477B4C
int item_material(Object* item)
{
    Proto* proto;
    proto_ptr(item->pid, &proto);

    return proto->item.material;
}

// 0x477B68
int item_size(Object* item)
{
    if (item == NULL) {
        return 0;
    }

    Proto* proto;
    proto_ptr(item->pid, &proto);

    return proto->item.size;
}

// 0x477B88
int item_weight(Object* item)
{
    if (item == NULL) {
        return 0;
    }

    Proto* proto;
    proto_ptr(item->pid, &proto);
    int weight = proto->item.weight;

    // NOTE: Uninline.
    if (item_is_hidden(item)) {
        weight = 0;
    }

    int itemType = proto->item.type;
    if (itemType == ITEM_TYPE_ARMOR) {
        switch (proto->pid) {
        case PROTO_ID_POWER_ARMOR:
        case PROTO_ID_HARDENED_POWER_ARMOR:
        case PROTO_ID_ADVANCED_POWER_ARMOR:
        case PROTO_ID_ADVANCED_POWER_ARMOR_MK_II:
            weight /= 2;
            break;
        }
    } else if (itemType == ITEM_TYPE_CONTAINER) {
        weight += item_total_weight(item);
    } else if (itemType == ITEM_TYPE_WEAPON) {
        // NOTE: Uninline.
        int ammoQuantity = item_w_curr_ammo(item);
        if (ammoQuantity > 0) {
            // NOTE: Uninline.
            int ammoTypePid = item_w_ammo_pid(item);
            if (ammoTypePid != -1) {
                Proto* ammoProto;
                if (proto_ptr(ammoTypePid, &ammoProto) != -1) {
                    weight += ammoProto->item.weight * ((ammoQuantity - 1) / ammoProto->item.data.ammo.quantity + 1);
                }
            }
        }
    }

    return weight;
}

// Returns cost of item.
//
// When [item] is container the returned cost includes cost of container
// itself plus cost of contained items.
//
// When [item] is a weapon the returned value includes cost of weapon
// itself plus cost of remaining ammo (see below).
//
// When [item] is an ammo it's cost is calculated from ratio of fullness.
//
// 0x477CAC
int item_cost(Object* obj)
{
    // TODO: This function needs review. A lot of functionality is inlined.
    // Find these functions and use them.
    if (obj == NULL) {
        return 0;
    }

    Proto* proto;
    proto_ptr(obj->pid, &proto);

    int cost = proto->item.cost;

    switch (proto->item.type) {
    case ITEM_TYPE_CONTAINER:
        cost += item_total_cost(obj);
        break;
    case ITEM_TYPE_WEAPON:
        if (1) {
            // NOTE: Uninline.
            int ammoQuantity = item_w_curr_ammo(obj);
            if (ammoQuantity > 0) {
                // NOTE: Uninline.
                int ammoTypePid = item_w_ammo_pid(obj);
                if (ammoTypePid != -1) {
                    Proto* ammoProto;
                    proto_ptr(ammoTypePid, &ammoProto);

                    cost += ammoQuantity * ammoProto->item.cost / ammoProto->item.data.ammo.quantity;
                }
            }
        }
        break;
    case ITEM_TYPE_AMMO:
        if (1) {
            // NOTE: Uninline.
            int ammoQuantity = item_w_curr_ammo(obj);
            cost *= ammoQuantity;
            // NOTE: Uninline.
            int ammoCapacity = item_w_max_ammo(obj);
            cost /= ammoCapacity;
        }
        break;
    }

    return cost;
}

// Returns cost of object's items.
//
// 0x477DAC
int item_total_cost(Object* obj)
{
    if (obj == NULL) {
        return 0;
    }

    int cost = 0;

    Inventory* inventory = &(obj->data.inventory);
    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        if (item_get_type(inventoryItem->item) == ITEM_TYPE_AMMO) {
            Proto* proto;
            proto_ptr(inventoryItem->item->pid, &proto);

            // Ammo stack in inventory is a bit special. It is counted in clips,
            // `inventoryItem->quantity` is the number of clips. The ammo object
            // itself tracks remaining number of ammo in only one instance of
            // the clip implying all other clips in the stack are full.
            //
            // In order to correctly calculate cost of the ammo stack, add cost
            // of all full clips...
            cost += proto->item.cost * (inventoryItem->quantity - 1);

            // ...and add cost of the current clip, which is proportional to
            // it's capacity.
            cost += item_cost(inventoryItem->item);
        } else {
            cost += item_cost(inventoryItem->item) * inventoryItem->quantity;
        }
    }

    if (FID_TYPE(obj->fid) == OBJ_TYPE_CRITTER) {
        Object* item2 = inven_right_hand(obj);
        if (item2 != NULL && (item2->flags & OBJECT_IN_RIGHT_HAND) == 0) {
            cost += item_cost(item2);
        }

        Object* item1 = inven_left_hand(obj);
        if (item1 != NULL && (item1->flags & OBJECT_IN_LEFT_HAND) == 0) {
            cost += item_cost(item1);
        }

        Object* armor = inven_worn(obj);
        if (armor != NULL && (armor->flags & OBJECT_WORN) == 0) {
            cost += item_cost(armor);
        }
    }

    return cost;
}

// Calculates total weight of the items in inventory.
//
// 0x477E98
int item_total_weight(Object* obj)
{
    if (obj == NULL) {
        return 0;
    }

    int weight = 0;

    Inventory* inventory = &(obj->data.inventory);
    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        Object* item = inventoryItem->item;
        weight += item_weight(item) * inventoryItem->quantity;
    }

    if (FID_TYPE(obj->fid) == OBJ_TYPE_CRITTER) {
        Object* item2 = inven_right_hand(obj);
        if (item2 != NULL) {
            if ((item2->flags & OBJECT_IN_RIGHT_HAND) == 0) {
                weight += item_weight(item2);
            }
        }

        Object* item1 = inven_left_hand(obj);
        if (item1 != NULL) {
            if ((item1->flags & OBJECT_IN_LEFT_HAND) == 0) {
                weight += item_weight(item1);
            }
        }

        Object* armor = inven_worn(obj);
        if (armor != NULL) {
            if ((armor->flags & OBJECT_WORN) == 0) {
                weight += item_weight(armor);
            }
        }
    }

    return weight;
}

// 0x477F3C
bool item_grey(Object* weapon)
{
    if (weapon == NULL) {
        return false;
    }

    if (item_get_type(weapon) != ITEM_TYPE_WEAPON) {
        return false;
    }

    int flags = obj_dude->data.critter.combat.results;
    if ((flags & DAM_CRIP_ARM_LEFT) != 0 && (flags & DAM_CRIP_ARM_RIGHT) != 0) {
        return true;
    }

    // NOTE: Uninline.
    bool isTwoHanded = item_w_is_2handed(weapon);
    if (isTwoHanded) {
        if ((flags & DAM_CRIP_ARM_LEFT) != 0 || (flags & DAM_CRIP_ARM_RIGHT) != 0) {
            return true;
        }
    }

    return false;
}

// 0x477FB0
int item_inv_fid(Object* item)
{
    Proto* proto;

    if (item == NULL) {
        return -1;
    }

    proto_ptr(item->pid, &proto);

    return proto->item.inventoryFid;
}

// 0x477FF8
Object* item_hit_with(Object* critter, int hitMode)
{
    switch (hitMode) {
    case HIT_MODE_LEFT_WEAPON_PRIMARY:
    case HIT_MODE_LEFT_WEAPON_SECONDARY:
    case HIT_MODE_LEFT_WEAPON_RELOAD:
        return inven_left_hand(critter);
    case HIT_MODE_RIGHT_WEAPON_PRIMARY:
    case HIT_MODE_RIGHT_WEAPON_SECONDARY:
    case HIT_MODE_RIGHT_WEAPON_RELOAD:
        return inven_right_hand(critter);
    }

    return NULL;
}

// 0x478040
int item_mp_cost(Object* obj, int hitMode, bool aiming)
{
    if (obj == NULL) {
        return 0;
    }

    Object* item_obj = item_hit_with(obj, hitMode);

    if (item_obj != NULL && item_get_type(item_obj) != ITEM_TYPE_WEAPON) {
        return 2;
    }

    return item_w_mp_cost(obj, hitMode, aiming);
}

// Returns quantity of [a2] in [obj]s inventory.
//
// 0x47808C
int item_count(Object* obj, Object* a2)
{
    int quantity = 0;

    Inventory* inventory = &(obj->data.inventory);
    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        Object* item = inventoryItem->item;
        if (item == a2) {
            quantity = inventoryItem->quantity;
        } else {
            if (item_get_type(item) == ITEM_TYPE_CONTAINER) {
                quantity = item_count(item, a2);
                if (quantity > 0) {
                    return quantity;
                }
            }
        }
    }

    return quantity;
}

// Returns true if [a1] posesses an item with 0x2000 flag.
//
// 0x4780E4
int item_queued(Object* obj)
{
    if (obj == NULL) {
        return false;
    }

    if ((obj->flags & OBJECT_USED) != 0) {
        return true;
    }

    Inventory* inventory = &(obj->data.inventory);
    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        if ((inventoryItem->item->flags & OBJECT_USED) != 0) {
            return true;
        }

        if (item_get_type(inventoryItem->item) == ITEM_TYPE_CONTAINER) {
            if (item_queued(inventoryItem->item)) {
                return true;
            }
        }
    }

    return false;
}

// 0x478154
Object* item_replace(Object* a1, Object* a2, int a3)
{
    if (a1 == NULL) {
        return NULL;
    }

    if (a2 == NULL) {
        return NULL;
    }

    Inventory* inventory = &(a1->data.inventory);
    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);
        if (item_identical(inventoryItem->item, a2)) {
            Object* item = inventoryItem->item;
            if (item_remove_mult(a1, item, 1) == 0) {
                item->flags |= a3;
                if (item_add_force(a1, item, 1) == 0) {
                    return item;
                }

                item->flags &= ~a3;
                if (item_add_force(a1, item, 1) != 0) {
                    obj_destroy(item);
                }
            }
        }

        if (item_get_type(inventoryItem->item) == ITEM_TYPE_CONTAINER) {
            Object* obj = item_replace(inventoryItem->item, a2, a3);
            if (obj != NULL) {
                return obj;
            }
        }
    }

    return NULL;
}

// Returns true if [item] is an natural weapon of it's owner.
//
// See [ItemProtoExtendedFlags_NaturalWeapon] for more details on natural weapons.
//
// 0x478244
int item_is_hidden(Object* obj)
{
    Proto* proto;

    if (PID_TYPE(obj->pid) != OBJ_TYPE_ITEM) {
        return 0;
    }

    if (proto_ptr(obj->pid, &proto) == -1) {
        return 0;
    }

    return proto->item.extendedFlags & ItemProtoExtendedFlags_NaturalWeapon;
}

// 0x478280
int item_w_subtype(Object* weapon, int hitMode)
{
    if (weapon == NULL) {
        return ATTACK_TYPE_UNARMED;
    }

    Proto* proto;
    proto_ptr(weapon->pid, &proto);

    int index;
    if (hitMode == HIT_MODE_LEFT_WEAPON_PRIMARY || hitMode == HIT_MODE_RIGHT_WEAPON_PRIMARY) {
        index = proto->item.extendedFlags & 0xF;
    } else {
        index = (proto->item.extendedFlags & 0xF0) >> 4;
    }

    return attack_subtype[index];
}

// 0x4782CC
int item_w_skill(Object* weapon, int hitMode)
{
    if (weapon == NULL) {
        return SKILL_UNARMED;
    }

    Proto* proto;
    proto_ptr(weapon->pid, &proto);

    int index;
    if (hitMode == HIT_MODE_LEFT_WEAPON_PRIMARY || hitMode == HIT_MODE_RIGHT_WEAPON_PRIMARY) {
        index = proto->item.extendedFlags & 0xF;
    } else {
        index = (proto->item.extendedFlags & 0xF0) >> 4;
    }

    int skill = attack_skill[index];

    if (skill == SKILL_SMALL_GUNS) {
        int damageType = item_w_damage_type(NULL, weapon);
        if (damageType == DAMAGE_TYPE_LASER || damageType == DAMAGE_TYPE_PLASMA || damageType == DAMAGE_TYPE_ELECTRICAL) {
            skill = SKILL_ENERGY_WEAPONS;
        } else {
            if ((proto->item.extendedFlags & ItemProtoExtendedFlags_BigGun) != 0) {
                skill = SKILL_BIG_GUNS;
            }
        }
    }

    return skill;
}

// Returns skill value when critter is about to perform hitMode.
//
// 0x478370
int item_w_skill_level(Object* critter, int hitMode)
{
    if (critter == NULL) {
        return 0;
    }

    int skill;

    // NOTE: Uninline.
    Object* weapon = item_hit_with(critter, hitMode);
    if (weapon != NULL) {
        skill = item_w_skill(weapon, hitMode);
    } else {
        skill = SKILL_UNARMED;
    }

    return skill_level(critter, skill);
}

// 0x4783B8
int item_w_damage_min_max(Object* weapon, int* minDamagePtr, int* maxDamagePtr)
{
    if (weapon == NULL) {
        return -1;
    }

    Proto* proto;
    proto_ptr(weapon->pid, &proto);

    if (minDamagePtr != NULL) {
        *minDamagePtr = proto->item.data.weapon.minDamage;
    }

    if (maxDamagePtr != NULL) {
        *maxDamagePtr = proto->item.data.weapon.maxDamage;
    }

    return 0;
}

// 0x478448
int item_w_damage(Object* critter, int hitMode)
{
    if (critter == NULL) {
        return 0;
    }

    int minDamage = 0;
    int maxDamage = 0;
    int meleeDamage = 0;
    int unarmedDamage = 0;

    // NOTE: Uninline.
    Object* weapon = item_hit_with(critter, hitMode);

    if (weapon != NULL) {
        Proto* proto;
        proto_ptr(weapon->pid, &proto);

        minDamage = proto->item.data.weapon.minDamage;
        maxDamage = proto->item.data.weapon.maxDamage;

        int attackType = item_w_subtype(weapon, hitMode);
        if (attackType == ATTACK_TYPE_MELEE || attackType == ATTACK_TYPE_UNARMED) {
            meleeDamage = critterGetStat(critter, STAT_MELEE_DAMAGE);
        }
    } else {
        minDamage = 1;
        maxDamage = critterGetStat(critter, STAT_MELEE_DAMAGE) + 2;

        switch (hitMode) {
        case HIT_MODE_STRONG_PUNCH:
        case HIT_MODE_JAB:
            unarmedDamage = 3;
            break;
        case HIT_MODE_HAMMER_PUNCH:
        case HIT_MODE_STRONG_KICK:
            unarmedDamage = 4;
            break;
        case HIT_MODE_HAYMAKER:
        case HIT_MODE_PALM_STRIKE:
        case HIT_MODE_SNAP_KICK:
        case HIT_MODE_HIP_KICK:
            unarmedDamage = 7;
            break;
        case HIT_MODE_POWER_KICK:
        case HIT_MODE_HOOK_KICK:
            unarmedDamage = 9;
            break;
        case HIT_MODE_PIERCING_STRIKE:
            unarmedDamage = 10;
            break;
        case HIT_MODE_PIERCING_KICK:
            unarmedDamage = 12;
            break;
        }
    }

    return roll_random(unarmedDamage + minDamage, unarmedDamage + meleeDamage + maxDamage);
}

// 0x478570
int item_w_damage_type(Object* critter, Object* weapon)
{
    Proto* proto;

    if (weapon != NULL) {
        proto_ptr(weapon->pid, &proto);

        return proto->item.data.weapon.damageType;
    }

    if (critter != NULL) {
        return critter_get_base_damage_type(critter);
    }

    return 0;
}

// 0x478598
int item_w_is_2handed(Object* weapon)
{
    Proto* proto;

    if (weapon == NULL) {
        return 0;
    }

    proto_ptr(weapon->pid, &proto);

    return (proto->item.extendedFlags & WEAPON_TWO_HAND) != 0;
}

// 0x4785DC
int item_w_anim(Object* critter, int hitMode)
{
    // NOTE: Uninline.
    Object* weapon = item_hit_with(critter, hitMode);
    return item_w_anim_weap(weapon, hitMode);
}

// 0x47860C
int item_w_anim_weap(Object* weapon, int hitMode)
{
    if (hitMode == HIT_MODE_KICK || (hitMode >= FIRST_ADVANCED_KICK_HIT_MODE && hitMode <= LAST_ADVANCED_KICK_HIT_MODE)) {
        return ANIM_KICK_LEG;
    }

    if (weapon == NULL) {
        return ANIM_THROW_PUNCH;
    }

    Proto* proto;
    proto_ptr(weapon->pid, &proto);

    int index;
    if (hitMode == HIT_MODE_LEFT_WEAPON_PRIMARY || hitMode == HIT_MODE_RIGHT_WEAPON_PRIMARY) {
        index = proto->item.extendedFlags & 0xF;
    } else {
        index = (proto->item.extendedFlags & 0xF0) >> 4;
    }

    return attack_anim[index];
}

// 0x478674
int item_w_max_ammo(Object* ammoOrWeapon)
{
    if (ammoOrWeapon == NULL) {
        return 0;
    }

    Proto* proto;
    proto_ptr(ammoOrWeapon->pid, &proto);

    if (proto->item.type == ITEM_TYPE_AMMO) {
        return proto->item.data.ammo.quantity;
    } else {
        return proto->item.data.weapon.ammoCapacity;
    }
}

// 0x4786A0
int item_w_curr_ammo(Object* ammoOrWeapon)
{
    if (ammoOrWeapon == NULL) {
        return 0;
    }

    Proto* proto;
    proto_ptr(ammoOrWeapon->pid, &proto);

    // NOTE: Looks like the condition jumps were erased during compilation only
    // because ammo's quantity and weapon's ammo quantity coincidently stored
    // in the same offset relative to [Object].
    if (proto->item.type == ITEM_TYPE_AMMO) {
        return ammoOrWeapon->data.item.ammo.quantity;
    } else {
        return ammoOrWeapon->data.item.weapon.ammoQuantity;
    }
}

// 0x4786C8
int item_w_caliber(Object* ammoOrWeapon)
{
    Proto* proto;

    if (ammoOrWeapon == NULL) {
        return 0;
    }

    proto_ptr(ammoOrWeapon->pid, &proto);

    if (proto->item.type != ITEM_TYPE_AMMO) {
        if (proto_ptr(ammoOrWeapon->data.item.weapon.ammoTypePid, &proto) == -1) {
            return 0;
        }
    }

    return proto->item.data.ammo.caliber;
}

// 0x478714
void item_w_set_curr_ammo(Object* ammoOrWeapon, int quantity)
{
    if (ammoOrWeapon == NULL) {
        return;
    }

    // NOTE: Uninline.
    int capacity = item_w_max_ammo(ammoOrWeapon);
    if (quantity > capacity) {
        quantity = capacity;
    }

    Proto* proto;
    proto_ptr(ammoOrWeapon->pid, &proto);

    if (proto->item.type == ITEM_TYPE_AMMO) {
        ammoOrWeapon->data.item.ammo.quantity = quantity;
    } else {
        ammoOrWeapon->data.item.weapon.ammoQuantity = quantity;
    }
}

// 0x478768
int item_w_try_reload(Object* critter, Object* weapon)
{
    // NOTE: Uninline.
    int quantity = item_w_curr_ammo(weapon);
    int capacity = item_w_max_ammo(weapon);
    if (quantity == capacity) {
        return -1;
    }

    if (weapon->pid != PROTO_ID_SOLAR_SCORCHER) {
        int inventoryItemIndex = -1;
        for (;;) {
            Object* ammo = inven_find_type(critter, ITEM_TYPE_AMMO, &inventoryItemIndex);
            if (ammo == NULL) {
                break;
            }

            if (weapon->data.item.weapon.ammoTypePid == ammo->pid) {
                if (item_w_can_reload(weapon, ammo) != 0) {
                    int rc = item_w_reload(weapon, ammo);
                    if (rc == 0) {
                        obj_destroy(ammo);
                    }

                    if (rc == -1) {
                        return -1;
                    }

                    return 0;
                }
            }
        }

        inventoryItemIndex = -1;
        for (;;) {
            Object* ammo = inven_find_type(critter, ITEM_TYPE_AMMO, &inventoryItemIndex);
            if (ammo == NULL) {
                break;
            }

            if (item_w_can_reload(weapon, ammo) != 0) {
                int rc = item_w_reload(weapon, ammo);
                if (rc == 0) {
                    obj_destroy(ammo);
                }

                if (rc == -1) {
                    return -1;
                }

                return 0;
            }
        }
    }

    if (item_w_reload(weapon, NULL) != 0) {
        return -1;
    }

    return 0;
}

// Checks if weapon can be reloaded with the specified ammo.
//
// 0x478874
bool item_w_can_reload(Object* weapon, Object* ammo)
{
    if (weapon->pid == PROTO_ID_SOLAR_SCORCHER) {
        // Check light level to recharge solar scorcher.
        if (light_get_ambient() > 62259) {
            return true;
        }

        // There is not enough light to recharge this item.
        MessageListItem messageListItem;
        char* msg = getmsg(&item_message_file, &messageListItem, 500);
        display_print(msg);

        return false;
    }

    if (ammo == NULL) {
        return false;
    }

    Proto* weaponProto;
    proto_ptr(weapon->pid, &weaponProto);

    Proto* ammoProto;
    proto_ptr(ammo->pid, &ammoProto);

    if (weaponProto->item.type != ITEM_TYPE_WEAPON) {
        return false;
    }

    if (ammoProto->item.type != ITEM_TYPE_AMMO) {
        return false;
    }

    // Check ammo matches weapon caliber.
    if (weaponProto->item.data.weapon.caliber != ammoProto->item.data.ammo.caliber) {
        return false;
    }

    // If weapon is not empty, we should only reload it with the same ammo.
    if (item_w_curr_ammo(weapon) != 0) {
        if (weapon->data.item.weapon.ammoTypePid != ammo->pid) {
            return false;
        }
    }

    return true;
}

// 0x478918
int item_w_reload(Object* weapon, Object* ammo)
{
    if (!item_w_can_reload(weapon, ammo)) {
        return -1;
    }

    // NOTE: Uninline.
    int ammoQuantity = item_w_curr_ammo(weapon);

    // NOTE: Uninline.
    int ammoCapacity = item_w_max_ammo(weapon);

    if (weapon->pid == PROTO_ID_SOLAR_SCORCHER) {
        item_w_set_curr_ammo(weapon, ammoCapacity);
        return 0;
    }

    // NOTE: Uninline.
    int v10 = item_w_curr_ammo(ammo);

    int v11 = v10;
    if (ammoQuantity < ammoCapacity) {
        int v12;
        if (ammoQuantity + v10 > ammoCapacity) {
            v11 = v10 - (ammoCapacity - ammoQuantity);
            v12 = ammoCapacity;
        } else {
            v11 = 0;
            v12 = ammoQuantity + v10;
        }

        weapon->data.item.weapon.ammoTypePid = ammo->pid;

        item_w_set_curr_ammo(ammo, v11);
        item_w_set_curr_ammo(weapon, v12);
    }

    return v11;
}

// 0x478A1C
int item_w_range(Object* critter, int hitMode)
{
    int range;
    int v12;

    // NOTE: Uninline.
    Object* weapon = item_hit_with(critter, hitMode);

    if (weapon != NULL && hitMode != 4 && hitMode != 5 && (hitMode < 8 || hitMode > 19)) {
        Proto* proto;
        proto_ptr(weapon->pid, &proto);
        if (hitMode == HIT_MODE_LEFT_WEAPON_PRIMARY || hitMode == HIT_MODE_RIGHT_WEAPON_PRIMARY) {
            range = proto->item.data.weapon.maxRange1;
        } else {
            range = proto->item.data.weapon.maxRange2;
        }

        if (item_w_subtype(weapon, hitMode) == ATTACK_TYPE_THROW) {
            if (critter == obj_dude) {
                v12 = critterGetStat(critter, STAT_STRENGTH) + 2 * perk_level(critter, PERK_HEAVE_HO);
            } else {
                v12 = critterGetStat(critter, STAT_STRENGTH);
            }

            int maxRange = 3 * v12;
            if (range >= maxRange) {
                range = maxRange;
            }
        }

        return range;
    }

    if (critter_flag_check(critter->pid, CRITTER_LONG_LIMBS)) {
        return 2;
    }

    return 1;
}

// Returns action points required for hit mode.
//
// 0x478B24
int item_w_mp_cost(Object* critter, int hitMode, bool aiming)
{
    int actionPoints;

    // NOTE: Uninline.
    Object* weapon = item_hit_with(critter, hitMode);

    if (hitMode == HIT_MODE_LEFT_WEAPON_RELOAD || hitMode == HIT_MODE_RIGHT_WEAPON_RELOAD) {
        if (weapon != NULL) {
            Proto* proto;
            proto_ptr(weapon->pid, &proto);
            if (proto->item.data.weapon.perk == PERK_WEAPON_FAST_RELOAD) {
                return 1;
            }

            if (weapon->pid == PROTO_ID_SOLAR_SCORCHER) {
                return 0;
            }
        }
        return 2;
    }

    switch (hitMode) {
    case HIT_MODE_PALM_STRIKE:
        actionPoints = 6;
        break;
    case HIT_MODE_PIERCING_STRIKE:
        actionPoints = 8;
        break;
    case HIT_MODE_STRONG_KICK:
    case HIT_MODE_SNAP_KICK:
    case HIT_MODE_POWER_KICK:
        actionPoints = 4;
        break;
    case HIT_MODE_HIP_KICK:
    case HIT_MODE_HOOK_KICK:
        actionPoints = 7;
        break;
    case HIT_MODE_PIERCING_KICK:
        actionPoints = 9;
        break;
    default:
        // TODO: Inverse conditions.
        if (weapon != NULL && hitMode != HIT_MODE_PUNCH && hitMode != HIT_MODE_KICK && hitMode != HIT_MODE_STRONG_PUNCH && hitMode != HIT_MODE_HAMMER_PUNCH && hitMode != HIT_MODE_HAYMAKER) {
            if (hitMode == HIT_MODE_LEFT_WEAPON_PRIMARY || hitMode == HIT_MODE_RIGHT_WEAPON_PRIMARY) {
                // NOTE: Uninline.
                actionPoints = item_w_primary_mp_cost(weapon);
            } else {
                // NOTE: Uninline.
                actionPoints = item_w_secondary_mp_cost(weapon);
            }

            if (critter == obj_dude) {
                if (traitIsSelected(TRAIT_FAST_SHOT)) {
                    if (item_w_range(critter, hitMode) > 2) {
                        actionPoints--;
                    }
                }
            }
        } else {
            actionPoints = 3;
        }
        break;
    }

    if (critter == obj_dude) {
        int attackType = item_w_subtype(weapon, hitMode);

        if (perkHasRank(obj_dude, PERK_BONUS_HTH_ATTACKS)) {
            if (attackType == ATTACK_TYPE_MELEE || attackType == ATTACK_TYPE_UNARMED) {
                actionPoints -= 1;
            }
        }

        if (perkHasRank(obj_dude, PERK_BONUS_RATE_OF_FIRE)) {
            if (attackType == ATTACK_TYPE_RANGED) {
                actionPoints -= 1;
            }
        }
    }

    if (aiming) {
        actionPoints += 1;
    }

    if (actionPoints < 1) {
        actionPoints = 1;
    }

    return actionPoints;
}

// 0x478D08
int item_w_min_st(Object* weapon)
{
    if (weapon == NULL) {
        return -1;
    }

    Proto* proto;
    proto_ptr(weapon->pid, &proto);

    return proto->item.data.weapon.minStrength;
}

// 0x478D30
int item_w_crit_fail(Object* weapon)
{
    if (weapon == NULL) {
        return -1;
    }

    Proto* proto;
    proto_ptr(weapon->pid, &proto);

    return proto->item.data.weapon.criticalFailureType;
}

// 0x478D58
int item_w_perk(Object* weapon)
{
    if (weapon == NULL) {
        return -1;
    }

    Proto* proto;
    proto_ptr(weapon->pid, &proto);

    return proto->item.data.weapon.perk;
}

// 0x478D80
int item_w_rounds(Object* weapon)
{
    if (weapon == NULL) {
        return -1;
    }

    Proto* proto;
    proto_ptr(weapon->pid, &proto);

    return proto->item.data.weapon.rounds;
}

// 0x478DA8
int item_w_anim_code(Object* weapon)
{
    if (weapon == NULL) {
        return -1;
    }

    Proto* proto;
    proto_ptr(weapon->pid, &proto);

    return proto->item.data.weapon.animationCode;
}

// 0x478DD0
int item_w_proj_pid(Object* weapon)
{
    if (weapon == NULL) {
        return -1;
    }

    Proto* proto;
    proto_ptr(weapon->pid, &proto);

    return proto->item.data.weapon.projectilePid;
}

// 0x478DF8
int item_w_ammo_pid(Object* weapon)
{
    if (weapon == NULL) {
        return -1;
    }

    if (item_get_type(weapon) != ITEM_TYPE_WEAPON) {
        return -1;
    }

    return weapon->data.item.weapon.ammoTypePid;
}

// 0x478E18
char item_w_sound_id(Object* weapon)
{
    if (weapon == NULL) {
        return '\0';
    }

    Proto* proto;
    proto_ptr(weapon->pid, &proto);

    return proto->item.data.weapon.soundCode & 0xFF;
}

// 0x478E5C
int item_w_called_shot(Object* critter, int hitMode)
{
    if (critter == obj_dude && traitIsSelected(TRAIT_FAST_SHOT)) {
        return 0;
    }

    // NOTE: Uninline.
    int anim = item_w_anim(critter, hitMode);
    if (anim == ANIM_FIRE_BURST || anim == ANIM_FIRE_CONTINUOUS) {
        return 0;
    }

    // NOTE: Uninline.
    Object* weapon = item_hit_with(critter, hitMode);
    int damageType = item_w_damage_type(critter, weapon);

    return damageType != DAMAGE_TYPE_EXPLOSION
        && damageType != DAMAGE_TYPE_FIRE
        && damageType != DAMAGE_TYPE_EMP
        && (damageType != DAMAGE_TYPE_PLASMA || anim != ANIM_THROW_ANIM);
}

// 0x478EF4
int item_w_can_unload(Object* weapon)
{
    if (weapon == NULL) {
        return false;
    }

    if (item_get_type(weapon) != ITEM_TYPE_WEAPON) {
        return false;
    }

    // NOTE: Uninline.
    int ammoCapacity = item_w_max_ammo(weapon);
    if (ammoCapacity <= 0) {
        return false;
    }

    // NOTE: Uninline.
    int ammoQuantity = item_w_curr_ammo(weapon);
    if (ammoQuantity <= 0) {
        return false;
    }

    if (weapon->pid == PROTO_ID_SOLAR_SCORCHER) {
        return false;
    }

    if (item_w_ammo_pid(weapon) == -1) {
        return false;
    }

    return true;
}

// 0x478F80
Object* item_w_unload(Object* weapon)
{
    if (!item_w_can_unload(weapon)) {
        return NULL;
    }

    // NOTE: Uninline.
    int ammoTypePid = item_w_ammo_pid(weapon);
    if (ammoTypePid == -1) {
        return NULL;
    }

    Object* ammo;
    if (obj_pid_new(&ammo, ammoTypePid) != 0) {
        return NULL;
    }

    obj_disconnect(ammo, NULL);

    // NOTE: Uninline.
    int ammoQuantity = item_w_curr_ammo(weapon);

    // NOTE: Uninline.
    int ammoCapacity = item_w_max_ammo(ammo);

    int remainingQuantity;
    if (ammoQuantity <= ammoCapacity) {
        item_w_set_curr_ammo(ammo, ammoQuantity);
        remainingQuantity = 0;
    } else {
        item_w_set_curr_ammo(ammo, ammoCapacity);
        remainingQuantity = ammoQuantity - ammoCapacity;
    }
    item_w_set_curr_ammo(weapon, remainingQuantity);

    return ammo;
}

// 0x47905C
int item_w_primary_mp_cost(Object* weapon)
{
    if (weapon == NULL) {
        return -1;
    }

    Proto* proto;
    proto_ptr(weapon->pid, &proto);

    return proto->item.data.weapon.actionPointCost1;
}

// NOTE: Inlined.
//
// 0x479084
int item_w_secondary_mp_cost(Object* weapon)
{
    if (weapon == NULL) {
        return -1;
    }

    Proto* proto;
    proto_ptr(weapon->pid, &proto);

    return proto->item.data.weapon.actionPointCost2;
}

// 0x4790AC
int item_w_compute_ammo_cost(Object* obj, int* inout_a2)
{
    int pid;

    if (inout_a2 == NULL) {
        return -1;
    }

    if (obj == NULL) {
        return 0;
    }

    pid = obj->pid;
    if (pid == PROTO_ID_SUPER_CATTLE_PROD || pid == PROTO_ID_MEGA_POWER_FIST) {
        *inout_a2 *= 2;
    }

    return 0;
}

// 0x4790E8
bool item_w_is_grenade(Object* weapon)
{
    int damageType = item_w_damage_type(NULL, weapon);
    return damageType == DAMAGE_TYPE_EXPLOSION || damageType == DAMAGE_TYPE_PLASMA || damageType == DAMAGE_TYPE_EMP;
}

// 0x47910C
int item_w_area_damage_radius(Object* weapon, int hitMode)
{
    int attackType = item_w_subtype(weapon, hitMode);
    int anim = item_w_anim_weap(weapon, hitMode);
    int damageType = item_w_damage_type(NULL, weapon);

    int damageRadius = 0;
    if (attackType == ATTACK_TYPE_RANGED) {
        if (anim == ANIM_FIRE_SINGLE && damageType == DAMAGE_TYPE_EXPLOSION) {
            // NOTE: Uninline.
            damageRadius = item_w_rocket_dmg_radius(weapon);
        }
    } else if (attackType == ATTACK_TYPE_THROW) {
        // NOTE: Uninline.
        if (item_w_is_grenade(weapon)) {
            // NOTE: Uninline.
            damageRadius = item_w_grenade_dmg_radius(weapon);
        }
    }
    return damageRadius;
}

// 0x479180
int item_w_grenade_dmg_radius(Object* weapon)
{
    return 2;
}

// 0x479188
int item_w_rocket_dmg_radius(Object* weapon)
{
    return 3;
}

// 0x479190
int item_w_ac_adjust(Object* weapon)
{
    // NOTE: Uninline.
    int ammoTypePid = item_w_ammo_pid(weapon);
    if (ammoTypePid == -1) {
        return 0;
    }

    Proto* proto;
    if (proto_ptr(ammoTypePid, &proto) == -1) {
        return 0;
    }

    return proto->item.data.ammo.armorClassModifier;
}

// 0x4791E0
int item_w_dr_adjust(Object* weapon)
{
    // NOTE: Uninline.
    int ammoTypePid = item_w_ammo_pid(weapon);
    if (ammoTypePid == -1) {
        return 0;
    }

    Proto* proto;
    if (proto_ptr(ammoTypePid, &proto) == -1) {
        return 0;
    }

    return proto->item.data.ammo.damageResistanceModifier;
}

// 0x479230
int item_w_dam_mult(Object* weapon)
{
    // NOTE: Uninline.
    int ammoTypePid = item_w_ammo_pid(weapon);
    if (ammoTypePid == -1) {
        return 1;
    }

    Proto* proto;
    if (proto_ptr(ammoTypePid, &proto) == -1) {
        return 1;
    }

    return proto->item.data.ammo.damageMultiplier;
}

// 0x479294
int item_w_dam_div(Object* weapon)
{
    // NOTE: Uninline.
    int ammoTypePid = item_w_ammo_pid(weapon);
    if (ammoTypePid == -1) {
        return 1;
    }

    Proto* proto;
    if (proto_ptr(ammoTypePid, &proto) == -1) {
        return 1;
    }

    return proto->item.data.ammo.damageDivisor;
}

// 0x4792F8
int item_ar_ac(Object* armor)
{
    if (armor == NULL) {
        return 0;
    }

    Proto* proto;
    proto_ptr(armor->pid, &proto);

    return proto->item.data.armor.armorClass;
}

// 0x479318
int item_ar_dr(Object* armor, int damageType)
{
    if (armor == NULL) {
        return 0;
    }

    Proto* proto;
    proto_ptr(armor->pid, &proto);

    return proto->item.data.armor.damageResistance[damageType];
}

// 0x479338
int item_ar_dt(Object* armor, int damageType)
{
    if (armor == NULL) {
        return 0;
    }

    Proto* proto;
    proto_ptr(armor->pid, &proto);

    return proto->item.data.armor.damageThreshold[damageType];
}

// 0x479358
int item_ar_perk(Object* armor)
{
    if (armor == NULL) {
        return -1;
    }

    Proto* proto;
    proto_ptr(armor->pid, &proto);

    return proto->item.data.armor.perk;
}

// 0x479380
int item_ar_male_fid(Object* armor)
{
    if (armor == NULL) {
        return -1;
    }

    Proto* proto;
    proto_ptr(armor->pid, &proto);

    return proto->item.data.armor.maleFid;
}

// 0x4793A8
int item_ar_female_fid(Object* armor)
{
    if (armor == NULL) {
        return -1;
    }

    Proto* proto;
    proto_ptr(armor->pid, &proto);

    return proto->item.data.armor.femaleFid;
}

// 0x4793D0
int item_m_max_charges(Object* miscItem)
{
    if (miscItem == NULL) {
        return 0;
    }

    Proto* proto;
    proto_ptr(miscItem->pid, &proto);

    return proto->item.data.misc.charges;
}

// 0x4793F0
int item_m_curr_charges(Object* miscItem)
{
    if (miscItem == NULL) {
        return 0;
    }

    return miscItem->data.item.misc.charges;
}

// 0x4793F8
int item_m_set_charges(Object* miscItem, int charges)
{
    // NOTE: Uninline.
    int maxCharges = item_m_max_charges(miscItem);

    if (charges > maxCharges) {
        charges = maxCharges;
    }

    miscItem->data.item.misc.charges = charges;

    return 0;
}

// NOTE: Unused.
//
// 0x479434
int item_m_cell(Object* miscItem)
{
    if (miscItem == NULL) {
        return 0;
    }

    Proto* proto;
    proto_ptr(miscItem->pid, &proto);

    return proto->item.data.misc.powerType;
}

// NOTE: Inlined.
//
// 0x479454
int item_m_cell_pid(Object* miscItem)
{
    if (miscItem == NULL) {
        return -1;
    }

    Proto* proto;
    proto_ptr(miscItem->pid, &proto);

    return proto->item.data.misc.powerTypePid;
}

// 0x47947C
bool item_m_uses_charges(Object* miscItem)
{
    if (miscItem == NULL) {
        return false;
    }

    Proto* proto;
    proto_ptr(miscItem->pid, &proto);

    return proto->item.data.misc.charges != 0;
}

// 0x4794A4
int item_m_use_charged_item(Object* critter, Object* miscItem)
{
    int pid = miscItem->pid;
    if (pid == PROTO_ID_STEALTH_BOY_I
        || pid == PROTO_ID_GEIGER_COUNTER_I
        || pid == PROTO_ID_STEALTH_BOY_II
        || pid == PROTO_ID_GEIGER_COUNTER_II) {
        // NOTE: Uninline.
        bool isOn = item_m_on(miscItem);

        if (isOn) {
            item_m_turn_off(miscItem);
        } else {
            item_m_turn_on(miscItem);
        }
    } else if (pid == PROTO_ID_MOTION_SENSOR) {
        // NOTE: Uninline.
        if (item_m_dec_charges(miscItem) == 0) {
            automap(true, true);
        } else {
            MessageListItem messageListItem;
            // %s has no charges left.
            messageListItem.num = 5;
            if (message_search(&item_message_file, &messageListItem)) {
                char text[80];
                const char* itemName = object_name(miscItem);
                sprintf(text, messageListItem.text, itemName);
                display_print(text);
            }
        }
    }

    return 0;
}

// 0x4795A4
int item_m_dec_charges(Object* item)
{
    // NOTE: Uninline.
    int charges = item_m_curr_charges(item);
    if (charges <= 0) {
        return -1;
    }

    // NOTE: Uninline.
    item_m_set_charges(item, charges - 1);

    return 0;
}

// 0x4795F0
int item_m_trickle(Object* item, void* data)
{
    // NOTE: Uninline.
    if (item_m_dec_charges(item) == 0) {
        int delay;
        if (item->pid == PROTO_ID_STEALTH_BOY_I || item->pid == PROTO_ID_STEALTH_BOY_II) {
            delay = 600;
        } else {
            delay = 3000;
        }

        queue_add(delay, item, NULL, EVENT_TYPE_ITEM_TRICKLE);
    } else {
        Object* critter = obj_top_environment(item);
        if (critter == obj_dude) {
            MessageListItem messageListItem;
            // %s has no charges left.
            messageListItem.num = 5;
            if (message_search(&item_message_file, &messageListItem)) {
                char text[80];
                const char* itemName = object_name(item);
                sprintf(text, messageListItem.text, itemName);
                display_print(text);
            }
        }
        item_m_turn_off(item);
    }

    return 0;
}

// 0x4796A8
bool item_m_on(Object* obj)
{
    if (obj == NULL) {
        return false;
    }

    if (!item_m_uses_charges(obj)) {
        return false;
    }

    return queue_find(obj, EVENT_TYPE_ITEM_TRICKLE);
}

// Turns on geiger counter or stealth boy.
//
// 0x4796D0
int item_m_turn_on(Object* item)
{
    MessageListItem messageListItem;
    char text[80];

    Object* critter = obj_top_environment(item);
    if (critter == NULL) {
        // This item can only be used from the interface bar.
        messageListItem.num = 9;
        if (message_search(&item_message_file, &messageListItem)) {
            display_print(messageListItem.text);
        }

        return -1;
    }

    // NOTE: Uninline.
    if (item_m_dec_charges(item) != 0) {
        if (critter == obj_dude) {
            messageListItem.num = 5;
            if (message_search(&item_message_file, &messageListItem)) {
                char* name = object_name(item);
                sprintf(text, messageListItem.text, name);
                display_print(text);
            }
        }

        return -1;
    }

    if (item->pid == PROTO_ID_STEALTH_BOY_I || item->pid == PROTO_ID_STEALTH_BOY_II) {
        queue_add(600, item, 0, EVENT_TYPE_ITEM_TRICKLE);
        item->pid = PROTO_ID_STEALTH_BOY_II;

        if (critter != NULL) {
            // NOTE: Uninline.
            item_m_stealth_effect_on(critter);
        }
    } else {
        queue_add(3000, item, 0, EVENT_TYPE_ITEM_TRICKLE);
        item->pid = PROTO_ID_GEIGER_COUNTER_II;
    }

    if (critter == obj_dude) {
        // %s is on.
        messageListItem.num = 6;
        if (message_search(&item_message_file, &messageListItem)) {
            char* name = object_name(item);
            sprintf(text, messageListItem.text, name);
            display_print(text);
        }

        if (item->pid == PROTO_ID_GEIGER_COUNTER_II) {
            // You pass the Geiger counter over you body. The rem counter reads: %d
            messageListItem.num = 8;
            if (message_search(&item_message_file, &messageListItem)) {
                int radiation = critter_get_rads(critter);
                sprintf(text, messageListItem.text, radiation);
                display_print(text);
            }
        }
    }

    return 0;
}

// Turns off geiger counter or stealth boy.
//
// 0x479898
int item_m_turn_off(Object* item)
{
    Object* owner = obj_top_environment(item);

    queue_remove_this(item, EVENT_TYPE_ITEM_TRICKLE);

    if (owner != NULL && item->pid == PROTO_ID_STEALTH_BOY_II) {
        item_m_stealth_effect_off(owner, item);
    }

    if (item->pid == PROTO_ID_STEALTH_BOY_I || item->pid == PROTO_ID_STEALTH_BOY_II) {
        item->pid = PROTO_ID_STEALTH_BOY_I;
    } else {
        item->pid = PROTO_ID_GEIGER_COUNTER_I;
    }

    if (owner == obj_dude) {
        intface_update_items(false, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);
    }

    if (owner == obj_dude) {
        // %s is off.
        MessageListItem messageListItem;
        messageListItem.num = 7;
        if (message_search(&item_message_file, &messageListItem)) {
            const char* name = object_name(item);
            char text[80];
            sprintf(text, messageListItem.text, name);
            display_print(text);
        }
    }

    return 0;
}

// 0x479954
int item_m_turn_off_from_queue(Object* obj, void* data)
{
    item_m_turn_off(obj);
    return 1;
}

// NOTE: Inlined.
//
// 0x479960
static int item_m_stealth_effect_on(Object* object)
{
    if ((object->flags & OBJECT_TRANS_GLASS) != 0) {
        return -1;
    }

    object->flags |= OBJECT_TRANS_GLASS;

    Rect rect;
    obj_bound(object, &rect);
    tileWindowRefreshRect(&rect, object->elevation);

    return 0;
}

// 0x479998
static int item_m_stealth_effect_off(Object* critter, Object* item)
{
    Object* item1 = inven_left_hand(critter);
    if (item1 != NULL && item1 != item && item1->pid == PROTO_ID_STEALTH_BOY_II) {
        return -1;
    }

    Object* item2 = inven_right_hand(critter);
    if (item2 != NULL && item2 != item && item2->pid == PROTO_ID_STEALTH_BOY_II) {
        return -1;
    }

    if ((critter->flags & OBJECT_TRANS_GLASS) == 0) {
        return -1;
    }

    critter->flags &= ~OBJECT_TRANS_GLASS;

    Rect rect;
    obj_bound(critter, &rect);
    tileWindowRefreshRect(&rect, critter->elevation);

    return 0;
}

// 0x479A00
int item_c_max_size(Object* container)
{
    if (container == NULL) {
        return 0;
    }

    Proto* proto;
    proto_ptr(container->pid, &proto);

    return proto->item.data.container.maxSize;
}

// 0x479A20
int item_c_curr_size(Object* container)
{
    if (container == NULL) {
        return 0;
    }

    int totalSize = 0;

    Inventory* inventory = &(container->data.inventory);
    for (int index = 0; index < inventory->length; index++) {
        InventoryItem* inventoryItem = &(inventory->items[index]);

        int size = item_size(inventoryItem->item);
        totalSize += inventory->items[index].quantity * size;
    }

    return totalSize;
}

// 0x479A74
int item_a_ac_adjust(Object* armor)
{
    if (armor == NULL) {
        return 0;
    }

    Proto* proto;
    if (proto_ptr(armor->pid, &proto) == -1) {
        return 0;
    }

    return proto->item.data.ammo.armorClassModifier;
}

// 0x479AA4
int item_a_dr_adjust(Object* armor)
{
    if (armor == NULL) {
        return 0;
    }

    Proto* proto;
    if (proto_ptr(armor->pid, &proto) == -1) {
        return 0;
    }

    return proto->item.data.ammo.damageResistanceModifier;
}

// 0x479AD4
int item_a_dam_mult(Object* armor)
{
    if (armor == NULL) {
        return 0;
    }

    Proto* proto;
    if (proto_ptr(armor->pid, &proto) == -1) {
        return 0;
    }

    return proto->item.data.ammo.damageMultiplier;
}

// 0x479B04
int item_a_dam_div(Object* armor)
{
    if (armor == NULL) {
        return 0;
    }

    Proto* proto;
    if (proto_ptr(armor->pid, &proto) == -1) {
        return 0;
    }

    return proto->item.data.ammo.damageDivisor;
}

// 0x479B44
static int insert_drug_effect(Object* critter, Object* item, int a3, int* stats, int* mods)
{
    int index;
    for (index = 0; index < 3; index++) {
        if (mods[index] != 0) {
            break;
        }
    }

    if (index == 3) {
        return -1;
    }

    DrugEffectEvent* drugEffectEvent = (DrugEffectEvent*)internal_malloc(sizeof(*drugEffectEvent));
    if (drugEffectEvent == NULL) {
        return -1;
    }

    drugEffectEvent->drugPid = item->pid;

    for (index = 0; index < 3; index++) {
        drugEffectEvent->stats[index] = stats[index];
        drugEffectEvent->modifiers[index] = mods[index];
    }

    int delay = 600 * a3;
    if (critter == obj_dude) {
        if (traitIsSelected(TRAIT_CHEM_RESISTANT)) {
            delay /= 2;
        }
    }

    if (queue_add(delay, critter, drugEffectEvent, EVENT_TYPE_DRUG) == -1) {
        internal_free(drugEffectEvent);
        return -1;
    }

    return 0;
}

// 0x479C20
static void perform_drug_effect(Object* critter, int* stats, int* mods, bool isImmediate)
{
    int v10;
    int v11;
    int v12;
    MessageListItem messageListItem;
    const char* name;
    const char* text;
    char v24[92]; // TODO: Size is probably wrong.
    char str[92]; // TODO: Size is probably wrong.

    bool statsChanged = false;

    int v5 = 0;
    bool v32 = false;
    if (stats[0] == -2) {
        v5 = 1;
        v32 = true;
    }

    for (int index = v5; index < 3; index++) {
        int stat = stats[index];
        if (stat == -1) {
            continue;
        }

        if (stat == STAT_CURRENT_HIT_POINTS) {
            critter->data.critter.combat.maneuver &= ~CRITTER_MANUEVER_FLEEING;
        }

        v10 = critterGetBonusStat(critter, stat);

        int before;
        if (critter == obj_dude) {
            before = critterGetStat(obj_dude, stat);
        }

        if (v32) {
            v11 = roll_random(mods[index - 1], mods[index]) + v10;
            v32 = false;
        } else {
            v11 = mods[index] + v10;
        }

        if (stat == STAT_CURRENT_HIT_POINTS) {
            v12 = critterGetBaseStatWithTraitModifier(critter, STAT_CURRENT_HIT_POINTS);
            if (v11 + v12 <= 0 && critter != obj_dude) {
                name = critter_name(critter);
                // %s succumbs to the adverse effects of chems.
                text = getmsg(&item_message_file, &messageListItem, 600);
                sprintf(v24, text, name);
                combatKillCritterOutsideCombat(critter, v24);
            }
        }

        critterSetBonusStat(critter, stat, v11);

        if (critter == obj_dude) {
            if (stat == STAT_CURRENT_HIT_POINTS) {
                intface_update_hit_points(true);
            }

            int after = critterGetStat(critter, stat);
            if (after != before) {
                // 1 - You gained %d %s.
                // 2 - You lost %d %s.
                messageListItem.num = after < before ? 2 : 1;
                if (message_search(&item_message_file, &messageListItem)) {
                    char* statName = statGetName(stat);
                    sprintf(str, messageListItem.text, after < before ? before - after : after - before, statName);
                    display_print(str);
                    statsChanged = true;
                }
            }
        }
    }

    if (critterGetStat(critter, STAT_CURRENT_HIT_POINTS) > 0) {
        if (critter == obj_dude && !statsChanged && isImmediate) {
            // Nothing happens.
            messageListItem.num = 10;
            if (message_search(&item_message_file, &messageListItem)) {
                display_print(messageListItem.text);
            }
        }
    } else {
        if (critter == obj_dude) {
            // You suffer a fatal heart attack from chem overdose.
            messageListItem.num = 4;
            if (message_search(&item_message_file, &messageListItem)) {
                strcpy(v24, messageListItem.text);
                // TODO: Why message is ignored?
            }
        } else {
            name = critter_name(critter);
            // %s succumbs to the adverse effects of chems.
            text = getmsg(&item_message_file, &messageListItem, 600);
            sprintf(v24, text, name);
            // TODO: Why message is ignored?
        }
    }
}

// 0x479EE4
static bool drug_effect_allowed(Object* critter, int pid)
{
    int index;
    DrugDescription* drugDescription;
    for (index = 0; index < ADDICTION_COUNT; index++) {
        drugDescription = &(drugInfoList[index]);
        if (drugDescription->drugPid == pid) {
            break;
        }
    }

    if (index == ADDICTION_COUNT) {
        return true;
    }

    if (drugDescription->field_8 == 0) {
        return true;
    }

    // TODO: Probably right, but let's check it once.
    int count = 0;
    DrugEffectEvent* drugEffectEvent = (DrugEffectEvent*)queue_find_first(critter, EVENT_TYPE_DRUG);
    while (drugEffectEvent != NULL) {
        if (drugEffectEvent->drugPid == pid) {
            count++;
            if (count >= drugDescription->field_8) {
                return false;
            }
        }
        drugEffectEvent = (DrugEffectEvent*)queue_find_next(critter, EVENT_TYPE_DRUG);
    }

    return true;
}

// 0x479F60
int item_d_take_drug(Object* critter, Object* item)
{
    if (critter_is_dead(critter)) {
        return -1;
    }

    if (critter_body_type(critter) == BODY_TYPE_ROBOTIC) {
        return -1;
    }

    Proto* proto;
    proto_ptr(item->pid, &proto);

    if (item->pid == PROTO_ID_JET_ANTIDOTE) {
        if (item_d_check_addict(PROTO_ID_JET)) {
            perform_withdrawal_end(critter, PERK_JET_ADDICTION);

            if (critter == obj_dude) {
                // NOTE: Uninline.
                item_d_unset_addict(PROTO_ID_JET);
            }

            return 0;
        }
    }

    wd_obj = critter;
    wd_gvar = pid_to_gvar(item->pid);
    wd_onset = proto->item.data.drug.withdrawalOnset;

    queue_clear_type(EVENT_TYPE_WITHDRAWAL, item_wd_clear_all);

    if (drug_effect_allowed(critter, item->pid)) {
        perform_drug_effect(critter, proto->item.data.drug.stat, proto->item.data.drug.amount, true);
        insert_drug_effect(critter, item, proto->item.data.drug.duration1, proto->item.data.drug.stat, proto->item.data.drug.amount1);
        insert_drug_effect(critter, item, proto->item.data.drug.duration2, proto->item.data.drug.stat, proto->item.data.drug.amount2);
    } else {
        if (critter == obj_dude) {
            MessageListItem messageListItem;
            // That didn't seem to do that much.
            char* msg = getmsg(&item_message_file, &messageListItem, 50);
            display_print(msg);
        }
    }

    if (!item_d_check_addict(item->pid)) {
        int addictionChance = proto->item.data.drug.addictionChance;
        if (critter == obj_dude) {
            if (traitIsSelected(TRAIT_CHEM_RELIANT)) {
                addictionChance *= 2;
            }

            if (traitIsSelected(TRAIT_CHEM_RESISTANT)) {
                addictionChance /= 2;
            }

            if (perk_level(obj_dude, PERK_FLOWER_CHILD)) {
                addictionChance /= 2;
            }
        }

        if (roll_random(1, 100) <= addictionChance) {
            insert_withdrawal(critter, 1, proto->item.data.drug.withdrawalOnset, proto->item.data.drug.withdrawalEffect, item->pid);

            if (critter == obj_dude) {
                // NOTE: Uninline.
                item_d_set_addict(item->pid);
            }
        }
    }

    return 1;
}

// 0x47A178
int item_d_clear(Object* obj, void* data)
{
    if (isPartyMember(obj)) {
        return 0;
    }

    item_d_process(obj, data);

    return 1;
}

// 0x47A198
int item_d_process(Object* obj, void* data)
{
    DrugEffectEvent* drugEffectEvent = (DrugEffectEvent*)data;

    if (obj == NULL) {
        return 0;
    }

    if (PID_TYPE(obj->pid) != OBJ_TYPE_CRITTER) {
        return 0;
    }

    perform_drug_effect(obj, drugEffectEvent->stats, drugEffectEvent->modifiers, false);

    if (!(obj->data.critter.combat.results & DAM_DEAD)) {
        return 0;
    }

    return 1;
}

// 0x47A1D0
int item_d_load(File* stream, void** dataPtr)
{
    DrugEffectEvent* drugEffectEvent = (DrugEffectEvent*)internal_malloc(sizeof(*drugEffectEvent));
    if (drugEffectEvent == NULL) {
        return -1;
    }

    if (fileReadInt32List(stream, drugEffectEvent->stats, 3) == -1) goto err;
    if (fileReadInt32List(stream, drugEffectEvent->modifiers, 3) == -1) goto err;

    *dataPtr = drugEffectEvent;
    return 0;

err:

    internal_free(drugEffectEvent);
    return -1;
}

// 0x47A254
int item_d_save(File* stream, void* data)
{
    DrugEffectEvent* drugEffectEvent = (DrugEffectEvent*)data;

    if (fileWriteInt32List(stream, drugEffectEvent->stats, 3) == -1) return -1;
    if (fileWriteInt32List(stream, drugEffectEvent->modifiers, 3) == -1) return -1;

    return 0;
}

// 0x47A290
static int insert_withdrawal(Object* obj, int a2, int duration, int perk, int pid)
{
    WithdrawalEvent* withdrawalEvent = (WithdrawalEvent*)internal_malloc(sizeof(*withdrawalEvent));
    if (withdrawalEvent == NULL) {
        return -1;
    }

    withdrawalEvent->field_0 = a2;
    withdrawalEvent->pid = pid;
    withdrawalEvent->perk = perk;

    if (queue_add(600 * duration, obj, withdrawalEvent, EVENT_TYPE_WITHDRAWAL) == -1) {
        internal_free(withdrawalEvent);
        return -1;
    }

    return 0;
}

// 0x47A2FC
int item_wd_clear(Object* obj, void* data)
{
    WithdrawalEvent* withdrawalEvent = (WithdrawalEvent*)data;

    if (isPartyMember(obj)) {
        return 0;
    }

    if (!withdrawalEvent->field_0) {
        perform_withdrawal_end(obj, withdrawalEvent->perk);
    }

    return 1;
}

// 0x47A324
static int item_wd_clear_all(Object* a1, void* data)
{
    WithdrawalEvent* withdrawalEvent = (WithdrawalEvent*)data;

    if (a1 != wd_obj) {
        return 0;
    }

    if (pid_to_gvar(withdrawalEvent->pid) != wd_gvar) {
        return 0;
    }

    if (!withdrawalEvent->field_0) {
        perform_withdrawal_end(wd_obj, withdrawalEvent->perk);
    }

    insert_withdrawal(a1, 1, wd_onset, withdrawalEvent->perk, withdrawalEvent->pid);

    wd_obj = NULL;

    return 1;
}

// 0x47A384
int item_wd_process(Object* obj, void* data)
{
    WithdrawalEvent* withdrawalEvent = (WithdrawalEvent*)data;

    if (withdrawalEvent->field_0) {
        perform_withdrawal_start(obj, withdrawalEvent->perk, withdrawalEvent->pid);
    } else {
        if (withdrawalEvent->perk == PERK_JET_ADDICTION) {
            return 0;
        }

        perform_withdrawal_end(obj, withdrawalEvent->perk);

        if (obj == obj_dude) {
            // NOTE: Uninline.
            item_d_unset_addict(withdrawalEvent->pid);
        }
    }

    if (obj == obj_dude) {
        return 1;
    }

    return 0;
}

// 0x47A404
int item_wd_load(File* stream, void** dataPtr)
{
    WithdrawalEvent* withdrawalEvent = (WithdrawalEvent*)internal_malloc(sizeof(*withdrawalEvent));
    if (withdrawalEvent == NULL) {
        return -1;
    }

    if (fileReadInt32(stream, &(withdrawalEvent->field_0)) == -1) goto err;
    if (fileReadInt32(stream, &(withdrawalEvent->pid)) == -1) goto err;
    if (fileReadInt32(stream, &(withdrawalEvent->perk)) == -1) goto err;

    *dataPtr = withdrawalEvent;
    return 0;

err:

    internal_free(withdrawalEvent);
    return -1;
}

// 0x47A484
int item_wd_save(File* stream, void* data)
{
    WithdrawalEvent* withdrawalEvent = (WithdrawalEvent*)data;

    if (fileWriteInt32(stream, withdrawalEvent->field_0) == -1) return -1;
    if (fileWriteInt32(stream, withdrawalEvent->pid) == -1) return -1;
    if (fileWriteInt32(stream, withdrawalEvent->perk) == -1) return -1;

    return 0;
}

// 0x47A4C4
static void perform_withdrawal_start(Object* obj, int perk, int pid)
{
    if (PID_TYPE(obj->pid) != OBJ_TYPE_CRITTER) {
        debugPrint("\nERROR: perform_withdrawal_start: Was called on non-critter!");
        return;
    }

    perk_add_effect(obj, perk);

    if (obj == obj_dude) {
        char* description = perk_description(perk);
        display_print(description);
    }

    int duration = 10080;
    if (obj == obj_dude) {
        if (traitIsSelected(TRAIT_CHEM_RELIANT)) {
            duration /= 2;
        }

        if (perk_level(obj, PERK_FLOWER_CHILD)) {
            duration /= 2;
        }
    }

    insert_withdrawal(obj, 0, duration, perk, pid);
}

// 0x47A558
static void perform_withdrawal_end(Object* obj, int perk)
{
    if (PID_TYPE(obj->pid) != OBJ_TYPE_CRITTER) {
        debugPrint("\nERROR: perform_withdrawal_end: Was called on non-critter!");
        return;
    }

    perk_remove_effect(obj, perk);

    if (obj == obj_dude) {
        MessageListItem messageListItem;
        messageListItem.num = 3;
        if (message_search(&item_message_file, &messageListItem)) {
            display_print(messageListItem.text);
        }
    }
}

// 0x47A5B4
static int pid_to_gvar(int drugPid)
{
    for (int index = 0; index < ADDICTION_COUNT; index++) {
        DrugDescription* drugDescription = &(drugInfoList[index]);
        if (drugDescription->drugPid == drugPid) {
            return drugDescription->gvar;
        }
    }

    return -1;
}

// NOTE: Inlined.
//
// 0x47A5E8
void item_d_set_addict(int drugPid)
{
    int gvar = pid_to_gvar(drugPid);
    if (gvar != -1) {
        game_global_vars[gvar] = 1;
    }

    pc_flag_on(DUDE_STATE_ADDICTED);
}

// NOTE: Inlined.
//
// 0x47A60C
void item_d_unset_addict(int drugPid)
{
    int gvar = pid_to_gvar(drugPid);
    if (gvar != -1) {
        game_global_vars[gvar] = 0;
    }

    if (!item_d_check_addict(-1)) {
        pc_flag_off(DUDE_STATE_ADDICTED);
    }
}

// Returns `true` if dude has addiction to item with given pid or any addition
// if [pid] is -1.
//
// 0x47A640
bool item_d_check_addict(int drugPid)
{
    for (int index = 0; index < ADDICTION_COUNT; index++) {
        DrugDescription* drugDescription = &(drugInfoList[index]);
        if (drugPid == -1 || drugPid == drugDescription->drugPid) {
            if (game_global_vars[drugDescription->gvar] != 0) {
                return true;
            } else {
                return false;
            }
        }
    }

    return false;
}

// item_caps_total
// 0x47A6A8
int item_caps_total(Object* obj)
{
    int amount = 0;

    Inventory* inventory = &(obj->data.inventory);
    for (int i = 0; i < inventory->length; i++) {
        InventoryItem* inventoryItem = &(inventory->items[i]);
        Object* item = inventoryItem->item;

        if (item->pid == PROTO_ID_MONEY) {
            amount += inventoryItem->quantity;
        } else {
            if (item_get_type(item) == ITEM_TYPE_CONTAINER) {
                // recursively collect amount of caps in container
                amount += item_caps_total(item);
            }
        }
    }

    return amount;
}

// item_caps_adjust
// 0x47A6F8
int item_caps_adjust(Object* obj, int amount)
{
    int caps = item_caps_total(obj);
    if (amount < 0 && caps < -amount) {
        return -1;
    }

    if (amount <= 0 || caps != 0) {
        Inventory* inventory = &(obj->data.inventory);

        for (int index = 0; index < inventory->length && amount != 0; index++) {
            InventoryItem* inventoryItem = &(inventory->items[index]);
            Object* item = inventoryItem->item;
            if (item->pid == PROTO_ID_MONEY) {
                if (amount <= 0 && -amount >= inventoryItem->quantity) {
                    obj_erase_object(item, NULL);

                    amount += inventoryItem->quantity;

                    // NOTE: Uninline.
                    item_compact(index, inventory);

                    index = -1;
                } else {
                    inventoryItem->quantity += amount;
                    amount = 0;
                }
            }
        }

        for (int index = 0; index < inventory->length && amount != 0; index++) {
            InventoryItem* inventoryItem = &(inventory->items[index]);
            Object* item = inventoryItem->item;
            if (item_get_type(item) == ITEM_TYPE_CONTAINER) {
                int capsInContainer = item_caps_total(item);
                if (amount <= 0 || capsInContainer <= 0) {
                    if (amount < 0) {
                        if (capsInContainer < -amount) {
                            if (item_caps_adjust(item, capsInContainer) == 0) {
                                amount += capsInContainer;
                            }
                        } else {
                            if (item_caps_adjust(item, amount) == 0) {
                                amount = 0;
                            }
                        }
                    }
                } else {
                    if (item_caps_adjust(item, amount) == 0) {
                        amount = 0;
                    }
                }
            }
        }

        return 0;
    }

    Object* item;
    if (obj_pid_new(&item, PROTO_ID_MONEY) == 0) {
        obj_disconnect(item, NULL);
        if (item_add_force(obj, item, amount) != 0) {
            obj_erase_object(item, NULL);
            return -1;
        }
    }

    return 0;
}

// 0x47A8C8
int item_caps_get_amount(Object* item)
{
    if (item->pid != PROTO_ID_MONEY) {
        return -1;
    }

    return item->data.item.misc.charges;
}

// 0x47A8D8
int item_caps_set_amount(Object* item, int amount)
{
    if (item->pid != PROTO_ID_MONEY) {
        return -1;
    }

    item->data.item.misc.charges = amount;

    return 0;
}
