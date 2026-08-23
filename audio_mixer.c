/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Renderer-neutral, C89 stereo sample mixer and spatializer.           */
/***********************************************************************/

#include "audio_mixer.h"

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AUDIO_REFERENCE_DISTANCE 2.0f
#define AUDIO_MAX_DISTANCE 48.0f
#define AUDIO_SMOOTHING_MILLISECONDS 30UL

#ifndef AUDIO_SMOOTHING_DEFAULT
#define AUDIO_SMOOTHING_DEFAULT 1
#endif

struct AudioMixerSample
    {
    short *frames;
    unsigned long frameCount;
    unsigned long sampleRate;
    int channels;
    };

struct AudioMixerVoice
    {
    int active;
    int slot;
    int soundType;
    int loop;
    int positioned;
    float x;
    float y;
    float z;
    float gain;
    float leftGain;
    float rightGain;
    float targetLeftGain;
    float targetRightGain;
    float leftGainStep;
    float rightGainStep;
    unsigned long gainRampFrames;
    double position;
    };

static struct AudioMixerSample mixerSamples[AUDIO_MIXER_MAX_SAMPLES];
static struct AudioMixerVoice mixerVoices[AUDIO_MIXER_MAX_VOICES];
static pthread_mutex_t mixerMutex = PTHREAD_MUTEX_INITIALIZER;
static float listenerX;
static float listenerY;
static float listenerZ;
static float listenerHeading;
static unsigned long mixerOutputRate = 44100UL;
static int smoothingEnabled;

static unsigned short readLittle16(const unsigned char *bytes)
    {
    return (unsigned short)((unsigned short)bytes[0] |
                            ((unsigned short)bytes[1] << 8));
    }

static unsigned long readLittle32(const unsigned char *bytes)
    {
    return (unsigned long)bytes[0] |
           ((unsigned long)bytes[1] << 8) |
           ((unsigned long)bytes[2] << 16) |
           ((unsigned long)bytes[3] << 24);
    }

static void setMixerError(char *error, unsigned long errorSize,
                          const char *message)
    {
    if (error == NULL || errorSize == 0)
        return;
    strncpy(error, message, errorSize - 1);
    error[errorSize - 1] = '\0';
    }

void audioMixerComputeSpatialGains(float lx, float ly, float lz,
                                   float headingRadians,
                                   float sx, float sy, float sz,
                                   float gain, float *left, float *right)
    {
    float dx;
    float dy;
    float dz;
    float horizontal;
    float distance;
    float attenuation;
    float pan;
    float rightX;
    float rightZ;

    dx = sx - lx;
    dy = sy - ly;
    dz = sz - lz;
    horizontal = (float)sqrt((double)(dx * dx + dz * dz));
    distance = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
    if (distance <= AUDIO_REFERENCE_DISTANCE)
        attenuation = 1.0f;
    else if (distance >= AUDIO_MAX_DISTANCE)
        attenuation = 0.0f;
    else
        attenuation = (AUDIO_MAX_DISTANCE - distance) /
                      (AUDIO_MAX_DISTANCE - AUDIO_REFERENCE_DISTANCE);

    if (horizontal > 0.0001f)
        {
        rightX = (float)cos((double)headingRadians);
        rightZ = (float)-sin((double)headingRadians);
        pan = (dx * rightX + dz * rightZ) / horizontal;
        if (pan < -1.0f)
            pan = -1.0f;
        if (pan > 1.0f)
            pan = 1.0f;
        }
    else
        pan = 0.0f;

    if (left != NULL)
        *left = gain * attenuation *
                (float)sqrt((double)(0.5f * (1.0f - pan)));
    if (right != NULL)
        *right = gain * attenuation *
                 (float)sqrt((double)(0.5f * (1.0f + pan)));
    }

static void updateVoiceTarget(struct AudioMixerVoice *voice, int transition)
    {
    if (voice->positioned)
        audioMixerComputeSpatialGains(listenerX, listenerY, listenerZ,
                                      listenerHeading,
                                      voice->x, voice->y, voice->z,
                                      voice->gain,
                                      &voice->targetLeftGain,
                                      &voice->targetRightGain);
    else
        {
        voice->targetLeftGain = voice->gain;
        voice->targetRightGain = voice->gain;
        }

    if (transition && smoothingEnabled)
        {
        voice->gainRampFrames =
            (mixerOutputRate * AUDIO_SMOOTHING_MILLISECONDS) / 1000UL;
        if (voice->gainRampFrames == 0)
            voice->gainRampFrames = 1;
        voice->leftGainStep =
            (voice->targetLeftGain - voice->leftGain) /
            (float)voice->gainRampFrames;
        voice->rightGainStep =
            (voice->targetRightGain - voice->rightGain) /
            (float)voice->gainRampFrames;
        }
    else
        {
        voice->leftGain = voice->targetLeftGain;
        voice->rightGain = voice->targetRightGain;
        voice->leftGainStep = 0.0f;
        voice->rightGainStep = 0.0f;
        voice->gainRampFrames = 0;
        }
    }

void audioMixerInitialize(void)
    {
    pthread_mutex_lock(&mixerMutex);
    memset(mixerSamples, 0, sizeof(mixerSamples));
    memset(mixerVoices, 0, sizeof(mixerVoices));
    listenerX = listenerY = listenerZ = listenerHeading = 0.0f;
    mixerOutputRate = 44100UL;
    smoothingEnabled = AUDIO_SMOOTHING_DEFAULT ? 1 : 0;
    pthread_mutex_unlock(&mixerMutex);
    }

void audioMixerSetSmoothingEnabled(int enabled)
    {
    int index;

    pthread_mutex_lock(&mixerMutex);
    smoothingEnabled = enabled ? 1 : 0;
    if (!smoothingEnabled)
        for (index = 0; index < AUDIO_MIXER_MAX_VOICES; index++)
            if (mixerVoices[index].active)
                updateVoiceTarget(&mixerVoices[index], 0);
    pthread_mutex_unlock(&mixerMutex);
    }

int audioMixerGetSmoothingEnabled(void)
    {
    int enabled;

    pthread_mutex_lock(&mixerMutex);
    enabled = smoothingEnabled;
    pthread_mutex_unlock(&mixerMutex);
    return enabled;
    }

void audioMixerShutdown(void)
    {
    int index;

    pthread_mutex_lock(&mixerMutex);
    for (index = 0; index < AUDIO_MIXER_MAX_SAMPLES; index++)
        {
        free(mixerSamples[index].frames);
        memset(&mixerSamples[index], 0, sizeof(mixerSamples[index]));
        }
    memset(mixerVoices, 0, sizeof(mixerVoices));
    pthread_mutex_unlock(&mixerMutex);
    }

int audioMixerLoadWav(int slot, const char *path, char *error,
                      unsigned long errorSize)
    {
    FILE *file;
    unsigned char header[12];
    unsigned char chunkHeader[8];
    unsigned char format[16];
    unsigned char *data;
    unsigned long chunkSize;
    unsigned long dataSize;
    unsigned long sampleRate;
    unsigned long sampleCount;
    unsigned long index;
    unsigned short audioFormat;
    unsigned short channels;
    unsigned short bits;
    int haveFormat;
    short *frames;

    if (slot < 0 || slot >= AUDIO_MIXER_MAX_SAMPLES || path == NULL)
        return 0;
    file = fopen(path, "rb");
    if (file == NULL)
        {
        setMixerError(error, errorSize, "Could not open WAV file");
        return 0;
        }
    if (fread(header, 1, sizeof(header), file) != sizeof(header) ||
        memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0)
        {
        setMixerError(error, errorSize, "Invalid WAV header");
        fclose(file);
        return 0;
        }

    haveFormat = 0;
    data = NULL;
    dataSize = 0;
    sampleRate = 0;
    channels = 0;
    bits = 0;
    audioFormat = 0;
    while (fread(chunkHeader, 1, sizeof(chunkHeader), file) ==
           sizeof(chunkHeader))
        {
        chunkSize = readLittle32(chunkHeader + 4);
        if (memcmp(chunkHeader, "fmt ", 4) == 0)
            {
            if (chunkSize < 16 || fread(format, 1, 16, file) != 16)
                break;
            audioFormat = readLittle16(format);
            channels = readLittle16(format + 2);
            sampleRate = readLittle32(format + 4);
            bits = readLittle16(format + 14);
            haveFormat = 1;
            if (chunkSize > 16 && fseek(file, (long)(chunkSize - 16), SEEK_CUR))
                break;
            }
        else if (memcmp(chunkHeader, "data", 4) == 0)
            {
            data = (unsigned char *)malloc(chunkSize);
            if (data == NULL || fread(data, 1, chunkSize, file) != chunkSize)
                {
                free(data);
                data = NULL;
                break;
                }
            dataSize = chunkSize;
            }
        else if (fseek(file, (long)chunkSize, SEEK_CUR))
            break;
        if ((chunkSize & 1) != 0)
            fseek(file, 1L, SEEK_CUR);
        if (haveFormat && data != NULL)
            break;
        }
    fclose(file);

    if (!haveFormat || data == NULL || audioFormat != 1 ||
        (channels != 1 && channels != 2) || bits != 16 || sampleRate == 0 ||
        dataSize < (unsigned long)channels * 2)
        {
        free(data);
        setMixerError(error, errorSize,
                      "WAV must be 16-bit mono or stereo PCM");
        return 0;
        }

    sampleCount = dataSize / 2;
    frames = (short *)malloc(sampleCount * sizeof(short));
    if (frames == NULL)
        {
        free(data);
        setMixerError(error, errorSize, "Out of memory loading WAV");
        return 0;
        }
    for (index = 0; index < sampleCount; index++)
        frames[index] = (short)readLittle16(data + index * 2);
    free(data);

    pthread_mutex_lock(&mixerMutex);
    free(mixerSamples[slot].frames);
    mixerSamples[slot].frames = frames;
    mixerSamples[slot].frameCount = sampleCount / channels;
    mixerSamples[slot].sampleRate = sampleRate;
    mixerSamples[slot].channels = channels;
    pthread_mutex_unlock(&mixerMutex);
    return 1;
    }

void audioMixerSetListener(float x, float y, float z, float headingRadians)
    {
    int index;

    pthread_mutex_lock(&mixerMutex);
    listenerX = x;
    listenerY = y;
    listenerZ = z;
    listenerHeading = headingRadians;
    for (index = 0; index < AUDIO_MIXER_MAX_VOICES; index++)
        if (mixerVoices[index].active)
            updateVoiceTarget(&mixerVoices[index], 1);
    pthread_mutex_unlock(&mixerMutex);
    }

int audioMixerPlay(int slot, int soundType, int loop, int unique,
                   int positioned, float x, float y, float z, float gain)
    {
    int index;
    int selected;
    int continuing;
    struct AudioMixerVoice *voice;

    if (slot < 0 || slot >= AUDIO_MIXER_MAX_SAMPLES)
        return 0;
    pthread_mutex_lock(&mixerMutex);
    if (mixerSamples[slot].frames == NULL)
        {
        pthread_mutex_unlock(&mixerMutex);
        return 0;
        }
    selected = -1;
    for (index = 0; index < AUDIO_MIXER_MAX_VOICES; index++)
        {
        if (unique && mixerVoices[index].active &&
            mixerVoices[index].soundType == soundType)
            {
            selected = index;
            break;
            }
        if (selected < 0 && !mixerVoices[index].active)
            selected = index;
        }
    if (selected < 0)
        selected = 0;
    voice = &mixerVoices[selected];
    continuing = unique && voice->active && voice->soundType == soundType;
    if (!continuing)
        voice->position = 0.0;
    voice->active = 1;
    voice->slot = slot;
    voice->soundType = soundType;
    voice->loop = loop;
    voice->positioned = positioned;
    voice->x = x;
    voice->y = y;
    voice->z = z;
    voice->gain = gain;
    updateVoiceTarget(voice, continuing);
    pthread_mutex_unlock(&mixerMutex);
    return 1;
    }

void audioMixerStopType(int soundType)
    {
    int index;

    pthread_mutex_lock(&mixerMutex);
    for (index = 0; index < AUDIO_MIXER_MAX_VOICES; index++)
        if (mixerVoices[index].active &&
            mixerVoices[index].soundType == soundType)
            mixerVoices[index].active = 0;
    pthread_mutex_unlock(&mixerMutex);
    }

int audioMixerIsTypePlaying(int soundType)
    {
    int index;
    int active;

    active = 0;
    pthread_mutex_lock(&mixerMutex);
    for (index = 0; index < AUDIO_MIXER_MAX_VOICES; index++)
        if (mixerVoices[index].active &&
            mixerVoices[index].soundType == soundType)
            {
            active = 1;
            break;
            }
    pthread_mutex_unlock(&mixerMutex);
    return active;
    }

static short clipSample(double sample)
    {
    if (sample > 32767.0)
        return 32767;
    if (sample < -32768.0)
        return -32768;
    return (short)sample;
    }

void audioMixerRender(short *stereoFrames, unsigned long frameCount,
                      unsigned long outputRate)
    {
    unsigned long frame;
    int voiceIndex;
    unsigned long sourceFrame;
    long leftSample;
    long rightSample;
    double mixedLeft;
    double mixedRight;
    double step;
    struct AudioMixerVoice *voice;
    struct AudioMixerSample *sample;

    if (stereoFrames == NULL || outputRate == 0)
        return;
    memset(stereoFrames, 0, frameCount * 2 * sizeof(short));
    if (pthread_mutex_trylock(&mixerMutex) != 0)
        return;
    mixerOutputRate = outputRate;
    for (frame = 0; frame < frameCount; frame++)
        {
        mixedLeft = 0.0;
        mixedRight = 0.0;
        for (voiceIndex = 0; voiceIndex < AUDIO_MIXER_MAX_VOICES;
             voiceIndex++)
            {
            voice = &mixerVoices[voiceIndex];
            if (!voice->active)
                continue;
            sample = &mixerSamples[voice->slot];
            sourceFrame = (unsigned long)voice->position;
            if (sourceFrame >= sample->frameCount)
                {
                if (!voice->loop)
                    {
                    voice->active = 0;
                    continue;
                    }
                voice->position = 0.0;
                sourceFrame = 0;
                }
            if (sample->channels == 1)
                leftSample = rightSample = sample->frames[sourceFrame];
            else
                {
                leftSample = sample->frames[sourceFrame * 2];
                rightSample = sample->frames[sourceFrame * 2 + 1];
                }
            mixedLeft += (double)leftSample * voice->leftGain;
            mixedRight += (double)rightSample * voice->rightGain;
            if (voice->gainRampFrames > 0)
                {
                voice->leftGain += voice->leftGainStep;
                voice->rightGain += voice->rightGainStep;
                voice->gainRampFrames--;
                if (voice->gainRampFrames == 0)
                    {
                    voice->leftGain = voice->targetLeftGain;
                    voice->rightGain = voice->targetRightGain;
                    }
                }
            step = (double)sample->sampleRate / (double)outputRate;
            voice->position += step;
            }
        stereoFrames[frame * 2] = clipSample(mixedLeft);
        stereoFrames[frame * 2 + 1] = clipSample(mixedRight);
        }
    pthread_mutex_unlock(&mixerMutex);
    }
