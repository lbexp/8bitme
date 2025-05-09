#include <stdint.h>
#include <stdio.h>
#include <string.h>

/***********************
 * STRUCTS - Start
 ***********************/

#pragma pack(push, 1)

// Placeholder

#pragma pack(pop)

/***********************
 * STRUCTS - End
 ***********************/

/***********************
 * CONSTANTS - Start
 ***********************/

const uint8_t PNG_SIGNATURE[8] = {137, 80, 78, 71, 13, 10, 26, 10};

/***********************
 * CONSTANTS - End
 ***********************/

/***********************
 * IMAGE - Start
 ***********************/

/*
 * read_big_endian
 */
uint32_t read_big_endian(FILE *file) {
    uint8_t b[4];
    fread(b, 1, 4, file);

    return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

/*
 * validate_signature
 */
int validate_signature(FILE *file) {
    uint8_t *signature[8];

    fread(signature, 1, 8, file);

    if (memcmp(signature, PNG_SIGNATURE, 8)) {
        printf("Wrong PNG signature!\n");

        return 0;
    }

    return 1;
}

/*
 * get_chunks
 */
void get_chunks(FILE *file) {
    while (!feof(file)) {
        uint32_t length = read_big_endian(file);
        char type[5] = {0};

        fread(type, 1, 4, file);

        printf("Chunk: %s, Length: %d\n", type, length);

        fseek(file, length + 4, SEEK_CUR);
        if (strcmp(type, "IEND") == 0) {
            break;
        }
    }
}

/*
 * parse_data
 * PNG binary data structure:
 * - 8-byte signature
 * - Chunks data:
 *   - 4-byte length
 *   - 4-byte chunk type
 *   - N-byte data
 *   - 4-byte CRC
 */
int parse_data(FILE **file) {
    if (!validate_signature(*file)) {
        return 0;
    }

    get_chunks(*file);

    return 1;
}

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
