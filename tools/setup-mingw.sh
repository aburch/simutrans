echo "First we update the system"
pacman -Syu
pacman -Syu

echo "Now installing dependencies"

# neccessary packets
pacman -S --noconfirm make autoconf pkg-config subversion zip unzip zstd
pacman -S --noconfirm $MINGW_PACKAGE_PREFIX-gcc $MINGW_PACKAGE_PREFIX-bzip2 $MINGW_PACKAGE_PREFIX-freetype $MINGW_PACKAGE_PREFIX-libpng $MINGW_PACKAGE_PREFIX-brotli $MINGW_PACKAGE_PREFIX-cmake

# optional for SLD2 builds:
pacman -S --noconfirm $MINGW_PACKAGE_PREFIX-SDL2

# optional for installer:
pacman -S --noconfirm $MINGW_PACKAGE_PREFIX-nsis

rm -rf master.zip
wget https://github.com/miniupnp/miniupnp/archive/master.zip
unzip -o master.zip
cd miniupnp-master/miniupnpc
cat Makefile.mingw | sed 's|[ \t]wingenminiupnpcstrings.exe |'"$(printf '\t')"'./wingenminiupnpcstrings.exe |' >Makefile.mingw2
make -f Makefile.mingw2
cp libminiupnpc.a $MINGW_PREFIX/lib
cp *.h $MINGW_PREFIX/lib
mkdir $MINGW_PREFIX/include/miniupnpc
cp *.h $MINGW_PREFIX/include/miniupnpc

cd ../..
rm -rf master.zip
rm -rf miniupnpc-master

# libbrotli static is broken but needed for freetype
for f in libbrotlidec libbrotlienc libbrotlicommon; do
	mv "$MINGW_PREFIX/lib/$f-static.a" "$MINGW_PREFIX/lib/$f.a"
done
