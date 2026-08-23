#include "audio_mixer.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static long channelEnergy(const short *frames, unsigned long count,
                          int channel)
    {
    unsigned long index;
    long energy;
    long value;

    energy = 0;
    for (index = 0; index < count; index++)
        {
        value = frames[index * 2 + channel];
        if (value < 0)
            value = -value;
        energy += value;
        }
    return energy;
    }

int main(void)
    {
    float left;
    float right;
    float nearLeft;
    float farLeft;
    short frames[512 * 2];
    short transitionFrames[512 * 2];
    short settlingFrames[1024 * 2];
    char error[160];
    long leftEnergy;
    long rightEnergy;

    audioMixerComputeSpatialGains(0.0f, 0.0f, 0.0f, 0.0f,
                                  10.0f, 0.0f, 0.0f, 1.0f,
                                  &left, &right);
    assert(right > left);
    audioMixerComputeSpatialGains(0.0f, 0.0f, 0.0f, 0.0f,
                                  -10.0f, 0.0f, 0.0f, 1.0f,
                                  &left, &right);
    assert(left > right);
    audioMixerComputeSpatialGains(0.0f, 0.0f, 0.0f,
                                  3.14159265f,
                                  10.0f, 0.0f, 0.0f, 1.0f,
                                  &left, &right);
    assert(left > right);
    audioMixerComputeSpatialGains(0.0f, 0.0f, 0.0f, 0.0f,
                                  0.0f, 0.0f, 2.0f, 1.0f,
                                  &nearLeft, &right);
    audioMixerComputeSpatialGains(0.0f, 0.0f, 0.0f, 0.0f,
                                  0.0f, 0.0f, 30.0f, 1.0f,
                                  &farLeft, &right);
    assert(nearLeft > farLeft);

    audioMixerInitialize();
    assert(audioMixerLoadWav(0, "assets/sounds/beam.wav",
                             error, sizeof(error)));
    audioMixerSetListener(0.0f, 0.0f, 0.0f, 0.0f);
    assert(audioMixerPlay(0, 1, 0, 0, 1,
                          10.0f, 0.0f, 0.0f, 1.0f));
    audioMixerRender(frames, 512, 44100);
    leftEnergy = channelEnergy(frames, 512, 0);
    rightEnergy = channelEnergy(frames, 512, 1);
    assert(rightEnergy > leftEnergy * 2);
    audioMixerShutdown();

    audioMixerInitialize();
    audioMixerSetSmoothingEnabled(1);
    assert(audioMixerGetSmoothingEnabled());
    assert(audioMixerLoadWav(0, "assets/sounds/beam.wav",
                             error, sizeof(error)));
    audioMixerSetListener(0.0f, 0.0f, 0.0f, 0.0f);
    assert(audioMixerPlay(0, 2, 0, 0, 1,
                          10.0f, 0.0f, 0.0f, 1.0f));
    audioMixerRender(frames, 512, 44100);
    audioMixerSetListener(0.0f, 0.0f, 0.0f, 3.14159265f);
    audioMixerRender(transitionFrames, 512, 44100);
    leftEnergy = channelEnergy(transitionFrames, 512, 0);
    rightEnergy = channelEnergy(transitionFrames, 512, 1);
    assert(rightEnergy > leftEnergy);
    audioMixerRender(settlingFrames, 1024, 44100);
    audioMixerRender(frames, 512, 44100);
    leftEnergy = channelEnergy(frames, 512, 0);
    rightEnergy = channelEnergy(frames, 512, 1);
    assert(leftEnergy > rightEnergy * 2);
    audioMixerShutdown();

    audioMixerInitialize();
    audioMixerSetSmoothingEnabled(0);
    assert(!audioMixerGetSmoothingEnabled());
    assert(audioMixerLoadWav(0, "assets/sounds/beam.wav",
                             error, sizeof(error)));
    audioMixerSetListener(0.0f, 0.0f, 0.0f, 0.0f);
    assert(audioMixerPlay(0, 3, 0, 0, 1,
                          10.0f, 0.0f, 0.0f, 1.0f));
    audioMixerRender(frames, 512, 44100);
    audioMixerSetListener(0.0f, 0.0f, 0.0f, 3.14159265f);
    audioMixerRender(frames, 512, 44100);
    leftEnergy = channelEnergy(frames, 512, 0);
    rightEnergy = channelEnergy(frames, 512, 1);
    assert(leftEnergy > rightEnergy * 2);
    audioMixerShutdown();
    printf("audio mixer tests passed\n");
    return 0;
    }
