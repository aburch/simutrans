if (NOT ANDROID)
	find_package(CCache)
	find_package(ZLIB REQUIRED)
	find_package(BZip2 REQUIRED)
	find_package(PNG REQUIRED)
	find_package(MiniUPNP)
	if (MSVC)
		find_package(ZSTD)
		find_package(SDL2)
		find_package(Freetype)
		find_package(FluidSynth)
	else ()
		find_package(PkgConfig MODULE REQUIRED)
		pkg_check_modules(ZSTD REQUIRED IMPORTED_TARGET libzstd)
		pkg_check_modules(SDL2 IMPORTED_TARGET sdl2)
		pkg_check_modules(Freetype IMPORTED_TARGET freetype2)
		pkg_check_modules(FluidSynth IMPORTED_TARGET fluidsynth>=2.1.0)
	endif ()

	set(CMAKE_THREAD_PREFER_PTHREAD ON)
	find_package(Threads)
else ()

	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(PREBUILT_DIR "debug")
	else ()
		set(PREBUILT_DIR "release")
	endif ()

	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../zlib/)
	add_library(ZLIB::ZLIB SHARED IMPORTED)
	set(SHARED_LIBRARY_SO ${CMAKE_CURRENT_SOURCE_DIR}/../zlib/prebuilt/${PREBUILT_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libz.so)
	set_target_properties(ZLIB::ZLIB PROPERTIES IMPORTED_LOCATION ${SHARED_LIBRARY_SO})

	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../bzip2/)
	add_library(BZip2::BZip2 SHARED IMPORTED)
	set(SHARED_LIBRARY_SO ${CMAKE_CURRENT_SOURCE_DIR}/../bzip2/prebuilt/${PREBUILT_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libbz2.so)
	set_target_properties(BZip2::BZip2 PROPERTIES IMPORTED_LOCATION ${SHARED_LIBRARY_SO})

	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libpng/)
	add_library(PNG::PNG SHARED IMPORTED)
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(SHARED_LIBRARY_SO ${CMAKE_CURRENT_SOURCE_DIR}/../libpng/prebuilt/${PREBUILT_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libpng16d.so)
	else ()
		set(SHARED_LIBRARY_SO ${CMAKE_CURRENT_SOURCE_DIR}/../libpng/prebuilt/release/${CMAKE_ANDROID_ARCH_ABI}/libpng16.so)
	endif ()
	set_target_properties(PNG::PNG PROPERTIES IMPORTED_LOCATION ${SHARED_LIBRARY_SO})

	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../zstd/lib/)
	add_library(ZSTD::ZSTD SHARED IMPORTED)
	set(SHARED_LIBRARY_SO ${CMAKE_CURRENT_SOURCE_DIR}/../zstd/prebuilt/${PREBUILT_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libzstd.so)
	set_target_properties(ZSTD::ZSTD PROPERTIES IMPORTED_LOCATION ${SHARED_LIBRARY_SO})

	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../freetype/include/)
	add_library(Freetype::Freetype SHARED IMPORTED)
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(SHARED_LIBRARY_SO ${CMAKE_CURRENT_SOURCE_DIR}/../freetype/prebuilt/${PREBUILT_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libfreetyped.so)
	else ()
		set(SHARED_LIBRARY_SO ${CMAKE_CURRENT_SOURCE_DIR}/../freetype/prebuilt/${PREBUILT_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libfreetype.so)
	endif ()
	set_target_properties(Freetype::Freetype PROPERTIES IMPORTED_LOCATION ${SHARED_LIBRARY_SO})

	find_library(SDL2 SDL2)

	# Fluidsynth dependencies are a big family
	# We may be able to reduce the family numbers by compiling Fluidsynth ourselves
	# But such process is long and tedious (see: https://github.com/FluidSynth/fluidsynth/blob/master/.azure/azure-pipelines-android.yml )
	# For now we use the release libraries
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/include/)

	find_package(OpenMP REQUIRED)

	add_library(libfluidsynth SHARED IMPORTED)
	set(SHARED_LIBRARY_SO ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libfluidsynth.so)
	set_target_properties(libfluidsynth PROPERTIES IMPORTED_LOCATION ${SHARED_LIBRARY_SO})

	add_library(libFLAC SHARED IMPORTED)
	set_target_properties(libFLAC PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libFLAC.so)

	add_library(libfluidsynth-assetloader SHARED IMPORTED)
	set_target_properties(libfluidsynth-assetloader PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libfluidsynth-assetloader.so)

	add_library(libgio-2.0 SHARED IMPORTED)
	set_target_properties(libgio-2.0 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libgio-2.0.so)

	add_library(libglib-2.0 SHARED IMPORTED)
	set_target_properties(libglib-2.0 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libglib-2.0.so)

	add_library(libgmodule-2.0 SHARED IMPORTED)
	set_target_properties(libgmodule-2.0 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libgmodule-2.0.so)

	add_library(libgobject-2.0 SHARED IMPORTED)
	set_target_properties(libgobject-2.0 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libgobject-2.0.so)

	add_library(libgthread-2.0 SHARED IMPORTED)
	set_target_properties(libgthread-2.0 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libgthread-2.0.so)

	add_library(libinstpatch-1.0 SHARED IMPORTED)
	set_target_properties(libinstpatch-1.0 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libinstpatch-1.0.so)

	add_library(liboboe SHARED IMPORTED)
	set_target_properties(liboboe PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/liboboe.so)

	add_library(libogg SHARED IMPORTED)
	set_target_properties(libogg PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libogg.so)

	add_library(libopus SHARED IMPORTED)
	set_target_properties(libopus PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libopus.so)

	add_library(libpcre SHARED IMPORTED)
	set_target_properties(libpcre PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libpcre.so)

	add_library(libpcreposix SHARED IMPORTED)
	set_target_properties(libpcreposix PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libpcreposix.so)

	add_library(libsndfile SHARED IMPORTED)
	set_target_properties(libsndfile PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libsndfile.so)

	add_library(libvorbis SHARED IMPORTED)
	set_target_properties(libvorbis PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libvorbis.so)

	add_library(libvorbisenc SHARED IMPORTED)
	set_target_properties(libvorbisenc PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libvorbisenc.so)

	add_library(libvorbisfile SHARED IMPORTED)
	set_target_properties(libvorbisfile PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../fluidsynth/lib/${CMAKE_ANDROID_ARCH_ABI}/libvorbisfile.so)

	add_library(fluidsynth INTERFACE)
	target_link_libraries(fluidsynth INTERFACE
		libFLAC
		libfluidsynth-assetloader
		libgio-2.0
		libglib-2.0
		libgmodule-2.0
		libgobject-2.0
		libgthread-2.0
		libinstpatch-1.0
		liboboe
		libogg
		libopus
		libpcre
		libpcreposix
		libsndfile
		libvorbis
		libvorbisenc
		libvorbisfile
		libfluidsynth
		OpenMP::OpenMP_CXX
	)

endif ()
