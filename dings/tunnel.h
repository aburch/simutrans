#ifndef dings_tunnel_h
#define dings_tunnel_h

#include "../simdings.h"
#include "../simimg.h"

class tunnel_besch_t;

class tunnel_t : public ding_t
{
	const tunnel_besch_t *besch;
	image_id after_bild;
	bool clean_up:1;

public:
	tunnel_t(karte_t *welt, loadsave_t *file);
	tunnel_t(karte_t *welt, koord3d pos, spieler_t *sp, const tunnel_besch_t *besch);

	const char *gib_name() const {return "Tunnelmuendung";}
	enum ding_t::typ gib_typ() const {return tunnel;}

	void calc_bild();

	virtual image_id gib_after_bild() const { return after_bild; }

	const tunnel_besch_t *gib_besch() const { return besch; }

    void zeige_info() {} // show no info

	virtual void rdwr(loadsave_t *file);

	virtual void laden_abschliessen();

	/**
	 * Normally step is disabled; enable only for removal!
	 * @author prissi
	 */
	virtual bool step(long /*delta_t*/) {return false;}
};

#endif
