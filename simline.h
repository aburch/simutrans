/*
 * part of the Simutrans project
 * @author hsiegeln
 * 01/12/2003
 */
#ifndef simline_h
#define simline_h

#include <string.h>

#include "simtypes.h"
#include "dataobj/loadsave.h"
#include "dataobj/fahrplan.h"

#include "tpl/vector_tpl.h"
#include "simconvoi.h"

#include "simdebug.h"
#include "linehandle_t.h"
#include "convoihandle_t.h"

#define MAX_LINE_COST   6 // Total number of cost items
#define MAX_MONTHS     12 // Max history
#define MAX_NON_MONEY_TYPES 2 // number of non money types in line's financial statistic

#define LINE_CAPACITY   0 // the amount of ware that could be transported, theoretically
#define LINE_TRANSPORTED_GOODS 1 // the amount of ware that has been transported
#define LINE_CONVOIS		2 // number of convois for this line
#define LINE_REVENUE		3 // the income this line generated
#define LINE_OPERATIONS         4 // the cost of operations this line generated
#define LINE_PROFIT             5 // total profit of line

class karte_t;
class simlinemgmt_t;

class simline_t {

public:
	enum linetype { line = 0, truckline = 1, trainline = 2, shipline = 3, airline = 4, monorailline=5, tramline=6, maglevline=7, narrowgaugeline=8};
	static uint8 convoi_to_line_catgory[MAX_CONVOI_COST];

protected:
	simline_t(karte_t* welt, spieler_t*sp);

	schedule_t * fpl,  *old_fpl;
	linetype type;
	spieler_t *sp;

private:
	static karte_t * welt;
	char name[128];

	/**
	 * Handle for ourselves. Can be used like the 'this' pointer
	 * @author Hj. Malthaner
	 */
	linehandle_t self;

	/*
	 * the line id
	 * @author hsiegeln
	 */
	uint16 id;

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
	 * a list of all convoys assigned to this line
	 * @author hsiegeln
	 */
	minivec_tpl<uint8> goods_catg_index;

	/*
 	 * struct holds new financial history for line
	 * @author hsiegeln
	 */
	sint64 financial_history[MAX_MONTHS][MAX_LINE_COST];

	void init_financial_history();

	void recalc_status();

public:
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
	schedule_t * get_schedule() { return fpl; }

	void set_schedule(schedule_t* fpl)
	{
		delete this->fpl;
		this->fpl = fpl;
	}

	/*
	 * get name of line
	 * @author hsiegeln
	 */
	char *get_name() {return name;}

	uint16 get_line_id() const {return id;}

	void set_line_id(uint32 id) {this->id=id;}

	/*
	 * load or save the line
	 */
	void rdwr(loadsave_t * file);

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

	int operator==(const simline_t &s) {
		return id == s.id;
	}

	int operator!=(const simline_t &s) {
		return ! (*this == s);
	}

	sint64* get_finance_history() { return *financial_history; }

	sint64 get_finance_history(int month, int cost_type) { return financial_history[month][cost_type]; }

	void book(sint64 amount, int cost_type) { financial_history[0][cost_type] += amount; }

	void new_month();

	/*
	 * called from line_management_gui.cc to prepare line for a change of its schedule
	 */
	void prepare_for_update();

	linetype get_linetype() { return type; }

	void set_linetype(linetype lt) { type = lt; }

	const minivec_tpl<uint8> &get_goods_catg_index() const { return goods_catg_index; }

	// recalculates the good transported by this line and (in case of changes) will start schedule recalculation
	void recalc_catg_index();

public:
	spieler_t *get_besitzer() const {return sp;}

};



class truckline_t : public simline_t
{
	public:
		truckline_t(karte_t* welt, spieler_t* sp) : simline_t(welt, sp)
		{
			type = simline_t::truckline;
			set_schedule(new autofahrplan_t());
		}
};

class trainline_t : public simline_t
{
	public:
		trainline_t(karte_t* welt, spieler_t* sp) : simline_t(welt, sp)
		{
			type = simline_t::trainline;
			set_schedule(new zugfahrplan_t());
		}
};

class shipline_t : public simline_t
{
	public:
		shipline_t(karte_t* welt, spieler_t* sp) : simline_t(welt, sp)
		{
			type = simline_t::shipline;
			set_schedule(new schifffahrplan_t());
		}
};

class airline_t : public simline_t
{
	public:
		airline_t(karte_t* welt, spieler_t* sp) : simline_t(welt, sp)
		{
			type = simline_t::airline;
			set_schedule(new airfahrplan_t());
		}
};

class monorailline_t : public simline_t
{
	public:
		monorailline_t(karte_t* welt, spieler_t* sp) : simline_t(welt, sp)
		{
			type = simline_t::monorailline;
			set_schedule(new monorailfahrplan_t());
		}
};

class tramline_t : public simline_t
{
	public:
		tramline_t(karte_t* welt, spieler_t* sp) : simline_t(welt, sp)
		{
			type = simline_t::tramline;
			set_schedule(new tramfahrplan_t());
		}
};

class narrowgaugeline_t : public simline_t
{
	public:
		narrowgaugeline_t(karte_t* welt, spieler_t* sp) : simline_t(welt, sp)
		{
			type = simline_t::narrowgaugeline;
			set_schedule(new narrowgaugefahrplan_t());
		}
};

class maglevline_t : public simline_t
{
	public:
		maglevline_t(karte_t* welt, spieler_t* sp) : simline_t(welt, sp)
		{
			type = simline_t::maglevline;
			set_schedule(new maglevfahrplan_t());
		}
};

#endif
