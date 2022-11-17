#include "game/sfxlist.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plib/db/db.h"
#include "plib/gnw/debug.h"
#include "plib/gnw/memory.h"
#include "sound_decoder.h"

typedef struct SoundEffectsListEntry {
    char* name;
    int dataSize;
    int fileSize;
    int tag;
} SoundEffectsListEntry;

static int sfxl_index_to_tag(int tag, int* indexPtr);
static void sfxl_destroy();
static int sfxl_get_names();
static int sfxl_copy_names(char** fileNameList);
static int sfxl_get_sizes();
static int sfxl_sort_by_name();
static int sfxl_compare_by_name(const void* a1, const void* a2);
static int sfxl_ad_reader(int fileHandle, void* buf, unsigned int size);

// 0x51C8F8
static bool sfxl_initialized = false;

// 0x51C8FC
static int sfxl_dlevel = INT_MAX;

// 0x51C900
static char* sfxl_effect_path = NULL;

// 0x51C904
static int sfxl_effect_path_len = 0;

// sndlist.lst
//
// 0x51C908
static SoundEffectsListEntry* sfxl_list = NULL;

// The length of [sfxl_list] array.
//
// 0x51C90C
static int sfxl_files_total = 0;

// 0x667F94
static int sfxl_compression;

// 0x4A98E0
bool sfxl_tag_is_legal(int a1)
{
    return sfxl_index_to_tag(a1, NULL) == SFXL_OK;
}

// 0x4A98F4
int sfxl_init(const char* soundEffectsPath, int a2, int debugLevel)
{
    char path[FILENAME_MAX];

    // TODO: What for?
    // memcpy(path, byte_4A97E0, 0xFF);

    sfxl_dlevel = debugLevel;
    sfxl_compression = a2;
    sfxl_files_total = 0;

    sfxl_effect_path = mem_strdup(soundEffectsPath);
    if (sfxl_effect_path == NULL) {
        return SFXL_ERR;
    }

    sfxl_effect_path_len = strlen(sfxl_effect_path);

    if (sfxl_effect_path_len == 0 || soundEffectsPath[sfxl_effect_path_len - 1] == '\\') {
        sprintf(path, "%sSNDLIST.LST", soundEffectsPath);
    } else {
        sprintf(path, "%s\\SNDLIST.LST", soundEffectsPath);
    }

    File* stream = db_fopen(path, "rt");
    if (stream != NULL) {
        db_fgets(path, 255, stream);
        sfxl_files_total = atoi(path);

        sfxl_list = (SoundEffectsListEntry*)mem_malloc(sizeof(*sfxl_list) * sfxl_files_total);
        for (int index = 0; index < sfxl_files_total; index++) {
            SoundEffectsListEntry* entry = &(sfxl_list[index]);

            db_fgets(path, 255, stream);

            // Remove trailing newline.
            *(path + strlen(path) - 1) = '\0';
            entry->name = mem_strdup(path);

            db_fgets(path, 255, stream);
            entry->dataSize = atoi(path);

            db_fgets(path, 255, stream);
            entry->fileSize = atoi(path);

            db_fgets(path, 255, stream);
            entry->tag = atoi(path);
        }

        db_fclose(stream);

        debug_printf("Reading SNDLIST.LST Sound FX Count: %d", sfxl_files_total);
    } else {
        int err;

        err = sfxl_get_names();
        if (err != SFXL_OK) {
            mem_free(sfxl_effect_path);
            return err;
        }

        err = sfxl_get_sizes();
        if (err != SFXL_OK) {
            sfxl_destroy();
            mem_free(sfxl_effect_path);
            return err;
        }

        // NOTE: For unknown reason tag generation functionality is missing.
        // You won't be able to produce the same SNDLIST.LST as the game have.
        // All tags will be 0 (see [sfxl_get_names]).
        //
        // On the other hand, tags read from the SNDLIST.LST are not used in
        // the game. Instead tag is automatically determined from entry's
        // index (see [sfxl_name_to_tag]).

        // NOTE: Uninline.
        sfxl_sort_by_name();

        File* stream = db_fopen(path, "wt");
        if (stream != NULL) {
            db_fprintf(stream, "%d\n", sfxl_files_total);

            for (int index = 0; index < sfxl_files_total; index++) {
                SoundEffectsListEntry* entry = &(sfxl_list[index]);

                db_fprintf(stream, "%s\n", entry->name);
                db_fprintf(stream, "%d\n", entry->dataSize);
                db_fprintf(stream, "%d\n", entry->fileSize);
                db_fprintf(stream, "%d\n", entry->tag);
            }

            db_fclose(stream);
        } else {
            debug_printf("SFXLIST: Can't open file for write %s\n", path);
        }
    }

    sfxl_initialized = true;

    return SFXL_OK;
}

// 0x4A9C04
void sfxl_exit()
{
    if (sfxl_initialized) {
        sfxl_destroy();
        mem_free(sfxl_effect_path);
        sfxl_initialized = false;
    }
}

// 0x4A9C28
int sfxl_name_to_tag(char* name, int* tagPtr)
{
    if (strnicmp(sfxl_effect_path, name, sfxl_effect_path_len) != 0) {
        return SFXL_ERR;
    }

    SoundEffectsListEntry dummy;
    dummy.name = name + sfxl_effect_path_len;

    SoundEffectsListEntry* entry = (SoundEffectsListEntry*)bsearch(&dummy, sfxl_list, sfxl_files_total, sizeof(*sfxl_list), sfxl_compare_by_name);
    if (entry == NULL) {
        return SFXL_ERR;
    }

    int index = entry - sfxl_list;
    if (index < 0 || index >= sfxl_files_total) {
        return SFXL_ERR;
    }

    *tagPtr = 2 * index + 2;

    return SFXL_OK;
}

// 0x4A9CD8
int sfxl_name(int tag, char** pathPtr)
{
    int index;
    int err = sfxl_index_to_tag(tag, &index);
    if (err != SFXL_OK) {
        return err;
    }

    char* name = sfxl_list[index].name;

    char* path = (char*)mem_malloc(strlen(sfxl_effect_path) + strlen(name) + 1);
    if (path == NULL) {
        return SFXL_ERR;
    }

    strcpy(path, sfxl_effect_path);
    strcat(path, name);

    *pathPtr = path;

    return SFXL_OK;
}

// 0x4A9D90
int sfxl_size_full(int tag, int* sizePtr)
{
    int index;
    int rc = sfxl_index_to_tag(tag, &index);
    if (rc != SFXL_OK) {
        return rc;
    }

    SoundEffectsListEntry* entry = &(sfxl_list[index]);
    *sizePtr = entry->dataSize;

    return SFXL_OK;
}

// 0x4A9DBC
int sfxl_size_cached(int tag, int* sizePtr)
{
    int index;
    int err = sfxl_index_to_tag(tag, &index);
    if (err != SFXL_OK) {
        return err;
    }

    SoundEffectsListEntry* entry = &(sfxl_list[index]);
    *sizePtr = entry->fileSize;

    return SFXL_OK;
}

// 0x4A9DE8
static int sfxl_index_to_tag(int tag, int* indexPtr)
{
    if (tag <= 0) {
        return SFXL_ERR_TAG_INVALID;
    }

    if ((tag & 1) != 0) {
        return SFXL_ERR_TAG_INVALID;
    }

    int index = (tag / 2) - 1;
    if (index >= sfxl_files_total) {
        return SFXL_ERR_TAG_INVALID;
    }

    if (indexPtr != NULL) {
        *indexPtr = index;
    }

    return SFXL_OK;
}

// 0x4A9E44
static void sfxl_destroy()
{
    if (sfxl_files_total < 0) {
        return;
    }

    if (sfxl_list == NULL) {
        return;
    }

    for (int index = 0; index < sfxl_files_total; index++) {
        SoundEffectsListEntry* entry = &(sfxl_list[index]);
        if (entry->name != NULL) {
            mem_free(entry->name);
        }
    }

    mem_free(sfxl_list);
    sfxl_list = NULL;

    sfxl_files_total = 0;
}

// 0x4A9EA0
static int sfxl_get_names()
{
    const char* extension;
    switch (sfxl_compression) {
    case 0:
        extension = "*.SND";
        break;
    case 1:
        extension = "*.ACM";
        break;
    default:
        return SFXL_ERR;
    }

    char* pattern = (char*)mem_malloc(strlen(sfxl_effect_path) + strlen(extension) + 1);
    if (pattern == NULL) {
        return SFXL_ERR;
    }

    strcpy(pattern, sfxl_effect_path);
    strcat(pattern, extension);

    char** fileNameList;
    sfxl_files_total = db_get_file_list(pattern, &fileNameList, 0, 0);
    mem_free(pattern);

    if (sfxl_files_total > 10000) {
        db_free_file_list(&fileNameList, 0);
        return SFXL_ERR;
    }

    if (sfxl_files_total <= 0) {
        return SFXL_ERR;
    }

    sfxl_list = (SoundEffectsListEntry*)mem_malloc(sizeof(*sfxl_list) * sfxl_files_total);
    if (sfxl_list == NULL) {
        db_free_file_list(&fileNameList, 0);
        return SFXL_ERR;
    }

    memset(sfxl_list, 0, sizeof(*sfxl_list) * sfxl_files_total);

    int err = sfxl_copy_names(fileNameList);

    db_free_file_list(&fileNameList, 0);

    if (err != SFXL_OK) {
        sfxl_destroy();
        return err;
    }

    return SFXL_OK;
}

// 0x4AA000
static int sfxl_copy_names(char** fileNameList)
{
    for (int index = 0; index < sfxl_files_total; index++) {
        SoundEffectsListEntry* entry = &(sfxl_list[index]);
        entry->name = mem_strdup(*fileNameList++);
        if (entry->name == NULL) {
            sfxl_destroy();
            return SFXL_ERR;
        }
    }

    return SFXL_OK;
}

// 0x4AA050
static int sfxl_get_sizes()
{

    char* path = (char*)mem_malloc(sfxl_effect_path_len + 13);
    if (path == NULL) {
        return SFXL_ERR;
    }

    strcpy(path, sfxl_effect_path);

    char* fileName = path + sfxl_effect_path_len;

    for (int index = 0; index < sfxl_files_total; index++) {
        SoundEffectsListEntry* entry = &(sfxl_list[index]);
        strcpy(fileName, entry->name);

        int fileSize;
        if (db_dir_entry(path, &fileSize) != 0) {
            mem_free(path);
            return SFXL_ERR;
        }

        if (fileSize <= 0) {
            mem_free(path);
            return SFXL_ERR;
        }

        entry->fileSize = fileSize;

        switch (sfxl_compression) {
        case 0:
            entry->dataSize = fileSize;
            break;
        case 1:
            if (1) {
                File* stream = db_fopen(path, "rb");
                if (stream == NULL) {
                    mem_free(path);
                    return 1;
                }

                int v1;
                int v2;
                int v3;
                SoundDecoder* soundDecoder = soundDecoderInit(sfxl_ad_reader, (int)stream, &v1, &v2, &v3);
                entry->dataSize = 2 * v3;
                soundDecoderFree(soundDecoder);
                db_fclose(stream);
            }
            break;
        default:
            mem_free(path);
            return SFXL_ERR;
        }
    }

    mem_free(path);

    return SFXL_OK;
}

// NOTE: Inlined.
//
// 0x4AA200
static int sfxl_sort_by_name()
{
    if (sfxl_files_total != 1) {
        qsort(sfxl_list, sfxl_files_total, sizeof(*sfxl_list), sfxl_compare_by_name);
    }
    return 0;
}

// 0x4AA228
static int sfxl_compare_by_name(const void* a1, const void* a2)
{
    SoundEffectsListEntry* v1 = (SoundEffectsListEntry*)a1;
    SoundEffectsListEntry* v2 = (SoundEffectsListEntry*)a2;

    return stricmp(v1->name, v2->name);
}

// 0x4AA234
static int sfxl_ad_reader(int fileHandle, void* buf, unsigned int size)
{
    return db_fread(buf, 1, size, (File*)fileHandle);
}
