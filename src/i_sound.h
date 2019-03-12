// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_sound.h 1417 2019-01-29 08:00:14Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2011 by DooM Legacy Team.
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
// $Log: i_sound.h,v $
// Revision 1.9  2003/07/13 13:16:15  hurdler
//
// Revision 1.8  2002/12/13 22:34:27  ssntails
// MP3/OGG support!
//
// Revision 1.7  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.6  2001/02/24 13:35:20  bpereira
//
// Revision 1.5  2000/09/10 10:42:33  metzgermeister
// fixed qmus2mid SDL
//
// Revision 1.4  2000/04/19 15:21:02  hurdler
// add SDL midi support
//
// Revision 1.3  2000/03/22 18:49:38  metzgermeister
// added I_PauseCD() for Linux
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      System interface, sound.
//
//-----------------------------------------------------------------------------

#ifndef I_SOUND_H
#define I_SOUND_H

#include "doomdef.h"
  // MACOS_DI
#include "doomtype.h"
  // boolean
#include "sounds.h"
#include "command.h"
  // consvar_t


void  I_GetSfx (sfxinfo_t*  sfx);  // read lump to sfx data, length
void  I_FreeSfx (sfxinfo_t* sfx);

// The volumes for the hardware and software mixers.
extern int mix_sfxvolume;
extern int mix_musicvolume;


// Init at program start...
void I_StartupSound();

// ... update sound buffer and audio device at runtime...
void I_UpdateSound(void);
void I_SubmitSound(void);

// ... shut down and relase at program termination.
void I_ShutdownSound(void);


//
//  SFX I/O
//

// Starts a sound in a particular sound channel.
//  vol : 0..255
int I_StartSound ( sfxid_t sfxid, int vol, int sep, int pitch, int priority );


// Stops a sound channel.
void I_StopSound(int handle);

// Called by S_*() functions
//  to see if a channel is still playing.
// Returns 0 if no longer playing, 1 if playing.
int I_SoundIsPlaying(int handle);

// Updates the volume, separation,
//  and pitch of a sound channel.
void I_UpdateSoundParams ( int handle, int vol, int sep, int pitch );


//
//  MUSIC I/O
//
// Volume.
void I_SetMusicVolume(int volume);
void I_SetMusicOption(void);
void I_SetSfxVolume(int volume);
// PAUSE game handling.
void I_PauseSong(int handle);
void I_ResumeSong(int handle);

#ifdef MUSSERV
// Information for ports with music servers.
//  name : name of song
//  lumpnum : lumpnum of the song data
int I_PlayServerSong( char * name, lumpnum_t lumpnum, byte looping );
#else
// Registers a song handle to song data.
//  data : ptr to lump data
//  len : length of data
int I_RegisterSong( void* data, int len );
#endif
// Called by anything that wishes to start music.
//  plays a song, and when the song is done,
//  starts playing it again in an endless loop.
// Horrible thing to do, considering.
void I_PlaySong ( int handle, byte looping );
// Stops a song over 3 seconds.
void I_StopSong(int handle);
// See above (register), then think backwards
void I_UnRegisterSong(int handle);

#ifdef MACOS_DI
// in macos directory
void MusicEvents (void);        //needed to give quicktime some processor

#define PLAYLIST_LENGTH 10
extern consvar_t user_songs[PLAYLIST_LENGTH];
#endif


// cd music interface

void   I_InitCD (void);
void   I_PauseCD (void);
void   I_ResumeCD (void);
void   I_UpdateCD (void);
void   I_PlayCD (unsigned int track, boolean looping);
void   I_ShutdownCD (void);

#endif
