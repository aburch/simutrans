# Simutrans

- 1.0) [About](#1-about)
	- 1.1) [Download Simutrans](#11-download-simutrans)
	- 1.2) [Helpful links](#12-helpful-links)
- 2.0) [Compiling Simutrans](#2-compiling-simutrans)
	- 2.1) [Getting the Source Code](#21-getting-the-source-code)
	- 2.2) [Getting the libraries](#22-getting-the-libraries)
	- 2.3) [Compiling](#23-compiling)
		- 2.3.1) [Compiling with make](#232-compiling-with-make)
		- 2.3.2) [Compiling with MSVC](#233-compiling-with-microsoft-visual-studio)
		- 2.3.3) [Compiling with CMake](#231-compiling-with-cmake)
	- 2.4) [Cross-Compiling](#24-Cross-Compiling)
- 3.0) [Contribute](#3-contribute)
	- 3.1) [Coding](#31-coding)
	- 3.2) [Getting the libraries](#32-translating)
	- 3.3) [Painting](#33-painting)
	- 3.4) [Reporting-bugs](#34-reporting-bugs)
- 4.0) [License](#4-license)
- 5.0) [Credits](#5-credits)

## 1) About

Simutrans is a freeware and open-source transportation simulator. Your goal is to establish a successful transport company. Transport passengers, mail and goods by rail, road, ship, and even air. Interconnect districts, cities, public buildings, industries and tourist attractions by building a transport network you always dreamed of.

## 1.1) Download Simutrans

You can download Simutrans from:

- [Simutrans Official Website](https://www.simutrans.com/en/)
- [Steam](https://store.steampowered.com/app/434520/Simutrans/)
- [Linux package managers](https://pkgs.org/search/?q=simutrans)

There is a "nightly" version available, but most most people should use the last "stable" release.

## 1.2) Helpful links

### Main sites

- [Main website](https://www.simutrans.com/)
- [International Forum](https://forum.simutrans.com/)
- [German Forumurl](https://www.simutrans-forum.de/)
- [Japanese Forum](https://forum.japanese.simutrans.com/)

### Help

- [Simutrans Wiki](https://wiki.simutrans.com)
- [Starter Guide (PDF)](http://simutrans.igoreliezer.com/docs/Simutrans%20Starter%20Guide.pdf)
- [Help Center (International Forum)](https://forum.simutrans.com/index.php/board,7.0.html)

### Additional downloads

- [Paksets](https://www.simutrans.com/en/paksets/)
- [Addons](https://simutrans-germany.com/wiki/wiki/en_Addons)

## 2) Compiling Simutrans

This is a short guide on compiling simutrans. If you want more detailed information, read the [Compiling Simutrans](https://simutrans-germany.com/wiki/wiki/en_CompilingSimutrans) wiki page.

If you are on Windows, download either [Microsoft Visual Studio](https://visualstudio.microsoft.com/) or [MSYS2](https://www.msys2.org/). MSVC is easy for debugging, MSYS2 is easy to set up (but it has to be done on the command line).

### 2.1) Getting the Source Code

You can download the latest version with a SVN client:
```
svn checkout svn://servers.simutrans.org/simutrans/trunk
```
If you prefer to use git, there is a mirror of the svn repository available at github:
```
git clone http://github.com/aburch/simutrans.git
```
Note that the svn repository is the main repository, and the git repository is just a mirror. If you use git instead of svn, you will need to set the game version manually for network play.

### 2.2) Getting the libraries

This is a list of libraries used by Simutrans. Not all of them are necessary, some are optional, so pick them according to your needs. Read below about how to install them.

| Library       | Website                             | Necessary? | Notes                                                                    |
|---------------|-------------------------------------|------------|---------------------------------------------------------------------------|
| zlib          | https://zlib.net/                   | Necessary  | Basic compression support                                                 |
| bzip2         | https://www.bzip.org/downloads.html | Necessary  | Alternative compression. You can pick this or zstd                         |
| libpng        | http://www.libpng.org/pub/png/      | Necessary  | Image manipulation                                                         |
| libSDL2       | http://www.libsdl.org/              | Necessary* | *On Linux & Mac. Optional but recommended for Windows. Graphics back-end |
| libzstd       | https://github.com/facebook/zstd    | Optional   | Alternative compression (larger save files than bzip2, but faster)         |
| libfreetype   | http://www.freetype.org/            | Optional   | TrueType font support                                                     |
| libminiupnpc  | http://miniupnp.free.fr/            | Optional   | Easy Server option                                                         |
| libfluidsynth | https://www.fluidsynth.org/         | Optional   | MIDI playback recommended on Linux & temporarily on Mac                   |
| libSDL2_mixer | http://www.libsdl.org/              | Optional   | Alternative MIDI playback and sound system                                 |

You will also need pkgconfig (Unix) or [vcpkg](https://github.com/Microsoft/vcpkg) (Microsoft Visual C++)

- MSVC: Copy install-building-libs-{architecture}.bat to the vcpkg folder and run it.
- MSYS2: Run [setup-mingw.sh](./setup-mingw.sh) to get the libraries and set up the environment.
- Linux: Use [pkgs.org](https://pkgs.org/) to search for development libraries available in your package manager.
- Mac: Install libraries via [Homebrew](https://brew.sh/).

### 2.3) Compiling

Go to the source code directory of simutrans (simutrans/trunk if you downloaded from svn). You have three build systems to choose from: make, MSVC, and CMake. We recommend make or MSVC for debug builds.

Compiling will give you only the executable, you still need a Simutrans installation to run the program. You can start simutrans with `-use_workdir` to point it to an existing installation.

#### 2.3.1) Compiling with make

The executable will be built in build/default.

```
(Linux) autoconf
(MacOS) autoreconf -ivf
./configure
make -j 4
(MacOS) make OSX/getversion
```

#### 2.3.2) Compiling with Microsoft Visual Studio

Simutrans solution is a single solution file [simutrans.sln](./simutrans.sln) with 4 projects:

- Simutrans-Main: The project that holds the shared, non back-end specific, files. All the followings use Main to build the specific back-end executables.
- Simutrans SDL2: Preferred back-end for Simutrans.
- Simutrans GDI: Windows-only back-end.
- Simutrans Server: Server back-end with no graphical interface.

#### 2.3.3) Compiling with CMake

The executable will be built in build/simutrans.

##### Linux/MinGW/MacOS
```
mkdir build && cd build
cmake -G "Insert Correct Makefiles Generator" ..
cmake --build . -j 4
```
See [here](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html) for a list of generators.

##### MSVC
```
mkdir build && cd build
cmake.exe .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

### 2.4) Cross-Compiling

If you want to cross-compile Simutrans from Linux for Windows, see the [Cross-Compiling Simutrans](https://simutrans-germany.com/wiki/wiki/en_Cross-Compiling_Simutrans) wiki page.

## 3) Contribute
You cand find general information about contributing to Simutrans in the [Development Index](https://simutrans-germany.com/wiki/wiki/en_Devel_Index?structure=en_Devel_Index) of the wiki.

### 3.1) Coding

- If you want to contribute, read the coding guidelines in simutrans/documentation/coding_styles.txt
- You definitely should check out the [Technical Documentation Sub-Forum](https://forum.simutrans.com/index.php/board,112.0.html) as well.
- **Do not open Pull Requests** in GitHub. Use the [Patches & Projects](https://forum.simutrans.com/index.php/board,33.0.html) Sub-Forum instead.

### 3.2) Translating

Simutrans is constantly updating and adding texts so we are always in need for translators:

- To help with translation use the [SimuTranslator](https://translator.simutrans.com/) web tool.
- To request a translator account use the [Translation Sub-Forum](https://forum.simutrans.com/index.php/board,47.0.html).

### 3.3) Painting

Simutrans is always looking for artists! If you want to paint graphics for Simutrans, check:

- The "Creating images" section of the [Development Index](https://simutrans-germany.com/wiki/wiki/en_Devel_Index).
- The [General Resources and Tools](https://forum.simutrans.com/index.php/board,108.0.html) Sub-Forum.

### 3.4) Reporting bugs

For bug reports use the [Bug Reports](https://forum.simutrans.com/index.php/board,8.0.html) Sub-Forum.

## 4) License

Simutrans is licensed under the Artistic License version 1.0. The Artistic License 1.0 is an OSI-approved license which allows for use, distribution, modification, and distribution of modified versions, under the terms of the Artistic License 1.0. For the complete license text see [LICENSE.txt](./LICENSE.txt).

Simutrans paksets (which are necessary to run the game) have their own license, but no one is included alongside this code.

## 5) Credits

Simutrans was originally written by Hansjörg Malthaner "Hajo" from 1997 until he retired from development around 2004. Since then a team of contributors (The Simutrans Team) lead by Markus Pristovsek "Prissi" develop Simutrans.

A list of early contributors can be found in [simutrans/thanks.txt](./simutrans/thanks.txt)
