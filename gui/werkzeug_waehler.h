#ifndef gui_werkzeug_wehler_h
#define gui_werkzeug_wehler_h

#include "../ifc/gui_fenster.h"

class spieler_t;
class karte_t;
struct event_t;
class skin_besch_t;

/**
 * Eine Klasse, die ein Fenster zur Auswahl von bis zu acht
 * Werkzeugen per Icon darstellt.
 *
 * @author Hj. Malthaner
 */

class werkzeug_waehler_t : public gui_fenster_t
{
private:
    int (* wz[8])(spieler_t *, karte_t *, koord pos, value_t param);
    int wz_zeiger[8];
    int wz_versatz[8];

    int wz_sound_ok[8];
    int wz_sound_ko[8];

    value_t wz_param[8];

    const char * tooltips[8];

    karte_t *welt;

    /**
     * Erste Icon-Gruppe (2x2)
     * @author Hj. Malthaner
     */
    int bild1_nr;

    /**
     * Zweite Icon-Gruppe (2x2)
     * @author Hj. Malthaner
     */
    int bild2_nr;

    char * titel;


    /**
     * Name of the help file
     * @author Hj. Malthaner
     */
    const char * hilfe_datei;

public:

    werkzeug_waehler_t(karte_t *welt, const skin_besch_t *bilder,
		       char * titel);


    void setze_werkzeug(int index,
                        int (* wz1)(spieler_t *, karte_t *, koord),
			int zeiger_bild,
			int versatz,
			int sound_ok,
			int sound_ko);

    void setze_werkzeug(int index,
                        int (* wz1)(spieler_t *, karte_t *, koord, value_t),
			int zeiger_bild,
			int versatz,
			value_t param,
			int sound_ok,
			int sound_ko);


    /**
     * Sets the tooltip text for a tool.
     * Text must already be translated.
     * @author Hj. Malthaner
     */
    void set_tooltip(int tipnr, const char * text);


    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    const char * gib_hilfe_datei() const {return hilfe_datei;};

    void setze_hilfe_datei(const char *datei) {hilfe_datei = datei;};


    /**
     * manche Fenster kanns nur einmal geben, dazu die magic number
     * @return -1 wenn fenster schon vorhanden, sonst >= 0
     */
    int zeige_info(int magic);


    /**
     * 'Jedes Ding braucht einen Namen.'
     * @return Gibt den Namen des Objekts zurück.
     */
    virtual const char *gib_name() const;


    /**
     * @return gibt wunschgroesse für das beobachtungsfenster zurueck
     */
    virtual koord gib_fenstergroesse() const {return koord(128, 64+16);};


    /**
     * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
     * zurück
     */
    fensterfarben gib_fensterfarben() const;


    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    virtual void infowin_event(const event_t *ev);


    /**
     * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     * @author Hj. Malthaner
     */
    void zeichnen(koord pos, koord gr);
};

#endif
