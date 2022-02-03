@echo Please add the english language pack to MSVC. Then clone
@echo https://github.com/Microsoft/vcpkg
@echo open a commandline in this folder and run
@echo then run bootstrap-vcpkg.bat before running this script
@echo from the vcpgk folder!
@pause
vcpkg integrate install
@rem arm64 builds
vcpkg install freetype:arm64-windows-static
vcpkg install miniupnpc:arm64-windows-static
vcpkg install pthreads:arm64-windows-static
vcpkg install zstd:arm64-windows-static
@pause