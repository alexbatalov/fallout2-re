#include "game/selfrun.h"

#include <direct.h>
#include <stdlib.h>

#include "plib/gnw/input.h"
#include "plib/db/db.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "plib/gnw/vcr.h"

typedef enum SelfrunState {
    SELFRUN_STATE_TURNED_OFF,
    SELFRUN_STATE_PLAYING,
    SELFRUN_STATE_RECORDING,
} SelfrunState;

static void selfrun_playback_callback(int reason);
static int selfrun_load_data(const char* path, SelfrunData* selfrunData);
static int selfrun_save_data(const char* path, SelfrunData* selfrunData);

// 0x51C8D8
static int selfrun_state = SELFRUN_STATE_TURNED_OFF;

// 0x4A8BE0
int selfrun_get_list(char*** fileListPtr, int* fileListLengthPtr)
{
    if (fileListPtr == NULL) {
        return -1;
    }

    if (fileListLengthPtr == NULL) {
        return -1;
    }

    *fileListLengthPtr = db_get_file_list("selfrun\\*.sdf", fileListPtr, 0, 0);

    return 0;
}

// 0x4A8C10
int selfrun_free_list(char*** fileListPtr)
{
    if (fileListPtr == NULL) {
        return -1;
    }

    db_free_file_list(fileListPtr, 0);

    return 0;
}

// 0x4A8C28
int selfrun_prep_playback(const char* fileName, SelfrunData* selfrunData)
{
    if (fileName == NULL) {
        return -1;
    }

    if (selfrunData == NULL) {
        return -1;
    }

    if (vcr_status() != VCR_STATE_TURNED_OFF) {
        return -1;
    }

    if (selfrun_state != SELFRUN_STATE_TURNED_OFF) {
        return -1;
    }

    char path[MAX_PATH];
    sprintf(path, "%s%s", "selfrun\\", fileName);

    if (selfrun_load_data(path, selfrunData) != 0) {
        return -1;
    }

    selfrun_state = SELFRUN_STATE_PLAYING;

    return 0;
}

// 0x4A8C88
void selfrun_playback_loop(SelfrunData* selfrunData)
{
    if (selfrun_state == SELFRUN_STATE_PLAYING) {
        char path[MAX_PATH];
        sprintf(path, "%s%s", "selfrun\\", selfrunData->recordingFileName);

        if (vcr_play(path, VCR_TERMINATE_ON_KEY_PRESS | VCR_TERMINATE_ON_MOUSE_PRESS, selfrun_playback_callback)) {
            bool cursorWasHidden = mouse_hidden();
            if (cursorWasHidden) {
                mouse_show();
            }

            while (selfrun_state == SELFRUN_STATE_PLAYING) {
                int keyCode = get_input();
                if (keyCode != selfrunData->stopKeyCode) {
                    game_handle_input(keyCode, false);
                }
            }

            while (mouse_get_buttons() != 0) {
                get_input();
            }

            if (cursorWasHidden) {
                mouse_hide();
            }
        }
    }
}

// 0x4A8D28
int selfrun_prep_recording(const char* recordingName, const char* mapFileName, SelfrunData* selfrunData)
{
    if (recordingName == NULL) {
        return -1;
    }

    if (mapFileName == NULL) {
        return -1;
    }

    if (vcr_status() != VCR_STATE_TURNED_OFF) {
        return -1;
    }

    if (selfrun_state != SELFRUN_STATE_TURNED_OFF) {
        return -1;
    }

    sprintf(selfrunData->recordingFileName, "%s%s", recordingName, ".vcr");
    strcpy(selfrunData->mapFileName, mapFileName);

    selfrunData->stopKeyCode = KEY_CTRL_R;

    char path[MAX_PATH];
    sprintf(path, "%s%s%s", "selfrun\\", recordingName, ".sdf");

    if (selfrun_save_data(path, selfrunData) != 0) {
        return -1;
    }

    selfrun_state = SELFRUN_STATE_RECORDING;

    return 0;
}

// 0x4A8DDC
void selfrun_recording_loop(SelfrunData* selfrunData)
{
    if (selfrun_state == SELFRUN_STATE_RECORDING) {
        char path[MAX_PATH];
        sprintf(path, "%s%s", "selfrun\\", selfrunData->recordingFileName);
        if (vcr_record(path)) {
            if (!mouse_hidden()) {
                mouse_show();
            }

            bool done = false;
            while (!done) {
                int keyCode = get_input();
                if (keyCode == selfrunData->stopKeyCode) {
                    vcr_stop();
                    game_user_wants_to_quit = 2;
                    done = true;
                } else {
                    game_handle_input(keyCode, false);
                }
            }
        }
        selfrun_state = SELFRUN_STATE_TURNED_OFF;
    }
}

// 0x4A8E74
static void selfrun_playback_callback(int reason)
{
    game_user_wants_to_quit = 2;
    selfrun_state = SELFRUN_STATE_TURNED_OFF;
}

// 0x4A8E8C
static int selfrun_load_data(const char* path, SelfrunData* selfrunData)
{
    if (path == NULL) {
        return -1;
    }

    if (selfrunData == NULL) {
        return -1;
    }

    File* stream = db_fopen(path, "rb");
    if (stream == NULL) {
        return -1;
    }

    int rc = -1;
    if (db_freadByteCount(stream, selfrunData->recordingFileName, SELFRUN_RECORDING_FILE_NAME_LENGTH) == 0
        && db_freadByteCount(stream, selfrunData->mapFileName, SELFRUN_MAP_FILE_NAME_LENGTH) == 0
        && db_freadInt(stream, &(selfrunData->stopKeyCode)) == 0) {
        rc = 0;
    }

    db_fclose(stream);

    return rc;
}

// 0x4A8EF4
static int selfrun_save_data(const char* path, SelfrunData* selfrunData)
{
    if (path == NULL) {
        return -1;
    }

    if (selfrunData == NULL) {
        return -1;
    }

    char* masterPatches;
    config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_MASTER_PATCHES_KEY, &masterPatches);

    char selfrunDirectoryPath[MAX_PATH];
    sprintf(selfrunDirectoryPath, "%s\\%s", masterPatches, "selfrun\\");

    mkdir(selfrunDirectoryPath);

    File* stream = db_fopen(path, "wb");
    if (stream == NULL) {
        return -1;
    }

    int rc = -1;
    if (db_fwriteByteCount(stream, selfrunData->recordingFileName, SELFRUN_RECORDING_FILE_NAME_LENGTH) == 0
        && db_fwriteByteCount(stream, selfrunData->mapFileName, SELFRUN_MAP_FILE_NAME_LENGTH) == 0
        && db_fwriteInt(stream, selfrunData->stopKeyCode) == 0) {
        rc = 0;
    }

    db_fclose(stream);

    return rc;
}
