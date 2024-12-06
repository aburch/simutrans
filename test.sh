#!/bin/bash
make

if [ $? -eq 0 ]; then
    echo "testing"
    runas /user:admin /savecred cp -rf sim-OTRP.exe /c/Program\ Files\ \(x86\)/Simutrans
    ./c/Program\ Files\ \(x86\)/Simutrans/sim-OTRP.exe
else
    echo "ERROR"
fi