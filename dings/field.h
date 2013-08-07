#ifndef dings_field_h
#define dings_field_h

#include "../simdings.h"
#include "../display/simimg.h"


class field_class_besch_t;
class fabrik_t;

class field_t : public ding_t
{
	fabrik_t *fab;
	const field_class_besch_t *besch;

public:
	field_t(karte_t *welt, const koord3d pos, spieler_t *sp, const field_class_besch_t *besch, fabrik_t *fab);
	virtual ~field_t();

	const char* get_name() const { return "Field"; }
	typ get_typ() const { return ding_t::field; }

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
