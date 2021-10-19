# Downloades a simple zip file and unzips it. Assumes everything is according to Simutrans norm.
Write-Output "Downloading and installing "$($args[0])
(New-Object System.Net.WebClient).DownloadFile($args[0], "temp.zip")
expand-archive -path "temp.zip" -DestinationPath ".." -Force
del "temp.zip"
