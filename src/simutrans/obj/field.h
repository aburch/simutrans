/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_FIELD_H
#define OBJ_FIELD_H


#include "simobj.h"
#include "../display/simimg.h"


class field_class_desc_t;
class fabrik_t;

class field_t : public obj_t
{
	fabrik_t *fab;
	const field_class_desc_t *desc;

public:
	field_t(const koord3d pos, player_t *player, const field_class_desc_t *desc, fabrik_t *fab);
	virtual ~field_t();

	const char* get_name() const OVERRIDE { return "Field"; }
	typ get_typ() const OVERRIDE { return obj_t::field; }

	image_id get_image() const OVERRIDE;

	/// @copydoc obj_t::show_info
	void show_info() OVERRIDE;

	/**
	 * @return NULL when OK, otherwise an error message
	 */
	const char * is_deletable(const player_t *) OVERRIDE;

	void cleanup(player_t *player) OVERRIDE;

	fabrik_t* get_factory() const { return fab; }
};

#endif
