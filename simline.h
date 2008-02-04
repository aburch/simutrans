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

	/**
	 * Handle for ourselves. Can be used like the 'this' pointer
	 * @author Hj. Malthaner
	 */
	linehandle_t self;

	enum linetype { line = 0, truckline = 1, trainline = 2, shipline = 3, airline = 4, monorailline=5, tramline=6};
	static uint8 convoi_to_line_catgory[MAX_CONVOI_COST];

	simline_t(simlinemgmt_t* simlinemgmt, fahrplan_t* fpl);
	simline_t(karte_t* welt, loadsave_t* file);
	~simline_t();

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
	fahrplan_t * get_fahrplan() { return fpl; }

	void set_fahrplan(fahrplan_t* fpl)
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
	void register_stops(fahrplan_t * fpl);

	void laden_abschliessen();

	/*
	 * unregister line from stop
	 */
	void unregister_stops(fahrplan_t * fpl);
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

protected:
	fahrplan_t * fpl,  * old_fpl;
	linetype type;

private:
	static karte_t * welt;
	char name[128];

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

	void recalc_catg_index();
};

class truckline_t : public simline_t
{
	public:
		truckline_t(simlinemgmt_t* simlinemgmt, fahrplan_t* fpl) : simline_t(simlinemgmt, fpl)
		{
			type = simline_t::truckline;
		}

		truckline_t(karte_t* welt, loadsave_t* file) : simline_t(welt, file)
		{
			type = simline_t::truckline;
			set_fahrplan(new autofahrplan_t(fpl));
		}
};

class trainline_t : public simline_t
{
	public:
		trainline_t(simlinemgmt_t* simlinemgmt, fahrplan_t* fpl) : simline_t(simlinemgmt, fpl)
		{
			type = simline_t::trainline;
		}

		trainline_t(karte_t* welt, loadsave_t* file) : simline_t(welt, file)
		{
			type = simline_t::trainline;
			set_fahrplan(new zugfahrplan_t(fpl));
		}
};

class shipline_t : public simline_t
{
	public:
		shipline_t(simlinemgmt_t* simlinemgmt, fahrplan_t* fpl) : simline_t(simlinemgmt, fpl)
		{
			type = simline_t::shipline;
		}

		shipline_t(karte_t* welt, loadsave_t* file) : simline_t(welt, file)
		{
			type = simline_t::shipline;
			set_fahrplan(new schifffahrplan_t(fpl));
		}
};

class airline_t : public simline_t
{
	public:
		airline_t(simlinemgmt_t* simlinemgmt, fahrplan_t* fpl) : simline_t(simlinemgmt, fpl)
		{
			type = simline_t::airline;
		}

		airline_t(karte_t* welt, loadsave_t* file) : simline_t(welt, file)
		{
			type = simline_t::airline;
			set_fahrplan(new airfahrplan_t(fpl));
		}
};

class monorailline_t : public simline_t
{
	public:
		monorailline_t(simlinemgmt_t* simlinemgmt, fahrplan_t* fpl) : simline_t(simlinemgmt, fpl)
		{
			type = simline_t::monorailline;
		}

		monorailline_t(karte_t* welt, loadsave_t* file) : simline_t(welt, file)
		{
			type = simline_t::monorailline;
			set_fahrplan(new monorailfahrplan_t(fpl));
		}
};

class tramline_t : public simline_t
{
	public:
		tramline_t(simlinemgmt_t* simlinemgmt, fahrplan_t* fpl) : simline_t(simlinemgmt, fpl) { type = simline_t::tramline; }

		tramline_t(karte_t* welt, loadsave_t* file) : simline_t(welt, file)
		{
			type = simline_t::tramline;
			set_fahrplan(new tramfahrplan_t(fpl));
		}
};

#endif
