/*
 * Copyright (c) 2026 Battalion contributors.
 */

#ifndef BATTALION_MAP_H
#define BATTALION_MAP_H

#include <stddef.h>
#include <stdio.h>

void battalionMapRequest(const char *selector);
int battalionMapResolve(const char *dataPath, char *error, size_t errorSize);
int battalionMapBuildPackagePath(const char *dataPath,
                                 char *path, size_t pathSize);
const char *battalionMapId(void);
const char *battalionMapName(void);
void battalionMapPrintAvailable(const char *dataPath, FILE *stream);

#endif
