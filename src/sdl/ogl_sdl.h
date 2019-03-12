// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: ogl_sdl.h 1423 2019-01-29 08:06:47Z wesleyjohnson $
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
// $Log: ogl_sdl.h,v $
// Revision 1.2  2000/11/02 19:49:40  bpereira
// Revision 1.1  2000/09/26 17:54:42  metzgermeister
// initial import
//
//
// DESCRIPTION:
//      SDL specific part of the OpenGL API for Doom Legacy
//
//-----------------------------------------------------------------------------

#ifndef OGL_SDL_H
#define OGL_SDL_H

#include "doomtype.h"

extern byte  ogl_active;

boolean OglSdlSurface(int w, int h, int isFullscreen);

void OglSdlFinishUpdate(boolean vidwait);

void OglSdlShutdown(void);

void OglSdlSetPalette(RGBA_t *palette, int gammaRed, int gammaGreen, int gammaBlue);

#endif

