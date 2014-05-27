/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Simple goods transport AI
 */


#include "ai.h"


class ai_goods_t : public ai_t
{
private:
	enum zustand {
		NR_INIT,
		NR_SAMMLE_ROUTEN,
		NR_BAUE_ROUTE1,
		NR_BAUE_SIMPLE_SCHIENEN_ROUTE,
		NR_BAUE_STRASSEN_ROUTE,
		NR_BAUE_WATER_ROUTE,
		NR_BAUE_CLEAN_UP,
		NR_RAIL_SUCCESS,
		NR_ROAD_SUCCESS,
		NR_WATER_SUCCESS,
		CHECK_CONVOI
	};

	// vars für die KI
	zustand state;

	/* test more than one supplier and more than one good *
	 * save last factory for building next supplier/consumer *
	 * @author prissi
	 */
	fabrik_t *root;

	// actual route to be built between those
	fabrik_t *start;
	fabrik_t *ziel;
	const ware_besch_t *freight;

	// we will use this vehicle!
	const vehikel_besch_t *rail_vehicle;
	const vehikel_besch_t *rail_engine;
	const vehikel_besch_t *road_vehicle;
	const vehikel_besch_t *ship_vehicle;

	// and the convoi will run on this track:
	const weg_besch_t *rail_weg;
	const weg_besch_t *road_weg;

	sint32 count_rail;
	sint32 count_road;

	// multi-purpose counter
	sint32 count;

	// time to wait before next contruction
	sint32 next_construction_steps;

	/* start and end stop position (and their size) */
	koord platz1, size1, platz2, size2, harbour;

	// KI helper class
	class fabconnection_t{
		friend class ai_goods_t;
		fabrik_t *fab1;
		fabrik_t *fab2;	// koord1 must be always "smaller" than koord2
		const ware_besch_t *ware;

	public:
		fabconnection_t( fabrik_t *k1=0, fabrik_t *k2=0, const ware_besch_t *w=0 ) : fab1(k1), fab2(k2), ware(w) {}
		void rdwr( loadsave_t *file );

		bool operator != (const fabconnection_t & k) { return fab1 != k.fab1 || fab2 != k.fab2 || ware != k.ware; }
		bool operator == (const fabconnection_t & k) { return fab1 == k.fab1 && fab2 == k.fab2 && ware == k.ware; }
//		const bool operator < (const fabconnection_t & k) { return (abs(fab1.x)+abs(fab1.y)) - (abs(k.fab1.x)+abs(k.fab1.y)) < 0; }
	};

	slist_tpl<fabconnection_t*> forbidden_connections;

	// return true, if this a route to avoid (i.e. we did a construction without sucess here ...)
	bool is_forbidden( fabrik_t *fab1, fabrik_t *fab2, const ware_besch_t *w ) const;

	/* recursive lookup of a factory tree:
	 * sets start and ziel to the next needed supplier
	 * start always with the first branch, if there are more goods
	 */
	bool get_factory_tree_lowest_missing( fabrik_t *fab );

	/* recursive lookup of a tree and how many factories must be at least connected
	 * returns -1, if this tree is can't be completed
	 */
	int get_factory_tree_missing_count( fabrik_t *fab );

	bool suche_platz1_platz2(fabrik_t *qfab, fabrik_t *zfab, int length);

	int baue_bahnhof(const koord* p, int anz_vehikel);

	bool create_simple_rail_transport();

	// create way and stops for these routes
	bool create_ship_transport_vehikel(fabrik_t *qfab, int anz_vehikel);
	void create_road_transport_vehikel(fabrik_t *qfab, int anz_vehikel);
	void create_rail_transport_vehikel(const koord pos1,const koord pos2, int anz_vehikel, int ladegrad);

public:
	ai_goods_t(karte_t *wl, uint8 nr);

	// this type of AIs identifier
	virtual uint8 get_ai_id() const { return AI_GOODS; }

	// cannot do airfreight at the moment
	virtual void set_air_transport( bool ) { air_transport = false; }

	virtual void rdwr(loadsave_t *file);

	virtual void bescheid_vehikel_problem(convoihandle_t cnv,const koord3d ziel);

	bool set_active( bool b );

	void step();

	void neues_jahr();

	virtual void rotate90( const sint16 y_size );

	virtual void notify_factory(notification_factory_t flag, const fabrik_t*);
};
