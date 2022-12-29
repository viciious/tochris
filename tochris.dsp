# Microsoft Developer Studio Project File - Name="tochris" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=tochris - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tochris.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tochris.mak" CFG="tochris - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tochris - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "tochris - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "tochris - Win32 GL Debug" (based on "Win32 (x86) Application")
!MESSAGE "tochris - Win32 GL Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tochris - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /W3 /GX /I ".\win32\scitech\include" /I ".\win32\dxsdk\sdk\inc" /I ".\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# SUBTRACT CPP /Ox /Ot /Og /Oi /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 .\win32\dxsdk\sdk\lib\dxguid.lib .\win32\scitech\lib\win32\vc\mgllt.lib winmm.lib wsock32.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /profile /map /debug

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /ML /W3 /GX /ZI /Od /I ".\win32\scitech\include" /I ".\win32\dxsdk\sdk\inc" /I ".\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 .\win32\dxsdk\sdk\lib\dxguid.lib .\win32\scitech\lib\win32\vc\mgllt.lib winmm.lib wsock32.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\tochris"
# PROP BASE Intermediate_Dir ".\tochris"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\debug_gl"
# PROP Intermediate_Dir ".\debug_gl"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /GX /Zi /Od /I ".\scitech\include" /I ".\dxsdk\sdk\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /ML /W3 /GX /ZI /Od /I ".\win32\dxsdk\sdk\inc" /I ".\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "GLQUAKE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib wsock32.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib .\scitech\lib\win32\vc\mgllt.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 .\win32\dxsdk\sdk\lib\dxguid.lib comctl32.lib winmm.lib wsock32.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /out:".\debug_gl/gltochris.exe"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\winquak0"
# PROP BASE Intermediate_Dir ".\winquak0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\release_gl"
# PROP Intermediate_Dir ".\release_gl"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /GX /Ox /Ot /Ow /I ".\scitech\include" /I ".\dxsdk\sdk\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# SUBTRACT BASE CPP /Oa /Og
# ADD CPP /nologo /G5 /W3 /GX /Ot /Ow /I ".\win32\dxsdk\sdk\inc" /I ".\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "GLQUAKE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib wsock32.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib .\scitech\lib\win32\vc\mgllt.lib /nologo /subsystem:windows /profile /machine:I386
# SUBTRACT BASE LINK32 /map /debug
# ADD LINK32 .\win32\dxsdk\sdk\lib\dxguid.lib comctl32.lib winmm.lib wsock32.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /out:".\release_gl/gltochris.exe"
# SUBTRACT LINK32 /profile /map /debug

!ENDIF 

# Begin Target

# Name "tochris - Win32 Release"
# Name "tochris - Win32 Debug"
# Name "tochris - Win32 GL Debug"
# Name "tochris - Win32 GL Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\win32\cd_win.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_demo.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_effects.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_input.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_main.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_parse.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_screen.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_tent.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_view.c
# End Source File
# Begin Source File

SOURCE=.\common\cmd.c
# End Source File
# Begin Source File

SOURCE=.\common\cmodel.c
# End Source File
# Begin Source File

SOURCE=.\common\common.c
# End Source File
# Begin Source File

SOURCE=.\win32\conproc.c
# End Source File
# Begin Source File

SOURCE=.\common\console.c
# End Source File
# Begin Source File

SOURCE=.\common\crc.c
# End Source File
# Begin Source File

SOURCE=.\common\cvar.c
# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_edge.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_init.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_modech.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_part.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_parta.s

!IF  "$(CFG)" == "tochris - Win32 Release"

# Begin Custom Build - gas2masm
OutDir=.\Release
InputPath=.\ref_soft\d_parta.s
InputName=d_parta

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# Begin Custom Build - gas2masm
OutDir=.\Debug
InputPath=.\ref_soft\d_parta.s
InputName=d_parta

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_polysa.s

!IF  "$(CFG)" == "tochris - Win32 Release"

# Begin Custom Build - gas2masm
OutDir=.\Release
InputPath=.\ref_soft\d_polysa.s
InputName=d_polysa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# Begin Custom Build - gas2masm
OutDir=.\Debug
InputPath=.\ref_soft\d_polysa.s
InputName=d_polysa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_polyse.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_rast.s

!IF  "$(CFG)" == "tochris - Win32 Release"

# Begin Custom Build - gas2masm
OutDir=.\Release
InputPath=.\ref_soft\d_rast.s
InputName=d_rast

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# Begin Custom Build - gas2masm
OutDir=.\Debug
InputPath=.\ref_soft\d_rast.s
InputName=d_rast

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_scan.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_scana.s

!IF  "$(CFG)" == "tochris - Win32 Release"

# Begin Custom Build - gas2masm
OutDir=.\Release
InputPath=.\ref_soft\d_scana.s
InputName=d_scana

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# Begin Custom Build - gas2masm
OutDir=.\Debug
InputPath=.\ref_soft\d_scana.s
InputName=d_scana

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_sky.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_spr8.s

!IF  "$(CFG)" == "tochris - Win32 Release"

# Begin Custom Build - gas2masm
OutDir=.\Release
InputPath=.\ref_soft\d_spr8.s
InputName=d_spr8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# Begin Custom Build - gas2masm
OutDir=.\Debug
InputPath=.\ref_soft\d_spr8.s
InputName=d_spr8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_sprite.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_surf.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_vars.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_varsa.s

!IF  "$(CFG)" == "tochris - Win32 Release"

# Begin Custom Build - gas2masm
OutDir=.\Release
InputPath=.\ref_soft\d_varsa.s
InputName=d_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# Begin Custom Build - gas2masm
OutDir=.\Debug
InputPath=.\ref_soft\d_varsa.s
InputName=d_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_draw.c

!IF  "$(CFG)" == "tochris - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_mesh.c

!IF  "$(CFG)" == "tochris - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_model.c

!IF  "$(CFG)" == "tochris - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_rlight.c

!IF  "$(CFG)" == "tochris - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_rmain.c

!IF  "$(CFG)" == "tochris - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_rmisc.c

!IF  "$(CFG)" == "tochris - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_rsurf.c

!IF  "$(CFG)" == "tochris - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_warp.c

!IF  "$(CFG)" == "tochris - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\common\host.c
# End Source File
# Begin Source File

SOURCE=.\common\host_cmd.c
# End Source File
# Begin Source File

SOURCE=.\win32\in_win.c
# End Source File
# Begin Source File

SOURCE=.\client\keys.c
# End Source File
# Begin Source File

SOURCE=.\common\math.s

!IF  "$(CFG)" == "tochris - Win32 Release"

# Begin Custom Build - gas2masm
OutDir=.\Release
InputPath=.\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# Begin Custom Build - gas2masm
OutDir=.\Debug
InputPath=.\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# Begin Custom Build - gas2masm
OutDir=.\debug_gl
InputPath=.\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# Begin Custom Build - gas2masm
OutDir=.\release_gl
InputPath=.\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\common\mathlib.c
# End Source File
# Begin Source File

SOURCE=.\client\menu.c
# End Source File
# Begin Source File

SOURCE=.\common\net_dgrm.c
# End Source File
# Begin Source File

SOURCE=.\common\net_loop.c
# End Source File
# Begin Source File

SOURCE=.\common\net_main.c
# End Source File
# Begin Source File

SOURCE=.\common\net_vcr.c
# End Source File
# Begin Source File

SOURCE=.\win32\net_win.c
# End Source File
# Begin Source File

SOURCE=.\win32\net_wins.c
# End Source File
# Begin Source File

SOURCE=.\win32\net_wipx.c
# End Source File
# Begin Source File

SOURCE=.\ref_soft\pcx.c
# End Source File
# Begin Source File

SOURCE=.\vm\pr_cmds.c
# End Source File
# Begin Source File

SOURCE=.\vm\pr_edict.c
# End Source File
# Begin Source File

SOURCE=.\vm\pr_exec.c
# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_aclip.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_aclipa.s

!IF  "$(CFG)" == "tochris - Win32 Release"

# Begin Custom Build - gas2masm
OutDir=.\Release
InputPath=.\ref_soft\r_aclipa.s
InputName=r_aclipa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# Begin Custom Build - gas2masm
OutDir=.\Debug
InputPath=.\ref_soft\r_aclipa.s
InputName=r_aclipa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_alias.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_aliasa.s

!IF  "$(CFG)" == "tochris - Win32 Release"

# Begin Custom Build - gas2masm
OutDir=.\Release
InputPath=.\ref_soft\r_aliasa.s
InputName=r_aliasa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# Begin Custom Build - gas2masm
OutDir=.\Debug
InputPath=.\ref_soft\r_aliasa.s
InputName=r_aliasa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_bsp.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_draw.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_edge.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_edgea.s

!IF  "$(CFG)" == "tochris - Win32 Release"

# Begin Custom Build - gas2masm
OutDir=.\Release
InputPath=.\ref_soft\r_edgea.s
InputName=r_edgea

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# Begin Custom Build - gas2masm
OutDir=.\Debug
InputPath=.\ref_soft\r_edgea.s
InputName=r_edgea

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_light.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_main.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_misc.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_model.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_rast.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_rasta.s

!IF  "$(CFG)" == "tochris - Win32 Release"

# Begin Custom Build - gas2masm
OutDir=.\Release
InputPath=.\ref_soft\r_rasta.s
InputName=r_rasta

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# Begin Custom Build - gas2masm
OutDir=.\Debug
InputPath=.\ref_soft\r_rasta.s
InputName=r_rasta

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_sky.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_sprite.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_surf.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_surf8.s

!IF  "$(CFG)" == "tochris - Win32 Release"

# Begin Custom Build - gas2masm
OutDir=.\Release
InputPath=.\ref_soft\r_surf8.s
InputName=r_surf8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# Begin Custom Build - gas2masm
OutDir=.\Debug
InputPath=.\ref_soft\r_surf8.s
InputName=r_surf8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_vars.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_varsa.s

!IF  "$(CFG)" == "tochris - Win32 Release"

# Begin Custom Build - gas2masm
OutDir=.\Release
InputPath=.\ref_soft\r_varsa.s
InputName=r_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# Begin Custom Build - gas2masm
OutDir=.\Debug
InputPath=.\ref_soft\r_varsa.s
InputName=r_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\client\snd_dma.c
# End Source File
# Begin Source File

SOURCE=.\client\snd_mem.c
# End Source File
# Begin Source File

SOURCE=.\client\snd_mix.c
# End Source File
# Begin Source File

SOURCE=.\win32\snd_win.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_main.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_move.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_phys.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_user.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_world.c
# End Source File
# Begin Source File

SOURCE=.\win32\sys_win.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Intermediate_Dir ".\release_gl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\win32\sys_wina.s

!IF  "$(CFG)" == "tochris - Win32 Release"

# Begin Custom Build - gas2masm
OutDir=.\Release
InputPath=.\win32\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# Begin Custom Build - gas2masm
OutDir=.\Debug
InputPath=.\win32\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# Begin Custom Build - gas2masm
OutDir=.\debug_gl
InputPath=.\win32\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Intermediate_Dir ".\release_gl"
# Begin Custom Build - gas2masm
OutDir=.\release_gl
InputPath=.\win32\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\win32\tochris.rc
# End Source File
# Begin Source File

SOURCE=.\win32\vid_wgl.c

!IF  "$(CFG)" == "tochris - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\win32\vid_win.c

!IF  "$(CFG)" == "tochris - Win32 Release"

!ELSEIF  "$(CFG)" == "tochris - Win32 Debug"

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "tochris - Win32 GL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\common\wad.c
# End Source File
# Begin Source File

SOURCE=.\common\zone.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\ref_soft\adivtab.h
# End Source File
# Begin Source File

SOURCE=.\ref_soft\anorm_dots.h
# End Source File
# Begin Source File

SOURCE=.\ref_soft\anorms.h
# End Source File
# Begin Source File

SOURCE=.\ref_soft\asm_draw.h
# End Source File
# Begin Source File

SOURCE=.\common\asm_i386.h
# End Source File
# Begin Source File

SOURCE=.\common\bspfile.h
# End Source File
# Begin Source File

SOURCE=.\client\cdaudio.h
# End Source File
# Begin Source File

SOURCE=.\client\client.h
# End Source File
# Begin Source File

SOURCE=.\common\cmd.h
# End Source File
# Begin Source File

SOURCE=.\common\common.h
# End Source File
# Begin Source File

SOURCE=.\win32\conproc.h
# End Source File
# Begin Source File

SOURCE=.\common\console.h
# End Source File
# Begin Source File

SOURCE=.\common\crc.h
# End Source File
# Begin Source File

SOURCE=.\common\cvar.h
# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_iface.h
# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_ifacea.h
# End Source File
# Begin Source File

SOURCE=.\ref_soft\d_local.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_local.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_model.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\gl_warp_sin.h
# End Source File
# Begin Source File

SOURCE=.\client\input.h
# End Source File
# Begin Source File

SOURCE=.\client\keys.h
# End Source File
# Begin Source File

SOURCE=.\common\mathlib.h
# End Source File
# Begin Source File

SOURCE=.\client\menu.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\modelgen.h
# End Source File
# Begin Source File

SOURCE=.\ref_soft\modelgen.h
# End Source File
# Begin Source File

SOURCE=.\common\net.h
# End Source File
# Begin Source File

SOURCE=.\common\net_dgrm.h
# End Source File
# Begin Source File

SOURCE=.\common\net_loop.h
# End Source File
# Begin Source File

SOURCE=.\common\net_udp.h
# End Source File
# Begin Source File

SOURCE=.\common\net_vcr.h
# End Source File
# Begin Source File

SOURCE=.\win32\net_wins.h
# End Source File
# Begin Source File

SOURCE=.\win32\net_wipx.h
# End Source File
# Begin Source File

SOURCE=.\vm\pr_comp.h
# End Source File
# Begin Source File

SOURCE=.\vm\progdefs.h
# End Source File
# Begin Source File

SOURCE=.\common\protocol.h
# End Source File
# Begin Source File

SOURCE=.\common\quakeasm.h
# End Source File
# Begin Source File

SOURCE=.\common\quakedef.h
# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_local.h
# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_model.h
# End Source File
# Begin Source File

SOURCE=.\ref_soft\r_shared.h
# End Source File
# Begin Source File

SOURCE=.\client\ref.h
# End Source File
# Begin Source File

SOURCE=.\client\render.h
# End Source File
# Begin Source File

SOURCE=.\win32\resource.h
# End Source File
# Begin Source File

SOURCE=.\server\server.h
# End Source File
# Begin Source File

SOURCE=.\client\sound.h
# End Source File
# Begin Source File

SOURCE=.\ref_gl\spritegn.h
# End Source File
# Begin Source File

SOURCE=.\ref_soft\spritegn.h
# End Source File
# Begin Source File

SOURCE=.\server\sv_progs.h
# End Source File
# Begin Source File

SOURCE=.\server\sv_world.h
# End Source File
# Begin Source File

SOURCE=.\common\sys.h
# End Source File
# Begin Source File

SOURCE=.\client\vid.h
# End Source File
# Begin Source File

SOURCE=.\common\wad.h
# End Source File
# Begin Source File

SOURCE=.\win32\winquake.h
# End Source File
# Begin Source File

SOURCE=.\common\zone.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\win32\qe3.ico
# End Source File
# End Group
# End Target
# End Project
