#include "game/lip_sync.h"

#include <stdio.h>
#include <string.h>

#include "int/audio.h"
#include "plib/gnw/input.h"
#include "plib/gnw/debug.h"
#include "game/gsound.h"
#include "plib/gnw/memory.h"
#include "int/sound.h"

static char* lips_fix_string(const char* fileName, size_t length);
static int lips_stop_speech();
static int lips_read_phoneme_type(unsigned char* phoneme_type, File* stream);
static int lips_read_marker_type(SpeechMarker* marker_type, File* stream);
static int lips_read_lipsynch_info(LipsData* a1, File* stream);
static int lips_make_speech();

// 0x519240
unsigned char head_phoneme_current = 0;

// 0x519241
unsigned char head_phoneme_drawn = 0;

// 0x519244
int head_marker_current = 0;

// 0x519248
bool lips_draw_head = true;

// 0x51924C
LipsData lip_info = {
    2,
    22528,
    0,
    NULL,
    -1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    50,
    100,
    0,
    0,
    0,
    "TEST",
    "VOC",
    "TXT",
    "LIP",
};

// 0x5193B4
int speechStartTime = 0;

// 0x613CA0
static char lips_subdir_name[14];

// 0x47AAC0
static char* lips_fix_string(const char* fileName, size_t length)
{
    // 0x613CAE
    static char tmp_str[50];

    strncpy(tmp_str, fileName, length);
    return tmp_str;
}

// 0x47AAD8
void lips_bkg_proc()
{
    int v0;
    SpeechMarker* speech_marker;
    int v5;

    v0 = head_marker_current;

    if ((lip_info.flags & LIPS_FLAG_0x02) != 0) {
        int v1 = soundGetPosition(lip_info.sound);

        speech_marker = &(lip_info.markers[v0]);
        while (v1 > speech_marker->position) {
            head_phoneme_current = lip_info.phonemes[v0];
            v0++;

            if (v0 >= lip_info.marker_count) {
                v0 = 0;
                head_phoneme_current = lip_info.phonemes[0];

                if ((lip_info.flags & LIPS_FLAG_0x01) == 0) {
                    // NOTE: Uninline.
                    lips_stop_speech();
                    v0 = head_marker_current;
                }

                break;
            }

            speech_marker = &(lip_info.markers[v0]);
        }

        if (v0 >= lip_info.marker_count - 1) {
            head_marker_current = v0;

            v5 = 0;
            if (lip_info.marker_count <= 5) {
                debug_printf("Error: Too few markers to stop speech!");
            } else {
                v5 = 3;
            }

            speech_marker = &(lip_info.markers[v5]);
            if (v1 < speech_marker->position) {
                v0 = 0;
                head_phoneme_current = lip_info.phonemes[0];

                if ((lip_info.flags & LIPS_FLAG_0x01) == 0) {
                    // NOTE: Uninline.
                    lips_stop_speech();
                    v0 = head_marker_current;
                }
            }
        }
    }

    if (head_phoneme_drawn != head_phoneme_current) {
        head_phoneme_drawn = head_phoneme_current;
        lips_draw_head = true;
    }

    head_marker_current = v0;

    soundContinueAll();
}

// 0x47AC2C
int lips_play_speech()
{
    lip_info.flags |= LIPS_FLAG_0x02;
    head_marker_current = 0;

    if (soundSetPosition(lip_info.sound, lip_info.field_20) != 0) {
        debug_printf("Failed set of start_offset!\n");
    }

    int v2 = head_marker_current;
    while (1) {
        head_marker_current = v2;

        SpeechMarker* speechEntry = &(lip_info.markers[v2]);
        if (lip_info.field_20 <= speechEntry->position) {
            break;
        }

        v2++;

        head_phoneme_current = lip_info.phonemes[v2];
    }

    int speechVolume = gsound_speech_volume_get();
    soundVolume(lip_info.sound, (int)(speechVolume * 0.69));

    speechStartTime = get_time();

    if (soundPlay(lip_info.sound) != 0) {
        debug_printf("Failed play!\n");

        // NOTE: Uninline.
        lips_stop_speech();
    }

    return 0;
}

// NOTE: Inlined.
//
// 0x47AD2C
static int lips_stop_speech()
{
    head_marker_current = 0;
    soundStop(lip_info.sound);
    lip_info.flags &= ~(LIPS_FLAG_0x01 | LIPS_FLAG_0x02);
    return 0;
}

// NOTE: Inlined.
//
// 0x47AD4C
static int lips_read_phoneme_type(unsigned char* phoneme_type, File* stream)
{
    return db_freadByte(stream, phoneme_type);
}

// NOTE: Inlined.
//
// 0x47AD5C
static int lips_read_marker_type(SpeechMarker* marker_type, File* stream)
{
    int marker;

    // Marker is read into temporary variable.
    if (db_freadInt(stream, &marker) == -1) return -1;

    // Position is read directly into struct.
    if (db_freadLong(stream, &(marker_type->position)) == -1) return -1;

    marker_type->marker = marker;

    return 0;
}

// 0x47AD98
static int lips_read_lipsynch_info(LipsData* lipsData, File* stream)
{
    int sound;
    int field_14;
    int phonemes;
    int markers;

    if (db_freadLong(stream, &(lipsData->version)) == -1) return -1;
    if (db_freadLong(stream, &(lipsData->field_4)) == -1) return -1;
    if (db_freadLong(stream, &(lipsData->flags)) == -1) return -1;
    if (db_freadInt(stream, &(sound)) == -1) return -1;
    if (db_freadLong(stream, &(lipsData->field_10)) == -1) return -1;
    if (db_freadInt(stream, &(field_14)) == -1) return -1;
    if (db_freadInt(stream, &(phonemes)) == -1) return -1;
    if (db_freadLong(stream, &(lipsData->field_1C)) == -1) return -1;
    if (db_freadLong(stream, &(lipsData->field_20)) == -1) return -1;
    if (db_freadLong(stream, &(lipsData->phoneme_count)) == -1) return -1;
    if (db_freadLong(stream, &(lipsData->field_28)) == -1) return -1;
    if (db_freadLong(stream, &(lipsData->marker_count)) == -1) return -1;
    if (db_freadInt(stream, &(markers)) == -1) return -1;
    if (db_freadLong(stream, &(lipsData->field_34)) == -1) return -1;
    if (db_freadLong(stream, &(lipsData->field_38)) == -1) return -1;
    if (db_freadLong(stream, &(lipsData->field_3C)) == -1) return -1;
    if (db_freadLong(stream, &(lipsData->field_40)) == -1) return -1;
    if (db_freadLong(stream, &(lipsData->field_44)) == -1) return -1;
    if (db_freadLong(stream, &(lipsData->field_48)) == -1) return -1;
    if (db_freadLong(stream, &(lipsData->field_4C)) == -1) return -1;
    if (db_freadByteCount(stream, lipsData->field_50, 8) == -1) return -1;
    if (db_freadByteCount(stream, lipsData->field_58, 4) == -1) return -1;
    if (db_freadByteCount(stream, lipsData->field_5C, 4) == -1) return -1;
    if (db_freadByteCount(stream, lipsData->field_60, 4) == -1) return -1;
    if (db_freadByteCount(stream, lipsData->field_64, 260) == -1) return -1;

    // TODO: What for?
    lipsData->sound = (Sound*)sound;
    lipsData->field_14 = (void*)field_14;
    lipsData->phonemes = (unsigned char*)phonemes;
    lipsData->markers = (SpeechMarker*)markers;

    return 0;
}

// lips_load_file
// 0x47AFAC
int lips_load_file(const char* audioFileName, const char* headFileName)
{
    char* sep;
    int i;
    char v60[16];

    SpeechMarker* speech_marker;
    SpeechMarker* prev_speech_marker;

    char path[260];
    strcpy(path, "SOUND\\SPEECH\\");

    strcpy(lips_subdir_name, headFileName);

    strcat(path, lips_subdir_name);

    strcat(path, "\\");

    sep = strchr(path, '.');
    if (sep != NULL) {
        *sep = '\0';
    }

    strcpy(v60, audioFileName);

    sep = strchr(v60, '.');
    if (sep != NULL) {
        *sep = '\0';
    }

    strcpy(lip_info.field_50, v60);

    strcat(path, lips_fix_string(lip_info.field_50, sizeof(lip_info.field_50)));
    strcat(path, ".");
    strcat(path, lip_info.field_60);

    lips_free_speech();

    // FIXME: stream is not closed if any error is encountered during reading.
    File* stream = db_fopen(path, "rb");
    if (stream != NULL) {
        if (db_freadLong(stream, &(lip_info.version)) == -1) {
            return -1;
        }

        if (lip_info.version == 1) {
            debug_printf("\nLoading old save-file version (1)");

            if (db_fseek(stream, 0, SEEK_SET) != 0) {
                return -1;
            }

            if (lips_read_lipsynch_info(&lip_info, stream) != 0) {
                return -1;
            }
        } else if (lip_info.version == 2) {
            debug_printf("\nLoading current save-file version (2)");

            if (db_freadLong(stream, &(lip_info.field_4)) == -1) return -1;
            if (db_freadLong(stream, &(lip_info.flags)) == -1) return -1;
            if (db_freadLong(stream, &(lip_info.field_10)) == -1) return -1;
            if (db_freadLong(stream, &(lip_info.field_1C)) == -1) return -1;
            if (db_freadLong(stream, &(lip_info.phoneme_count)) == -1) return -1;
            if (db_freadLong(stream, &(lip_info.field_28)) == -1) return -1;
            if (db_freadLong(stream, &(lip_info.marker_count)) == -1) return -1;
            if (db_freadByteCount(stream, lip_info.field_50, 8) == -1) return -1;
            if (db_freadByteCount(stream, lip_info.field_58, 4) == -1) return -1;
        } else {
            debug_printf("\nError: Lips file WRONG version: %s!", path);
        }
    }

    lip_info.phonemes = (unsigned char*)mem_malloc(lip_info.phoneme_count);
    if (lip_info.phonemes == NULL) {
        debug_printf("Out of memory in lips_load_file.'\n");
        return -1;
    }

    if (stream != NULL) {
        for (i = 0; i < lip_info.phoneme_count; i++) {
            if (lips_read_phoneme_type(&(lip_info.phonemes[i]), stream) != 0) {
                debug_printf("lips_load_file: Error reading phoneme type.\n");
                return -1;
            }
        }

        for (i = 0; i < lip_info.phoneme_count; i++) {
            unsigned char phoneme = lip_info.phonemes[i];
            if (phoneme >= PHONEME_COUNT) {
                debug_printf("\nLoad error: Speech phoneme %d is invalid (%d)!", i, phoneme);
            }
        }
    }

    lip_info.markers = (SpeechMarker*)mem_malloc(sizeof(*speech_marker) * lip_info.marker_count);
    if (lip_info.markers == NULL) {
        debug_printf("Out of memory in lips_load_file.'\n");
        return -1;
    }

    if (stream != NULL) {
        for (i = 0; i < lip_info.marker_count; i++) {
            // NOTE: Uninline.
            if (lips_read_marker_type(&(lip_info.markers[i]), stream) != 0) {
                debug_printf("lips_load_file: Error reading marker type.");
                return -1;
            }
        }

        speech_marker = &(lip_info.markers[0]);

        if (speech_marker->marker != 1 && speech_marker->marker != 0) {
            debug_printf("\nLoad error: Speech marker 0 is invalid (%d)!", speech_marker->marker);
        }

        if (speech_marker->position != 0) {
            debug_printf("Load error: Speech marker 0 has invalid position(%d)!", speech_marker->position);
        }

        for (i = 1; i < lip_info.marker_count; i++) {
            speech_marker = &(lip_info.markers[i]);
            prev_speech_marker = &(lip_info.markers[i - 1]);

            if (speech_marker->marker != 1 && speech_marker->marker != 0) {
                debug_printf("\nLoad error: Speech marker %d is invalid (%d)!", i, speech_marker->marker);
            }

            if (speech_marker->position < prev_speech_marker->position) {
                debug_printf("Load error: Speech marker %d has invalid position(%d)!", i, speech_marker->position);
            }
        }
    }

    if (stream != NULL) {
        db_fclose(stream);
    }

    lip_info.field_38 = 0;
    lip_info.field_34 = 0;
    lip_info.field_48 = 0;
    lip_info.field_20 = 0;
    lip_info.field_3C = 50;
    lip_info.field_40 = 100;

    if (lip_info.version == 1) {
        lip_info.field_4 = 22528;
    }

    strcpy(lip_info.field_58, "ACM");
    strcpy(lip_info.field_5C, "TXT");
    strcpy(lip_info.field_60, "LIP");

    lips_make_speech();

    head_marker_current = 0;
    head_phoneme_current = lip_info.phonemes[0];

    return 0;
}

// 0x47B5D0
static int lips_make_speech()
{
    if (lip_info.field_14 != NULL) {
        mem_free(lip_info.field_14);
        lip_info.field_14 = NULL;
    }

    char path[MAX_PATH];
    char* v1 = lips_fix_string(lip_info.field_50, sizeof(lip_info.field_50));
    sprintf(path, "%s%s\\%s.%s", "SOUND\\SPEECH\\", lips_subdir_name, v1, "ACM");

    if (lip_info.sound != NULL) {
        soundDelete(lip_info.sound);
        lip_info.sound = NULL;
    }

    lip_info.sound = soundAllocate(1, 8);
    if (lip_info.sound == NULL) {
        debug_printf("\nsoundAllocate falied in lips_make_speech!");
        return -1;
    }

    if (soundSetFileIO(lip_info.sound, audioOpen, audioCloseFile, audioRead, NULL, audioSeek, NULL, audioFileSize)) {
        debug_printf("Ack!");
        debug_printf("Error!");
    }

    if (soundLoad(lip_info.sound, path)) {
        soundDelete(lip_info.sound);
        lip_info.sound = NULL;

        debug_printf("lips_make_speech: soundLoad failed with path ");
        debug_printf("%s -- file probably doesn't exist.\n", path);
        return -1;
    }

    lip_info.field_34 = 8 * (lip_info.field_1C / lip_info.marker_count);

    return 0;
}

// 0x47B730
int lips_free_speech()
{
    if (lip_info.field_14 != NULL) {
        mem_free(lip_info.field_14);
        lip_info.field_14 = NULL;
    }

    if (lip_info.sound != NULL) {
        // NOTE: Uninline.
        lips_stop_speech();

        soundDelete(lip_info.sound);

        lip_info.sound = NULL;
    }

    if (lip_info.phonemes != NULL) {
        mem_free(lip_info.phonemes);
        lip_info.phonemes = NULL;
    }

    if (lip_info.markers != NULL) {
        mem_free(lip_info.markers);
        lip_info.markers = NULL;
    }

    return 0;
}
