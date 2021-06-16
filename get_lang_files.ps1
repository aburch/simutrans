# Downloade translations after post request and extract them
Write-Output "Downloading and installing languag files"
(New-Object System.Net.WebClient).UploadString("https://translator.simutrans.com/script/main.php?page=wrap", "version=0&choice=all&submit=Export%21")
(New-Object System.Net.WebClient).DownloadFile("https://translator.simutrans.com/data/tab/language_pack-Base+texts.zip", "temp.zip")
Remove-Item "simutrans/text" -Recurse
expand-archive -path "temp.zip" -DestinationPath "simutrans/text"
Remove-Item "temp.zip"
