#ifndef dings_field_h
#define dings_field_h

#include "../tpl/stringhashtable_tpl.h"

#include "../simdings.h"
#include "../simworld.h"
#include "../simimg.h"


class loadsave_t;
class fabrik_t;

class field_t : public ding_t
{
	fabrik_t *fab;
	const field_besch_t *besch;

public:
	field_t(karte_t *welt, const koord3d pos, spieler_t *sp, const field_besch_t *besch, fabrik_t *fab);
	virtual ~field_t();

	const char* get_name() const { return "Field"; }
	enum ding_t::typ get_typ() const { return ding_t::field; }

	image_id get_bild() const;

	/**
	 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	 * Beobachtungsfenster angezeigt wird.
	 * @author Hj. Malthaner
	 */
	void zeige_info();

	/**
	 * @returns NULL wenn OK, ansonsten eine Fehlermeldung
	 * @author Hj. Malthaner
	 */
	const char * ist_entfernbar(const spieler_t *);

	void entferne(spieler_t *sp);

	// static routines from here
private:
	static stringhashtable_tpl<const field_besch_t *> besch_table;

public:
	static void register_besch(field_besch_t *besch, const char *name);

	static const field_besch_t *get_besch(const char *name);
};

#endif
