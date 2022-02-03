/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMLINE_H
#define SIMLINE_H


#include "simtypes.h"
#include "simcolor.h"
#include "convoihandle.h"
#include "linehandle.h"

#include "tpl/minivec_tpl.h"
#include "tpl/vector_tpl.h"
#include "utils/plainstring.h"

#define MAX_MONTHS     12 // Max history

#define LINE_CAPACITY          0 // the amount of ware that could be transported, theoretically
#define LINE_TRANSPORTED_GOODS 1 // the amount of ware that has been transported
#define LINE_CONVOIS           2 // number of convois for this line
#define LINE_REVENUE           3 // the income this line generated
#define LINE_OPERATIONS        4 // the cost of operations this line generated
#define LINE_PROFIT            5 // total profit of line
#define LINE_DISTANCE          6 // distance covered by all convois
#define LINE_MAXSPEED          7 // maximum speed for bonus calculation of all convois
#define LINE_WAYTOLL           8 // way toll paid by vehicles of line
#define MAX_LINE_COST          9 // Total number of cost items

class karte_ptr_t;
class loadsave_t;
class player_t;
class schedule_t;

class simline_t {

public:
	enum linetype {
		line            = 0,
		truckline       = 1,
		trainline       = 2,
		shipline        = 3,
		airline         = 4,
		monorailline    = 5,
		tramline        = 6,
		maglevline      = 7,
		narrowgaugeline = 8,
		MAX_LINE_TYPE
	};

protected:
	schedule_t * schedule;
	player_t *player;
	linetype type;

	bool withdraw;

private:
	static karte_ptr_t welt;
	plainstring name;

	/**
	 * Handle for ourselves. Can be used like the 'this' pointer
	 * Initialized by constructors
	 */
	linehandle_t self;

	/**
	 * the current state saved as color
	 * Meanings are BLACK (ok), WHITE (no convois), YELLOW (no vehicle moved), RED (last month income minus), BLUE (at least one convoi vehicle is obsolete)
	 */
	PIXVAL state_color;

	/**
	 * a list of all convoys assigned to this line
	 */
	vector_tpl<convoihandle_t> line_managed_convoys;

	/**
	 * a list of all catg_index, which can be transported by this line.
	 */
	minivec_tpl<uint8> goods_catg_index;

	/**
	 * struct holds new financial history for line
	 */
	sint64 financial_history[MAX_MONTHS][MAX_LINE_COST];

	/**
	 * creates empty schedule with type depending on line-type
	 */
	void create_schedule();

	void init_financial_history();

	void recalc_status();

public:
	simline_t(player_t *player, linetype type);
	simline_t(player_t *player, linetype type, loadsave_t *file);

	~simline_t();

	linehandle_t get_handle() const { return self; }

	/**
	 * add convoy to route
	 */
	void add_convoy(convoihandle_t cnv);

	/**
	 * remove convoy from route
	 */
	void remove_convoy(convoihandle_t cnv);

	/**
	 * get convoy
	 */
	convoihandle_t get_convoy(int i) const { return line_managed_convoys[i]; }

	/**
	 * return number of manages convoys in this line
	 */
	uint32 count_convoys() const { return line_managed_convoys.get_count(); }

	vector_tpl<convoihandle_t> const& get_convoys() const { return line_managed_convoys; }

	/**
	 * returns the state of the line
	 */
	PIXVAL get_state_color() const { return state_color; }

	/**
	 * return the schedule of the line
	 */
	schedule_t * get_schedule() const { return schedule; }

	void set_schedule(schedule_t* schedule);

	/**
	 * get name of line
	 */
	char const* get_name() const { return name; }
	void set_name(const char *str);

	/*
	 * load or save the line
	 */
	void rdwr(loadsave_t * file);

	/**
	 * method to load/save linehandles
	 */
	static void rdwr_linehandle_t(loadsave_t *file, linehandle_t &line);

	/*
	 * register line with stop
	 */
	void register_stops(schedule_t * schedule);

	void finish_rd();

	/*
	 * unregister line from stop
	 */
	void unregister_stops(schedule_t * schedule);
	void unregister_stops();

	/*
	 * renew line registration for stops
	 */
	void renew_stops();

	// called after tiles are removed from stations to change the load of connected convois
	void check_freight();

	sint64* get_finance_history() { return *financial_history; }

	sint64 get_finance_history(int month, int cost_type) const { return financial_history[month][cost_type]; }
	sint64 get_stat_converted(int month, int cost_type) const;

	void book(sint64 amount, int cost_type) { financial_history[0][cost_type] += amount; }

	static uint8 convoi_to_line_catgory(uint8 convoi_cost_type);

	void new_month();

	linetype get_linetype() { return type; }

	static waytype_t linetype_to_waytype( const linetype lt );
	static linetype waytype_to_linetype( const waytype_t wt );
	static const char *get_linetype_name( const linetype lt );

	const minivec_tpl<uint8> &get_goods_catg_index() const { return goods_catg_index; }

	// recalculates the good transported by this line and (in case of changes) will start schedule recalculation
	void recalc_catg_index();

	void set_withdraw( bool yes_no );

	bool get_withdraw() const { return withdraw; }

	player_t *get_owner() const {return player;}


};

#endif
