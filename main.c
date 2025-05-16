#include "file.h"
#include "png.h"
#include <stdio.h>

int main() {
    FILE *file;
    char fileName[64];

    printf("Please input file name: ");
    scanf("%s", (char *)&fileName);

    if (!validate_file_ext(fileName)) {
        printf("File should be in supported format");
        return 0;
    }

    file = fopen(fileName, "rb");

    if (!file) {
        printf("Can't find file named %s", fileName);
        return 0;
    }

    int isParsed = parse_data(&file);

    if (!isParsed) {
        fclose(file);
        return 0;
    }

    fclose(file);

    return 0;
}
