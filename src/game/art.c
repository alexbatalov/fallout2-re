#include "game/art.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/anim.h"
#include "game/artload.h"
#include "plib/gnw/debug.h"
#include "plib/gnw/grbuf.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "plib/gnw/memory.h"
#include "game/object.h"
#include "game/proto.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// 0x510738
static ArtListDescription art[OBJ_TYPE_COUNT] = {
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
// together with [darn_foreign_sub_path].
//
// 0x510898
static bool darn_foreigners = false;

// 0x51089C
static const char* head1 = "gggnnnbbbgnb";

// 0x5108A0
static const char* head2 = "vfngfbnfvppp";

// Current native look base fid.
//
// 0x5108A4
int art_vault_guy_num = 0;

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
int art_vault_person_nums[DUDE_NATIVE_LOOK_COUNT][GENDER_COUNT];

// Index of "grid001.frm" in tiles.lst.
//
// 0x5108B8
int art_mapper_blank_tile = 1;

// Non-english language name.
//
// This value is used as a directory name to display localized arts.
//
// 0x56C970
static char darn_foreign_sub_path[32];

// 0x56C990
Cache art_cache;

// 0x56C9E4
static char art_name[MAX_PATH];

// 0x56CAE8
HeadDescription* head_info;

// 0x56CAEC
static int* anon_alias;

// 0x56CAF0
static int* artCritterFidShouldRunData;

// 0x418840
int art_init()
{
    char path[MAX_PATH];
    File* stream;
    char string[200];

    int cacheSize;
    if (!config_get_value(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_ART_CACHE_SIZE_KEY, &cacheSize)) {
        cacheSize = 8;
    }

    if (!cache_init(&art_cache, art_data_size, art_data_load, art_data_free, cacheSize << 20)) {
        debug_printf("cache_init failed in art_init\n");
        return -1;
    }

    char* language;
    if (config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_LANGUAGE_KEY, &language) && stricmp(language, ENGLISH) != 0) {
        strcpy(darn_foreign_sub_path, language);
        darn_foreigners = true;
    }

    bool critterDbSelected = false;
    for (int objectType = 0; objectType < OBJ_TYPE_COUNT; objectType++) {
        art[objectType].flags = 0;
        sprintf(path, "%s%s%s\\%s.lst", cd_path_base, "art\\", art[objectType].name, art[objectType].name);

        int oldDb;
        if (objectType == OBJ_TYPE_CRITTER) {
            oldDb = _db_current();
            critterDbSelected = true;
            _db_select(critter_db_handle);
        }

        if (art_read_lst(path, &(art[objectType].fileNames), &(art[objectType].fileNamesLength)) != 0) {
            debug_printf("art_read_lst failed in art_init\n");
            if (critterDbSelected) {
                _db_select(oldDb);
            }
            cache_exit(&art_cache);
            return -1;
        }

        if (objectType == OBJ_TYPE_CRITTER) {
            critterDbSelected = false;
            _db_select(oldDb);
        }
    }

    anon_alias = (int*)mem_malloc(sizeof(*anon_alias) * art[OBJ_TYPE_CRITTER].fileNamesLength);
    if (anon_alias == NULL) {
        art[OBJ_TYPE_CRITTER].fileNamesLength = 0;
        debug_printf("Out of memory for anon_alias in art_init\n");
        cache_exit(&art_cache);
        return -1;
    }

    artCritterFidShouldRunData = (int*)mem_malloc(sizeof(*artCritterFidShouldRunData) * art[1].fileNamesLength);
    if (artCritterFidShouldRunData == NULL) {
        art[OBJ_TYPE_CRITTER].fileNamesLength = 0;
        debug_printf("Out of memory for artCritterFidShouldRunData in art_init\n");
        cache_exit(&art_cache);
        return -1;
    }

    for (int critterIndex = 0; critterIndex < art[OBJ_TYPE_CRITTER].fileNamesLength; critterIndex++) {
        artCritterFidShouldRunData[critterIndex] = 0;
    }

    sprintf(path, "%s%s%s\\%s.lst", cd_path_base, "art\\", art[OBJ_TYPE_CRITTER].name, art[OBJ_TYPE_CRITTER].name);

    stream = fileOpen(path, "rt");
    if (stream == NULL) {
        debug_printf("Unable to open %s in art_init\n", path);
        cache_exit(&art_cache);
        return -1;
    }

    char* critterFileNames = art[OBJ_TYPE_CRITTER].fileNames;
    for (int critterIndex = 0; critterIndex < art[OBJ_TYPE_CRITTER].fileNamesLength; critterIndex++) {
        if (stricmp(critterFileNames, "hmjmps") == 0) {
            art_vault_person_nums[DUDE_NATIVE_LOOK_JUMPSUIT][GENDER_MALE] = critterIndex;
        } else if (stricmp(critterFileNames, "hfjmps") == 0) {
            art_vault_person_nums[DUDE_NATIVE_LOOK_JUMPSUIT][GENDER_FEMALE] = critterIndex;
        }

        if (stricmp(critterFileNames, "hmwarr") == 0) {
            art_vault_person_nums[DUDE_NATIVE_LOOK_TRIBAL][GENDER_MALE] = critterIndex;
            art_vault_guy_num = critterIndex;
        } else if (stricmp(critterFileNames, "hfprim") == 0) {
            art_vault_person_nums[DUDE_NATIVE_LOOK_TRIBAL][GENDER_FEMALE] = critterIndex;
        }

        critterFileNames += 13;
    }

    for (int critterIndex = 0; critterIndex < art[OBJ_TYPE_CRITTER].fileNamesLength; critterIndex++) {
        if (!fileReadString(string, sizeof(string), stream)) {
            break;
        }

        char* sep1 = strchr(string, ',');
        if (sep1 != NULL) {
            anon_alias[critterIndex] = atoi(sep1 + 1);

            char* sep2 = strchr(sep1 + 1, ',');
            if (sep2 != NULL) {
                artCritterFidShouldRunData[critterIndex] = atoi(sep2 + 1);
            } else {
                artCritterFidShouldRunData[critterIndex] = 0;
            }
        } else {
            anon_alias[critterIndex] = art_vault_guy_num;
            artCritterFidShouldRunData[critterIndex] = 1;
        }
    }

    fileClose(stream);

    char* tileFileNames = art[OBJ_TYPE_TILE].fileNames;
    for (int tileIndex = 0; tileIndex < art[OBJ_TYPE_TILE].fileNamesLength; tileIndex++) {
        if (stricmp(tileFileNames, "grid001.frm") == 0) {
            art_mapper_blank_tile = tileIndex;
        }
        tileFileNames += 13;
    }

    head_info = (HeadDescription*)mem_malloc(sizeof(*head_info) * art[OBJ_TYPE_HEAD].fileNamesLength);
    if (head_info == NULL) {
        art[OBJ_TYPE_HEAD].fileNamesLength = 0;
        debug_printf("Out of memory for head_info in art_init\n");
        cache_exit(&art_cache);
        return -1;
    }

    sprintf(path, "%s%s%s\\%s.lst", cd_path_base, "art\\", art[OBJ_TYPE_HEAD].name, art[OBJ_TYPE_HEAD].name);

    stream = fileOpen(path, "rt");
    if (stream == NULL) {
        debug_printf("Unable to open %s in art_init\n", path);
        cache_exit(&art_cache);
        return -1;
    }

    for (int headIndex = 0; headIndex < art[OBJ_TYPE_HEAD].fileNamesLength; headIndex++) {
        if (!fileReadString(string, sizeof(string), stream)) {
            break;
        }

        char* sep1 = strchr(string, ',');
        if (sep1 != NULL) {
            *sep1 = '\0';
        } else {
            sep1 = string;
        }

        char* sep2 = strchr(sep1, ',');
        if (sep2 != NULL) {
            *sep2 = '\0';
        } else {
            sep2 = sep1;
        }

        head_info[headIndex].goodFidgetCount = atoi(sep1 + 1);

        char* sep3 = strchr(sep2, ',');
        if (sep3 != NULL) {
            *sep3 = '\0';
        } else {
            sep3 = sep2;
        }

        head_info[headIndex].neutralFidgetCount = atoi(sep2 + 1);

        char* sep4 = strpbrk(sep3 + 1, " ,;\t\n");
        if (sep4 != NULL) {
            *sep4 = '\0';
        }

        head_info[headIndex].badFidgetCount = atoi(sep3 + 1);
    }

    fileClose(stream);

    return 0;
}

// 0x418EB8
void art_reset()
{
}

// 0x418EBC
void art_exit()
{
    cache_exit(&art_cache);

    mem_free(anon_alias);
    mem_free(artCritterFidShouldRunData);

    for (int index = 0; index < OBJ_TYPE_COUNT; index++) {
        mem_free(art[index].fileNames);
        art[index].fileNames = NULL;

        mem_free(art[index].field_18);
        art[index].field_18 = NULL;
    }

    mem_free(head_info);
}

// 0x418F1C
char* art_dir(int objectType)
{
    return objectType >= OBJ_TYPE_ITEM && objectType < OBJ_TYPE_COUNT ? art[objectType].name : NULL;
}

// 0x418F34
int art_get_disable(int objectType)
{
    return objectType >= OBJ_TYPE_ITEM && objectType < OBJ_TYPE_COUNT ? art[objectType].flags & 1 : 0;
}

// NOTE: Unused.
//
// 0x418F50
void art_toggle_disable(int objectType)
{
    if (objectType >= 0 && objectType < OBJ_TYPE_COUNT) {
        art[objectType].flags ^= 1;
    }
}

// NOTE: Unused.
//
// 0x418F64
int art_total(int objectType)
{
    return objectType >= 0 && objectType < OBJ_TYPE_COUNT ? art[objectType].fileNamesLength : 0;
}

// 0x418F7C
int art_head_fidgets(int headFid)
{
    if (FID_TYPE(headFid) != OBJ_TYPE_HEAD) {
        return 0;
    }

    int head = headFid & 0xFFF;

    if (head > art[OBJ_TYPE_HEAD].fileNamesLength) {
        return 0;
    }

    HeadDescription* headDescription = &(head_info[head]);

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
void scale_art(int fid, unsigned char* dest, int width, int height, int pitch)
{
    // NOTE: Original code is different. For unknown reason it directly calls
    // many art functions, for example instead of [art_ptr_lock] it calls lower level
    // [cache_lock], instead of [art_frame_width] is calls [frame_ptr], then get
    // width from frame's struct field. I don't know if this was intentional or
    // not. I've replaced these calls with higher level functions where
    // appropriate.

    CacheEntry* handle;
    Art* frm = art_ptr_lock(fid, &handle);
    if (frm == NULL) {
        return;
    }

    unsigned char* frameData = art_frame_data(frm, 0, 0);
    int frameWidth = art_frame_width(frm, 0, 0);
    int frameHeight = art_frame_length(frm, 0, 0);

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

    art_ptr_unlock(handle);
}

// 0x419160
Art* art_ptr_lock(int fid, CacheEntry** handlePtr)
{
    if (handlePtr == NULL) {
        return NULL;
    }

    Art* art = NULL;
    cache_lock(&art_cache, fid, (void**)&art, handlePtr);
    return art;
}

// 0x419188
unsigned char* art_ptr_lock_data(int fid, int frame, int direction, CacheEntry** handlePtr)
{
    Art* art;
    ArtFrame* frm;

    art = NULL;
    if (handlePtr) {
        cache_lock(&art_cache, fid, (void**)&art, handlePtr);
    }

    if (art != NULL) {
        frm = frame_ptr(art, frame, direction);
        if (frm != NULL) {

            return (unsigned char*)frm + sizeof(*frm);
        }
    }

    return NULL;
}

// 0x4191CC
unsigned char* art_lock(int fid, CacheEntry** handlePtr, int* widthPtr, int* heightPtr)
{
    *handlePtr = NULL;

    Art* art;
    cache_lock(&art_cache, fid, (void**)&art, handlePtr);

    if (art == NULL) {
        return NULL;
    }

    // NOTE: Uninline.
    *widthPtr = art_frame_width(art, 0, 0);
    if (*widthPtr == -1) {
        return NULL;
    }

    // NOTE: Uninline.
    *heightPtr = art_frame_length(art, 0, 0);
    if (*heightPtr == -1) {
        return NULL;
    }

    // NOTE: Uninline.
    return art_frame_data(art, 0, 0);
}

// 0x419260
int art_ptr_unlock(CacheEntry* handle)
{
    return cache_unlock(&art_cache, handle);
}

// 0x41927C
int art_flush()
{
    return cache_flush(&art_cache);
}

// NOTE: Unused.
//
// 0x419294
int art_discard(int fid)
{
    if (cache_discard(&art_cache, fid) == 0) {
        return -1;
    }

    return 0;
}

// 0x4192B0
int art_get_base_name(int objectType, int id, char* dest)
{
    ArtListDescription* ptr;

    if (objectType < OBJ_TYPE_ITEM && objectType >= OBJ_TYPE_COUNT) {
        return -1;
    }

    ptr = &(art[objectType]);

    if (id >= ptr->fileNamesLength) {
        return -1;
    }

    strcpy(dest, ptr->fileNames + id * 13);

    return 0;
}

// 0x419314
int art_get_code(int animation, int weaponType, char* a3, char* a4)
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
char* art_get_name(int fid)
{
    int v1, v2, v3, v4, v5, type, v8, v10;
    char v9, v11, v12;

    v2 = fid;

    v10 = (fid & 0x70000000) >> 28;

    v1 = art_alias_fid(fid);
    if (v1 != -1) {
        v2 = v1;
    }

    *art_name = '\0';

    v3 = v2 & 0xFFF;
    v4 = FID_ANIM_TYPE(v2);
    v5 = (v2 & 0xF000) >> 12;
    type = FID_TYPE(v2);

    if (v3 >= art[type].fileNamesLength) {
        return NULL;
    }

    if (type < OBJ_TYPE_ITEM || type >= OBJ_TYPE_COUNT) {
        return NULL;
    }

    v8 = v3 * 13;

    if (type == 1) {
        if (art_get_code(v4, v5, &v11, &v12) == -1) {
            return NULL;
        }
        if (v10) {
            sprintf(art_name, "%s%s%s\\%s%c%c.fr%c", cd_path_base, "art\\", art[1].name, art[1].fileNames + v8, v11, v12, v10 + 47);
        } else {
            sprintf(art_name, "%s%s%s\\%s%c%c.frm", cd_path_base, "art\\", art[1].name, art[1].fileNames + v8, v11, v12);
        }
    } else if (type == 8) {
        v9 = head2[v4];
        if (v9 == 'f') {
            sprintf(art_name, "%s%s%s\\%s%c%c%d.frm", cd_path_base, "art\\", art[8].name, art[8].fileNames + v8, head1[v4], 102, v5);
        } else {
            sprintf(art_name, "%s%s%s\\%s%c%c.frm", cd_path_base, "art\\", art[8].name, art[8].fileNames + v8, head1[v4], v9);
        }
    } else {
        sprintf(art_name, "%s%s%s\\%s", cd_path_base, "art\\", art[type].name, art[type].fileNames + v8);
    }

    return art_name;
}

// art_read_lst
// 0x419664
int art_read_lst(const char* path, char** artListPtr, int* artListSizePtr)
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

    char* artList = (char*)mem_malloc(13 * count);
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
int art_frame_fps(Art* art)
{
    if (art == NULL) {
        return 10;
    }

    return art->framesPerSecond == 0 ? 10 : art->framesPerSecond;
}

// 0x419778
int art_frame_action_frame(Art* art)
{
    return art == NULL ? -1 : art->actionFrame;
}

// 0x41978C
int art_frame_max_frame(Art* art)
{
    return art == NULL ? -1 : art->frameCount;
}

// 0x4197A0
int art_frame_width(Art* art, int frame, int direction)
{
    ArtFrame* frm;

    frm = frame_ptr(art, frame, direction);
    if (frm == NULL) {
        return -1;
    }

    return frm->width;
}

// 0x4197B8
int art_frame_length(Art* art, int frame, int direction)
{
    ArtFrame* frm;

    frm = frame_ptr(art, frame, direction);
    if (frm == NULL) {
        return -1;
    }

    return frm->height;
}

// 0x4197D4
int art_frame_width_length(Art* art, int frame, int direction, int* widthPtr, int* heightPtr)
{
    ArtFrame* frm;

    frm = frame_ptr(art, frame, direction);
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
int art_frame_hot(Art* art, int frame, int direction, int* xPtr, int* yPtr)
{
    ArtFrame* frm;

    frm = frame_ptr(art, frame, direction);
    if (frm == NULL) {
        return -1;
    }

    *xPtr = frm->x;
    *yPtr = frm->y;

    return 0;
}

// 0x41984C
int art_frame_offset(Art* art, int rotation, int* xPtr, int* yPtr)
{
    if (art == NULL) {
        return -1;
    }

    *xPtr = art->xOffsets[rotation];
    *yPtr = art->yOffsets[rotation];

    return 0;
}

// 0x419870
unsigned char* art_frame_data(Art* art, int frame, int direction)
{
    ArtFrame* frm;

    frm = frame_ptr(art, frame, direction);
    if (frm == NULL) {
        return NULL;
    }

    return (unsigned char*)frm + sizeof(*frm);
}

// 0x419880
ArtFrame* frame_ptr(Art* art, int frame, int rotation)
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
bool art_exists(int fid)
{
    bool result = false;
    int oldDb = -1;

    if (FID_TYPE(fid) == OBJ_TYPE_CRITTER) {
        oldDb = _db_current();
        _db_select(critter_db_handle);
    }

    char* filePath = art_get_name(fid);
    if (filePath != NULL) {
        int fileSize;
        if (dbGetFileSize(filePath, &fileSize) != -1) {
            result = true;
        }
    }

    if (oldDb != -1) {
        _db_select(oldDb);
    }

    return result;
}

// NOTE: Exactly the same implementation as `art_exists`.
//
// 0x419930
bool art_fid_valid(int fid)
{
    bool result = false;
    int oldDb = -1;

    if (FID_TYPE(fid) == OBJ_TYPE_CRITTER) {
        oldDb = _db_current();
        _db_select(critter_db_handle);
    }

    char* filePath = art_get_name(fid);
    if (filePath != NULL) {
        int fileSize;
        if (dbGetFileSize(filePath, &fileSize) != -1) {
            result = true;
        }
    }

    if (oldDb != -1) {
        _db_select(oldDb);
    }

    return result;
}

// 0x419998
int art_alias_num(int index)
{
    return anon_alias[index];
}

// 0x4199AC
int artCritterFidShouldRun(int fid)
{
    if (FID_TYPE(fid) == OBJ_TYPE_CRITTER) {
        return artCritterFidShouldRunData[fid & 0xFFF];
    }

    return 0;
}

// 0x4199D4
int art_alias_fid(int fid)
{
    int type = FID_TYPE(fid);
    int anim = FID_ANIM_TYPE(fid);
    if (type == OBJ_TYPE_CRITTER) {
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
            return (fid & 0x70000000) | ((anim << 16) & 0xFF0000) | 0x1000000 | (fid & 0xF000) | (anon_alias[fid & 0xFFF] & 0xFFF);
        }
    }

    return -1;
}

// 0x419A78
int art_data_size(int fid, int* sizePtr)
{
    int oldDb = -1;
    int result = -1;

    if (FID_TYPE(fid) == OBJ_TYPE_CRITTER) {
        oldDb = _db_current();
        _db_select(critter_db_handle);
    }

    char* artFilePath = art_get_name(fid);
    if (artFilePath != NULL) {
        int fileSize;
        bool loaded = false;

        if (darn_foreigners) {
            char* pch = strchr(artFilePath, '\\');
            if (pch == NULL) {
                pch = artFilePath;
            }

            char localizedPath[MAX_PATH];
            sprintf(localizedPath, "art\\%s\\%s", darn_foreign_sub_path, pch);

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
        _db_select(oldDb);
    }

    return result;
}

// 0x419B78
int art_data_load(int fid, int* sizePtr, unsigned char* data)
{
    int oldDb = -1;
    int result = -1;

    if (FID_TYPE(fid) == OBJ_TYPE_CRITTER) {
        oldDb = _db_current();
        _db_select(critter_db_handle);
    }

    char* artFileName = art_get_name(fid);
    if (artFileName != NULL) {
        bool loaded = false;
        if (darn_foreigners) {
            char* pch = strchr(artFileName, '\\');
            if (pch == NULL) {
                pch = artFileName;
            }

            char localizedPath[MAX_PATH];
            sprintf(localizedPath, "art\\%s\\%s", darn_foreign_sub_path, pch);

            if (load_frame_into(localizedPath, data) == 0) {
                loaded = true;
            }
        }

        if (!loaded) {
            if (load_frame_into(artFileName, data) == 0) {
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
        _db_select(oldDb);
    }

    return result;
}

// 0x419C80
void art_data_free(void* ptr)
{
    mem_free(ptr);
}

// 0x419C88
int art_id(int objectType, int frmId, int animType, int a3, int rotation)
{
    int v7, v8, v9, v10;

    v10 = rotation;

    if (objectType != OBJ_TYPE_CRITTER) {
        goto zero;
    }

    if (animType == ANIM_FIRE_DANCE || animType < ANIM_FALL_BACK || animType > ANIM_FALL_FRONT_BLOOD) {
        goto zero;
    }

    v7 = ((a3 << 12) & 0xF000) | ((animType << 16) & 0xFF0000) | 0x1000000;
    v8 = ((rotation << 28) & 0x70000000) | v7;
    v9 = frmId & 0xFFF;

    if (art_exists(v9 | v8) != 0) {
        goto out;
    }

    if (objectType == rotation) {
        goto zero;
    }

    v10 = objectType;
    if (art_exists(v9 | v7 | 0x10000000) != 0) {
        goto out;
    }

zero:

    v10 = 0;

out:

    return ((v10 << 28) & 0x70000000) | (objectType << 24) | ((animType << 16) & 0xFF0000) | ((a3 << 12) & 0xF000) | (frmId & 0xFFF);
}
