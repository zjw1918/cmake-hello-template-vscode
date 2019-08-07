#include <stdlib.h>
#include <stdio.h>
#include "zjw.h"

#include "func/display.h"
#include "parse/parse.h"

int main(int, char **)
{
    printf("add: %d\n", add(1, 2));
    show();
    say_hello();

    FILE *pFile;
    long lSize;
    char *buffer;
    size_t result;

    pFile = fopen("/Users/mega/Downloads/parse_error_190806/1_5b4e9c600b61600031170ac1_1531870464.bin", "rb");
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
        exit(2);
    }

    result = fread(buffer, 1, lSize, pFile);
    if (result != lSize)
    {
        fputs("Reading error", stderr);
        exit(3);
    }

    printf("file length: %ld\n", lSize);

    fclose(pFile);
    free(buffer);

    printf("Done!\n");
    return 0;
}

// void xx_read()
// {
//     FILE *pFile;
//     long lSize;
//     char *buffer;
//     size_t result;

//     const char* path = "/Users/mega/Downloads/parse_error_190806/1_5b4e9c600b61600031170ac1_1531870464.bin";

//     pFile = fopen(path, "rb");
//     if (pFile == NULL)
//     {
//         fputs("File error", stderr);
//         exit(1);
//     }

//     fseek(pFile, 0, SEEK_END);
//     lSize = ftell(pFile);
//     rewind(pFile);

//     buffer = (char *)malloc(sizeof(char) * lSize);
//     if (buffer == NULL)
//     {
//         fputs("Memory error", stderr);
//         exit(2);
//     }

//     result = fread(buffer, 1, lSize, pFile);
//     if (result != lSize)
//     {
//         fputs("Reading error", stderr);
//         exit(3);
//     }

//     printf("%s", buffer);

//     fclose(pFile);
//     free(buffer);
// }