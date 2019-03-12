// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: hu_stuff.c 1418 2019-01-29 08:01:04Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2010 by DooM Legacy Team.
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
// $Log: hu_stuff.c,v $
// Revision 1.18  2003/11/22 00:22:09  darkwolf95
// get rid of FS hud pics on level exit and new game, also added exl's fix
// for clearing hub variables on new game
//
// Revision 1.17  2003/07/14 12:39:12  darkwolf95
// Made rankings screen smaller for splitscreen.  Ditched the graphical title
// and limited max rankings to four.
//
// Revision 1.16  2002/07/23 15:07:10  mysterial
// Messages to second player appear on his half of the screen
//
// Revision 1.15  2001/12/15 18:41:35  hurdler
// small commit, mainly splitscreen fix
//
// Revision 1.14  2001/08/20 20:40:39  metzgermeister
//
// Revision 1.13  2001/07/16 22:35:40  bpereira
// - fixed crash of e3m8 in heretic
// - fixed crosshair not drawed bug
//
// Revision 1.12  2001/05/16 21:21:14  bpereira
// Revision 1.11  2001/04/01 17:35:06  bpereira
// Revision 1.10  2001/02/24 13:35:20  bpereira
//
// Revision 1.9  2001/02/19 17:40:34  hurdler
// Fix a bug with "chat on" in hw mode
//
// Revision 1.8  2001/01/25 22:15:42  bpereira
// added heretic support
//
// Revision 1.7  2000/11/04 16:23:43  bpereira
//
// Revision 1.6  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.5  2000/09/28 20:57:15  bpereira
// Revision 1.4  2000/08/31 14:30:55  bpereira
// Revision 1.3  2000/08/03 17:57:42  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      heads up displays, cleaned up (hasta la vista hu_lib)
//      because a lot of code was unnecessary now
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "hu_stuff.h"

#include "d_netcmd.h"
#include "d_clisrv.h"

#include "g_game.h"
#include "g_input.h"

#include "i_video.h"

// Data.
#include "dstrings.h"
#include "st_stuff.h"
  //added:05-02-98: ST_HEIGHT
#include "r_local.h"
#include "wi_stuff.h"
  // for drawrankings
#include "p_info.h"
#include "p_inter.h"
  // P_SetMessage

#include "keys.h"
#include "v_video.h"

#include "w_wad.h"
#include "z_zone.h"

#include "console.h"
#include "am_map.h"
#include "d_main.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif


// coords are scaled
#define HU_INPUTX       0
#define HU_INPUTY       0

//-------------------------------------------
//              heads up font
//-------------------------------------------
patch_t*                hu_font[HU_FONTSIZE];


static player_t*        plr;
boolean                 chat_on;

static boolean          headsup_active = false;

boolean                 hu_showscores;        // draw deathmatch rankings

static char             hu_tick;

//-------------------------------------------
//              misc vars
//-------------------------------------------

consvar_t*   chat_macros[10];

//added:16-02-98: crosshair 0=off, 1=cross, 2=angle, 3=point, see m_menu.c
patch_t*     crosshair[3];     //3 precached crosshair graphics

static byte  hu_fonts_loaded = 0;

#ifdef HWRENDER
// The settings of HWR_patchstore by SCR_SetMode seem to be adequate.
// This only would be needed if there were additional problems.
// It is more expensive.
//#define HU_HWR_PATCHSTORE_SAVE
#ifdef HU_HWR_PATCHSTORE_SAVE
static byte  hu_HWR_patchstore;  // the HWR_patchstore setting used by fonts.
#endif
#endif


// -------
// protos.
// -------
void   HU_Draw_DeathmatchRankings (void);
void   HU_Draw_Crosshair (void);
static void HU_Draw_Tip();



//======================================================================
//                          HEADS UP INIT
//======================================================================

void HU_Stop(void)
{
    headsup_active = false;
}

// 
// Reset Heads up when consoleplayer spawns
//
void HU_Start(void)
{
    if (headsup_active)
        HU_Stop();

    plr = consoleplayer_ptr;
    chat_on = false;

    headsup_active = true;
}

void HU_Load_Graphics( void )
{
    int         i, j;
    char        buffer[9];

    use_font1 = 0;
    // cache the heads-up font
    // Patches are endian fixed when loaded.
    j = (EN_heretic)? 1 : HU_FONTSTART;
    for (i=0; i<HU_FONTSIZE; i++)
    {
        if( EN_heretic_hexen )
            sprintf(buffer, "FONTA%.2d", ((j>59)? 59 : j));
        else
            sprintf(buffer, "STCFN%.3d", j);

        j++;
        if( ! VALID_LUMP( W_CheckNumForName( buffer ) ) )
        {
            // font not found
            hu_font[i] = NULL;
            use_font1 = 1;
            continue;
        }
        hu_font[i] = W_CachePatchName(buffer, PU_STATIC);
    }

    // cache the crosshairs, dont bother to know which one is being used,
    // just cache them 3 all, they're so small anyway.
    for(i=0; i<HU_CROSSHAIRS; i++)
    {
       sprintf(buffer, "CROSHAI%c", '1'+i);
       crosshair[i] = W_CachePatchName(buffer, PU_STATIC);
    }

    hu_fonts_loaded = 1;
#ifdef HU_HWR_PATCHSTORE_SAVE
    hu_HWR_patchstore = HWR_patchstore;
#endif
}

void HU_Release_Graphics( void )
{
#ifdef HU_HWR_PATCHSTORE_SAVE
    byte saved_HWR_patchstore = HWR_patchstore;
#endif

    if( hu_fonts_loaded )
    {
        hu_fonts_loaded = 0;
        
#ifdef HU_HWR_PATCHSTORE_SAVE
        // Must use setting from when fonts were saved.
	// This is necessary because releasing the font is delayed until the last second.
        HWR_patchstore = hu_HWR_patchstore;
#endif

        // Has protection against individual NULL ptr in array.
        release_patch_array( hu_font, HU_FONTSIZE );
        release_patch_array( crosshair, HU_CROSSHAIRS );
       
#ifdef HU_HWR_PATCHSTORE_SAVE
        HWR_patchstore = saved_HWR_patchstore;
#endif
    }
}


//======================================================================
//                            EXECUTION
//======================================================================

void TeamPlay_OnChange(void)
{
    int i;
    // Change the name of the teams

    if(cv_teamplay.EV == 1)
    {
        // color
        for(i=0; i<NUMSKINCOLORS; i++)
            set_team_name( i, Color_Names[i]);
    }
    else
    if(cv_teamplay.EV == 2)
    {
        // skins
        for(i=0; i<numskins; i++)
            set_team_name( i, skins[i]->name);
    }
}


// SAY: Broadcast to all players.
void Command_Say_f (void)
{
    char buf[255];
    int i,j;

    if((j=COM_Argc())<2)
    {
        CONS_Printf ("say <message> : send a message\n");
        return;
    }

    buf[0]=255;  // broadcast
    strcpy(&buf[1],COM_Argv(1));
    for(i=2; i<j; i++)
    {
        strcat(&buf[1]," ");
        strcat(&buf[1],COM_Argv(i));
    }
    Send_NetXCmd(XD_SAY, buf, strlen(buf+1)+2);
       // +2 because 1 for buf[0] and the other for null terminated string
}

// SAYTO: Send to a player.
void Command_Sayto_f (void)
{
    byte playernum;
    int i,j;
    char buf[255];

    if((j=COM_Argc())<3)
    {
        CONS_Printf ("sayto <playername|playernum> <message> : send a message to a player\n");
        return;
    }

    // Players 0..(MAXPLAYERS-1) are known as Player 1 to MAXPLAYERS to user.
    playernum = player_name_to_num(COM_Argv(1));
    if(playernum > MAXPLAYERS)
        return;  // not found

    buf[0] = playernum;    // 0..127
    strcpy(&buf[1],COM_Argv(2));
    for(i=3; i<j; i++)
    {
        strcat(&buf[1]," ");
        strcat(&buf[1],COM_Argv(i));
    }
    Send_NetXCmd(XD_SAY, buf, strlen(buf+1)+2);
}

// SAYTEAM: To all team members of this player.
void Command_Sayteam_f (void)
{
    char buf[255];
    int i,j;

    if((j=COM_Argc())<2)
    {
        CONS_Printf ("sayteam <message> : send a message to your team\n");
        return;
    }

    // Players 0..(MAXPLAYERS-1) are known as Player 1 to MAXPLAYERS to user.
    buf[0] = consoleplayer & 0x80;  // 128..254
    strcpy(&buf[1],COM_Argv(1));
    for(i=2; i<j; i++)
    {
        strcat(&buf[1]," ");
        strcat(&buf[1],COM_Argv(i));
    }
    Send_NetXCmd(XD_SAY, buf, strlen(buf+1)+2);
        // +2 because 1 for buf[0] and the other for null terminated string
}

// [WDJ] Previous Say/Sayto/Sayteam system was broken.
// New as of DoomLegacy 1.46:
//  to: 0..127 player
//      0x80 & team number, team= 0..126
//      255 broadcast

void Got_NetXCmd_Saycmd( xcmd_t * xc )
{
    // Command: ( byte: to_player_id, string0: message )
    // XCmd buffer has forced 0 term to protect against malicious message.
    const char * tostr = "";
    char * fromstr;
    byte * p = xc->curpos;
    byte to = *(p++);
    byte pn = (to & 0x7F); // to player num 0..126

    if( xc->playernum >= MAXPLAYERS )  goto done;  // cannot index player_names
    fromstr = player_names[xc->playernum];  // never NULL

    if( to==255 )
    {
        // Broadcast
        tostr = " All";
    }
    else
    {
        if( pn >= MAXPLAYERS )  goto done;  // cannot index tables
        if( to & 0x80 )
        {
            // To Team
            tostr = " Team";
        }
    }

    if(xc->playernum == consoleplayer
       || to==255 // broadcast
       || ( (to < MAXPLAYERS) && pn==consoleplayer )
       || ( (to & 0x80) // Team broadcast from pn
            && ST_SameTeam(consoleplayer_ptr,&players[pn])) )
    {
        GenPrintf( EMSG_playmsg, "\3%s%s: %s\n", fromstr, tostr, p);
    }

    if(  displayplayer2_ptr )
    {
        // Splitscreen
        if(xc->playernum == displayplayer2
           || to==255 // broadcast
           || ( (to < MAXPLAYERS) && pn==displayplayer2 )
           || ( (to & 0x80) // Team broadcast from pn
                && ST_SameTeam(displayplayer2_ptr,&players[pn])) )
        {
            GenPrintf( EMSG_playmsg2, "\3%s%s: %s\n", fromstr, tostr, p);
        }
    }

done:
    p += strlen((char*)p) + 1;  // incl term 0
    xc->curpos = p;
}




//
//
void HU_Ticker(void)
{
    player_t    *pl;

    if(dedicated)
        return;
    
    hu_tick++;
    hu_tick &= 7;        //currently only to blink chat input cursor

    // display message if necessary
    // (display the viewplayer's messages)
    pl = displayplayer_ptr;
    if ( pl->message )
    {
        // Player message blocking is handled by P_SetMessage.
        GenPrintf(EMSG_playmsg, "%s\n", pl->message);
        pl->message = NULL;
        pl->msglevel = 0;
    }

    // In splitscreen, display second player's messages
    if (cv_splitscreen.value && displayplayer2_ptr )
    {
        pl = displayplayer2_ptr;
        if ( pl->message )
        {
            // Player message blocking is handled by P_SetMessage.
            GenPrintf(EMSG_playmsg2, "%s\n", pl->message);
            pl->message = NULL;
            pl->msglevel = 0;
        }
    }
    
    //deathmatch rankings overlay if press key or while in death view
    if( cv_deathmatch.EV )
    {
        if (gamekeydown[gamecontrol[gc_scores][0]] ||
            gamekeydown[gamecontrol[gc_scores][1]] )
            hu_showscores = !chat_on;
        else
            hu_showscores = playerdeadview; //hack from P_DeathThink()
    }
    else
        hu_showscores = false;
}



// [smite] there's no reason to use a queue here, a normal buffer will do
static char     w_chat[HU_MAXMSGLEN+1]; // always NUL-terminated
static unsigned tail = 0; // first free cell, should contain NUL

// simplified stl::vector implementation
static boolean HU_Chat_push_back(char c)
{
  if (tail >= HU_MAXMSGLEN)
    return false;

  w_chat[tail++] = c;
  w_chat[tail] = '\0';
  return true;
}

static boolean HU_Chat_pop_back()
{
  if (tail == 0)
    return false;

  tail--;
  w_chat[tail] = '\0';
  return true;
}

static void HU_Chat_clear()
{
  tail = 0;
  w_chat[tail] = '\0';
}

static boolean HU_Chat_empty()
{
  return tail == 0;
}

static void HU_Chat_send()
{
  COM_BufInsertText(va("say %s", w_chat));
}



//
//  Returns true if key eaten
//
boolean HU_Responder (event_t *ev)
{
  if (ev->type != ev_keydown)
    return false;

  // only KeyDown events now...
  int key = ev->data1;

  if (!chat_on)
  {
      // enter chat mode
      if (key == gamecontrol[gc_talkkey][0] || key == gamecontrol[gc_talkkey][1])
      {
          chat_on = true;
          HU_Chat_clear();
          return true;
      }
  }
  else
  {
      int c = ev->data2; // ASCII character

      // send a macro
      if (altdown)
      {
          c = c - '0';
          if (c > 9 || c < 0)
            return false;

          // current message stays unchanged

          // send the macro message
          COM_BufInsertText(va("say %s", chat_macros[c]->string));

          // if there is no unfinished message, leave chat mode and notify that it was sent
          if (HU_Chat_empty())
            chat_on = false;
      }
      else
      {
          // chat input
          if (key == KEY_ESCAPE)
          {
              // close chat
              chat_on = false;
          }
          else if (key == KEY_ENTER)
          {
              // send the message
              if (tail > 1)
                HU_Chat_send(w_chat);

              HU_Chat_clear();
              chat_on = false;
          }
          else if (key == KEY_BACKSPACE)
          {
              // erase a char
              HU_Chat_pop_back();
          }
          else if (c >= ' ' && c <= '~')
          {
              // add a char
              if (!HU_Chat_push_back(c))
                P_SetMessage( plr, HUSTR_MSGU, 63);  // out of space
          }
          else
            return false; // let the event go
      }

      return true; // ate the key
  }

  return false; // let the event go
}


//======================================================================
//                         HEADS UP DRAWING
//======================================================================

//  Draw chat input
//
static void HU_Draw_Chat (void)
{
    // vid : from video setup
    int  i,x,y;
   
    V_SetupFont( cv_msg_fontsize.value, NULL, V_NOSCALE );

    i=0;
    x=HU_INPUTX;
    y=HU_INPUTY;
    while (w_chat[i])
    {
#if 1
        // Proportional Font  
        x += V_DrawCharacter( x, y, w_chat[i++] | 0x80 );  // white
#else
        // Fixed width Font  
        V_DrawCharacter( x, y, w_chat[i++] | 0x80 );  // white
        x += drawfont.xinc;
#endif
       
        if (x >= vid.width)
        {
            x = HU_INPUTX;
            y += drawfont.yinc;
        }
    }

    // Cursor blink
    if (hu_tick<4)
        V_DrawCharacter( x, y, '_' | 0x80 );  // white
}


extern consvar_t cv_chasecam;

//  Heads up displays drawer, call each frame
//
void HU_Drawer(void)
{
    // draw chat string plus cursor
    if (chat_on)
        HU_Draw_Chat ();

    // draw deathmatch rankings
    if (hu_showscores)
        HU_Draw_DeathmatchRankings ();

    // draw the crosshair, not when viewing demos nor with chasecam
    if (!automapactive && cv_crosshair.value && !demoplayback && !cv_chasecam.value)
        HU_Draw_Crosshair ();

    HU_Draw_Tip();
    HU_Draw_FSPics();
}

//======================================================================
//                          PLAYER TIPS
//======================================================================
#define MAXTIPLINES 20
char    *tiplines[MAXTIPLINES];
int     numtiplines = 0;
int     tiptime = 0;
int     largestline = 0;



void HU_SetTip(char *tip, int displaytics)
{
  int    i;
  char   *rover, *ctipline, *ctipline_p;


  for(i = 0; i < numtiplines; i++)
    Z_Free(tiplines[i]);


  numtiplines = 0;

  rover = tip;
  ctipline = ctipline_p = Z_Malloc(128, PU_STATIC, NULL);
  *ctipline = 0;
  largestline = 0;

  while(*rover)
  {
    if(*rover == '\n' || strlen(ctipline) + 2 >= 128 || V_StringWidth(ctipline) + 16 >= BASEVIDWIDTH)
    {
      if(numtiplines > MAXTIPLINES)
        break;
      if(V_StringWidth(ctipline) > largestline)
        largestline = V_StringWidth(ctipline);

      tiplines[numtiplines] = ctipline;
      ctipline = ctipline_p = Z_Malloc(128, PU_STATIC, NULL);
      *ctipline = 0;
      numtiplines ++;
    }
    else
    {
      *ctipline_p = *rover;
      ctipline_p++;
      *ctipline_p = 0;
    }
    rover++;

    if(!*rover)
    {
      if(V_StringWidth(ctipline) > largestline)
        largestline = V_StringWidth(ctipline);
      tiplines[numtiplines] = ctipline;
      numtiplines ++;
    }
  }

  tiptime = displaytics;
}




static void HU_Draw_Tip()
{
  int    i;
  if(!numtiplines) return;
  if(!tiptime)
  {
    for(i = 0; i < numtiplines; i++)
      Z_Free(tiplines[i]);
    numtiplines = 0;
    return;
  }
  tiptime--;


  // Draw screen0, scaled
  V_SetupDraw( 0 | V_SCALESTART | V_SCALEPATCH );
  for(i = 0; i < numtiplines; i++)
  {
    V_DrawString((BASEVIDWIDTH - largestline) / 2,
                 ((BASEVIDHEIGHT - (numtiplines * 8)) / 2) + ((i + 1) * 8),
                 0,
                 tiplines[i]);
  }
}


void HU_Clear_Tips()
{
  int    i;

  for(i = 0; i < numtiplines; i++)
    Z_Free(tiplines[i]);
  numtiplines = 0;

  tiptime = 0;
}


//======================================================================
//                           FS HUD Grapics!
//======================================================================
typedef struct
{
  lumpnum_t lumpnum;
  int       xpos;
  int       ypos;
  patch_t   *data;
  boolean   draw;
} fspic_t;

fspic_t*   piclist = NULL;	// realloc, never deallocated
int        num_piclist_alloc = 0;


// HU_InitFSPics
// This function is called when Doom starts and every time the piclist needs
// to be expanded.
void HU_Init_FSPics()
{
  fspic_t * npp;
  int  newstart, newend, i;

  newstart = num_piclist_alloc;
  if( num_piclist_alloc == 0 )
  {
    // Initial allocation.
    newend = 128;
  }
  else
  {
    // Double current allocation.
    newend = (num_piclist_alloc * 2);
  }

  npp = realloc(piclist, sizeof(fspic_t) * newend);
  // Check allocation fail [WDJ]
  if( npp == NULL )
     return;
  
  // Commit to new allocation.
  piclist = npp;
  num_piclist_alloc = newend;

  // Init the added slots to empty.
  for(i = newstart; i < newend; i++)
  {
    piclist[i].lumpnum = NO_LUMP;
    piclist[i].data = NULL;
    piclist[i].draw = false;
    piclist[i].xpos = 0;
    piclist[i].ypos = 0;
  }
}

// Return slot number (handle) for pic, [ 0 .. num_piclist_alloc-1 ].
int  HU_Get_FSPic( lumpnum_t lumpnum, int xpos, int ypos )
{
  int  i;

  if(!num_piclist_alloc)
    HU_Init_FSPics();

getpic_retry:  // retry
  for(i = 0; i < num_piclist_alloc; i++)
  {
    // Find empty slot
    if( VALID_LUMP(piclist[i].lumpnum) )
      continue;

    piclist[i].lumpnum = lumpnum;
    piclist[i].xpos = xpos;
    piclist[i].ypos = ypos;
    piclist[i].draw = false;
    return i;
  }

  // Did not find an empty slot.
  HU_Init_FSPics();
  goto getpic_retry;
}


int  HU_Delete_FSPic(int handle)
{
  if(handle < 0 || handle >= num_piclist_alloc)
    return -1;

  piclist[handle].lumpnum = NO_LUMP;
  piclist[handle].data = NULL;
  return 0;
}


int  HU_Modify_FSPic( int handle, lumpnum_t lumpnum, int xpos, int ypos )
{
  if(handle < 0 || handle >= num_piclist_alloc)
    return -1;

  if( ! VALID_LUMP(piclist[handle].lumpnum) )
    return -1;

  piclist[handle].lumpnum = lumpnum;
  piclist[handle].xpos = xpos;
  piclist[handle].ypos = ypos;
  piclist[handle].data = NULL;
  return 0;
}


// Enable or disable the drawing of a Pic.
int  HU_FS_Display(int handle, boolean enable_draw)
{
  if(handle < 0 || handle >= num_piclist_alloc)
    return -1;
  if( ! VALID_LUMP(piclist[handle].lumpnum) )
    return -1;

  piclist[handle].draw = enable_draw;
  return 0;
}


void HU_Draw_FSPics()
{
  // vid : from video setup
  int  i;

  // [WDJ] Fragglescript overlays must be centered.
  // Needed for Chexquest-newmaps scope with crosshairs.
  // Draw screen0, scaled, menu centering.
  V_SetupDraw( 0 | V_SCALEPATCH | V_SCALESTART | V_CENTERMENU );

  for(i = 0; i < num_piclist_alloc; i++)
  {
    if( ! VALID_LUMP(piclist[i].lumpnum) )
      continue;

    if( ! piclist[i].draw )
      continue;  // not enabled

    if(piclist[i].xpos >= vid.width || piclist[i].ypos >= vid.height)
      continue;  // off screen right

    if(!piclist[i].data)
      piclist[i].data = (patch_t *) W_CachePatchNum(piclist[i].lumpnum, PU_STATIC); // endian fix

    if((piclist[i].xpos + piclist[i].data->width) < 0
       || (piclist[i].ypos + piclist[i].data->height) < 0)
      continue;  // off screen left

    V_DrawScaledPatch(piclist[i].xpos, piclist[i].ypos, piclist[i].data);
  }
  // restore
  //V_SetupDraw( drawinfo.prev_screenflags );
}

void HU_Clear_FSPics()
{
        piclist = NULL;
        num_piclist_alloc = 0;

        HU_Init_FSPics();
}

//======================================================================
//                 HUD MESSAGES CLEARING FROM SCREEN
//======================================================================

//  Clear old messages from the borders around the view window
//  (only for reduced view, refresh the borders when needed)
//
//  startline  : y coord to start clear,
//  clearlines : how many lines to clear.
//
static int     oldclearlines;

void HU_Erase (void)
{
    static  int     secondframelines;

    // vid : from video setup
    int topline;
    int bottomline;
    int y,yoffset;

    //faB: clear hud msgs on double buffer (Glide mode)
    boolean secondframe;

    if (con_clearlines==oldclearlines && !con_hudupdate && !chat_on)
        return;

    // clear the other frame in double-buffer modes
    secondframe = (con_clearlines!=oldclearlines);
    if (secondframe)
        secondframelines = oldclearlines;

    // clear the message lines that go away, so use _oldclearlines_
    bottomline = oldclearlines;
    oldclearlines = con_clearlines;
    if( chat_on )
        if( bottomline < 8 )
            bottomline=8;

    if (automapactive || viewwindowx==0)   // hud msgs don't need to be cleared
        return;

    if( rendermode == render_soft )
    {
        // software mode copies view border pattern & beveled edges from the backbuffer
        topline = 0;
        for (y=topline,yoffset=y*vid.width; y<bottomline ; y++,yoffset+=vid.width)
        {
            if (y < viewwindowy || y >= viewwindowy + rdraw_viewheight)
                R_VideoErase(yoffset, vid.width); // erase entire line
            else
            {
                R_VideoErase(yoffset, viewwindowx); // erase left border
                // erase right border
                R_VideoErase(yoffset + viewwindowx + rdraw_viewwidth, viewwindowx);
            }
        }
        con_hudupdate = false;      // if it was set..
    }
#ifdef HWRENDER 
    else
    {
        // refresh just what is needed from the view borders
        HWR_DrawViewBorder (secondframelines);
        con_hudupdate = secondframe;
    }
#endif
}



//======================================================================
//                   IN-LEVEL DEATHMATCH RANKINGS
//======================================================================

// count frags for each team
int HU_Create_TeamFragTbl(fragsort_t *fragtab,
                         int dmtotals[], int fragtbl[MAXPLAYERS][MAXPLAYERS])
{
    int i,j,k,scorelines,team;

    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            team = (cv_teamplay.EV==1) ? players[i].skincolor
                                       : players[i].skin;

            for(j=0; j<scorelines; j++)
            {
                if (fragtab[j].num == team)
                { // found there team
                     if(fragtbl)
                     {
                         for(k=0; k<MAXPLAYERS; k++)
                         {
                             if(playeringame[k])
                             {
                                 int k_indx = (cv_teamplay.EV==1) ?
                                     players[k].skincolor : players[k].skin;
                                 fragtbl[team][k_indx] += players[i].frags[k];
                             }
                         }
                     }

                     fragtab[j].count += ST_PlayerFrags(i);
                     if(dmtotals)
                         dmtotals[team]=fragtab[j].count;
                     break;
                }
            }  // for j

            if (j==scorelines)
            {   // team not found, add it

                if(fragtbl)
                {
                    for(k=0; k<MAXPLAYERS; k++)
                        fragtbl[team][k] = 0;
                }

                fragtab[scorelines].count = ST_PlayerFrags(i);
                fragtab[scorelines].num   = team;
                fragtab[scorelines].color = players[i].skincolor;
                fragtab[scorelines].name  = get_team_name(team);

                if(fragtbl)
                {
                    for(k=0; k<MAXPLAYERS; k++)
                    {
                        if(playeringame[k])
                        {
                            int k_indx = (cv_teamplay.EV==1) ?
                                players[k].skincolor : players[k].skin;
                            fragtbl[team][k_indx] += players[i].frags[k];
                        }
                    }
                }

                if(dmtotals)
                    dmtotals[team]=fragtab[scorelines].count;

                scorelines++;
            }
        }
    }
    return scorelines;
}


//
//  draw Deathmatch Rankings
//
void HU_Draw_DeathmatchRankings (void)
{
    fragsort_t   fragtab[MAXPLAYERS];
    int          i;
    int          scorelines;
    int          whiteplayer;
    int          y;
    char*	 title;
    boolean	 large;

    // Draw screen0, scaled, centered
    V_SetupDraw( 0 | V_SCALEPATCH | V_SCALESTART | V_CENTERHORZ );

    // draw the ranking title panel
    if(!cv_splitscreen.value)
    {
        patch_t*  p = W_CachePatchName("RANKINGS",PU_CACHE);  // endian fix
        V_DrawScaledPatch ((BASEVIDWIDTH-p->width)/2, 5, p);
    }

    // count frags for each present player
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            fragtab[scorelines].count = ST_PlayerFrags(i);
            fragtab[scorelines].num   = i;
            fragtab[scorelines].color = players[i].skincolor;
            fragtab[scorelines].name  = player_names[i];
            scorelines++;
        }
    }

    //Fab:25-04-98: when you play, you quickly see your frags because your
    //  name is displayed white, when playback demo, you quickly see who's the
    //  view.
    whiteplayer = demoplayback ? displayplayer : consoleplayer;

    if (scorelines>9)
        scorelines = 9; //dont draw past bottom of screen, show the best only
    else if (cv_splitscreen.value && scorelines > 4)
        scorelines = 4;

    if(cv_splitscreen.value)
    {
        y = (100 - (12 * (scorelines + 1) / 2)) + 15;
        title = "Rankings";
        large = false;
    }
    else
    {
        y = 70;
        title = NULL;
        large = true;
    }

    if(cv_teamplay.EV==0)
        WI_Draw_Ranking(title, 80, y, fragtab, scorelines, large, whiteplayer, 32);
    else
    {
        // draw the frag to the right
//        WI_Draw_Ranking("Individual",170,70,fragtab,scorelines,true,whiteplayer);

        scorelines = HU_Create_TeamFragTbl(fragtab,NULL,NULL);

        // and the team frag to the left
        WI_Draw_Ranking("Teams", 80, y, fragtab, scorelines, large, players[whiteplayer].skincolor, 32);
    }
}


// draw the Crosshair, at the exact center of the view.
//
// Crosshairs are pre-cached at HU_Init
#ifdef HWRENDER // not win32 only 19990829 by Kin
    extern float gr_basewindowcentery;
    extern float gr_viewheight;
#endif

void HU_Draw_Crosshair (void)
{
    // vid : from video setup
    int y;

    int chv = cv_crosshair.value & 3;
    if (!chv)
        return;

#if 0
    if (cv_crosshairscale.value)
        V_SetupDraw( 0 | V_SCALEPATCH | V_SCALESTART );
    else
        V_SetupDraw( 0 | V_SCALEPATCH | V_NOSCALE );
#else
    V_SetupDraw( 0 | V_SCALEPATCH | V_NOSCALE );
#endif

#ifdef HWRENDER
    if( rendermode != render_soft ) 
        y = gr_basewindowcentery;
    else
#endif
        y = viewwindowy+(rdraw_viewheight>>1);

    V_DrawTranslucentPatch (vid.width>>1, y, crosshair[chv-1]);

    if( cv_splitscreen.value )
    {
#ifdef HWRENDER
        if ( rendermode != render_soft )
            y += gr_viewheight;
        else
#endif
            y += rdraw_viewheight;

        V_DrawTranslucentPatch (vid.width>>1, y, crosshair[chv-1]);
    }
    // V_SetupDraw( drawinfo.prev_screenflags );  // restore
}


//======================================================================
//                    CHAT MACROS COMMAND & VARS
//======================================================================

// better do HackChatmacros() because the strings are NULL !!

consvar_t cv_chatmacro1 = {"_chatmacro1", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro2 = {"_chatmacro2", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro3 = {"_chatmacro3", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro4 = {"_chatmacro4", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro5 = {"_chatmacro5", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro6 = {"_chatmacro6", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro7 = {"_chatmacro7", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro8 = {"_chatmacro8", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro9 = {"_chatmacro9", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro0 = {"_chatmacro0", NULL, CV_SAVE,NULL};


// Set the chatmacros original text, before config is executed.
// If a dehacked patch is loaded, it will change the default text strings.
// The config.cfg strings will override these.
//
void HU_Init_Chatmacros (void)
{
    int    i;

    // this is the original text, dehacked can modify it as a default value
    cv_chatmacro0.defaultvalue = HUSTR_CHATMACRO0;
    cv_chatmacro1.defaultvalue = HUSTR_CHATMACRO1;
    cv_chatmacro2.defaultvalue = HUSTR_CHATMACRO2;
    cv_chatmacro3.defaultvalue = HUSTR_CHATMACRO3;
    cv_chatmacro4.defaultvalue = HUSTR_CHATMACRO4;
    cv_chatmacro5.defaultvalue = HUSTR_CHATMACRO5;
    cv_chatmacro6.defaultvalue = HUSTR_CHATMACRO6;
    cv_chatmacro7.defaultvalue = HUSTR_CHATMACRO7;
    cv_chatmacro8.defaultvalue = HUSTR_CHATMACRO8;
    cv_chatmacro9.defaultvalue = HUSTR_CHATMACRO9;

    // link chatmacros to cvars
    chat_macros[0] = &cv_chatmacro0;
    chat_macros[1] = &cv_chatmacro1;
    chat_macros[2] = &cv_chatmacro2;
    chat_macros[3] = &cv_chatmacro3;
    chat_macros[4] = &cv_chatmacro4;
    chat_macros[5] = &cv_chatmacro5;
    chat_macros[6] = &cv_chatmacro6;
    chat_macros[7] = &cv_chatmacro7;
    chat_macros[8] = &cv_chatmacro8;
    chat_macros[9] = &cv_chatmacro9;

    // register chatmacro vars ready for config.cfg
    for (i=0; i<10; i++)
       CV_RegisterVar (chat_macros[i]);
}


//  chatmacro <0-9> "chat message"
//
void Command_Chatmacro_f (void)
{
    int    i;

    if (COM_Argc()<2)
    {
        CONS_Printf ("chatmacro <0-9> : view chatmacro\n"
                     "chatmacro <0-9> \"chat message\" : change chatmacro\n");
        return;
    }

    i = atoi(COM_Argv(1)) % 10;

    if (COM_Argc()==2)
    {
        CONS_Printf("chatmacro %d is \"%s\"\n",i,chat_macros[i]->string);
        return;
    }

    // change a chatmacro
    CV_Set (chat_macros[i], COM_Argv(2));
}


void HU_Register_Commands( void )
{
    COM_AddCommand ("say"    , Command_Say_f);
    COM_AddCommand ("sayto"  , Command_Sayto_f);
    COM_AddCommand ("sayteam", Command_Sayteam_f);
    Register_NetXCmd(XD_SAY, Got_NetXCmd_Saycmd);
}
