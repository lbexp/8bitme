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

    PNGDecoded *decoded = decode_data(&file);

    if (decoded == NULL) {
        fclose(file);
        return 0;
    }

    FILE *newFile;
    char *newFileName = "result.png";

    file = fopen(newFileName, "w");

    if (!encode_data(&newFile, decoded)) {
        printf("Can't generate 8bit file");
        return 0;
    }

    printf("Success");

    fclose(file);

    return 0;
}
