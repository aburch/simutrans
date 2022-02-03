@echo Please add the english language pack to MSVC. Then clone
@echo https://github.com/Microsoft/vcpkg
@echo open a commandline in this folder and run
@echo then run bootstrap-vcpkg.bat before running this script
@echo from the vcpgk folder!
@pause
vcpkg integrate install
@rem 64 bit builds
vcpkg install freetype:x64-windows-static
vcpkg install miniupnpc:x64-windows-static
vcpkg install pthreads:x64-windows-static
vcpkg install zstd:x64-windows-static
@pause