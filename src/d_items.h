// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: d_items.h 1347 2017-07-29 18:21:14Z wesleyjohnson $
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
// $Log: d_items.h,v $
// Revision 1.8  2004/07/27 08:19:34  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.7  2003/08/11 13:50:02  hurdler
// go final + translucent HUD + fix spawn in net game
//
// Revision 1.6  2003/07/21 11:33:57  hurdler
//
// Revision 1.5  2003/06/11 03:38:09  ssntails
// THING Z definable in levels by using upper 9 bits
//
// Revision 1.4  2002/07/24 19:03:09  ssntails
// Added support for things to retain spawned Z position.
//
// Revision 1.3  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Items: key cards, artifacts, weapon, ammunition.
//
//-----------------------------------------------------------------------------

#ifndef D_ITEMS_H
#define D_ITEMS_H

#include "doomdef.h"
  // TICRATE
#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif


// ==================================
// Difficulty/skill settings/filters.
// ==================================

// MTF_ are flags in mthing->options  (16 bit field)
typedef enum
{
// As defined by Doom and Boom specs.
// Skill flags.
  MTF_EASY        = 0x0001,
  MTF_NORMAL      = 0x0002,
  MTF_HARD        = 0x0004,

// Deaf monsters/do not react to sound.
  MTF_AMBUSH      = 0x0008,

// 24/5/04: Exl: werent defined before
  MTF_MPSPAWN     = 0x0010,  // Spawn in multiplayer
  MTF_NODM        = 0x0020,  // Do not spawn in deathmatch mode
  MTF_NOCOOP      = 0x0040,  // Do not spawn in coop mode

// MBF
  MTF_FRIEND      = 0x0080,  // Friend to player
  MTF_RESERVED    = 0x0100,  // Reserved (identify bad editor)

// DoomLegacy additions
//Hurdler: special option to tell the things has been spawned by an FS
  MTF_FS_SPAWNED  = 0x1000,  // fragglescript spawned
  MTF_EXTRA       = 0x8000   // extra mapthing
} mapthing_options_e;


// heretic stuff
#define AMMO_GWND_WIMPY 10
#define AMMO_GWND_HEFTY 50
#define AMMO_CBOW_WIMPY 5
#define AMMO_CBOW_HEFTY 20
#define AMMO_BLSR_WIMPY 10
#define AMMO_BLSR_HEFTY 25
#define AMMO_SKRD_WIMPY 20
#define AMMO_SKRD_HEFTY 100
#define AMMO_PHRD_WIMPY 1
#define AMMO_PHRD_HEFTY 10
#define AMMO_MACE_WIMPY 20
#define AMMO_MACE_HEFTY 100

#define USE_GWND_AMMO_1 1
#define USE_GWND_AMMO_2 1
#define USE_CBOW_AMMO_1 1
#define USE_CBOW_AMMO_2 1
#define USE_BLSR_AMMO_1 1
#define USE_BLSR_AMMO_2 5
#define USE_SKRD_AMMO_1 1
#define USE_SKRD_AMMO_2 5
#define USE_PHRD_AMMO_1 1
#define USE_PHRD_AMMO_2 1
#define USE_MACE_AMMO_1 1
#define USE_MACE_AMMO_2 5

//
// Key cards.
//
typedef enum
{
    it_bluecard   =    1,
    it_yellowcard =    2,
    it_redcard    =    4,
    it_blueskull  =    8,
    it_yellowskull= 0x10,
    it_redskull   = 0x20,
    it_allkeys    = 0x3f,
    NUMCARDS      = 6

} card_t;

typedef enum
{
        arti_none,
        arti_invulnerability,
        arti_invisibility,
        arti_health,
        arti_superhealth,
        arti_tomeofpower,
        arti_torch,
        arti_firebomb,
        arti_egg,
        arti_fly,
        arti_teleport,
        NUMARTIFACTS
} artitype_t;

#define NUMINVENTORYSLOTS  14
#define MAXARTECONT        16 
typedef struct
{
    byte type;
    byte count;
} inventory_t;

// Power up artifacts.
typedef enum
{
    pw_invulnerability,
    pw_strength,
    pw_invisibility,
    pw_ironfeet,
    pw_allmap,
    pw_infrared,

    // heretic
    pw_weaponlevel2,
    pw_flight,

    NUMPOWERS

} powertype_t;

//
// Power up durations,
//  how many seconds till expiration,
//  assuming TICRATE is 35 ticks/second.
//
typedef enum
{
    INVULNTICS  = (30*TICRATE),
    INVISTICS   = (60*TICRATE),
    INFRATICS   = (120*TICRATE),
    IRONTICS    = (60*TICRATE)

} powerduration_t;


// The defined weapons,
//  including a marker indicating
//  user has not changed weapon.
typedef enum
{
    wp_fist,
    wp_pistol,
    wp_shotgun,
    wp_chaingun,
    wp_missile,
    wp_plasma,
    wp_bfg,
    wp_chainsaw,
    wp_supershotgun,

    // heretic stuff
    wp_staff=wp_fist,
    wp_goldwand,
    wp_crossbow,
    wp_blaster,
    wp_skullrod,
    wp_phoenixrod,
    wp_mace,
    wp_gauntlets,
    wp_beak,

    NUMWEAPONS,

    // No pending weapon change.
    wp_nochange

} weapontype_t;


// Ammunition types defined.
typedef enum
{
    am_clip,    // Pistol / chaingun ammo.
    am_shell,   // Shotgun / double barreled shotgun.
    am_cell,    // Plasma rifle, BFG.
    am_misl,    // Missile launcher.

    // heretic stuff
    am_goldwand = am_clip,
    am_crossbow,
    am_blaster,
    am_skullrod,
    am_phoenixrod,
    am_mace,

    NUMAMMO,
    am_noammo   // Unlimited for chainsaw / fist.

} ammotype_t;


// Weapon info: sprite frames, ammunition use.
typedef struct
{
    ammotype_t  ammo;
    int         ammopershoot;
    statenum_t  upstate;
    statenum_t  downstate;
    statenum_t  readystate;
    statenum_t  atkstate;
    statenum_t  holdatkstate;
    statenum_t  flashstate;

} weaponinfo_t;

extern weaponinfo_t doomweaponinfo[NUMWEAPONS];
extern weaponinfo_t wpnlev1info[NUMWEAPONS];
extern weaponinfo_t wpnlev2info[NUMWEAPONS];

#endif
