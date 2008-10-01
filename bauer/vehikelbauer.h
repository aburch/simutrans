/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef vehikelbauer_t_h
#define vehikelbauer_t_h


#include "../dataobj/koord3d.h"
#include "../simimg.h"
#include "../simtypes.h"

class vehikel_t;
class cstring_t;
class spieler_t;
class convoi_t;
class vehikel_besch_t;
class ware_besch_t;
template <class T> class slist_tpl;
template <class T> class vector_tpl;


/**
 * Baut Fahrzeuge. Fahrzeuge sollten nicht direct instanziiert werden
 * sondern immer von einem vehikelbauer_t erzeugt werden.
 *
 * @author Hj. Malthaner
 */
class vehikelbauer_t
{
public:
	static bool speedbonus_init(cstring_t objfilename);
	static sint32 get_speedbonus( sint32 monthyear, waytype_t wt );

	static bool register_besch(const vehikel_besch_t *besch);
	static bool alles_geladen();

	static vehikel_t* baue(koord3d k, spieler_t* sp, convoi_t* cnv, const vehikel_besch_t* vb );

	/**
	* ermittelt ein basis bild fuer ein Fahrzeug das den angegebenen
	* Bedingungen entspricht.
	*
	* @param vtyp strasse, schiene oder wasser
	* @param min_power minimalleistung des gesuchten Fahrzeuges (inclusiv)
	* @author Hansjörg Malthaner
	*/
	static const vehikel_besch_t * gib_info(const ware_besch_t *wtyp,waytype_t vtyp,uint32 min_power);

	static const vehikel_besch_t * gib_info(image_id base_img);
	static const vehikel_besch_t * gib_info(const char *name);
	static slist_tpl<const vehikel_besch_t*>* gib_info(waytype_t typ);

	/* extended sreach for vehicles for KI
	* @author prissi
	*/
	static const vehikel_besch_t *vehikel_search(waytype_t typ,const uint16 month_now,const uint32 target_power,const uint32 target_speed,const ware_besch_t * target_freight, bool include_electric, bool not_obsolete );

	static const vehikel_besch_t *vehikel_fuer_leistung(uint32 leistung, waytype_t typ,const unsigned month_now);
};

#endif
