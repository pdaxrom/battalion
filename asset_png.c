/*
 * Copyright (c) 2026 Battalion contributors.
 */
/* Small C89 PNG/deflate decoder for embedded glTF base-color images.   */
/***********************************************************************/

#include "asset_png.h"
#include "asset_tga.h"

#include <stdlib.h>
#include <string.h>

#define PNG_HUFFMAN_SIZE 32768
#define PNG_MAX_CODE_BITS 15

struct PngBits
    {
    const unsigned char *data;
    size_t length;
    size_t position;
    unsigned long value;
    int count;
    };

struct PngHuffman
    {
    short symbols[PNG_HUFFMAN_SIZE];
    unsigned char lengths[PNG_HUFFMAN_SIZE];
    };

static void pngError(char *error, size_t errorSize, const char *message)
    {
    if (error == NULL || errorSize == 0)
        return;
    strncpy(error, message, errorSize - 1);
    error[errorSize - 1] = '\0';
    }

static unsigned long pngReadU32(const unsigned char *bytes)
    {
    return ((unsigned long)bytes[0] << 24) |
           ((unsigned long)bytes[1] << 16) |
           ((unsigned long)bytes[2] << 8) |
           (unsigned long)bytes[3];
    }

static int pngBitsFill(struct PngBits *bits, int wanted)
    {
    while (bits->count < wanted && bits->position < bits->length)
        {
        bits->value |= (unsigned long)bits->data[bits->position++] <<
                       bits->count;
        bits->count += 8;
        }
    return bits->count >= wanted;
    }

static int pngBitsRead(struct PngBits *bits, int count, unsigned int *value)
    {
    unsigned long mask;

    if (count == 0)
        {
        *value = 0;
        return 1;
        }
    if (!pngBitsFill(bits, count))
        return 0;
    mask = (1UL << count) - 1UL;
    *value = (unsigned int)(bits->value & mask);
    bits->value >>= count;
    bits->count -= count;
    return 1;
    }

static unsigned int pngReverseBits(unsigned int value, int count)
    {
    unsigned int result;

    result = 0;
    while (count-- > 0)
        {
        result = (result << 1) | (value & 1U);
        value >>= 1;
        }
    return result;
    }

static int pngBuildHuffman(struct PngHuffman *table,
                           const unsigned char *lengths, int symbolCount)
    {
    unsigned int counts[PNG_MAX_CODE_BITS + 1];
    unsigned int nextCode[PNG_MAX_CODE_BITS + 1];
    unsigned int code;
    unsigned int reversed;
    unsigned int entry;
    int length;
    int symbol;

    memset(table, 0, sizeof(*table));
    memset(counts, 0, sizeof(counts));
    for (symbol = 0; symbol < symbolCount; symbol++)
        {
        if (lengths[symbol] > PNG_MAX_CODE_BITS)
            return 0;
        if (lengths[symbol] > 0)
            counts[lengths[symbol]]++;
        }
    code = 0;
    counts[0] = 0;
    for (length = 1; length <= PNG_MAX_CODE_BITS; length++)
        {
        code = (code + counts[length - 1]) << 1;
        nextCode[length] = code;
        if (code + counts[length] > (1UL << length))
            return 0;
        }
    for (symbol = 0; symbol < symbolCount; symbol++)
        {
        length = lengths[symbol];
        if (length == 0)
            continue;
        reversed = pngReverseBits(nextCode[length]++, length);
        for (entry = reversed; entry < PNG_HUFFMAN_SIZE;
             entry += 1U << length)
            {
            table->symbols[entry] = (short)symbol;
            table->lengths[entry] = (unsigned char)length;
            }
        }
    return 1;
    }

static int pngDecodeSymbol(struct PngBits *bits,
                           const struct PngHuffman *table, int *symbol)
    {
    unsigned int entry;
    int length;

    if (bits->count == 0 && !pngBitsFill(bits, 1))
        return 0;
    pngBitsFill(bits, PNG_MAX_CODE_BITS);
    entry = (unsigned int)(bits->value & (PNG_HUFFMAN_SIZE - 1));
    length = table->lengths[entry];
    if (length == 0 || length > bits->count)
        return 0;
    *symbol = table->symbols[entry];
    bits->value >>= length;
    bits->count -= length;
    return 1;
    }

static int pngDynamicTables(struct PngBits *bits,
                            struct PngHuffman *literalTable,
                            struct PngHuffman *distanceTable)
    {
    static const int order[19] =
        {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    unsigned char codeLengths[19];
    unsigned char lengths[320];
    struct PngHuffman *codeTable;
    unsigned int value;
    unsigned int repeat;
    int literalCount;
    int distanceCount;
    int codeCount;
    int total;
    int index;
    int symbol;
    int previous;
    int success;

    if (!pngBitsRead(bits, 5, &value))
        return 0;
    literalCount = (int)value + 257;
    if (!pngBitsRead(bits, 5, &value))
        return 0;
    distanceCount = (int)value + 1;
    if (!pngBitsRead(bits, 4, &value))
        return 0;
    codeCount = (int)value + 4;
    if (literalCount > 286 || distanceCount > 32)
        return 0;
    memset(codeLengths, 0, sizeof(codeLengths));
    for (index = 0; index < codeCount; index++)
        {
        if (!pngBitsRead(bits, 3, &value))
            return 0;
        codeLengths[order[index]] = (unsigned char)value;
        }
    codeTable = (struct PngHuffman *)malloc(sizeof(*codeTable));
    if (codeTable == NULL)
        return 0;
    success = pngBuildHuffman(codeTable, codeLengths, 19);
    total = literalCount + distanceCount;
    index = 0;
    previous = 0;
    while (success && index < total)
        {
        if (!pngDecodeSymbol(bits, codeTable, &symbol))
            {
            success = 0;
            break;
            }
        if (symbol <= 15)
            {
            lengths[index++] = (unsigned char)symbol;
            previous = symbol;
            }
        else if (symbol == 16)
            {
            if (index == 0 || !pngBitsRead(bits, 2, &repeat))
                success = 0;
            else
                {
                repeat += 3;
                if (repeat > (unsigned int)(total - index))
                    success = 0;
                while (success && repeat-- > 0)
                    lengths[index++] = (unsigned char)previous;
                }
            }
        else if (symbol == 17 || symbol == 18)
            {
            if (!pngBitsRead(bits, symbol == 17 ? 3 : 7, &repeat))
                success = 0;
            else
                {
                repeat += symbol == 17 ? 3 : 11;
                if (repeat > (unsigned int)(total - index))
                    success = 0;
                while (success && repeat-- > 0)
                    lengths[index++] = 0;
                previous = 0;
                }
            }
        else
            success = 0;
        }
    free(codeTable);
    if (!success || lengths[256] == 0)
        return 0;
    return pngBuildHuffman(literalTable, lengths, literalCount) &&
           pngBuildHuffman(distanceTable, lengths + literalCount,
                           distanceCount);
    }

static int pngFixedTables(struct PngHuffman *literalTable,
                          struct PngHuffman *distanceTable)
    {
    unsigned char literalLengths[288];
    unsigned char distanceLengths[32];
    int index;

    for (index = 0; index <= 143; index++) literalLengths[index] = 8;
    for (; index <= 255; index++) literalLengths[index] = 9;
    for (; index <= 279; index++) literalLengths[index] = 7;
    for (; index <= 287; index++) literalLengths[index] = 8;
    for (index = 0; index < 32; index++) distanceLengths[index] = 5;
    return pngBuildHuffman(literalTable, literalLengths, 288) &&
           pngBuildHuffman(distanceTable, distanceLengths, 32);
    }

static int pngInflateCompressed(struct PngBits *bits,
                                struct PngHuffman *literalTable,
                                struct PngHuffman *distanceTable,
                                unsigned char *output, size_t outputSize,
                                size_t *outputPosition)
    {
    static const unsigned short lengthBase[29] =
        {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,
         99,115,131,163,195,227,258};
    static const unsigned char lengthExtra[29] =
        {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
    static const unsigned short distanceBase[30] =
        {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,
         1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
    static const unsigned char distanceExtra[30] =
        {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,
         12,12,13,13};
    unsigned int extra;
    size_t length;
    size_t distance;
    int symbol;
    int distanceSymbol;

    for (;;)
        {
        if (!pngDecodeSymbol(bits, literalTable, &symbol))
            return 0;
        if (symbol < 256)
            {
            if (*outputPosition >= outputSize)
                return 0;
            output[(*outputPosition)++] = (unsigned char)symbol;
            }
        else if (symbol == 256)
            return 1;
        else
            {
            if (symbol < 257 || symbol > 285)
                return 0;
            symbol -= 257;
            if (!pngBitsRead(bits, lengthExtra[symbol], &extra))
                return 0;
            length = (size_t)lengthBase[symbol] + extra;
            if (!pngDecodeSymbol(bits, distanceTable, &distanceSymbol) ||
                distanceSymbol < 0 || distanceSymbol >= 30 ||
                !pngBitsRead(bits, distanceExtra[distanceSymbol], &extra))
                return 0;
            distance = (size_t)distanceBase[distanceSymbol] + extra;
            if (distance == 0 || distance > *outputPosition ||
                length > outputSize - *outputPosition)
                return 0;
            while (length-- > 0)
                {
                output[*outputPosition] = output[*outputPosition - distance];
                (*outputPosition)++;
                }
            }
        }
    }

static unsigned long pngAdler32(const unsigned char *data, size_t length)
    {
    unsigned long a;
    unsigned long b;
    size_t block;
    size_t index;

    a = 1;
    b = 0;
    index = 0;
    while (index < length)
        {
        block = length - index;
        if (block > 5552)
            block = 5552;
        while (block-- > 0)
            {
            a += data[index++];
            b += a;
            }
        a %= 65521UL;
        b %= 65521UL;
        }
    return (b << 16) | a;
    }

static int pngInflateZlib(const unsigned char *data, size_t length,
                          unsigned char *output, size_t outputSize)
    {
    struct PngBits bits;
    struct PngHuffman *literalTable;
    struct PngHuffman *distanceTable;
    unsigned int finalBlock;
    unsigned int blockType;
    unsigned int storedLength;
    unsigned int storedInverse;
    size_t outputPosition;
    int success;

    if (length < 6 || (data[0] & 15) != 8 ||
        (((unsigned int)data[0] << 8) + data[1]) % 31 != 0 ||
        (data[1] & 32) != 0)
        return 0;
    bits.data = data + 2;
    bits.length = length - 6;
    bits.position = 0;
    bits.value = 0;
    bits.count = 0;
    outputPosition = 0;
    success = 1;
    literalTable = (struct PngHuffman *)malloc(sizeof(*literalTable));
    distanceTable = (struct PngHuffman *)malloc(sizeof(*distanceTable));
    if (literalTable == NULL || distanceTable == NULL)
        success = 0;
    finalBlock = 0;
    while (success && !finalBlock)
        {
        if (!pngBitsRead(&bits, 1, &finalBlock) ||
            !pngBitsRead(&bits, 2, &blockType))
            {
            success = 0;
            break;
            }
        if (blockType == 0)
            {
            bits.value = 0;
            bits.count = 0;
            if (bits.position + 4 > bits.length)
                success = 0;
            else
                {
                storedLength = (unsigned int)bits.data[bits.position] |
                    ((unsigned int)bits.data[bits.position + 1] << 8);
                storedInverse = (unsigned int)bits.data[bits.position + 2] |
                    ((unsigned int)bits.data[bits.position + 3] << 8);
                bits.position += 4;
                if ((storedLength ^ 65535U) != storedInverse ||
                    storedLength > bits.length - bits.position ||
                    storedLength > outputSize - outputPosition)
                    success = 0;
                else
                    {
                    memcpy(output + outputPosition,
                           bits.data + bits.position, storedLength);
                    outputPosition += storedLength;
                    bits.position += storedLength;
                    }
                }
            }
        else if (blockType == 1 || blockType == 2)
            {
            if ((blockType == 1 &&
                 !pngFixedTables(literalTable, distanceTable)) ||
                (blockType == 2 &&
                 !pngDynamicTables(&bits, literalTable, distanceTable)) ||
                !pngInflateCompressed(&bits, literalTable, distanceTable,
                                      output, outputSize, &outputPosition))
                success = 0;
            }
        else
            success = 0;
        }
    free(literalTable);
    free(distanceTable);
    if (!success || outputPosition != outputSize)
        return 0;
    return pngAdler32(output, outputSize) == pngReadU32(data + length - 4);
    }

static unsigned char pngPaeth(unsigned char left, unsigned char above,
                              unsigned char upperLeft)
    {
    int prediction;
    int leftDistance;
    int aboveDistance;
    int upperLeftDistance;

    prediction = (int)left + (int)above - (int)upperLeft;
    leftDistance = prediction > left ? prediction - left : left - prediction;
    aboveDistance = prediction > above ? prediction - above :
                                                 above - prediction;
    upperLeftDistance = prediction > upperLeft ? prediction - upperLeft :
                                                       upperLeft - prediction;
    if (leftDistance <= aboveDistance && leftDistance <= upperLeftDistance)
        return left;
    if (aboveDistance <= upperLeftDistance)
        return above;
    return upperLeft;
    }

static int pngUnfilter(unsigned char *result, const unsigned char *raw,
                       size_t width, size_t height, int components)
    {
    size_t rowBytes;
    size_t row;
    size_t column;
    size_t rawOffset;
    size_t resultOffset;
    unsigned char filter;
    unsigned char left;
    unsigned char above;
    unsigned char upperLeft;

    rowBytes = width * (size_t)components;
    rawOffset = 0;
    for (row = 0; row < height; row++)
        {
        filter = raw[rawOffset++];
        if (filter > 4)
            return 0;
        resultOffset = row * rowBytes;
        for (column = 0; column < rowBytes; column++)
            {
            left = column >= (size_t)components ?
                result[resultOffset + column - components] : 0;
            above = row > 0 ? result[resultOffset + column - rowBytes] : 0;
            upperLeft = row > 0 && column >= (size_t)components ?
                result[resultOffset + column - rowBytes - components] : 0;
            if (filter == 1)
                result[resultOffset + column] = raw[rawOffset] + left;
            else if (filter == 2)
                result[resultOffset + column] = raw[rawOffset] + above;
            else if (filter == 3)
                result[resultOffset + column] = raw[rawOffset] +
                    (unsigned char)(((unsigned int)left + above) / 2);
            else if (filter == 4)
                result[resultOffset + column] = raw[rawOffset] +
                    pngPaeth(left, above, upperLeft);
            else
                result[resultOffset + column] = raw[rawOffset];
            rawOffset++;
            }
        }
    return 1;
    }

int assetImageDecodePNG(const unsigned char *data, size_t length,
                        struct AssetImage *image,
                        char *error, size_t errorSize)
    {
    static const unsigned char signature[8] =
        {137, 80, 78, 71, 13, 10, 26, 10};
    unsigned char palette[256 * 4];
    unsigned char *compressed;
    unsigned char *expanded;
    unsigned char *filtered;
    size_t compressedSize;
    size_t chunkLength;
    size_t offset;
    size_t rowBytes;
    size_t rawSize;
    size_t pixelCount;
    size_t pixel;
    unsigned long width;
    unsigned long height;
    int bitDepth;
    int colorType;
    int components;
    int paletteCount;
    int headerFound;
    int endFound;
    int index;

    if (data == NULL || image == NULL || length < 8 ||
        memcmp(data, signature, 8) != 0)
        {
        pngError(error, errorSize, "Invalid PNG signature");
        return 0;
        }
    assetImageDestroy(image);
    compressed = NULL;
    expanded = NULL;
    filtered = NULL;
    compressedSize = 0;
    width = height = 0;
    bitDepth = colorType = components = paletteCount = 0;
    headerFound = endFound = 0;
    for (index = 0; index < 256; index++)
        {
        palette[index * 4] = palette[index * 4 + 1] =
            palette[index * 4 + 2] = 0;
        palette[index * 4 + 3] = 255;
        }
    offset = 8;
    while (offset + 12 <= length)
        {
        chunkLength = (size_t)pngReadU32(data + offset);
        if (chunkLength > length - offset - 12)
            goto invalid;
        if (memcmp(data + offset + 4, "IHDR", 4) == 0)
            {
            if (headerFound || chunkLength != 13)
                goto invalid;
            width = pngReadU32(data + offset + 8);
            height = pngReadU32(data + offset + 12);
            bitDepth = data[offset + 16];
            colorType = data[offset + 17];
            if (data[offset + 18] != 0 || data[offset + 19] != 0 ||
                data[offset + 20] != 0)
                goto unsupported;
            headerFound = 1;
            }
        else if (memcmp(data + offset + 4, "PLTE", 4) == 0)
            {
            if (chunkLength == 0 || chunkLength > 256 * 3 ||
                chunkLength % 3 != 0)
                goto invalid;
            paletteCount = (int)(chunkLength / 3);
            for (index = 0; index < paletteCount; index++)
                memcpy(palette + index * 4,
                       data + offset + 8 + index * 3, 3);
            }
        else if (memcmp(data + offset + 4, "tRNS", 4) == 0 && colorType == 3)
            {
            if (chunkLength > 256)
                goto invalid;
            for (index = 0; index < (int)chunkLength; index++)
                palette[index * 4 + 3] = data[offset + 8 + index];
            }
        else if (memcmp(data + offset + 4, "IDAT", 4) == 0)
            {
            unsigned char *grown;
            if (!headerFound || chunkLength > ((size_t)-1) - compressedSize)
                goto invalid;
            grown = (unsigned char *)realloc(compressed,
                                             compressedSize + chunkLength);
            if (grown == NULL)
                goto memory;
            compressed = grown;
            memcpy(compressed + compressedSize, data + offset + 8,
                   chunkLength);
            compressedSize += chunkLength;
            }
        else if (memcmp(data + offset + 4, "IEND", 4) == 0)
            {
            endFound = 1;
            break;
            }
        offset += chunkLength + 12;
        }
    if (!headerFound || !endFound || compressedSize == 0 ||
        width == 0 || height == 0 || width > 32768UL || height > 32768UL ||
        bitDepth != 8)
        goto unsupported;
    if (colorType == 0) components = 1;
    else if (colorType == 2) components = 3;
    else if (colorType == 3) components = 1;
    else if (colorType == 4) components = 2;
    else if (colorType == 6) components = 4;
    else goto unsupported;
    if (colorType == 3 && paletteCount == 0)
        goto invalid;
    if ((size_t)width > ((size_t)-1) / (size_t)components)
        goto invalid;
    rowBytes = (size_t)width * (size_t)components;
    if (rowBytes == (size_t)-1 ||
        (size_t)height > ((size_t)-1) / (rowBytes + 1))
        goto invalid;
    rawSize = (rowBytes + 1) * (size_t)height;
    pixelCount = (size_t)width * (size_t)height;
    if (pixelCount > ((size_t)-1) / 4)
        goto invalid;
    filtered = (unsigned char *)malloc(rawSize);
    expanded = (unsigned char *)malloc(rowBytes * (size_t)height);
    image->pixels = (unsigned char *)malloc(pixelCount * 4);
    if (filtered == NULL || expanded == NULL || image->pixels == NULL)
        goto memory;
    if (!pngInflateZlib(compressed, compressedSize, filtered, rawSize) ||
        !pngUnfilter(expanded, filtered, (size_t)width, (size_t)height,
                     components))
        goto invalid;
    for (pixel = 0; pixel < pixelCount; pixel++)
        {
        if (colorType == 0)
            {
            image->pixels[pixel * 4] = image->pixels[pixel * 4 + 1] =
                image->pixels[pixel * 4 + 2] = expanded[pixel];
            image->pixels[pixel * 4 + 3] = 255;
            }
        else if (colorType == 2)
            {
            memcpy(image->pixels + pixel * 4, expanded + pixel * 3, 3);
            image->pixels[pixel * 4 + 3] = 255;
            }
        else if (colorType == 3)
            {
            index = expanded[pixel];
            if (index >= paletteCount)
                goto invalid;
            memcpy(image->pixels + pixel * 4, palette + index * 4, 4);
            }
        else if (colorType == 4)
            {
            image->pixels[pixel * 4] = image->pixels[pixel * 4 + 1] =
                image->pixels[pixel * 4 + 2] = expanded[pixel * 2];
            image->pixels[pixel * 4 + 3] = expanded[pixel * 2 + 1];
            }
        else
            memcpy(image->pixels + pixel * 4, expanded + pixel * 4, 4);
        }
    image->width = (int)width;
    image->height = (int)height;
    free(compressed);
    free(expanded);
    free(filtered);
    return 1;

memory:
    pngError(error, errorSize, "Out of memory decoding PNG image");
    goto failed;
unsupported:
    pngError(error, errorSize,
             "Unsupported PNG format (requires non-interlaced 8-bit PNG)");
    goto failed;
invalid:
    pngError(error, errorSize, "Invalid or corrupt PNG image");
failed:
    free(compressed);
    free(expanded);
    free(filtered);
    assetImageDestroy(image);
    return 0;
    }
