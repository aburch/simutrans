#ifndef boden_fundament_h
#define boden_fundament_h

#include "grund.h"


/**
 * The foundation serves as base ground for all buildings in Simutrans.
 *
 * @author Hj. Malthaner
 */
class fundament_t : public grund_t
{
protected:
	/**
	* Das Fundament hat immer das gleiche Bild.
	* @author Hj. Malthaner
	*/
	void calc_image_internal(const bool calc_only_snowline_change);

public:
	fundament_t(loadsave_t *file, koord pos );
	fundament_t(koord3d pos,slope_t::type hang, bool build_up = true);

	/**
	* The foundation is called 'Fundament'.
	* @return 'Fundament'.
	* @author Hj. Malthaner
	*/
	const char *get_name() const {return "Fundament";}

	typ get_typ() const { return fundament; }

	bool set_slope(slope_t::type) { slope = 0; return false; }
};

#endif
