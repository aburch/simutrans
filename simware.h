#ifndef simware_h
#define simware_h

#include "dataobj/koord.h"

struct ware_t;
class ware_besch_t;


/**
 * Eine Klasse zur Verwaltung von Informationen ueber Fracht und Waren.
 * Diese Klasse darf gesubclassed werden, da sie new() und delete()
 * redefiniert!
 *
 * @author Hj. Malthaner
 */
struct ware_t
{
public:
    // ein paar waren sind fix
    enum typ {Pax=0, Post=1, None=2};

private:
    const ware_besch_t *type;

    /**
     * Koordinate der Zielhaltestelle
     * @author Hj. Malthaner
     */
    koord ziel;

    /**
     * Koordinte des nächsten Zwischenstops
     * @author Hj. Malthaner
     */
    koord zwischenziel;

    /**
     * die engültige Zielposition,
     * das ist i.a. nicht die Zielhaltestellenposition
     * @author Hj. Malthaner
     */
    koord zielpos;

public:
    sint32 menge;
    sint32 max;

    koord gib_ziel() const {return ziel;};
    void setze_ziel(koord ziel) {this->ziel = ziel;};

    koord gib_zwischenziel() const {return zwischenziel;};
    void setze_zwischenziel(koord zwischenziel) {this->zwischenziel = zwischenziel;};

    koord gib_zielpos() const {return zielpos;};
    void setze_zielpos(koord zielpos) {this->zielpos = zielpos;};

    ware_t();
    ware_t(const ware_besch_t *typ);
    ware_t(loadsave_t *file);

    ~ware_t();

    /**
     * gibt den nicht-uebersetzten warennamen zurück
     * @author Hj. Malthaner
     */
    const char *gib_name() const;
    const char *gib_mass() const;
    int gib_preis() const;
    int gib_catg() const;


    inline const ware_besch_t *gib_typ() const { return type; };
    inline void setze_typ( const ware_besch_t *type ) { this->type = type; };

    void rdwr(loadsave_t *file);

    int operator==(const ware_t &w) {
	return type  == w.type  &&
               menge == w.menge &&
               max   == w.max   &&
               ziel  == w.ziel  &&
               zwischenziel == w.zwischenziel &&
               zielpos      == w.zielpos;
    }

    int operator!=(const ware_t &w) {
		return ! (*this == w);
    }

};

#endif
