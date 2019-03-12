// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: wi_stuff.c 1426 2019-01-29 08:09:01Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// $Log: wi_stuff.c,v $
// Revision 1.16  2004/09/12 20:24:26  darkwolf95
// fix: no more animations or "YAH" on top of FS specified interpics
//
// Revision 1.15  2003/05/04 04:21:39  sburke
// Use SHORT macro to convert little-endian shorts on big-endian machines.
//
// Revision 1.14  2001/06/30 15:06:01  bpereira
// fixed wronf next level name in intermission
//
// Revision 1.13  2001/05/16 21:21:15  bpereira
//
// Revision 1.12  2001/05/14 19:02:58  metzgermeister
//   * Fixed floor not moving up with player on E3M1
//   * Fixed crash due to oversized string in screen message ... bad bug!
//   * Corrected some typos
//   * fixed sound bug in SDL
//
// Revision 1.11  2001/03/03 06:17:34  bpereira
// Revision 1.10  2001/02/24 13:35:21  bpereira
// Revision 1.9  2001/02/10 12:27:14  bpereira
// Revision 1.8  2001/01/27 11:02:36  bpereira
//
// Revision 1.7  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.6  2000/11/02 17:50:10  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.5  2000/09/21 16:45:09  bpereira
// Revision 1.4  2000/08/31 14:30:56  bpereira
// Revision 1.3  2000/04/16 18:38:07  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Intermission screens.
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "wi_stuff.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "m_random.h"
#include "r_local.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "i_video.h"
#include "v_video.h"
#include "screen.h"
#include "z_zone.h"
#include "console.h"
#include "p_info.h"
#include "dehacked.h"
  // pars_valid_bex
#include "m_menu.h"
  // M_DrawTextBox

//
// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.
//


//
// Different between registered DOOM (1994) and
//  Ultimate DOOM - Final edition (ultdoom_retail, 1995?).
// This is supposedly ignored for commercial
//  release (aka DOOM II) (doom2_commercial), which had 34 maps
//  in one episode. So there.
#define NUM_EPISODES     4
#define NUM_MAPS_PER_EPI        9


// GLOBAL LOCATIONS
#define WI_TITLEY               2
#define WI_SPACINGY             16
  //TODO: was 33

// SINGPLE-PLAYER STUFF
#define SP_STATSX               50
#define SP_STATSY               50

#define SP_TIMEX                16
#define SP_TIMEY                (BASEVIDHEIGHT-32)


// NET GAME STUFF
#define NG_STATSY               50
#define NG_STATSX               32

#define NG_SPACINGX             64


// DEATHMATCH STUFF
#define DM_MATRIXX              16
#define DM_MATRIXY              24

#define DM_SPACINGX             32

#define DM_TOTALSX              269

#define DM_KILLERSX             0
#define DM_KILLERSY             100
#define DM_VICTIMSX             5
#define DM_VICTIMSY             50
// in sec
#define DM_WAIT                 20



typedef enum
{
    ANIM_ALWAYS,
    ANIM_RANDOM,
    ANIM_LEVEL
} animtype_e;

typedef struct
{
    int         x;
    int         y;
} point_t;


//
// Background Animation in Intermission.
// Texture animation is in p_spec.c.
//
typedef struct
{
    // Setup data (Constant)
    byte        type;  // animtype_e

    // period in tics between animations
    uint16_t    period;  // nominal  35/3

    // number of animation frames
    byte        num_anims;  // nominal 3

    // location of animation
    point_t     loc;

    // type specific setup information
    // ALWAYS: n/a,
    // RANDOM: period deviation (<256),
    // LEVEL: level
    byte        data1;

    // ALWAYS: n/a,
    // RANDOM: random base period,
    // LEVEL: n/a
    byte        data2;

    // Variables
   
    // actual graphics for frames of animations
    patch_t*    p[3];
   
    // following must be initialized to zero before use!

    // next value of bcnt (used in conjunction with period)
    uint32_t    nexttic;

    // next frame number to animate, init to -1, -1 is off
    int8_t      frame_num;   // 0 .. num_anim-1  (0..3)

#if 0
    // [WDJ] Unused

    // last drawn animation frame
    int         lastdrawn;

    // used by RANDOM and LEVEL when animating
    int         state;
#endif

} anim_inter_t;

// Doom YAH nodes (lnode)
static point_t doom_YAH_nodes[NUM_EPISODES][NUM_MAPS_PER_EPI] =
{
    // Episode 0 World Map
    {
        { 185, 164 },   // location of level 0 (CJ)
        { 148, 143 },   // location of level 1 (CJ)
        { 69, 122 },    // location of level 2 (CJ)
        { 209, 102 },   // location of level 3 (CJ)
        { 116, 89 },    // location of level 4 (CJ)
        { 166, 55 },    // location of level 5 (CJ)
        { 71, 56 },     // location of level 6 (CJ)
        { 135, 29 },    // location of level 7 (CJ)
        { 71, 24 }      // location of level 8 (CJ)
    },

    // Episode 1 World Map should go here
    {
        { 254, 25 },    // location of level 0 (CJ)
        { 97, 50 },     // location of level 1 (CJ)
        { 188, 64 },    // location of level 2 (CJ)
        { 128, 78 },    // location of level 3 (CJ)
        { 214, 92 },    // location of level 4 (CJ)
        { 133, 130 },   // location of level 5 (CJ)
        { 208, 136 },   // location of level 6 (CJ)
        { 148, 140 },   // location of level 7 (CJ)
        { 235, 158 }    // location of level 8 (CJ)
    },

    // Episode 2 World Map should go here
    {
        { 156, 168 },   // location of level 0 (CJ)
        { 48, 154 },    // location of level 1 (CJ)
        { 174, 95 },    // location of level 2 (CJ)
        { 265, 75 },    // location of level 3 (CJ)
        { 130, 48 },    // location of level 4 (CJ)
        { 279, 23 },    // location of level 5 (CJ)
        { 198, 48 },    // location of level 6 (CJ)
        { 140, 25 },    // location of level 7 (CJ)
        { 281, 136 }    // location of level 8 (CJ)
    }
};

// Heretic YAH
static point_t Heretic_YAHspot[3][9] =
{
    {
        { 172, 78 },
        { 86, 90 },
        { 73, 66 },
        { 159, 95 },
        { 148, 126 },
        { 132, 54 },
        { 131, 74 },
        { 208, 138 },
        { 52, 101 }
    },
    {
        { 218, 57 },
        { 137, 81 },
        { 155, 124 },
        { 171, 68 },
        { 250, 86 },
        { 136, 98 },
        { 203, 90 },
        { 220, 140 },
        { 279, 106 }
    },
    {
        { 86, 99 },
        { 124, 103 },
        { 154, 79 },
        { 202, 83 },
        { 178, 59 },
        { 142, 58 },
        { 219, 66 },
        { 247, 57 },
        { 107, 80 }
    }
};


//
// Animation locations for episode 0 (1).
// Using patches saves a lot of space,
//  as they replace 320x200 full screen frames.
//
static anim_inter_t epsd0_animinfo[] =
{
    { ANIM_ALWAYS, TICRATE/3, 3, { 224, 104 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 184, 160 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 112, 136 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 72, 112 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 88, 96 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 64, 48 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 192, 40 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 136, 16 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 80, 16 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 64, 24 } }
};

static anim_inter_t epsd1_animinfo[] =
{
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 1 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 2 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 3 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 4 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 5 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 6 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 7 },
    { ANIM_LEVEL, TICRATE/3, 3, { 192, 144 }, 8 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 8 }
};

static anim_inter_t epsd2_animinfo[] =
{
    { ANIM_ALWAYS, TICRATE/3, 3, { 104, 168 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 40, 136 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 160, 96 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 104, 80 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 120, 32 } },
    { ANIM_ALWAYS, TICRATE/4, 3, { 40, 0 } }
};

static byte  num_anim[NUM_EPISODES] =
{
    sizeof(epsd0_animinfo)/sizeof(anim_inter_t),
    sizeof(epsd1_animinfo)/sizeof(anim_inter_t),
    sizeof(epsd2_animinfo)/sizeof(anim_inter_t)
};

static anim_inter_t * anim_inter_info[NUM_EPISODES] =
{
    epsd0_animinfo,
    epsd1_animinfo,
    epsd2_animinfo
};


//
// GENERAL DATA
//

//
// Locally used stuff.
//

// States for the intermission

typedef enum
{
    // [WDJ] There is no good reason for using -1.
    NoState,
    StatCount,
    ShowNextLoc
} state_e;

// States for single-player
#define SP_KILLS                0
#define SP_ITEMS                2
#define SP_SECRET               4
#define SP_FRAGS                6
#define SP_TIME                 8
#define SP_PAR                  ST_TIME

#define SP_PAUSE                1

// in seconds
#define SHOWNEXTLOCDELAY        4
//#define SHOWLASTLOCDELAY      SHOWNEXTLOCDELAY


// used to accelerate or skip a stage
static byte             accelerate_stage;

// signals to refresh everything for one frame
static byte             first_refresh;

// wbs->pnum
static int              me;

// specifies current state
static byte            state;  // state_e

// contains information passed into intermission
static wb_start_t     * wbs;

static wb_player_t    * wb_plyr;  // wbs->plyr[]

// used for general timing
static int              cnt;

// used for timing of background animation
static uint32_t         bcnt;

static int              cnt_kills[MAXPLAYERS];
static int              cnt_items[MAXPLAYERS];
static int              cnt_secret[MAXPLAYERS];
static int              cnt_time;
static int              cnt_par;
static int              cnt_pause;


//
//      GRAPHICS
//
// [WDJ] all patches are saved endian fixed

// background (map of levels).
//static patch_t*       bg;
static char             bgname[9];

// You Are Here graphic
// [2]  - You Were Here - splat
static patch_t*         yah[3];

// %, : graphics
static patch_t*         percent;
static patch_t*         colon;

// 0-9 graphic
static patch_t*         num[10];

// minus sign
static patch_t*         wiminus;

// "Entering" and "Finished!" graphics
static patch_t*         finished;
static patch_t*         entering;

// "secret"
static patch_t*         sp_secret;

 // "Kills", "Scrt", "Items", "Frags"
static patch_t*         kills;
static patch_t*         secret;
static patch_t*         items;
static patch_t*         frags;

// Time sucks.
static patch_t*         timePatch;
static patch_t*         par;
static patch_t*         sucks;

// "killers", "victims"
static patch_t*         killers;
static patch_t*         victims;

// "Total", your face, your dead face
static patch_t*         total;
static patch_t*         pl_face;
static patch_t*         dead_face;

//added:08-02-98: use STPB0 for all players, but translate the colors
static patch_t*         stpb;

// Name graphics of each level (centered)
static patch_t**        lnames = NULL;

// # of doom2_commercial levels
static int              num_lnames = 0;


// [WDJ] All patch endian conversion is done in W_CachePatchNum

//
// CODE
//

// slam background
// UNUSED static unsigned char *background=0;

// Called by WI_Draw_Stats, WI_Draw_DeathmatchStats, WI_Draw_TeamsStats
// Called by WI_Draw_NetgameStats, WI_Draw_ShowNextLoc
static void WI_Slam_Background(void)
{
    // all WI_Draw_ is from WI_Drawer, draw screen0, scale
   
    // vid : from video setup
    // draw background on screen0
    if( EN_heretic && state == StatCount)
        V_ScreenFlatFill( W_CheckNumForName("FLOOR16") );
    else
    if( rendermode == render_soft ) 
    {
        memcpy(screens[0], screens[1], vid.screen_size);  // background to display
#ifdef DIRTY_RECT
        V_MarkRect (0, 0, vid.width, vid.height);
#endif
    }
    else 
    {
        // [WDJ] was draw to screen1, but hw draw does not differentiate
        // hardware draw, ( to screen0 same as above )
        V_DrawScaledPatch(0, 0, W_CachePatchName(bgname, PU_CACHE));
    }
}

// The ticker is used to detect keys
//  because of timing issues in netgames.
boolean WI_Responder(event_t* ev)
{
    return false;
}


// Draws "<Levelname> Finished!"
static void WI_Draw_LF(void)
{
    // Hardware or software render.
    patch_t * pp, * pf;
    int y = WI_TITLEY;

    // draw <LevelName>
    if( FontBBaseLump )
    {
        V_DrawTextB(P_LevelName(), (BASEVIDWIDTH - V_TextBWidth(P_LevelName()))/2, y);
        y += (5*V_TextBHeight(P_LevelName()))/4;
        V_DrawTextB("Finished", (BASEVIDWIDTH - V_TextBWidth("Finished"))/2, y);
    }
    else
    {
        //[segabor]: 'SHORT' BUG !  [WDJ] Patch read does endian conversion
        pp = lnames[wbs->lev_prev];
        pf = V_patch( pp );  // access patch fields
        V_DrawScaledPatch ((BASEVIDWIDTH - pf->width)/2, y, pp);
        y += (5 * pf->height)/4;
        // draw "Finished!"
        V_DrawScaledPatch ((BASEVIDWIDTH - (V_patch(finished)->width))/2,
                            y, finished);
    }
}



// Draws "Entering <LevelName>"
static void WI_Draw_EL(void)
{
    // Hardware or software render.
    patch_t * pp, * pf;
    int y = WI_TITLEY;

    // draw "Entering"
    if( FontBBaseLump )
    {
        const char * levname = P_LevelNameByNum(wbs->epsd+1, wbs->lev_next+1);
        V_DrawTextB("Entering", (BASEVIDWIDTH - V_TextBWidth("Entering"))/2, y);
        y += (5*V_TextBHeight("Entering"))/4;
        V_DrawTextB( levname, (BASEVIDWIDTH - V_TextBWidth(levname))/2, y);
    }
    else
    {
        //[segabor]: 'SHORT' BUG !    [WDJ] Patch read does endian conversion
        V_DrawScaledPatch((BASEVIDWIDTH - (V_patch(entering)->width))/2,
                          y, entering);
        // draw level
        pp = lnames[wbs->lev_next];
        pf = V_patch( pp );  // access patch fields
        y += (5 * pf->height)/4;

        V_DrawScaledPatch((BASEVIDWIDTH - pf->width)/2, y, pp);
    }

}

// [WDJ] Made more resistent to segfault.
// Doom YAH draw
//  n : YAH index
//  yi : yah index, 2=splat
static void WI_Doom_Draw_YAH ( int  n, int yi )
{
    // Hardware or software render.
    patch_t   * p;
    patch_t   * pf;
    point_t   * lnodes;
    int         left, top;

    lnodes = &doom_YAH_nodes[wbs->epsd][n];

    for(;;)
    {
        p = yah[yi];
        pf = V_patch( p );
        left   = lnodes->x - pf->leftoffset;
        top    = lnodes->y - pf->topoffset;
        if (left >= 0
            && (left + pf->width) < BASEVIDWIDTH
            && top >= 0
            && (top + pf->height) < BASEVIDHEIGHT)
        {
            V_DrawScaledPatch(lnodes->x, lnodes->y, p);
            return;
        }

        // yah[0] -> yah[1]
        if( yi > 0 )  break;
        yi++;
    }

    // DEBUG
    debug_Printf("Could not place patch on level %d\n", n+1);
}


//========================================================================
//
// WI_Heretic_Draw_YAH
//
//========================================================================
static void WI_Heretic_Draw_YAH(void)
{
    int i;
    int x;
    int prevmap;
    point_t *  yah_pts = Heretic_YAHspot[gameepisode-1];

    x = (BASEVIDWIDTH-V_StringWidth("NOW ENTERING:"))/2;
    V_DrawString(x, 10, 0, "NOW ENTERING:");

    x = (BASEVIDWIDTH-V_TextBWidth(P_LevelNameByNum(wbs->epsd+1, wbs->lev_next+1)))/2;
    V_DrawTextB(P_LevelNameByNum(wbs->epsd+1, wbs->lev_next+1), x, 20);

    prevmap = (wbs->lev_prev == 8) ? wbs->lev_next - 1 : wbs->lev_prev;

    for(i=0; i<=prevmap; i++)
    {
        V_DrawScaledPatch(yah_pts[i].x, yah_pts[i].y, yah[2]);  // splat
    }
    if(players[consoleplayer].didsecret)
    {
        V_DrawScaledPatch(yah_pts[8].x, yah_pts[8].y, yah[2]);  // splat
    }
    if(!(bcnt&16) || state == ShowNextLoc)
    { // draw the destination 'X'
        V_DrawScaledPatch(yah_pts[wbs->lev_next].x, yah_pts[wbs->lev_next].y, yah[0]);
    }
}



// Called by WI_Start->WI_Init_Stats
// Called by WI_Start->WI_Init_DeathmatchStats
// Called by WI_Init_ShowNextLoc
// Called by WI_update_ShowNextLoc
static void WI_Init_AnimatedBack(void)
{
    byte       i;
    anim_inter_t*  ai;

        //DarkWolf95:September 12, 2004: Don't draw animations for FS changed interpic
    if (gamemode == doom2_commercial || gamemode == heretic || *info_interpic)
        return;

    if (wbs->epsd > 2)
        return;

    for (i=0; i<num_anim[wbs->epsd]; i++)
    {
        ai = &anim_inter_info[wbs->epsd][i];

        // init variables
        ai->frame_num = -1;

        // specify the next time to draw it
        if (ai->type == ANIM_ALWAYS)
            ai->nexttic = bcnt + 1 + (M_Random()%ai->period);
        else if (ai->type == ANIM_RANDOM)
        {
            // data1 = period deviation, data2 = period base
            ai->nexttic = bcnt + 1 + ai->data2+(M_Random()%ai->data1);
        }
        else if (ai->type == ANIM_LEVEL)
            ai->nexttic = bcnt + 1;
    }

}

static void WI_update_AnimatedBack(void)
{
    byte       i;
    anim_inter_t*  ai;

        //DarkWolf95:September 12, 2004: Don't draw animations for FS changed interpic
    if (gamemode == doom2_commercial || gamemode == heretic || *info_interpic)
        return;

    if (wbs->epsd > 2)
        return;

    for (i=0; i<num_anim[wbs->epsd]; i++)
    {
        ai = &anim_inter_info[wbs->epsd][i];

        if( ai->nexttic > bcnt )  continue;

        switch (ai->type)
        {
          case ANIM_ALWAYS:
            if (++ai->frame_num >= ai->num_anims)   ai->frame_num = 0;
            ai->nexttic = bcnt + ai->period;
            break;

          case ANIM_RANDOM:
            ai->frame_num++;
            if (ai->frame_num == ai->num_anims)
            {
                ai->frame_num = -1;
                // data1 = period deviation, data2 = period base
                ai->nexttic = bcnt + ai->data2 + (M_Random()%ai->data1);
            }
            else
            {
                ai->nexttic = bcnt + ai->period;
            }
            break;

          case ANIM_LEVEL:
            // gawd-awful hack for level anims
            if( state == StatCount && i == 7 )  break;
            // data1 = level
            if( wbs->lev_next == ai->data1 )
            {
                ai->frame_num++;
                if (ai->frame_num == ai->num_anims)   ai->frame_num--;
                ai->nexttic = bcnt + ai->period;
            }
            break;
        }
    }
}

static void WI_Draw_AnimatedBack(void)
{
    byte  i;
    anim_inter_t*  ai; // interpic animation data

    //BP: fixed it was "if (doom2_commercial)" 
        //DarkWolf95:September 12, 2004: Don't draw animations for FS changed interpic
    if (gamemode == doom2_commercial || gamemode == heretic || *info_interpic)
        return;

    if (wbs->epsd > 2)
        return;

    for (i=0 ; i<num_anim[wbs->epsd] ; i++)
    {
        ai = &anim_inter_info[wbs->epsd][i];

        if(ai->frame_num >= 0)
            V_DrawScaledPatch(ai->loc.x, ai->loc.y, ai->p[ai->frame_num]);
    }

}

//
// Draws a number.
// If digits > 0, then use that many digits minimum,
//  otherwise only use as many as necessary.
// Returns new x position.
//
//  n : number to be drawn.  NON_NUMBER is not drawn
//  digits : number of digits,  -1 is variable length

static int WI_Draw_Num ( int  x, int  y,
                        int  n,
                        int  digits )
{
    // Hardware or software render, access patch fields.
    int  fontwidth = V_patch( num[0] )->width;
    int  neg;
    int  temp;

    if (digits < 0)
    {
        if (!n)
        {
            // make variable-length zeros 1 digit long
            digits = 1;
        }
        else
        {
            // figure out # of digits in #
            digits = 0;
            temp = n;

            while (temp)
            {
                temp /= 10;
                digits++;
            }
        }
    }

    neg = n < 0;
    if (neg)
        n = -n;

    // if non-number, do not draw it
    if (n == NON_NUMBER)
        return 0;

    // draw the new number
    while (digits--)
    {
        x -= fontwidth;
        V_DrawScaledPatch(x, y, num[ n % 10 ]);
        n /= 10;
    }

    // draw a minus sign if necessary
    if (neg)
        V_DrawScaledPatch(x-=8, y, wiminus);

    return x;

}

// draw a percentage at x,y, blank when -1, a dash when -100
static void WI_Draw_Percent( int  x, int  y, int  pernum )
{
    if (pernum < 0)
    {
        if (pernum == -100 )  // no secrets, items, etc..
           V_DrawScaledPatch(x, y, wiminus);
        return;
    }

    V_DrawScaledPatch(x, y, percent);
    WI_Draw_Num(x, y, pernum, -1);
}



//
// Display level completion time and par,
//  or "sucks" message if overflow.
//
static void WI_Draw_Time ( int x, int y, int t )
{
    int  timediv;  // div is keyword
    int  n;

    if (t<0)
        return;

    // [WDJ] 1/12/2009 fix crashes in heretic, no sucks
    // Old PAR behavior for id wads, otherwise allow them 24 hrs.
    if( (t <= ((gamedesc.gameflags & GD_idwad)? (61*59) : (24*60*60)) )
        || (sucks == NULL) )
    {
        timediv = 1;

        // Hardware or software render.
        do
        {
            n = (t / timediv) % 60;
            x = WI_Draw_Num(x, y, n, 2) - V_patch(colon)->width;
            timediv *= 60;

            // draw
            if (timediv==60 || t / timediv)
                V_DrawScaledPatch(x, y, colon);

        } while (t / timediv);
    }
    else
    {
        // "sucks"
        V_DrawScaledPatch(x - (V_patch(sucks)->width), y, sucks);
    }
}

// For startup wait, and deathmatch wait.
void WI_Draw_wait( int net_nodes, int net_players, int wait_players, int wait_tics )
{
    int  length = 25, lines = 1;
    char * waitmsg;
    char * msg2 = NULL;

    // Using va_buffer (m_misc.c)
    if( wait_players )
    {
        waitmsg = va("WAIT PLAYERS %2d/%2d : NODES %2d : TIMEOUT %4d",
                    net_players, wait_players, net_nodes, wait_tics/TICRATE);
        length = 38;  // Doom 28, but Heretic text uses more
        if( server )
        {
            lines = 2;
            msg2 = " s = start now,  q = escape";
        }
    }
    else
    {
        waitmsg = va("START IN %4d", wait_tics/TICRATE);
        length = 18;  // Doom 10
    }
    // Heretic: The wait message barely fits within the screen width.
    //i=V_StringWidth(num);
    M_DrawTextBox( 2, 20, length, lines );
    V_DrawString( 12, 28, V_WHITEMAP, waitmsg );
    if( msg2 )
        V_DrawString( 12, 36, V_WHITEMAP, msg2 );
}



// used for write introduce next level
static void WI_Init_NoState(void)
{
    state = NoState;
    accelerate_stage = 0;
    cnt = 10;
}

static void WI_update_NoState(void) {

    WI_update_AnimatedBack();

    if (--cnt==0)
    {
        WI_Release_Data();
        G_NextLevel();
    }

}

static boolean          snl_pointeron = false;


// Called by WI_update_NetgameStats, WI_update_Stats
static void WI_Init_ShowNextLoc(void)
{
    state = ShowNextLoc;
    accelerate_stage = 0;
    cnt = SHOWNEXTLOCDELAY * TICRATE;

    WI_Init_AnimatedBack();
}

static void WI_update_ShowNextLoc(void)
{
    WI_update_AnimatedBack();

    if (!--cnt || accelerate_stage)
        WI_Init_NoState();
    else
        snl_pointeron = (cnt & 31) < 20;
}

// Called by WI_Drawer, WI_Draw_NoState
static void WI_Draw_ShowNextLoc(void)
{

    int  i;
    int  last;

    if (cnt<=0)  // all removed no draw !!!
        return;

    WI_Slam_Background();

    // draw animated background
    WI_Draw_AnimatedBack();

    if( EN_heretic )
    {
        if( gameepisode < 4 )
            WI_Heretic_Draw_YAH();
    }
        //DarkWolf95:September 12, 2004: Don't draw YAH for FS changed interpic
    else
    if ( (gamemode != doom2_commercial)
         && wbs->epsd<=2 && !*info_interpic)
    {
        // You are here  (YAH).
        last = (wbs->lev_prev == 8) ? wbs->lev_next - 1 : wbs->lev_prev;

        // draw a yah splat on taken cities.
        for (i=0 ; i<=last ; i++)
            WI_Doom_Draw_YAH(i, 2);  // splat

        // yah splat the secret level?
        if (wbs->didsecret)
            WI_Doom_Draw_YAH(8, 2);  // splat

        // draw flashing ptr
        if (snl_pointeron)
            WI_Doom_Draw_YAH(wbs->lev_next, 0);  // yah[0] or yah[1]
    }

    // draws which level you are entering..
    if ( EN_doom_etc
         && !((gamemode == doom2_commercial) && (wbs->lev_next == 30)) )
        WI_Draw_EL();

}

// Called by WI_Drawer
static void WI_Draw_NoState(void)
{
    snl_pointeron = true;
    WI_Draw_ShowNextLoc();
}


static int              dm_frags[MAXPLAYERS][MAXPLAYERS];
static int              dm_totals[MAXPLAYERS];

// Called by WI_Start
static void WI_Init_DeathmatchStats(void)
{
    int i, j;

    state = StatCount;
    accelerate_stage = 0;

    cnt_pause = TICRATE*DM_WAIT;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
         if (playeringame[i])
         {
             for(j=0; j<MAXPLAYERS; j++)
             {
                 if( playeringame[j] )
                     dm_frags[i][j] = wb_plyr[i].frags[j];
             }
             
             dm_totals[i] = ST_PlayerFrags(i);
         }
    }

    WI_Init_AnimatedBack();
}

// Called by WI_Ticker
static void WI_update_DeathmatchStats(void)
{
    WI_update_AnimatedBack();

    if( paused )
        return;
    if (cnt_pause>0)   cnt_pause--;
    if (cnt_pause==0)
    {
        S_StartSound(sfx_slop);

        WI_Init_NoState();
    }
}


//  Quick-patch for the Cave party 19-04-1998 !!
//
//  width : the column width
void WI_Draw_Ranking(const char *title, int x, int y, fragsort_t *fragtable,
                    int scorelines, boolean large, int white, int colwidth)
{
    char  buf[33];
    int   i,j;
    int   skin_color, color;
    int   plnum;
    int   frags;
    int   colornum;
    fragsort_t temp;

   
    if( EN_heretic )
        colornum = 230;
    else
        colornum = 0x78;
   
    if( colwidth > 32 )  colwidth=32;

    // sort the frags count
    for (i=0; i<scorelines; i++)
    {
        for(j=0; j<scorelines-1-i; j++)
        {
            if( fragtable[j].count < fragtable[j+1].count )
            {
                temp = fragtable[j];
                fragtable[j] = fragtable[j+1];
                fragtable[j+1] = temp;
            }
        }
    }

    if(title)
        V_DrawString (x, y-14, 0, title);
    // draw rankings
    for (i=0; i<scorelines; i++)
    {
        frags = fragtable[i].count;
        plnum = fragtable[i].num;

        // draw color background
        skin_color = fragtable[i].color;
        color = (skin_color) ?
           SKIN_TO_SKINMAP(skin_color)[ colornum ]
         : reg_colormaps[ colornum ];  // default green skin
        V_DrawScaledFill (x-1,y-1, (large ? 40 : 26),9, color);

        // draw frags count, right justified
        sprintf(buf,"%3i", frags );
        V_DrawString (x+(large ? 32 : 24)-V_StringWidth(buf), y, 0, buf);

        // draw name, truncate to colwidth
        memset(buf, ' ', 32);  // to defeat string centering
        snprintf(buf, 31, "%s", fragtable[i].name );
        buf[colwidth] = 0;  // truncate to column width
        V_DrawString (x+(large ? 64 : 29), y,
                      ((plnum == white) ? V_WHITEMAP : 0), buf);

        y += 12;
        if (y>=BASEVIDHEIGHT)
            break;            // dont draw past bottom of screen
    }
}

#define RANKINGY 60

// Called by WI_Drawer
static void WI_Draw_DeathmatchStats(void)
{
    int          i,j;
    int          scorelines;
    int          whiteplayer;
    fragsort_t   fragtab[MAXPLAYERS];

    // all WI is draw screen0, scale
    WI_Slam_Background();

    // draw animated background
    WI_Draw_AnimatedBack();
    WI_Draw_LF();

    //Fab:25-04-98: when you play, you quickly see your frags because your
    //  name is displayed white, when playback demo, you quickly see who's the
    //  view.
    whiteplayer = demoplayback ? displayplayer : consoleplayer;

    // count frags for each present player
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            fragtab[scorelines].count = dm_totals[i];
            fragtab[scorelines].num   = i;
            fragtab[scorelines].color = players[i].skincolor;
            fragtab[scorelines].name  = player_names[i];
            scorelines++;
        }
    }
    WI_Draw_Ranking("Frags", 5, RANKINGY, fragtab, scorelines, false, whiteplayer, 6);

    // count buchholz
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
                if (playeringame[j] && i!=j)
                     fragtab[scorelines].count+= dm_frags[i][j]*(dm_totals[j]+dm_frags[j][j]);

            fragtab[scorelines].num = i;
            fragtab[scorelines].color = players[i].skincolor;
            fragtab[scorelines].name  = player_names[i];
            scorelines++;
        }
    }
    WI_Draw_Ranking("Buchholz", 85, RANKINGY, fragtab, scorelines, false, whiteplayer, 6);

    // count individual
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
            {
                if (playeringame[j] && i!=j)
                {
                     if(dm_frags[i][j]>dm_frags[j][i])
                         fragtab[scorelines].count+=3;
                     else
                         if(dm_frags[i][j]==dm_frags[j][i])
                              fragtab[scorelines].count+=1;
                }
            }

            fragtab[scorelines].num = i;
            fragtab[scorelines].color = players[i].skincolor;
            fragtab[scorelines].name  = player_names[i];
            scorelines++;
        }
    }
    WI_Draw_Ranking("indiv.", 165, RANKINGY, fragtab, scorelines, false, whiteplayer, 6);

    // count deads
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
            {
                if (playeringame[j])
                     fragtab[scorelines].count+=dm_frags[j][i];
            }
            fragtab[scorelines].num   = i;
            fragtab[scorelines].color = players[i].skincolor;
            fragtab[scorelines].name  = player_names[i];

            scorelines++;
        }
    }
    WI_Draw_Ranking("deads", 245, RANKINGY, fragtab, scorelines, false, whiteplayer, 6);
}

boolean teamingame(int teamnum)
{
   int i;

   if( cv_teamplay.EV == 1 )
   {
       for(i=0;i<MAXPLAYERS;i++)
       {
          if(playeringame[i] && players[i].skincolor==teamnum)
              return true;
       }
   }
   else
   if( cv_teamplay.EV == 2)
   {
       for(i=0;i<MAXPLAYERS;i++)
       {
          if(playeringame[i] && players[i].skin==teamnum)
              return true;
       }
   }
   return false;
}

// Called by WI_Drawer
static void WI_Draw_TeamsStats(void)
{
    int          i,j;
    int          scorelines;
    int          whiteplayer;
    fragsort_t   fragtab[MAXPLAYERS];

    // all WI is draw screen0, scale
    WI_Slam_Background();

    // draw animated background
    WI_Draw_AnimatedBack();
    WI_Draw_LF();

    //Fab:25-04-98: when you play, you quickly see your frags because your
    //  name is displayed white, when playback demo, you quickly see who's the
    //  view.
    if( cv_teamplay.EV == 1 )
        whiteplayer = demoplayback ? displayplayer_ptr->skincolor
                                   : consoleplayer_ptr->skincolor;
    else
        whiteplayer = demoplayback ? displayplayer_ptr->skin
                                   : consoleplayer_ptr->skin;

    // count frags for each present player
    scorelines = HU_Create_TeamFragTbl(fragtab,dm_totals,dm_frags);

    WI_Draw_Ranking("Frags", 5, 80, fragtab, scorelines, false, whiteplayer, 6);

    // count buchholz
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (teamingame(i))
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
            {
                if (teamingame(j) && i!=j)
                    fragtab[scorelines].count+= dm_frags[i][j]*dm_totals[j];
            }

            fragtab[scorelines].num   = i;
            fragtab[scorelines].color = i;
            fragtab[scorelines].name  = get_team_name(i);
            scorelines++;
        }
    }
    WI_Draw_Ranking("Buchholz", 85, 80, fragtab, scorelines, false, whiteplayer, 6);

    // count individuel
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (teamingame(i))
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
            {
                if (teamingame(j) && i!=j)
                {
                     if(dm_frags[i][j]>dm_frags[j][i])
                         fragtab[scorelines].count+=3;
                     else
                         if(dm_frags[i][j]==dm_frags[j][i])
                              fragtab[scorelines].count+=1;
                }
            }

            fragtab[scorelines].num = i;
            fragtab[scorelines].color = i;
            fragtab[scorelines].name  = get_team_name(i);
            scorelines++;
        }
    }
    WI_Draw_Ranking("indiv.", 165, 80, fragtab, scorelines, false, whiteplayer, 6);

    // count deads
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (teamingame(i))
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
            {
                if (teamingame(j))
                     fragtab[scorelines].count+=dm_frags[j][i];
            }
            fragtab[scorelines].num   = i;
            fragtab[scorelines].color = i;
            fragtab[scorelines].name  = get_team_name(i);

            scorelines++;
        }
    }
    WI_Draw_Ranking("deads", 245, 80, fragtab, scorelines, false, whiteplayer, 6);
}


/* old code
#define FB  0
static void WI_ddrawDeathmatchStats(void)
{

    int         i;
    int         j;
    int         x;
    int         y;
    int         w;

    int         lh;     // line height

    byte*       colormap;       //added:08-02-98:see below

    lh = WI_SPACINGY;

    WI_Slam_Background();

    // draw animated background
    WI_Draw_AnimatedBack();
    WI_Draw_LF();

    // draw stat titles (top line)
    V_DrawScaledPatch(DM_TOTALSX - V_patch(total)->width/2,
                DM_MATRIXY-WI_SPACINGY+10,
                total);

    V_DrawScaledPatch(DM_KILLERSX, DM_KILLERSY, killers);
    V_DrawScaledPatch(DM_VICTIMSX, DM_VICTIMSY, victims);

    // draw P?
    x = DM_MATRIXX + DM_SPACINGX;
    y = DM_MATRIXY;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (playeringame[i])
        {
            //added:08-02-98: use V_DrawMappedPatch instead of
            //                    V_DrawScaledPatch, so that the
            // graphics are 'colormapped' to the player's colors!
            if (players[i].skincolor==0)
                colormap = colormaps;
            else
                colormap = (byte *) translationtables - 256 + (players[i].skincolor<<8);

            V_DrawMappedPatch(x - (V_patch(stpb)->width/2),
                        DM_MATRIXY - WI_SPACINGY,
                        stpb,      //p[i], now uses a common STPB0 translated
                        colormap); //      to the right colors

            V_DrawMappedPatch(DM_MATRIXX - (V_patch(stpb)->width/2),
                        y,
                        stpb,      //p[i]
                        colormap);

            if (i == me)
            {
                V_DrawScaledPatch(x - (V_patch(stpb)->width/2),
                            DM_MATRIXY - WI_SPACINGY,
                            dead_face);

                V_DrawScaledPatch(DM_MATRIXX - (V_patch(stpb)->width/2),
                            y,
                            pl_face);
            }
        }
        else
        {
            // V_DrawPatch(x - (V_patch(bp[i])->width/2),
            //   DM_MATRIXY - WI_SPACINGY, FB, bp[i]);
            // V_DrawPatch(DM_MATRIXX - (V_patch(bp[i])->width/2),
            //   y, FB, bp[i]);
        }
        x += DM_SPACINGX;
        y += WI_SPACINGY;
    }

    // draw stats
    y = DM_MATRIXY+10;
    w = V_patch(num[0])->width;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        x = DM_MATRIXX + DM_SPACINGX;

        if (playeringame[i])
        {
            for (j=0 ; j<MAXPLAYERS ; j++)
            {
                if (playeringame[j])
                    WI_Draw_Num(x+w, y, dm_frags[i][j], 2);

                x += DM_SPACINGX;
            }
            WI_Draw_Num(DM_TOTALSX+w, y, dm_totals[i], 2);
        }
        y += WI_SPACINGY;
    }
}

*/

static int      cnt_frags[MAXPLAYERS];
static int      ng_state;
static byte     dofrags;

// Called by WI_Start
static void WI_Init_NetgameStats(void)
{
    int i;
    int cnt_playfrags = 0;

    state = StatCount;
    accelerate_stage = 0;
    ng_state = 1;

    cnt_pause = TICRATE;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (!playeringame[i])
            continue;

        cnt_kills[i] = cnt_items[i] = cnt_secret[i] = cnt_frags[i] = 0;

        cnt_playfrags += ST_PlayerFrags(i);
    }

    dofrags = (cnt_playfrags > 0);

    WI_Init_AnimatedBack();
}



static void WI_update_NetgameStats(void)
{

    int  i, cnt_target;
    boolean     stillticking = false;

    WI_update_AnimatedBack();

    if (accelerate_stage && ng_state != 10)
    {
        accelerate_stage = 0;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_kills[i] = ( wbs->maxkills > 0 ) ?
               (wb_plyr[i].skills * 100) / wbs->maxkills : -100;
            cnt_items[i] = ( wbs->maxitems > 0 ) ?
               (wb_plyr[i].sitems * 100) / wbs->maxitems : -100;
            cnt_secret[i] = ( wbs->maxsecret > 0 ) ?
               (wb_plyr[i].ssecret * 100) / wbs->maxsecret : -100;

            if (dofrags)
                cnt_frags[i] = ST_PlayerFrags(i);
        }
        S_StartSound(sfx_barexp);
        ng_state = 10;
    }

    if (ng_state == 2)
    {
        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            if( wbs->maxkills <= 0 )
            {
               // no kills
               cnt_kills[i] = -100;
               continue;
            }

            cnt_target = (wb_plyr[i].skills * 100) / wbs->maxkills;
            cnt_kills[i] += 2;
            if (cnt_kills[i] >= cnt_target)
                cnt_kills[i] = cnt_target;
            else
                stillticking = true;
        }

        if (!stillticking)
            goto next_state;
    }
    else if (ng_state == 4)
    {
        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            if( wbs->maxitems <= 0 )
            {
               // no items
               cnt_items[i] = -100;
               continue;
            }

            cnt_target = (wb_plyr[i].sitems * 100) / wbs->maxitems;
            cnt_items[i] += 2;
            if (cnt_items[i] >= cnt_target)
                cnt_items[i] = cnt_target;
            else
                stillticking = true;
        }
        if (!stillticking)
            goto next_state;
    }
    else if (ng_state == 6)
    {
        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            if( wbs->maxsecret <= 0 )
            {
               // no secrets
               cnt_secret[i] = -100;
               continue;
            }

            cnt_target = (wb_plyr[i].ssecret * 100) / wbs->maxsecret;
            cnt_secret[i] += 2;
            if (cnt_secret[i] >= cnt_target)
                cnt_secret[i] = cnt_target;
            else
                stillticking = true;
        }

        if (!stillticking)
        {
            // skip ng_state 8 if no frags
            if ( !dofrags )  ng_state += 2;
            goto next_state;
        }
    }
    else if (ng_state == 8)
    {
        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_target = ST_PlayerFrags(i);
            cnt_frags[i] += 1;
            if (cnt_frags[i] >= cnt_target)
                cnt_frags[i] = cnt_target;
            else
                stillticking = true;
        }

        if (!stillticking)
        {
            S_StartSound(sfx_pldeth);
            ng_state++;
            goto done;
        }
    }
    else if (ng_state == 10)
    {
        if (accelerate_stage)
        {
            S_StartSound(sfx_sgcock);
            if ( gamemode == doom2_commercial )
                WI_Init_NoState();
            else
                WI_Init_ShowNextLoc();
        }
        goto done;
    }
    else if (ng_state & 1)
    {
        if (!--cnt_pause)
        {
            ng_state++;
            cnt_pause = TICRATE;
        }
        goto done;
    }

    // tick sound, a single place to change it
    if (!(bcnt&3))
        S_StartSound(sfx_pistol);
    return;
   
next_state:
    S_StartSound(sfx_barexp);
    ng_state++;
done:
    return;
}



// Called by WI_Drawer
static void WI_Draw_NetgameStats(void)
{
    // Hardware or software render.
    int  i, x, y;
    int  pwidth, ngsx;
    byte*       colormap;   //added:08-02-98: remap STBP0 to player color

    // all WI is draw screen0, scale
    WI_Slam_Background();

    // draw animated background
    WI_Draw_AnimatedBack();

    WI_Draw_LF();

    ngsx = NG_STATSX + (V_patch(pl_face)->width/2) + (dofrags? 0 : 32);
    // draw stat titles (top line)
    if( FontBBaseLump )
    {
        // use FontB if any
        V_DrawTextB("Kills", ngsx +  NG_SPACINGX - V_TextBWidth("Kills"), NG_STATSY);
        V_DrawTextB("Items", ngsx + 2*NG_SPACINGX - V_TextBWidth("Items"), NG_STATSY);
        V_DrawTextB("Scrt", ngsx + 3*NG_SPACINGX - V_TextBWidth("Scrt"), NG_STATSY);
        if (dofrags)
            V_DrawTextB("Frgs", ngsx + 4*NG_SPACINGX - V_TextBWidth("Frgs"), NG_STATSY);

        y = NG_STATSY + V_TextBHeight("Kills");
    }
    else
    {
        V_DrawScaledPatch(ngsx + NG_SPACINGX - (V_patch(kills)->width),
            NG_STATSY, kills);
        
        V_DrawScaledPatch(ngsx + 2*NG_SPACINGX - (V_patch(items)->width),
            NG_STATSY, items);
        
        V_DrawScaledPatch(ngsx + 3*NG_SPACINGX - (V_patch(secret)->width),
            NG_STATSY, secret);
        if (dofrags)
            V_DrawScaledPatch(ngsx + 4*NG_SPACINGX - (V_patch(frags)->width),
                              NG_STATSY, frags);
        // draw stats
        y = NG_STATSY + (V_patch(kills)->height);
    }


    pwidth = V_patch(percent)->width;
    //added:08-02-98: p[i] replaced by stpb (see WI_Load_Data for more)
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (!playeringame[i])
            continue;

        x = ngsx;
        colormap = (players[i].skincolor) ?
             SKIN_TO_SKINMAP( players[i].skincolor ) // skins 1..
           : & reg_colormaps[0]; // no translation table for green guy

        V_DrawMappedPatch(x - (V_patch(stpb)->width), y, stpb, colormap);

        if (i == me)
            V_DrawScaledPatch(x - (V_patch(stpb)->width), y, pl_face);

        x += NG_SPACINGX;
        WI_Draw_Percent(x-pwidth, y+10, cnt_kills[i]);
        x += NG_SPACINGX;
        WI_Draw_Percent(x-pwidth, y+10, cnt_items[i]);
        x += NG_SPACINGX;
        WI_Draw_Percent(x-pwidth, y+10, cnt_secret[i]);
        x += NG_SPACINGX;

        if (dofrags)
            WI_Draw_Num(x, y+10, cnt_frags[i], -1);

        y += WI_SPACINGY;
    }

}

static int sp_state;

// Called by WI_Start
static void WI_Init_Stats(void)
{
    state = StatCount;
    accelerate_stage = 0;
    sp_state = 1;
    cnt_kills[0] = cnt_items[0] = cnt_secret[0] = -1;
    cnt_time = cnt_par = -1;
    cnt_pause = TICRATE;

    WI_Init_AnimatedBack();
}

static void WI_update_Stats(void)
{

    WI_update_AnimatedBack();

    if (accelerate_stage && sp_state != 10)
    {
        accelerate_stage = 0;
        cnt_kills[0] = ( wbs->maxkills > 0 ) ?
           (wb_plyr[me].skills * 100) / wbs->maxkills : -100;
        cnt_items[0] = ( wbs->maxitems > 0 ) ?
           (wb_plyr[me].sitems * 100) / wbs->maxitems : -100;
        cnt_secret[0] = ( wbs->maxsecret > 0 ) ?
           (wb_plyr[me].ssecret * 100) / wbs->maxsecret : -100;
        cnt_time = wb_plyr[me].stime / TICRATE;
        cnt_par = wbs->partime / TICRATE;
        S_StartSound(sfx_barexp);
        sp_state = 10;
    }

    if (sp_state == 2)
    {
        if ( wbs->maxkills <= 0 )
        {
            cnt_kills[0] = -100;
            goto next_state;  // no kills
        }

        cnt_kills[0] += 2;

        if (cnt_kills[0] >= (wb_plyr[me].skills * 100) / wbs->maxkills)
        {
            cnt_kills[0] = (wb_plyr[me].skills * 100) / wbs->maxkills;
            goto next_state;
        }
    }
    else if (sp_state == 4)
    {
        if ( wbs->maxitems <= 0 )
        {
            cnt_items[0] = -100;
            goto next_state;  // no items
        }

        cnt_items[0] += 2;

        if (cnt_items[0] >= (wb_plyr[me].sitems * 100) / wbs->maxitems)
        {
            cnt_items[0] = (wb_plyr[me].sitems * 100) / wbs->maxitems;
            goto next_state;
        }
    }
    else if (sp_state == 6)
    {
        if ( wbs->maxsecret <= 0 )
        {
            cnt_secret[0] = -100;
            goto next_state;  // no secrets
        }

        cnt_secret[0] += 2;

        if (cnt_secret[0] >= (wb_plyr[me].ssecret * 100) / wbs->maxsecret)
        {
            cnt_secret[0] = (wb_plyr[me].ssecret * 100) / wbs->maxsecret;
            goto next_state;
        }
    }

    else if (sp_state == 8)
    {
        cnt_time += 3;

        if (cnt_time >= wb_plyr[me].stime / TICRATE)
            cnt_time = wb_plyr[me].stime / TICRATE;

        cnt_par += 3;

        if (cnt_par >= wbs->partime / TICRATE)
        {
            cnt_par = wbs->partime / TICRATE;

            if (cnt_time >= wb_plyr[me].stime / TICRATE)
                goto next_state;
        }
    }
    else if (sp_state == 10)
    {
        if (accelerate_stage)
        {
            S_StartSound(sfx_sgcock);

            if (gamemode == doom2_commercial)
                WI_Init_NoState();
            else
                WI_Init_ShowNextLoc();
        }
        goto done;
    }
    else if (sp_state & 1)
    {
        if (!--cnt_pause)
        {
            sp_state++;
            cnt_pause = TICRATE;
        }
        goto done;
    }
   
    // tick sound, a single place to change it
    if (!(bcnt&3))
        S_StartSound(sfx_pistol);
    return;

next_state:
    // done incrementing the count
    S_StartSound(sfx_barexp);
    sp_state++;
done:
    return;
}

// Called by WI_Drawer
static void WI_Draw_Stats(void)
{
    // all WI is draw screen0, scale
    // [WDJ] Display PAR for certain id games, unless modified,
    // but not PWAD unless BEX has set PARS.
    boolean draw_pars = pars_valid_bex
     || ( EN_doom_etc
          && (gamedesc.gameflags & GD_idwad)
          && (wbs->epsd < 3)
          && !modifiedgame );
    // Hardware or softare render.
    // line height
    int lh = (3 * (V_patch(num[0])->height))/2;

    // all WI is draw screen0, scale
    WI_Slam_Background();

    // draw animated background
    WI_Draw_AnimatedBack();

    WI_Draw_LF();

    if( FontBBaseLump )
    {
        // use FontB if any
        V_DrawTextB("Kills", SP_STATSX, SP_STATSY);
        V_DrawTextB("Items", SP_STATSX, SP_STATSY+lh);
        V_DrawTextB("Secrets", SP_STATSX, SP_STATSY+2*lh);
        V_DrawTextB("Time", SP_TIMEX, SP_TIMEY);
        if (draw_pars)
            V_DrawTextB("Par", BASEVIDWIDTH/2 + SP_TIMEX, SP_TIMEY);
    }
    else
    {
        V_DrawScaledPatch(SP_STATSX, SP_STATSY, kills);
        V_DrawScaledPatch(SP_STATSX, SP_STATSY+lh, items);
        V_DrawScaledPatch(SP_STATSX, SP_STATSY+2*lh, sp_secret);
        V_DrawScaledPatch(SP_TIMEX, SP_TIMEY, timePatch);
        if (draw_pars)
            V_DrawScaledPatch(BASEVIDWIDTH/2 + SP_TIMEX, SP_TIMEY, par);
    }
    WI_Draw_Percent(BASEVIDWIDTH - SP_STATSX, SP_STATSY, cnt_kills[0]);
    WI_Draw_Percent(BASEVIDWIDTH - SP_STATSX, SP_STATSY+lh, cnt_items[0]);
    WI_Draw_Percent(BASEVIDWIDTH - SP_STATSX, SP_STATSY+2*lh, cnt_secret[0]);
    WI_Draw_Time(BASEVIDWIDTH/2 - SP_TIMEX, SP_TIMEY, cnt_time);

    if (draw_pars)
        WI_Draw_Time(BASEVIDWIDTH - SP_TIMEX, SP_TIMEY, cnt_par);

}

// Called by WI_Ticker
static void WI_checkForAccelerate(void)
{
    int   i;
    player_t  *player;

    // check for button presses to skip delays
    for (i=0, player = players ; i<MAXPLAYERS ; i++, player++)
    {
        if (playeringame[i])
        {
            if (player->cmd.buttons & BT_ATTACK)
            {
                if (!player->attackdown)
                    accelerate_stage = 1;
                player->attackdown = true;
            }
            else
                player->attackdown = false;
            if (player->cmd.buttons & BT_USE)
            {
                if (!player->usedown)
                    accelerate_stage = 1;
                player->usedown = true;
            }
            else
                player->usedown = false;
        }
    }
}



// Updates stuff each tick
void WI_Ticker(void)
{
    // counter for general background animation
    bcnt++;

    if (bcnt == 1)
    {
        // intermission music
        if ( gamemode == doom2_commercial )
          S_ChangeMusic(mus_dm2int, true);
        else
          S_ChangeMusic(mus_inter, true);
    }

    WI_checkForAccelerate();

    switch (state)
    {
      case StatCount:
        if( cv_deathmatch.EV )
            WI_update_DeathmatchStats();
        else if (multiplayer)
            WI_update_NetgameStats();
        else
            WI_update_Stats();
        break;

      case ShowNextLoc:
        WI_update_ShowNextLoc();
        break;

      case NoState:
        WI_update_NoState();
        break;
    }

}

// [WDJ] Patch lists.

byte doom_wi_patches_loaded = 0;
load_patch_t  doom_wi_patches[] =
{
  { &finished, "WIF" },
  { &entering, "WIENTER" },
  { &kills, "WIOSTK" },
  { &secret, "WIOSTS" },
  { &sp_secret, "WISCRT2" },
  { &items, "WIOSTI" },
  { &frags, "WIFRGS" },
  { &timePatch, "WITIME" },
  { &sucks, "WISUCKS" },
  { &par, "WIPAR" },
  { &killers, "WIKILRS" },  // vertical
  { &victims, "WIVCTMS" },  // horiz
  { &total, "WIMSTT" },
  { &yah[0], "WIURH0" },   // yah point RH
  { &yah[1], "WIURH1" },   // yad point LH
  { &yah[2], "WISPLAT" },  // yah splat
  { &colon, "WICOLON" },
  { &percent, "WIPCNT" },
  { &wiminus, "WIMINUS" },
  { NULL, NULL }
};


byte heretic_wi_patches_loaded = 0;
load_patch_t  heretic_wi_patches[13] =
{
  { &yah[0], "IN_YAH" }, // yah point
  { &yah[2], "IN_X" },  // yah splat
  { &colon, "FONTB26" },
  { &percent, "FONTB05" },
  { &wiminus, "FONTB13" },
  { NULL, NULL }
};

     

// Called by WI_Start, SCR_SetMode
void WI_Load_Data(void)
{
    // vid : from video setup
    int   i;
    anim_inter_t*  ai; // interpic animation data
    // [Stylinski] Compiler warns buffer overrun, requires [17], maybe up to [27].
    char  name[28];
    byte  j;
    byte  wb_epsd;


    if( (info_interpic == NULL) || (wbs == NULL) )  return;
   
    // [WDJ] Lock the interpic graphics against release by other users.

    wb_epsd = wbs->epsd;
    // choose the background of the intermission
    if (*info_interpic)
        strcpy(bgname, info_interpic);
    else if (gamemode == doom2_commercial)
        strcpy(bgname, "INTERPIC");
    else if( gamemode == heretic )
        sprintf(bgname, "MAPE%d", wb_epsd+1);
    else
        sprintf(bgname, "WIMAP%d", wb_epsd);

    if ( gamemode == ultdoom_retail )
    {
        if (wb_epsd == 3)
            strcpy(bgname,"INTERPIC");
    }
    
    
    if( rendermode == render_soft )
    {
        memset(screens[0], 0, vid.screen_size);

        // clear backbuffer from status bar stuff and borders
        memset(screens[1], 0, vid.screen_size);
  
        // Draw background on screen1
        V_SetupDraw( 1 | V_SCALESTART | V_SCALEPATCH | V_CENTERHORZ ); // screen 1
        V_DrawScaledPatch(0, 0, W_CachePatchName(bgname, PU_CACHE));
        V_SetupDraw( drawinfo.prev_screenflags );  // restore
    }

    // UNUSED unsigned char *pic = screens[1];
    // if (gamemode == doom2_commercial)
    // {
    // darken the background image
    // while (pic != screens[1] + SCREENHEIGHT*SCREENWIDTH)
    // {
    //   *pic = colormaps[256*25 + *pic];
    //   pic++;
    // }
    //}

    if (gamemode == doom2_commercial)
    {
        num_lnames = 32;
        lnames = (patch_t **) Z_Malloc(sizeof(patch_t*) * num_lnames,
                                       PU_STATIC, 0);
        for (i=0 ; i<num_lnames ; i++)
        {
            sprintf(name, "CWILV%2.2d", i);
            lnames[i] = W_CachePatchName(name, PU_LOCK_SB);
        }
    }
    else
    {
        // doom1, doom, doomu
        num_lnames = NUM_MAPS_PER_EPI;
        lnames = (patch_t **) Z_Malloc(sizeof(patch_t*) * NUM_MAPS_PER_EPI,
                                       PU_STATIC, 0);
        for (i=0 ; i<NUM_MAPS_PER_EPI ; i++)
        {
            sprintf(name, "WILV%d%d", wb_epsd, i);
            lnames[i] = W_CachePatchName(name, PU_LOCK_SB);
        }

        if (wb_epsd < 3)
        {
            for (j=0; j<num_anim[wb_epsd]; j++)
            {
                ai = &anim_inter_info[wb_epsd][j];
                for (i=0; i<ai->num_anims; i++)
                {
                    if(wb_epsd == 1 && j == 8)  // shares
                    {
                        // [1][8] shares the patch of [1][4]
                        ai->p[i] = anim_inter_info[1][4].p[i];
                        continue;
                    }

                    // animations
                    sprintf(name, "WIA%d%.2d%.2d", wb_epsd, j, i);
                    ai->p[i] = W_CachePatchName(name, PU_LOCK_SB);
                }
            }
        }
    }

    for (i=0;i<10;i++)
    {
         // numbers 0-9
        if( EN_heretic )
            sprintf(name, "FONTB%d", 16+i);
        else
            sprintf(name, "WINUM%d", i);
        num[i] = W_CachePatchName(name, PU_LOCK_SB);
    }

    if( EN_doom_etc )
    {
        load_patch_list( doom_wi_patches );
        doom_wi_patches_loaded = 1;
    }
    else if( EN_heretic )
    {
        load_patch_list( heretic_wi_patches );
        heretic_wi_patches_loaded = 1;
    }
    
    // your face
    pl_face = W_CachePatchName("STFST01", PU_LOCK_SB);  // never unlocked

    // dead face
    dead_face = W_CachePatchName("STFDEAD0", PU_LOCK_SB);  // never unlocked


    //added:08-02-98: now uses a single STPB0 which is remapped to the
    //                player translation table. Whatever new colors we add
    //                since we'll have to define a translation table for
    //                it, we'll have the right colors here automatically.
    stpb = W_CachePatchName("STPB0", PU_LOCK_SB);  // never unlocked
}

// Called by  WI_update_NoState, SCR_SetMode
void WI_Release_Data(void)
{
    byte j;
    byte wb_epsd;

    //faB: never Z_ChangeTag() a pointer returned by W_CachePatchxxx()
    //     it doesn't work and is unecessary
    if( lnames )
    {
      release_patch_array( num, 10 );
      release_patch_array( lnames, num_lnames );

      Z_Free(lnames);
      lnames = NULL;

      if( (gamemode != doom2_commercial) && wbs )
      {
        wb_epsd = wbs->epsd;
        if (wb_epsd < 3)
        {
            for (j=0; j<num_anim[wb_epsd]; j++)
            {
                if(wb_epsd == 1 && j == 8)  continue;  // shared

                release_patch_array( anim_inter_info[wb_epsd][j].p,
                                     anim_inter_info[wb_epsd][j].num_anims );
            }
        }
      }
    }

    if( doom_wi_patches_loaded )
    {
        release_patch_list( doom_wi_patches );
        doom_wi_patches_loaded = 0;
    }

    if( heretic_wi_patches_loaded )
    {
        release_patch_list( heretic_wi_patches );
        heretic_wi_patches_loaded = 0;
    }
}

void WI_Drawer (void)
{
    // all WI is draw screen0, scale
    V_SetupDraw( 0 | V_SCALESTART | V_SCALEPATCH | V_CENTERHORZ );

    switch (state)
    {
      case StatCount:
        if( cv_deathmatch.EV )
        {
            if( cv_teamplay.EV )
                WI_Draw_TeamsStats();
            else
                WI_Draw_DeathmatchStats();

            WI_Draw_wait( 0, 0, 0, cnt_pause );
        }
        else if (multiplayer)
            WI_Draw_NetgameStats();
        else
            WI_Draw_Stats();
        break;

      case ShowNextLoc:
        WI_Draw_ShowNextLoc();
        break;

      case NoState:
        WI_Draw_NoState();
        break;
    }
}


static void WI_Init_Variables( wb_start_t * wb_start)
{

    wbs = wb_start;

#ifdef RANGECHECKING
    if (gamemode != doom2_commercial)
    {
      if ( gamemode == ultdoom_retail )
        RNGCHECK(wbs->epsd, 0, 3);
      else
        RNGCHECK(wbs->epsd, 0, 2);
    }
    else
    {
        RNGCHECK(wbs->lev_prev, 0, 8);
        RNGCHECK(wbs->lev_next, 0, 8);
    }
    RNGCHECK(wbs->pnum, 0, MAXPLAYERS);
    RNGCHECK(wbs->pnum, 0, MAXPLAYERS);
#endif

    accelerate_stage = 0;
    cnt = bcnt = 0;
    first_refresh = 1;
    me = wbs->pnum;
    wb_plyr = wbs->plyr;

    if ( gamemode != ultdoom_retail )
    {
      if (wbs->epsd > 2)
        wbs->epsd -= 3;
    }
}

void WI_Start(wb_start_t * wb_start)
{

    WI_Init_Variables(wb_start);
    WI_Load_Data();

    if( cv_deathmatch.EV )
        WI_Init_DeathmatchStats();
    else if (multiplayer)
        WI_Init_NetgameStats();
    else
        WI_Init_Stats();
}
