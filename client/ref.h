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

// ref.h

#ifndef __REF_H__
#define __REF_H__

#define	TOP_RANGE		16			// soldier uniform colors
#define	BOTTOM_RANGE	96

//=============================================================================

#define	MAX_DLIGHTS				32
#define MAX_ENTITIES			256
#define MAX_PARTICLES			2048
#define MAX_CSHIFTS				16

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct
{
	vec3_t		org;
	int			color;
} particle_t;

typedef struct
{
	vec3_t		origin;
	float		radius;
} dlight_t;

typedef struct
{
	int			destcolor[3];
	int			percent;		// 0-256
} cshift_t;

typedef struct
{
	int			length;
	char		map[MAX_STYLESTRING];
} lightstyle_t;

typedef struct entity_s
{
	float			framelerp;

	struct model_s	*model;			// NULL = no model

	int				number;
	int				flags;
	int				skinnum;		// for Alias models

	byte			*colormap;

	vec3_t			origin;
	vec3_t			angles;

	int				frame;
	int				oldframe;
	float			syncbase;		// for client-side animations
} entity_t;

typedef struct
{
	int				x, y, width, height;	// subwindow in video for refresh

	float			time;
	float			oldtime;

	int				flags;

	float			fov_x, fov_y;

	vec3_t			vieworg;
	vec3_t			viewangles;

	lightstyle_t	*lightstyles;			// [MAX_LIGHTSTYLES]

	int				num_entities;
	entity_t		*entities;

	int				num_dlights;
	dlight_t		*dlights;

	int				num_particles;
	particle_t		*particles;

	int				num_cshifts;
	cshift_t		*cshifts;
} refdef_t;


#endif //__REF_H__