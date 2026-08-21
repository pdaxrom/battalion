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

#ifndef BATTALION_ASSET_RENDER_H
#define BATTALION_ASSET_RENDER_H

#include <stddef.h>

struct AssetModel;
struct AssetPose;

void assetModelDraw(const struct AssetModel *model, const struct AssetPose *pose);
unsigned int assetModelCreateDisplayList(const struct AssetModel *model);
unsigned int assetLoadGLBDisplayList(const char *path,
                                     char *error, size_t errorSize);

#endif
