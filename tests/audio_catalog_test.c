#include "audio_catalog.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define TEST_PATH_SIZE 512

static int catalogContainsPath(const struct AudioCatalog *catalog,
                               const char *path)
    {
    size_t index;

    for (index = 0; index < catalog->count; index++)
        if (strcmp(catalog->entries[index].path, path) == 0)
            return 1;
    return 0;
    }

static int verifyAudioDirectory(const struct AudioCatalog *catalog,
                                const char *directoryName)
    {
    struct dirent *entry;
    struct stat status;
    DIR *directory;
    char fullPath[TEST_PATH_SIZE];
    char relativePath[TEST_PATH_SIZE];

    directory = opendir(directoryName);
    if (directory == NULL)
        return 0;
    while ((entry = readdir(directory)) != NULL)
        {
        if (entry->d_name[0] == '.')
            continue;
        if (strlen(directoryName) + strlen(entry->d_name) + 2 >
            sizeof(fullPath))
            {
            closedir(directory);
            return 0;
            }
        strcpy(fullPath, directoryName);
        strcat(fullPath, "/");
        strcat(fullPath, entry->d_name);
        if (stat(fullPath, &status) != 0 || !S_ISREG(status.st_mode))
            {
            closedir(directory);
            return 0;
            }
        strcpy(relativePath, fullPath + strlen("assets/"));
        if (!catalogContainsPath(catalog, relativePath))
            {
            fprintf(stderr, "orphan audio asset: %s\n", fullPath);
            closedir(directory);
            return 0;
            }
        }
    closedir(directory);
    return 1;
    }

int main(void)
    {
    struct AudioCatalog catalog;
    const struct AudioCatalogEntry *entry;
    char error[160];

    audioCatalogInitialize(&catalog);
    if (!audioCatalogLoad("assets/", "assets/audio/catalog.txt", &catalog,
                          error, sizeof(error)))
        {
        fprintf(stderr, "audio catalog failed to load: %s\n", error);
        return 1;
        }
    entry = audioCatalogFind(&catalog, "beam", "pcm");
    if (catalog.count != 20 || entry == NULL || entry->slot != 0 ||
        strcmp(entry->path, "sounds/beam.wav") != 0 ||
        audioCatalogFind(&catalog, "music-5", "pcm") == NULL ||
        audioCatalogFind(&catalog, "monster-beam", "pcm") == NULL ||
        audioCatalogFind(&catalog, "teleporter", "pcm") == NULL ||
        !verifyAudioDirectory(&catalog, "assets/sounds") ||
        !verifyAudioDirectory(&catalog, "assets/music"))
        {
        fprintf(stderr, "unexpected external audio catalog contents\n");
        audioCatalogDestroy(&catalog);
        return 1;
        }
    audioCatalogDestroy(&catalog);
    return 0;
    }
