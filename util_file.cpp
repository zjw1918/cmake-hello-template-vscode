#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#include "util_file.h"
#include "mega.h"

void read_from_path(const char *path)
{
    FILE *pFile;
    long lSize;
    char *buffer;
    size_t result;

    pFile = fopen(path, "rb");
    if (pFile == NULL)
    {
        fputs("File error", stderr);
        exit(1);
    }

    fseek(pFile, 0, SEEK_END);
    lSize = ftell(pFile);
    rewind(pFile);

    buffer = (char *)malloc(sizeof(char) * lSize);
    if (buffer == NULL)
    {
        fputs("Memory error", stderr);
        fclose(pFile);
        exit(2);
    }

    result = fread(buffer, 1, lSize, pFile);
    if (result != lSize)
    {
        fputs("Reading error", stderr);
        free(buffer);
        fclose(pFile);
        exit(3);
    }
    printf("file length: %ld\n", lSize);

    // handle parse
    parse(buffer, lSize);

    fclose(pFile);
    free(buffer);

    printf("------------------------------Done!\n");
}

void read_dir(const char *basepath)
{
    struct dirent *de; // Pointer for directory entry
    char base[1024];

    // opendir() returns a pointer of DIR type.
    DIR *dr = opendir(basepath);

    if (dr == NULL) // opendir returns NULL if couldn't open directory
    {
        printf("Could not open current directory");
        return;
    }

    int cnt = 0;
    while ((de = readdir(dr)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
        // printf("%s\n", de->d_name);
        memset(base, '\0', sizeof(base));
        strcpy(base, basepath);
        // strcat(base, "/");
        strcat(base, de->d_name);
        printf("%s\n", base);

        // parse
        read_from_path(base);
        printf("cnt: %d\n", cnt++);
    }

    closedir(dr);
}