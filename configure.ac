#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

simutrans.ac

AC_INIT(simutrans, version-0.2)

AC_PROG_CC
AC_LANG(C++)

# architecture
AC_C_BIGENDIAN([AC_SUBST(endian, '-DSIM_BIG_ENDIAN')],
	[AC_SUBST(endian, '')],
	[AC_SUBST(endian, '')],
	[AC_SUBST(endian, '')])

# missing libs
AC_CHECK_LIB(png, png_read_image, [],
	[AC_MSG_WARN([libpng is missing! Makeobj will not compile!])] )
AC_CHECK_LIB(bz2, BZ2_bzReadOpen, [],
	[AC_MSG_WARN([libbz2 is missing! Only zlib compressed saves will be available.])] )

# optional (but highly recommended) multithreading
AC_SEARCH_LIBS(pthread_mutex_destroy, pthread,
	[AC_SUBST(multithread, 1)],
	[AC_SUBST(multithread, 0)] )

AC_CHECK_LIB(pthreadGC2, pthread_mutex_destroy,
	[AC_SUBST(bundlepthread, 1)],
	[AC_SUBST(bundlepthread, 0)] )

# find OS and backend by libs ...
AC_CHECK_LIB(SDL2, SDL_GetRenderDriverInfo)

AC_CHECK_HEADERS(windows.h)
AC_CHECK_HEADERS(LocaleRoster.h)

# optional zstd
AC_SEARCH_LIBS(ZSTD_CStreamInSize, zstd,
	[AC_SUBST(zstd, 1)],
	[AC_SUBST(zstd, 0)] )

# hackish detection of OS ...
if test "$ac_cv_header_windows_h" == yes
then
	AC_SUBST(os_type, mingw)
	# Mac stuff
	AC_SUBST(av_foundation, 0)
	# optional upnp
	AC_SEARCH_LIBS(upnpDiscover, miniupnpc,
		[AC_SUBST(upnp, 1)],
		[AC_SUBST(upnp, 0)],
		-liphlpapi -lws2_32 )
	# optional freetype
	AC_SEARCH_LIBS(FT_Init_FreeType, freetype,
		[AC_SUBST(freetype, 1)],
		[AC_SUBST(freetype, 0)],
		-lpng -lharfbuzz -lgraphite2 -lfreetype )
	# optional fluidsynth
	AC_SEARCH_LIBS(new_fluid_settings, fluidsynth,
		[AC_SUBST(fluidsynth, 1)],
		[AC_SUBST(fluidsynth, 0)],
		-lglib-2.0 -lintl -liconv -ldsound -lole32)
else
	# optional upnp
	AC_SEARCH_LIBS(upnpDiscover, miniupnpc,
		[AC_SUBST(upnp, 1)],
		[AC_SUBST(upnp, 0)] )
	# optional freetype
	AC_SEARCH_LIBS(FT_Init_FreeType, freetype,
		[AC_SUBST(freetype, 1)],
		[AC_SUBST(freetype, 0)],
		-lpng )
	# optional fluidsynth
	AC_SEARCH_LIBS(new_fluid_settings, fluidsynth,
		[AC_SUBST(fluidsynth, 1)],
		[AC_SUBST(fluidsynth, 0)] )
	if uname | grep "Darwin"
	then
		AC_LANG_PUSH(Objective C++)
		AC_CHECK_HEADERS(AVFoundation/AVFoundation.h)
		AC_CHECK_HEADERS(QTKit/QTMovie.h)
		AC_LANG_POP(Objective C++)
		if test "$ac_cv_header_AVFoundation_AVFoundation_h" == yes
		then
			AC_SUBST(av_foundation, 1)
		else
			AC_SUBST(av_foundation, 0)
			if test "$ac_cv_header_QTKit_QTMovie_h" != yes
			then
				AC_MSG_ERROR([Neither AVFoundation nor QTKit are available as a sound driver. Simutrans will not compile!])
			fi
		fi
		AC_SUBST(os_type, mac)
	else
		# Mac stuff
		AC_SUBST(av_foundation, 0)
		if test "$ac_cv_header_LocaleRoster_h" == yes
		then
			AC_SUBST(os_type, haiku)
		elif uname | grep "Linux"
		then
			AC_SUBST(os_type, linux)
		elif uname | grep "BSD"
		then
			AC_SUBST(os_type, freebsd)
		elif uname | grep "miga"
		then
			AC_SUBST(os_type, amiga)
		else
			AC_MSG_ERROR([Unknow OS!])
		fi
	fi
fi

# and backend ...
AC_ARG_ENABLE([server],
	[AS_HELP_STRING(
		[--enable-server],
		[Builds a server without graphics])],
	[],
	[enable_server=no])

# first test if forced as server
if test "x$enable_server" != "xno"
then
	AC_SUBST(backend, posix)
elif test "$ac_cv_header_windows_h" == yes
then
	AC_SUBST(backend, gdi)
	AC_MSG_WARN([Using GDI backend!])
elif test "$ac_cv_lib_SDL2_SDL_GetRenderDriverInfo" == yes
then
	AC_SUBST(backend, sdl2)
	AC_MSG_WARN([Using SDL2 backend!])
else
	AC_SUBST(backend, posix)
	AC_MSG_WARN([No backend found, using server (posix)!])
fi


# are we in a svn?
if svn info
then
	AC_SUBST(svn, 1)
else
	AC_SUBST(svn, 0)
fi


#switch off assembler on 64 bit (MISSING: Check for GNU Assembler and i86)
AC_CHECK_SIZEOF(void*)
if test "$ac_cv_sizeof_voidp" == 4
then
	if test "$ac_cv_cxx_compiler_gnu" == yes
	then
		AC_SUBST(flags, '')
	else
		AC_SUBST(flags, '-DUSE_C')
	fi
fi

AC_OUTPUT(config.default)
