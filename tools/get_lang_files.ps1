# Downloade translations after post request and extract them

$translator_url = "https://makie.de/translator"
<# not sure why the second does not work ...
$translator_url = "https://translator.simutrans.com"

# Stackoverflow: First we create the request and get the status code
$HTTP_Request = [System.Net.WebRequest]::Create("$translator_url")
$HTTP_Response = $HTTP_Request.GetResponse()
$HTTP_Status = [int]$HTTP_Response.StatusCode
If ($HTTP_Status -ne 200) {
	$translator_url = "https://makie.de/translator"
}
#>
Write-Output "Downloading and installing languag files from $translator_url"

# now download
$dummy=(New-Object System.Net.WebClient).UploadString("$translator_url/script/main.php?page=wrap", "version=0&choice=all&submit=Export!")
Invoke-WebRequest -Uri "$translator_url/data/tab/language_pack-Base+texts.zip" -OutFile "temp.zip"
#(New-Object System.Net.WebClient).DownloadFile("$translator_url/data/tab/language_pack-Base+texts.zip", ".")
Remove-Item -ErrorAction Ignore "simutrans/text" -Recurse ; $null
expand-archive -path "temp.zip" -DestinationPath "simutrans/text"
Remove-Item "temp.zip"
