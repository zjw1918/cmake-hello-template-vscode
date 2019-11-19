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

    // for (int i = 20 - 1; i >= 0; i--)
    // {
    //     const char *filepath =
    //         "/Users/mega/A-file/py-bins/1_5c7c6056ac502e00668fdad3_1551630599.bin";
    //     read_from_path(filepath);
    // }

    // const char *bin_dir = "/Volumes/ssd_hub/Z-dev/bins/new_old/";
    const char *bin_dir = "/Volumes/ssd_hub/Z-dev/bins/py-bins/";
    read_dir(bin_dir);
    


    return 0;
}
