#ifndef PNG_H
#define PNG_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

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

extern const uint8_t PNG_SIGNATURE[8];

/*
 * parse_data
 *
 * PNG binary data structure:
 * - 8-byte signature
 * - Chunks data:
 *   - 4-byte length
 *   - 4-byte chunk type
 *   - N-byte data
 *   - 4-byte CRC (Ignored)
 */
int parse_data(FILE **file);

#endif
