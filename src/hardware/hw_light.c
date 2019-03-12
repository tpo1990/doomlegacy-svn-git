// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: hw_light.c 1417 2019-01-29 08:00:14Z wesleyjohnson $
//
// Copyright (C) 1998-2015 by DooM Legacy Team.
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
// $Log: hw_light.c,v $
// Revision 1.45  2004/07/27 08:19:38  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.44  2002/09/14 12:36:54  hurdler
// Add heretic lights (I'm happy with that for now)
//
// Revision 1.43  2002/09/10 20:49:07  hurdler
// Add lights to Heretic weapons
//
// Revision 1.42  2001/12/28 16:57:45  hurdler
// Add setcorona command to FS
//
// Revision 1.41  2001/12/26 17:24:47  hurdler
// Update Linux version
//
// Revision 1.40  2001/08/27 19:59:35  hurdler
// Fix colormap in heretic + opengl, fixedcolormap and NEWCORONA
//
// Revision 1.39  2001/08/26 15:27:29  bpereira
// added fov for glide and fixed newcoronas code
//
// Revision 1.38  2001/08/11 01:24:30  hurdler
// Fix backface culling problem with floors/ceiling
//
// Revision 1.37  2001/08/09 21:35:23  hurdler
// Add translucent 3D water in hw mode
//
// Revision 1.36  2001/08/08 20:34:43  hurdler
// Big TANDL update
//
// Revision 1.35  2001/08/06 14:13:45  hurdler
// Crappy MD2 implementation (still need lots of work)
//
// Revision 1.34  2001/04/28 15:18:46  hurdler
// newcoronas defined again
//
// Revision 1.33  2001/04/17 22:30:40  hurdler
// Revision 1.32  2001/04/09 14:17:45  hurdler
// Revision 1.31  2001/02/28 17:50:56  bpereira
// Revision 1.30  2001/02/24 13:35:22  bpereira
// Revision 1.29  2001/01/25 18:56:27  bpereira
// Revision 1.28  2000/11/18 15:51:25  bpereira
// Revision 1.27  2000/11/02 19:49:39  bpereira
// Revision 1.26  2000/10/04 16:21:57  hurdler
// Revision 1.25  2000/09/28 20:57:20  bpereira
// Revision 1.24  2000/09/21 16:45:11  bpereira
// Revision 1.23  2000/08/31 14:30:57  bpereira
// Revision 1.22  2000/08/11 19:11:57  metzgermeister
// Revision 1.21  2000/08/11 12:27:43  hurdler
// Revision 1.20  2000/08/10 19:58:04  bpereira
// Revision 1.19  2000/08/10 14:16:25  hurdler
// Revision 1.18  2000/08/03 17:57:42  bpereira
// Revision 1.17  2000/07/01 09:23:50  bpereira
//
// Revision 1.16  2000/05/09 21:09:18  hurdler
// people prefer coronas on plasma riffles
//
// Revision 1.15  2000/04/24 20:24:38  bpereira
//
// Revision 1.14  2000/04/24 15:46:34  hurdler
// Support colormap for text
//
// Revision 1.13  2000/04/23 16:19:52  bpereira
//
// Revision 1.12  2000/04/18 12:52:21  hurdler
//
// Revision 1.10  2000/04/14 16:34:26  hurdler
// some nice changes for coronas
//
// Revision 1.9  2000/04/12 16:03:51  hurdler
// ready for T&L code and true static lighting
//
// Revision 1.8  2000/04/11 01:00:59  hurdler
// Better coronas support
//
// Revision 1.7  2000/04/09 17:18:01  hurdler
// modified coronas' code for 16 bits video mode
//
// Revision 1.6  2000/04/06 20:50:23  hurdler
// add Boris' changes for coronas in doom3.wad
//
// Revision 1.5  2000/03/29 19:39:49  bpereira
//
// Revision 1.4  2000/03/07 03:31:45  hurdler
// fix linux compilation
//
// Revision 1.3  2000/03/05 17:10:56  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Dynamic/Static lighting & coronas add on by Hurdler
//      !!! Under construction !!!
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "hw_light.h"
#include "hw_main.h"
#include "i_video.h"
#include "z_zone.h"
#include "m_random.h"
#include "m_bbox.h"
#include "m_swap.h"
#include "w_wad.h"
#include "r_state.h"
#include "r_main.h"
#include "p_local.h"

//=============================================================================
//                                                                      DEFINES
//=============================================================================

// [WDJ] Makes debugging difficult when references like these are hidden.
// Make them visible in the code.
//#define DL_SQRRADIUS(x)     dynlights->p_lspr[(x)]->dynamic_sqrradius
//#define DL_RADIUS(x)        dynlights->p_lspr[(x)]->dynamic_radius
//#define LIGHT_POS(i)        dynlights->position[(i)]

#define DL_HIGH_QUALITY
//#define LIGHTMAPFLAGS  (PF_Masked|PF_Clip|PF_NoAlphaTest)  // debug see overdraw
#define LIGHTMAPFLAGS (PF_Modulated|PF_Additive|PF_Clip)

//=============================================================================
//                                                                       GLOBAL
//=============================================================================


void CV_grMonsterDL_OnChange (void);

consvar_t cv_grdynamiclighting = {"gr_dynamiclighting",  "On", CV_SAVE, CV_OnOff };
consvar_t cv_grstaticlighting  = {"gr_staticlighting",   "On", CV_SAVE, CV_OnOff };
#ifdef CORONA_CHOICE
CV_PossibleValue_t grcorona_cons_t[] = { {0, "Off"}, {1, "Sprite"}, {2, "Dyn"}, {3, "Auto"}, {0, NULL} };
consvar_t cv_grcoronas         = {"gr_coronas",          "Auto", CV_SAVE, grcorona_cons_t };
#else
consvar_t cv_grcoronas         = {"gr_coronas",          "On", CV_SAVE, CV_OnOff };
#endif
consvar_t cv_grcoronasize      = {"gr_coronasize",        "1", CV_SAVE| CV_FLOAT,0 };
consvar_t cv_grmblighting      = {"gr_mblighting",       "On", CV_SAVE|CV_CALL
                                  , CV_OnOff, CV_grMonsterDL_OnChange };

// Select by view, using indirection into view_dynlights.
static dynlights_t view_dynlights[2]; // 2 players in splitscreen mode
static dynlights_t *dynlights = &view_dynlights[0];


// Enum type  sprite_light_e  is defined in r_defs.h.
// It defines SPLGT_xxx for the xxx_SPR names used in scripts.

//Hurdler: now we can change those values via FS :)
light_t lspr[NUMLIGHTS] = {
    // type       offset x,   y  coronas color, c_size,light color,l_radius, sqr radius computed at init
   // UNDEFINED: 0  
    { SPLGT_none,     0.0f,   0.0f,        0x0,  24.0f,        0x0,   0.0f },
    // weapons
    // PLASMA_L
    { SPLGT_dynamic,  0.0f,   0.0f, 0x60ff7750,  24.0f, 0x20f77760,  80.0f },
    // PLASMAEXP_L
    { SPLGT_dynamic,  0.0f,   0.0f, 0x60ff7750,  24.0f, 0x40f77760, 120.0f },
    // ROCKET_L
    { SPLGT_rocket,   0.0f,   0.0f, 0x606060f0,  20.0f, 0x4020f7f7, 120.0f },
    // ROCKETEXP_L
    { SPLGT_dynamic,  0.0f,   0.0f, 0x606060f0,  20.0f, 0x6020f7f7, 200.0f },
    // BFG_L
    { SPLGT_dynamic,  0.0f,   0.0f, 0x6077f777, 120.0f, 0x8060f060, 200.0f },
    // BFGEXP_L
    { SPLGT_dynamic,  0.0f,   0.0f, 0x6077f777, 120.0f, 0x6060f060, 400.0f },

    // tall lights
    // BLUETALL_L
    { SPLGT_light,    0.0f,  27.0f, 0x80ff7070,  75.0f, 0x40ff5050, 100.0f },
    // GREENTALL_L
    { SPLGT_light,    0.0f,  27.0f, 0x5060ff60,  75.0f, 0x4070ff70, 100.0f },
    // REDTALL_L
    { SPLGT_light,    0.0f,  27.0f, 0x705070ff,  75.0f, 0x405070ff, 100.0f },

    // small lights
    // BLUESMALL_L
    { SPLGT_light,    0.0f,  14.0f, 0x80ff7070,  60.0f, 0x40ff5050, 100.0f },
    // GREENSMALL_L
    { SPLGT_light,    0.0f,  14.0f, 0x6070ff70,  60.0f, 0x4070ff70, 100.0f },
    // REDSMALL_L
    { SPLGT_light,    0.0f,  14.0f, 0x705070ff,  60.0f, 0x405070ff, 100.0f },

    // other lights
    // TECHLAMP_L
    { SPLGT_light,    0.0f,  33.0f, 0x80ffb0b0,  75.0f, 0x40ffb0b0, 100.0f },
    // TECHLAMP2_L
    { SPLGT_light,    0.0f,  26.0f, 0x80ffb0b0,  60.0f, 0x40ffb0b0, 100.0f },
    // COLUMN_L
    { SPLGT_light,    3.0f,  19.0f, 0x80b0f0f0,  60.0f, 0x40b0f0f0, 100.0f },
    // CANDLE_L
    { SPLGT_light,    0.0f,   6.0f, 0x60b0f0f0,  20.0f, 0x30b0f0f0,  30.0f },
    // CANDLEABRE_L
    { SPLGT_light,    0.0f,  30.0f, 0x60b0f0f0,  60.0f, 0x30b0f0f0, 100.0f },
    
    // monsters
    // REDBALL_L
    { SPLGT_dynamic,   0.0f,   0.0f, 0x606060f0,   0.0f, 0x302070ff, 100.0f },
    // GREENBALL_L
    { SPLGT_dynamic,   0.0f,   0.0f, 0x6077f777, 120.0f, 0x3060f060, 100.0f },
    // ROCKET2_L
    { SPLGT_dynamic,   0.0f,   0.0f, 0x606060f0,  20.0f, 0x4020f7f7, 120.0f },

    // weapons
    // FX03_L
    { SPLGT_dynamic,   0.0f,   0.0f, 0x6077ff50,  24.0f, 0x2077f760,  80.0f },
    // FX17_L
    { SPLGT_dynamic,   0.0f,   0.0f, 0x60ff7750,  24.0f, 0x40f77760,  80.0f },
    // FX00_L
    { SPLGT_dynamic,   0.0f,   0.0f, 0x602020ff,  24.0f, 0x302020f7,  80.0f },
    // FX08_L
    { SPLGT_rocket,    0.0f,   0.0f, 0x606060f0,  20.0f, 0x4020c0f7, 120.0f },
    // FX04_L
    { SPLGT_rocket,    0.0f,   0.0f, 0x606060f0,  20.0f, 0x2020c0f7, 120.0f },
    // FX02_L
    { SPLGT_rocket,    0.0f,   0.0f, 0x606060f0,  20.0f, 0x1720f7f7, 120.0f },

    //lights
    // WTRH_L
    { SPLGT_dynamic,   0.0f,  68.0f, 0x606060f0,  60.0f, 0x4020a0f7, 100.0f },
    // SRTC_L
    { SPLGT_dynamic,   0.0f,  27.0f, 0x606060f0,  60.0f, 0x4020a0f7, 100.0f },
    // CHDL_L
    { SPLGT_dynamic,   0.0f,  -8.0f, 0x606060f0,  60.0f, 0x502070f7, 100.0f },
    // KFR1_L
    { SPLGT_dynamic,   0.0f,  27.0f, 0x606060f0,  60.0f, 0x4020a0f7, 100.0f },
};


static light_t *t_lspr[NUMSPRITES] = {
    &lspr[NOLIGHT],     // SPR_TROO
    &lspr[NOLIGHT],     // SPR_SHTG
    &lspr[NOLIGHT],     // SPR_PUNG
    &lspr[NOLIGHT],     // SPR_PISG
    &lspr[NOLIGHT],     // SPR_PISF
    &lspr[NOLIGHT],     // SPR_SHTF
    &lspr[NOLIGHT],     // SPR_SHT2
    &lspr[NOLIGHT],     // SPR_CHGG
    &lspr[NOLIGHT],     // SPR_CHGF
    &lspr[NOLIGHT],     // SPR_MISG
    &lspr[NOLIGHT],     // SPR_MISF
    &lspr[NOLIGHT],     // SPR_SAWG
    &lspr[NOLIGHT],     // SPR_PLSG
    &lspr[NOLIGHT],     // SPR_PLSF
    &lspr[NOLIGHT],     // SPR_BFGG
    &lspr[NOLIGHT],     // SPR_BFGF
    &lspr[NOLIGHT],     // SPR_BLUD
    &lspr[NOLIGHT],     // SPR_PUFF
    &lspr[REDBALL_L],   // SPR_BAL1 * // imp
    &lspr[REDBALL_L],   // SPR_BAL2 * // cacodemon
    &lspr[PLASMA_L],    // SPR_PLSS * // plasma
    &lspr[PLASMAEXP_L], // SPR_PLSE * // plasma explosion
    &lspr[ROCKET_L],    // SPR_MISL * // rocket
    &lspr[BFG_L],       // SPR_BFS1 * // bfg
    &lspr[BFGEXP_L],    // SPR_BFE1 * // bfg explosion
    &lspr[NOLIGHT],     // SPR_BFE2
    &lspr[GREENBALL_L], // SPR_TFOG * teleport fog
    &lspr[PLASMA_L],    // SPR_IFOG * respaw fog
    &lspr[NOLIGHT],     // SPR_PLAY
    &lspr[NOLIGHT],     // SPR_POSS
    &lspr[NOLIGHT],     // SPR_SPOS
    &lspr[NOLIGHT],     // SPR_VILE
    &lspr[NOLIGHT],     // SPR_FIRE
    &lspr[REDBALL_L],   // SPR_FATB * // revenent tracer
    &lspr[NOLIGHT],     // SPR_FBXP
    &lspr[NOLIGHT],     // SPR_SKEL
    &lspr[ROCKET2_L],   // SPR_MANF * // mancubus
    &lspr[NOLIGHT],     // SPR_FATT
    &lspr[NOLIGHT],     // SPR_CPOS
    &lspr[NOLIGHT],     // SPR_SARG
    &lspr[NOLIGHT],     // SPR_HEAD
    &lspr[GREENBALL_L], // SPR_BAL7 * // hell knight / baron of hell
    &lspr[NOLIGHT],     // SPR_BOSS
    &lspr[NOLIGHT],     // SPR_BOS2
    &lspr[REDBALL_L],   // SPR_SKUL // lost soul
    &lspr[NOLIGHT],     // SPR_SPID
    &lspr[NOLIGHT],     // SPR_BSPI
    &lspr[GREENBALL_L], // SPR_APLS * // arachnotron
    &lspr[GREENBALL_L], // SPR_APBX * // arachnotron explosion
    &lspr[NOLIGHT],     // SPR_CYBR
    &lspr[NOLIGHT],     // SPR_PAIN
    &lspr[NOLIGHT],     // SPR_SSWV
    &lspr[NOLIGHT],     // SPR_KEEN
    &lspr[NOLIGHT],     // SPR_BBRN
    &lspr[NOLIGHT],     // SPR_BOSF
    &lspr[NOLIGHT],     // SPR_ARM1
    &lspr[NOLIGHT],     // SPR_ARM2
    &lspr[NOLIGHT],     // SPR_BAR1
    &lspr[ROCKETEXP_L], // SPR_BEXP // barrel explosion
    &lspr[NOLIGHT],     // SPR_FCAN
    &lspr[NOLIGHT],     // SPR_BON1
    &lspr[NOLIGHT],     // SPR_BON2
    &lspr[NOLIGHT],     // SPR_BKEY
    &lspr[NOLIGHT],     // SPR_RKEY
    &lspr[NOLIGHT],     // SPR_YKEY
    &lspr[NOLIGHT],     // SPR_BSKU
    &lspr[NOLIGHT],     // SPR_RSKU
    &lspr[NOLIGHT],     // SPR_YSKU
    &lspr[NOLIGHT],     // SPR_STIM
    &lspr[NOLIGHT],     // SPR_MEDI
    &lspr[NOLIGHT],     // SPR_SOUL
    &lspr[NOLIGHT],     // SPR_PINV
    &lspr[NOLIGHT],     // SPR_PSTR
    &lspr[NOLIGHT],     // SPR_PINS
    &lspr[NOLIGHT],     // SPR_MEGA
    &lspr[NOLIGHT],     // SPR_SUIT
    &lspr[NOLIGHT],     // SPR_PMAP
    &lspr[NOLIGHT],     // SPR_PVIS
    &lspr[NOLIGHT],     // SPR_CLIP
    &lspr[NOLIGHT],     // SPR_AMMO
    &lspr[NOLIGHT],     // SPR_ROCK
    &lspr[NOLIGHT],     // SPR_BROK
    &lspr[NOLIGHT],     // SPR_CELL
    &lspr[NOLIGHT],     // SPR_CELP
    &lspr[NOLIGHT],     // SPR_SHEL
    &lspr[NOLIGHT],     // SPR_SBOX
    &lspr[NOLIGHT],     // SPR_BPAK
    &lspr[NOLIGHT],     // SPR_BFUG
    &lspr[NOLIGHT],     // SPR_MGUN
    &lspr[NOLIGHT],     // SPR_CSAW
    &lspr[NOLIGHT],     // SPR_LAUN
    &lspr[NOLIGHT],     // SPR_PLAS
    &lspr[NOLIGHT],     // SPR_SHOT
    &lspr[NOLIGHT],     // SPR_SGN2
    &lspr[COLUMN_L],    // SPR_COLU * // yellow little light column
    &lspr[NOLIGHT],     // SPR_SMT2
    &lspr[NOLIGHT],     // SPR_GOR1
    &lspr[NOLIGHT],     // SPR_POL2
    &lspr[NOLIGHT],     // SPR_POL5
    &lspr[NOLIGHT],     // SPR_POL4
    &lspr[NOLIGHT],     // SPR_POL3
    &lspr[NOLIGHT],     // SPR_POL1
    &lspr[NOLIGHT],     // SPR_POL6
    &lspr[NOLIGHT],     // SPR_GOR2
    &lspr[NOLIGHT],     // SPR_GOR3
    &lspr[NOLIGHT],     // SPR_GOR4
    &lspr[NOLIGHT],     // SPR_GOR5
    &lspr[NOLIGHT],     // SPR_SMIT
    &lspr[NOLIGHT],     // SPR_COL1
    &lspr[NOLIGHT],     // SPR_COL2
    &lspr[NOLIGHT],     // SPR_COL3
    &lspr[NOLIGHT],     // SPR_COL4
    &lspr[CANDLE_L],    // SPR_CAND * // candle
    &lspr[CANDLEABRE_L],// SPR_CBRA * // candleabre
    &lspr[NOLIGHT],     // SPR_COL6
    &lspr[NOLIGHT],     // SPR_TRE1
    &lspr[NOLIGHT],     // SPR_TRE2
    &lspr[NOLIGHT],     // SPR_ELEC
    &lspr[NOLIGHT],     // SPR_CEYE
    &lspr[NOLIGHT],     // SPR_FSKU
    &lspr[NOLIGHT],     // SPR_COL5
    &lspr[BLUETALL_L],  // SPR_TBLU *
    &lspr[GREENTALL_L], // SPR_TGRN *
    &lspr[REDTALL_L],   // SPR_TRED *
    &lspr[BLUESMALL_L], // SPR_SMBT *
    &lspr[GREENSMALL_L],// SPR_SMGT *
    &lspr[REDSMALL_L],  // SPR_SMRT *
    &lspr[NOLIGHT],     // SPR_HDB1
    &lspr[NOLIGHT],     // SPR_HDB2
    &lspr[NOLIGHT],     // SPR_HDB3
    &lspr[NOLIGHT],     // SPR_HDB4
    &lspr[NOLIGHT],     // SPR_HDB5
    &lspr[NOLIGHT],     // SPR_HDB6
    &lspr[NOLIGHT],     // SPR_POB1
    &lspr[NOLIGHT],     // SPR_POB2
    &lspr[NOLIGHT],     // SPR_BRS1
    &lspr[TECHLAMP_L],  // SPR_TLMP *
    &lspr[TECHLAMP2_L], // SPR_TLP2 *
    &lspr[NOLIGHT],     // SPR_SMOK
    &lspr[NOLIGHT],     // SPR_SPLA
    &lspr[NOLIGHT],     // SPR_TNT1

// heretic sprites

    &lspr[NOLIGHT],     // SPR_IMPX,
    &lspr[NOLIGHT],     // SPR_ACLO,
    &lspr[NOLIGHT],     // SPR_PTN1,
    &lspr[NOLIGHT],     // SPR_SHLD,
    &lspr[NOLIGHT],     // SPR_SHD2,
    &lspr[NOLIGHT],     // SPR_BAGH,
    &lspr[NOLIGHT],     // SPR_SPMP,
    &lspr[NOLIGHT],     // SPR_INVS,
    &lspr[NOLIGHT],     // SPR_PTN2,
    &lspr[NOLIGHT],     // SPR_SOAR,
    &lspr[NOLIGHT],     // SPR_INVU,
    &lspr[NOLIGHT],     // SPR_PWBK,
    &lspr[NOLIGHT],     // SPR_EGGC,
    &lspr[NOLIGHT],     // SPR_EGGM,
    &lspr[NOLIGHT],     // SPR_FX01,
    &lspr[NOLIGHT],     // SPR_SPHL,
    &lspr[NOLIGHT],     // SPR_TRCH,
    &lspr[NOLIGHT],     // SPR_FBMB,
    &lspr[NOLIGHT],     // SPR_XPL1,
    &lspr[NOLIGHT],     // SPR_ATLP,
    &lspr[NOLIGHT],     // SPR_PPOD,
    &lspr[NOLIGHT],     // SPR_AMG1,
    &lspr[NOLIGHT],     // SPR_SPSH,
    &lspr[NOLIGHT],     // SPR_LVAS,
    &lspr[NOLIGHT],     // SPR_SLDG,
    &lspr[NOLIGHT],     // SPR_SKH1,
    &lspr[NOLIGHT],     // SPR_SKH2,
    &lspr[NOLIGHT],     // SPR_SKH3,
    &lspr[NOLIGHT],     // SPR_SKH4,
    &lspr[CHDL_L],      // SPR_CHDL,
    &lspr[SRTC_L],      // SPR_SRTC,
    &lspr[NOLIGHT],     // SPR_SMPL,
    &lspr[NOLIGHT],     // SPR_STGS,
    &lspr[NOLIGHT],     // SPR_STGL,
    &lspr[NOLIGHT],     // SPR_STCS,
    &lspr[NOLIGHT],     // SPR_STCL,
    &lspr[KFR1_L],      // SPR_KFR1,
    &lspr[NOLIGHT],     // SPR_BARL,
    &lspr[NOLIGHT],     // SPR_BRPL,
    &lspr[NOLIGHT],     // SPR_MOS1,
    &lspr[NOLIGHT],     // SPR_MOS2,
    &lspr[WTRH_L],      // SPR_WTRH,
    &lspr[NOLIGHT],     // SPR_HCOR,
    &lspr[NOLIGHT],     // SPR_KGZ1,
    &lspr[NOLIGHT],     // SPR_KGZB,
    &lspr[NOLIGHT],     // SPR_KGZG,
    &lspr[NOLIGHT],     // SPR_KGZY,
    &lspr[NOLIGHT],     // SPR_VLCO,
    &lspr[NOLIGHT],     // SPR_VFBL,
    &lspr[NOLIGHT],     // SPR_VTFB,
    &lspr[NOLIGHT],     // SPR_SFFI,
    &lspr[NOLIGHT],     // SPR_TGLT,
    &lspr[NOLIGHT],     // SPR_TELE,
    &lspr[NOLIGHT],     // SPR_STFF,
    &lspr[NOLIGHT],     // SPR_PUF3,
    &lspr[NOLIGHT],     // SPR_PUF4,
    &lspr[NOLIGHT],     // SPR_BEAK,
    &lspr[NOLIGHT],     // SPR_WGNT,
    &lspr[NOLIGHT],     // SPR_GAUN,
    &lspr[NOLIGHT],     // SPR_PUF1,
    &lspr[NOLIGHT],     // SPR_WBLS,
    &lspr[NOLIGHT],     // SPR_BLSR,
    &lspr[NOLIGHT],     // SPR_FX18,
    &lspr[FX17_L],      // SPR_FX17,
    &lspr[NOLIGHT],     // SPR_WMCE,
    &lspr[NOLIGHT],     // SPR_MACE,
    &lspr[FX02_L],      // SPR_FX02,
    &lspr[NOLIGHT],     // SPR_WSKL,
    &lspr[NOLIGHT],     // SPR_HROD,
    &lspr[FX00_L],      // SPR_FX00,
    &lspr[NOLIGHT],     // SPR_FX20,
    &lspr[NOLIGHT],     // SPR_FX21,
    &lspr[NOLIGHT],     // SPR_FX22,
    &lspr[NOLIGHT],     // SPR_FX23,
    &lspr[NOLIGHT],     // SPR_GWND,
    &lspr[NOLIGHT],     // SPR_PUF2,
    &lspr[NOLIGHT],     // SPR_WPHX,
    &lspr[NOLIGHT],     // SPR_PHNX,
    &lspr[FX04_L],      // SPR_FX04,
    &lspr[FX08_L],      // SPR_FX08,
    &lspr[NOLIGHT],     // SPR_FX09,
    &lspr[NOLIGHT],     // SPR_WBOW,
    &lspr[NOLIGHT],     // SPR_CRBW,
    &lspr[FX03_L],      // SPR_FX03,
//    &lspr[NOLIGHT],     // SPR_BLOD,
//    &lspr[NOLIGHT],     // SPR_PLAY,
    &lspr[NOLIGHT],     // SPR_FDTH,
    &lspr[NOLIGHT],     // SPR_BSKL,
    &lspr[NOLIGHT],     // SPR_CHKN,
    &lspr[NOLIGHT],     // SPR_MUMM,
    &lspr[NOLIGHT],     // SPR_FX15,
    &lspr[NOLIGHT],     // SPR_BEAS,
    &lspr[NOLIGHT],     // SPR_FRB1,
    &lspr[NOLIGHT],     // SPR_SNKE,
    &lspr[NOLIGHT],     // SPR_SNFX,
    &lspr[NOLIGHT],     // SPR_HHEAD,
    &lspr[NOLIGHT],     // SPR_FX05,
    &lspr[NOLIGHT],     // SPR_FX06,
    &lspr[NOLIGHT],     // SPR_FX07,
    &lspr[NOLIGHT],     // SPR_CLNK,
    &lspr[NOLIGHT],     // SPR_WZRD,
    &lspr[NOLIGHT],     // SPR_FX11,
    &lspr[NOLIGHT],     // SPR_FX10,
    &lspr[NOLIGHT],     // SPR_KNIG,
    &lspr[NOLIGHT],     // SPR_SPAX,
    &lspr[NOLIGHT],     // SPR_RAXE,
    &lspr[NOLIGHT],     // SPR_SRCR,
    &lspr[NOLIGHT],     // SPR_FX14,
    &lspr[NOLIGHT],     // SPR_SOR2,
    &lspr[NOLIGHT],     // SPR_SDTH,
    &lspr[NOLIGHT],     // SPR_FX16,
    &lspr[NOLIGHT],     // SPR_MNTR,
    &lspr[NOLIGHT],     // SPR_FX12,
    &lspr[NOLIGHT],     // SPR_FX13,
    &lspr[NOLIGHT],     // SPR_AKYY,
    &lspr[NOLIGHT],     // SPR_BKYY,
    &lspr[NOLIGHT],     // SPR_CKYY,
    &lspr[NOLIGHT],     // SPR_AMG2,
    &lspr[NOLIGHT],     // SPR_AMM1,
    &lspr[NOLIGHT],     // SPR_AMM2,
    &lspr[NOLIGHT],     // SPR_AMC1,
    &lspr[NOLIGHT],     // SPR_AMC2,
    &lspr[NOLIGHT],     // SPR_AMS1,
    &lspr[NOLIGHT],     // SPR_AMS2,
    &lspr[NOLIGHT],     // SPR_AMP1,
    &lspr[NOLIGHT],     // SPR_AMP2,
    &lspr[NOLIGHT],     // SPR_AMB1,
    &lspr[NOLIGHT],     // SPR_AMB2,
 
 };
 
 
 //=============================================================================
 //                                                                       EXTERN
 //=============================================================================
 
 extern  float   gr_viewludsin;
 extern  float   gr_viewludcos;


//=============================================================================
//                                                                       EXTERN
//=============================================================================

extern  float   gr_viewludsin;
extern  float   gr_viewludcos;


//=============================================================================
//                                                                       PROTOS
//=============================================================================

static void  HWR_SetLight( void );

void CV_grMonsterDL_OnChange (void)
{
    if (cv_grmblighting.value)
    {
        t_lspr[SPR_BAL1] = &lspr[REDBALL_L];
        t_lspr[SPR_BAL2] = &lspr[REDBALL_L];
        t_lspr[SPR_MANF] = &lspr[ROCKET2_L];
        t_lspr[SPR_BAL7] = &lspr[GREENBALL_L];
        t_lspr[SPR_APLS] = &lspr[GREENBALL_L];
        t_lspr[SPR_APBX] = &lspr[GREENBALL_L];
        t_lspr[SPR_SKUL] = &lspr[REDBALL_L];
        t_lspr[SPR_FATB] = &lspr[REDBALL_L];
    }
    else
    {
        t_lspr[SPR_BAL1] = &lspr[NOLIGHT];
        t_lspr[SPR_BAL2] = &lspr[NOLIGHT];
        t_lspr[SPR_MANF] = &lspr[NOLIGHT];
        t_lspr[SPR_BAL7] = &lspr[NOLIGHT];
        t_lspr[SPR_APLS] = &lspr[NOLIGHT];
        t_lspr[SPR_APBX] = &lspr[NOLIGHT];
        t_lspr[SPR_SKUL] = &lspr[NOLIGHT];
        t_lspr[SPR_FATB] = &lspr[NOLIGHT];
    }
}

// --------------------------------------------------------------------------
// calcul la projection d'un point sur une droite (determin�e par deux 
// points) et ensuite calcul la distance (au carr�) de ce point au point
// project� sur cette droite
// --------------------------------------------------------------------------
static float HWR_DistP2D(vxtx3d_t *p1, vxtx3d_t *p2, v3d_t *p3, v3d_t *inter)
{
    if (p1->z == p2->z) {
        inter->x = p3->x;
        inter->z = p1->z;
    } else if (p1->x == p2->x) {
        inter->x = p1->x;
        inter->z = p3->z;
    } else {
        register float local, pente;
        // Wat een mooie formula! Hurdler's math ;-)
        pente = ( p1->z - p2->z ) / ( p1->x - p2->x );
        local = p1->z - p1->x*pente;
        inter->x = (p3->z - local + p3->x/pente) * (pente/(pente*pente+1));
        inter->z = inter->x*pente + local;
    }

    return (p3->x - inter->x) * (p3->x - inter->x)
         + (p3->z - inter->z) * (p3->z - inter->z);
}

// check if sphere (radius r) centred in p3 touch the bounding box defined by p1, p2
static
boolean SphereTouchBBox3D(vxtx3d_t *p1, vxtx3d_t *p2, v3d_t *p3, float r)
{
    float minx=p1->x,maxx=p2->x,miny=p2->y,maxy=p1->y,minz=p2->z,maxz=p1->z;

    if( minx>maxx )
    {
        minx=maxx;
        maxx=p1->x;
    }
    if( minx-r > p3->x ) return false;
    if( maxx+r < p3->x ) return false;

    if( miny>maxy )
    {
        miny=maxy;
        maxy=p2->y;
    }
    if( miny-r > p3->y ) return false;
    if( maxy+r < p3->y ) return false;

    if( minz>maxz )
    {
        minz=maxz;
        maxz=p2->z;
    }
    if( minz-r > p3->z ) return false;
    if( maxz+r < p3->z ) return false;

    return true;
}

// Hurdler: The old code was removed by me because I don't think it will be used one day.
//          (It's still available on the CVS for educational purpose: Revision 1.8)

// --------------------------------------------------------------------------
// calcul du dynamic lighting sur les murs
// lVerts contient les coords du mur sans le mlook (up/down)
// --------------------------------------------------------------------------
void HWR_WallLighting(vxtx3d_t *wlVerts)
{
    FSurfaceInfo_t  Surf;
    light_t *       lsp;  // dynlights lspr
    v3d_t *         light_pos;
    v3d_t           inter;
    int             i, j;
    float           dist_p2d, d[4], s;

    // dynlights->nb == 0 if cv_grdynamiclighting.value is not set
    for (j=0; j<dynlights->nb; j++) {
        lsp = dynlights->p_lspr[j];
        light_pos = & dynlights->position[j];
        // check bounding box first
        if( SphereTouchBBox3D(&wlVerts[2], &wlVerts[0], light_pos, lsp->dynamic_radius )
	    ==false )
            continue;

        d[0] = wlVerts[2].x - wlVerts[0].x;
        d[1] = wlVerts[2].z - wlVerts[0].z;
        d[2] = light_pos->x - wlVerts[0].x;
        d[3] = light_pos->z - wlVerts[0].z;
        // backface cull
        if( d[2]*d[1] - d[3]*d[0] < 0 )
            continue;

        // check exact distance
        dist_p2d = HWR_DistP2D(&wlVerts[2], &wlVerts[0], light_pos, &inter);
        if (dist_p2d >= lsp->dynamic_sqrradius)
            continue;

        d[0] = sqrt((wlVerts[0].x-inter.x)*(wlVerts[0].x-inter.x)+(wlVerts[0].z-inter.z)*(wlVerts[0].z-inter.z));
        d[1] = sqrt((wlVerts[2].x-inter.x)*(wlVerts[2].x-inter.x)+(wlVerts[2].z-inter.z)*(wlVerts[2].z-inter.z));
        //dAB = sqrt((wlVerts[0].x-wlVerts[2].x)*(wlVerts[0].x-wlVerts[2].x)+(wlVerts[0].z-wlVerts[2].z)*(wlVerts[0].z-wlVerts[2].z));
        //if ( (d[0] < dAB) && (d[1] < dAB) ) // test if the intersection is on the wall
        //{
        //    d[0] = -d[0]; // if yes, the left distance must be negative for texcoord
        //}
        // test if the intersection is on the wall
        if ( (wlVerts[0].x<inter.x && wlVerts[2].x>inter.x) ||
             (wlVerts[0].x>inter.x && wlVerts[2].x<inter.x) ||
             (wlVerts[0].z<inter.z && wlVerts[2].z>inter.z) ||
             (wlVerts[0].z>inter.z && wlVerts[2].z<inter.z) )
        {
            d[0] = -d[0]; // if yes, the left distance must be negative for texcoord
        }
        d[2] = d[1]; d[3] = d[0];
#ifdef DL_HIGH_QUALITY
        s = 0.5f / lsp->dynamic_radius;
#else
        s = 0.5f / sqrt(lsp->dynamic_sqrradius - dist_p2d);
#endif
        for (i=0; i<4; i++) {
            wlVerts[i].sow = 0.5f + d[i]*s;
            wlVerts[i].tow = 0.5f + (wlVerts[i].y - light_pos->y)*s*1.2f;
        }

        HWR_SetLight();

        // [WDJ] FIXME: Do not know why this is swap, it is not done for corona.
        // Is hardware little-endian ??
        Surf.FlatColor.rgba = LE_SWAP32(dynlights->p_lspr[j]->dynamic_color);
#ifdef DL_HIGH_QUALITY
        Surf.FlatColor.s.alpha *= (1 - dist_p2d/lsp->dynamic_sqrradius);
#endif
        if( !dynlights->mo[j]->state )
            return;
        // next state is null so fade out with alpha
        if( (dynlights->mo[j]->state->nextstate == S_NULL) )
            Surf.FlatColor.s.alpha *= (float)dynlights->mo[j]->tics/(float)dynlights->mo[j]->state->tics;

        HWD.pfnDrawPolygon ( &Surf, wlVerts, 4, LIGHTMAPFLAGS );

    } // end for (j=0; j<dynlights->nb; j++)
}


// BP: big hack for a test in lighting ref:1249753487AB
extern int *bbox;
extern FTransform_t atransform;
// --------------------------------------------------------------------------
// calcul du dynamic lighting sur le sol
// clVerts contient les coords du sol avec le mlook (up/down)
// --------------------------------------------------------------------------
void HWR_PlaneLighting(vxtx3d_t *clVerts, int nrClipVerts)
{
    FSurfaceInfo_t  Surf;
    light_t *       lsp;  // dynlights lspr
    v3d_t *         light_pos;
    float           dist_p2d, s;
    int     i, j;
    vxtx3d_t p1,p2;

    p1.z=FIXED_TO_FLOAT( bbox[BOXTOP] );
    p1.x=FIXED_TO_FLOAT( bbox[BOXLEFT] );
    p2.z=FIXED_TO_FLOAT( bbox[BOXBOTTOM] );
    p2.x=FIXED_TO_FLOAT( bbox[BOXRIGHT] );
    p2.y=clVerts[0].y;
    p1.y=clVerts[0].y;

    for (j=0; j<dynlights->nb; j++) {
        lsp = dynlights->p_lspr[j];
        light_pos = & dynlights->position[j];
        // BP: The kickass Optimization: check if light touch bounding box
        if( SphereTouchBBox3D(&p1, &p2, light_pos, lsp->dynamic_radius)
	    ==false )
            continue;

        // backface cull
        //Hurdler: doesn't work with new TANDL code
        if( (clVerts[0].y > atransform.z)       // true mean it is a ceiling false is a floor
             ^ (light_pos->y < clVerts[0].y) ) // true mean light is down plane, false light is up plane
             continue;

        dist_p2d = (clVerts[0].y - light_pos->y);
        dist_p2d *= dist_p2d;
        // done in SphereTouchBBox3D
        //if (dist_p2d >= lsp->dynamic_sqrradius)
        //    continue;
        
#ifdef DL_HIGH_QUALITY
        s = 0.5f / lsp->dynamic_radius;
#else
        s = 0.5f / sqrt(lsp->dynamic_sqrradius - dist_p2d);
#endif
        for (i=0; i<nrClipVerts; i++) {
            clVerts[i].sow = 0.5f + (clVerts[i].x - light_pos->x)*s;
            clVerts[i].tow = 0.5f + (clVerts[i].z - light_pos->z)*s*1.2f;
        }

        HWR_SetLight();

        // [WDJ] FIXME: Do not know why this is swap, it is not done for corona.
        // Is hardware little-endian ??
        Surf.FlatColor.rgba = LE_SWAP32(dynlights->p_lspr[j]->dynamic_color);
#ifdef DL_HIGH_QUALITY
        Surf.FlatColor.s.alpha *= (1 - dist_p2d/lsp->dynamic_sqrradius);
#endif
        if( !dynlights->mo[j]->state )
            return;

        // next state is null so fade out with alpha
        if( (dynlights->mo[j]->state->nextstate == S_NULL) )
            Surf.FlatColor.s.alpha *= (float)dynlights->mo[j]->tics/(float)dynlights->mo[j]->state->tics;

        HWD.pfnDrawPolygon ( &Surf, clVerts, nrClipVerts, LIGHTMAPFLAGS );

    } // end for (j=0; j<dynlights->nb; j++)
}


static lumpnum_t  corona_lumpnum;

#ifdef SPDR_CORONAS
// --------------------------------------------------------------------------
// coronas lighting
// --------------------------------------------------------------------------
void HWR_DoCoronasLighting(vxtx3d_t *outVerts, gr_vissprite_t *spr) 
{
    FSurfaceInfo_t  Surf;
    vxtx3d_t        light[4];
    light_t   * p_lspr;
    float size;

    //CONS_Printf("sprite (type): %d (%s)\n", spr->type, sprnames[spr->type]);
    p_lspr = t_lspr[spr->mobj->sprite];
    if( (spr->mobj->state >= &states[S_EXPLODE1]
         && spr->mobj->state <= &states[S_EXPLODE3])
     || (spr->mobj->state >= &states[S_FATSHOTX1]
         && spr->mobj->state <= &states[S_FATSHOTX3]))
    {
        p_lspr = &lspr[ROCKETEXP_L];
    }

    // Objects which emit light.
    if ( cv_grcoronas.value && (p_lspr->splgt_flags & SPLGT_corona) )
    {
        // Sprite has a corona, and coronas are enabled.
        float cx=0.0f, cy=0.0f, cz=0.0f; // gravity center

        // Each of these types (flag combinations) has a corona.
        switch (p_lspr->splgt_flags)
        {
            case SPLGT_light:
                // dimming with distance
                // d'ou vienne ces constante ?
                size  = p_lspr->corona_radius  * ((outVerts[0].z+120.0f)/950.0f);
                break;
            case SPLGT_rocket:
                p_lspr->corona_color = (((M_Random()>>1)&0xff)<<24)|0x0040ff;
                // don't need a break, has corona too
            case SPLGT_corona:
                // d'ou vienne ces constante ?
                size  = p_lspr->corona_radius  * ((outVerts[0].z+60.0f)/100.0f);
                break;
            default:
                I_SoftError("HWR_DoCoronasLighting: unknown light type %x", p_lspr->splgt_flags);
                return;
        }
        if (size > p_lspr->corona_radius) 
            size = p_lspr->corona_radius;
        size *= FIXED_TO_FLOAT( cv_grcoronasize.value<<1 );

#if 1
        // compute position doing average
        cx = (outVerts[0].x + outVerts[2].x) / 2.0;
        cy = (outVerts[0].y + outVerts[2].y) / 2.0;
        cz = (outVerts[0].z + outVerts[2].z) / 2.0;
#else
        // compute position doing average
        int i;
        for (i=0; i<4; i++) {
            cx += outVerts[i].x;
            cy += outVerts[i].y;
            cz += outVerts[i].z;
        }
        cx /= 4.0f;  cy /= 4.0f;  cz /= 4.0f;
#endif

        // more realistique corona !
        if( cz>=255*8+250 )
            return;
        Surf.FlatColor.rgba = p_lspr->corona_color;
        Surf.FlatColor.s.alpha = ( cz>250.0f )? 0xff-((int)cz-250)/8 : 0xff;

        // put light little forward of the sprite so there is no 
        // z-blocking or z-fighting
        if( cz > 0.5f )  // correction for side drift due to cz change
        {  // -0.75 per unit of cz
           cx += cx * ((-6.0f) / cz);
           cy += cy * ((-6.0f) / cz);
        }
        // need larger value to avoid z-blocking when look down
        cz -= 8.0f;  // larger causes more side-to-side drift

        // Bp; je comprend pas, ou est la rotation haut/bas ?
        //     tu ajoute un offset a y mais si la tu la reguarde de haut 
        //     sa devrais pas marcher ... comprend pas :(
        //     (...) bon je croit que j'ai comprit il est tout pourit le code ?
        //           car comme l'offset est minime sa ce voit pas !
        cy += p_lspr->light_yoffset;
        light[0].x = light[3].x = cx - size;
        light[1].x = light[2].x = cx + size;
        light[0].y = light[1].y = cy - (size*1.33f); 
        light[2].y = light[3].y = cy + (size*1.33f); 
        light[0].z = light[1].z = light[2].z = light[3].z = cz;
        light[0].sow = 0.0f;   light[0].tow = 0.0f;
        light[1].sow = 1.0f;   light[1].tow = 0.0f;
        light[2].sow = 1.0f;   light[2].tow = 1.0f;
        light[3].sow = 0.0f;   light[3].tow = 1.0f;

        HWR_GetPic(corona_lumpnum);  // TODO: use different coronas

        HWD.pfnDrawPolygon ( &Surf, light, 4, PF_Modulated | PF_Additive | PF_Clip | PF_Corona | PF_NoDepthTest);
    }
}
#endif

#ifdef DYLT_CORONAS
// Draw coronas from dynamic light list
void HWR_DrawCoronas( void )
{
    int j;
    float           size;
    FSurfaceInfo_t  Surf;
    vxtx3d_t        light[4];
    v3d_t *         light_pos;
    float           cx, cy, cz;
    light_t         * p_lspr;

    if( !cv_grcoronas.value || dynlights->nb<=0)
        return;
    
    HWR_GetPic(corona_lumpnum);  // TODO: use different coronas
    for( j=0;j<dynlights->nb;j++ )
    {
        light_pos = & dynlights->position[j];
        cx=light_pos->x;
        cy=light_pos->y;
        cz=light_pos->z; // gravity center
        p_lspr = dynlights->p_lspr[j];
        
        // it's an object which emits light
        if ( !(p_lspr->splgt_flags & SPLGT_corona) )
            continue;

        transform_world_to_gr(&cx,&cy,&cz);

        // more realistique corona !
        if( cz>=255*8+250 )
            continue;

        Surf.FlatColor.rgba = p_lspr->corona_color;
        if( cz>250.0f )
            Surf.FlatColor.s.alpha = 0xff-((int)cz-250)/8;
        else
            Surf.FlatColor.s.alpha = 0xff;

        switch (p_lspr->splgt_flags)
        {
            case SPLGT_light:
                size  = p_lspr->corona_radius  * ((cz+120.0f)/950.0f); // d'ou vienne ces constante ?
                break;
            case SPLGT_rocket:
                Surf.FlatColor.s.alpha = (M_Random()>>1)&0xff;
                // don't need a break
            case SPLGT_corona:
                size  = p_lspr->corona_radius  * ((cz+60.0f)/100.0f); // d'ou vienne ces constante ?
                break;
            default:
                I_SoftError("HWR_DoCoronasLighting: unknown light type %d",p_lspr->splgt_flags);
                continue;
        }
        if (size > p_lspr->corona_radius)
            size = p_lspr->corona_radius;
        size *= FIXED_TO_FLOAT( cv_grcoronasize.value<<1 );

        // put light little forward the sprite so there is no 
        // z-buffer problem (coplanaire polygons)
        // BP: use PF_Decal do not help :(
        if( cz > 0.5f )  // correction for side drift due to cz change
        {
           cx += cx * ((-3.8f) / cz);
           cy += cy * ((-3.8f) / cz);
        }
        cz = cz - 5.0f; 

        light[0].x = light[3].x = cx - size;
        light[1].x = light[2].x = cx + size;
        light[0].y = light[1].y = cy - (size*1.33f); 
        light[2].y = light[3].y = cy + (size*1.33f); 
        light[0].z = light[1].z = light[2].z = light[3].z = cz;
        light[0].sow = 0.0f;   light[0].tow = 0.0f;
        light[1].sow = 1.0f;   light[1].tow = 0.0f;
        light[2].sow = 1.0f;   light[2].tow = 1.0f;
        light[3].sow = 0.0f;   light[3].tow = 1.0f;
        light[3].sow = 0.0f;   light[3].tow = 1.0f;

        HWD.pfnDrawPolygon ( &Surf, light, 4, PF_Modulated | PF_Additive | PF_Clip | PF_NoDepthTest | PF_Corona );
    }
}
#endif

// --------------------------------------------------------------------------
// Remove all the dynamic lights at each frame
// --------------------------------------------------------------------------
// Called from P_SetupLevel, and maybe from HWR_RenderPlayerView
void HWR_Reset_Lights(void)
{
    dynlights->nb = 0;
}

// --------------------------------------------------------------------------
// Change view, thus change lights (splitscreen)
// --------------------------------------------------------------------------
// Called from HWR_RenderPlayerView
void HWR_Set_Lights(int viewnumber)
{
    dynlights = &view_dynlights[viewnumber];
}

// --------------------------------------------------------------------------
// Add a light for dynamic lighting
// The light position is already transformed except for mlook
// --------------------------------------------------------------------------
void HWR_DL_AddLightSprite(gr_vissprite_t *spr, MipPatch_t *mpatch)
{
    v3d_t *    light_pos;
    light_t *  p_lspr;

    //Hurdler: moved here because it's better ;-)
    if (!cv_grdynamiclighting.value)
        return;

#ifdef PARANOIA
    if(!spr->mobj)
    {
        I_SoftError("AddLight: vissprite without mobj !!!");
        return;
    }
#endif
    // check if sprite contain dynamic light
    p_lspr = t_lspr[spr->mobj->sprite];
    //CONS_Printf("sprite (sprite): %d (%s): %d\n", spr->mobj->sprite, sprnames[spr->mobj->sprite], p_lspr->splgt_flags);
    if ( (p_lspr->splgt_flags & SPLGT_dynamic)
         && ((p_lspr->splgt_flags!=SPLGT_light) || cv_grstaticlighting.value) 
         && (dynlights->nb < DL_MAX_LIGHT) 
         && spr->mobj->state )
    {
        // Create a dynamic light.
        // Dynamic light position.
        light_pos = & dynlights->position[ dynlights->nb ];
        light_pos->x = FIXED_TO_FLOAT( spr->mobj->x );
        light_pos->y = FIXED_TO_FLOAT( spr->mobj->z )
	 + FIXED_TO_FLOAT( spr->mobj->height>>1 ) + p_lspr->light_yoffset;
        light_pos->z = FIXED_TO_FLOAT( spr->mobj->y );

        dynlights->mo[dynlights->nb] = spr->mobj;
        if( (spr->mobj->state>=&states[S_EXPLODE1]
             && spr->mobj->state<=&states[S_EXPLODE3])
         || (spr->mobj->state>=&states[S_FATSHOTX1]
             && spr->mobj->state<=&states[S_FATSHOTX3]))
        {
            p_lspr = &lspr[ROCKETEXP_L];
        }

        dynlights->p_lspr[dynlights->nb] = p_lspr;
        
        dynlights->nb++;
    } 
}

static MipPatch_t lightmappatch;

void HWR_Init_Light( void )
{
    int i;

    // precalculate sqr radius
    for(i=0;i<NUMLIGHTS;i++)
        lspr[i].dynamic_sqrradius = lspr[i].dynamic_radius*lspr[i].dynamic_radius;

    lightmappatch.mipmap.downloaded = false;
    corona_lumpnum = W_GetNumForName("corona");
}

// -----------------+
// HWR_SetLight     : Download a disc shaped alpha map for rendering fake lights
// -----------------+
void HWR_SetLight( void )
{
    int    i, j;

    if (!lightmappatch.mipmap.downloaded && !lightmappatch.mipmap.grInfo.data)
    {

        uint16_t *Data = Z_Malloc( 128*128*sizeof(uint16_t), PU_HWRCACHE, &lightmappatch.mipmap.grInfo.data );
                
        for( i=0; i<128; i++ )
        {
            for( j=0; j<128; j++ )
            {
                int pos = ((i-64)*(i-64))+((j-64)*(j-64));
                if (pos <= 63*63)
                    Data[i*128+j] = ((byte)(255-(4*sqrt(pos)))) << 8 | 0xff;
                else
                    Data[i*128+j] = 0;
            }
        }
        lightmappatch.mipmap.grInfo.format = GR_TEXFMT_ALPHA_INTENSITY_88;

        lightmappatch.width = 128;
        lightmappatch.height = 128;
        lightmappatch.mipmap.width = 128;
        lightmappatch.mipmap.height = 128;
        lightmappatch.mipmap.grInfo.smallLodLog2 = GR_LOD_LOG2_128;
        lightmappatch.mipmap.grInfo.largeLodLog2 = GR_LOD_LOG2_128;
        lightmappatch.mipmap.grInfo.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
        lightmappatch.mipmap.tfflags = 0; //TF_WRAPXY; // DEBUG: view the overdraw !
    }
    HWD.pfnSetTexture( &lightmappatch.mipmap );
}


void HWR_DynamicShadowing(vxtx3d_t *clVerts, int nrClipVerts, player_t *p)
{
    int  i;
    FSurfaceInfo_t  Surf;

    if (!cv_grdynamiclighting.value)
        return;

    for (i=0; i<nrClipVerts; i++) {
        clVerts[i].sow = 0.5f + clVerts[i].x*0.01f;
        clVerts[i].tow = 0.5f + clVerts[i].z*0.01f*1.2f;
    }
    
    HWR_SetLight();

    Surf.FlatColor.rgba = 0x70707070;

    HWD.pfnDrawPolygon ( &Surf, clVerts, nrClipVerts, LIGHTMAPFLAGS );
    //HWD.pfnDrawPolygon ( &Surf, clVerts, nrClipVerts, PF_Modulated|PF_Environment|PF_Clip );
}


#ifdef STATICLIGHTMAPS
//**********************************************************
// Hurdler: new code for faster static lighting and and T&L
//**********************************************************

// est ce bien necessaire ?
//static sector_t *gr_frontsector;
static sector_t *gr_backsector ;
static seg_t    *gr_curline;



/*
static void HWR_StoreWallRange (int startfrac, int endfrac)
{
...(voir hw_main.c)...
}
*/


// p1 et p2 c'est le deux bou du seg en float
static
void HWR_Create_WallLightmaps(v3d_t *p1, v3d_t *p2, int lightnum, seg_t *line)
{
    lightmap_t *lp;

    // (...) calcul presit de la projection et de la distance

//    if (dist_p2d >= dynamic_light->p_lspr[lightnum]->dynamic_sqrradius)
//        return;

    // (...) attention faire le backfase cull histoir de faire mieux que Q3 !

    lp = malloc(sizeof(lightmap_t));
    lp->next = line->lightmaps;
    line->lightmaps = lp;
    
    // (...) encore des b� calcul bien lourd et on stock tout sa dans la lightmap
}

static void HWR_AddLightMapForLine( int lightnum, seg_t *line)
{
    /*
    int                 x1;
    int                 x2;
    angle_t             angle1;
    angle_t             angle2;
    angle_t             span;
    angle_t             tspan;
    */
    v3d_t  p1,p2;
    
    gr_curline = line;
    gr_backsector = line->backsector;
    
    // Reject empty lines used for triggers and special events.
    // Identical floor and ceiling on both sides,
    //  identical light levels on both sides,
    //  and no middle texture.
/*
    if (   gr_backsector->ceilingpic == gr_frontsector->ceilingpic
        && gr_backsector->floorpic == gr_frontsector->floorpic
        && gr_backsector->lightlevel == gr_frontsector->lightlevel
        && gr_curline->sidedef->midtexture == 0)
    {
        return;
    }
*/

    p1.y=FIXED_TO_FLOAT( gr_curline->v1->y );
    p1.x=FIXED_TO_FLOAT( gr_curline->v1->x );
    p2.y=FIXED_TO_FLOAT( gr_curline->v2->y );
    p2.x=FIXED_TO_FLOAT( gr_curline->v2->x );

#if 0   
    // check bbox of the seg
    if( CircleTouchBBox(&p1, &p2,
			&dynlights->position[lightnum],
			dynlights->p_lspr[lightnum]->dynamic_radius )
	==false )    
        return;
#endif

    HWR_Create_WallLightmaps(&p1, &p2, lightnum, line);
}


//TODO: see what HWR_AddLine does
static void HWR_CheckSubsector( int num, fixed_t *bbox )
{
    int         count;
    seg_t       *line;
    subsector_t *sub;
    v3d_t       p1,p2;
    int         lightnum;

    p1.y=FIXED_TO_FLOAT( bbox[BOXTOP] );
    p1.x=FIXED_TO_FLOAT( bbox[BOXLEFT] );
    p2.y=FIXED_TO_FLOAT( bbox[BOXBOTTOM] );
    p2.x=FIXED_TO_FLOAT( bbox[BOXRIGHT] );


    if (num < numsubsectors)
    {
        sub = &subsectors[num];         // subsector
        for(lightnum=0; lightnum<dynlights->nb; lightnum++)
        {
#if 0   
            // check bbox of the seg
            if( CircleTouchBBox(&p1, &p2,
                &dynlights->position[lightnum],
                dynlights->p_lspr[lightnum]->dynamic_radius )
                ==false )    
                continue;
#endif

            count = sub->numlines;          // how many linedefs
            line = &segs[sub->firstline];   // first line seg
            while (count--)
            {
                HWR_AddLightMapForLine (lightnum, line);       // compute lightmap
                line++;
            }
        }
    }
}


// --------------------------------------------------------------------------
// Hurdler: this adds lights by mobj.
// --------------------------------------------------------------------------
static void HWR_AddMobjLights(mobj_t *thing)
{
    if ( t_lspr[thing->sprite]->splgt_flags & SPLGT_corona )
    {
        // Sprite has a corona.
	// Create a corona dynamic light.
        v3d_t * light_pos = & dynlights->position[ dynlights->nb ];
        light_pos->x = FIXED_TO_FLOAT( thing->x );
        light_pos->y = FIXED_TO_FLOAT( thing->z ) + t_lspr[thing->sprite]->light_yoffset;
        light_pos->z = FIXED_TO_FLOAT( thing->y );
        
        dynlights->p_lspr[dynlights->nb] = t_lspr[thing->sprite];
        
        dynlights->nb++;
        if (dynlights->nb>DL_MAX_LIGHT)
            dynlights->nb = DL_MAX_LIGHT;
    }
}

//Hurdler: The goal of this function is to walk through all the bsp starting
//         on the top. 
//         We need to do that to know all the lights in the map and all the walls
static void HWR_ComputeLightMapsInBSPNode(int bspnum, fixed_t *bbox)
{
    if (bspnum & NF_SUBSECTOR) // Found a subsector?
    {
        if (bspnum == -1)
            HWR_CheckSubsector(0, bbox);  // probably unecessary: see boris' comment in hw_bsp
        else
            HWR_CheckSubsector(bspnum&(~NF_SUBSECTOR), bbox);
        return;
    }
    HWR_ComputeLightMapsInBSPNode(nodes[bspnum].children[0], nodes[bspnum].bbox[0]);
    HWR_ComputeLightMapsInBSPNode(nodes[bspnum].children[1], nodes[bspnum].bbox[1]);
}

static void HWR_SearchLightsInMobjs(void)
{
    thinker_t*          th;
    //mobj_t*             mobj;

    // search in the list of thinkers
    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
        // a mobj ?
        if (th->function.acp1 == (actionf_p1)P_MobjThinker)
            HWR_AddMobjLights((mobj_t *)th);
    }
}
#endif

//
// HWR_Create_StaticLightmaps()
//
// Called from P_SetupLevel
void HWR_Create_StaticLightmaps( void )
{
#ifdef STATICLIGHTMAPS
    //Hurdler: TODO!
    CONS_Printf("HWR_CreateStaticLightmaps\n");

    dynlights->nb = 0;

    // First: Searching for lights
    // BP: if i was you, I will make it in create mobj since mobj can be create 
    //     at runtime now with fragle scipt
    HWR_SearchLightsInMobjs();
    CONS_Printf("%d lights found\n", dynlights->nb);

    // Second: Build all lightmap for walls covered by lights
    validcount++; // to be sure
    HWR_ComputeLightMapsInBSPNode( numnodes-1, NULL);

    dynlights->nb = 0;
#endif
    return;
}

/*
TODO:

  - Les coronas ne sont pas g�rer avec le nouveau systeme, seul le dynamic lighting l'est
  - calculer l'offset des coronas au chargement du level et non faire la moyenne
    au moment de l'afficher
     BP: euh non en fait il faux encoder la position de la light dans le sprite
         car c'est pas focement au mileux de plus il peut en y avoir plusieur (chandelier)
  - changer la comparaison pour l'affichage des coronas (+ un epsilon)
    BP: non non j'ai trouver mieux :) : lord du AddSprite tu rajoute aussi la coronas
        dans la sprite list ! avec un z de epsilon (attention au ZCLIP_PLANE) et donc on 
        l'affiche en dernier histoir qu'il puisse etre cacher par d'autre sprite :)
        Bon fait metre pas mal de code special dans hwr_project sprite mais sa vaux le 
        coup
  - gerer dynamic et static : retenir le nombre de lightstatic et clearer toute les 
        light>lightstatic (les dynamique) et les lightmap correspondant dans les segs
        puit refaire une passe avec le code si dessus mais rien que pour les dynamiques
        (tres petite modification)
  - finalement virer le hack splitscreen, il n'est plus necessaire !
*/
