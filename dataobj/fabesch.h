/* fabesch.h
 *
 * Fabrikbeschreibungen, genutzt vom Fabrikbauer
 * Hj. Malthaner, 2000
 */

#ifndef dataobj_fabesch_h
#define dataobj_fabesch_h

#include "../tpl/vector_tpl.h"

struct ware_t;

/**
 * Einige Fabriken rauchen. Dies sind die Raucherbeschreibungsdaten.
 * @author Hj. Malthaner
 * @version $Revision: 1.9 $
 */
class rabesch_t
{
private:
    koord pos_off;
    koord xy_off;

    int zeitmaske;
    int rauch;

public:

    koord gib_pos_off() const {return pos_off;};
    koord gib_xy_off() const {return xy_off;};
    int   gib_zeitmaske() const {return zeitmaske;};
    int   gib_rauch() const {return rauch;};

    rabesch_t(char * dataline);
};


/**
 * Fabrikbeschreibungen, genutzt vom Fabrikbauer.
 * @author Hj. Malthaner
 */
class fabesch_t
{
public:
    enum platzierung {Land, Wasser, Stadt};

private:
    char * name;
    int  produktivitaet;
    int  bereich;
    int  gewichtung;	// Wie wahrscheinlich soll der Bau sein?

    /**
     * Color to use in the relief map
     * @author Hj. Malthaner
     */
    int  kennfarbe;

    enum platzierung platzierung;

    vector_tpl<ware_t> *eingang;
    vector_tpl<ware_t> *ausgang;

    rabesch_t * raucherdaten;

    int anz_lieferanten;

    bool laden(FILE *file);

public:
    const char * gib_name() const {return name;};
    int gib_produktivitaet() const {return produktivitaet;};

    /**
     * Color to use in the relief map
     * @author Hj. Malthaner
     */
    int gib_kennfarbe() const {return kennfarbe;};


    fabesch_t(FILE *file) {laden(file);};

    ~fabesch_t();


    /**
     * Schwankungsbereich (+) für Produktivität
     * @author Hj. Malthaner
     */
    int gib_bereich() const {return bereich;};

    int gib_gewichtung() const {return gewichtung;};

    enum platzierung gib_platzierung() const {return platzierung;};

    vector_tpl<ware_t> *gib_eingang() const {return eingang;};
    vector_tpl<ware_t> *gib_ausgang() const {return ausgang;};

    rabesch_t *gib_raucherdaten() const {return raucherdaten;};


    int gib_anz_lieferanten() const {return anz_lieferanten;};
};


#endif
