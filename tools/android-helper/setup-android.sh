#!/bin/bash

sudo apt-get update
sudo dpkg --add-architecture i386
sudo apt-get -yqq update
sudo apt-get -ym install curl expect git libc6:i386 libgcc1:i386 libncurses6:i386 libstdc++6:i386 zlib1g:i386 openjdk-21-jdk wget unzip vim make subversion zip python3

sudo mkdir -p /opt/android-sdk-linux
sudo chown -R $USER /opt/android-sdk-linux

cd /opt/android-sdk-linux
wget https://dl.google.com/android/repository/commandlinetools-linux-13114758_latest.zip
unzip commandlinetools-linux-13114758_latest.zip
rm commandlinetools-linux-13114758_latest.zip
mv cmdline-tools latest
mkdir cmdline-tools
mv latest cmdline-tools/latest

echo "export ANDROID_HOME=/opt/android-sdk-linux" >> ~/.profile
echo "export ANDROID_SDK_HOME=/opt/android-sdk-linux" >> ~/.profile
echo "export ANDROID_SDK_ROOT=/opt/android-sdk-linux" >> ~/.profile
echo "export ANDROID_SDK=/opt/android-sdk-linux" >> ~/.profile
echo "export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64" >> ~/.profile
echo "export ANDROID_NDK=/opt/android-sdk-linux/ndk/29.0.14206865" >> ~/.profile

echo 'alias bundletool="java -jar /opt/android-sdk-linux/cmdline-tools/latest/bin/bundletool-all-1.18.2.jar"' >> ~/.profile

echo 'PATH=$PATH:/opt/android-sdk-linux/cmdline-tools/latest/bin:/opt/android-sdk-linux/platform-tools:/opt/android-sdk-linux/build-tools/35.0.0:/opt/android-sdk-linux/cmake/3.31.6/bin:/opt/android-sdk-linux/ndk/29.0.14206865:/opt/android-sdk-linux/ndk/29.0.14206865/toolchains/llvm/prebuilt/linux-x86_64/bin/' >> ~/.profile

export ANDROID_HOME=/opt/android-sdk-linux
export ANDROID_SDK_HOME=/opt/android-sdk-linux
export ANDROID_SDK_ROOT=/opt/android-sdk-linux
export ANDROID_SDK=/opt/android-sdk-linux
export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64
export ANDROID_NDK=/opt/android-sdk-linux/ndk/29.0.14206865/

alias bundletool="java -jar /opt/android-sdk-linux/cmdline-tools/latest/bin/bundletool-all-1.18.2.jar"

PATH=$PATH:/opt/android-sdk-linux/cmdline-tools/latest/bin:/opt/android-sdk-linux/platform-tools:/opt/android-sdk-linux/build-tools/35.0.0:/opt/android-sdk-linux/cmake/3.31.6/bin:/opt/android-sdk-linux/ndk/29.0.14206865:/opt/android-sdk-linux/ndk/29.0.14206865/toolchains/llvm/prebuilt/linux-x86_64/bin/

cd /opt/android-sdk-linux
wget https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip
unzip commandlinetools-linux-11076708_latest.zip
rm commandlinetools-linux-11076708_latest.zip
mv cmdline-tools latest
mkdir cmdline-tools
mv latest cmdline-tools/latest
yes | cmdline-tools/latest/bin/sdkmanager --licenses
yes | cmdline-tools/latest/bin/sdkmanager --install "platform-tools"
yes | cmdline-tools/latest/bin/sdkmanager --install "build-tools;35.0.0"
yes | cmdline-tools/latest/bin/sdkmanager --install "cmake;3.31.6"
yes | cmdline-tools/latest/bin/sdkmanager --install "ndk;29.0.14206865"
ln -s llvm-objdump /opt/android-sdk-linux/ndk/29.0.14206865/toolchains/llvm/prebuilt/linux-x86_64/bin/objdump

cd cmdline-tools/latest/bin
wget https://github.com/google/bundletool/releases/download/1.18.2/bundletool-all-1.18.2.jar
chmod 775 bundletool-all-1.18.2.jar

mkdir ~/.android/
keytool -genkey -v -keystore ~/.android/debug.keystore -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 -keypass android -storepass android -dname "cn=example.com,ou=exampleou,dc=example,dc=com"

export SIGNING_KEYSTORE=~/.android/debug.keystore
export SIGNING_STORE_PASSWORD=android
export SIGNING_KEY_PASSWORD=android
export SIGNING_KEY_ALIAS=androiddebugkey
echo "export SIGNING_KEYSTORE=~/.android/debug.keystore" >> ~/.profile
echo "export SIGNING_STORE_PASSWORD=android" >> ~/.profile
echo "export SIGNING_KEY_PASSWORD=android" >> ~/.profile
echo "export SIGNING_KEY_ALIAS=androiddebugkey" >> ~/.profile

echo "**************************************************"
echo "** Please logout before run the next setup script!"
echo "**************************************************"
