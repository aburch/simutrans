simutrans.ac

AC_INIT(simutrans, version-0.1)

AC_PROG_CC
AC_LANG(C++)

# architecture
AC_C_BIGENDIAN([AC_SUBST(endian, '-DSIM_BIG_ENDIAN')], [AC_SUBST(endian, '')], [AC_SUBST(endian, '')], [AC_SUBST(endian, '')])

# missing libs
AC_CHECK_LIB(png, png_read_image, [], [AC_MSG_WARN([Error, libpng is missing!])] )
AC_CHECK_LIB(bz2, BZ2_bzReadOpen, [], [AC_MSG_WARN([Error, libbz2 is missing!])] )

#optional freetype
AC_CHECK_LIB(freetype2, load_FT_font, [AC_SUBST(freetype2, '-DUSE_FREETYPE')], [AC_SUBST(freetype2, '')] )

# optional (but highly recommended) multithreading
AC_CHECK_LIB(pthread, pthread_mutex_destroy, [AC_SUBST(multithread, 1)], [AC_SUBST(multithread, 0)] )

# find OS and backend by libs ...
AC_CHECK_LIB(SDL2, SDL_GetRenderDriverInfo)
AC_CHECK_LIB(SDL, SDL_Init)
AC_CHECK_LIB(allegro, get_desktop_resolution)

AC_CHECK_HEADERS(windows.h)
AC_CHECK_HEADERS(LocaleRoster.h)


# hackish detection of OS ...
if test "$ac_cv_header_windows_h" == yes
then
	AC_SUBST(os_type, mingw)
elif test "$ac_cv_header_LocaleRoster_h" == yes
then
	AC_SUBST(os_type, haiku)
elif uname |grep "Darwin"
then
	AC_LANG_PUSH(Objective C++)
	AC_CHECK_HEADERS(QTKit/QTMovie.h)
	AC_LANG_POP(Objective C++)
	AC_SUBST(os_type, mac)
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


# and backend ...
AC_ARG_ENABLE([server], [AS_HELP_STRING([--enable-server],[Builds a server without graphics])], [], [enable_server=no])

# first test if forced as server
if test "x$enable_server" != "xno"
then
	AC_SUBST(backend, posix)
	AC_SUBST(color, 0)
elif test "$ac_cv_header_windows_h" == yes
then
	AC_SUBST(backend, gdi)
	AC_SUBST(color, 16)
	AC_MSG_WARN([Using GDI backend!])
elif test "$ac_cv_lib_SDL2_SDL_GetRenderDriverInfo" == yes
then
	AC_SUBST(backend, sdl2)
	AC_SUBST(color, 16)
	AC_MSG_WARN([Using SDL2 backend!])
elif test "$ac_cv_lib_SDL_SDL_Init" == yes
then
	AC_SUBST(backend, sdl)
	AC_SUBST(color, 16)
	AC_MSG_WARN([Using SDL backend!])
elif test "$ac_cv_lib_allegro_get_desktop_resolution" == yes
then
	AC_SUBST(backend, allegro)
	AC_SUBST(color, 16)
	AC_MSG_WARN([Using SDL2 backend!])
else
	AC_SUBST(backend, posix)
	AC_SUBST(color, 0)
	AC_MSG_WARN([No backend found, use server (posix)!])
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
