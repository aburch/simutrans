$env:Path += ";C:\msys64\usr\bin;C:\msys64\mingw32\bin"
Set-Location C:\Users\catro\Documents\Git\Github\simutrans
make -j5
# Move-Item .\build\default\sim.exe ..\KU-TA_simutrans -Force