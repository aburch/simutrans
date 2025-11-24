#!/bin/bash

git clone https://github.com/simutrans/simutrans-android-project
cd simutrans-android-project
git submodule init
git submodule update --recursive --depth 1

cd simutrans/jni
mkdir simutrans
svn checkout svn://servers.simutrans.org/simutrans/trunk simutrans

./build_libraries.sh

# Fluidsynth is a PITA to build; using the prebuilt release instead
wget https://github.com/FluidSynth/fluidsynth/releases/download/v2.5.1/fluidsynth-2.5.1-android24.zip
unzip fluidsynth-*.zip -d fluidsynth

./simutrans/src/android/AndroidPreBuild.sh
