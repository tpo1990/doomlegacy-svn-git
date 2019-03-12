// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_segs.c 1429 2019-02-11 21:41:27Z wesleyjohnson $
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
// $Log: r_segs.c,v $
// Revision 1.32  2003/05/04 04:15:09  sburke
// Wrap patch->width, patch->height references in SHORT for big-endian machines.
//
// Revision 1.31  2002/09/25 16:38:35  ssntails
// Alpha support for trans 3d floors in software
//
// Revision 1.30  2002/01/12 02:21:36  stroggonmeth
//
// Revision 1.29  2001/08/29 18:58:57  hurdler
//
// Revision 1.28  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.27  2001/05/30 04:00:52  stroggonmeth
// Fixed crashing bugs in software with 3D floors.
//
// Revision 1.26  2001/05/27 13:42:48  bpereira
//
// Revision 1.25  2001/04/30 17:19:24  stroggonmeth
// HW fix and misc. changes
//
// Revision 1.24  2001/03/21 18:24:39  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.23  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.22  2001/02/24 13:35:21  bpereira
// Revision 1.21  2000/11/26 01:02:27  hurdler
// Revision 1.20  2000/11/25 18:41:21  stroggonmeth
//
// Revision 1.19  2000/11/21 21:13:18  stroggonmeth
// Optimised 3D floors and fixed crashing bug in high resolutions.
//
// Revision 1.18  2000/11/14 16:23:16  hurdler
// Revision 1.17  2000/11/09 17:56:20  stroggonmeth
// Revision 1.16  2000/11/03 03:27:17  stroggonmeth
// Revision 1.15  2000/11/02 19:49:36  bpereira
//
// Revision 1.14  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.13  2000/09/28 20:57:17  bpereira
// Revision 1.12  2000/04/30 10:30:10  bpereira
// Revision 1.11  2000/04/18 17:39:40  stroggonmeth
// Revision 1.10  2000/04/16 18:38:07  bpereira
// Revision 1.9  2000/04/15 22:12:58  stroggonmeth
//
// Revision 1.8  2000/04/13 23:47:47  stroggonmeth
// See logs
//
// Revision 1.7  2000/04/08 17:29:25  stroggonmeth
//
// Revision 1.6  2000/04/06 21:06:19  stroggonmeth
// Optimized extra_colormap code...
// Added #ifdefs for older water code.
//
// Revision 1.5  2000/04/05 15:47:47  stroggonmeth
// Added hack for Dehacked lumps. Transparent sprites are now affected by colormaps.
//
// Revision 1.4  2000/04/04 19:28:43  stroggonmeth
// Global colormaps working. Added a new linedef type 272.
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
//      All the clipping: columns, horizontal spans, sky columns.
//
//-----------------------------------------------------------------------------

#include <stddef.h>

#include "doomincl.h"
#include "r_local.h"
#include "r_sky.h"

#include "r_splats.h"           //faB: testing

#include "w_wad.h"
#include "z_zone.h"
#include "d_netcmd.h"
#include "p_local.h" //Camera...
#include "console.h" //Con_clipviewtop

// Light added for wall orientation (in 0..255 scale)
#define ORIENT_LIGHT   16

// OPTIMIZE: closed two sided lines as single sided

// True if any of the segs textures might be visible.
static boolean         segtextured;
static boolean         markfloor; // False if the back side is the same plane.
static boolean         markceiling;

static boolean         maskedtexture;
// maskedtexture can use transparent patches
// Only single-sided linedefs use midtexture, 2-sided sets maskedtexture instead.

// toptexture, bottomtexture, midtexture will call draw routines that do not
// look for the post structure of patches.  They assume a full column of pixels,
// without transparent areas, such as TM_picture.
// They can use TM_combine or TM_patch only where there is a single full
// height post per column.
// Violation of this (by the wad) will give tutti-frutti colors.

// texture num, 0=no-texture, otherwise is a valid texture index
static int             toptexture;
static int             bottomtexture;
static int             midtexture;  // single-sided only

static int             numthicksides;
//static short*          thicksidecol;


angle_t         rw_normalangle;
// angle to line origin
int             rw_angle1;
fixed_t         rw_distance;

//
// regular wall
//
static int             rw_x;
static int             rw_stopx;
static angle_t         rw_centerangle;
static fixed_t         rw_offset;
static fixed_t         rw_offset2; // for splats

static fixed_t         rw_scale;
static fixed_t         rw_scalestep;
static fixed_t         rw_midtexturemid;
static fixed_t         rw_toptexturemid;
static fixed_t         rw_bottomtexturemid;

// [WDJ] 2/22/2010 actually is fixed_t in all usage
static fixed_t         worldtop;	// front sector
static fixed_t         worldbottom;
static fixed_t         worldbacktop;	// back sector, only used on two sided lines
static fixed_t         worldbackbottom;

// RenderSegLoop global parameters
static fixed_t         pixhigh;
static fixed_t         pixlow;
static fixed_t         pixhighstep;
static fixed_t         pixlowstep;

static fixed_t         topfrac;
static fixed_t         topstep;

static fixed_t         bottomfrac;
static fixed_t         bottomstep;

lighttable_t**  walllights;  // array[] of colormap selected by lightlevel

short*          maskedtexturecol;


// ==========================================================================
// R_Splats Wall Splats Drawer
// ==========================================================================

#ifdef WALLSPLATS
#define BORIS_FIX
#ifdef BORIS_FIX
static short last_ceilingclip[MAXVIDWIDTH];
static short last_floorclip[MAXVIDWIDTH];
#endif

// Called by R_DrawWallSplats
static void R_DrawSplatColumn (column_t* column)
{
    fixed_t     top_post_sc, bottom_post_sc;  // fixed_t screen coord.
    fixed_t     basetexturemid = dc_texturemid;  // save to restore later

    // dc_x is limited to 0..rdraw_viewwidth by caller x1,x2
//    if ( (unsigned) dc_x >= rdraw_viewwidth )   return;
#ifdef RANGECHECK
    if ( (unsigned) dc_x >= rdraw_viewwidth )   return;
        I_Error ("R_DrawSplatColumn dc_x: %i\n", dc_x);
#endif

    // over all column posts for this column
    for ( ; column->topdelta != 0xff ; )
    {
        // calculate unclipped screen coordinates
        //  for post
        top_post_sc = dm_top_patch + dm_yscale*column->topdelta;
        bottom_post_sc = top_post_sc + dm_yscale*column->length;

        // fixed_t to int screen coord.
        dc_yl = (top_post_sc+FRACUNIT-1)>>FRACBITS;
        dc_yh = (bottom_post_sc-1)>>FRACBITS;


#ifndef BORIS_FIX
        if (dc_yh >= dm_floorclip[dc_x])
            dc_yh = dm_floorclip[dc_x] - 1;
        if (dc_yl < dm_ceilingclip[dc_x])
            dc_yl = dm_ceilingclip[dc_x] + 1;
#else
        if (dc_yh >= last_floorclip[dc_x])
            dc_yh =  last_floorclip[dc_x]-1;
        if (dc_yl <= last_ceilingclip[dc_x])
            dc_yl =  last_ceilingclip[dc_x]+1;
#endif
        //[WDJ] phobiata.wad has many views that need clipping
        if ( dc_yl < 0 ) dc_yl = 0;
        if ( dc_yh >= rdraw_viewheight )   dc_yh = rdraw_viewheight - 1;
        if (dc_yl <= dc_yh && dc_yh >= 0 )
        {
            dc_source = (byte *)column + 3;
            dc_texturemid = basetexturemid - (column->topdelta<<FRACBITS);
            
            //debug_Printf("l %d h %d %d\n",dc_yl,dc_yh, column->length);
            // Drawn by either R_DrawColumn
            //  or (SHADOW) R_DrawFuzzColumn.
            colfunc ();
        }
        column = (column_t *)(  (byte *)column + column->length + 4);
    }

    dc_texturemid = basetexturemid;
}


// Draw splats for a lineseg.
// Caller sets frontsector.
static void R_DrawWallSplats (void)
{
    wallsplat_t*    splat;
//    seg_t*      seg;
    angle_t     angle1, angle2;
    int         angf;
    int         x1, x2;
    column_t*   col;
    patch_t*    patch;
    lighttable_t  * ro_colormap = NULL;  // override colormap
    fixed_t     texturecolumn;

    splat = (wallsplat_t*) linedef->splats;

#ifdef PARANOIA
    if (!splat)
        I_Error ("R_DrawWallSplats: splat is NULL");
#endif

//    seg = ds_p->curline;

    // [WDJ] Initialize dc_colormap.
    // If fixedcolormap == NULL, then the loop will scale the light and colormap.
    dc_colormap = fixedcolormap;
    // [WDJ] Fixed determinations, taken out of draw loop.
    // Overrides of colormap, with same usage.
    if( fixedcolormap )
        ro_colormap = fixedcolormap;
    else if( view_colormap )
        ro_colormap = view_colormap;
    else if( frontsector->extra_colormap )  // over the whole line
        ro_colormap = frontsector->extra_colormap->colormap;

    // draw all splats from the line that touches the range of the seg
    for ( ; splat ; splat=splat->next)
    {
        angle1 = R_PointToAngle (splat->v1.x, splat->v1.y);
        angle2 = R_PointToAngle (splat->v2.x, splat->v2.y);
#if 0
        if (angle1>clipangle)
            angle1=clipangle;
        if (angle2>clipangle)
            angle2=clipangle;
        if ((int)angle1<-(int)clipangle)
            angle1=-clipangle;
        if ((int)angle2<-(int)clipangle)
            angle2=-clipangle;
        int angf1 = ANGLE_TO_FINE(angle1 - viewangle + ANG90);
        int angf2 = ANGLE_TO_FINE(angle2 - viewangle + ANG90);
#else
        int angf1 = ANGLE_TO_FINE(angle1 - viewangle + ANG90);
        int angf2 = ANGLE_TO_FINE(angle2 - viewangle + ANG90);
        // BP: out of the viewangle_to_x lut, TODO clip it to the screen
        if( angle1 > FINE_ANG180 || angle2 > FINE_ANG180)
            continue;
#endif
        // viewangle_to_x table is limited to (0..rdraw_viewwidth)
        x1 = viewangle_to_x[angf1];
        x2 = viewangle_to_x[angf2];

        if (x1 >= x2)
            continue;                         // smaller than a pixel

        // splat is not in this seg range
        if (x2 < ds_p->x1 || x1 > ds_p->x2)
            continue;

        if (x1 < ds_p->x1)
            x1 = ds_p->x1;
        if (x2 > ds_p->x2)
            x2 = ds_p->x2;
        if( x2<=x1 )
            continue;

        // calculate incremental stepping values for texture edges
        rw_scalestep = ds_p->scalestep;
        dm_yscale = ds_p->scale1 + (x1 - ds_p->x1)*rw_scalestep;
        dm_floorclip = floorclip;
        dm_ceilingclip = ceilingclip;

        patch = W_CachePatchNum (splat->patch, PU_CACHE); // endian fix

        // clip splat range to seg range left
        /*if (x1 < ds_p->x1)
        {
            dm_yscale += (rw_scalestep * (ds_p->x1 - x1));
            x1 = ds_p->x1;
        }*/
        // clip splat range to seg range right


        // SoM: This is set already. THIS IS WHAT WAS CAUSING PROBLEMS WITH
        // BOOM WATER!
        // frontsector = ds_p->curline->frontsector;

        // [WDJ] FIXME: ?? top of texture, plus 1/2 height ?, relative to viewer
        // So, either splat->top is not top, or something else weird.
        // This is world coord.
        dc_texturemid = splat->top + (patch->height<<(FRACBITS-1)) - viewz;
        if( splat->yoffset )
            dc_texturemid += *splat->yoffset;

        // top of splat, screen coord.
        dm_top_patch = centeryfrac - FixedMul(dc_texturemid,dm_yscale);

        // set drawing mode for single sided textures
        switch (splat->flags & SPLATDRAWMODE_MASK)
        {
            case SPLATDRAWMODE_OPAQUE:
                colfunc = basecolfunc;
                break;
            case SPLATDRAWMODE_TRANS:
                if( cv_translucency.value == 0 )
                    colfunc = basecolfunc;
                else
                {
                    dr_alpha = 128;  // use normal dc_translucent_index
                    dc_translucent_index = TRANSLU_med;
                    dc_translucentmap = & translucenttables[ TRANSLU_TABLE_med ];
                    colfunc = transcolfunc;
                }
    
                break;
            case SPLATDRAWMODE_SHADE:
                colfunc = shadecolfunc;
                break;
        }

        dc_texheight = 0;

        // draw the columns
        // x1,x2 are already limited to 0..rdraw_viewwidth
        for (dc_x = x1 ; dc_x <= x2 ; dc_x++, dm_yscale += rw_scalestep)
        {
            if( !fixedcolormap )
            {
                // distance effect on light, yscale is smaller at distance.
                unsigned  dlit = dm_yscale>>LIGHTSCALESHIFT;
                if (dlit >=  MAXLIGHTSCALE )
                   dlit = MAXLIGHTSCALE-1;

                dc_colormap = walllights[dlit];
                if( ro_colormap )
                {
                    // reverse indexing, and change to extra_colormap
                    int lightindex = dc_colormap - reg_colormaps;
                    dc_colormap = & ro_colormap[ lightindex ];
                }
            }

            dm_top_patch = centeryfrac - FixedMul(dc_texturemid, dm_yscale);
            dc_iscale = 0xffffffffu / (unsigned)dm_yscale;

            // find column of patch, from perspective
            angf = ANGLE_TO_FINE(rw_centerangle + x_to_viewangle[dc_x]);
            texturecolumn = rw_offset2 - splat->offset - FixedMul(finetangent[angf],rw_distance);

            //texturecolumn &= 7;
            //DEBUG

            // FIXME !
//            CONS_Printf ("%.2f width %d, %d[x], %.1f[off]-%.1f[soff]-tg(%d)=%.1f*%.1f[d] = %.1f\n", 
//                         FIXED_TO_FLOAT(texturecolumn), patch->width,
//                         dc_x,FIXED_TO_FLOAT(rw_offset2),FIXED_TO_FLOAT(splat->offset),angf,FIXED_TO_FLOAT(finetangent[angf]),FIXED_TO_FLOAT(rw_distance),FIXED_TO_FLOAT(FixedMul(finetangent[angf],rw_distance)));
            texturecolumn >>= FRACBITS;
            if (texturecolumn < 0 || texturecolumn >= patch->width) 
                continue;

            // draw the texture
            col = (column_t *) ((byte *)patch + patch->columnofs[texturecolumn]);
            R_DrawSplatColumn (col);

        }

    }// next splat

    colfunc = basecolfunc;
}

#endif //WALLSPLATS


// ==========================================================================
// Lightlist and Openings
// [WDJ] separate functions for expand of lists, with error handling

void  expand_lightlist( void )
{
    struct r_lightlist_s *  newlist = 
        realloc(dc_lightlist, sizeof(r_lightlist_t) * dc_numlights);

    if( newlist )
    {
        dc_lightlist = newlist;
        dc_maxlights = dc_numlights;
    }
    else
    {
        // non-fatal protection, allow savegame
        // realloc fail does not disturb existing allocation
        dc_numlights = dc_maxlights;
        I_SoftError( "Expand lightlist realloc failed.\n" );
    }
}


extern short *openings;
extern size_t maxopenings;

void  expand_openings( size_t  need )
{
    size_t lastindex = lastopening - openings;
    drawseg_t *ds;  //needed for fix from *cough* zdoom *cough*
    uintptr_t  adjustdiff;
   
    if( maxopenings < 1024 )
        maxopenings = 16384;
    while (need > maxopenings)
        maxopenings *= 2;
    short * newopenings = realloc(openings, maxopenings * sizeof(*openings));
    if( newopenings == NULL )
    {
        I_Error( "Failed realloc for openings\n" );
    }
    adjustdiff = (void*)newopenings - (void*)openings; // byte difference in locations
   
    // borrowed fix from *cough* zdoom *cough*
    // [RH] We also need to adjust the openings pointers that
    //    were already stored in drawsegs.
    for (ds = drawsegs; ds < ds_p; ds++)
    {
#define ADJUST(p) if (ds->p + ds->x1 >= openings && ds->p + ds->x1 <= lastopening)\
                        ds->p = ((void*) ds->p) + adjustdiff;
        ADJUST (maskedtexturecol);
        ADJUST (spr_topclip);
        ADJUST (spr_bottomclip);
        ADJUST (thicksidecol);
    }
#undef ADJUST
    openings = newopenings;
    lastopening = & openings[ lastindex ];
}


void expand_drawsegs( void )
{
    // drawsegs is NULL on first execution
    // Realloc larger drawseg memory, and adjust old drawseg ptrs
    drawseg_t * old_drawsegs = drawsegs;
    unsigned newmax = maxdrawsegs ? maxdrawsegs*2 : 128;
    drawseg_t * new_drawsegs = realloc(drawsegs, newmax*sizeof(*drawsegs));
    if( new_drawsegs == 0 )
    {
        I_Error( "Failed realloc for drawsegs\n" );
    }
    drawsegs = new_drawsegs;
    maxdrawsegs = newmax;
    // Adjust ptrs by adding the difference in drawseg area position
    // [WDJ] Avoid divide and mult by sizeof(drawsegs) by using void* difference
    // If NULL, then point to drawsegs after first alloc.
    ptrdiff_t  drawsegs_diff = (void*)drawsegs - (void*)old_drawsegs;
    ds_p = (drawseg_t*)((void*)ds_p + drawsegs_diff);
    firstnewseg = (drawseg_t*)((void*)firstnewseg + drawsegs_diff);
    if (firstseg)  // if NULL then keep it NULL
        firstseg = (drawseg_t*)((void*)firstseg + drawsegs_diff);
}



// ==========================================================================
// R_RenderMaskedSegRange
// ==========================================================================

// If we have a multi-patch texture on a 2sided wall (rare) then we draw
//  it using R_DrawColumn, else we draw it using R_DrawMaskedColumn, this
//  way we don't have to store extra post_t info with each column for
//  multi-patch textures. They are not normally needed as multi-patch
//  textures don't have holes in it. At least not for now.
static int  column2s_length;     // column->length : for multi-patch on 2sided wall = texture->height

// The colfunc_2s function for TM_picture
void R_Render2sidedMultiPatchColumn (column_t* column)
{
    fixed_t  top_post_sc, bottom_post_sc; // patch on screen, fixed_t screen coords.

    if ( (unsigned) dc_x >= rdraw_viewwidth )   return;
   
    top_post_sc = dm_top_patch; // + dm_yscale*column->topdelta;  topdelta is 0 for the wall
    bottom_post_sc = top_post_sc + dm_yscale * column2s_length;

    // set y bounds to patch bounds, unless there is window
    dc_yl = (dm_top_patch+FRACUNIT-1)>>FRACBITS;
    dc_yh = (bottom_post_sc-1)>>FRACBITS;

    if(dm_windowtop != FIXED_MAX && dm_windowbottom != FIXED_MAX)
    {
      dc_yl = ((dm_windowtop + FRACUNIT) >> FRACBITS);
      dc_yh = (dm_windowbottom - 1) >> FRACBITS;
    }

    {
      if (dc_yh >= dm_floorclip[dc_x])
          dc_yh =  dm_floorclip[dc_x]-1;
      if (dc_yl <= dm_ceilingclip[dc_x])
          dc_yl =  dm_ceilingclip[dc_x]+1;
    }

    // [WDJ] Draws only within borders
    if (dc_yl >= rdraw_viewheight || dc_yh < 0)
      return;

    //[WDJ] phobiata.wad has many views that need clipping
    if ( dc_yl < 0 )   dc_yl = 0;
    if ( dc_yh >= rdraw_viewheight )   dc_yh = rdraw_viewheight - 1;
    if (dc_yl <= dc_yh)
    {
        dc_source = (byte *)column + 3;
        colfunc ();
    }
}


// Render with fog, translucent, and transparent, over range x1..x2
// Called from R_DrawMasked.
void R_RenderMaskedSegRange( drawseg_t* ds, int x1, int x2 )
{
    column_t*       col;
    lightlev_t      vlight;  // visible light 0..255
    lightlev_t      orient_light = 0;  // wall orientation effect
    int             texnum;
    int             i;
    fixed_t	    windowclip_top, windowclip_bottom;
    fixed_t         lightheight;
    fixed_t         realbot;
    // Setup lightlist for all 3dfloors, then use it over all x.
    r_lightlist_t * rlight;  // dc_lightlist
    ff_light_t    * ff_light;  // ffloor lightlist item
    lighttable_t  * ro_colormap = NULL;  // override colormap

    void (*colfunc_2s) (column_t*);

    line_t* ldef;   //faB

    // Calculate light table.
    // Use different light tables
    //   for horizontal / vertical / diagonal. Diagonal?
    curline = ds->curline;
    frontsector = curline->frontsector;
    backsector = curline->backsector;
    if (curline->v1->y == curline->v2->y)
        orient_light = -ORIENT_LIGHT;
    else if (curline->v1->x == curline->v2->x)
        orient_light = ORIENT_LIGHT;

    // midtexture, 0=no-texture, otherwise valid
    texnum = texturetranslation[curline->sidedef->midtexture];

    dm_windowbottom = dm_windowtop = dm_bottom_patch = FIXED_MAX; // default no clip
    windowclip_top = windowclip_bottom = FIXED_MAX;

    // Select the default, or special effect column drawing functions,
    // which are called by the colfunc_2s functions.

    //faB: hack translucent linedef types (201-205 for translucenttables 1-5)
    //SoM: 201-205 are taken... So I'm switching to 284 - 288
    ldef = curline->linedef;
    if (ldef->special>=284 && ldef->special<=288)  // Legacy translucents
    {
        dc_translucent_index = ldef->special-284+1;
        dc_translucentmap = & translucenttables[ ((ldef->special-284)<<FF_TRANSSHIFT) ];
        dr_alpha = 128; // use normal dc_translucent, all tables are < TRANSLU_REV_ALPHA
        colfunc = transcolfunc;
    }
    else
    if (ldef->special==260 || ldef->translu_eff )  // Boom make translucent
    {
        // Boom 260, make translucent, direct and by tags
        dc_translucent_index = TRANSLU_med; // 50/50
        dc_translucentmap = & translucenttables[TRANSLU_TABLE_INDEX(TRANSLU_med)]; // get transtable 50/50
        dr_alpha = 128;
        if( ldef->translu_eff >= TRANSLU_ext )
        {
            // Boom transparency map
            translucent_map_t * tm = & translu_store[ ldef->translu_eff - TRANSLU_ext ];
#if 0
            if( vid.drawmode == DRAW8PAL )
            {
                // dc_translucentmap only works on DRAW8PAL
                dc_translucentmap = W_CacheLumpNum( tm->translu_lump_num, PU_CACHE );
            }
#else
            // dc_translucentmap required for DRAW8PAL,
            // but is used indirectly in other draw modes, for some TRANSLU.
            dc_translucentmap = W_CacheLumpNum( tm->translu_lump_num, PU_CACHE );
#endif
            // for other draws
            dc_translucent_index = tm->substitute_std_translucent;
        }
        colfunc = transcolfunc;
    }
    else
    if (ldef->special==283)	// Legacy Fog sheet
    {
        // Display fog sheet (128 high) as transparent middle texture.
        // Only where there is a middle texture (in place of it).
        colfunc = fogcolfunc; // R_DrawFogColumn_8 16 ..
        fog_col_length = (textures[texnum]->texture_model == TM_masked)? 2: textures[texnum]->height;
        fog_index = fog_tic % fog_col_length;  // pixel selection
        fog_init = 1;
        dr_alpha = 64;  // default
        // [WDJ] clip at ceiling and floor, unlike other transparent texture
        // world coord, relative to viewer
        windowclip_top = frontsector->ceilingheight - viewz;
        windowclip_bottom = frontsector->floorheight - viewz;
    }
    else
        colfunc = basecolfunc;

    // Select the 2s draw functions, they are called later.
    //faB: handle case where multipatch texture is drawn on a 2sided wall, multi-patch textures
    //     are not stored per-column with post info anymore in Doom Legacy
    // [WDJ] multi-patch transparent texture restored
  retry_texture_model:
    switch (textures[texnum]->texture_model)
    {
     case TM_patch:
        colfunc_2s = R_DrawMaskedColumn;                    //render the usual 2sided single-patch packed texture
        break;
     case TM_combine_patch:
        colfunc_2s = R_DrawMaskedColumn;                    //render combined as 2sided single-patch packed texture
        break;
     case TM_picture:    
        colfunc_2s = R_Render2sidedMultiPatchColumn;        //render multipatch with no holes (no post_t info)
        column2s_length = textures[texnum]->height;
        break;
     case TM_masked:
     case TM_none:
        R_GenerateTexture( texnum );	// first time
        goto retry_texture_model;
     default:
        return;	// no draw routine
    }

    rw_scalestep = ds->scalestep;
    dm_yscale = ds->scale1 + (x1 - ds->x1)*rw_scalestep;

    // Setup lighting based on the presence/lack-of 3D floors.
    dc_numlights = frontsector->numlights;
    if( dc_numlights )
    {
      if(dc_numlights >= dc_maxlights)   expand_lightlist();

      // setup lightlist
      // highest light to lowest light, [0] is sector light at top
      for(i = 0; i < dc_numlights; i++)
      {
        // setup a lightlist entry
        ff_light = &frontsector->lightlist[i];
        rlight = &dc_lightlist[i];  // create in this list slot

        // fake floor light heights in screen coord.
        rlight->height = (centeryfrac) - FixedMul((ff_light->height - viewz), dm_yscale);
        rlight->heightstep = -FixedMul (rw_scalestep, (ff_light->height - viewz));
        rlight->lightlevel = *ff_light->lightlevel;
        rlight->extra_colormap = ff_light->extra_colormap;
        rlight->flags = ff_light->flags;

#if 0
        // [WDJ] When NOSHADE, the light is not used.
        // Questionable if really worth it, for the few times it could skip the light setup.
        if( ff_light->flags & FF_NOSHADE )
           continue; // next 3dfloor light
#endif

        if(rlight->flags & FF_FOG)
          vlight = rlight->lightlevel + extralight_fog;
        else if(rlight->extra_colormap && rlight->extra_colormap->fog)
          vlight = rlight->lightlevel + extralight_cm;
        else if(colfunc == transcolfunc)
          vlight = 255 + orient_light;
        else
          vlight = rlight->lightlevel + extralight + orient_light;

        rlight->vlightmap =
            (vlight < 0) ? scalelight[0]
          : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
          : scalelight[vlight>>LIGHTSEGSHIFT];

        // [WDJ] Fixed determinations, taken out of draw loop.
        // Overrides of colormap, with same usage.
        if( fixedcolormap )
            rlight->rcolormap = fixedcolormap;
        else if( view_extracolormap )
            rlight->extra_colormap = view_extracolormap;
      }  // for
    }
    else
    {
      // frontsector->numlights == 0
      if(colfunc == fogcolfunc) // Legacy Fog sheet
        vlight = frontsector->lightlevel + extralight_fog;
      else if(frontsector->extra_colormap && frontsector->extra_colormap->fog)
        vlight = frontsector->lightlevel + extralight_cm;
      else if(colfunc == transcolfunc)  // Translucent 
        vlight = 255 + orient_light;
      else
        vlight = frontsector->lightlevel + extralight + orient_light;

      walllights =
          (vlight < 0) ? scalelight[0]
        : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
        : scalelight[vlight>>LIGHTSEGSHIFT];
       
      // [WDJ] Fixed determinations, taken out of loop.
      // Overrides of colormap, with same usage.
      if( fixedcolormap )
        ro_colormap = fixedcolormap;
      else if( view_colormap )
        ro_colormap = view_colormap;
      else if( frontsector->extra_colormap )
        ro_colormap = frontsector->extra_colormap->colormap;
    }

    maskedtexturecol = ds->maskedtexturecol;

    dm_floorclip = ds->spr_bottomclip;
    dm_ceilingclip = ds->spr_topclip;

    if (curline->linedef->flags & ML_DONTPEGBOTTOM)
    {
        // highest floor
        dc_texturemid =
         (frontsector->floorheight > backsector->floorheight) ?
            frontsector->floorheight : backsector->floorheight;
        // top of texture, relative to viewer
        dc_texturemid = dc_texturemid + textureheight[texnum] - viewz;
    }
    else
    {
        // lowest ceiling
        dc_texturemid =
         (frontsector->ceilingheight < backsector->ceilingheight) ?
           frontsector->ceilingheight : backsector->ceilingheight;
        // top of texture, relative to viewer
        dc_texturemid = dc_texturemid - viewz;
    }
    // top of texture, relative to viewer, with rowoffset, world coord.
    dc_texturemid += curline->sidedef->rowoffset;

    dc_texheight = textureheight[texnum] >> FRACBITS;

    // [WDJ] Initialize dc_colormap.
    // If fixedcolormap == NULL, then the loop will scale the light and colormap.
    dc_colormap = fixedcolormap;

    // draw the columns
    // [WDJ] x1,x2 are limited to 0..rdraw_viewwidth to protect [dc_x] access.
#ifdef RANGECHECK
    if( x1 < 0 || x2 >= rdraw_viewwidth )
       I_Error( "R_RenderMaskedSegRange: %i  %i\n", x1, x2);
#endif
    if( x1 < 0 )  x1 = 0;
    if( x2 >= rdraw_viewwidth )  x2 = rdraw_viewwidth-1;
    for (dc_x = x1 ; dc_x <= x2 ; dc_x++)
    {
        if (maskedtexturecol[dc_x] != MAXSHORT)
        {
          // if not masked
          // calculate 3Dfloor lighting
          if(dc_numlights)
          {
            // Where there are 3dfloors ...
            dm_bottom_patch = FIXED_MAX;
            // top/bottom of texture, relative to viewer, screen coord.
            dm_top_patch = dm_windowtop = (centeryfrac - FixedMul(dc_texturemid, dm_yscale));
            realbot = dm_windowbottom = FixedMul(textureheight[texnum], dm_yscale) + dm_top_patch;
            dc_iscale = 0xffffffffu / (unsigned)dm_yscale;
            
            col = (column_t *)((byte *)R_GetColumn(texnum,maskedtexturecol[dc_x]) - 3);

            // top floor colormap, or fixedcolormap
            dc_colormap = dc_lightlist[0].rcolormap;

            // for each 3Dfloor light
            // highest light to lowest light, [0] is sector light at top
            for(i = 0; i < dc_numlights; i++)
            {
              rlight = &dc_lightlist[i];

              if((rlight->flags & FF_NOSHADE))
                continue; // next 3dfloor light

              if( !fixedcolormap )
              {
                 // distance effect on light, yscale is smaller at distance.
                 unsigned dlit = dm_yscale>>LIGHTSCALESHIFT;
                 if (dlit >=  MAXLIGHTSCALE )
                     dlit = MAXLIGHTSCALE-1;

                 // light table for the distance
                 rlight->rcolormap = rlight->vlightmap[dlit];
                 if( rlight->extra_colormap )
                 {
                     // reverse indexing, and change to extra_colormap
                     int lightindex = rlight->rcolormap - reg_colormaps;
                     rlight->rcolormap = & rlight->extra_colormap->colormap[ lightindex ];
                 }
              }

              rlight->height += rlight->heightstep;

              lightheight = rlight->height;
              if(lightheight <= dm_windowtop)
              {
                // above view window, just get the colormap
                dc_colormap = rlight->rcolormap;
                continue;  // next 3dfloor light
              }

              // actual drawing using col and colfunc, between 3Dfloors
              dm_windowbottom = lightheight;
              if(dm_windowbottom >= realbot)
              {
                // past bottom of view window
                dm_windowbottom = realbot;
                colfunc_2s (col);

                // Finish dc_lightlist height adjustments.
                // highest light to lowest light, [0] is sector light at top
                for(i++ ; i < dc_numlights; i++)
                {
                  rlight = &dc_lightlist[i];
                  rlight->height += rlight->heightstep;
                }
                goto next_x;
              }  // if( dm_windowbottom >= realbot )

              colfunc_2s (col);

              // for next draw, downward from this light height
              dm_windowtop = dm_windowbottom + 1;
              dc_colormap = rlight->rcolormap;
            } // for( dc_numlights )
            // draw down to sector floor
            dm_windowbottom = realbot;
            if(dm_windowtop < dm_windowbottom)
              colfunc_2s (col);

          next_x:
            dm_yscale += rw_scalestep;
            continue;  // next x
          }  // if( dc_numlights )


          // Where there are no 3Dfloors ...
          // calculate lighting for distance using dm_yscale
          if (!fixedcolormap)
          {
              // distance effect on light, yscale is smaller at distance.
              unsigned dlit = dm_yscale>>LIGHTSCALESHIFT;
              if (dlit >=  MAXLIGHTSCALE )
                 dlit = MAXLIGHTSCALE-1;

              // light table for the distance
              dc_colormap = walllights[dlit];
              if( ro_colormap )
              {
                 // reverse indexing, and change to extra_colormap
                 int lightindex = dc_colormap - reg_colormaps;
                 dc_colormap = & ro_colormap[ lightindex ];
              }
          } // fixedcolormap

          if( windowclip_top != FIXED_MAX )
          {
            // fog sheet clipping to ceiling and floor
            dm_windowtop = centeryfrac - FixedMul(windowclip_top, dm_yscale);
            dm_windowbottom = centeryfrac - FixedMul(windowclip_bottom, dm_yscale);
          }

          // top of texture, screen coord.
          dm_top_patch = centeryfrac - FixedMul(dc_texturemid, dm_yscale);
          dc_iscale = 0xffffffffu / (unsigned)dm_yscale;

          // draw texture, as clipped
          col = (column_t *)(
                (byte *)R_GetColumn(texnum,maskedtexturecol[dc_x]) - 3);
            
          colfunc_2s (col);
        } // if (maskedtexturecol[dc_x] != MAXSHORT)
        dm_yscale += rw_scalestep;
    } // for( dx_x = x1..x2 )
    colfunc = basecolfunc;
}





//
// R_RenderThickSideRange
// Renders all the thick sides for 3dfloors.
//  x1, x2 : the x range of the seg, to be rendered
//  ffloor : the fake-floor whose thick side is to be rendered
// Called by R_DrawMasked.
void R_RenderThickSideRange( drawseg_t* ds, int x1, int x2, ffloor_t* ffloor)
{
    column_t*       col;
    lightlev_t      vlight;  // visible light 0..255
    lightlev_t      orient_light = 0;  // wall orientation effect
    int             texnum;
    sector_t        tempsec;
    int             base_fog_alpha;
    int             i, cnt;
    fixed_t         bottombounds = rdraw_viewheight << FRACBITS;
    fixed_t         topbounds = (con_clipviewtop - 1) << FRACBITS;
    fixed_t         offsetvalue = 0;
    fixed_t         lheight;
    r_lightlist_t * rlight; // dc_lightlist
    ff_light_t    * ff_light; // light list item
    lighttable_t  * ro_colormap = NULL;  // override colormap
    extracolormap_t  * ro_extracolormap = NULL;  // override extracolormap

    void (*colfunc_2s) (column_t*);

    // Calculate light table.
    // Use different light tables
    //   for horizontal / vertical / diagonal. Diagonal?

    curline = ds->curline;
    backsector = ffloor->taggedtarget;
    frontsector = (curline->frontsector == ffloor->taggedtarget) ?
                   curline->backsector : curline->frontsector;

    if (curline->v1->y == curline->v2->y)
        orient_light = -ORIENT_LIGHT;
    else if (curline->v1->x == curline->v2->x)
        orient_light = ORIENT_LIGHT;

    // midtexture, 0=no-texture, otherwise valid
    texnum = sides[ffloor->master->sidenum[0]].midtexture;
    if( texnum == 0 )  return;  // no texture to display (when 3Dslab is missing side texture)
    texnum = texturetranslation[texnum];

    colfunc = basecolfunc;

    if(ffloor->flags & FF_TRANSLUCENT)
    {
      // Hacked up support for alpha value in software mode SSNTails 09-24-2002
      // [WDJ] 11-2012
      dr_alpha = ffloor->alpha; // translucent alpha 0..255
//      dr_alpha = fweff[ffloor->fw_effect].alpha;
      dc_translucent_index = 0; // force use of dr_alpha by RGB drawers
      dc_translucentmap = & translucenttables[ translucent_alpha_table[dr_alpha >> 4] ];
      colfunc = transcolfunc;
    }
    else if(ffloor->flags & FF_FOG)
    {
      colfunc = fogcolfunc;  // R_DrawFogColumn_8 16 ..
      // from fogsheet
      fog_col_length = (textures[texnum]->texture_model == TM_masked)? 2: textures[texnum]->height;
      fog_index = fog_tic % fog_col_length;  // pixel selection
      fog_init = 1;
      dr_alpha = fweff[ffloor->fw_effect].fsh_alpha; // dr_alpha 0..255
    }
    base_fog_alpha = dr_alpha;

    // [WDJ] Overrides of colormap.
    if( fixedcolormap )
        ro_colormap = fixedcolormap;
    else if( view_extracolormap )
    {
        ro_extracolormap = view_extracolormap;
    }
    else if(ffloor->flags & FF_FOG)   // Same result if test for colormap, or not.
    {
        // [WDJ] FF_FOG has optional colormap.
        // Use that colormap if it is present, there usually is only one colormap in a situation.
        // If no colormap (ro_extracolormap == NULL), then ff_light->extra_colormap can be used.
        ro_extracolormap = ffloor->master->frontsector->extra_colormap;
    }

    //SoM: Moved these up here so they are available for my lightlist calculations
    rw_scalestep = ds->scalestep;
    dm_yscale = ds->scale1 + (x1 - ds->x1)*rw_scalestep;

    dc_numlights = frontsector->numlights;
    if( dc_numlights )
    {
      if(dc_numlights > dc_maxlights)    expand_lightlist();

      cnt = 0; // cnt of rlight created, some ff_light will be skipped
      // highest light to lowest light, [0] is sector light at top
      for(i = 0; i < dc_numlights; i++)
      {
        // Limit list to lights that affect this thickside.
        ff_light = &frontsector->lightlist[i];
        rlight = &dc_lightlist[cnt];	// create in this list slot

        if(ff_light->height < *ffloor->bottomheight)
          continue;  // too low, next ff_light

        if(ff_light->height > *ffloor->topheight)
        {
          // This light is above the ffloor thickside.
          // Ignore it if the next light down is also above the ffloor thickside, when
          // that light will block.
          if(i+1 < dc_numlights
             && frontsector->lightlist[i+1].height > *ffloor->topheight
             && !(frontsector->lightlist[i+1].flags & FF_NOSHADE) )
            continue;  // too high, next ff_light
        }

        lheight = ff_light->height;// > *ffloor->topheight ? *ffloor->topheight + FRACUNIT : ff_light->height;
        rlight->heightstep = -FixedMul (rw_scalestep, (lheight - viewz));
        rlight->height = (centeryfrac) - FixedMul((lheight - viewz), dm_yscale) - rlight->heightstep;
        rlight->flags = ff_light->flags;
        if(ff_light->flags & (FF_CUTSOLIDS|FF_CUTEXTRA))
        {
          lheight = *ff_light->caster->bottomheight;// > *ffloor->topheight ? *ffloor->topheight + FRACUNIT : *ff_light->caster->bottomheight;
          rlight->botheightstep = -FixedMul (rw_scalestep, (lheight - viewz));
          rlight->botheight = (centeryfrac) - FixedMul((lheight - viewz), dm_yscale) - rlight->botheightstep;
        }

        rlight->lightlevel = *ff_light->lightlevel;
        rlight->extra_colormap = ff_light->extra_colormap;

        // Check if the current light affects the colormap/lightlevel
        if( ff_light->flags & FF_NOSHADE )
          continue; // next ff_light

        // Allows FOG on ffloor thickside to override lights.
        // Really is only meant to handle one of these options at a time.
        // Light and colormap precedence should match.
        if(ffloor->flags & FF_FOG)
          vlight = ffloor->master->frontsector->lightlevel + extralight_fog;
        else if(rlight->flags & FF_FOG)
          vlight = rlight->lightlevel + extralight_fog;
        else if(rlight->extra_colormap && rlight->extra_colormap->fog)
          vlight = rlight->lightlevel + extralight_cm;
        else
          vlight = rlight->lightlevel + extralight + orient_light;

        rlight->vlightmap =
             (vlight < 0) ? scalelight[0]
           : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
           : scalelight[vlight>>LIGHTSEGSHIFT];

        // [WDJ] Fixed determinations, taken out of loop.
        // Overrides of colormap, with same usage.
        // colormap precedence:
        //  fixedcolormap, ffloor FF_FOG colormap, rlight->extra_colormap
        if( fixedcolormap )
            rlight->rcolormap = fixedcolormap;
        else if( ro_extracolormap ) // override light extracolormap
            rlight->extra_colormap = ro_extracolormap;

        cnt++;
      }
      dc_numlights = cnt;
    }
    else
    {
      // Render ffloor thickside when frontsector does not have lightlist.
      // This happens for fog effects in otherwise normal sector, and probably others.
      //SoM: Get correct light level!
      if(ffloor->flags & FF_FOG)
        vlight = ffloor->master->frontsector->lightlevel + extralight_fog;
      else if(frontsector->extra_colormap && frontsector->extra_colormap->fog)
        vlight = frontsector->lightlevel + extralight_cm;
      else if(colfunc == transcolfunc)
        vlight = 255 + orient_light;
      else
      {
        sector_t * lightsec = R_FakeFlat(frontsector, &tempsec, false,
                                         /*OUT*/ NULL, NULL );
        vlight = lightsec->lightlevel + extralight + orient_light;
      }

      walllights =
           (vlight < 0) ? scalelight[0]
         : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
         : scalelight[vlight>>LIGHTSEGSHIFT];

      // colormap precedence:
      //  fixedcolormap, ffloor FF_FOG colormap, frontsector->extra_colormap
      if( !ro_extracolormap )
      {
        ro_extracolormap = frontsector->extra_colormap;
      }
       
      if( ro_extracolormap )
        ro_colormap = ro_extracolormap->colormap;
    }

    maskedtexturecol = ds->thicksidecol;

    dm_floorclip = ds->spr_bottomclip;
    dm_ceilingclip = ds->spr_topclip;

    dc_texheight = textureheight[texnum] >> FRACBITS;

    dc_texturemid = *ffloor->topheight - viewz;

    offsetvalue = sides[ffloor->master->sidenum[0]].rowoffset;
    if(curline->linedef->flags & ML_DONTPEGBOTTOM)
      offsetvalue -= *ffloor->topheight - *ffloor->bottomheight;

    dc_texturemid += offsetvalue;

    // [WDJ] Initialize dc_colormap.
    // If fixedcolormap == NULL, then the loop will scale the light and colormap.
    dc_colormap = fixedcolormap;


    //faB: handle case where multipatch texture is drawn on a 2sided wall, multi-patch textures
    //     are not stored per-column with post info anymore in Doom Legacy
    // [WDJ] multi-patch transparent texture restored
  retry_texture_model:
    switch (textures[texnum]->texture_model)
    {
     case TM_patch:
        colfunc_2s = R_DrawMaskedColumn;                    //render the usual 2sided single-patch packed texture
        break;
     case TM_combine_patch:
        colfunc_2s = R_DrawMaskedColumn;                    //render combined as 2sided single-patch packed texture
        break;
     case TM_picture:    
        colfunc_2s = R_Render2sidedMultiPatchColumn;        //render multipatch with no holes (no post_t info)
        column2s_length = textures[texnum]->height;
        break;
     case TM_masked:
     case TM_none:
        R_GenerateTexture( texnum );	// first time
        goto retry_texture_model;
     default:
        return;	// no draw routine
    }

    // [WDJ] x1,x2 are limited to 0..rdraw_viewwidth to protect [dc_x] access.
#ifdef RANGECHECK
    if( x1 < 0 || x2 >= rdraw_viewwidth )
       I_Error( "R_RenderThickSideRange: %i  %i\n", x1, x2);
#endif
    if( x1 < 0 )  x1 = 0;
    if( x2 >= rdraw_viewwidth )  x2 = rdraw_viewwidth-1;
    // draw the columns
    for (dc_x = x1 ; dc_x <= x2 ; dc_x++)
    {
      if(maskedtexturecol[dc_x] != MAXSHORT)
      {
        dm_top_patch = dm_windowtop = (centeryfrac - FixedMul((dc_texturemid - offsetvalue), dm_yscale));
        dm_bottom_patch = dm_windowbottom = FixedMul(*ffloor->topheight - *ffloor->bottomheight, dm_yscale) + dm_top_patch;

        // SoM: New code does not rely on r_drawColumnShadowed_8 which
        // will (hopefully) put less strain on the stack.
        if(dc_numlights)
        {
          fixed_t        height;
          fixed_t        bheight = 0;
          byte   solid = 0;  // when light source is treated as a solid
          byte   lighteffect = 0;  // when not NOSHADE, light affects the colormap and lightlevel

          if(dm_windowbottom < topbounds || dm_windowtop > bottombounds)
          {
            // Apply dc_lightlist height adjustments. The height at the following x are dependent upon this.
            // highest light to lowest light, [0] is sector light at top
            for(i = 0; i < dc_numlights; i++)
            {
              rlight = &dc_lightlist[i];
              rlight->height += rlight->heightstep;
              if(rlight->flags & (FF_CUTSOLIDS|FF_CUTEXTRA))
                rlight->botheight += rlight->botheightstep;
            }
            goto next_x;	    
          }

          dc_iscale = 0xffffffffu / (unsigned)dm_yscale;
            
          // draw the texture
          col = (column_t *)((byte *)R_GetColumn(texnum,maskedtexturecol[dc_x]) - 3);

          // Top level colormap, or fixedcolormap.
          dc_colormap = dc_lightlist[0].rcolormap;

          // Setup dc_lightlist as to the 3dfloor light sources and effects.
          // highest light to lowest light, [0] is sector light at top
          for(i = 0; i < dc_numlights; i++)
          {
            // Check if the current light affects the colormap/lightlevel
            rlight = &dc_lightlist[i];
            lighteffect = !(rlight->flags & FF_NOSHADE);
            if(lighteffect)
            {
              // use rlight->rcolormap only when lighteffect
              if( !fixedcolormap )
              {
                // distance effect on light, yscale is smaller at distance.
                unsigned  dlit = dm_yscale>>LIGHTSCALESHIFT;
                if (dlit >=  MAXLIGHTSCALE )
                  dlit = MAXLIGHTSCALE-1;

                // light table for the distance
                rlight->rcolormap = rlight->vlightmap[dlit];
                if( rlight->extra_colormap )
                {
                  // reverse indexing, and change to extra_colormap
                  int lightindex = rlight->rcolormap - reg_colormaps;
                  rlight->rcolormap = & rlight->extra_colormap->colormap[ lightindex ];
                }
              } // not fixedcolormap
            } // lighteffect

            // Check if the current light can cut the current 3D floor.
            if(rlight->flags & FF_CUTSOLIDS && !(ffloor->flags & FF_EXTRA))
              solid = 1;
            else if(rlight->flags & FF_CUTEXTRA && ffloor->flags & FF_EXTRA)
            {
              if(rlight->flags & FF_EXTRA)
              {
                // The light is from an extra 3D floor... Check the flags so
                // there are no undesired cuts.
                if((rlight->flags & (FF_TRANSLUCENT|FF_FOG)) == (ffloor->flags & (FF_TRANSLUCENT|FF_FOG)))
                {
                   if( ffloor->flags & FF_JOIN_SIDES )
                   {
                      float fm = base_fog_alpha * 0.4;  // JOIN
                      if( view_fogfloor && (dc_iscale < 0x4000))
                      {
                          // fade JOIN fogsheet as player approaches it
                          fm = (fm * dc_iscale) * (1.0/0x4000);
                      }
                      dr_alpha = (int)fm;
                   }
                   else
                      solid = 1;
                }
              }
              else
                solid = 1;
            }

            rlight->height += rlight->heightstep;
            height = rlight->height;

            if(solid)
            {
              rlight->botheight += rlight->botheightstep;
              bheight = rlight->botheight - (FRACUNIT >> 1);
            }

            if(height <= dm_windowtop)
            {
              if(lighteffect)
                dc_colormap = rlight->rcolormap;
              if(solid && dm_windowtop < bheight)
                dm_windowtop = bheight;
              continue;  // next light
            }

            dm_windowbottom = height;
            if(dm_windowbottom >= dm_bottom_patch)
            {
              // bottom of view window
              dm_windowbottom = dm_bottom_patch;
              colfunc_2s (col);

              // Finish dc_lightlist height adjustments.
              // highest light to lowest light, [0] is sector light at top
              for(i++ ; i < dc_numlights; i++)
              {
                rlight = &dc_lightlist[i];
                rlight->height += rlight->heightstep;
                if(rlight->flags & (FF_CUTSOLIDS|FF_CUTEXTRA))
                  rlight->botheight += rlight->botheightstep;
              }
              goto next_x;
            }

            colfunc_2s (col);  // draw

            // downward light from this light level
            dm_windowtop = solid ? bheight : (dm_windowbottom + 1);
            if(lighteffect)
              dc_colormap = rlight->rcolormap;
          } // for lights
          // bottom floor
          dm_windowbottom = dm_bottom_patch;
          if(dm_windowtop < dm_windowbottom)
            colfunc_2s (col);

        next_x:
          dm_yscale += rw_scalestep;
          continue;  // dc_x
        }

        // No ffloor.
        // calculate lighting for distance using dm_yscale
        if (!fixedcolormap)
        {
            // distance effect on light, yscale is smaller at distance.
            unsigned  dlit = dm_yscale>>LIGHTSCALESHIFT;
            if (dlit >=  MAXLIGHTSCALE )
                dlit = MAXLIGHTSCALE-1;

            // light table for the distance
            dc_colormap = walllights[dlit];
            if( ro_colormap )
            {
                // reverse indexing, and change to extra_colormap
                int lightindex = dc_colormap - reg_colormaps;
                dc_colormap = & ro_colormap[ lightindex ];
            }
        }

        dc_iscale = 0xffffffffu / (unsigned)dm_yscale;
            
        // draw the texture
        col = (column_t *)((byte *)R_GetColumn(texnum,maskedtexturecol[dc_x]) - 3);
            
        colfunc_2s (col);
        dm_yscale += rw_scalestep;
      }
    }
    colfunc = basecolfunc;
}


// [WDJ] Render a fog sheet, generated from midtexture, with alpha
void R_RenderFog( ffloor_t* fff, sector_t * intosec, lightlev_t foglight,
                  fixed_t scale )
{
    line_t * fogline = fff->master;
    side_t * fogside = & sides[ fogline->sidenum[0] ];
    sector_t * modelsec = fogside->sector;
    lighttable_t** xwalllights = scalelight[0];  // local selection of light table
    lighttable_t  * ro_colormap = NULL;  // override colormap

    int      texnum, texheight;
    fixed_t  windowclip_top, windowclip_bottom;
    fixed_t  topheight, heightstep, bot_patch;
    fixed_t  sec_ceilingheight_viewrel;
    fixed_t  x1 = 0, x2 = rdraw_viewwidth - 1;

    column_t*   col;
    void (*colfunc_2s) (column_t*);

    // midtexture, 0=no-texture, otherwise valid
    texnum = texturetranslation[fogside->midtexture];
    if( texnum == 0 )  goto nofog;

    dm_windowbottom = dm_windowtop = dm_bottom_patch = FIXED_MAX; // default no clip
    // [WDJ] clip at ceiling and floor
    // world coord, relative to viewer
    windowclip_top = intosec->ceilingheight - viewz;
    windowclip_bottom = intosec->floorheight - viewz;

    if( scale > 10 ) {
        dm_yscale = scale;
        rw_scalestep = 0;
    }
    else
    {
        // random fog scale
        dm_yscale = ((int)fog_wave2 << (FRACBITS-6)) + (14<<FRACBITS);  // 30 .. 14
        rw_scalestep = ((int)fog_wave1 << (FRACBITS-7)) - (4<<FRACBITS);  // ( 4 .. -4 )
        dm_yscale -= rw_scalestep/2;
        rw_scalestep /= rdraw_viewwidth;
    }

    // Select the default, or special effect column drawing functions,
    // which are called by the colfunc_2s functions.
    if (fff->flags & FF_FOG)	// Legacy Fog sheet or Fog/water with FF_FOG
    {
        // Display fog sheet (128 high) as transparent middle texture.
        // Only where there is a middle texture (in place of it).
        colfunc = fogcolfunc; // R_DrawFogColumn_8 16 ..
          // need dc_source, dc_colormap, dc_yh, dc_yl, dc_x
        fog_col_length = (textures[texnum]->texture_model == TM_masked)? 2: textures[texnum]->height;
        // add in player movement to fog
        int playermov = (viewmobj->x + viewmobj->y + (viewmobj->angle>>6)) >> (FRACBITS+6);
        fog_index = (fog_tic + playermov) % fog_col_length;  // pixel selection
        fog_init = 1;
        dr_alpha = fweff[fff->fw_effect].fsh_alpha;  // dr_alpha 0..255
    }
    else
        goto nofog;

    // Select the 2s draw functions, they are called later.
    //faB: handle case where multipatch texture is drawn on a 2sided wall, multi-patch textures
    //     are not stored per-column with post info anymore in Doom Legacy
    // [WDJ] multi-patch transparent texture restored
    // DrawMasked needs: dm_floorclip, dm_ceilingclip, dm_yscale,
    //  dm_top_patch, dm_bottom_patch, dm_windowtop, dm_windowbottom

  retry_texture_model:
    switch (textures[texnum]->texture_model)
    {
     case TM_patch:
        colfunc_2s = R_DrawMaskedColumn;                    //render the usual 2sided single-patch packed texture
        break;
     case TM_combine_patch:
        colfunc_2s = R_DrawMaskedColumn;                    //render combined as 2sided single-patch packed texture
        break;
     case TM_picture:    
        colfunc_2s = R_Render2sidedMultiPatchColumn;        //render multipatch with no holes (no post_t info)
        column2s_length = textures[texnum]->height;
        break;
     case TM_masked:
     case TM_none:
        R_GenerateTexture( texnum );	// first time
        goto retry_texture_model;
     default:
        goto nofog;  // no draw routine
    }
    texheight = textureheight[texnum];

    // fake floor light heights in screen coord. , at x=0
    sec_ceilingheight_viewrel = modelsec->ceilingheight - viewz;  // for fog sector
    topheight = (centeryfrac) - FixedMul(sec_ceilingheight_viewrel, dm_yscale);
    heightstep = -FixedMul (rw_scalestep, sec_ceilingheight_viewrel);

    // Setup lighting based on the presence/lack-of 3D floors.
    dc_numlights = 1;

    dm_floorclip = screenheightarray;  // noclip
    dm_ceilingclip = negonearray;  // noclip

    // top of texture, relative to viewer, with rowoffset, world coord.
    dc_texturemid = modelsec->ceilingheight + fogside->rowoffset - viewz;
    dc_texheight = texheight >> FRACBITS;

    // [WDJ] Initialize dc_colormap.
    // If fixedcolormap == NULL, then the loop will scale the light and colormap.
    dc_colormap = fixedcolormap;

    // [WDJ] Fixed determinations, taken out of loop.
    // Overrides of colormap, with same usage.
    if( fixedcolormap )
        ro_colormap = fixedcolormap;
    else if( view_colormap )
        ro_colormap = view_colormap;
    else if( modelsec->extra_colormap )
        ro_colormap = modelsec->extra_colormap->colormap;;

    if( !fixedcolormap )
    {
        lightlev_t vlight = modelsec->lightlevel + foglight;
        xwalllights =
            (vlight < 0) ? scalelight[0]
          : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
          : scalelight[vlight>>LIGHTSEGSHIFT];
    }

    // draw the columns, for one fog FakeFloor
    for (dc_x = x1 ; dc_x <= x2 ; dc_x++)
    {
        // top/bottom of texture, relative to viewer, screen coord.
        dm_top_patch = (centeryfrac - FixedMul(dc_texturemid, dm_yscale));
        bot_patch = FixedMul(texheight, dm_yscale) + dm_top_patch;
        // fog sheet clipping to ceiling and floor
        dm_windowtop = centeryfrac - FixedMul(windowclip_top, dm_yscale);
        dm_windowbottom = centeryfrac - FixedMul(windowclip_bottom, dm_yscale);
        
        topheight += heightstep;
//        if(topheight > dm_windowtop)
        {
//	    if( dm_windowbottom > topheight )
//	        dm_windowbottom = topheight;
            if(dm_windowbottom >= bot_patch)
                dm_windowbottom = bot_patch;

            if( ! fixedcolormap )
            {
                // distance effect on light, yscale is smaller at distance.
                unsigned dlit = dm_yscale>>LIGHTSCALESHIFT;
                if (dlit >=  MAXLIGHTSCALE )
                    dlit = MAXLIGHTSCALE-1;

                // light table for the distance
                dc_colormap = xwalllights[dlit];
                if( ro_colormap )
                {
                    // reverse indexing, and change to extra_colormap
                    int lightindex = dc_colormap - reg_colormaps;
                    dc_colormap = & ro_colormap[ lightindex ];
                }
            }

            dc_iscale = 0xffffffffu / (unsigned)dm_yscale;
            // draw texture, as clipped
            col = (column_t *)((byte *)R_GetColumn(texnum,0) - 3);
            colfunc_2s (col);
        }
        dm_yscale += rw_scalestep;
    } // for( dx_x = x1..x2 )
    colfunc = basecolfunc;

nofog:
    return;
}



//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked
//  texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling
//  textures.
// CALLED: CORE LOOPING ROUTINE.
//
#define HEIGHTBITS              12
#define HEIGHTUNIT              (1<<HEIGHTBITS)


//profile stuff ---------------------------------------------------------
//#define TIMING
#ifdef TIMING
#include "p5prof.h"
long long mycount;
long long mytotal = 0;
unsigned long   nombre = 100000;
//static   char runtest[10][80];
#endif
//profile stuff ---------------------------------------------------------


// Software Render
// IN: rw_ parameters
// Called by R_StoreWallRange
static
void R_RenderSegLoop (void)
{
    int        orient_light = 0;  // wall orientation effect

    int        angf;
    int        yl, yh;

    fixed_t    texturecolumn = 0;
    int        mid, top, bottom;
    int        i;
    lighttable_t  * ro_colormap = NULL;  // override colormap
    extracolormap_t  * ro_extracolormap = NULL;  // override colormap
    

#if 0   
    // [WDJ] R_StoreWallRange violates rdraw_viewwidth.
#define INDIVIDUAL_X_CLIP 
    // [WDJ] R_StoreWallRange fixed 3/24/2010, keep just in case problems arise.
    if( rw_stopx >= rdraw_viewwidth )
    {
//        printf("limiting rw_stopx %i\n", rw_stopx);
        rw_stopx = rdraw_viewwidth;
    }
#endif

    // line orientation light, out of the loop
    if (curline->v1->y == curline->v2->y)
        orient_light = -ORIENT_LIGHT;
    else if (curline->v1->x == curline->v2->x)
        orient_light = ORIENT_LIGHT;

    // [WDJ] Initialize dc_colormap.
    // If fixedcolormap == NULL, then the loop will scale the light and colormap.
    dc_colormap = fixedcolormap;

    if( dc_numlights )
    {
        r_lightlist_t * rlight;
        lightlev_t  vlight;

        // Setup dc_lightlist as to the 3dfloor light and colormaps.
        // The lightlevel and extra_colormap were copied from ffloor by R_StoreWallRange.
        // NOTE: Due to StoreWallRange, the dc_lightlist entries do NOT correspond to
        // the sector lightlist[i].
        for(i = 0; i < dc_numlights; i++)
        {
            rlight = & dc_lightlist[i];

            // [WDJ] FF_FOG is also set when frontsector lightlist caster has FF_FOG.
            if( rlight->flags & FF_FOG )
                vlight = rlight->lightlevel + extralight_fog;
            else if( rlight->extra_colormap && rlight->extra_colormap->fog)
                vlight = rlight->lightlevel + extralight_cm;
            else
                vlight = rlight->lightlevel + extralight + orient_light;

            rlight->vlightmap =
                  (vlight < 0) ? scalelight[0]
                : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
                : scalelight[vlight>>LIGHTSEGSHIFT];

            // [WDJ] Fixed determinations, taken out of line loop.
            // Colormap overrides, with the same usage.
            if( fixedcolormap )
                rlight->rcolormap = fixedcolormap;
            else if( view_extracolormap )
                rlight->extra_colormap = view_extracolormap;
        }
        // Select draw function that will step through dc_lightlist, and sets dc_colormap.
        colfunc = R_DrawColumnShadowed;  // generic 8 16
    }
    else
    {
        // [WDJ] Fixed determinations, taken out of line loop.
        // Colormap overrides, with the same usage.
        if( fixedcolormap )
            ro_colormap = fixedcolormap;
        else if( view_extracolormap )
            ro_extracolormap = view_extracolormap;
        else  // over the whole line
            ro_extracolormap = frontsector->extra_colormap;
   
        if( ro_extracolormap )
            ro_colormap = ro_extracolormap->colormap;
    }


    for ( ; rw_x < rw_stopx ; rw_x++)
    {
        // mark floor / ceiling areas
        yl = (topfrac+HEIGHTUNIT-1)>>HEIGHTBITS;
        
        // no space above wall?
        if (yl < ceilingclip[rw_x]+1)
            yl = ceilingclip[rw_x]+1;
        
        if (markceiling)
        {
            top = ceilingclip[rw_x]+1;
            bottom = yl-1;
            
            if (bottom >= floorclip[rw_x])
                bottom = floorclip[rw_x]-1;
            
            // visplane global parameter vsp_ceilingplane
            if (top <= bottom)
            {
                vsp_ceilingplane->top[rw_x] = top;
                vsp_ceilingplane->bottom[rw_x] = bottom;
            }
        }

        
        yh = bottomfrac>>HEIGHTBITS;
        
        if (yh >= floorclip[rw_x])
            yh = floorclip[rw_x]-1;
        
        if (markfloor)
        {
            top = yh+1;
            bottom = floorclip[rw_x]-1;
            if (top <= ceilingclip[rw_x])
                top = ceilingclip[rw_x]+1;
            // visplane global parameter vsp_floorplane
            if (top <= bottom && vsp_floorplane)
            {
                vsp_floorplane->top[rw_x] = top;
                vsp_floorplane->bottom[rw_x] = bottom;
            }
        }


        if (numffplane)
        {
          firstseg->backscale[rw_x] = backscale[rw_x];
//	  firstseg->frontscale[rw_x] = rw_scale;
          for(i = 0; i < numffplane; i++)
          {
            if(ffplane[i].height < viewz)
            {
              int top_w = (ffplane[i].front_frac >> HEIGHTBITS) + 1;
              int bottom_w = ffplane[i].front_clip[rw_x];

              if(top_w < ceilingclip[rw_x] + 1)
                top_w = ceilingclip[rw_x] + 1;

              if (bottom_w > floorclip[rw_x] - 1)
                bottom_w = floorclip[rw_x] - 1;

              if (top_w <= bottom_w)
              {
                ffplane[i].plane->top[rw_x] = top_w;
                ffplane[i].plane->bottom[rw_x] = bottom_w;
              }
            }
            else if (ffplane[i].height > viewz)
            {
              int top_w = ffplane[i].con_clip[rw_x] + 1;
              int bottom_w = (ffplane[i].front_frac >> HEIGHTBITS);

              if (top_w < ceilingclip[rw_x] + 1)
                top_w = ceilingclip[rw_x] + 1;

              if (bottom_w > floorclip[rw_x] - 1)
                bottom_w = floorclip[rw_x] - 1;

              if (top_w <= bottom_w)
              {
                ffplane[i].plane->top[rw_x] = top_w;
                ffplane[i].plane->bottom[rw_x] = bottom_w;
              }
            }
          }
        } // if numffplane

        //SoM: Calculate offsets for Thick fake floors.
        // calculate texture offset
        angf = ANGLE_TO_FINE(rw_centerangle + x_to_viewangle[rw_x]);
        texturecolumn = rw_offset - FixedMul(finetangent[angf], rw_distance);
        texturecolumn >>= FRACBITS;

        // texturecolumn and lighting are independent of wall tiers
        if (segtextured)
        {
            dc_x = rw_x;
            dc_iscale = 0xffffffffu / (unsigned)rw_scale;

            if( ! fixedcolormap )
            {
                // distance effect on light, rw_scale is smaller at distance.
                unsigned  dlit = rw_scale>>LIGHTSCALESHIFT;
                if (dlit >=  MAXLIGHTSCALE )
                    dlit = MAXLIGHTSCALE-1;

                // light table for the distance
                dc_colormap = walllights[dlit];
                if( ro_colormap )
                {
                    // reverse indexing, and change to extra_colormap
                    int lightindex = dc_colormap - reg_colormaps;
                    dc_colormap = & ro_colormap[ lightindex ];
                }
            }
        }

        if(dc_numlights)
        {
          r_lightlist_t * rlight;

          // Setup dc_lightlist as to the 3dfloor light and colormaps.
          // highest light to lowest light, [0] is sector light at top
          for(i = 0; i < dc_numlights; i++)
          {
            rlight = & dc_lightlist[i];

            if( !fixedcolormap )
            {
              // distance effect on light, rw_scale is smaller at distance.
              unsigned  dlit = rw_scale>>LIGHTSCALESHIFT;
              if (dlit >=  MAXLIGHTSCALE )
                dlit = MAXLIGHTSCALE-1;

              // light table for the distance
              rlight->rcolormap = rlight->vlightmap[dlit];
              if( rlight->extra_colormap )
              {
                // reverse indexing, and change to extra_colormap
                int lightindex = rlight->rcolormap - reg_colormaps;
                rlight->rcolormap = & rlight->extra_colormap->colormap[ lightindex ];
              }
            }
          }
          // The colfunc will step through dc_lightlist, and sets dc_colormap.
        } // if dc_numlights

        backscale[rw_x] = rw_scale;

        // [WDJ] if(dx_x >= viewwidth),  either return
        // or individual clip and execute bottom of loop

        // draw the wall tiers
        if (midtexture)
        {
#ifdef INDIVIDUAL_X_CLIP
          if( yl < rdraw_viewheight && yh >= 0 && yh >= yl
              && ((unsigned) dc_x < rdraw_viewwidth) ) // not disabled
#else
          if( yl < rdraw_viewheight && yh >= 0 && yh >= yl ) // not disabled
#endif
          {
            // single sided line
            dc_yl = yl;
            dc_yh = yh;
            //[WDJ] phobiata.wad has many views that need clipping
            if ( dc_yl < 0 )   dc_yl = 0;
            if ( dc_yh >= rdraw_viewheight )   dc_yh = rdraw_viewheight - 1;

            dc_texturemid = rw_midtexturemid;
            dc_source = R_GetColumn(midtexture,texturecolumn);
            dc_texheight = textureheight[midtexture] >> FRACBITS;
            //profile stuff ---------------------------------------------------------
#ifdef TIMING
            ProfZeroTimer();
#endif
#ifdef HORIZONTALDRAW
            hcolfunc ();
#else
            colfunc ();
#endif
#ifdef TIMING
            RDMSR(0x10,&mycount);
            mytotal += mycount;      //64bit add
            
            if(nombre--==0)
                I_Error("R_DrawColumn CPU Spy reports: 0x%d %d\n", *((int*)&mytotal+1),
                (int)mytotal );
#endif
            //profile stuff ---------------------------------------------------------
          }
            // dont draw anything more for this column, since
            // a midtexture blocks the view
            ceilingclip[rw_x] = rdraw_viewheight;
            floorclip[rw_x] = -1;
        }
        else
        {
            // two sided line
            if (toptexture)
            {
                // top wall
                mid = pixhigh>>HEIGHTBITS;
                pixhigh += pixhighstep;
                
                if (mid >= floorclip[rw_x])
                    mid = floorclip[rw_x]-1;
                
                if (mid >= yl)
                {
#ifdef INDIVIDUAL_X_CLIP
                  if( yl < rdraw_viewheight && mid >= 0
                      && ((unsigned) dc_x < rdraw_viewwidth) ) // not disabled
#else
                  if( yl < rdraw_viewheight && mid >= 0 ) // not disabled
#endif
                  {
                    dc_yl = yl;
                    dc_yh = mid;
                    //[WDJ] phobiata.wad has many views that need clipping
                    if ( dc_yl < 0 )   dc_yl = 0;
                    if ( dc_yh >= rdraw_viewheight )   dc_yh = rdraw_viewheight - 1;

                    dc_texturemid = rw_toptexturemid;
                    dc_source = R_GetColumn(toptexture,texturecolumn);
                    dc_texheight = textureheight[toptexture] >> FRACBITS;
#ifdef HORIZONTALDRAW
                    hcolfunc ();
#else
                    colfunc ();
#endif
                  } // if mid >= 0
                    ceilingclip[rw_x] = mid;
                }
                else
                {
                    // mid < yl
                    ceilingclip[rw_x] = yl-1;
                }
            }
            else
            {
                // no top wall
                if (markceiling)
                {
                    ceilingclip[rw_x] = yl-1;
                }
            }
            
            if (bottomtexture)
            {
                // bottom wall
                mid = (pixlow+HEIGHTUNIT-1)>>HEIGHTBITS;
                pixlow += pixlowstep;
                
                // no space above wall?
                if (mid <= ceilingclip[rw_x])
                    mid = ceilingclip[rw_x]+1;

                if (mid <= yh)
                {
#ifdef INDIVIDUAL_X_CLIP
                  if( mid < rdraw_viewheight && yh >= 0
                      && ((unsigned) dc_x < rdraw_viewwidth) ) // not disabled
#else
                  if( mid < rdraw_viewheight && yh >= 0 ) // not disabled
#endif
                  {
                    dc_yl = mid;
                    dc_yh = yh;
                    //[WDJ] phobiata.wad has many views that need clipping
                    if ( dc_yl < 0 )   dc_yl = 0;
                    if ( dc_yh >= rdraw_viewheight )   dc_yh = rdraw_viewheight - 1;

                    dc_texturemid = rw_bottomtexturemid;
                    dc_source = R_GetColumn(bottomtexture,
                        texturecolumn);

                    dc_texheight = textureheight[bottomtexture] >> FRACBITS;
#ifdef HORIZONTALDRAW
                    hcolfunc ();
#else
                    colfunc ();
#endif
                  } // if mid >= 0
                    floorclip[rw_x] = mid;
                }
                else
                {
                    floorclip[rw_x] = yh+1;
                }

            }
            else
            {
                // no bottom wall
                if (markfloor)
                {
                    floorclip[rw_x] = yh+1;
                }
            }
        }

        if (maskedtexture || numthicksides)
        {
          // save texturecol
          //  for backdrawing of masked mid texture
          maskedtexturecol[rw_x] = texturecolumn;
        }

        if(dc_numlights)
        {
          // Apply dc_lightlist height adjustments.
          // highest light to lowest light, [0] is sector light at top
          for(i = 0; i < dc_numlights; i++)
          {
            dc_lightlist[i].height += dc_lightlist[i].heightstep;
            if(dc_lightlist[i].flags & FF_SOLID)
              dc_lightlist[i].botheight += dc_lightlist[i].botheightstep;
          }
        }


        /*if(dc_wallportals)
        {
          wallportal_t* wpr;
          for(wpr = dc_wallportals; wpr; wpr = wpr->next)
          {
            wpr->top += wpr->topstep;
            wpr->bottom += wpr->bottomstep;
          }
        }*/


        for(i = 0; i < MAXFFLOORS; i++)
        {
          if (ffplane[i].valid_mark)
          {
            int y_w = ffplane[i].back_frac >> HEIGHTBITS;

            ffplane[i].front_clip[rw_x] = ffplane[i].con_clip[rw_x] = y_w;
            ffplane[i].back_frac += ffplane[i].back_step;
          }

          ffplane[i].front_frac += ffplane[i].front_step;
        }

        rw_scale += rw_scalestep;
        topfrac += topstep;
        bottomfrac += bottomstep;
        // [WDJ] Overflow protection.  Overflow and underflow of topfrac and
        // bottomfrac cause off-screen textures to be drawn as large bars.
        // See phobiata.wad map07, which has a floor at -20000.
        // The cause of the overflow many times seems to be the step value.
        if( bottomfrac < topfrac ) {
           // Uncomment to see which map areas cause this overflow.
//	   debug_Printf("Overflow break: bottomfrac(%i) < topfrac(%i)\n", bottomfrac, topfrac );
           break;
        }
    }
}



//
// R_StoreWallRange, software render.
// A wall segment will be drawn
//  between start and stop pixels (inclusive).
//
void R_StoreWallRange( int   start, int   stop)
{
    fixed_t             hyp;
    fixed_t             sineval;
    angle_t             distangle, offsetangle;
    fixed_t             vtop;
    lightlev_t          vlight;  // visible light 0..255
    lightlev_t          orient_light = 0;  // wall orientation effect
    int                 i, cnt;
    ff_light_t        * ff_light;  // light list item
    r_lightlist_t     * rlight;
    ffloor_t          * bff, * fff;  // backsector fake floor, frontsector fake floor
//    fixed_t             lheight;  // unused

    if (ds_p == &drawsegs[maxdrawsegs])   expand_drawsegs();
    
#ifdef RANGECHECK
    if (start >=rdraw_viewwidth || start > stop)
        I_Error ("Bad R_RenderWallRange: %i to %i", start , stop);
#endif
    
    if (curline->v1->y == curline->v2->y)
        orient_light = -ORIENT_LIGHT;
    else if (curline->v1->x == curline->v2->x)
        orient_light = ORIENT_LIGHT;

    sidedef = curline->sidedef;
    linedef = curline->linedef;
    
    // mark the segment as visible for auto map
    linedef->flags |= ML_MAPPED;
    
    // calculate rw_distance for scale calculation
    rw_normalangle = curline->angle + ANG90;
    offsetangle = abs(rw_normalangle-rw_angle1);
    
    if (offsetangle > ANG90)
        offsetangle = ANG90;

    distangle = ANG90 - offsetangle;
    hyp = R_PointToDist (curline->v1->x, curline->v1->y);
    sineval = sine_ANG(distangle);
    rw_distance = FixedMul (hyp, sineval);

    // segment ends
    ds_p->x1 = rw_x = start;
    ds_p->x2 = stop;
    ds_p->curline = curline;
    rw_stopx = stop+1;
    // [WDJ] draw range is rw_x .. rw_stopx-1
    if( rw_stopx > rdraw_viewwidth )  rw_stopx = rdraw_viewwidth;

    //SoM: Code to remove limits on openings.
    {
      size_t lastindex = lastopening - openings;
      size_t needindex = (rw_stopx - start)*4 + lastindex;
      if (needindex > maxopenings)  expand_openings( needindex );
    }  // end of code to remove limits on openings

    // calculate scale at both ends and step
    ds_p->scale1 = rw_scale =
        R_ScaleFromGlobalAngle (viewangle + x_to_viewangle[start]);

    if (stop > start)
    {
        ds_p->scale2 = R_ScaleFromGlobalAngle (viewangle + x_to_viewangle[stop]);
        ds_p->scalestep = rw_scalestep = (ds_p->scale2 - rw_scale) / (stop-start);
    }
    else
    {
#if 0
        // UNUSED: try to fix the stretched line bug
        if (rw_distance < FRACUNIT/2)
        {
            fixed_t         tr_x,tr_y;
            fixed_t         gxt,gyt;
            
            tr_x = curline->v1->x - viewx;
            tr_y = curline->v1->y - viewy;
            
            gxt = FixedMul(tr_x,viewcos);
            gyt = -FixedMul(tr_y,viewsin);
            ds_p->scale1 = FixedDiv(projection, gxt-gyt)<<detailshift;
        }
#endif
        ds_p->scale2 = ds_p->scale1;
        ds_p->scalestep = 0;
    }
    
    // calculate texture boundaries
    //  and decide if floor / ceiling marks are needed
    worldtop = frontsector->ceilingheight - viewz;
    worldbottom = frontsector->floorheight - viewz;

    midtexture = toptexture = bottomtexture = maskedtexture = 0; // no-texture
    ds_p->maskedtexturecol = NULL;
    ds_p->numthicksides = numthicksides = 0;
    ds_p->thicksidecol = NULL;

    for(i = 0; i < MAXFFLOORS; i++)
    {
      ffplane[i].valid_mark = false;
      ds_p->thicksides[i] = NULL;
    }

    if(numffplane)
    {
      for(i = 0; i < numffplane; i++)
        ffplane[i].front_pos = ffplane[i].height - viewz;
    }

    if (!backsector)
    {
        // single sided line
        // Single sided: assumes that there MUST be a midtexture on this side.
        // midtexture, 0=no-texture, otherwise valid
        midtexture = texturetranslation[sidedef->midtexture];
        // a single sided line is terminal, so it must mark ends
        markfloor = markceiling = true;
        
        if (linedef->flags & ML_DONTPEGBOTTOM)
        {
            vtop = frontsector->floorheight +
                textureheight[sidedef->midtexture];
            // bottom of texture at bottom
            rw_midtexturemid = vtop - viewz;
        }
        else
        {
            // top of texture at top
            rw_midtexturemid = worldtop;
        }
        rw_midtexturemid += sidedef->rowoffset;

        // drawseg does not clip sprites
        ds_p->silhouette = SIL_TOP|SIL_BOTTOM; // BOTH
        ds_p->spr_topclip = screenheightarray;
        ds_p->spr_bottomclip = negonearray;
        ds_p->sil_bottom_height = FIXED_MAX;
        ds_p->sil_top_height = FIXED_MIN;
    }
    else
    {
        // two sided line
        ds_p->spr_topclip = ds_p->spr_bottomclip = NULL;
        ds_p->silhouette = 0;
        
        if (frontsector->floorheight > backsector->floorheight)
        {
            // frontsector floor clips backsector floor and sprites
            ds_p->silhouette = SIL_BOTTOM;
            ds_p->sil_bottom_height = frontsector->floorheight;
        }
        else if (backsector->floorheight > viewz)
        {
            // backsector floor not visible, clip sprites
            ds_p->silhouette = SIL_BOTTOM;
            ds_p->sil_bottom_height = FIXED_MAX;
            // ds_p->spr_bottomclip = negonearray;
        }
        
        if (frontsector->ceilingheight < backsector->ceilingheight)
        {
            // frontsector ceiling clips backsector ceiling and sprites
            ds_p->silhouette |= SIL_TOP;
            ds_p->sil_top_height = frontsector->ceilingheight;
        }
        else if (backsector->ceilingheight < viewz)
        {
            // backsector ceiling not visible, clip sprites
            ds_p->silhouette |= SIL_TOP;
            ds_p->sil_top_height = FIXED_MIN;
            // ds_p->spr_topclip = screenheightarray;
        }
        

#if 0
// Duplicated by fix below.
        if (backsector->ceilingheight <= frontsector->floorheight)
        {
            // backsector below frontsector
            ds_p->spr_bottomclip = negonearray;
            ds_p->sil_bottom_height = FIXED_MAX;
            ds_p->silhouette |= SIL_BOTTOM;
        }
        
        if (backsector->floorheight >= frontsector->ceilingheight)
        {
            // backsector above frontsector
            ds_p->spr_topclip = screenheightarray;
            ds_p->sil_top_height = FIXED_MIN;
            ds_p->silhouette |= SIL_TOP;
        }
#endif

        //SoM: 3/25/2000: This code fixes an automap bug that didn't check
        // frontsector->ceiling and backsector->floor to see if a door was closed.
        // Without the following code, sprites get displayed behind closed doors.
        if (doorclosed || backsector->ceilingheight<=frontsector->floorheight)
        {
            ds_p->spr_bottomclip = negonearray;
            ds_p->sil_bottom_height = FIXED_MAX;
            ds_p->silhouette |= SIL_BOTTOM;
        }
        if (doorclosed || backsector->floorheight>=frontsector->ceilingheight)
        {                   // killough 1/17/98, 2/8/98
            ds_p->spr_topclip = screenheightarray;
            ds_p->sil_top_height = FIXED_MIN;
            ds_p->silhouette |= SIL_TOP;
        }

        worldbacktop = backsector->ceilingheight - viewz;
        worldbackbottom = backsector->floorheight - viewz;
        
        // hack to allow height changes in outdoor areas
        if (frontsector->ceilingpic == skyflatnum
            && backsector->ceilingpic == skyflatnum)
        {
            // SKY to SKY
            // [WDJ] Prevent worldtop < worldbottom, is used as error test
            if( worldbacktop < worldbottom )    worldbacktop = worldbottom;
            worldtop = worldbacktop;  // disable upper texture tests
        }
        
        
        if (worldbackbottom != worldbottom
            || backsector->floorpic != frontsector->floorpic
            || backsector->lightlevel != frontsector->lightlevel
            //SoM: 3/22/2000: Check floor x and y offsets.
            || backsector->floor_xoffs != frontsector->floor_xoffs
            || backsector->floor_yoffs != frontsector->floor_yoffs
            //SoM: 3/22/2000: Prevents bleeding.
            || (frontsector->model > SM_fluid)
            || backsector->modelsec != frontsector->modelsec
            || backsector->floorlightsec != frontsector->floorlightsec
            //SoM: 4/3/2000: Check for colormaps
            || frontsector->extra_colormap != backsector->extra_colormap
            || (frontsector->ffloors != backsector->ffloors && frontsector->tag != backsector->tag))
        {
            markfloor = true;  // backsector and frontsector floor are different
        }
        else
        {
            // same plane on both sides
            markfloor = false;
        }
        
        
        if (worldbacktop != worldtop
            || backsector->ceilingpic != frontsector->ceilingpic
            || backsector->lightlevel != frontsector->lightlevel
            //SoM: 3/22/2000: Check floor x and y offsets.
            || backsector->ceiling_xoffs != frontsector->ceiling_xoffs
            || backsector->ceiling_yoffs != frontsector->ceiling_yoffs
            //SoM: 3/22/2000: Prevents bleeding.
//            || (frontsector->modelsec != -1 &&
            || (frontsector->model > SM_fluid &&
                frontsector->ceilingpic != skyflatnum)
            || backsector->modelsec != frontsector->modelsec
            || backsector->floorlightsec != frontsector->floorlightsec
            //SoM: 4/3/2000: Check for colormaps
            || frontsector->extra_colormap != backsector->extra_colormap
            || (frontsector->ffloors != backsector->ffloors && frontsector->tag != backsector->tag))
        {
            markceiling = true;  // backsector and frontsector ceilings are different
        }
        else
        {
            // same plane on both sides
            markceiling = false;
        }
        
        if (backsector->ceilingheight <= frontsector->floorheight
            || backsector->floorheight >= frontsector->ceilingheight)
        {
            // closed door
            markceiling = markfloor = true;
        }

        // check TOP TEXTURE
        if (worldbacktop < worldtop)
        {
            // top texture, 0=no-texture, otherwise valid
            toptexture = texturetranslation[sidedef->toptexture];
            if (linedef->flags & ML_DONTPEGTOP)
            {
                // top of texture at top
                rw_toptexturemid = worldtop;
            }
            else
            {
                vtop = backsector->ceilingheight
                     + textureheight[sidedef->toptexture];
                
                // bottom of texture
                rw_toptexturemid = vtop - viewz;
            }
        }
        // check BOTTOM TEXTURE
        if (worldbackbottom > worldbottom)     //seulement si VISIBLE!!!
        {
            // bottom texture, 0=no-texture, otherwise valid
            bottomtexture = texturetranslation[sidedef->bottomtexture];
            
            if (linedef->flags & ML_DONTPEGBOTTOM )
            {
                // bottom of texture at bottom
                // top of texture at top
                rw_bottomtexturemid = worldtop;
            }
            else    // top of texture at top
                rw_bottomtexturemid = worldbackbottom;
        }
        
        rw_toptexturemid += sidedef->rowoffset;
        rw_bottomtexturemid += sidedef->rowoffset;

        // allocate space for masked texture tables
        if (frontsector && backsector && frontsector->tag != backsector->tag
            && (backsector->ffloors || frontsector->ffloors))
        {
          fixed_t   lowcut, highcut;

          //markceiling = markfloor = true;
          maskedtexture = true;

          // segment 3d floor sides
          ds_p->thicksidecol = maskedtexturecol = lastopening - rw_x;
          lastopening += rw_stopx - rw_x;

          lowcut = frontsector->floorheight > backsector->floorheight ? frontsector->floorheight : backsector->floorheight;
          highcut = frontsector->ceilingheight < backsector->ceilingheight ? frontsector->ceilingheight : backsector->ceilingheight;

          if(frontsector->ffloors && backsector->ffloors)
          {
            i = 0;

            // For all backsector, check all frontsector
            for(bff = backsector->ffloors; bff; bff = bff->next)
            {
              if(!(bff->flags & FF_OUTER_SIDES) || !(bff->flags & FF_EXISTS))
                continue;
              // outside sides of bff
              if(*bff->topheight < lowcut || *bff->bottomheight > highcut)
                continue;

              // look for matching ffloor where we do not render the join side
              for(fff = frontsector->ffloors; fff; fff = fff->next)
              {
                if(!(fff->flags & (FF_OUTER_SIDES|FF_INNER_SIDES))
                   || !(fff->flags & FF_EXISTS)
                   || *fff->topheight < lowcut || *fff->bottomheight > highcut)
                  continue;
                // check against sides of fff

                if(bff->flags & FF_EXTRA)
                {
                  if(!(fff->flags & FF_CUTEXTRA))
                    continue;

                  if(fff->flags & FF_EXTRA
                     && (fff->flags & (FF_TRANSLUCENT|FF_FOG)) != (bff->flags & (FF_TRANSLUCENT|FF_FOG)))
                    continue;
                }
                else
                {
                  if(!(fff->flags & FF_CUTSOLIDS))
                    continue;
                }

                if(*bff->topheight > *fff->topheight
                   || *bff->bottomheight < *fff->bottomheight)
                  continue;

                // check for forced fogsheet
                if(bff->flags & FF_JOIN_SIDES)
                {
                    // and only one forced fogsheet
                    // note: fweff[0] unused, so safe to check it
                    if(( ! (fff->flags & FF_INNER_SIDES) )
                       || (fff->flags & FF_JOIN_SIDES) )
                    {
                        continue;  // bff fogsheet
                    }
                }
                break;  // fff overlaps bff, do not render bff side
              } // for fff
              if(fff)  // found fff that completely overlaps bff
                continue;

              ds_p->thicksides[i] = bff;  // backsector 3d floor outer side
              i++;
              if( i >= MAXFFLOORS )
                 break;
            } // for bff

            // For all frontsector, check all backsector
            for(fff = frontsector->ffloors; fff; fff = fff->next)
            {
              if(!(fff->flags & FF_INNER_SIDES) || !(fff->flags & FF_EXISTS))
                continue;
              // inside sides of fff
              if(*fff->topheight < lowcut || *fff->bottomheight > highcut)
                continue;

              // check for forced fogsheet
              if(fff->flags & FF_JOIN_SIDES)
                 goto render_side_fff;

              // look for matching ffloor where we do not render the join side
              for(bff = backsector->ffloors; bff; bff = bff->next)
              {
                if(!(bff->flags & (FF_OUTER_SIDES|FF_INNER_SIDES))
                   || !(bff->flags & FF_EXISTS)
                   || *bff->topheight < lowcut || *bff->bottomheight > highcut)
                  continue;
                // check against sides of bff

                if(fff->flags & FF_EXTRA)
                {
                  if(!(bff->flags & FF_CUTEXTRA))
                    continue;

                  if(bff->flags & FF_EXTRA
                     && (bff->flags & (FF_TRANSLUCENT|FF_FOG)) != (fff->flags & (FF_TRANSLUCENT|FF_FOG)))
                    continue;
                }
                else
                {
                  if(!(bff->flags & FF_CUTSOLIDS))
                    continue;
                }

                if(*fff->topheight > *bff->topheight
                   || *fff->bottomheight < *bff->bottomheight)
                  continue;

                break;  // bff overlaps fff, do not render fff side
              } // for bff
              if(bff)  // found bff that completely overlaps fff
                continue;

            render_side_fff:
              if( i >= MAXFFLOORS ) // also protects against exit from bff loop
                 break;
              ds_p->thicksides[i] = fff;  // frontsector 3d floor inner side
              i++;
            } // for fff
          }
          else if(backsector->ffloors)
          {
            i = 0;
            // For all backsector
            for(bff = backsector->ffloors; bff; bff = bff->next)
            {
              if(!(bff->flags & FF_OUTER_SIDES) || !(bff->flags & FF_EXISTS))
                continue;
              // outer sides of bff
              if(*bff->topheight <= frontsector->floorheight
                 || *bff->bottomheight >= frontsector->ceilingheight)
                continue;

              ds_p->thicksides[i] = bff; // backsector 3d floor outer side
              i++;
              if( i >= MAXFFLOORS )
                 break;
            }
          }
          else if(frontsector->ffloors)
          {
            i = 0;
            // For all frontsector
            for(fff = frontsector->ffloors; fff; fff = fff->next)
            {
              if(!(fff->flags & FF_INNER_SIDES) || !(fff->flags & FF_EXISTS))
                continue;
              // inner sides of fff
              if(*fff->topheight <= frontsector->floorheight
                 || *fff->bottomheight >= frontsector->ceilingheight)
                continue;
              if(*fff->topheight <= backsector->floorheight
                 || *fff->bottomheight >= backsector->ceilingheight)
                continue;

              ds_p->thicksides[i] = fff;  // frontsector 3d floor inner side
              i++;
              if( i >= MAXFFLOORS )
                 break;
            } // for fff
          }

          ds_p->numthicksides = numthicksides = i;
        } // if frontsector && backsector ..
        // midtexture, 0=no-texture, otherwise valid
        if (sidedef->midtexture)
        {
            // masked midtexture
            if(!ds_p->thicksidecol)
            {
              ds_p->maskedtexturecol = maskedtexturecol = lastopening - rw_x;
              lastopening += rw_stopx - rw_x;
            }
            else
              ds_p->maskedtexturecol = ds_p->thicksidecol;

            maskedtexture = true;
        }
    }
    
    // calculate rw_offset (only needed for textured lines)
    segtextured = midtexture || toptexture || bottomtexture || maskedtexture || (numthicksides > 0);
    
    if (segtextured)
    {
        offsetangle = rw_normalangle-rw_angle1;
        
        if (offsetangle > ANG180)
            offsetangle = -offsetangle;
        
        if (offsetangle > ANG90)
            offsetangle = ANG90;
        
        sineval = sine_ANG(offsetangle);
        rw_offset = FixedMul (hyp, sineval);
        
        if (rw_normalangle-rw_angle1 < ANG180)
            rw_offset = -rw_offset;
        
        /// don't use texture offset for splats
        rw_offset2 = rw_offset + curline->offset;
        rw_offset += sidedef->textureoffset + curline->offset;
        rw_centerangle = ANG90 + viewangle - rw_normalangle;
        
        // calculate light table
        //  use different light tables
        //  for horizontal / vertical / diagonal
        if (!fixedcolormap)
        {
            vlight = frontsector->lightlevel + extralight + orient_light;

            walllights =
                (vlight < 0) ? scalelight[0]
              : (vlight >= 255) ? scalelight[LIGHTLEVELS-1]
              : scalelight[vlight>>LIGHTSEGSHIFT];
        }
    }
    
    // if a floor / ceiling plane is on the wrong side
    //  of the view plane, it is definitely invisible
    //  and doesn't need to be marked.
    
    //added:18-02-98: WATER! cacher ici dans certaines conditions?
    //                la surface eau est visible de dessous et dessus...
    if (frontsector->model > SM_fluid)
    {
        if (frontsector->floorheight >= viewz)
        {
            // above view plane
            markfloor = false;
        }

        if (frontsector->ceilingheight <= viewz
            && frontsector->ceilingpic != skyflatnum)
        {
            // below view plane
            markceiling = false;
        }
    }

    // calculate incremental stepping values for texture edges
    worldtop >>= 4;
    worldbottom >>= 4;
    
    topstep = -FixedMul (rw_scalestep, worldtop);
    topfrac = (centeryfrac>>4) - FixedMul (worldtop, rw_scale);

    bottomstep = -FixedMul (rw_scalestep,worldbottom);
    bottomfrac = (centeryfrac>>4) - FixedMul (worldbottom, rw_scale);        

    // [WDJ] Intercept overflow in FixedMul math
    if( bottomfrac < topfrac )
    {
       // enable print to see where this happens
//     debug_Printf("Overflow mult: bottomfrac(%i) < topfrac(%i)\n", bottomfrac, topfrac );
       return;
    }

    dc_numlights = frontsector->numlights;
    if( dc_numlights )  // has ff_lights
    {
      if(dc_numlights >= dc_maxlights)    expand_lightlist();

      cnt = 0; // cnt of rlight created, some ff_light will be skipped
      // Setup dc_lightlist, as to 3dfloor heights, lights, and extra_colormap.
      // highest light to lowest light, [0] is sector light at top
      for(i = 0; i < dc_numlights; i++)
      {
        // [WDJ] This makes the dc_lightlist shorter in the render loop, but the resultant
        // entries dc_lightlist[i] will not correspond to the sector lightlist[i].
        // All references to ff_light must be done here.
        ff_light = &frontsector->lightlist[i];
        rlight = &dc_lightlist[cnt];

        if(i != 0)
        {
          if(ff_light->height < frontsector->floorheight)
            continue;

          if(ff_light->height > frontsector->ceilingheight)
            if(i+1 < dc_numlights && frontsector->lightlist[i+1].height > frontsector->ceilingheight)
              continue;
        }
        rlight->height = (centeryfrac>>4) - FixedMul((ff_light->height - viewz) >> 4, rw_scale);
        rlight->heightstep = -FixedMul (rw_scalestep, (ff_light->height - viewz) >> 4);
        rlight->flags = ff_light->flags;
        if(ff_light->caster && ff_light->caster->flags & FF_SOLID)
        {
#if 0
          // [WDJ] At some time this became unused.
          lheight = (*ff_light->caster->bottomheight > frontsector->ceilingheight) ?
              frontsector->ceilingheight + FRACUNIT
             : *ff_light->caster->bottomheight;
#endif
          // in screen coord.
          rlight->botheight = (centeryfrac >> 4) - FixedMul((*ff_light->caster->bottomheight - viewz) >> 4, rw_scale);
          rlight->botheightstep = -FixedMul (rw_scalestep, (*ff_light->caster->bottomheight - viewz) >> 4);
        }

#if 1
// Was in SegLoop, which was an error, because the dc_lightlist entries do NOT correspond to
// the sector lightlist[i] entries.
// From R_Prep3DFloors:
//   ff_light->caster = ff_floor, NULL for top floor
//   ff_light->caster->flags = ff_floor->flags = ff_light->flags = rlight->flags
//   ff_light->height = sector->ceilingheight + 1;
        // [WDJ] Cannot figure out what the height test accomplishes.
        // This seems to be close to the original, whatever it did.
        if( ff_light->caster && ff_light->caster->flags & FF_FOG
            && (ff_light->height != *ff_light->caster->bottomheight))
            rlight->flags |= FF_FOG;
#endif

        rlight->lightlevel = *ff_light->lightlevel;
        rlight->extra_colormap = ff_light->extra_colormap;

        cnt++;
      }
      dc_numlights = cnt;
    }

    if(numffplane)
    {
      for(i = 0; i < numffplane; i++)
      {
        ffplane[i].front_pos >>= 4;
        ffplane[i].front_step = FixedMul(-rw_scalestep, ffplane[i].front_pos);
        ffplane[i].front_frac = (centeryfrac>>4) - FixedMul(ffplane[i].front_pos, rw_scale);
      }
    }

    if (backsector)
    {
        worldbacktop >>= 4;
        worldbackbottom >>= 4;
        
        if (worldbacktop < worldtop)
        {
            pixhigh = (centeryfrac>>4) - FixedMul (worldbacktop, rw_scale);
            pixhighstep = -FixedMul (rw_scalestep,worldbacktop);
        }
        
        if (worldbackbottom > worldbottom)
        {
            pixlow = (centeryfrac>>4) - FixedMul (worldbackbottom, rw_scale);
            pixlowstep = -FixedMul (rw_scalestep,worldbackbottom);
        }

        i = 0;

        if(backsector->ffloors)
        {
            ffloor_t * bff; // backsector fake floor
            for(bff = backsector->ffloors; bff; bff = bff->next)
            {
                if(!(bff->flags & (FF_OUTER_PLANES|FF_INNER_PLANES))
                   || !(bff->flags & FF_EXISTS))
                  continue;

                if(   *bff->bottomheight <= backsector->ceilingheight
                   && *bff->bottomheight >= backsector->floorheight
                   && ((viewz < *bff->bottomheight && (bff->flags & FF_OUTER_PLANES))
                       || (viewz > *bff->bottomheight && (bff->flags & FF_INNER_PLANES))))
                {
                  ffplane[i].valid_mark = true;
                  ffplane[i].back_pos = (*bff->bottomheight - viewz) >> 4;
                  ffplane[i].back_step = FixedMul(-rw_scalestep, ffplane[i].back_pos);
                  ffplane[i].back_frac = (centeryfrac >> 4) - FixedMul(ffplane[i].back_pos, rw_scale);
                  i++;
                  if(i >= MAXFFLOORS)
                      break;
                }
                if(   *bff->topheight >= backsector->floorheight
                   && *bff->topheight <= backsector->ceilingheight
                   && ((viewz > *bff->topheight && (bff->flags & FF_OUTER_PLANES))
                       || (viewz < *bff->topheight && (bff->flags & FF_INNER_PLANES))))
                {
                  ffplane[i].valid_mark = true;
                  ffplane[i].back_pos = (*bff->topheight - viewz) >> 4;
                  ffplane[i].back_step = FixedMul(-rw_scalestep, ffplane[i].back_pos);
                  ffplane[i].back_frac = (centeryfrac >> 4) - FixedMul(ffplane[i].back_pos, rw_scale);
                  i++;
                  if(i >= MAXFFLOORS)
                      break;
                }
            }
        } // if(backsector->ffloors)
        else if(frontsector && frontsector->ffloors)
        {
            ffloor_t * fff; // frontsector fake floor
            for(fff = frontsector->ffloors; fff; fff = fff->next)
            {
                if(!(fff->flags & (FF_OUTER_PLANES|FF_INNER_PLANES))
                   || !(fff->flags & FF_EXISTS))
                  continue;

                if(   *fff->bottomheight <= frontsector->ceilingheight
                   && *fff->bottomheight >= frontsector->floorheight
                   && ((viewz < *fff->bottomheight && (fff->flags & FF_OUTER_PLANES))
                       || (viewz > *fff->bottomheight && (fff->flags & FF_INNER_PLANES))))
                {
                  ffplane[i].valid_mark = true;
                  ffplane[i].back_pos = (*fff->bottomheight - viewz) >> 4;
                  ffplane[i].back_step = FixedMul(-rw_scalestep, ffplane[i].back_pos);
                  ffplane[i].back_frac = (centeryfrac >> 4) - FixedMul(ffplane[i].back_pos, rw_scale);
                  i++;
                  if(i >= MAXFFLOORS)
                      break;
                }
                if(   *fff->topheight >= frontsector->floorheight
                   && *fff->topheight <= frontsector->ceilingheight
                   && ((viewz > *fff->topheight && (fff->flags & FF_OUTER_PLANES))
                       || (viewz < *fff->topheight && (fff->flags & FF_INNER_PLANES))))
                {
                  ffplane[i].valid_mark = true;
                  ffplane[i].back_pos = (*fff->topheight - viewz) >> 4;
                  ffplane[i].back_step = FixedMul(-rw_scalestep, ffplane[i].back_pos);
                  ffplane[i].back_frac = (centeryfrac >> 4) - FixedMul(ffplane[i].back_pos, rw_scale);
                  i++;
                  if(i >= MAXFFLOORS)
                      break;
                }
            }
        }
    } // if backsector
    
    // get a new or use the same visplane
    if (markceiling)
    {
      // visplane global parameter vsp_ceilingplane
      if(vsp_ceilingplane) //SoM: 3/29/2000: Check for null ceiling planes
        vsp_ceilingplane = R_CheckPlane (vsp_ceilingplane, rw_x, rw_stopx-1);
      else
        markceiling = 0;
    }
    
    // get a new or use the same visplane
    if (markfloor)
    {
      // visplane global parameter vsp_floorplane
      if(vsp_floorplane) //SoM: 3/29/2000: Check for null planes
        vsp_floorplane = R_CheckPlane (vsp_floorplane, rw_x, rw_stopx-1);
      else
        markfloor = 0;
    }

    ds_p->numffloorplanes = 0;
    if(numffplane)
    {
      if(firstseg == NULL)
      {
        for(i = 0; i < numffplane; i++)
          ds_p->ffloorplanes[i] = ffplane[i].plane = R_CheckPlane(ffplane[i].plane, rw_x, rw_stopx - 1);

        ds_p->numffloorplanes = numffplane;
        firstseg = ds_p;
      }
      else
      {
        for(i = 0; i < numffplane; i++)
          R_ExpandPlane(ffplane[i].plane, rw_x, rw_stopx - 1);
      }
    }

    // [WDJ] Intercept overflow in math
    if( bottomfrac < topfrac )
    {
       // Enable to see where this happens.
//       debug_Printf("Overflow in call: bottomfrac(%i) < topfrac(%i)\n", bottomfrac, topfrac );
       return;
    }

#ifdef BORIS_FIX
    if( linedef->splats && cv_splats.EV )
    {
        // SoM: Isn't a bit wasteful to copy the ENTIRE array for every drawseg?
        memcpy(&last_ceilingclip[ds_p->x1], &ceilingclip[ds_p->x1], sizeof(short) * (ds_p->x2 - ds_p->x1 + 1));
        memcpy(&last_floorclip[ds_p->x1], &floorclip[ds_p->x1], sizeof(short) * (ds_p->x2 - ds_p->x1 + 1));
        R_RenderSegLoop ();
        R_DrawWallSplats ();
    }
    else
    {
        R_RenderSegLoop ();
    }
#else
    R_RenderSegLoop ();
#ifdef WALLSPLATS
    if (linedef->splats)
        R_DrawWallSplats ();
#endif
#endif
    colfunc = basecolfunc;


    // save sprite clipping info
    if ( ((ds_p->silhouette & SIL_TOP) || maskedtexture)
        && !ds_p->spr_topclip)
    {
        memcpy (lastopening, &ceilingclip[start], 2*(rw_stopx-start));
        ds_p->spr_topclip = lastopening - start;
        lastopening += rw_stopx - start;
    }
    
    if ( ((ds_p->silhouette & SIL_BOTTOM) || maskedtexture)
        && !ds_p->spr_bottomclip)
    {
        memcpy (lastopening, &floorclip[start], 2*(rw_stopx-start));
        ds_p->spr_bottomclip = lastopening - start;
        lastopening += rw_stopx - start;
    }
    
    if (maskedtexture && !(ds_p->silhouette&SIL_TOP))
    {
        ds_p->silhouette |= SIL_TOP;
        // midtexture, 0=no-texture, otherwise valid
        ds_p->sil_top_height = sidedef->midtexture ? FIXED_MIN: FIXED_MAX;
    }
    if (maskedtexture && !(ds_p->silhouette&SIL_BOTTOM))
    {
        ds_p->silhouette |= SIL_BOTTOM;
        // midtexture, 0=no-texture, otherwise valid
        ds_p->sil_bottom_height = sidedef->midtexture ? FIXED_MAX: FIXED_MIN;
    }
    ds_p++;
}
