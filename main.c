#include <stdio.h>
#include <string.h>

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

/*
 * validate_file_ext
 */
int validate_file_ext(char fileName[64]) {
    const char *dot = strchr(fileName, '.');

    if (!dot || fileName == dot) {
        return 0;
    }

    return strcmp(dot, ".png") == 0;
}

/*
 * read_file
 */
void read_file(FILE **file, char name[64]) {
    *file = fopen(name, "r");
}

void parse_file(FILE **file) {
    char data[100];

    fgets(data, sizeof(data), *file);

    printf("Data contains: %s\n", data);
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
        printf("File should be in .png");
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
