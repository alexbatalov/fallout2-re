#include "game/gsound.h"

#include <stdio.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "game/anim.h"
#include "int/audio.h"
#include "int/audiof.h"
#include "game/combat.h"
#include "plib/gnw/input.h"
#include "plib/db/db.h"
#include "plib/gnw/debug.h"
#include "game/gconfig.h"
#include "game/item.h"
#include "game/map.h"
#include "plib/gnw/memory.h"
#include "int/movie.h"
#include "game/object.h"
#include "game/proto.h"
#include "game/queue.h"
#include "game/roll.h"
#include "game/sfxcache.h"
#include "game/stat.h"
#include "plib/gnw/gnw.h"
#include "game/worldmap.h"

static void gsound_bkg_proc();
static int gsound_open(const char* fname, int access, ...);
static long gsound_compressed_tell(int handle);
static int gsound_write(int handle, const void* buf, unsigned int size);
static int gsound_close(int handle);
static int gsound_read(int handle, void* buf, unsigned int size);
static long gsound_seek(int handle, long offset, int origin);
static long gsound_tell(int handle);
static long gsound_filesize(int handle);
static bool gsound_compressed_query(char* filePath);
static void gsound_internal_speech_callback(void* userData, int a2);
static void gsound_internal_background_callback(void* userData, int a2);
static void gsound_internal_effect_callback(void* userData, int a2);
static int gsound_background_allocate(Sound** out_s, int a2, int a3);
static int gsound_background_find_with_copy(char* dest, const char* src);
static int gsound_background_find_dont_copy(char* dest, const char* src);
static int gsound_speech_find_dont_copy(char* dest, const char* src);
static void gsound_background_remove_last_copy();
static int gsound_background_start();
static int gsound_speech_start();
static int gsound_get_music_path(char** out_value, const char* key);
static Sound* gsound_get_sound_ready_for_effect();
static bool gsound_file_exists_f(const char* fname);
static int gsound_file_exists_db(const char* path);
static int gsound_setup_paths();

// TODO: Remove.
// 0x5035BC
char _aSoundSfx[] = "sound\\sfx\\";

// TODO: Remove.
// 0x5035C8
char _aSoundMusic_0[] = "sound\\music\\";

// TODO: Remove.
// 0x5035D8
char _aSoundSpeech_0[] = "sound\\speech\\";

// 0x518E30
static bool gsound_initialized = false;

// 0x518E34
static bool gsound_debug = false;

// 0x518E38
static bool gsound_background_enabled = false;

// 0x518E3C
static int _gsound_background_df_vol = 0;

// 0x518E40
static int gsound_background_fade = 0;

// 0x518E44
static bool gsound_speech_enabled = false;

// 0x518E48
static bool gsound_sfx_enabled = false;

// number of active effects (max 4)
//
// 0x518E4C
static int gsound_active_effect_counter;

// 0x518E50
static Sound* gsound_background_tag = NULL;

// 0x518E54
static Sound* gsound_speech_tag = NULL;

// 0x518E58
static SoundEndCallback* gsound_background_callback_fp = NULL;

// 0x518E5C
static SoundEndCallback* gsound_speech_callback_fp = NULL;

// 0x518E60
static char snd_lookup_weapon_type[WEAPON_SOUND_EFFECT_COUNT] = {
    'R', // Ready
    'A', // Attack
    'O', // Out of ammo
    'F', // Firing
    'H', // Hit
};

// 0x518E65
static char snd_lookup_scenery_action[SCENERY_SOUND_EFFECT_COUNT] = {
    'O', // Open
    'C', // Close
    'L', // Lock
    'N', // Unlock
    'U', // Use
};

// 0x518E6C
static int background_storage_requested = -1;

// 0x518E70
static int background_loop_requested = -1;

// 0x518E74
static char sound_sfx_path[] = "sound\\sfx\\";

// 0x518E78
static char* sound_music_path1 = _aSoundMusic_0;

// 0x518E7C
static char* sound_music_path2 = _aSoundMusic_0;

// 0x518E80
static char* sound_speech_path = _aSoundSpeech_0;

// 0x518E84
static int master_volume = VOLUME_MAX;

// 0x518E88
static int background_volume = VOLUME_MAX;

// 0x518E8C
static int speech_volume = VOLUME_MAX;

// 0x518E90
static int sndfx_volume = VOLUME_MAX;

// 0x518E94
static int detectDevices = -1;

// 0x596EB0
static char background_fname_copied[MAX_PATH];

// 0x596FB5
static char sfx_file_name[13];

// NOTE: I'm mot sure about it's size. Why not MAX_PATH?
//
// 0x596FC2
static char background_fname_requested[270];

// 0x44FC70
int gsound_init()
{
    if (gsound_initialized) {
        if (gsound_debug) {
            debug_printf("Trying to initialize gsound twice.\n");
        }
        return -1;
    }

    bool initialize;
    configGetBool(&game_config, GAME_CONFIG_SOUND_KEY, GAME_CONFIG_INITIALIZE_KEY, &initialize);
    if (!initialize) {
        return 0;
    }

    configGetBool(&game_config, GAME_CONFIG_SOUND_KEY, GAME_CONFIG_DEBUG_KEY, &gsound_debug);

    if (gsound_debug) {
        debug_printf("Initializing sound system...");
    }

    if (gsound_get_music_path(&sound_music_path1, GAME_CONFIG_MUSIC_PATH1_KEY) != 0) {
        return -1;
    }

    if (gsound_get_music_path(&sound_music_path2, GAME_CONFIG_MUSIC_PATH2_KEY) != 0) {
        return -1;
    }

    if (strlen(sound_music_path1) > 247 || strlen(sound_music_path2) > 247) {
        if (gsound_debug) {
            debug_printf("Music paths way too long.\n");
        }
        return -1;
    }

    // gsound_setup_paths
    if (gsound_setup_paths() != 0) {
        return -1;
    }

    soundRegisterAlloc(mem_malloc, mem_realloc, mem_free);

    // initialize direct sound
    if (soundInit(detectDevices, 24, 0x8000, 0x8000, 22050) != 0) {
        if (gsound_debug) {
            debug_printf("failed!\n");
        }

        return -1;
    }

    if (gsound_debug) {
        debug_printf("success.\n");
    }

    initAudiof(gsound_compressed_query);
    initAudio(gsound_compressed_query);

    int cacheSize;
    config_get_value(&game_config, GAME_CONFIG_SOUND_KEY, GAME_CONFIG_CACHE_SIZE_KEY, &cacheSize);
    if (cacheSize >= 0x40000) {
        debug_printf("\n!!! Config file needs adustment.  Please remove the ");
        debug_printf("cache_size line and run fallout again.  This will reset ");
        debug_printf("cache_size to the new default, which is expressed in K.\n");
        return -1;
    }

    if (sfxc_init(cacheSize << 10, sound_sfx_path) != 0) {
        if (gsound_debug) {
            debug_printf("Unable to initialize sound effects cache.\n");
        }
    }

    if (soundSetDefaultFileIO(gsound_open, gsound_close, gsound_read, gsound_write, gsound_seek, gsound_tell, gsound_filesize) != 0) {
        if (gsound_debug) {
            debug_printf("Failure setting sound I/O calls.\n");
        }
        return -1;
    }

    add_bk_process(gsound_bkg_proc);
    gsound_initialized = true;

    // SOUNDS
    bool sounds = 0;
    configGetBool(&game_config, GAME_CONFIG_SOUND_KEY, GAME_CONFIG_SOUNDS_KEY, &sounds);

    if (gsound_debug) {
        debug_printf("Sounds are ");
    }

    if (sounds) {
        // NOTE: Uninline.
        gsound_sfx_enable();
    } else {
        if (gsound_debug) {
            debug_printf(" not ");
        }
    }

    if (gsound_debug) {
        debug_printf("on.\n");
    }

    // MUSIC
    bool music = 0;
    configGetBool(&game_config, GAME_CONFIG_SOUND_KEY, GAME_CONFIG_MUSIC_KEY, &music);

    if (gsound_debug) {
        debug_printf("Music is ");
    }

    if (music) {
        // NOTE: Uninline.
        gsound_background_enable();
    } else {
        if (gsound_debug) {
            debug_printf(" not ");
        }
    }

    if (gsound_debug) {
        debug_printf("on.\n");
    }

    // SPEEECH
    bool speech = 0;
    configGetBool(&game_config, GAME_CONFIG_SOUND_KEY, GAME_CONFIG_SPEECH_KEY, &speech);

    if (gsound_debug) {
        debug_printf("Speech is ");
    }

    if (speech) {
        // NOTE: Uninline.
        gsound_speech_enable();
    } else {
        if (gsound_debug) {
            debug_printf(" not ");
        }
    }

    if (gsound_debug) {
        debug_printf("on.\n");
    }

    config_get_value(&game_config, GAME_CONFIG_SOUND_KEY, GAME_CONFIG_MASTER_VOLUME_KEY, &master_volume);
    gsound_set_master_volume(master_volume);

    config_get_value(&game_config, GAME_CONFIG_SOUND_KEY, GAME_CONFIG_MUSIC_VOLUME_KEY, &background_volume);
    gsound_background_volume_set(background_volume);

    config_get_value(&game_config, GAME_CONFIG_SOUND_KEY, GAME_CONFIG_SNDFX_VOLUME_KEY, &sndfx_volume);
    gsound_set_sfx_volume(sndfx_volume);

    config_get_value(&game_config, GAME_CONFIG_SOUND_KEY, GAME_CONFIG_SPEECH_VOLUME_KEY, &speech_volume);
    gsound_speech_volume_set(speech_volume);

    // NOTE: Uninline.
    gsound_background_fade_set(0);
    background_fname_requested[0] = '\0';

    return 0;
}

// 0x450164
void gsound_reset()
{
    if (!gsound_initialized) {
        return;
    }

    if (gsound_debug) {
        debug_printf("Resetting sound system...");
    }

    // NOTE: Uninline.
    gsound_speech_stop();

    if (_gsound_background_df_vol) {
        // NOTE: Uninline.
        gsound_background_enable();
    }

    gsound_background_stop();

    // NOTE: Uninline.
    gsound_background_fade_set(0);

    soundFlushAllSounds();

    sfxc_flush();

    gsound_active_effect_counter = 0;

    if (gsound_debug) {
        debug_printf("done.\n");
    }

    return;
}

// 0x450244
int gsound_exit()
{
    if (!gsound_initialized) {
        return -1;
    }

    remove_bk_process(gsound_bkg_proc);

    // NOTE: Uninline.
    gsound_speech_stop();

    gsound_background_stop();
    gsound_background_remove_last_copy();
    soundClose();
    sfxc_exit();
    audiofClose();
    audioClose();

    gsound_initialized = false;

    return 0;
}

// NOTE: Inlined.
//
// 0x4502BC
void gsound_sfx_enable()
{
    if (gsound_initialized) {
        gsound_sfx_enabled = true;
    }
}

// NOTE: Inlined.
//
// 0x4502D0
void gsound_sfx_disable()
{
    if (gsound_initialized) {
        gsound_sfx_enabled = false;
    }
}

// 0x4502E4
int gsound_sfx_is_enabled()
{
    return gsound_sfx_enabled;
}

// 0x4502EC
int gsound_set_master_volume(int volume)
{
    if (!gsound_initialized) {
        return -1;
    }

    if (volume < VOLUME_MIN && volume > VOLUME_MAX) {
        if (gsound_debug) {
            debug_printf("Requested master volume out of range.\n");
        }
        return -1;
    }

    if (_gsound_background_df_vol && volume != 0 && gsound_background_volume_get() != 0) {
        // NOTE: Uninline.
        gsound_background_enable();
        _gsound_background_df_vol = 0;
    }

    if (soundSetMasterVolume(volume) != 0) {
        if (gsound_debug) {
            debug_printf("Error setting master sound volume.\n");
        }
        return -1;
    }

    master_volume = volume;
    if (gsound_background_enabled && volume == 0) {
        // NOTE: Uninline.
        gsound_background_disable();
        _gsound_background_df_vol = 1;
    }

    return 0;
}

// 0x450410
int gsound_get_master_volume()
{
    return master_volume;
}

// 0x450418
int gsound_set_sfx_volume(int volume)
{
    if (!gsound_initialized || volume < VOLUME_MIN || volume > VOLUME_MAX) {
        if (gsound_debug) {
            debug_printf("Error setting sfx volume.\n");
        }
        return -1;
    }

    sndfx_volume = volume;

    return 0;
}

// 0x450454
int gsound_get_sfx_volume()
{
    return sndfx_volume;
}

// NOTE: Inlined.
//
// 0x45045C
void gsound_background_disable()
{
    if (gsound_initialized) {
        if (gsound_background_enabled) {
            gsound_background_stop();
            movieSetVolume(0);
            gsound_background_enabled = false;
        }
    }
}

// NOTE: Inlined.
//
// 0x450488
void gsound_background_enable()
{
    if (gsound_initialized) {
        if (!gsound_background_enabled) {
            movieSetVolume((int)(background_volume * 0.94));
            gsound_background_enabled = true;
            gsound_background_restart_last(12);
        }
    }
}

// 0x4504D4
int gsound_background_is_enabled()
{
    return gsound_background_enabled;
}

// 0x4504DC
void gsound_background_volume_set(int volume)
{
    if (!gsound_initialized) {
        return;
    }

    if (volume < VOLUME_MIN || volume > VOLUME_MAX) {
        if (gsound_debug) {
            debug_printf("Requested background volume out of range.\n");
        }
        return;
    }

    background_volume = volume;

    if (_gsound_background_df_vol) {
        // NOTE: Uninline.
        gsound_background_enable();
        _gsound_background_df_vol = 0;
    }

    if (gsound_background_enabled) {
        movieSetVolume((int)(volume * 0.94));
    }

    if (gsound_background_enabled) {
        if (gsound_background_tag != NULL) {
            soundVolume(gsound_background_tag, (int)(background_volume * 0.94));
        }
    }

    if (gsound_background_enabled) {
        if (volume == 0 || gsound_get_master_volume() == 0) {
            // NOTE: Uninline.
            gsound_background_disable();
            _gsound_background_df_vol = 1;
        }
    }
}

// 0x450618
int gsound_background_volume_get()
{
    return background_volume;
}

// 0x450620
int gsound_background_volume_get_set(int volume)
{
    int oldMusicVolume;

    // NOTE: Uninline.
    oldMusicVolume = gsound_background_volume_get();
    gsound_background_volume_set(volume);

    return oldMusicVolume;
}

// NOTE: Inlined.
//
// 0x450630
void gsound_background_fade_set(int value)
{
    gsound_background_fade = value;
}

// NOTE: Inlined.
//
// 0x450638
int gsound_background_fade_get()
{
    return gsound_background_fade;
}

// NOTE: Unused.
//
// 0x450640
int gsound_background_fade_get_set(int value)
{
    int oldValue;

    // NOTE: Uninline.
    oldValue = gsound_background_fade_get();

    // NOTE: Uninline.
    gsound_background_fade_set(value);

    return oldValue;
}

// 0x450650
void gsound_background_callback_set(SoundEndCallback* callback)
{
    gsound_background_callback_fp = callback;
}

// 0x450658
SoundEndCallback* gsound_background_callback_get()
{
    return gsound_background_callback_fp;
}

// 0x450660
SoundEndCallback* gsound_background_callback_get_set(SoundEndCallback* callback)
{
    SoundEndCallback* oldCallback;

    // NOTE: Uninline.
    oldCallback = gsound_background_callback_get();

    // NOTE: Uninline.
    gsound_background_callback_set(callback);

    return oldCallback;
}

// NOTE: There are no references to this function.
//
// 0x450670
int gsound_background_length_get()
{
    return soundLength(gsound_background_tag);
}

// [fileName] is base file name, without path and extension.
//
// 0x45067C
int gsound_background_play(const char* fileName, int a2, int a3, int a4)
{
    int rc;

    background_storage_requested = a3;
    background_loop_requested = a4;

    strcpy(background_fname_requested, fileName);

    if (!gsound_initialized) {
        return -1;
    }

    if (!gsound_background_enabled) {
        return -1;
    }

    if (gsound_debug) {
        debug_printf("Loading background sound file %s%s...", fileName, ".acm");
    }

    gsound_background_stop();

    rc = gsound_background_allocate(&gsound_background_tag, a3, a4);
    if (rc != 0) {
        if (gsound_debug) {
            debug_printf("failed because sound could not be allocated.\n");
        }

        gsound_background_tag = NULL;
        return -1;
    }

    rc = soundSetFileIO(gsound_background_tag, audiofOpen, audiofCloseFile, audiofRead, NULL, audiofSeek, gsound_compressed_tell, audiofFileSize);
    if (rc != 0) {
        if (gsound_debug) {
            debug_printf("failed because file IO could not be set for compression.\n");
        }

        soundDelete(gsound_background_tag);
        gsound_background_tag = NULL;

        return -1;
    }

    rc = soundSetChannel(gsound_background_tag, 3);
    if (rc != 0) {
        if (gsound_debug) {
            debug_printf("failed because the channel could not be set.\n");
        }

        soundDelete(gsound_background_tag);
        gsound_background_tag = NULL;

        return -1;
    }

    char path[MAX_PATH + 1];
    if (a3 == 13) {
        rc = gsound_background_find_dont_copy(path, fileName);
    } else if (a3 == 14) {
        rc = gsound_background_find_with_copy(path, fileName);
    }

    if (rc != SOUND_NO_ERROR) {
        if (gsound_debug) {
            debug_printf("'failed because the file could not be found.\n");
        }

        soundDelete(gsound_background_tag);
        gsound_background_tag = NULL;

        return -1;
    }

    if (a4 == 16) {
        rc = soundLoop(gsound_background_tag, 0xFFFF);
        if (rc != SOUND_NO_ERROR) {
            if (gsound_debug) {
                debug_printf("failed because looping could not be set.\n");
            }

            soundDelete(gsound_background_tag);
            gsound_background_tag = NULL;

            return -1;
        }
    }

    rc = soundSetCallback(gsound_background_tag, gsound_internal_background_callback, NULL);
    if (rc != SOUND_NO_ERROR) {
        if (gsound_debug) {
            debug_printf("soundSetCallback failed for background sound\n");
        }
    }

    if (a2 == 11) {
        rc = soundSetReadLimit(gsound_background_tag, 0x40000);
        if (rc != SOUND_NO_ERROR) {
            if (gsound_debug) {
                debug_printf("unable to set read limit ");
            }
        }
    }

    rc = soundLoad(gsound_background_tag, path);
    if (rc != SOUND_NO_ERROR) {
        if (gsound_debug) {
            debug_printf("failed on call to soundLoad.\n");
        }

        soundDelete(gsound_background_tag);
        gsound_background_tag = NULL;

        return -1;
    }

    if (a2 != 11) {
        rc = soundSetReadLimit(gsound_background_tag, 0x40000);
        if (rc != 0) {
            if (gsound_debug) {
                debug_printf("unable to set read limit ");
            }
        }
    }

    if (a2 == 10) {
        return 0;
    }

    rc = gsound_background_start();
    if (rc != 0) {
        if (gsound_debug) {
            debug_printf("failed starting to play.\n");
        }

        soundDelete(gsound_background_tag);
        gsound_background_tag = NULL;

        return -1;
    }

    if (gsound_debug) {
        debug_printf("succeeded.\n");
    }

    return 0;
}

// 0x450A08
int gsound_background_play_level_music(const char* a1, int a2)
{
    return gsound_background_play(a1, a2, 14, 16);
}

// 0x450A1C
int gsound_background_play_preloaded()
{
    if (!gsound_initialized) {
        return -1;
    }

    if (!gsound_background_enabled) {
        return -1;
    }

    if (gsound_background_tag == NULL) {
        return -1;
    }

    if (soundPlaying(gsound_background_tag)) {
        return -1;
    }

    if (soundPaused(gsound_background_tag)) {
        return -1;
    }

    if (soundDone(gsound_background_tag)) {
        return -1;
    }

    if (gsound_background_start() != 0) {
        soundDelete(gsound_background_tag);
        gsound_background_tag = NULL;
        return -1;
    }

    return 0;
}

// 0x450AB4
void gsound_background_stop()
{
    if (gsound_initialized && gsound_background_enabled && gsound_background_tag) {
        if (gsound_background_fade) {
            if (soundFade(gsound_background_tag, 2000, 0) == 0) {
                gsound_background_tag = NULL;
                return;
            }
        }

        soundDelete(gsound_background_tag);
        gsound_background_tag = NULL;
    }
}

// 0x450B0C
void gsound_background_restart_last(int value)
{
    if (background_fname_requested[0] != '\0') {
        if (gsound_background_play(background_fname_requested, value, background_storage_requested, background_loop_requested) != 0) {
            if (gsound_debug)
                debug_printf(" background restart failed ");
        }
    }
}

// 0x450B50
void gsound_background_pause()
{
    if (gsound_background_tag != NULL) {
        soundPause(gsound_background_tag);
    }
}

// 0x450B64
void gsound_background_unpause()
{
    if (gsound_background_tag != NULL) {
        soundUnpause(gsound_background_tag);
    }
}

// NOTE: Inlined.
//
// 0x450B78
void gsound_speech_disable()
{
    if (gsound_initialized) {
        if (gsound_speech_enabled) {
            gsound_speech_stop();
            gsound_speech_enabled = false;
        }
    }
}

// NOTE: Inlined.
//
// 0x450BC0
void gsound_speech_enable()
{
    if (gsound_initialized) {
        if (!gsound_speech_enabled) {
            gsound_speech_enabled = true;
        }
    }
}

// 0x450BE0
int gsound_speech_is_enabled()
{
    return gsound_speech_enabled;
}

// 0x450BE8
void gsound_speech_volume_set(int volume)
{
    if (!gsound_initialized) {
        return;
    }

    if (volume < VOLUME_MIN || volume > VOLUME_MAX) {
        if (gsound_debug) {
            debug_printf("Requested speech volume out of range.\n");
        }
        return;
    }

    speech_volume = volume;

    if (gsound_speech_enabled) {
        if (gsound_speech_tag != NULL) {
            soundVolume(gsound_speech_tag, (int)(volume * 0.69));
        }
    }
}

// 0x450C5C
int gsound_speech_volume_get()
{
    return speech_volume;
}

// 0x450C64
int gsound_speech_volume_get_set(int volume)
{
    int oldVolume = speech_volume;
    gsound_speech_volume_set(volume);
    return oldVolume;
}

// 0x450C74
void gsound_speech_callback_set(SoundEndCallback* callback)
{
    gsound_speech_callback_fp = callback;
}

// 0x450C7C
SoundEndCallback* gsound_speech_callback_get()
{
    return gsound_speech_callback_fp;
}

// 0x450C84
SoundEndCallback* gsound_speech_callback_get_set(SoundEndCallback* callback)
{
    SoundEndCallback* oldCallback;

    // NOTE: Uninline.
    oldCallback = gsound_speech_callback_get();

    // NOTE: Uninline.
    gsound_speech_callback_set(callback);

    return oldCallback;
}

// 0x450C94
int gsound_speech_length_get()
{
    return soundLength(gsound_speech_tag);
}

// 0x450CA0
int gsound_speech_play(const char* fname, int a2, int a3, int a4)
{
    char path[MAX_PATH + 1];

    if (!gsound_initialized) {
        return -1;
    }

    if (!gsound_speech_enabled) {
        return -1;
    }

    if (gsound_debug) {
        debug_printf("Loading speech sound file %s%s...", fname, ".ACM");
    }

    // uninline
    gsound_speech_stop();

    if (gsound_background_allocate(&gsound_speech_tag, a3, a4)) {
        if (gsound_debug) {
            debug_printf("failed because sound could not be allocated.\n");
        }
        gsound_speech_tag = NULL;
        return -1;
    }

    if (soundSetFileIO(gsound_speech_tag, &audioOpen, &audioCloseFile, &audioRead, NULL, &audioSeek, &gsound_compressed_tell, &audioFileSize)) {
        if (gsound_debug) {
            debug_printf("failed because file IO could not be set for compression.\n");
        }
        soundDelete(gsound_speech_tag);
        gsound_speech_tag = NULL;
        return -1;
    }

    if (gsound_speech_find_dont_copy(path, fname)) {
        if (gsound_debug) {
            debug_printf("failed because the file could not be found.\n");
        }
        soundDelete(gsound_speech_tag);
        gsound_speech_tag = NULL;
        return -1;
    }

    if (a4 == 16) {
        if (soundLoop(gsound_speech_tag, 0xFFFF)) {
            if (gsound_debug) {
                debug_printf("failed because looping could not be set.\n");
            }
            soundDelete(gsound_speech_tag);
            gsound_speech_tag = NULL;
            return -1;
        }
    }

    if (soundSetCallback(gsound_speech_tag, gsound_internal_speech_callback, NULL)) {
        if (gsound_debug) {
            debug_printf("soundSetCallback failed for speech sound\n");
        }
    }

    if (a2 == 11) {
        if (soundSetReadLimit(gsound_speech_tag, 0x40000)) {
            if (gsound_debug) {
                debug_printf("unable to set read limit ");
            }
        }
    }

    if (soundLoad(gsound_speech_tag, path)) {
        if (gsound_debug) {
            debug_printf("failed on call to soundLoad.\n");
        }
        soundDelete(gsound_speech_tag);
        gsound_speech_tag = NULL;
        return -1;
    }

    if (a2 != 11) {
        if (soundSetReadLimit(gsound_speech_tag, 0x40000)) {
            if (gsound_debug) {
                debug_printf("unable to set read limit ");
            }
        }
    }

    if (a2 == 10) {
        return 0;
    }

    if (gsound_speech_start()) {
        if (gsound_debug) {
            debug_printf("failed starting to play.\n");
        }
        soundDelete(gsound_speech_tag);
        gsound_speech_tag = NULL;
        return -1;
    }

    if (gsound_debug) {
        debug_printf("succeeded.\n");
    }

    return 0;
}

// 0x450F8C
int gsound_speech_play_preloaded()
{
    if (!gsound_initialized) {
        return -1;
    }

    if (!gsound_speech_enabled) {
        return -1;
    }

    if (gsound_speech_tag == NULL) {
        return -1;
    }

    if (soundPlaying(gsound_speech_tag)) {
        return -1;
    }

    if (soundPaused(gsound_speech_tag)) {
        return -1;
    }

    if (soundDone(gsound_speech_tag)) {
        return -1;
    }

    if (gsound_speech_start() != 0) {
        soundDelete(gsound_speech_tag);
        gsound_speech_tag = NULL;

        return -1;
    }

    return 0;
}

// 0x451024
void gsound_speech_stop()
{
    if (gsound_initialized && gsound_speech_enabled) {
        if (gsound_speech_tag != NULL) {
            soundDelete(gsound_speech_tag);
            gsound_speech_tag = NULL;
        }
    }
}

// 0x451054
void gsound_speech_pause()
{
    if (gsound_speech_tag != NULL) {
        soundPause(gsound_speech_tag);
    }
}

// 0x451068
void gsound_speech_unpause()
{
    if (gsound_speech_tag != NULL) {
        soundUnpause(gsound_speech_tag);
    }
}

// 0x45108C
int gsound_play_sfx_file_volume(const char* a1, int a2)
{
    Sound* v1;

    if (!gsound_initialized) {
        return -1;
    }

    if (!gsound_sfx_enabled) {
        return -1;
    }

    v1 = gsound_load_sound_volume(a1, NULL, a2);
    if (v1 == NULL) {
        return -1;
    }

    soundPlay(v1);

    return 0;
}

// 0x4510DC
Sound* gsound_load_sound(const char* name, Object* object)
{
    if (!gsound_initialized) {
        return NULL;
    }

    if (!gsound_sfx_enabled) {
        return NULL;
    }

    if (gsound_debug) {
        debug_printf("Loading sound file %s%s...", name, ".ACM");
    }

    if (gsound_active_effect_counter >= SOUND_EFFECTS_MAX_COUNT) {
        if (gsound_debug) {
            debug_printf("failed because there are already %d active effects.\n", gsound_active_effect_counter);
        }

        return NULL;
    }

    Sound* sound = gsound_get_sound_ready_for_effect();
    if (sound == NULL) {
        if (gsound_debug) {
            debug_printf("failed.\n");
        }

        return NULL;
    }

    ++gsound_active_effect_counter;

    char path[MAX_PATH];
    sprintf(path, "%s%s%s", sound_sfx_path, name, ".ACM");

    if (soundLoad(sound, path) == 0) {
        if (gsound_debug) {
            debug_printf("succeeded.\n");
        }

        return sound;
    }

    if (object != NULL) {
        if (FID_TYPE(object->fid) == OBJ_TYPE_CRITTER && (name[0] == 'H' || name[0] == 'N')) {
            char v9 = name[1];
            if (v9 == 'A' || v9 == 'F' || v9 == 'M') {
                if (v9 == 'A') {
                    if (critterGetStat(object, STAT_GENDER)) {
                        v9 = 'F';
                    } else {
                        v9 = 'M';
                    }
                }
            }

            sprintf(path, "%sH%cXXXX%s%s", sound_sfx_path, v9, name + 6, ".ACM");

            if (gsound_debug) {
                debug_printf("tyring %s ", path + strlen(sound_sfx_path));
            }

            if (soundLoad(sound, path) == 0) {
                if (gsound_debug) {
                    debug_printf("succeeded (with alias).\n");
                }

                return sound;
            }

            if (v9 == 'F') {
                sprintf(path, "%sHMXXXX%s%s", sound_sfx_path, name + 6, ".ACM");

                if (gsound_debug) {
                    debug_printf("tyring %s ", path + strlen(sound_sfx_path));
                }

                if (soundLoad(sound, path) == 0) {
                    if (gsound_debug) {
                        debug_printf("succeeded (with male alias).\n");
                    }

                    return sound;
                }
            }
        }
    }

    if (strncmp(name, "MALIEU", 6) == 0 || strncmp(name, "MAMTN2", 6) == 0) {
        sprintf(path, "%sMAMTNT%s%s", sound_sfx_path, name + 6, ".ACM");

        if (gsound_debug) {
            debug_printf("tyring %s ", path + strlen(sound_sfx_path));
        }

        if (soundLoad(sound, path) == 0) {
            if (gsound_debug) {
                debug_printf("succeeded (with alias).\n");
            }

            return sound;
        }
    }

    --gsound_active_effect_counter;

    soundDelete(sound);

    if (gsound_debug) {
        debug_printf("failed.\n");
    }

    return NULL;
}

// 0x45145C
Sound* gsound_load_sound_volume(const char* name, Object* object, int volume)
{
    Sound* sound = gsound_load_sound(name, object);

    if (sound != NULL) {
        soundVolume(sound, (volume * sndfx_volume) / VOLUME_MAX);
    }

    return sound;
}

// 0x45148C
void gsound_delete_sfx(Sound* sound)
{
    if (!gsound_initialized) {
        return;
    }

    if (!gsound_sfx_enabled) {
        return;
    }

    if (soundPlaying(sound)) {
        if (gsound_debug) {
            debug_printf("Trying to manually delete a sound effect after it has started playing.\n");
        }
        return;
    }

    if (soundDelete(sound) != 0) {
        if (gsound_debug) {
            debug_printf("Unable to delete sound effect -- active effect counter may get out of sync.\n");
        }
        return;
    }

    --gsound_active_effect_counter;
}

// 0x4514F0
int gsnd_anim_sound(Sound* sound, void* a2)
{
    if (!gsound_initialized) {
        return 0;
    }

    if (!gsound_sfx_enabled) {
        return 0;
    }

    if (sound == NULL) {
        return 0;
    }

    soundPlay(sound);

    return 0;
}

// 0x451510
int gsound_play_sound(Sound* sound)
{
    if (!gsound_initialized) {
        return -1;
    }

    if (!gsound_sfx_enabled) {
        return -1;
    }

    if (sound == NULL) {
        return -1;
    }

    soundPlay(sound);

    return 0;
}

// Probably returns volume dependending on the distance between the specified
// object and dude.
//
// 0x451534
int gsound_compute_relative_volume(Object* obj)
{
    int type;
    int v3;
    Object* v7;
    Rect v12;
    Rect v14;
    Rect iso_win_rect;
    int distance;
    int perception;

    v3 = 0x7FFF;

    if (obj) {
        type = FID_TYPE(obj->fid);
        if (type == 0 || type == 1 || type == 2) {
            v7 = obj_top_environment(obj);
            if (!v7) {
                v7 = obj;
            }

            obj_bound(v7, &v14);

            win_get_rect(display_win, &iso_win_rect);

            if (rect_inside_bound(&v14, &iso_win_rect, &v12) == -1) {
                distance = obj_dist(v7, obj_dude);
                perception = critterGetStat(obj_dude, STAT_PERCEPTION);
                if (distance > perception) {
                    if (distance < 2 * perception) {
                        v3 = 0x7FFF - 0x5554 * (distance - perception) / perception;
                    } else {
                        v3 = 0x2AAA;
                    }
                } else {
                    v3 = 0x7FFF;
                }
            }
        }
    }

    return v3;
}

// sfx_build_char_name
// 0x451604
char* gsnd_build_character_sfx_name(Object* a1, int anim, int extra)
{
    char v7[13];
    char v8;
    char v9;

    if (art_get_base_name(FID_TYPE(a1->fid), a1->fid & 0xFFF, v7) == -1) {
        return NULL;
    }

    if (anim == ANIM_TAKE_OUT) {
        if (art_get_code(anim, extra, &v8, &v9) == -1) {
            return NULL;
        }
    } else {
        if (art_get_code(anim, (a1->fid & 0xF000) >> 12, &v8, &v9) == -1) {
            return NULL;
        }
    }

    // TODO: Check.
    if (anim == ANIM_FALL_FRONT || anim == ANIM_FALL_BACK) {
        if (extra == CHARACTER_SOUND_EFFECT_PASS_OUT) {
            v8 = 'Y';
        } else if (extra == CHARACTER_SOUND_EFFECT_DIE) {
            v8 = 'Z';
        }
    } else if ((anim == ANIM_THROW_PUNCH || anim == ANIM_KICK_LEG) && extra == CHARACTER_SOUND_EFFECT_CONTACT) {
        v8 = 'Z';
    }

    sprintf(sfx_file_name, "%s%c%c", v7, v8, v9);
    strupr(sfx_file_name);
    return sfx_file_name;
}

// sfx_build_ambient_name
// 0x4516F0
char* gsnd_build_ambient_sfx_name(const char* a1)
{
    sprintf(sfx_file_name, "A%6s%1d", a1, 1);
    strupr(sfx_file_name);
    return sfx_file_name;
}

// sfx_build_interface_name
// 0x451718
char* gsnd_build_interface_sfx_name(const char* a1)
{
    sprintf(sfx_file_name, "N%6s%1d", a1, 1);
    strupr(sfx_file_name);
    return sfx_file_name;
}

// sfx_build_weapon_name
// 0x451760
char* gsnd_build_weapon_sfx_name(int effectType, Object* weapon, int hitMode, Object* target)
{
    int v6;
    char weaponSoundCode;
    char effectTypeCode;
    char materialCode;
    Proto* proto;

    weaponSoundCode = item_w_sound_id(weapon);
    effectTypeCode = snd_lookup_weapon_type[effectType];

    if (effectType != WEAPON_SOUND_EFFECT_READY
        && effectType != WEAPON_SOUND_EFFECT_OUT_OF_AMMO) {
        if (hitMode != HIT_MODE_LEFT_WEAPON_PRIMARY
            && hitMode != HIT_MODE_RIGHT_WEAPON_PRIMARY
            && hitMode != HIT_MODE_PUNCH) {
            v6 = 2;
        } else {
            v6 = 1;
        }
    } else {
        v6 = 1;
    }

    if (effectTypeCode != 'H' || target == NULL || item_w_is_grenade(weapon)) {
        materialCode = 'X';
    } else {
        const int type = FID_TYPE(target->fid);
        int material;
        switch (type) {
        case OBJ_TYPE_ITEM:
            proto_ptr(target->pid, &proto);
            material = proto->item.material;
            break;
        case OBJ_TYPE_SCENERY:
            proto_ptr(target->pid, &proto);
            material = proto->scenery.field_2C;
            break;
        case OBJ_TYPE_WALL:
            proto_ptr(target->pid, &proto);
            material = proto->wall.material;
            break;
        default:
            material = -1;
            break;
        }

        switch (material) {
        case MATERIAL_TYPE_GLASS:
        case MATERIAL_TYPE_METAL:
        case MATERIAL_TYPE_PLASTIC:
            materialCode = 'M';
            break;
        case MATERIAL_TYPE_WOOD:
            materialCode = 'W';
            break;
        case MATERIAL_TYPE_DIRT:
        case MATERIAL_TYPE_STONE:
        case MATERIAL_TYPE_CEMENT:
            materialCode = 'S';
            break;
        default:
            materialCode = 'F';
            break;
        }
    }

    sprintf(sfx_file_name, "W%c%c%1d%cXX%1d", effectTypeCode, weaponSoundCode, v6, materialCode, 1);
    strupr(sfx_file_name);
    return sfx_file_name;
}

// sfx_build_scenery_name
// 0x451898
char* gsnd_build_scenery_sfx_name(int actionType, int action, const char* name)
{
    char actionTypeCode = actionType == SOUND_EFFECT_ACTION_TYPE_PASSIVE ? 'P' : 'A';
    char actionCode = snd_lookup_scenery_action[action];

    sprintf(sfx_file_name, "S%c%c%4s%1d", actionTypeCode, actionCode, name, 1);
    strupr(sfx_file_name);

    return sfx_file_name;
}

// sfx_build_open_name
// 0x4518D
char* gsnd_build_open_sfx_name(Object* object, int action)
{
    if (FID_TYPE(object->fid) == OBJ_TYPE_SCENERY) {
        char scenerySoundId;
        Proto* proto;
        if (proto_ptr(object->pid, &proto) != -1) {
            scenerySoundId = proto->scenery.field_34;
        } else {
            scenerySoundId = 'A';
        }
        sprintf(sfx_file_name, "S%cDOORS%c", snd_lookup_scenery_action[action], scenerySoundId);
    } else {
        Proto* proto;
        proto_ptr(object->pid, &proto);
        sprintf(sfx_file_name, "I%cCNTNR%c", snd_lookup_scenery_action[action], proto->item.field_80);
    }
    strupr(sfx_file_name);
    return sfx_file_name;
}

// 0x451970
void gsound_red_butt_press(int btn, int keyCode)
{
    gsound_play_sfx_file("ib1p1xx1");
}

// 0x451978
void gsound_red_butt_release(int btn, int keyCode)
{
    gsound_play_sfx_file("ib1lu1x1");
}

// 0x451980
void gsound_toggle_butt_press(int btn, int keyCode)
{
    gsound_play_sfx_file("toggle");
}

// NOTE: Uncollapsed from 0x451980.
//
// 0x451980
void gsound_toggle_butt_release(int btn, int keyCode)
{
    gsound_play_sfx_file("toggle");
}

// 0x451988
void gsound_med_butt_press(int btn, int keyCode)
{
    gsound_play_sfx_file("ib2p1xx1");
}

// 0x451990
void gsound_med_butt_release(int btn, int keyCode)
{
    gsound_play_sfx_file("ib2lu1x1");
}

// 0x451998
void gsound_lrg_butt_press(int btn, int keyCode)
{
    gsound_play_sfx_file("ib3p1xx1");
}

// 0x4519A0
void gsound_lrg_butt_release(int btn, int keyCode)
{
    gsound_play_sfx_file("ib3lu1x1");
}

// 0x4519A8
int gsound_play_sfx_file(const char* name)
{
    if (!gsound_initialized) {
        return -1;
    }

    if (!gsound_sfx_enabled) {
        return -1;
    }

    Sound* sound = gsound_load_sound(name, NULL);
    if (sound == NULL) {
        return -1;
    }

    soundPlay(sound);

    return 0;
}

// 0x451A00
static void gsound_bkg_proc()
{
    soundContinueAll();
}

// 0x451A08
static int gsound_open(const char* fname, int flags, ...)
{
    if ((flags & 2) != 0) {
        return -1;
    }

    File* stream = db_fopen(fname, "rb");
    if (stream == NULL) {
        return -1;
    }

    return (int)stream;
}

// 0x451A1C
static long gsound_compressed_tell(int fileHandle)
{
    return -1;
}

// NOTE: Uncollapsed 0x451A1C.
static int gsound_write(int fileHandle, const void* buf, unsigned int size)
{
    return -1;
}

// 0x451A24
static int gsound_close(int fileHandle)
{
    if (fileHandle == -1) {
        return -1;
    }

    return db_fclose((File*)fileHandle);
}

// 0x451A30
static int gsound_read(int fileHandle, void* buffer, unsigned int size)
{
    if (fileHandle == -1) {
        return -1;
    }

    return db_fread(buffer, 1, size, (File*)fileHandle);
}

// 0x451A4C
static long gsound_seek(int fileHandle, long offset, int origin)
{
    if (fileHandle == -1) {
        return -1;
    }

    if (db_fseek((File*)fileHandle, offset, origin) != 0) {
        return -1;
    }

    return db_ftell((File*)fileHandle);
}

// 0x451A70
static long gsound_tell(int handle)
{
    if (handle == -1) {
        return -1;
    }

    return db_ftell((File*)handle);
}

// 0x451A7C
static long gsound_filesize(int handle)
{
    if (handle == -1) {
        return -1;
    }

    return db_filelength((File*)handle);
}

// 0x451A88
static bool gsound_compressed_query(char* filePath)
{
    return true;
}

// 0x451A90
static void gsound_internal_speech_callback(void* userData, int a2)
{
    if (a2 == 1) {
        gsound_speech_tag = NULL;

        if (gsound_speech_callback_fp) {
            gsound_speech_callback_fp();
        }
    }
}

// 0x451AB0
static void gsound_internal_background_callback(void* userData, int a2)
{
    if (a2 == 1) {
        gsound_background_tag = NULL;

        if (gsound_background_callback_fp) {
            gsound_background_callback_fp();
        }
    }
}

// 0x451AD0
static void gsound_internal_effect_callback(void* userData, int a2)
{
    if (a2 == 1) {
        --gsound_active_effect_counter;
    }
}

// 0x451ADC
static int gsound_background_allocate(Sound** soundPtr, int a2, int a3)
{
    int v5 = 10;
    int v6 = 0;
    if (a2 == 13) {
        v6 |= 0x01;
    } else if (a2 == 14) {
        v6 |= 0x02;
    }

    if (a3 == 15) {
        v6 |= 0x04;
    } else if (a3 == 16) {
        v5 = 42;
    }

    Sound* sound = soundAllocate(v6, v5);
    if (sound == NULL) {
        return -1;
    }

    *soundPtr = sound;

    return 0;
}

// gsound_background_find_with_copy
// 0x451B30
static int gsound_background_find_with_copy(char* dest, const char* src)
{
    size_t len = strlen(src) + strlen(".ACM");
    if (strlen(sound_music_path1) + len > MAX_PATH || strlen(sound_music_path2) + len > MAX_PATH) {
        if (gsound_debug) {
            debug_printf("Full background path too long.\n");
        }

        return -1;
    }

    if (gsound_debug) {
        debug_printf(" finding background sound ");
    }

    char outPath[MAX_PATH];
    sprintf(outPath, "%s%s%s", sound_music_path1, src, ".ACM");
    if (gsound_file_exists_f(outPath)) {
        strncpy(dest, outPath, MAX_PATH);
        dest[MAX_PATH] = '\0';
        return 0;
    }

    if (gsound_debug) {
        debug_printf("by copy ");
    }

    gsound_background_remove_last_copy();

    char inPath[MAX_PATH];
    sprintf(inPath, "%s%s%s", sound_music_path2, src, ".ACM");

    FILE* inStream = fopen(inPath, "rb");
    if (inStream == NULL) {
        if (gsound_debug) {
            debug_printf("Unable to find music file %s to copy down.\n", src);
        }

        return -1;
    }

    FILE* outStream = fopen(outPath, "wb");
    if (outStream == NULL) {
        if (gsound_debug) {
            debug_printf("Unable to open music file %s for copying to.", src);
        }

        fclose(inStream);

        return -1;
    }

    void* buffer = mem_malloc(0x2000);
    if (buffer == NULL) {
        if (gsound_debug) {
            debug_printf("Out of memory in gsound_background_find_with_copy.\n", src);
        }

        fclose(outStream);
        fclose(inStream);

        return -1;
    }

    bool err = false;
    while (!feof(inStream)) {
        size_t bytesRead = fread(buffer, 1, 0x2000, inStream);
        if (bytesRead == 0) {
            break;
        }

        if (fwrite(buffer, 1, bytesRead, outStream) != bytesRead) {
            err = true;
            break;
        }
    }

    mem_free(buffer);
    fclose(outStream);
    fclose(inStream);

    if (err) {
        if (gsound_debug) {
            debug_printf("Background sound file copy failed on write -- ");
            debug_printf("likely out of disc space.\n");
        }

        return -1;
    }

    strcpy(background_fname_copied, src);

    strncpy(dest, outPath, MAX_PATH);
    dest[MAX_PATH] = '\0';

    return 0;
}

// 0x451E2C
static int gsound_background_find_dont_copy(char* dest, const char* src)
{
    char path[MAX_PATH];
    int len;

    len = strlen(src) + strlen(".ACM");
    if (strlen(sound_music_path1) + len > MAX_PATH || strlen(sound_music_path2) + len > MAX_PATH) {
        if (gsound_debug) {
            debug_printf("Full background path too long.\n");
        }

        return -1;
    }

    if (gsound_debug) {
        debug_printf(" finding background sound ");
    }

    sprintf(path, "%s%s%s", sound_music_path1, src, ".ACM");
    if (gsound_file_exists_f(path)) {
        strncpy(dest, path, MAX_PATH);
        dest[MAX_PATH] = '\0';
        return 0;
    }

    if (gsound_debug) {
        debug_printf("in 2nd path ");
    }

    sprintf(path, "%s%s%s", sound_music_path2, src, ".ACM");
    if (gsound_file_exists_f(path)) {
        strncpy(dest, path, MAX_PATH);
        dest[MAX_PATH] = '\0';
        return 0;
    }

    if (gsound_debug) {
        debug_printf("-- find failed ");
    }

    return -1;
}

// 0x451F94
static int gsound_speech_find_dont_copy(char* dest, const char* src)
{
    char path[MAX_PATH];

    if (strlen(sound_speech_path) + strlen(".acm") > MAX_PATH) {
        if (gsound_debug) {
            // FIXME: The message is wrong (notes background path, but here
            // we're dealing with speech path).
            debug_printf("Full background path too long.\n");
        }

        return -1;
    }

    if (gsound_debug) {
        debug_printf(" finding speech sound ");
    }

    sprintf(path, "%s%s%s", sound_speech_path, src, ".ACM");

    // NOTE: Uninline.
    if (gsound_file_exists_db(path)) {
        if (gsound_debug) {
            debug_printf("-- find failed ");
        }

        return -1;
    }

    strncpy(dest, path, MAX_PATH);
    dest[MAX_PATH] = '\0';

    return 0;
}

// delete old music file
// 0x452088
static void gsound_background_remove_last_copy()
{
    if (background_fname_copied[0] != '\0') {
        char path[MAX_PATH];
        sprintf(path, "%s%s%s", "sound\\music\\", background_fname_copied, ".ACM");
        if (remove(path)) {
            if (gsound_debug) {
                debug_printf("Deleting old music file failed.\n");
            }
        }

        background_fname_copied[0] = '\0';
    }
}

// 0x4520EC
static int gsound_background_start()
{
    int result;

    if (gsound_debug) {
        debug_printf(" playing ");
    }

    if (gsound_background_fade) {
        soundVolume(gsound_background_tag, 1);
        result = soundFade(gsound_background_tag, 2000, (int)(background_volume * 0.94));
    } else {
        soundVolume(gsound_background_tag, (int)(background_volume * 0.94));
        result = soundPlay(gsound_background_tag);
    }

    if (result != 0) {
        if (gsound_debug) {
            debug_printf("Unable to play background sound.\n");
        }

        result = -1;
    }

    return result;
}

// 0x45219C
static int gsound_speech_start()
{
    if (gsound_debug) {
        debug_printf(" playing ");
    }

    soundVolume(gsound_speech_tag, (int)(speech_volume * 0.69));

    if (soundPlay(gsound_speech_tag) != 0) {
        if (gsound_debug) {
            debug_printf("Unable to play speech sound.\n");
        }

        return -1;
    }

    return 0;
}

// 0x452208
static int gsound_get_music_path(char** out_value, const char* key)
{
    int len;
    char* copy;
    char* value;

    config_get_string(&game_config, GAME_CONFIG_SOUND_KEY, key, out_value);

    value = *out_value;
    len = strlen(value);

    if (value[len - 1] == '\\' || value[len - 1] == '/') {
        return 0;
    }

    copy = (char*)mem_malloc(len + 2);
    if (copy == NULL) {
        if (gsound_debug) {
            debug_printf("Out of memory in gsound_get_music_path.\n");
        }
        return -1;
    }

    strcpy(copy, value);
    copy[len] = '\\';
    copy[len + 1] = '\0';

    if (config_set_string(&game_config, GAME_CONFIG_SOUND_KEY, key, copy) != 1) {
        if (gsound_debug) {
            debug_printf("config_set_string failed in gsound_music_path.\n");
        }

        return -1;
    }

    if (config_get_string(&game_config, GAME_CONFIG_SOUND_KEY, key, out_value)) {
        mem_free(copy);
        return 0;
    }

    if (gsound_debug) {
        debug_printf("config_get_string failed in gsound_music_path.\n");
    }

    return -1;
}

// 0x452378
static Sound* gsound_get_sound_ready_for_effect()
{
    int rc;

    Sound* sound = soundAllocate(5, 10);
    if (sound == NULL) {
        if (gsound_debug) {
            debug_printf(" Can't allocate sound for effect. ");
        }

        if (gsound_debug) {
            debug_printf("soundAllocate returned: %d, %s\n", 0, soundError(0));
        }

        return NULL;
    }

    if (sfxc_is_initialized()) {
        rc = soundSetFileIO(sound, sfxc_cached_open, sfxc_cached_close, sfxc_cached_read, sfxc_cached_write, sfxc_cached_seek, sfxc_cached_tell, sfxc_cached_file_size);
    } else {
        rc = soundSetFileIO(sound, audioOpen, audioCloseFile, audioRead, NULL, audioSeek, gsound_compressed_tell, audioFileSize);
    }

    if (rc != 0) {
        if (gsound_debug) {
            debug_printf("Can't set file IO on sound effect.\n");
        }

        if (gsound_debug) {
            debug_printf("soundSetFileIO returned: %d, %s\n", rc, soundError(rc));
        }

        soundDelete(sound);

        return NULL;
    }

    rc = soundSetCallback(sound, gsound_internal_effect_callback, NULL);
    if (rc != 0) {
        if (gsound_debug) {
            debug_printf("failed because the callback could not be set.\n");
        }

        if (gsound_debug) {
            debug_printf("soundSetCallback returned: %d, %s\n", rc, soundError(rc));
        }

        soundDelete(sound);

        return NULL;
    }

    soundVolume(sound, sndfx_volume);

    return sound;
}

// Check file for existence.
//
// 0x4524E0
static bool gsound_file_exists_f(const char* fname)
{
    FILE* f = fopen(fname, "rb");
    if (f == NULL) {
        return false;
    }

    fclose(f);

    return true;
}

// 0x4524FC
static int gsound_file_exists_db(const char* path)
{
    int size;

    return db_dir_entry(path, &size) == 0;
}

// 0x452518
static int gsound_setup_paths()
{
    // TODO: Incomplete.

    return 0;
}

// 0x452628
int gsound_sfx_q_start()
{
    return gsound_sfx_q_process(0, NULL);
}

// 0x452634
int gsound_sfx_q_process(Object* a1, void* data)
{
    // 0x518E98
    static int lastTime = 0;

    queue_clear_type(EVENT_TYPE_GSOUND_SFX_EVENT, NULL);

    AmbientSoundEffectEvent* soundEffectEvent = (AmbientSoundEffectEvent*)data;
    int ambientSoundEffectIndex = -1;
    if (soundEffectEvent != NULL) {
        ambientSoundEffectIndex = soundEffectEvent->ambientSoundEffectIndex;
    } else {
        if (wmSfxMaxCount() > 0) {
            ambientSoundEffectIndex = wmSfxRollNextIdx();
        }
    }

    AmbientSoundEffectEvent* nextSoundEffectEvent = (AmbientSoundEffectEvent*)mem_malloc(sizeof(*nextSoundEffectEvent));
    if (nextSoundEffectEvent == NULL) {
        return -1;
    }

    if (map_data.name[0] == '\0') {
        return 0;
    }

    int delay = 10 * roll_random(15, 20);
    if (wmSfxMaxCount() > 0) {
        nextSoundEffectEvent->ambientSoundEffectIndex = wmSfxRollNextIdx();
        if (queue_add(delay, NULL, nextSoundEffectEvent, EVENT_TYPE_GSOUND_SFX_EVENT) == -1) {
            return -1;
        }
    }

    if (isInCombat()) {
        ambientSoundEffectIndex = -1;
    }

    if (ambientSoundEffectIndex != -1) {
        char* fileName;
        if (wmSfxIdxName(ambientSoundEffectIndex, &fileName) == 0) {
            int v7 = get_bk_time();
            if (elapsed_tocks(v7, lastTime) >= 5000) {
                if (gsound_play_sfx_file(fileName) == -1) {
                    debug_printf("\nGsound: playing ambient map sfx: %s.  FAILED", fileName);
                } else {
                    debug_printf("\nGsound: playing ambient map sfx: %s", fileName);
                }
            }
            lastTime = v7;
        }
    }

    return 0;
}
