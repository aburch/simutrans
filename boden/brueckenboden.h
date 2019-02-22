#ifndef brueckenboden_h
#define brueckenboden_h

#include "grund.h"

class brueckenboden_t : public grund_t
{
private:
	uint8 weg_hang;

protected:
	void calc_image_internal(const bool calc_only_snowline_change) OVERRIDE;

public:
	brueckenboden_t(loadsave_t *file, koord pos ) : grund_t(koord3d(pos,0) ) { rdwr(file); }
	brueckenboden_t(koord3d pos, int grund_hang, int weg_hang);

	void rdwr(loadsave_t *file) OVERRIDE;

	// map rotation
	void rotate90() OVERRIDE;

	sint8 get_weg_yoff() const OVERRIDE;

	slope_t::type get_weg_hang() const OVERRIDE { return weg_hang; }

	const char *get_name() const OVERRIDE {return "Brueckenboden";}
	typ get_typ() const OVERRIDE { return brueckenboden; }

	void info(cbuffer_t & buf) const OVERRIDE;
};

#endif
