#ifndef boden_fundament_h
#define boden_fundament_h

#include "grund.h"


/**
 * Das Fundament dient als Untergrund fuer alle Bauwerke in Simutrans.
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
	void calc_bild_internal();

public:
	fundament_t(loadsave_t *file, koord pos );
	fundament_t(koord3d pos, hang_t::typ hang);

	/**
	* Das Fundament heisst 'Fundament'.
	* @return gibt 'Fundament' zurueck.
	* @author Hj. Malthaner
	*/
	const char *get_name() const {return "Fundament";}

	typ get_typ() const { return fundament; }

	bool set_slope(hang_t::typ) { slope = 0; return false; }
};

#endif
