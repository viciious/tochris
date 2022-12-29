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

// r_shared.h: general refresh-related stuff shared between the refresh and the
// driver

// FIXME: clean up and move into d_iface.h

#ifndef _R_SHARED_H_
#define _R_SHARED_H_

#include "../common/quakedef.h"
#include "../client/vid.h"
#include "../client/ref.h"
#include "../client/render.h"
#include "r_model.h"
#include "d_iface.h"

#define	MAXVERTS	16					// max points in a surface polygon
#define MAXWORKINGVERTS	(MAXVERTS+4)	// max points in an intermediate
										//  polygon (while processing)
// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define	MAXHEIGHT		1024
#define	MAXWIDTH		1280
#define MAXDIMENSION	((MAXHEIGHT > MAXWIDTH) ? MAXHEIGHT : MAXWIDTH)

#define SIN_BUFFER_SIZE	(MAXDIMENSION+CYCLE)

#define INFINITE_DISTANCE	0x10000		// distance that's always guaranteed to
										//  be farther away than anything in
										//  the scene

//===================================================================

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	vrect_t		vrect;				// subwindow in video for refresh
									// FIXME: not need vrect next field here?
	vrect_t		aliasvrect;			// scaled Alias version
	int			vrectright, vrectbottom;	// right & bottom screen coords
	int			aliasvrectright, aliasvrectbottom;	// scaled Alias versions
	float		vrectrightedge;			// rightmost right edge we care about,
										//  for use in edge list
	float		fvrectx, fvrecty;		// for floating-point compares
	float		fvrectx_adj, fvrecty_adj; // left and top edges, for clamping
	int			vrect_x_adj_shift20;	// (vrect.x + 0.5 - epsilon) << 20
	int			vrectright_adj_shift20;	// (vrectright + 0.5 - epsilon) << 20
	float		fvrectright_adj, fvrectbottom_adj;
										// right and bottom edges, for clamping
	float		fvrectright;			// rightmost edge, for Alias clamping
	float		fvrectbottom;			// bottommost edge, for Alias clamping
	float		horizontalFieldOfView;	// at Z = 1.0, this many X is visible 
										// 2.0 = 90 degrees
	float		xOrigin;			// should probably always be 0.5
	float		yOrigin;			// between be around 0.3 to 0.5

	int			ambientlight;
} oldrefdef_t;

typedef struct mpic_s
{
	int			width;
	short		height;
	byte		alpha;
	byte		pad;
	byte		data[4];	// variable sized
} mpic_t;

extern	oldrefdef_t	r_oldrefdef;

//===================================================================

extern int		cachewidth;
extern byte		*cacheblock;
extern int		screenwidth;

extern	float	pixelAspect;

extern int		r_drawnpolycount;

extern cvar_t	r_clearcolor;

extern int	sintable[SIN_BUFFER_SIZE];
extern int	intsintable[SIN_BUFFER_SIZE];

extern	vec3_t	vup, base_vup;
extern	vec3_t	vpn, base_vpn;
extern	vec3_t	vright, base_vright;
extern	entity_t		*currententity;
extern	model_t			*currentmodel;

#define NUMSTACKEDGES		2000
#define	MINEDGES			NUMSTACKEDGES
#define NUMSTACKSURFACES	1000
#define MINSURFACES			NUMSTACKSURFACES
#define	MAXSPANS			3000

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct espan_s
{
	int				u, v, count;
	struct espan_s	*pnext;
} espan_t;

// FIXME: compress, make a union if that will help
// insubmodel is only 1, flags is fewer than 32, spanstate could be a byte
typedef struct surf_s
{
	struct surf_s	*next;			// active surface stack in r_edge.c
	struct surf_s	*prev;			// used in r_edge.c for active surf stack
	struct espan_s	*spans;			// pointer to linked list of spans to draw
	int			key;				// sorting key (BSP order)
	int			last_u;				// set during tracing
	int			spanstate;			// 0 = not in span
									// 1 = in span
									// -1 = in inverted span (end before
									//  start)
	int			flags;				// currentface flags
	void		*data;				// associated data like msurface_t
	entity_t	*entity;
	float		nearzi;				// nearest 1/z on surface, for mipmapping
	qboolean	insubmodel;
	float		d_ziorigin, d_zistepu, d_zistepv;

	int			pad[2];				// to 64 bytes
} surf_t;

extern	surf_t	*surfaces, *surface_p, *surf_max;

// surfaces are generated in back to front order by the bsp, so if a surf
// pointer is greater than another one, it should be drawn in front
// surfaces[1] is the background, and is used as the active surface stack.
// surfaces[0] is a dummy, because index 0 is used to indicate no surface
//  attached to an edge_t

//===================================================================

extern vec3_t	sxformaxis[4];	// s axis transformed into viewspace
extern vec3_t	txformaxis[4];	// t axis transformed into viewspac

extern vec3_t	modelorg, base_modelorg;

extern	float	xcenter, ycenter;
extern	float	xscale, yscale;
extern	float	xscaleinv, yscaleinv;
extern	float	xscaleshrink, yscaleshrink;

extern	int		d_lightstylevalue[256]; // 8.8 frac of base light value

extern void TransformVector (vec3_t in, vec3_t out);
extern void SetUpForLineScan(fixed8_t startvertu, fixed8_t startvertv,
	fixed8_t endvertu, fixed8_t endvertv);

extern int	r_skymade;
extern void R_MakeSky (void);

extern	entity_t	r_worldent;
extern	vec3_t	r_origin, vpn, vright, vup;
extern	float	viewmatrix[3][4];

extern	refdef_t	r_refdef;

extern	struct model_s		*r_worldmodel;

extern	struct texture_s	*r_notexture_mip;

extern	int	ubasestep, errorterm, erroradjustup, erroradjustdown;

// flags in finalvert_t.flags
#define ALIAS_LEFT_CLIP				0x0001
#define ALIAS_TOP_CLIP				0x0002
#define ALIAS_RIGHT_CLIP			0x0004
#define ALIAS_BOTTOM_CLIP			0x0008
#define ALIAS_Z_CLIP				0x0010
// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define ALIAS_ONSEAM				0x0020	// also defined in modelgen.h;
											//  must be kept in sync
#define ALIAS_XY_CLIP_MASK			0x000F

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct edge_s
{
	fixed16_t		u;
	fixed16_t		u_step;
	struct edge_s	*prev, *next;
	unsigned short	surfs[2];
	struct edge_s	*nextremove;
	float			nearzi;
	medge_t			*owner;
} edge_t;

#endif	// _R_SHARED_H_
