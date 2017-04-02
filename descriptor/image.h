/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __BILD_BESCH_H
#define __BILD_BESCH_H

#include "../display/simgraph.h"
#include "../display/simimg.h"

#include "obj_desc.h"

// number of special colors
#define SPECIAL (31)

//#define TRANSPARENT 0x808088
#define SPECIAL_TRANSPARENT (0x00E7FFFF)

/*
 *  Autor:
 *      Volker Meyer
 *
 *  Description:
 *      Description eines Bildes.
 *
 *  Child nodes:
 *	(none)
 */
class image_t : public obj_desc_t
{
public:
	static const uint32 rgbtab[SPECIAL];
	static const uint8  special_pal[224*3];

	// returns next matching color to an rgb
	static COLOR_VAL get_index_from_rgb( uint8 r, uint8 g, uint8 b );

	size_t len;       ///< length of data[] in PIXVAL units
	scr_coord_val x;  ///< x offset of data[] image
	scr_coord_val y;  ///< y offset of data[] image
	scr_coord_val w;  ///< width of data[] image
	scr_coord_val h;  ///< height of data[] image
	image_id imageid; ///< set by register_image()
	uint8 zoomable;   ///< some images may not be zoomed i.e. icons
	PIXVAL *data;     ///< RLE encoded image data

	image_t(size_t len_ = 0) : data(NULL)
	{
		if (len_) {
			alloc(len_);
		}
	}

	~image_t()
	{
		delete[] data;
	}

	void alloc(size_t len_)
	{
		delete[] data;
		data = new PIXVAL[len_];
		len = len_;
	}

	static image_t* copy_image(const image_t& other);

	const image_t* get_pic() const { return this; }

	uint16 const* get_data() const { return data; }
	uint16*       get_data() { return data; }
	
	image_id get_id() const { return imageid; }

	/* rotate_image_data - produces a (rotated) bild_desc
	 * only rotates by 90 degrees or multiples thereof, and assumes a square image
	 * Otherwise it will only succeed for angle=0;
	 */
	image_t* copy_rotate(const sint16 angle) const;

	image_t* copy_flipvertical() const;
	image_t* copy_fliphorizontal() const;

	static image_t* create_single_pixel();

	void register_image() { ::register_image(this); }

	// decodes this image into a 32 bit bitmap with width target_width
	void decode_img( sint16 xoff, sint16 yoff, uint32 *target, uint32 target_width, uint32 target_height ) const;

	friend class image_reader_t;
};

#endif
