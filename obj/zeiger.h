#ifndef obj_zeiger_h
#define obj_zeiger_h

#include "../simobj.h"
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
	koord area;
	bool center;
	/// images
	image_id image, after_bild;

public:
	zeiger_t(loadsave_t *file);
	zeiger_t(koord3d pos, player_t *player);

	void change_pos(koord3d k);

	const char *get_name() const {return "Zeiger";}
#ifdef INLINE_OBJ_TYPE
#else
	typ get_typ() const { return zeiger; }
#endif

	/**
	* Set area to be marked around cursor
	* @param area size of marked area
	* @param center true if cursor is centered within marked area
	*/
	void set_area( koord area, bool center );

	/// set back image
	void set_bild( image_id b );
	/// get back image
	image_id get_image() const {return image;}

	/// set front image
	void set_after_bild( image_id b );
	/// get front image
	image_id get_front_image() const {return after_bild;}
};

#endif
