simutrans.ac

AC_INIT(simutrans, version-0.1)

AC_PROG_CC
AC_LANG(C++)

AC_CHECK_LIB(png, png_read_image)
AC_CHECK_LIB(pthread, pthread_mutex_destroy)
AC_CHECK_LIB(bz2, BZ2_bzReadOpen)
AC_CHECK_LIB(SDL2, SDL_GetRenderDriverInfo)
AC_CHECK_LIB(SDL, SDL_Init)
AC_CHECK_LIB(allegro, get_desktop_resolution)

AC_CHECK_HEADERS(QTKit/QTMovie.h)
AC_CHECK_HEADERS(windows.h)
AC_CHECK_HEADERS(LocaleRoster.h)


# hackish detection of OS ...
if test "$ac_cv_header_windows_h" == yes
then
	AC_SUBST(os_type, mingw)
elif test "$ac_cv_header_LocaleRoster_h" == yes
then
	AC_SUBST(os_type, haiku)
elif test "$ac_cv_header_QTKit_QTMovie_h" == yes
then
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

AC_ARG_ENABLE([server], [AS_HELP_STRING([--enable-server],[Builds a server without graphics])] )

# and backend ...
if test "x$enable_server" != "xno"
then
	AC_SUBST(backend, posix)
	AC_SUBST(color, 0)
elif test "$ac_cv_header_windows_h" == yes
then
	AC_SUBST(backend, GDI)
	AC_SUBST(color, 16)
	AC_MSG_WARN([Using GDI backend!])
elif test "$ac_cv_lib_SDL2_SDL_GetRenderDriverInfo" == yes
then
	AC_SUBST(backend, SDL2)
	AC_SUBST(color, 16)
	AC_MSG_WARN([Using SDL2 backend!])
elif test "$ac_cv_lib_SDL_SDL_Init" == yes
then
	AC_SUBST(backend, SDL)
	AC_SUBST(color, 16)
	AC_MSG_WARN([Using SDL backend!])
elif test "$ac_cv_lib_allegro_get_desktop_resolution" == yes
then
	AC_SUBST(backend, allegro)
	AC_SUBST(color, 16)
	AC_MSG_WARN([Using SDL2 backend!])
else
	AC_SUBST(backend, Posix)
	AC_SUBST(color, 0)
	AC_MSG_WARN([No backend found, use server (posix)!])
fi


# no multthreading without pthreads ...
if test "$ac_cv_lib_pthread_pthread_mutex_destroy" == yes
then
	AC_SUBST(multithread, 1)
else
	AC_SUBST(multithread, 0)
fi


# are we in a svn?
if svn info
then
	AC_SUBST(svn, 1)
else
	AC_SUBST(svn, 0)
fi


#switch off assembler on 64 bit
AC_CHECK_SIZEOF(void*)
if test "$ac_cv_sizeof_voidp" == 4
then
	AC_SUBST(flags, '')
else
	AC_SUBST(flags, '-DUSE_C')
fi

# missing libs
if test "$ac_cv_lib_bz2_BZ2_bzReadOpen" == no
then
	AC_MSG_WARN([Error, the bz2lib is missing!])
fi

if test "$ac_cv_lib_png_png_read_image" == no
then
	AC_MSG_WARN([Error, the libpng is missing!])
fi


AC_OUTPUT(config.default)
