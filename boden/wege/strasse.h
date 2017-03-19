#ifndef boden_wege_strasse_h
#define boden_wege_strasse_h

#include "weg.h"
#include "../../tpl/minivec_tpl.h"

class fabrik_t;
class gebaeude_t;

/**
 * Cars are able to drive on roads.
 *
 * @author Hj. Malthaner
 */
class strasse_t : public weg_t
{
public:
	static const way_desc_t *default_strasse;

	minivec_tpl<gebaeude_t*> connected_buildings;

	strasse_t(loadsave_t *file);
	strasse_t();

	//inline waytype_t get_waytype() const {return road_wt;}

	void set_gehweg(bool janein);

	virtual void rdwr(loadsave_t *file);
};

#endif
