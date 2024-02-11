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

!define PAKSETINSTALL 1

!include "preparation-functions.nsh"
!include "languages.nsh"

!define MULTIUSER_INSTALLMODE_INSTDIR "Simutrans"
!define MULTIUSER_EXECUTIONLEVEL Standard

Name "Simutrans Pakset Installer"
OutFile "download-paksets.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Simutrans

SectionGroup Simutrans

Section /o "Chinese Font" wenquanyi_font
  AddSize 3245
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/wenquanyi_9pt-font-bdf.zip"
  StrCpy $archievename "wenquanyi_9pt-font-bdf.zip"
  StrCpy $downloadname "wenquanyi_9pt"
  Call DownloadInstallZip
SectionEnd

SectionGroupEnd


!include "paksets.nsh"


;************************** from here on other helper stuff *****************

!include "other-functions.nsh"


;********************* from here on special own helper functions ************

; make sure, at least one executable is installed
Function .onSelChange
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
  ; not pak selected!
  MessageBox MB_OK|MB_ICONSTOP "At least on pak set must be selected!" /SD IDOK
  SetErrorLevel 4
show_not:
FunctionEnd
