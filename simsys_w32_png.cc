#include "macros.h"
#include "simmem.h"
#include "simsys_w32_png.h"

#include <stdio.h>

#ifndef _WIN32
#error "Only Windows has GDI+!"
#endif

// windows Bibliotheken DirectDraw 5.x
// windows.h defines min and max macros which we don't want
#define NOMINMAX 1
#include <windows.h>

// structures, since we use the C-interface
struct ImageCodecInfo
{
	CLSID Clsid;
	GUID  FormatID;
	const WCHAR* CodecName;
	const WCHAR* DllName;
	const WCHAR* FormatDescription;
	const WCHAR* FilenameExtension;
	const WCHAR* MimeType;
	DWORD Flags;
	DWORD Version;
	DWORD SigCount;
	DWORD SigSize;
	const BYTE* SigPattern;
	const BYTE* SigMask;
};

#define    PixelFormatIndexed      0x00010000 // Indexes into a palette
#define    PixelFormatGDI          0x00020000 // Is a GDI-supported format
#define    PixelFormat8bppIndexed     (3 | ( 8 << 8) | PixelFormatIndexed | PixelFormatGDI)
#define    PixelFormat16bppRGB555     (5 | (16 << 8) | PixelFormatGDI)
#define    PixelFormat16bppRGB565     (6 | (16 << 8) | PixelFormatGDI)
#define    PixelFormat24bppRGB        (8 | (24 << 8) | PixelFormatGDI)

struct EncoderParameters;

struct GdiplusStartupInput
{
    UINT32 GdiplusVersion;             // Must be 1
    void *DebugEventCallback; // Ignored on free builds
    BOOL SuppressBackgroundThread;     // FALSE unless you're prepared to call
                                       // the hook/unhook functions properly
    BOOL SuppressExternalCodecs;       // FALSE unless you want GDI+ only to use
                                       // its internal image codecs.
};

// and the functions from the library
static int (WINAPI* GdiplusStartup)(ULONG_PTR* token, GdiplusStartupInput* size, void*);
static int (WINAPI* GdiplusShutdown)(ULONG_PTR token);
static int (WINAPI* GdipGetImageEncodersSize)(UINT* numEncoders, UINT* size);
static int (WINAPI* GdipGetImageEncoders)(UINT numEncoders, UINT size, ImageCodecInfo* encoders);
static int (WINAPI* GdipCreateBitmapFromScan0)(INT width, INT height, INT stride, INT PixelFormat, BYTE* scan0, ULONG** bitmap);
static int (WINAPI* GdipDeleteCachedBitmap)(ULONG* image);
static int (WINAPI* GdipSaveImageToFile)(ULONG* image, WCHAR const* filename, CLSID const* clsidEncoder, EncoderParameters const* encoderParams);


// Die GetEncoderClsid() Funktion wurde einfach aus der MSDN/PSDK Doku kopiert.
// Zu finden mit dem Suchstring "Retrieving the Class Identifier for an Encoder"
// Sucht zu z.B. 'image/jpeg' den passenden Encoder und liefert dessen CLSID...
static int GetEncoderClsid(wchar_t const* const format, CLSID* const pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes
	UINT j;
	ImageCodecInfo* pImageCodecInfo = NULL;

	GdipGetImageEncodersSize(&num, &size);

	if(size == 0) return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL) return -1;  // Failure

	GdipGetImageEncoders(num, size, pImageCodecInfo);

	for(  j = 0; j < num; ++j  ) {
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 ) {
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}


template<typename T> static void GetProcAddress(T& dst, HMODULE const module, char const* const procname)
{
	dst = reinterpret_cast<T>(GetProcAddress(module, procname));
}


/* this works only, if gdiplus.dll is there.
 * It should be there on
 * Windows XP and newer
 * On Win98 and up, if .NET is installed
 */
bool dr_screenshot_png(char const* filename, int w, int h, int maxwidth, unsigned short* data, int bitdepth)
{
	// first we try as PNG
	CLSID encoderClsid;
	ULONG *myImage = NULL;
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;

	HMODULE hGDIplus = LoadLibraryA( "gdiplus.dll" );
	if(hGDIplus==NULL) {
		return false;
	}

	// retrieve names ...
	GetProcAddress(GdiplusStartup,            hGDIplus, "GdiplusStartup");
	GetProcAddress(GdiplusShutdown,           hGDIplus, "GdiplusShutdown");
	GetProcAddress(GdipGetImageEncodersSize,  hGDIplus, "GdipGetImageEncodersSize");
	GetProcAddress(GdipGetImageEncoders,      hGDIplus, "GdipGetImageEncoders");
	GetProcAddress(GdipCreateBitmapFromScan0, hGDIplus, "GdipCreateBitmapFromScan0");
	GetProcAddress(GdipDeleteCachedBitmap,    hGDIplus, "GdipDeleteCachedBitmap");
	GetProcAddress(GdipSaveImageToFile,       hGDIplus, "GdipSaveImageToFile");

	/* Win2k can do without init, WinXP not ... */
	gdiplusStartupInput.GdiplusVersion = 1;
	gdiplusStartupInput.DebugEventCallback = NULL;
	gdiplusStartupInput.SuppressBackgroundThread = FALSE;
	gdiplusStartupInput.SuppressExternalCodecs = FALSE;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	if(  bitdepth==8  ) {
		GdipCreateBitmapFromScan0(w, h, maxwidth, PixelFormat8bppIndexed , (BYTE*)data, &myImage);
	}
	else {
		GdipCreateBitmapFromScan0(w, h, maxwidth * 2, bitdepth == 16 ? PixelFormat16bppRGB565 : PixelFormat16bppRGB555, (BYTE*)data, &myImage);
	}
	if(  myImage==NULL  &&  bitdepth>8  ) {
		/* we may have XP or newer => have to convert them to 32 first to save them ... Grrrr */
		BYTE* const newdata = MALLOCN(BYTE, w * h * 4);
		BYTE *dest = newdata;
		unsigned short *src = data;
		int ww = maxwidth;
		int x, y;

		for(  y=0;  y<h;  y++  ) {
			for(  x=0;  x<w;  x++  ) {
				unsigned short color = src[x];
				*dest++ = (color & 0xF800)>>8;
				*dest++ = (color & 0x07E0)>>3;
				*dest++ = (color & 0x001F)<<5;
				*dest++ = 0xFF;
			}
			src += ww;
		}
		GdipCreateBitmapFromScan0(w, h, w * 4, PixelFormat24bppRGB, newdata, &myImage);
		free( newdata );
	}

	// Passenden Encoder für jpegs suchen:
	// Genausogut kann man auch image/png benutzen um png's zu speichern ;D
	// ...oder image/gif um gif's zu speichern, ...
	bool ok = false;
	if(myImage!=NULL  &&  GetEncoderClsid(L"image/png", &encoderClsid)!=-1) {
		char cfilename[1024];
		sprintf(cfilename, "%.*s.png", (int)(strlen(filename) - 4), filename);

		WCHAR wfilename[1024];
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cfilename, -1, wfilename, lengthof(wfilename));

		if (GdipSaveImageToFile(myImage, wfilename, &encoderClsid, 0) == 0) {
			ok = true;
		}
		else {
			//printf( filename, "Save() for png failed!\n");
		}
	}
	else {
		//printf( filename, "No encoder for 'image/jpeg' found!\n");
	}

	// GDI+ freigeben:
	GdipDeleteCachedBitmap(myImage);
	GdiplusShutdown(gdiplusToken);
	FreeLibrary( hGDIplus );

	return ok;
}
