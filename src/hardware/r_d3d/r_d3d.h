// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_d3d.h 1245 2016-08-04 14:21:00Z wesleyjohnson $
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// $Log: r_d3d.h,v $
// Revision 1.2  2000/10/22 14:18:22  hurdler
// Adjust version string
//
// Revision 1.1  2000/10/01 15:14:31  hurdler
// Completely rewritten d3d driver... absolutely not finished at all
//
//
// DESCRIPTION:
//      
//
//-----------------------------------------------------------------------------

#ifndef R_D3D_H
#define R_D3D_H

#ifndef SMIF_WIN_NATIVE
# error r_d3d is WIN_NATIVE only
#endif

#include "doomtype.h"
  // RGBA_t
#include "gl2d3d.h"

// necessary for Unix AND Windows
#define  HWRAPI_CREATE_DLL
#include "hardware/hw_drv.h"

// ==========================================================================
//                                                                DEFINITIONS
// ==========================================================================

#define MIN(x,y) ( ((x)<(y)) ? (x) : (y) )
#define MAX(x,y) ( ((x)>(y)) ? (x) : (y) )


#define DRIVER_STRING "HWRAPI Init(): DooM Legacy Direct3D renderer"

// ==========================================================================
//                                                                     PROTOS
// ==========================================================================

void DBG_Printf(LPCTSTR lpFmt, ...);                                          
void Flush(void);
int  isExtAvailable(char *extension);
int  SetRes(viddef_t *lvid, vmode_t *pcurrentmode);
void UnSetRes(void);
boolean SetupPixelFormat(int WantColorBits, int WantStencilBits, int WantDepthBits);
void SetModelView(GLint w, GLint h);
void SetStates(void);

// ==========================================================================
//                                                                     GLOBAL
// ==========================================================================

extern const GLubyte    *gl_extensions;
extern RGBA_t           myPaletteData[];
//extern HANDLE           logstream;   // name clash
extern GLint            screen_width;
extern GLint            screen_height;
extern GLbyte           screen_depth;
extern int              oglflags;

typedef enum {
    GLF_NOZBUFREAD = 0x01,
} oglflags_t;

#endif
