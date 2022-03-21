;
; This file is part of the Simutrans project under the Artistic License.
; (see LICENSE.txt)
;

; Just the paksets

Section /o "pak48.excentrique V0.19 rc3" pak48excentrique
  AddSize 1908
  StrCpy $downloadlink "https://github.com/Varkalandar/pak48.Excentrique/releases/download/v0.19_RC3/pak48.excentrique_v019rc3.zip"
  SetOutPath $PAKDIR
  StrCpy $archievename "pak48.excentrique_v019rc3.zip"
  StrCpy $downloadname "pak48.Excentrique"
  StrCpy $VersionString "pak48.Excentrique v0.19"
  Call DownloadInstallZipWithoutSimutrans
SectionEnd

SectionGroup /e "Pak64: main and addons" pak64group

Section "!pak (64 size) (standard)" pak
  AddSize 15766
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64/123-0/simupak64-123-0.zip"
  SetOutPath $PAKDIR
  StrCpy $archievename "simupak64-123-0.zip"
  StrCpy $downloadname "pak"
  StrCpy $VersionString "pak64 123 r2102"
  Call DownloadInstallZip
SectionEnd


Section /o "pak64 Food addon"
  AddSize 228
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64/121-0/simupak64-addon-food-120-4.zip"
  StrCpy $archievename "simupak64-addon-food-120-4.zip"
  StrCpy $downloadname "pak"
  StrCpy $VersionString ""
  StrCmp $multiuserinstall "1" +3
  ; no multiuser => install in normal directory
  Call DownloadInstallAddonZipPortable
  goto +2
  Call DownloadInstallAddonZip
SectionEnd

SectionGroupEnd

Section "pak64.german (Freeware) for 123" pak64german
  AddSize 22460
  StrCpy $downloadlink "https://simutrans-germany.com/pak.german/pak64.german_0-123-0-0-2_full.zip"
  StrCpy $archievename "pak64.german_0-123-0-0-2_full.zip"
  StrCpy $downloadname "pak64.german"
  StrCpy $VersionString "pak64.german 0.123.0.0.2"
  SetOutPath $PAKDIR
  Call DownloadInstallZip
SectionEnd


Section /o "pak.japan (64 size)" pak64japan
  AddSize 10175
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64.japan/123-0/simupak64.japan-123-0.zip"
  StrCpy $archievename "simupak64.japan-123-0.zip"
  StrCpy $downloadname "pak.japan"
  StrCpy $VersionString "pak64.japan 123 r2101"
  SetOutPath $PAKDIR
  Call DownloadInstallZip
SectionEnd


Section /o "pak.Nippon (64 size) V0.5" pak64nippon
  AddSize 48300
  StrCpy $downloadlink "https://github.com/wa-st/pak-nippon/releases/download/v0.5.0/pak.nippon-v0.5.0.zip"
  StrCpy $archievename "pak.nippon-v0.5.0.zip"
  StrCpy $downloadname "pak.nippon"
  StrCpy $VersionString "pak.nippon v0.5.0"
  SetOutPath $PAKDIR
  Call DownloadInstallZipWithoutSimutrans
SectionEnd

Section "pak128.German V2.1" pak128german
  AddSize 558832
	StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/PAK128.german/PAK128.german_2.1_for_ST_123.0/PAK128.german_2.1_for_ST_123.0.zip"
  StrCpy $archievename "PAK128.german_2.1_for_ST_123.0.zip"
  StrCpy $downloadname "pak128.German"
  StrCpy $VersionString "Pak128.german VS 2.1 (Rev. 473)"
  SetOutPath $PAKDIR
  Call DownloadInstallZip
SectionEnd

; name does not match folder name (pak128.japan) but otherwise always selected for update
Section /o "pak128.CZ (0.3) r2078" pak128cz
  AddSize 78164
  StrCpy $downloadlink "https://downloads.sourceforge.net/project/simutrans/Pak128.CS/nightly%20builds/pak128.CS-r2078.zip"
  StrCpy $archievename "pak128.CS-r2078.zip"
  StrCpy $downloadname "pak128.CZ"
  StrCpy $VersionString "Pak128.CS 0.3.0"
  SetOutPath $PAKDIR
  Call DownloadInstallZipWithoutSimutrans
SectionEnd

Section /o "pak128 V2.8.2" pak128
  AddSize 413800
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak128/pak128%202.8.2%20for%20ST%20123up/simupak128-2.8.2-for123.zip"
  StrCpy $archievename "simupak128-2.8.2-for123.zip"
  StrCpy $downloadname "pak128"
  StrCpy $VersionString "Pak128 2.8.1"
  SetOutPath $PAKDIR
  Call DownloadInstallZip
SectionEnd

Section "pak192.Comic 0.6 RC2 (CC-BY-SA)" pak192comic
  AddSize 794299
  StrCpy $downloadlink "https://github.com/Flemmbrav/Pak192.Comic/releases/download/2021-V0.6-RC2/pak192.comic.0.6.RC2.zip"
  StrCpy $archievename "pak192.comic.0.6.RC2.zip"
  StrCpy $downloadname "pak192.comic-nightly-datconverter"
  StrCpy $VersionString "Pak192.Comic 2021 [V 0.6 RC 1]"
  SetOutPath $PAKDIR
  Call DownloadInstallZipWithoutSimutrans
SectionEnd



; OBSOLETE PAKS from here
SectionGroup /e "Not currently developed (still popular)" slowPakgroup

; name does not match folder name (pak96.comic) but otherwise always selected for update
Section /o "pak96 Comic (Freeware) V0.4.10 expansion" pak96comic
  AddSize 32304
  StrCpy $downloadlink "https://downloads.sourceforge.net/project/simutrans/pak96.comic/pak96.comic%20for%20111-3/pak96.comic-0.4.10-plus.zip"
  StrCpy $archievename "pak96.comic-0.4.10-plus.zip"
  StrCpy $downloadname "pak96.comic"
  StrCpy $VersionString "pak96.comic V4.1 plus"
  SetOutPath $PAKDIR
  Call DownloadInstallZip
SectionEnd

; name does not match folder name (pak128.japan) but otherwise always selected for update
Section /o "pak128.Japan for Simutrans 120.0" pak128japan
  AddSize 26419
  StrCpy $downloadlink "http://pak128.jpn.org/souko/pak128.japan.120.0.cab"
  StrCpy $archievename "pak128.japan.120.0.cab"
  StrCpy $downloadname "pak128.Japan"
  StrCpy $VersionString "Pak128.Japan 120.0"
  SetOutPath $PAKDIR
  Call DownloadInstallCabWithoutSimutrans
SectionEnd


Section /o "pak128.Britain V1.18" pak128britain
  AddSize 234108
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak128.britain/pak128.Britain%20for%20120-3/pak128.Britain.1.18-120-3.zip"
  StrCpy $archievename "pak128.Britain.1.18-120-3.zip"
  StrCpy $downloadname "pak128.Britain"
  StrCpy $VersionString "pak128.Britain 1.18 120.3 r1991"
  SetOutPath $PAKDIR
  Call DownloadInstallZip
SectionEnd

SectionGroupEnd



SectionGroup "Obsolete pak set (still working)" obsoltePakgroup

; name does not match folder name (pak32) but otherwise always selected for update
Section /o "pak32.Comic (alpha) for 102.2.1" pak32comic
  AddSize 2108
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak32.comic/pak32.comic%20for%20102-0/pak32.comic_102-0.zip"
  StrCpy $archievename "pak32.comic_102-0.zip"
  StrCpy $downloadname "pak32.Comic"
  StrCpy $VersionString ""
  SetOutPath $PAKDIR
  Call DownloadInstallZip
SectionEnd

; name does not match folder name (pak64.ho-scale) but otherwise always updated
Section /o "pak64 HO-scale (GPL)" pak64HO
  AddSize 8532
  StrCpy $downloadlink "http://simutrans.bilkinfo.de/pak64.ho-scale-latest.tar.gz"
  StrCpy $archievename "pak64.ho-scale-latest.tar.gz"
  StrCpy $downloadname "pak64.HO"
  StrCpy $VersionString ""
  SetOutPath $PAKDIR
  Call DownloadInstallTgzWithoutSimutrans
SectionEnd

; name does not match folder name (pakHajo) but not update expected anyway ...
Section /o "pak64.HAJO (Freeware) for 102.2.2" pak64HAJO
  AddSize 6376
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pakHAJO/pakHAJO_102-2-2/pakHAJO_0-102-2-2.zip"
  StrCpy $archievename "pakHAJO_0-102-2-2.zip"
  StrCpy $downloadname "pak64.HAJO"
  StrCpy $VersionString ""
  SetOutPath $PAKDIR
  Call DownloadInstallZip
SectionEnd


Section /o "pak64.contrast (GPL) for 102.2.2" pak64contrast
   AddSize 1367
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64.contrast/pak64.Contrast_910.zip"
  StrCpy $archievename "pak64.Contrast_910.zip"
  StrCpy $downloadname "pak64.contrast"
  StrCpy $VersionString ""
  SetOutPath $PAKDIR
  Call DownloadInstallZipWithoutSimutrans
SectionEnd


Section /o "pak64.SciFi V0.2 (alpha)" pak64scifi
  AddSize 3028
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64.scifi/pak64.scifi_112.x_v0.2.zip"
  StrCpy $archievename "pak64.scifi_112.x_v0.2.zip"
  StrCpy $VersionString "pak64.SciFi V0.2"
  StrCpy $downloadname "pak64.SciFi"
  SetOutPath $PAKDIR
  Call DownloadInstallZip
SectionEnd

; name does not match folder name (pakHD) but not update expected anyway ...
Section /o "pak96.HD (96 size) V0.4 for 102.2.2" pak96HD
  AddSize 12307
  StrCpy $downloadlink "https://hd.simutrans.com/release/PakHD_v04B_100-0.zip"
  StrCpy $archievename "PakHD_v04B_100-0.zip"
  StrCpy $downloadname "pak96.HD"
  StrCpy $VersionString "Martin"
  SetOutPath $PAKDIR
# since download works different, we have to do it by hand
  RMdir /r "$TEMP\simutrans"
  CreateDirectory "$TEMP\simutrans"
  inetc::get /WEAKSECURITY $downloadlink "$Temp\$archievename" /END
  Pop $0
  StrCmp $0 "OK" +3
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

SectionGroupEnd

