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
// cl_main.c  -- client main loop

#include "client.h"

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

// these two are not intended to be set directly
cvar_t	cl_name = {"_cl_name", "player", true};
cvar_t	cl_color = {"_cl_color", "0", true};

cvar_t	cl_shownet = {"cl_shownet","0"};	// can be 0, 1, or 2
cvar_t	cl_nolerp = {"cl_nolerp","0"};

cvar_t	cl_bobbingitems = { "cl_bobbingitems", "0", true };

cvar_t	lookspring = {"lookspring","0", true};
cvar_t	lookstrafe = {"lookstrafe","0", true};
cvar_t	sensitivity = {"sensitivity","3", true};

cvar_t	m_pitch = {"m_pitch","0.022", true};
cvar_t	m_yaw = {"m_yaw","0.022", true};
cvar_t	m_forward = {"m_forward","1", true};
cvar_t	m_side = {"m_side","0.8", true};


client_static_t	cls;
client_state_t	cl;

// FIXME: put these on hunk?
centity_t		cl_entities[MAX_EDICTS];
entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
cdlight_t		cl_dlights[MAX_DLIGHTS];

int				cl_parse_entities[MAX_PARSE_ENTITIES];

/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState (void)
{
	extern float scr_centertime_off;

	if (!Com_ServerState())
		Host_ClearMemory ();

// wipe the entire cl structure
	memset (&cl, 0, sizeof(cl));

	scr_centertime_off = 0;

	SZ_Clear (&cls.message);

// clear other arrays	
	memset (cl_entities, 0, sizeof(cl_entities));
	memset (cl_dlights, 0, sizeof(cl_dlights));
	memset (cl_lightstyle, 0, sizeof(cl_lightstyle));
	memset (cl_temp_entities, 0, sizeof(cl_temp_entities));
	memset (cl_beams, 0, sizeof(cl_beams));
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void)
{
// stop sounds (especially looping!)
	S_StopAllSounds (true);

// clear effects
	CL_ClearCshifts ();
	
// if running a local server, shut it down
	if (cls.demoplayback)
		CL_StopPlayback ();
	else if (Com_ClientState () == ca_connected)
	{
		if (cls.demorecording)
			CL_Stop_f ();

		Con_DPrintf ("Sending clc_disconnect\n");
		SZ_Clear (&cls.message);
		MSG_WriteByte (&cls.message, clc_disconnect);
		NET_SendUnreliableMessage (cls.netcon, &cls.message);
		SZ_Clear (&cls.message);
		NET_Close (cls.netcon);

		Com_SetClientState (ca_disconnected);

		if (Com_ServerState())
			Host_ShutdownServer(false);
	}

	cls.demoplayback = cls.timedemo = false;
	cls.signon = 0;
}

void CL_Disconnect_f (void)
{
	CL_Disconnect ();
	if (Com_ServerState())
		Host_ShutdownServer (false);
}




/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address to be passed on
=====================
*/
void CL_EstablishConnection (char *host)
{
	if (Com_ClientState() == ca_dedicated)
		return;
	if (cls.demoplayback)
		return;

	CL_Disconnect ();

	cls.netcon = NET_Connect (host);
	if (!cls.netcon)
		Host_Error ("CL_Connect: connect failed\n");
	Con_DPrintf ("CL_EstablishConnection: connected to %s\n", host);
	
	cls.demonum = -1;			// not in the demo loop now
	cls.signon = 0;				// need all the signon messages before playing
	Com_SetClientState (ca_connected);
}

/*
===================
Cmd_ForwardToServer

Sends the entire command line over to the server
===================
*/
void Cmd_ForwardToServer (void)
{
	if (Com_ClientState() != ca_connected)
	{
		Con_Printf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}
	
	if (cls.demoplayback)
		return;		// not really connected

	MSG_WriteByte (&cls.message, clc_stringcmd);
	if (Q_strcasecmp(Cmd_Argv(0), "cmd") != 0)
	{
		SZ_Print (&cls.message, Cmd_Argv(0));
		SZ_Print (&cls.message, " ");
	}
	if (Cmd_Argc() > 1)
		SZ_Print (&cls.message, Cmd_Args());
	else
		SZ_Print (&cls.message, "\n");
}

/*
=====================
CL_SignonReply

An svc_signonnum has been received, perform a client side setup
=====================
*/
void CL_SignonReply (void)
{
	char 	str[8192];

	Con_DPrintf ("CL_SignonReply: %i\n", cls.signon);

	switch (cls.signon)
	{
	case 1:
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, "prespawn");
		break;
		
	case 2:		
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, va("name \"%s\"\n", cl_name.string));
	
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, va("color %i %i\n", ((int)cl_color.value)>>4, ((int)cl_color.value)&15));
	
		MSG_WriteByte (&cls.message, clc_stringcmd);
		sprintf (str, "spawn %s", cls.spawnparms);
		MSG_WriteString (&cls.message, str);
		break;
		
	case 3:	
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, "begin");
		Cache_Report ();		// print remaining memory
		break;
		
	case 4:
		SCR_EndLoadingPlaque ();		// allow normal screen updates
		break;
	}
}

/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo (void)
{
	char	str[1024];

	if (cls.demonum == -1)
		return;		// don't play demos

	SCR_BeginLoadingPlaque ();

	if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS)
	{
		cls.demonum = 0;
		if (!cls.demos[cls.demonum][0])
		{
			Con_Printf ("No demos listed with startdemos\n");
			cls.demonum = -1;
			return;
		}
	}

	sprintf (str,"playdemo %s\n", cls.demos[cls.demonum]);
	Cbuf_InsertText (str);
	cls.demonum++;
}

/*
===============
CL_LerpPoint

Determines the fraction between the last two messages that the objects
should be put at.
===============
*/
float CL_LerpPoint (void)
{
	float	f, frac;

	if (cl.oldframe.time < cl.frame.time - 0.1)
	{	// dropped packet, or start of demo
		cl.oldframe.time = cl.frame.time - 0.1;
	}

	cl.time = bound (cl.oldframe.time, cl.time, cl.frame.time);

	f = cl.frame.time - cl.oldframe.time;
	if (!f || cl_nolerp.value || cls.timedemo || (Com_ServerState() && Com_ServerMaxclients() == 1))
	{
		cl.time = cl.frame.time;
		return 1;
	}

	frac = (cl.time - cl.oldframe.time) / f;
	return bound (0, frac, 1);
}

#define BOB_SCALE	180

static float itembob[BOB_SCALE];

/*
===============
CL_BuildBobTable
===============
*/
void CL_BuildBobTable (void)
{
	int i;

	for (i = 0; i < BOB_SCALE; i++)
		itembob[i] = sin (i / 90.0f * M_PI) * 5 + 5;
}
		
/*
===============
CL_AddEntities
===============
*/
void CL_AddEntities (void)
{
	int			i, num;
	entity_t	ent;
	centity_t	*cent;
	int			effects, flags;
	float		frac, f;
	float		bobjrotate;
	vec3_t		oldorg;
	cdlight_t	*dl;

	// determine partial update time	
	frac = CL_LerpPoint ();

	//
	// interpolate player info
	//
	LerpVectors (cl.mvelocity[1], frac, cl.mvelocity[0], cl.velocity);

	if (cls.demoplayback)
	{	// interpolate the angles
		LerpAngles (cl.mviewangles[1], frac, cl.mviewangles[0], cl.viewangles);
	}
	
	// add all visible entities that were included in the last packet
	for (i=0 ; i<cl.frame.num_entities ; i++)
	{
		num = cl_parse_entities[(cl.frame.parse_entities+i)&(MAX_PARSE_ENTITIES-1)];
		cent = &cl_entities[num];

		if (!cent->current.modelindex)
		{	// empty slot
			continue;
		}

		memset (&ent, 0, sizeof(ent));

		ent.model = cl.model_precache[cent->current.modelindex];
		if (!ent.model)
			continue;

		if (cent->deltalerp > 0)
		{
			f = (cl.time - cent->startlerp) / cent->deltalerp;
			f = bound (0, f, 1);
		}
		else
		{
			f = 1;
		}

		// interpolate the origin and angles
		LerpVectors (cent->prev.origin, f, cent->current.origin, ent.origin);
		LerpAngles (cent->prev.angles, f, cent->current.angles, ent.angles);

		// save current origin for trails
		flags = Mod_Flags (ent.model);
		effects = cent->current.effects;
		VectorCopy (cent->lerp_origin, oldorg);
		VectorCopy (ent.origin, cent->lerp_origin);

		if (!cent->current.colormap)
			ent.colormap = vid.colormap;
		else
			ent.colormap = cl.scores[cent->current.colormap-1].translations;

		ent.number = num;
		ent.syncbase = cent->syncbase;
		ent.oldframe = cent->prev.frame;
		ent.frame = cent->current.frame;
		ent.framelerp = (cl.time - cent->frametime) * 10;
		ent.framelerp = bound (0, ent.framelerp, 1);
		ent.skinnum = cent->current.skin;

		// do not allow players to go totally black
		if (num > 0 && num <= Com_ClientMaxclients ())
			ent.flags |= RF_MINLIGHT;

		// rotate binary objects locally
		if (flags & MF_ROTATE)
		{
			bobjrotate = anglemod(100*(cl.time + num*0.1));
			ent.angles[1] = bobjrotate;
			if (cl_bobbingitems.value)
				ent.origin[2] += itembob[(int)bobjrotate % (BOB_SCALE-1)];
		}

		if (effects)
		{
			if (effects & EF_BRIGHTFIELD)
				CL_EntityParticles (&ent);

			if (effects & EF_MUZZLEFLASH)
			{
				vec3_t fv;

				dl = CL_AllocDlight (num);
				VectorCopy (ent.origin, dl->origin);
				dl->origin[2] += 16;
				AngleVectors (ent.angles, fv, NULL, NULL);
				 
				VectorMA (dl->origin, 18, fv, dl->origin);
				dl->radius = 200 + (rand()&31);
				dl->die = cl.time + 0.1;
			}
			if (effects & EF_BRIGHTLIGHT)
			{			
				dl = CL_AllocDlight (num);
				VectorCopy (ent.origin, dl->origin);
				dl->origin[2] += 16;
				dl->radius = 400 + (rand()&31);
				dl->die = cl.time + 0.001;
			}
			if (effects & EF_DIMLIGHT)
			{			
				dl = CL_AllocDlight (num);
				VectorCopy (ent.origin, dl->origin);
				dl->radius = 200 + (rand()&31);
				dl->die = cl.time + 0.001;
			}
		}

		if (flags & ~MF_ROTATE)
		{
			if (flags & MF_GIB)
				CL_BloodTrail (oldorg, ent.origin);
			else if (flags & MF_ZOMGIB)
				CL_SlightBloodTrail (oldorg, ent.origin);
			else if (flags & MF_TRACER)
				CL_TracerTrail (oldorg, ent.origin, 52);
			else if (flags & MF_TRACER2)
				CL_TracerTrail (oldorg, ent.origin, 230);
			else if (flags & MF_ROCKET)
			{
				CL_RocketTrail (oldorg, ent.origin);
				dl = CL_AllocDlight (num);
				VectorCopy (ent.origin, dl->origin);
				dl->radius = 200;
				dl->die = cl.time + 0.01;
			}
			else if (flags & MF_GRENADE)
				CL_GrenadeTrail (oldorg, ent.origin);
			else if (flags & MF_TRACER3)
				CL_VoorTrail (oldorg, ent.origin);
		}

		if (num == cl.viewentity)
		{
			if (!chase_active.value)
				continue;

			ent.flags |= RF_VIEWERMODEL;
			ent.angles[YAW] = cl.viewangles[YAW];	// the model should face the view dir
			ent.angles[PITCH] = -cl.viewangles[PITCH] * 0.3;	// the model should face the view dir
		}

		V_AddEntity (&ent);
	}
}

/*
===============
CL_AddStaticEntities
===============
*/
void CL_AddStaticEntities (void)
{
	int			i;
	entity_t	*ent;

	for (i=0,ent=cl_static_entities ; i<cl.num_statics ; i++,ent++)
	{
		if (!ent->model)
			continue;
		V_AddEntity (ent);
	}
}

/*
===============
CL_ClipMoveToEntities

Trace against all bmodels (normal entities to follow?) present in the last frame
===============
*/
void CL_ClipMoveToEntities (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, trace_t *tr, int passedict)
{
	int i, num;
	vec3_t angles;
	trace_t	trace;
	struct cmodel_s *cmodel;
	centity_t *cent;

	for (i=0 ; i<cl.frame.num_entities ; i++)
	{
		num = cl_parse_entities[(cl.frame.parse_entities+i)&(MAX_PARSE_ENTITIES-1)];
		cent = &cl_entities[num];

		if (num == passedict)
			continue;
		if (!cent->current.modelindex || cent->current.modelindex == 1)
			continue;

		if (cl.model_clip[cent->current.modelindex])
		{
			cmodel = cl.model_clip[cent->current.modelindex];
			VectorCopy (cent->current.angles, angles);
		}
		else
		{
			continue;
		}

		trace = CM_TransformedBoxTrace (start, mins, maxs, end, cmodel, cent->current.origin, angles);

		if (trace.startsolid || trace.allsolid || trace.fraction < tr->fraction)
		{
			trace.ent = (struct edict_s *)cent;
		 	if (tr->startsolid)
			{
				*tr = trace;
				tr->startsolid = true;
			}
			else
				*tr = trace;
		}
		else if (trace.startsolid)
			tr->startsolid = true;
	}
}

/*
===============
CL_TraceLine
===============
*/
trace_t CL_TraceLine (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passedict)
{
	trace_t	t;

	// check against world
	t = CM_BoxTrace (start, mins, maxs, end, cl.model_clip[1]);
	if (t.fraction < 1.0)
		t.ent = (struct edict_s *)1;

	// check all other solid models
	CL_ClipMoveToEntities (start, mins, maxs, end, &t, passedict);

	return t;
}

/*
===============
CL_ReadFromServer

Read all incoming data from the server
===============
*/
int CL_ReadFromServer (void)
{
	int		ret;

	cl.oldtime = cl.time;
	cl.time += host_frametime;
	
	do
	{
		ret = CL_GetMessage ();
		if (ret == -1)
			Host_Error ("CL_ReadFromServer: lost server connection");
		if (!ret)
			break;
		
		cl.last_received_message = realtime;
		CL_ParseServerMessage ();
	} while (ret && Com_ClientState() == ca_connected);
	
	if (cl_shownet.value)
		Con_Printf ("\n");

//
// bring the links up to date
//
	return 0;
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	usercmd_t		cmd;

	if (Com_ClientState() != ca_connected)
		return;

	if (cls.signon == SIGNONS)
	{
	// get basic movement from keyboard
		CL_BaseMove (&cmd);

	// allow mice or other external controllers to add to the move
		IN_Move (&cmd);

	// send the unreliable message
		CL_SendMove (&cmd);
	}

	if (cls.demoplayback)
	{
		SZ_Clear (&cls.message);
		return;
	}

// send the reliable message
	if (!cls.message.cursize)
		return;		// no message at all

	if (!NET_CanSendMessage (cls.netcon))
	{
		Con_DPrintf ("CL_SendCmd: can't send\n");
		return;
	}

	if (NET_SendMessage (cls.netcon, &cls.message) == -1)
		Host_Error ("CL_SendCmd: lost server connection");

	SZ_Clear (&cls.message);
}

/*
=================
CL_Init
=================
*/
void CL_Init (void)
{	
	SZ_Alloc (&cls.message, 1024);

	CL_InitInput ();
	CL_InitTEnts ();
	CL_InitParticles ();
	CL_BuildBobTable ();
	
//
// register our commands
//
	Cvar_RegisterVariable (&cl_name);
	Cvar_RegisterVariable (&cl_color);
	Cvar_RegisterVariable (&cl_shownet);
	Cvar_RegisterVariable (&cl_nolerp);

	Cvar_RegisterVariable (&cl_bobbingitems);

	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("stop", CL_Stop_f);
	Cmd_AddCommand ("playdemo", CL_PlayDemo_f);
	Cmd_AddCommand ("timedemo", CL_TimeDemo_f);
	Cmd_AddCommand ("cmd", Cmd_ForwardToServer);
}
