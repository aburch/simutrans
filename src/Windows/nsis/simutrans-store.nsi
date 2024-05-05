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

!define VERSION "0.124.0.0"

RequestExecutionLevel user
!define MULTIUSER_EXECUTIONLEVEL user

VIProductVersion "${VERSION}"
VIFileVersion "${VERSION}"

VIAddVersionKey "ProductName" "Simutrans"
;VIAddVersionKey "Comments" "A test comment"
VIAddVersionKey "CompanyName" "Simutrans Team"
VIAddVersionKey "LegalCopyright" "Artistic License"
VIAddVersionKey "FileDescription" "Simutrans installer (local user)"
VIAddVersionKey "FileVersion" "${VERSION}"

!define INSTALLSIZE 21412
!define HELPURL "http://forum.simutrans.com/" # "Support Information" link


SilentInstall silent

Unicode true

var group1
Name "Simutrans Transport Simulator"
OutFile "simutrans-store.exe"

InstallDir $LOCALAPPDATA\Simutrans

!define MUI_UNICON "..\stormoog.ico"

!include "preparation-functions.nsh"

!include "languages.nsh"

Function PostExeInstall
  WriteUninstaller $INSTDIR\uninstall.exe

  ; make start menu entries
  CreateDirectory "$SMPROGRAMS\Simutrans"
  CreateShortCut "$SMPROGRAMS\Simutrans\Simutrans.lnk" "$INSTDIR\Simutrans.exe" ""
  CreateShortCut "$SMPROGRAMS\Simutrans\Simutrans (Debug).lnk" "$INSTDIR\Simutrans.exe" "-log 1 -debug 3"
  CreateShortCut "$SMPROGRAMS\Simutrans\Simutrans Uninstall.lnk" "$INSTDIR\uninstall.exe" ""
FunctionEnd

Section "Executable (SDL2)" SDLexe
  SetOutPath $INSTDIR
  File /r simuwin-sdl-124-0\Simutrans\*.*
;  AddSize 21412
;  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/124-0/simuwin-sdl-124-0.zip"
;  StrCpy $archievename "simuwin-sdl-124-0.zip"
;  StrCpy $downloadname "Simutrans Executable (SDL2)"
;  SetOutPath $INSTDIR
;  Call DownloadInstallZip
  Call PostExeInstall
SectionEnd

Section "Uninstall"
  Delete $INSTDIR\Uninstall.exe ; delete self (see explanation below why this works)
  Delete "$SMPROGRAMS\Simutrans\Simutrans.lnk"
  Delete "$SMPROGRAMS\Simutrans\Simutrans (Debug).lnk"
  Delete $INSTDIR\Uninst.exe ; delete self (see explanation below why this works)
  RMDir /r $INSTDIR
  SetShellVarContext all
  StrCpy $PAKDIR "$LOCALAPPDATA\simutrans"
  SetShellVarContext current
  MessageBox MB_YESNO "Remove global paksets from $PAKDIR?" /SD IDYES IDNO +2
  RMDir /r $PAKDIR
SectionEnd

; ******************************** From here on Functions ***************************


Function .oninit
  StrCpy $USERDIR "$LOCALAPPDATA\simutrans"
  SetShellVarContext all
  StrCpy $PAKDIR "$LOCALAPPDATA\simutrans"
;  SectionSetFlags SDLexe
  SetShellVarContext current
FunctionEnd

;********************* from here on special own helper functions ************
