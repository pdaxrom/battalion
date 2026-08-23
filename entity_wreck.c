/*
 * Copyright (c) 2026 Battalion contributors.
 */
#include "entity_wreck.h"

int entityWreckStep(float *x, float *y, float *z,
                    float *xVelocity, float *yVelocity,
                    float *zVelocity, float groundY,
                    float gravity, int *grounded)
    {
    if (*grounded)
        return 0;

    *x += *xVelocity;
    *y += *yVelocity;
    *z += *zVelocity;
    *yVelocity -= gravity;

    if (*y > groundY)
        return 0;

    *y = groundY;
    *xVelocity = 0.0f;
    *yVelocity = 0.0f;
    *zVelocity = 0.0f;
    *grounded = 1;
    return 1;
    }
