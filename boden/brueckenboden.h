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
	brueckenboden_t(loadsave_t *file, koord pos ) : grund_t(koord3d(pos,0) ) { rdwr(file); }
	brueckenboden_t(koord3d pos, int grund_hang, int weg_hang);

	virtual void rdwr(loadsave_t *file);

	// map rotation
	virtual void rotate90();

	virtual sint8 get_weg_yoff() const;

	hang_t::typ get_weg_hang() const { return weg_hang; }

	const char *get_name() const {return "Brueckenboden";}
	typ get_typ() const { return brueckenboden; }

	void info(cbuffer_t & buf) const;
};

#endif
