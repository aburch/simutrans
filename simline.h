/*
 * part of the Simutrans project
 * @author hsiegeln
 * 01/12/2003
 */
#ifndef simline_h
#define simline_h

#include "convoihandle_t.h"
#include "linehandle_t.h"
#include "simtypes.h"

#include "tpl/minivec_tpl.h"
#include "tpl/vector_tpl.h"
#include "utils/plainstring.h"

#define MAX_MONTHS     12 // Max history
#define MAX_NON_MONEY_TYPES 2 // number of non money types in line's financial statistic

#define LINE_CAPACITY   0 // the amount of ware that could be transported, theoretically
#define LINE_TRANSPORTED_GOODS 1 // the amount of ware that has been transported
#define LINE_CONVOIS		2 // number of convois for this line
#define LINE_REVENUE		3 // the income this line generated
#define LINE_OPERATIONS     4 // the cost of operations this line generated
#define LINE_PROFIT         5 // total profit of line
#define LINE_DISTANCE       6 // distance converd by all convois
#define LINE_MAXSPEED       7 // maximum speed for bonus calculation of all convois
#define LINE_WAYTOLL        8 // way toll paid by vehicles of line
#define MAX_LINE_COST   9 // Total number of cost items

class karte_t;
class loadsave_t;
class spieler_t;
class schedule_t;

class simline_t {

public:
	enum linetype { line = 0, truckline = 1, trainline = 2, shipline = 3, airline = 4, monorailline=5, tramline=6, maglevline=7, narrowgaugeline=8, MAX_LINE_TYPE};

protected:
	schedule_t * fpl;
	spieler_t *sp;
	linetype type;

	bool withdraw;

private:
	static karte_t * welt;
	plainstring name;

	/**
	 * Handle for ourselves. Can be used like the 'this' pointer
	 * Initialized by constructors
	 * @author Hj. Malthaner
	 */
	linehandle_t self;

	/*
	 * the current state saved as color
	 * Meanings are BLACK (ok), WHITE (no convois), YELLOW (no vehicle moved), RED (last month income minus), BLUE (at least one convoi vehicle is obsolete)
	 */
	uint8 state_color;

	/*
	 * a list of all convoys assigned to this line
	 * @author hsiegeln
	 */
	vector_tpl<convoihandle_t> line_managed_convoys;

	/*
	 * a list of all catg_index, which can be transported by this line.
	 */
	minivec_tpl<uint8> goods_catg_index;

	/*
	 * struct holds new financial history for line
	 * @author hsiegeln
	 */
	sint64 financial_history[MAX_MONTHS][MAX_LINE_COST];

	/**
	 * creates empty schedule with type depending on line-type
	 */
	void create_schedule();

	void init_financial_history();

	void recalc_status();

public:
	simline_t(karte_t* welt, spieler_t *sp, linetype type);
	simline_t(karte_t* welt, spieler_t *sp, linetype type, loadsave_t *file);

	~simline_t();

	linehandle_t get_handle() const { return self; }

	/*
	 * add convoy to route
	 * @author hsiegeln
	 */
	void add_convoy(convoihandle_t cnv);

	/*
	 * remove convoy from route
	 * @author hsiegeln
	 */
	void remove_convoy(convoihandle_t cnv);

	/*
	 * get convoy
	 * @author hsiegeln
	 */
	convoihandle_t get_convoy(int i) const { return line_managed_convoys[i]; }

	/*
	 * return number of manages convoys in this line
	 * @author hsiegeln
	 */
	uint32 count_convoys() const { return line_managed_convoys.get_count(); }

	/*
	 * returns the state of the line
	 * @author prissi
	 */
	uint8 get_state_color() const { return state_color; }

	/*
	 * return fahrplan of line
	 * @author hsiegeln
	 */
	schedule_t * get_schedule() const { return fpl; }

	void set_schedule(schedule_t* fpl);

	/*
	 * get name of line
	 * @author hsiegeln
	 */
	char const* get_name() const { return name; }
	void set_name(const char *str) { name = str; }

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
	void register_stops(schedule_t * fpl);

	void laden_abschliessen();

	/*
	 * unregister line from stop
	 */
	void unregister_stops(schedule_t * fpl);
	void unregister_stops();

	/*
	 * renew line registration for stops
	 */
	void renew_stops();

	// called after tiles are removed from stations to change the load of connected convois
	void check_freight();

	sint64* get_finance_history() { return *financial_history; }

	sint64 get_finance_history(int month, int cost_type) const { return financial_history[month][cost_type]; }

	void book(sint64 amount, int cost_type) { financial_history[0][cost_type] += amount; }

	static uint8 convoi_to_line_catgory(uint8 convoi_cost_type);

	void new_month();

	linetype get_linetype() { return type; }

	const minivec_tpl<uint8> &get_goods_catg_index() const { return goods_catg_index; }

	// recalculates the good transported by this line and (in case of changes) will start schedule recalculation
	void recalc_catg_index();

	void set_withdraw( bool yes_no );

	bool get_withdraw() const { return withdraw; }

	spieler_t *get_besitzer() const {return sp;}

};

#endif
