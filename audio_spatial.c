/****************************************************************/
/* code copyright 1995-1996 Andrew Johnson - ALL RIGHTS RESERVED*/
/*                          ajohnson@eecs.uic.edu               */
/*                                                              */
/*                      Electronic Visualization Lab (M/C 154)  */
/*                      University of Illinois at Chicago       */
/*                      851 South Morgan St. Room 1120 SEO      */
/*                      Chicago, IL 60607-7053                  */
/*                                                              */
/*                      (312) 996-3002     (312) 413-7585 FAX   */
/***********************************************************************/
/* Cross-platform audio policy shared by the PCM output backends.       */
/***********************************************************************/

#include "battalion.h"
#include "audio_backend.h"
#include "audio_catalog.h"
#include "audio_mixer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int noSound;
extern struct monsterInfo Googelon;

int soundOn;
int musicOn;
int playingTankFire, playingHeloRocket, playingTankMaser;
int playingExplosion, playingSlag, playingTech;
int musicCount, deadCount;

static struct AudioCatalog spatialCatalog;
static int musicSlots[MAXMUSIC];
static int musicSlotCount;
static int deathMusicSlots[MAXDEADMUSIC];
static int deathMusicSlotCount;
static int backendStarted;

static void stopEffects(void)
    {
    audioMixerStopType(TANKFIRE);
    audioMixerStopType(HELOROCKET);
    audioMixerStopType(TANKMASER);
    audioMixerStopType(EXPLOSION);
    audioMixerStopType(MONSTERBEAM);
    audioMixerStopType(SLAG);
    audioMixerStopType(CRASH);
    audioMixerStopType(TECHSHOOT);
    }

static const struct AudioCatalogEntry *requiredEntry(const char *key,
                                                      int expectedSlot)
    {
    const struct AudioCatalogEntry *entry;

    entry = audioCatalogFind(&spatialCatalog, key, "pcm");
    if (entry == NULL || entry->slot != expectedSlot)
        {
        fprintf(stderr, "Required PCM audio asset is missing: %s\n", key);
        showError("Required PCM audio asset is missing from assets");
        exit(1);
        }
    return entry;
    }

static void loadEntry(const char *dataPath,
                      const struct AudioCatalogEntry *entry)
    {
    char path[MAXPATH];
    char error[160];

    if (entry->slot < 0 || entry->slot >= AUDIO_MIXER_MAX_SAMPLES)
        {
        fprintf(stderr, "PCM audio slot is out of range: %s\n", entry->key);
        showError("PCM audio slot is out of range");
        exit(1);
        }
    error[0] = '\0';
    if (!audioCatalogBuildPath(dataPath, entry, path, sizeof(path)) ||
        !audioMixerLoadWav(entry->slot, path, error, sizeof(error)))
        {
        fprintf(stderr, "Could not load audio asset %s: %s\n",
                entry->path, error);
        showError("Cannot load PCM audio asset");
        exit(1);
        }
    }

static int collectSlots(const char *prefix, int *slots, int maximum)
    {
    const struct AudioCatalogEntry *entry;
    size_t prefixLength;
    size_t index;
    int count;

    prefixLength = strlen(prefix);
    count = 0;
    for (index = 0; index < spatialCatalog.count; index++)
        {
        entry = &spatialCatalog.entries[index];
        if (strcmp(entry->backend, "pcm") == 0 &&
            strncmp(entry->key, prefix, prefixLength) == 0)
            {
            if (count >= maximum)
                return -1;
            slots[count++] = entry->slot;
            }
        }
    return count;
    }

void killSounds(void)
    {
    if (backendStarted)
        audioBackendStop();
    backendStarted = 0;
    audioMixerShutdown();
    audioCatalogDestroy(&spatialCatalog);
    }

void turnSoundOff(void)
    {
    soundOn = 0;
    stopEffects();
    }

void turnSoundOn(void)
    {
    soundOn = 1;
    }

void toggleSound(void)
    {
    if (soundOn)
        turnSoundOff();
    else
        turnSoundOn();
    }

int getSoundOn(void)
    {
    return soundOn;
    }

void turnMusicOff(void)
    {
    musicOn = 0;
    audioMixerStopType(MUSIC);
    }

void turnMusicOn(void)
    {
    musicOn = 1;
    }

void toggleMusic(void)
    {
    if (musicOn)
        turnMusicOff();
    else
        turnMusicOn();
    }

int getMusicOn(void)
    {
    return musicOn;
    }

void initSounds(void)
    {
    audioCatalogInitialize(&spatialCatalog);
    audioMixerInitialize();
    musicCount = 0;
    deadCount = 0;
    backendStarted = 0;
    playingTankFire = playingHeloRocket = playingTankMaser = 0;
    playingExplosion = playingSlag = playingTech = 0;
    }

void checkSound(char *dataPath)
    {
    const struct AudioCatalogEntry *entry;
    char catalogPath[MAXPATH];
    char catalogError[160];
    int loadedSlots[AUDIO_MIXER_MAX_SAMPLES];
    size_t index;

    if (strlen(dataPath) + strlen("audio/catalog.txt") + 1 >
        sizeof(catalogPath))
        {
        showError("Audio catalog path is too long");
        exit(1);
        }
    strcpy(catalogPath, dataPath);
    strcat(catalogPath, "audio/catalog.txt");
    if (!audioCatalogLoad(dataPath, catalogPath, &spatialCatalog,
                          catalogError, sizeof(catalogError)))
        {
        fprintf(stderr, "Could not load audio catalog: %s\n", catalogError);
        showError("Cannot load audio catalog");
        exit(1);
        }

    requiredEntry("beam", 0);
    requiredEntry("boom", 1);
    requiredEntry("crash", 2);
    requiredEntry("monster-beam", 3);
    requiredEntry("rocket", 4);
    requiredEntry("slag", 5);
    requiredEntry("tank", 6);
    requiredEntry("tech", 7);
    requiredEntry("music-1", 10);
    requiredEntry("music-2", 11);
    requiredEntry("music-3", 12);
    requiredEntry("music-4", 13);
    requiredEntry("music-5", 14);
    requiredEntry("death-1", 15);
    requiredEntry("death-2", 16);
    requiredEntry("death-3", 17);
    requiredEntry("death-4", 18);
    requiredEntry("death-5", 19);
    memset(loadedSlots, 0, sizeof(loadedSlots));
    for (index = 0; index < spatialCatalog.count; index++)
        {
        entry = &spatialCatalog.entries[index];
        if (strcmp(entry->backend, "pcm") == 0)
            {
            if (entry->slot < 0 ||
                entry->slot >= AUDIO_MIXER_MAX_SAMPLES ||
                loadedSlots[entry->slot])
                {
                showError("PCM audio catalog has duplicate or invalid slots");
                exit(1);
                }
            loadedSlots[entry->slot] = 1;
            loadEntry(dataPath, entry);
            }
        }
    musicSlotCount = collectSlots("music-", musicSlots, MAXMUSIC);
    deathMusicSlotCount = collectSlots("death-", deathMusicSlots,
                                       MAXDEADMUSIC);
    if (musicSlotCount <= 0 || deathMusicSlotCount <= 0)
        {
        showError("Audio catalog has an invalid music sequence");
        exit(1);
        }

    backendStarted = audioBackendStart();
    if (!backendStarted)
        {
        fprintf(stderr, "Audio output is unavailable; continuing silently\n");
        noSound = 1;
        soundOn = 0;
        musicOn = 0;
        }
    else
        {
        noSound = 0;
        atexit(killSounds);
        }
    }

void flushSounds(void)
    {
    }

void soundKiller(int soundType)
    {
    audioMixerStopType(soundType);
    }

void audioSetListener(float x, float y, float z, float headingRadians)
    {
    audioMixerSetListener(x, y, z, headingRadians);
    }

static int effectSlot(int soundType)
    {
    switch (soundType)
        {
        case TANKFIRE: return 6;
        case HELOROCKET: return 4;
        case TANKMASER: return 0;
        case EXPLOSION: return 1;
        case MONSTERBEAM: return 3;
        case SLAG: return 5;
        case CRASH: return 2;
        case TECHSHOOT: return 7;
        }
    return -1;
    }

void doSoundAt(int theSound, float x, float y, float z)
    {
    int slot;
    int unique;

    if (noSound)
        return;
    if (theSound == MUSIC)
        {
        if (!musicOn || audioMixerIsTypePlaying(MUSIC))
            return;
        if (Googelon.monsterIsDead)
            {
            slot = deathMusicSlots[deadCount];
            deadCount = (deadCount + 1) % deathMusicSlotCount;
            musicCount = 0;
            }
        else
            {
            slot = musicSlots[musicCount];
            musicCount = (musicCount + 1) % musicSlotCount;
            deadCount = 0;
            }
        audioMixerPlay(slot, MUSIC, 0, 1, 0, 0.0f, 0.0f, 0.0f, 0.55f);
        return;
        }
    if (!soundOn)
        return;
    slot = effectSlot(theSound);
    if (slot < 0)
        return;
    unique = (theSound == TANKMASER || theSound == MONSTERBEAM);
    audioMixerPlay(slot, theSound, 0, unique, 1, x, y, z, 0.9f);
    }

void doSound(int theSound)
    {
    if (theSound == MUSIC)
        doSoundAt(theSound, 0.0f, 0.0f, 0.0f);
    else
        doSoundAt(theSound, Googelon.x,
                  (Googelon.bottom + Googelon.height) * 0.5f,
                  Googelon.z);
    }

void OutAudio(int slot)
    {
    audioMixerPlay(slot, -1, 0, 0, 0, 0.0f, 0.0f, 0.0f, 1.0f);
    }

void InitAudio(const char *fileName, const char *dataPath, int slot)
    {
    char path[MAXPATH];
    char error[160];

    if (strlen(dataPath) + strlen(fileName) + 1 > sizeof(path))
        {
        showError("Audio asset path is too long");
        return;
        }
    strcpy(path, dataPath);
    strcat(path, fileName);
    if (!audioMixerLoadWav(slot, path, error, sizeof(error)))
        {
        fprintf(stderr, "Could not load audio asset %s: %s\n", path, error);
        showError("Cannot load PCM audio asset");
        }
    }
