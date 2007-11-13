/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef WIN32
#error "Only Windows has GDI+!"
#endif

#define UNICODE 1
#include <stdio.h>

// windows Bibliotheken DirectDraw 5.x
#include <windows.h>
#include <gdiplus.h> // not found? => http://lists.zerezo.com/mingw-users/msg00862.html


using namespace Gdiplus;

// Die GetEncoderClsid() Funktion wurde einfach aus der MSDN/PSDK Doku kopiert.
// Zu finden mit dem Suchstring "Retrieving the Class Identifier for an Encoder"
// Sucht zu z.B. 'image/jpeg' den passenden Encoder und liefert dessen CLSID...
int GetEncoderClsid(const wchar_t *format, CLSID *pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if(size == 0) return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL) return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for(  UINT j = 0; j < num; ++j  ) {
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 ) {
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}


extern "C" bool dr_screenshot_png(const char *filename, int w, int h, unsigned short *data, int bitdepth );

bool dr_screenshot_png(const char *filename,  int w, int h, unsigned short *data, int bitdepth )
{
	// first we try as PNG
	GdiplusStartupInput gsi;
	ULONG_PTR gdiplusToken;
	CLSID encoderClsid;
	bool ok=false;

	// GDI+ initialisieren: (this code is stolen from www.geeky.de)
	GdiplusStartup(&gdiplusToken, &gsi, NULL);

	// we cannot use automatic decleration, since it must be freed before GDI+ is shut down
	Bitmap *myImage = new Bitmap( w, h, ((w+15) & 0xFFF0)*2, bitdepth==16 ? PixelFormat16bppRGB565 : PixelFormat16bppRGB555, (BYTE *)data );

	// Passenden Encoder für jpegs suchen:
	// Genausogut kann man auch image/png benutzen um png's zu speichern ;D
	// ...oder image/gif um gif's zu speichern, ...
	if(myImage!=NULL  &&  GetEncoderClsid(L"image/png", &encoderClsid)!=-1) {
		EncoderParameters ep;
		ep.Count = 0;
		wchar_t wfilename[1024];
		MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, filename, -1, wfilename, 1024 );

		if (myImage->Save(wfilename,&encoderClsid,&ep)==Ok) {
			ok = true;
		}
		else {
			printf( filename, "Save() for png failed!\n");
		}
	}
	else {
		printf( filename, "No encoder for 'image/jpeg' found!\n");
	}

	// GDI+ freigeben:
	delete myImage;	// we cannot use automatic decleration, since it must be free before GDI+ is closed
	GdiplusShutdown(gdiplusToken);

	return ok;
}
