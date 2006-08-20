#ifndef fahrplan_h
#define fahrplan_h

#include "linieneintrag.h"

#include "../tpl/minivec_tpl.h"


// forward decl

class spieler_t;
class karte_t;
class zeiger_t;
class grund_t;
class planquadrat_t;

// class decl

/**
 * Eine Klasse zur Speicherung von Fahrplänen in Simutrans.
 *
 * @author Hj. Malthaner
 */
class fahrplan_t
{
public:

  enum fahrplan_type {
    fahrplan = 0, autofahrplan = 1, zugfahrplan = 2, schifffahrplan = 3, airfahrplan = 4
  };

private:
//    array_tpl<zeiger_t *> stops;

    bool abgeschlossen;

protected:

    fahrplan_type type;

	/*
	 * initializes the fahrplan object
	 * is shared by fahrplan_t constructors
	 * @author hsiegeln
	 */
	void init();

    /**
     * sollte eine Fehlermeldung ausgeben, wenn halt nicht erlaubt ist
     * @author Hj. Malthaner
     */
    virtual void zeige_fehlermeldung(karte_t *) const {};

public:
    /**
     * der allgemeine Fahrplan erlaubt haltestellen überall.
     * diese Methode sollte in den unterklassen redefiniert werden.
     * @author Hj. Malthaner
     */
    virtual bool ist_halt_erlaubt(const grund_t *) const {return true;};

    minivec_tpl<struct linieneintrag_t> eintrag;
    short aktuell;

    int maxi() const { return eintrag.get_count(); };

    fahrplan_type get_type() const {return type;};

    /**
     * get current stop of fahrplan
     * @author hsiegeln
     */
    int get_aktuell() const { return aktuell; };

	/**
	 * set the current stop of the fahrplan
	 * if new value is bigger than stops available, the max stop will be used
	 * @author hsiegeln
	 */
    void set_aktuell(int new_aktuell) { aktuell = (unsigned)new_aktuell>=eintrag.get_count() ? eintrag.get_count()-1 : new_aktuell; };

    inline bool ist_abgeschlossen() const {return abgeschlossen;};
    void eingabe_abschliessen() {abgeschlossen = true;};
    void eingabe_beginnen() {abgeschlossen = false;};

    fahrplan_t();

    /**
     * Copy constructor
     * @author hsiegeln
     */
    fahrplan_t(fahrplan_t *);

    virtual ~fahrplan_t();

    fahrplan_t(loadsave_t *file);


    /**
     * fügt eine koordinate an stelle aktuell in den Fahrplan ein
     * alle folgenden Koordinaten verschieben sich dadurch
     */
    bool insert(karte_t *welt, const grund_t *gr,int ladegrad=0);

    /**
     * hängt eine koordinate an den fahrplan an
     */
    bool append(karte_t *welt, const grund_t *gr,int ladegrad=0);

	// cleanup a schedule, removes double entries
	bool cleanup();

    /**
     * entfern eintrag[aktuell] aus dem fahrplan
     * alle folgenden Koordinaten verschieben sich dadurch
     */
    bool remove();

    void rdwr(loadsave_t *file);

    /**
     * if the passed in fahrplan matches "this", then return true
     * @author hsiegeln
     */
    bool matches(const fahrplan_t *fpl);

    /**
     * calculates a return way for this schedule
     * will add elements 1 to maxi-1 in reverse order to schedule
     * @author hsiegeln
     */
     void add_return_way(karte_t * welt);

     fahrplan_type get_type(karte_t * welt) const;

     virtual fahrplan_t * copy() { return new fahrplan_t(this); };

	// copy all entries from schedule src to this and adjusts aktuell
	void copy_from(const fahrplan_t *src);
};


/**
 * Eine Spezialisierung des Fahrplans die nur Stops auf Schienen
 * zuläßt.
 *
 * @author Hj. Malthaner
 */
class zugfahrplan_t : public fahrplan_t
{
protected:
    virtual bool ist_halt_erlaubt(const grund_t *) const;
    virtual void zeige_fehlermeldung(karte_t *) const;

public:
    zugfahrplan_t() : fahrplan_t() {type = zugfahrplan;};
    zugfahrplan_t(loadsave_t *file) : fahrplan_t(file) {type = zugfahrplan;};
    zugfahrplan_t(fahrplan_t * fpl) : fahrplan_t(fpl) {type = zugfahrplan;};
    fahrplan_t * copy() { return new zugfahrplan_t(this); };
};


/**
 * Eine Spezialisierung des Fahrplans die nur Stops auf Straßen
 * zuläßt.
 *
 * @author Hj. Malthaner
 */
class autofahrplan_t : public fahrplan_t
{
protected:
    virtual bool ist_halt_erlaubt(const grund_t *) const;
    virtual void zeige_fehlermeldung(karte_t *) const;

public:
    autofahrplan_t() : fahrplan_t() {type = autofahrplan;};
    autofahrplan_t(loadsave_t *file) : fahrplan_t(file) {type = autofahrplan;};
    autofahrplan_t(fahrplan_t * fpl) : fahrplan_t(fpl) {type = autofahrplan;};
    fahrplan_t * copy() { return new autofahrplan_t(this); };
};


/**
 * Eine Spezialisierung des Fahrplans die nur Stops auf Wasser
 * zuläßt.
 *
 * @author Hj. Malthaner
 */
class schifffahrplan_t : public fahrplan_t
{
protected:
    virtual bool ist_halt_erlaubt(const grund_t *) const;
    virtual void zeige_fehlermeldung(karte_t *) const;

public:
    schifffahrplan_t() : fahrplan_t() {type = schifffahrplan;};
    schifffahrplan_t(loadsave_t *file) : fahrplan_t(file) {type = schifffahrplan;};
    schifffahrplan_t(fahrplan_t * fpl) : fahrplan_t(fpl) {type = schifffahrplan;};
    fahrplan_t * copy() { return new schifffahrplan_t(this); };
};


/* the schedule for air ...
 * @author Hj. Malthaner
 */
class airfahrplan_t : public fahrplan_t
{
protected:
    virtual bool ist_halt_erlaubt(const grund_t *) const;
    virtual void zeige_fehlermeldung(karte_t *) const;

public:
    airfahrplan_t() : fahrplan_t() {type = airfahrplan;};
    airfahrplan_t(loadsave_t *file) : fahrplan_t(file) {type = airfahrplan;};
    airfahrplan_t(fahrplan_t * fpl) : fahrplan_t(fpl) {type = airfahrplan;};
    fahrplan_t * copy() { return new airfahrplan_t(this); };
};


#endif
