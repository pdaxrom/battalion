#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static int fileContains(const char *path, const char *pattern)
    {
    char line[1024];
    FILE *file;

    file = fopen(path, "r");
    if (file == NULL)
        {
        fprintf(stderr, "could not audit source file: %s\n", path);
        return -1;
        }
    while (fgets(line, sizeof(line), file) != NULL)
        if (strstr(line, pattern) != NULL)
            {
            fclose(file);
            return 1;
            }
    fclose(file);
    return 0;
    }

static int auditSources(void)
    {
    static const char *sources[] = {
        "battalion.c", "graphics.c", "text.c", "update.c", "camera.c",
        "audio.c"
    };
    static const char *forbidden[] = {
        "glBegin(", "glVertex", "\"models/", "\"textures/",
        "\"sounds/", "\"music/", "strokeFont[", "battalion.sho",
        "requiredModel(\"", "requiredModelEntry(\"",
        "requiredTexture(\"", "system("
    };
    size_t source;
    size_t pattern;
    int found;

    for (source = 0; source < sizeof(sources) / sizeof(sources[0]); source++)
        for (pattern = 0;
             pattern < sizeof(forbidden) / sizeof(forbidden[0]); pattern++)
            {
            found = fileContains(sources[source], forbidden[pattern]);
            if (found != 0)
                {
                if (found > 0)
                    fprintf(stderr, "embedded asset pattern '%s' in %s\n",
                            forbidden[pattern], sources[source]);
                return 0;
                }
            }
    return 1;
    }

static int retiredFilesAreAbsent(void)
    {
    static const char *retired[] = {
        "objects.c", "gprim.c", "tk.c", "battalion.data"
    };
    struct stat status;
    size_t index;

    for (index = 0; index < sizeof(retired) / sizeof(retired[0]); index++)
        if (stat(retired[index], &status) == 0)
            {
            fprintf(stderr, "retired embedded asset source returned: %s\n",
                    retired[index]);
            return 0;
            }
    return 1;
    }

int main(void)
    {
    return auditSources() && retiredFilesAreAbsent() ? 0 : 1;
    }
