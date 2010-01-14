# Microsoft Developer Studio Project File - Name="libovd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libovd - Win32 DebugUnicode
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "libovd.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "libovd.mak" CFG="libovd - Win32 DebugUnicode"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "libovd - Win32 Release" ("Win32 (x86) Static Library" 用)
!MESSAGE "libovd - Win32 Debug" ("Win32 (x86) Static Library" 用)
!MESSAGE "libovd - Win32 ReleaseUnicode" ("Win32 (x86) Static Library" 用)
!MESSAGE "libovd - Win32 DebugUnicode" ("Win32 (x86) Static Library" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libovd - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /Ob2 /I "../include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "FIXP_SPEED" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\Release\libovd.lib"

!ELSEIF  "$(CFG)" == "libovd - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "FIXP_SPEED" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\Debug\libovd.lib"

!ELSEIF  "$(CFG)" == "libovd - Win32 ReleaseUnicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libovd___Win32_ReleaseUnicode"
# PROP BASE Intermediate_Dir "libovd___Win32_ReleaseUnicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUnicode"
# PROP Intermediate_Dir "ReleaseUnicode"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /Ob2 /I "../include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "FIXP_SPEED" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /Ob2 /I "../include" /D "NDEBUG" /D "WIN32" /D "_UNICODE" /D "UNICODE" /D "_LIB" /D "FIXP_SPEED" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\Release\libovd.lib"
# ADD LIB32 /nologo /out:"..\lib\ReleaseUnicode\libovd.lib"

!ELSEIF  "$(CFG)" == "libovd - Win32 DebugUnicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libovd___Win32_DebugUnicode"
# PROP BASE Intermediate_Dir "libovd___Win32_DebugUnicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugUnicode"
# PROP Intermediate_Dir "DebugUnicode"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "FIXP_SPEED" /FR /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_UNICODE" /D "UNICODE" /D "_LIB" /D "FIXP_SPEED" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\Debug\libovd.lib"
# ADD LIB32 /nologo /out:"..\lib\DebugUnicode\libovd.lib"

!ENDIF 

# Begin Target

# Name "libovd - Win32 Release"
# Name "libovd - Win32 Debug"
# Name "libovd - Win32 ReleaseUnicode"
# Name "libovd - Win32 DebugUnicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Tremor\bitwise.c
# End Source File
# Begin Source File

SOURCE=.\Tremor\block.c
# End Source File
# Begin Source File

SOURCE=.\Tremor\codebook.c
# End Source File
# Begin Source File

SOURCE=.\Tremor\floor0.c
# End Source File
# Begin Source File

SOURCE=.\Tremor\floor1.c
# End Source File
# Begin Source File

SOURCE=.\Tremor\framing.c
# End Source File
# Begin Source File

SOURCE=.\Tremor\info.c
# End Source File
# Begin Source File

SOURCE=.\libovd.cpp
# End Source File
# Begin Source File

SOURCE=.\Tremor\mapping0.c
# End Source File
# Begin Source File

SOURCE=.\Tremor\mdct.c
# End Source File
# Begin Source File

SOURCE=.\Tremor\registry.c
# End Source File
# Begin Source File

SOURCE=.\Tremor\res012.c
# End Source File
# Begin Source File

SOURCE=.\Tremor\sharedbook.c
# End Source File
# Begin Source File

SOURCE=.\Tremor\synthesis.c
# End Source File
# Begin Source File

SOURCE=.\Tremor\vorbisfile.c
# End Source File
# Begin Source File

SOURCE=.\Tremor\window.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Tremor\asm_arm.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\backends.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\codebook.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\codec_internal.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\config_types.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\ivorbiscodec.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\ivorbisfile.h
# End Source File
# Begin Source File

SOURCE=..\include\libovd.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\lsp_lookup.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\mdct.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\mdct_lookup.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\misc.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\ogg.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\os.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\os_types.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\registry.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\window.h
# End Source File
# Begin Source File

SOURCE=.\Tremor\window_lookup.h
# End Source File
# End Group
# End Target
# End Project
