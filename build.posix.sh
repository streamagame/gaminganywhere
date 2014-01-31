#!/bin/bash

pushd `dirname $0` > /dev/null
GAPATH=`pwd`
popd > /dev/null

export GADEPS=$GAPATH/deps.posix
export PKG_CONFIG_PATH=$GADEPS/lib/pkgconfig:/opt/local/lib/pkgconfig:/usr/lib/i386-linux-gnu/pkgconfig/:/usr/lib/pkgconfig:/usr/local/lib/pkgconfig
export PATH=$GADEPS/bin:$PATH
export EXTRACFLAGS="-I/usr/local/include" # Fix needed to compile on OS X Mavericks, because of incorrect SDL pkgconfig in homebrew

cd $GAPATH

# Build the dependencies
#if [ ! -d "deps.posix" ]; then
	echo "*** Building dependencies ***"
	cd deps.src/
	make || exit 1;
	cd ../
#fi

# Build GamingAnywhere
echo "*** Building GamingAnywhere ***"
cd ga/
make all || exit 1;
make install || exit 1;
cd ../

echo "*** Done. ***"
