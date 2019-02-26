#ifndef boden_wege_strasse_h
#define boden_wege_strasse_h

#include "weg.h"

/**
 * Cars are able to drive on roads.
 *
 * @author Hj. Malthaner
 */
class strasse_t : public weg_t
{
public:
	static const way_desc_t *default_strasse;

	strasse_t(loadsave_t *file);
	strasse_t();

	inline waytype_t get_waytype() const OVERRIDE {return road_wt;}

	void set_gehweg(bool janein);

	void rdwr(loadsave_t *file) OVERRIDE;
};

#endif
