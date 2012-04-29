; JUst the paksets

SectionGroup "Pak64: main and addons" pak64group

Section "!pak64 (standard)" pak64
  AddSize 10704
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64/111-2/simupak64-111-2.zip"
  StrCpy $archievename "simupak64-111-2.zip"
  StrCpy $downloadname "pak64"
  Call DownloadInstallZip
SectionEnd


Section /o "pak64 Food addon"
  AddSize 280
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64/111-1/simupak64-food-111-1.zip"
  StrCpy $archievename "simupak64-addon-food-111-1.zip"
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
  AddSize 2873
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak.german/pak64.german-110-0c/simupak-german64-industry-110-0.zip"
  StrCpy $archievename "simupak-german64-industry-110-0.zip"
  StrCpy $downloadname "pak64.german"
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



Section /o "pak128 2.1.0" pak128
  AddSize 340600
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak128/pak128%20for%20111-2/pak128-2.1.0--111.2.zip"
  StrCpy $archievename "pak128-2.1.0--111.2.zip"
  StrCpy $downloadname "pak128"
  Call DownloadInstallZip
SectionEnd



Section /o "pak128 Britain (1.11) 111.0" pak128britain
  AddSize 170376
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak128.britain/pak128.Britain%20for%20111-0/pak128.Britain.1.11-111-2.zip"
  StrCpy $archievename "pak128.Britain.1.11-111-2.zip"
  StrCpy $downloadname "pak128.Britain"
  Call DownloadInstallZipWithoutSimutrans
SectionEnd



Section /o "pak128 German (0.3) 111.2" pak128german
  AddSize 53397
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/PAK128.german/PAK128.german_111.2/PAK128.german_0.3_111.2.zip"
  StrCpy $archievename "PAK128.german_0.3_111.2.zip"
  StrCpy $downloadname "pak128.German"
  Call DownloadInstallZip
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



Section /o "pak48 excentrique 0.16" pak48excentrique
  AddSize 1544
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak48.Excentrique/v0.16-for-Simutrans-111.1/pak48_excentrique-v0_16.zip"
  StrCpy $archievename "pak48_excentrique-v0_16.zip"
  StrCpy $downloadname "pak48.Excentrique"
  Call DownloadInstallZipWithoutSimutrans
SectionEnd



Section /o "pak32 Comic (alpha) 102.2.1" pak32comic
  AddSize 2108
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak32.comic/pak32.comic%20for%20102-0/pak32.comic_102-0.zip"
  StrCpy $archievename "pak32.comic_102-0.zip"
  StrCpy $downloadname "pak32.Comic"
  Call DownloadInstallZip
SectionEnd
