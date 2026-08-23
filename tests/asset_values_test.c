/*
 * Copyright (c) 2026 Battalion contributors.
 */
#include "asset_values.h"

#include <math.h>
#include <stdio.h>

static int nearValue(float left, float right)
    {
    return fabs(left - right) < 0.0001f;
    }

int main(void)
    {
    struct AssetValues values;
    const struct AssetValue *value;
    char error[160];

    assetValuesInitialize(&values);
    if (!assetValuesLoad("assets/config/effects.values", &values,
                         error, sizeof(error)))
        {
        fprintf(stderr, "effect values failed to load: %s\n", error);
        return 1;
        }
    value = assetValuesFind(&values, "beam-line-counts");
    if (value == NULL || value->componentCount != 3 ||
        !nearValue(value->components[0], 1.0f) ||
        !nearValue(value->components[2], 8.0f) ||
        assetValuesFind(&values, "trail-line-width") == NULL)
        {
        fprintf(stderr, "unexpected dynamic effect values\n");
        assetValuesDestroy(&values);
        return 1;
        }
    assetValuesDestroy(&values);
    if (!assetValuesLoad("assets/config/palette.values", &values,
                         error, sizeof(error)))
        {
        fprintf(stderr, "palette values failed to load: %s\n", error);
        return 1;
        }
    value = assetValuesFind(&values, "black");
    if (value == NULL || value->componentCount != 4 ||
        !nearValue(value->components[0], 0.2f) ||
        !nearValue(value->components[3], 0.95f) ||
        assetValuesFind(&values, "tree-2") == NULL)
        {
        fprintf(stderr, "unexpected external palette values\n");
        assetValuesDestroy(&values);
        return 1;
        }
    assetValuesDestroy(&values);
    if (!assetValuesLoad("assets/config/ui.values", &values,
                         error, sizeof(error)))
        {
        fprintf(stderr, "UI values failed to load: %s\n", error);
        return 1;
        }
    value = assetValuesFind(&values, "energy-bar");
    if (value == NULL || value->componentCount != 5 ||
        !nearValue(value->components[0], -4.0f) ||
        !nearValue(value->components[3], 4.81f) ||
        assetValuesFind(&values, "summary-box") == NULL)
        {
        fprintf(stderr, "unexpected external UI values\n");
        assetValuesDestroy(&values);
        return 1;
        }
    assetValuesDestroy(&values);
    return 0;
    }
