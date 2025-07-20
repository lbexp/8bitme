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

void write_big_endian(uint8_t *bytes, uint32_t source) {
    bytes[0] = (source >> 24) & 0xFF;
    bytes[1] = (source >> 16) & 0xFF;
    bytes[2] = (source >> 8) & 0xFF;
    bytes[3] = source & 0xFF;
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

void get_chunks(PNGChunks *meta, PNGChunks *idat, FILE *file) {
    printf("************************ GET CHUNKS ************************\n");
    meta->value = NULL;
    meta->used = 0;
    meta->size = 0;

    idat->value = NULL;
    idat->used = 0;
    idat->size = 0;

    while (!feof(file)) {
        PNGChunk chunk;

        chunk.length = read_big_endian(file);

        fread(chunk.type, 1, 4, file);
        chunk.type[4] = '\0';

        chunk.data = malloc(chunk.length);
        fread(chunk.data, 1, chunk.length, file);

        /* fseek(file, 4, SEEK_CUR);    // SKIP CRC on chunk */
        uint32_t crc = read_big_endian(file);
        /* fread(&crc, 1, 4, file); // SKIP CRC on chunk */

        printf("Chunk -> length %d, type %s, crc %d\n", chunk.length,
               chunk.type, crc);
        if (strcmp(chunk.type, "IEND") == 0) {
            break;
        } else if (strcmp(chunk.type, "IDAT") == 0) {
            if (idat->used >= idat->size) {
                idat->size = idat->size ? idat->size * 2 : 8;
                idat->value =
                    realloc(idat->value, idat->size * sizeof(PNGChunk));
            }

            idat->value[idat->used++] = chunk;
        } else {
            if (meta->used >= meta->size) {
                meta->size = meta->size ? meta->size * 2 : 8;
                meta->value =
                    realloc(meta->value, meta->size * sizeof(PNGChunk));
            }

            meta->value[meta->used++] = chunk;
        }
    }
}

void generate_chunks(FILE *file, uint8_t *compressedData, uLongf *size,
                     PNGChunks *meta) {
    fwrite(PNG_SIGNATURE, 1, 8, file);

    // Write meta chunks (IHDR, etc)
    for (int i = 0; i < meta->used; i++) {
        PNGChunk chunk = meta->value[i];

        uint8_t bigEndianLength[4];
        write_big_endian(bigEndianLength, chunk.length);

        fwrite(bigEndianLength, 1, 4, file);
        fwrite(chunk.type, 1, 4, file);
        fwrite(chunk.data, 1, chunk.length, file);

        uLongf crc = crc32(0, NULL, 0);
        crc = crc32(crc, (const Bytef *)chunk.type, 4);
        crc = crc32(crc, chunk.data, chunk.length);

        uint8_t bigEndianCrc[4];
        write_big_endian(bigEndianCrc, crc);
        fwrite(bigEndianCrc, 1, 4, file);

        printf("Chunk -> length %d, type %s, crc %d\n", chunk.length,
               chunk.type, *bigEndianCrc);
    }

    // Writing IDAT by loop through *compressedData
    uLongf idatPointer = 0;
    while (idatPointer < *size) {
        uLongf remaining = *size - idatPointer;
        uint32_t length =
            (remaining > IDAT_LENGTH) ? IDAT_LENGTH : (uint32_t)remaining;

        uint8_t bigEndianLength[4];
        write_big_endian(bigEndianLength, length);

        uint8_t *currentStart = compressedData + idatPointer;

        fwrite(bigEndianLength, 1, 4, file);
        fwrite("IDAT", 1, 4, file);
        fwrite(currentStart, 1, length, file);

        uLongf crc = crc32(0, NULL, 0);
        crc = crc32(crc, (const Bytef *)"IDAT", 4);
        crc = crc32(crc, currentStart, length);

        uint8_t bigEndianCrc[4];
        write_big_endian(bigEndianCrc, crc);
        fwrite(bigEndianCrc, 1, 4, file);
        printf("Chunk -> length %d, type %s, crc %d\n", length, "IDAT",
               *bigEndianCrc);

        if (length != (uint32_t)IDAT_LENGTH) {
            break;
        }

        idatPointer += IDAT_LENGTH;
    }

    fwrite("IEND", 1, 4, file);
};

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

int generate_filtered_data(uint8_t *filteredData, uint8_t *pixels,
                           uint32_t width, uint32_t height, int bytesPerPixel) {
    int scanlineLength = width * bytesPerPixel;
    int stride = scanlineLength + 1;
    int offset = 0;

    for (int y = 0; y < height; y++) {
        uint8_t *scanline = pixels + (y * stride);
        // TODO: Add other filter type to get more compressed data
        // Hardcoded to 0 for MVP
        uint8_t filterType = 0;
        filteredData[offset] = filterType;

        memcpy(filteredData, scanline, stride);

        offset += stride + 1;
    }

    return 1;
}

/*
 * Decode PNG data from file to PNGDecoded format
 * {
 *  IHDRData ihdr;
 *  uint8_t *pixels;
 * }
 * the pixels data is chunk that got flatten out
 * and visually would look like this:
 * [<c>, <h>, <u>, <n>, <k>, <c>, <h>, <u>, ...]
 */
PNGDecoded *decode_data(FILE **file) {
    printf("\n************************************************************\n");
    printf("************************* DECODING *************************");
    printf("\n************************************************************\n");
    if (!validate_signature(*file)) {
        return NULL;
    }

    PNGDecoded *decoded;

    // Get PNGChunks data, which consists of
    // array of chunk information data per length
    // [<ihdr>, <idat>, <idat>, ...]
    PNGChunks metaChunks;
    PNGChunks idatChunks;
    get_chunks(&metaChunks, &idatChunks, *file);
    decoded->metaChunks = metaChunks;

    // Get IDAT data only (by default as compressed data)
    // would be store into pointer uint8_t (chunks being flatten out)
    // [<i>, <d>, <a>, <t>, <i>, <d>, <a>, <t>, ...]
    uint8_t *compressedData = NULL;
    size_t compressedSize = get_compressed_data(&compressedData, &idatChunks);

    // Get IHDR data separately
    get_ihdr_data(&decoded->ihdr, &metaChunks.value[0]);

    // Get bytesPerPixel to determine uncompressed data size
    // per image row (ihdr resolution width * bytesPerPixel)
    // and each pixel data,
    // bytesPerPixel meant pixel format (RGB, RGBA, Grayscale, etc)
    int bytesPerPixel = get_bytes_per_pixel(decoded->ihdr.colorType);

    // Generate uncompressed data (still same format as compressed data, only
    // longer values)
    uLongf uncompressedSize = get_uncompressed_size(
        decoded->ihdr.width, decoded->ihdr.height, bytesPerPixel);
    uint8_t *uncompressedData = malloc(uncompressedSize);
    int result = uncompress(uncompressedData, &uncompressedSize, compressedData,
                            compressedSize);

    if (result != Z_OK) {
        fprintf(stderr, "Failed to decompress PNG data: %d\n", result);
        return NULL;
    }

    // Generate raw pixels data,
    // converting uncompressed data into unfiltered data which contains raw
    // pixel data
    decoded->pixels = get_pixels(uncompressedData, decoded->ihdr.width,
                                 decoded->ihdr.height, bytesPerPixel);

    return decoded;
}

/*
 * Encode PNGDecoded into binary file PNG
 */
int encode_data(FILE **file, PNGDecoded *decoded) {
    printf("\n************************************************************\n");
    printf("************************* ENCODING *************************");
    printf("\n************************************************************\n");
    int bytesPerPixel = get_bytes_per_pixel(decoded->ihdr.colorType);

    // Generate filtered data
    // Change pixles data into filtered data
    uLongf filteredSize = get_uncompressed_size(
        decoded->ihdr.width, decoded->ihdr.height, bytesPerPixel);
    uint8_t *filteredData = malloc(filteredSize);
    int filteredResult = generate_filtered_data(
        filteredData, decoded->pixels, decoded->ihdr.width,
        decoded->ihdr.height, bytesPerPixel);

    if (!filteredResult) {
        printf("Failed to generate filtered data\n");
        return 0;
    }

    // Generate compressed filtered data
    // [<i>, <d>, <a>, <t>, <i>, <d>, <a>, <t>, ...]
    uLongf compressedSize = compressBound(filteredSize);
    uint8_t *compressedData = malloc(compressedSize);
    int compressResult =
        compress(compressedData, &compressedSize, filteredData, filteredSize);

    if (compressResult != Z_OK) {
        printf("Failed to compress data %d\n", compressResult);
        return 0;
    }

    // Generate chunks data and put into file
    // [<ihdr>, <idat>, <idat>, ...]
    generate_chunks(*file, compressedData, &compressedSize,
                    &decoded->metaChunks);

    return 1;
}
