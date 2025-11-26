#!/bin/bash

cd simutrans-android-project/simutrans/jni

# Assets
cp -rf simutrans/simutrans/. ../src/main/assets

# Revision
cd simutrans/tools
REVISION=$(./get_revision.sh)
sed -i "s/versionCode [0-9]\+/versionCode $REVISION/" ../../../build.gradle

cd ../../../..
VERSION=`sed -n 's/#define SIM_VERSION_MAJOR *\([0-9]*\)$/\1/ p' <simutrans/jni/simutrans/src/simutrans/simversion.h`.`sed -n 's/#define SIM_VERSION_MINOR *\([0-9]*\)$/\1/ p' <simutrans/jni/simutrans/src/simutrans/simversion.h`.`sed -n 's/#define SIM_VERSION_PATCH *\([0-9]*\)$/\1/ p' <simutrans/jni/simutrans/src/simutrans/simversion.h`
NIGHTLY=`sed -n 's/#define SIM_VERSION_BUILD SIM_BUILD_NIGHTLY/ Nightly/ p' <simutrans/jni/simutrans/src/simutrans/simversion.h``sed -n 's/#define SIM_VERSION_BUILD SIM_BUILD_RELEASE_CANDIDATE/ Release candidate/ p' <simutrans/jni/simutrans/src/simutrans/simversion.h`
sed -i 's/versionName.*$/versionName "'"$VERSION$NIGHTLY"'"/' simutrans/build.gradle

echo "Build Simutrans $VERSION$NIGHTLY r$REVISION"
echo "$PWD"

# Build Android project
cp -r simutrans/jni/SDL/android-project/app/src/main/java simutrans/src/main

#cat simutrans/build.gradle
./gradlew assembleRelease
./gradlew bundleRelease

cp simutrans/build/outputs/bundle/release/simutrans-release.aab ./test-simutrans.aab
#jarsigner -verbose -sigalg SHA256withRSA -digestalg SHA-256 -keystore $SIGNING_KEYSTORE test-simutrans.aab $SIGNING_KEY_ALIAS -storepass $SIGNING_STORE_PASSWORD 
java -jar /opt/android-sdk-linux/cmdline-tools/latest/bin/bundletool-all-1.18.2.jar build-apks --bundle=test-simutrans.aab --output=temp.apks --mode=universal
unzip temp.apks
rm -rf temp.apks toc.pb
mv universal.apk simutrans-nightly.apk


