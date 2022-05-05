;
; This file is part of the Simutrans project under the Artistic License.
; (see LICENSE.txt)
;

; ************************ Preparations for installer / downloader **************************

!include "MUI2.nsh"
!include "Sections.nsh"

VAR /Global PAKDIR
VAR /Global USERDIR

; needs the following plugins:
; nsisunz
; inetc
; CabDll
; untgz
; ShellLink


; Parameter for functions
var downloadlink
var downloadname
var archievename
var VersionString

var multiuserinstall
var installinsimutransfolder
var installpaksetfolder

XPStyle on

!define MUI_ICON "..\stormoog.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "simutranssmall.bmp"
!define MUI_BGCOLOR 000000

!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_COMMANDLINE

!include MultiUser.nsh

