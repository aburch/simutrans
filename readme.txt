About
=====

Simutrans is a freeware and open-source transportation simulator. Your goal is to establish a successful transport company. Transport passengers, mail and goods by rail, road, ship, and even air. Interconnect districts, cities, public buildings, industries and tourist attractions by building a transport network you always dreamed of.

You can download Simutrans from:

- Simutrans Website:   https://www.simutrans.com/en/
- Steam:               https://store.steampowered.com/app/434520/Simutrans/

Check the forum or wiki if you need help:

- International Forum: https://forum.simutrans.com/
- Simutrans Wiki:      https://wiki.simutrans.com/


Compiling Simutrans
===================

This is a short guide on compiling simutrans. If you want more detailed information, read the Compiling Simutrans forum thread https://simutrans-germany.com/wiki/wiki/en_CompilingSimutrans

If you are on Windows, download either MSVC or MinGW. MSVC is easy for debugging, MSYS2 is easy to set up (but it has to be done on the command line).
- MSVC:  https://visualstudio.microsoft.com/
- MSYS2: https://www.msys2.org/ 

Getting the libraries
------------------------

You will need pkgconfig (Unix) or vcpkg (Microsoft Visual C++) https://github.com/Microsoft/vcpkg

- Needed (All): libpng2 libbzip2 zlib 
- Needed (Linux/Mac): libSDL2 libfluidsynth (for midi music)
- Optional but recommended: libzstd (faster compression) libfreetype (TrueType font support) miniupnpc (for easy server setup)

- MSVC: Copy install-building-libs-{architecture}.bat to the vcpkg folder and run it.
- MSYS2: Run setup-mingw.sh to get the libraries and set up the environment.
- Linux: Use https://pkgs.org/ to search for development libraries available in your package manager.
- Mac: Install libraries via Homebrew: https://brew.sh/

Compiling
---------

Go to the source code directory of simutrans (simutrans/trunk if you downloaded from svn). You have three build systems to choose from: make, MSVC, and CMake. We recommend make or MSVC for debug builds.

Compiling will give you only the executable, you still need a Simutrans installation to run the program. You can start simutrans with "-use_workdir" to point it to an existing installation.

1) make

(MacOS) autoreconf -ivf
(Linux) autoconf
./configure
make -j 4
(MacOS) make OSX/getversion

2) Microsoft Visual C++

Open the simutrans.sln, select the target (default is GDI) and build.

3) CMake

mkdir build && cd build
(Unix) cmake -G "Insert Correct Makefiles Generator" ..
(Unix) cmake build . -j 4
(MSVC) cmake.exe .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
(MSVC) cmake.exe --build . --config Release

See here a list of generators: https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html


Cross-Compiling
===============
If you want to cross-compile Simutrans from Linux for Windows, see the Cross-Compiling Simutrans wiki page https://simutrans-germany.com/wiki/wiki/en_Cross-Compiling_Simutrans


Contribute
==========

You cand find general information about contributing to Simutrans in the Development Index of the wiki: https://simutrans-germany.com/wiki/wiki/en_Devel_Index?structure=en_Devel_Index


Reporting bugs
==============

For bug reports use the Bug Reports Sub-Forum: https://forum.simutrans.com/index.php/board,8.0.html


License
=======

Simutrans is licensed under the Artistic License version 1.0. The Artistic License 1.0 is an OSI-approved license which allows for use, distribution, modification, and distribution of modified versions, under the terms of the Artistic License 1.0. For the complete license text see LICENSE.txt.

Simutrans paksets (which are necessary to run the game) have their own license, but no one is included alongside this code.


Credits
=======

Simutrans was originally written by Hansjörg Malthaner "Hajo" from 1997 until he retired from development around 2004. Since then a team of contributors (The Simutrans Team) lead by Markus Pristovsek "Prissi" develop Simutrans. 

A list of early contributors can be found in simutrans/thanks.txt
