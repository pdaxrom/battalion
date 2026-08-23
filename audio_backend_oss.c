/*
 * Copyright (c) 2026 Battalion contributors.
 */

#include "audio_backend.h"
#include "audio_mixer.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/soundcard.h>
#else
#include <machine/soundcard.h>
#endif

#ifndef AFMT_S16_NE
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define AFMT_S16_NE AFMT_S16_BE
#else
#define AFMT_S16_NE AFMT_S16_LE
#endif
#endif

#define OSS_AUDIO_FRAMES 1024

static int ossFile = -1;
static int ossRunning;
static unsigned long ossRate;
static pthread_t ossThread;

static void *runOssAudio(void *unused)
    {
    short frames[OSS_AUDIO_FRAMES * 2];
    unsigned char *bytes;
    int remaining;
    int written;

    (void)unused;
    while (ossRunning)
        {
        audioMixerRender(frames, OSS_AUDIO_FRAMES, ossRate);
        bytes = (unsigned char *)frames;
        remaining = sizeof(frames);
        while (remaining > 0 && ossRunning)
            {
            written = (int)write(ossFile, bytes, (unsigned)remaining);
            if (written > 0)
                {
                bytes += written;
                remaining -= written;
                }
            else if (errno != EINTR)
                {
                ossRunning = 0;
                break;
                }
            }
        }
    return NULL;
    }

int audioBackendStart(void)
    {
    int format;
    int channels;
    int rate;

    ossFile = open("/dev/dsp", O_WRONLY);
    if (ossFile < 0)
        return 0;
    format = AFMT_S16_NE;
    channels = 2;
    rate = BATTALION_AUDIO_RATE;
    if (ioctl(ossFile, SNDCTL_DSP_SETFMT, &format) < 0 ||
        ioctl(ossFile, SNDCTL_DSP_CHANNELS, &channels) < 0 ||
        ioctl(ossFile, SNDCTL_DSP_SPEED, &rate) < 0 ||
        format != AFMT_S16_NE || channels != 2 || rate <= 0)
        {
        close(ossFile);
        ossFile = -1;
        return 0;
        }
    ossRate = (unsigned long)rate;
    ossRunning = 1;
    if (pthread_create(&ossThread, NULL, runOssAudio, NULL) != 0)
        {
        ossRunning = 0;
        close(ossFile);
        ossFile = -1;
        return 0;
        }
    return 1;
    }

void audioBackendStop(void)
    {
    if (ossFile < 0)
        return;
    ossRunning = 0;
    pthread_join(ossThread, NULL);
    close(ossFile);
    ossFile = -1;
    }
