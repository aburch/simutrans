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
	static const way_desc_t *default_narrowgauge;

	narrowgauge_t() : schiene_t() { set_desc(default_narrowgauge); }

	/**
	 * File loading constructor.
	 * @author prissi
	 */
	narrowgauge_t(loadsave_t *file);

	waytype_t get_waytype() const OVERRIDE {return narrowgauge_wt;}

	void rdwr(loadsave_t *file) OVERRIDE;
};

#endif
