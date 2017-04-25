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

#if !defined(__W_WAD_H__)
#define __W_WAD_H__

#include "doomdef.h"
#include "doomtype.h"
#include "w_file.h"

//
// TYPES
//

//
// WADFILE I/O related stuff.
//

#define IWAD 1
#define PWAD 2

typedef struct lumpinfo_s lumpinfo_t;
typedef int lumpindex_t;

struct lumpinfo_s
{
    char        name[9];
    int         size;
    void        *data;

    // killough 1/31/98: hash table fields, used for ultra-fast hash table lookup
    int         index;
    lumpindex_t next;

    int         position;

    wadfile_t   *wadfile;
};

extern lumpinfo_t       **lumpinfo;
extern int              numlumps;

wadfile_t *W_AddFile(char *filename, dboolean automatic);
int W_WadType(char *filename);

lumpindex_t W_CheckNumForName(char *name);

lumpindex_t W_RangeCheckNumForName(lumpindex_t min, lumpindex_t max, char *name);
lumpindex_t W_GetNumForName(char *name);
lumpindex_t W_GetNumForName2(char *name);
lumpindex_t W_GetNumForNameX(char *name, unsigned int count);

int W_CheckMultipleLumps(char *name);

int W_LumpLength(lumpindex_t lumpnum);
void W_ReadLump(lumpindex_t lumpnum, void *dest);

void *W_CacheLumpNum(lumpindex_t lumpnum, int tag);

#define W_CacheLumpName(name, tag)      W_CacheLumpNum(W_GetNumForName(name), (tag))
#define W_CacheLumpName2(name, tag)     W_CacheLumpNum(W_GetNumForName2(name), (tag))

void W_InitHashTable(void);

unsigned int W_LumpNameHash(const char *s);

void W_ReleaseLumpNum(lumpindex_t lump);

#define W_ReleaseLumpName(name)         W_ReleaseLumpNum(W_GetNumForName(name))

int IWADRequiredByPWAD(const char *pwadname);
dboolean HasDehackedLump(const char *pwadname);

#endif
