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

#include "client.h"

#define ABSOLUTE_MIN_PARTICLES	512		// no fewer than this no matter what's
										//  on the command line

typedef enum {
	pt_static, pt_grav, pt_slowgrav, pt_fire, pt_explode, pt_explode2, pt_blob, pt_blob2,
		pt_railtrail
} ptype_t;

int		ramp1[8] = {0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61};
int		ramp2[8] = {0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66};
int		ramp3[8] = {0x6d, 0x6b, 6, 5, 4, 3};

cparticle_t	*active_particles, *free_particles;

cparticle_t	particles[MAX_PARTICLES];
int			cl_numparticles;

vec3_t		r_pright, r_pup, r_ppn;

void CL_ReadPointFile_f (void);

/*
===============
CL_InitParticles
===============
*/
void CL_InitParticles (void)
{
	int		i;

	Cmd_AddCommand ("pointfile", CL_ReadPointFile_f);	

	i = COM_CheckParm ("-particles");
	if (i)
	{
		cl_numparticles = (int)(atoi(com_argv[i+1]));
		if (cl_numparticles < ABSOLUTE_MIN_PARTICLES)
			cl_numparticles = ABSOLUTE_MIN_PARTICLES;
	}
	else
	{
		cl_numparticles = MAX_PARTICLES;
	}
}

cparticle_t *new_particle (void)
{
	cparticle_t *p;

	if (!free_particles)
		return NULL;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	return p;
}

/*
===============
CL_EntityParticles
===============
*/

#define NUMVERTEXNORMALS	162
float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

vec3_t	avelocities[NUMVERTEXNORMALS];
float	beamlength = 16;
vec3_t	avelocity = {23, 7, 3};
float	partstep = 0.01;
float	timescale = 0.01;

void CL_EntityParticles (entity_t *ent)
{
	int			i;
	cparticle_t	*p;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;

	if (!avelocities[0][0])
	{
		for (i=0 ; i<NUMVERTEXNORMALS*3 ; i++)
			avelocities[0][i] = (rand()&255) * 0.01;
	}

	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		angle = cl.time * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = cl.time * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		angle = cl.time * avelocities[i][2];
		sr = sin(angle);
		cr = cos(angle);
	
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		if (!(p = new_particle ()))
			return;

		p->die = cl.time + 0.01;
		p->color = 0x6f;
		p->type = pt_explode;
		VectorClear (p->vel);
		
		p->org[0] = ent->origin[0] + r_avertexnormals[i][0]*64 + forward[0]*beamlength;			
		p->org[1] = ent->origin[1] + r_avertexnormals[i][1]*64 + forward[1]*beamlength;			
		p->org[2] = ent->origin[2] + r_avertexnormals[i][2]*64 + forward[2]*beamlength;			
	}
}


/*
===============
CL_ClearParticles
===============
*/
void CL_ClearParticles (void)
{
	int		i;
	
	free_particles = &particles[0];
	active_particles = NULL;

	for (i=0 ;i<cl_numparticles ; i++)
		particles[i].next = &particles[i+1];
	particles[cl_numparticles-1].next = NULL;
}

void CL_ReadPointFile_f (void)
{
	FILE	*f;
	vec3_t	org;
	int		r;
	int		c;
	cparticle_t	*p;
	char	name[MAX_OSPATH];

	COM_StripExtension (CM_MapName(), name);
	strcat (name, ".pts");

	COM_FOpenFile (name, &f);
	if (!f)
	{
		Con_Printf ("couldn't open %s\n", name);
		return;
	}
	
	Con_Printf ("Reading %s...\n", name);
	c = 0;
	for ( ;; )
	{
		r = fscanf (f,"%f %f %f\n", &org[0], &org[1], &org[2]);
		if (r != 3)
			break;
		c++;
		
		if (!(p = new_particle()))
		{
			Con_Printf ("Not enough free particles\n");
			break;
		}
		
		p->die = 99999;
		p->color = (-c)&15;
		p->type = pt_static;
		VectorClear (p->vel);
		VectorCopy (org, p->org);
	}

	fclose (f);
	Con_Printf ("%i points read\n", c);
}

/*
===============
CL_ParseParticleEffect

Parse an effect out of the server message
===============
*/
void CL_ParseParticleEffect (void)
{
	vec3_t		org, dir;
	int			i, count, color;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	for (i=0 ; i<3 ; i++)
		dir[i] = MSG_ReadChar () * (1.0/16);
	count = MSG_ReadByte ();
	color = MSG_ReadByte ();

	if (count == 255)
		CL_RocketSplash (org, dir, color);
	else
		CL_RunParticleEffect (org, dir, color, count);
}
	
/*
===============
CL_ParticleExplosion

===============
*/
void CL_ParticleExplosion (vec3_t org)
{
	int			i, j;
	cparticle_t	*p;
	
	for (i=0 ; i<1024 ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->die = cl.time + 5;
		p->color = ramp1[0];
		p->ramp = rand()&3;
		if (i & 1)
		{
			p->type = pt_explode;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
		else
		{
			p->type = pt_explode2;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
	}
}

/*
===============
CL_ParticleExplosion2

===============
*/
void CL_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength)
{
	int			i, j;
	cparticle_t	*p;
	int			colorMod = 0;

	for (i=0; i<512; i++)
	{
		if (!(p = new_particle()))
			return;

		p->die = cl.time + 0.3;
		p->color = colorStart + (colorMod % colorLength);
		colorMod++;

		p->type = pt_blob;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()%32)-16);
			p->vel[j] = (rand()%512)-256;
		}
	}
}

/*
===============
CL_BlobExplosion

===============
*/
void CL_BlobExplosion (vec3_t org)
{
	int			i, j;
	cparticle_t	*p;
	
	for (i=0 ; i<1024 ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->die = cl.time + 1 + (rand()&8)*0.05;

		if (i & 1)
		{
			p->type = pt_blob;
			p->color = 66 + rand()%6;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
		else
		{
			p->type = pt_blob2;
			p->color = 150 + rand()%6;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
	}
}

/*
===============
CL_RunParticleEffect

===============
*/
void CL_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j;
	cparticle_t	*p;
	
	for (i=0 ; i<count ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->die = cl.time + 0.1*(rand()%5);
		p->color = (color&~7) + (rand()&7);
		p->type = pt_slowgrav;

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()&15)-8);
			p->vel[j] = dir[j]*15;
		}
	}
}

/*
===============
CL_RocketSplash

===============
*/
void CL_RocketSplash (vec3_t org, vec3_t dir, int color)
{
	int			i, j;
	cparticle_t	*p;
	
	for (i=0 ; i<1024 ; i++)
	{
		if (!(p = new_particle()))
			return;

		// rocket explosion
		p->die = cl.time + 5;
		p->color = ramp1[0];
		p->ramp = rand()&3;
		if (i & 1)
		{
			p->type = pt_explode;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
		else
		{
			p->type = pt_explode2;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
	}
}

/*
===============
C_LavaSplash

===============
*/
void CL_LavaSplash (vec3_t org)
{
	int			i, j, k;
	cparticle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<16 ; i++)
		for (j=-16 ; j<16 ; j++)
			for (k=0 ; k<1 ; k++)
			{
				if (!(p = new_particle()))
					return;

				p->die = cl.time + 2 + (rand()&31) * 0.02;
				p->color = 224 + (rand()&7);
				p->type = pt_slowgrav;
				
				dir[0] = j*8 + (rand()&7);
				dir[1] = i*8 + (rand()&7);
				dir[2] = 256;
	
				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + (rand()&63);
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}

void CL_RailTrail (vec3_t start, vec3_t end)
{
	vec3_t		vec, org, vel, right;
	float		len, roll = 0.0f;
	float       sr, sp, sy, cr, cp, cy;
	cparticle_t	*p;

	VectorSubtract (end, start, vec);
	Vector2Angles (vec, org);

	org[0] *= (M_PI * 2 / 360);
	org[1] *= (M_PI * 2 / 360);

	sp = sin (org[0]);
	cp = cos (org[0]);
	sy = sin (org[1]);
	cy = cos (org[1]);

	len = VectorNormalize(vec);
	VectorScale (vec, 1.5, vec);
	VectorSet (right, sy, cy, 0);

	while (len > 0)
	{
		if (!(p = new_particle()))
			return;

		VectorCopy (start, p->org);
		VectorClear (p->vel);
		p->color = (rand() & 3) + 225;
		p->type = pt_railtrail;
		p->die = cl.time + 1.0;

		if (!(p = new_particle()))
			return;

		VectorMA (start, 4, right, org);
		VectorScale (right, 8, vel);

		VectorCopy (org, p->org);
		VectorCopy (vel, p->vel);
		p->color = (rand() & 7) + 206;
		p->type = pt_railtrail;
		p->die = cl.time + 1.0;

		roll += 7.5 * (M_PI / 180.0); 

		if (roll > 2*M_PI)
			roll -= 2*M_PI;

		sr = sin (roll);
		cr = cos (roll);
		right[0] = (cr * sy - sr * sp * cy);
		right[1] = -(sr * sp * sy + cr * cy);
		right[2] = -sr * cp;

		len -= 1.5;
		VectorAdd (start, vec, start);
	}
}

/*
===============
CL_TeleportSplash

===============
*/
void CL_TeleportSplash (vec3_t org)
{
	int			i, j, k;
	cparticle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<16 ; i+=4)
		for (j=-16 ; j<16 ; j+=4)
			for (k=-24 ; k<32 ; k+=4)
			{
				if (!(p = new_particle()))
					return;
		
				p->die = cl.time + 0.2 + (rand()&7) * 0.02;
				p->color = 7 + (rand()&7);
				p->type = pt_slowgrav;
				
				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;
	
				p->org[0] = org[0] + i + (rand()&3);
				p->org[1] = org[1] + j + (rand()&3);
				p->org[2] = org[2] + k + (rand()&3);
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}

/*
===============
CL_RocketTrail
===============
*/
void CL_RocketTrail (vec3_t start, vec3_t end)
{
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	float		dec;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	dec = 3;
	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if (!(p = new_particle()))
			return;
		
		VectorClear (p->vel);
		p->die = cl.time + 2;
		p->ramp = (rand()&3);
		p->color = ramp3[(int)p->ramp];
		p->type = pt_fire;
		for (j=0 ; j<3 ; j++)
			p->org[j] = start[j] + ((rand()%6)-3);

		VectorAdd (start, vec, start);
	}
}

/*
===============
CL_GrenadeTrail
===============
*/
void CL_GrenadeTrail (vec3_t start, vec3_t end)
{
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	float		dec;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	dec = 3;
	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if (!(p = new_particle()))
			return;
		
		VectorClear (p->vel);
		p->die = cl.time + 2;
		p->ramp = (rand()&3) + 2;
		p->color = ramp3[(int)p->ramp];
		p->type = pt_fire;
		for (j=0 ; j<3 ; j++)
			p->org[j] = start[j] + ((rand()%6)-3);

		VectorAdd (start, vec, start);
	}
}

/*
===============
CL_BloodTrail
===============
*/
void CL_BloodTrail (vec3_t start, vec3_t end)
{
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	float		dec;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	dec = 3;
	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if (!(p = new_particle()))
			return;
		
		VectorClear (p->vel);
		p->die = cl.time + 2;
		p->type = pt_grav;
		p->color = 67 + (rand()&3);
		for (j=0 ; j<3 ; j++)
			p->org[j] = start[j] + ((rand()%6)-3);

		VectorAdd (start, vec, start);
	}
}

/*
===============
CL_SlightBloodTrail
===============
*/
void CL_SlightBloodTrail (vec3_t start, vec3_t end)
{
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	float		dec;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	dec = 3;
	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if (!(p = new_particle()))
			return;
		
		VectorClear (p->vel);
		p->die = cl.time + 2;
		p->type = pt_grav;
		p->color = 67 + (rand()&3);
		for (j=0 ; j<3 ; j++)
			p->org[j] = start[j] + ((rand()%6)-3);

		len -= 3;
		VectorAdd (start, vec, start);
	}
}

/*
===============
CL_TracerTrail
===============
*/
void CL_TracerTrail (vec3_t start, vec3_t end, int color)
{
	vec3_t		vec;
	float		len;
	cparticle_t	*p;
	float		dec;
	static int	tracercount;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	dec = 3;
	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if (!(p = new_particle()))
			return;
		
		p->die = cl.time + 0.5;
		p->type = pt_static;
		p->color = color + ((tracercount&4)<<1);

		tracercount++;

		VectorClear (p->vel);
		VectorCopy (start, p->org);
		if (tracercount & 1)
		{
			p->vel[0] = 30*vec[1];
			p->vel[1] = 30*-vec[0];
		}
		else
		{
			p->vel[0] = 30*-vec[1];
			p->vel[1] = 30*vec[0];
		}

		VectorAdd (start, vec, start);
	}
}

/*
===============
CL_VoorTrail
===============
*/
void CL_VoorTrail (vec3_t start, vec3_t end)
{
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	float		dec;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	dec = 3;
	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if (!(p = new_particle()))
			return;
		
		p->die = cl.time + 0.3;
		p->color = 9*16 + 8 + (rand()&3);
		p->type = pt_static;

		VectorClear (p->vel);
		for (j=0 ; j<3 ; j++)
			p->org[j] = start[j] + ((rand()&15)-8);

		VectorAdd (start, vec, start);
	}
}

/*
===============
CL_AddParticles
===============
*/
void CL_AddParticles (void)
{
	cparticle_t		*p, *kill;
	float			grav;
	int				i;
	float			time2, time3;
	float			time1;
	float			dvel;
	float			frametime;

	if (!active_particles)
		return;

	frametime = cl.time - cl.oldtime;
	time3 = frametime * 15;
	time2 = frametime * 10; // 15;
	time1 = frametime * 5;
	grav = frametime * 800 * 0.05;
	dvel = 4*frametime;
	
	for ( ;; ) 
	{
		kill = active_particles;
		if (kill && kill->die < cl.time)
		{
			active_particles = kill->next;
			kill->next = free_particles;
			free_particles = kill;
			continue;
		}
		break;
	}

	for (p=active_particles ; p ; p=p->next)
	{
		for ( ;; )
		{
			kill = p->next;
			if (kill && kill->die < cl.time)
			{
				p->next = kill->next;
				kill->next = free_particles;
				free_particles = kill;
				continue;
			}
			break;
		}

		V_AddParticle (p->org, p->color);

		p->org[0] += p->vel[0]*frametime;
		p->org[1] += p->vel[1]*frametime;
		p->org[2] += p->vel[2]*frametime;
		
		switch (p->type)
		{
		case pt_static:
			break;
		case pt_fire:
			p->ramp += time1;
			if (p->ramp >= 6)
				p->die = -1;
			else
				p->color = ramp3[(int)p->ramp];
			p->vel[2] += grav;
			break;

		case pt_explode:
			p->ramp += time2;
			if (p->ramp >=8)
				p->die = -1;
			else
				p->color = ramp1[(int)p->ramp];
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_explode2:
			p->ramp += time3;
			if (p->ramp >=8)
				p->die = -1;
			else
				p->color = ramp2[(int)p->ramp];
			for (i=0 ; i<3 ; i++)
				p->vel[i] -= p->vel[i]*frametime;
			p->vel[2] -= grav;
			break;

		case pt_blob:
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_blob2:
			for (i=0 ; i<2 ; i++)
				p->vel[i] -= p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_grav:
		case pt_slowgrav:
			p->vel[2] -= grav;
			break;

		case pt_railtrail:
			break;
		}
	}
}

//=============================================================================

/*
===============
CL_AllocDlight

===============
*/
cdlight_t *CL_AllocDlight (int key)
{
	int		i;
	cdlight_t	*dl;

// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;
		for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
		{
			if (dl->key == key)
			{
				memset (dl, 0, sizeof(*dl));
				dl->key = key;
				return dl;
			}
		}
	}

// then look for anything else
	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time)
		{
			memset (dl, 0, sizeof(*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	memset (dl, 0, sizeof(*dl));
	dl->key = key;
	return dl;
}

/*
===============
CL_AddDlights

===============
*/
void CL_AddDlights (void)
{
	int			i;
	cdlight_t	*dl;
	float		time;
	
	time = cl.time - cl.oldtime;

	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time || !dl->radius)
			continue;
		
		V_AddDlight (dl->origin, dl->radius);

		dl->radius -= time*dl->decay;
		if (dl->radius < 0)
			dl->radius = 0;
	}
}
