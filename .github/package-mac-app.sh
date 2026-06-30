#!/bin/bash
set -euo pipefail

usage() {
	echo "usage: $0 --binary BINARY --app APP" >&2
	exit 2
}

BINARY=
APP=
while [ "$#" -gt 0 ]; do
	case "$1" in
		--binary)
			[ "$#" -ge 2 ] || usage
			BINARY="$2"
			shift 2
			;;
		--app)
			[ "$#" -ge 2 ] || usage
			APP="$2"
			shift 2
			;;
		*)
			usage
			;;
	esac
done

[ -n "$BINARY" ] || usage
[ -n "$APP" ] || usage
[ -f "$BINARY" ] || { echo "missing binary: $BINARY" >&2; exit 1; }

ROOT_DIR="$(pwd)"
BINARY="$(cd "$(dirname "$BINARY")" && pwd -P)/$(basename "$BINARY")"
CONTENTS="$APP/Contents"
MACOS="$CONTENTS/MacOS"
FRAMEWORKS="$CONTENTS/Frameworks"
RESOURCES="$CONTENTS/Resources"
APP_EXE="$MACOS/sim"

FREETYPE_PREFIX="$(brew --prefix freetype)"
PNG_PREFIX="$(brew --prefix libpng)"
ZSTD_PREFIX="$(brew --prefix zstd)"
SDL2_PREFIX="$(brew --prefix sdl2)"
SDL3_PREFIX="$(brew --prefix sdl3)"

FREETYPE_DYLIB="$FREETYPE_PREFIX/lib/libfreetype.6.dylib"
PNG_DYLIB="$PNG_PREFIX/lib/libpng16.16.dylib"
ZSTD_DYLIB="$ZSTD_PREFIX/lib/libzstd.1.dylib"
SDL2_DYLIB="$SDL2_PREFIX/lib/libSDL2-2.0.0.dylib"
SDL3_DYLIB="$SDL3_PREFIX/lib/libSDL3.0.dylib"

for dylib in "$FREETYPE_DYLIB" "$PNG_DYLIB" "$ZSTD_DYLIB" "$SDL2_DYLIB" "$SDL3_DYLIB"; do
	[ -f "$dylib" ] || { echo "missing dylib: $dylib" >&2; exit 1; }
done

rm -rf "$APP"
mkdir -p "$MACOS" "$FRAMEWORKS" "$RESOURCES"
cp -p "$BINARY" "$APP_EXE"
cp -p "$ROOT_DIR/OSX/Info.plist" "$CONTENTS/Info.plist"
cp -p "$ROOT_DIR/OSX/simutrans.icns" "$RESOURCES/simutrans.icns"
printf 'APPL????\n' > "$CONTENTS/PkgInfo"

cp -p "$FREETYPE_DYLIB" "$FRAMEWORKS/libfreetype.6.dylib"
cp -p "$PNG_DYLIB" "$FRAMEWORKS/libpng16.16.dylib"
cp -p "$ZSTD_DYLIB" "$FRAMEWORKS/libzstd.1.dylib"
cp -p "$SDL2_DYLIB" "$FRAMEWORKS/libSDL2-2.0.0.dylib"

# Homebrew's SDL2 package is currently sdl2-compat, which dlopens SDL3.
cp -p "$SDL3_DYLIB" "$FRAMEWORKS/libSDL3.0.dylib"
cp -p "$SDL3_DYLIB" "$FRAMEWORKS/libSDL3.dylib"

install_name_tool \
	-change "$FREETYPE_DYLIB" "@executable_path/../Frameworks/libfreetype.6.dylib" \
	-change "$PNG_DYLIB" "@executable_path/../Frameworks/libpng16.16.dylib" \
	-change "$ZSTD_DYLIB" "@executable_path/../Frameworks/libzstd.1.dylib" \
	-change "$SDL2_DYLIB" "@executable_path/../Frameworks/libSDL2-2.0.0.dylib" \
	"$APP_EXE"

install_name_tool \
	-change "$PNG_DYLIB" "@executable_path/../Frameworks/libpng16.16.dylib" \
	"$FRAMEWORKS/libfreetype.6.dylib"

install_name_tool -id "@executable_path/../Frameworks/libfreetype.6.dylib" "$FRAMEWORKS/libfreetype.6.dylib"
install_name_tool -id "@executable_path/../Frameworks/libpng16.16.dylib" "$FRAMEWORKS/libpng16.16.dylib"
install_name_tool -id "@executable_path/../Frameworks/libzstd.1.dylib" "$FRAMEWORKS/libzstd.1.dylib"
install_name_tool -id "@executable_path/../Frameworks/libSDL2-2.0.0.dylib" "$FRAMEWORKS/libSDL2-2.0.0.dylib"
install_name_tool -id "@loader_path/libSDL3.0.dylib" "$FRAMEWORKS/libSDL3.0.dylib"
install_name_tool -id "@loader_path/libSDL3.dylib" "$FRAMEWORKS/libSDL3.dylib"

while IFS= read -r rpath; do
	[ -n "$rpath" ] || continue
	install_name_tool -delete_rpath "$rpath" "$FRAMEWORKS/libSDL2-2.0.0.dylib" 2>/dev/null || true
done < <(otool -l "$FRAMEWORKS/libSDL2-2.0.0.dylib" | awk '
	$1 == "cmd" && $2 == "LC_RPATH" {
		getline
		getline
		if ($1 == "path") print $2
	}
')
install_name_tool -add_rpath "@loader_path" "$FRAMEWORKS/libSDL2-2.0.0.dylib" 2>/dev/null || true

codesign --force --deep --sign - "$APP"
codesign --verify --deep --strict --verbose=2 "$APP"

echo "Created $APP"
