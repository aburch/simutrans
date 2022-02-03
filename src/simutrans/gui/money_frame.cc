/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>

#include "money_frame.h"
#include "ai_option.h"
#include "headquarter_info.h"
#include "simwin.h"

#include "../world/simworld.h"
#include "../simdebug.h"
#include "../display/simgraph.h"
#include "../simcolor.h"
#include "../utils/simstring.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/scenario.h"
#include "../dataobj/loadsave.h"

#include "components/gui_button_to_chart.h"

// for headquarter construction only ...
#include "../tool/simmenu.h"


// remembers last settings
static vector_tpl<sint32> bFilterStates;

#define BUTTONSPACE max(D_BUTTON_HEIGHT, LINESPACE)

static const char *cost_type_name[MAX_PLAYER_COST_BUTTON] =
{
	"Transported",
	"Revenue",
	"Operation",
	"Maintenance",
	"Road toll",
	"Ops Profit",
	"New Vehicles",
	"Construction_Btn",
	"Gross Profit",
	"Cash",
	"Assets",
	"Margin (%)",
	"Net Wealth"
};


static const uint8 cost_type_color[MAX_PLAYER_COST_BUTTON] =
{
	COL_TRANSPORTED,
	COL_REVENUE,
	COL_OPERATION,
	COL_MAINTENANCE,
	COL_TOLL,
	COL_OPS_PROFIT,
	COL_NEW_VEHICLES,
	COL_CONSTRUCTION,
	COL_PROFIT,
	COL_CASH,
	COL_VEHICLE_ASSETS,
	COL_MARGIN,
	COL_WEALTH
};


static const uint8 cost_type[3*MAX_PLAYER_COST_BUTTON] =
{
	ATV_TRANSPORTED,                TT_ALL, STANDARD, // all transported goods
	ATV_REVENUE_TRANSPORT,          TT_ALL, MONEY,    // Income
	ATV_RUNNING_COST,               TT_ALL, MONEY,    // Vehicle running costs
	ATV_INFRASTRUCTURE_MAINTENANCE, TT_ALL, MONEY,    // Upkeep
	ATV_WAY_TOLL,                   TT_ALL, MONEY,
	ATV_OPERATING_PROFIT,           TT_ALL, MONEY,
	ATV_NEW_VEHICLE,                TT_ALL, MONEY,   // New vehicles
	ATV_CONSTRUCTION_COST,          TT_ALL, MONEY,   // Construction
	ATV_PROFIT,                     TT_ALL, MONEY,
	ATC_CASH,                       TT_MAX, MONEY,   // Cash
	ATV_NON_FINANCIAL_ASSETS,       TT_ALL, MONEY,   // value of all vehicles and buildings
	ATV_PROFIT_MARGIN,              TT_ALL, PERCENT,
	ATC_NETWEALTH,                  TT_MAX, MONEY,   // Total Cash + Assets
};

static const sint8 cell_to_buttons[] =
{
	0,  -1,  -1,  -1,  -1,
	1,  -1,  -1,  -1,  -1,
	2,  -1,  -1,  -1,  -1,
	3,  -1,  -1,  -1,  -1,
	4,  -1,  -1,  -1,  -1,
	5,  -1,  -1,   9,  -1,
	6,  -1,  -1,  10,  -1,
	7,  -1,  -1,  11,  -1,
	8,  -1,  -1,  12,  -1
};


// money label types: tt, atv, current/previous, type
static const uint16 label_type[] =
{
	TT_ALL, ATV_TRANSPORTED,                0, STANDARD,
	TT_ALL, ATV_TRANSPORTED,                1, STANDARD,
	TT_ALL, ATV_REVENUE_TRANSPORT,          0, MONEY,
	TT_ALL, ATV_REVENUE_TRANSPORT,          1, MONEY,
	TT_ALL, ATV_RUNNING_COST,               0, MONEY,
	TT_ALL, ATV_RUNNING_COST,               1, MONEY,
	TT_ALL, ATV_INFRASTRUCTURE_MAINTENANCE, 0, MONEY,
	TT_ALL, ATV_INFRASTRUCTURE_MAINTENANCE, 1, MONEY,
	TT_ALL, ATV_WAY_TOLL,                   0, MONEY,
	TT_ALL, ATV_WAY_TOLL,                   1, MONEY,
	TT_ALL, ATV_OPERATING_PROFIT,           0, MONEY,
	TT_ALL, ATV_OPERATING_PROFIT,           1, MONEY,
	TT_ALL, ATV_NEW_VEHICLE,                0, MONEY,
	TT_ALL, ATV_NEW_VEHICLE,                1, MONEY,
	TT_ALL, ATV_CONSTRUCTION_COST,          0, MONEY,
	TT_ALL, ATV_CONSTRUCTION_COST,          1, MONEY,
	TT_ALL, ATV_PROFIT,                     0, MONEY,
	TT_ALL, ATV_PROFIT,                     1, MONEY,
	TT_MAX, ATC_CASH,                       0, MONEY,
	TT_ALL, ATV_NON_FINANCIAL_ASSETS,       0, MONEY,
	TT_ALL, ATV_PROFIT_MARGIN,              0, PERCENT,
	TT_MAX, ATC_NETWEALTH,                  0, MONEY
};

static const sint8 cell_to_moneylabel[] =
{
	-1,   0,   1,  -1,  -1,
	-1,   2,   3,  -1,  -1,
	-1,   4,   5,  -1,  -1,
	-1,   6,   7,  -1,  -1,
	-1,   8,   9,  -1,  -1,
	-1,  10,  11,  -1,  18,
	-1,  12,  13,  -1,  19,
	-1,  14,  15,  -1,  20,
	-1,  16,  17,  -1,  21,
};

/* order has to be same as in enum transport_type in file finance.h */
/* Also these have to match the strings in simline_t::linetype2name! */
/* (and it is sad that the order between those do not match ...) */
const char * transport_type_values[TT_MAX] = {
	"All",
	"Truck",
	"Train",
	"Ship",
	"Monorail",
	"Maglev",
	"Tram",
	"Narrowgauge",
	"Air",
	"tt_Other",
	"Powerlines",
};

/// Helper method to query data from players statistics
sint64 money_frame_t::get_statistics_value(int tt, uint8 type, int yearmonth, bool monthly)
{
	const finance_t* finance = player->get_finance();
	if (tt == TT_MAX) {
		return monthly ? finance->get_history_com_month(yearmonth, type)
		               : finance->get_history_com_year( yearmonth, type);
	}
	else {
		assert(0 <= tt  &&  tt < TT_MAX);
		return monthly ? finance->get_history_veh_month((transport_type)tt, yearmonth, type)
		               : finance->get_history_veh_year( (transport_type)tt, yearmonth, type);
	}
}

class money_frame_label_t : public gui_label_buf_t
{
	uint8 transport_type;
	uint8 type;
	uint8 label_type;
	uint8 index;
	bool monthly;

public:
	money_frame_label_t(uint8 tt, uint8 t, uint8 lt, uint8 i, bool mon)
	: gui_label_buf_t(lt == STANDARD ? SYSCOL_TEXT : MONEY_PLUS, lt != MONEY ? gui_label_t::right : gui_label_t::money_right)
	, transport_type(tt), type(t), label_type(lt), index(i), monthly(mon)
	{
	}

	void update(money_frame_t *mf)
	{
		uint8 tt = transport_type == TT_ALL ? mf->transport_type_option : transport_type;
		sint64 value = mf->get_statistics_value(tt, type, index, monthly ? 1 : 0);
		PIXVAL color = value >= 0 ? (value > 0 ? MONEY_PLUS : SYSCOL_TEXT_UNUSED) : MONEY_MINUS;

		switch (label_type) {
			case MONEY:
				buf().append_money(value / 100.0);
				break;
			case PERCENT:
				buf().append(value / 100.0, 2);
				buf().append("%");
				break;
			default:
				buf().append(value * 1.0, 0);
		}
		gui_label_buf_t::update();
		set_color(color);
	}
};


void money_frame_t::fill_chart_tables()
{
	// fill tables for chart curves
	for (int i = 0; i<MAX_PLAYER_COST_BUTTON; i++) {
		const uint8 tt = cost_type[3*i+1] == TT_ALL ? transport_type_option : cost_type[3*i+1];

		for(int j=0; j<MAX_PLAYER_HISTORY_MONTHS; j++) {
			chart_table_month[j][i] = get_statistics_value(tt, cost_type[3*i], j, true);
		}

		for(int j=0; j<MAX_PLAYER_HISTORY_YEARS; j++) {
			chart_table_year[j][i] =  get_statistics_value(tt, cost_type[3*i], j, false);
		}
	}
}


bool money_frame_t::is_chart_table_zero(int ttoption)
{
	if (player->get_finance()->get_maintenance_with_bits((transport_type)ttoption) != 0) {
		return false;
	}
	// search for any non-zero values
	for (int i = 0; i<MAX_PLAYER_COST_BUTTON; i++) {
		const uint8 tt = cost_type[3*i+1] == TT_ALL ? ttoption : cost_type[3*i+1];

		if (tt == TT_MAX  &&  ttoption != TT_ALL) {
			continue;
		}

		for(int j=0; j<MAX_PLAYER_HISTORY_MONTHS; j++) {
			if (get_statistics_value(tt, cost_type[3*i], j, true) != 0) {
				return false;
			}
		}

		for(int j=0; j<MAX_PLAYER_HISTORY_YEARS; j++) {
			if (get_statistics_value(tt, cost_type[3*i], j, false) != 0) {
				return false;
			}
		}
	}
	return true;
}


money_frame_t::money_frame_t(player_t *player) :
	gui_frame_t( translator::translate("Finanzen"), player),
	maintenance_money(MONEY_PLUS, gui_label_t::money_right),
	scenario_desc(SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left),
	scenario_completion(SYSCOL_TEXT, gui_label_t::left),
	warn(SYSCOL_TEXT_STRONG, gui_label_t::left),
	transport_type_option(0)
{
	if(welt->get_player(0)!=player) {
		money_frame_title.printf(translator::translate("Finances of %s"), translator::translate(player->get_name()) );
		set_name(money_frame_title);
	}

	this->player = player;

	set_table_layout(1,0);

	// bankruptcy notice
	add_component(&warn);
	warn.set_visible(false);

	// scenario name
	if(player->get_player_nr()!=1  &&  welt->get_scenario()->active()) {

		add_table(3,1);
		new_component<gui_label_t>("Scenario_", SYSCOL_TEXT_HIGHLIGHT);
		add_component(&scenario_desc);
		add_component(&scenario_completion);
		end_table();
	}

	// select transport type
	gui_aligned_container_t *top = add_table(4,1);
	{
		new_component<gui_label_t>("Show finances for transport type");

		transport_type_c.set_selection(0);
		transport_type_c.set_focusable( false );

		for(int i=0, count=0; i<TT_MAX; ++i) {
			if (!is_chart_table_zero(i)) {
				transport_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(transport_type_values[i]), SYSCOL_TEXT);
				transport_types[ count++ ] = i;
			}
		}

		add_component(&transport_type_c);
		transport_type_c.add_listener( this );

		transport_type_c.set_selection(0);
		top->set_focus( &transport_type_c );
		set_focus(top);

		new_component<gui_fill_t>();

		add_component(&headquarter);
		headquarter.init(button_t::roundbox, "");
		headquarter.add_listener(this);
	}
	end_table();

	// tab panels
	// tab (month/year)
	year_month_tabs.add_tab( &container_year, translator::translate("Years"));
	year_month_tabs.add_tab( &container_month, translator::translate("Months"));
	year_month_tabs.add_listener(this);
	add_component(&year_month_tabs);

	// fill both containers
	gui_aligned_container_t *current = &container_year;
	gui_chart_t *current_chart = &chart;
	// .. put the same buttons in both containers
	button_t* buttons[MAX_PLAYER_COST_BUTTON];

	for(uint8 i = 0; i < 2 ; i++) {
		uint8 k = 0;
		current->set_table_layout(1,0);

		current->add_table(5,10);

		// first row: some labels
		current->new_component<gui_empty_t>();
		current->new_component<gui_label_t>(i==0 ? "This Year" : "This Month", SYSCOL_TEXT_HIGHLIGHT);
		current->new_component<gui_label_t>(i==0 ? "Last Year" : "Last Month", SYSCOL_TEXT_HIGHLIGHT);
		current->new_component<gui_empty_t>();
		current->new_component<gui_empty_t>();

		// all other rows: mix of buttons and money-labels
		for(uint8 r = 0; r < 9; r++) {
			for(uint8 c = 0; c < 5; c++, k++) {
				sint8 cost = cell_to_buttons[k];
				sint8 l = cell_to_moneylabel[k];
				// button + chart line
				if (cost >=0 ) {
					// add chart line
					const int curve_type = cost_type[3*cost+2];
					const int curve_precision = curve_type == STANDARD ? 0 : 2;
					sint16 curve = i == 0
					? chart.add_curve(  color_idx_to_rgb(cost_type_color[cost]), *chart_table_year,  MAX_PLAYER_COST_BUTTON, cost, MAX_PLAYER_HISTORY_YEARS,  curve_type, false, true, curve_precision)
					: mchart.add_curve( color_idx_to_rgb(cost_type_color[cost]), *chart_table_month, MAX_PLAYER_COST_BUTTON, cost, MAX_PLAYER_HISTORY_MONTHS, curve_type, false, true, curve_precision);
					// add button
					button_t *b;
					if (i == 0) {
						b = current->new_component<button_t>();
						b->init(button_t::box_state_automatic | button_t::flexible, cost_type_name[cost]);
						b->background_color = color_idx_to_rgb(cost_type_color[cost]);
						b->pressed = false;
						buttons[cost] = b;
					}
					else {
						b = buttons[cost];
						current->add_component(b);
					}
					button_to_chart.append(b, current_chart, curve);
				}
				else if (l >= 0) {
					// money_frame_label_t(uint8 tt, uint8 t, uint8 lt, uint8 i, bool mon)
					money_labels.append( current->new_component<money_frame_label_t>(label_type[4*l], label_type[4*l+1], label_type[4*l+3], label_type[4*l+2], i==1) );
				}
				else {
					if (r >= 2  &&  r<=4  &&  c == 4) {
						switch(r) {
							case 2: current->new_component<gui_label_t>("This Month", SYSCOL_TEXT_HIGHLIGHT); break;
							case 3: current->add_component(&maintenance_money); break;
							case 4: current->new_component<gui_label_t>("This Year", SYSCOL_TEXT_HIGHLIGHT);
						}
					}
					else {
						current->new_component<gui_empty_t>();
					}
				}
			}
		}
		current->end_table();

		current->add_component(current_chart);
		current = &container_month;
		current_chart = &mchart;
	}

	// recover button states
	if (bFilterStates.get_count() > 0) {
		for(uint8 i = 0; i<bFilterStates.get_count(); i++) {
			button_to_chart[i]->get_button()->pressed = bFilterStates[i] > 0;
			button_to_chart[i]->update();
		}
	}

	chart.set_min_size(scr_size(0 ,8*BUTTONSPACE));
	chart.set_dimension(MAX_PLAYER_HISTORY_YEARS, 10000);
	chart.set_seed(welt->get_last_year());
	chart.set_background(SYSCOL_CHART_BACKGROUND);

	mchart.set_min_size(scr_size(0,8*BUTTONSPACE));
	mchart.set_dimension(MAX_PLAYER_HISTORY_MONTHS, 10000);
	mchart.set_seed(0);
	mchart.set_background(SYSCOL_CHART_BACKGROUND);

	update_labels();

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
	set_resizemode(diagonal_resize);
}


money_frame_t::~money_frame_t()
{
	bFilterStates.clear();
	// save button states
	for(gui_button_to_chart_t* b2c : button_to_chart.list()) {
		bFilterStates.append( b2c->get_button()->pressed ? 1 : 0);
	}
}

void money_frame_t::update_labels()
{
	for(money_frame_label_t* lb : money_labels) {
		lb->update(this);
	}

	// scenario
	if(player->get_player_nr()!=1  &&  welt->get_scenario()->active()) {
		welt->get_scenario()->update_scenario_texts();
		scenario_desc.buf().append( welt->get_scenario()->description_text );
		scenario_desc.buf().append( ":" );
		scenario_desc.update();

		sint32 percent = welt->get_scenario()->get_completion(player->get_player_nr());
		if (percent >= 0) {
			scenario_completion.buf().printf(translator::translate("Scenario complete: %i%%"), percent );
		}
		else {
			scenario_completion.buf().printf(translator::translate("Scenario lost!"));
		}
		scenario_completion.update();
	}

	// update headquarter button
	headquarter.disable();
	if(  player->get_ai_id()!=player_t::HUMAN  ) {
		headquarter.set_tooltip( "Configure AI setttings" );
		headquarter.set_text( "Configure AI" );
		headquarter.enable();
	}
	else {
		koord pos = player->get_headquarter_pos();
		headquarter.set_text( pos != koord::invalid ? "show HQ" : "build HQ" );
		headquarter.set_tooltip( NULL );

		if (pos == koord::invalid) {
			if(player == welt->get_active_player()) {
				// reuse tooltip from tool_headquarter_t
				const char * c = tool_t::general_tool[TOOL_HEADQUARTER]->get_tooltip(player);
				if(c) {
					// only true, if the headquarter can be built/updated
					headquarter_tooltip.clear();
					headquarter_tooltip.append(c);
					headquarter.set_tooltip( headquarter_tooltip );
					headquarter.enable();
				}

			}
		}
		else {
			headquarter.enable();
		}
	}

	// current maintenance
	double maintenance = player->get_finance()->get_maintenance_with_bits((transport_type)transport_type_option) / 100.0;
	maintenance_money.append_money(-maintenance);
	maintenance_money.update();

	// bankruptcy warning
	bool visible = warn.is_visible();
	if(player->get_finance()->get_history_com_year(0, ATC_NETWEALTH)<0) {
		warn.set_color( MONEY_MINUS );
		warn.buf().append( translator::translate("Company bankrupt") );
		warn.set_visible(true);
	}
	else if(  player->get_finance()->get_history_com_year(0, ATC_NETWEALTH)*10 < welt->get_settings().get_starting_money(welt->get_current_month()/12)  ){
		warn.set_color( MONEY_MINUS );
		warn.buf().append( translator::translate("Net wealth near zero") );
		warn.set_visible(true);
	}
	else if(  player->get_account_overdrawn()  ) {
		warn.set_color( SYSCOL_TEXT_STRONG );
		warn.buf().printf(translator::translate("On loan since %i month(s)"), player->get_account_overdrawn() );
		warn.set_visible(true);
	}
	else {
		warn.set_visible(false);
	}
	warn.update();
	if (visible != warn.is_visible()) {
		resize(scr_size(0,0));
	}
}


void money_frame_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);
	// recompute the active container
	if (year_month_tabs.get_active_tab_index() == 0) {
		container_year.set_size( container_year.get_size() );
	}
	else {
		container_month.set_size( container_month.get_size() );
	}
}


void money_frame_t::draw(scr_coord pos, scr_size size)
{

	player->get_finance()->calc_finance_history();
	fill_chart_tables();
	update_labels();

	// update chart seed
	chart.set_seed(welt->get_last_year());

	gui_frame_t::draw(pos, size);
}


bool money_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(  comp == &headquarter  ) {
		if(  player->get_ai_id()!=player_t::HUMAN  ) {
			create_win( new ai_option_t(player), w_info, magic_ai_options_t+player->get_player_nr() );
		}
		else {
			if (player->get_headquarter_pos() == koord::invalid) {
				welt->set_tool( tool_t::general_tool[TOOL_HEADQUARTER], player );
			}
			else {
				// open dedicated HQ window
				create_win( new headquarter_info_t(player), w_info, magic_headquarter+player->get_player_nr() );
			}
		}
		return true;
	}
	if (comp == &year_month_tabs) {
		if (year_month_tabs.get_active_tab_index() == 0) {
			container_year.set_size( container_year.get_size() );
		}
		else {
			container_month.set_size( container_month.get_size() );
		}
	}
	if(  comp == &transport_type_c) {
		int tmp = transport_type_c.get_selection();
		if((0 <= tmp) && (tmp < transport_type_c.count_elements())) {
			transport_type_option = transport_types[tmp];
		}
		return true;
	}
	return false;
}


bool money_frame_t::infowin_event(const event_t *ev)
{
	bool swallowed = gui_frame_t::infowin_event(ev);
	set_focus( &transport_type_c );
	return swallowed;
}


uint32 money_frame_t::get_rdwr_id()
{
	return magic_finances_t+player->get_player_nr();
}


void money_frame_t::rdwr( loadsave_t *file )
{
	// button-to-chart array
	button_to_chart.rdwr(file);

	year_month_tabs.rdwr(file);

	file->rdwr_short(transport_type_option);
	if (file->is_loading()) {
		for(int i=0; i<transport_type_c.count_elements(); i++) {
			if (transport_types[i] == transport_type_option) {
				transport_type_c.set_selection(i);
				break;
			}
		}
	}
}
