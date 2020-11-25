/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_ZEIGER_H
#define OBJ_ZEIGER_H


#include "simobj.h"
#include "../simtypes.h"
#include "../display/simimg.h"

/** @file zeiger.h object to mark tiles */

/**
 * These objects mark tiles for tools. It marks the current tile
 * pointed to by mouse pointer (and possibly an area around it).
 */
class zeiger_t : public obj_no_info_t
{
private:
	koord area, offset;
	/// images
	image_id image, foreground_image;

public:
	zeiger_t(loadsave_t *file);
	zeiger_t(koord3d pos, player_t *player);

	void change_pos(koord3d k);

	const char *get_name() const OVERRIDE {return "Zeiger";}
	typ get_typ() const OVERRIDE { return zeiger; }

	/**
	 * Set area to be marked around cursor
	 * @param area size of marked area
	 * @param center true if cursor is centered within marked area
	 * @param offset if center==false then cursor is at position @p offset
	 */
	void set_area( koord area, bool center, koord offset = koord(0,0) );

	/// set back image
	void set_image( image_id b );
	/// get back image
	image_id get_image() const OVERRIDE {return image;}

	/// set front image
	void set_foreground_image( image_id b );
	/// get front image
	image_id get_front_image() const OVERRIDE {return foreground_image;}

	bool has_managed_lifecycle() const OVERRIDE;
};

#endif
