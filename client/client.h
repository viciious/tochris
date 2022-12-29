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

// client.h
#include "../common/quakedef.h"
#include "keys.h"
#include "vid.h"
#include "ref.h"
#include "menu.h"
#include "cdaudio.h"
#include "input.h"
#include "sound.h"
#include "render.h"

typedef struct
{
	char	name[MAX_SCOREBOARDNAME];
	float	entertime;
	int		frags;
	int		colors;			// two 4 bit fields
	byte	translations[VID_GRADES*256];
} scoreboard_t;

typedef struct
{
	double	time;			// the timestamp
	int		num_entities;
	int		parse_entities;	// non-masked index into cl_parse_entities array
} entframe_t;

enum
{
	CSHIFT_CONTENTS,
	CSHIFT_DAMAGE,
	CSHIFT_BONUS,
	CSHIFT_POWERUP,

	NUM_CSHIFTS
};

#define	NAME_LENGTH	64


//
// client_state_t should hold all pieces of the client state
//

typedef struct
{
	float					msgtime;
	float					startlerp;
	float					deltalerp;
	float					frametime;
	float					syncbase;

	entity_state_t			baseline;		// to fill in defaults in updates
	entity_state_t			prev;
	entity_state_t			current;

	vec3_t					lerp_origin;
} centity_t;

typedef struct cparticle_s
{
	vec3_t		org;
	int			color;
	struct cparticle_s	*next;
	vec3_t		vel;
	float		ramp;
	float		die;
	int			type;
} cparticle_t;

typedef struct
{
	vec3_t	origin;
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	int		key;
} cdlight_t;


#define	MAX_BEAMS	24
typedef struct
{
	int		entity;
	struct model_s	*model;
	float	endtime;
	vec3_t	start, end;
} beam_t;

#define	MAX_MAPSTRING	2048
#define	MAX_DEMOS		8
#define	MAX_DEMONAME	16

#define	SIGNONS		4			// signon messages to receive before connected

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
typedef struct
{
// personalization data sent to server	
	char		mapstring[MAX_QPATH];
	char		spawnparms[MAX_MAPSTRING];	// to restart a level

// demo loop control
	int			demonum;		// -1 = don't play demos
	char		demos[MAX_DEMOS][MAX_DEMONAME];		// when not playing

// demo recording info must be here, because record is started before
// entering a map (and clearing client_state_t)
	qboolean	demorecording;
	qboolean	demoplayback;
	qboolean	timedemo;
	int			forcetrack;			// -1 = use normal cd track
	FILE		*demofile;
	int			td_lastframe;		// to meter out one message a frame
	int			td_startframe;		// host_framecount at start
	float		td_starttime;		// realtime at second frame of timedemo


// connection information
	int			signon;			// 0 to SIGNONS
	struct qsocket_s	*netcon;
	sizebuf_t	message;		// writing buffer to send to server
	
} client_static_t;

extern client_static_t	cls;

//
// the client_state_t structure is wiped completely at every
// server signon
//
typedef struct
{
	int			movemessages;	// since connecting to this server
								// throw out the first couple, so the player
								// doesn't accidentally do something the 
								// first frame
	usercmd_t	cmd;			// last command sent to the server

// information for local display
	int			stats[MAX_CL_STATS];	// health, etc
	int			items;			// inventory bit flags
	float		item_gettime[32];	// cl.time of aquiring item, for blinking
	float		faceanimtime;	// use anim frame if cl.time < this

	cshift_t	cshifts[NUM_CSHIFTS];	// color shifts for damage, powerups
	cshift_t	prev_cshifts[NUM_CSHIFTS];	// and content types

// the client maintains its own idea of view angles, which are
// sent to the server each frame.  The server sets punchangle when
// the view is temporarliy offset, and an angle reset commands at the start
// of each level and after teleporting.
	vec3_t		mviewangles[2];	// during demo playback viewangles is lerped
								// between these
	vec3_t		viewangles;
	
	vec3_t		mvelocity[2];	// update by server, used for lean+bob
								// (0 is newest)
	vec3_t		velocity;		// lerped between mvelocity[0] and [1]

	vec3_t		punchangle;		// temporary offset
	
// pitch drifting vars
	float		idealpitch;
	float		pitchvel;
	qboolean	nodrift;
	float		driftmove;
	double		laststop;

	float		viewheight;
	float		crouch;			// local amount for smoothing stepups

	qboolean	paused;			// send over by server
	qboolean	onground;
	qboolean	inwater;
	
	int			intermission;	// don't change view angle, full screen, etc
	int			completed_time;	// latched at intermission start
	
	entframe_t	frame;
	entframe_t	oldframe;
	int			parse_entities;

	double		time;			// clients view of time, should be between
								// servertime and oldservertime to generate
								// a lerp point for other data
	double		oldtime;		// previous cl.time, time-oldtime is used
								// to decay light values and smooth step ups
	

	float		last_received_message;	// (realtime) for net trouble icon

//
// information that is static for the entire time connected to a server
//
	struct cmodel_s		*model_clip[MAX_MODELS];
	struct model_s		*model_precache[MAX_MODELS];
	struct sfx_s		*sound_precache[MAX_SOUNDS];

	char		levelname[40];	// for display on solo scoreboard
	int			viewentity;		// cl_entitites[cl.viewentity] = player
	int			gametype;

// refresh related state
	struct model_s	*worldmodel;	// cl_entitites[0].model
	int			num_statics;	// held in cl_staticentities array

	centity_t	viewent;			// the gun model

	refdef_t	refdef;
	vec3_t		v_forward, v_right, v_up;

	int			cdtrack, looptrack;	// cd audio

// frag scoreboard
	scoreboard_t	*scores;		// [cl.maxclients]
} client_state_t;


//
// cvars
//
extern	cvar_t	cl_name;
extern	cvar_t	cl_color;

extern	cvar_t	cl_upspeed;
extern	cvar_t	cl_forwardspeed;
extern	cvar_t	cl_backspeed;
extern	cvar_t	cl_sidespeed;

extern	cvar_t	cl_movespeedkey;

extern	cvar_t	cl_yawspeed;
extern	cvar_t	cl_pitchspeed;

extern	cvar_t	cl_anglespeedkey;

extern	cvar_t	cl_shownet;
extern	cvar_t	cl_nolerp;

extern	cvar_t  cl_bobbingitems;

extern	cvar_t	cl_pitchdriftspeed;
extern	cvar_t	lookspring;
extern	cvar_t	lookstrafe;
extern	cvar_t	sensitivity;
extern	cvar_t	cl_freelook;

#define freelook (cl_freelook.value || (in_mlook.state&1))

extern	cvar_t	m_pitch;
extern	cvar_t	m_yaw;
extern	cvar_t	m_forward;
extern	cvar_t	m_side;

extern	cvar_t	chase_active;

extern	client_state_t	cl;

#define MAX_PARSE_ENTITIES	1024
extern int cl_parse_entities[MAX_PARSE_ENTITIES];

#define	MAX_TEMP_ENTITIES	128			// lightning bolts, etc
#define	MAX_STATIC_ENTITIES	128			// torches, etc

extern	centity_t		cl_entities[MAX_EDICTS];
extern	entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
extern	lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
extern	cdlight_t		cl_dlights[MAX_DLIGHTS];
extern	entity_t		cl_temp_entities[MAX_TEMP_ENTITIES];
extern	beam_t			cl_beams[MAX_BEAMS];

//=============================================================================

//
// cl_main.c
//
cdlight_t *CL_AllocDlight (int key);

void CL_Init (void);
void CL_ClearState (void);

void CL_EstablishConnection (char *host);
void CL_Signon1 (void);
void CL_Signon2 (void);
void CL_Signon3 (void);
void CL_Signon4 (void);

void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_NextDemo (void);

void CL_AddEntities (void);
void CL_AddStaticEntities (void);

int  CL_ReadFromServer (void);
trace_t CL_TraceLine (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passedict);


//
// cl_input
//
typedef struct
{
	int		down[2];		// key nums holding it down
	int		state;			// low bit is down state
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_SendMove (usercmd_t *cmd);
void CL_BaseMove (usercmd_t *cmd);

float CL_KeyState (kbutton_t *key);


//
// cl_effects.c
//
void CL_InitParticles (void);
void CL_ParseParticleEffect (void);
void CL_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count);
void CL_RocketSplash (vec3_t org, vec3_t dir, int color);
void CL_RocketTrail (vec3_t start, vec3_t end);
void CL_GrenadeTrail (vec3_t start, vec3_t end);
void CL_BloodTrail (vec3_t start, vec3_t end);
void CL_SlightBloodTrail (vec3_t start, vec3_t end);
void CL_TracerTrail (vec3_t start, vec3_t end, int color);
void CL_VoorTrail (vec3_t start, vec3_t end);

void CL_EntityParticles (entity_t *ent);
void CL_BlobExplosion (vec3_t org);
void CL_ParticleExplosion (vec3_t org);
void CL_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength);
void CL_LavaSplash (vec3_t org);
void CL_TeleportSplash (vec3_t org);
void CL_RailTrail (vec3_t start, vec3_t end);

void CL_ClearParticles (void);

void CL_AddParticles (void);
void CL_AddDlights (void);


//
// cl_demo.c
//
void CL_StopPlayback (void);
int CL_GetMessage (void);

void CL_Stop_f (void);
void CL_Record_f (void);
void CL_PlayDemo_f (void);
void CL_TimeDemo_f (void);


//
// cl_parse.c
//
void CL_ParseServerMessage (void);
void CL_NewTranslation (int slot);
void CL_ParseEntityLump (char *entdata);
void CL_ParseDamage (void);


//
// cl_view.c
//
void V_Init (void);

void V_ClearScene (void);
void V_AddParticle (vec3_t org, int color);
void V_AddEntity (entity_t *ent);
void V_AddDlight (vec3_t org, float radius);
void V_AddLightstyle (lightstyle_t *ls);

void CL_ClearCshifts (void);

void CL_StartPitchDrift (void);
void CL_StopPitchDrift (void);

void V_RenderView (void);


//
// cl_screen.c
//
#define	SBAR_HEIGHT		24

extern	int			sb_lines;			// scan lines to draw

void Sbar_Init (void);

void Sbar_Draw (void);
// called every frame by screen

void Sbar_IntermissionOverlay (void);
// called each frame after the level has been completed

void Sbar_FinaleOverlay (void);

void SCR_Init (void);
void SCR_UpdateScreen (void);
void SCR_CenterPrint (char *str);

void SCR_BeginLoadingPlaque (void);
void SCR_EndLoadingPlaque (void);

extern	float		scr_con_current;
extern	float		scr_conlines;		// lines of console to display

extern	int			sb_lines;

extern	int			clearnotify;	// set to 0 whenever notify text is drawn
extern	qboolean	scr_disabled_for_loading;

extern	cvar_t		scr_viewsize;
extern	cvar_t		scr_fov;

extern	vrect_t		scr_vrect;

// only the refresh window will be updated unless these variables are flagged 
extern qboolean		block_drawing;


//
// cl_tent.c
//
void CL_InitTEnts (void);
void CL_ParseTEnt (void);
void CL_AddTempEntities (void);
void CL_SignonReply (void);
