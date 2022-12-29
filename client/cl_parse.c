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
// cl_parse.c  -- parse a message received from the server

#include "client.h"

char *svc_strings[] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",
	"svc_updatestat",
	"svc_version",		// [long] server version
	"svc_setview",		// [short] entity number
	"svc_sound",			// <see code>
	"svc_time",			// [float] server time
	"svc_print",			// [string] null terminated string
	"svc_stufftext",		// [string] stuffed into client's console buffer
						// the string should be \n terminated
	"svc_setangle",		// [vec3] set the view angle to this absolute value
	
	"svc_serverinfo",		// [long] version
						// [string] signon string
						// [string]..[0]model cache [string]...[0]sounds cache
						// [string]..[0]item cache
	"svc_lightstyle",		// [byte] [string]
	"svc_updatename",		// [byte] [string]
	"svc_updatefrags",	// [byte] [short]
	"svc_clientdata",		// <shortbits + data>
	"svc_stopsound",		// <see code>
	"svc_updatecolors",	// [byte] [byte]
	"svc_particle",		// [vec3] <variable>
	"svc_damage",			// [byte] impact [byte] blood [vec3] from
	
	"svc_spawnstatic",
	"OBSOLETE svc_spawnbinary",
	"svc_spawnbaseline",
	
	"svc_temp_entity",		// <variable>
	"svc_setpause",
	"svc_signonnum",
	"svc_centerprint",
	"svc_killedmonster",
	"svc_foundsecret",
	"svc_spawnstaticsound",
	"svc_intermission",
	"svc_finale",			// [string] music [string] text
	"svc_cdtrack",			// [byte] track [byte] looptrack
	"svc_sellscreen",
	"svc_cutscene"
};

//=============================================================================

/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
    vec3_t  pos;
    int 	channel, ent;
    int 	sound_num;
    int 	volume;
    int 	field_mask;
    float 	attenuation;  
 	int		i;
	           
    field_mask = MSG_ReadByte(); 

    if (field_mask & SND_VOLUME)
		volume = MSG_ReadByte ();
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;
	
    if (field_mask & SND_ATTENUATION)
		attenuation = MSG_ReadByte () / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;
	
	channel = MSG_ReadShort ();
	sound_num = MSG_ReadByte ();

	ent = channel >> 3;
	channel &= 7;

	if (ent > MAX_EDICTS)
		Host_Error ("CL_ParseStartSoundPacket: ent = %i", ent);
	
	for (i=0 ; i<3 ; i++)
		pos[i] = MSG_ReadCoord ();
 
    S_StartSound (ent, channel, cl.sound_precache[sound_num], pos, volume/255.0, attenuation);
}       

/*
==================
CL_KeepaliveMessage

When the client is taking a long time to load stuff, send keepalive messages
so the server doesn't disconnect.
==================
*/
void CL_KeepaliveMessage (void)
{
	float	time;
	static float lastmsg;
	int		ret;
	sizebuf_t	old;
	byte		olddata[8192];
	
	if (Com_ServerState())
		return;		// no need if server is local
	if (cls.demoplayback)
		return;

// read messages from server, should just be nops
	old = net_message;
	memcpy (olddata, net_message.data, net_message.cursize);
	
	do
	{
		ret = CL_GetMessage ();
		switch (ret)
		{
		default:
			Host_Error ("CL_KeepaliveMessage: CL_GetMessage failed");		
		case 0:
			break;	// nothing waiting
		case 1:
			Host_Error ("CL_KeepaliveMessage: received a message");
			break;
		case 2:
			if (MSG_ReadByte() != svc_nop)
				Host_Error ("CL_KeepaliveMessage: datagram wasn't a nop");
			break;
		}
	} while (ret);

	net_message = old;
	memcpy (net_message.data, olddata, net_message.cursize);

// check time
	time = Sys_FloatTime ();
	if (time - lastmsg < 5)
		return;
	lastmsg = time;

// write out a nop
	Con_Printf ("--> client to server keepalive\n");

	MSG_WriteByte (&cls.message, clc_nop);
	NET_SendMessage (cls.netcon, &cls.message);
	SZ_Clear (&cls.message);
}

/*
==================
CL_ParseServerInfo
==================
*/
void CL_ParseServerInfo (void)
{
	char	*str;
	int		i;
	int		nummodels, numsounds;
	char	model_precache[MAX_MODELS][MAX_QPATH];
	char	sound_precache[MAX_SOUNDS][MAX_QPATH];
	
	Con_DPrintf ("Serverinfo packet received.\n");
//
// wipe the client_state_t struct
//
	CL_ClearState ();

// parse protocol version number
	i = MSG_ReadLong ();
	if (i != PROTOCOL_VERSION)
	{
		Con_Printf ("Server returned version %i, not %i", i, PROTOCOL_VERSION);
		return;
	}

// parse maxclients
	Com_SetClientMaxclients (MSG_ReadByte ());
	if (Com_ClientMaxclients() < 1 || Com_ClientMaxclients() > MAX_SCOREBOARD)
	{
		Con_Printf("Bad maxclients (%u) from server\n", Com_ClientMaxclients());
		return;
	}
	cl.scores = Hunk_AllocName (Com_ClientMaxclients()*sizeof(*cl.scores), "scores");

// parse gametype
	cl.gametype = MSG_ReadByte ();

// parse signon message
	str = MSG_ReadString ();
	strncpy (cl.levelname, str, sizeof(cl.levelname)-1);

// seperate the printfs so the server message can have a color
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_Printf ("%c%s\n", 2, str);

//
// first we go through and touch all of the precache data that still
// happens to be in the cache, so precaching something else doesn't
// needlessly purge it
//

// precache models
	memset (cl.model_precache, 0, sizeof(cl.model_precache));
	for (nummodels=1 ; ; nummodels++)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		if (nummodels==MAX_MODELS)
		{
			Con_Printf ("Server sent too many model precaches\n");
			return;
		}
		strcpy (model_precache[nummodels], str);
		Mod_TouchModel (str);
	}

// precache sounds
	memset (cl.sound_precache, 0, sizeof(cl.sound_precache));
	for (numsounds=1 ; ; numsounds++)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		if (numsounds==MAX_SOUNDS)
		{
			Con_Printf ("Server sent too many sound precaches\n");
			return;
		}
		strcpy (sound_precache[numsounds], str);
		S_TouchSound (str);
	}

//
// now we try to load everything else until a cache allocation fails
//
	cl.model_clip[1] = CM_LoadMap (model_precache[1], true);
	for (i=2 ; i<CM_NumSubmodels() ; i++)
	{
		cl.model_clip[i] = CM_InlineModel (i-1);
	}

	for (i=1 ; i<nummodels ; i++)
	{
		cl.model_precache[i] = Mod_ForName (model_precache[i], false);
		if (cl.model_precache[i] == NULL)
		{
			Con_Printf("Model %s not found\n", model_precache[i]);
			return;
		}
		CL_KeepaliveMessage ();
	}

	S_BeginPrecaching ();
	for (i=1 ; i<numsounds ; i++)
	{
		cl.sound_precache[i] = S_PrecacheSound (sound_precache[i]);
		CL_KeepaliveMessage ();
	}
	S_EndPrecaching ();

	cl.worldmodel = cl.model_precache[1];

	CL_ClearCshifts ();
	CL_ClearParticles ();
	CL_ParseEntityLump (CM_EntitiesString());

	R_NewMap (cl.worldmodel);

	Hunk_Check ();		// make sure nothing is hurt
}


/*
==================
CL_ParseUpdate

Parse an entity update message from the server
If an entities model or origin changes from frame to frame, it must be
relinked. Other attributes can change without relinking.
==================
*/
void CL_ParseEntityBits (entity_state_t *to, entity_state_t *from, int bits)
{
	// default to something
	*to = *from;

	if (bits & U_MODEL)
		to->modelindex = MSG_ReadByte ();

	if (bits & U_FRAME)
		to->frame = MSG_ReadByte ();

	if (bits & U_COLORMAP)
		to->colormap = MSG_ReadByte ();

	if (bits & U_SKIN)
		to->skin = MSG_ReadByte ();

	if (bits & U_EFFECTS)
		to->effects = MSG_ReadByte ();

	if (bits & U_ORIGIN1)
		to->origin[0] = MSG_ReadCoord ();
	if (bits & U_ANGLE1)
		to->angles[0] = MSG_ReadAngle ();

	if (bits & U_ORIGIN2)
		to->origin[1] = MSG_ReadCoord ();
	if (bits & U_ANGLE2)
		to->angles[1] = MSG_ReadAngle ();

	if (bits & U_ORIGIN3)
		to->origin[2] = MSG_ReadCoord ();
	if (bits & U_ANGLE3)
		to->angles[2] = MSG_ReadAngle ();
}

void CL_ParseUpdate (int bits)
{
	centity_t	*cent;
	int			num;
	entity_state_t state;

	if (bits & U_MOREBITS)
		bits |= (MSG_ReadByte () << 8);

	if (bits & U_LONGENTITY)	
		num = MSG_ReadShort ();
	else
		num = MSG_ReadByte ();

	if (num >= MAX_EDICTS)
		Host_Error ("CL_EntityNum: %i is an invalid number", num);

	cl_parse_entities[cl.parse_entities & (MAX_PARSE_ENTITIES-1)] = num;
	cl.parse_entities++;
	cl.frame.num_entities++;

	cent = &cl_entities[num];
	CL_ParseEntityBits (&state, &cent->baseline, bits);

	if (state.modelindex != cent->current.modelindex) 
	{
		if (state.modelindex >= MAX_MODELS)
			Host_Error ("CL_ParseModel: bad modnum");

	// automatic animation (torches, etc) can be either all together
	// or randomized
		cent->syncbase = (float)(rand()&0x7fff) / 0x7fff;
	
	// reset frame interpolation
		cent->frametime = cl.time;

		if (num > 0 && num <= Com_ClientMaxclients())
			R_TranslatePlayerSkin (num - 1, cl.model_precache[state.modelindex], state.skin,
				cl.scores[num - 1].colors & 0xf0, (cl.scores[num - 1].colors &15)<<4);
	}

	if (state.skin != cent->current.skin)
	{
		if (num > 0 && num <= Com_ClientMaxclients())
			R_TranslatePlayerSkin (num - 1, cl.model_precache[state.modelindex], state.skin,
				cl.scores[num - 1].colors & 0xf0, (cl.scores[num - 1].colors &15)<<4);
	}

	if (state.colormap != cent->current.colormap)
	{
		if (state.colormap > Com_ClientMaxclients())
			Sys_Error ("state.colormap >= Com_ClientMaxclients()");
	}

	if (state.frame != cent->current.frame) 
		cent->frametime = cl.time;
	else
		cent->current.frame = cent->prev.frame;	// save previous frame for interpolation

	if ( state.modelindex != cent->current.modelindex
		// if the delta is large, assume a teleport and don't lerp
		|| fabs (state.origin[0] - cent->current.origin[0]) > 200
		|| fabs (state.origin[1] - cent->current.origin[1]) > 200
		|| fabs (state.origin[2] - cent->current.origin[2]) > 200
		)
	{	// we can't lerp
		cent->msgtime = -99999;
	}

	if (cent->msgtime != cl.oldframe.time || cls.timedemo || cl_nolerp.value)
	{	// didn't have an update last message
		cent->prev = state;
		cent->deltalerp = 0;
		cent->startlerp = cl.oldframe.time;
		VectorCopy (state.origin, cent->lerp_origin);
	}
	else if (!(bits & U_STEP))		// most likely a player
	{	// no lerp if it's singleplayer
		cent->prev = cent->current;

		if (Com_ServerState() && Com_ServerMaxclients())
			cent->deltalerp = 0;
		else
			cent->deltalerp = cl.frame.time - cl.oldframe.time;
		cent->startlerp = cl.oldframe.time;
	}
	else 							// most likely a monster
	{	// lerp
		if (!VectorCompare (state.origin, cent->current.origin) ||
			!VectorCompare (state.angles, cent->current.angles))
		{
			cent->prev = cent->current;
			cent->deltalerp = bound (0, cl.oldframe.time - cent->startlerp, 0.1);
			cent->startlerp = cl.oldframe.time;
		}
		else
		{
			vec3_t oldorigin, oldangles;
			VectorCopy (cent->prev.origin, oldorigin);
			VectorCopy (cent->prev.angles, oldangles);

			cent->prev = cent->current;

			VectorCopy (oldorigin, cent->prev.origin);
			VectorCopy (oldangles, cent->prev.angles);
		}
	}

	cent->msgtime = cl.frame.time;
	cent->current = state;
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline (entity_state_t *baseline)
{
	int			i;
	
	baseline->modelindex = MSG_ReadByte ();
	baseline->frame = MSG_ReadByte ();
	baseline->colormap = MSG_ReadByte();
	baseline->skin = MSG_ReadByte();
	for (i=0 ; i<3 ; i++)
	{
		baseline->origin[i] = MSG_ReadCoord ();
		baseline->angles[i] = MSG_ReadAngle ();
	}
}


/*
==================
CL_ParseClientdata

Server information pertaining to this client only
==================
*/
void CL_ParseClientdata (int bits)
{
	int		i, j;
	centity_t *view = &cl.viewent;

	if (bits & SU_VIEWHEIGHT)
		cl.viewheight = MSG_ReadChar ();
	else
		cl.viewheight = DEFAULT_VIEWHEIGHT;

	if (bits & SU_IDEALPITCH)
		cl.idealpitch = MSG_ReadChar ();
	else
		cl.idealpitch = 0;
	
	VectorCopy (cl.mvelocity[0], cl.mvelocity[1]);
	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i) )
			cl.punchangle[i] = MSG_ReadChar();
		else
			cl.punchangle[i] = 0;
		if (bits & (SU_VELOCITY1<<i) )
			cl.mvelocity[0][i] = MSG_ReadChar()*16;
		else
			cl.mvelocity[0][i] = 0;
	}

// [always sent]	if (bits & SU_ITEMS)
	i = MSG_ReadLong ();

	if (cl.items != i)
	{	// set flash times
		for (j=0 ; j<32 ; j++)
			if ( (i & (1<<j)) && !(cl.items & (1<<j)))
				cl.item_gettime[j] = cl.time;
		cl.items = i;
	}
		
	cl.onground = (bits & SU_ONGROUND) != 0;
	cl.inwater = (bits & SU_INWATER) != 0;

	if (bits & SU_WEAPONFRAME)
		cl.stats[STAT_WEAPONFRAME] = MSG_ReadByte ();
	else
		cl.stats[STAT_WEAPONFRAME] = 0;

	if (bits & SU_ARMOR)
		cl.stats[STAT_ARMOR] = MSG_ReadByte ();
	else
		cl.stats[STAT_ARMOR] = 0;

	if (bits & SU_WEAPON)
		cl.stats[STAT_WEAPON] = MSG_ReadByte ();
	else
		cl.stats[STAT_WEAPON] = 0;
	
	cl.stats[STAT_HEALTH] = MSG_ReadShort ();
	cl.stats[STAT_AMMO] = MSG_ReadByte ();

	for (i=0 ; i<4 ; i++)
		cl.stats[STAT_SHELLS+i] = MSG_ReadByte ();

	if (standard_quake)
		cl.stats[STAT_ACTIVEWEAPON] = MSG_ReadByte ();
	else
		cl.stats[STAT_ACTIVEWEAPON] = (MSG_ReadByte ()<<i);

	if (cl.stats[STAT_WEAPON] != view->current.modelindex)
	{
		view->current.modelindex = cl.stats[STAT_WEAPON];
		view->prev.frame = view->current.frame = cl.stats[STAT_WEAPONFRAME];
		view->frametime = cl.time;
	}
	else if (cl.stats[STAT_WEAPONFRAME] != view->current.frame) 
	{
		view->prev.frame = view->current.frame;
		view->current.frame = cl.stats[STAT_WEAPONFRAME];
		view->frametime = cl.time;
	}
}

/*
=====================
CL_NewTranslation
=====================
*/
void CL_NewTranslation (int slot)
{
	int		i, j;
	int		top, bottom;
	byte	*dest, *source;
	
	if (slot > Com_ClientMaxclients())
		Sys_Error ("CL_NewTranslation: slot > cl.maxclients");
	dest = cl.scores[slot].translations;
	source = vid.colormap;
	memcpy (dest, vid.colormap, sizeof(cl.scores[slot].translations));

	top = cl.scores[slot].colors & 0xf0;
	bottom = (cl.scores[slot].colors &15)<<4;

	R_TranslatePlayerSkin (slot, cl.model_precache[cl_entities[slot+1].current.modelindex],
		cl_entities[slot+1].current.skin, top, bottom);

	for (i=0 ; i<VID_GRADES ; i++, dest += 256, source+=256)
	{
		if (top < 128)	// the artists made some backwards ranges.  sigh.
			memcpy (dest + TOP_RANGE, source + top, 16);
		else
			for (j=0 ; j<16 ; j++)
				dest[TOP_RANGE+j] = source[top+15-j];
				
		if (bottom < 128)
			memcpy (dest + BOTTOM_RANGE, source + bottom, 16);
		else
			for (j=0 ; j<16 ; j++)
				dest[BOTTOM_RANGE+j] = source[bottom+15-j];		
	}
}

/*
=====================
CL_ParseStatic
=====================
*/
void CL_ParseStatic (void)
{
	entity_t *ent;
	entity_state_t state;
		
	if (cl.num_statics >= MAX_STATIC_ENTITIES)
		Host_Error ("Too many static entities");

	CL_ParseBaseline (&state);

// copy it to the current state
	if (!state.modelindex)
		return;

	ent = &cl_static_entities[cl.num_statics++];
	memset (ent, 0, sizeof(*ent));
	ent->model = cl.model_precache[state.modelindex];
	ent->frame = ent->oldframe = state.frame;
	ent->colormap = vid.colormap;
	ent->skinnum = state.skin;
	ent->syncbase = (float)(rand()&0x7fff) / 0x7fff;
	ent->framelerp = 0;
	ent->flags = RF_CULLVIS;
	VectorCopy (state.origin, ent->origin);
	VectorCopy (state.angles, ent->angles);
}

/*
===================
CL_ParseStaticSound
===================
*/
void CL_ParseStaticSound (void)
{
	vec3_t		org;
	int			sound_num, vol, atten;
	int			i;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	sound_num = MSG_ReadByte ();
	vol = MSG_ReadByte ();
	atten = MSG_ReadByte ();
	
	S_StaticSound (cl.sound_precache[sound_num], org, vol, atten);
}


#define SHOWNET(x) if(cl_shownet.value==2)Con_Printf ("%3i:%s\n", msg_readcount-1, x);

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage (void)
{
	int			cmd;
	int			i;
	
//
// if recording demos, copy the message out
//
	if (cl_shownet.value == 1)
		Con_Printf ("%i ",net_message.cursize);
	else if (cl_shownet.value == 2)
		Con_Printf ("------------------\n");
	
	cl.onground = false;	// unless the server says otherwise	
//
// parse the message
//
	MSG_BeginReading ();
	
	while (1)
	{
		if (msg_badread)
			Host_Error ("CL_ParseServerMessage: Bad server message");

		cmd = MSG_ReadByte ();

		if (cmd == -1)
		{
			SHOWNET("END OF MESSAGE");
			return;		// end of message
		}

	// if the high bit of the command byte is set, it is a fast update
		if (cmd & 128)
		{
			SHOWNET("fast update");

			if (cls.signon == SIGNONS - 1)
			{	// first update is the final signon stage
				cls.signon = SIGNONS;
				CL_SignonReply ();
			}

			CL_ParseUpdate (cmd&127);
			continue;
		}

		SHOWNET(svc_strings[cmd]);
	
	// other commands
		switch (cmd)
		{
		default:
			Host_Error ("CL_ParseServerMessage: Illegible server message\n");
			break;
			
		case svc_nop:
			break;
			
		case svc_time:
			cl.oldframe = cl.frame;
			cl.frame.num_entities = 0;
			cl.frame.parse_entities = cl.parse_entities;
			cl.frame.time = MSG_ReadFloat ();			
			break;
			
		case svc_clientdata:
			i = MSG_ReadShort ();
			CL_ParseClientdata (i);
			break;
		
		case svc_version:
			i = MSG_ReadLong ();
			if (i != PROTOCOL_VERSION)
				Host_Error ("CL_ParseServerMessage: Server is protocol %i instead of %i\n", i, PROTOCOL_VERSION);
			break;
			
		case svc_disconnect:
			Host_EndGame ("Server disconnected\n");

		case svc_print:
			Con_Printf ("%s", MSG_ReadString ());
			break;
			
		case svc_centerprint:
			SCR_CenterPrint (MSG_ReadString ());
			break;
			
		case svc_stufftext:
			Cbuf_AddText (MSG_ReadString ());
			break;
			
		case svc_damage:
			CL_ParseDamage ();
			break;
			
		case svc_serverinfo:
			CL_ParseServerInfo ();
			break;
			
		case svc_setangle:
			for (i=0 ; i<3 ; i++)
				cl.viewangles[i] = MSG_ReadAngle ();
			break;
			
		case svc_setview:
			cl.viewentity = MSG_ReadShort ();
			break;
					
		case svc_lightstyle:
			i = MSG_ReadByte ();
			if (i >= MAX_LIGHTSTYLES)
				Sys_Error ("svc_lightstyle > MAX_LIGHTSTYLES");
			strcpy (cl_lightstyle[i].map,  MSG_ReadString());
			cl_lightstyle[i].length = strlen(cl_lightstyle[i].map);
			break;
			
		case svc_sound:
			CL_ParseStartSoundPacket();
			break;
			
		case svc_stopsound:
			i = MSG_ReadShort();
			S_StopSound(i>>3, i&7);
			break;
		
		case svc_updatename:
			i = MSG_ReadByte ();
			if (i >= Com_ClientMaxclients())
				Host_Error ("CL_ParseServerMessage: svc_updatename > MAX_SCOREBOARD");
			strcpy (cl.scores[i].name, MSG_ReadString ());
			break;
			
		case svc_updatefrags:
			i = MSG_ReadByte ();
			if (i >= Com_ClientMaxclients())
				Host_Error ("CL_ParseServerMessage: svc_updatefrags > MAX_SCOREBOARD");
			cl.scores[i].frags = MSG_ReadShort ();
			break;			

		case svc_updatecolors:
			i = MSG_ReadByte ();
			if (i >= Com_ClientMaxclients())
				Host_Error ("CL_ParseServerMessage: svc_updatecolors > MAX_SCOREBOARD");
			cl.scores[i].colors = MSG_ReadByte ();
			CL_NewTranslation (i);
			break;
			
		case svc_particle:
			CL_ParseParticleEffect ();
			break;

		case svc_spawnbaseline:
			i = MSG_ReadShort ();
			if (i >= MAX_EDICTS)
				Host_Error ("CL_EntityNum: %i is an invalid number", i);
			CL_ParseBaseline (&cl_entities[i].baseline);
			break;
		case svc_spawnstatic:
			CL_ParseStatic ();
			break;			
		case svc_temp_entity:
			CL_ParseTEnt ();
			break;

		case svc_setpause:
			{
				cl.paused = MSG_ReadByte ();

				if (cl.paused)
				{
					CDAudio_Pause ();
#ifdef _WIN32
					VID_HandlePause (true);
#endif
				}
				else
				{
					CDAudio_Resume ();
#ifdef _WIN32
					VID_HandlePause (false);
#endif
				}
			}
			break;
			
		case svc_signonnum:
			i = MSG_ReadByte ();
			if (i <= cls.signon)
				Host_Error ("Received signon %i when at %i", i, cls.signon);
			cls.signon = i;
			CL_SignonReply ();
			break;

		case svc_killedmonster:
			cl.stats[STAT_MONSTERS]++;
			break;

		case svc_foundsecret:
			cl.stats[STAT_SECRETS]++;
			break;

		case svc_updatestat:
			i = MSG_ReadByte ();
			if (i < 0 || i >= MAX_CL_STATS)
				Sys_Error ("svc_updatestat: %i is invalid", i);
			cl.stats[i] = MSG_ReadLong ();
			break;
			
		case svc_spawnstaticsound:
			CL_ParseStaticSound ();
			break;

		case svc_cdtrack:
			cl.cdtrack = MSG_ReadByte ();
			cl.looptrack = MSG_ReadByte ();
			if ( (cls.demoplayback || cls.demorecording) && (cls.forcetrack != -1) )
				CDAudio_Play ((byte)cls.forcetrack, true);
			else
				CDAudio_Play ((byte)cl.cdtrack, true);
			break;

		case svc_intermission:
			cl.intermission = 1;
			cl.completed_time = cl.time;
			break;

		case svc_finale:
			cl.intermission = 2;
			cl.completed_time = cl.time;
			SCR_CenterPrint (MSG_ReadString ());			
			break;

		case svc_cutscene:
			cl.intermission = 3;
			cl.completed_time = cl.time;
			SCR_CenterPrint (MSG_ReadString ());			
			break;

		case svc_sellscreen:
			Cmd_ExecuteString ("help", src_command);
			break;
		}
	}
}

void CL_ParseEntityLump (char *entdata)
{
	char *data;
	char key[128], value[4096];
	extern cvar_t r_skyname;

	data = entdata;

	if (!data)
		return;
	data = COM_Parse (data);
	if (!data || com_token[0] != '{')
		return;							// error

	while (1)
	{
		data = COM_Parse (data);
		if (!data)
			return;						// error
		if (com_token[0] == '}')
			break;						// end of worldspawn

		if (com_token[0] == '_')
			strcpy(key, com_token + 1);
		else
			strcpy(key, com_token);

		while (key[strlen(key)-1] == ' ')
			key[strlen(key)-1] = 0;		// remove trailing spaces

		data = COM_Parse (data);
		if (!data)
			return;						// error
		strcpy (value, com_token);

		if (strcmp (key, "sky") == 0 || strcmp (key, "skyname") == 0 ||
				strcmp (key, "qlsky") == 0)
		Cvar_Set (&r_skyname, value);
		// more checks here..
	}
}