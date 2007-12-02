The purpose of this document is to explain the configuration options of
Simutrans. There are currently two styles of options:

1) entries in the simuconf.tab and forrestconf.tab files
2) command line options


Simuconf.tab
------------

Simuconf.tab contains a rather large number of general configuration
settings for Simutrans.

Simuconf.tab can be found at several locations. First searched is
config/simuconf.tab. Next is pak/config/simuconf.tab (or whatever
pak-path your specified. Finally, simutrans/simuconf.tab in your home
directory ("My Documents" under Windows resp. "~" under Unix).

It is a textfile and can be edited with a text editor. Each entry has
a comment explaining what the entry is good for and what values are
allowed for this entry.


forrestconf.tab
---------------

forrestconf.tab contains some settings that control the generation
of forests during map creation.

Forrestconf.tab is either located pak/config/forestconf.tab or in
simutrans/forestconf.tab in your user directory. It is a textfile and
can be edited with a text editor. Each entry has a comment explaining
what the entry is good for and what values are allowed for this entry.


cityrules.tab
-------------

cityrules.tab is either located pak/config/cityrules.tab or in
simutrans/cityrules.tab in your user directory. It is a textfile and
can be edited with a text editor.

cityrules.tab contains the set of rules which determines the extension
of towns during city growth. There are two city related settings in the
top, the minimum distance between cities (minimum_city_distance)
and the step whenever a new industry will be founded
(industry_increase_every). Set this to zero, if no new industry should
be founded during a game.

The actual rules are of two types:
house building rules and road building rules.
Each rule has a probablility (house_xyz.chance) between -1 and -7 and
a square with describes allowed surroundings for this house rule. The
square is composed by ASCII rows, with each symbol standing for:
S = must not be road
s = must be road
n = must be bare land
H = must not be house
h = must be house
T = must not be a stop
t = must be a stop
. = matches anything


Simutrans command line options
------------------------------

1.) Choose a resolution:

1a.) Choose a 'standard'  resolution

simutrans -res n

where n is one of 1,2,3,4,5

The switch -res chooses the resolution at program start:

1 = 640x480
2 = 800x600
3 = 1024x768
4 = 1280x1024

Resolution no. 5 runs Simutrans in a window instead of fullscreen mode
(Windows version only)

THIS SWITICH IS DEPRECATED AND ONLY KEPT FOR COMPATIBILITY!
Please use -screensize WxH instead (see section 1b) !

Use this switch at your own risk! Using a wrong resolution may damage your
monitor! Up to date monitors should support all resolutions, but older
monitors may have problems with the higher resolution settings. Don't expect
all resolutions to be available with your graphics card. The program will
abort the starting process if the graphics card doesn't support the chosen
resolution.

The default resolution was 640x480. Simutrans version 0.78.4 and newer
have a default resolution of 800x600. Please note that bigger resolutions
slow down the game. If Simutrans runs too slowly on your system, use a
smaller screen resolution. Windowed mode (-res 5 on Windows) may be
slower than fullscreen, but this depends strongly on your resolution and
machine.


1b.) Choose an arbitrary resolution

simutrans -screensize 900x400

More general the syntax is -screensize WIDTHxHEIGHT
where width and height are integral values separated by a small x
This format is strict. Simutrans will not tolerate spaces or other
format elements.

I suggest only to use widths being a multiple of 16 and even heights,
other may cause problems.

Full screen support by -fullscreen.

Use with care! I'm sure you can screw up things if you use values which
are not supported by your system. Use on your own risk ... i.e.
using -screensize 1600x1400 might damage your monitor if the graphics
card switches to that resolution, but your monitor cannot! I give
absolutely no warranty for any results of using that option!!!

Please note that if width is set below 640, parts of the toolbar will
not be visible and can't be used. In general setting width to less than 640
and height to less than 400 will most likely cause trouble ... the program
does not check the values! As said above, use with care, no warranties are
given!


2.) Copy output messages into a logfile:

simutrans -log 1

This writes all messages which are output after the game switches to graphics
mode to a file named "simu.log". This file might be helpful when reporting
problems, you might consider to log a replay of the problem and send the log
file together with a problem report to hansjoerg.malthaner@gmx.de. Please
zip (compress) large log files before sending them. Don't forget your
explantion of the problem is much more important than the log file when
reporting problems.

The -debug switch turns on additional debug messages. The recommended
combination is "-log 1 -debug"


3.) Run Simutrans in free playing mode (bankrupt check turned off):

simutrans -freeplay


4.) Screen refresh setting:

On slow machines even updating 640x480 pixels each frame may be
too much load for the CPU. To run Simutrans on slow machines, frame
skipping can be activated. Vehicles may move less smoothly this way,
but at least it is possible to run Simutrans at all. This is helpful
to run the big resolutions on standard machines, too.

simutrans -refresh n

displays every nth frame only. n=2 should result in a speedup of 25%,
n=3 in a speedup of 33%
You can chose n in a range from 1 to 16, where n=1 has the same effect as
omitting this setting at all. n >= 4 may result in 'jumpy' vehicle movement.

Try several values to find the best setting for your hardware. Usually
n=2 or n=3 works best on slow machines. n=16 may be a kind of slideshow
mode.


5.) Getting help (well it just says read this readme file)

simutrans -h
simutrans -?
simutrans -help
simutrans --help

All print a quick help message, which basically says
"Read the readme file"


6.) Use alternative PAK files

If you have an alternative PAK file set for Simutrans, such as the
winter scenario, you can use the following command line argument:

simutrans -objects winter_pak/

"winter_pak" is the directory where the PAK files are stored. Use the
name of your setup instead! The trailing slash is mandatory.



7) Turn sound and music off (as of Simutrans 0.81.23exp)

7a) Turn sound off

simutrans -nosound

7b) Turn music off

simutrans -nomidi

This also helps if Simutrans crashes on startup due to buggy or
incompatible sound drivers.


8) Switch on/off multiplayer

-singleuser

Will not search for files in your home directory. Simutrans will
write all files in its program directory, which must not be write
protected.


9) Timeline and startyear

Example: simutrans -timeline 1 -startyear 1950

9b) Timeline

simutrans -timeline <1 or 0>

0 = all vehicles are available from the start of the game
1 = More relistic. In 1930 are only some historical vehicles
    available. New vehicles will be introduced from time to time.

Example: simutrans -timeline 1 -startyear 1930

Default value: setting use_timeline in simuconf.tab

9b) Start year

simutrans -startyear <year>

Sets the starting year of the game

Example: simutrans -timeline 1 -startyear 1950

Default value : setting starting_year in simuconf.tab


10) load a game and do not show the initial window

simutrans -load mygame.sve



All the above mentioned options can be combined, i.e.

simutrans -res 2 -log -objects winter_pak/

runs Simutrans in test mode, writes output to "simu.log", switches to
a resolution of 800x600 pixels and uses the winter pak files. Not
all options are available on all platforms, i.e. -async and -net are
only supported on Linux/X-Windows.


If you run into problems, or if you have questions, please visit the
feedback forum:

http://forum.simutrans.com/


If the forum entries and community can't help, you can one of the developer
by email:

team@64.simutrans.com


Suggestions how to improve Simutrans are also always welcome.

If you're unsure how to report problems or change request, there are
two example forms packaged with simutrans. Use "problem_report.txt"
for problem reports and "change_request.txt" for change requests
if you send reports by email.



There are some Simutrans related web pages, you may want to visit them:

The official Simutrans home page:
http://www.simutrans.com

This readme file was written by Hansjörg Malthaner, November 2000,
Last update 24-Jul-2006 by Markus Pristovsek
