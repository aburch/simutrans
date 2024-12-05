/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMWARE_H
#define SIMWARE_H


#include "halthandle_t.h"
#include "dataobj/koord.h"
#include "descriptor/goods_desc.h"
#include "tpl/vector_tpl.h"

class goods_manager_t;
class karte_t;
class player_t;

/** Class to handle goods packets (and their destinations) */
class ware_t
{
	friend class goods_manager_t;

private:
	/// private lookup table to speedup
	static const goods_desc_t *index_to_desc[256];

public:
	// Type used to specify an amount of goods. Always positive.
	typedef uint32 goods_amount_t;

	// Maximum number of goods per ware package. Limited by the bit field used.
	static goods_amount_t const GOODS_AMOUNT_LIMIT = (1 << 23) - 1;

	/// type of good, used as index into goods-types array
	uint32 index: 8;

	/// amount of goods
	goods_amount_t menge : 23;

	/**
	 * To indicate that the ware's destination is a factory/consumer store
	 */
	uint32 to_factory : 1;

private:
	/**
	 * Handle of target station.
	 */
	halthandle_t ziel;

	/**
	 * The halts where the packet has to leave convoy.
	 * It includes the final destination halt.
	 */
	vector_tpl<halthandle_t> transit_halts;

	// A placeholder object for get_zwischenziel()
	const halthandle_t invalid_halt = halthandle_t();

	/**
	 * Target position (factory, etc)
	 */
	koord zielpos;

	/**
	 * Update target (zielpos) for factory-going goods (after loading or rotating)
	 */
	void update_factory_target();

public:
	const halthandle_t &get_ziel() const { return ziel; }
	void set_ziel(const halthandle_t &ziel) { this->ziel = ziel; }

	const vector_tpl<halthandle_t>& get_transit_halts() const { return transit_halts; }
	void set_transit_halts(vector_tpl<halthandle_t> const& th) { 
		// Copy all transit_halts entries
		vector_tpl<halthandle_t> tmp_vector = th;
		swap(transit_halts, tmp_vector);
	}
	void clear_transit_halts() { transit_halts.clear(); }
	void pop_first_transit_halts() { transit_halts.remove_at(0); }
	// Returns the first transit halt, or an invalid handle if there are none.
	const halthandle_t &get_zwischenziel() const {
		return transit_halts.empty() ? invalid_halt : transit_halts.front();
	}

	koord get_zielpos() const { return zielpos; }
	void set_zielpos(const koord zielpos) { this->zielpos = zielpos; }

	ware_t();
	ware_t(const goods_desc_t *typ);
	ware_t(loadsave_t *file);
	ware_t(const ware_t&);
	ware_t& operator=(const ware_t&);

	/// @returns the non-translated name of the ware.
	const char *get_name() const { return get_desc()->get_name(); }
	const char *get_mass() const { return get_desc()->get_mass(); }
	uint8 get_catg() const { return get_desc()->get_catg(); }
	uint8 get_index() const { return index; }

	const goods_desc_t* get_desc() const { return index_to_desc[index]; }

	void rdwr(loadsave_t *file);

	void finish_rd(karte_t *welt);

	// find out the category ...
	bool is_passenger() const {  return index==0; }
	bool is_mail() const {  return index==1; }
	bool is_freight() const {  return index>2; }

	int operator==(const ware_t &w) {
		if(  w.transit_halts.get_count()!=transit_halts.get_count()  ) {
			return false;
		}
		for(  uint32 i=0;  i<transit_halts.get_count();  i++  ) {
			if(  w.transit_halts[i]!=transit_halts[i]  ) {
				return false;
			}
		}
		return index  == w.index  &&
			menge == w.menge &&
			to_factory == w.to_factory &&
			ziel  == w.ziel  &&
			zielpos == w.zielpos;
	}

	int operator!=(const ware_t &w) { return !(*this == w); }

	// mail and passengers just care about target station
	// freight needs to obey coordinates (since more than one factory might by connected!)
	inline bool same_destination(const ware_t &w) const {
		return index==w.get_index()  &&  ziel==w.get_ziel()  &&  to_factory==w.to_factory  &&  (!to_factory  ||  zielpos==w.get_zielpos());
	}

	/**
	 * Adjust target coordinates.
	 * Must be called after factories have been rotated!
	 */
	void rotate90( sint16 y_size );

	/**
	 * Calculates transport revenue per tile and freight unit.
	 * Takes speedbonus into account!
	 * @param desc the freight
	 * @param wt waytype of vehicle
	 * @param speedkmh actual achieved speed in km/h
	 */
	static sint64 calc_revenue(const goods_desc_t* desc, waytype_t wt, sint32 speedkmh);

	/**
	 * Adds the number of goods to this goods packet.
	 * @param number The number of goods to add to this packet.
	 * @return Any excess goods that could not be added, eg due to logical limits.
	 */
	goods_amount_t add_goods(goods_amount_t const number);

	/**
	* Removes the number of goods from this goods packet.
	* @param number The number of goods to remove from this packet.
	* @return Any excess goods that could not be removed, eg due to logical limits.
	*/
	goods_amount_t remove_goods(goods_amount_t const number);

	/**
	 * Checks if the goods amount is maxed.
	 * @return True if goods amount is maxed out so no more goods can be added.
	 */
	bool is_goods_amount_maxed() const;
};

#endif
