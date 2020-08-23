# downloads and unpacks the unicode plugin needed for NSIS installer of paksetdownloader

Invoke-WebRequest -Uri "https://nsis.sourceforge.io/mediawiki/images/5/5a/NSISunzU.zip" -OutFile "NSISunzU.zip"
Invoke-WebRequest -Uri "https://nsis.sourceforge.io/mediawiki/images/c/c9/Inetc.zip" -OutFile "Inetc.zip"
Invoke-WebRequest -Uri "https://nsis.sourceforge.io/mediawiki/images/c/c7/CabX.zip" -OutFile "CabX.zip"
Invoke-WebRequest -Uri "https://nsis.sourceforge.io/mediawiki/images/9/9d/Untgz.zip" -OutFile "Untgz.zip"
Invoke-WebRequest -Uri "https://nsis.sourceforge.io/mediawiki/images/6/6c/Shelllink.zip" -OutFile "Shelllink.zip"
    
7z e -aoa -o"NSIS_Plugins/plugins/x86-unicode" "NSISunzU.zip" -ir!"nsisunz.dll"
7z x -aoa -o"NSIS_Plugins" "Inetc.zip"
7z x -aoa -o"NSIS_Plugins" "CabX.zip"
7z x -aoa -o"NSIS_Plugins" "Untgz.zip"
move .\NSIS_Plugins\Untgz\unicode\untgz.dll .\NSIS_Plugins\plugins\x86-unicode\
7z x -aoa -o"NSIS_Plugins" "Shelllink.zip"
move .\NSIS_Plugins\Unicode\Plugins\ShellLink.dll .\NSIS_Plugins\plugins\x86-unicode\
del *.zip
