/*
 * Copyright (c) 2026 Battalion contributors.
 */
#ifndef ENTITY_WRECK_H
#define ENTITY_WRECK_H

/*
 * Renderer-neutral motion for one fragment of a destroyed entity.
 * Returns non-zero on the frame where the fragment reaches the ground.
 */
int entityWreckStep(float *x, float *y, float *z,
                    float *xVelocity, float *yVelocity,
                    float *zVelocity, float groundY,
                    float gravity, int *grounded);

#endif
