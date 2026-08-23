/*
 * Copyright (c) 2026 Battalion contributors.
 */
#include "world_environment.h"

#include <math.h>

#define ENVIRONMENT_PI 3.14159265358979323846

static float clampUnit(float value)
    {
    if (value < 0.0f)
        return 0.0f;
    if (value > 1.0f)
        return 1.0f;
    return value;
    }

static float mixValue(float start, float end, float amount)
    {
    return start + (end - start) * amount;
    }

static void mixColor(float output[4], const float start[4],
                     const float end[4], float amount)
    {
    int component;

    for (component = 0; component < 4; component++)
        output[component] = mixValue(start[component], end[component],
                                     amount);
    }

void worldEnvironmentSetHour(struct WorldEnvironment *environment,
                             float hour)
    {
    static const float nightZenith[4] = {0.008f, 0.015f, 0.05f, 1.0f};
    static const float dayZenith[4] = {0.10f, 0.42f, 0.82f, 1.0f};
    static const float nightHorizon[4] = {0.025f, 0.035f, 0.08f, 1.0f};
    static const float dayHorizon[4] = {0.52f, 0.72f, 0.94f, 1.0f};
    float angle;
    float daylight;
    float horizontal;
    float length;
    float sunset;

    while (hour < 0.0f)
        hour += 24.0f;
    while (hour >= 24.0f)
        hour -= 24.0f;
    environment->hour = hour;
    angle = (hour - 6.0f) * (float)(2.0 * ENVIRONMENT_PI / 24.0);
    horizontal = (float)cos((double)angle);
    environment->sunDirection[0] = horizontal * 0.82f;
    environment->sunDirection[1] = (float)sin((double)angle);
    environment->sunDirection[2] = horizontal * 0.57f;
    length = (float)sqrt((double)(
        environment->sunDirection[0] * environment->sunDirection[0] +
        environment->sunDirection[1] * environment->sunDirection[1] +
        environment->sunDirection[2] * environment->sunDirection[2]));
    environment->sunDirection[0] /= length;
    environment->sunDirection[1] /= length;
    environment->sunDirection[2] /= length;
    environment->moonDirection[0] = -environment->sunDirection[0];
    environment->moonDirection[1] = -environment->sunDirection[1];
    environment->moonDirection[2] = -environment->sunDirection[2];

    daylight = clampUnit(environment->sunDirection[1] * 3.2f + 0.25f);
    environment->sunVisibility = clampUnit(
        environment->sunDirection[1] * 6.0f + 0.15f);
    environment->moonVisibility = clampUnit(
        environment->moonDirection[1] * 6.0f + 0.15f);
    environment->starVisibility = clampUnit(
        -environment->sunDirection[1] * 4.0f + 0.05f);
    mixColor(environment->skyZenith, nightZenith, dayZenith, daylight);
    mixColor(environment->skyHorizon, nightHorizon, dayHorizon, daylight);

    sunset = clampUnit(1.0f -
        (float)fabs((double)environment->sunDirection[1]) * 5.0f);
    sunset *= clampUnit(horizontal * horizontal * 1.4f);
    environment->skyHorizon[0] += sunset * 0.35f;
    environment->skyHorizon[1] += sunset * 0.10f;
    if (environment->skyHorizon[0] > 1.0f)
        environment->skyHorizon[0] = 1.0f;
    if (environment->skyHorizon[1] > 1.0f)
        environment->skyHorizon[1] = 1.0f;

    environment->sunColor[0] = 1.0f;
    environment->sunColor[1] = mixValue(0.62f, 0.95f, daylight);
    environment->sunColor[2] = mixValue(0.25f, 0.78f, daylight);
    environment->sunColor[3] = environment->sunVisibility;
    environment->moonColor[0] = 0.72f;
    environment->moonColor[1] = 0.80f;
    environment->moonColor[2] = 1.0f;
    environment->moonColor[3] = environment->moonVisibility;

    if (environment->sunDirection[1] > 0.02f)
        {
        environment->lightDirection[0] = environment->sunDirection[0];
        environment->lightDirection[1] = environment->sunDirection[1];
        environment->lightDirection[2] = environment->sunDirection[2];
        environment->shadowStrength = 0.18f + daylight * 0.28f;
        }
    else
        {
        environment->lightDirection[0] = environment->moonDirection[0];
        environment->lightDirection[1] = environment->moonDirection[1];
        environment->lightDirection[2] = environment->moonDirection[2];
        environment->shadowStrength = environment->moonVisibility * 0.16f;
        }
    }

void worldEnvironmentAdvance(struct WorldEnvironment *environment,
                             float hours)
    {
    worldEnvironmentSetHour(environment, environment->hour + hours);
    }
