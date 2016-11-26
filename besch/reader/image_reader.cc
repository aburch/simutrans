#include <stdio.h>
#include <string.h>

#include "../../display/simgraph.h"
#include "../../simdebug.h"
#include "../../display/simimg.h"

#include "../bild_besch.h"
#include "image_reader.h"
#include "../obj_node_info.h"

#include <zlib.h>
#include "../../tpl/inthashtable_tpl.h"

// if without graphics backend, do not copy any pixel
#if COLOUR_DEPTH != 0
#define skip_reading_pixels_if_no_graphics
#else
#define skip_reading_pixels_if_no_graphics goto adjust_image
#endif

obj_besch_t *image_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);
	bild_besch_t* besch=NULL;

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf+6;

	// always zero in old version, since length was always less than 65535
	// because a node could not hold more data
	uint8 version = decode_uint8(p);
	p = besch_buf;

#if COLOUR_DEPTH != 0
	besch = new bild_besch_t();
#else
	// reserve space for one single pixel and initialize data
	besch = bild_besch_t::create_single_pixel();
#endif

	if(version==0) {
		besch->x = decode_uint8(p);
		besch->w = decode_uint8(p);
		besch->y = decode_uint8(p);
		besch->h = decode_uint8(p);
		besch->alloc(decode_uint32(p)); // len
		besch->bild_nr = IMG_EMPTY;
		p += 2;	// dummys
		besch->zoomable = decode_uint8(p);

		skip_reading_pixels_if_no_graphics;
		//DBG_DEBUG("bild_besch_t::read_node()","x,y=%d,%d  w,h=%d,%d, len=%i",besch->x,besch->y,besch->w,besch->h, besch->len);

		uint16* dest = besch->data;
		p = besch_buf+12;

		if (besch->h > 0) {
			for (uint i = 0; i < besch->len; i++) {
				uint16 data = decode_uint16(p);
				if(data>=0x8000u  &&  data<=0x800Fu) {
					// player color offset changed
					data ++;
				}
				*dest++ = data;
			}
		}
	}
	else if(version<=2) {
		besch->x = decode_sint16(p);
		besch->y = decode_sint16(p);
		besch->w = decode_uint8(p);
		besch->h = decode_uint8(p);
		p++; // skip version information
		besch->alloc(decode_uint16(p)); // len
		besch->zoomable = decode_uint8(p);
		besch->bild_nr = IMG_EMPTY;

		skip_reading_pixels_if_no_graphics;
		uint16* dest = besch->data;
		if (besch->h > 0) {
			for (uint i = 0; i < besch->len; i++) {
				*dest++ = decode_uint16(p);
			}
		}
	}
	else if(version==3) {
		besch->x = decode_sint16(p);
		besch->y = decode_sint16(p);
		besch->w = decode_sint16(p);
		p++; // skip version information
		besch->h = decode_sint16(p);
		besch->alloc((node.size-10)/2); // len
		besch->zoomable = decode_uint8(p);
		besch->bild_nr = IMG_EMPTY;

		skip_reading_pixels_if_no_graphics;
		uint16* dest = besch->data;
		if (besch->h > 0) {
			for (uint i = 0; i < besch->len; i++) {
				*dest++ = decode_uint16(p);
			}
		}
	}
	else {
		dbg->fatal("image_reader_t::read_node()","illegal versions %d", version );
	}

#if COLOUR_DEPTH == 0
adjust_image:
	// reset image parameters, but only for non-empty images
	if(  besch->h > 0  ) {
		besch->h = 1;
	}
	if(  besch->w > 0  ) {
		besch->w = 1;
	}
	if(  besch->len > 0  ) {
		besch->len = 4;
	}
	besch->x = 0;
	besch->y = 0;
#endif

	// check for left corner
	if(version<2  &&  besch->h>0) {
		// find left left border
		uint16 left = 255;
		uint16 *dest = besch->data;
		for( uint8 y=0;  y<besch->h;  y++  ) {
			if(*dest<left) {
				left = *dest;
			}
			// skip rest of the line
			do {
				dest++;
				dest += *dest + 1;
			} while (*dest);
			dest++;	// skip trailing zero
		}

		if(left<besch->x) {
			dbg->warning( "image_reader_t::read_node()","left(%i)<x(%i) (may be intended)", left, besch->x );
		}

		dest = besch->data;
		for( uint8 y=0;  y<besch->h;  y++  ) {
			*dest -= left;
			// skip rest of the line
			do {
				dest++;
				dest += *dest + 1;
			} while (*dest);
			dest++;	// skip trailing zero
		}
	}

	if (besch->len != 0) {
		// get the adler hash (since we have zlib on board anyway ... )
		bool do_register_image = true;
		uint32 adler = adler32(0L, NULL, 0 );
		// remember len is sizeof(uint16)!
		adler = adler32(adler, (const Bytef *)(besch->data), besch->len*2 );
		static inthashtable_tpl<uint32, bild_besch_t *> images_adlers;
		bild_besch_t *same = images_adlers.get(adler);
		if (same) {
			// same checksum => if same then skip!
			bild_besch_t const& a = *besch;
			bild_besch_t const& b = *same;
			do_register_image =
				a.x        != b.x        ||
				a.y        != b.y        ||
				a.w        != b.w        ||
				a.h        != b.h        ||
				a.zoomable != b.zoomable ||
				a.len      != b.len      ||
				memcmp(a.data, b.data, sizeof(*a.data) * a.len) != 0;
		}
		// unique image here
		if(  do_register_image  ) {
			if(!same) {
				images_adlers.put(adler,besch);	// still with bild_nr == IMG_EMPTY!
			}
			// register image adds this image to the internal array maintained by simgraph??.cc
			register_image(besch);
		}
		else {
			// no need to load doubles ...
			delete besch;
			besch = same;
		}
	}

	return besch;
}
