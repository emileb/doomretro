/*
========================================================================

                           D O O M  R e t r o
         The classic, refined DOOM source port. For Windows PC.

========================================================================

  Copyright © 1993-2012 id Software LLC, a ZeniMax Media company.
  Copyright © 2013-2017 Brad Harding.

  DOOM Retro is a fork of Chocolate DOOM.
  For a list of credits, see <http://wiki.doomretro.com/credits>.

  This file is part of DOOM Retro.

  DOOM Retro is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation, either version 3 of the License, or (at your
  option) any later version.

  DOOM Retro is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with DOOM Retro. If not, see <https://www.gnu.org/licenses/>.

  DOOM is a registered trademark of id Software LLC, a ZeniMax Media
  company, in the US and/or other countries and is used without
  permission. All other trademarks are the property of their respective
  holders. DOOM Retro is in no way affiliated with nor endorsed by
  id Software.

========================================================================
*/

// Eternity Engine's Client Interface to RPC Midi Server by James Haley

#if !defined(__I_MIDIRPC_H__)
#define __I_MIDIRPC_H__

#include "doomtype.h"

dboolean I_MidiRPCInitServer();
dboolean I_MidiRPCInitClient();
void I_MidiRPCClientShutDown();
dboolean I_MidiRPCReady();

dboolean I_MidiRPCRegisterSong(void *data, int size);
dboolean I_MidiRPCPlaySong(dboolean looping);
dboolean I_MidiRPCStopSong();
dboolean I_MidiRPCSetVolume(int volume);
dboolean I_MidiRPCPauseSong();
dboolean I_MidiRPCResumeSong();

#endif

// EOF
