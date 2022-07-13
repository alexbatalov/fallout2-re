#include "art.h"

#include "animation.h"
#include "debug.h"
#include "draw.h"
#include "game.h"
#include "game_config.h"
#include "memory.h"
#include "object.h"
#include "proto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 0x510738
ArtListDescription gArtListDescriptions[OBJ_TYPE_COUNT] = {
    { 0, "items", 0, 0, 0 },
    { 0, "critters", 0, 0, 0 },
    { 0, "scenery", 0, 0, 0 },
    { 0, "walls", 0, 0, 0 },
    { 0, "tiles", 0, 0, 0 },
    { 0, "misc", 0, 0, 0 },
    { 0, "intrface", 0, 0, 0 },
    { 0, "inven", 0, 0, 0 },
    { 0, "heads", 0, 0, 0 },
    { 0, "backgrnd", 0, 0, 0 },
    { 0, "skilldex", 0, 0, 0 },
};

// This flag denotes that localized arts should be looked up first. Used
// together with [gArtLanguage].
//
// 0x510898
bool gArtLanguageInitialized = false;

// 0x51089C
const char* _head1 = "gggnnnbbbgnb";

// 0x5108A0
const char* _head2 = "vfngfbnfvppp";

// Current native look base fid.
//
// 0x5108A4
int _art_vault_guy_num = 0;

// Base fids for unarmored dude.
//
// Outfit file names:
// - tribal: "hmwarr", "hfprim"
// - jumpsuit: "hmjmps", "hfjmps"
//
// NOTE: This value could have been done with two separate arrays - one for
// tribal look, and one for jumpsuit look. However in this case it would have
// been accessed differently in 0x49F984, which clearly uses look type as an
// index, not gender.
//
// 0x5108A8
int _art_vault_person_nums[DUDE_NATIVE_LOOK_COUNT][GENDER_COUNT];

// Index of "grid001.frm" in tiles.lst.
//
// 0x5108B8
int _art_mapper_blank_tile = 1;

// Non-english language name.
//
// This value is used as a directory name to display localized arts.
//
// 0x56C970
char gArtLanguage[32];

// 0x56C990
Cache gArtCache;

// 0x56C9E4
char _art_name[MAX_PATH];

// head_info
// 0x56CAE8
HeadDescription* gHeadDescriptions;

// anon_alias
// 0x56CAEC
int* _anon_alias;

// artCritterFidShouldRunData
// 0x56CAF0
int* gArtCritterFidShoudRunData;

// 0x418840
int artInit()
{
    char path[MAX_PATH];
    int i;
    File* stream;
    char str[200];
    char *ptr, *curr;

    int cacheSize;
    if (!configGetInt(&gGameConfig, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_ART_CACHE_SIZE_KEY, &cacheSize)) {
        cacheSize = 8;
    }

    if (!cacheInit(&gArtCache, artCacheGetFileSizeImpl, artCacheReadDataImpl, artCacheFreeImpl, cacheSize << 20)) {
        debugPrint("cache_init failed in art_init\n");
        return -1;
    }

    char* language;
    if (configGetString(&gGameConfig, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_LANGUAGE_KEY, &language) && stricmp(language, ENGLISH) != 0) {
        strcpy(gArtLanguage, language);
        gArtLanguageInitialized = true;
    }

    for (i = 0; i < 11; i++) {
        gArtListDescriptions[i].flags = 0;
        sprintf(path, "%s%s%s\\%s.lst", _cd_path_base, "art\\", gArtListDescriptions[i].name, gArtListDescriptions[i].name);

        if (artReadList(path, &(gArtListDescriptions[i].fileNames), &(gArtListDescriptions[i].fileNamesLength)) != 0) {
            debugPrint("art_read_lst failed in art_init\n");
            cacheFree(&gArtCache);
            return -1;
        }
    }

    _anon_alias = (int*)internal_malloc(sizeof(*_anon_alias) * gArtListDescriptions[1].fileNamesLength);
    if (_anon_alias == NULL) {
        gArtListDescriptions[1].fileNamesLength = 0;
        debugPrint("Out of memory for anon_alias in art_init\n");
        cacheFree(&gArtCache);
        return -1;
    }

    gArtCritterFidShoudRunData = (int*)internal_malloc(sizeof(*gArtCritterFidShoudRunData) * gArtListDescriptions[1].fileNamesLength);
    if (gArtCritterFidShoudRunData == NULL) {
        gArtListDescriptions[1].fileNamesLength = 0;
        debugPrint("Out of memory for artCritterFidShouldRunData in art_init\n");
        cacheFree(&gArtCache);
        return -1;
    }

    for (i = 0; i < gArtListDescriptions[1].fileNamesLength; i++) {
        gArtCritterFidShoudRunData[i] = 0;
    }

    sprintf(path, "%s%s%s\\%s.lst", _cd_path_base, "art\\", gArtListDescriptions[1].name, gArtListDescriptions[1].name);

    stream = fileOpen(path, "rt");
    if (stream == NULL) {
        debugPrint("Unable to open %s in art_init\n", path);
        cacheFree(&gArtCache);
        return -1;
    }

    ptr = gArtListDescriptions[1].fileNames;
    for (i = 0; i < gArtListDescriptions[1].fileNamesLength; i++) {
        if (stricmp(ptr, "hmjmps") == 0) {
            _art_vault_person_nums[DUDE_NATIVE_LOOK_JUMPSUIT][GENDER_MALE] = i;
        } else if (stricmp(ptr, "hfjmps") == 0) {
            _art_vault_person_nums[DUDE_NATIVE_LOOK_JUMPSUIT][GENDER_FEMALE] = i;
        }

        if (stricmp(ptr, "hmwarr") == 0) {
            _art_vault_person_nums[DUDE_NATIVE_LOOK_TRIBAL][GENDER_MALE] = i;
            _art_vault_guy_num = i;
        } else if (stricmp(ptr, "hfprim") == 0) {
            _art_vault_person_nums[DUDE_NATIVE_LOOK_TRIBAL][GENDER_FEMALE] = i;
        }

        ptr += 13;
    }

    for (i = 0; i < gArtListDescriptions[1].fileNamesLength; i++) {
        if (!fileReadString(str, sizeof(str), stream)) {
            break;
        }

        ptr = str;
        curr = ptr;
        while (*curr != '\0' && *curr != ',') {
            curr++;
        }

        if (*curr != '\0') {
            _anon_alias[i] = atoi(curr + 1);

            ptr = curr + 1;
            curr = ptr;
            while (*curr != '\0' && *curr != ',') {
                curr++;
            }

            gArtCritterFidShoudRunData[i] = *curr != '\0' ? atoi(ptr) : 0;
        } else {
            _anon_alias[i] = _art_vault_guy_num;
            gArtCritterFidShoudRunData[i] = 1;
        }
    }

    fileClose(stream);

    ptr = gArtListDescriptions[4].fileNames;
    for (i = 0; i < gArtListDescriptions[4].fileNamesLength; i++) {
        if (stricmp(ptr, "grid001.frm") == 0) {
            _art_mapper_blank_tile = i;
        }
    }

    gHeadDescriptions = (HeadDescription*)internal_malloc(sizeof(HeadDescription) * gArtListDescriptions[8].fileNamesLength);
    if (gHeadDescriptions == NULL) {
        gArtListDescriptions[8].fileNamesLength = 0;
        debugPrint("Out of memory for head_info in art_init\n");
        cacheFree(&gArtCache);
        return -1;
    }

    sprintf(path, "%s%s%s\\%s.lst", _cd_path_base, "art\\", gArtListDescriptions[8].name, gArtListDescriptions[8].name);

    stream = fileOpen(path, "rt");
    if (stream == NULL) {
        debugPrint("Unable to open %s in art_init\n", path);
        cacheFree(&gArtCache);
        return -1;
    }

    for (i = 0; i < gArtListDescriptions[8].fileNamesLength; i++) {
        if (!fileReadString(str, sizeof(str), stream)) {
            break;
        }

        ptr = str;
        curr = ptr;
        while (*curr != '\0' && *curr != ',') {
            curr++;
        }

        if (*curr != '\0') {
            ptr = curr + 1;
            curr = ptr;
            while (*curr != '\0' && *curr != ',') {
                curr++;
            }

            if (*curr != '\0') {
                gHeadDescriptions[i].goodFidgetCount = atoi(ptr);

                ptr = curr + 1;
                curr = ptr;
                while (*curr != '\0' && *curr != ',') {
                    curr++;
                }

                if (*curr != '\0') {
                    gHeadDescriptions[i].neutralFidgetCount = atoi(ptr);

                    ptr = curr + 1;
                    curr = strpbrk(ptr, " ,;\t\n");
                    if (curr != NULL) {
                        *curr = '\0';
                    }

                    gHeadDescriptions[i].badFidgetCount = atoi(ptr);
                }
            }
        }
    }

    fileClose(stream);

    return 0;
}

// 0x418EB8
void artReset()
{
}

// 0x418EBC
void artExit()
{
    cacheFree(&gArtCache);

    internal_free(_anon_alias);
    internal_free(gArtCritterFidShoudRunData);

    for (int index = 0; index < OBJ_TYPE_COUNT; index++) {
        internal_free(gArtListDescriptions[index].fileNames);
        gArtListDescriptions[index].fileNames = NULL;

        internal_free(gArtListDescriptions[index].field_18);
        gArtListDescriptions[index].field_18 = NULL;
    }

    internal_free(gHeadDescriptions);
}

// 0x418F1C
char* artGetObjectTypeName(int objectType)
{
    return objectType >= 0 && objectType < OBJ_TYPE_COUNT ? gArtListDescriptions[objectType].name : NULL;
}

// 0x418F34
int artIsObjectTypeHidden(int objectType)
{
    return objectType >= 0 && objectType < OBJ_TYPE_COUNT ? gArtListDescriptions[objectType].flags & 1 : 0;
}

// 0x418F7C
int artGetFidgetCount(int headFid)
{
    if (OBJECT_TYPE(headFid) != OBJ_TYPE_HEAD) {
        return 0;
    }

    int head = headFid & 0xFFF;

    if (head > gArtListDescriptions[OBJ_TYPE_HEAD].fileNamesLength) {
        return 0;
    }

    HeadDescription* headDescription = &(gHeadDescriptions[head]);

    int fidget = (headFid & 0xFF0000) >> 16;
    switch (fidget) {
    case FIDGET_GOOD:
        return headDescription->goodFidgetCount;
    case FIDGET_NEUTRAL:
        return headDescription->neutralFidgetCount;
    case FIDGET_BAD:
        return headDescription->badFidgetCount;
    }
    return 0;
}

// 0x418FFC
void artRender(int fid, unsigned char* dest, int width, int height, int pitch)
{
    // NOTE: Original code is different. For unknown reason it directly calls
    // many art functions, for example instead of [artLock] it calls lower level
    // [cacheLock], instead of [artGetWidth] is calls [artGetFrame], then get
    // width from frame's struct field. I don't know if this was intentional or
    // not. I've replaced these calls with higher level functions where
    // appropriate.

    CacheEntry* handle;
    Art* frm = artLock(fid, &handle);
    if (frm == NULL) {
        return;
    }

    unsigned char* frameData = artGetFrameData(frm, 0, 0);
    int frameWidth = artGetWidth(frm, 0, 0);
    int frameHeight = artGetHeight(frm, 0, 0);

    int remainingWidth = width - frameWidth;
    int remainingHeight = height - frameHeight;
    if (remainingWidth < 0 || remainingHeight < 0) {
        if (height * frameWidth >= width * frameHeight) {
            blitBufferToBufferStretchTrans(frameData,
                frameWidth,
                frameHeight,
                frameWidth,
                dest + pitch * ((height - width * frameHeight / frameWidth) / 2),
                width,
                width * frameHeight / frameWidth,
                pitch);
        } else {
            blitBufferToBufferStretchTrans(frameData,
                frameWidth,
                frameHeight,
                frameWidth,
                dest + (width - height * frameWidth / frameHeight) / 2,
                height * frameWidth / frameHeight,
                height,
                pitch);
        }
    } else {
        blitBufferToBufferTrans(frameData,
            frameWidth,
            frameHeight,
            frameWidth,
            dest + pitch * (remainingHeight / 2) + remainingWidth / 2,
            pitch);
    }

    artUnlock(handle);
}

// 0x419160
Art* artLock(int fid, CacheEntry** handlePtr)
{
    if (handlePtr == NULL) {
        return NULL;
    }

    Art* art = NULL;
    cacheLock(&gArtCache, fid, (void**)&art, handlePtr);
    return art;
}

// 0x419188
unsigned char* artLockFrameData(int fid, int frame, int direction, CacheEntry** handlePtr)
{
    Art* art;
    ArtFrame* frm;

    art = NULL;
    if (handlePtr) {
        cacheLock(&gArtCache, fid, (void**)&art, handlePtr);
    }

    if (art != NULL) {
        frm = artGetFrame(art, frame, direction);
        if (frm != NULL) {

            return (unsigned char*)frm + sizeof(*frm);
        }
    }

    return NULL;
}

// 0x4191CC
unsigned char* artLockFrameDataReturningSize(int fid, CacheEntry** handlePtr, int* widthPtr, int* heightPtr)
{
    *handlePtr = NULL;

    Art* art;
    cacheLock(&gArtCache, fid, (void**)&art, handlePtr);

    if (art == NULL) {
        return NULL;
    }

    // NOTE: Uninline.
    *widthPtr = artGetWidth(art, 0, 0);
    if (*widthPtr == -1) {
        return NULL;
    }

    // NOTE: Uninline.
    *heightPtr = artGetHeight(art, 0, 0);
    if (*heightPtr == -1) {
        return NULL;
    }

    // NOTE: Uninline.
    return artGetFrameData(art, 0, 0);
}

// 0x419260
int artUnlock(CacheEntry* handle)
{
    return cacheUnlock(&gArtCache, handle);
}

// 0x41927C
int artCacheFlush()
{
    return cacheFlush(&gArtCache);
}

// 0x4192B0
int artCopyFileName(int objectType, int id, char* dest)
{
    ArtListDescription* ptr;

    if (objectType < OBJ_TYPE_ITEM && objectType >= OBJ_TYPE_COUNT) {
        return -1;
    }

    ptr = &(gArtListDescriptions[objectType]);

    if (id >= ptr->fileNamesLength) {
        return -1;
    }

    strcpy(dest, ptr->fileNames + id * 13);

    return 0;
}

// 0x419314
int _art_get_code(int animation, int weaponType, char* a3, char* a4)
{
    if (weaponType < 0 || weaponType >= WEAPON_ANIMATION_COUNT) {
        return -1;
    }

    if (animation >= ANIM_TAKE_OUT && animation <= ANIM_FIRE_CONTINUOUS) {
        *a4 = 'c' + (animation - ANIM_TAKE_OUT);
        if (weaponType == WEAPON_ANIMATION_NONE) {
            return -1;
        }

        *a3 = 'd' + (weaponType - 1);
        return 0;
    } else if (animation == ANIM_PRONE_TO_STANDING) {
        *a4 = 'h';
        *a3 = 'c';
        return 0;
    } else if (animation == ANIM_BACK_TO_STANDING) {
        *a4 = 'j';
        *a3 = 'c';
        return 0;
    } else if (animation == ANIM_CALLED_SHOT_PIC) {
        *a4 = 'a';
        *a3 = 'n';
        return 0;
    } else if (animation >= FIRST_SF_DEATH_ANIM) {
        *a4 = 'a' + (animation - FIRST_SF_DEATH_ANIM);
        *a3 = 'r';
        return 0;
    } else if (animation >= FIRST_KNOCKDOWN_AND_DEATH_ANIM) {
        *a4 = 'a' + (animation - FIRST_KNOCKDOWN_AND_DEATH_ANIM);
        *a3 = 'b';
        return 0;
    } else if (animation == ANIM_THROW_ANIM) {
        if (weaponType == WEAPON_ANIMATION_KNIFE) {
            // knife
            *a3 = 'd';
            *a4 = 'm';
        } else if (weaponType == WEAPON_ANIMATION_SPEAR) {
            // spear
            *a3 = 'g';
            *a4 = 'm';
        } else {
            // other -> probably rock or grenade
            *a3 = 'a';
            *a4 = 's';
        }
        return 0;
    } else if (animation == ANIM_DODGE_ANIM) {
        if (weaponType <= 0) {
            *a3 = 'a';
            *a4 = 'n';
        } else {
            *a3 = 'd' + (weaponType - 1);
            *a4 = 'e';
        }
        return 0;
    }

    *a4 = 'a' + animation;
    if (animation <= ANIM_WALK && weaponType > 0) {
        *a3 = 'd' + (weaponType - 1);
        return 0;
    }
    *a3 = 'a';

    return 0;
}

// 0x419428
char* artBuildFilePath(int fid)
{
    int v1, v2, v3, v4, v5, type, v8, v10;
    char v9, v11, v12;

    v2 = fid;

    v10 = (fid & 0x70000000) >> 28;

    v1 = artAliasFid(fid);
    if (v1 != -1) {
        v2 = v1;
    }

    *_art_name = '\0';

    v3 = v2 & 0xFFF;
    v4 = (v2 & 0xFF0000) >> 16;
    v5 = (v2 & 0xF000) >> 12;
    type = OBJECT_TYPE(v2);

    if (v3 >= gArtListDescriptions[type].fileNamesLength) {
        return NULL;
    }

    if (type < 0 || type >= 11) {
        return NULL;
    }

    v8 = v3 * 13;

    if (type == 1) {
        if (_art_get_code(v4, v5, &v11, &v12) == -1) {
            return NULL;
        }
        if (v10) {
            sprintf(_art_name, "%s%s%s\\%s%c%c.fr%c", _cd_path_base, "art\\", gArtListDescriptions[1].name, gArtListDescriptions[1].fileNames + v8, v11, v12, v10 + 47);
        } else {
            sprintf(_art_name, "%s%s%s\\%s%c%c.frm", _cd_path_base, "art\\", gArtListDescriptions[1].name, gArtListDescriptions[1].fileNames + v8, v11, v12);
        }
    } else if (type == 8) {
        v9 = _head2[v4];
        if (v9 == 'f') {
            sprintf(_art_name, "%s%s%s\\%s%c%c%d.frm", _cd_path_base, "art\\", gArtListDescriptions[8].name, gArtListDescriptions[8].fileNames + v8, _head1[v4], 102, v5);
        } else {
            sprintf(_art_name, "%s%s%s\\%s%c%c.frm", _cd_path_base, "art\\", gArtListDescriptions[8].name, gArtListDescriptions[8].fileNames + v8, _head1[v4], v9);
        }
    } else {
        sprintf(_art_name, "%s%s%s\\%s", _cd_path_base, "art\\", gArtListDescriptions[type].name, gArtListDescriptions[type].fileNames + v8);
    }

    return _art_name;
}

// art_read_lst
// 0x419664
int artReadList(const char* path, char** artListPtr, int* artListSizePtr)
{
    File* stream = fileOpen(path, "rt");
    if (stream == NULL) {
        return -1;
    }

    int count = 0;
    char string[200];
    while (fileReadString(string, sizeof(string), stream)) {
        count++;
    }

    fileSeek(stream, 0, SEEK_SET);

    *artListSizePtr = count;

    char* artList = (char*)internal_malloc(13 * count);
    *artListPtr = artList;
    if (artList == NULL) {
        fileClose(stream);
        return -1;
    }

    while (fileReadString(string, sizeof(string), stream)) {
        char* brk = strpbrk(string, " ,;\r\t\n");
        if (brk != NULL) {
            *brk = '\0';
        }

        strncpy(artList, string, 12);
        artList[12] = '\0';

        artList += 13;
    }

    fileClose(stream);

    return 0;
}

// 0x419760
int artGetFramesPerSecond(Art* art)
{
    if (art == NULL) {
        return 10;
    }

    return art->framesPerSecond == 0 ? 10 : art->framesPerSecond;
}

// 0x419778
int artGetActionFrame(Art* art)
{
    return art == NULL ? -1 : art->actionFrame;
}

// 0x41978C
int artGetFrameCount(Art* art)
{
    return art == NULL ? -1 : art->frameCount;
}

// 0x4197A0
int artGetWidth(Art* art, int frame, int direction)
{
    ArtFrame* frm;

    frm = artGetFrame(art, frame, direction);
    if (frm == NULL) {
        return -1;
    }

    return frm->width;
}

// 0x4197B8
int artGetHeight(Art* art, int frame, int direction)
{
    ArtFrame* frm;

    frm = artGetFrame(art, frame, direction);
    if (frm == NULL) {
        return -1;
    }

    return frm->height;
}

// 0x4197D4
int artGetSize(Art* art, int frame, int direction, int* widthPtr, int* heightPtr)
{
    ArtFrame* frm;

    frm = artGetFrame(art, frame, direction);
    if (frm == NULL) {
        if (widthPtr != NULL) {
            *widthPtr = 0;
        }

        if (heightPtr != NULL) {
            *heightPtr = 0;
        }

        return -1;
    }

    if (widthPtr != NULL) {
        *widthPtr = frm->width;
    }

    if (heightPtr != NULL) {
        *heightPtr = frm->height;
    }

    return 0;
}

// 0x419820
int artGetFrameOffsets(Art* art, int frame, int direction, int* xPtr, int* yPtr)
{
    ArtFrame* frm;

    frm = artGetFrame(art, frame, direction);
    if (frm == NULL) {
        return -1;
    }

    *xPtr = frm->x;
    *yPtr = frm->y;

    return 0;
}

// 0x41984C
int artGetRotationOffsets(Art* art, int rotation, int* xPtr, int* yPtr)
{
    if (art == NULL) {
        return -1;
    }

    *xPtr = art->xOffsets[rotation];
    *yPtr = art->yOffsets[rotation];

    return 0;
}

// 0x419870
unsigned char* artGetFrameData(Art* art, int frame, int direction)
{
    ArtFrame* frm;

    frm = artGetFrame(art, frame, direction);
    if (frm == NULL) {
        return NULL;
    }

    return (unsigned char*)frm + sizeof(*frm);
}

// 0x419880
ArtFrame* artGetFrame(Art* art, int frame, int rotation)
{
    if (rotation < 0 || rotation >= 6) {
        return NULL;
    }

    if (art == NULL) {
        return NULL;
    }

    if (frame < 0 || frame >= art->frameCount) {
        return NULL;
    }

    ArtFrame* frm = (ArtFrame*)((unsigned char*)art + sizeof(*art) + art->dataOffsets[rotation]);
    for (int index = 0; index < frame; index++) {
        frm = (ArtFrame*)((unsigned char*)frm + sizeof(*frm) + frm->size);
    }
    return frm;
}

// 0x4198C8
bool artExists(int fid)
{
    bool result = false;
    int oldDb = -1;

    if (OBJECT_TYPE(fid) == OBJ_TYPE_CRITTER) {
        oldDb = _db_current(1);
        _db_current(_critter_db_handle);
    }

    char* filePath = artBuildFilePath(fid);
    if (filePath != NULL) {
        int fileSize;
        if (dbGetFileSize(filePath, &fileSize) != -1) {
            result = true;
        }
    }

    if (oldDb != -1) {
        _db_current(oldDb);
    }

    return result;
}

// 0x419930
bool _art_fid_valid(int fid)
{
    // NOTE: Original Code involves calling some unknown function. Check in debugger in mapper.
    char* filePath = artBuildFilePath(fid);
    if (filePath == NULL) {
        return false;
    }

    int fileSize;
    if (dbGetFileSize(filePath, &fileSize) == -1) {
        return false;
    }

    return true;
}

// 0x419998
int _art_alias_num(int index)
{
    return _anon_alias[index];
}

// 0x4199AC
int artCritterFidShouldRun(int fid)
{
    if (OBJECT_TYPE(fid) == OBJ_TYPE_CRITTER) {
        return gArtCritterFidShoudRunData[fid & 0xFFF];
    }

    return 0;
}

// 0x4199D4
int artAliasFid(int fid)
{
    int type = OBJECT_TYPE(fid);
    int anim = (fid & 0xFF0000) >> 16;
    if (type == 1) {
        if (anim == ANIM_ELECTRIFY
            || anim == ANIM_BURNED_TO_NOTHING
            || anim == ANIM_ELECTRIFIED_TO_NOTHING
            || anim == ANIM_ELECTRIFY_SF
            || anim == ANIM_BURNED_TO_NOTHING_SF
            || anim == ANIM_ELECTRIFIED_TO_NOTHING_SF
            || anim == ANIM_FIRE_DANCE
            || anim == ANIM_CALLED_SHOT_PIC) {
            // NOTE: Original code is slightly different. It uses many mutually
            // mirrored bitwise operators. Probably result of some macros for
            // getting/setting individual bits on fid.
            return (fid & 0x70000000) | ((anim << 16) & 0xFF0000) | 0x1000000 | (fid & 0xF000) | (_anon_alias[fid & 0xFFF] & 0xFFF);
        }
    }

    return -1;
}

// 0x419A78
int artCacheGetFileSizeImpl(int fid, int* sizePtr)
{
    int oldDb = -1;
    int result = -1;

    if (OBJECT_TYPE(fid) == OBJ_TYPE_CRITTER) {
        oldDb = _db_current(1);
        _db_current(_critter_db_handle);
    }

    char* artFilePath = artBuildFilePath(fid);
    if (artFilePath != NULL) {
        int fileSize;
        bool loaded = false;

        if (gArtLanguageInitialized) {
            char* pch = artFilePath;
            while (*pch != '\0' && *pch != '\\') {
                pch++;
            }

            if (*pch == '\0') {
                pch = artFilePath;
            }

            char localizedPath[MAX_PATH];
            sprintf(localizedPath, "art\\%s\\%s", gArtLanguage, pch);

            if (dbGetFileSize(localizedPath, &fileSize) == 0) {
                loaded = true;
            }
        }

        if (!loaded) {
            if (dbGetFileSize(artFilePath, &fileSize) == 0) {
                loaded = true;
            }
        }

        if (loaded) {
            *sizePtr = fileSize;
            result = 0;
        }
    }

    if (oldDb != -1) {
        _db_current(oldDb);
    }

    return result;
}

// 0x419B78
int artCacheReadDataImpl(int fid, int* sizePtr, unsigned char* data)
{
    int oldDb = -1;
    int result = -1;

    if (OBJECT_TYPE(fid) == OBJ_TYPE_CRITTER) {
        oldDb = _db_current(1);
        _db_current(_critter_db_handle);
    }

    char* artFileName = artBuildFilePath(fid);
    if (artFileName != NULL) {
        bool loaded = false;
        if (gArtLanguageInitialized) {
            char* pch = artFileName;
            while (*pch != '\0' && *pch != '\\') {
                pch++;
            }

            if (*pch == '\0') {
                pch = artFileName;
            }

            char localizedPath[MAX_PATH];
            sprintf(localizedPath, "art\\%s\\%s", gArtLanguage, pch);

            if (artRead(localizedPath, data) == 0) {
                loaded = true;
            }
        }

        if (!loaded) {
            if (artRead(artFileName, data) == 0) {
                loaded = true;
            }
        }

        if (loaded) {
            // TODO: Why it adds 74?
            *sizePtr = ((Art*)data)->field_3A + 74;
            result = 0;
        }
    }

    if (oldDb != -1) {
        _db_current(oldDb);
    }

    return result;
}

// 0x419C80
void artCacheFreeImpl(void* ptr)
{
    internal_free(ptr);
}

// 0x419C88
int buildFid(int objectType, int frmId, int anim, int a3, int rotation)
{
    int v7, v8, v9, v10;

    v10 = rotation;

    if (objectType != OBJ_TYPE_CRITTER) {
        goto zero;
    }

    if (anim == 33 || anim < 20 || anim > 35) {
        goto zero;
    }

    v7 = ((a3 << 12) & 0xF000) | (anim << 16) & 0xFF0000 | 0x1000000;
    v8 = (rotation << 28) & 0x70000000 | v7;
    v9 = frmId & 0xFFF;

    if (artExists(v9 | v8) != 0) {
        goto out;
    }

    if (objectType == rotation) {
        goto zero;
    }

    v10 = objectType;
    if (artExists(v9 | v7 | 0x10000000) != 0) {
        goto out;
    }

zero:

    v10 = 0;

out:

    return (v10 << 28) & 0x70000000 | (objectType << 24) | (anim << 16) & 0xFF0000 | (a3 << 12) & 0xF000 | frmId & 0xFFF;
}

// 0x419D60
int artReadFrameData(unsigned char* data, File* stream, int count)
{
    unsigned char* ptr = data;
    for (int index = 0; index < count; index++) {
        ArtFrame* frame = (ArtFrame*)ptr;

        if (fileReadInt16(stream, &(frame->width)) == -1) return -1;
        if (fileReadInt16(stream, &(frame->height)) == -1) return -1;
        if (fileReadInt32(stream, &(frame->size)) == -1) return -1;
        if (fileReadInt16(stream, &(frame->x)) == -1) return -1;
        if (fileReadInt16(stream, &(frame->y)) == -1) return -1;
        if (fileRead(ptr + sizeof(ArtFrame), frame->size, 1, stream) != 1) return -1;

        ptr += sizeof(ArtFrame) + frame->size;
    }

    return 0;
}

// 0x419E1C
int artReadHeader(Art* art, File* stream)
{
    if (fileReadInt32(stream, &(art->field_0)) == -1) return -1;
    if (fileReadInt16(stream, &(art->framesPerSecond)) == -1) return -1;
    if (fileReadInt16(stream, &(art->actionFrame)) == -1) return -1;
    if (fileReadInt16(stream, &(art->frameCount)) == -1) return -1;
    if (fileReadInt16List(stream, art->xOffsets, ROTATION_COUNT) == -1) return -1;
    if (fileReadInt16List(stream, art->yOffsets, ROTATION_COUNT) == -1) return -1;
    if (fileReadInt32List(stream, art->dataOffsets, ROTATION_COUNT) == -1) return -1;
    if (fileReadInt32(stream, &(art->field_3A)) == -1) return -1;

    return 0;
}

// 0x419FC0
int artRead(const char* path, unsigned char* data)
{
    File* stream = fileOpen(path, "rb");
    if (stream == NULL) {
        return -2;
    }

    Art* art = (Art*)data;
    if (artReadHeader(art, stream) != 0) {
        fileClose(stream);
        return -3;
    }

    for (int index = 0; index < ROTATION_COUNT; index++) {
        if (index == 0 || art->dataOffsets[index - 1] != art->dataOffsets[index]) {
            if (artReadFrameData(data + sizeof(Art) + art->dataOffsets[index], stream, art->frameCount) != 0) {
                fileClose(stream);
                return -5;
            }
        }
    }

    fileClose(stream);
    return 0;
}

// NOTE: Unused.
//
// 0x41A070
int artWriteFrameData(unsigned char* data, File* stream, int count)
{
    unsigned char* ptr = data;
    for (int index = 0; index < count; index++) {
        ArtFrame* frame = (ArtFrame*)ptr;

        if (fileWriteInt16(stream, frame->width) == -1) return -1;
        if (fileWriteInt16(stream, frame->height) == -1) return -1;
        if (fileWriteInt32(stream, frame->size) == -1) return -1;
        if (fileWriteInt16(stream, frame->x) == -1) return -1;
        if (fileWriteInt16(stream, frame->y) == -1) return -1;
        if (fileWrite(ptr + sizeof(ArtFrame), frame->size, 1, stream) != 1) return -1;

        ptr += sizeof(ArtFrame) + frame->size;
    }

    return 0;
}

// NOTE: Unused.
//
// 0x41A138
int artWriteHeader(Art* art, File* stream)
{
    if (fileWriteInt32(stream, art->field_0) == -1) return -1;
    if (fileWriteInt16(stream, art->framesPerSecond) == -1) return -1;
    if (fileWriteInt16(stream, art->actionFrame) == -1) return -1;
    if (fileWriteInt16(stream, art->frameCount) == -1) return -1;
    if (fileWriteInt16List(stream, art->xOffsets, ROTATION_COUNT) == -1) return -1;
    if (fileWriteInt16List(stream, art->yOffsets, ROTATION_COUNT) == -1) return -1;
    if (fileWriteInt32List(stream, art->dataOffsets, ROTATION_COUNT) == -1) return -1;
    if (fileWriteInt32(stream, art->field_3A) == -1) return -1;

    return 0;
}

// NOTE: Unused.
//
// 0x41A1E8
int artWrite(const char* path, unsigned char* data)
{
    if (data == NULL) {
        return -1;
    }

    File* stream = fileOpen(path, "wb");
    if (stream == NULL) {
        return -1;
    }

    Art* art = (Art*)data;
    if (artWriteHeader(art, stream) == -1) {
        fileClose(stream);
        return -1;
    }

    for (int index = 0; index < ROTATION_COUNT; index++) {
        if (index == 0 || art->dataOffsets[index - 1] != art->dataOffsets[index]) {
            if (artWriteFrameData(data + sizeof(Art) + art->dataOffsets[index], stream, art->frameCount) != 0) {
                fileClose(stream);
                return -1;
            }
        }
    }

    fileClose(stream);
    return 0;
}
