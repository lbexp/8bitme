#include "file.h"
#include "png.h"
#include <stdio.h>

int main() {
    FILE *file;
    char fileName[64];

    printf("Please input file name: ");
    scanf("%s", (char *)&fileName);

    if (!validate_file_ext(fileName)) {
        printf("File should be in supported format\n");
        return 0;
    }

    file = fopen(fileName, "rb");

    if (!file) {
        printf("Can't find file named %s\n", fileName);
        return 0;
    }

    PNGDecoded *decoded = decode_data(&file);

    if (decoded == NULL) {
        fclose(file);
        return 0;
    }

    FILE *newFile;
    char *newFileName = "result.png";

    file = fopen(newFileName, "wb");

    if (!encode_data(&newFile, decoded)) {
        printf("Can't generate 8bit file\n");
        return 0;
    }

    printf("Success");

    fclose(file);
    fclose(newFile);

    return 0;
}
