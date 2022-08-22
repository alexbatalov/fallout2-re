#ifndef ANIMATION_H
#define ANIMATION_H

#include "art.h"
#include "combat_defs.h"
#include "obj_types.h"

#include <stdbool.h>

#define ANIMATION_SEQUENCE_LIST_CAPACITY 32
#define ANIMATION_DESCRIPTION_LIST_CAPACITY 55
#define ANIMATION_SAD_LIST_CAPACITY 24

#define ANIMATION_SEQUENCE_FORCED 0x01

typedef enum AnimationKind {
    ANIM_KIND_MOVE_TO_OBJECT = 0,
    ANIM_KIND_MOVE_TO_TILE = 1,
    ANIM_KIND_MOVE_TO_TILE_STRAIGHT = 2,
    ANIM_KIND_MOVE_TO_TILE_STRAIGHT_AND_WAIT_FOR_COMPLETE = 3,
    ANIM_KIND_ANIMATE = 4,
    ANIM_KIND_ANIMATE_REVERSED = 5,
    ANIM_KIND_ANIMATE_AND_HIDE = 6,
    ANIM_KIND_ROTATE_TO_TILE = 7,
    ANIM_KIND_ROTATE_CLOCKWISE = 8,
    ANIM_KIND_ROTATE_COUNTER_CLOCKWISE = 9,
    ANIM_KIND_HIDE = 10,
    ANIM_KIND_CALLBACK = 11,
    ANIM_KIND_CALLBACK3 = 12,
    ANIM_KIND_SET_FLAG = 14,
    ANIM_KIND_UNSET_FLAG = 15,
    ANIM_KIND_TOGGLE_FLAT = 16,
    ANIM_KIND_SET_FID = 17,
    ANIM_KIND_TAKE_OUT_WEAPON = 18,
    ANIM_KIND_SET_LIGHT_DISTANCE = 19,
    ANIM_KIND_20 = 20,
    ANIM_KIND_23 = 23,
    ANIM_KIND_TOGGLE_OUTLINE = 24,
    ANIM_KIND_ANIMATE_FOREVER = 25,
    ANIM_KIND_26 = 26,
    ANIM_KIND_27 = 27,
    ANIM_KIND_NOOP = 28,
} AnimationKind;

typedef enum AnimationRequestOptions {
    ANIMATION_REQUEST_UNRESERVED = 0x01,
    ANIMATION_REQUEST_RESERVED = 0x02,
    ANIMATION_REQUEST_NO_STAND = 0x04,
    ANIMATION_REQUEST_0x100 = 0x100,
    ANIMATION_REQUEST_INSIGNIFICANT = 0x200,
} AnimationRequestOptions;

typedef enum AnimationSequenceFlags {
    // Specifies that the animation sequence has high priority, it cannot be
    // cleared.
    ANIM_SEQ_PRIORITIZED = 0x01,

    // Specifies that the animation sequence started combat animation mode and
    // therefore should balance it with appropriate finish call.
    ANIM_SEQ_COMBAT_ANIM_STARTED = 0x02,

    // Specifies that the animation sequence is reserved (TODO: explain what it
    // actually means).
    ANIM_SEQ_RESERVED = 0x04,

    // Specifies that the animation sequence is in the process of adding actions
    // to it (that is in the middle of begin/end calls).
    ANIM_SEQ_ACCUMULATING = 0x08,

    // TODO: Add description.
    ANIM_SEQ_0x10 = 0x10,

    // TODO: Add description.
    ANIM_SEQ_0x20 = 0x20,

    // Specifies that the animation sequence is negligible and will be end
    // immediately when a new animation sequence is requested for the same
    // object.
    ANIM_SEQ_INSIGNIFICANT = 0x40,

    // Specifies that the animation sequence should not return to ANIM_STAND
    // when it's completed.
    ANIM_SEQ_NO_STAND = 0x80,
} AnimationSequenceFlags;

typedef enum AnimationSadFlags {
    // Specifies that the animation should play from end to start.
    ANIM_SAD_REVERSE = 0x01,

    // Specifies that the animation should use straight movement mode (as
    // opposed to normal movement mode).
    ANIM_SAD_STRAIGHT = 0x02,

    // Specifies that no frame change should occur during animation.
    ANIM_SAD_NO_ANIM = 0x04,

    // Specifies that the animation should be played fully from start to finish.
    //
    // NOTE: This flag is only used together with straight movement mode to
    // implement knockdown. Without this flag when the knockdown distance is
    // short, say 1 or 2 tiles, knockdown animation might not be completed by
    // the time critter reached it's destination. With this flag set animation
    // will be played to it's final frame.
    ANIM_SAD_WAIT_FOR_COMPLETION = 0x10,

    // Unknown, set once, never read.
    ANIM_SAD_0x20 = 0x20,

    // Specifies that the owner of the animation should be hidden when animation
    // is completed.
    ANIM_SAD_HIDE_ON_END = 0x40,

    // Specifies that the animation should never end.
    ANIM_SAD_FOREVER = 0x80,
} AnimationSadFlags;

// Basic animations: 0-19
// Knockdown and death: 20-35
// Change positions: 36-37
// Weapon: 38-47
// Single-frame death animations (the last frame of knockdown and death animations): 48-63
typedef enum AnimationType {
    ANIM_STAND = 0,
    ANIM_WALK = 1,
    ANIM_JUMP_BEGIN = 2,
    ANIM_JUMP_END = 3,
    ANIM_CLIMB_LADDER = 4,
    ANIM_FALLING = 5,
    ANIM_UP_STAIRS_RIGHT = 6,
    ANIM_UP_STAIRS_LEFT = 7,
    ANIM_DOWN_STAIRS_RIGHT = 8,
    ANIM_DOWN_STAIRS_LEFT = 9,
    ANIM_MAGIC_HANDS_GROUND = 10,
    ANIM_MAGIC_HANDS_MIDDLE = 11,
    ANIM_MAGIC_HANDS_UP = 12,
    ANIM_DODGE_ANIM = 13,
    ANIM_HIT_FROM_FRONT = 14,
    ANIM_HIT_FROM_BACK = 15,
    ANIM_THROW_PUNCH = 16,
    ANIM_KICK_LEG = 17,
    ANIM_THROW_ANIM = 18,
    ANIM_RUNNING = 19,
    ANIM_FALL_BACK = 20,
    ANIM_FALL_FRONT = 21,
    ANIM_BAD_LANDING = 22,
    ANIM_BIG_HOLE = 23,
    ANIM_CHARRED_BODY = 24,
    ANIM_CHUNKS_OF_FLESH = 25,
    ANIM_DANCING_AUTOFIRE = 26,
    ANIM_ELECTRIFY = 27,
    ANIM_SLICED_IN_HALF = 28,
    ANIM_BURNED_TO_NOTHING = 29,
    ANIM_ELECTRIFIED_TO_NOTHING = 30,
    ANIM_EXPLODED_TO_NOTHING = 31,
    ANIM_MELTED_TO_NOTHING = 32,
    ANIM_FIRE_DANCE = 33,
    ANIM_FALL_BACK_BLOOD = 34,
    ANIM_FALL_FRONT_BLOOD = 35,
    ANIM_PRONE_TO_STANDING = 36,
    ANIM_BACK_TO_STANDING = 37,
    ANIM_TAKE_OUT = 38,
    ANIM_PUT_AWAY = 39,
    ANIM_PARRY_ANIM = 40,
    ANIM_THRUST_ANIM = 41,
    ANIM_SWING_ANIM = 42,
    ANIM_POINT = 43,
    ANIM_UNPOINT = 44,
    ANIM_FIRE_SINGLE = 45,
    ANIM_FIRE_BURST = 46,
    ANIM_FIRE_CONTINUOUS = 47,
    ANIM_FALL_BACK_SF = 48,
    ANIM_FALL_FRONT_SF = 49,
    ANIM_BAD_LANDING_SF = 50,
    ANIM_BIG_HOLE_SF = 51,
    ANIM_CHARRED_BODY_SF = 52,
    ANIM_CHUNKS_OF_FLESH_SF = 53,
    ANIM_DANCING_AUTOFIRE_SF = 54,
    ANIM_ELECTRIFY_SF = 55,
    ANIM_SLICED_IN_HALF_SF = 56,
    ANIM_BURNED_TO_NOTHING_SF = 57,
    ANIM_ELECTRIFIED_TO_NOTHING_SF = 58,
    ANIM_EXPLODED_TO_NOTHING_SF = 59,
    ANIM_MELTED_TO_NOTHING_SF = 60,
    ANIM_FIRE_DANCE_SF = 61,
    ANIM_FALL_BACK_BLOOD_SF = 62,
    ANIM_FALL_FRONT_BLOOD_SF = 63,
    ANIM_CALLED_SHOT_PIC = 64,
    ANIM_COUNT = 65,
    FIRST_KNOCKDOWN_AND_DEATH_ANIM = ANIM_FALL_BACK,
    LAST_KNOCKDOWN_AND_DEATH_ANIM = ANIM_FALL_FRONT_BLOOD,
    FIRST_SF_DEATH_ANIM = ANIM_FALL_BACK_SF,
    LAST_SF_DEATH_ANIM = ANIM_FALL_FRONT_BLOOD_SF,
} AnimationType;

#define FID_ANIM_TYPE(value) ((value) & 0xFF0000) >> 16

// Signature of animation callback accepting 2 parameters.
typedef int AnimationCallback(void*, void*);

// Signature of animation callback accepting 3 parameters.
typedef int AnimationCallback3(void*, void*, void*);

typedef struct AnimationDescription {
    int kind;
    union {
        Object* owner;

        // - ANIM_KIND_CALLBACK
        // - ANIM_KIND_CALLBACK3
        void* param2;
    };

    union {
        // - ANIM_KIND_MOVE_TO_OBJECT
        Object* destination;

        // - ANIM_KIND_CALLBACK
        void* param1;
    };
    union {
        // - ANIM_KIND_MOVE_TO_TILE
        // - ANIM_KIND_ANIMATE_AND_MOVE_TO_TILE_STRAIGHT
        // - ANIM_KIND_MOVE_TO_TILE_STRAIGHT
        struct {
            int tile;
            int elevation;
        };

        // ANIM_KIND_SET_FID
        int fid;

        // ANIM_KIND_TAKE_OUT_WEAPON
        int weaponAnimationCode;

        // ANIM_KIND_SET_LIGHT_DISTANCE
        int lightDistance;

        // ANIM_KIND_TOGGLE_OUTLINE
        bool outline;
    };
    int anim;
    int delay;

    // ANIM_KIND_CALLBACK
    AnimationCallback* callback;

    // ANIM_KIND_CALLBACK3
    AnimationCallback3* callback3;

    union {
        // - ANIM_KIND_SET_FLAG
        // - ANIM_KIND_UNSET_FLAG
        unsigned int objectFlag;

        // - ANIM_KIND_HIDE
        // - ANIM_KIND_CALLBACK
        unsigned int extendedFlags;
    };

    union {
        // - ANIM_KIND_MOVE_TO_TILE
        // - ANIM_KIND_MOVE_TO_OBJECT
        int actionPoints;

        // ANIM_KIND_26
        int animationSequenceIndex;

        // ANIM_KIND_CALLBACK3
        void* param3;
    };
    CacheEntry* artCacheKey;
} AnimationDescription;

typedef struct AnimationSequence {
    int field_0;
    // Index of current animation in [animations] array or -1 if animations in
    // this sequence is not playing.
    int animationIndex;
    // Number of scheduled animations in [animations] array.
    int length;
    unsigned int flags;
    AnimationDescription animations[ANIMATION_DESCRIPTION_LIST_CAPACITY];
} AnimationSequence;

typedef struct PathNode {
    int tile;
    int from;
    // actual type is likely char
    int rotation;
    int field_C;
    int field_10;
} PathNode;

typedef struct STRUCT_530014_28 {
    int tile;
    int elevation;
    int x;
    int y;
} STRUCT_530014_28;

// TODO: I don't know what `sad` means, but it's definitely better than
// `STRUCT_530014`. Find a better name.
typedef struct AnimationSad {
    unsigned int flags;
    Object* obj;
    int fid; // fid
    int anim;

    // Timestamp (in game ticks) when animation last occurred.
    unsigned int animationTimestamp;

    // Number of ticks per frame (taking art's fps and overall animation speed
    // settings into account).
    unsigned int ticksPerFrame;

    int animationSequenceIndex;
    int field_1C; // length of field_28
    int field_20; // current index in field_28
    int field_24;
    union {
        unsigned char rotations[3200];
        STRUCT_530014_28 field_28[200];
    };
} AnimationSad;

static_assert(sizeof(AnimationSad) == 3240, "wrong size");

typedef Object* PathBuilderCallback(Object* object, int tile, int elevation);

extern int gAnimationCurrentSad;
extern int gAnimationSequenceCurrentIndex;
extern bool gAnimationInInit;
extern bool gAnimationInStop;
extern bool _anim_in_bk;
extern int _lastDestination;
extern unsigned int _last_time_;
extern unsigned int _next_time;

extern AnimationSad gAnimationSads[ANIMATION_SAD_LIST_CAPACITY];
extern PathNode gClosedPathNodeList[2000];
extern AnimationSequence gAnimationSequences[32];
extern unsigned char gPathfinderProcessedTiles[5000];
extern PathNode gOpenPathNodeList[2000];
extern int gAnimationDescriptionCurrentIndex;
extern Object* dword_56C7E0[100];

void animationInit();
void animationReset();
void animationExit();
int reg_anim_begin(int a1);
int _anim_free_slot(int a1);
int _register_priority(int a1);
int reg_anim_clear(Object* a1);
int reg_anim_end();
int _anim_preload(Object* object, int fid, CacheEntry** cacheEntryPtr);
void _anim_cleanup();
int _check_registry(Object* obj);
int animationIsBusy(Object* a1);
int animationRegisterMoveToObject(Object* owner, Object* destination, int actionPoints, int delay);
int animationRegisterRunToObject(Object* owner, Object* destination, int actionPoints, int delay);
int animationRegisterMoveToTile(Object* owner, int tile, int elevation, int actionPoints, int delay);
int animationRegisterRunToTile(Object* owner, int tile, int elevation, int actionPoints, int delay);
int animationRegisterMoveToTileStraight(Object* object, int tile, int elevation, int anim, int delay);
int animationRegisterMoveToTileStraightAndWaitForComplete(Object* owner, int tile, int elev, int anim, int delay);
int animationRegisterAnimate(Object* owner, int anim, int delay);
int animationRegisterAnimateReversed(Object* owner, int anim, int delay);
int animationRegisterAnimateAndHide(Object* owner, int anim, int delay);
int animationRegisterRotateToTile(Object* owner, int tile);
int animationRegisterRotateClockwise(Object* owner);
int animationRegisterRotateCounterClockwise(Object* owner);
int animationRegisterHideObject(Object* object);
int animationRegisterHideObjectForced(Object* object);
int animationRegisterCallback(void* a1, void* a2, AnimationCallback* proc, int delay);
int animationRegisterCallback3(void* a1, void* a2, void* a3, AnimationCallback3* proc, int delay);
int animationRegisterCallbackForced(void* a1, void* a2, AnimationCallback* proc, int delay);
int animationRegisterSetFlag(Object* object, int flag, int delay);
int animationRegisterUnsetFlag(Object* object, int flag, int delay);
int animationRegisterSetFid(Object* owner, int fid, int delay);
int animationRegisterTakeOutWeapon(Object* owner, int weaponAnimationCode, int delay);
int animationRegisterSetLightDistance(Object* owner, int lightDistance, int delay);
int animationRegisterToggleOutline(Object* object, bool outline, int delay);
int animationRegisterPlaySoundEffect(Object* owner, const char* soundEffectName, int delay);
int animationRegisterAnimateForever(Object* owner, int anim, int delay);
int reg_anim_26(int a1, int a2);
int animationRunSequence(int a1);
int _anim_set_continue(int a1, int a2);
int _anim_set_end(int a1);
bool canUseDoor(Object* critter, Object* door);
int _make_path(Object* object, int from, int to, unsigned char* a4, int a5);
int pathfinderFindPath(Object* object, int from, int to, unsigned char* rotations, int a5, PathBuilderCallback* callback);
int _idist(int a1, int a2, int a3, int a4);
int _tile_idistance(int tile1, int tile2);
int _make_straight_path(Object* a1, int from, int to, STRUCT_530014_28* pathNodes, Object** a5, int a6);
int _make_straight_path_func(Object* a1, int from, int to, STRUCT_530014_28* a4, Object** a5, int a6, Object* (*a7)(Object*, int, int));
int animateMoveObjectToObject(Object* from, Object* to, int a3, int anim, int animationSequenceIndex);
int _make_stair_path(Object* object, int from, int fromElevation, int to, int toElevation, STRUCT_530014_28* a6, Object** obstaclePtr);
int animateMoveObjectToTile(Object* obj, int tile_num, int elev, int a4, int anim, int animationSequenceIndex);
int _anim_move(Object* obj, int tile, int elev, int a3, int anim, int a5, int animationSequenceIndex);
int animateMoveObjectToTileStraight(Object* obj, int tile, int elevation, int anim, int animationSequenceIndex, int flags);
int _anim_move_on_stairs(Object* obj, int tile, int elevation, int anim, int animationSequenceIndex);
int _check_for_falling(Object* obj, int anim, int a3);
void _object_move(int index);
void _object_straight_move(int index);
int _anim_animate(Object* obj, int anim, int animationSequenceIndex, int flags);
void _object_animate();
void _object_anim_compact();
int _check_move(int* a1);
int _dude_move(int a1);
int _dude_run(int a1);
void _dude_fidget();
void _dude_stand(Object* obj, int rotation, int fid);
void _dude_standup(Object* a1);
int actionRotate(Object* obj, int delta, int animationSequenceIndex);
int _anim_change_fid(Object* obj, int animationSequenceIndex, int fid);
void animationStop();
int _check_gravity(int tile, int elevation);
unsigned int animationComputeTicksPerFrame(Object* object, int fid);

#endif /* ANIMATION_H */
