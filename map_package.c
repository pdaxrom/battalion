/****************************************************************/
/* code copyright 1995-1996 Andrew Johnson - ALL RIGHTS RESERVED*/
/*                          ajohnson@eecs.uic.edu               */
/*                                                              */
/*                      Electronic Visualization Lab (M/C 154)  */
/*                      University of Illinois at Chicago       */
/*                      851 South Morgan St. Room 1120 SEO      */
/*                      Chicago, IL 60607-7053                  */
/*                                                              */
/*                      (312) 996-3002     (312) 413-7585 FAX   */
/***********************************************************************/
/* Versioned JSON battlefield package loader.                          */
/***********************************************************************/

#include "map_package.h"

#include <ctype.h>
#include <errno.h>
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

    if (object < 0 || object >= tokenCount ||
        tokens[object].type != MAP_JSON_OBJECT)
        return -1;
    for (index = object + 1; index + 1 < tokenCount; index++)
        {
        if (tokens[index].start >= tokens[object].end)
            break;
        if (tokens[index].parent == object && tokenEquals(json, &tokens[index], key))
            {
            if (tokens[index + 1].parent == object)
                return index + 1;
            return -1;
            }
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

static int loadRoads(const char *json, const struct MapJsonToken *tokens,
                     int tokenCount, int array, struct BattalionMapPackage *map)
    {
    size_t output;
    int index;
    int xToken;
    int zToken;
    int typeToken;

    if (array < 0 || tokens[array].type != MAP_JSON_ARRAY)
        return 0;
    map->roadCount = arrayCount(tokens, tokenCount, array);
    if (map->roadCount > 0)
        map->roads = (struct BattalionMapRoad *)calloc(
            map->roadCount, sizeof(struct BattalionMapRoad));
    if (map->roadCount > 0 && map->roads == NULL)
        return 0;
    output = 0;
    for (index = array + 1; index < tokenCount && output < map->roadCount; index++)
        if (tokens[index].parent == array)
            {
            if (tokens[index].type != MAP_JSON_OBJECT)
                return 0;
            xToken = objectValue(json, tokens, tokenCount, index, "x");
            zToken = objectValue(json, tokens, tokenCount, index, "z");
            typeToken = objectValue(json, tokens, tokenCount, index, "type");
            if (xToken < 0 || zToken < 0 || typeToken < 0 ||
                !tokenFloat(json, &tokens[xToken], &map->roads[output].x) ||
                !tokenFloat(json, &tokens[zToken], &map->roads[output].z) ||
                !tokenInt(json, &tokens[typeToken], &map->roads[output].type))
                return 0;
            output++;
            }
    return output == map->roadCount;
    }

static int loadStructures(const char *json, const struct MapJsonToken *tokens,
                          int tokenCount, int array,
                          struct BattalionMapPackage *map)
    {
    size_t output;
    int index;
    int xToken;
    int zToken;
    int typeToken;
    int shapeToken;

    if (array < 0 || tokens[array].type != MAP_JSON_ARRAY)
        return 0;
    map->structureCount = arrayCount(tokens, tokenCount, array);
    if (map->structureCount > 0)
        map->structures = (struct BattalionMapStructure *)calloc(
            map->structureCount, sizeof(struct BattalionMapStructure));
    if (map->structureCount > 0 && map->structures == NULL)
        return 0;
    output = 0;
    for (index = array + 1; index < tokenCount && output < map->structureCount; index++)
        if (tokens[index].parent == array)
            {
            if (tokens[index].type != MAP_JSON_OBJECT)
                return 0;
            xToken = objectValue(json, tokens, tokenCount, index, "x");
            zToken = objectValue(json, tokens, tokenCount, index, "z");
            typeToken = objectValue(json, tokens, tokenCount, index, "type");
            shapeToken = objectValue(json, tokens, tokenCount, index, "shape");
            if (xToken < 0 || zToken < 0 || typeToken < 0 || shapeToken < 0 ||
                !tokenFloat(json, &tokens[xToken], &map->structures[output].x) ||
                !tokenFloat(json, &tokens[zToken], &map->structures[output].z) ||
                !tokenInt(json, &tokens[typeToken], &map->structures[output].type) ||
                !tokenInt(json, &tokens[shapeToken], &map->structures[output].shape))
                return 0;
            output++;
            }
    return output == map->structureCount;
    }

static int loadUnits(const char *json, const struct MapJsonToken *tokens,
                     int tokenCount, int array, struct BattalionMapPackage *map)
    {
    size_t output;
    int index;
    int xToken;
    int zToken;
    int typeToken;

    if (array < 0 || tokens[array].type != MAP_JSON_ARRAY)
        return 0;
    map->unitCount = arrayCount(tokens, tokenCount, array);
    if (map->unitCount > 0)
        map->units = (struct BattalionMapUnit *)calloc(
            map->unitCount, sizeof(struct BattalionMapUnit));
    if (map->unitCount > 0 && map->units == NULL)
        return 0;
    output = 0;
    for (index = array + 1; index < tokenCount && output < map->unitCount; index++)
        if (tokens[index].parent == array)
            {
            if (tokens[index].type != MAP_JSON_OBJECT)
                return 0;
            xToken = objectValue(json, tokens, tokenCount, index, "x");
            zToken = objectValue(json, tokens, tokenCount, index, "z");
            typeToken = objectValue(json, tokens, tokenCount, index, "type");
            if (xToken < 0 || zToken < 0 || typeToken < 0 ||
                !tokenFloat(json, &tokens[xToken], &map->units[output].x) ||
                !tokenFloat(json, &tokens[zToken], &map->units[output].z) ||
                !tokenInt(json, &tokens[typeToken], &map->units[output].type))
                return 0;
            output++;
            }
    return output == map->unitCount;
    }

static int loadActorPath(const char *json,
                         const struct MapJsonToken *tokens,
                         int tokenCount, int array,
                         struct BattalionMapActor *actor)
    {
    size_t output;
    int index;
    int xToken;
    int zToken;

    if (array < 0)
        return 1;
    if (tokens[array].type != MAP_JSON_ARRAY)
        return 0;
    actor->waypointCount = arrayCount(tokens, tokenCount, array);
    if (actor->waypointCount > 0)
        actor->waypoints = (struct EntityWaypoint *)calloc(
            actor->waypointCount, sizeof(struct EntityWaypoint));
    if (actor->waypointCount > 0 && actor->waypoints == NULL)
        return 0;
    output = 0;
    for (index = array + 1;
         index < tokenCount && output < actor->waypointCount; index++)
        if (tokens[index].parent == array)
            {
            if (tokens[index].type != MAP_JSON_OBJECT)
                return 0;
            xToken = objectValue(json, tokens, tokenCount, index, "x");
            zToken = objectValue(json, tokens, tokenCount, index, "z");
            if (xToken < 0 || zToken < 0 ||
                !tokenFloat(json, &tokens[xToken],
                            &actor->waypoints[output].x) ||
                !tokenFloat(json, &tokens[zToken],
                            &actor->waypoints[output].z))
                return 0;
            output++;
            }
    return output == actor->waypointCount;
    }

static int loadActorTriggers(const char *json,
                             const struct MapJsonToken *tokens,
                             int tokenCount, int array,
                             struct BattalionMapActor *actor)
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
    actor->triggerCount = arrayCount(tokens, tokenCount, array);
    if (actor->triggerCount > 0)
        actor->triggers = (struct BattalionMapTrigger *)calloc(
            actor->triggerCount, sizeof(struct BattalionMapTrigger));
    if (actor->triggerCount > 0 && actor->triggers == NULL)
        return 0;
    output = 0;
    for (index = array + 1;
         index < tokenCount && output < actor->triggerCount; index++)
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
            trigger = &actor->triggers[output];
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
    return output == actor->triggerCount;
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

static int loadActors(const char *json, const struct MapJsonToken *tokens,
                      int tokenCount, int array,
                      struct BattalionMapPackage *map)
    {
    size_t output;
    int index;
    int idToken;
    int archetypeToken;
    int groupToken;
    int xToken;
    int zToken;
    int headingToken;
    int activeToken;
    int patrolModeToken;
    int pathToken;
    int triggersToken;
    struct BattalionMapActor *actor;

    if (array < 0 || tokens[array].type != MAP_JSON_ARRAY)
        return 0;
    map->actorCount = arrayCount(tokens, tokenCount, array);
    if (map->actorCount > 0)
        map->actors = (struct BattalionMapActor *)calloc(
            map->actorCount, sizeof(struct BattalionMapActor));
    if (map->actorCount > 0 && map->actors == NULL)
        return 0;
    output = 0;
    for (index = array + 1;
         index < tokenCount && output < map->actorCount; index++)
        if (tokens[index].parent == array)
            {
            if (tokens[index].type != MAP_JSON_OBJECT)
                return 0;
            actor = &map->actors[output];
            actor->active = 1;
            actor->patrolMode = ENTITY_PATROL_LOOP;
            idToken = objectValue(json, tokens, tokenCount, index, "id");
            archetypeToken = objectValue(json, tokens, tokenCount, index,
                                         "archetype");
            groupToken = objectValue(json, tokens, tokenCount, index,
                                     "group");
            xToken = objectValue(json, tokens, tokenCount, index, "x");
            zToken = objectValue(json, tokens, tokenCount, index, "z");
            headingToken = objectValue(json, tokens, tokenCount, index,
                                       "heading");
            activeToken = objectValue(json, tokens, tokenCount, index,
                                      "active");
            patrolModeToken = objectValue(json, tokens, tokenCount, index,
                                          "patrol-mode");
            pathToken = objectValue(json, tokens, tokenCount, index, "path");
            triggersToken = objectValue(json, tokens, tokenCount, index,
                                        "triggers");
            if (idToken < 0 || archetypeToken < 0 || xToken < 0 ||
                zToken < 0 ||
                !copyString(json, &tokens[idToken], actor->id,
                            sizeof(actor->id)) ||
                !copyString(json, &tokens[archetypeToken], actor->archetype,
                            sizeof(actor->archetype)) ||
                !validEntityName(actor->id) ||
                !validEntityName(actor->archetype) ||
                !tokenFloat(json, &tokens[xToken], &actor->x) ||
                !tokenFloat(json, &tokens[zToken], &actor->z))
                return 0;
            if (groupToken >= 0 &&
                (!copyString(json, &tokens[groupToken], actor->group,
                             sizeof(actor->group)) ||
                 !validEntityName(actor->group)))
                return 0;
            if (headingToken >= 0 &&
                !tokenFloat(json, &tokens[headingToken], &actor->heading))
                return 0;
            if (activeToken >= 0 &&
                !tokenBool(json, &tokens[activeToken], &actor->active))
                return 0;
            if (patrolModeToken >= 0)
                {
                actor->patrolMode = patrolModeValue(
                    json, &tokens[patrolModeToken]);
                if (actor->patrolMode == 0)
                    return 0;
                }
            if (!loadActorPath(json, tokens, tokenCount, pathToken, actor) ||
                !loadActorTriggers(json, tokens, tokenCount, triggersToken,
                                   actor))
                return 0;
            output++;
            }
    return output == map->actorCount;
    }

static int actorIdExists(const struct BattalionMapPackage *map,
                         const char *id)
    {
    size_t index;

    for (index = 0; index < map->actorCount; index++)
        if (strcmp(map->actors[index].id, id) == 0)
            return 1;
    return 0;
    }

static int actorGroupExists(const struct BattalionMapPackage *map,
                            const char *group)
    {
    size_t index;

    for (index = 0; index < map->actorCount; index++)
        if (strcmp(map->actors[index].group, group) == 0)
            return 1;
    return 0;
    }

static int validateActors(const struct BattalionMapPackage *map)
    {
    size_t actorIndex;
    size_t otherIndex;
    size_t triggerIndex;
    const struct BattalionMapTrigger *trigger;

    for (actorIndex = 0; actorIndex < map->actorCount; actorIndex++)
        {
        for (otherIndex = actorIndex + 1; otherIndex < map->actorCount;
             otherIndex++)
            if (strcmp(map->actors[actorIndex].id,
                       map->actors[otherIndex].id) == 0)
                return 0;
        for (triggerIndex = 0;
             triggerIndex < map->actors[actorIndex].triggerCount;
             triggerIndex++)
            {
            trigger = &map->actors[actorIndex].triggers[triggerIndex];
            if (trigger->targetType == BATTALION_MAP_TRIGGER_ID)
                {
                if (!actorIdExists(map, trigger->target))
                    return 0;
                }
            else if (trigger->targetType == BATTALION_MAP_TRIGGER_GROUP)
                {
                if (!actorGroupExists(map, trigger->target))
                    return 0;
                }
            else
                return 0;
            }
        }
    return 1;
    }

void battalionMapPackageInit(struct BattalionMapPackage *map)
    {
    if (map != NULL)
        {
        memset(map, 0, sizeof(*map));
        map->version = 1;
        }
    }

void battalionMapPackageFree(struct BattalionMapPackage *map)
    {
    size_t index;

    if (map == NULL)
        return;
    free(map->roads);
    free(map->structures);
    free(map->units);
    for (index = 0; index < map->actorCount; index++)
        {
        free(map->actors[index].waypoints);
        free(map->actors[index].triggers);
        }
    free(map->actors);
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
    int roadsToken;
    int structuresToken;
    int unitsToken;
    int actorsToken;
    int version;
    struct BattalionMapPackage loaded;

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
    if (formatToken < 0 || versionToken < 0 || idToken < 0 || nameToken < 0 ||
        !tokenEquals(json, &tokens[formatToken], "battalion-map") ||
        !tokenInt(json, &tokens[versionToken], &version) ||
        (version != 1 && version != 2) ||
        !copyString(json, &tokens[idToken], loaded.id, sizeof(loaded.id)) ||
        !copyString(json, &tokens[nameToken], loaded.name, sizeof(loaded.name)) ||
        !loadRoads(json, tokens, tokenCount, roadsToken, &loaded) ||
        !loadStructures(json, tokens, tokenCount, structuresToken, &loaded) ||
        !loadUnits(json, tokens, tokenCount, unitsToken, &loaded) ||
        (version == 2 && !loadActors(json, tokens, tokenCount, actorsToken,
                                    &loaded)))
        goto invalid;
    loaded.version = version;
    if (!validateActors(&loaded))
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
    setMapError(error, errorSize, "Invalid or unsupported battalion-map JSON");
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

static void writeActors(FILE *file, const struct BattalionMapPackage *map)
    {
    size_t actorIndex;
    size_t waypointIndex;
    size_t triggerIndex;
    const struct BattalionMapActor *actor;
    const struct BattalionMapTrigger *trigger;

    fputs(",\n  \"actors\": [\n", file);
    for (actorIndex = 0; actorIndex < map->actorCount; actorIndex++)
        {
        actor = &map->actors[actorIndex];
        fputs("    {\"id\": ", file);
        writeJsonString(file, actor->id);
        fputs(", \"archetype\": ", file);
        writeJsonString(file, actor->archetype);
        if (actor->group[0] != '\0')
            {
            fputs(", \"group\": ", file);
            writeJsonString(file, actor->group);
            }
        fprintf(file, ", \"x\": %.9g, \"z\": %.9g, \"heading\": %.9g, "
                      "\"active\": %s, \"patrol-mode\": ",
                actor->x, actor->z, actor->heading,
                actor->active ? "true" : "false");
        writeJsonString(file, patrolModeName(actor->patrolMode));
        fputs(",\n      \"path\": [", file);
        for (waypointIndex = 0; waypointIndex < actor->waypointCount;
             waypointIndex++)
            {
            if (waypointIndex > 0)
                fputs(", ", file);
            fprintf(file, "{\"x\": %.9g, \"z\": %.9g}",
                    actor->waypoints[waypointIndex].x,
                    actor->waypoints[waypointIndex].z);
            }
        fputs("],\n      \"triggers\": [", file);
        for (triggerIndex = 0; triggerIndex < actor->triggerCount;
             triggerIndex++)
            {
            trigger = &actor->triggers[triggerIndex];
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
                actorIndex + 1 < map->actorCount ? "," : "");
        }
    fputs("  ]", file);
    }

int battalionMapPackageWrite(const char *path,
                             const struct BattalionMapPackage *map,
                             char *error, size_t errorSize)
    {
    FILE *file;
    size_t index;
    int version;

    if (map == NULL || !validateActors(map))
        {
        setMapError(error, errorSize, "Invalid map actors or trigger links");
        return 0;
        }
    version = map->version == 2 || map->actorCount > 0 ? 2 : 1;

    file = fopen(path, "wb");
    if (file == NULL)
        {
        setMapError(error, errorSize, "Could not create map package");
        return 0;
        }
    fputs("{\n  \"format\": \"battalion-map\",\n  \"version\": ", file);
    fprintf(file, "%d,\n  \"id\": ", version);
    writeJsonString(file, map->id);
    fputs(",\n  \"name\": ", file);
    writeJsonString(file, map->name);
    fputs(",\n  \"roads\": [\n", file);
    for (index = 0; index < map->roadCount; index++)
        fprintf(file, "    {\"x\": %.9g, \"z\": %.9g, \"type\": %d}%s\n",
                map->roads[index].x, map->roads[index].z,
                map->roads[index].type,
                index + 1 < map->roadCount ? "," : "");
    fputs("  ],\n  \"structures\": [\n", file);
    for (index = 0; index < map->structureCount; index++)
        fprintf(file, "    {\"x\": %.9g, \"z\": %.9g, \"type\": %d, \"shape\": %d}%s\n",
                map->structures[index].x, map->structures[index].z,
                map->structures[index].type, map->structures[index].shape,
                index + 1 < map->structureCount ? "," : "");
    fputs("  ],\n  \"units\": [\n", file);
    for (index = 0; index < map->unitCount; index++)
        fprintf(file, "    {\"x\": %.9g, \"z\": %.9g, \"type\": %d}%s\n",
                map->units[index].x, map->units[index].z,
                map->units[index].type,
                index + 1 < map->unitCount ? "," : "");
    fputs("  ]", file);
    if (version == 2)
        writeActors(file, map);
    fputs("\n}\n", file);
    if (fclose(file) != 0)
        {
        setMapError(error, errorSize, "Could not finish map package");
        return 0;
        }
    return 1;
    }
