#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

/***********************
 * STRUCTS - Start
 ***********************/

/* #pragma pack(push, 1) */

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t bitDepth;
    uint8_t colorType;
    uint8_t compressionMethod;
    uint8_t filterMethod;
    uint8_t interfaceMethod;
} IHDRData;

typedef struct {
    uint32_t length;
    char type[5];
    uint8_t *data;
} PNGChunk;

typedef struct {
    PNGChunk *value;
    size_t size;
    size_t used;
} PNGChunks;

/* #pragma pack(pop) */

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
 * get_ihdr_data
 */
void get_ihdr_data(IHDRData *ihdr, PNGChunk *chunk) {
    uint8_t *chunkData = chunk->data;

    ihdr->width = (chunkData[0] << 24) | (chunkData[1] << 16) |
                  (chunkData[2] << 8) | chunkData[3];
    ihdr->height = (chunkData[4] << 24) | (chunkData[5] << 16) |
                   (chunkData[6] << 8) | chunkData[7];
    ihdr->bitDepth = chunkData[8];
    ihdr->colorType = chunkData[9];
    ihdr->compressionMethod = chunkData[10];
    ihdr->filterMethod = chunkData[11];
    ihdr->interfaceMethod = chunkData[12];
}

/*
 * get_chunks
 */
void get_chunks(PNGChunks *chunks, FILE *file) {
    chunks->value = NULL;
    chunks->used = 0;
    chunks->size = 0;

    while (!feof(file)) {
        PNGChunk chunk;

        chunk.length = read_big_endian(file);

        fread(chunk.type, 1, 4, file);
        chunk.type[4] = '\0';

        chunk.data = malloc(chunk.length);
        fread(chunk.data, 1, chunk.length, file);

        printf("Chunk: %s, Length: %d, Data: %p\n", chunk.type, chunk.length,
               chunk.data);

        fseek(file, 4, SEEK_CUR); // SKIP CRC on chunk

        if (chunks->used >= chunks->size) {
            chunks->size = chunks->size ? chunks->size * 2 : 8;
            chunks->value =
                realloc(chunks->value, chunks->size * sizeof(PNGChunk));
        }

        chunks->value[chunks->used++] = chunk;

        if (strcmp(chunk.type, "IEND") == 0) {
            break;
        }
    }
}

/*
 * get_compressed_data
 */
size_t get_compressed_data(uint8_t **compressedData, PNGChunks *chunks) {
    size_t size = 0;

    for (int i = 0; i < chunks->used; i++) {
        if (strcmp(chunks->value[i].type, "IDAT") == 0) {
            *compressedData =
                realloc(*compressedData, size + chunks->value[i].length);
            memcpy(*compressedData + size, chunks->value[i].data,
                   chunks->value[i].length);
            size += chunks->value[i].length;
        }
    }

    return size;
}

int get_bytes_per_pixel(uint8_t colorType) {
    switch (colorType) {
    case 0: // Grayscale
        return 1;
        break;
    case 2: // RGB
        return 3;
        break;
    case 3: // Indexed
        return 1;
        break;
    case 4: // Grayscale + alpha
        return 2;
        break;
    case 6: // RGBA
        return 4;
    default:
        printf("Unsupported color type");
        return 0;
    }
}

/*
 * get_uncompressed_size
 */
uLongf get_uncompressed_size(uint32_t width, uint32_t height,
                             uint8_t bytesPerPixel) {
    // Row size (width * bytesPerPixel) * height
    return ((width * bytesPerPixel) + 1) * height;
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

    PNGChunks chunks;
    get_chunks(&chunks, *file);

    uint8_t *compressedData = NULL;
    size_t compressedSize = get_compressed_data(&compressedData, &chunks);

    IHDRData ihdr;
    get_ihdr_data(&ihdr, &chunks.value[0]);

    int bytesPerPixel = get_bytes_per_pixel(ihdr.colorType);

    uLongf uncompressedSize =
        get_uncompressed_size(ihdr.width, ihdr.height, bytesPerPixel);
    uint8_t *uncompressedData = malloc(uncompressedSize);

    int result = uncompress(uncompressedData, &uncompressedSize, compressedData,
                            compressedSize);

    if (result != Z_OK) {
        fprintf(stderr, "Failed to decompress PNG data: %d\n", result);
        return 0;
    }

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
