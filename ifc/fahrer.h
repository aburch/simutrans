/* fahrer.h
 *
 * Interface für Verbindung von Fahrzeugen mit der Route
 *
 * 15.01.00, Hj. Malthaner
 */

#ifndef fahrer_h
#define fahrer_h

#ifndef boden_wege_weg_h
#include "../boden/wege/weg.h"
#endif

class grund_t;

/**
 * Interface für Verbindung von Fahrzeugen mit der Route.
 *
 * @author Hj. Malthaner, 15.01.00
 * @version $Revision: 1.8 $
 */
class fahrer_t
{
public:
	virtual bool ist_befahrbar(const grund_t* ) const = 0;

	/**
	 * Ermittelt die für das Fahrzeug geltenden Richtungsbits,
	 * abhängig vom Untergrund.
	 *
	 * @author Hj. Malthaner, 03.01.01
	 */
	virtual ribi_t::ribi gib_ribi(const grund_t* ) const = 0;

	virtual weg_t::typ gib_wegtyp() const = 0;

	// how expensive to go here (for way search) with the maximum convoi speed as second parameter
	virtual int gib_kosten(const grund_t *,const uint32) const = 0;

	// returns true for the way search to an unknown target.
	virtual bool ist_ziel(const grund_t *) const = 0;
};


#endif
