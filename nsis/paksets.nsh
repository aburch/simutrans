; JUst the paksets

SectionGroup "Pak64: main and addons" pak64group

Section "!pak (64 size) (standard)" pak
  AddSize 10723
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64/111-3/simupak64-111-3.zip"
  StrCpy $archievename "simupak64-111-3.zip"
  StrCpy $downloadname "pak"
  StrCpy $VersionString "pak64 111.3 r892"
  Call DownloadInstallZip
SectionEnd


Section /o "pak64 Food addon"
  AddSize 280
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64/111-1/simupak64-food-111-1.zip"
  StrCpy $archievename "simupak64-addon-food-111-1.zip"
  StrCpy $downloadname "pak"
  StrCpy $VersionString ""
  StrCmp $multiuserinstall "1" +3
  ; no multiuser => install in normal directory
  Call DownloadInstallZip
  goto +2
  Call DownloadInstallAddonZip
SectionEnd

SectionGroupEnd



SectionGroup "Pak64.german: main and addons" pak64germangroup

Section /o "pak64.german (Freeware) for 110.0c" pak64german
  AddSize 14971
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak.german/pak64.german-110-0c/simupak-german64-110-0c.zip"
  StrCpy $archievename "simupak-german64-110-0c.zip"
  StrCpy $downloadname "pak64.german"
  StrCpy $VersionString "pak64.german 0.110.0b"
  Call DownloadInstallZip
SectionEnd

Section /o "pak64.german full industries"
  AddSize 2873
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak.german/pak64.german-110-0c/simupak-german64-industry-110-0.zip"
  StrCpy $archievename "simupak-german64-industry-110-0.zip"
  StrCpy $downloadname "pak64.german"
  StrCpy $VersionString ""
  StrCmp $multiuserinstall "1" +3
  ; no multiuser => install in normal directory
  Call DownloadInstallZip
  goto +2
  Call DownloadInstallAddonZip
SectionEnd

Section /o "pak64.german Tourist addon"
  AddSize 424
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak.german/pak64.german-110-0c/simutrans-german64-addons-110-0.zip"
  StrCpy $archievename "simutrans-german64-addons-110-0.zip"
  StrCpy $downloadname "pak64.german"
  StrCpy $VersionString ""
  StrCmp $multiuserinstall "1" +3
  ; no multiuser => install in normal directory
  Call DownloadInstallZip
  goto +2
  Call DownloadInstallAddonZip
SectionEnd

SectionGroupEnd




Section /o "pak.japan (64 size) for 111.3" pak64japan
  AddSize 6709
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64.japan/111-3/simupak64.japan-111-3.zip"
  StrCpy $archievename "simupak64.japan-111-3.zip"
  StrCpy $downloadname "pak.japan"
  StrCpy $VersionString "pak64.japan 111.3 r892"
  Call DownloadInstallZip
SectionEnd



; name does not match folder name (pak64.ho-scale) but otherwise always updated
Section /o "pak64 HO-scale (GPL)" pak64HO
  AddSize 8527
  StrCpy $downloadlink "http://simutrans.bilkinfo.de/pak64.ho-scale-latest.tar.gz"
  StrCpy $archievename "pak64.ho-scale-latest.tar.gz"
  StrCpy $downloadname "pak64.HO"
  StrCpy $VersionString ""
  Call DownloadInstallTgzWithoutSimutrans
SectionEnd


; name does not match folder name (pakHajo) but not update expected anyway ...
Section /o "pak64.HAJO (Freeware) for 102.2.2" pak64HAJO
  AddSize 6376
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pakHAJO/pakHAJO_102-2-2/pakHAJO_0-102-2-2.zip"
  StrCpy $archievename "pakHAJO_0-102-2-2.zip"
  StrCpy $downloadname "pak64.HAJO"
  StrCpy $VersionString ""
  Call DownloadInstallZip
SectionEnd



Section /o "pak64.contrast (GPL) for 102.2.2" pak64contrast
   AddSize 1367
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64.contrast/pak64.Contrast_910.zip"
  StrCpy $archievename "pak64.Contrast_910.zip"
  StrCpy $downloadname "pak64.contrast"
  StrCpy $VersionString ""
  Call DownloadInstallZipWithoutSimutrans
SectionEnd



; name does not match folder name (pak96.comic) but otherwise always selected for update
Section /o "pak96 Comic (Freeware) V0.4.10 expansion" pak96comic
  AddSize 32304
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak96.comic/pak96.comic%20for%20111-3/pak96.comic-0.4.10-plus.zip"
  StrCpy $archievename "pak96.comic-0.4.10-plus.zip"
  StrCpy $downloadname "pak96.Comic"
  StrCpy $VersionString "pak96.comic V4.1 plus"
  Call DownloadInstallZip
SectionEnd



; name does not match folder name (pakHD) but not update expected anyway ...
Section /o "pak96.HD (96 size) V0.4 for 102.2.2" pak96HD
  AddSize 26189
  StrCpy $downloadlink "http://hd.simutrans.com/release/PakHD_v04B_100-0.zip"
  StrCpy $archievename "PakHD_v04B_100-0.zip"
  StrCpy $downloadname "pak96.HD"
  StrCpy $VersionString "Martin"
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



Section /o "pak128 V2.1.0" pak128
  AddSize 340600
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak128/pak128%20for%20111-2/pak128-2.1.0--111.2.zip"
  StrCpy $archievename "pak128-2.1.0--111.2.zip"
  StrCpy $downloadname "pak128"
  StrCpy $VersionString "pak128 2.1.0"
  Call DownloadInstallZip
SectionEnd



Section /o "pak128.Britain.111 V1.11 for 111.0" pak128britain
  AddSize 170376
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak128.britain/pak128.Britain%20for%20111-0/pak128.Britain.1.11-111-2.zip"
  StrCpy $archievename "pak128.Britain.1.11-111-2.zip"
  StrCpy $downloadname "pak128.Britain.111"
  StrCpy $VersionString "pak128.Britain 1.11 Simutrans 111.2"
  Call DownloadInstallZipWithoutSimutrans
SectionEnd



Section /o "pak128.German V0.4 for 111.3" pak128german
  AddSize 58309
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/PAK128.german/PAK128.german_0.4_111.3/PAK128.german_0.4_111.3.zip"
  StrCpy $archievename "PAK128.german_0.4_111.3.zip"
  StrCpy $downloadname "pak128.German"
  StrCpy $VersionString "  PAK128.german V 0.4 (Rev. 1201)"
  Call DownloadInstallZip
SectionEnd


; name does not match folder name (pak128.japan) but otherwise always selected for update
Section /o "pak128-Japan for 110.0.1" pak128japan
  AddSize 17555
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak128.japan/for%20Simutrans%20110.0.1/pak128.japan-110.0.1-version16-08-2011.zip"
  StrCpy $archievename "pak128.japan-110.0.1-version16-08-2011.zip"
  StrCpy $downloadname "pak128.Japan"
  StrCpy $VersionString ""
  Call DownloadInstallZipWithoutSimutrans
SectionEnd


; name does not match folder name (pak192 japan) but otherwise always selected for update
Section /o "pak192 Comic (Freeware) for 102.2.1" pak192comic
  AddSize 23893
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak192.comic/pak192.comic_102-2-1/pak192.comic_0-1-9-1_102-2-1.zip"
  StrCpy $archievename "pak192.comic_0-1-9-1_102-2-1.zip"
  StrCpy $downloadname "pak192.Comic"
  StrCpy $VersionString ""
  Call DownloadInstallZip
SectionEnd



Section /o "pak48.excentrique V0.18" pak48excentrique
  AddSize 1385
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/ironsimu/pak48.Excentrique/v018/pak48-excentrique_v018.zip"
  StrCpy $archievename "pak48-excentrique_v018.zip"
  StrCpy $downloadname "pak48.Excentrique"
  StrCpy $VersionString "pak48.Excentrique v0.18"
  Call DownloadInstallZipWithoutSimutrans
SectionEnd



; name does not match folder name (pak32) but otherwise always selected for update
Section /o "pak32.Comic (alpha) for 102.2.1" pak32comic
  AddSize 2108
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak32.comic/pak32.comic%20for%20102-0/pak32.comic_102-0.zip"
  StrCpy $archievename "pak32.comic_102-0.zip"
  StrCpy $downloadname "pak32.Comic"
  StrCpy $VersionString ""
  Call DownloadInstallZip
SectionEnd
