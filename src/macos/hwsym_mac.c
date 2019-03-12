// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: hwsym_mac.c 1257 2016-09-20 17:14:21Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2016 by DooM Legacy Team.
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
// Revision 1.1  2001/04/17 22:23:38  calumr
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//
//
//
// DESCRIPTION:
//      Tool for dynamic referencing of hardware rendering functions
//
//      Declaration and definition of the HW rendering 
//      functions do have the same name. Originally, the
//      implementation was stored in a separate library.
//      For SDL, we need some function to return the addresses,
//      otherwise we have a conflict with the compiler.
//      
//-----------------------------------------------------------------------------

#include "hardware/r_opengl/r_opengl.h"
#include "hwsym_mac.h"

//
// Stupid function to return function addresses
//
void *hwSym(char *funcName)
{
    if(0 == strcmp("Init", funcName))
    {
	return &Init;
    }
    else if(0 == strcmp("FinishUpdate", funcName))
    {
	return NULL; //&FinishUpdate;
    }
    else if(0 == strcmp("Draw2DLine", funcName))
    {
	return &Draw2DLine;
    }
    else if(0 == strcmp("DrawPolygon", funcName))
    {
	return &DrawPolygon;
    }
    else if(0 == strcmp("SetBlend", funcName))
    {
	return &SetBlend;
    }
    else if(0 == strcmp("ClearBuffer", funcName))
    {
	return &ClearBuffer;
    }
    else if(0 == strcmp("SetTexture", funcName))
    {
	return &SetTexture;
    }
    else if(0 == strcmp("ReadRect", funcName))
    {
	return &ReadRect;
    }
    else if(0 == strcmp("GClipRect", funcName))
    {
	return &GClipRect;
    }
    else if(0 == strcmp("ClearMipMapCache", funcName))
    {
	return &ClearMipMapCache;
    }
    else if(0 == strcmp("SetSpecialState", funcName))
    {
	return &SetSpecialState;
    }
    else if(0 == strcmp("SetTransform", funcName))
    {
	return &SetTransform;
    }
    else if(0 == strcmp("DrawMD2", funcName))
    {
	return &DrawMD2;
    }

    return NULL;
}
