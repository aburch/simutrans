/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_IMAGE_H
#define DESCRIPTOR_IMAGE_H


#include "../display/simgraph.h"
#include "../display/simimg.h"
#include "obj_desc.h"


// number of special colors
#define SPECIAL (31)

#define SPECIAL_TRANSPARENT (0x00E7FFFF)



/**
 * Data of one image
 *
 * Child nodes:
 *  (none)
 */
class image_t : public obj_desc_t
{
public:
	static const uint32 rgbtab[SPECIAL];

	size_t len;       ///< length of data[] in PIXVAL units
	scr_coord_val x;  ///< x offset of data[] image
	scr_coord_val y;  ///< y offset of data[] image
	scr_coord_val w;  ///< width of data[] image
	scr_coord_val h;  ///< height of data[] image
	image_id imageid; ///< set by register_image()
	uint8 zoomable;   ///< some images may not be zoomed i.e. icons
	PIXVAL *data;     ///< RLE encoded image data

	image_t(size_t len_=0) : data(NULL)
	{
		if (len_) {
			alloc(len_);
		}
	}

	~image_t()
	{
		delete [] data;
	}

	void alloc(size_t len_)
	{
		delete [] data;
		data = new PIXVAL[len_];
		len = len_;
	}

	static image_t* copy_image(const image_t& other);

	const image_t* get_pic() const { return this; }

	uint16 const* get_data() const { return data; }
	uint16*       get_data()       { return data; }

	image_id get_id() const { return imageid; }

	/* rotate_image_data - produces a (rotated) image
	 * only rotates by 90 degrees or multiples thereof, and assumes a square image
	 * Otherwise it will only succeed for angle=0;
	 */
	image_t* copy_rotate(const sint16 angle) const;

	image_t* copy_flipvertical() const;
	image_t* copy_fliphorizontal() const;

	static image_t* create_single_pixel();

	void register_image() { ::register_image(this); }

private:
	friend class image_reader_t;
};

#endif
