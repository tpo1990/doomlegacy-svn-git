// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: hw3sound.h 1361 2017-10-16 16:26:45Z wesleyjohnson $
//
// Copyright (C) 2001-2016 by DooM Legacy Team.
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
// $Log: hw3sound.h,v $
// Revision 1.4  2003/01/19 21:24:26  bock
// Make sources buildable on FreeBSD 5-CURRENT.
//
// Revision 1.3  2002/01/21 23:27:06  judgecutor
// Added HW3S_I_StartSound low-level fuction for arbitrary managing of 3D sources
//
// Revision 1.2  2001/05/27 13:42:48  bpereira
// Revision 1.1  2001/04/04 19:41:16  judgecutor
// Initial release of 3D Sound Support
//
//
// DESCRIPTION:
//      High-level functions of hardware 3D sound
//
//-----------------------------------------------------------------------------

#ifndef HW3_SOUND_H
#define HW3_SOUND_H

#ifdef HW3SOUND
#include "hw3dsdrv.h"
//#include "s_sound.h"
//#include "p_mobj.h"

// Default sound mode (original stereo mode)
enum { HWS_DEFAULT_MODE = 0 };

extern int  hws_mode;           // Current sound mode

extern int  HW3S_Init(I_Error_t FatalErrorFunction, snddev_t *snd_dev);
extern void HW3S_Shutdown(void);
extern int  HW3S_GetVersion(void);


// Starts 3D sound with specified parameters
// origin      - for object movement tracking. NULL are possible value (no movement tracking)
// source_parm - 3D source parameters (position etc.)
// cone_parm   - 3D source cone parameters. NULL are possible value (default cone will be used)
// channel     - 
// sfx_id      - sfx id
// priority    - -10..2560, ascending priority, Heretic style
// vol         - sound volume
// pitch       - sound pitching value
// Returns:    - sound id
extern int HW3S_I_StartSound(const void *origin, source3D_data_t *source_parm, channel_type_t channel, sfxid_t sfx_id,
                             int16_t priority, int vol, int pitch, int sep);

extern void HW3S_FillSourceParameters(mobj_t *origin, source3D_data_t *data, channel_type_t   c_type);

extern void HW3S_StopSound(void *origin);
extern void HW3S_Stop_LevelSound(void);

extern void HW3S_BeginFrameUpdate();
extern void HW3S_EndFrameUpdate();
//extern void HW3S_UpdateListener(mobj_t *listener);
extern void HW3S_UpdateSources(void);

extern void HW3S_SetSfxVolume(int volume);

// Utility functions
extern int  HW3S_SoundIsPlaying(int handle);
extern void HW3S_SetSourcesNum(void);
extern int  HW3S_SoundPlaying(void *origin, int id);

#endif // HW3SOUND


#endif // HW3_SOUND_H

