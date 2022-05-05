;
; This file is part of the Simutrans project under the Artistic License.
; (see LICENSE.txt)
;

; ************************************* Pakset downloader for simutrans *********************************************

; needs the following plugins:
; nsisunz
; inetc
; CabX
; untgz
; ShellLink

Unicode true

; Request application privileges for Windows Vista
;RequestExecutionLevel highest

!define MULTIUSER_INSTALLMODE_INSTDIR "Simutrans"
!define MULTIUSER_EXECUTIONLEVEL Highest

var group1
Name "Simutrans Transport Simulator"
OutFile "simutrans-online-install.exe"

InstallDir $PROGRAMFILES\Simutrans

!define MUI_UNICON "..\stormoog.ico"

!include "preparation-functions.nsh"

!insertmacro MULTIUSER_PAGE_INSTALLMODE

!include "languages.nsh"

SectionGroup /e !Simutrans

Function PostExeInstall
  StrCmp $multiuserinstall "1" NotPortable

  ; just change to simuconf.tab "singleuser_install = 1"
  FileOpen $0 "$INSTDIR\config\simuconf.tab" a
  FileSeek $0 924
  FileWrite $0 "singleuser_install = 1 "
  FileClose $0
  goto finishGDIexe

NotPortable:
  ; make start menu entries
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\Simutrans"
  CreateShortCut "$SMPROGRAMS\Simutrans\Simutrans.lnk" "$INSTDIR\Simutrans.exe" ""
  CreateShortCut "$SMPROGRAMS\Simutrans\Simutrans (Debug).lnk" "$INSTDIR\Simutrans.exe" "-log 1 -debug 3"
  ExecWait 'Icacls "$PAKDIR" /grant Users:(OI)(CI)M'
  WriteUninstaller $INSTDIR\uninstall.exe
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Simutrans" "DisplayName" "Simutrans Game"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Simutrans" "DisplayIcon" "$\"$INSTDIR\uninstall.exe$\""
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Simutrans" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
finishGDIexe:
FunctionEnd

Section /o "Executable (GDI)" GDIexe
  AddSize 18656
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/123-0-1/simuwin-123-0-1.zip"
  StrCpy $archievename "simuwin-123-0-1.zip"
  StrCpy $downloadname "Simutrans Executable (GDI)"
  SetOutPath $INSTDIR
  Call DownloadInstallZip
  Call PostExeInstall
SectionEnd

Section "Executable (SDL2)" SDLexe
  AddSize 20372
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/123-0-1/simuwin-sdl-123-0-1.zip"
  StrCpy $archievename "simuwin-sdl-123-0-1.zip"
  StrCpy $downloadname "Simutrans Executable (SDL2)"
  SetOutPath $INSTDIR
  Call DownloadInstallZip
  Call PostExeInstall
SectionEnd

Section /o "Executable (GDI 64bit)" GDI64exe
  AddSize 18196
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/123-0/simuwin-x64-123-0-1.zip"
  StrCpy $archievename "simuwin-x64-123-0-1.zip"
  StrCpy $downloadname "Simutrans Executable (GDI) only needed for huge maps"
  SetOutPath $INSTDIR
  Call DownloadInstallZip
  Call PostExeInstall
SectionEnd

Section /o "Executable (SDL2 64bit)" SDL64exe
  AddSize 19776
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/123-0/simuwin-x64-sdl-123-0-1.zip"
  StrCpy $archievename "simuwin-sdl-x64-123-0-1.zip"
  StrCpy $downloadname "Simutrans Executable (SDL2) only needed for huge maps"
  SetOutPath $INSTDIR
  Call DownloadInstallZip
  Call PostExeInstall
SectionEnd

Section "Chinese Font" wenquanyi_font
  AddSize 3169
  IfFileExists $INSTDIR\font\wenquanyi_9pt.bdf no_chinese_needed
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/wenquanyi_9pt-font-bdf.zip"
  StrCpy $archievename "wenquanyi_9pt-font-bdf.zip"
  StrCpy $downloadname "wenquanyi_9pt"
  SetOutPath $INSTDIR
  Call DownloadInstallZipWithoutSimutrans
  Rename $INSTDIR\wenquanyi_9pt.bdf $INSTDIR\font\wenquanyi_9pt.bdf
  Delete $INSTDIR\wenquanyi_9pt.bdf
no_chinese_needed:
SectionEnd

SectionGroupEnd


Section "Uninstall"
  SetShellVarContext all
  Delete $INSTDIR\Uninst.exe ; delete self (see explanation below why this works)
  Delete "$SMPROGRAMS\Simutrans\Simutrans.lnk"
  Delete "$SMPROGRAMS\Simutrans\Simutrans (Debug).lnk"
  Delete $INSTDIR\Uninst.exe ; delete self (see explanation below why this works)
  RMDir /r $INSTDIR
  RMDir /r $PAKDIR
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Simutrans"
SectionEnd

!include "paksets.nsh"


;************************** from here on other helper stuff *****************

!include "other-functions.nsh"

;********************* from here on special own helper functions ************

; make sure, at least one executable is installed
Function .onSelChange
!insertmacro StartRadioButtons $9
    !insertmacro RadioButton ${GDIexe}
    !insertmacro RadioButton ${SDLexe}
    !insertmacro RadioButton ${GDI64exe}
    !insertmacro RadioButton ${SDL64exe}
!insertmacro EndRadioButtons

test_for_pak:
  ; save last state of SDLexe selection
  SectionGetFlags ${SDLexe} $R0
  IntOp $group1 $R0 & ${SF_SELECTED}

  ; Make sure at least some pak is selected
  SectionGetFlags ${pak} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak64german} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak64HAJO} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak64japan} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak64HO} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak64contrast} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak64contrast} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak64scifi} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak96comic} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak96HD} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak128} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak128japan} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak128britain} $R0
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak128german} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak128cz} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak192comic} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak48excentrique} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak32comic} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  ; not pak selected!
  MessageBox MB_OK|MB_ICONSTOP "At least on pak set must be selected!"
show_not:
FunctionEnd



