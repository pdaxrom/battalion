/*
 * Copyright (c) 2026 Battalion contributors.
 */

#include "audio_backend.h"
#include "audio_mixer.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/audioio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define SUN_AUDIO_FRAMES 1024

static int sunFile = -1;
static int sunRunning;
static unsigned long sunRate;
static pthread_t sunThread;

static void *runSunAudio(void *unused)
    {
    short frames[SUN_AUDIO_FRAMES * 2];
    unsigned char *bytes;
    int remaining;
    int written;

    (void)unused;
    while (sunRunning)
        {
        audioMixerRender(frames, SUN_AUDIO_FRAMES, sunRate);
        bytes = (unsigned char *)frames;
        remaining = sizeof(frames);
        while (remaining > 0 && sunRunning)
            {
            written = (int)write(sunFile, bytes, (unsigned)remaining);
            if (written > 0)
                {
                bytes += written;
                remaining -= written;
                }
            else if (errno != EINTR)
                {
                sunRunning = 0;
                break;
                }
            }
        }
    return NULL;
    }

int audioBackendStart(void)
    {
    audio_info_t information;

    sunFile = open("/dev/audio", O_WRONLY);
    if (sunFile < 0)
        return 0;
    AUDIO_INITINFO(&information);
    information.play.sample_rate = BATTALION_AUDIO_RATE;
    information.play.channels = 2;
    information.play.precision = 16;
    information.play.encoding = AUDIO_ENCODING_LINEAR;
    if (ioctl(sunFile, AUDIO_SETINFO, &information) < 0)
        {
        close(sunFile);
        sunFile = -1;
        return 0;
        }
    if (information.play.sample_rate <= 0 ||
        information.play.channels != 2 ||
        information.play.precision != 16 ||
        information.play.encoding != AUDIO_ENCODING_LINEAR)
        {
        close(sunFile);
        sunFile = -1;
        return 0;
        }
    sunRate = (unsigned long)information.play.sample_rate;
    sunRunning = 1;
    if (pthread_create(&sunThread, NULL, runSunAudio, NULL) != 0)
        {
        sunRunning = 0;
        close(sunFile);
        sunFile = -1;
        return 0;
        }
    return 1;
    }

void audioBackendStop(void)
    {
    if (sunFile < 0)
        return;
    sunRunning = 0;
    pthread_join(sunThread, NULL);
    close(sunFile);
    sunFile = -1;
    }
