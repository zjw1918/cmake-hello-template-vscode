#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// #include "zjw.h"
// #include "func/display.h"
// #include "parse/parse.h"
#include "util_file.h"


int main(int, char **)
{
    // printf("add: %d\n", add(1, 2));
    // show();
    // say_hello();

    // const char *filepath = 
    // "/Users/mega/Downloads/parse_error_190806/1_5b5106ce9f5454003cd60cbc_1532020525.bin";
    // read_from_path(filepath);

    read_dir("/Users/mega/Downloads/parse_error_190806/");

    return 0;
}

