#ifndef boden_wege_strasse_h
#define boden_wege_strasse_h

#include "weg.h"

/**
 * Auf der Strasse können Autos fahren.
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.5 $
 */

class strasse_t : public weg_t
{
public:
	static const weg_besch_t *default_strasse;

	strasse_t(karte_t *welt, loadsave_t *file);
	strasse_t(karte_t *welt);
	strasse_t(karte_t *welt,int top_speed);

	/**
	 * @return Infotext zur Schiene
	 * @author Hj. Malthaner
	 */
	void info(cbuffer_t & buf) const;

	inline waytype_t gib_waytype() const {return road_wt;}

	void setze_gehweg(bool janein);

	virtual void rdwr(loadsave_t *file);
};

#endif
