;***************************************************** from here on come licence *****************************************

!include "TextFunc.nsh"


; we just ask for Artistic licences, the other re only asked for certain paks
PageEx license
 LicenseData "license.rtf"
PageExEnd


; If not installed to program dir, ask for a portable installation
Function CheckForPortableInstall
  ; defaults in progdir, and ending with simutrans
  StrCpy $installinsimutransfolder "1"
  StrCpy $multiuserinstall "1"
  ; if the destination directory is the program dir, we must use use own documents directory for data
  StrCmp $INSTDIR $PROGRAMFILES\Simutrans AllSetPortable
  StrLen $1 $PROGRAMFILES
  StrCpy $0 $INSTDIR $1
  StrCmp $0 $PROGRAMFILES YesPortable +1
  StrCpy $multiuserinstall "0"  ; check whether we already have a simuconf.tab, to get state from file
  ${ConfigRead} "$INSTDIR\config\simuconf.tab" "singleuser_install = " $R0
  IfErrors PortableUnknown
  StrCpy $multiuserinstall $R0
  Goto AllSetPortable
PortableUnknown:
  ; ask whether this is a protable installation
  MessageBox MB_YESNO|MB_ICONINFORMATION "Should this be a portable installation?" IDYES YesPortable
  StrCpy $multiuserinstall "1"
YesPortable:
  ; now check, whether the path ends with "simutrans"
  StrCpy $0 $INSTDIR 9 -9
  StrCmp $0 simutrans +2
  StrCpy $installinsimutransfolder "0"
AllSetPortable:
  ; here everything is ok
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
 ; avoids two instance at the same time ...
 System::Call 'kernel32::CreateMutexA(i 0, i 0, t "SimutransMutex") i .r1 ?e'
 Pop $R0
 StrCmp $R0 0 +3
   MessageBox MB_OK|MB_ICONEXCLAMATION "The installer is already running."
   Abort

  ;# call userInfo plugin to get user info.  The plugin puts the result in the stack
  userInfo::getAccountType
  pop $0
  ; compare the result with the string "Admin" to see if the user is admin.
  ; If match, jump 3 lines down.
  strCmp $0 "Admin" +3
  ; we are not admin: default install in a different dir
  StrCpy $INSTDIR "C:\simutrans"
  ; ok, we are admin
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

  ; we need the magic with temporary copy only if the folder does not end with simutrans ...
  StrCmp $installinsimutransfolder "0" +4
    CreateDirectory "$INSTDIR"
    nsisunz::Unzip "$TEMP\$archievename" "$INSTDIR\.."
    goto +2
    nsisunz::Unzip "$TEMP\$archievename" "$TEMP"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +4
    MessageBox MB_OK|MB_ICONINFORMATION "$R0"
    RMdir /r "$TEMP\simutrans"
    Quit

  Delete "$Temp\$archievename"
  StrCmp $installinsimutransfolder "1" +3
  CreateDirectory "$INSTDIR"
  CopyFiles "$TEMP\Simutrans\*.*" "$INSTDIR"
  RMdir /r "$TEMP\simutrans"
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
