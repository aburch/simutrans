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

!include "preparation-functions.nsh"

var group1
Name "Simutrans Transport Simulator"
OutFile "simutrans-online-install.exe"

InstallDir $PROGRAMFILES\Simutrans


SectionGroup !Simutrans

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
  CreateDirectory "$SMPROGRAMS\Simutrans"
  CreateShortCut "$SMPROGRAMS\Simutrans\Simutrans.lnk" "$INSTDIR\Simutrans.exe" "-log 1 -debug 3"
finishGDIexe:
  ; uninstaller not working yet
  ;WriteUninstaller $INSTDIR\uninstaller.exe
FunctionEnd

Section /o "Executable (GDI, run one more computers)" GDIexe
  AddSize 15051
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/122-0/simuwin-122-0.zip"
  StrCpy $archievename "simuwin-122-0.zip"
  StrCpy $downloadname "Simutrans Executable (GDI)"
  Call DownloadInstallZip
  Call PostExeInstall
SectionEnd

Section "Executable (SDL2, better sound)" SDLexe
  AddSize 17768
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/122-0/simuwin-sdl-122-0.zip"
  StrCpy $archievename "simuwin-sdl-122-0.zip"
  StrCpy $downloadname "Simutrans Executable (SDL2)"
  Call DownloadInstallZip
  Call PostExeInstall
SectionEnd

Section "Chinese Font" wenquanyi_font
  AddSize 3169
  IfFileExists $INSTDIR\font\wenquanyi_9pt.bdf no_chinese_needed
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/wenquanyi_9pt-font-bdf.zip"
  StrCpy $archievename "wenquanyi_9pt-font-bdf.zip"
  StrCpy $downloadname "wenquanyi_9pt"
  Call DownloadInstallZipWithoutSimutrans
  Rename $INSTDIR\wenquanyi_9pt.bdf $INSTDIR\font\wenquanyi_9pt.bdf
  Delete $INSTDIR\wenquanyi_9pt.bdf
no_chinese_needed:
SectionEnd

SectionGroupEnd


!include "paksets.nsh"


;************************** from here on other helper stuff *****************

!include "other-functions.nsh"


;********************* from here on special own helper functions ************


; make sure, at least one executable is installed
Function .onSelChange

  ; radio button macro does not work as intended now => do it yourself
  SectionSetFlags ${wenquanyi_font} ${SF_SELECTED}
  SectionGetFlags ${SDLexe} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} +1 test_for_pak

  ; SDL is selected
  SectionGetFlags ${GDIexe} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} +1 select_SDL
  ; ok both selected
  IntCmp $group1 0 select_SDL
  ; But GDI was not previously selected => unselect SDL
  SectionGetFlags ${SDLexe} $R0
  IntOp $R0 $R0 & ${SECTION_OFF}
  SectionSetFlags ${SDLexe} $R0
  Goto test_for_pak

select_SDL:
  ; ok SDL was selected
  SectionGetFlags ${GDIexe} $R0
  IntOp $R0 $R0 & ${SECTION_OFF}
  SectionSetFlags ${GDIexe} $R0

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



