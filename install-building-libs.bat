@echo Please add the english language pack to MSVC. Then clone
@echo https://github.com/Microsoft/vcpkg
@echo open a commandline in this folder and run
@echo then run bootstrap-vcpkg.bat before running this script
@echo from the vcpgk folder!
@pause
vcpkg install freetype
vcpkg install miniupnpc
vcpkg install pthreads
vcpkg install zstd
@pause