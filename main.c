#include <stdint.h>
#include <stdio.h>
#include <string.h>

/***********************
 * Structs - Start
 ***********************/

typedef struct {
    uint16_t type;
    uint32_t fileSize;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} BMPHeader;

/***********************
 * Structs - End
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

const char formats[2][8] = {
    ".png",
    ".bmp",
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

void parse_file(FILE **file) {
    BMPHeader header;

    fread(&header, sizeof(header), 1, *file);
    printf("Header type: %d\n", header.type);
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

    parse_file(&file);

    return 0;
}
