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

#ifndef BATTALION_AUDIO_MIXER_H
#define BATTALION_AUDIO_MIXER_H

#define AUDIO_MIXER_MAX_SAMPLES 25
#define AUDIO_MIXER_MAX_VOICES 32

void audioMixerInitialize(void);
void audioMixerShutdown(void);
void audioMixerSetSmoothingEnabled(int enabled);
int audioMixerGetSmoothingEnabled(void);
int audioMixerLoadWav(int slot, const char *path, char *error,
                      unsigned long errorSize);
void audioMixerSetListener(float x, float y, float z, float headingRadians);
void audioMixerComputeSpatialGains(float listenerX, float listenerY,
                                   float listenerZ, float headingRadians,
                                   float sourceX, float sourceY,
                                   float sourceZ, float gain,
                                   float *left, float *right);
int audioMixerPlay(int slot, int soundType, int loop, int unique,
                   int positioned, float x, float y, float z, float gain);
void audioMixerStopType(int soundType);
int audioMixerIsTypePlaying(int soundType);
void audioMixerRender(short *stereoFrames, unsigned long frameCount,
                      unsigned long outputRate);

#endif
