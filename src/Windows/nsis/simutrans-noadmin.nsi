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

Unicode true

var group1
Name "Simutrans Transport Simulator"
OutFile "simutrans-online-install-noadmin.exe"

InstallDir $LOCALAPPDATA\Simutrans

!define MUI_UNICON "..\stormoog.ico"

!include "preparation-functions.nsh"

!include "languages.nsh"

SectionGroup /e !Simutrans

Function PostExeInstall
  WriteUninstaller $INSTDIR\uninstall.exe

  StrCmp $multiuserinstall "1" NotPortable

  ; just change to simuconf.tab "singleuser_install = 1"
  FileOpen $0 "$INSTDIR\config\simuconf.tab" a
  FileSeek $0 924
  FileWrite $0 "singleuser_install = 1 "
  FileClose $0
  goto finishGDIexe

NotPortable:
  ; make start menu entries
  CreateDirectory "$SMPROGRAMS\Simutrans"
  CreateShortCut "$SMPROGRAMS\Simutrans\Simutrans.lnk" "$INSTDIR\Simutrans.exe" ""
  CreateShortCut "$SMPROGRAMS\Simutrans\Simutrans (Debug).lnk" "$INSTDIR\Simutrans.exe" "-log 1 -debug 3"
  CreateShortCut "$SMPROGRAMS\Simutrans\Simutrans Uninstall.lnk" "$INSTDIR\uninstall.exe" ""

finishGDIexe:
FunctionEnd

Section /o "Executable (GDI)" GDIexe
  AddSize 17562
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/124-0/simuwin-124-0.zip"
  StrCpy $archievename "simuwin-124-0.zip"
  StrCpy $downloadname "Simutrans Executable (GDI)"
  SetOutPath $INSTDIR
  Call DownloadInstallZip
  Call PostExeInstall
SectionEnd

Section "Executable (SDL2)" SDLexe
  AddSize 21412
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/124-0/simuwin-sdl-124-0.zip"
  StrCpy $archievename "simuwin-sdl-124-0.zip"
  StrCpy $downloadname "Simutrans Executable (SDL2)"
  SetOutPath $INSTDIR
  Call DownloadInstallZip
  Call PostExeInstall
SectionEnd

Section /o "Executable (GDI 64bit)" GDI64exe
  AddSize 18196
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/124-0/simuwin-x64-124-0.zip"
  StrCpy $archievename "simuwin-x64-124-0.zip"
  StrCpy $downloadname "Simutrans Executable (GDI) only needed for huge maps"
  SetOutPath $INSTDIR
  Call DownloadInstallZip
  Call PostExeInstall
SectionEnd

Section /o "Executable (SDL2 64bit)" SDL64exe
  AddSize 16908
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/124-0/simuwin-x64-sdl-124-0.zip"
  StrCpy $archievename "simuwin-sdl-x64-124-0.zip"
  StrCpy $downloadname "Simutrans Executable (SDL2) only needed for huge maps"
  SetOutPath $INSTDIR
  Call DownloadInstallZip
  Call PostExeInstall
SectionEnd

SectionGroupEnd


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
  SectionGetFlags ${pak64.german} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pakHAJO} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak.japan} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak.nippon} $R0
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak64.ho-scale} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pakcontrast} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak64.scifi} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak96.comic} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pakHD} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak128} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak128.japan} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak128.britain} $R0
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${PAK128.german} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak128.CS} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak192.comic} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak144.Excentrique} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak48.Excentrique} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak32} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pakTTD} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  ; not pak selected!
  MessageBox MB_OK|MB_ICONSTOP "At least 1 pak set must be selected!" /SD IDOK
show_not:
FunctionEnd



