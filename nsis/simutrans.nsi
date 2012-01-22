; Section define/macro header file
; See this header file for more info
!include "MUI2.nsh"
!include "Sections.nsh"
#!include "zipdll.nsh"


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
  AddSize 8588
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/111-0/simuwin-111-0.zip"
  StrCpy $archievename "simuwin-111-0.zip"
  StrCpy $downloadname "Simutrans Executable (GDI)"
  Call DownloadInstallZip
  Call PostExeInstall
SectionEnd


Section /o "Executable (SDL, better sound)" SDLexe
  AddSize 9277
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/111-0/simuwin-sdl-111-0.zip"
  StrCpy $archievename "simuwin-sdl-111-0.zip"
  StrCpy $downloadname "Simutrans Executable (SDL)"
  Call DownloadInstallZip
  Call PostExeInstall
SectionEnd

Section /o "Chinese Font" wenquanyi_font
  AddSize 3245
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/simutrans/wenquanyi_9pt-font-bdf.zip"
  StrCpy $archievename "wenquanyi_9pt-font-bdf.zip"
  StrCpy $downloadname "wenquanyi_9pt"
  Call DownloadInstallZip
SectionEnd

SectionGroupEnd



SectionGroup "Pak64: main and addons" pak64group

Section "!pak64 (standard)" pak64
  AddSize 10683
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64/111-1/simupak64-111-1.zip"
  StrCpy $archievename "simupak64-111-0.zip"
  StrCpy $downloadname "pak64"
  Call DownloadInstallZip
SectionEnd


Section /o "pak64 Food addon"
  AddSize 280
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64/111-1/simupak64-food-111-1.zip"
  StrCpy $archievename "simupak64-addon-food-111-0.zip"
  StrCpy $downloadname "pak64"
  StrCmp $multiuserinstall "1" +3
  ; no multiuser => install in normal directory
  Call DownloadInstallZip
  goto +2
  Call DownloadInstallAddonZip
SectionEnd

SectionGroupEnd



SectionGroup "Pak64.german: main and addons" pak64germangroup

Section /o "pak64.german (Freeware) 110.0c" pak64german
  AddSize 14971
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak.german/pak64.german-110-0c/simupak-german64-110-0c.zip"
  StrCpy $archievename "simupak-german64-110-0c.zip"
  StrCpy $downloadname "pak64.german"
  Call DownloadInstallZip
SectionEnd

Section /o "pak64.german full industries"
  AddSize 222
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak.german/pak64.german-110-0c/simupak-german64-industry-110-0.zip"
  StrCpy $archievename "simupak-german64-industry-110-0.zip"
  StrCpy $downloadname "pak64.german"
  MessageBox MB_OK|MB_ICONINFORMATION "Download of $downloadname from\n$downloadlink to $archievename"
  StrCmp $multiuserinstall "1" +3
  ; no multiuser => install in normal directory
  Call DownloadInstallZip
  goto +2
  Call DownloadInstallAddonZip
SectionEnd

Section /o "pak64.german Tourist addon"
  AddSize 222
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak.german/pak64.german-110-0c/simutrans-german64-addons-110-0.zip"
  StrCpy $archievename "simutrans-german64-addons-110-0.zip"
  StrCpy $downloadname "pak64.german"
  MessageBox MB_OK|MB_ICONINFORMATION "Download of $downloadname from\n$downloadlink to $archievename"
  StrCmp $multiuserinstall "1" +3
  ; no multiuser => install in normal directory
  Call DownloadInstallZip
  goto +2
  Call DownloadInstallAddonZip
SectionEnd

SectionGroupEnd




Section /o "pak64.japan 110.0.0" pak64japan
  AddSize 6596
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak.japan/110-0/simupak64.japan-110-0.zip"
  StrCpy $archievename "simupak64.japan-110-0.zip"
  StrCpy $downloadname "pak64.japan"
  Call DownloadInstallZip
SectionEnd



Section /o "pak64 HO-scale (GPL)" pak64HO
  AddSize 8527
  StrCpy $downloadlink "http://simutrans.bilkinfo.de/pak64.ho-scale-latest.tar.gz"
  StrCpy $archievename "pak64.ho-scale-latest.tar.gz"
  StrCpy $downloadname "pak64.HO"
  Call DownloadInstallTgzWithoutSimutrans
SectionEnd



Section /o "pak64 HAJO (Freeware) 102.2.2" pak64HAJO
  AddSize 6376
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pakHAJO/pakHAJO_102-2-2/pakHAJO_0-102-2-2.zip"
  StrCpy $archievename "pakHAJO_0-102-2-2.zip"
  StrCpy $downloadname "pak64.HAJO"
  Call DownloadInstallZip
SectionEnd



Section /o "pak64.contrast (GPL) 102.2.2" pak64contrast
   AddSize 1367
  StrCpy $downloadlink "http://addons.simutrans.com/get.php?type=addon&aid=166"
  StrCpy $archievename "simuAddon_Contrast_910.zip"
  StrCpy $downloadname "pak64.contrast"
  Call DownloadInstallZipWithoutSimutrans
SectionEnd



Section /o "pak96 Comic (Freeware) V0.4.10" pak96comic
  AddSize 29447
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak96.comic/pak96.comic%20for%20110-0-1/pak96.comic-0.4.10.zip"
  StrCpy $archievename "pak96.comic-0.4.10.zip"
  StrCpy $downloadname "pak96.Comic"
  Call DownloadInstallZip
SectionEnd



Section /o "pak96.HD (0.4) for 102.2.2" pak96HD
  AddSize 26189
  StrCpy $downloadlink "http://hd.simutrans.com/release/PakHD_v04B_100-0.zip"
  StrCpy $archievename "PakHD_v04B_100-0.zip"
  StrCpy $downloadname "pak96.HD"
# since download works different, we have to do it by hand
  RMdir /r "$TEMP\simutrans"
  CreateDirectory "$TEMP\simutrans"
  NSISdl::download $downloadlink "$Temp\$archievename"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +3
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  nsisunz::Unzip "$TEMP\$archievename" "$TEMP\simutrans"
  Pop $R0
  StrCmp $R0 "success" +4
    DetailPrint "$0" ;print error message to log
    RMdir /r "$TEMP\simutrans"
    Quit

  CreateDirectory "$INSTDIR"
  Delete "$Temp\$archievename"
  RMdir /r "$TEMP\simutrans\config"
  CopyFiles "$TEMP\Simutrans\*.*" "$INSTDIR"
  RMdir /r "$TEMP\simutrans"
SectionEnd



Section /o "pak128 2.00" pak128
  AddSize 70765
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak128/pak128%20for%20111-0/pak128-2.0.0--111.0.zip"
  StrCpy $archievename "pak128-2.0.0--111.0.zip"
  StrCpy $downloadname "pak128"
  Call DownloadInstallZip
SectionEnd



Section /o "pak128 Britain (1.10) 111.0" pak128britain
  AddSize 144922
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak128.britain/pak128.Britain%20for%20111-0/pak128.Britain.1.10-111-0.zip"
  StrCpy $archievename "pak128.Britain.1.10-111-0.zip"
  StrCpy $downloadname "pak128.Britain"
  Call DownloadInstallZipWithoutSimutrans
SectionEnd



Section /o "pak128 German V0.2.1" pak128german
  AddSize 47307
  StrCpy $downloadlink "http://www.simutrans-germany.com/~pak128german/files/PAK128.german_0.2.1.zip"
  StrCpy $archievename "PAK128.german_0.2.1.zip"
  StrCpy $downloadname "pak128.German"
# since download works different, we have to do it by hand
  RMdir /r "$TEMP\simutrans"
  CreateDirectory "$TEMP\simutrans"
  NSISdl::download $downloadlink "$Temp\$archievename"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +3
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  nsisunz::Unzip "$TEMP\$archievename" "$TEMP\Simutrans"
  Pop $0
  StrCmp $0 "success" +4
    DetailPrint "$0" ;print error message to log
    RMdir /r "$TEMP\Simutrans"
    Quit

  CreateDirectory "$INSTDIR"
  Delete "$Temp\$archievename"
  RMdir /r "$TEMP\Simutrans\PAK128.german_0.2.1\de.tab_fuer_programmordner"
  RMdir "$TEMP\Simutrans\PAK128.german_0.2.1\*.txt"
  CopyFiles "$TEMP\Simutrans\PAK128.german_0.1.2\*.*" "$INSTDIR"
  RMdir /r "$TEMP\Simutrans"
SectionEnd


Section /o "pak128.Japan 110.0.1" pak128japan
  AddSize 17555
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak128.japan/for%20Simutrans%20110.0.1/pak128.japan-110.0.1-version16-08-2011.zip"
  StrCpy $archievename "pak128.japan-110.0.1-version16-08-2011.zip"
  StrCpy $downloadname "pak128.Japan"
  Call DownloadInstallZipWithoutSimutrans
SectionEnd


Section /o "pak192 Comic (Freeware) 102.2.1" pak192comic
  AddSize 23893
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak192.comic/pak192.comic_102-2-1/pak192.comic_0-1-9-1_102-2-1.zip"
  StrCpy $archievename "pak192.comic_0-1-9-1_102-2-1.zip"
  StrCpy $downloadname "pak192.Comic"
  Call DownloadInstallZip
SectionEnd



Section /o "pak48 excentrique 0.15" pak48excentrique
  AddSize 1136
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak48.Excentrique/v0.15-for-Simutrans-111.0/pak48_excentrique-v0_15.zip"
  StrCpy $archievename "pak48_excentrique-v0_15.zip"
  StrCpy $downloadname "pak48.Excentrique"
  Call DownloadInstallZip
SectionEnd



Section /o "pak32 Comic (alpha) 102.2.1" pak32comic
  AddSize 2108
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak32.comic/pak32.comic%20for%20102-0/pak32.comic_102-0.zip"
  StrCpy $archievename "pak32.comic_102-0.zip"
  StrCpy $downloadname "pak32.Comic"
  Call DownloadInstallZip
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
  IntCmp $R0 ${SF_SELECTED} showFW

  SectionGetFlags ${pak64HAJO} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showFW

  SectionGetFlags ${pak96comic} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showFW

  SectionGetFlags ${pak96HD} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showFW

  SectionGetFlags ${pak128japan} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showFW

  SectionGetFlags ${pak128german} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showFW

  SectionGetFlags ${pak192comic} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showFW

  Abort

showFW:
  ; here is ok
FunctionEnd

PageEx License
 LicenseData "pak128.txt"
 PageCallbacks CheckForClosedSource "" ""
PageExEnd



; Some packs are GPL
Function CheckForGPL

  SectionGetFlags ${pak64HO} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showGPL

  SectionGetFlags ${pak64contrast} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showGPL

  Abort

showGPL:
  ; here is ok
FunctionEnd

PageEx License
 LicenseData "GPL.txt"
 PageCallbacks CheckForGPL "" ""
PageExEnd



PageEx instfiles
PageExEnd


; ******************************** From here on Functions ***************************


Function .oninit
  InitPluginsDir
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



; $downloadlink is then name of the link, $downloadname the name of the pak for error messages
Function DownloadInstallZip
#  MessageBox MB_OK|MB_ICONINFORMATION "Download of $downloadname from\n$downloadlink to $archievename"
  Call ConnectInternet
  RMdir /r "$TEMP\simutrans"
  NSISdl::download $downloadlink "$Temp\$archievename"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +3
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  nsisunz::Unzip "$TEMP\$archievename" "$TEMP"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +4
    MessageBox MB_OK|MB_ICONINFORMATION "$R0"
    RMdir /r "$TEMP\simutrans"
    Quit

  CreateDirectory "$INSTDIR"
  CopyFiles "$TEMP\Simutrans\*.*" "$INSTDIR"
  RMdir /r "$TEMP\simutrans"
  Delete "$Temp\$archievename"
FunctionEnd




; $downloadlink is then name of the link, $downloadname the name of the pak for error messages
Function DownloadInstallAddonZip
#  MessageBox MB_OK|MB_ICONINFORMATION "Download of $downloadname from\n$downloadlink to $archievename"
  Call ConnectInternet
  RMdir /r "$TEMP\simutrans"
  NSISdl::download $downloadlink "$Temp\$archievename"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +3
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  nsisunz::Unzip "$TEMP\$archievename" "$DOCUMENTS"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +4
    DetailPrint "$0" ;print error message to log
    Delete "$TEMP\$archievename"
    Quit

  Delete "$Temp\$archievename"
FunctionEnd



; $downloadlink is then name of the link, $downloadname the name of the pak for error messages
Function DownloadInstallZipWithoutSimutrans
#  MessageBox MB_OK|MB_ICONINFORMATION "Download of $downloadname from\n$downloadlink to $archievename"
  Call ConnectInternet
  RMdir /r "$TEMP\simutrans"
  CreateDirectory "$TEMP\simutrans"
# since we also want to download from addons ...
   inetc::get $downloadlink "$Temp\$archievename"
#  NSISdl::download $downloadlink "$Temp\$archievename"
  Pop $R0 ;Get the return value
  StrCmp $R0 "OK" +3
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  CreateDirectory "$INSTDIR"
  nsisunz::Unzip "$TEMP\$archievename" "$INSTDIR"
  Pop $R0
  StrCmp $R0 "success" +4
    Delete "$Temp\$archievename"
    DetailPrint "$0" ;print error message to log
    Quit

  Delete "$Temp\$archievename"
FunctionEnd



Function DownloadInstallCabWithoutSimutrans
  MessageBox MB_OK|MB_ICONINFORMATION "Download of $downloadname from\n$downloadlink to $archievename"
  Call ConnectInternet
  RMdir /r "$TEMP\simutrans"
  NSISdl::download $downloadlink "$Temp\$archievename"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +3
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  CabDLL::CabView "$TEMP\$archievename"
  MessageBox MB_OK "Download of $archievename to $TEMP"
  CabDLL::CabExtractAll "$TEMP\$archievename" "$TEMP\Simutrans"
  DumpState::debug
  StrCmp $R0 "success" +4
    DetailPrint "$0" ;print error message to log
    RMdir /r "$TEMP\simutrans"
    Quit

  CreateDirectory "$INSTDIR"
  CopyFiles "$TEMP\Simutrans\*.*" "$INSTDIR"
  RMdir /r "$TEMP\Simutrans"
  Delete "$Temp\$archievename"
FunctionEnd



Function DownloadInstallTgzWithoutSimutrans
#  MessageBox MB_OK|MB_ICONINFORMATION "Download of $downloadname from\n$downloadlink to $archievename"
  Call ConnectInternet
  RMdir /r "$TEMP\simutrans"
  NSISdl::download $downloadlink "$Temp\$archievename"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +3
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  CreateDirectory "$INSTDIR"
  untgz::extract -d "$INSTDIR" "$TEMP\$archievename"
  StrCmp $R0 "success" +4
    Delete "$Temp\$archievename"
    MessageBox MB_OK "Extraction of $archievename failed: $R0"
    Quit

  Delete "$Temp\$archievename"
FunctionEnd

