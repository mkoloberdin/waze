# Microsoft Developer Studio Project File - Name="libmad_win32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libmad - Win32 DebugUnicode
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "libmad.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "libmad.mak" CFG="libmad - Win32 DebugUnicode"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "libmad - Win32 Release" ("Win32 (x86) Static Library" 用)
!MESSAGE "libmad - Win32 Debug" ("Win32 (x86) Static Library" 用)
!MESSAGE "libmad - Win32 ReleaseUnicode" ("Win32 (x86) Static Library" 用)
!MESSAGE "libmad - Win32 DebugUnicode" ("Win32 (x86) Static Library" 用)
!MESSAGE "libmad - Win32 Release64" ("Win32 (x86) Static Library" 用)
!MESSAGE "libmad - Win32 ReleaseUnicode64" ("Win32 (x86) Static Library" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libmad - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /Ob2 /I "../include" /D "NDEBUG" /D "_MBCS" /D "WIN32" /D "_LIB" /D "FPM_INTEL" /D "OPT_ACCURACY" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\Release\libmad.lib"

!ELSEIF  "$(CFG)" == "libmad - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_LIB" /D "FPM_INTEL" /D "OPT_ACCURACY" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\Debug\libmad.lib"

!ELSEIF  "$(CFG)" == "libmad - Win32 ReleaseUnicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libmad___Win32_ReleaseUnicode"
# PROP BASE Intermediate_Dir "libmad___Win32_ReleaseUnicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUnicode"
# PROP Intermediate_Dir "ReleaseUnicode"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /Ob2 /I "../include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "FPM_DEFAULT" /D "OPT_ACCURACY" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /Ob2 /I "../include" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_LIB" /D "FPM_INTEL" /D "OPT_ACCURACY" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\Release\libmad.lib"
# ADD LIB32 /nologo /out:"..\lib\ReleaseUnicode\libmad.lib"

!ELSEIF  "$(CFG)" == "libmad - Win32 DebugUnicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libmad___Win32_DebugUnicode"
# PROP BASE Intermediate_Dir "libmad___Win32_DebugUnicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugUnicode"
# PROP Intermediate_Dir "DebugUnicode"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "FPM_DEFAULT" /D "OPT_ACCURACY" /FR /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_LIB" /D "FPM_INTEL" /D "OPT_ACCURACY" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\Debug\libmad.lib"
# ADD LIB32 /nologo /out:"..\lib\DebugUnicode\libmad.lib"

!ELSEIF  "$(CFG)" == "libmad - Win32 Release64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libmad___Win32_Release64"
# PROP BASE Intermediate_Dir "libmad___Win32_Release64"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /Ob2 /I "../include" /D "NDEBUG" /D "_MBCS" /D "WIN32" /D "_LIB" /D "FPM_INTEL" /D "OPT_ACCURACY" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /Ob2 /I "../include" /D "NDEBUG" /D "_MBCS" /D "WIN32" /D "_LIB" /D "FPM_64BIT" /D "OPT_ACCURACY" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\Release\libmad.lib"
# ADD LIB32 /nologo /out:"..\lib\Release64\libmad.lib"

!ELSEIF  "$(CFG)" == "libmad - Win32 ReleaseUnicode64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libmad___Win32_ReleaseUnicode64"
# PROP BASE Intermediate_Dir "libmad___Win32_ReleaseUnicode64"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUnicode64"
# PROP Intermediate_Dir "ReleaseUnicode64"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /Ob2 /I "../include" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_LIB" /D "FPM_INTEL" /D "OPT_ACCURACY" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /Ob2 /I "../include" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_LIB" /D "FPM_64BIT" /D "OPT_ACCURACY" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\ReleaseUnicode\libmad.lib"
# ADD LIB32 /nologo /out:"..\lib\ReleaseUnicode64\libmad.lib"

!ENDIF 

# Begin Target

# Name "libmad - Win32 Release"
# Name "libmad - Win32 Debug"
# Name "libmad - Win32 ReleaseUnicode"
# Name "libmad - Win32 DebugUnicode"
# Name "libmad - Win32 Release64"
# Name "libmad - Win32 ReleaseUnicode64"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\bit.c
# End Source File
# Begin Source File

SOURCE=.\fixed.c
# End Source File
# Begin Source File

SOURCE=.\frame.c
# End Source File
# Begin Source File

SOURCE=.\huffman.c
# End Source File
# Begin Source File

SOURCE=.\layer12.c
# End Source File
# Begin Source File

SOURCE=.\layer3.c
# End Source File
# Begin Source File

SOURCE=.\libmad.c
# End Source File
# Begin Source File

SOURCE=.\stream.c
# End Source File
# Begin Source File

SOURCE=.\synth.c
# End Source File
# Begin Source File

SOURCE=.\timer.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\bit.h
# End Source File
# Begin Source File

SOURCE=.\c.h
# End Source File
# Begin Source File

SOURCE=.\fixed.h
# End Source File
# Begin Source File

SOURCE=.\frame.h
# End Source File
# Begin Source File

SOURCE=.\global.h
# End Source File
# Begin Source File

SOURCE=.\huffman.h
# End Source File
# Begin Source File

SOURCE=.\layer12.h
# End Source File
# Begin Source File

SOURCE=.\layer3.h
# End Source File
# Begin Source File

SOURCE=..\include\libmad.h
# End Source File
# Begin Source File

SOURCE=.\stream.h
# End Source File
# Begin Source File

SOURCE=.\synth.h
# End Source File
# Begin Source File

SOURCE=.\timer.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\id3genre.dat
# End Source File
# Begin Source File

SOURCE=.\imdct_s.dat
# End Source File
# Begin Source File

SOURCE=.\qc_table.dat
# End Source File
# Begin Source File

SOURCE=.\rq_table.dat
# End Source File
# Begin Source File

SOURCE=.\sf_table.dat
# End Source File
# End Target
# End Project
