
#ifndef WIN32
#error "Only Windows has GDI+!"
#endif

// windows Bibliotheken DirectDraw 5.x
#include <windows.h>

// structures, since we use the C-interface
typedef struct
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
} ImageCodecInfo;

#define    PixelFormatIndexed      0x00010000 // Indexes into a palette
#define    PixelFormatGDI          0x00020000 // Is a GDI-supported format
#define    PixelFormat8bppIndexed     (3 | ( 8 << 8) | PixelFormatIndexed | PixelFormatGDI)
#define    PixelFormat16bppRGB555     (5 | (16 << 8) | PixelFormatGDI)
#define    PixelFormat16bppRGB565     (6 | (16 << 8) | PixelFormatGDI)

typedef struct
{
    GUID    Guid;               // GUID of the parameter
    ULONG   NumberOfValues;     // Number of the parameter values
    ULONG   Type;               // Value type, like ValueTypeLONG  etc.
    VOID*   Value;              // A pointer to the parameter values
} EncoderParameter;

typedef struct
{
    UINT Count;                      // Number of parameters in this structure
    EncoderParameter Parameter[1];   // Parameter values
} EncoderParameters;

// and the functions from the library
int (WINAPI *GdipGetImageEncodersSize)(UINT *numEncoders, UINT *size);
int (WINAPI *GdipGetImageEncoders)(UINT numEncoders, UINT size, ImageCodecInfo *encoders);
int (WINAPI *GdipCreateBitmapFromScan0)(INT width, INT height, INT stride, INT PixelFormat, BYTE* scan0, ULONG** bitmap);
int (WINAPI *GdipDeleteCachedBitmap)(ULONG *image);
int (WINAPI *GdipSaveImageToFile)(ULONG *image, const WCHAR* filename, const CLSID* clsidEncoder, const EncoderParameters* encoderParams);




// Die GetEncoderClsid() Funktion wurde einfach aus der MSDN/PSDK Doku kopiert.
// Zu finden mit dem Suchstring "Retrieving the Class Identifier for an Encoder"
// Sucht zu z.B. 'image/jpeg' den passenden Encoder und liefert dessen CLSID...
int GetEncoderClsid(const wchar_t *format, CLSID *pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes
	UINT j;
	ImageCodecInfo* pImageCodecInfo = NULL;

	(*GdipGetImageEncodersSize)(&num, &size);
	if(size == 0) return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL) return -1;  // Failure

	(*GdipGetImageEncoders)(num, size, pImageCodecInfo);

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



/* this works only, if gdiplus.dll is there.
 * It should be there on
 * Windows XP and newer
 * On Win98 and up, if .NET is installed
 */
int dr_screenshot_png(const char *filename,  int w, int h, unsigned short *data, int bitdepth )
{
	// first we try as PNG
	CLSID encoderClsid;
	int ok=FALSE;
	ULONG *myImage = NULL;

	HMODULE hGDIplus = LoadLibraryA( "gdiplus.dll" );
	if(hGDIplus==NULL) {
		return FALSE;
	}

	// retrieve names ...
	GdipGetImageEncodersSize = (int (WINAPI *)(UINT *,UINT *)) GetProcAddress( hGDIplus, "GdipGetImageEncodersSize" );
	GdipGetImageEncoders = (int (WINAPI *)(UINT,UINT,ImageCodecInfo *)) GetProcAddress( hGDIplus, "GdipGetImageEncoders" );
	GdipCreateBitmapFromScan0 = (int (WINAPI *)(INT,INT,INT,INT,BYTE *,ULONG **)) GetProcAddress( hGDIplus, "GdipCreateBitmapFromScan0" );
	GdipDeleteCachedBitmap = (int (WINAPI *)(ULONG *)) GetProcAddress( hGDIplus, "GdipDeleteCachedBitmap" );
	GdipSaveImageToFile = (int (WINAPI *)(ULONG *,const WCHAR *,const CLSID *,const EncoderParameters *)) GetProcAddress( hGDIplus, "GdipSaveImageToFile" );

	// create a bitmap
	(*GdipCreateBitmapFromScan0)( w, h, ((w+15) & 0xFFF0)*2, bitdepth==16 ? PixelFormat16bppRGB565 : PixelFormat16bppRGB555, (BYTE *)data, &myImage );

	// Passenden Encoder für jpegs suchen:
	// Genausogut kann man auch image/png benutzen um png's zu speichern ;D
	// ...oder image/gif um gif's zu speichern, ...
	if(myImage!=NULL  &&  GetEncoderClsid(L"image/png", &encoderClsid)!=-1) {
		EncoderParameters ep;
		char cfilename[1024];
		unsigned short wfilename[1024];

		strcpy( cfilename, filename );
		strcpy( cfilename+strlen(cfilename)-3, "png" );

		MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, cfilename, -1, wfilename, 1024 );
		ep.Count = 0;

		if(  (*GdipSaveImageToFile)(myImage,wfilename,&encoderClsid,&ep) == 0  ) {
			ok = TRUE;
		}
		else {
			//printf( filename, "Save() for png failed!\n");
		}
	}
	else {
		//printf( filename, "No encoder for 'image/jpeg' found!\n");
	}

	// GDI+ freigeben:
	(*GdipDeleteCachedBitmap)(myImage);
	FreeLibrary( hGDIplus );

	return ok;
}
