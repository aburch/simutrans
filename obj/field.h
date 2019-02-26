#ifndef obj_field_h
#define obj_field_h

#include "../simobj.h"
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

	/**
	 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	 * Beobachtungsfenster angezeigt wird.
	 * @author Hj. Malthaner
	 */
	void show_info() OVERRIDE;

	/**
	 * @return NULL when OK, otherwise an error message
	 * @author Hj. Malthaner
	 */
	const char * is_deletable(const player_t *) OVERRIDE;

	void cleanup(player_t *player) OVERRIDE;
};

#endif
