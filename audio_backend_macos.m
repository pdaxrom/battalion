/*
 * Copyright (c) 2026 Battalion contributors.
 */

#import <AudioUnit/AudioUnit.h>

#include "audio_backend.h"
#include "audio_mixer.h"

#include <string.h>

static AudioComponentInstance outputUnit;

static OSStatus renderAudio(void *context,
                            AudioUnitRenderActionFlags *flags,
                            const AudioTimeStamp *timeStamp,
                            UInt32 busNumber,
                            UInt32 frameCount,
                            AudioBufferList *buffers)
    {
    AudioBuffer *buffer;
    unsigned long availableFrames;

    (void)context;
    (void)flags;
    (void)timeStamp;
    (void)busNumber;
    if (buffers == NULL || buffers->mNumberBuffers == 0)
        return noErr;
    buffer = &buffers->mBuffers[0];
    availableFrames = buffer->mDataByteSize / (sizeof(short) * 2);
    if (availableFrames > frameCount)
        availableFrames = frameCount;
    audioMixerRender((short *)buffer->mData, availableFrames,
                     BATTALION_AUDIO_RATE);
    return noErr;
    }

int audioBackendStart(void)
    {
    AudioComponentDescription description;
    AudioComponent component;
    AudioStreamBasicDescription format;
    AURenderCallbackStruct callback;
    OSStatus status;

    memset(&description, 0, sizeof(description));
    description.componentType = kAudioUnitType_Output;
    description.componentSubType = kAudioUnitSubType_DefaultOutput;
    description.componentManufacturer = kAudioUnitManufacturer_Apple;
    component = AudioComponentFindNext(NULL, &description);
    if (component == NULL)
        return 0;
    status = AudioComponentInstanceNew(component, &outputUnit);
    if (status != noErr)
        return 0;

    memset(&format, 0, sizeof(format));
    format.mSampleRate = BATTALION_AUDIO_RATE;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsSignedInteger |
                          kAudioFormatFlagIsPacked |
                          kAudioFormatFlagsNativeEndian;
    format.mBytesPerPacket = sizeof(short) * 2;
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = sizeof(short) * 2;
    format.mChannelsPerFrame = 2;
    format.mBitsPerChannel = 16;
    status = AudioUnitSetProperty(outputUnit,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input, 0,
                                  &format, sizeof(format));
    if (status != noErr)
        goto failed;

    callback.inputProc = renderAudio;
    callback.inputProcRefCon = NULL;
    status = AudioUnitSetProperty(outputUnit,
                                  kAudioUnitProperty_SetRenderCallback,
                                  kAudioUnitScope_Input, 0,
                                  &callback, sizeof(callback));
    if (status != noErr)
        goto failed;
    status = AudioUnitInitialize(outputUnit);
    if (status != noErr)
        goto failed;
    status = AudioOutputUnitStart(outputUnit);
    if (status != noErr)
        {
        AudioUnitUninitialize(outputUnit);
        goto failed;
        }
    return 1;

failed:
    AudioComponentInstanceDispose(outputUnit);
    outputUnit = NULL;
    return 0;
    }

void audioBackendStop(void)
    {
    if (outputUnit == NULL)
        return;
    AudioOutputUnitStop(outputUnit);
    AudioUnitUninitialize(outputUnit);
    AudioComponentInstanceDispose(outputUnit);
    outputUnit = NULL;
    }
