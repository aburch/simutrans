#ifndef boden_wasser_h
#define boden_wasser_h

#include "grund.h"

/**
 * Der Wasser-Untergrund modelliert Fluesse und Seen in Simutrans.
 *
 * @author Hj. Malthaner
 */

class wasser_t : public grund_t
{
public:
	wasser_t(karte_t *welt, loadsave_t *file) : grund_t (welt) { rdwr(file); }
	wasser_t(karte_t *welt, koord3d pos) : grund_t(welt, pos) {}

	inline bool ist_wasser() const { return true; }

	// returns all directions for waser and none for the rest ...
	ribi_t::ribi gib_weg_ribi(waytype_t typ) const { return (typ==water_wt) ? ribi_t::alle :ribi_t::keine; }
	ribi_t::ribi gib_weg_ribi_unmasked(waytype_t typ) const  { return (typ==water_wt) ? ribi_t::alle :ribi_t::keine; }

	/**
	 * Öffnet ein Info-Fenster für diesen Boden
	 * @author Hj. Malthaner
	 */
	virtual bool zeige_info();

	void calc_bild();

	const char *gib_name() const {return "Wasser";}
	grund_t::typ gib_typ() const {return wasser;}

	void * operator new(size_t s);
	void operator delete(void *p);
};

#endif
