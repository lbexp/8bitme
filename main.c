#include <stdint.h>
#include <stdio.h>
#include <string.h>

/***********************
 * Structs - Start
 ***********************/

#pragma pack(push, 1)

// Placeholder

#pragma pack(pop)

/***********************
 * Structs - End
 ***********************/

/***********************
 * Constants - Start
 ***********************/

const uint8_t PNG_SIGNATURE[8] = {137, 80, 78, 71, 13, 10, 26, 10};

/***********************
 * Constants - End
 ***********************/

/***********************
 * IMAGE - Start
 ***********************/

/*
 * create_img_data
 */
void create_img_data() {}

/***********************
 * IMAGE - End
 ***********************/

/***********************
 * FILE - Start
 ***********************/

const char formats[1][8] = {
    ".png",
};

/*
 * validate_file_ext
 */
int validate_file_ext(char fileName[64]) {
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

/*
 * read_file
 */
void read_file(FILE **file, char name[64]) {
    *file = fopen(name, "rb");
}

int parse_file(FILE **file) {
    uint8_t signature[8];

    fread(&signature, 8, 1, *file);

    if (memcmp(PNG_SIGNATURE, signature, 8)) {
        printf("Wrong PNG signature!\n");
        return 0;
    }

    return 1;
}

/***********************
 * FILE - End
 ***********************/

int main() {
    FILE *file;
    char fileName[64];

    printf("Please input file name: ");
    scanf("%s", (char *)&fileName);

    if (!validate_file_ext(fileName)) {
        printf("File should be in supported format");
        return 0;
    }

    read_file(&file, fileName);

    if (!file) {
        printf("Can't find file named %s", fileName);
        return 0;
    }

    int isParsed = parse_file(&file);

    if (!isParsed) {
        return 0;
    }

    return 0;
}
