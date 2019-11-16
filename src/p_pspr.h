/*
========================================================================

                           D O O M  R e t r o
         The classic, refined DOOM source port. For Windows PC.

========================================================================

  Copyright © 1993-2012 by id Software LLC, a ZeniMax Media company.
  Copyright © 2013-2019 by Brad Harding.

  DOOM Retro is a fork of Chocolate DOOM. For a list of credits, see
  <https://github.com/bradharding/doomretro/wiki/CREDITS>.

  This file is a part of DOOM Retro.

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
  company, in the US and/or other countries, and is used without
  permission. All other trademarks are the property of their respective
  holders. DOOM Retro is in no way affiliated with nor endorsed by
  id Software.

========================================================================
*/

#if !defined(__P_PSPR_H__)
#define __P_PSPR_H__

// Basic data types.
// Needs fixed point, and BAM angles.
#include "tables.h"

//
// Needs to include the precompiled
//  sprite animation tables.
// Header generated by multigen utility.
// This includes all the data for thing animation,
// i.e. the Thing Attributes table
// and the Frame Sequence table.
#include "states.h"

//
// Frame flags:
// handles maximum brightness (torches, muzzle flare, light sources)
//
#define FF_FULLBRIGHT   0x8000  // flag in thing->frame
#define FF_FRAMEMASK    0x7FFF

//
// Overlay psprites are scaled shapes
// drawn directly on the view screen,
// coordinates are given for a 320*200 view screen.
//
enum
{
    ps_weapon,
    ps_flash,
    NUMPSPRITES
};

typedef struct
{
    state_t *state; // a NULL state means not active
    int     tics;
    fixed_t sx, sy;
} pspdef_t;

dboolean P_CheckAmmo(weapontype_t weapon);

#endif
