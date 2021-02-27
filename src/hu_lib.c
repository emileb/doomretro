/*
========================================================================

                           D O O M  R e t r o
         The classic, refined DOOM source port. For Windows PC.

========================================================================

  Copyright © 1993-2012 by id Software LLC, a ZeniMax Media company.
  Copyright © 2013-2021 by Brad Harding.

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

#include <ctype.h>
#include <string.h>

#include "c_cmds.h"
#include "c_console.h"
#include "doomstat.h"
#include "hu_lib.h"
#include "hu_stuff.h"
#include "i_colors.h"
#include "i_swap.h"
#include "m_config.h"
#include "r_local.h"
#include "v_data.h"
#include "v_video.h"

byte            tempscreen[MAXSCREENAREA];

extern patch_t  *consolefont[CONSOLEFONTSIZE];
extern patch_t  *degree;
extern patch_t  *lsquote;
extern patch_t  *ldquote;
extern patch_t  *unknownchar;
extern patch_t  *altunderscores;

static void HUlib_ClearTextLine(hu_textline_t *t)
{
    t->len = 0;
    t->l[0] = '\0';
    t->needsupdate = 1;
}

void HUlib_InitTextLine(hu_textline_t *t, int x, int y, patch_t **f, int sc)
{
    t->x = x;
    t->y = y;
    t->f = f;
    t->sc = sc;
    HUlib_ClearTextLine(t);
}

dboolean HUlib_AddCharToTextLine(hu_textline_t *t, char ch)
{
    if (t->len == HU_MAXLINELENGTH)
        return false;
    else
    {
        t->l[t->len++] = ch;
        t->l[t->len] = '\0';
        t->needsupdate = 4;
        return true;
    }
}

static void HU_DrawDot(int x, int y, unsigned char src, int screenwidth)
{
    byte    *dest = &tempscreen[y * screenwidth + x];

    if (src == PINK)
        *dest = 0;
    else if (src != ' ')
        *dest = src;
}

// [BH] draw an individual character to temporary buffer
static void HU_DrawChar(int x, int y, int ch, int screenwidth)
{
    int w = (int)strlen(smallcharset[ch]) / 10;

    for (int y1 = 0; y1 < 10; y1++)
        for (int x1 = 0; x1 < w; x1++)
        {
            const unsigned char src = smallcharset[ch][y1 * w + x1];
            const int           i = (x + x1) * SCREENSCALE;
            const int           j = (y + y1) * SCREENSCALE;

            for (int yy = 0; yy < SCREENSCALE; yy++)
                for (int xx = 0; xx < SCREENSCALE; xx++)
                    HU_DrawDot(i + xx, j + yy, src, screenwidth);
        }
}

static void HUlib_DrawAltHUDTextLine(hu_textline_t *l)
{
    dboolean        italics = false;
    unsigned char   prevletter = '\0';
    int             x = 10;
    int             color = nearestwhite;
    int             len = l->len;

    if (!automapactive)
    {
        x = HU_ALTHUDMSGX;
        color = (r_textures ? (viewplayer->fixedcolormap == INVERSECOLORMAP ? colormaps[0][32 * 256 + nearestwhite] : nearestwhite) :
            (viewplayer->fixedcolormap == INVERSECOLORMAP ? colormaps[0][32 * 256 + nearestwhite] : nearestblack));
    }

    if (idbehold)
        althudtextfunc(x, HU_ALTHUDMSGY + 12, screens[0], altunderscores, false, color, SCREENWIDTH);

    for (int i = 0; i < len; i++)
    {
        unsigned char   letter = l->l[i];
        unsigned char   nextletter = l->l[i + 1];
        patch_t         *patch = unknownchar;

        if (letter == '<' && i < len - 2 && tolower(l->l[i + 1]) == 'b' && l->l[i + 2] == '>')
            i += 2;
        else if (letter == '<' && i < len - 3 && l->l[i + 1] == '/' && tolower(l->l[i + 2]) == 'b' && l->l[i + 3] == '>')
            i += 3;
        else if (letter == '<' && i < len - 2 && tolower(l->l[i + 1]) == 'i' && l->l[i + 2] == '>')
        {
            italics = true;
            i += 2;
        }
        else if (letter == '<' && i < len - 3 && l->l[i + 1] == '/' && tolower(l->l[i + 2]) == 'i' && l->l[i + 3] == '>')
        {
            italics = false;
            i += 3;
            x++;
        }
        else
        {
            if (letter == 194 && nextletter == 176)
            {
                patch = degree;
                i++;
            }
            else
            {
                const int   c = letter - CONSOLEFONTSTART;
                int         j = 0;

                if (c >= 0 && c < CONSOLEFONTSIZE)
                    patch = consolefont[c];

                if (!i || prevletter == ' ')
                {
                    if (letter == '\'')
                        patch = lsquote;
                    else if (letter == '"')
                        patch = ldquote;
                }

                // [BH] apply kerning to certain character pairs
                while (altkern[j].char1)
                {
                    if (prevletter == altkern[j].char1 && letter == altkern[j].char2)
                    {
                        x += altkern[j].adjust;
                        break;
                    }

                    j++;
                }
            }

            althudtextfunc(x, HU_ALTHUDMSGY, screens[0], patch, italics, color, SCREENWIDTH);
            x += SHORT(patch->width);
            prevletter = letter;
        }
    }
}

void HUlib_DrawAltAutomapTextLine(hu_textline_t *l, dboolean external)
{
    unsigned char   prevletter = '\0';
    int             x = 10;
    byte            *fb1 = (external ? mapscreen : screens[0]);
    int             len = l->len;

    for (int i = 0; i < len; i++)
    {
        unsigned char   letter = l->l[i];
        patch_t         *patch = unknownchar;
        int             j = 0;
        const int       c = letter - CONSOLEFONTSTART;

        if (c >= 0 && c < CONSOLEFONTSIZE)
            patch = consolefont[c];

        if (!i || prevletter == ' ')
        {
            if (letter == '\'')
                patch = lsquote;
            else if (letter == '"')
                patch = ldquote;
        }

        // [BH] apply kerning to certain character pairs
        while (altkern[j].char1)
        {
            if (prevletter == altkern[j].char1 && letter == altkern[j].char2)
            {
                x += altkern[j].adjust;
                break;
            }

            j++;
        }

        althudtextfunc(x, SCREENHEIGHT - 16, fb1, patch, false, nearestwhite, (external ? MAPWIDTH : SCREENWIDTH));
        x += SHORT(patch->width);
        prevletter = letter;
    }
}

const kern_t kern[] =
{
    { ',', '1',  -1 },
    { ',', '7',  -1 },
    { ',', 'Y',  -1 },
    { '.', '"',  -1 },
    { '.', '1',  -1 },
    { '.', '7',  -1 },
    { '3', '"',  -1 },
    { 'D', '\'', -1 },
    { 'F', '.',  -1 },
    { 'L', '"',  -1 },
    { 'L', 'T',  -1 },
    { 'P', ',',  -1 },
    { 'P', '.',  -1 },
    { 'T', ',',  -1 },
    { 'T', '.',  -1 },
    { 'Y', ',',  -1 },
    { 'Y', '.',  -1 },
    {  0,   0,    0 }
};

static void HUlib_DrawTextLine(hu_textline_t *l, dboolean external)
{
    int             w = 0;
    int             tw = 0;
    int             x, y;
    int             maxx, maxy;
    unsigned char   prev = '\0';
    byte            *fb1 = screens[0];
    byte            *fb2 = screens[(r_screensize < r_screensize_max - 1 && !automapactive)];
    int             len = l->len;
    const dboolean  idmypos = viewplayer->cheats & CF_MYPOS;

    if (external)
    {
        fb1 = mapscreen;
        fb2 = mapscreen;
    }

    // draw the new stuff
    x = l->x;
    y = l->y;

    memset(tempscreen, PINK, SCREENAREA);

    for (int i = 0; i < len; i++)
    {
        unsigned char   c = toupper(l->l[i]);

        if (c == ' ')
        {
            w = (vanilla ? 4 : (i > 0 && (prev == '.' || prev == '!' || prev == '?') ? 5 : 3));
            x += w;
            tw += w;
        }
        else if (c != '\n' && ((c >= l->sc && c <= '_') || c == 176))
        {
            int j = c - '!';

            // [BH] have matching curly single and double quotes
            if (!i || l->l[i - 1] == ' ')
            {
                if (c == '"')
                    j = 64;
                else if (c == '\'')
                    j = 65;
            }

            if (c == 176)
            {
                if (STCFN034)
                    continue;
                else
                    j = 66;
            }

            if (STCFN034)
            {
                // [BH] display lump from PWAD with shadow
                w = SHORT(l->f[c - l->sc]->width);

                if (prev == ' ' && c == '(' && !idmypos)
                    x -= 2;

                V_DrawPatchToTempScreen(x, l->y, l->f[c - l->sc]);
            }
            else
            {
                if (prev == ' ' && c == '(' && !idmypos)
                    x -= 2;
                else
                {
                    int k = 0;

                    // [BH] apply kerning to certain character pairs
                    while (kern[k].char1)
                    {
                        if (prev == kern[k].char1 && c == kern[k].char2)
                        {
                            x += kern[k].adjust;
                            break;
                        }

                        k++;
                    }
                }

                // [BH] draw individual character
                w = (int)strlen(smallcharset[j]) / 10 - 1;
                HU_DrawChar(x, y - 1, j, SCREENWIDTH);
            }

            x += w;
            tw += w;
        }

        prev = c;
    }

    // [BH] draw underscores for IDBEHOLD cheat message
    if (idbehold && !STCFN034 && s_STSTR_BEHOLD2 && !vanilla)
    {
        for (int y1 = 0; y1 < 4; y1++)
            for (int x1 = 0; x1 < VANILLAWIDTH; x1++)
            {
                unsigned char   src = underscores[y1 * VANILLAWIDTH + x1];

                if (src != ' ')
                    for (int y2 = 0; y2 < SCREENSCALE; y2++)
                        for (int x2 = 0; x2 < SCREENSCALE; x2++)
                        {
                            byte    *dest = &tempscreen[((l->y + y1 + 6) * SCREENSCALE + y2) * SCREENWIDTH
                                        + (l->x + x1 - 3) * SCREENSCALE + x2];

                            *dest = (src == PINK ? 0 : src);
                        }
            }
    }

    // [BH] draw entire message from buffer onto screen
    maxx = (l->x + tw + 1) * SCREENSCALE;
    maxy = (y + 11) * SCREENSCALE;

    for (int yy = MAX(0, l->y - 1); yy < maxy; yy++)
        for (int xx = l->x; xx < maxx; xx++)
        {
            int     dot = yy * SCREENWIDTH + xx;
            byte    *source = &tempscreen[dot];
            byte    *dest1 = &fb1[dot];

            if (!*source)
                *dest1 = tinttab50[(nearestblack << 8) + fb2[dot]];
            else if (*source != PINK)
            {
                if (r_hud_translucency)
                    *dest1 = tinttab75[(*source << 8) + fb2[dot]];
                else
                    *dest1 = *source;
            }
        }
}

void HUlib_DrawAutomapTextLine(hu_textline_t *l, dboolean external)
{
    int             w = (external ? MAPWIDTH : SCREENWIDTH);
    int             x, y;
    unsigned char   prev = '\0';
    byte            *fb1 = (external ? mapscreen : screens[0]);
    int             len = l->len;
    const dboolean  idmypos = viewplayer->cheats & CF_MYPOS;

    // draw the new stuff
    x = l->x;
    y = l->y;

    for (int i = 0; i < len; i++)
    {
        unsigned char   c = toupper(l->l[i]);

        if (c == ' ')
            x += (vanilla ? 4 : (i > 0 && (prev == '.' || prev == '!' || prev == '?') ? 5 : 3)) * SCREENSCALE;
        else if (c != '\n' && ((c >= l->sc && c <= '_') || c == 176))
        {
            int j = c - '!';

            // [BH] have matching curly single and double quotes
            if (!i || l->l[i - 1] == ' ')
            {
                if (c == '"')
                    j = 64;
                else if (c == '\'')
                    j = 65;
            }

            if (c == 176)
            {
                if (STCFN034)
                    continue;
                else
                    j = 66;
            }

            if (!STCFN034)
            {
                if (prev == ' ' && c == '(' && !idmypos)
                    x -= 2;
                else
                {
                    int k = 0;

                    // [BH] apply kerning to certain character pairs
                    while (kern[k].char1)
                    {
                        if (prev == kern[k].char1 && c == kern[k].char2)
                        {
                            x += kern[k].adjust;
                            break;
                        }

                        k++;
                    }
                }
            }

            // [BH] draw individual character
            if (r_hud_translucency)
                V_DrawTranslucentHUDText(x, y - 1, fb1, l->f[c - l->sc], w);
            else
                V_DrawHUDText(x, y - 1, fb1, l->f[c - l->sc], w);

            x += SHORT(l->f[c - l->sc]->width) * SCREENSCALE;
        }

        prev = c;
    }
}

// sorta called by HU_Erase and just better darn get things straight
void HUlib_EraseTextLine(hu_textline_t *l)
{
    // Only erases when NOT in automap and the screen is reduced,
    // and the text must either need updating or refreshing
    // (because of a recent change back from the automap)
    if (!automapactive && viewwindowx && l->needsupdate)
    {
        const int   lh = (SHORT(l->f[0]->height) + 4) * SCREENSCALE;

        for (int y = l->y, yoffset = y * SCREENWIDTH; y < l->y + lh; y++, yoffset += SCREENWIDTH)
            if (y < viewwindowy || y >= viewwindowy + viewheight)
                R_VideoErase(yoffset, SCREENWIDTH);                             // erase entire line
            else
            {
                R_VideoErase(yoffset, viewwindowx);                             // erase left border
                R_VideoErase(yoffset + viewwindowx + viewwidth, viewwindowx);   // erase right border
            }
    }

    if (l->needsupdate)
        l->needsupdate--;
}

void HUlib_InitSText(hu_stext_t *s, int x, int y, int h, patch_t **font, int startchar, dboolean *on)
{
    s->h = h;
    s->on = on;
    s->laston = true;
    s->cl = 0;

    for (int i = 0; i < h; i++)
        HUlib_InitTextLine(&s->l[i], x, y - i * (SHORT(font[0]->height) + 1), font, startchar);
}

static void HUlib_AddLineToSText(hu_stext_t *s)
{
    // add a clear line
    if (++s->cl == s->h)
        s->cl = 0;

    HUlib_ClearTextLine(&s->l[s->cl]);

    // everything needs updating
    for (int i = 0; i < s->h; i++)
        s->l[i].needsupdate = 4;
}

void HUlib_AddMessageToSText(hu_stext_t *s, const char *msg)
{
    HUlib_AddLineToSText(s);

    while (*msg)
        HUlib_AddCharToTextLine(&s->l[s->cl], *(msg++));
}

void HUlib_DrawSText(hu_stext_t *s, dboolean external)
{
    if (!*s->on)
        return;             // if not on, don't draw

    // draw everything
    for (int i = 0; i < s->h; i++)
    {
        int             idx = s->cl - i;
        hu_textline_t   *l;

        if (idx < 0)
            idx += s->h;    // handle queue of lines

        l = &s->l[idx];

        if (r_althud && r_screensize == r_screensize_max)
            HUlib_DrawAltHUDTextLine(l);
        else
            HUlib_DrawTextLine(l, external);
    }
}

void HUlib_EraseSText(hu_stext_t *s)
{
    for (int i = 0; i < s->h; i++)
    {
        if (s->laston && !*s->on)
            s->l[i].needsupdate = 4;

        HUlib_EraseTextLine(&s->l[i]);
    }

    s->laston = *s->on;
}
