#ifndef tunnelboden_h
#define tunnelboden_h

#include "boden.h"


class tunnelboden_t : public boden_t
{
protected:
	void calc_image_internal(const bool calc_only_snowline_change) OVERRIDE;

public:
	tunnelboden_t(loadsave_t *file, koord pos );
	tunnelboden_t(koord3d pos, slope_t::type slope_type) : boden_t(pos, slope_type) {}

	void rdwr(loadsave_t *file) OVERRIDE;

	slope_t::type get_weg_hang() const OVERRIDE { return ist_karten_boden() ? (slope_t::type)slope_t::flat : get_grund_hang(); }

	const char *get_name() const OVERRIDE {return "Tunnelboden";}
	typ get_typ() const OVERRIDE { return tunnelboden; }

	void info(cbuffer_t & buf) const OVERRIDE;
};

#endif
