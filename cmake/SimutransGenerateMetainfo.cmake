# gather informations to update flatpak and Android creation files
STRING(STRIP ${SIMUTRANS_WC_REVISION} SIMUTRANS_WC_REVISION)

file(READ "${SOURCE_DIR}/src/simutrans/simversion.h" FILE_CONTENT)
string(REGEX MATCH "#define SIM_VERSION_MAJOR[ ]+([0-9]*)" _ ${FILE_CONTENT})
set(ver_major ${CMAKE_MATCH_1})
string(REGEX MATCH "#define SIM_VERSION_MINOR[ ]+([0-9]*)" _ ${FILE_CONTENT})
set(ver_minor ${CMAKE_MATCH_1})
string(REGEX MATCH "#define SIM_VERSION_PATCH[ ]+([0-9]*)" _ ${FILE_CONTENT})
set(ver_patch ${CMAKE_MATCH_1})
string(REGEX MATCH "#define[ ]+SIM_VERSION_BUILD[ ]+SIM_BUILD_RELEASE[\r\n]+" RELEASE_FLAG ${FILE_CONTENT})

string(TIMESTAMP TODAY "%Y-%m-%d")

# finally, update flatpak xml
if (RELEASE_FLAG)
    set(nightly_suffix "")
else ()
    set(nightly_suffix " nightly")
endif ()

configure_file(
    "${SOURCE_DIR}/src/linux/com.simutrans.Simutrans.metainfo.xml.in"
    "${SOURCE_DIR}/src/linux/com.simutrans.Simutrans.metainfo.xml"
    @ONLY
)

# and Android files
if (RELEASE_FLAG)
    set(nightly_suffix "-Release")
else ()
    set(nightly_suffix "-Nightly")
endif ()

configure_file(
    "${SOURCE_DIR}/src/android/AndroidAppSettings.cfg.in"
    "${SOURCE_DIR}/src/android/AndroidAppSettings.cfg"
    @ONLY
)