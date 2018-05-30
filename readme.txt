How to compile
--------------

Congratulations, you checked out the simutrans source. To compile it,
you have two options, either using Microsoft Visual C++ Express (which
is free in Version 7.0 or up) or GCC.

To compile you will need the following libraries:
libz (http://www.zlib.net/)
libpng (http://www.libpng.org/pub/png/) for makeobj
libbz2.lib (compile from source from http://www.bzip.org/downloads.html)
The following are also recommendend, but optional
libfreetype (http://www.freetype.org/)
libminiupnpc (http://miniupnp.free.fr/)

For the recommended SDL2-support you need
libSDL2 or libSDL (http://www.libsdl.org/)
libSDL_mixer (link from the same page)

The link for allegro lib is (but the allegro backend has not been 
tested for a long time):
http://www.talula.demon.co.uk/allegro/
or
http://alleg.sourceforge.net/index.de.html

To make life easier, you can follow the instructions to compile OpenTTD:
http://wiki.openttd.org/Category:Compiling_OpenTTD
A system set up for OpenTTD will also compile simutrans (except for
bzlib2, see below sections).

If you are on a MS Windows machine, download either MS VC Express or
MSYS2. MSVC is easy for debugging, MSYS2 is easy to set up (but it has to be done on the commandline).

The packages needed for MSYS2 are
make
mingw-w64-i686-gcc
mingw-w64-i686-SDL (Only if you want an SDL build OR for sound on SDL2)
mingw-w64-i686-SDL2 (Only if you want an SDL2 build)
mingw-w64-i686-freetype (for scaleable font support)
mingw-w64-i686-miniupnpc (for easy server option)
mingw-w64-i686-libpng (for makeobj)
mingw-w64-i686-pkg-config (for makeobj)

For all other systems, it is recommended you get latest GCC 3.46 or higher
and matching zlib, libbzip2, and libpng and SDL or SDL2 libbrary. For linux
systems you may have to use tools like apt-get, yast2, yum, ...

Typical package names are (ending may be also -devel)
libsdl2-dev or libsdl1.2-dev
zlib-dev
libpng-dev
libbz2-dev
libminiupnpc-dev
libfreetype-dev
Depending on your distribution, there may be also number needed, like 
libfreetype2-dev or libminiupnpc6-dev

To built on Haiku you must use GCC4 (type "setarch x86" in the current
nightlies). To incorporate bz2lib, download make bz2lib and add them
manually (via FLAGS = -I/dwonloadeddir -L/downloadeddir). However, simutrans
has a Haikuporter package, which may built the lastest version.

A subversion will be also a good idea. You can find some of them on:
http://subversion.tigris.org/
or you some other client.

Check out the latest source from the SVN or check out a certain revision.
I recommend always to use the latest source, since it does not make any
sense to work with buggy code.

The address is:
svn://server.simutrans.org/simutrans

A commandline would look like this:
svn checkout svn://server.simutrans.org/simutrans

If everything is set up, you can run configure inside trunk. This should 
create a config.default file with all the needed settings. Try to compile 
using make. You can manually fine edit config.default to enable other 
settings.

Typical you type into a command window:
./configure
make


IMPORTANT:
----------

If you want to contribute, read the coding guidelines in
trunk/coding_styles.txt


The following instructions are manual setup for GCC systems:
------------------------------------------------------------

Go to simutrans/trunk.

Then copy the file trunk/config.template to trunk/config.default and edit 
the file. You need to specify:
- frontend (gdi, allegro, sdl, SDL, posix)
- color depth (usually 16 or zero)
- system (you should know it)

I recommend to uncomment #DEBUG=1 and #OPTIMISE = 1 (i.e. removing the #).

For allegro or libsdl you may need to define the path of the config file
(or at least on win98 and empty path).

Finally type make. If you want a smaller program and do not care about error
messages, you can comment out #DEBUG=1 and run strip sim resp. strip sim.exe
after compile and linking.


The following instructions are for MS Visual C Express:
-------------------------------------------------------

Download Visual Express C++ (tested for 2008 upwards)
http://www.microsoft.com/express/Downloads/

For most libraries you will easily find binaries. A quick start for some of 
them is the bundle used for OpenTTD:
https://www.openttd.org/en/download-openttd-useful/6.0

The bzip2 source tarball comes with an archive where you can easily built
your own libbz2.lib file. Or use the one posted in the forum:
https://forum.simutrans.com/index.php/board,112.0.html

For debugging, you have to set the correct working directory, i.e. the
directory where the pak/ folders are located and use the -use_workdir
command line option.

Nagoya, Mai 2018
