;
; This file is part of the Simutrans project under the Artistic License.
; (see LICENSE.txt)
;

; Section define/macro header file
; See this header file for more info
!include "MUI2.nsh"
!include "Sections.nsh"
!include "zipdll.nsh"


; Parameter for functions
var downloadlink
var downloadname
var archievename

var group1
var multiuserinstall

;******** This script assumes, you have installed all the paks and GDI exe to this directory *******
;******** There should be also a folder SDL containing the SDL exe and DLL                   *******
;******** There should be also a folder pak64addon containing the food chain addon           *******
;******** There should be also a folder pak64addon/font with the chinese font                *******
!define SP_PATH "C:\Programme\simutrans"


Name "Simutrans Transport Simulator"
OutFile "simutrans-offline-install.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Simutrans

XPStyle on

; Request application privileges for Windows Vista
RequestExecutionLevel admin

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "simutranssmall.bmp"
!define MUI_BGCOLOR 000000

  !insertmacro MUI_LANGUAGE "English" ;first language is the default language
  !insertmacro MUI_LANGUAGE "French"
  !insertmacro MUI_LANGUAGE "German"
  !insertmacro MUI_LANGUAGE "Spanish"
  !insertmacro MUI_LANGUAGE "SpanishInternational"
  !insertmacro MUI_LANGUAGE "SimpChinese"
  !insertmacro MUI_LANGUAGE "TradChinese"
  !insertmacro MUI_LANGUAGE "Japanese"
  !insertmacro MUI_LANGUAGE "Italian"
  !insertmacro MUI_LANGUAGE "Dutch"
  !insertmacro MUI_LANGUAGE "Danish"
  !insertmacro MUI_LANGUAGE "Swedish"
  !insertmacro MUI_LANGUAGE "Norwegian"
  !insertmacro MUI_LANGUAGE "NorwegianNynorsk"
  !insertmacro MUI_LANGUAGE "Finnish"
  !insertmacro MUI_LANGUAGE "Greek"
  !insertmacro MUI_LANGUAGE "Russian"
  !insertmacro MUI_LANGUAGE "Portuguese"
  !insertmacro MUI_LANGUAGE "PortugueseBR"
  !insertmacro MUI_LANGUAGE "Polish"
  !insertmacro MUI_LANGUAGE "Czech"
  !insertmacro MUI_LANGUAGE "Slovak"
  !insertmacro MUI_LANGUAGE "Croatian"
  !insertmacro MUI_LANGUAGE "Bulgarian"
  !insertmacro MUI_LANGUAGE "Hungarian"
  !insertmacro MUI_LANGUAGE "Romanian"
  !insertmacro MUI_LANGUAGE "Latvian"
  !insertmacro MUI_LANGUAGE "Estonian"
  !insertmacro MUI_LANGUAGE "Turkish"
  !insertmacro MUI_LANGUAGE "Lithuanian"
  !insertmacro MUI_LANGUAGE "Indonesian"
  !insertmacro MUI_LANGUAGE "Belarusian"
  !insertmacro MUI_LANGUAGE "Catalan"


; we need to start with sections first for the second licence


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

Section "Executable (GDI, Unicode)" GDIexe
  SetOutPath $INSTDIR
  File "${SP_PATH}\simutrans.exe"
  File "${SP_PATH}\*.txt"
  File /r "${SP_PATH}\text"
  File /r "${SP_PATH}\font"
  File /r "${SP_PATH}\skin"
  File /r "${SP_PATH}\music"
  File /r "${SP_PATH}\config\simuconf.tab"
SectionEnd

Section /o "Executable (SDL, better sound)" SDLexe
  SetOutPath $INSTDIR
  File "${SP_PATH}\SDL\*.*"
  File "${SP_PATH}\*.txt"
  File /r "${SP_PATH}\text"
  File /r "${SP_PATH}\font"
  File /r "${SP_PATH}\skin"
  File /r "${SP_PATH}\music"
  File /r "${SP_PATH}\config\simuconf.tab"
SectionEnd

Section /o "Chinese Font" wenquanyi_font
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak64addon\font"
SectionEnd

SectionGroupEnd



SectionGroup "Pak64: main and addons" pak64group

Section "!pak64 (standard)" pak64
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak"
SectionEnd


Section /o "pak64 Food addon 102.2.1"
  StrCmp $multiuserinstall "1" InstallInUserDir

  ; no multiuser => install in normal directory
  SetOutPath $INSTDIR
  goto FinishFood64

InstallInUserDir:
  SetOutPath $DOCUMENTS\simutrans
FinishFood64:
  File /r "${SP_PATH}\pak64addon\pak"
SectionEnd

SectionGroupEnd



Section /o "pak64.german (Freeware)" pak64german
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak64.german"
SectionEnd



Section /o "pak64.japan" pak64japan
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak.japan"
SectionEnd



Section /o "pak64.Nippon" pak64nippon
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak.nippon"
SectionEnd



Section /o "pak64.HO (GPL)" pak64HO
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak64.HO"
SectionEnd



Section /o "pak64.SciFi" pak64SF
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak64.ScfFi"
SectionEnd



Section /o "pak64 HAJO (Freeware)" pak64HAJO
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak.HAJO"
SectionEnd



Section /o "pak64 contrast (GPL)" pak64contrast
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak.HAJO"
SectionEnd



Section /o "pak96 Comic (beta, Freeware)" pak96comic
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak96.comic"
SectionEnd



Section /o "pak96 hand drawn" pak96HD
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pakHD"
SectionEnd



Section /o "pak128" pak128
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak128"
SectionEnd



Section /o "pak128 German" pak128german
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak128.german"
SectionEnd



Section /o "pak128 Japan" pak128japan
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak128.japan"
SectionEnd



Section /o "pak128 Britain" pak128britain
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak128.britain"
SectionEnd



Section /o "pak192 Comic (Freeware)" pak192comic
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak192.comic"
SectionEnd



Section /o "pak32 Comic (alpha)" pak32comic
  SetOutPath $INSTDIR
  File /r "${SP_PATH}\pak32.comic"
SectionEnd


# create a section to define what the uninstaller does.
# the section will always be named "Uninstall"
section "Uninstall"
 Delete $INSTDIR\uninstaller.exe
 RMDir /r $INSTDIR
sectionEnd



;***************************************************** from here on come sections *****************************************



; we just ask for Artistic licences, the other re only asked for certain paks
PageEx license
 LicenseData "license.rtf"
PageExEnd


; If not installed to program dir, ask for a portable installation
Function CheckForPortableInstall
  StrCpy $multiuserinstall "1"
  StrCmp $INSTDIR $PROGRAMFILES\Simutrans NonPortable
  MessageBox MB_YESNO|MB_ICONINFORMATION "Should this be a portable installation?" IDYES Portable IDNO NonPortable
Portable:
  StrCpy $multiuserinstall "0"
NonPortable:
FunctionEnd

PageEx directory
 PageCallbacks "" "" CheckForPortableInstall
PageExEnd



PageEx components
PageExEnd



; Some paksets don't have an open source license, so we have to show additional licences
Function CheckForClosedSource

  SectionGetFlags ${pak64german} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show

  SectionGetFlags ${pak64HAJO} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show

  SectionGetFlags ${pak96comic} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show

  SectionGetFlags ${pak128} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show

  SectionGetFlags ${pak192comic} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show

  Abort

show:
  ; here is ok
FunctionEnd

PageEx License
 LicenseData "pak128.txt"
 PageCallbacks CheckForClosedSource "" ""
PageExEnd



PageEx instfiles
PageExEnd


; ******************************** From here on Functions ***************************


Function .oninit
  StrCpy $multiuserinstall "1"
 ; activates GDI by default
 StrCpy $group1 ${GDIexe} ; Group 1 - Option 1 is selected by default
 ; avoids two instance at the same time ...
 System::Call 'kernel32::CreateMutexA(i 0, i 0, t "SimutransMutex") i .r1 ?e'
 Pop $R0
 StrCmp $R0 0 +3
   MessageBox MB_OK|MB_ICONEXCLAMATION "The installer is already running."
   Abort
FunctionEnd


; ConnectInternet (uses Dialer plug-in)
; Written by Joost Verburg
;
; This function attempts to make a connection to the internet if there is no
; connection available. If you are not sure that a system using the installer
; has an active internet connection, call this function before downloading
; files with NSISdl.
;
; The function requires Internet Explorer 3, but asks to connect manually if
; IE3 is not installed.
Function ConnectInternet
    Push $R0
     ClearErrors
     Dialer::AttemptConnect
     IfErrors noie3
     Pop $R0
     StrCmp $R0 "online" connected
       MessageBox MB_OK|MB_ICONSTOP "Cannot connect to the internet."
       Quit ;This will quit the installer. You might want to add your own error handling.
     noie3:
     ; IE3 not installed
     MessageBox MB_OK|MB_ICONINFORMATION "Please connect to the internet now."
     connected:
   Pop $R0
FunctionEnd



; make sure, at least one executable is installed
Function .onSelChange

  !insertmacro StartRadioButtons $group1
    !insertmacro RadioButton ${GDIexe}
    !insertmacro RadioButton ${SDLexe}
  !insertmacro EndRadioButtons

  ; make sure GDI is installed when chinese is selected
  SectionGetFlags ${wenquanyi_font} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} +1 test_for_pak

  ; force selection of GDI exe
  SectionGetFlags ${GDIexe} $R0
  IntOp $R0 $R0 | ${SF_SELECTED}

  SectionSetFlags ${GDIexe} $R0
  SectionGetFlags ${SDLexe} $R0
  IntOp $R0 $R0 & ${SECTION_OFF}
  SectionSetFlags ${SDLexe} $R0

test_for_pak:
  ; Make sure at least some pak is selected
  SectionGetFlags ${pak64} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak64german} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak64HAJO} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak96comic} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak128} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak128japan} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak128britain} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak192comic} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  SectionGetFlags ${pak32comic} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} show_not
  ; not pak selected!
  MessageBox MB_OK|MB_ICONSTOP "At least on pak set must be selected!"
show_not:
FunctionEnd



; $downloadlink is then name of the link, $downloadanme the name of the pak for error messages
Function DownloadInstall
;  MessageBox MB_OK|MB_ICONINFORMATION "Download of $downloadname from\n$downloadlink$archievename"
  Call ConnectInternet
  RMdir /r "$TEMP\simutrans"
  NSISdl::download $downloadlink$archievename "$Temp\$archievename"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +3
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  ZipDLL::extractall "$TEMP\$archievename" "$TEMP"
  Pop $0
  StrCmp $0 "success" exeok
    DetailPrint "$0" ;print error message to log
    RMdir /r "$TEMP\simutrans"
    Quit
  exeok:
  CreateDirectory "$INSTDIR"
  CopyFiles "$TEMP\Simutrans\*.*" "$INSTDIR"
  RMdir /r "$TEMP\simutrans"
FunctionEnd
