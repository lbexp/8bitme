#include "file.h"
#include <string.h>

int validate_file_ext(char fileName[64]) {
    const char formats[1][8] = {
        ".png",
        // Add other extensions when supported
    };
    const char *dot = strchr(fileName, '.');

    if (!dot || fileName == dot) {
        return 0;
    }

    for (int i = 0; i < sizeof(formats); i++) {
        if (strcmp(dot, formats[i]) == 0) {
            return 1;
        }
    }

    return 0;
}
