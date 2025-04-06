How to build simutrans for Android

You need either linux or the linux subsystem for windows. (It might work for MacOS too?)

Create a folder and put the three script into this folder.

Start ./setup-android.sh and log off (at least on WSL).
Next ./setup-simutrans-android.sh

These have only to run once. Finally, run build-only.sh, all from the same folder.

The code is under simutrans-android-project/simutrans/jni/simutrans in an svn repository (so it does not interfere with the git of the simutrans-android-project around it.

