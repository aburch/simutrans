#ifndef tunnelboden_h
#define tunnelboden_h

#include "boden.h"


class tunnelboden_t : public boden_t
{
protected:
	void calc_image_internal(const bool calc_only_snowline_change);

public:
	tunnelboden_t(loadsave_t *file, koord pos );
	tunnelboden_t(koord3d pos, slope_t::type slope_type) : boden_t(pos, slope_type) {}

	virtual void rdwr(loadsave_t *file);

	slope_t::type get_weg_hang() const { return ist_karten_boden() ? (slope_t::type)slope_t::flat : get_grund_hang(); }

	const char *get_name() const {return "Tunnelboden";}
	typ get_typ() const { return tunnelboden; }

	void info(cbuffer_t & buf, bool dummy = false) const;
};

#endif
