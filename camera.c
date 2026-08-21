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
/* Camera transforms; model geometry is loaded by asset_manifest.c.    */
/***********************************************************************/

#include "battalion.h"

GLuint makeMonsterLookat(void)
    {
    GLuint displayList;

    displayList = glGenLists(1);
    glNewList(displayList, GL_COMPILE);
    gluLookAt(0, PLANEY + 2.4, 0.1, 0, PLANEY, 1, 0, 1, 0);
    glEndList();
    return displayList;
    }

GLuint makeOverviewLookat(void)
    {
    GLuint displayList;

    displayList = glGenLists(1);
    glNewList(displayList, GL_COMPILE);
    gluLookAt(0, 0, 9, 0, 0, 0, 0, 1, 0);
    glEndList();
    return displayList;
    }
