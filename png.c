#include "png.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

const uint8_t PNG_SIGNATURE[8] = {137, 80, 78, 71, 13, 10, 26, 10};

uint32_t read_big_endian(FILE *file) {
    uint8_t b[4];
    fread(b, 1, 4, file);

    return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

int validate_signature(FILE *file) {
    uint8_t *signature[8];

    fread(signature, 1, 8, file);

    if (memcmp(signature, PNG_SIGNATURE, 8)) {
        printf("Wrong PNG signature!\n");

        return 0;
    }

    return 1;
}

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

uLongf get_uncompressed_size(uint32_t width, uint32_t height,
                             uint8_t bytesPerPixel) {
    // Row size (width * bytesPerPixel) * height
    return ((width * bytesPerPixel) + 1) * height;
}

uint8_t *get_pixels(uint8_t *data, uint32_t width, uint32_t height,
                    uint8_t bytesPerPixel) {
    int scanlineLength = width * bytesPerPixel;
    int stride = scanlineLength + 1; // +1 for filter byte

    uint8_t *pixels = malloc(scanlineLength * width);

    for (int y = 0; y < height; y++) {
        uint8_t *scanline = data + (y * stride);
        uint8_t filterType = scanline[0];
        uint8_t *currentFiltered = scanline + 1;

        uint8_t *prevScanline = y > 0 ? data + ((y - 1) * stride) : NULL;
        uint8_t *prevFiltered = y > 0 ? prevScanline + 1 : NULL;

        uint8_t *output = pixels + (y * scanlineLength);

        switch (filterType) {
        case 0: // None
            memcpy(output, currentFiltered, scanlineLength);
            break;
        case 1: // Sub
            for (int i = 0; i < scanlineLength; i++) {
                uint8_t left =
                    i >= bytesPerPixel ? output[i - bytesPerPixel] : 0;
                output[i] = currentFiltered[i] + left;
            }
            break;
        case 2: // Up
            for (int i = 0; i < scanlineLength; i++) {
                uint8_t up = prevFiltered ? prevFiltered[i] : 0;
                output[i] = currentFiltered[i] + up;
            }
            break;
        case 3: // Average
            for (int i = 0; i < scanlineLength; i++) {
                uint8_t left =
                    i >= bytesPerPixel ? output[i - bytesPerPixel] : 0;
                uint8_t up = prevFiltered ? prevFiltered[i] : 0;
                uint8_t avg = (left + up) / 2;
                output[i] = currentFiltered[i] + avg;
            }
            break;
        case 4: // Paeth
            for (int i = 0; i < scanlineLength; i++) {
                uint8_t left =
                    i >= bytesPerPixel ? output[i - bytesPerPixel] : 0;
                uint8_t up = prevFiltered ? prevFiltered[i] : 0;
                uint8_t upLeft = prevFiltered && i >= bytesPerPixel
                                     ? prevFiltered[i - bytesPerPixel]
                                     : 0;

                // Calculations for getting the paeth value
                int p = left + up - upLeft;
                int pLeft = abs(p - left);
                int pUp = abs(p - up);
                int pUpLeft = abs(p - upLeft);

                uint8_t paeth;
                if (pLeft <= pUp && pLeft <= pUpLeft) {
                    paeth = left;
                } else if (pUp <= pUpLeft) {
                    paeth = up;
                } else {
                    paeth = upLeft;
                }

                output[i] = currentFiltered[i] + paeth;
            }
            break;
        default:
            printf("Unsupported filter type: %d", filterType);
            break;
        }
    }

    return pixels;
}

PNGDecoded *decode_data(FILE **file) {
    if (!validate_signature(*file)) {
        return NULL;
    }

    PNGDecoded *decoded;

    PNGChunks chunks;
    get_chunks(&chunks, *file);

    uint8_t *compressedData = NULL;
    size_t compressedSize = get_compressed_data(&compressedData, &chunks);

    get_ihdr_data(&decoded->ihdr, &chunks.value[0]);

    int bytesPerPixel = get_bytes_per_pixel(decoded->ihdr.colorType);

    uLongf uncompressedSize = get_uncompressed_size(
        decoded->ihdr.width, decoded->ihdr.height, bytesPerPixel);
    uint8_t *uncompressedData = malloc(uncompressedSize);

    int result = uncompress(uncompressedData, &uncompressedSize, compressedData,
                            compressedSize);

    if (result != Z_OK) {
        fprintf(stderr, "Failed to decompress PNG data: %d\n", result);
        return NULL;
    }

    decoded->pixels = get_pixels(uncompressedData, decoded->ihdr.width,
                                 decoded->ihdr.height, bytesPerPixel);

    return decoded;
}
