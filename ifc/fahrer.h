/*
 * Interface für Verbindung von Fahrzeugen mit der Route
 *
 * 15.01.00, Hj. Malthaner
 */

#ifndef fahrer_h
#define fahrer_h


class grund_t;

/**
 * Interface für Verbindung von Fahrzeugen mit der Route.
 *
 * @author Hj. Malthaner, 15.01.00
 */
class fahrer_t
{
public:
	virtual ~fahrer_t() {}

	virtual bool ist_befahrbar(const grund_t* ) const = 0;

	/**
	 * Ermittelt die für das Fahrzeug geltenden Richtungsbits,
	 * abhängig vom Untergrund.
	 *
	 * @author Hj. Malthaner, 03.01.01
	 */
	virtual ribi_t::ribi get_ribi(const grund_t* ) const = 0;

	virtual waytype_t get_waytype() const = 0;

	// how expensive to go here (for way search) with the maximum convoi speed as second parameter
	virtual int get_kosten(const grund_t *,const uint32) const = 0;

	// returns true for the way search to an unknown target.
	// first is current ground, second is starting ground
	virtual bool ist_ziel(const grund_t *,const grund_t *) const = 0;
};

#endif
