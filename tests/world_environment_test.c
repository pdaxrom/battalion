/*
 * Copyright (c) 2026 Battalion contributors.
 */
#include "world_environment.h"

#include <stdio.h>

static int require(int condition, const char *message)
    {
    if (!condition)
        fprintf(stderr, "%s\n", message);
    return condition;
    }

int main(void)
    {
    struct WorldEnvironment noon;
    struct WorldEnvironment midnight;
    struct WorldEnvironment sunrise;

    worldEnvironmentSetHour(&noon, 12.0f);
    worldEnvironmentSetHour(&midnight, 0.0f);
    worldEnvironmentSetHour(&sunrise, 6.0f);
    if (!require(noon.sunDirection[1] > 0.95f,
                 "noon sun is not overhead") ||
        !require(noon.sunVisibility > 0.95f,
                 "noon sun is not visible") ||
        !require(noon.starVisibility < 0.05f,
                 "stars are visible at noon") ||
        !require(midnight.moonDirection[1] > 0.95f,
                 "midnight moon is not overhead") ||
        !require(midnight.starVisibility > 0.95f,
                 "stars are not visible at midnight") ||
        !require(sunrise.skyHorizon[0] > sunrise.skyZenith[0],
                 "sunrise horizon has no warm gradient"))
        return 1;
    worldEnvironmentAdvance(&midnight, 25.0f);
    if (!require(midnight.hour > 0.99f && midnight.hour < 1.01f,
                 "time-of-day wrapping failed"))
        return 1;
    return 0;
    }
