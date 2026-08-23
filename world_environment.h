/*
 * Copyright (c) 2026 Battalion contributors.
 */
#ifndef WORLD_ENVIRONMENT_H
#define WORLD_ENVIRONMENT_H

struct WorldEnvironment
    {
    float hour;
    float sunDirection[3];
    float moonDirection[3];
    float lightDirection[3];
    float skyZenith[4];
    float skyHorizon[4];
    float sunColor[4];
    float moonColor[4];
    float starVisibility;
    float sunVisibility;
    float moonVisibility;
    float shadowStrength;
    };

void worldEnvironmentSetHour(struct WorldEnvironment *environment,
                             float hour);
void worldEnvironmentAdvance(struct WorldEnvironment *environment,
                             float hours);

#endif
