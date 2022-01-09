#!/bin/sh
# Set path to your Android keystore and your keystore alias here, or put them in your environment
PASS=
[ -n "$ANDROID_UPLOAD_KEYSTORE_PASS" ] && PASS="-storepass:env ANDROID_UPLOAD_KEYSTORE_PASS"
[ -n "$ANDROID_UPLOAD_KEYSTORE_PASS_FILE" ] && PASS="-storepass:file $ANDROID_UPLOAD_KEYSTORE_PASS_FILE"

APPNAME=`grep AppName AndroidAppSettings.cfg | sed 's/.*=//' | tr -d '"' | tr " '/" '---'`
APPVER=`grep AppVersionName AndroidAppSettings.cfg | sed 's/.*=//' | tr -d '"' | tr " '/" '---'`

# Fix Gradle compilation error
[ -z "$ANDROID_NDK_HOME" ] && export ANDROID_NDK_HOME="`which ndk-build | sed 's@/ndk-build@@'`"

cd project

./gradlew bundleRelease || exit 1

../copyAssets.sh pack-binaries-bundle app/build/outputs/bundle/release/app-release.aab

cd app/build/outputs/bundle/release || exit 1

wget https://github.com/google/bundletool/releases/download/1.8.2/bundletool-all-1.8.2.jar
mv bundletool-all-1.8.2.jar bundletool.jar

java -jar bundletool.jar build-apks --bundle=app-release.aab --output=simutrans-multi.apks --mode=universal
# -ks=keystore.jks --ks-pass=file:keystore.pwd --ks-key-alias=MyKeyAlias --key-pass=file:key.pwd

unzip -p simutrans-multi.apks universal.apk > simuandroid-multi-nightly.apk
mv app-release.aab simutrans-nightly.aab

exit 0

# Remove old certificate
cp -f app-release.aab simutrans.aab || exit 1

# Sign with the new certificate
echo Using keystore $ANDROID_UPLOAD_KEYSTORE_FILE and alias $ANDROID_UPLOAD_KEYSTORE_ALIAS
stty -echo
jarsigner -verbose -sigalg SHA256withRSA -digestalg SHA-256 -keystore $ANDROID_UPLOAD_KEYSTORE_FILE $PASS simutrans-nightly.aab $ANDROID_UPLOAD_KEYSTORE_ALIAS || exit 1
stty echo
echo
