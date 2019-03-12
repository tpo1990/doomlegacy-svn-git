// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_cdmus.c 1035 2013-08-14 00:38:40Z wesleyjohnson $
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
// Revision 1.1  2001/04/17 22:23:38  calumr
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//
// DESCRIPTION:
//      cd music interface
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
  // stdlib, stdint, string, defines

#include <stdlib.h>

#include "i_sound.h"
#include "command.h"
#include "m_argv.h"

#define MAX_CD_TRACKS 256

static boolean cdValid = false;
static boolean playing = false;
static boolean wasPlaying = false;
static boolean initialized = false;
static boolean enabled = false;
static boolean playLooping = false;
static byte    playTrack;
static byte    maxTrack;
static byte    cdRemap[MAX_CD_TRACKS];
static int     cdvolume = -1;

CV_PossibleValue_t cd_volume_cons_t[]={{0,"MIN"},{31,"MAX"},{0,NULL}};

consvar_t cd_volume = {"cd_volume","31",CV_SAVE, cd_volume_cons_t};
consvar_t cdUpdate  = {"cd_update","1",CV_SAVE}; // FIXME: use this??
consvar_t cv_jigglecdvol = {"jigglecdvolume", "0", CV_SAVE}; // only for compatibility reason with Linux version

static int CDAudio_GetAudioDiskInfo(void)
{
    return 0;
}


static void I_EjectCD(void)
{
    return;
}

static void Command_Cd_f (void)
{
    char	*command;
    int		ret;
    int		n;

    if (!initialized)
	return;

    if (COM_Argc() < 2) {
	CONS_Printf ("cd [on] [off] [remap] [reset] [open]\n"
		     "   [info] [play <track>] [resume]\n"
		     "   [stop] [pause] [loop <track>]\n");
	return;
    }

    command = COM_Argv (1);

    if (!strncmp(command, "on", 2)) {
	enabled = true;
	return;
    }

    if (!strncmp(command, "off", 3)) {
	I_StopCD();
	enabled = false;
	return;
    }
	
    if (!strncmp(command, "remap", 5)) {
	ret = COM_Argc() - 2;
	if (ret <= 0) {
	    for (n = 1; n < MAX_CD_TRACKS; n++)
		if (cdRemap[n] != n)
		    CONS_Printf("  %u -> %u\n", n, cdRemap[n]);
	    return;
	}
	for (n = 1; n <= ret; n++)
	    cdRemap[n] = atoi(COM_Argv (n+1));
	return;
    }
        
    if (!strncmp(command, "reset", 5)) {
	enabled = true;
	I_StopCD();
            
	for (n = 0; n < MAX_CD_TRACKS; n++)
	    cdRemap[n] = n;
	CDAudio_GetAudioDiskInfo();
	return;
    }
        
    if (!cdValid) {
	CDAudio_GetAudioDiskInfo();
	if (!cdValid) {
	    CONS_Printf("No CD in player.\n");
	    return;
	}
    }

    if (!strncmp(command, "open", 4)) {
	I_EjectCD();
	cdValid = false;
	return;
    }

    if (!strncmp(command, "info", 4)) {
	CONS_Printf("%u tracks\n", maxTrack);
	if (playing)
	    CONS_Printf("Currently %s track %u\n", playLooping ? "looping" : "playing", playTrack);
	else if (wasPlaying)
	    CONS_Printf("Paused %s track %u\n", playLooping ? "looping" : "playing", playTrack);
	CONS_Printf("Volume is %d\n", cdvolume);
	return;
    }

    if (!strncmp(command, "play", 4)) {
	I_PlayCD((byte)atoi(COM_Argv (2)), false);
	return;
    }

    if (!strncmp(command, "loop", 4)) {
	I_PlayCD((byte)atoi(COM_Argv (2)), true);
	return;
    }

    if (!strncmp(command, "stop", 4)) {
	I_StopCD();
	return;
    }
        
    if (!strncmp(command, "pause", 5)) {
	I_PauseCD();
	return;
    }
        
    if (!strncmp(command, "resume", 6)) {
	I_ResumeCD();
	return;
    }
        
    CONS_Printf("Invalid command \"cd %s\"\n", COM_Argv (1));
}

void I_StopCD(void)
{

    
    if (!(playing || wasPlaying))
	return;
    
    
    wasPlaying = false;
    playing = false;
}

int I_PauseCD (void)
{
    
    if (!playing)
	return;
    
    wasPlaying = playing;
    playing = false;
}

// continue after a pause
void I_ResumeCD (void)
{

    
    if (!cdValid)
	return;
    
    if (!wasPlaying)
	return;
	
    if(cd_volume.value == 0)
	return;
    
    playing = true;
    wasPlaying = false;
 
    return;
}


void I_ShutdownCD (void)
{
    if (!initialized)
	return;

    I_StopCD();

    initialized = false;
    enabled = false;
}

void I_InitCD (void)
{
    int i;
    const char *cdName;
    
    // Don't start music on a dedicated server
    if (M_CheckParm("-dedicated"))
	return ;
    
    // Has been checked in d_main.c, but doesn't hurt here
    if (M_CheckParm ("-nocd"))
	return ;
    
    
    
    
    for (i = 0; i < MAX_CD_TRACKS; i++)
	cdRemap[i] = i;
    
    initialized = true;
    enabled = true;

    if (CDAudio_GetAudioDiskInfo()) {
	CONS_Printf("I_InitCD: No CD in player.\n");
	cdValid = false;
    }

    COM_AddCommand ("cd", Command_Cd_f);
    
    CONS_Printf("CD Audio Initialized\n");
    
    return ;
}



//
void I_UpdateCD (void)
{
    if (!enabled)
	return;
    
    I_SetVolumeCD(cd_volume.value);
	
    
    return;
}



// play the cd
void I_PlayCD (int track, boolean looping)
{

    
    if (!cdValid)
    {
	CDAudio_GetAudioDiskInfo();
	if (!cdValid)
	    return;
    }
    
    track = cdRemap[track];
    
    if (track < 1 || track > maxTrack)
    {
	CONS_Printf("I_PlayCD: Bad track number %u.\n", track);
	return;
    }
	
    if (playing)
    {
	if (playTrack == track)
	    return;
	I_StopCD();
    }
    
    playLooping = looping;
    playTrack = track;
    playing = true;

    if(cd_volume.value == 0)
    {
	I_PauseCD();
    }
    
}


// SDL does not support setting the CD volume
// use pause instead and toggle between full and no music
int I_SetVolumeCD (int volume)
{
    if(volume != cdvolume)
    {
	if(volume > 0 && volume < 16)
	{
	    CV_SetValue(&cd_volume, 31);
	    cdvolume = 31;
	    
	    I_ResumeCD();
	}
	else if(volume > 15 && volume < 31)
	{
	    CV_SetValue(&cd_volume, 0);
	    cdvolume = 0;
	    
	    I_PauseCD();
	}
    }
    
    return 0;
}
