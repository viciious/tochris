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

#include "gl_local.h"

refdef_t	r_refdef;
entity_t	r_worldentity;

vec3_t		modelorg, r_entorigin;
entity_t	*currententity;
model_t		*currentmodel;

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

cplane_t	frustum[4];

int			c_brush_polys, c_alias_polys;

int			currenttexture = -1;		// to avoid unnecessary texture sets

int			cnttextures[2] = {-1, -1};     // cached

int			particletexture;	// little dot for particles
int			playertextures;		// up to MAX_SCOREBOARD+1 color translated skins

int			glx, gly, glwidth, glheight;

model_t		*r_worldmodel;

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_refdef;

float		r_framelerp;

mleaf_t		*r_viewleaf, *r_oldviewleaf;

texture_t	*r_notexture_mip;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value


byte	gammatable[256];		// palette is sent through this
byte	ramps[3][256];
float	v_blend[4];				// rgba 0.0 - 1.0

void R_MarkLeaves (void);
void R_SkynameChanged_f (cvar_t *cvar) {
	r_drawskybox = R_LoadSkys (cvar->string);
}

void ( APIENTRY * qglPointParameterfEXT)( GLenum param, GLfloat value ) = NULL;
void ( APIENTRY * qglPointParameterfvEXT)( GLenum param, const GLfloat *value ) = NULL;

cvar_t	r_norefresh = {"r_norefresh","0"};
cvar_t	r_drawentities = {"r_drawentities","1"};
cvar_t	r_drawviewmodel = {"r_drawviewmodel","1"};
cvar_t	r_speeds = {"r_speeds","0"};
cvar_t	r_fullbright = {"r_fullbright","0"};
cvar_t	r_lightmap = {"r_lightmap","0"};
cvar_t	r_shadows = {"r_shadows","0"};
cvar_t	r_wateralpha = {"r_wateralpha","1"};
cvar_t	r_dynamic = {"r_dynamic","1"};
cvar_t	r_novis = {"r_novis","0"};
cvar_t	r_skyname = {"r_skyname", "", true, false, &R_SkynameChanged_f};
cvar_t  r_lerpmodels = { "r_lerpmodels", "1", true };

cvar_t	r_loadlits = {"r_loadlits", "0"};
cvar_t	r_fastsky = {"r_fastsky","0"};

cvar_t	gl_finish = {"gl_finish","0"};
cvar_t	gl_clear = {"gl_clear","0"};
cvar_t	gl_cull = {"gl_cull","1"};
cvar_t	gl_texsort = {"gl_texsort","1"};
cvar_t	gl_smoothmodels = {"gl_smoothmodels","1"};
cvar_t	gl_affinemodels = {"gl_affinemodels","0"};
cvar_t	gl_polyblend = {"gl_polyblend","1"};
cvar_t	gl_flashblend = {"gl_flashblend","0"};
cvar_t	gl_playermip = {"gl_playermip","0"};
cvar_t	gl_nocolors = {"gl_nocolors","0"};
cvar_t	gl_keeptjunctions = {"gl_keeptjunctions","0"};
cvar_t	gl_cshiftpercent = {"gl_cshiftpercent", "100", false};
cvar_t	gl_ext_pointparameters = {"gl_ext_pointparameters", "1", true};
cvar_t	gl_particle_min_size = {"gl_particle_min_size", "2", true};
cvar_t	gl_particle_max_size = {"gl_particle_max_size", "40", true};
cvar_t	gl_particle_size = {"gl_particle_size", "40", true};
cvar_t	gl_particle_att_a = {"gl_particle_att_a", "0.01", true};
cvar_t	gl_particle_att_b = {"gl_particle_att_b", "0.0", true};
cvar_t	gl_particle_att_c = {"gl_particle_att_c", "0.01", true};

cvar_t	v_gamma = {"gamma", "1", true};

extern	cvar_t	gl_ztrick;

/*
=================
R_CullBox

Returns true if the box is completely outside the frustum
=================
*/
qboolean R_CullBox (const vec3_t mins, const vec3_t maxs)
{
	int		i;
	cplane_t *p;

	for (i=0,p=frustum ; i<4; i++,p++)
	{
		switch (p->signbits)
		{
		case 0:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 1:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 2:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 3:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 4:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 5:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 6:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 7:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		default:
			return false;
		}
	}

	return false;
}

/*
=================
R_CullSphere

Returns true if the sphere is completely outside the frustum
=================
*/
qboolean R_CullSphere (const vec3_t centre, float radius)
{
	int		i;
	cplane_t *p;

	for (i=0,p=frustum ; i<4; i++,p++)
	{
		if ( DotProduct (centre, p->normal) - p->dist <= -radius )
			return true;
	}

	return false;
}

void R_RotateForEntity (entity_t *e)
{
    glTranslatef (e->origin[0], e->origin[1], e->origin[2]);

    glRotatef (e->angles[1], 0, 0, 1);
    glRotatef (-e->angles[0], 0, 1, 0);
    glRotatef (e->angles[2], 1, 0, 0);
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/

/*
================
R_GetSpriteFrame
================
*/
mspriteframe_t *R_GetSpriteFrame (entity_t *e)
{
	msprite_t		*psprite;
	mspritegroup_t	*pspritegroup;
	mspriteframe_t	*pspriteframe;
	int				i, numframes, frame;
	float			*pintervals, fullinterval, targettime, time;

	psprite = e->model->cache.data;
	frame = e->frame;

	if ((frame >= psprite->numframes) || (frame < 0))
	{
		Con_Printf ("R_DrawSprite: no such frame %d\n", frame);
		frame = 0;
	}

	if (psprite->frames[frame].type == SPR_SINGLE)
	{
		pspriteframe = psprite->frames[frame].frameptr;
	}
	else
	{
		pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes-1];

		if (e->model->synctype == ST_RAND)
			time = r_refdef.time + e->syncbase;
		else
			time = r_refdef.time;

	// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
		targettime = time - ((int)(time / fullinterval)) * fullinterval;

		for (i=0 ; i<(numframes-1) ; i++)
		{
			if (pintervals[i] > targettime)
				break;
		}

		pspriteframe = pspritegroup->frames[i];
	}

	return pspriteframe;
}


/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (entity_t *e)
{
	vec3_t			point;
	mspriteframe_t	*frame;
	float			*up, *right;
	vec3_t			v_forward, v_right, v_up;
	msprite_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache
	frame = R_GetSpriteFrame (e);
	psprite = e->model->cache.data;

	if (psprite->type == SPR_ORIENTED)
	{	// bullet marks on walls
		AngleVectors (e->angles, v_forward, v_right, v_up);
		up = v_up;
		right = v_right;
	}
	else
	{	// normal sprite
		up = vup;
		right = vright;
	}

	glColor3f (1,1,1);

	GL_DisableMultitexture();

    GL_Bind(frame->gl_texturenum);

	glEnable (GL_ALPHA_TEST);
	glBegin (GL_QUADS);

	glTexCoord2f (0, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->left, right, point);
	glVertex3fv (point);

	glTexCoord2f (0, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->left, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->right, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->right, right, point);
	glVertex3fv (point);
	
	glEnd ();

	glDisable (GL_ALPHA_TEST);
}

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

vec3_t	shadevector;
float	shadelight;

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "anorm_dots.h"
;

float	*shadedots = r_avertexnormal_dots[0];

/*
=============
GL_DrawAliasFrame
=============
*/
void GL_DrawAliasFrame (aliashdr_t *paliashdr, int oldposenum, int posenum)
{
	float 	l;
	trivertx_t	*v1, *v2;
	int		*order;
	int		count;
	vec3_t	v;

	v1 = v2 = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	v1 += oldposenum * paliashdr->poseverts;
	v2 += posenum * paliashdr->poseverts;

	order = (int *)((byte *)paliashdr + paliashdr->commands);

	while (count = *order++)
	{
		// get the vertex count and primitive type
		if (count < 0)
		{
			count = -count;
			glBegin (GL_TRIANGLE_FAN);
		}
		else
			glBegin (GL_TRIANGLE_STRIP);

		do
		{
			// texture coordinates come from the draw list
			glTexCoord2f (((float *)order)[0], ((float *)order)[1]);
			order += 2;

			// normals and vertexes come from the frame list
			l = shadedots[v1->lightnormalindex] + 
				(shadedots[v2->lightnormalindex] - shadedots[v1->lightnormalindex]) * r_framelerp;
			l *= shadelight;

			v[0] = (v1->v[0] + (v2->v[0] - v1->v[0])*r_framelerp) * paliashdr->scale[0] + paliashdr->scale_origin[0];
			v[1] = (v1->v[1] + (v2->v[1] - v1->v[1])*r_framelerp) * paliashdr->scale[1] + paliashdr->scale_origin[1];
			v[2] = (v1->v[2] + (v2->v[2] - v1->v[2])*r_framelerp) * paliashdr->scale[2] + paliashdr->scale_origin[2];

			glColor3f (bound(0, l, 1), bound(0, l, 1), bound(0, l, 1));
			glVertex3fv (v);

			v1++; v2++;
		} while (--count);

		glEnd ();
	}
}


/*
=============
GL_DrawAliasShadow
=============
*/
extern	vec3_t			lightspot;

void GL_DrawAliasShadow (aliashdr_t *paliashdr, int oldposenum, int posenum)
{
	trivertx_t	*v1, *v2;
	int		*order;
	vec3_t	point, end;
	float	height, lheight;
	int		count;
	trace_t	tr;

	VectorSet (end, currententity->origin[0], currententity->origin[1], currententity->origin[2] - 4096);
	tr = CL_TraceLine (currententity->origin, vec3_origin, vec3_origin, end, -1);
	if (tr.fraction == 1.0f)
		return;

	lheight = currententity->origin[2] - tr.endpos[2];

	height = 0;
	v1 = v2 = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	v1 += oldposenum * paliashdr->poseverts;
	v2 += posenum * paliashdr->poseverts;
	order = (int *)((byte *)paliashdr + paliashdr->commands);

	height = -lheight + 0.1;

	while (count = *order++)
	{
		// get the vertex count and primitive type
		if (count < 0)
		{
			count = -count;
			glBegin (GL_TRIANGLE_FAN);
		}
		else
			glBegin (GL_TRIANGLE_STRIP);

		do
		{
			// texture coordinates come from the draw list
			// (skipped for shadows) glTexCoord2fv ((float *)order);
			order += 2;

			// normals and vertexes come from the frame list
			point[0] = (v1->v[0] + (v2->v[0] - v1->v[0]) * r_framelerp) * paliashdr->scale[0] + paliashdr->scale_origin[0];
			point[1] = (v1->v[1] + (v2->v[1] - v1->v[1]) * r_framelerp) * paliashdr->scale[1] + paliashdr->scale_origin[1];
			point[2] = (v1->v[2] + (v2->v[2] - v1->v[2]) * r_framelerp) * paliashdr->scale[2] + paliashdr->scale_origin[2];

			point[0] = (point[0] - shadevector[0]*(point[2]+lheight));
			point[1] = (point[1] - shadevector[1]*(point[2]+lheight));
			point[2] = height;

			glVertex3fv (point);

			v1++; v2++;
		} while (--count);

		glEnd ();
	}	
}



/*
=================
R_SetupAliasFrame

=================
*/
void R_SetupAliasFrame (maliasframedesc_t *oldframe, maliasframedesc_t *frame, aliashdr_t *paliashdr)
{
	int				pose, oldpose, numposes;
	float			interval;

	oldpose = oldframe->firstpose;
	numposes = oldframe->numposes;

	if (numposes > 1)
	{
		interval = oldframe->interval;
		oldpose += (int)(r_refdef.oldtime / interval) % numposes;
	}

	pose = frame->firstpose;
	numposes = frame->numposes;

	if (numposes > 1)
	{
		interval = frame->interval;
		pose += (int)(r_refdef.time / interval) % numposes;
	}

	GL_DrawAliasFrame (paliashdr, oldpose, pose);
}



/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel (entity_t *e)
{
	int			i;
	int			lnum;
	vec3_t		dist;
	float		add;
	model_t		*clmodel = e->model;
	aliashdr_t	*paliashdr;
	int			anim;
	maliasframedesc_t *oldframe, *frame;

	//
	// locate the proper data
	//
	paliashdr = (aliashdr_t *)Mod_Extradata (clmodel);

	if ((e->frame >= paliashdr->numframes) || (e->frame < 0))
	{
		Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", e->frame);
		e->frame = 0;
	}

	if ((e->oldframe >= paliashdr->numframes) || (e->oldframe < 0))
	{
		Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", e->oldframe);
		e->oldframe = 0;
	}

	frame = &paliashdr->frames[e->frame];
	oldframe = &paliashdr->frames[e->oldframe];

	if (!(e->flags & RF_WEAPONMODEL))
	{
		if (e->angles[0] || e->angles[1] || e->angles[2])
		{
			if (R_CullSphere (e->origin, max (oldframe->radius, frame->radius)))
				return;
		}
		else
		{
			vec3_t	mins, maxs;

			for (i = 0; i < 3; i++)
			{
				mins[i] = e->origin[i] + min (oldframe->bboxmin[i], frame->bboxmin[i]);
				maxs[i] = e->origin[i] + max (oldframe->bboxmax[i], frame->bboxmax[i]);
			}

			if (R_CullBox (mins, maxs))
				return;
		}
	}

	VectorCopy (e->origin, r_entorigin);
	VectorSubtract (r_origin, r_entorigin, modelorg);

	//
	// get lighting information
	//
	if (e->flags & RF_FULLBRIGHT)
		shadelight = 255;
	else
	{
		if (!(r_refdef.flags & RDF_NOWORLDMODEL))
			shadelight = R_LightPoint (e->origin);
		else
			shadelight = 0;

		// always give some light
		if ((e->flags & RF_MINLIGHT) && (shadelight < 24))
			shadelight = 24;

		for (lnum=0 ; lnum<r_refdef.num_dlights ; lnum++)
		{
			VectorSubtract (e->origin, r_refdef.dlights[lnum].origin, dist);
			add = r_refdef.dlights[lnum].radius - VectorLength(dist);

			if (add > 0) {
				//ZOID models should be affected by dlights as well
				shadelight += add;
			}
		}
	}

	// HACK HACK HACK -- no fullbright colors, so make torches full light
	if (!strcmp (clmodel->name, "progs/flame2.mdl")
		|| !strcmp (clmodel->name, "progs/flame.mdl") )
		shadelight = 255;

	shadedots = r_avertexnormal_dots[((int)(e->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];
	shadelight = shadelight * (1.0 / 200.0);

	c_alias_polys += paliashdr->numtris;

	//
	// draw all the triangles
	//

	GL_DisableMultitexture();

    glPushMatrix ();
	R_RotateForEntity (e);

	if (e->flags & RF_DEPTHSCALE) {
		glDepthRange (gldepthmin, gldepthmin + 0.3*(gldepthmax-gldepthmin));
	}

	if (e->model->synctype == ST_RAND)
		anim = (int)((r_refdef.time + e->syncbase) * 10) & 3;
	else
		anim = (int)(r_refdef.time * 10) & 3;

    GL_Bind(paliashdr->gl_texturenum[e->skinnum][anim]);

	// we can't dynamically colormap textures, so they are cached
	// seperately for the players. Heads are just uncolored.
	if (e->colormap != vid.colormap && !gl_nocolors.value)
	{
		i = e->number;
		if (i >= 0 && i<=Com_ClientMaxclients() /* && !strcmp (e->model->name, "progs/player.mdl") */)
		    GL_Bind(playertextures + i);
	}

	if (gl_smoothmodels.value)
		glShadeModel (GL_SMOOTH);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (gl_affinemodels.value)
		glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	if (!r_lerpmodels.value) 
		r_framelerp = 1.0f;
	else
		r_framelerp = bound (0, e->framelerp, 1);

	R_SetupAliasFrame (oldframe, frame, paliashdr);

	if (e->flags & RF_DEPTHSCALE) {
		glDepthRange (gldepthmin, gldepthmax);
	}

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glShadeModel (GL_FLAT);
	if (gl_affinemodels.value)
		glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glPopMatrix ();

	if (r_shadows.value && !(e->flags & RF_NOSHADOW))
	{
		float an = -e->angles[1]/180*M_PI;
		shadevector[0] = cos(an);
		shadevector[1] = sin(an);
		shadevector[2] = 1;
		VectorNormalize (shadevector);

		glPushMatrix ();
		glTranslatef (e->origin[0], e->origin[1], e->origin[2]);
		glRotatef (e->angles[1], 0, 0, 1);
		glDisable (GL_TEXTURE_2D);
		glEnable (GL_BLEND);
		glColor4f (0,0,0,0.5);
		GL_DrawAliasShadow (paliashdr, e->oldframe, e->frame);
		glEnable (GL_TEXTURE_2D);
		glDisable (GL_BLEND);
		glColor4f (1,1,1,1);
		glPopMatrix ();
	}

}

//==================================================================================

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
	int		i;

	if (!r_drawentities.value)
		return;

	// draw sprites seperately, because of alpha blending
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
		case mod_alias:
			R_DrawAliasModel (currententity);
			break;

		case mod_brush:
			R_DrawBrushModel (currententity);
			break;

		default:
			break;
		}
	}

	for (i=0 ; i<r_refdef.num_entities ; i++)
	{
		currententity = &r_refdef.entities[i];
		currentmodel = currententity->model;
		if (!currentmodel)
			continue;

		switch (currentmodel->type)
		{
		case mod_sprite:
			R_DrawSpriteModel (currententity);
			break;
		}
	}
}


/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles (void)
{
	int				i;
	const particle_t *p;
	vec3_t			up, right;
	float			scale;

	if (!r_refdef.num_particles)
		return;

	if (gl_ext_pointparameters.value && qglPointParameterfEXT)
	{
		int i;

		glDepthMask (GL_FALSE);
		glEnable (GL_BLEND);
		glDisable (GL_TEXTURE_2D);

		glPointSize (gl_particle_size.value);

		glBegin(GL_POINTS);
		for (i = 0, p = r_refdef.particles; i < r_refdef.num_particles; i++, p++)
		{
			glColor3ubv ((byte *)&d_8to24table[p->color]);
			glVertex3fv (p->org);
		}
		glEnd();

		glDisable (GL_BLEND);
		glColor4f (1.0F, 1.0F, 1.0F, 1.0F);
		glDepthMask (GL_TRUE);
		glEnable (GL_TEXTURE_2D);

	}
	else
	{
		GL_Bind (particletexture);

		glDepthMask (GL_FALSE);
		glEnable (GL_BLEND);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glBegin (GL_TRIANGLES);

		VectorScale (vup, 1.5, up);
		VectorScale (vright, 1.5, right);

		for (i = 0, p = r_refdef.particles; i < r_refdef.num_particles; i++, p++)
		{
			// hack a scale up to keep particles from disapearing
			scale = (p->org[0] - r_origin[0])*vpn[0] + (p->org[1] - r_origin[1])*vpn[1]
				+ (p->org[2] - r_origin[2])*vpn[2];
			if (scale < 20)
				scale = 1;
			else
				scale = 1 + scale * 0.004;
			glColor3ubv ((byte *)&d_8to24table[p->color]);
			glTexCoord2f (0,0);
			glVertex3fv (p->org);
			glTexCoord2f (1,0);
			glVertex3f (p->org[0] + up[0]*scale, p->org[1] + up[1]*scale, p->org[2] + up[2]*scale);
			glTexCoord2f (0,1);
			glVertex3f (p->org[0] + right[0]*scale, p->org[1] + right[1]*scale, p->org[2] + right[2]*scale);
		}

		glEnd ();
		glDisable (GL_BLEND);
		glDepthMask (GL_TRUE);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	}
}

void GL_BuildGammaTable (float g)
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
GL_CheckGamma
=================
*/
qboolean GL_CheckGamma (void)
{
	static float oldgammavalue;
	
	if (v_gamma.value == oldgammavalue)
		return false;
	oldgammavalue = v_gamma.value;
	
	GL_BuildGammaTable (v_gamma.value);
	
	return true;
}

/*
=============
R_CalcBlend
=============
*/
void R_CalcBlend (void)
{
	float	r = 0, g = 0, b = 0, a = 0, a2;
	int		j;

	for (j=0 ; j<r_refdef.num_cshifts ; j++)	
	{
		if (!gl_cshiftpercent.value)
			continue;

		a2 = ((r_refdef.cshifts[j].percent * gl_cshiftpercent.value) / 100.0) * (1.0 / 255.0);
		if (!a2)
			continue;
		if (a2 > 1)
			a2 = 1;
		r += (r_refdef.cshifts[j].destcolor[0]-r) * a2;
		g += (r_refdef.cshifts[j].destcolor[1]-g) * a2;
		b += (r_refdef.cshifts[j].destcolor[2]-b) * a2;
		a = 1 - (1 - a) * (1 - a2); // correct alpha multiply...  took a while to find it on the web
	}

	if (a)
	{
		a2 = 1 / a;
		r *= a2;
		g *= a2;
		b *= a2;
	}

	v_blend[0] = r * (1.0/255.0);
	v_blend[1] = g * (1.0/255.0);
	v_blend[2] = b * (1.0/255.0);
	v_blend[0] = bound(0, v_blend[0], 1);
	v_blend[1] = bound(0, v_blend[1], 1);
	v_blend[2] = bound(0, v_blend[2], 1);
	v_blend[3] = bound(0, a			, 1);
}

/*
=============
R_UpdatePalette
=============
*/
void R_UpdatePalette (void)
{
	int		i;
	byte	*basepal, *newpal;
	byte	pal[768];
	float	r,g,b,a;
	int		ir, ig, ib;
	qboolean force;

	force = GL_CheckGamma ();
	if (!r_refdef.num_cshifts && !force)
		return;

	R_CalcBlend ();

	a = v_blend[3];
	r = 255*v_blend[0]*a;
	g = 255*v_blend[1]*a;
	b = 255*v_blend[2]*a;

	a = 1-a;
	for (i=0 ; i<256 ; i++)
	{
		ir = i*a + r;
		ig = i*a + g;
		ib = i*a + b;
		if (ir > 255)
			ir = 255;
		if (ig > 255)
			ig = 255;
		if (ib > 255)
			ib = 255;

		ramps[0][i] = gammatable[ir];
		ramps[1][i] = gammatable[ig];
		ramps[2][i] = gammatable[ib];
	}

	basepal = host_basepal;
	newpal = pal;
	
	for (i=0 ; i<256 ; i++)
	{
		ir = basepal[0];
		ig = basepal[1];
		ib = basepal[2];
		basepal += 3;
		
		newpal[0] = ramps[0][ir];
		newpal[1] = ramps[1][ig];
		newpal[2] = ramps[2][ib];
		newpal += 3;
	}

	VID_ShiftPalette (pal);	
}

/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	if (!gl_polyblend.value)
		return;
	if (!v_blend[3])
		return;

	GL_DisableMultitexture();

	glDisable (GL_ALPHA_TEST);
	glEnable (GL_BLEND);
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_TEXTURE_2D);

    glLoadIdentity ();

    glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up

	glColor4fv (v_blend);

	glBegin (GL_QUADS);

	glVertex3f (10, 100, 100);
	glVertex3f (10, -100, 100);
	glVertex3f (10, -100, -100);
	glVertex3f (10, 100, -100);
	glEnd ();

	glDisable (GL_BLEND);
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_ALPHA_TEST);
}


int SignbitsForPlane (cplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}


void R_SetFrustum (void)
{
	int		i;

	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-r_refdef.fov_x / 2 ) );
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-r_refdef.fov_x / 2 );
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-r_refdef.fov_y / 2 );
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_refdef.fov_y / 2 ) );

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}



/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
// don't allow cheats in multiplayer
	if (Com_ClientMaxclients() > 1)
		Cvar_Set (&r_fullbright, "0");

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_refdef.vieworg, r_origin);

	AngleVectors (r_refdef.viewangles, vpn, vright, vup);

// current viewleaf
	if (!(r_refdef.flags & RDF_NOWORLDMODEL))
	{
		R_AnimateLight ();

		r_oldviewleaf = r_viewleaf;
		r_viewleaf = Mod_PointInLeaf (r_origin, r_worldmodel);
	}

	c_brush_polys = 0;
	c_alias_polys = 0;
}


void MYgluPerspective( GLdouble fovy, GLdouble aspect,
		     GLdouble zNear, GLdouble zFar )
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan( fovy * M_PI / 360.0 );
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	glFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
	extern	int glwidth, glheight;
	int		x, x2, y2, y, w, h;

	//
	// set up viewpoint
	//
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
	x = r_refdef.x * glwidth/vid.width;
	x2 = (r_refdef.x + r_refdef.width) * glwidth/vid.width;
	y = (vid.height-r_refdef.y) * glheight/vid.height;
	y2 = (vid.height - (r_refdef.y + r_refdef.height)) * glheight/vid.height;

	w = x2 - x;
	h = y - y2;

	glViewport (glx + x, gly + y2, w, h);
    screenaspect = (float)r_refdef.width/r_refdef.height;

    MYgluPerspective (r_refdef.fov_y,  screenaspect,  4,  4096);

	glCullFace(GL_FRONT);

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();

    glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up
    glRotatef (-r_refdef.viewangles[2],  1, 0, 0);
    glRotatef (-r_refdef.viewangles[0],  0, 1, 0);
    glRotatef (-r_refdef.viewangles[1],  0, 0, 1);
    glTranslatef (-r_refdef.vieworg[0],  -r_refdef.vieworg[1],  -r_refdef.vieworg[2]);

	glGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

	//
	// set drawing parms
	//
	if (gl_cull.value)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);
}

/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
	if (gl_ztrick.value)
	{
		static int trickframe;

		if (gl_clear.value)
			glClear (GL_COLOR_BUFFER_BIT);

		trickframe++;
		if (trickframe & 1)
		{
			gldepthmin = 0;
			gldepthmax = 0.49999;
			glDepthFunc (GL_LEQUAL);
		}
		else
		{
			gldepthmin = 1;
			gldepthmax = 0.5;
			glDepthFunc (GL_GEQUAL);
		}
	}
	else
	{
		if (gl_clear.value)
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			glClear (GL_DEPTH_BUFFER_BIT);
		gldepthmin = 0;
		gldepthmax = 1;
		glDepthFunc (GL_LEQUAL);
	}

	glDepthRange (gldepthmin, gldepthmax);
}

/*
================
R_BeginFrame

================
*/
void R_BeginFrame (void)
{
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);
	GL_Set2D ();
}

/*
================
R_EndFrame

================
*/
void R_EndFrame (void)
{
	R_UpdatePalette ();
	GL_EndRendering ();
}

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (refdef_t *refdef)
{
	double	time1, time2;

	r_refdef = *refdef;

	if (r_norefresh.value)
		return;

	if (!r_worldentity.model || !r_worldmodel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	if (r_speeds.value)
	{
		glFinish ();
		time1 = Sys_FloatTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	if (gl_finish.value)
		glFinish ();

	R_Clear ();

	// render normal view

	R_SetupFrame ();

	R_PushDlights ();

	R_SetFrustum ();

	R_SetupGL ();

	R_MarkLeaves ();	// done here so we know if we're in water

	R_DrawWorld ();		// adds static entities to the list

	S_ExtraUpdate ();	// don't let sound get messed up if going slow

	R_DrawEntitiesOnList ();

	GL_DisableMultitexture();

	R_RenderDlights ();

	R_DrawParticles ();

	R_DrawWaterSurfaces ();

	R_DrawParticles ();

	if (!(r_refdef.flags & RDF_NOWORLDMODEL))
		R_PolyBlend ();

	if (r_speeds.value)
	{
		time2 = Sys_FloatTime ();
		Con_Printf ("%3i ms  %4i wpoly %4i epoly\n", (int)((time2-time1)*1000), c_brush_polys, c_alias_polys); 
	}

	GL_Set2D ();
}
