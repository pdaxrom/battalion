/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Verifies that authored runtime assets and manifest entries agree.    */
/***********************************************************************/

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_CATALOG_PATH 1024

struct CatalogEntry
    {
    char *key;
    char *path;
    };

struct Catalog
    {
    struct CatalogEntry *entries;
    size_t count;
    };

static char *copyString(const char *text)
    {
    char *copy;
    size_t length;

    length = strlen(text);
    copy = (char *)malloc(length + 1);
    if (copy != NULL)
        memcpy(copy, text, length + 1);
    return copy;
    }

static int lowercasePath(const char *path)
    {
    const unsigned char *cursor;

    for (cursor = (const unsigned char *)path; *cursor; cursor++)
        if (isupper(*cursor) || *cursor == '\\' || *cursor == ' ')
            return 0;
    return strstr(path, "..") == NULL && path[0] != '/';
    }

static int addEntry(struct Catalog *catalog, const char *key,
                    const char *path)
    {
    struct CatalogEntry *expanded;
    size_t index;

    if (!lowercasePath(key) || !lowercasePath(path))
        {
        fprintf(stderr, "catalog entry is not lowercase: %s %s\n", key, path);
        return 0;
        }
    for (index = 0; index < catalog->count; index++)
        if (strcmp(catalog->entries[index].key, key) == 0 ||
            strcmp(catalog->entries[index].path, path) == 0)
            {
            fprintf(stderr, "duplicate catalog key or path: %s %s\n",
                    key, path);
            return 0;
            }
    expanded = (struct CatalogEntry *)realloc(
        catalog->entries, (catalog->count + 1) * sizeof(*expanded));
    if (expanded == NULL)
        return 0;
    catalog->entries = expanded;
    catalog->entries[catalog->count].key = copyString(key);
    catalog->entries[catalog->count].path = copyString(path);
    if (catalog->entries[catalog->count].key == NULL ||
        catalog->entries[catalog->count].path == NULL)
        return 0;
    catalog->count++;
    return 1;
    }

static int loadCatalog(struct Catalog *catalog)
    {
    char line[2048];
    char *directive;
    char *key;
    char *path;
    FILE *file;
    int header;

    file = fopen("assets/manifest.txt", "r");
    if (file == NULL)
        return 0;
    header = 0;
    while (fgets(line, sizeof(line), file) != NULL)
        {
        directive = strtok(line, " \t\r\n");
        if (directive == NULL || directive[0] == '#')
            continue;
        if (!header)
            {
            key = strtok(NULL, " \t\r\n");
            if (strcmp(directive, "battalion-assets") != 0 || key == NULL ||
                strcmp(key, "2") != 0)
                {
                fclose(file);
                return 0;
                }
            header = 1;
            continue;
            }
        if (strcmp(directive, "model") != 0 &&
            strcmp(directive, "texture") != 0 &&
            strcmp(directive, "file") != 0)
            continue;
        key = strtok(NULL, " \t\r\n");
        path = strtok(NULL, " \t\r\n");
        if (key == NULL || path == NULL || !addEntry(catalog, key, path))
            {
            fclose(file);
            return 0;
            }
        }
    fclose(file);
    return header;
    }

static int catalogContains(const struct Catalog *catalog, const char *path)
    {
    size_t index;

    for (index = 0; index < catalog->count; index++)
        if (strcmp(catalog->entries[index].path, path) == 0)
            return 1;
    return 0;
    }

static int verifyFile(const struct Catalog *catalog, const char *path)
    {
    struct stat status;

    if (!lowercasePath(path) || stat(path, &status) != 0 ||
        !S_ISREG(status.st_mode))
        {
        fprintf(stderr, "missing or invalid asset file: %s\n", path);
        return 0;
        }
    if (!catalogContains(catalog, path + strlen("assets/")))
        {
        fprintf(stderr, "orphan asset file: %s\n", path);
        return 0;
        }
    return 1;
    }

static int scanDirectory(const struct Catalog *catalog, const char *path,
                         const char *suffix)
    {
    struct dirent *entry;
    struct stat status;
    DIR *directory;
    char child[MAX_CATALOG_PATH];
    size_t length;

    directory = opendir(path);
    if (directory == NULL)
        return 0;
    while ((entry = readdir(directory)) != NULL)
        {
        if (entry->d_name[0] == '.')
            continue;
        if (strlen(path) + strlen(entry->d_name) + 2 > sizeof(child))
            {
            closedir(directory);
            return 0;
            }
        strcpy(child, path);
        strcat(child, "/");
        strcat(child, entry->d_name);
        if (stat(child, &status) != 0)
            {
            closedir(directory);
            return 0;
            }
        if (S_ISDIR(status.st_mode))
            {
            if (!scanDirectory(catalog, child, suffix))
                {
                closedir(directory);
                return 0;
                }
            continue;
            }
        length = strlen(child);
        if (length < strlen(suffix) ||
            strcmp(child + length - strlen(suffix), suffix) != 0)
            {
            fprintf(stderr, "unexpected file in authored asset directory: %s\n",
                    child);
            closedir(directory);
            return 0;
            }
        if (!verifyFile(catalog, child))
            {
            closedir(directory);
            return 0;
            }
        }
    closedir(directory);
    return 1;
    }

static int verifyDeclaredFiles(const struct Catalog *catalog)
    {
    char path[MAX_CATALOG_PATH];
    struct stat status;
    size_t index;

    for (index = 0; index < catalog->count; index++)
        {
        if (strlen(catalog->entries[index].path) + strlen("assets/") + 1 >
            sizeof(path))
            return 0;
        strcpy(path, "assets/");
        strcat(path, catalog->entries[index].path);
        if (stat(path, &status) != 0 || !S_ISREG(status.st_mode))
            {
            fprintf(stderr, "manifest references missing file: %s\n", path);
            return 0;
            }
        }
    return 1;
    }

static void destroyCatalog(struct Catalog *catalog)
    {
    size_t index;

    for (index = 0; index < catalog->count; index++)
        {
        free(catalog->entries[index].key);
        free(catalog->entries[index].path);
        }
    free(catalog->entries);
    }

int main(void)
    {
    struct Catalog catalog;
    int result;

    catalog.entries = NULL;
    catalog.count = 0;
    result = loadCatalog(&catalog) && verifyDeclaredFiles(&catalog) &&
             scanDirectory(&catalog, "assets/models", ".glb") &&
             scanDirectory(&catalog, "assets/textures", ".tga");
    destroyCatalog(&catalog);
    return result ? 0 : 1;
    }
