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
// cl_view.c -- player eye positioning

#include "client.h"

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/

cvar_t	cl_rollspeed = {"cl_rollspeed", "200"};
cvar_t	cl_rollangle = {"cl_rollangle", "2.0"};

cvar_t	cl_bob = {"cl_bob","0.02", false};
cvar_t	cl_bobcycle = {"cl_bobcycle","0.6", false};
cvar_t	cl_bobup = {"cl_bobup","0.5", false};

cvar_t	cl_add_entities = {"cl_entities", "1", false};
cvar_t	cl_add_statice_ents = {"cl_static_ents", "1", false};
cvar_t	cl_add_particles = {"cl_particles", "1", false};
cvar_t	cl_add_lights = {"cl_lights", "1", false};
cvar_t	cl_add_cshifts = {"cl_cshifts", "1", false};

cvar_t	v_kicktime = {"v_kicktime", "0.5", false};
cvar_t	v_kickroll = {"v_kickroll", "0.6", false};
cvar_t	v_kickpitch = {"v_kickpitch", "0.6", false};

cvar_t	v_iyaw_cycle = {"v_iyaw_cycle", "2", false};
cvar_t	v_iroll_cycle = {"v_iroll_cycle", "0.5", false};
cvar_t	v_ipitch_cycle = {"v_ipitch_cycle", "1", false};
cvar_t	v_iyaw_level = {"v_iyaw_level", "0.3", false};
cvar_t	v_iroll_level = {"v_iroll_level", "0.1", false};
cvar_t	v_ipitch_level = {"v_ipitch_level", "0.3", false};

cvar_t	v_idlescale = {"v_idlescale", "0", false};

cvar_t	chase_back = {"chase_back", "100"};
cvar_t	chase_up = {"chase_up", "16"};
cvar_t	chase_right = {"chase_right", "0"};
cvar_t	chase_active = {"chase_active", "0"};

float	v_dmg_time, v_dmg_roll, v_dmg_pitch;

//=============================================================================

int				r_numentities;
entity_t		r_entities[MAX_ENTITIES];

int				r_numparticles;
particle_t		r_particles[MAX_PARTICLES];

int				r_numdlights;
dlight_t		r_dlights[MAX_DLIGHTS];

int				r_numcshifts;
cshift_t		r_cshifts[MAX_CSHIFTS];

/*
===============
V_ClearScene
===============
*/
void V_ClearScene (void)
{
	r_numcshifts = 0;
	r_numdlights = 0;
	r_numentities = 0;
	r_numparticles = 0;
}

/*
===============
V_AddEntity
===============
*/
void V_AddEntity (entity_t *ent)
{
	if (r_numentities >= MAX_ENTITIES)
		return;
	r_entities[r_numentities] = *ent;
	r_numentities++;
}

/*
===============
V_AddParticle
===============
*/
void V_AddParticle (vec3_t org, int color)
{
	if (r_numparticles >= MAX_PARTICLES)
		return;
	r_particles[r_numparticles].color = color;
	VectorCopy (org, r_particles[r_numparticles].org);
	r_numparticles++;
}

/*
===============
V_AddDlight
===============
*/
void V_AddDlight (vec3_t org, float radius)
{
	if (r_numdlights >= MAX_DLIGHTS)
		return;
	r_dlights[r_numdlights].radius = radius;
	VectorCopy (org, r_dlights[r_numdlights].origin);
	r_numdlights++;
}

/*
===============
V_AddCshift
===============
*/
void V_AddCshift (int r, int g, int b, int percent)
{
	if (r_numcshifts >= MAX_CSHIFTS)
		return;
	r_cshifts[r_numcshifts].destcolor[0] = r;
	r_cshifts[r_numcshifts].destcolor[1] = g;
	r_cshifts[r_numcshifts].destcolor[2] = b;
	r_cshifts[r_numcshifts].percent = percent;
	r_numcshifts++;
}

//=============================================================================

/*
===============
V_CalcRoll

Used by view and sv_user
===============
*/
float V_CalcRoll (vec3_t angles, vec3_t velocity)
{
	float	sign;
	float	side;
	float	value;
	vec3_t	forward, right;
	
	AngleVectors (angles, forward, right, NULL);
	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs(side);
	
	value = cl_rollangle.value;

	if (side < cl_rollspeed.value)
		side = side * value / cl_rollspeed.value;
	else
		side = value;
	
	return side*sign;
	
}


/*
===============
V_CalcBob

===============
*/
float V_CalcBob (void)
{
	float	bob;
	float	cycle;
	
	cycle = cl.time - (int)(cl.time/cl_bobcycle.value)*cl_bobcycle.value;
	cycle /= cl_bobcycle.value;
	if (cycle < cl_bobup.value)
		cycle = M_PI * cycle / cl_bobup.value;
	else
		cycle = M_PI + M_PI*(cycle-cl_bobup.value)/(1.0 - cl_bobup.value);

// bob is proportional to velocity in the xy plane
// (don't count Z, or jumping messes it up)

	bob = sqrt(cl.velocity[0]*cl.velocity[0] + cl.velocity[1]*cl.velocity[1]) * cl_bob.value;
	bob = bob * (0.3 + 0.7*sin(cycle));

	return bound (-7, bob, 4);
}


//=============================================================================


cvar_t	v_centermove = {"v_centermove", "0.15", false};
cvar_t	v_centerspeed = {"v_centerspeed","500"};


void CL_StartPitchDrift (void)
{
	if (cl.laststop == cl.time)
		return;		// something else is keeping it from drifting

	if (cl.nodrift || !cl.pitchvel)
	{
		cl.pitchvel = v_centerspeed.value;
		cl.nodrift = false;
		cl.driftmove = 0;
	}
}

void CL_StopPitchDrift (void)
{
	cl.laststop = cl.time;
	cl.nodrift = true;
	cl.pitchvel = 0;
}

/*
===============
CL_DriftPitch

Moves the client pitch angle towards cl.idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.

Drifting is enabled when the center view key is hit, mlook is released and
lookspring is non 0, or when 
===============
*/
void CL_DriftPitch (void)
{
	float		delta, move;

	if (!cl.onground || cls.demoplayback)
	{
		cl.driftmove = 0;
		cl.pitchvel = 0;
		return;
	}

// don't count small mouse motion
	if (cl.nodrift)
	{
		if (fabs(cl.cmd.forwardmove) < cl_forwardspeed.value)
			cl.driftmove = 0;
		else
			cl.driftmove += host_frametime;
	
		if (cl.driftmove > v_centermove.value)
		{
			CL_StartPitchDrift ();
		}
		return;
	}
	
	delta = cl.idealpitch - cl.viewangles[PITCH];

	if (!delta)
	{
		cl.pitchvel = 0;
		return;
	}

	move = host_frametime * cl.pitchvel;
	cl.pitchvel += host_frametime * v_centerspeed.value;

	if (delta > 0)
	{
		if (move > delta)
		{
			cl.pitchvel = 0;
			move = delta;
		}
		cl.viewangles[PITCH] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			cl.pitchvel = 0;
			move = -delta;
		}
		cl.viewangles[PITCH] -= move;
	}
}





/*
============================================================================== 
 
						PALETTE FLASHES 
 
============================================================================== 
*/ 
 
 
cshift_t	cshift_empty = { {130,80,50}, 0 };
cshift_t	cshift_water = { {130,80,50}, 128 };
cshift_t	cshift_slime = { {0,25,5}, 150 };
cshift_t	cshift_lava = { {255,80,0}, 150 };

/*
===============
CL_ParseDamage
===============
*/
void CL_ParseDamage (void)
{
	int		armor, blood;
	vec3_t	from;
	int		i;
	vec3_t	forward, right;
	centity_t	*ent;
	float	side;
	float	count;
	
	armor = MSG_ReadByte ();
	blood = MSG_ReadByte ();
	for (i=0 ; i<3 ; i++)
		from[i] = MSG_ReadCoord ();

	count = (blood + armor) * 0.5;
	if (count < 10)
		count = 10;

	cl.faceanimtime = cl.time + 0.2;		// but sbar face into pain frame

	if (armor > blood)
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 200;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 100;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 100;
	}
	else if (armor)
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 220;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 50;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 50;
	}
	else
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 255;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 0;
	}

	cl.cshifts[CSHIFT_DAMAGE].percent += 3*count;
	if (cl.cshifts[CSHIFT_DAMAGE].percent < 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;
	if (cl.cshifts[CSHIFT_DAMAGE].percent > 150)
		cl.cshifts[CSHIFT_DAMAGE].percent = 150;

//
// calculate view angle kicks
//
	ent = &cl_entities[cl.viewentity];

	VectorSubtract (from, ent->lerp_origin, from);
	VectorNormalize (from);
	
	AngleVectors (cl.viewangles, forward, right, NULL);

	side = DotProduct (from, right);
	v_dmg_roll = count*side*v_kickroll.value;
	
	side = DotProduct (from, forward);
	v_dmg_pitch = count*side*v_kickpitch.value;
	v_dmg_time = v_kicktime.value;
}


/*
==================
V_cshift_f
==================
*/
void V_cshift_f (void)
{
	cshift_empty.destcolor[0] = atoi(Cmd_Argv(1));
	cshift_empty.destcolor[1] = atoi(Cmd_Argv(2));
	cshift_empty.destcolor[2] = atoi(Cmd_Argv(3));
	cshift_empty.percent = atoi(Cmd_Argv(4));
}


/*
==================
V_BonusFlash_f

When you run over an item, the server sends this command
==================
*/
void V_BonusFlash_f (void)
{
	cl.cshifts[CSHIFT_BONUS].destcolor[0] = 215;
	cl.cshifts[CSHIFT_BONUS].destcolor[1] = 186;
	cl.cshifts[CSHIFT_BONUS].destcolor[2] = 69;
	cl.cshifts[CSHIFT_BONUS].percent = 50;
}

/*
=============
V_SetContentsColor

Underwater, lava, etc each has a color shift
=============
*/
void V_SetContentsColor (int contents)
{
	switch (contents)
	{
		case CONTENTS_EMPTY:
		case CONTENTS_SOLID:
			cl.cshifts[CSHIFT_CONTENTS] = cshift_empty;
			break;

		case CONTENTS_LAVA:
			cl.cshifts[CSHIFT_CONTENTS] = cshift_lava;
			break;

		case CONTENTS_SLIME:
			cl.cshifts[CSHIFT_CONTENTS] = cshift_slime;
			break;

		default:
			cl.cshifts[CSHIFT_CONTENTS] = cshift_water;
			break;
	}
}

/*
=============
V_CalcPowerupCshift
=============
*/
void V_CalcPowerupCshift (void)
{
	if (cl.items & IT_QUAD)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
		cl.cshifts[CSHIFT_POWERUP].percent = 30;
	}
	else if (cl.items & IT_SUIT)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		cl.cshifts[CSHIFT_POWERUP].percent = 20;
	}
	else if (cl.items & IT_INVISIBILITY)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 100;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 100;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
		cl.cshifts[CSHIFT_POWERUP].percent = 100;
	}
	else if (cl.items & IT_INVULNERABILITY)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
		cl.cshifts[CSHIFT_POWERUP].percent = 30;
		cl.cshifts[CSHIFT_POWERUP].percent = 30;
	}
	else
		cl.cshifts[CSHIFT_POWERUP].percent = 0;
}

/*
=============
CL_AddCshifts
=============
*/
void CL_ClearCshifts (void)
{
	int i;

	for (i = 0; i < NUM_CSHIFTS; i++)
		cl.cshifts[i].percent = 0;
}

/*
=============
CL_AddCshifts
=============
*/
void CL_AddCshifts (void)
{
	int i, j;
	qboolean newcs = false;

	V_CalcPowerupCshift ();

	for (i = 0; i < NUM_CSHIFTS; i++)
	{
		if (cl.cshifts[i].percent != cl.prev_cshifts[i].percent)
		{
			newcs = true;
			cl.prev_cshifts[i].percent = cl.cshifts[i].percent;
		}
		for (j=0 ; j<3 ; j++)
			if (cl.cshifts[i].destcolor[j] != cl.prev_cshifts[i].destcolor[j])
			{
				newcs = true;
				cl.prev_cshifts[i].destcolor[j] = cl.cshifts[i].destcolor[j];
			}
	}

// drop the damage value
	cl.cshifts[CSHIFT_DAMAGE].percent -= host_frametime*150;
	if (cl.cshifts[CSHIFT_DAMAGE].percent <= 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;

// drop the bonus value
	cl.cshifts[CSHIFT_BONUS].percent -= host_frametime*100;
	if (cl.cshifts[CSHIFT_BONUS].percent <= 0)
		cl.cshifts[CSHIFT_BONUS].percent = 0;

	if (!newcs)
		return;

	for (i = 0; i < NUM_CSHIFTS; i++)
	{
		if (cl.cshifts[i].percent >= 0)
			V_AddCshift (cl.cshifts[i].destcolor[0], cl.cshifts[i].destcolor[1], cl.cshifts[i].destcolor[2], cl.cshifts[i].percent);
	}
}

/* 
============================================================================== 
 
						VIEW RENDERING 
 
============================================================================== 
*/ 

vec3_t	chase_pos;
vec3_t	chase_angles;

vec3_t	chase_dest;
vec3_t	chase_dest_angles;

void TraceLine (vec3_t start, vec3_t end, vec3_t impact)
{
	trace_t	trace;

	trace = CL_TraceLine (start, vec3_origin, vec3_origin, end, -1);

	VectorCopy (trace.endpos, impact);
}

void Chase_Update (void)
{
	int		i;
	float	dist;
	vec3_t	forward, right;
	vec3_t	dest, stop;

	// if can't see player, reset
	AngleVectors (cl.viewangles, forward, right, NULL);

	// calc exact destination
	for (i=0 ; i<3 ; i++)
		chase_dest[i] = cl.refdef.vieworg[i] 
		- forward[i]*chase_back.value
		- right[i]*chase_right.value;
	chase_dest[2] = cl.refdef.vieworg[2] + chase_up.value;

	// find the spot the player is looking at
	VectorMA (cl.refdef.vieworg, 4096, forward, dest);
	TraceLine (cl.refdef.vieworg, dest, stop);

	// calculate pitch to look at the same spot from camera
	VectorSubtract (stop, cl.refdef.vieworg, stop);
	dist = DotProduct (stop, forward);
	if (dist < 1)
		dist = 1;
	cl.refdef.viewangles[PITCH] = -atan(stop[2] / dist) / M_PI * 180;

	TraceLine (cl.refdef.vieworg, chase_dest, stop);
	if (!VectorCompare(stop, vec3_origin))
		VectorCopy(stop, chase_dest);

	// move towards destination
	VectorCopy (chase_dest, cl.refdef.vieworg);
}

//=============================================================================

float angledelta (float a)
{
	a = anglemod(a);
	if (a > 180)
		a -= 360;
	return a;
}

/*
====================
CalcFov
====================
*/
float CalcFov (float fov_x, float width, float height)
{
	float   x;
	
	if (fov_x < 1 || fov_x > 179)
		Sys_Error ("Bad fov: %f", fov_x);
	
	x = width/tan(fov_x*(M_PI/360.0));
	
	return atan (height/x)*(360.0/M_PI);
}

/*
==============
V_BoundOffsets
==============
*/
void V_BoundOffsets (void)
{
	centity_t	*ent = &cl_entities[cl.viewentity];
	
// absolutely bound refresh reletive to entity clipping hull
// so the view can never be inside a solid wall

	if (cl.refdef.vieworg[0] < ent->lerp_origin[0] - 14)
		cl.refdef.vieworg[0] = ent->lerp_origin[0] - 14;
	else if (cl.refdef.vieworg[0] > ent->lerp_origin[0] + 14)
		cl.refdef.vieworg[0] = ent->lerp_origin[0] + 14;
	if (cl.refdef.vieworg[1] < ent->lerp_origin[1] - 14)
		cl.refdef.vieworg[1] = ent->lerp_origin[1] - 14;
	else if (cl.refdef.vieworg[1] > ent->lerp_origin[1] + 14)
		cl.refdef.vieworg[1] = ent->lerp_origin[1] + 14;
	if (cl.refdef.vieworg[2] < ent->lerp_origin[2] - 22)
		cl.refdef.vieworg[2] = ent->lerp_origin[2] - 22;
	else if (cl.refdef.vieworg[2] > ent->lerp_origin[2] + 30)
		cl.refdef.vieworg[2] = ent->lerp_origin[2] + 30;
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
void V_AddIdle (float scale)
{
	cl.refdef.viewangles[ROLL] += scale * sin(cl.time*v_iroll_cycle.value) * v_iroll_level.value;
	cl.refdef.viewangles[PITCH] += scale * sin(cl.time*v_ipitch_cycle.value) * v_ipitch_level.value;
	cl.refdef.viewangles[YAW] += scale * sin(cl.time*v_iyaw_cycle.value) * v_iyaw_level.value;
}

/*
==============
V_AddIdle

==============
*/
void V_AddGun (float bob)
{
	entity_t	gun;
	centity_t	*view = &cl.viewent;
	vec3_t		forward;

	if (chase_active.value || 
		(cl.items & IT_INVISIBILITY) || 
		(cl.stats[STAT_HEALTH] <= 0)) {
		return;
	}

// set up gun position
	memset (&gun, 0, sizeof(gun));
	
	gun.model = cl.model_precache[view->current.modelindex];
	if (!gun.model)
		return;

	gun.frame = view->current.frame;
	gun.oldframe = view->prev.frame;
	gun.flags = RF_WEAPONMODEL|RF_DEPTHSCALE|RF_MINLIGHT|RF_NOSHADOW;
	gun.framelerp = (cl.time - view->frametime) * 10;
	gun.framelerp = bound (0, gun.framelerp, 1);
	gun.colormap = vid.colormap;

	VectorCopy (cl.refdef.vieworg, gun.origin);
	VectorSet (gun.angles, -cl.refdef.viewangles[PITCH], cl.refdef.viewangles[YAW], cl.viewangles[ROLL]);

// add idle swaying
	gun.angles[ROLL] -= v_idlescale.value * sin(cl.time*v_iroll_cycle.value) * v_iroll_level.value;
	gun.angles[PITCH] -= v_idlescale.value * sin(cl.time*v_ipitch_cycle.value) * v_ipitch_level.value;
	gun.angles[YAW] -= v_idlescale.value * sin(cl.time*v_iyaw_cycle.value) * v_iyaw_level.value;

	AngleVectors (gun.angles, forward, NULL, NULL);
	VectorMA (gun.origin, bob * 0.4, forward, gun.origin);

	V_AddEntity (&gun);
}

/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
void V_CalcViewRoll (void)
{
	float		side;
		
	side = V_CalcRoll (cl.viewangles, cl.velocity);
	cl.refdef.viewangles[ROLL] += side;

	if (v_dmg_time > 0)
	{
		cl.refdef.viewangles[ROLL] += v_dmg_time/v_kicktime.value*v_dmg_roll;
		cl.refdef.viewangles[PITCH] += v_dmg_time/v_kicktime.value*v_dmg_pitch;
		v_dmg_time -= host_frametime;
	}

	if (cl.stats[STAT_HEALTH] <= 0)
		cl.refdef.viewangles[ROLL] = 80;	// dead view angle
}

/*
==================
V_CalcIntermissionRefdef

==================
*/
void V_CalcIntermissionRefdef (void)
{
	centity_t	*cent;
	int			contents;

// ent is the player model (visible when out of body)
	cent = &cl_entities[cl.viewentity];

	VectorCopy (cent->current.origin, cl.refdef.vieworg);
	VectorCopy (cent->current.angles, cl.refdef.viewangles);

// always idle in intermission
	V_AddIdle (1);

// never let it sit exactly on a node line, because a water plane can
// dissapear when viewed with the eye exactly on it.
// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	cl.refdef.vieworg[0] += 1.0/32;
	cl.refdef.vieworg[1] += 1.0/32;
	cl.refdef.vieworg[2] += 1.0/32;

	cl.refdef.x = scr_vrect.x;
	cl.refdef.y = scr_vrect.y;
	cl.refdef.width = scr_vrect.width;
	cl.refdef.height = scr_vrect.height;
	cl.refdef.fov_x = scr_fov.value;
	cl.refdef.fov_y = CalcFov (cl.refdef.fov_x, cl.refdef.width, cl.refdef.height);

// set view contents
	contents = CM_PointContents (cl.refdef.vieworg);
	if (contents <= CONTENTS_WATER)
		cl.refdef.flags = RDF_UNDERWATER;
	else
		cl.refdef.flags = 0;

	V_SetContentsColor (contents);
}

/*
==================
V_CalcRefdef

==================
*/
void V_CalcRefdef (void)
{
	centity_t	*cent;
	float		bob;
	int			contents;
	static float oldz = 0;

// ent is the player model (visible when out of body)
	cent = &cl_entities[cl.viewentity];

	CL_DriftPitch ();
	bob = V_CalcBob ();

// transform the view offset by the model's matrix to get the offset from
// model origin for the view

// refresh position
	VectorSet (cl.refdef.vieworg, cent->lerp_origin[0], cent->lerp_origin[1], cent->lerp_origin[2] + cl.viewheight + bob);

// never let it sit exactly on a node line, because a water plane can
// dissapear when viewed with the eye exactly on it.
// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	cl.refdef.vieworg[0] += 1.0/32;
	cl.refdef.vieworg[1] += 1.0/32;
	cl.refdef.vieworg[2] += 1.0/32;

	cl.refdef.x = scr_vrect.x;
	cl.refdef.y = scr_vrect.y;
	cl.refdef.width = scr_vrect.width;
	cl.refdef.height = scr_vrect.height;
	cl.refdef.fov_x = scr_fov.value;
	cl.refdef.fov_y = CalcFov (cl.refdef.fov_x, cl.refdef.width, cl.refdef.height);

	VectorCopy (cl.viewangles, cl.refdef.viewangles);

	V_CalcViewRoll ();
	V_AddIdle (v_idlescale.value);

// offsets
	V_BoundOffsets ();

// set up the refresh position
	VectorAdd (cl.refdef.viewangles, cl.punchangle, cl.refdef.viewangles);
	AngleVectors (cl.refdef.viewangles, cl.v_forward, cl.v_right, cl.v_up);

// smooth out stair step ups
	if (cl.onground && cent->lerp_origin[2] - oldz > 0)
	{
		float steptime = cl.time - cl.oldtime;

		if (steptime < 0)
			steptime = 0;

		oldz += steptime * 80;
		if (oldz > cent->lerp_origin[2])
			oldz = cent->lerp_origin[2];
		if (cent->lerp_origin[2] - oldz > 12)
			oldz = cent->lerp_origin[2] - 12;
		cl.refdef.vieworg[2] += oldz - cent->lerp_origin[2];
	}
	else
		oldz = cent->lerp_origin[2];

// set view contents
	contents = CM_PointContents (cl.refdef.vieworg);
	if (contents <= CONTENTS_WATER)
		cl.refdef.flags = RDF_UNDERWATER;
	else
		cl.refdef.flags = 0;

	V_SetContentsColor (contents);

// add gun
	V_AddGun (bob);

	if (chase_active.value)
		Chase_Update ();
}

/*
==================
V_RenderView

The player's clipping box goes from (-16 -16 -24) to (16 16 32) from
the entity origin, so any view position inside that will be valid
==================
*/
void V_RenderView (void)
{
	if (con_forcedup)
		return;
	if (Com_ClientState() != ca_connected)
		return;

	if (!cl.paused)
	{
		V_ClearScene ();

		if (cl.intermission)	// intermission / finale rendering
			V_CalcIntermissionRefdef ();
		else
			V_CalcRefdef ();

		CL_AddEntities ();

		if (cl_add_statice_ents.value)
			CL_AddStaticEntities ();

		CL_AddParticles ();
		CL_AddDlights ();
		CL_AddCshifts ();
		CL_AddTempEntities ();

	// set up the refresh information
		cl.refdef.time = cl.time;
		cl.refdef.oldtime = cl.oldtime;

		cl.refdef.lightstyles = cl_lightstyle;

		cl.refdef.entities = r_entities;
		cl.refdef.num_entities = r_numentities;

		cl.refdef.particles = r_particles;
		cl.refdef.num_particles = r_numparticles;

		cl.refdef.dlights = r_dlights;
		cl.refdef.num_dlights = r_numdlights;

		cl.refdef.cshifts = r_cshifts;
		cl.refdef.num_cshifts = r_numcshifts;

		if (!cl_add_entities.value)
			cl.refdef.num_entities = 0;
		if (!cl_add_particles.value)
			cl.refdef.num_particles = 0;
		if (!cl_add_lights.value)
			cl.refdef.num_dlights = 0;
		if (!cl_add_cshifts.value)
			cl.refdef.num_cshifts = 0;
	}

	R_RenderView (&cl.refdef);
}

//============================================================================

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	Cmd_AddCommand ("v_cshift", V_cshift_f);	
	Cmd_AddCommand ("bf", V_BonusFlash_f);
	Cmd_AddCommand ("centerview", CL_StartPitchDrift);

	Cvar_RegisterVariable (&v_centermove);
	Cvar_RegisterVariable (&v_centerspeed);

	Cvar_RegisterVariable (&v_iyaw_cycle);
	Cvar_RegisterVariable (&v_iroll_cycle);
	Cvar_RegisterVariable (&v_ipitch_cycle);
	Cvar_RegisterVariable (&v_iyaw_level);
	Cvar_RegisterVariable (&v_iroll_level);
	Cvar_RegisterVariable (&v_ipitch_level);

	Cvar_RegisterVariable (&v_idlescale);

	Cvar_RegisterVariable (&cl_rollspeed);
	Cvar_RegisterVariable (&cl_rollangle);
	Cvar_RegisterVariable (&cl_bob);
	Cvar_RegisterVariable (&cl_bobcycle);
	Cvar_RegisterVariable (&cl_bobup);

	Cvar_RegisterVariable (&cl_add_entities);
	Cvar_RegisterVariable (&cl_add_statice_ents);
	Cvar_RegisterVariable (&cl_add_particles);
	Cvar_RegisterVariable (&cl_add_lights);
	Cvar_RegisterVariable (&cl_add_cshifts);

	Cvar_RegisterVariable (&v_kicktime);
	Cvar_RegisterVariable (&v_kickroll);
	Cvar_RegisterVariable (&v_kickpitch);

	Cvar_RegisterVariable (&chase_back);
	Cvar_RegisterVariable (&chase_up);
	Cvar_RegisterVariable (&chase_right);
	Cvar_RegisterVariable (&chase_active);
}
