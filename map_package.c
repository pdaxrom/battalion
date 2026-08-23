/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Versioned JSON battlefield package loader.                          */
/***********************************************************************/

#include "map_package.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum MapJsonTokenType
    {
    MAP_JSON_UNDEFINED,
    MAP_JSON_OBJECT,
    MAP_JSON_ARRAY,
    MAP_JSON_STRING,
    MAP_JSON_PRIMITIVE
    };

struct MapJsonToken
    {
    enum MapJsonTokenType type;
    int start;
    int end;
    int parent;
    };

struct MapJsonParser
    {
    int position;
    int nextToken;
    int parent;
    };

static void setMapError(char *error, size_t errorSize, const char *message)
    {
    if (errorSize == 0)
        return;
    strncpy(error, message, errorSize - 1);
    error[errorSize - 1] = '\0';
    }

static struct MapJsonToken *allocateToken(struct MapJsonParser *parser,
                                           struct MapJsonToken *tokens,
                                           int capacity)
    {
    struct MapJsonToken *token;

    if (parser->nextToken >= capacity)
        return NULL;
    token = &tokens[parser->nextToken++];
    token->type = MAP_JSON_UNDEFINED;
    token->start = -1;
    token->end = -1;
    token->parent = -1;
    return token;
    }

static int parseString(struct MapJsonParser *parser, const char *json,
                       size_t length, struct MapJsonToken *tokens,
                       int capacity)
    {
    int start;
    struct MapJsonToken *token;

    start = parser->position + 1;
    parser->position++;
    while ((size_t)parser->position < length)
        {
        if (json[parser->position] == '"')
            {
            token = allocateToken(parser, tokens, capacity);
            if (token == NULL)
                return 0;
            token->type = MAP_JSON_STRING;
            token->start = start;
            token->end = parser->position;
            token->parent = parser->parent;
            return 1;
            }
        if (json[parser->position] == '\\')
            {
            parser->position++;
            if ((size_t)parser->position >= length)
                return 0;
            }
        parser->position++;
        }
    return 0;
    }

static int parsePrimitive(struct MapJsonParser *parser, const char *json,
                          size_t length, struct MapJsonToken *tokens,
                          int capacity)
    {
    int start;
    char value;
    struct MapJsonToken *token;

    start = parser->position;
    while ((size_t)parser->position < length)
        {
        value = json[parser->position];
        if (value == ',' || value == ']' || value == '}' || value == ' ' ||
            value == '\t' || value == '\r' || value == '\n')
            break;
        if ((unsigned char)value < 32)
            return 0;
        parser->position++;
        }
    if (start == parser->position)
        return 0;

    token = allocateToken(parser, tokens, capacity);
    if (token == NULL)
        return 0;
    token->type = MAP_JSON_PRIMITIVE;
    token->start = start;
    token->end = parser->position;
    token->parent = parser->parent;
    parser->position--;
    return 1;
    }

static int parseJson(const char *json, size_t length,
                     struct MapJsonToken *tokens, int capacity)
    {
    struct MapJsonParser parser;
    struct MapJsonToken *token;
    enum MapJsonTokenType type;
    int index;
    char value;

    parser.position = 0;
    parser.nextToken = 0;
    parser.parent = -1;
    while ((size_t)parser.position < length)
        {
        value = json[parser.position];
        if (value == '{' || value == '[')
            {
            token = allocateToken(&parser, tokens, capacity);
            if (token == NULL)
                return -1;
            token->type = value == '{' ? MAP_JSON_OBJECT : MAP_JSON_ARRAY;
            token->start = parser.position;
            token->parent = parser.parent;
            parser.parent = parser.nextToken - 1;
            }
        else if (value == '}' || value == ']')
            {
            type = value == '}' ? MAP_JSON_OBJECT : MAP_JSON_ARRAY;
            for (index = parser.nextToken - 1; index >= 0; index--)
                if (tokens[index].start >= 0 && tokens[index].end < 0)
                    {
                    if (tokens[index].type != type)
                        return -2;
                    tokens[index].end = parser.position + 1;
                    parser.parent = tokens[index].parent;
                    break;
                    }
            if (index < 0)
                return -2;
            }
        else if (value == '"')
            {
            if (!parseString(&parser, json, length, tokens, capacity))
                return -2;
            }
        else if (value == ' ' || value == '\t' || value == '\r' ||
                 value == '\n' || value == ':' || value == ',')
            {
            }
        else if (!parsePrimitive(&parser, json, length, tokens, capacity))
            return -2;
        parser.position++;
        }

    for (index = parser.nextToken - 1; index >= 0; index--)
        if (tokens[index].start >= 0 && tokens[index].end < 0)
            return -2;
    return parser.nextToken;
    }

static int tokenEquals(const char *json, const struct MapJsonToken *token,
                       const char *text)
    {
    size_t length;

    length = strlen(text);
    return token->type == MAP_JSON_STRING &&
           (size_t)(token->end - token->start) == length &&
           strncmp(json + token->start, text, length) == 0;
    }

static int objectValue(const char *json, const struct MapJsonToken *tokens,
                       int tokenCount, int object, const char *key)
    {
    int index;
    int expectKey;

    if (object < 0 || object >= tokenCount ||
        tokens[object].type != MAP_JSON_OBJECT)
        return -1;
    expectKey = 1;
    for (index = object + 1; index + 1 < tokenCount; index++)
        {
        if (tokens[index].start >= tokens[object].end)
            break;
        if (tokens[index].parent != object)
            continue;
        if (expectKey && tokenEquals(json, &tokens[index], key))
            {
            if (tokens[index + 1].parent == object)
                return index + 1;
            return -1;
            }
        expectKey = !expectKey;
        }
    return -1;
    }

static int copyString(const char *json, const struct MapJsonToken *token,
                      char *destination, size_t destinationSize)
    {
    size_t length;
    size_t source;
    size_t target;
    char value;

    if (token->type != MAP_JSON_STRING || destinationSize == 0)
        return 0;
    source = (size_t)token->start;
    target = 0;
    while (source < (size_t)token->end)
        {
        value = json[source++];
        if (value == '\\')
            {
            if (source >= (size_t)token->end)
                return 0;
            value = json[source++];
            if (value == 'n') value = '\n';
            else if (value == 'r') value = '\r';
            else if (value == 't') value = '\t';
            else if (value != '\\' && value != '"' && value != '/')
                return 0;
            }
        if (target + 1 >= destinationSize)
            return 0;
        destination[target++] = value;
        }
    destination[target] = '\0';
    length = target;
    return length < destinationSize;
    }

static int tokenFloat(const char *json, const struct MapJsonToken *token,
                      float *value)
    {
    char number[64];
    char *end;
    size_t length;
    double parsed;

    if (token->type != MAP_JSON_PRIMITIVE)
        return 0;
    length = (size_t)(token->end - token->start);
    if (length == 0 || length >= sizeof(number))
        return 0;
    memcpy(number, json + token->start, length);
    number[length] = '\0';
    errno = 0;
    parsed = strtod(number, &end);
    if (errno || *end)
        return 0;
    *value = (float)parsed;
    return 1;
    }

static int tokenInt(const char *json, const struct MapJsonToken *token,
                    int *value)
    {
    char number[32];
    char *end;
    long parsed;
    size_t length;

    if (token->type != MAP_JSON_PRIMITIVE)
        return 0;
    length = (size_t)(token->end - token->start);
    if (length == 0 || length >= sizeof(number))
        return 0;
    memcpy(number, json + token->start, length);
    number[length] = '\0';
    errno = 0;
    parsed = strtol(number, &end, 10);
    if (errno || *end || parsed < -2147483647L || parsed > 2147483647L)
        return 0;
    *value = (int)parsed;
    return 1;
    }

static int tokenBool(const char *json, const struct MapJsonToken *token,
                     int *value)
    {
    size_t length;

    if (token->type != MAP_JSON_PRIMITIVE)
        return 0;
    length = (size_t)(token->end - token->start);
    if (length == 4 && strncmp(json + token->start, "true", 4) == 0)
        {
        *value = 1;
        return 1;
        }
    if (length == 5 && strncmp(json + token->start, "false", 5) == 0)
        {
        *value = 0;
        return 1;
        }
    return 0;
    }

static int validEntityName(const char *name)
    {
    const unsigned char *cursor;

    if (name == NULL || name[0] == '\0')
        return 0;
    for (cursor = (const unsigned char *)name; *cursor; cursor++)
        if (!(islower(*cursor) || isdigit(*cursor) || *cursor == '-'))
            return 0;
    return 1;
    }

static size_t arrayCount(const struct MapJsonToken *tokens, int tokenCount,
                         int array)
    {
    size_t count;
    int index;

    count = 0;
    for (index = array + 1; index < tokenCount; index++)
        {
        if (tokens[index].start >= tokens[array].end)
            break;
        if (tokens[index].parent == array)
            count++;
        }
    return count;
    }

static int loadObjectPath(const char *json,
                          const struct MapJsonToken *tokens,
                          int tokenCount, int array,
                          struct BattalionMapObject *object)
    {
    size_t output;
    int index;
    int xToken;
    int zToken;

    if (array < 0)
        return 1;
    if (tokens[array].type != MAP_JSON_ARRAY)
        return 0;
    object->waypointCount = arrayCount(tokens, tokenCount, array);
    if (object->waypointCount > 0)
        object->waypoints = (struct EntityWaypoint *)calloc(
            object->waypointCount, sizeof(struct EntityWaypoint));
    if (object->waypointCount > 0 && object->waypoints == NULL)
        return 0;
    output = 0;
    for (index = array + 1;
         index < tokenCount && output < object->waypointCount; index++)
        if (tokens[index].parent == array)
            {
            if (tokens[index].type != MAP_JSON_OBJECT)
                return 0;
            xToken = objectValue(json, tokens, tokenCount, index, "x");
            zToken = objectValue(json, tokens, tokenCount, index, "z");
            if (xToken < 0 || zToken < 0 ||
                !tokenFloat(json, &tokens[xToken],
                            &object->waypoints[output].x) ||
                !tokenFloat(json, &tokens[zToken],
                            &object->waypoints[output].z))
                return 0;
            output++;
            }
    return output == object->waypointCount;
    }

static int loadObjectGeometry(const char *json,
                              const struct MapJsonToken *tokens,
                              int tokenCount, int array,
                              struct BattalionMapObject *object)
    {
    size_t output;
    int index;
    int xToken;
    int zToken;

    if (array < 0)
        return 1;
    if (tokens[array].type != MAP_JSON_ARRAY)
        return 0;
    object->geometryCount = arrayCount(tokens, tokenCount, array);
    if (object->geometryCount > 0)
        object->geometry = (struct BattalionMapPoint *)calloc(
            object->geometryCount, sizeof(struct BattalionMapPoint));
    if (object->geometryCount > 0 && object->geometry == NULL)
        return 0;
    output = 0;
    for (index = array + 1;
         index < tokenCount && output < object->geometryCount; index++)
        if (tokens[index].parent == array)
            {
            if (tokens[index].type != MAP_JSON_OBJECT)
                return 0;
            xToken = objectValue(json, tokens, tokenCount, index, "x");
            zToken = objectValue(json, tokens, tokenCount, index, "z");
            if (xToken < 0 || zToken < 0 ||
                !tokenFloat(json, &tokens[xToken],
                            &object->geometry[output].x) ||
                !tokenFloat(json, &tokens[zToken],
                            &object->geometry[output].z))
                return 0;
            output++;
            }
    return output == object->geometryCount;
    }

static int loadObjectProperties(const char *json,
                                const struct MapJsonToken *tokens,
                                int tokenCount, int array,
                                struct BattalionMapObject *object)
    {
    size_t output;
    int index;
    int nameToken;
    int valueToken;
    struct BattalionMapProperty *property;

    if (array < 0)
        return 1;
    if (tokens[array].type != MAP_JSON_ARRAY)
        return 0;
    object->propertyCount = arrayCount(tokens, tokenCount, array);
    if (object->propertyCount > 0)
        object->properties = (struct BattalionMapProperty *)calloc(
            object->propertyCount, sizeof(struct BattalionMapProperty));
    if (object->propertyCount > 0 && object->properties == NULL)
        return 0;
    output = 0;
    for (index = array + 1;
         index < tokenCount && output < object->propertyCount; index++)
        if (tokens[index].parent == array)
            {
            if (tokens[index].type != MAP_JSON_OBJECT)
                return 0;
            nameToken = objectValue(json, tokens, tokenCount, index, "name");
            valueToken = objectValue(json, tokens, tokenCount, index, "value");
            if (nameToken < 0 || valueToken < 0)
                return 0;
            property = &object->properties[output];
            if (!copyString(json, &tokens[nameToken], property->name,
                            sizeof(property->name)) ||
                !copyString(json, &tokens[valueToken], property->value,
                            sizeof(property->value)) ||
                !validEntityName(property->name))
                return 0;
            output++;
            }
    return output == object->propertyCount;
    }

static int loadObjectTriggers(const char *json,
                              const struct MapJsonToken *tokens,
                              int tokenCount, int array,
                              struct BattalionMapObject *object)
    {
    size_t output;
    int index;
    int eventToken;
    int targetIdToken;
    int targetGroupToken;
    struct BattalionMapTrigger *trigger;

    if (array < 0)
        return 1;
    if (tokens[array].type != MAP_JSON_ARRAY)
        return 0;
    object->triggerCount = arrayCount(tokens, tokenCount, array);
    if (object->triggerCount > 0)
        object->triggers = (struct BattalionMapTrigger *)calloc(
            object->triggerCount, sizeof(struct BattalionMapTrigger));
    if (object->triggerCount > 0 && object->triggers == NULL)
        return 0;
    output = 0;
    for (index = array + 1;
         index < tokenCount && output < object->triggerCount; index++)
        if (tokens[index].parent == array)
            {
            if (tokens[index].type != MAP_JSON_OBJECT)
                return 0;
            eventToken = objectValue(json, tokens, tokenCount, index,
                                     "event");
            targetIdToken = objectValue(json, tokens, tokenCount, index,
                                        "target-id");
            targetGroupToken = objectValue(json, tokens, tokenCount, index,
                                           "target-group");
            if (eventToken < 0 ||
                ((targetIdToken >= 0) == (targetGroupToken >= 0)))
                return 0;
            trigger = &object->triggers[output];
            if (!copyString(json, &tokens[eventToken], trigger->event,
                            sizeof(trigger->event)))
                return 0;
            if (targetIdToken >= 0)
                {
                trigger->targetType = BATTALION_MAP_TRIGGER_ID;
                if (!copyString(json, &tokens[targetIdToken],
                                trigger->target, sizeof(trigger->target)))
                    return 0;
                }
            else
                {
                trigger->targetType = BATTALION_MAP_TRIGGER_GROUP;
                if (!copyString(json, &tokens[targetGroupToken],
                                trigger->target, sizeof(trigger->target)))
                    return 0;
                }
            if (!validEntityName(trigger->event) ||
                !validEntityName(trigger->target))
                return 0;
            output++;
            }
    return output == object->triggerCount;
    }

static int patrolModeValue(const char *json,
                           const struct MapJsonToken *token)
    {
    if (tokenEquals(json, token, "loop"))
        return ENTITY_PATROL_LOOP;
    if (tokenEquals(json, token, "pingpong"))
        return ENTITY_PATROL_PINGPONG;
    if (tokenEquals(json, token, "once"))
        return ENTITY_PATROL_ONCE;
    return 0;
    }

static int alarmResponseValue(const char *json,
                              const struct MapJsonToken *token)
    {
    if (tokenEquals(json, token, "inherit"))
        return ENTITY_ALARM_INHERIT;
    if (tokenEquals(json, token, "continue"))
        return ENTITY_ALARM_CONTINUE;
    if (tokenEquals(json, token, "attack"))
        return ENTITY_ALARM_ATTACK;
    if (tokenEquals(json, token, "retreat"))
        return ENTITY_ALARM_RETREAT;
    if (tokenEquals(json, token, "random"))
        return ENTITY_ALARM_RANDOM;
    return -1;
    }

static int loadObjects(const char *json, const struct MapJsonToken *tokens,
                       int tokenCount, int array,
                       struct BattalionMapPackage *map)
    {
    size_t output;
    int index;
    int idToken;
    int archetypeToken;
    int descriptionToken;
    int groupToken;
    int factionToken;
    int patrolRouteToken;
    int safeZoneToken;
    int xToken;
    int yToken;
    int zToken;
    int headingToken;
    int scaleToken;
    int activeToken;
    int patrolModeToken;
    int alarmResponseToken;
    int safeWaitToken;
    int pathToken;
    int geometryToken;
    int propertiesToken;
    int triggersToken;
    struct BattalionMapObject *object;

    if (array < 0 || tokens[array].type != MAP_JSON_ARRAY)
        return 0;
    map->objectCount = arrayCount(tokens, tokenCount, array);
    if (map->objectCount > 0)
        map->objects = (struct BattalionMapObject *)calloc(
            map->objectCount, sizeof(struct BattalionMapObject));
    if (map->objectCount > 0 && map->objects == NULL)
        return 0;
    output = 0;
    for (index = array + 1;
         index < tokenCount && output < map->objectCount; index++)
        if (tokens[index].parent == array)
            {
            if (tokens[index].type != MAP_JSON_OBJECT)
                return 0;
            object = &map->objects[output];
            object->active = 1;
            object->scale = 1.0f;
            object->patrolMode = ENTITY_PATROL_LOOP;
            object->alarmResponse = ENTITY_ALARM_INHERIT;
            idToken = objectValue(json, tokens, tokenCount, index, "id");
            archetypeToken = objectValue(json, tokens, tokenCount, index,
                                         "archetype");
            descriptionToken = objectValue(json, tokens, tokenCount, index,
                                           "description");
            groupToken = objectValue(json, tokens, tokenCount, index,
                                     "group");
            factionToken = objectValue(json, tokens, tokenCount, index,
                                       "faction");
            patrolRouteToken = objectValue(json, tokens, tokenCount, index,
                                           "patrol-route");
            safeZoneToken = objectValue(json, tokens, tokenCount, index,
                                        "safe-zone");
            xToken = objectValue(json, tokens, tokenCount, index, "x");
            yToken = objectValue(json, tokens, tokenCount, index, "y");
            zToken = objectValue(json, tokens, tokenCount, index, "z");
            headingToken = objectValue(json, tokens, tokenCount, index,
                                       "heading");
            scaleToken = objectValue(json, tokens, tokenCount, index,
                                     "scale");
            activeToken = objectValue(json, tokens, tokenCount, index,
                                      "active");
            patrolModeToken = objectValue(json, tokens, tokenCount, index,
                                          "patrol-mode");
            alarmResponseToken = objectValue(
                json, tokens, tokenCount, index, "alarm-response");
            safeWaitToken = objectValue(
                json, tokens, tokenCount, index, "safe-wait-seconds");
            pathToken = objectValue(json, tokens, tokenCount, index, "path");
            geometryToken = objectValue(json, tokens, tokenCount, index,
                                        "geometry");
            propertiesToken = objectValue(json, tokens, tokenCount, index,
                                          "properties");
            triggersToken = objectValue(json, tokens, tokenCount, index,
                                        "triggers");
            if (archetypeToken < 0 || xToken < 0 || zToken < 0 ||
                !copyString(json, &tokens[archetypeToken], object->archetype,
                            sizeof(object->archetype)) ||
                !validEntityName(object->archetype) ||
                !tokenFloat(json, &tokens[xToken], &object->x) ||
                !tokenFloat(json, &tokens[zToken], &object->z))
                return 0;
            if (idToken >= 0 &&
                (!copyString(json, &tokens[idToken], object->id,
                             sizeof(object->id)) ||
                 !validEntityName(object->id)))
                return 0;
            if (descriptionToken >= 0 &&
                !copyString(json, &tokens[descriptionToken],
                            object->description,
                            sizeof(object->description)))
                return 0;
            if (groupToken >= 0 &&
                (!copyString(json, &tokens[groupToken], object->group,
                             sizeof(object->group)) ||
                 !validEntityName(object->group)))
                return 0;
            if (factionToken >= 0 &&
                (!copyString(json, &tokens[factionToken], object->faction,
                             sizeof(object->faction)) ||
                 !validEntityName(object->faction)))
                return 0;
            if (patrolRouteToken >= 0 &&
                (!copyString(json, &tokens[patrolRouteToken],
                             object->patrolRoute,
                             sizeof(object->patrolRoute)) ||
                 !validEntityName(object->patrolRoute)))
                return 0;
            if (safeZoneToken >= 0 &&
                (!copyString(json, &tokens[safeZoneToken],
                             object->safeZone,
                             sizeof(object->safeZone)) ||
                 !validEntityName(object->safeZone)))
                return 0;
            if (headingToken >= 0 &&
                !tokenFloat(json, &tokens[headingToken], &object->heading))
                return 0;
            if (yToken >= 0 &&
                !tokenFloat(json, &tokens[yToken], &object->y))
                return 0;
            if (scaleToken >= 0 &&
                !tokenFloat(json, &tokens[scaleToken], &object->scale))
                return 0;
            if (activeToken >= 0 &&
                !tokenBool(json, &tokens[activeToken], &object->active))
                return 0;
            if (patrolModeToken >= 0)
                {
                object->patrolMode = patrolModeValue(
                    json, &tokens[patrolModeToken]);
                if (object->patrolMode == 0)
                    return 0;
                }
            if (alarmResponseToken >= 0)
                {
                object->alarmResponse = alarmResponseValue(
                    json, &tokens[alarmResponseToken]);
                if (object->alarmResponse < 0)
                    return 0;
                }
            if (safeWaitToken >= 0 &&
                !tokenFloat(json, &tokens[safeWaitToken],
                            &object->safeWaitSeconds))
                return 0;
            if (!loadObjectPath(json, tokens, tokenCount, pathToken, object) ||
                !loadObjectGeometry(json, tokens, tokenCount, geometryToken,
                                    object) ||
                !loadObjectProperties(json, tokens, tokenCount,
                                      propertiesToken, object) ||
                !loadObjectTriggers(json, tokens, tokenCount, triggersToken,
                                    object))
                return 0;
            output++;
            }
    return output == map->objectCount;
    }

static int objectIdExists(const struct BattalionMapPackage *map,
                          const char *id)
    {
    size_t index;

    for (index = 0; index < map->objectCount; index++)
        if (strcmp(map->objects[index].id, id) == 0)
            return 1;
    return 0;
    }

static int objectGroupExists(const struct BattalionMapPackage *map,
                             const char *group)
    {
    size_t index;

    for (index = 0; index < map->objectCount; index++)
        if (strcmp(map->objects[index].group, group) == 0)
            return 1;
    return 0;
    }

static int validFloat(float value)
    {
    return value == value && value <= FLT_MAX && value >= -FLT_MAX;
    }

static int validateObjects(const struct BattalionMapPackage *map,
                           char *error, size_t errorSize)
    {
    size_t objectIndex;
    size_t otherIndex;
    size_t propertyIndex;
    size_t triggerIndex;
    const struct BattalionMapTrigger *trigger;
    const struct BattalionMapObject *object;

    for (objectIndex = 0; objectIndex < map->objectCount; objectIndex++)
        {
        object = &map->objects[objectIndex];
        if ((object->id[0] != '\0' && !validEntityName(object->id)) ||
            !validEntityName(object->archetype) ||
            (object->group[0] != '\0' && !validEntityName(object->group)) ||
            (object->faction[0] != '\0' &&
             !validEntityName(object->faction)) ||
            (object->patrolRoute[0] != '\0' &&
             !validEntityName(object->patrolRoute)) ||
            (object->safeZone[0] != '\0' &&
             !validEntityName(object->safeZone)) ||
            !validFloat(object->x) || !validFloat(object->y) ||
            !validFloat(object->z) || !validFloat(object->heading) ||
            !validFloat(object->scale) || object->scale <= 0.0f ||
            !validFloat(object->safeWaitSeconds) ||
            object->safeWaitSeconds < 0.0f ||
            (object->active != 0 && object->active != 1) ||
            (object->patrolMode != ENTITY_PATROL_LOOP &&
             object->patrolMode != ENTITY_PATROL_PINGPONG &&
             object->patrolMode != ENTITY_PATROL_ONCE) ||
            (object->alarmResponse < ENTITY_ALARM_INHERIT ||
             object->alarmResponse > ENTITY_ALARM_RANDOM) ||
            (object->waypointCount > 0 && object->waypoints == NULL) ||
            (object->geometryCount > 0 && object->geometry == NULL) ||
            (object->propertyCount > 0 && object->properties == NULL) ||
            (object->triggerCount > 0 && object->triggers == NULL))
            {
            setMapError(error, errorSize, "Invalid map object fields");
            return 0;
            }
        if (object->patrolRoute[0] != '\0' &&
            !objectIdExists(map, object->patrolRoute))
            {
            setMapError(error, errorSize,
                        "Patrol route references an unknown object id");
            return 0;
            }
        if (object->safeZone[0] != '\0' &&
            !objectIdExists(map, object->safeZone))
            {
            setMapError(error, errorSize,
                        "Safe zone references an unknown object id");
            return 0;
            }
        if (object->alarmResponse != ENTITY_ALARM_INHERIT &&
            object->patrolRoute[0] == '\0')
            {
            setMapError(error, errorSize,
                        "Alarm response requires a patrol route");
            return 0;
            }
        if ((object->alarmResponse == ENTITY_ALARM_RETREAT ||
             object->alarmResponse == ENTITY_ALARM_RANDOM) !=
            (object->safeZone[0] != '\0'))
            {
            setMapError(error, errorSize,
                        "Retreat response requires exactly one safe zone");
            return 0;
            }
        if (object->alarmResponse != ENTITY_ALARM_RETREAT &&
            object->alarmResponse != ENTITY_ALARM_RANDOM &&
            object->safeWaitSeconds != 0.0f)
            {
            setMapError(error, errorSize,
                        "Safe wait is only valid for retreat responses");
            return 0;
            }
        if (object->id[0] != '\0')
            for (otherIndex = objectIndex + 1;
                 otherIndex < map->objectCount; otherIndex++)
                if (strcmp(object->id, map->objects[otherIndex].id) == 0)
                {
                setMapError(error, errorSize, "Map object ids must be unique");
                return 0;
                }
        for (otherIndex = 0;
             otherIndex < object->waypointCount;
             otherIndex++)
            if (!validFloat(object->waypoints[otherIndex].x) ||
                !validFloat(object->waypoints[otherIndex].z))
                {
                setMapError(error, errorSize,
                            "Invalid patrol waypoint coordinates");
                return 0;
                }
        for (otherIndex = 0; otherIndex < object->geometryCount; otherIndex++)
            if (!validFloat(object->geometry[otherIndex].x) ||
                !validFloat(object->geometry[otherIndex].z))
                {
                setMapError(error, errorSize,
                            "Invalid object geometry coordinates");
                return 0;
                }
        for (propertyIndex = 0; propertyIndex < object->propertyCount;
             propertyIndex++)
            {
            if (!validEntityName(object->properties[propertyIndex].name))
                {
                setMapError(error, errorSize, "Invalid object property name");
                return 0;
                }
            for (otherIndex = propertyIndex + 1;
                 otherIndex < object->propertyCount; otherIndex++)
                if (strcmp(object->properties[propertyIndex].name,
                           object->properties[otherIndex].name) == 0)
                    {
                    setMapError(error, errorSize,
                                "Object property names must be unique");
                    return 0;
                    }
            }
        for (triggerIndex = 0;
             triggerIndex < object->triggerCount;
             triggerIndex++)
            {
            trigger = &object->triggers[triggerIndex];
            if (!validEntityName(trigger->event) ||
                !validEntityName(trigger->target))
                {
                setMapError(error, errorSize, "Invalid trigger route fields");
                return 0;
                }
            if (trigger->targetType == BATTALION_MAP_TRIGGER_ID)
                {
                if (!objectIdExists(map, trigger->target))
                    {
                    setMapError(error, errorSize,
                                "Trigger route targets an unknown object id");
                    return 0;
                    }
                }
            else if (trigger->targetType == BATTALION_MAP_TRIGGER_GROUP)
                {
                if (!objectGroupExists(map, trigger->target))
                    {
                    setMapError(error, errorSize,
                                "Trigger route targets an unknown object group");
                    return 0;
                    }
                }
            else
                {
                setMapError(error, errorSize,
                            "Trigger route has an invalid target type");
                return 0;
                }
            }
        }
    return 1;
    }

int battalionMapPackageValidate(const struct BattalionMapPackage *map,
                                char *error, size_t errorSize)
    {
    size_t index;
    size_t nested;

    if (map == NULL ||
        memchr(map->id, '\0', sizeof(map->id)) == NULL ||
        memchr(map->name, '\0', sizeof(map->name)) == NULL ||
        map->id[0] == '\0' || map->name[0] == '\0' ||
        map->version != 2 ||
        (map->objectCount > 0 && map->objects == NULL))
        {
        setMapError(error, errorSize, "Invalid map package fields");
        return 0;
        }
    for (index = 0; index < map->objectCount; index++)
        {
        if (memchr(map->objects[index].id, '\0',
                   sizeof(map->objects[index].id)) == NULL ||
            memchr(map->objects[index].archetype, '\0',
                   sizeof(map->objects[index].archetype)) == NULL ||
            memchr(map->objects[index].description, '\0',
                   sizeof(map->objects[index].description)) == NULL ||
            memchr(map->objects[index].group, '\0',
                   sizeof(map->objects[index].group)) == NULL ||
            memchr(map->objects[index].faction, '\0',
                   sizeof(map->objects[index].faction)) == NULL ||
            memchr(map->objects[index].patrolRoute, '\0',
                   sizeof(map->objects[index].patrolRoute)) == NULL ||
            memchr(map->objects[index].safeZone, '\0',
                   sizeof(map->objects[index].safeZone)) == NULL ||
            (map->objects[index].waypointCount > 0 &&
             map->objects[index].waypoints == NULL) ||
            (map->objects[index].geometryCount > 0 &&
             map->objects[index].geometry == NULL) ||
            (map->objects[index].propertyCount > 0 &&
             map->objects[index].properties == NULL) ||
            (map->objects[index].triggerCount > 0 &&
             map->objects[index].triggers == NULL))
            {
            setMapError(error, errorSize, "Invalid map object storage");
            return 0;
            }
        for (nested = 0; nested < map->objects[index].propertyCount; nested++)
            if (memchr(map->objects[index].properties[nested].name, '\0',
                       sizeof(map->objects[index].properties[nested].name)) ==
                    NULL ||
                memchr(map->objects[index].properties[nested].value, '\0',
                       sizeof(map->objects[index].properties[nested].value)) ==
                    NULL)
                {
                setMapError(error, errorSize, "Invalid property storage");
                return 0;
                }
        for (nested = 0; nested < map->objects[index].triggerCount; nested++)
            if (memchr(map->objects[index].triggers[nested].event, '\0',
                       sizeof(map->objects[index].triggers[nested].event)) ==
                    NULL ||
                memchr(map->objects[index].triggers[nested].target, '\0',
                       sizeof(map->objects[index].triggers[nested].target)) ==
                    NULL)
                {
                setMapError(error, errorSize, "Invalid trigger route storage");
                return 0;
                }
        }
    return validateObjects(map, error, errorSize);
    }

void battalionMapPackageInit(struct BattalionMapPackage *map)
    {
    if (map != NULL)
        {
        memset(map, 0, sizeof(*map));
        map->version = 2;
        }
    }

void battalionMapPackageFree(struct BattalionMapPackage *map)
    {
    size_t index;

    if (map == NULL)
        return;
    for (index = 0; index < map->objectCount; index++)
        {
        free(map->objects[index].waypoints);
        free(map->objects[index].geometry);
        free(map->objects[index].properties);
        free(map->objects[index].triggers);
        }
    free(map->objects);
    battalionMapPackageInit(map);
    }

static int readFile(const char *path, char **contents, size_t *length,
                    char *error, size_t errorSize)
    {
    FILE *file;
    long fileLength;
    char *bytes;

    file = fopen(path, "rb");
    if (file == NULL)
        {
        setMapError(error, errorSize, "Could not open map package");
        return 0;
        }
    if (fseek(file, 0, SEEK_END) != 0 || (fileLength = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) != 0)
        {
        fclose(file);
        setMapError(error, errorSize, "Could not measure map package");
        return 0;
        }
    bytes = (char *)malloc((size_t)fileLength + 1);
    if (bytes == NULL || fread(bytes, 1, (size_t)fileLength, file) !=
                         (size_t)fileLength)
        {
        free(bytes);
        fclose(file);
        setMapError(error, errorSize, "Could not read map package");
        return 0;
        }
    fclose(file);
    bytes[fileLength] = '\0';
    *contents = bytes;
    *length = (size_t)fileLength;
    return 1;
    }

int battalionMapPackageLoad(const char *path,
                            struct BattalionMapPackage *map,
                            char *error, size_t errorSize)
    {
    char *json;
    size_t length;
    struct MapJsonToken *tokens;
    int tokenCapacity;
    int tokenCount;
    int formatToken;
    int versionToken;
    int idToken;
    int nameToken;
    int objectsToken;
    int roadsToken;
    int structuresToken;
    int unitsToken;
    int actorsToken;
    int version;
    struct BattalionMapPackage loaded;

    if (error != NULL && errorSize > 0)
        error[0] = '\0';
    battalionMapPackageInit(&loaded);
    if (!readFile(path, &json, &length, error, errorSize))
        return 0;
    tokenCapacity = (int)(length + 1);
    tokens = (struct MapJsonToken *)calloc((size_t)tokenCapacity,
                                            sizeof(struct MapJsonToken));
    if (tokens == NULL)
        {
        free(json);
        setMapError(error, errorSize, "Out of memory reading map package");
        return 0;
        }
    tokenCount = parseJson(json, length, tokens, tokenCapacity);
    if (tokenCount <= 0 || tokens[0].type != MAP_JSON_OBJECT)
        goto invalid;

    formatToken = objectValue(json, tokens, tokenCount, 0, "format");
    versionToken = objectValue(json, tokens, tokenCount, 0, "version");
    idToken = objectValue(json, tokens, tokenCount, 0, "id");
    nameToken = objectValue(json, tokens, tokenCount, 0, "name");
    roadsToken = objectValue(json, tokens, tokenCount, 0, "roads");
    structuresToken = objectValue(json, tokens, tokenCount, 0, "structures");
    unitsToken = objectValue(json, tokens, tokenCount, 0, "units");
    actorsToken = objectValue(json, tokens, tokenCount, 0, "actors");
    objectsToken = objectValue(json, tokens, tokenCount, 0, "objects");
    if (formatToken < 0 || versionToken < 0 || idToken < 0 || nameToken < 0 ||
        roadsToken >= 0 || structuresToken >= 0 || unitsToken >= 0 ||
        actorsToken >= 0 || objectsToken < 0 ||
        !tokenEquals(json, &tokens[formatToken], "battalion-map") ||
        !tokenInt(json, &tokens[versionToken], &version) ||
        version != 2 ||
        !copyString(json, &tokens[idToken], loaded.id, sizeof(loaded.id)) ||
        !copyString(json, &tokens[nameToken], loaded.name, sizeof(loaded.name)) ||
        !loadObjects(json, tokens, tokenCount, objectsToken, &loaded))
        goto invalid;
    loaded.version = version;
    if (!battalionMapPackageValidate(&loaded, error, errorSize))
        goto invalid;

    free(tokens);
    free(json);
    battalionMapPackageFree(map);
    *map = loaded;
    return 1;

invalid:
    battalionMapPackageFree(&loaded);
    free(tokens);
    free(json);
    if (error == NULL || errorSize == 0 || error[0] == '\0')
        setMapError(error, errorSize,
                    "Invalid or unsupported battalion-map JSON");
    return 0;
    }

static void writeJsonString(FILE *file, const char *text)
    {
    const unsigned char *cursor;

    fputc('"', file);
    for (cursor = (const unsigned char *)text; *cursor; cursor++)
        {
        if (*cursor == '"' || *cursor == '\\')
            fputc('\\', file);
        if (*cursor == '\n')
            fputs("\\n", file);
        else if (*cursor == '\r')
            fputs("\\r", file);
        else if (*cursor == '\t')
            fputs("\\t", file);
        else if (*cursor >= 32)
            fputc(*cursor, file);
        }
    fputc('"', file);
    }

static const char *patrolModeName(int patrolMode)
    {
    if (patrolMode == ENTITY_PATROL_PINGPONG)
        return "pingpong";
    if (patrolMode == ENTITY_PATROL_ONCE)
        return "once";
    return "loop";
    }

static const char *alarmResponseName(int response)
    {
    if (response == ENTITY_ALARM_CONTINUE)
        return "continue";
    if (response == ENTITY_ALARM_ATTACK)
        return "attack";
    if (response == ENTITY_ALARM_RETREAT)
        return "retreat";
    if (response == ENTITY_ALARM_RANDOM)
        return "random";
    return "inherit";
    }

static void writeObjects(FILE *file, const struct BattalionMapPackage *map)
    {
    size_t objectIndex;
    size_t waypointIndex;
    size_t geometryIndex;
    size_t propertyIndex;
    size_t triggerIndex;
    const struct BattalionMapObject *object;
    const struct BattalionMapTrigger *trigger;

    fputs(",\n  \"objects\": [\n", file);
    for (objectIndex = 0; objectIndex < map->objectCount; objectIndex++)
        {
        object = &map->objects[objectIndex];
        fputs("    {", file);
        if (object->id[0] != '\0')
            {
            fputs("\"id\": ", file);
            writeJsonString(file, object->id);
            fputs(", ", file);
            }
        fputs("\"archetype\": ", file);
        writeJsonString(file, object->archetype);
        if (object->description[0] != '\0')
            {
            fputs(", \"description\": ", file);
            writeJsonString(file, object->description);
            }
        if (object->group[0] != '\0')
            {
            fputs(", \"group\": ", file);
            writeJsonString(file, object->group);
            }
        if (object->faction[0] != '\0')
            {
            fputs(", \"faction\": ", file);
            writeJsonString(file, object->faction);
            }
        if (object->patrolRoute[0] != '\0')
            {
            fputs(", \"patrol-route\": ", file);
            writeJsonString(file, object->patrolRoute);
            }
        if (object->alarmResponse != ENTITY_ALARM_INHERIT)
            {
            fputs(", \"alarm-response\": ", file);
            writeJsonString(file,
                            alarmResponseName(object->alarmResponse));
            if (object->safeZone[0] != '\0')
                {
                fputs(", \"safe-zone\": ", file);
                writeJsonString(file, object->safeZone);
                }
            fprintf(file, ", \"safe-wait-seconds\": %.9g",
                    object->safeWaitSeconds);
            }
        fprintf(file, ", \"x\": %.9g, \"y\": %.9g, \"z\": %.9g, "
                      "\"heading\": %.9g, \"scale\": %.9g, "
                      "\"active\": %s, \"patrol-mode\": ",
                object->x, object->y, object->z, object->heading,
                object->scale, object->active ? "true" : "false");
        writeJsonString(file, patrolModeName(object->patrolMode));
        fputs(",\n      \"path\": [", file);
        for (waypointIndex = 0; waypointIndex < object->waypointCount;
             waypointIndex++)
            {
            if (waypointIndex > 0)
                fputs(", ", file);
            fprintf(file, "{\"x\": %.9g, \"z\": %.9g}",
                    object->waypoints[waypointIndex].x,
                    object->waypoints[waypointIndex].z);
            }
        fputs("],\n      \"geometry\": [", file);
        for (geometryIndex = 0; geometryIndex < object->geometryCount;
             geometryIndex++)
            {
            if (geometryIndex > 0)
                fputs(", ", file);
            fprintf(file, "{\"x\": %.9g, \"z\": %.9g}",
                    object->geometry[geometryIndex].x,
                    object->geometry[geometryIndex].z);
            }
        fputs("],\n      \"properties\": [", file);
        for (propertyIndex = 0; propertyIndex < object->propertyCount;
             propertyIndex++)
            {
            if (propertyIndex > 0)
                fputs(", ", file);
            fputs("{\"name\": ", file);
            writeJsonString(file, object->properties[propertyIndex].name);
            fputs(", \"value\": ", file);
            writeJsonString(file, object->properties[propertyIndex].value);
            fputc('}', file);
            }
        fputs("],\n      \"triggers\": [", file);
        for (triggerIndex = 0; triggerIndex < object->triggerCount;
             triggerIndex++)
            {
            trigger = &object->triggers[triggerIndex];
            if (triggerIndex > 0)
                fputs(", ", file);
            fputs("{\"event\": ", file);
            writeJsonString(file, trigger->event);
            if (trigger->targetType == BATTALION_MAP_TRIGGER_ID)
                fputs(", \"target-id\": ", file);
            else
                fputs(", \"target-group\": ", file);
            writeJsonString(file, trigger->target);
            fputc('}', file);
            }
        fprintf(file, "]}%s\n",
                objectIndex + 1 < map->objectCount ? "," : "");
        }
    fputs("  ]", file);
    }

int battalionMapPackageWrite(const char *path,
                             const struct BattalionMapPackage *map,
                             char *error, size_t errorSize)
    {
    FILE *file;
    if (!battalionMapPackageValidate(map, error, errorSize))
        {
        return 0;
        }
    file = fopen(path, "wb");
    if (file == NULL)
        {
        setMapError(error, errorSize, "Could not create map package");
        return 0;
        }
    fputs("{\n  \"format\": \"battalion-map\",\n  \"version\": ", file);
    fprintf(file, "2,\n  \"id\": ");
    writeJsonString(file, map->id);
    fputs(",\n  \"name\": ", file);
    writeJsonString(file, map->name);
    writeObjects(file, map);
    fputs("\n}\n", file);
    if (fclose(file) != 0)
        {
        setMapError(error, errorSize, "Could not finish map package");
        return 0;
        }
    return 1;
    }
