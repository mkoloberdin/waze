# Microsoft Developer Studio Project File - Name="maplay" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=maplay - Win32 DebugUnicode
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "maplay.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "maplay.mak" CFG="maplay - Win32 DebugUnicode"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "maplay - Win32 Release" ("Win32 (x86) Static Library" 用)
!MESSAGE "maplay - Win32 Debug" ("Win32 (x86) Static Library" 用)
!MESSAGE "maplay - Win32 ReleaseUnicode" ("Win32 (x86) Static Library" 用)
!MESSAGE "maplay - Win32 DebugUnicode" ("Win32 (x86) Static Library" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "maplay - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /Ob2 /I "../include" /D "_LIB" /D "NDEBUG" /D "WIN32" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\Release\maplay.lib"

!ELSEIF  "$(CFG)" == "maplay - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../include" /D "_LIB" /D "_DEBUG" /D "WIN32" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\Debug\maplay.lib"

!ELSEIF  "$(CFG)" == "maplay - Win32 ReleaseUnicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "maplay___Win32_ReleaseUnicode"
# PROP BASE Intermediate_Dir "maplay___Win32_ReleaseUnicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUnicode"
# PROP Intermediate_Dir "ReleaseUnicode"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /Ob2 /I "../include" /D "_LIB" /D "NDEBUG" /D "WIN32" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /Ob2 /I "../include" /D "NDEBUG" /D "_LIB" /D "WIN32" /D "_UNICODE" /D "UNICODE" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\Release\maplay.lib"
# ADD LIB32 /nologo /out:"..\lib\ReleaseUnicode\maplay.lib"

!ELSEIF  "$(CFG)" == "maplay - Win32 DebugUnicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "maplay___Win32_DebugUnicode"
# PROP BASE Intermediate_Dir "maplay___Win32_DebugUnicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugUnicode"
# PROP Intermediate_Dir "DebugUnicode"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "../include" /D "_LIB" /D "_DEBUG" /D "WIN32" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../include" /D "_DEBUG" /D "_LIB" /D "WIN32" /D "_UNICODE" /D "UNICODE" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\Debug\maplay.lib"
# ADD LIB32 /nologo /out:"..\lib\DebugUnicode\maplay.lib"

!ENDIF 

# Begin Target

# Name "maplay - Win32 Release"
# Name "maplay - Win32 Debug"
# Name "maplay - Win32 ReleaseUnicode"
# Name "maplay - Win32 DebugUnicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\decoder.cpp
# End Source File
# Begin Source File

SOURCE=.\effect.cpp
# End Source File
# Begin Source File

SOURCE=.\mahelper.cpp
# End Source File
# Begin Source File

SOURCE=.\maplay.cpp
# End Source File
# Begin Source File

SOURCE=.\MultiBuff.cpp
# End Source File
# Begin Source File

SOURCE=.\output.cpp
# End Source File
# Begin Source File

SOURCE=.\player.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerMpg.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerNet.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerOv.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerPlugIn.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerWav.cpp
# End Source File
# Begin Source File

SOURCE=.\reader.cpp
# End Source File
# Begin Source File

SOURCE=.\Receiver.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\decoder.h
# End Source File
# Begin Source File

SOURCE=.\effect.h
# End Source File
# Begin Source File

SOURCE=.\helper.h
# End Source File
# Begin Source File

SOURCE=.\mahelper.h
# End Source File
# Begin Source File

SOURCE=..\include\maplay.h
# End Source File
# Begin Source File

SOURCE=..\include\mapplugin.h
# End Source File
# Begin Source File

SOURCE=.\MultiBuff.h
# End Source File
# Begin Source File

SOURCE=.\output.h
# End Source File
# Begin Source File

SOURCE=.\player.h
# End Source File
# Begin Source File

SOURCE=.\reader.h
# End Source File
# Begin Source File

SOURCE=.\Receiver.h
# End Source File
# End Group
# End Target
# End Project
