// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: st_stuff.c 1426 2019-01-29 08:09:01Z wesleyjohnson $
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
// $Log: st_stuff.c,v $
// Revision 1.22  2003/08/11 13:50:00  hurdler
// go final + translucent HUD + fix spawn in net game
//
// Revision 1.21  2001/08/20 21:37:35  hurdler
// fix palette in splitscreen + hardware mode
//
// Revision 1.20  2001/08/20 20:40:39  metzgermeister
//
// Revision 1.19  2001/08/08 20:34:43  hurdler
// Big TANDL update
//
// Revision 1.18  2001/05/16 21:21:14  bpereira
// Revision 1.17  2001/04/01 17:35:07  bpereira
// Revision 1.16  2001/03/03 06:17:34  bpereira
// Revision 1.15  2001/02/24 13:35:21  bpereira
// Revision 1.14  2001/02/10 13:05:45  hurdler
//
// Revision 1.13  2001/01/31 17:14:07  hurdler
// Add cv_scalestatusbar in hardware mode
//
// Revision 1.12  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.11  2000/11/02 19:49:37  bpereira
//
// Revision 1.10  2000/10/04 16:34:51  hurdler
// Change a little the presentation of monsters/secrets numbers
//
// Revision 1.9  2000/10/02 18:25:45  bpereira
// Revision 1.8  2000/10/01 10:18:19  bpereira
//
// Revision 1.7  2000/10/01 01:12:00  hurdler
// Add number of monsters and secrets in overlay
//
// Revision 1.6  2000/09/28 20:57:18  bpereira
//
// Revision 1.5  2000/09/25 19:28:15  hurdler
// Enable Direct3D support as OpenGL
//
// Revision 1.4  2000/09/21 16:45:09  bpereira
// Revision 1.3  2000/08/31 14:30:56  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Status bar code.
//      Does the face/direction indicator animatin.
//      Does palette indicators as well (red pain/berserk, bright pickup)
//
//-----------------------------------------------------------------------------

#include "doomincl.h"

#include "am_map.h"

#include "g_game.h"
#include "m_cheat.h"

#include "screen.h"
#include "r_local.h"
#include "p_local.h"
#include "p_inter.h"
#include "m_random.h"

#include "st_stuff.h"
#include "st_lib.h"
#include "i_video.h"
#include "v_video.h"

#include "keys.h"

#include "z_zone.h"
#include "hu_stuff.h"
#include "d_main.h"

#ifdef HWRENDER
#include "hardware/hw_drv.h"
#include "hardware/hw_main.h"
#endif

// Doom and Heretic use ST_Drawer.
// Almost everything else that is ST_  is Doom Only.
// Heretic has its own status bar drawer that uses SB_ functions in sb_bar.c.
// There is very little shared code, but some sharing attempts are being made.

//protos
static void ST_Create_Widgets(void);


#define FLASH_COLOR  0x72

//
// STATUS BAR DATA
//

// Palette indices.
// For damage/bonus red-/gold-shifts
#define STARTREDPALS            1
#define STARTBONUSPALS          9
#define NUMREDPALS              8
#define NUMBONUSPALS            4
// Radiation suit, green shift.
#define RADIATIONPAL            13

// N/256*100% probability
//  that the normal face state will change
#define ST_FACEPROBABILITY              96

// For Responder
#define ST_TOGGLECHAT           KEY_ENTER

// Location of status bar
  //added:08-01-98:status bar position changes according to resolution.
#define ST_FX                     143
// This is now dynamic
// #define ST_Y                    stbar_y

// Number of status faces.
#define ST_NUMPAINFACES         5
#define ST_NUMSTRAIGHTFACES     3
#define ST_NUMTURNFACES         2
#define ST_NUMSPECIALFACES      3

#define ST_FACESTRIDE \
          (ST_NUMSTRAIGHTFACES+ST_NUMTURNFACES+ST_NUMSPECIALFACES)

#define ST_NUMEXTRAFACES        2

#define ST_NUMFACES \
          (ST_FACESTRIDE*ST_NUMPAINFACES+ST_NUMEXTRAFACES)

#define ST_TURNOFFSET           (ST_NUMSTRAIGHTFACES)
#define ST_OUCHOFFSET           (ST_TURNOFFSET + ST_NUMTURNFACES)
#define ST_EVILGRINOFFSET       (ST_OUCHOFFSET + 1)
#define ST_RAMPAGEOFFSET        (ST_EVILGRINOFFSET + 1)
#define ST_GODFACE              (ST_NUMPAINFACES*ST_FACESTRIDE)
#define ST_DEADFACE             (ST_GODFACE+1)

#define ST_FACESX               143
#define ST_FACESY               0

#define ST_EVILGRINCOUNT        (2*TICRATE)
#define ST_STRAIGHTFACECOUNT    (TICRATE/2)
#define ST_TURNCOUNT            (1*TICRATE)
#define ST_OUCHCOUNT            (1*TICRATE)
#define ST_RAMPAGEDELAY         (2*TICRATE)

#define ST_MUCHPAIN             20


// Location and size of statistics,
//  justified according to widget type.
// Problem is, within which space? STbar? Screen?
// Note: this could be read in by a lump.
//       Problem is, is the stuff rendered
//       into a buffer,
//       or into the frame buffer?

// AMMO number pos.
#define ST_AMMOWIDTH            3
#define ST_AMMOX                44
#define ST_AMMOY                3

// HEALTH number pos.
#define ST_HEALTHWIDTH          3
#define ST_HEALTHX              90
#define ST_HEALTHY              3

// Weapon pos.
#define ST_ARMSX                111
#define ST_ARMSY                4
#define ST_ARMSBGX              104
#define ST_ARMSBGY              0
#define ST_ARMSXSPACE           12
#define ST_ARMSYSPACE           10

// Frags pos.
#define ST_FRAGSX               138
#define ST_FRAGSY               3
#define ST_FRAGSWIDTH           2

// ARMOR number pos.
#define ST_ARMORWIDTH           3
#define ST_ARMORX               221
#define ST_ARMORY               3

// Key icon positions.
#define ST_KEYSBOX_X   236
#define ST_KEYSBOX_Y   2
#define ST_KEYSBOX_W   13
#define ST_KEYSBOX_H   30
// They appear in a vertical column, so share x positions.
#define ST_KEY_WIDTH   6
#define ST_KEY_HEIGHT  5
#define ST_KEYX        239
#define ST_KEYDX       2
#define ST_KEYDY       2
static uint8_t  keybox_y[6] = { 3, 13, 23, 3, 13, 23 };
static uint8_t  keybox_dual_x[6] =
 { ST_KEYX-ST_KEYDX, ST_KEYX-ST_KEYDX, ST_KEYX-ST_KEYDX,
   ST_KEYX+ST_KEYDX, ST_KEYX+ST_KEYDX, ST_KEYX+ST_KEYDX };
static uint8_t  keybox_dual_y[6] =
 {  3-ST_KEYDY, 13-ST_KEYDY, 23-ST_KEYDY,
    3+ST_KEYDY, 13+ST_KEYDY, 23+ST_KEYDY };

// Ammunition counter, and max ammo, in two columns in status bar.
// Max ammo changes when backpack aquired.
#define ST_AMMOS_WIDTH          3
#define ST_MAXAMMOS_WIDTH       3
#define ST_AMMOSX               288
#define ST_MAXAMMOSX            314
static uint8_t  ammobox_y[4] = { 5, 11, 23, 17 };


//faB: unused stuff from the Doom alpha version ?
// pistol
//#define ST_WEAPON0X           110
//#define ST_WEAPON0Y           4
// shotgun
//#define ST_WEAPON1X           122
//#define ST_WEAPON1Y           4
// chain gun
//#define ST_WEAPON2X           134
//#define ST_WEAPON2Y           4
// missile launcher
//#define ST_WEAPON3X           110
//#define ST_WEAPON3Y           13
// plasma gun
//#define ST_WEAPON4X           122
//#define ST_WEAPON4Y           13
// bfg
//#define ST_WEAPON5X           134
//#define ST_WEAPON5Y           13

// WPNS title
//#define ST_WPNSX              109
//#define ST_WPNSY              23

 // DETH title
//#define ST_DETHX              109
//#define ST_DETHY              23

//Incoming messages window location
// #define ST_MSGTEXTX     (viewwindowx)
// #define ST_MSGTEXTY     (viewwindowy+viewheight-18)
//#define ST_MSGTEXTX             0
//#define ST_MSGTEXTY             0     //added:08-01-98:unused
// Dimensions given in characters.
#define ST_MSGWIDTH             52
// Or shall I say, in lines?
#define ST_MSGHEIGHT            1

#define ST_OUTTEXTX             0
#define ST_OUTTEXTY             6

// Width, in characters again.
#define ST_OUTWIDTH             52
 // Height, in lines.
#define ST_OUTHEIGHT            1

#if 0
// UNUSED
#define ST_MAPWIDTH     \
    (strlen(mapnames[(gameepisode-1)*9+(gamemap-1)]))

//added:24-01-98:unused ?
//#define ST_MAPTITLEX  (vid.width - ST_MAPWIDTH * ST_CHATFONTWIDTH)
#endif

#define ST_MAPTITLEY            0
#define ST_MAPHEIGHT            1


int stbar_height = ST_HEIGHT;
int stbar_y = BASEVIDHEIGHT - ST_HEIGHT;
int stbar_x = 0;
int stbar_scalex, stbar_scaley;
int stbar_fg = FG | V_TRANSLUCENTPATCH;


//added:02-02-98: set true if widgets coords need to be recalculated
// Set by functions that change the window or status bar size.
boolean     stbar_recalc;

// ST_Start() has just been called
boolean     st_force_refresh;

// facegraphics loaded
static byte  facegraphics_loaded = false;

// used for timing
static unsigned int     st_clock;

// used for making messages go away
static int              st_msgcounter=0;

// used when in chat
static st_chatstateenum_t       st_chatstate;

// whether left-side main status bar is active
boolean                 stbar_on;

// whether status bar chat is active
static boolean          st_chat;

// value of st_chat before message popped up
static boolean          st_oldchat;

// whether chat window has the cursor on
static boolean          st_cursor_on;

// !deathmatch
static boolean          st_notdeathmatch;

// main bar left
static patch_t*         sbar = NULL;

// 0-9, tall numbers, minus at [10], percent at [11].
static patch_t*         tallnum[12];

// 0-9, short, yellow (,different!) numbers
static patch_t*         shortnum[12];

// 3 key-cards, 3 skulls
static patch_t*         keys[NUMCARDS];

// face status patches
static patch_t*         faces[ST_NUMFACES];

// face background
static patch_t*         faceback;

 // main bar right
static patch_t*         armsbg;

// weapon ownership patches
static patch_t*         arms[6][2];

// ready-weapon widget
static st_number_t      w_ready;

 // in deathmatch only, summary of frags stats
static st_number_t      w_frags;

// health widget
static st_number_t      w_health;

// arms background
static st_binicon_t     w_armsbg;

// weapon ownership widgets
static st_multicon_t    w_arms[6];

// face status widget
static st_multicon_t    w_faces;

// keycard widgets
static st_multicon_t    w_keyboxes[6];

// armor widget
static st_number_t      w_armor;

// ammo widgets
static st_number_t      w_ammo[4];

// max ammo widgets
static st_number_t      w_maxammo[4];


// ------------------------------------------
// Status bar state.
// Doom only.
// Single player only (can handle only one status bar).
// Splitscreen must use status overlay, which does not have state.

// Status bar player, also used by Heretic status display
player_t*  st_plyr = NULL;

// number of frags so far in deathmatch
static int      st_fragscount;

// used to use appopriately pained face
static int      st_oldhealth = -1;

// Doom only, but has room for Heretic weapons.
// used for evil grin
static boolean  oldweaponsowned[NUMWEAPONS];

 // count until face changes
static int      st_facecount = 0;

// current face index, used by w_faces
static int      st_faceindex = 0;

// holds key-type for each key box on bar
byte     st_card;  // card state displayed  (Doom, Heretic)
static byte     st_num_keyboxes;
static int      keyboxes[6];  // 0..3 keycards skulls, 4..6 dual display

// a random number per tick
static int      st_randomnumber;


// ------------------------------------------
//        Doom status overlay variables
// ------------------------------------------

// Doom only.
// Icons for status bar.
static lumpnum_t  sbo_health, sbo_frags, sbo_armor;
static lumpnum_t  sbo_ammo[NUMWEAPONS];


// ------------------------------------------
//        Doom status bar
// ------------------------------------------

//
// STATUS BAR CODE
//


// Single player only, when stbar_on.
//  Global : st_plyr
static void ST_Refresh_Background( void )
{
    byte * colormap;

    // Draw background, with status bar flag settings
    V_SetupDraw( BG | (stbar_fg & ~V_SCREENMASK) );

    // software mode copies patch to BG buffer,
    // hardware modes directly draw the statusbar to the screen
    V_DrawScaledPatch(stbar_x, stbar_y, sbar);

    // draw the faceback for the statusbarplayer
    colormap = (st_plyr->skincolor) ?
             SKIN_TO_SKINMAP( st_plyr->skincolor )
           : & reg_colormaps[0]; // default green skin

    V_DrawMappedPatch (stbar_x+ST_FX, stbar_y, faceback, colormap);

    // copy the statusbar buffer to the screen
    if( rendermode == render_soft )
        V_CopyRect(0, vid.height-stbar_height, BG, vid.width, stbar_height, 0, vid.height-stbar_height, FG);
}


// Respond to keyboard input events,
//  intercept cheats.
boolean ST_Responder (event_t* ev)
{

  if (ev->type == ev_keyup)
  {
    // Filter automap on/off : activates the statusbar while automap is active
    if( (ev->data1 & 0xffff0000) == AM_MSGHEADER )
    {
        switch(ev->data1)
        {
          case AM_MSGENTERED:
            st_force_refresh = true;        // force refresh of status bar
            break;

          case AM_MSGEXITED:
            break;
        }
    }

  }
  return false;
}

// Global : st_plyr
static int ST_calcPainOffset(void)
{
    // [WDJ] FIXME : This thrashes when splitplayer, but it is correct.
    static int  st_pain;
    static int  oldhealth = -1;

    int  health = st_plyr->health > 100 ? 100 : st_plyr->health;
    if (health != oldhealth)
    {
        st_pain = ST_FACESTRIDE * (((100 - health) * ST_NUMPAINFACES) / 101);
        oldhealth = health;
    }
    return st_pain;
}


//
// This is a not-very-pretty routine which handles
//  the face states and their timing.
// the precedence of expressions is:
//  dead > evil grin > turned head > straight ahead
//
// Doom only.
// Single Player status only.
// Global : st_plyr
static void ST_updateFaceWidget(void)
{
    static int  fw_last_attackdown_cnt = -1;
    static int  fw_priority = 0;  // priority of face effects

    player_t *  plyr = st_plyr;
    int         i;
    angle_t     badguyangle;
    angle_t     diffang;
    boolean     doevilgrin;

    // Highest priority first
    if (fw_priority < 10)
    {
        if (!plyr->health)
        {
            // dead
            fw_priority = 9;
            st_faceindex = ST_DEADFACE;
            st_facecount = 1;
        }
    }

    if (fw_priority < 9)
    {
        if (plyr->bonuscount)
        {
            // picking up bonus
            doevilgrin = false;

            // Doom only.
            for (i=0;i<NUMWEAPONS;i++)
            {
                if (oldweaponsowned[i] != plyr->weaponowned[i])
                {
                    doevilgrin = true;
                    oldweaponsowned[i] = plyr->weaponowned[i];
                }
            }
            if (doevilgrin)
            {
                // evil grin if just picked up weapon
                fw_priority = 8;  // allow retrigger evil grin
                st_facecount = ST_EVILGRINCOUNT;
                st_faceindex = ST_calcPainOffset() + ST_EVILGRINOFFSET;
            }
        }

    }

    if (fw_priority < 8)
    {
        if (plyr->damagecount
            && plyr->attacker
            && plyr->attacker != plyr->mo)
        {
            // being attacked
            fw_priority = 7;  // allow attack test retrigger

            // [WDJ] Ouch-face when damage>20, fix from DoomWiki, same as prboom
//            if (plyr->health - st_oldhealth > ST_MUCHPAIN) // orig bug
            if (st_oldhealth - plyr->health > ST_MUCHPAIN)
            {
                st_facecount = ST_TURNCOUNT;
                st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
                fw_priority = 8; // [WDJ] Necessary to keep ouchface visible
            }
            else
            {
                badguyangle = R_PointToAngle2(plyr->mo->x,
                                              plyr->mo->y,
                                              plyr->attacker->x,
                                              plyr->attacker->y);

                if (badguyangle > plyr->mo->angle)
                {
                    // whether right or left
                    diffang = badguyangle - plyr->mo->angle;
                    i = diffang > ANG180;
                }
                else
                {
                    // whether left or right
                    diffang = plyr->mo->angle - badguyangle;
                    i = diffang <= ANG180;
                } // confusing, aint it?


                st_facecount = ST_TURNCOUNT;
                st_faceindex = ST_calcPainOffset();

                if (diffang < ANG45)
                {
                    // head-on
                    st_faceindex += ST_RAMPAGEOFFSET;
                }
                else if (i)
                {
                    // turn face right
                    st_faceindex += ST_TURNOFFSET;
                }
                else
                {
                    // turn face left
                    st_faceindex += ST_TURNOFFSET+1;
                }
            }
        }
    }

    if (fw_priority < 7)
    {
        // getting hurt because of your own damn stupidity
        if (plyr->damagecount)
        {
            // [WDJ] Ouch-face when damage>20, fix from DoomWiki, same as prboom
//            if (plyr->health - st_oldhealth > ST_MUCHPAIN)
            if (st_oldhealth - plyr->health > ST_MUCHPAIN)
            {
                fw_priority = 7;  // no pain retrigger
                st_facecount = ST_TURNCOUNT;
                st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
            }
            else
            {
                fw_priority = 6;  // allow pain test retrigger
                st_facecount = ST_TURNCOUNT;
                st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
            }
        }
    }

    if (fw_priority < 6)
    {
        // rapid firing
        if (plyr->attackdown)
        {
            if (fw_last_attackdown_cnt < 0)
                fw_last_attackdown_cnt = ST_RAMPAGEDELAY;
            else if (--fw_last_attackdown_cnt == 0)
            {
                fw_last_attackdown_cnt = 1;
                fw_priority = 5;  // continual retrigger, no timer
                st_facecount = 1;
                st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
            }
        }
        else
            fw_last_attackdown_cnt = -1;
    }

    if (fw_priority < 5)
    {
        // invulnerability
        if ((plyr->cheats & CF_GODMODE)
            || plyr->powers[pw_invulnerability])
        {
            fw_priority = 4;  // continual retrigger, no timer
            st_facecount = 1;
            st_faceindex = ST_GODFACE;
        }
    }

    // look left or look right if the facecount has timed out
    if (st_facecount == 0)
    {
        fw_priority = 0;  // clear
        st_faceindex = ST_calcPainOffset() + (st_randomnumber % 3);
        st_facecount = ST_STRAIGHTFACECOUNT;
    }

    st_facecount--;
}

boolean ST_SameTeam(player_t *a,player_t *b)
{
    switch( cv_teamplay.EV ) {
       case 0 : return false;
       case 1 : return (a->skincolor == b->skincolor);
       case 2 : return (a->skin == b->skin);
    }
    return false;
}

// count the frags of the playernum player
//Fab: made as a tiny routine so ST_overlayDrawer() can use it
//Boris: rename ST_countFrags in to ST_PlayerFrags for use anytime
//       when we need the frags
int ST_PlayerFrags (int playernum)
{
    player_t * player = &players[playernum];
    int    i,frags;

    frags = player->addfrags;
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if( ((cv_teamplay.EV == 0) && i != playernum)
            || (cv_teamplay.EV && !ST_SameTeam(&players[i], player)) )
            frags += player->frags[i];
        else
            frags -= player->frags[i];
    }

    return frags;
}


// Doom only.
// Single player status bar only.
//  Global : st_plyr
// Called by: ST_Ticker
static void ST_updateWidgets(void)
{
    static int  largeammo = NON_NUMBER; // means "n/a"

    int         i;
    player_t *  plyr = st_plyr;

#ifdef PARANOIA
    if(!plyr)  return;  // not likely, but have soft fail
#endif
    // must redirect the pointer if the ready weapon has changed.
    //  if (w_ready.data != plyr->readyweapon)
    //  {
    if (plyr->weaponinfo[plyr->readyweapon].ammo == am_noammo)
        w_ready.num = &largeammo;
    else
        w_ready.num = &plyr->ammo[plyr->weaponinfo[plyr->readyweapon].ammo];
    //{
    // static int tic=0;
    // static int dir=-1;
    // if (!(tic&15))
    //   plyr->ammo[weaponinfo[plyr->readyweapon].ammo]+=dir;
    // if (plyr->ammo[weaponinfo[plyr->readyweapon].ammo] == -100)
    //   dir = 1;
    // tic++;
    // }
//    w_ready.data = plyr->readyweapon;

    // if (*w_ready.on)
    //  STlib_updateNum(&w_ready, true);
    // refresh weapon change
    //  }

    // update keycard multiple widgets
    if( plyr->cards != st_card )
    {
        // fraggle script can take a key, so keyboxes must not be sticky.
        if( ~plyr->cards & st_card )
        {
             st_force_refresh = true;  // a card was taken
             st_card = 0;
        }
        if( (plyr->cards & 0x07) && (plyr->cards & 0x38) )
        {
            // Have both keycards and skulls.
            if(((st_card & 0x07) == 0) || ((st_card & 0x38) == 0))
            {
                // Enable display of both skulls and keycards.
                st_num_keyboxes = 6;
                // Shift keybox[ 0..6 ] positions, into two vertical columns
                // Skulls will now be in [3..6]
                for( i=0; i<6; i++ )
                {
                    keyboxes[i] = -1;  // clear previous recorded skulls
                    w_keyboxes[i].x = stbar_x + keybox_dual_x[i],
                    w_keyboxes[i].y = stbar_y + keybox_dual_y[i],
                    w_keyboxes[i].command = STLIB_REFRESH;   // to clear old card positions
                }
            }
        }
        st_card = plyr->cards;

        for (i=0;i<3;i++)
        {
            // keycards
            keyboxes[i] = ((st_card >> i) & 0x01) ? i : -1;

            // skull keys
            if ((st_card >> i) & 0x08)
            {
                keyboxes[i+3] = i+3;  // dual display
                if( st_num_keyboxes == 3 )
                    keyboxes[i] = i+3;  // only skull display
            }
            else
            {
                keyboxes[i+3] = -1;  // skull off in dual display
            }
        }
    }

    // refresh everything if this is him coming back to life
    ST_updateFaceWidget();
    st_oldhealth = plyr->health;

    // used by the w_armsbg widget
    st_notdeathmatch = !cv_deathmatch.EV;

    st_fragscount = ST_PlayerFrags(statusbarplayer);

    // get rid of chat window if up because of message
    if (!--st_msgcounter)
        st_chat = st_oldchat;
}

static boolean  st_stopped = true;

//  Global : st_plyr
void ST_Ticker (void)
{
    if( st_stopped )
        return;

    if( EN_heretic )
    {
        SB_Heretic_Ticker();
        return;
    }

    // Doom only.
    st_clock++;
    st_randomnumber = M_Random();

    // Update immediately upon display changes.
    if((cv_viewsize.value<11) || automapactive )
        ST_updateWidgets();
}

// These are used by Heretic too.
int st_palette = 0;
byte pickupflash_table[ 4 ] = { 6, 5, 4, 3 }; // Vanilla=[3]=3

// Single and SplitPlayer, Software and Hardware Render.
// Called by: R_SetupFrame, from R_RenderPlayerView, HWR_RenderPlayerView
void ST_doPaletteStuff( player_t * plyr )
{
    int  palette;
    int  red_cnt;

    red_cnt = plyr->damagecount;

    if (plyr->powers[pw_strength])
    {
        // slowly fade the berzerk out
        int bzc = 12 - (plyr->powers[pw_strength]>>6);

        if (bzc > red_cnt)
            red_cnt = bzc;
    }

    if (red_cnt)
    {
        palette = STARTREDPALS + ((red_cnt+7)>>3);

        if (palette >= (STARTREDPALS+NUMREDPALS))
            palette = STARTREDPALS+NUMREDPALS-1;
    }
    else
    if (plyr->bonuscount && (cv_pickupflash.EV>=2))
    {
        // Pickup object palette flash.
        palette = STARTBONUSPALS
           + ((plyr->bonuscount+7)>>(pickupflash_table[cv_pickupflash.EV]));

        if (palette >= (STARTBONUSPALS+NUMBONUSPALS))
            palette = STARTBONUSPALS+NUMBONUSPALS-1;
    }
    else
    if ( plyr->powers[pw_ironfeet] > BLINKTHRESHOLD
         || plyr->powers[pw_ironfeet]&0x08)  // blink rate
        palette = RADIATIONPAL;
    else
        palette = 0;


    //added:28-02-98:quick hack underwater palette
    /*if (plyr->mo &&
        (plyr->mo->z + (((unsigned int)cv_viewheight.EV)<<FRACBITS) < plyr->mo->waterz) )
        palette = RADIATIONPAL;*/

    if (palette != st_palette)
    {
        st_palette = palette;

#ifdef HWRENDER
        if( EN_HWR_flashpalette )  // some hardware draw can flash palette
        {
            // Imitate the palette flash
            //debug_Printf("palette: %d\n", palette);
            HWR_SetFlashPalette( palette );
        }
        else
#endif
        {
            // Splitscreen cannot use palette effects when 8bit palette draw,
            // but other draw modes can.
            if( ((cv_splitscreen.EV == 0) || (vid.drawmode != DRAW8PAL))
                || !palette )
                V_SetPalette (palette);
        }
    }
}

// Set status palette 0 for camera.
void ST_Palette0( void )
{
    if (st_palette)
    {
#ifdef HWRENDER
        if ( EN_HWR_flashpalette )
        {
            // Imitate the palette flash
            HWR_SetFlashPalette( 0 );	    
        }
        else
#endif
        {
            V_SetPalette(0);
        }

        // Record it as the status palette.
        st_palette = 0;
    }
}


// Single player only, when stbar_on.
// Called by: ST_Drawer, when stbar_on.
// STlib refresh enable is now setup by caller.
// Only called when stbar_on == true, so more tests are pointless.
static void ST_Draw_Widgets( void )
{
    int  i;
    player_t * plyr;

    // Draw stbar_fg, screen0 status bar
    V_SetupDraw( stbar_fg );  // for all STlib

    if( cv_pickupflash.EV == 1 )
    {
        plyr = st_plyr;
        // Pickup flash on the status bar.
        if( plyr->ammo_pickup )
        {
            w_ready.command = STLIB_FLASH;
        }
        if( plyr->armor_pickup )
        {
            w_armor.command = STLIB_FLASH;
        }
        if( plyr->health_pickup )
        {
            w_health.command = STLIB_FLASH;
        }

        if( plyr->key_pickup )
        {
            // Flash entire box.
            // Do not know which one was picked up.
            V_DrawScaledFill(stbar_x + ST_KEYSBOX_X, stbar_y + ST_KEYSBOX_Y,
                       ST_KEYSBOX_W, ST_KEYSBOX_H, FLASH_COLOR);
            // Prevent the key icons from performing background refresh.
            for (i=0;i<st_num_keyboxes;i++)
                w_keyboxes[i].command = STLIB_FLASH;
        }
        else if( w_keyboxes[0].command == STLIB_FLASH_CLEAR
                 && ( rendermode == render_soft ) )
        {
            // Restore the background
            V_CopyRect(stbar_x + ST_KEYSBOX_X, stbar_y + ST_KEYSBOX_Y, BG,
                       ST_KEYSBOX_W, ST_KEYSBOX_H,
                       stbar_x + ST_KEYSBOX_X, stbar_y + ST_KEYSBOX_Y, stbar_fg);
            // Refresh the key icons.
            for (i=0;i<st_num_keyboxes;i++)
                w_keyboxes[i].command = STLIB_REFRESH;
        }
    }

    STlib_updateNum(&w_ready);  // current weapon ammo

    for (i=0;i<4;i++)
    {
        STlib_updateNum(&w_ammo[i]);
        STlib_updateNum(&w_maxammo[i]);
    }

    STlib_updatePercent(&w_health);
    STlib_updatePercent(&w_armor);

    STlib_updateBinIcon(&w_armsbg);

    if( cv_deathmatch.EV )
    {
        // frags on
        STlib_updateNum(&w_frags);
    }
    else   
    {
        // arms on
        for (i=0;i<6;i++)
            STlib_updateMultIcon(&w_arms[i]);
    }

    STlib_updateMultIcon(&w_faces);

    for (i=0;i<st_num_keyboxes;i++)
        STlib_updateMultIcon(&w_keyboxes[i]);
}


void ST_Invalidate(void)
{
    st_force_refresh = true;
    st_card = 0;
}

static void ST_overlayDrawer ( byte status_position, player_t * plyr );

// Doom and Heretic.
// For player, and both splitscreen players.
// Called by D_Display
void ST_Drawer ( boolean refresh )
{
    // Respond to these changes immediately, so cannot be in any setup.
    stbar_on = (cv_viewsize.value<11) || automapactive;

    if( EN_heretic )
    {
        SB_Heretic_Drawer( refresh );
        return;
    }

    // Doom Only.
    //added:30-01-98:force a set of the palette by doPaletteStuff()
    if (vid.recalc)
        st_palette = -1;

    // Player status palette interactions moved to R_SetupFrame
    // so that Splitplayer can be handled.

    // Splitplayer restricted to overlay or status bar off.
    if( stbar_on )
    {
        // Single player only (st_plyr), keeping state.
        if (st_force_refresh || refresh || stbar_recalc )
        {
            // after ST_Start(), screen refresh needed, or vid mode change
            if (stbar_recalc)  //recalc widget coords after vid mode change
            {
                ST_Create_Widgets ();
                stbar_recalc = false;
            }
            st_force_refresh = false;
            st_card = 0;

            // This is not executed as frequently as drawing, so it is more
            // complicated, in order to keep ST_Draw_Widgets simpler.
    
            // Draw status bar background to BG buffer
            ST_Refresh_Background();   // st_plyr

            stlib_enable_erase = (rendermode == render_soft);
            stlib_force_refresh = true;  // stlib refreshes from BG buffer.
        }
        else
        {
            // Otherwise, update as little as possible
            stlib_force_refresh = false;
        }
        // Update all widgets using stlib.
        ST_Draw_Widgets();
    }
    else if( st_overlay_on )
    {
        // Overlay status over screen.
        // Any minimal state kept, must be per splitscreen (see hardware).
        // Does not use stlib.
        if( cv_splitscreen.EV )
        {
            if((vid.drawmode != DRAW8PAL) && st_palette != 0 )
                ST_Palette0();

            // player 1 is upper
            ST_overlayDrawer ( 1, displayplayer_ptr );
            if( displayplayer2_ptr )
            {
                // player 2 is lower
                ST_overlayDrawer( 0, displayplayer2_ptr );
            }
        }
        else if( !playerdeadview )
        {
            ST_overlayDrawer( 0, displayplayer_ptr );
        }
    }
}


byte st_patches_loaded = 0;
load_patch_t  st_patches[13] =
{
  { &armsbg, "STARMS" }, // arms background
  { &sbar, "STBAR" },    // status bar background bits
  { NULL, NULL }
};


// Called by ST_Init, SCR_SetMode
void ST_Load_Graphics(void)
{
    int         i;
    // [Stylinski] Compiler complains of possible buffer overrun, requires [10].
    char        namebuf[12];
    // [WDJ] all ST graphics are loaded endian fixed
    // [WDJ] Lock the status bar graphics against other texture users.

    if( EN_heretic )
    {
        SB_Heretic_Load_Graphics();
        return;
    }
   
    st_patches_loaded = 1;
    load_patch_list( st_patches );

    // Load the numbers, tall and short
    for (i=0;i<10;i++)
    {
        sprintf(namebuf, "STTNUM%d", i);
        tallnum[i] = W_CachePatchName(namebuf, PU_LOCK_SB);

        sprintf(namebuf, "STYSNUM%d", i);
        shortnum[i] = W_CachePatchName(namebuf, PU_LOCK_SB);
    }
    tallnum[10] = W_CachePatchName("STTMINUS", PU_LOCK_SB);
    tallnum[11] = W_CachePatchName("STTPRCNT", PU_LOCK_SB);
    shortnum[10] = NULL; // has no minus
    shortnum[11] = NULL; // has no percent


    // key cards
    // FreeDoom and DoomII have STKEYS 0..5.
    for (i=0;i<NUMCARDS;i++)
    {
        sprintf(namebuf, "STKEYS%d", i);
        keys[i] = W_CachePatchName(namebuf, PU_LOCK_SB);
    }

    // arms ownership widgets
    for (i=0;i<6;i++)
    {
        sprintf(namebuf, "STGNUM%d", i+2);

        // gray #
        arms[i][0] = W_CachePatchName(namebuf, PU_LOCK_SB);

        // yellow #
        arms[i][1] = shortnum[i+2];  // shared patch
    }

    // the original Doom uses 'STF' as base name for all face graphics
    ST_Load_FaceGraphics ("STF");
}


// made separate so that skins code can reload custom face graphics
void ST_Load_FaceGraphics (const char *facestr)
{
    lumpnum_t ln;
    int   i,j;
    int   facenum;
    char  namelump[9];
    char* namebuf;
    // [WDJ] all ST graphics are loaded endian fixed

    //hack: make sure base face name is no more than 3 chars
    // bug: core dump fixed 19990220 by Kin
    // [WDJ] Cannot modify facestr.
    strncpy (namelump, facestr, 3);  // copy base name
    namelump[3] = '\0';
    // namebuf points after base face name, for appending to base name
    namebuf = namelump;
    while (*namebuf>' ') namebuf++;

    // face states
    facenum = 0;
    for (i=0;i<ST_NUMPAINFACES;i++)
    {
        for (j=0;j<ST_NUMSTRAIGHTFACES;j++)
        {
            sprintf(namebuf, "ST%d%d", i, j);
            faces[facenum++] = W_CachePatchName(namelump, PU_LOCK_SB);
        }
        sprintf(namebuf, "TR%d0", i);        // turn right
        faces[facenum++] = W_CachePatchName(namelump, PU_LOCK_SB);
        sprintf(namebuf, "TL%d0", i);        // turn left
        faces[facenum++] = W_CachePatchName(namelump, PU_LOCK_SB);
        sprintf(namebuf, "OUCH%d", i);       // ouch!
        faces[facenum++] = W_CachePatchName(namelump, PU_LOCK_SB);
        sprintf(namebuf, "EVL%d", i);        // evil grin ;)
        faces[facenum++] = W_CachePatchName(namelump, PU_LOCK_SB);
        sprintf(namebuf, "KILL%d", i);       // pissed off
        faces[facenum++] = W_CachePatchName(namelump, PU_LOCK_SB);
    }
    strcpy (namebuf, "GOD0");
    faces[facenum++] = W_CachePatchName(namelump, PU_LOCK_SB);
    strcpy (namebuf, "DEAD0");
    faces[facenum++] = W_CachePatchName(namelump, PU_LOCK_SB);

    // face backgrounds for different player colors
    //added:08-02-98: uses only STFB0, which is remapped to the right
    //                colors using the player translation tables, so if
    //                you add new player colors, it is automatically
    //                used for the statusbar.
    strcpy (namebuf, "B0");
    ln = W_CheckNumForName(namelump);
    if( VALID_LUMP( ln ) )
        faceback = W_CachePatchNum( ln, PU_LOCK_SB );
    else
        faceback = W_CachePatchName("STFB0", PU_LOCK_SB);

    ST_Invalidate();
    facegraphics_loaded = true;
}


void ST_Release_Graphics(void)
{
    int i;

    if( EN_heretic )
    {
        SB_Heretic_Release_Graphics();
        return;
    }
   
    //faB: GlidePatch_t are always purgeable
    if( st_patches_loaded )
    {
        st_patches_loaded = 0;
        release_patch_list( st_patches );

        // unload the numbers, tall and short
        release_patch_array( tallnum, 12 );
        release_patch_array( shortnum, 10 );

        // unload gray #'s
        for (i=0;i<6;i++)
            W_release_patch( arms[i][0] );

        // unload the key cards
        release_patch_array( keys, NUMCARDS );
    }

    ST_Release_FaceGraphics ();
}

// made separate so that skins code can reload custom face graphics
// Called by SetPlayerSkin, ST_Release_Graphics
void ST_Release_FaceGraphics (void)
{
    //faB: MipPatch_t are always purgeable
    if( facegraphics_loaded )
    {
        facegraphics_loaded = false;
        release_patch_array( faces, ST_NUMFACES );
        
        // face background
        W_release_patch( faceback );
    }
}



// Doom only.
static void ST_init_stbar(void)
{

    int         i;

    st_force_refresh = true;

    //added:16-01-98:'link' the statusbar display to a player, which could be
    //               another player than consoleplayer, for example, when you
    //               change the view in a multiplayer demo with F12.
    if (singledemo)
        statusbarplayer = displayplayer;
    else
        statusbarplayer = consoleplayer;

    // Single player status state init.
    st_plyr = &players[statusbarplayer];

    st_clock = 0;
    st_chatstate = StartChatState;

    stbar_on = true;  // ST_Drawer clears it for Splitscreen
    st_oldchat = st_chat = false;
    st_cursor_on = false;

    st_faceindex = 0;
    st_palette = -1;

    st_oldhealth = -1;

    // Doom only.
    for (i=0;i<NUMWEAPONS;i++)
        oldweaponsowned[i] = st_plyr->weaponowned[i];

    st_card = 0;  // no keys
    st_num_keyboxes = 3;
    for (i=0;i<6;i++)
        keyboxes[i] = -1;

}


void ST_CalcPos(void)
{
    if( cv_scalestatusbar.EV || cv_viewsize.value>=11 )
    {
        // large scaled status bar
        stbar_fg = FG | V_SCALEPATCH | V_SCALESTART | V_TRANSLUCENTPATCH;
        stbar_scalex = vid.dupx;
        stbar_scaley = vid.dupy;

#ifdef HWRENDER
        if( rendermode != render_soft )
        {
            stbar_x = 0;
            stbar_y = BASEVIDHEIGHT - stbar_height/vid.fdupy;
        }
        else
#endif

        {
            stbar_x = ((vid.width - ST_WIDTH*vid.dupx)>>1)/vid.dupx;
            stbar_y = (vid.height - stbar_height)/vid.dupy;
        }
    }
    else
    {
        // smaller unscaled status bar in center
        stbar_fg = FG | V_NOSCALE | V_TRANSLUCENTPATCH;
        stbar_scalex = stbar_scaley = 1;
        stbar_x = (vid.width - ST_WIDTH)>>1;  // center
        stbar_y = vid.height - stbar_height;
    }
}

// Single player init.
// Also can be called at init of Splitscreen game.
//added:30-01-98: NOTE: this is called at any level start, view change,
//                      and after vid mode change.
static
void ST_Create_Widgets(void)
{
    int i;

    ST_CalcPos();

    // ready weapon ammo
    STlib_initNum(&w_ready,
                  stbar_x + ST_AMMOX,
                  stbar_y + ST_AMMOY,
                  tallnum,
                  &st_plyr->ammo[st_plyr->weaponinfo[st_plyr->readyweapon].ammo],
                  ST_AMMOWIDTH );

    // the last weapon type
//    w_ready.data = st_plyr->readyweapon;

    // health percentage
    STlib_initNum(&w_health,
                  stbar_x + ST_HEALTHX,
                  stbar_y + ST_HEALTHY,
                  tallnum,
                  &st_plyr->health,
                  3 );

    // arms background
    STlib_initBinIcon(&w_armsbg,
                      stbar_x + ST_ARMSBGX,
                      stbar_y + ST_ARMSBGY,
                      armsbg,
                      &st_notdeathmatch );

    // weapons owned, draw enabled by !cv_deathmatch
    for(i=0;i<6;i++)
    {
        STlib_initMultIcon(&w_arms[i],
                      stbar_x + ST_ARMSX + (i%3)*ST_ARMSXSPACE,
                      stbar_y + ST_ARMSY + (i/3)*ST_ARMSYSPACE,
                      arms[i], (int *) &st_plyr->weaponowned[i+1] );
    }

    // frags sum, draw enabled by cv_deathmatch
    STlib_initNum(&w_frags,
                  stbar_x + ST_FRAGSX,
                  stbar_y + ST_FRAGSY,
                  tallnum,
                  &st_fragscount,
                  ST_FRAGSWIDTH);

    // faces
    STlib_initMultIcon(&w_faces,
                       stbar_x + ST_FACESX,
                       stbar_y + ST_FACESY,
                       faces,
                       &st_faceindex );

    // armor percentage - should be colored later
    STlib_initNum(&w_armor,
                  stbar_x + ST_ARMORX,
                  stbar_y + ST_ARMORY,
                  tallnum,
                  &st_plyr->armorpoints,
                  3 );

    // keyboxes 0-6, in vertical column
    for( i=0; i<6; i++ )
    {
        STlib_initMultIcon(&w_keyboxes[i],
                       stbar_x + ST_KEYX,
                       stbar_y + keybox_y[i],
                       keys,  // patches
                       &keyboxes[i] );
    }

    for( i=0; i<4; i++ )
    {
        // In vertical column.
        // ammo count (all four kinds)
        STlib_initNum(&w_ammo[i],
                  stbar_x + ST_AMMOSX,
                  stbar_y + ammobox_y[i],
                  shortnum,
                  &st_plyr->ammo[i],
                  ST_AMMOS_WIDTH);
        // max ammo count (all four kinds)
        STlib_initNum(&w_maxammo[i],
                  stbar_x + ST_MAXAMMOSX,
                  stbar_y + ammobox_y[i],
                  shortnum,
                  &st_plyr->maxammo[i],
                  ST_MAXAMMOS_WIDTH);
    }
}

static void ST_Stop (void)
{
    if (st_stopped)
        return;

//    V_SetPalette (0);
    ST_Palette0();

    st_stopped = true;
}

// Doom or Heretic.
// Single and SplitPlayer.
// Called by G_DoLoadLevel, P_SpawnPlayer, P_AddWadFile
// Called by ST_Change_DemoView
void ST_Start (void)
{
    // Doom and Heretic common.
    st_plyr = &players[statusbarplayer];

    if( EN_heretic )
    {
        st_stopped = false;
        return;
    }

    // Doom only.
    if (!st_stopped)
        ST_Stop();

    // Init as if Single player.
    // When AutoMap displayed, shows Status bar for player1.
    ST_init_stbar();
    ST_Create_Widgets();
    st_stopped = false;
    stbar_recalc = false;  //added:02-02-98: widgets coords have been setup
                           // see ST_drawer()
}

//
//  Initializes the status bar,
//  sets the defaults border patch for the window borders.
//

//faB: used by Glide mode, holds lumpnum of flat used to fill space around the viewwindow
lumpnum_t  st_borderflat_num;  // extern in r_draw.h

void ST_Init (void)
{
    int     i;

    if(dedicated)
        return;
    
    //added:26-01-98:screens[4] is allocated at videomode setup, and
    //               set at V_Init(), the first time being at SCR_Recalc()

    // choose and cache the default border patch
    switch(gamemode) {
        case doom2_commercial :
            // DOOM II border patch, original was GRNROCK
            st_borderflat_num = W_GetNumForName ("GRNROCK");
            break;
        case heretic :
            if( ! VALID_LUMP( W_CheckNumForName("e2m1") ) )
            {
                // GDESC_heretic_shareware
                st_borderflat_num = W_GetNumForName ("FLOOR04");
            }
            else
                st_borderflat_num = W_GetNumForName ("FLAT513");
            break;
        case hexen :
            st_borderflat_num = W_GetNumForName ("F_022");
            break;
        default :
            // DOOM border patch.
            st_borderflat_num = W_GetNumForName ("FLOOR7_2");
    }
    // [WDJ] Lock against other users of same patch releasing it!.
    scr_borderflat = W_CacheLumpNum (st_borderflat_num, PU_LOCK_SB);

    ST_Load_Graphics();  // Doom and Heretic

    if( EN_heretic )
        return;

    // Doom only
    //
    // cache the status bar overlay icons  (fullscreen mode)
    //
    sbo_health = W_GetNumForName ("SBOHEALT");
    sbo_frags  = W_GetNumForName ("SBOFRAGS");
    sbo_armor  = W_GetNumForName ("SBOARMOR");

    // With Heretic, NUMWEAPONS = 18.
    // Doom weapons are 0..8, chainsaw = 7.
    for (i=0;i<NUMWEAPONS;i++)
    {
        sbo_ammo[i] = (i>0 && i!=7 && i<=8)?
            W_GetNumForName (va("SBOAMMO%c",'0'+i))
            : 0;
    }
}

//added:16-01-98: change the status bar too, when pressing F12 while viewing
//                 a demo.
void ST_Change_DemoView (void)
{
    //the same routine is called at multiplayer deathmatch spawn
    // so it can be called multiple times
    ST_Start();
}


// =========================================================================
//                         STATUS BAR OVERLAY
// =========================================================================

consvar_t cv_stbaroverlay = {"overlay","kahmf",CV_SAVE,NULL};

boolean   st_overlay_on;  // status overlay for Doom and Heretic


void ST_Register_Commands (void)
{
    CV_RegisterVar (&cv_stbaroverlay);
}


//  Draw a number, scaled, over the view
//  Always draw the number completely since it's overlay
//
//   x, y: scaled position, right border!
static
void ST_drawOverlayNum (int x, int y,
                        int       num,
                        patch_t** numpat,
                        patch_t*  percent,
                        byte      pickup_flash )
{
    // Hardware or software draw.
    patch_t * pf = V_patch( numpat[0] );
    int  hf = pf->height;
    int  wf = pf->width;
    int  wfv = wf * vid.dupx;
    boolean   neg;

    V_SetupDraw( FG | V_NOSCALE | V_SCALEPATCH | V_TRANSLUCENTPATCH );
   
    if( pickup_flash && (cv_pickupflash.EV == 1))
    {
        // Assume 3 digits  0..200
        V_DrawVidFill(x - (wfv*3), y, wfv*3, hf*vid.dupy, FLASH_COLOR);
    }

    // in the special case of 0, you draw 0
    if (num == 0)
    {
        V_DrawScaledPatch(x - wfv, y, numpat[ 0 ]);
        return;
    }

    neg = num < 0;

    if (neg)
        num = -num;

    // draw the number
    while (num)
    {
        x -= wfv;
        V_DrawScaledPatch(x, y, numpat[ num % 10 ]);
        num /= 10;
    }

    // draw a minus sign if necessary, minus is at [10] in the number font
    if (neg && numpat[10])
        V_DrawScaledPatch(x - (8*vid.dupx), y, numpat[10]);
}

//  y : status position normally
//  y0 : status base position as modified for splitscreen
static inline int SCY( int y, int y0 )
{ 
    //31/10/99: fixed by Hurdler so it _works_ also in hardware mode
    // do not scale to resolution for hardware accelerated
    // because these modes always scale by default
    y = (int)( y * vid.fdupy );     // scale to resolution
    if( cv_splitscreen.EV ) {
        y >>= 1; // half sized screens
        y += y0; // base position of upper or lower screen
    }
    return y;
}


static inline int SCX( int x )
{
    return x * vid.fdupx;
}

static
void  ST_drawOverlayKeys( int x, int y, player_t * plyr )
{
    int  i, yh, xinc, yinc;
    byte cards = plyr->cards;

    xinc = (int)((ST_KEY_WIDTH + 1) * vid.fdupx);
    yinc = (int)((ST_KEY_HEIGHT + 1) * vid.fdupy);
    yh = y;  // upper row is same as lower row when no skull keys
    // if both skull and cards, then move cards up a row	  
    if( cards & 0x38 )
        yh -= yinc;

    if( plyr->key_pickup && (cv_pickupflash.EV == 1))
    {
        V_DrawVidFill(x - (xinc*3), yh, (xinc*3), y - yh + yinc, FLASH_COLOR);
    }
   
    for (i=0;i<3;i++)
    {
        x -= xinc;
        if( (cards >> i) & 0x08 ) // skull
        {
            V_DrawScaledPatch(x, y, keys[i+3]);  // skull graphic lower row
        }
        if( (cards >> i) & 0x01 ) // card
        {
            V_DrawScaledPatch(x, yh, keys[i]);  // keycard graphic upper row
        }
    }
}


//  Draw the status bar overlay, customisable : the user choose which
//  kind of information to overlay
//
//   status_position : 0=lower, 1=upper
static
void ST_overlayDrawer ( byte status_position, player_t * plyr )
{
    const char *  cmds;
    char   c;
    int    i;
    // [WDJ] 8/2012 fix opengl overlay position to use fdupy
    float  sf_dupy = (rendermode == render_soft)? vid.dupy : vid.fdupy ;
    int  y0 = status_position ? 0 : vid.height / 2;
    int  lowerbar_y = SCY(198,y0) - (int)( 16 * sf_dupy );

    // Draw screen0, scaled, abs position
    V_SetupDraw( FG | V_NOSCALE | V_SCALEPATCH );
    // x, y are already scaled.

    cmds = cv_stbaroverlay.string;

    while ((c=*cmds++))
    {
       if (c>='A' && c<='Z')
           c = c + 'a' - 'A';
       switch (c)
       {
         case 'h': // draw health
           ST_drawOverlayNum(SCX(50), lowerbar_y,
                             plyr->health,
                             tallnum, NULL, plyr->health_pickup);

           V_DrawScalePic_Num (SCX(52), lowerbar_y, sbo_health);
           break;

         case 'f': // draw frags
           st_fragscount = ST_PlayerFrags(plyr-players);

           if (cv_deathmatch.EV)
           {
               ST_drawOverlayNum(SCX(300), SCY(2,y0),
                                 st_fragscount,
                                 tallnum, NULL, 0);

               V_DrawScalePic_Num (SCX(302), SCY(2,y0), sbo_frags);
           }
           break;

         case 'a': // draw ammo
           i = sbo_ammo[plyr->readyweapon];
           if (i)
           {
               ST_drawOverlayNum(SCX(234), lowerbar_y,
                                 plyr->ammo[plyr->weaponinfo[plyr->readyweapon].ammo],
                                 tallnum, NULL, plyr->ammo_pickup);

               V_DrawScalePic_Num (SCX(236), lowerbar_y, i);
           }
           break;

         case 'k': // draw keys
           ST_drawOverlayKeys( SCX(318), lowerbar_y - (8 * sf_dupy), plyr );
           break;

         case 'm': // draw armor
           ST_drawOverlayNum(SCX(300), lowerbar_y,
                             plyr->armorpoints,
                             tallnum, NULL, plyr->armor_pickup);

           V_DrawScalePic_Num (SCX(302), lowerbar_y, sbo_armor);
           break;

         // added by Hurdler for single player only
         case 'e': // number of monster killed 
           if( (!cv_deathmatch.EV) && (!cv_splitscreen.EV) )
           {
               char buf[16];
               sprintf(buf, "%d/%d", plyr->killcount, totalkills);
               V_DrawString(SCX(318-V_StringWidth(buf)), SCY(1,y0), 0, buf);
           }
           break;

         case 's': // number of secrets found
           if( (!cv_deathmatch.EV) && (!cv_splitscreen.EV) )
           {
               char buf[16];
               sprintf(buf, "%d/%d", plyr->secretcount, totalsecret);
               V_DrawString(SCX(318-V_StringWidth(buf)), SCY(11,y0), 0, buf);
           }
           break;

           /* //TODO
         case 'r': // current frame rate
           {
               char buf[8];
               int framerate = 35;
               sprintf(buf, "%d FPS", framerate);
               V_DrawString(SCX(2), SCY(4,y0), 0, buf);
           }
           break;
           */
       }
    }
}
