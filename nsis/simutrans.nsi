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

Name "Simutrans Transport Simulator"
OutFile "simutrans-online-install.exe"

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
  FileSeek $0 866
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
  AddSize 7279
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/102-2-2/"
  StrCpy $archievename "simuwin-102-2-2.zip"
  StrCpy $downloadname "Simutrans Executable (GDI)"
  Call DownloadInstall
  Call PostExeInstall
SectionEnd


Section /o "Executable (SDL, better sound)" SDLexe
  AddSize 8227
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/102-2-2/"
  StrCpy $archievename "simuwin-sdl-102-2-2.zip"
  StrCpy $downloadname "Simutrans Executable (SDL)"
  Call DownloadInstall
  Call PostExeInstall
SectionEnd

Section /o "Chinese Font" wenquanyi_font
  AddSize 3245
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/"
  StrCpy $archievename "wenquanyi_9pt-font-bdf.zip"
  StrCpy $downloadname "Pak64"
  Call DownloadInstall
SectionEnd

SectionGroupEnd



SectionGroup "Pak64: main and addons" pak64group

Section "!pak64 (standard)" pak64
  AddSize 10882
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64/102-2-2/"
  StrCpy $archievename "simupak64-102-2-2.zip"
  StrCpy $downloadname "Pak64"
  Call DownloadInstall
SectionEnd


Section /o "pak64 Food addon 102.2.1"
  AddSize 272
  StrCmp $multiuserinstall "1" InstallInUserDir

  ; no multiuser => install in normal directory
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64/102-2-1/"
  StrCpy $archievename "simupak64-addon-food-102-2-1.zip"
  StrCpy $downloadname "Pak64"
  Call DownloadInstall
  goto FinishFood64

InstallInUserDir:
  ; else install in User directory
  Call ConnectInternet
  RMdir /r "$TEMP\simutrans"
  NSISdl::download "http://downloads.sourceforge.net/project/simutrans/pak64/102-2-1/simupak64-addon-food-102-2-1.zip" "$Temp\simupak64-addon-food-102-2-1.zip"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +3
     MessageBox MB_OK "Download of food addon pak64 failed: $R0"
     Abort

  ZipDLL::extractall "$TEMP\simupak64-addon-food-102-2-1.zip" "$TEMP"
  Pop $0
  StrCmp $0 "success" +4
    DetailPrint "$0" ;print error message to log
    RMdir /r "$TEMP\simutrans"
    Abort

  CopyFiles "$TEMP\Simutrans" "$DOCUMENTS"
  RMdir /r "$TEMP\simutrans"
FinishFood64:
SectionEnd

SectionGroupEnd



Section /o "pak64.german (Freeware) 102.2.1" pak64german
  AddSize 23040
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak.german/pak.german_102-2-1/"
  StrCpy $archievename "pak.german_0-102-2-1_full.zip"
  StrCpy $downloadname "Pak64.German"
  Call DownloadInstall
  RMdir /r "$INSTDIR\Simutrans\Maps"
SectionEnd



Section /o "pak64 HAJO (Freeware) 102.2" pak64HAJO
  AddSize 6237
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pakHAJO/pakHAJO%20for%20102-0/"
  StrCpy $archievename "pakHAJO_102-0.zip"
  StrCpy $downloadname "Pak64.HAJO"
  Call DownloadInstall
SectionEnd



Section /o "pak96 Comic (beta, Freeware) 102.2" pak96comic
  AddSize 16976
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pakHAJO/pakHAJO%20for%20102-0/"
  StrCpy $downloadlink "http://www.simutrans-forum.de/forum/attachment.php?attachmentid=8734"
  StrCpy $downloadname "Pak96.Comic"
  Call DownloadInstall
SectionEnd



Section /o "pak128 (Freeware) 102.2.2" pak128
  AddSize 63445
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak128/pak128%20for%20102-2-2/"
  StrCpy $archievename "pak128-1.4.6--102.2.zip"
  StrCpy $downloadname "Pak128"
  Call DownloadInstall
SectionEnd



Section /o "pak128 Japan 101.0" pak128japan
  AddSize 15605
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak128.japan/for%20Simutrans%20101-0/"
  StrCpy $archievename "pak128.japan_0-101.zip"
  StrCpy $downloadname "Pak128.Japan"
  Call DownloadInstall
SectionEnd



Section /o "pak128 Britain (0.7) 102.2.1" pak128britain
  AddSize 94003
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak128.britain/pak128.britain%20for%20102-0/"
  StrCpy $archievename "pak128Britain_1-0-7_0-102.zip"
  StrCpy $downloadname "Pak128.Britain"
  Call DownloadInstall
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



; Some packs have not opene source license, so we have to show additional licences
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



