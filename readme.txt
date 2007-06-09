How to compile
--------------

Congratulations, you checked out the simutrans source. To compile it,
you have two options, either using Microsoft Visual C++ Express (which
is free in Version 7.0) or GCC.

To compile you will need the following libraries:
libz (http://www.zlib.net/)
libpng (http://www.libpng.org/pub/png/) for makeobj

For the recommended SDL-support you need
libSDL (http://www.libsdl.org/)
libSDL_mixer (link from the same page)

The link for allegro lib is:
http://www.talula.demon.co.uk/allegro/
or
http://alleg.sourceforge.net/index.de.html

To make life easier, you can follow the instructions to compile OpenTTD.
A system set up for OpenTTD will also compile simutrans without problems.

If you are on a MS Windows machine, download either MS VC Express or
MingW. The latter is easier to use as part of the DEV-C++ IDE, which
makes the installation of additional libraries like libz and libsdl and so
on very easy. However, to compile the command line is easier.

For all other systems, it is recommended you get the latest GCC and
matching zlib, and libpng and a game library. For unix system you may
have to use tools like apt-get or yast.

Check out the latest source from the SVN or check out a certain revision.
I recommend always to use the latest source, since it does not make any
sense to work with buggy code.


IMPORTANT:
----------

If you want to contribute, read the coding guidelines in
sim/documentation/coding_styles.txt


The following instructions are for GCC systems:
-----------------------------------------------

Go to Simutrans/sim.

Then copy the file sim/config.template to sim/config.default and edit the
file. You need to specify:
- frontend (gdi, allegro, sdl)
- color depth (usually 16)
- system (you should know it)

I recommend to uncomment #DEBUG=1 and #OPTIMISE = 1 (i.e. removing the #).

For allegro or libsdl you may need to define the path of the config file
(or at least on win98 and empty path).

Finally type make. If you want a smaller program and do not care about error
messages, you can comment out #DEBUG=1 and run strip sim resp. strip sim.exe
after compile and linking.

For users on window systems:
To debug, I recommend to run drmingw -i once in a shell. You will get a
caller history in case of an error. gdb does not really work well and is a
pain to use with the text interface.


The following instructions are for MS Visual C Express:
-------------------------------------------------------

For MS VC Express you have to hunt the matching zlib. Any version
which is compatible with VC will do. Take a look at the instructions for
compiling OpenTTD, which contains a libpng and a libz suited for
simutrans too:
http://wiki.openttd.org/index.php/MicrosoftVisualCExpress

You can compile without zlib, just make sure you disabled support for
compressed savegames in your simuconf.tab. But you will not able to
load compressed savegames then.

For debugging, you have to set the correct working directory, i.e. the
directory where the pak/ folders are located and use the -use_workdir
command line option.

Berlin, Mai 2007
