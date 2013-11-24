#ifndef obj_field_h
#define obj_field_h

#include "../simobj.h"
#include "../display/simimg.h"


class field_class_besch_t;
class fabrik_t;

class field_t : public obj_t
{
	fabrik_t *fab;
	const field_class_besch_t *besch;

public:
	field_t(const koord3d pos, spieler_t *sp, const field_class_besch_t *besch, fabrik_t *fab);
	virtual ~field_t();

	const char* get_name() const { return "Field"; }
	typ get_typ() const { return obj_t::field; }

	image_id get_bild() const;

	/**
	 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	 * Beobachtungsfenster angezeigt wird.
	 * @author Hj. Malthaner
	 */
	void zeige_info();

	/**
	 * @return NULL wenn OK, ansonsten eine Fehlermeldung
	 * @author Hj. Malthaner
	 */
	const char * ist_entfernbar(const spieler_t *);

	void entferne(spieler_t *sp);
};

#endif
