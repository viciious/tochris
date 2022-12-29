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
// r_alias.c: routines for setting up to draw alias models

#include "r_local.h"

#define LIGHT_MIN	5		// lowest light value we'll allow, to avoid the
							//  need for inner-loop light clamping

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "anorm_dots.h"
;

int				r_shadedots_quant;
float			*r_shadedots;

mtriangle_t		*ptriangles;
affinetridesc_t	r_affinetridesc;

void			*acolormap;	// FIXME: should go away

trivertx_t		*r_apverts;
trivertx_t		*r_oldapverts;
float			r_framelerp;

// TODO: these probably will go away with optimized rasterization
mdl_t				*pmdl;
int					r_ambientlight;
float				r_shadelight;
aliashdr_t			*paliashdr;
finalvert_t			*pfinalverts;
auxvert_t			*pauxverts;
static float		ziscale;
static model_t		*pmodel;

static vec3_t		alias_forward, alias_right, alias_up;

static maliasskindesc_t	*pskindesc;

int				r_amodels_drawn;
int				r_anumverts;

float	aliastransform[3][4];

#define R_AliasTransformVector(in,out) \
	(((out)[0]=DotProduct((in),aliastransform[0])+aliastransform[0][3]),	\
	((out)[1]=DotProduct((in),aliastransform[1])+aliastransform[1][3]),		\
	((out)[2]=DotProduct((in),aliastransform[2])+aliastransform[2][3]))

typedef struct {
	int	index0;
	int	index1;
} aedge_t;

static aedge_t	aedges[12] = {
{0, 1}, {1, 2}, {2, 3}, {3, 0},
{4, 5}, {5, 6}, {6, 7}, {7, 4},
{0, 5}, {1, 4}, {2, 7}, {3, 6}
};

void R_AliasTransformAndProjectFinalVerts (finalvert_t *fv,
	stvert_t *pstverts);
void R_AliasSetUpTransform (int trivial_accept);
void R_AliasTransformFinalVert (finalvert_t *fv, auxvert_t *av,
	trivertx_t *pverts1, trivertx_t *pverts2, stvert_t *pstverts);
void R_AliasProjectFinalVert (finalvert_t *fv, auxvert_t *av);

void R_AliasSetTransform (int trivial_accept);

/*
================
R_AliasFrameBoundingBox
================
*/
void R_AliasFrameBoundingBox (int frame, float time, trivertx_t **mins, trivertx_t **maxs)
{
	int	i, numposes;
	maliasgroup_t *paliasgroup;
	float *pintervals, fullinterval, targettime;

	if (paliashdr->frames[frame].type == ALIAS_SINGLE)
	{
		*mins = &paliashdr->frames[frame].bboxmin;
		*maxs = &paliashdr->frames[frame].bboxmax;
	}
	else
	{
		paliasgroup = (maliasgroup_t *)
			((byte *)paliashdr + paliashdr->frames[frame].frame);
		pintervals = (float *)((byte *)paliashdr + paliasgroup->intervals);
		numposes = paliasgroup->frames[frame].numframes;
		fullinterval = pintervals[numposes-1];

	//
	// when loading in Mod_LoadAliasGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
	//
		targettime = time - ((int)(time / fullinterval)) * fullinterval;

		for (i=0 ; i<(numposes-1) ; i++)
		{
			if (pintervals[i] > targettime)
				break;
		}
		
		*mins = &paliasgroup->frames[i].bboxmin;
		*maxs = &paliasgroup->frames[i].bboxmax;
	}
}

/*
================
R_AliasCheckBBox
================
*/
int R_AliasCheckBBox (void)
{
	int					i, flags, frame, oldframe, numv;
	float				zi, basepts[8][3], v0, v1, frac;
	finalvert_t			*pv0, *pv1, viewpts[16];
	auxvert_t			*pa0, *pa1, viewaux[16];
	qboolean			zclipped, zfullyclipped;
	unsigned			anyclip, allclip;
	int					minz, trivial_accept;
	trivertx_t			*mins, *maxs, *oldmins, *oldmaxs;

// expand, rotate, and translate points into worldspace
	trivial_accept = 0;
	pmodel = currententity->model;

	R_AliasSetUpTransform (0);

// construct the base bounding box for this frame
	frame = currententity->frame;
	oldframe = currententity->oldframe;
// TODO: don't repeat this check when drawing?
	if ((frame >= pmdl->numframes) || (frame < 0))
	{
		Con_DPrintf ("No such frame %d %s\n", frame,
				pmodel->name);
		frame = 0;
	}
	if ((oldframe >= pmdl->numframes) || (oldframe < 0))
	{
		Con_DPrintf ("No such frame %d %s\n", oldframe,
				pmodel->name);
		oldframe = 0;
	}

	if (r_lerpmodels.value) 
	{
		R_AliasFrameBoundingBox (frame, r_refdef.time+currententity->syncbase, &mins, &maxs);
		R_AliasFrameBoundingBox (oldframe, r_refdef.oldtime+currententity->syncbase, &oldmins, &oldmaxs);

	// x worldspace coordinates
		basepts[0][0] = basepts[1][0] = basepts[2][0] = basepts[3][0] =	(float)min(mins->v[0], oldmins->v[0]);
		basepts[4][0] = basepts[5][0] = basepts[6][0] = basepts[7][0] =	(float)max(maxs->v[0], oldmaxs->v[0]);

	// y worldspace coordinates
		basepts[0][1] = basepts[3][1] = basepts[5][1] = basepts[6][1] =	(float)min(mins->v[1], oldmins->v[1]);
		basepts[1][1] = basepts[2][1] = basepts[4][1] = basepts[7][1] =	(float)max(maxs->v[1], oldmaxs->v[1]);

	// z worldspace coordinates
		basepts[0][2] = basepts[1][2] = basepts[4][2] = basepts[5][2] =	(float)min(mins->v[2], oldmins->v[2]);
		basepts[2][2] = basepts[3][2] = basepts[6][2] = basepts[7][2] = (float)max(maxs->v[2], oldmaxs->v[2]);
	}
	else
	{
		R_AliasFrameBoundingBox (frame, r_refdef.time+currententity->syncbase, &mins, &maxs);

	// x worldspace coordinates
		basepts[0][0] = basepts[1][0] = basepts[2][0] = basepts[3][0] =	(float)mins->v[0];
		basepts[4][0] = basepts[5][0] = basepts[6][0] = basepts[7][0] = (float)maxs->v[0];

	// y worldspace coordinates
		basepts[0][1] = basepts[3][1] = basepts[5][1] = basepts[6][1] =	(float)mins->v[1];
		basepts[1][1] = basepts[2][1] = basepts[4][1] = basepts[7][1] = (float)maxs->v[1];

	// z worldspace coordinates
		basepts[0][2] = basepts[1][2] = basepts[4][2] = basepts[5][2] = (float)mins->v[2];
		basepts[2][2] = basepts[3][2] = basepts[6][2] = basepts[7][2] = (float)maxs->v[2];
	}

	zclipped = false;
	zfullyclipped = true;

	minz = 9999;
	for (i=0; i<8 ; i++)
	{
		R_AliasTransformVector (&basepts[i][0], &viewaux[i].fv[0]);

		if (viewaux[i].fv[2] < ALIAS_Z_CLIP_PLANE)
		{
		// we must clip points that are closer than the near clip plane
			viewpts[i].flags = ALIAS_Z_CLIP;
			zclipped = true;
		}
		else
		{
			if (viewaux[i].fv[2] < minz)
				minz = viewaux[i].fv[2];
			viewpts[i].flags = 0;
			zfullyclipped = false;
		}
	}

	if (zfullyclipped)
		return -1;	// everything was near-z-clipped

	numv = 8;

	if (zclipped)
	{
	// organize points by edges, use edges to get new points (possible trivial
	// reject)
		for (i=0 ; i<12 ; i++)
		{
		// edge endpoints
			pv0 = &viewpts[aedges[i].index0];
			pv1 = &viewpts[aedges[i].index1];
			pa0 = &viewaux[aedges[i].index0];
			pa1 = &viewaux[aedges[i].index1];

		// if one end is clipped and the other isn't, make a new point
			if (pv0->flags ^ pv1->flags)
			{
				frac = (ALIAS_Z_CLIP_PLANE - pa0->fv[2]) /
					   (pa1->fv[2] - pa0->fv[2]);
				viewaux[numv].fv[0] = pa0->fv[0] +
						(pa1->fv[0] - pa0->fv[0]) * frac;
				viewaux[numv].fv[1] = pa0->fv[1] +
						(pa1->fv[1] - pa0->fv[1]) * frac;
				viewaux[numv].fv[2] = ALIAS_Z_CLIP_PLANE;
				viewpts[numv].flags = 0;
				numv++;
			}
		}
	}

// project the vertices that remain after clipping
	anyclip = 0;
	allclip = ALIAS_XY_CLIP_MASK;

// TODO: probably should do this loop in ASM, especially if we use floats
	for (i=0 ; i<numv ; i++)
	{
	// we don't need to bother with vertices that were z-clipped
		if (viewpts[i].flags & ALIAS_Z_CLIP)
			continue;

		zi = 1.0 / viewaux[i].fv[2];

	// FIXME: do with chop mode in ASM, or convert to float
		v0 = (viewaux[i].fv[0] * xscale * zi) + xcenter;
		v1 = (viewaux[i].fv[1] * yscale * zi) + ycenter;

		flags = 0;

		if (v0 < r_oldrefdef.fvrectx)
			flags |= ALIAS_LEFT_CLIP;
		if (v1 < r_oldrefdef.fvrecty)
			flags |= ALIAS_TOP_CLIP;
		if (v0 > r_oldrefdef.fvrectright)
			flags |= ALIAS_RIGHT_CLIP;
		if (v1 > r_oldrefdef.fvrectbottom)
			flags |= ALIAS_BOTTOM_CLIP;

		anyclip |= flags;
		allclip &= flags;
	}

	if (allclip)
		return -1;	// trivial reject off one side

	return !anyclip & !zclipped;
}

/*
================
R_AliasPreparePoints

General clipped case
================
*/
void R_AliasPreparePoints (void)
{
	int			i;
	stvert_t	*pstverts;
	finalvert_t	*fv;
	auxvert_t	*av;
	mtriangle_t	*ptri;
	finalvert_t	*pfv[3];

	pstverts = (stvert_t *)((byte *)paliashdr + paliashdr->stverts);
	r_anumverts = pmdl->numverts;
 	fv = pfinalverts;
	av = pauxverts;

	for (i=0 ; i<r_anumverts ; i++, fv++, av++, r_oldapverts++, r_apverts++, pstverts++)
	{
		R_AliasTransformFinalVert (fv, av, r_oldapverts, r_apverts, pstverts);
		if (av->fv[2] < ALIAS_Z_CLIP_PLANE)
			fv->flags |= ALIAS_Z_CLIP;
		else
		{
			 R_AliasProjectFinalVert (fv, av);

			if (fv->v[0] < r_oldrefdef.aliasvrect.x)
				fv->flags |= ALIAS_LEFT_CLIP;
			if (fv->v[1] < r_oldrefdef.aliasvrect.y)
				fv->flags |= ALIAS_TOP_CLIP;
			if (fv->v[0] > r_oldrefdef.aliasvrectright)
				fv->flags |= ALIAS_RIGHT_CLIP;
			if (fv->v[1] > r_oldrefdef.aliasvrectbottom)
				fv->flags |= ALIAS_BOTTOM_CLIP;	
		}
	}

//
// clip and draw all triangles
//
	r_affinetridesc.numtriangles = 1;

	ptri = (mtriangle_t *)((byte *)paliashdr + paliashdr->triangles);
	for (i=0 ; i<pmdl->numtris ; i++, ptri++)
	{
		pfv[0] = &pfinalverts[ptri->vertindex[0]];
		pfv[1] = &pfinalverts[ptri->vertindex[1]];
		pfv[2] = &pfinalverts[ptri->vertindex[2]];

		if ( pfv[0]->flags & pfv[1]->flags & pfv[2]->flags & (ALIAS_XY_CLIP_MASK | ALIAS_Z_CLIP) )
			continue;		// completely clipped
		
		if ( ! ( (pfv[0]->flags | pfv[1]->flags | pfv[2]->flags) &
			(ALIAS_XY_CLIP_MASK | ALIAS_Z_CLIP) ) )
		{	// totally unclipped
			r_affinetridesc.pfinalverts = pfinalverts;
			r_affinetridesc.ptriangles = ptri;
			D_PolysetDraw ();
		}
		else		
		{	// partially clipped
			R_AliasClipTriangle (ptri);
		}
	}
}

void R_AliasSetUpTransform (int trivial_accept) 
{
	int	  i;
	vec3_t angles;
	float rotationmatrix[3][4];

// TODO: should really be stored with the entity instead of being reconstructed
// TODO: should use a look-up table
// TODO: could cache lazily, stored in the entity

	angles[ROLL] = currententity->angles[ROLL];
	angles[PITCH] = -currententity->angles[PITCH];
	angles[YAW] = currententity->angles[YAW];
	AngleVectors (angles, alias_forward, alias_right, alias_up);

	for (i = 0; i < 3; i++)
	{
		rotationmatrix[i][0] = alias_forward[i] * pmdl->scale[0];
		rotationmatrix[i][1] = -alias_right[i] * pmdl->scale[1];
		rotationmatrix[i][2] = alias_up[i] * pmdl->scale[2];
		rotationmatrix[i][3] = alias_forward[i] * pmdl->scale_origin[0] -
			alias_right[i] * pmdl->scale_origin[1] + 
			alias_up[i] * pmdl->scale_origin[2] - modelorg[i];
	}

	VectorCopy (vright, viewmatrix[0]);
	VectorCopy (vup, viewmatrix[1]);
	VectorInverse (viewmatrix[1]);
	VectorCopy (vpn, viewmatrix[2]);

	R_ConcatTransforms (viewmatrix, rotationmatrix, aliastransform);

// do the scaling up of x and y to screen coordinates as part of the transform
// for the unclipped case (it would mess up clipping in the clipped case).
// Also scale down z, so 1/z is scaled 31 bits for free, and scale down x and y
// correspondingly so the projected x and y come out right
// FIXME: make this work for clipped case too?
	if (trivial_accept)
	{
		for (i=0 ; i<4 ; i++)
		{
			aliastransform[0][i] *= aliasxscale *
					(1.0 / ((float)0x8000 * 0x10000));
			aliastransform[1][i] *= aliasyscale *
					(1.0 / ((float)0x8000 * 0x10000));
			aliastransform[2][i] *= 1.0 / ((float)0x8000 * 0x10000);

		}
	}
}

/*
================
R_AliasTransformFinalVert
================
*/
void R_AliasTransformFinalVert (finalvert_t *fv, auxvert_t *av,
	trivertx_t *pvert1, trivertx_t *pvert2, stvert_t *pstverts)
{
	float temp;
	float v[3];

	v[0] = pvert1->v[0] + r_framelerp * ((float)pvert2->v[0] - (float)pvert1->v[0]);
	v[1] = pvert1->v[1] + r_framelerp * ((float)pvert2->v[1] - (float)pvert1->v[1]);
	v[2] = pvert1->v[2] + r_framelerp * ((float)pvert2->v[2] - (float)pvert1->v[2]);

	R_AliasTransformVector (v, av->fv);

	fv->v[2] = pstverts->s;
	fv->v[3] = pstverts->t;

	fv->flags = pstverts->onseam;

// lighting
	temp = r_shadedots[pvert1->lightnormalindex] + r_framelerp * (r_shadedots[pvert2->lightnormalindex] - r_shadedots[pvert1->lightnormalindex]);
	fv->v[4] = r_ambientlight - temp * r_shadelight;
	if (fv->v[4] < 0)
		fv->v[4] = 0;
}

#if !id386
/*
================
R_AliasTransformAndProjectFinalVerts
================
*/
void R_AliasTransformAndProjectFinalVerts (finalvert_t *fv, stvert_t *pstverts)
{
	int			i;
	float		zi, temp;
	trivertx_t	*pvert1 = r_oldapverts, *pvert2 = r_apverts;
	float		v[3];

	for (i=0 ; i<r_anumverts ; i++, fv++, pvert1++, pvert2++, pstverts++)
	{
		v[0] = pvert1->v[0] + r_framelerp * ((float)pvert2->v[0] - (float)pvert1->v[0]);
		v[1] = pvert1->v[1] + r_framelerp * ((float)pvert2->v[1] - (float)pvert1->v[1]);
		v[2] = pvert1->v[2] + r_framelerp * ((float)pvert2->v[2] - (float)pvert1->v[2]);

	// transform and project
		zi = 1.0 / (DotProduct(v, aliastransform[2]) +
				aliastransform[2][3]);

	// x, y, and z are scaled down by 1/2**31 in the transform, so 1/z is
	// scaled up by 1/2**31, and the scaling cancels out for x and y in the
	// projection
		fv->v[5] = zi;

		fv->v[0] = ((DotProduct(v, aliastransform[0]) +
				aliastransform[0][3]) * zi) + aliasxcenter;
		fv->v[1] = ((DotProduct(v, aliastransform[1]) +
				aliastransform[1][3]) * zi) + aliasycenter;

		fv->v[2] = pstverts->s;
		fv->v[3] = pstverts->t;
		fv->flags = pstverts->onseam;

// lighting
		temp = r_shadedots[pvert1->lightnormalindex] + r_framelerp * (r_shadedots[pvert2->lightnormalindex] - r_shadedots[pvert1->lightnormalindex]);
		fv->v[4] = r_ambientlight - temp * r_shadelight;
		if (fv->v[4] < 0)
			fv->v[4] = 0;
	}
}
#endif

/*
================
R_AliasProjectFinalVert
================
*/
void R_AliasProjectFinalVert (finalvert_t *fv, auxvert_t *av)
{
	float	zi;

// project points
	zi = 1.0 / av->fv[2];

	fv->v[5] = zi * ziscale;

	fv->v[0] = (av->fv[0] * aliasxscale * zi) + aliasxcenter;
	fv->v[1] = (av->fv[1] * aliasyscale * zi) + aliasycenter;
}


/*
================
R_AliasPrepareUnclippedPoints
================
*/
void R_AliasPrepareUnclippedPoints (void)
{
	stvert_t	*pstverts;

	pstverts = (stvert_t *)((byte *)paliashdr + paliashdr->stverts);
	r_anumverts = pmdl->numverts;

	R_AliasTransformAndProjectFinalVerts (pfinalverts, pstverts);

	r_affinetridesc.pfinalverts = pfinalverts;
	r_affinetridesc.ptriangles = (mtriangle_t *)
			((byte *)paliashdr + paliashdr->triangles);
	r_affinetridesc.numtriangles = pmdl->numtris;

	D_PolysetDraw ();
}

/*
===============
R_AliasSetupSkin
===============
*/
void R_AliasSetupSkin (void)
{
	int					skinnum;
	int					i, numskins;
	maliasskingroup_t	*paliasskingroup;
	float				*pskinintervals, fullskininterval;
	float				skintargettime, skintime;

	skinnum = currententity->skinnum;
	if ((skinnum >= pmdl->numskins) || (skinnum < 0))
	{
		Con_DPrintf ("R_AliasSetupSkin: no such skin # %d\n", skinnum);
		skinnum = 0;
	}

	pskindesc = ((maliasskindesc_t *)
			((byte *)paliashdr + paliashdr->skindesc)) + skinnum;

	if (pskindesc->type == ALIAS_SKIN_GROUP)
	{
		paliasskingroup = (maliasskingroup_t *)((byte *)paliashdr +
				pskindesc->skin);
		pskinintervals = (float *)
				((byte *)paliashdr + paliasskingroup->intervals);
		numskins = paliasskingroup->numskins;
		fullskininterval = pskinintervals[numskins-1];
	
		if (currententity->model->synctype == ST_RAND)
			skintime = r_refdef.time + currententity->syncbase;
		else
			skintime = r_refdef.time;

	// when loading in Mod_LoadAliasSkinGroup, we guaranteed all interval
	// values are positive, so we don't have to worry about division by 0
		skintargettime = skintime -
				((int)(skintime / fullskininterval)) * fullskininterval;
	
		for (i=0 ; i<(numskins-1) ; i++)
		{
			if (pskinintervals[i] > skintargettime)
				break;
		}
	
		pskindesc = &paliasskingroup->skindescs[i];
	}

	r_affinetridesc.pskindesc = pskindesc;
	r_affinetridesc.pskin = (void *)((byte *)paliashdr + pskindesc->skin);
	r_affinetridesc.skinwidth = pmdl->skinwidth;
	r_affinetridesc.seamfixupX16 = (r_affinetridesc.skinwidth >> 1) << 16;
	r_affinetridesc.skinheight = pmdl->skinheight;
}

/*
================
R_AliasSetupLighting
================
*/
void R_AliasSetupLighting (void)
{
	int			lnum, r;
	vec3_t		dist;
	float		add;
	
	//
	// get lighting information
	//
	if (currententity->flags & RF_FULLBRIGHT)
		r = 255;
	else
		r = R_LightPoint (currententity->origin);

	if (r < r_oldrefdef.ambientlight)
		r = r_oldrefdef.ambientlight;
	
	r_ambientlight = r_shadelight = r;
	
	if ((currententity->flags & RF_MINLIGHT) && r_ambientlight < 24)
		r_ambientlight = r_shadelight = 24;
	
	for (lnum = 0; lnum < r_refdef.num_dlights; lnum++)
	{
		VectorSubtract (currententity->origin, r_refdef.dlights[lnum].origin, dist);
		add = r_refdef.dlights[lnum].radius - VectorLength(dist);
		
		if (add > 0) 
		{
			r_ambientlight += add;
			// ZOID models should be affected by dlights as well
			r_shadelight += add;
		}
	}
	
	// clamp lighting so it doesn't overbright as much
	if (r_ambientlight > 128)
		r_ambientlight = 128;
	if (r_ambientlight + r_shadelight > 192)
		r_shadelight = 192 - r_ambientlight;
	
// guarantee that no vertex will ever be lit below LIGHT_MIN, so we don't have
// to clamp off the bottom
	if (r_ambientlight < LIGHT_MIN)
		r_ambientlight = LIGHT_MIN;

	r_ambientlight = (255 - r_ambientlight) << VID_CBITS;

	if (r_ambientlight < LIGHT_MIN)
		r_ambientlight = LIGHT_MIN;

	if (r_shadelight < 0)
		r_shadelight = 0;

	r_shadelight *= ((100.0 / 255.0) * VID_GRADES);

	r_shadedots_quant = ((int)(currententity->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1);
	r_shadedots = r_avertexnormal_dots[r_shadedots_quant];
}

/*
=================
R_AliasSetupFrame

=================
*/
void R_AliasFrameVerts (int frame, float time, trivertx_t **verts)
{
	int	i, numposes;
	maliasgroup_t *paliasgroup;
	float *pintervals, fullinterval, targettime;

	if (paliashdr->frames[frame].type == ALIAS_SINGLE)
	{
		*verts = (trivertx_t *)((byte *)paliashdr + paliashdr->frames[frame].frame);
	}
	else 
	{
		paliasgroup = (maliasgroup_t *)
			((byte *)paliashdr + paliashdr->frames[frame].frame);
		pintervals = (float *)((byte *)paliashdr + paliasgroup->intervals);
		numposes = paliasgroup->frames[frame].numframes;
		fullinterval = pintervals[numposes-1];

	//
	// when loading in Mod_LoadAliasGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
	//
		targettime = time - ((int)(time / fullinterval)) * fullinterval;

		for (i=0 ; i<(numposes-1) ; i++)
		{
			if (pintervals[i] > targettime)
				break;
		}

		*verts = (trivertx_t *)((byte *)paliashdr + paliasgroup->frames[i].frame);
	}
}

/*
=================
R_AliasSetupFrame

set r_oldapverts, r_apverts and r_framelerp
=================
*/
void R_AliasSetupFrame (void)
{
	entity_t		*e = currententity;
	int				frame = e->frame, oldframe = e->oldframe;

	if ((frame >= pmdl->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", frame);
		frame = 0;
	}
	if ((oldframe >= pmdl->numframes) || (oldframe < 0))
	{
		Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", oldframe);
		oldframe = 0;
	}

	R_AliasFrameVerts (frame, r_refdef.time+e->syncbase, &r_apverts);
	R_AliasFrameVerts (oldframe, r_refdef.oldtime+e->syncbase, &r_oldapverts);

	if (!r_lerpmodels.value) 
		r_framelerp = 1.0f;
	else
		r_framelerp = bound (0, e->framelerp, 1);
}

/*
================
R_DrawAliasModel
================
*/
void R_DrawAliasModel (void)
{
	finalvert_t		finalverts[MAXALIASVERTS +
						((CACHE_SIZE - 1) / sizeof(finalvert_t)) + 1];
	auxvert_t		auxverts[MAXALIASVERTS];
	int				trivial_accept;

	paliashdr = (aliashdr_t *)Mod_Extradata (currentmodel);
	pmdl = (mdl_t *)((byte *)paliashdr + paliashdr->model);

	// see if the bounding box lets us trivially reject, also sets
	// trivial accept status
	if (currententity->flags & RF_WEAPONMODEL)
		trivial_accept = 0;
	else
	{
		trivial_accept = R_AliasCheckBBox ();
		if (trivial_accept == -1)		// clipped away
			return;
	}

	r_amodels_drawn++;

// cache align
	pfinalverts = (finalvert_t *)
			(((long)&finalverts[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
	pauxverts = &auxverts[0];

	R_AliasSetupSkin ();

	R_AliasSetUpTransform (trivial_accept);

	R_AliasSetupLighting ();

	R_AliasSetupFrame ();

	if (!currententity->colormap)
		Sys_Error ("R_DrawAliasModel: !currententity->colormap");
	acolormap = currententity->colormap;

#if id386
	D_Aff8Patch (acolormap);
#endif

	if (currententity->flags & RF_DEPTHSCALE)
		ziscale = (float)0x8000 * (float)0x10000 * 3.0;
	else
		ziscale = (float)0x8000 * (float)0x10000;

	if (trivial_accept)
		R_AliasPrepareUnclippedPoints ();
	else
		R_AliasPreparePoints ();
}

