#!/bin/bash

pushd `dirname $0` > /dev/null
GAPATH=`pwd`
popd > /dev/null

export KERNEL=`uname -s | tr A-Z a-z`
# Adapt to own file structure!
export ANDROID_HOME=$HOME/android/android-sdk-linux/.
export NDK_ROOT=$HOME/android/android-ndk-r8b/.

export GADEPS=$GAPATH/deps.android
export PKG_CONFIG_PATH=$GADEPS/lib/pkgconfig
export PATH=$GADEPS/bin:$ANDROID_HOME/tools:$ANDROID_HOME/platform-tools:$ANDROID_HOME/build-tools:$NDK_ROOT:$NDK_ROOT/toolchains/arm-linux-androideabi-4.8/prebuilt/$KERNEL"-x86_64"/bin:$PATH


cd $GAPATH

# Build the dependencies
#if [ ! -d "deps.android" ]; then
	echo "*** Building dependencies ***"
	cd deps.src/
	make -f Makefile.android|| exit 1;
	cd ../
#fi

# Build Android Environment
echo "*** Building Android Environment ***"

echo -n "ANDROID_HOME="
env

cd ga/android/
make || exit 1;
cd ../../

echo "*** Done. ***"
