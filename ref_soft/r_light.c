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
// r_light.c

#include "quakedef.h"
#include "r_local.h"

int	r_dlightframecount;


/*
==================
R_AnimateLight
==================
*/
void R_AnimateLight (void)
{
	int			i, j;
	
//
// light animations
// 'm' is normal light, 'a' is no light, 'z' is double bright
	i = (int)(r_refdef.time*10);
	for (j=0 ; j<MAX_LIGHTSTYLES ; j++)
	{
		if (!r_refdef.lightstyles[j].length)
		{
			d_lightstylevalue[j] = 256;
			continue;
		}
		d_lightstylevalue[j] = (r_refdef.lightstyles[j].map[i % r_refdef.lightstyles[j].length] - 'a') * 22;
	}	
}


/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
void R_MarkLights (dlight_t *light, int bit, mnode_t *node)
{
	cplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;
	
	while (node->contents >= 0)
	{
		splitplane = node->plane;
		dist = PlaneDiff (light->origin, splitplane);
		
		if (dist > light->radius)
		{
			node = node->children[0];
			continue;
		}

		if (dist < -light->radius)
		{
			node = node->children[1];
			continue;
		}
			
	// mark the polygons
		surf = currentmodel->surfaces + node->firstsurface;
		for (i=0 ; i<node->numsurfaces ; i++, surf++)
		{
			if (surf->dlightframe != r_dlightframecount)
			{
				surf->dlightbits = 0;
				surf->dlightframe = r_dlightframecount;
			}
			surf->dlightbits |= bit;
		}

		if (node->children[0]->contents >= 0)
		{
			if (node->children[1]->contents >= 0)
			{
				R_MarkLights (light, bit, node->children[0]);
				node = node->children[1];
				continue;
			}
			else
			{
				node = node->children[0];
				continue;
			}
		}
		else if (node->children[1]->contents >= 0)
		{
			node = node->children[1];
			continue;
		}

		return;
	}
}


/*
=============
R_PushDlights
=============
*/
void R_PushDlights (void)
{
	int		i;
	dlight_t	*l;

	r_dlightframecount = r_framecount;
	l = r_refdef.dlights;

	currentmodel = r_worldmodel;		// HACK
	for (i=0 ; i<r_refdef.num_dlights ; i++, l++)
	{
		R_MarkLights ( l, 1<<i, r_worldmodel->nodes );
	}
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

int RecursiveLightPoint (mnode_t *node, vec3_t start, vec3_t end)
{
	int			r;
	float		front, back, frac;
	int			side;
	cplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			s, t, ds, dt;
	int			i;
	mtexinfo_t	*tex;

	if (node->contents < 0)
		return -1;		// didn't hit anything
	
// calculate mid point

// FIXME: optimize for axial
	plane = node->plane;
	front = PlaneDiff (start, plane);
	back = PlaneDiff (end, plane);
	side = front < 0;
	
	if ( (back < 0) == side)
		return RecursiveLightPoint (node->children[side], start, end);
	
	frac = front / (front-back);
	LerpVectors (start, frac, end, mid);
	
// go down front side	
	r = RecursiveLightPoint (node->children[side], start, mid);
	if (r >= 0)
		return r;		// hit something
		
	if ( (back < 0) == side )
		return -1;		// didn't hit anuthing
		
// check for impact on this node

	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags & SURF_DRAWTILED)
			continue;	// no lightmaps

		tex = surf->texinfo;
		
		s = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3];
		if (s < surf->texturemins[0])
			continue;
		ds = s - surf->texturemins[0];
		if (ds > surf->extents[0])
			continue;

		t = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3];
		if (t < surf->texturemins[1])
			continue;
		dt = t - surf->texturemins[1];
		if (dt > surf->extents[1])
			continue;

		r = 0;
		if (surf->samples)
		{
			int maps, line, dsfrac = ds & 15, dtfrac = dt & 15, r00 = 0, r01 = 0, r10 = 0, r11 = 0;
			float scale;
			byte *lightmap;

			line = (surf->extents[0] >> 4) + 1;
			lightmap = surf->samples + (dt>>4) * line + (ds>>4);

			for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
			{
				scale = d_lightstylevalue[surf->styles[maps]] * (1.0 / 256.0);
				r00 += (float)lightmap[     0] * scale;
				r01 += (float)lightmap[     1] * scale;
				r10 += (float)lightmap[line+0] * scale;
				r11 += (float)lightmap[line+1] * scale;
				lightmap += line * ((surf->extents[1]>>4)+1);
			}
			
			r = ((int) ((((((((r11-r10) * dsfrac) >> 4) + r10)-((((r01-r00) * dsfrac) >> 4) + r00)) * dtfrac) >> 4) + ((((r01-r00) * dsfrac) >> 4) + r00)));
		}
		
		return r;
	}

// go down back side
	return RecursiveLightPoint (node->children[!side], mid, end);
}

int R_LightPoint (vec3_t p)
{
	vec3_t		end;
	int			r;
	
	if (!r_worldmodel->lightdata)
		return 255;
	
	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;
	
	r = RecursiveLightPoint (r_worldmodel->nodes, p, end);
	
	if (r == -1)
		r = 0;

	return r;
}

unsigned		blocklights[32*32];

/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights (void)
{
	msurface_t *surf;
	int			lnum;
	int			sd, td;
	float		dist, rad;
	vec3_t		impact, local;
	int			s, t;
	int			i;
	int			smax, tmax;
	mtexinfo_t	*tex;
	cplane_t	*plane;
	dlight_t	*l;
	qboolean	rotated = false;
	vec3_t		dlorigin, axis[3];

	surf = r_drawsurf.surf;
	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	tex = surf->texinfo;

	if (currententity->angles[0] || currententity->angles[1] || currententity->angles[2])
	{
		rotated = true;
		AngleVectors (currententity->angles, axis[0], axis[1], axis[2]);
	}

	l = r_refdef.dlights;
	for (lnum=0 ; lnum<r_refdef.num_dlights ; lnum++, l++)
	{
		if ( !(surf->dlightbits & (1<<lnum) ) )
			continue;		// not lit by this light

		VectorSubtract (l->origin, currententity->origin, dlorigin);
		if (rotated)
		{
			vec3_t temp;

			VectorCopy (dlorigin, temp);
			dlorigin[0] = DotProduct (temp, axis[0]);
			dlorigin[1] = -DotProduct (temp, axis[1]);
			dlorigin[2] = DotProduct (temp, axis[2]);
		}

		rad = l->radius;
		plane = surf->plane;
		dist = PlaneDiff (dlorigin, plane);
		rad -= fabs(dist);
		if (rad < 0)
			continue;

		if (plane->type < 3)
		{
			VectorCopy (dlorigin, impact);
			impact[plane->type] -= dist;
		}
		else
		{
			for (i=0 ; i<3 ; i++)
			{
				impact[i] = dlorigin[i] - plane->normal[i]*dist;
			}
		}

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];
		
		for (t = 0 ; t<tmax ; t++)
		{
			td = local[1] - t*16;
			if (td < 0)
				td = -td;
			for (s=0 ; s<smax ; s++)
			{
				sd = local[0] - s*16;
				if (sd < 0)
					sd = -sd;
				if (sd > td)
					dist = sd + (td>>1);
				else
					dist = td + (sd>>1);
				if (dist < rad)
					blocklights[t*smax + s] += (rad - dist)*256;
			}
		}
	}
}

/*
===============
R_BuildLightmap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
void R_BuildLightmap (void)
{
	int			smax, tmax;
	int			t;
	int			i, size;
	byte		*lightmap;
	unsigned	scale;
	int			maps;
	msurface_t	*surf;

	surf = r_drawsurf.surf;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	size = smax*tmax;
	lightmap = surf->samples;

	if (r_fullbright.value || !r_worldmodel->lightdata)
	{
		for (i=0 ; i<size ; i++)
			blocklights[i] = 0;
		return;
	}

// clear to ambient
	for (i=0 ; i<size ; i++)
		blocklights[i] = r_oldrefdef.ambientlight<<8;

// add all the lightmaps
	if (lightmap)
		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			scale = r_drawsurf.lightadj[maps];	// 8.8 fraction		
			for (i=0 ; i<size ; i++)
				blocklights[i] += lightmap[i] * scale;
			lightmap += size;	// skip to next lightmap
		}

// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLights ();

// bound, invert, and shift
	for (i=0 ; i<size ; i++)
	{
		t = (255*256 - (int)blocklights[i]) >> (8 - VID_CBITS);

		if (t < (1 << 6))
			t = (1 << 6);

		blocklights[i] = t;
	}
}

