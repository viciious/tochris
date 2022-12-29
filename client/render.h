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

// refresh.h -- public interface to refresh functions
#ifndef __RENDER_H__
#define __RENDER_H__

struct model_s *Mod_ForName (char *name, qboolean crash);
void	Mod_TouchModel (char *name);

void R_Init (void);
void R_RenderView (refdef_t *refdef);		// must set r_refdef first
void R_ScreenShot_f (void);

void Mod_Init (void);
void Mod_ClearAll (void);
int	 Mod_Flags (struct model_s *model);

void R_BeginFrame (void);
void R_EndFrame (void);

void R_NewMap (struct model_s *worldmodel);
void R_TranslatePlayerSkin (int playernum, struct model_s *model, int skinnum, int top, int bottom);

void Draw_Init (void);
void Draw_Character (int x, int y, int num);
void Draw_Pic (int x, int y, struct mpic_s *pic);
void Draw_TransPic (int x, int y, struct mpic_s *pic);
void Draw_TransPicTranslate (int x, int y, struct mpic_s *pic, byte *translation);
void Draw_ConsoleBackground (int lines);
void Draw_TileClear (int x, int y, int w, int h);
void Draw_Fill (int x, int y, int w, int h, int c);
void Draw_FadeScreen (void);
void Draw_String (int x, int y, char *str);
struct mpic_s *Draw_PicFromWad (char *name);
struct mpic_s *Draw_CachePic (char *path);

//========================================================

void S_ExtraUpdate (void);
trace_t CL_TraceLine (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passedict);

#endif //__RENDER_H__