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


/// Class to handle goods packets (and their destinations)
class ware_t
{
	friend class goods_manager_t;

private:
	/// private lookup table to speedup
	static const goods_desc_t *index_to_desc[256];

public:
	/// Type used to specify an amount of goods. Always positive.
	typedef uint32 goods_amount_t;

	/// Maximum number of goods per ware package. Limited by the bit field used.
	static goods_amount_t const GOODS_AMOUNT_LIMIT = (1 << 23) - 1;

	/// type of good, used as index into goods-types array
	uint32 index : 8;

	/// amount of goods
	goods_amount_t amount : 23;

	/// To indicate that the ware's destination is a factory/consumer store
	uint32 to_factory : 1;

private:
	/// Final destination halt.
	halthandle_t target_halt;

	/// Halt where the packet has to leave the convoy.
	halthandle_t via_halt;

	/// Target position (factory, etc)
	koord target_pos;

	/// Update target (zielpos) for factory-going goods (after loading or rotating)
	void update_factory_target();

public:
	ware_t();
	ware_t(const goods_desc_t *desc);
	ware_t(loadsave_t *file);

public:
	const halthandle_t &get_target_halt() const { return target_halt; }
	void set_target_halt(const halthandle_t &halt) { target_halt = halt; }

	const halthandle_t &get_via_halt() const { return via_halt; }
	void set_via_halt(const halthandle_t &halt) { via_halt = halt; }

	/// @returns the next halt of a ware
	const halthandle_t get_next_halt() const { return via_halt.is_bound() ? via_halt : target_halt; }

	koord get_target_pos() const { return target_pos; }
	void set_target_pos(const koord new_pos) { target_pos = new_pos; }

	/// @returns the non-translated name of the ware.
	const char *get_name() const { return get_desc()->get_name(); }
	const char *get_mass() const { return get_desc()->get_mass(); }
	uint8 get_catg() const { return get_desc()->get_catg(); }
	uint8 get_index() const { return index; }

	const goods_desc_t *get_desc() const { return index_to_desc[index]; }

	void rdwr(loadsave_t *file);

	void finish_rd(karte_t *welt);

	// find out the category ...
	bool is_passenger() const { return index==0; }
	bool is_mail()      const { return index==1; }
	bool is_freight()   const { return index>2;  }

	bool operator==(const ware_t &other) const
	{
		return
			index       == other.index       &&
			amount      == other.amount      &&
			to_factory  == other.to_factory  &&
			target_halt == other.target_halt &&
			via_halt    == other.via_halt    &&
			target_pos  == other.target_pos;
	}

	bool operator!=(const ware_t &w) const { return !(*this == w); }

	/// Mail and passengers just care about target station
	/// Freight needs to obey coordinates (since more than one factory might by connected)
	inline bool same_destination(const ware_t &w) const
	{
		return
			index       == w.index       &&
			target_halt == w.target_halt &&
			to_factory  == w.to_factory  &&
			(!to_factory || target_pos==w.target_pos);
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
	static sint64 calc_revenue(const goods_desc_t *desc, waytype_t wt, sint32 speedkmh);

	/// Adds the number of goods to this goods packet.
	/// @return Any excess goods that could not be added, eg due to logical limits.
	goods_amount_t add_goods(goods_amount_t const to_add);

	/// Removes the number of goods from this goods packet.
	/// @return Any excess goods that could not be removed, eg due to logical limits.
	goods_amount_t remove_goods(goods_amount_t const to_remove);

	/// @returns True if goods amount is maxed out so no more goods can be added to this packet
	bool is_goods_amount_maxed() const { return amount == GOODS_AMOUNT_LIMIT; }
};


#endif
