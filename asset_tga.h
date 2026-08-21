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

#ifndef BATTALION_ASSET_TGA_H
#define BATTALION_ASSET_TGA_H

#include <stddef.h>

struct AssetImage
    {
    unsigned char *pixels;
    int width;
    int height;
    };

void assetImageInitialize(struct AssetImage *image);
void assetImageDestroy(struct AssetImage *image);
int assetImageLoadTGA(const char *path, struct AssetImage *image,
                      char *error, size_t errorSize);

#endif
