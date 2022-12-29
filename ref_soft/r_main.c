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
// r_main.c

#include "r_local.h"

void		*colormap;
float		r_time1;
int			r_numallocatededges;
int			r_pixbytes = 1;
int			r_pixbytes2 = 0;
int			r_outofsurfaces;
int			r_outofedges;

qboolean	r_dowarp;

mvertex_t	*r_pcurrentvertbase;

int			c_surf;
int			r_maxsurfsseen, r_maxedgesseen, r_cnumsurfs;
qboolean	r_surfsonstack;
int			r_clipflags;

byte		*r_warpbuffer;

byte		*r_stack_start;

refdef_t	r_refdef;

model_t		*r_worldmodel;

//
// view origin
//
vec3_t	vup, base_vup;
vec3_t	vpn, base_vpn;
vec3_t	vright, base_vright;
vec3_t	r_origin;

float	viewmatrix[3][4];

//
// screen size info
//
oldrefdef_t	r_oldrefdef;
float		xcenter, ycenter;
float		xscale, yscale;
float		xscaleinv, yscaleinv;
float		xscaleshrink, yscaleshrink;
float		aliasxscale, aliasyscale, aliasxcenter, aliasycenter;

int		screenwidth;

float	pixelAspect;
float	screenAspect;
float	verticalFieldOfView;
float	xOrigin, yOrigin;

cplane_t	screenedge[4];

//
// refresh flags
//
int		r_framecount = 1;	// so frame counts initialized to 0 don't match
int		r_visframecount;
int		r_polycount;
int		r_drawnpolycount;
int		r_wholepolycount;

#define		VIEWMODNAME_LENGTH	256
char		viewmodname[VIEWMODNAME_LENGTH+1];
int			modcount;

int			*pfrustum_indexes[4];
int			r_frustum_indexes[4*6];

byte		gammatable[256];	// palette is sent through this

mleaf_t		*r_viewleaf, *r_oldviewleaf;

texture_t	*r_notexture_mip;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value

float	dp_time1, dp_time2, db_time1, db_time2, rw_time1, rw_time2;
float	se_time1, se_time2, de_time1, de_time2, dv_time1, dv_time2;

void R_MarkLeaves (void);
void R_BuildGammaTable (float g);

cvar_t	r_draworder = {"r_draworder","0"};
cvar_t	r_speeds = {"r_speeds","0"};
cvar_t	r_timegraph = {"r_timegraph","0"};
cvar_t	r_graphheight = {"r_graphheight","10"};
cvar_t	r_clearcolor = {"r_clearcolor","2"};
cvar_t	r_waterwarp = {"r_waterwarp","1"};
cvar_t	r_fullbright = {"r_fullbright","0"};
cvar_t	r_drawentities = {"r_drawentities","1"};
cvar_t	r_drawviewmodel = {"r_drawviewmodel","1"};
cvar_t	r_aliasstats = {"r_polymodelstats","0"};
cvar_t	r_dspeeds = {"r_dspeeds","0"};
cvar_t	r_ambient = {"r_ambient", "0"};
cvar_t	r_reportsurfout = {"r_reportsurfout", "0"};
cvar_t	r_maxsurfs = {"r_maxsurfs", "0"};
cvar_t	r_numsurfs = {"r_numsurfs", "0"};
cvar_t	r_reportedgeout = {"r_reportedgeout", "0"};
cvar_t	r_maxedges = {"r_maxedges", "0"};
cvar_t	r_numedges = {"r_numedges", "0"};
cvar_t	r_aliastransbase = {"r_aliastransbase", "200"};
cvar_t	r_aliastransadj = {"r_aliastransadj", "100"};
cvar_t	r_skycolor = {"r_skycolor","4"};
cvar_t	r_fastsky = {"r_fastsky","0"};
cvar_t	r_loadlits = {"r_loadlits", "0"};
cvar_t	v_gamma = {"gamma", "1", true};

// Vic
cvar_t  r_lerpmodels = { "r_lerpmodels", "1", true };

/*
==================
R_InitTextures
==================
*/
void R_InitTextures (void)
{
	int		x,y, m;
	byte	*dest;
	
// create a simple checkerboard texture for the default
	r_notexture_mip = Hunk_AllocName (sizeof(texture_t) + 16*16+8*8+4*4+2*2, "notexture");
	
	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->offsets[0] = sizeof(texture_t);
	r_notexture_mip->offsets[1] = r_notexture_mip->offsets[0] + 16*16;
	r_notexture_mip->offsets[2] = r_notexture_mip->offsets[1] + 8*8;
	r_notexture_mip->offsets[3] = r_notexture_mip->offsets[2] + 4*4;
	
	for (m=0 ; m<4 ; m++)
	{
		dest = (byte *)r_notexture_mip + r_notexture_mip->offsets[m];
		for (y=0 ; y< (16>>m) ; y++)
			for (x=0 ; x< (16>>m) ; x++)
			{
				if (  (y< (8>>m) ) ^ (x< (8>>m) ) )
					*dest++ = 0;
				else
					*dest++ = 0xff;
			}
	}	
}

void R_LoadSky_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("loadsky <name> : load a skybox\n");
		return;
	}

	Cvar_Set (&r_skyname, Cmd_Argv(1));
}

/*
===============
R_Init
===============
*/
void R_Init (void)
{
	int		dummy;
	
// get stack position so we can guess if we are going to overflow
	r_stack_start = (byte *)&dummy;
	
	R_InitTurb ();
	R_InitTextures ();

	Cmd_AddCommand ("screenshot", R_ScreenShot_f);
	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);	
	Cmd_AddCommand ("loadsky", R_LoadSky_f);

	Cvar_RegisterVariable (&r_draworder);
	Cvar_RegisterVariable (&r_speeds);
	Cvar_RegisterVariable (&r_timegraph);
	Cvar_RegisterVariable (&r_graphheight);
	Cvar_RegisterVariable (&r_ambient);
	Cvar_RegisterVariable (&r_clearcolor);
	Cvar_RegisterVariable (&r_waterwarp);
	Cvar_RegisterVariable (&r_fullbright);
	Cvar_RegisterVariable (&r_drawentities);
	Cvar_RegisterVariable (&r_drawviewmodel);
	Cvar_RegisterVariable (&r_aliasstats);
	Cvar_RegisterVariable (&r_dspeeds);
	Cvar_RegisterVariable (&r_reportsurfout);
	Cvar_RegisterVariable (&r_maxsurfs);
	Cvar_RegisterVariable (&r_numsurfs);
	Cvar_RegisterVariable (&r_reportedgeout);
	Cvar_RegisterVariable (&r_maxedges);
	Cvar_RegisterVariable (&r_numedges);
	Cvar_RegisterVariable (&r_aliastransbase);
	Cvar_RegisterVariable (&r_aliastransadj);
	
	// Vic
	Cvar_RegisterVariable (&r_skyname);
	Cvar_RegisterVariable (&r_lerpmodels);
	Cvar_RegisterVariable (&r_loadlits);
	Cvar_RegisterVariable (&r_skycolor);
	Cvar_RegisterVariable (&r_fastsky);
	
	R_BuildGammaTable (1.0);	// no gamma yet

	Cvar_RegisterVariable (&v_gamma);

	Cvar_SetValue (&r_maxedges, (float)NUMSTACKEDGES);
	Cvar_SetValue (&r_maxsurfs, (float)NUMSTACKSURFACES);

	view_clipplanes[0].leftedge = true;
	view_clipplanes[1].rightedge = true;
	view_clipplanes[1].leftedge = view_clipplanes[2].leftedge =
			view_clipplanes[3].leftedge = false;
	view_clipplanes[0].rightedge = view_clipplanes[2].rightedge =
			view_clipplanes[3].rightedge = false;

	r_oldrefdef.xOrigin = XCENTERING;
	r_oldrefdef.yOrigin = YCENTERING;

// TODO: collect 386-specific code in one place
#if	id386
	Sys_MakeCodeWriteable ((long)R_EdgeCodeStart,
					     (long)R_EdgeCodeEnd - (long)R_EdgeCodeStart);
#endif	// id386

	D_Init ();
}

/*
===============
R_NewMap
===============
*/
void R_NewMap (model_t *worldmodel)
{
	r_viewleaf = NULL;
	r_worldmodel = worldmodel;

	R_InitSkyBox ();

	r_cnumsurfs = r_maxsurfs.value;

	if (r_cnumsurfs <= MINSURFACES)
		r_cnumsurfs = MINSURFACES;

	if (r_cnumsurfs > NUMSTACKSURFACES)
	{
		surfaces = Hunk_AllocName (r_cnumsurfs * sizeof(surf_t), "surfaces");
		surface_p = surfaces;
		surf_max = &surfaces[r_cnumsurfs];
		r_surfsonstack = false;
	// surface 0 doesn't really exist; it's just a dummy because index 0
	// is used to indicate no edge attached to surface
		surfaces--;
#if id386
		R_SurfacePatch ();
#endif
	}
	else
	{
		r_surfsonstack = true;
	}

	r_maxedgesseen = 0;
	r_maxsurfsseen = 0;

	r_numallocatededges = r_maxedges.value;

	if (r_numallocatededges < MINEDGES)
		r_numallocatededges = MINEDGES;

	if (r_numallocatededges <= NUMSTACKEDGES)
		auxedges = NULL;
	else
		auxedges = Hunk_AllocName (r_numallocatededges * sizeof(edge_t),
								   "edges");
}


/*
===============
R_ViewChanged

Called every time the vid structure or r_refdef changes.
Guaranteed to be called before the first refresh
===============
*/
void R_ViewChanged (vrect_t *pvrect, float aspect)
{
	int		i;

	r_oldrefdef.vrect = *pvrect;
	r_oldrefdef.horizontalFieldOfView = 2.0 * tan (r_refdef.fov_x/360*M_PI);

	r_oldrefdef.fvrectx = (float)r_oldrefdef.vrect.x;
	r_oldrefdef.fvrectx_adj = (float)r_oldrefdef.vrect.x - 0.5;
	r_oldrefdef.vrect_x_adj_shift20 = (r_oldrefdef.vrect.x<<20) + (1<<19) - 1;
	r_oldrefdef.fvrecty = (float)r_oldrefdef.vrect.y;
	r_oldrefdef.fvrecty_adj = (float)r_oldrefdef.vrect.y - 0.5;
	r_oldrefdef.vrectright = r_oldrefdef.vrect.x + r_oldrefdef.vrect.width;
	r_oldrefdef.vrectright_adj_shift20 = (r_oldrefdef.vrectright<<20) + (1<<19) - 1;
	r_oldrefdef.fvrectright = (float)r_oldrefdef.vrectright;
	r_oldrefdef.fvrectright_adj = (float)r_oldrefdef.vrectright - 0.5;
	r_oldrefdef.vrectrightedge = (float)r_oldrefdef.vrectright - 0.99;
	r_oldrefdef.vrectbottom = r_oldrefdef.vrect.y + r_oldrefdef.vrect.height;
	r_oldrefdef.fvrectbottom = (float)r_oldrefdef.vrectbottom;
	r_oldrefdef.fvrectbottom_adj = (float)r_oldrefdef.vrectbottom - 0.5;

	r_oldrefdef.aliasvrect.x = r_oldrefdef.vrect.x;
	r_oldrefdef.aliasvrect.y = r_oldrefdef.vrect.y;
	r_oldrefdef.aliasvrect.width = r_oldrefdef.vrect.width;
	r_oldrefdef.aliasvrect.height = r_oldrefdef.vrect.height;
	r_oldrefdef.aliasvrectright = r_oldrefdef.aliasvrect.x +
			r_oldrefdef.aliasvrect.width;
	r_oldrefdef.aliasvrectbottom = r_oldrefdef.aliasvrect.y +
			r_oldrefdef.aliasvrect.height;

	pixelAspect = aspect;
	xOrigin = r_oldrefdef.xOrigin;
	yOrigin = r_oldrefdef.yOrigin;
	
	screenAspect = r_oldrefdef.vrect.width*pixelAspect /
			r_oldrefdef.vrect.height;
// 320*200 1.0 pixelAspect = 1.6 screenAspect
// 320*240 1.0 pixelAspect = 1.3333 screenAspect
// proper 320*200 pixelAspect = 0.8333333

	verticalFieldOfView = r_oldrefdef.horizontalFieldOfView / screenAspect;

// values for perspective projection
// if math were exact, the values would range from 0.5 to to range+0.5
// hopefully they wll be in the 0.000001 to range+.999999 and truncate
// the polygon rasterization will never render in the first row or column
// but will definately render in the [range] row and column, so adjust the
// buffer origin to get an exact edge to edge fill
	xcenter = ((float)r_oldrefdef.vrect.width * XCENTERING) +
			r_oldrefdef.vrect.x - 0.5;
	aliasxcenter = xcenter;
	ycenter = ((float)r_oldrefdef.vrect.height * YCENTERING) +
			r_oldrefdef.vrect.y - 0.5;
	aliasycenter = ycenter;

	xscale = r_oldrefdef.vrect.width / r_oldrefdef.horizontalFieldOfView;
	aliasxscale = xscale;
	xscaleinv = 1.0 / xscale;
	yscale = xscale * pixelAspect;
	aliasyscale = yscale;
	yscaleinv = 1.0 / yscale;
	xscaleshrink = (r_oldrefdef.vrect.width-6)/r_oldrefdef.horizontalFieldOfView;
	yscaleshrink = xscaleshrink*pixelAspect;

// left side clip
	screenedge[0].normal[0] = -1.0 / (xOrigin*r_oldrefdef.horizontalFieldOfView);
	screenedge[0].normal[1] = 0;
	screenedge[0].normal[2] = 1;
	screenedge[0].type = PLANE_ANYZ;
	
// right side clip
	screenedge[1].normal[0] =
			1.0 / ((1.0-xOrigin)*r_oldrefdef.horizontalFieldOfView);
	screenedge[1].normal[1] = 0;
	screenedge[1].normal[2] = 1;
	screenedge[1].type = PLANE_ANYZ;
	
// top side clip
	screenedge[2].normal[0] = 0;
	screenedge[2].normal[1] = -1.0 / (yOrigin*verticalFieldOfView);
	screenedge[2].normal[2] = 1;
	screenedge[2].type = PLANE_ANYZ;
	
// bottom side clip
	screenedge[3].normal[0] = 0;
	screenedge[3].normal[1] = 1.0 / ((1.0-yOrigin)*verticalFieldOfView);
	screenedge[3].normal[2] = 1;	
	screenedge[3].type = PLANE_ANYZ;
	
	for (i=0 ; i<4 ; i++)
		VectorNormalize (screenedge[i].normal);

// TODO: collect 386-specific code in one place
#if	id386
	Sys_MakeCodeWriteable ((long)R_Surf8Start, (long)R_Surf8End - (long)R_Surf8Start);
	colormap = vid.colormap;
	R_Surf8Patch ();
#endif	// id386

	D_ViewChanged ();
}


/*
===============
R_MarkLeaves
===============
*/
void R_MarkLeaves (void)
{
	byte	*vis;
	mnode_t	*node;
	int		i;

	if (r_oldviewleaf == r_viewleaf)
		return;
	
	r_visframecount++;
	r_oldviewleaf = r_viewleaf;

	vis = Mod_LeafPVS (r_viewleaf, r_worldmodel);
		
	for (i=0 ; i<r_worldmodel->numleafs ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			node = (mnode_t *)&r_worldmodel->leafs[i+1];
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}

static vec3_t r_entvismins, r_entvismaxs;

/*
===================
R_VisCullEntity
===================
*/
qboolean R_VisCullEntity (mnode_t *node)
{
	int			sides;

	while (1)
	{
		if (node->contents == CONTENTS_SOLID)
			return false;
		if (node->visframe != r_visframecount)
			return false;
		if (node->contents < 0)
			return true;

		sides = BOX_ON_PLANE_SIDE (r_entvismins, r_entvismaxs, node->plane);
		if (sides == 3)
		{	// go down both
			if (R_VisCullEntity (node->children[0]))
				return true;
			node = node->children[1];
		}
		else if (sides == 1)
			node = node->children[0];
		else
			node = node->children[1];
	}
}

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int			i;

	if (!r_drawentities.value)
		return;

	for (i=0 ; i<r_refdef.num_entities ; i++)
	{
		currententity = &r_refdef.entities[i];
		currentmodel = currententity->model;
		if (!currentmodel)
			continue;

		if ((currententity->flags & RF_WEAPONMODEL) && !r_drawviewmodel.value)
			continue;
		if (currententity->flags & RF_CULLVIS)
		{
			VectorAdd (currententity->origin, currententity->model->mins, r_entvismins);
			VectorAdd (currententity->origin, currententity->model->maxs, r_entvismaxs);

			if (!R_VisCullEntity (r_worldmodel->nodes))
				continue;
		}

		switch (currentmodel->type)
		{
			case mod_sprite:
				VectorCopy (currententity->origin, r_entorigin);
				VectorSubtract (r_origin, r_entorigin, modelorg);
				R_DrawSprite ();
				break;

			case mod_alias:
				VectorCopy (currententity->origin, r_entorigin);
				VectorSubtract (r_origin, r_entorigin, modelorg);
				R_DrawAliasModel ();		
				break;

			default:
				break;
		}
	}
}

/*
=============
R_BmodelCheckBBox
=============
*/
int R_BmodelCheckBBox (float *minmaxs)
{
	int			i, *pindex, clipflags;
	vec3_t		acceptpt, rejectpt;
	double		d;

	clipflags = 0;

	if (currententity->angles[0] || currententity->angles[1]
		|| currententity->angles[2])
	{
		for (i=0 ; i<4 ; i++)
		{
			d = DotProduct (currententity->origin, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;

			if (d <= -r_worldmodel->radius)
				return BMODEL_FULLY_CLIPPED;

			if (d <= r_worldmodel->radius)
				clipflags |= (1<<i);
		}
	}
	else
	{
		for (i=0 ; i<4 ; i++)
		{
		// generate accept and reject points
		// FIXME: do with fast look-ups or integer tests based on the sign bit
		// of the floating point values

			pindex = pfrustum_indexes[i];

			rejectpt[0] = minmaxs[pindex[0]];
			rejectpt[1] = minmaxs[pindex[1]];
			rejectpt[2] = minmaxs[pindex[2]];
			
			d = DotProduct (rejectpt, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;

			if (d <= 0)
				return BMODEL_FULLY_CLIPPED;

			acceptpt[0] = minmaxs[pindex[3+0]];
			acceptpt[1] = minmaxs[pindex[3+1]];
			acceptpt[2] = minmaxs[pindex[3+2]];

			d = DotProduct (acceptpt, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;

			if (d <= 0)
				clipflags |= (1<<i);
		}
	}

	return clipflags;
}

/*
===================
R_FindTopNode
===================
*/
mnode_t *R_FindTopNode (vec3_t mins, vec3_t maxs)
{
	cplane_t	*splitplane;
	int			sides;
	mnode_t		*node = r_worldmodel->nodes;

	while (1)
	{
		if (node->visframe != r_visframecount)
			return NULL;		// not visible at all
		
		if (node->contents < 0)
		{
			if (node->contents != CONTENTS_SOLID)
				return node; // we've reached a non-solid leaf, so it's
							//  visible and not BSP clipped
			return NULL;	// in solid, so not visible
		}
		
		splitplane = node->plane;
		sides = BOX_ON_PLANE_SIDE (mins, maxs, splitplane);
		
		if (sides == 3)
			return node;	// this is the splitter
		
	// not split yet; recurse down the contacted side
		if (sides & 1)
			node = node->children[0];
		else
			node = node->children[1];
	}
}

/*
=============
R_DrawBEntitiesOnList
=============
*/
void R_DrawBEntitiesOnList (void)
{
	int			i, j, k, clipflags;
	dlight_t	*l;
	vec3_t		oldorigin;
	float		minmaxs[6];
	mnode_t		*topnode;
	qboolean	rotated;

	if (!r_drawentities.value)
		return;

	VectorCopy (modelorg, oldorigin);
	insubmodel = true;
	r_dlightframecount = r_framecount;

	for (i=0 ; i<r_refdef.num_entities ; i++)
	{
		currententity = &r_refdef.entities[i];
		currentmodel = currententity->model;
		if (!currentmodel)
			continue;

		if (currentmodel->type != mod_brush)
			continue;
		
		// see if the bounding box lets us trivially reject, also sets
		// trivial accept status
		if (currententity->angles[0] || currententity->angles[1] || currententity->angles[2])
		{
			rotated = true;

			for (j=0 ; j<3 ; j++)
			{
				minmaxs[j] = currententity->origin[j] -	currentmodel->radius;
				minmaxs[3+j] = currententity->origin[j] + currentmodel->radius;
			}
		}
		else 
		{
			rotated = false;

			for (j=0 ; j<3 ; j++)
			{
				minmaxs[j] = currententity->origin[j] +	currentmodel->mins[j];
				minmaxs[3+j] = currententity->origin[j] + currentmodel->maxs[j];
			}
		}
		
		clipflags = R_BmodelCheckBBox (minmaxs);
		
		if (clipflags == BMODEL_FULLY_CLIPPED)
			continue;

		topnode = R_FindTopNode (minmaxs, minmaxs+3);
		
		if (!topnode)
			continue;

		VectorCopy (currententity->origin, r_entorigin);
		VectorSubtract (r_origin, r_entorigin, modelorg);

		r_pcurrentvertbase = currentmodel->vertexes;
		
		// FIXME: stop transforming twice
		R_RotateBmodel ();
		
		// calculate dynamic lighting for bmodel
		if (rotated)
		{
			vec3_t temp;
			vec3_t axis[3];
			
			AngleVectors (currententity->angles, axis[0], axis[1], axis[2]);
			
			l = r_refdef.dlights;
			for (k=0 ; k<r_refdef.num_dlights ; k++, l++)
			{
				VectorSubtract (l->origin, currententity->origin, temp);
				l->origin[0] = DotProduct (temp, axis[0]);
				l->origin[1] = -DotProduct (temp, axis[1]);
				l->origin[2] = DotProduct (temp, axis[2]);
				R_MarkLights (l, 1<<k, currentmodel->nodes + currentmodel->headnode);
				VectorAdd (temp, currententity->origin, l->origin);
			}
		}
		else
		{
			l = r_refdef.dlights;
			for (k=0 ; k<r_refdef.num_dlights ; k++, l++)
			{
				VectorSubtract (l->origin, currententity->origin, l->origin);
				R_MarkLights (l, 1<<k, currentmodel->nodes + currentmodel->headnode);
				VectorAdd (l->origin, currententity->origin, l->origin);
			}
		}

		if (topnode->contents >= 0)
		{
			// not a leaf; has to be clipped to the world BSP
			r_clipflags = clipflags;
			R_DrawSolidClippedSubmodelPolygons (currentmodel, topnode);
		}
		else
		{
			// falls entirely in one leaf, so we just put all the
			// edges in the edge list and let 1/z sorting handle
			// drawing order
			R_DrawSubmodelPolygons (currentmodel, clipflags, topnode);
		}

		// put back world rotation and frustum clipping		
		// FIXME: R_RotateBmodel should just work off base_vxx
		VectorCopy (base_vpn, vpn);
		VectorCopy (base_vup, vup);
		VectorCopy (base_vright, vright);
		VectorCopy (oldorigin, modelorg);
		
		VectorCopy (vright, viewmatrix[0]);
		VectorNegate (vup, viewmatrix[1]);
		VectorCopy (vpn, viewmatrix[2]);
		
		R_TransformFrustum ();
	}

	insubmodel = false;
}

/*
================
R_DrawParticles
================
*/
void R_DrawParticles (void)
{
	int				i;
	particle_t		*p;

	if (!r_refdef.num_particles)
		return;

	VectorScale (vright, xscaleshrink, r_pright);
	VectorScale (vup, yscaleshrink, r_pup);
	VectorCopy (vpn, r_ppn);

	for (i = 0, p = r_refdef.particles; i < r_refdef.num_particles; i++, p++)
		D_DrawParticle (p);
}

/*
================
R_BuildGammaTable
================
*/
void R_BuildGammaTable (float g)
{
	int		i, inf;
	
	if (g == 1.0)
	{
		for (i=0 ; i<256 ; i++)
			gammatable[i] = i;
		return;
	}
	
	for (i=0 ; i<256 ; i++)
	{
		inf = (int)(255.0 * pow ( ((float)i+0.5)/255.5 , g ) + 0.5);
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		gammatable[i] = inf;
	}
}

/*
=================
R_CheckGamma
=================
*/
qboolean R_CheckGamma (void)
{
	static float oldgammavalue;
	
	if (v_gamma.value == oldgammavalue)
		return false;
	oldgammavalue = v_gamma.value;
	
	R_BuildGammaTable (v_gamma.value);
	
	return true;
}

/*
=================
R_UpdatePalette
=================
*/
void R_UpdatePalette (void)
{
	int		i, j;
	byte	*basepal, *newpal;
	byte	pal[768];
	int		r, g, b;
	qboolean force;

	force = R_CheckGamma ();
	if (!r_refdef.num_cshifts && !force)
		return;
			
	basepal = host_basepal;
	newpal = pal;
	
	for (i=0 ; i<256 ; i++)
	{
		r = basepal[0];
		g = basepal[1];
		b = basepal[2];
		basepal += 3;
	
		for (j=0 ; j<r_refdef.num_cshifts ; j++)	
		{
			r += (r_refdef.cshifts[j].percent*(r_refdef.cshifts[j].destcolor[0]-r))>>8;
			g += (r_refdef.cshifts[j].percent*(r_refdef.cshifts[j].destcolor[1]-g))>>8;
			b += (r_refdef.cshifts[j].percent*(r_refdef.cshifts[j].destcolor[2]-b))>>8;
		}
		
		newpal[0] = gammatable[r];
		newpal[1] = gammatable[g];
		newpal[2] = gammatable[b];
		newpal += 3;
	}

	VID_ShiftPalette (pal);	
}

/*
================
R_EdgeDrawing
================
*/
void R_EdgeDrawing (void)
{
	edge_t	ledges[NUMSTACKEDGES +
			((CACHE_SIZE - 1) / sizeof(edge_t)) + 1];
	surf_t	lsurfs[NUMSTACKSURFACES +
			((CACHE_SIZE - 1) / sizeof(surf_t)) + 1];

	if (r_refdef.flags & RDF_NOWORLDMODEL)
		return;

	if (auxedges)
		r_edges = auxedges;
	else
		r_edges =  (edge_t *)
				(((long)&ledges[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));

	if (r_surfsonstack)
	{
		surfaces =  (surf_t *)
				(((long)&lsurfs[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
		surf_max = &surfaces[r_cnumsurfs];
	// surface 0 doesn't really exist; it's just a dummy because index 0
	// is used to indicate no edge attached to surface
		surfaces--;
#if id386
		R_SurfacePatch ();
#endif
	}

	R_BeginEdgeFrame ();

	if (r_dspeeds.value)
	{
		rw_time1 = Sys_FloatTime ();
	}

	R_RenderWorld ();

	if (r_dspeeds.value)
	{
		rw_time2 = Sys_FloatTime ();
		db_time1 = rw_time2;
	}

	R_DrawBEntitiesOnList ();

	if (r_dspeeds.value)
	{
		db_time2 = Sys_FloatTime ();
		se_time1 = db_time2;
	}

	if (!r_dspeeds.value)
	{
		VID_UnlockBuffer ();
		S_ExtraUpdate ();	// don't let sound get messed up if going slow
		VID_LockBuffer ();
	}
	
	R_ScanEdges ();
}

/*
================
R_BeginFrame

================
*/
void R_BeginFrame (void)
{
	VID_LockBuffer ();
}

/*
================
R_EndFrame

================
*/
void R_EndFrame (void)
{
	vrect_t		vrect;

	VID_UnlockBuffer ();

	R_UpdatePalette ();

//
// update the whole screen
//
	vrect.x = 0;
	vrect.y = 0;
	vrect.width = vid.width;
	vrect.height = vid.height;
	vrect.pnext = 0;

	VID_Update (&vrect);
}

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (refdef_t *refdef)
{
	byte	warpbuffer[WARP_WIDTH * WARP_HEIGHT];

	r_refdef = *refdef;
	r_warpbuffer = warpbuffer;

	if (r_timegraph.value || r_speeds.value || r_dspeeds.value)
		r_time1 = Sys_FloatTime ();

	R_SetupFrame ();

	R_PushDlights ();

	R_MarkLeaves ();	// done here so we know if we're in water

	if (!r_worldmodel && !(r_refdef.flags & RDF_NOWORLDMODEL))
		Sys_Error ("R_RenderView: NULL worldmodel");
		
	if (!r_dspeeds.value)
	{
		VID_UnlockBuffer ();
		S_ExtraUpdate ();	// don't let sound get messed up if going slow
		VID_LockBuffer ();
	}
	
	R_EdgeDrawing ();

	if (!r_dspeeds.value)
	{
		VID_UnlockBuffer ();
		S_ExtraUpdate ();	// don't let sound get messed up if going slow
		VID_LockBuffer ();
	}
	
	if (r_dspeeds.value)
	{
		se_time2 = Sys_FloatTime ();
		de_time1 = se_time2;
	}

	R_DrawEntitiesOnList ();

	if (r_dspeeds.value)
	{
		de_time2 = Sys_FloatTime ();
		dp_time1 = Sys_FloatTime ();
	}

	R_DrawParticles ();

	if (r_dspeeds.value)
		dp_time2 = Sys_FloatTime ();

	if (r_dowarp)
		D_WarpScreen ();

	if (r_timegraph.value)
		R_TimeGraph ();

	if (r_aliasstats.value)
		R_PrintAliasStats ();
		
	if (r_speeds.value)
		R_PrintTimes ();

	if (r_dspeeds.value)
		R_PrintDSpeeds ();

	if (r_reportsurfout.value && r_outofsurfaces)
		Con_Printf ("Short %d surfaces\n", r_outofsurfaces);

	if (r_reportedgeout.value && r_outofedges)
		Con_Printf ("Short roughly %d edges\n", r_outofedges * 2 / 3);
}

/*
================
R_InitTurb
================
*/
void R_InitTurb (void)
{
	int		i;
	
	for (i=0 ; i<(SIN_BUFFER_SIZE) ; i++)
	{
		sintable[i] = AMP + sin(i*3.14159*2/CYCLE)*AMP;
		intsintable[i] = AMP2 + sin(i*3.14159*2/CYCLE)*AMP2;	// AMP2, not 20
	}
}

