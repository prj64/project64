# Microsoft Developer Studio Project File - Name="Project64" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Project64 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Project64.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Project64.mak" CFG="Project64 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Project64 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Project64 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Project64 - Win32 External Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Project64/Project 64 1.7", RCBAAAAA"
# PROP Scc_LocalPath ".."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Project64 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../Bin/Release"
# PROP Intermediate_Dir "../../Build/Project64/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "Project64 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../Bin/Debug"
# PROP Intermediate_Dir "../../Build/Project64/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /map

!ELSEIF  "$(CFG)" == "Project64 - Win32 External Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Project64___Win32_External_Release"
# PROP BASE Intermediate_Dir "Project64___Win32_External_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../Bin/External"
# PROP Intermediate_Dir "../../Build/Project64/External"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "EXTERNAL_RELEASE" /YX /FD /EHa /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /out:"..\Bin\Release\Project64.exe"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ENDIF 

# Begin Target

# Name "Project64 - Win32 Release"
# Name "Project64 - Win32 Debug"
# Name "Project64 - Win32 External Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Settings Files"

# PROP Default_Filter ""
# Begin Group "Setting Types Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-Application.cpp"
# End Source File
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-ApplicationIndex.cpp"
# End Source File
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-GameSetting.cpp"
# End Source File
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-RelativePath.cpp"
# End Source File
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-RomDatabase.cpp"
# End Source File
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-TempBool.cpp"
# End Source File
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-TempNumber.cpp"
# End Source File
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-TempString.cpp"
# End Source File
# End Group
# Begin Source File

SOURCE="Settings\N64System Settings.cpp"
# End Source File
# Begin Source File

SOURCE="Settings\Notification Settings.cpp"
# End Source File
# Begin Source File

SOURCE="Settings\Recompiler Settings.cpp"
# End Source File
# Begin Source File

SOURCE="Settings\Setting Config.cpp"
# End Source File
# Begin Source File

SOURCE="Settings\Settings Class.cpp"
# End Source File
# End Group
# Begin Group "User Interface Source"

# PROP Default_Filter ""
# Begin Group "Settings Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\User Interface\Settings\Settings Page - Game - General.cpp"
# End Source File
# Begin Source File

SOURCE=".\User Interface\Settings\Settings Page - Game - Plugin.cpp"
# End Source File
# Begin Source File

SOURCE=".\User Interface\Settings\Settings Page - Game - Recompiler.cpp"
# End Source File
# Begin Source File

SOURCE=".\User Interface\Settings\Settings Page - Game - Status.cpp"
# End Source File
# Begin Source File

SOURCE=".\User Interface\Settings\Settings Page.cpp"
# End Source File
# End Group
# Begin Source File

SOURCE="User Interface\Cheats.cpp"
# End Source File
# Begin Source File

SOURCE="User Interface\Frame Per Second Class.cpp"
# End Source File
# Begin Source File

SOURCE="User Interface\Gui Class.cpp"
# End Source File
# Begin Source File

SOURCE="User Interface\Main Menu Class.cpp"
# End Source File
# Begin Source File

SOURCE="User Interface\Menu Class.cpp"
# End Source File
# Begin Source File

SOURCE="User Interface\Notification Class.cpp"
# End Source File
# Begin Source File

SOURCE="User Interface\Rom Browser Class.cpp"
# End Source File
# Begin Source File

SOURCE=".\User Interface\Settings Config.cpp"
# End Source File
# End Group
# Begin Group "Multilanguage Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE="Multilanguage\Language Class.cpp"
# End Source File
# End Group
# Begin Group "N64 System Source"

# PROP Default_Filter ""
# Begin Group "Mips Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE="N64 System\Mips\Audio.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Mips\Memory Labels Class.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Mips\Memory.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Mips\OpCode Analysis Class.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Mips\OpCode Class.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Mips\Register Class.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Mips\System Timing.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Mips\TLB class.cpp"
# End Source File
# End Group
# Begin Group "C Core Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\N64 System\C Core\BreakPoints.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\C Core Interface.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\C main.c"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\C Memory.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\CPU Log.c"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\CPU.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Dma.cpp"
# End Source File
# Begin Source File

SOURCE=".\N64 System\C Core\Eeprom.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Exception.c"
# End Source File
# Begin Source File

SOURCE=".\N64 System\C Core\FlashRam.cpp"
# End Source File
# Begin Source File

SOURCE=".\N64 System\C Core\Interpreter CPU.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Interpreter Ops.c"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Logging.c"
# End Source File
# Begin Source File

SOURCE=".\N64 System\C Core\Mempak.cpp"
# End Source File
# Begin Source File

SOURCE=".\N64 System\C Core\Pif.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\r4300i Commands.c"
# End Source File
# Begin Source File

SOURCE=".\N64 System\C Core\r4300i Memory.cpp"
# End Source File
# Begin Source File

SOURCE=".\N64 System\C Core\r4300i Registers.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Recompiler CPU.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Recompiler Fpu Ops.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Recompiler Ops.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Registers.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Sram.c"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Sync CPU.c"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\TLB Display.c"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Tlb.c"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Win32Timer.c"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\x86 fpu.c"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\X86.c"
# End Source File
# End Group
# Begin Group "Debugger Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE="N64 System\Debugger\Debugger - Memory Dump.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Debugger\Debugger - Memory Search.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Debugger\Debugger - TLB.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Debugger\Debugger - View Memory.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Debugger\Debugger.cpp"
# End Source File
# End Group
# Begin Group "Recompiler Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE="N64 System\Recompiler\Delay Slot Map Class.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Recompiler\Function Info.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Recompiler\Function Map Class.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Recompiler\Recompiler Class.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Recompiler\Section Info.cpp"
# End Source File
# End Group
# Begin Source File

SOURCE="N64 System\Cheat Class.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\N64 Class.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\N64 Rom Class.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Profiling Class.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Rom Information Class.cpp"
# End Source File
# Begin Source File

SOURCE="N64 System\Speed Limitor Class.cpp"
# End Source File
# End Group
# Begin Group "Plugin Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE="Plugins\Audio Plugin.cpp"
# End Source File
# Begin Source File

SOURCE="Plugins\Controller Plugin.cpp"
# End Source File
# Begin Source File

SOURCE="Plugins\GFX plugin.cpp"
# End Source File
# Begin Source File

SOURCE="Plugins\Plugin Class.cpp"
# End Source File
# Begin Source File

SOURCE="Plugins\Plugin List.cpp"
# End Source File
# Begin Source File

SOURCE="Plugins\RSP Plugin.cpp"
# End Source File
# End Group
# Begin Group "3rd Party Source"

# PROP Default_Filter ""
# Begin Group "ZLib Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE="3rd Party\zlib\UNZIP.C"
# End Source File
# Begin Source File

SOURCE="3rd Party\zlib\zip.c"
# End Source File
# End Group
# Begin Source File

SOURCE="3rd Party\7zip.cpp"
# End Source File
# Begin Source File

SOURCE="3rd Party\Processor Info.cpp"
# End Source File
# End Group
# Begin Source File

SOURCE="Settings\Gui Settings.cpp"
# End Source File
# Begin Source File

SOURCE=main.cpp
# End Source File
# Begin Source File

SOURCE=ValidateBinary.cpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE="User Interface\Bitmaps\AboutScreen.bmp"
# End Source File
# Begin Source File

SOURCE="User Interface\Bitmaps\AboutScreenBottom.bmp"
# End Source File
# Begin Source File

SOURCE="User Interface\Bitmaps\AboutScreenMiddle.bmp"
# End Source File
# Begin Source File

SOURCE="User Interface\Bitmaps\AboutScreenTop.bmp"
# End Source File
# Begin Source File

SOURCE="User Interface\Bitmaps\CloseNormal.bmp"
# End Source File
# Begin Source File

SOURCE="User Interface\Icons\divider.cur"
# End Source File
# Begin Source File

SOURCE="User Interface\Icons\hand.cur"
# End Source File
# Begin Source File

SOURCE="User Interface\Bitmaps\LangOK.bmp"
# End Source File
# Begin Source File

SOURCE="User Interface\Bitmaps\LangOK_down.bmp"
# End Source File
# Begin Source File

SOURCE="User Interface\Icons\left.ico"
# End Source File
# Begin Source File

SOURCE="User Interface\Bitmaps\ListItems.bmp"
# End Source File
# Begin Source File

SOURCE="User Interface\Bitmaps\pj64.BMP"
# End Source File
# Begin Source File

SOURCE="User Interface\Icons\PJ64.ICO"
# End Source File
# Begin Source File

SOURCE="User Interface\Icons\right.ico"
# End Source File
# Begin Source File

SOURCE="User Interface\Bitmaps\tri-state.bmp"
# End Source File
# Begin Source File

SOURCE="User Interface\UI Resources.rc"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "Settings Headers"

# PROP Default_Filter ""
# Begin Group "Setting Types Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-Application.h"
# End Source File
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-ApplicationIndex.h"
# End Source File
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-Base.h"
# End Source File
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-GameSetting.h"
# End Source File
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-RelativePath.h"
# End Source File
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-RomDatabase.h"
# End Source File
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-TempBool.h"
# End Source File
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-TempNumber.h"
# End Source File
# Begin Source File

SOURCE=".\Settings\SettingType\SettingsType-TempString.h"
# End Source File
# End Group
# Begin Source File

SOURCE="Settings\Gui Settings.h"
# End Source File
# Begin Source File

SOURCE="Settings\N64System Settings.h"
# End Source File
# Begin Source File

SOURCE="Settings\Notification Settings.h"
# End Source File
# Begin Source File

SOURCE="Settings\Recompiler Settings.h"
# End Source File
# Begin Source File

SOURCE="Settings\Settings Class.h"
# End Source File
# End Group
# Begin Group "User Interface Headers"

# PROP Default_Filter ""
# Begin Group "Settings Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\User Interface\Settings\Settings Page - Game - General.h"
# End Source File
# Begin Source File

SOURCE=".\User Interface\Settings\Settings Page - Game - Plugin.h"
# End Source File
# Begin Source File

SOURCE=".\User Interface\Settings\Settings Page - Game - Recompiler.h"
# End Source File
# Begin Source File

SOURCE=".\User Interface\Settings\Settings Page - Game - Status.h"
# End Source File
# Begin Source File

SOURCE=".\User Interface\Settings\Settings Page.h"
# End Source File
# End Group
# Begin Source File

SOURCE="User Interface\Cheats.h"
# End Source File
# Begin Source File

SOURCE="User Interface\Frame Per Second Class.h"
# End Source File
# Begin Source File

SOURCE="User Interface\Gui Class.h"
# End Source File
# Begin Source File

SOURCE="User Interface\Log Class.h"
# End Source File
# Begin Source File

SOURCE="User Interface\Main Menu Class.h"
# End Source File
# Begin Source File

SOURCE="User Interface\Menu Class.h"
# End Source File
# Begin Source File

SOURCE="User Interface\Notification Class.h"
# End Source File
# Begin Source File

SOURCE="User Interface\resource.h"
# End Source File
# Begin Source File

SOURCE="User Interface\Rom Browser.h"
# End Source File
# End Group
# Begin Group "Multilanguage Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE="Multilanguage\Language Class.h"
# End Source File
# End Group
# Begin Group "N64 System Headers"

# PROP Default_Filter ""
# Begin Group "Mips Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE="N64 System\Mips\Audio.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Mips\Memory Class.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Mips\Memory Labels Class.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Mips\OpCode Analysis Class.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Mips\OpCode Class.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Mips\OpCode.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Mips\Register Class.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Mips\System Timing.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Mips\TLB Class.h"
# End Source File
# End Group
# Begin Group "C Core Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE="N64 System\C Core\BreakPoints.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\C Core Interface.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\C Core.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\C Global Variable.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\C Memory.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Core Settings.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\CPU Log.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\CPU.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Debugger.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Dma.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Eeprom.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Exception.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Flashram.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Interpreter CPU.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Interpreter Ops.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Logging.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Main.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\MEMPAK.H"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Pif.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Plugin.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\r4300i Commands.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\r4300i Memory.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\r4300i Registers.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Recompiler CPU.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Recompiler Ops.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Registers.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Sram.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Sync CPU.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\tlb Display.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Tlb.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\Win32Timer.h"
# End Source File
# Begin Source File

SOURCE="N64 System\C Core\X86.h"
# End Source File
# End Group
# Begin Group "Debugger Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE="N64 System\Debugger\Debugger - Memory Dump.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Debugger\Debugger - Memory Search.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Debugger\Debugger - TLB.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Debugger\Debugger - View Memory.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Debugger\debugger.h"
# End Source File
# End Group
# Begin Group "Recompiler Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE="N64 System\Recompiler\Delay Slot Map Class.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Recompiler\Function Info.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Recompiler\Function Map Class.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Recompiler\Recompiler Class.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Recompiler\Section Info.h"
# End Source File
# End Group
# Begin Source File

SOURCE="N64 System\Cheat Class.h"
# End Source File
# Begin Source File

SOURCE="N64 System\N64 Class.h"
# End Source File
# Begin Source File

SOURCE="N64 System\N64 Rom Class.h"
# End Source File
# Begin Source File

SOURCE="N64 System\N64 Types.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Profiling Class.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Rom Information Class.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Speed Limitor Class.h"
# End Source File
# Begin Source File

SOURCE="N64 System\Types.h"
# End Source File
# End Group
# Begin Group "Plugin Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE="Plugins\Audio Plugin.h"
# End Source File
# Begin Source File

SOURCE="Plugins\Controller Plugin.h"
# End Source File
# Begin Source File

SOURCE="Plugins\GFX plugin.h"
# End Source File
# Begin Source File

SOURCE="Plugins\Plugin Class.h"
# End Source File
# Begin Source File

SOURCE="Plugins\Plugin List.h"
# End Source File
# Begin Source File

SOURCE="Plugins\RSP Plugin.h"
# End Source File
# End Group
# Begin Group "3rd Party Headers"

# PROP Default_Filter ""
# Begin Group "HTML Help"

# PROP Default_Filter ""
# Begin Source File

SOURCE="3rd Party\HTML Help\HTMLHELP.H"
# End Source File
# End Group
# Begin Group "Zlib Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE="3rd Party\zlib\UNZIP.H"
# End Source File
# Begin Source File

SOURCE="3rd Party\zlib\ZCONF.H"
# End Source File
# Begin Source File

SOURCE="3rd Party\zlib\zip.h"
# End Source File
# Begin Source File

SOURCE="3rd Party\zlib\ZLIB.H"
# End Source File
# End Group
# Begin Source File

SOURCE="3rd Party\7zip.h"
# End Source File
# Begin Source File

SOURCE="3rd Party\Processor Info.h"
# End Source File
# Begin Source File

SOURCE="3rd Party\zip.h"
# End Source File
# End Group
# Begin Source File

SOURCE="C:\Program Files\Microsoft Visual Studio\VC98\ATL\Include\ATLWIN.H"
# End Source File
# Begin Source File

SOURCE=Multilanguage.h
# End Source File
# Begin Source File

SOURCE="N64 System.h"
# End Source File
# Begin Source File

SOURCE=Plugin.h
# End Source File
# Begin Source File

SOURCE=Settings.h
# End Source File
# Begin Source File

SOURCE="User Interface.h"
# End Source File
# Begin Source File

SOURCE="Validate Binary.h"
# End Source File
# Begin Source File

SOURCE=".\WTL App.h"
# End Source File
# End Group
# Begin Group "Logs"

# PROP Default_Filter ""
# Begin Group "Debug"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Bin\Debug\CPUoutput.log
# End Source File
# Begin Source File

SOURCE=..\Bin\Debug\Project64.log
# End Source File
# Begin Source File

SOURCE="..\Bin\Debug\Sync Errors.txt"
# End Source File
# End Group
# Begin Group "Release"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Bin\Release\CPUoutput.log
# End Source File
# Begin Source File

SOURCE=..\Bin\Release\Project64.log
# End Source File
# Begin Source File

SOURCE="..\Bin\Release\Sync Errors.txt"
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE="User Interface\about.htm"
# End Source File
# End Target
# End Project