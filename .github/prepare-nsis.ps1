# downloads and unpacks the unicode plugin needed for NSIS installer of paksetdownloader

Invoke-WebRequest -Uri "https://nsis.sourceforge.io/mediawiki/images/5/5a/NSISunzU.zip" -OutFile "NSISunzU.zip"
Invoke-WebRequest -Uri "https://nsis.sourceforge.io/mediawiki/images/c/c9/Inetc.zip" -OutFile "Inetc.zip"
Invoke-WebRequest -Uri "https://nsis.sourceforge.io/mediawiki/images/c/c7/CabX.zip" -OutFile "CabX.zip"
Invoke-WebRequest -Uri "https://nsis.sourceforge.io/mediawiki/images/9/9d/Untgz.zip" -OutFile "Untgz.zip"
Invoke-WebRequest -Uri "https://nsis.sourceforge.io/mediawiki/images/6/6c/Shelllink.zip" -OutFile "Shelllink.zip"
    
expand-archive -path "CabX.zip" -DestinationPath "NSIS_Plugins" -Force
expand-archive -path "Inetc.zip" -DestinationPath "NSIS_Plugins" -Force
expand-archive -path "NSISunzU.zip" -DestinationPath "dummy" -Force
move ".\dummy\NSISunzU\Plugin unicode\nsisunz.dll" .\NSIS_Plugins\plugins\x86-unicode\
expand-archive -path "Untgz.zip" -DestinationPath "dummy" -Force
move .\dummy\Untgz\unicode\untgz.dll .\NSIS_Plugins\plugins\x86-unicode\
expand-archive -path "Shelllink.zip" -DestinationPath "dummy" -Force
move .\dummy\Unicode\Plugins\ShellLink.dll .\NSIS_Plugins\plugins\x86-unicode\
Remove-Item 'dummy' -Recurse
del *.zip
