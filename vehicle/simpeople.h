#ifndef simpeople_h
#define simpeople_h

#include "simverkehr.h"

class fussgaenger_besch_t;

/**
 * Pedestrians also are road users.
 *
 * @author Hj. Malthaner
 * @see verkehrsteilnehmer_t
 */
class fussgaenger_t : public verkehrsteilnehmer_t
{
private:
	static stringhashtable_tpl<const fussgaenger_besch_t *> table;

private:
	const fussgaenger_besch_t *besch;

protected:
	void rdwr(loadsave_t *file);

	void calc_bild();

	/**
	 * Creates pedestrian at position given by @p gr.
	 * Does not add pedestrian to the tile!
	 */
	fussgaenger_t(grund_t *gr);

public:
	fussgaenger_t(loadsave_t *file);

	virtual ~fussgaenger_t();

	const fussgaenger_besch_t *get_besch() const { return besch; }

	const char *get_name() const {return "Fussgaenger";}
	typ get_typ() const { return fussgaenger; }

	bool sync_step(long delta_t);

	// prissi: always free
	virtual bool ist_weg_frei() { return true; }
	virtual bool hop_check() { return true; }
	virtual grund_t* hop();

	// class register functions
	static bool register_besch(const fussgaenger_besch_t *besch);
	static bool alles_geladen();

	static void erzeuge_fussgaenger_an(koord3d k, int &anzahl);
};

#endif
