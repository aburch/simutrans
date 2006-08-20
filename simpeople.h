#ifndef simpeople_h
#define simpeople_h

#include "simverkehr.h"
//#include "tpl/slist_mit_gewichten_tpl.h"
template<class T> class slist_mit_gewichten_tpl;

class fussgaenger_besch_t;

/**
 * Fuﬂg‰nger sind auch Verkehrsteilnehmer.
 *
 * @author Hj. Malthaner
 * @see verkehrsteilnehmer_t
 */
class fussgaenger_t : public verkehrsteilnehmer_t
{
private:
	static slist_mit_gewichten_tpl<const fussgaenger_besch_t *> liste;
	static stringhashtable_tpl<const fussgaenger_besch_t *> table;

public:
	static int count;

private:
	static int strecke[8];

	int schritte;
	int sum;
	uint8 is_sync;

	const fussgaenger_besch_t *besch;

protected:
	void rdwr(loadsave_t *file);

public:

	fussgaenger_t(karte_t *welt, loadsave_t *file);
	fussgaenger_t(karte_t *welt, koord3d pos);

	~fussgaenger_t();

	static bool register_besch(const fussgaenger_besch_t *besch);
	static bool laden_erfolgreich();

	static int gib_anzahl_besch();

	const char *gib_name() const {return "Fussgaenger";};
	enum ding_t::typ gib_typ() const {return fussgaenger;};

	bool sync_step(long delta_t);

	// prissi: always free
	virtual bool ist_weg_frei() { return 1; };
	virtual bool hop_check() { return 1; };

	void calc_bild();
};


#endif
