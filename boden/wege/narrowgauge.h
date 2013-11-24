/*
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef boden_wege_narrowgauge_h
#define boden_wege_narrowgauge_h


#include "schiene.h"


/**
 * derived from schiene, because signals will behave similar
 */
class narrowgauge_t : public schiene_t
{
public:
	static const weg_besch_t *default_narrowgauge;

	narrowgauge_t() : schiene_t() { set_besch(default_narrowgauge); }

	/**
	 * File loading constructor.
	 * @author prissi
	 */
	narrowgauge_t(loadsave_t *file);

	virtual waytype_t get_waytype() const {return narrowgauge_wt;}

	void rdwr(loadsave_t *file);
};

#endif
