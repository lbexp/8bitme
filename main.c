#include <stdio.h>

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
void validate_file_ext() {}

/*
 * read_file
 */
FILE *read_file(char name[64]) {
    FILE *file;
    file = fopen(name, "r");

    return file;
}

void parse_file() {}

/***********************
 * FILE - End
 ***********************/

int main() {
    FILE *file;
    char fileName[64];

    printf("Please input file name: ");
    scanf("%s", (char *)&fileName);

    file = read_file(fileName);

    if (file) {
        printf("File found");
    } else {
        printf("Can't find file named %s", fileName);
    }

    return 0;
}
