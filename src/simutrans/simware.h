/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMWARE_H
#define SIMWARE_H


#include "halthandle.h"
#include "dataobj/koord.h"
#include "descriptor/goods_desc.h"

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
	 * Handle of station, where the packet has to leave convoy.
	 */
	halthandle_t zwischenziel;

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

	const halthandle_t &get_zwischenziel() const { return zwischenziel; }
	halthandle_t &access_zwischenziel() { return zwischenziel; }
	void set_zwischenziel(const halthandle_t &zwischenziel) { this->zwischenziel = zwischenziel; }

	koord get_zielpos() const { return zielpos; }
	void set_zielpos(const koord zielpos) { this->zielpos = zielpos; }

	ware_t();
	ware_t(const goods_desc_t *typ);
	ware_t(loadsave_t *file);

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
		return index  == w.index  &&
			menge == w.menge &&
			to_factory == w.to_factory &&
			ziel  == w.ziel  &&
			zwischenziel == w.zwischenziel &&
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
