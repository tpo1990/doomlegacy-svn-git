// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_plane.h 1361 2017-10-16 16:26:45Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2012 by DooM Legacy Team.
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
// $Log: r_plane.h,v $
// Revision 1.8  2001/05/30 04:00:52  stroggonmeth
// Fixed crashing bugs in software with 3D floors.
//
// Revision 1.7  2001/03/21 18:24:39  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.6  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.5  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.4  2000/04/13 23:47:47  stroggonmeth
// See logs
//
// Revision 1.3  2000/04/04 00:32:48  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Refresh, visplane stuff (floor, ceilings).
//
//-----------------------------------------------------------------------------

#ifndef R_PLANE_H
#define R_PLANE_H

#include "doomtype.h"
#include "screen.h"
  // MAXVIDWIDTH, MAXVIDHEIGHT
#include "r_data.h"
#include "m_fixed.h"
#include "d_player.h"
  // player_t

// unsigned short invalid for top[], bottom[] arrays in visplane
// Maximum value disables use in top calculations, it is off bottom of screen.
#define TOP_MAX   0xffff

//
// Now what is a visplane, anyway?
// Simple : kinda floor/ceiling polygon optimised for Doom rendering.
// 4124 bytes!
//
typedef struct visplane_s
{
  //SoM: 3/17/2000
  struct visplane_s*    next;  // linked in hash table

  fixed_t               height;
  fixed_t               viewz;
  angle_t               viewangle;
  lightlev_t            lightlevel;
  int                   picnum;
  int                   minx, maxx;

  //SoM: 4/3/2000: Colormaps per sector!
  extracolormap_t*      extra_colormap;

  //faB: words sucks .. should get rid of that.. but eats memory
  //added:08-02-98: THIS IS UNSIGNED! VERY IMPORTANT!!
  unsigned short         pad1;   // leave pads for [minx-1] and [maxx+1]
  unsigned short         top[MAXVIDWIDTH];  // screen coord of top edge
  unsigned short         pad2;
  unsigned short         pad3;
  unsigned short         bottom[MAXVIDWIDTH];  // screen coord of bottom edge
  unsigned short         pad4;

  // SoM: [WDJ] highest top and lowest bottom as found by R_PlaneBounds
  // Set and used only in R_CreateDrawNodes
  int                    highest_top, lowest_bottom;

  fixed_t xoffs, yoffs;  // SoM: 3/6/2000: Scrolling flats.

  // SoM: frontscale should be stored in the first seg of the subsector
  // where the planes themselves are stored. I'm doing this now because
  // the old way caused trouble with the drawseg array was re-sized.
//  int    scaleseg;

  struct ffloor_s* ffloor;  // ffloor_t, when derived from fake floor
} visplane_t;


// [WDJ] visplane_t global parameters  vsp
// visplane used for drawing in r_bsp and r_segs
extern visplane_t*    vsp_floorplane;
extern visplane_t*    vsp_ceilingplane;


// Visplane related.
extern  short*          lastopening;

typedef void (*planefunction_t) (int top, int bottom);

extern planefunction_t  floorfunc;
extern planefunction_t  ceilingfunc_t;

extern short            floorclip[MAXVIDWIDTH];
extern short            ceilingclip[MAXVIDWIDTH];
//extern short            waterclip[MAXVIDWIDTH];   //added:18-02-98:WATER!
extern fixed_t          backscale[MAXVIDWIDTH];
extern fixed_t          yslopetab[MAXVIDHEIGHT*4];

extern fixed_t*         yslope;
extern fixed_t          distscale[MAXVIDWIDTH];

void R_Init_Planes (void);
void R_Clear_Planes (player_t *player);
void R_Draw_Planes (void);

// Draw plane span at row y, span=(x1..x2)
// at planeheight, using spanfunc
void R_MapPlane ( int y, int x1, int x2 );

// Draw plane spans at rows (t1..b1), span=(spanstart..x-1)
// and Setup spanstart for next span at rows (t2..b2).
// Param t1,b1,t2,b2 are y values.
void R_MakeSpans ( int x, int t1, int b1, int t2, int b2 );

visplane_t* R_FindPlane( fixed_t height,
                         int     picnum,
                         int     lightlevel,
                         fixed_t xoff,
                         fixed_t yoff,
                         extracolormap_t* planecolormap,
                         ffloor_t* ffloor);

// return visplane or alloc a new one if needed
visplane_t* R_CheckPlane ( visplane_t*  pl, int start, int stop );

void R_ExpandPlane(visplane_t*  pl, int start, int stop);

// SoM: Draws a single visplane.
void R_DrawSinglePlane(visplane_t* pl);
void R_PlaneBounds(visplane_t* plane);


typedef struct ff_planemgr_s
{
  visplane_t*  plane;
  fixed_t      height;
  boolean      valid_mark;
  fixed_t      front_pos;  // Front sector
  fixed_t      back_pos;   // Back sector
  fixed_t      front_frac;	// from front_pos and scale
  fixed_t      front_step;
  fixed_t      back_frac;	// from back_pos and scale
  fixed_t      back_step;
  short        front_clip[MAXVIDWIDTH];
  short        con_clip[MAXVIDWIDTH];	// console clipping

  struct ffloor_s  *ffloor;
} ff_planemgr_t;

extern ff_planemgr_t  ffplane[MAXFFLOORS];
extern int           numffplane;
#endif
