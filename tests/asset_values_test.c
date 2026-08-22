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
    if (!assetValuesLoad("assets/config/animation.values", &values,
                         error, sizeof(error)))
        {
        fprintf(stderr, "animation values failed to load: %s\n", error);
        return 1;
        }
    value = assetValuesFind(&values, "googelon-leg-left");
    if (value == NULL || value->componentCount != 3 ||
        !nearValue(value->components[0], 0.2f) ||
        !nearValue(value->components[2], 1.0f) ||
        assetValuesFind(&values, "missing-value") != NULL)
        {
        fprintf(stderr, "unexpected animation controller values\n");
        assetValuesDestroy(&values);
        return 1;
        }
    assetValuesDestroy(&values);
    return 0;
    }
