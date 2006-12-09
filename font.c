#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "simtypes.h"
#include "macros.h"
#include "font.h"


static int nibble(sint8 c)
{
	return (c > '9') ? 10 + c - 'A' : c - '0';
}


/**
 * Decodes a single line of a character
 */
static void dsp_decode_bdf_data_row(uint8 *data, int y, int g_width, char *str)
{
	uint8 data2;
	char buf[3];

	buf[0] = str[0];
	buf[1] = str[1];
	buf[2] = '\0';
	data[y] = (uint8)strtol(buf, NULL ,16);
	// read second byte but use only first nibble
	if (g_width > 8) {
		buf[0] = str[2];
		buf[1] = str[3];
		buf[2] = '\0';
		data2 = (uint8)(strtol(buf, NULL, 16) & 0xF0);
		if((y&1)==0) {
			data[12 + (y / 2)] |= data2;
		}
		else {
			data[12 + (y / 2)] |= data2>>4;
		}
	}
}


/**
 * Reads a single character
 */
static int dsp_read_bdf_glyph(FILE *fin, uint8 *data, uint8 *screen_w, int char_limit, int f_height, int f_desc)
{
	int	char_nr = 0;
	int g_width, h, g_desc;
	int d_width = 0;

	while (!feof(fin)) {
		char str[256];

		fgets(str, sizeof(str), fin);

		// endcoding (sint8 number) in decimal
		if (strncmp(str, "ENCODING", 8) == 0) {
			char_nr = atoi(str + 8);
			if (char_nr <= 0 || char_nr >= char_limit) {
				fprintf(stderr, "Unexpected character (%i) for %i character font!\n", char_nr, char_limit);
				char_nr = 0;
			}
			memset( data+CHARACTER_LEN*char_nr, 0, CHARACTER_LEN );
			continue;
		}

		// information over size and coding
		if (strncmp(str, "BBX", 3) == 0) {
			sscanf(str + 3, "%d %d %*d %d", &g_width, &h, &g_desc);
			continue;
		}

		// information over size and coding
		if (strncmp(str, "DWIDTH", 6) == 0) {
			d_width = atoi(str + 6);
			continue;
		}

		// start if bitmap data
		if (strncmp(str, "BITMAP", 6) == 0) {
			const int top = f_height + f_desc - h - g_desc;
			int y;

			// maximum size 10 pixels
			h += top;
			if (h > 12) {
				h = 12;
			}

			// read for height times
			for (y = top; y < h; y++) {
				fgets(str, sizeof(str), fin);
				dsp_decode_bdf_data_row(data + char_nr*CHARACTER_LEN, y, g_width, str);
			}
			continue;
		}

		// finally add width information (width = 0: not there!)
		if (strncmp(str, "ENDCHAR", 7) == 0) {
			uint8 start_h=0, i;

			data[CHARACTER_LEN*char_nr + CHARACTER_LEN-1] = g_width;
			if (d_width == 0) {
#ifdef DEBUG
				// no screen width: should not happen, but we can recover
				fprintf(stderr, "BDF warning: %i has no screen width assigned!\n", char_nr);
#endif
				d_width = g_width + 1;
			}
			screen_w[char_nr] = d_width;
			// find the start offset
			for( i=0;  i<6;  i++  ) {
				if(data[CHARACTER_LEN*char_nr + i*2]==0  &&  (data[CHARACTER_LEN*char_nr + 12+i]&0xF0)==0) {
					start_h++;
				}
				else {
					break;
				}
				if(data[CHARACTER_LEN*char_nr + i *2 + 1]==0  &&  (data[CHARACTER_LEN*char_nr + 12+i]&0x0F)==0) {
					start_h++;
				}
				else {
					break;
				}
			}
			if(start_h==12) {
				g_width = 0;
			}
			data[CHARACTER_LEN * char_nr + CHARACTER_LEN-2] = start_h;
			// finished
			return char_nr;
			continue;
		}
	}
	return 0;
}


/**
 * Reads a single character
 */
static bool dsp_read_bdf_font(FILE* fin, font_type* font)
{
	uint8* screen_widths = NULL;
	uint8* data = NULL;
	int	f_height;
	int f_desc;
	int f_chars = 0;

	while (!feof(fin)) {
		char str[256];

		fgets(str, sizeof(str), fin);

		if (strncmp(str, "FONTBOUNDINGBOX", 15) == 0) {
			sscanf(str + 15, "%*d %d %*d %d", &f_height, &f_desc);
			continue;
		}

		if (strncmp(str, "CHARS", 5) == 0) {
			f_chars = (atoi(str + 5) > 255) ? 65536 : 256;

			data = calloc(f_chars, CHARACTER_LEN);
			if (data == NULL) {
				fprintf(stderr, "No enough memory for font allocation!\n");
				return false;
			}

			screen_widths = calloc(f_chars, 1);
			if (screen_widths == NULL) {
				free(data);
				fprintf(stderr, "No enough memory for font allocation!\n");
				return false;
			}

			data[32 * CHARACTER_LEN] = 0; // screen width of space
			screen_widths[32] = clamp(f_height / 2, 3, 12);
			continue;
		}

		if (strncmp(str, "STARTCHAR", 9) == 0 && f_chars > 0) {
			dsp_read_bdf_glyph(fin, data, screen_widths, f_chars, f_height, f_desc);
			continue;
		}
	}

	// ok, was successful?
	if (f_chars > 0) {
		int h;

		// init default char for missing characters (just a box)
		screen_widths[0] = 8;
		data[0] = 0x7E;
		for (h = 1; h < f_height - 2; h++) {
			data[h] = 0x42;
		}
		data[h++] = 0x7E;
		for (; h < CHARACTER_LEN-2; h++) {
			data[h] = 0;
		}
		data[CHARACTER_LEN-2] = 1;	// y-offset
		data[CHARACTER_LEN-1] = 7;  // real width

		free(font->screen_width);
		free(font->char_data);
		font->screen_width = screen_widths;
		font->char_data    = data;
		font->height       = f_height;
		font->descent      = f_height + f_desc;
		font->num_chars    = f_chars;
		return true;
	}
	return false;
}


bool load_font(font_type* fnt, const char* fname)
{
	FILE* f = fopen(fname, "rb");
	uint8 c;

	if (f == NULL) {
		fprintf(stderr, "Error: Cannot open '%s'\n", fname);
		return false;
	}
	c = getc(f);

	// binary => the assume dumpe prop file
	if (c < 32) {
		// read classical prop font
		uint8 npr_fonttab[3072];
		int i;

		rewind(f);
		fprintf(stderr, "Loading font '%s'\n", fname);

		if (fread(npr_fonttab, sizeof(npr_fonttab), 1, f) != 1) {
			fprintf(stderr, "Error: %s wrong size for old format prop font!\n", fname);
			fclose(f);
			return false;
		}
		fclose(f);

		// convert to new standard font
		free(fnt->screen_width);
		free(fnt->char_data);
		fnt->screen_width = malloc(256);
		fnt->char_data    = malloc(CHARACTER_LEN * 256);
		fnt->num_chars    = 256;
		fnt->height       = 10;
		fnt->descent      = -1;

		for (i = 0; i < 256; i++) {
			int j;
			uint8 start_h;

			fnt->screen_width[i] = npr_fonttab[256 + i];
			for (j = 0; j < 10; j++) {
				fnt->char_data[i * CHARACTER_LEN + j] = npr_fonttab[512 + i * 10 + j];
			}
			for (; j < CHARACTER_LEN-2; j++) {
				fnt->char_data[i * CHARACTER_LEN + j] = 0;
			}
			// find the start offset
			for( start_h=0;  fnt->char_data[CHARACTER_LEN*i + start_h]==0  &&  start_h<10;  start_h++  )
				;
			fnt->char_data[CHARACTER_LEN * i + CHARACTER_LEN-2] = start_h;

			fnt->char_data[CHARACTER_LEN * i + CHARACTER_LEN-1] = npr_fonttab[i];
		}
		fnt->screen_width[32] = 4;
		fnt->char_data[CHARACTER_LEN*32 + CHARACTER_LEN-1] = 0;	// space width
		fprintf(stderr, "%s sucessfully loaded as old format prop font!\n", fname);
		return true;
	}

	// load old hex font format
	if (c == '0') {
		uint8 dr_4x7font[7 * 256];
		char buf[80];
		int i;

		rewind(f);

		while (fgets(buf, sizeof(buf), f) != NULL) {
			const char* p;
			int line;
			int n;

			sscanf(buf, "%4x", &n);

			p = buf + 5;
			for (line = 0; line < 7; line++) {
				int val = nibble(p[0]) * 16 + nibble(p[1]);

				dr_4x7font[n * 7 + line] = val;
				p += 2;
			}
		}
		fclose(f);
		// convert to new standard font
		free(fnt->screen_width);
		free(fnt->char_data);
		fnt->screen_width = malloc(256);
		fnt->char_data    = malloc(CHARACTER_LEN * 256);
		fnt->num_chars    = 256;
		fnt->height       = 7;
		fnt->descent      = -1;

		for (i = 0; i < 256; i++) {
			int j;

			fnt->screen_width[i] = 4;
			for (j = 0; j < 7; j++) {
				fnt->char_data[i * CHARACTER_LEN + j] = dr_4x7font[i * 7 + j];
			}
			for (; j < 15; j++) {
				fnt->char_data[i * CHARACTER_LEN + j] = 0;
			}
			fnt->char_data[i * CHARACTER_LEN + CHARACTER_LEN-2] = 0;
			fnt->char_data[i * CHARACTER_LEN + CHARACTER_LEN-1] = 3;
		}
		fprintf(stderr, "%s sucessful loaded as old format hex font!\n", fname);
		return true;
	}

	fprintf(stderr, "Loading BDF font '%s'\n", fname);
	if (dsp_read_bdf_font(f, fnt)) {
		fprintf(stderr, "Loading BDF font %s ok\n", fname);
		fclose(f);
		return true;
	}
	return false;
}
