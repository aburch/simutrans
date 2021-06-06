@echo Please add the english language pack to MSVC. Then clone
@echo https://github.com/Microsoft/vcpkg
@echo open a commandline in this folder and run
@echo then run bootstrap-vcpkg.bat before running this script
@echo from the vcpgk folder!
@pause
vcpkg integrate install
@rem 32 bit builds
vcpkg install freetype:x86-windows-static
vcpkg install miniupnpc:x86-windows-static
vcpkg install pthreads:x86-windows-static
vcpkg install zstd:x86-windows-static
@pause