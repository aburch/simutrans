/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef dings_gebaeude_h
#define dings_gebaeude_h

#include "../ifc/sync_steppable.h"
#include "../simdings.h"
#include "../simcolor.h"

class haus_tile_besch_t;
class fabrik_t;
class stadt_t;

/**
 * Asynchron oder synchron animierte Gebaeude für Simutrans.
 * @author Hj. Malthaner
 */
class gebaeude_t : public ding_t, sync_steppable
{
public:
	/**
	 * Vom typ "unbekannt" sind auch spezielle gebaeude z.B. das Rathaus
	 * @author Hj. Malthaner
	 */
	enum typ {wohnung, gewerbe, industrie, unbekannt};

private:
	const haus_tile_besch_t *tile;

	/**
	* either point to a factory or a city
	* @author Hj. Malthaner
	*/
	union {
		fabrik_t  *fab;
		stadt_t *stadt;
	} ptr;

	/**
	 * Zeitpunkt an dem das Gebaeude Gebaut wurde
	 * @author Hj. Malthaner
	 */
	uint32 insta_zeit;

	/**
	 * Time control for animation progress.
	 * @author Hj. Malthaner
	 */
	uint16 anim_time;

	/**
	 * Current anim frame
	 * @author Hj. Malthaner
	 */
	uint8 count;

	/**
	 * Is this a sync animated object?
	 * @author Hj. Malthaner
	 */
	uint8 sync:1;

	/**
	 * Boolean flag if a construction site or buildings image
	 * shall be displayed.
	 * @author Hj. Malthaner
	 */
	uint8 zeige_baugrube:1;

	/**
	 * if true, this ptr union contains a factory pointer
	 * @author Hj. Malthaner
	 */
	uint8 is_factory:1;

	/**
	 * Initializes all variables with save, usable values
	 * @author Hj. Malthaner
	 */
	void init();


protected:
	gebaeude_t(karte_t *welt);

	/**
	 * Erzeugt ein Info-Fenster für dieses Objekt
	 * @author V. Meyer
	 */
	ding_infowin_t *new_info();

public:
	gebaeude_t(karte_t *welt, loadsave_t *file);
	gebaeude_t(karte_t *welt, koord3d pos,spieler_t *sp, const haus_tile_besch_t *t);
	virtual ~gebaeude_t();

	typ gib_haustyp() const;

	void add_alter(uint32 a);

	void setze_fab(fabrik_t *fb);
	void setze_stadt(stadt_t *s);

	/**
	 * Ein Gebaeude kann zu einer Fabrik gehören.
	 * @return Einen Zeiger auf die Fabrik zu der das Objekt gehört oder NULL,
	 * wenn das Objekt zu keiner Fabrik gehört.
	 * @author Hj. Malthaner
	 */
	fabrik_t* get_fabrik() const { return is_factory ? ptr.fab : NULL; }
	stadt_t* get_stadt() const { return is_factory ? NULL : ptr.stadt; }

	enum ding_t::typ gib_typ() const {return ding_t::gebaeude;}

	// snowline height may have been changed
	bool check_season(const long /*month*/) { calc_bild(); return true; }

	image_id gib_bild() const;
	image_id gib_bild(int nr) const;
	image_id gib_after_bild() const;

	image_id gib_outline_bild() const;
	PLAYER_COLOR_VAL gib_outline_colour() const;

	// caches image at height 0
	void calc_bild();

	/**
	 * @return eigener Name oder Name der Fabrik falls Teil einer Fabrik
	 * @author Hj. Malthaner
	 */
	virtual const char *gib_name() const;

	bool ist_rathaus() const;

	bool ist_firmensitz() const;

	bool is_monument() const;

	/**
	 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	 * Beobachtungsfenster angezeigt wird.
	 * @author Hj. Malthaner
	 */
	void info(cbuffer_t & buf) const;

	void rdwr(loadsave_t *file);

	/**
	 * Methode für Echtzeitfunktionen eines Objekts. Spielt animation.
	 * @return true
	 * @author Hj. Malthaner
	 */
	bool sync_step(long delta_t);

	/**
	 * @return Den level (die Ausbaustufe) des Gebaudes
	 * @author Hj. Malthaner
	 */
	int gib_passagier_level() const;

	int gib_post_level() const;

	void setze_tile(const haus_tile_besch_t *t);

	const haus_tile_besch_t *gib_tile() const { return tile; }

	virtual void zeige_info();

	void entferne(spieler_t *sp);

	void laden_abschliessen();
};

#endif
