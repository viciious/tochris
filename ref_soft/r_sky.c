/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_sky.c

#include "r_local.h"
#include "d_local.h"


int		iskyspeed = 8;
int		iskyspeed2 = 2;
float	skyspeed, skyspeed2;

byte	*r_skysource;
float	skyshift;

int		r_skymade;


byte	skydata[SKYSIZE*SKYWIDTH];
byte	newsky[SKYSIZE*SKYWIDTH];	// newsky and topsky both pack in here, 128 bytes
									//  of newsky on the left of each scan, 128 bytes
									//  of topsky on the right, because the low-level
									//  drawers need 256-byte scan widths

void R_SkynameChanged_f (cvar_t *cvar);

cvar_t	r_skyname = { "r_skyname", "", true, false, &R_SkynameChanged_f };

/*
=============
R_InitSky

A sky texture is 256*128, with the right side being a masked overlay
==============
*/
void R_InitSky (texture_t *mt)
{
	byte		*src;

	src = (byte *)mt + mt->offsets[0];
	memcpy (skydata, (byte *) mt + mt->offsets[0], SKYSIZE*SKYWIDTH);
	r_skysource = newsky;
}


/*
=================
R_MakeSky
=================
*/
void R_MakeSky (void)
{
	int			x, y;
	byte		*base1, *base2, *out;
	int			xshift1, yshift1, xshift2, yshift2;
	static int	xlast = -1, ylast = -1;

	xshift2 = skyshift;
	yshift2 = skyshift;

	if ((xshift2 == xlast) && (yshift2 == ylast))
		return;

	xlast = xshift2;
	ylast = yshift2;
	xshift1 = xshift2 >> 1;
	yshift1 = yshift2 >> 1;

	out = newsky;
	for (y = 0; y < SKYSIZE; y++)
	{
		base1 = &skydata[((y + yshift1) & SKYMASK) * SKYWIDTH];
		base2 = &skydata[((y + yshift2) & SKYMASK) * SKYWIDTH + SKYSIZE];

		for (x = 0;x < SKYSIZE;x++)
		{
			if (base1[(x + xshift1) & SKYMASK])
				*out++ = base1[(x + xshift1) & SKYMASK];
			else
				*out++ = base2[(x + xshift2) & SKYMASK];
		}

		out += SKYSIZE;
	}

	r_skymade = 1;
}


/*
=============
R_SetSkyFrame
==============
*/
void R_SetSkyFrame (void)
{
	int		g, s1, s2;
	float	temp;
	float	skytime;

	skyspeed = iskyspeed;
	skyspeed2 = iskyspeed2;

	g = GreatestCommonDivisor (iskyspeed, iskyspeed2);
	s1 = iskyspeed / g;
	s2 = iskyspeed2 / g;
	temp = SKYSIZE * s1 * s2;

	skytime = r_refdef.time - ((int)(r_refdef.time / temp) * temp);
	skyshift = skytime * skyspeed;
	
	r_skymade = 0;
}

extern	mtexinfo_t		r_skytexinfo[6];
extern	qboolean		r_drawskybox;
byte					r_skypixels[6][256*256];
texture_t				r_skytextures[6];
char					skyname[MAX_QPATH];

qboolean R_LoadSkybox (char *name)
{
	int		i;
	char	pathname[MAX_QPATH];
	byte	*pic;
	char	*suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
	int		r_skysideimage[6] = {5, 2, 4, 1, 0, 3};
	int		width, height;

	if (!name || !name[0])
	{
		skyname[0] = 0;
		return false;
	}

	// the same skybox we are using now
	if (!stricmp (name, skyname))
		return true;

	strncpy (skyname, name, sizeof(skyname)-1);

	for (i=0 ; i<6 ; i++)
	{
#ifdef __linux__
		snprintf (pathname, sizeof(pathname), "env/%s%s.pcx\0", skyname, suf[r_skysideimage[i]]);
#else
		_snprintf (pathname, sizeof(pathname), "env/%s%s.pcx\0", skyname, suf[r_skysideimage[i]]);
#endif
		LoadPCX (pathname, &pic, &width, &height);

		if (!pic) {
			Con_Printf ("Couldn't load %s\n", pathname);
			return false;
		}
		if (width != 256 || height != 256) {
			Con_Printf ("%s must be 256x256\n", pathname);
			free (pic);
			return false;
		}

		r_skytexinfo[i].texture = &r_skytextures[i];
		r_skytexinfo[i].texture->width = 256;
		r_skytexinfo[i].texture->height = 256;
		r_skytexinfo[i].texture->offsets[0] = i;
		memcpy (r_skypixels[i], pic, 256*256);
		free (pic);
	}

	return true;
}

void R_SkynameChanged_f (cvar_t *cvar)
{
	if (cvar->string[0])
		r_drawskybox = R_LoadSkybox (cvar->string);
	else
		r_drawskybox = false;
}