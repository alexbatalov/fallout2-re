#ifndef GAME_SOUND_H
#define GAME_SOUND_H

#include "obj_types.h"
#include "sound.h"

#include <stdbool.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef enum WeaponSoundEffect {
    WEAPON_SOUND_EFFECT_READY,
    WEAPON_SOUND_EFFECT_ATTACK,
    WEAPON_SOUND_EFFECT_OUT_OF_AMMO,
    WEAPON_SOUND_EFFECT_AMMO_FLYING,
    WEAPON_SOUND_EFFECT_HIT,
    WEAPON_SOUND_EFFECT_COUNT,
} WeaponSoundEffect;

typedef enum SoundEffectActionType {
    SOUND_EFFECT_ACTION_TYPE_ACTIVE,
    SOUND_EFFECT_ACTION_TYPE_PASSIVE,
} SoundEffectActionType;

typedef enum ScenerySoundEffect {
    SCENERY_SOUND_EFFECT_OPEN,
    SCENERY_SOUND_EFFECT_CLOSED,
    SCENERY_SOUND_EFFECT_LOCKED,
    SCENERY_SOUND_EFFECT_UNLOCKED,
    SCENERY_SOUND_EFFECT_USED,
    SCENERY_SOUND_EFFECT_COUNT,
} ScenerySoundEffect;

typedef enum CharacterSoundEffect {
    CHARACTER_SOUND_EFFECT_UNUSED,
    CHARACTER_SOUND_EFFECT_KNOCKDOWN,
    CHARACTER_SOUND_EFFECT_PASS_OUT,
    CHARACTER_SOUND_EFFECT_DIE,
    CHARACTER_SOUND_EFFECT_CONTACT,
} CharacterSoundEffect;

typedef void(SoundEndCallback)();

extern char off_5035BC[];
extern char off_5035C8[];
extern char off_5035D8[];

extern bool gGameSoundInitialized;
extern bool gGameSoundDebugEnabled;
extern bool gMusicEnabled;
extern int dword_518E3;
extern int dword_518E40;
extern bool gSpeechEnabled;
extern bool gSoundEffectsEnabled;
extern int dword_518E4C;
extern Sound* gBackgroundSound;
extern Sound* gSpeechSound;
extern SoundEndCallback* gBackgroundSoundEndCallback;
extern SoundEndCallback* gSpeechEndCallback;
extern char byte_518E60[WEAPON_SOUND_EFFECT_COUNT];
extern char byte_518E65[SCENERY_SOUND_EFFECT_COUNT];
extern int dword_518E6C;
extern int dword_518E70;
extern char* off_518E74;
extern char* off_518E78;
extern char* off_518E7C;
extern char* off_518E80;
extern int gMasterVolume;
extern int gMusicVolume;
extern int gSpeechVolume;
extern int gSoundEffectsVolume;
extern int dword_518E94;
extern int dword_518E98;

extern char byte_596EB0[MAX_PATH];
extern char byte_596FB5[13];
extern char gBackgroundSoundFileName[270];

int gameSoundInit();
void gameSoundReset();
int gameSoundExit();
void soundEffectsEnable();
void soundEffectsDisable();
int soundEffectsIsEnabled();
int gameSoundSetMasterVolume(int value);
int gameSoundGetMasterVolume();
int soundEffectsSetVolume(int value);
int soundEffectsGetVolume();
void backgroundSoundDisable();
void backgroundSoundEnable();
int backgroundSoundIsEnabled();
void backgroundSoundSetVolume(int value);
int backgroundSoundGetVolume();
int gsound_background_volume_get_set(int a1);
void backgroundSoundSetEndCallback(SoundEndCallback* callback);
int backgroundSoundGetDuration();
int backgroundSoundLoad(const char* fileName, int a2, int a3, int a4);
int gsound_background_play_level_music(const char* a1, int a2);
void backgroundSoundDelete();
void backgroundSoundRestart(int value);
void backgroundSoundPause();
void backgroundSoundResume();
void speechDisable();
void speechEnable();
int speechIsEnabled();
void speechSetVolume(int value);
int speechGetVolume();
int gsound_speech_volume_get_set(int volume);
void speechSetEndCallback(SoundEndCallback* callback);
int speechGetDuration();
int speechLoad(const char* fname, int a2, int a3, int a4);
int gsound_speech_play_preloaded();
void speechDelete();
void speechPause();
void speechResume();
int gsound_play_sfx_file_volume(const char* a1, int a2);
Sound* soundEffectLoad(const char* name, Object* a2);
Sound* soundEffectLoadWithVolume(const char* a1, Object* a2, int a3);
void soundEffectDelete(Sound* a1);
int gsnd_anim_sound(Sound* a1);
int soundEffectPlay(Sound* a1);
int gsound_compute_relative_volume(Object* obj);
char* sfxBuildCharName(Object* a1, int anim, int extra);
char* gameSoundBuildAmbientSoundEffectName(const char* a1);
char* gameSoundBuildInterfaceName(const char* a1);
char* sfxBuildWeaponName(int effectType, Object* weapon, int hitMode, Object* target);
char* sfxBuildSceneryName(int actionType, int action, const char* name);
char* sfxBuildOpenName(Object* a1, int a2);
void gsound_red_butt_press(int btn, int keyCode);
void gsound_red_butt_release(int btn, int keyCode);
void sub_451980(int btn, int keyCode);
void gsound_med_butt_press(int btn, int keyCode);
void gsound_med_butt_release(int btn, int keyCode);
void gsound_lrg_butt_press(int btn, int keyCode);
void gsound_lrg_butt_release(int btn, int keyCode);
int soundPlayFile(const char* name);
void gsound_bkg_proc();
int gameSoundFileOpen(const char* fname, int access, ...);
long sub_451A1C();
long gameSoundFileTellNotImplemented(int handle);
int gameSoundFileWrite(int handle, const void* buf, unsigned int size);
int gameSoundFileClose(int handle);
int gameSoundFileRead(int handle, void* buf, unsigned int size);
long gameSoundFileSeek(int handle, long offset, int origin);
long gameSoundFileTell(int handle);
long gameSoundFileGetSize(int handle);
bool gameSoundIsCompressed(char* filePath);
void speechCallback(void* userData, int a2);
void backgroundSoundCallback(void* userData, int a2);
void soundEffectCallback(void* userData, int a2);
int gsound_background_allocate(Sound** out_s, int a2, int a3);
int gameSoundFindBackgroundSoundPathWithCopy(char* dest, const char* src);
int gameSoundFindBackgroundSoundPath(char* dest, const char* src);
int gameSoundFindSpeechSoundPath(char* dest, const char* src);
void gameSoundDeleteOldMusicFile();
int backgroundSoundPlay();
int speechPlay();
int gsound_get_music_path(char** out_value, const char* key);
Sound* gsound_get_sound_ready_for_effect();
bool gsound_file_exists_f(const char* fname);
int gsound_setup_paths();
int gsound_sfx_q_start();
int ambientSoundEffectEventProcess(Object* a1, void* a2);

#endif /* GAME_SOUND_H */
