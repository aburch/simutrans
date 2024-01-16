# downloads and unpacks the unicode plugin needed for NSIS installer of paksetdownloader

Write-Output "Downloading and installing NSIS"
Invoke-WebRequest -UserAgent "Wget" -Uri "https://sourceforge.net/projects/nsis/files/NSIS%203/3.09/nsis-3.09.zip/download" -OutFile "nsis.zip"
expand-archive -path "nsis.zip" -DestinationPath "." -Force

Invoke-WebRequest -Uri "https://nsis.sourceforge.io/mediawiki/images/5/5a/NSISunzU.zip" -OutFile "NSISunzU.zip"
Invoke-WebRequest -Uri "https://nsis.sourceforge.io/mediawiki/images/c/c9/Inetc.zip" -OutFile "Inetc.zip"
Invoke-WebRequest -Uri "https://nsis.sourceforge.io/mediawiki/images/c/c7/CabX.zip" -OutFile "CabX.zip"
Invoke-WebRequest -Uri "https://nsis.sourceforge.io/mediawiki/images/9/9d/Untgz.zip" -OutFile "Untgz.zip"
Invoke-WebRequest -Uri "https://nsis.sourceforge.io/mediawiki/images/6/6c/Shelllink.zip" -OutFile "Shelllink.zip"
    
expand-archive -path "CabX.zip" -DestinationPath "nsis-3.09" -Force
expand-archive -path "Inetc.zip" -DestinationPath "nsis-3.09" -Force
expand-archive -path "NSISunzU.zip" -DestinationPath "dummy" -Force
move ".\dummy\NSISunzU\Plugin unicode\nsisunz.dll" .\nsis-3.09\plugins\x86-unicode\
expand-archive -path "Untgz.zip" -DestinationPath "dummy" -Force
move .\dummy\Untgz\unicode\untgz.dll .\nsis-3.09\plugins\x86-unicode\
expand-archive -path "Shelllink.zip" -DestinationPath "dummy" -Force
move .\dummy\Unicode\Plugins\ShellLink.dll .\nsis-3.09\plugins\x86-unicode\
Remove-Item 'dummy' -Recurse
del *.zip

./nsis-3.09/makensis.exe ./src/Windows/nsis/onlineupgrade.nsi
