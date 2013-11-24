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
protected:
	void calc_bild_internal();
	ribi_t::ribi ribi;

public:
	wasser_t(loadsave_t *file, koord pos ) : grund_t(koord3d(pos,0) ), ribi(ribi_t::keine) { rdwr(file); }
	wasser_t(koord3d pos) : grund_t(pos), ribi(ribi_t::keine) {}

	inline bool ist_wasser() const { return true; }

	// returns correct directions for water and none for the rest ...
	ribi_t::ribi get_weg_ribi(waytype_t typ) const { return (typ==water_wt) ? ribi : (ribi_t::ribi)ribi_t::keine; }
	ribi_t::ribi get_weg_ribi_unmasked(waytype_t typ) const  { return (typ==water_wt) ? ribi : (ribi_t::ribi)ribi_t::keine; }

	const char *get_name() const {return "Wasser";}
	grund_t::typ get_typ() const {return wasser;}

	// map rotation
	void rotate90();

	// static stuff from here on for water animation
	static int stage;
	static bool change_stage;

	static void prepare_for_refresh();
};

#endif
