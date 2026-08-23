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
/* Silent backend selected by AUDIO=none.                              */
/***********************************************************************/

#include "battalion.h"

extern int noSound;

int soundOn;
int musicOn;
int playingTankFire, playingHeloRocket, playingTankMaser;
int playingExplosion, playingSlag, playingTech;
int musicCount, deadCount;

void turnSoundOff(void)
    {
    soundOn = 0;
    }

void turnSoundOn(void)
    {
    soundOn = 1;
    }

void toggleSound(void)
    {
    soundOn = !soundOn;
    }

int getSoundOn(void)
    {
    return soundOn;
    }

void turnMusicOff(void)
    {
    musicOn = 0;
    }

void turnMusicOn(void)
    {
    musicOn = 1;
    }

void toggleMusic(void)
    {
    musicOn = !musicOn;
    }

int getMusicOn(void)
    {
    return musicOn;
    }

void initSounds(void)
    {
    soundOn = 0;
    musicOn = 0;
    musicCount = 0;
    deadCount = 0;
    playingTankFire = playingHeloRocket = playingTankMaser = 0;
    playingExplosion = playingSlag = playingTech = 0;
    }

void checkSound(char *dataPath)
    {
    (void)dataPath;
    noSound = 1;
    soundOn = 0;
    musicOn = 0;
    }

void flushSounds(void)
    {
    }

void soundKiller(int soundType)
    {
    (void)soundType;
    }

void doSound(int soundType)
    {
    (void)soundType;
    }

void doSoundAt(int soundType, float x, float y, float z)
    {
    (void)soundType;
    (void)x;
    (void)y;
    (void)z;
    }

void audioSetListener(float x, float y, float z, float headingRadians)
    {
    (void)x;
    (void)y;
    (void)z;
    (void)headingRadians;
    }

void audioSetSmoothingEnabled(int enabled)
    {
    (void)enabled;
    }

int audioGetSmoothingEnabled(void)
    {
    return 0;
    }

void OutAudio(int slot)
    {
    (void)slot;
    }

void InitAudio(const char *fileName, const char *dataPath, int slot)
    {
    (void)fileName;
    (void)dataPath;
    (void)slot;
    }
