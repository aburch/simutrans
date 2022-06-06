#!/bin/sh
# Set path to your Android keystore and your keystore alias here, or put them in your environment


APPNAME=`grep AppName AndroidAppSettings.cfg | sed 's/.*=//' | tr -d '"' | tr " '/" '---'`
APPVER=`grep AppVersionName AndroidAppSettings.cfg | sed 's/.*=//' | tr -d '"' | tr " '/" '---'`

# Fix Gradle compilation error
[ -z "$ANDROID_NDK_HOME" ] && export ANDROID_NDK_HOME="`which ndk-build | sed 's@/ndk-build@@'`"

cd project

./gradlew bundleRelease || exit 1

../copyAssets.sh pack-binaries-bundle app/build/outputs/bundle/release/app-release.aab

cd app/build/outputs/bundle/release || exit 1

#[[ ! -z "$STORE_PASS" ]] && 
jarsigner -verbose -sigalg SHA256withRSA -digestalg SHA-256 -keystore /android-sdl/project/jni/application/simutrans/simutrans/src/android/Simutrans-upload.keystore --storepass $STORE_PASS app-release.aab $STORE_ALIAS

wget https://github.com/google/bundletool/releases/download/1.8.2/bundletool-all-1.8.2.jar
mv bundletool-all-1.8.2.jar bundletool.jar

if [ -z "$STORE_PASS" ]
then
# no key defined
java -jar bundletool.jar build-apks --bundle=app-release.aab --output=simutrans-multi.apks --mode=universal
else
java -jar bundletool.jar build-apks --bundle=app-release.aab --output=simutrans-multi.apks --mode=universal --ks=/android-sdl/project/jni/application/simutrans/simutrans/src/android/Simutrans-upload.keystore --ks-pass=pass:$STORE_PASS --ks-key-alias=$STORE_ALIAS
fi

unzip -p simutrans-multi.apks universal.apk > simuandroid-multi-nightly.apk
mv app-release.aab simutrans-nightly.aab

# prepare for Play Store upload to beta track
cp simutrans-nightly.aab simutrans-nightly-r`cd /android-sdl/project/jni/application/simutrans/simutrans/;tools/get_revision.sh`.aab

mkdir whatsNewDirectory
echo `git log --pretty=format:'%s' -1` > whatsNewDirectory/whatsnew-en-US

