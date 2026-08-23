/*
 * Copyright (c) 2026 Battalion contributors.
 */
#include "entity_wreck.h"

#include <stdio.h>

static int closeEnough(float left, float right)
    {
    float difference;

    difference = left - right;
    if (difference < 0.0f)
        difference = -difference;
    return difference < 0.00001f;
    }

static int testAirFragmentFallsAndStops(void)
    {
    float x;
    float y;
    float z;
    float xv;
    float yv;
    float zv;
    int grounded;
    int landed;
    int steps;

    x = 1.0f;
    y = 3.0f;
    z = -2.0f;
    xv = 0.1f;
    yv = 0.05f;
    zv = -0.05f;
    grounded = 0;
    landed = 0;
    steps = 0;
    while (!grounded && steps < 200)
        {
        landed += entityWreckStep(&x, &y, &z, &xv, &yv, &zv,
                                  -0.9f, 0.01f, &grounded);
        steps++;
        }

    if (!grounded || landed != 1 || !closeEnough(y, -0.9f))
        return 0;
    if (!closeEnough(xv, 0.0f) || !closeEnough(yv, 0.0f) ||
        !closeEnough(zv, 0.0f))
        return 0;
    return x > 1.0f && z < -2.0f;
    }

static int testGroundedFragmentDoesNotMove(void)
    {
    float x;
    float y;
    float z;
    float xv;
    float yv;
    float zv;
    int grounded;

    x = 4.0f;
    y = -0.8f;
    z = 5.0f;
    xv = 1.0f;
    yv = 1.0f;
    zv = 1.0f;
    grounded = 1;
    if (entityWreckStep(&x, &y, &z, &xv, &yv, &zv,
                        -0.8f, 0.01f, &grounded))
        return 0;
    return closeEnough(x, 4.0f) && closeEnough(y, -0.8f) &&
           closeEnough(z, 5.0f);
    }

int main(void)
    {
    if (!testAirFragmentFallsAndStops())
        {
        fprintf(stderr, "air wreck fragment physics failed\n");
        return 1;
        }
    if (!testGroundedFragmentDoesNotMove())
        {
        fprintf(stderr, "grounded wreck fragment moved\n");
        return 1;
        }
    return 0;
    }
