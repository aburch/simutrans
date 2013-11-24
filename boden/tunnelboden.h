#ifndef tunnelboden_h
#define tunnelboden_h

#include "boden.h"


class tunnelboden_t : public boden_t
{
protected:
	void calc_bild_internal();

public:
	tunnelboden_t(loadsave_t *file, koord pos );
	tunnelboden_t(koord3d pos, hang_t::typ hang_typ) : boden_t(pos, hang_typ) {}

	virtual void rdwr(loadsave_t *file);

	hang_t::typ get_weg_hang() const { return ist_karten_boden() ? (hang_t::typ)hang_t::flach : get_grund_hang(); }

	const char *get_name() const {return "Tunnelboden";}
	typ get_typ() const { return tunnelboden; }

	void info(cbuffer_t & buf) const;
};

#endif
