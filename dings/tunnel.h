#ifndef dings_tunnel_h
#define dings_tunnel_h

#include "../simdings.h"
#include "../simimg.h"

class tunnel_besch_t;

class tunnel_t : public ding_no_info_t
{
private:
	const tunnel_besch_t *besch;
	image_id bild;
	image_id after_bild;
	uint8 broad_type; // Is this a broad tunnel mouth?

public:
	tunnel_t(karte_t *welt, loadsave_t *file);
	tunnel_t(karte_t *welt, koord3d pos, spieler_t *sp, const tunnel_besch_t *besch);

	const char *get_name() const {return "Tunnelmuendung";}
	typ get_typ() const { return tunnel; }

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const;

	void calc_bild();

	void set_bild( image_id b );
	void set_after_bild( image_id b );
	image_id get_bild() const {return bild;}
	image_id get_after_bild() const { return after_bild; }

	const tunnel_besch_t *get_besch() const { return besch; }

	void set_besch( const tunnel_besch_t *_besch ) { besch = _besch; }

	void rdwr(loadsave_t *file);

	void laden_abschliessen();

	void entferne(spieler_t *sp);

	bool check_season( const long ) { calc_bild(); return true; };

	uint8 get_broad_type() const { return broad_type; };
	/**
	 * @return NULL wenn OK, ansonsten eine Fehlermeldung
	 * @author Hj. Malthaner
	 */
	virtual const char *ist_entfernbar(const spieler_t *sp);
};

#endif
