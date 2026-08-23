/*
 * Copyright (c) 2026 Battalion contributors.
 */
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
