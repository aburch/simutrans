#
# This file is part of the Simutrans-Extended project under the Artistic License.
# (see LICENSE.txt)
#

find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(PNG)
find_package(Freetype)
find_package(MiniUPNP)
find_package(ZSTD)

set(CMAKE_THREAD_PREFER_PTHREAD ON)
find_package(Threads)
