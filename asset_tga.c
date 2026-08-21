/****************************************************************/
/* code copyright 1995-1996 Andrew Johnson - ALL RIGHTS RESERVED*/
/*                          ajohnson@eecs.uic.edu               */
/*                                                              */
/*                      Electronic Visualization Lab (M/C 154)  */
/*                      University of Illinois at Chicago       */
/*                      851 South Morgan St. Room 1120 SEO      */
/*                      Chicago, IL 60607-7053                  */
/*                                                              */
/*                      (312) 996-3002     (312) 413-7585 FAX   */
/***********************************************************************/
/* Minimal TGA decoder used by the external asset pipeline.             */
/***********************************************************************/

#include "asset_tga.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void setError(char *error, size_t errorSize, const char *message)
    {
    if (error == NULL || errorSize == 0)
        return;
    strncpy(error, message, errorSize - 1);
    error[errorSize - 1] = '\0';
    }

static unsigned int readU16(const unsigned char *bytes)
    {
    return (unsigned int)bytes[0] | ((unsigned int)bytes[1] << 8);
    }

void assetImageInitialize(struct AssetImage *image)
    {
    image->pixels = NULL;
    image->width = 0;
    image->height = 0;
    }

void assetImageDestroy(struct AssetImage *image)
    {
    if (image == NULL)
        return;
    free(image->pixels);
    assetImageInitialize(image);
    }

static int readPixel(FILE *file, int components, unsigned char rgba[4])
    {
    unsigned char source[4];

    if (fread(source, 1, (size_t)components, file) != (size_t)components)
        return 0;
    rgba[0] = source[2];
    rgba[1] = source[1];
    rgba[2] = source[0];
    rgba[3] = components == 4 ? source[3] : 255;
    return 1;
    }

static void storePixel(struct AssetImage *image, size_t sourceIndex,
                       int topOrigin, int rightOrigin,
                       const unsigned char rgba[4])
    {
    size_t sourceX;
    size_t sourceY;
    size_t destinationX;
    size_t destinationY;
    size_t destination;

    sourceX = sourceIndex % (size_t)image->width;
    sourceY = sourceIndex / (size_t)image->width;
    destinationX = rightOrigin ? (size_t)image->width - 1 - sourceX : sourceX;
    destinationY = topOrigin ? (size_t)image->height - 1 - sourceY : sourceY;
    destination = (destinationY * (size_t)image->width + destinationX) * 4;
    memcpy(image->pixels + destination, rgba, 4);
    }

int assetImageLoadTGA(const char *path, struct AssetImage *image,
                      char *error, size_t errorSize)
    {
    unsigned char header[18];
    unsigned char rgba[4];
    unsigned char packet;
    FILE *file;
    size_t pixelCount;
    size_t pixelIndex;
    size_t packetCount;
    size_t packetIndex;
    int components;
    int topOrigin;
    int rightOrigin;

    if (path == NULL || image == NULL)
        {
        setError(error, errorSize, "TGA path and output image are required");
        return 0;
        }
    assetImageDestroy(image);
    file = fopen(path, "rb");
    if (file == NULL)
        {
        setError(error, errorSize, "Could not open TGA file");
        return 0;
        }
    if (fread(header, 1, sizeof(header), file) != sizeof(header))
        {
        fclose(file);
        setError(error, errorSize, "TGA header is truncated");
        return 0;
        }
    if (header[1] != 0 || (header[2] != 2 && header[2] != 10) ||
        (header[16] != 24 && header[16] != 32))
        {
        fclose(file);
        setError(error, errorSize,
                 "Only unpaletted 24/32-bit true-color TGA files are supported");
        return 0;
        }
    image->width = (int)readU16(header + 12);
    image->height = (int)readU16(header + 14);
    if (image->width <= 0 || image->height <= 0 ||
        (size_t)image->width > ((size_t)-1) / (size_t)image->height ||
        (size_t)image->width * (size_t)image->height > ((size_t)-1) / 4)
        {
        fclose(file);
        assetImageInitialize(image);
        setError(error, errorSize, "Invalid TGA dimensions");
        return 0;
        }
    pixelCount = (size_t)image->width * (size_t)image->height;
    image->pixels = (unsigned char *)malloc(pixelCount * 4);
    if (image->pixels == NULL)
        {
        fclose(file);
        assetImageInitialize(image);
        setError(error, errorSize, "Out of memory loading TGA file");
        return 0;
        }
    if (header[0] > 0 && fseek(file, (long)header[0], SEEK_CUR) != 0)
        {
        fclose(file);
        assetImageDestroy(image);
        setError(error, errorSize, "Could not skip TGA image identifier");
        return 0;
        }
    components = header[16] / 8;
    topOrigin = (header[17] & 0x20) != 0;
    rightOrigin = (header[17] & 0x10) != 0;
    pixelIndex = 0;
    if (header[2] == 2)
        {
        while (pixelIndex < pixelCount)
            {
            if (!readPixel(file, components, rgba))
                goto truncated;
            storePixel(image, pixelIndex++, topOrigin, rightOrigin, rgba);
            }
        }
    else
        {
        while (pixelIndex < pixelCount)
            {
            if (fread(&packet, 1, 1, file) != 1)
                goto truncated;
            packetCount = (size_t)(packet & 0x7f) + 1;
            if (packetCount > pixelCount - pixelIndex)
                goto truncated;
            if (packet & 0x80)
                {
                if (!readPixel(file, components, rgba))
                    goto truncated;
                for (packetIndex = 0; packetIndex < packetCount; packetIndex++)
                    storePixel(image, pixelIndex++, topOrigin, rightOrigin,
                               rgba);
                }
            else
                for (packetIndex = 0; packetIndex < packetCount; packetIndex++)
                    {
                    if (!readPixel(file, components, rgba))
                        goto truncated;
                    storePixel(image, pixelIndex++, topOrigin, rightOrigin,
                               rgba);
                    }
            }
        }
    fclose(file);
    return 1;

truncated:
    fclose(file);
    assetImageDestroy(image);
    setError(error, errorSize, "TGA pixel data is truncated or malformed");
    return 0;
    }
