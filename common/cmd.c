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
// cmd.c -- Quake script command processing module

#include "quakedef.h"

#define	MAX_ALIAS_NAME	32

typedef struct cmdalias_s
{
	struct cmdalias_s	*next;
	char	name[MAX_ALIAS_NAME];
	char	*value;
} cmd_alias_t;

cmd_alias_t	*cmd_alias;

int trashtest;
int *trashspot;

qboolean	cmd_wait;

#define	ALIAS_LOOP_COUNT	16
int		alias_count;		// for detecting runaway loops

//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
void Cmd_Wait_f (void)
{
	cmd_wait = true;
}

/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

sizebuf_t	cmd_text;

/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void)
{
	SZ_Alloc (&cmd_text, 8192);		// space for commands and script files
}


/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText (char *text)
{
	int		l;
	
	l = strlen (text);

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		Con_Printf ("Cbuf_AddText: overflow\n");
		return;
	}

	SZ_Write (&cmd_text, text, strlen (text));
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
============
*/
void Cbuf_InsertText (char *text)
{
	char	*temp;
	int		templen;

// copy off any commands still remaining in the exec buffer
	templen = cmd_text.cursize;
	if (templen)
	{
		temp = Z_Malloc (templen);
		memcpy (temp, cmd_text.data, templen);
		SZ_Clear (&cmd_text);
	}
	else
		temp = NULL;	// shut up compiler
		
// add the entire text of the file
	Cbuf_AddText (text);
	
// add the copied off data
	if (templen)
	{
		SZ_Write (&cmd_text, temp, templen);
		Z_Free (temp);
	}
}

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void)
{
	int		i;
	char	*text;
	char	line[1024];
	int		quotes;

	alias_count = 0;		// don't allow infinite alias loops

	while (cmd_text.cursize)
	{
// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = 0;
		for (i=0 ; i< cmd_text.cursize ; i++)
		{
			if (text[i] == '"')
				quotes++;
			if ( !(quotes&1) && text[i] == ';')
				break;	// don't break if inside a quoted string
			if (text[i] == '\n')
				break;
		}
			
				
		memcpy (line, text, i);
		line[i] = 0;
		
// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec, alias) can insert data at the
// beginning of the text buffer

		if (i == cmd_text.cursize)
			cmd_text.cursize = 0;
		else
		{
			i++;
			cmd_text.cursize -= i;
			memcpy (text, text+i, cmd_text.cursize);
		}

// execute the command line
		Cmd_ExecuteString (line, src_command);
		
		if (cmd_wait)
		{	// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait = false;
			break;
		}
	}
}

/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/

/*
===============
Cmd_StuffCmds_f

Adds command line parameters as script statements
Commands lead with a +, and continue until a - or another +
quake +prog jctest.qp +cmd amlev1
quake -nosound +cmd amlev1
===============
*/
void Cmd_StuffCmds_f (void)
{
	int		i, j;
	int		s;
	char	*text, *build, c;
		
	if (Cmd_Argc () != 1)
	{
		Con_Printf ("stuffcmds : execute command line parameters\n");
		return;
	}

// build the combined string to parse from
	s = 0;
	for (i=1 ; i<com_argc ; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP nulls out -NXHost
		s += strlen (com_argv[i]) + 1;
	}
	if (!s)
		return;
		
	text = Z_Malloc (s+1);
	text[0] = 0;
	for (i=1 ; i<com_argc ; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP nulls out -NXHost
		strcat (text,com_argv[i]);
		if (i != com_argc-1)
			strcat (text, " ");
	}
	
// pull out the commands
	build = Z_Malloc (s+1);
	build[0] = 0;
	
	for (i=0 ; i<s-1 ; i++)
	{
		if (text[i] == '+')
		{
			i++;

			for (j=i ; (text[j] != '+') && (text[j] != '-') && (text[j] != 0) ; j++)
				;

			c = text[j];
			text[j] = 0;
			
			strcat (build, text+i);
			strcat (build, "\n");
			text[j] = c;
			i = j-1;
		}
	}
	
	if (build[0])
		Cbuf_InsertText (build);
	
	Z_Free (text);
	Z_Free (build);
}


/*
===============
Cmd_Exec_f
===============
*/
void Cmd_Exec_f (void)
{
	char	*f;
	int		mark;

	if (Cmd_Argc () != 2)
	{
		Con_Printf ("exec <filename> : execute a script file\n");
		return;
	}

	mark = Hunk_LowMark ();
	f = (char *)COM_LoadHunkFile (Cmd_Argv(1));
	if (!f)
	{
		Con_Printf ("couldn't exec %s\n",Cmd_Argv(1));
		return;
	}
	Con_Printf ("execing %s\n",Cmd_Argv(1));
	
	Cbuf_InsertText (f);
	Hunk_FreeToLowMark (mark);
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
void Cmd_Echo_f (void)
{
	int		i;
	
	for (i=1 ; i<Cmd_Argc() ; i++)
		Con_Printf ("%s ",Cmd_Argv(i));
	Con_Printf ("\n");
}

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/
char *CopyString (char *in)
{
	char	*out;
	
	out = Z_Malloc (strlen(in)+1);
	strcpy (out, in);
	return out;
}

void Cmd_Alias_f (void)
{
	cmd_alias_t	*a;
	char		cmd[1024];
	int			i, c;
	char		*s;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Current alias commands:\n");
		for (a = cmd_alias ; a ; a=a->next)
			Con_Printf ("%s : %s\n", a->name, a->value);
		return;
	}

	s = Cmd_Argv(1);
	if (strlen(s) >= MAX_ALIAS_NAME)
	{
		Con_Printf ("Alias name is too long\n");
		return;
	}

	// if the alias already exists, reuse it
	for (a = cmd_alias ; a ; a=a->next)
	{
		if (!strcmp(s, a->name))
		{
			Z_Free (a->value);
			break;
		}
	}

	if (!a)
	{
		a = Z_Malloc (sizeof(cmd_alias_t));
		a->next = cmd_alias;
		cmd_alias = a;
	}
	strcpy (a->name, s);	

// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	c = Cmd_Argc();
	for (i=2 ; i< c ; i++)
	{
		strcat (cmd, Cmd_Argv(i));
		if (i != c)
			strcat (cmd, " ");
	}
	strcat (cmd, "\n");
	
	a->value = CopyString (cmd);
}

/*
===============
Cmd_Viewalias_f

===============
*/
void Cmd_Viewalias_f (void)
{
	cmd_alias_t *alias;
	char *name;

	if (Cmd_Argc() < 2)
	{
		Con_Printf ("viewalias <aliasname> : view body of alias\n");
		return;
	}

	// Check aliases
	for (alias = cmd_alias, name = Cmd_Argv(1); alias; alias = alias->next)
		if (!strcmp(name, alias->name))
			break;

	if (alias)
		Con_Printf ("%s : \"%s\"\n", name, alias->value);
	else
		Con_Printf ("No such alias: %s\n", name);
}

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

typedef struct cmd_function_s
{
	struct cmd_function_s	*next;
	char					*name;
	xcommand_t				function;
} cmd_function_t;


#define	MAX_ARGS		80

static	int			cmd_argc;
static	char		*cmd_argv[MAX_ARGS];
static	char		*cmd_null_string = "";
static	char		*cmd_args = NULL;

cmd_source_t	cmd_source;


static	cmd_function_t	*cmd_functions;		// possible commands to execute

/*
============
Cmd_Init
============
*/
void Cmd_Init (void)
{
//
// register our commands
//
	Cmd_AddCommand ("stuffcmds",Cmd_StuffCmds_f);
	Cmd_AddCommand ("exec",Cmd_Exec_f);
	Cmd_AddCommand ("echo",Cmd_Echo_f);
	Cmd_AddCommand ("alias",Cmd_Alias_f);
	Cmd_AddCommand ("wait", Cmd_Wait_f);
	Cmd_AddCommand ("viewalias", Cmd_Viewalias_f);
}

/*
============
Cmd_Argc
============
*/
int		Cmd_Argc (void)
{
	return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
char	*Cmd_Argv (int arg)
{
	if ( (unsigned)arg >= cmd_argc )
		return cmd_null_string;
	return cmd_argv[arg];	
}

/*
============
Cmd_Args
============
*/
char		*Cmd_Args (void)
{
	return cmd_args;
}


/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
============
*/
void Cmd_TokenizeString (char *text)
{
	int		i;
	
// clear the args from the last string
	for (i=0 ; i<cmd_argc ; i++)
		Z_Free (cmd_argv[i]);
		
	cmd_argc = 0;
	cmd_args = NULL;
	
	while (1)
	{
// skip whitespace up to a /n
		while (*text && *text <= ' ' && *text != '\n')
		{
			text++;
		}
		
		if (*text == '\n')
		{	// a newline seperates commands in the buffer
			text++;
			break;
		}

		if (!*text)
			return;
	
		if (cmd_argc == 1)
			 cmd_args = text;
			
		text = COM_Parse (text);
		if (!text)
			return;

		if (cmd_argc < MAX_ARGS)
		{
			cmd_argv[cmd_argc] = Z_Malloc (strlen(com_token)+1);
			strcpy (cmd_argv[cmd_argc], com_token);
			cmd_argc++;
		}
	}
	
}


/*
============
Cmd_AddCommand
============
*/
void	Cmd_AddCommand (char *cmd_name, xcommand_t function)
{
	cmd_function_t	*cmd;
	
	if (host_initialized)	// because hunk allocation would get stomped
		Sys_Error ("Cmd_AddCommand after host_initialized");
		
// fail if the command is a variable name
	if (Cvar_VariableString(cmd_name)[0])
	{
		Con_Printf ("Cmd_AddCommand: %s already defined as a var\n", cmd_name);
		return;
	}
	
// fail if the command already exists
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!Q_strcmp (cmd_name, cmd->name))
		{
			Con_Printf ("Cmd_AddCommand: %s already defined\n", cmd_name);
			return;
		}
	}

	cmd = Hunk_Alloc (sizeof(cmd_function_t));
	cmd->name = cmd_name;
	cmd->function = function;
	cmd->next = cmd_functions;
	cmd_functions = cmd;
}

/*
============
Cmd_Exists
============
*/
qboolean	Cmd_Exists (char *cmd_name)
{
	cmd_function_t	*cmd;

	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!Q_strcmp (cmd_name,cmd->name))
			return true;
	}

	return false;
}



/*
============
Cmd_CompleteCommand
============
*/
char *Cmd_CompleteCommand (char *partial)
{
	cmd_function_t	*cmd;
	int				len;
	
	len = strlen(partial);
	
	if (!len)
		return NULL;
		
// check functions
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
		if (!Q_strncmp (partial,cmd->name, len))
			return cmd->name;

	return NULL;
}

/*
	Cmd_CompleteCountPossible
	Thanks for Taniwha's Help -EvilTypeGuy
*/
int
Cmd_CompleteCountPossible (char *partial)
{
	cmd_function_t	*cmd;
	int				len;
	int				h;
	
	h = 0;
	len = strlen(partial);
	
	if (!len)
		return 0;
	
	// Loop through the command list and count all partial matches
	for (cmd = cmd_functions; cmd; cmd = cmd->next)
		if (!_strnicmp(partial, cmd->name, len))
			h++;

	return h;
}

/*
	Cmd_CompleteBuildList
	Thanks for Taniwha's Help -EvilTypeGuy
*/
char	**
Cmd_CompleteBuildList (char *partial)
{
	cmd_function_t	*cmd;
	int				len = 0;
	int				bpos = 0;
	int				sizeofbuf = (Cmd_CompleteCountPossible (partial) + 1) * sizeof (char *);
	char			**buf;

	len = strlen(partial);
	buf = malloc(sizeofbuf + sizeof (char *));
	// Loop through the alias list and print all matches
	for (cmd = cmd_functions; cmd; cmd = cmd->next)
		if (!_strnicmp(partial, cmd->name, len))
			buf[bpos++] = cmd->name;

	buf[bpos] = NULL;
	return buf;
}

/*
	Cmd_CompleteAlias
	Thanks for Taniwha's Help -EvilTypeGuy
*/
char
*Cmd_CompleteAlias (char * partial)
{
	static cmd_alias_t	*alias;
	int			len;

	len = strlen(partial);

	if (!len)
		return NULL;

	// Check aliases
	for (alias = cmd_alias; alias; alias = alias->next)
		if (!_strnicmp(partial, alias->name, len))
			return alias->name;

	return NULL;
}

/*
	Cmd_CompleteAliasCountPossible
	Thanks for Taniwha's Help -EvilTypeGuy
*/
int
Cmd_CompleteAliasCountPossible (char *partial)
{
	cmd_alias_t	*alias;
	int			len;
	int			h;

	h = 0;

	len = strlen(partial);

	if (!len)
		return 0;

	// Loop through the command list and count all partial matches
	for (alias = cmd_alias; alias; alias = alias->next)
		if (!_strnicmp(partial, alias->name, len))
			h++;

	return h;
}

/*
	Cmd_CompleteAliasBuildList
	Thanks for Taniwha's Help -EvilTypeGuy
*/
char	**
Cmd_CompleteAliasBuildList (char *partial)
{
	cmd_alias_t	*alias;
	int			len = 0;
	int			bpos = 0;
	int			sizeofbuf = (Cmd_CompleteAliasCountPossible (partial) + 1) * sizeof (char *);
	char		**buf;

	len = strlen(partial);
	buf = malloc(sizeofbuf + sizeof (char *));
	// Loop through the alias list and print all matches
	for (alias = cmd_alias; alias; alias = alias->next)
		if (!_strnicmp(partial, alias->name, len))
			buf[bpos++] = alias->name;

	buf[bpos] = NULL;
	return buf;
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
void	Cmd_ExecuteString (char *text, cmd_source_t src)
{	
	cmd_function_t	*cmd;
	cmd_alias_t		*a;

	cmd_source = src;
	Cmd_TokenizeString (text);
			
// execute the command line
	if (!Cmd_Argc())
		return;		// no tokens

// check functions
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
	{
		if (!Q_strcasecmp (cmd_argv[0],cmd->name))
		{
			cmd->function ();
			return;
		}
	}

// check alias
	for (a=cmd_alias ; a ; a=a->next)
	{
		if (!Q_strcasecmp (cmd_argv[0], a->name))
		{
			if (++alias_count == ALIAS_LOOP_COUNT)
			{
				Con_Printf ("ALIAS_LOOP_COUNT\n");
				return;
			}

			Cbuf_InsertText (a->value);
			return;
		}
	}
	
// check cvars
	if (!Cvar_Command ())
		Con_Printf ("Unknown command \"%s\"\n", Cmd_Argv(0));
}


/*
================
Cmd_CheckParm

Returns the position (1 to argc-1) in the command's argument list
where the given parameter apears, or 0 if not present
================
*/

int Cmd_CheckParm (char *parm)
{
	int i;
	
	if (!parm)
		Sys_Error ("Cmd_CheckParm: NULL");

	for (i = 1; i < Cmd_Argc (); i++)
		if (!Q_strcasecmp (parm, Cmd_Argv (i)))
			return i;
			
	return 0;
}
