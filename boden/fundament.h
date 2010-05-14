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
	fundament_t(karte_t *welt, loadsave_t *file, koord pos );
	fundament_t(karte_t *welt, koord3d pos,hang_t::typ hang);

	/**
	* Das Fundament heisst 'Fundament'.
	* @return gibt 'Fundament' zurueck.
	* @author Hj. Malthaner
	*/
	const char *get_name() const {return "Fundament";}

	typ get_typ() const { return fundament; }

	/**
	* Auffforderung, ein Infofenster zu öffnen.
	* @author Hj. Malthaner
	*/
	virtual bool zeige_info();

	bool set_slope(hang_t::typ) { slope = 0; return false; }
};

#endif
