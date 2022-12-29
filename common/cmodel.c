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
// cmodel.c -- collision model code for client and server

#include "quakedef.h"

typedef struct chull_s
{
	cplane_t		*planes;
	cclipnode_t		*clipnodes;

	int				firstclipnode;
	int				lastclipnode;

	vec3_t			clip_mins;
	vec3_t			clip_maxs;
} chull_t;

typedef struct cmodel_s
{
	float			mins[3], maxs[3];
	float			radius;
	int				headnode[MAX_MAP_HULLS];
	chull_t			hulls[MAX_MAP_HULLS];
} cmodel_t;

typedef struct cnode_s
{
	int				contents;

	cplane_t		*plane;
	struct cnode_s	*children[2];
} cnode_t;

typedef struct
{
	int				contents;

	byte			*compressed_vis;
	byte			ambient_sound_level[NUM_AMBIENTS];
} cleaf_t;

int					cmap_numsubmodels;
cmodel_t			cmap_submodels[MAX_MAP_MODELS];

int					cmap_numplanes;
cplane_t			cmap_planes[MAX_MAP_PLANES];

int					cmap_numnodes;
cnode_t				cmap_nodes[MAX_MAP_NODES];

int					cmap_numhull0clipnodes;
cclipnode_t			cmap_hull0clipnodes[MAX_MAP_NODES];

int					cmap_numclipnodes;
cclipnode_t			cmap_clipnodes[MAX_MAP_CLIPNODES];

int					cmap_numleafs;
cleaf_t				cmap_leafs[MAX_MAP_LEAFS];

int					cmap_visdatasize;
byte				cmap_visdata[MAX_MAP_VISIBILITY];

int					cmap_entstringsize;
char				cmap_entstring[MAX_MAP_ENTSTRING];

byte				*cmod_base;
char				cmap_name[MAX_QPATH];

byte				cmod_novis[MAX_MAP_LEAFS/8];

void CM_InitBoxHull (void);

/*
=================
CM_LoadEntities
=================
*/
void CM_LoadEntities (lump_t *l)
{
	if (!l->filelen)
	{
		cmap_entstringsize = 0;
		memset (cmap_entstring, 0, sizeof(cmap_entstring));
		return;
	}

	cmap_entstringsize = l->filelen;
	memcpy (cmap_entstring, cmod_base + l->fileofs, l->filelen);
}

/*
=================
CM_LoadPlanes
=================
*/
void CM_LoadPlanes (lump_t *l)
{
	int			i, j;
	dplane_t 	*in;
	cplane_t	*out;
	int			bits;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_EndGame ("CM_LoadPlanes: funny lump size in %s", cmap_name);
	cmap_numplanes = l->filelen / sizeof(*in);
	out = cmap_planes;

	for ( i=0 ; i<cmap_numplanes ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signbits = bits;
	}
}

/*
=================
CM_LoadNodes
=================
*/
void CM_LoadNodes (lump_t *l)
{
	dnode_t		*in;
	cnode_t 	*out;
	int			i, j, p;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_EndGame ("CM_LoadNodes: funny lump size in %s", cmap_name);
	cmap_numnodes = l->filelen / sizeof(*in);
	out = cmap_nodes;

	for ( i=0 ; i<cmap_numnodes ; i++, in++, out++)
	{
		out->plane = cmap_planes + LittleLong (in->planenum);

		for (j=0 ; j<2 ; j++)
		{
			p = LittleShort (in->children[j]);
			if (p >= 0)
				out->children[j] = cmap_nodes + p;
			else
				out->children[j] = (cnode_t *)(cmap_leafs + (-1 - p));
		}
	}
}

/*
=================
CM_LoadLeafs
=================
*/
void CM_LoadLeafs (lump_t *l)
{
	dleaf_t 	*in;
	cleaf_t 	*out;
	int			i, j, p;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_EndGame ("Host_EndGame: funny lump size in %s", cmap_name);
	cmap_numleafs = l->filelen / sizeof(*in);
	out = cmap_leafs;

	for ( i=0 ; i<cmap_numleafs ; i++, in++, out++)
	{
		out->contents = LittleLong (in->contents);
		
		p = LittleLong (in->visofs);

		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = cmap_visdata + p;
		
		for (j=0 ; j<4 ; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];
	}	
}

/*
=================
CM_LoadClipnodes
=================
*/
void CM_LoadClipnodes (lump_t *l)
{
	int			i, j;
	dclipnode_t *in;
	cnode_t		*node, *child;
	cclipnode_t *out;
	chull_t		*hull;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_EndGame ("CM_LoadClipnodes: funny lump size in %s", cmap_name);

// duplicate the drawing hull structure as a clipping hull
	cmap_numhull0clipnodes = cmap_numnodes;
	out = cmap_hull0clipnodes;	

	hull = &cmap_submodels[0].hulls[0];	
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = cmap_numhull0clipnodes-1;
	hull->planes = cmap_planes;

	node = cmap_nodes;
	for (i=0 ; i<cmap_numhull0clipnodes ; i++, out++, node++)
	{
		out->plane = node->plane;

		for (j=0 ; j<2 ; j++)
		{
			child = node->children[j];

			if (child->contents < 0)
				out->children[j] = child->contents;
			else
				out->children[j] = child - cmap_nodes;
		}
	}

	cmap_numclipnodes = l->filelen / sizeof(*in);
	out = cmap_clipnodes;

	hull = &cmap_submodels[0].hulls[1];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = cmap_numclipnodes-1;
	hull->planes = cmap_planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 32;

	hull = &cmap_submodels[0].hulls[2];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = cmap_numclipnodes-1;
	hull->planes = cmap_planes;
	hull->clip_mins[0] = -32;
	hull->clip_mins[1] = -32;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 32;
	hull->clip_maxs[1] = 32;
	hull->clip_maxs[2] = 64;

	for (i=0 ; i<cmap_numclipnodes ; i++, out++, in++)
	{
		out->plane = cmap_planes + LittleLong (in->planenum);
		out->children[0] = LittleShort (in->children[0]);
		out->children[1] = LittleShort (in->children[1]);
	}
}

/*
=================
CM_LoadSubmodels
=================
*/
void CM_LoadSubmodels (lump_t *l)
{
	dmodel_t	*in;
	cmodel_t	*out;
	int			i, j;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_EndGame ("CM_LoadSubmodels: funny lump size in %s", cmap_name);
	cmap_numsubmodels = l->filelen / sizeof(*in);
	out = cmap_submodels;

	for ( i=0 ; i<cmap_numsubmodels ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
		}

		out->radius = RadiusFromBounds (out->mins, out->maxs);

		for (j=0 ; j<MAX_MAP_HULLS ; j++)
			out->headnode[j] = LittleLong (in->headnode[j]);
	}
}

/*
=================
CM_LoadVisibility
=================
*/
void CM_LoadVisibility (lump_t *l)
{
	if (!l->filelen)
	{
		cmap_visdatasize = sizeof(cmap_visdata);
		memset (cmap_visdata, 0xff, sizeof(cmap_visdata));
		return;
	}

	cmap_visdatasize = l->filelen;	
	memcpy (cmap_visdata, cmod_base + l->fileofs, l->filelen);
}

/*
=================
CM_LoadHulls
=================
*/
void CM_LoadHulls (void)
{
	int i, j;
	cmodel_t *bm;

//
// set up the submodels (FIXME: this is confusing)
//
	for (i=0, bm = cmap_submodels ; i<cmap_numsubmodels ; i++, bm++)
	{
		for (j=0 ; j<MAX_MAP_HULLS ; j++)
		{
			bm->hulls[j] = cmap_submodels[0].hulls[j];
		}

		bm->hulls[0].firstclipnode = bm->headnode[0];
		for (j=1 ; j<MAX_MAP_HULLS ; j++)
		{
			bm->hulls[j].firstclipnode = bm->headnode[j];
			bm->hulls[j].lastclipnode = cmap_numclipnodes-1;
		}
	}
}

/*
=================
CM_LoadMap
=================
*/
cmodel_t *CM_LoadMap (char *name, qboolean clientload)
{
	int			i;
	byte		*buffer;
	dheader_t	*header;

	if (!strcmp (name, cmap_name))
	{
		return &cmap_submodels[0];
	}

	cmap_numleafs = 0;
	cmap_numnodes = 0;
	cmap_numclipnodes = 0;
	cmap_numplanes = 0;
	cmap_numsubmodels = 0;

//
// because the world is so huge, load it one piece at a time
//
	
//
// load the file
//
	buffer = COM_LoadTempFile (name);
	if (!buffer)
		return &cmap_submodels[0];

	header = (dheader_t *)buffer;
	i = LittleLong (header->version);
	if (i != BSPVERSION)
		Host_EndGame ("CM_LoadMap: %s has wrong version number (%i should be %i)", name, i, BSPVERSION);

// swap all the lumps
	cmod_base = (byte *)header;

	for (i=0 ; i<sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

// load into heap

	CM_LoadPlanes (&header->lumps[LUMP_PLANES]);
	CM_LoadVisibility (&header->lumps[LUMP_VISIBILITY]);
	CM_LoadLeafs (&header->lumps[LUMP_LEAFS]);
	CM_LoadNodes (&header->lumps[LUMP_NODES]);
	CM_LoadClipnodes (&header->lumps[LUMP_CLIPNODES]);
	CM_LoadEntities (&header->lumps[LUMP_ENTITIES]);
	CM_LoadSubmodels (&header->lumps[LUMP_MODELS]);

	CM_LoadHulls ();
	CM_InitBoxHull ();

	memset (cmod_novis, 0xff, sizeof(cmod_novis));

	strcpy (cmap_name, name);

	return &cmap_submodels[0];
}

/*
=================
CM_LoadInlineModel

Loads a .bsp file if possible and sets the bounding box for collision detection
=================
*/
void CM_LoadInlineModel (char *name, qboolean clientload, vec3_t mins, vec3_t maxs)
{
	int			i, j;
	lump_t		*l;
	dmodel_t	*in;
	byte		*buffer;
	dheader_t	*header;

	VectorSet (mins, -15, -15, -15);
	VectorSet (maxs, 15, 15, 15);

//
// load the file
//
	buffer = COM_LoadTempFile (name);
	if (!buffer)
		return;

	header = (dheader_t *)buffer;
	i = LittleLong (header->version);
	if (i != BSPVERSION)
		return;

// swap all the lumps
	for (i=0 ; i<sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

// load into heap
	l = &header->lumps[LUMP_MODELS];
	in = (void *)(buffer + l->fileofs);
	if (l->filelen % sizeof(*in))
		Host_EndGame ("CM_LoadInlineModel: funny lump size in %s", cmap_name);

	for (j=0 ; j<3 ; j++)
	{
		mins[j] = LittleFloat (in->mins[j]) - 1;
		maxs[j] = LittleFloat (in->maxs[j]) + 1;
	}
}

/*
=================
CM_InlineModel
=================
*/
cmodel_t *CM_InlineModel (int modelnum)
{
	if (modelnum < 0 || modelnum >= cmap_numsubmodels)
		Host_EndGame ("CM_InlineModel: bad model number");

	return &cmap_submodels[modelnum];
}

/*
===============
CM_InlineModelBox
===============
*/
void CM_InlineModelBox (cmodel_t *cmodel, vec3_t mins, vec3_t maxs)
{
	if (!cmodel)
		Host_EndGame ("CM_InlineModelBox: bad model");

	VectorCopy (cmodel->mins, mins);
	VectorCopy (cmodel->maxs, maxs);
}

/*
===============
CM_InlineModelRadius
===============
*/
float CM_InlineModelRadius (cmodel_t *cmodel)
{
	if (!cmodel)
		Host_EndGame ("CM_InlineModelRadius: bad model");

	return cmodel->radius;
}

/*
=================
CM_MapName
=================
*/
char *CM_MapName (void)
{
	return cmap_name;
}

/*
===============
CM_EntitiesString
===============
*/
char *CM_EntitiesString (void)
{
	return cmap_entstring;
}

/*
===============
CM_NumLeafs
===============
*/
int CM_NumLeafs (void)
{
	return cmap_numleafs;
}

/*
===============
CM_NumSubmodels
===============
*/
int CM_NumSubmodels (void)
{
	return cmap_numsubmodels;
}

/*
===============
CM_LeafAmbientLevel
===============
*/
int CM_LeafAmbientLevel (int leafnum, int ambient)
{
	if (ambient < 0 || ambient >= NUM_AMBIENTS)
		Host_EndGame ("CM_LeafAmbientLevel: bad ambient number");
	if (leafnum < 0 || leafnum >= cmap_numleafs)
		Host_EndGame ("CM_LeafAmbientLevel: bad leaf number");

	return cmap_leafs[leafnum].ambient_sound_level[ambient];
}

/*
===============================================================================

HULL BOXES

===============================================================================
*/

static	chull_t		box_hull;
static	cmodel_t	box_cmodel;
static	cclipnode_t	box_clipnodes[6];
static	cplane_t	box_planes[6];

/*
===================
CM_InitBoxHull

Set up the planes and clipnodes so that the six floats of a bounding box
can just be stored out and get a proper hull_t structure.
===================
*/
void CM_InitBoxHull (void)
{
	int		i;
	int		side;

	box_hull.clipnodes = box_clipnodes;
	box_hull.planes = box_planes;
	box_hull.firstclipnode = 0;
	box_hull.lastclipnode = 5;

	for (i=0 ; i<6 ; i++)
	{
		box_clipnodes[i].plane = &box_planes[i];

		side = i&1;

		box_clipnodes[i].children[side] = CONTENTS_EMPTY;
		if (i != 5)
			box_clipnodes[i].children[side^1] = i + 1;
		else
			box_clipnodes[i].children[side^1] = CONTENTS_SOLID;

		box_planes[i].type = i>>1;
		box_planes[i].normal[i>>1] = 1;
	}
}

/*
===============
CM_HullForBSP

Returns a hull that can be used for testing or clipping an object of mins/maxs
size.
===============
*/
chull_t *CM_HullForBSP (cmodel_t *cmodel, const vec3_t mins, const vec3_t maxs)
{
	vec3_t size;
	chull_t *hull;

	if (!cmodel)
		Host_EndGame ("CM_HullForBSP: bad model");

	VectorSubtract (maxs, mins, size);
	if (size[0] < 3)
		hull = &cmodel->hulls[0];
	else if (size[0] <= 32)
		hull = &cmodel->hulls[1];
	else
		hull = &cmodel->hulls[2];

	return hull;
}

/*
===================
CM_TempModelForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
cmodel_t *CM_TempModelForBox (const vec3_t emins, const vec3_t emaxs, const vec3_t mins, const vec3_t maxs)
{
	vec3_t	 hullmins, hullmaxs;

	VectorSubtract (emins, maxs, hullmins);
	VectorSubtract (emaxs, mins, hullmaxs);

	VectorCopy (mins, box_hull.clip_mins);
	VectorCopy (maxs, box_hull.clip_maxs);

	box_planes[0].dist = hullmaxs[0];
	box_planes[1].dist = hullmins[0];
	box_planes[2].dist = hullmaxs[1];
	box_planes[3].dist = hullmins[1];
	box_planes[4].dist = hullmaxs[2];
	box_planes[5].dist = hullmins[2];

	return &box_cmodel;
}

/*
==================
CM_HullPointContents

==================
*/
int CM_HullPointContents (chull_t *hull, int num, vec3_t p)
{
	float		d;
	cclipnode_t	*node;
	cplane_t	*plane;

	while (num >= 0)
	{
		if (num < hull->firstclipnode || num > hull->lastclipnode)
			Host_EndGame ("CM_HullPointContents: bad node number");
	
		node = hull->clipnodes + num;
		plane = node->plane;
		d = PlaneDiff (p, plane);

		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}
	
	return num;
}

/*
===============================================================================

LINE TESTING IN HULLS

===============================================================================
*/

// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	(0.03125)

/*
==================
CM_RecursiveHullCheck

==================
*/
qboolean CM_RecursiveHullCheck (chull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t *trace)
{
	cclipnode_t	*node;
	cplane_t	*plane;
	float		t1, t2;
	float		frac;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

// check for empty
	if (num < 0)
	{
		if (num != CONTENTS_SOLID)
		{
			trace->allsolid = false;
			if (num == CONTENTS_EMPTY)
				trace->inopen = true;
			else
				trace->inwater = true;
		}
		else
			trace->startsolid = true;
		return true;		// empty
	}

	if (num < hull->firstclipnode || num > hull->lastclipnode)
		Sys_Error ("SV_RecursiveHullCheck: bad node number");

//
// find the point distances
//
	node = hull->clipnodes + num;
	plane = node->plane;

	t1 = PlaneDiff (p1, plane);
	t2 = PlaneDiff (p2, plane);
	
	if (t1 >= 0 && t2 >= 0)
		return CM_RecursiveHullCheck (hull, node->children[0], p1f, p2f, p1, p2, trace);
	if (t1 < 0 && t2 < 0)
		return CM_RecursiveHullCheck (hull, node->children[1], p1f, p2f, p1, p2, trace);

// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < 0)
		frac = (t1 + DIST_EPSILON)/(t1-t2);
	else
		frac = (t1 - DIST_EPSILON)/(t1-t2);
	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;
		
	midf = p1f + (p2f - p1f)*frac;
	LerpVectors (p1, frac, p2, mid);

	side = (t1 < 0);

// move up to the node
	if (!CM_RecursiveHullCheck (hull, node->children[side], p1f, midf, p1, mid, trace) )
		return false;

	if (CM_HullPointContents (hull, node->children[side^1], mid) != CONTENTS_SOLID)
// go past the node
		return CM_RecursiveHullCheck (hull, node->children[side^1], midf, p2f, mid, p2, trace);
	
	if (trace->allsolid)
		return false;		// never got out of the solid area
		
//==================
// the other side of the node is solid, this is the impact point
//==================
	if (!side)
	{
		VectorCopy (plane->normal, trace->plane.normal);
		trace->plane.dist = plane->dist;
	}
	else
	{
		VectorNegate (plane->normal, trace->plane.normal);
		trace->plane.dist = -plane->dist;
	}

	while (CM_HullPointContents (hull, hull->firstclipnode, mid) == CONTENTS_SOLID)
	{ // shouldn't really happen, but does occasionally
		frac -= 0.1;
		if (frac < 0)
		{
			trace->fraction = midf;
			VectorCopy (mid, trace->endpos);
			Con_DPrintf ("backup past 0\n");
			return false;
		}
		midf = p1f + (p2f - p1f)*frac;
		for (i=0 ; i<3 ; i++)
			mid[i] = p1[i] + frac*(p2[i] - p1[i]);
	}

	trace->fraction = midf;
	VectorCopy (mid, trace->endpos);

	return false;
}

/*
==================
CM_BoxTrace

==================
*/
trace_t CM_BoxTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, cmodel_t *cmodel)
{
	trace_t		trace;
	vec3_t		offset;
	vec3_t		start_l, end_l;
	chull_t		*hull;

// fill in a default trace
	memset (&trace, 0, sizeof(trace_t));
	trace.fraction = 1;
	trace.allsolid = true;
	VectorCopy (end, trace.endpos);

// get the clipping hull offset
	if (cmodel != &box_cmodel)
	{
		hull = CM_HullForBSP (cmodel, mins, maxs);
		VectorSubtract (hull->clip_mins, mins, offset);
		VectorSubtract (start, offset, start_l);
		VectorSubtract (end, offset, end_l);
	}
	else
	{
		hull = &box_hull;
		VectorCopy (start, start_l);
		VectorCopy (end, end_l);
	}

// trace a line through the apropriate clipping hull
	CM_RecursiveHullCheck (hull, hull->firstclipnode, 0, 1, start_l, end_l, &trace);

// fix trace up by the offset
	if (trace.fraction != 1)
		VectorAdd (trace.endpos, offset, trace.endpos);

	return trace;
}

/*
==================
CM_TransformedBoxTrace

==================
*/
trace_t CM_TransformedBoxTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, cmodel_t *cmodel, vec3_t origin, vec3_t angles)
{
	trace_t		trace;
	vec3_t		offset;
	vec3_t		start_l, end_l;
	chull_t		*hull;

// fill in a default trace
	memset (&trace, 0, sizeof(trace_t));
	trace.fraction = 1;
	trace.allsolid = true;
	VectorCopy (end, trace.endpos);

// get the clipping hull offset
	if (cmodel != &box_cmodel)
	{
		hull = CM_HullForBSP (cmodel, mins, maxs);
		VectorSubtract (hull->clip_mins, mins, offset);
		VectorAdd (offset, origin, offset);
	}
	else
	{
		hull = &box_hull;
		VectorCopy (origin, offset);
	}

	VectorSubtract (start, offset, start_l);
	VectorSubtract (end, offset, end_l);

// rotate start and end into the models frame of reference
	if (cmodel != &box_cmodel && (angles[0] || angles[1] || angles[2]) )
	{
		vec3_t	forward, right, up;
		vec3_t	temp;

		AngleVectors (angles, forward, right, up);

		VectorCopy (start_l, temp);
		start_l[0] = DotProduct (temp, forward);
		start_l[1] = -DotProduct (temp, right);
		start_l[2] = DotProduct (temp, up);

		VectorCopy (end_l, temp);
		end_l[0] = DotProduct (temp, forward);
		end_l[1] = -DotProduct (temp, right);
		end_l[2] = DotProduct (temp, up);
	}

// trace a line through the apropriate clipping hull
	CM_RecursiveHullCheck (hull, hull->firstclipnode, 0, 1, start_l, end_l, &trace);

// rotate endpos back to world frame of reference
	if (cmodel != &box_cmodel && (angles[0] || angles[1] || angles[2]) )
	{
		vec3_t	a;
		vec3_t	forward, right, up;
		vec3_t	temp;

		if (trace.fraction != 1)
		{
			VectorNegate (angles, a);
			AngleVectors (a, forward, right, up);

			VectorCopy (trace.endpos, temp);
			trace.endpos[0] = DotProduct (temp, forward);
			trace.endpos[1] = -DotProduct (temp, right);
			trace.endpos[2] = DotProduct (temp, up);

			VectorCopy (trace.plane.normal, temp);
			trace.plane.normal[0] = DotProduct (temp, forward);
			trace.plane.normal[1] = -DotProduct (temp, right);
			trace.plane.normal[2] = DotProduct (temp, up);
		}
	}

// fix trace up by the offset
	if (trace.fraction != 1)
		VectorAdd (trace.endpos, offset, trace.endpos);

	return trace;
}

//=========================================================================

/*
===============
CM_PointInLeaf
===============
*/
cleaf_t *CM_PointInLeaf (vec3_t p)
{
	float		d;
	cnode_t		*node;
	cplane_t	*plane;

	if (!cmap_numnodes)
		Host_EndGame ("CM_PointInLeaf: bad model");

	node = cmap_nodes;
	while (1)
	{
		if (node->contents < 0)
			return (cleaf_t *)node;

		plane = node->plane;
		d = DotProduct (p, plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;		// never reached
}

int *tl_leafs;
int tl_max_leafs;
int	tl_num_leafs;
int tl_topnode;

/*
===============
Mod_TouchedLeafs
===============
*/
void Mod_TouchedLeafs_r (cnode_t *node, vec3_t mins, vec3_t maxs)
{
	int sides;
	cplane_t *plane;

	if (tl_num_leafs >= tl_max_leafs)
		return;

	while (1)
	{
	// if this is a leaf, accumulate the pvs bits
		if (node->contents < 0)
		{
			if (node->contents != CONTENTS_SOLID)
			{
				if (tl_num_leafs < tl_max_leafs)
					tl_leafs[tl_num_leafs++] = ((cleaf_t *)node) - cmap_leafs;
			}
			return;
		}

		plane = node->plane;
		sides = BOX_ON_PLANE_SIDE (mins, maxs, plane);

		if (sides == 3)
		{	// go down both
			if (tl_topnode == -1)
				tl_topnode = node - cmap_nodes;

			Mod_TouchedLeafs_r (node->children[0], mins, maxs);
			node = node->children[1];
		}
		else if (sides == 1)
		{
			node = node->children[0];
		}
		else
		{
			node = node->children[1];
		}
	}
}

/*
===============
CM_TouchedLeafs
===============
*/
int CM_TouchedLeafs (vec3_t p, vec3_t mins, vec3_t maxs, int leafs[], int maxleafs, int *topnode)
{
	cleaf_t *leaf;
	vec3_t absmin, absmax;

	if (!cmap_numleafs || maxleafs <= 0)
		return 0;

	// special point case
	if (VectorCompare (mins, vec3_origin) && VectorCompare (maxs, vec3_origin))
	{
		leaf = CM_PointInLeaf (p);

		if (leaf)
		{
			*leafs = leaf - cmap_leafs;
			return 1;
		}

		return 0;
	}

	tl_leafs = leafs;
	tl_max_leafs = maxleafs;
	tl_num_leafs = 0;
	tl_topnode = 0;

	VectorAdd (p, mins, absmin);
	VectorAdd (p, maxs, absmax);

	Mod_TouchedLeafs_r (cmap_nodes, absmin, absmax);

	if (topnode)
		*topnode = tl_topnode;

	return tl_num_leafs;
}

/*
===============
CM_PointContents
===============
*/
int CM_PointContents (vec3_t p)
{
	float		d;
	cnode_t		*node;
	cplane_t	*plane;
	
	if (!cmap_numnodes)
		Host_EndGame ("CM_PointContents: bad model");

	node = cmap_nodes;
	while (1)
	{
		if (node->contents < 0)
			return ((cleaf_t *)node)->contents;

		plane = node->plane;
		d = DotProduct (p, plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}
	
	return 0;	// never reached
}

/*
===================
CM_DecompressVis
===================
*/
byte *CM_DecompressVis (byte *in)
{
	static byte	decompressed[MAX_MAP_LEAFS/8];
	int		c;
	byte	*out;
	int		row;

	row = (cmap_numleafs+7)>>3;	
	out = decompressed;

	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;		
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
	
	return decompressed;
}

/*
===================
CM_LeafPVS
===================
*/
byte *CM_LeafPVS (int leafnum)
{
	if (leafnum <= 0 || leafnum >= cmap_numleafs)
		return cmod_novis;
	return CM_DecompressVis (cmap_leafs[leafnum].compressed_vis);
}

qboolean CM_HeadnodeVisible_r (cnode_t *node, byte *visbits)
{
	int		leafnum;

	while (1)
	{
		if (node->contents < 0)
		{
			leafnum = ((cleaf_t *)node) - cmap_leafs - 1;
			if (visbits[leafnum>>3] & (1<<(leafnum&7)))
				return true;
			return false;
		}

		if (CM_HeadnodeVisible_r (node->children[0], visbits))
			return true;
		node = node->children[1];
	}
}

/*
=============
CM_HeadnodeVisible

Returns true if any leaf under headnode is potentially visible
=============
*/
qboolean CM_HeadnodeVisible (int nodenum, byte *visbits)
{
	if (nodenum < 0 || nodenum >= cmap_numnodes)
		Host_EndGame ("CM_HeadnodeVisible: bad node number");

	return CM_HeadnodeVisible_r (&cmap_nodes[nodenum], visbits);
}
