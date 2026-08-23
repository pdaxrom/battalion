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

#include "audio_backend.h"
#include "audio_mixer.h"

#include <dmedia/audio.h>
#include <pthread.h>
#include <unistd.h>

#define SGI_AUDIO_FRAMES 1024

static ALport sgiPort;
static int sgiRunning;
static unsigned long sgiRate;
static pthread_t sgiThread;

static void *runSgiAudio(void *unused)
    {
    short frames[SGI_AUDIO_FRAMES * 2];
    long fillable;
    long frameCount;

    (void)unused;
    while (sgiRunning)
        {
        fillable = ALgetfillable(sgiPort);
        frameCount = fillable / 2;
        if (frameCount > SGI_AUDIO_FRAMES)
            frameCount = SGI_AUDIO_FRAMES;
        if (frameCount > 0)
            {
            audioMixerRender(frames, (unsigned long)frameCount, sgiRate);
            ALwritesamps(sgiPort, frames, frameCount * 2);
            }
        else
            usleep(1000);
        }
    return NULL;
    }

int audioBackendStart(void)
    {
    ALconfig configuration;
    long parameters[2];

    ALseterrorhandler(0);
    parameters[0] = AL_OUTPUT_RATE;
    parameters[1] = BATTALION_AUDIO_RATE;
    ALsetparams(AL_DEFAULT_DEVICE, parameters, 2);
    ALgetparams(AL_DEFAULT_DEVICE, parameters, 2);
    if (parameters[1] <= 0)
        return 0;
    sgiRate = (unsigned long)parameters[1];
    configuration = ALnewconfig();
    if (configuration == 0)
        return 0;
    ALsetwidth(configuration, AL_SAMPLE_16);
    ALsetchannels(configuration, AL_STEREO);
    ALsetqueuesize(configuration, SGI_AUDIO_FRAMES * 8);
    sgiPort = ALopenport("battalion", "w", configuration);
    ALfreeconfig(configuration);
    if (sgiPort == 0)
        return 0;
    sgiRunning = 1;
    if (pthread_create(&sgiThread, NULL, runSgiAudio, NULL) != 0)
        {
        sgiRunning = 0;
        ALcloseport(sgiPort);
        sgiPort = 0;
        return 0;
        }
    return 1;
    }

void audioBackendStop(void)
    {
    if (sgiPort == 0)
        return;
    sgiRunning = 0;
    pthread_join(sgiThread, NULL);
    ALcloseport(sgiPort);
    sgiPort = 0;
    }
