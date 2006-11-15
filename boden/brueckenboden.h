#ifndef brueckenboden_h
#define brueckenboden_h

#include "grund.h"

class brueckenboden_t : public grund_t
{
	uint8 weg_hang;

public:
	brueckenboden_t(karte_t *welt, loadsave_t *file);
	brueckenboden_t(karte_t *welt, koord3d pos, int grund_hang, int weg_hang);
	virtual ~brueckenboden_t() {}

	virtual void rdwr(loadsave_t *file);

	virtual int gib_weg_yoff() const;

	bool setze_grund_hang(hang_t::typ /*sl*/) const { return false; }
	hang_t::typ gib_weg_hang() const { return weg_hang; }

	void calc_bild();

	/**
	* ground info, needed for stops
	* @author prissi
	*/
	virtual bool zeige_info();

	const char *gib_name() const {return "Brueckenboden";}
	enum grund_t::typ gib_typ() const {return brueckenboden;}

	void * operator new(size_t s);
	void operator delete(void *p);
};

#endif
