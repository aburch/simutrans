#ifndef brueckenboden_h
#define brueckenboden_h

#include "grund.h"

class brueckenboden_t : public grund_t
{
private:
	uint8 weg_hang;

protected:
	void calc_bild_internal();

public:
	brueckenboden_t(karte_t *welt, loadsave_t *file);
	brueckenboden_t(karte_t *welt, koord3d pos, int grund_hang, int weg_hang);

	virtual void rdwr(loadsave_t *file);

	// map rotation
	virtual void rotate90();

	virtual sint8 gib_weg_yoff() const;

	hang_t::typ gib_weg_hang() const { return weg_hang; }

	/**
	* ground info, needed for stops
	* @author prissi
	*/
	virtual bool zeige_info();

	const char *gib_name() const {return "Brueckenboden";}
	enum grund_t::typ gib_typ() const {return brueckenboden;}
};

#endif
