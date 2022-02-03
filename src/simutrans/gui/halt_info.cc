/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <algorithm>

#include "halt_info.h"
#include "components/gui_button_to_chart.h"

#include "../world/simworld.h"
#include "../simware.h"
#include "../simcolor.h"
#include "../simconvoi.h"
#include "../simfab.h"
#include "../simintr.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../tool/simmenu.h"
#include "../simskin.h"
#include "../simline.h"

#include "../freight_list_sorter.h"

#include "../dataobj/schedule.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"

#include "../player/simplay.h"

#include "../vehicle/vehicle.h"

#include "../utils/simstring.h"

#include "../descriptor/skin_desc.h"


#define CHART_HEIGHT (100)

/**
 * Window with destination information for a stop
 */
class gui_departure_board_t : public gui_aligned_container_t
{
	// helper class to compute departure board
	class dest_info_t {
	public:
		bool compare( const dest_info_t &other ) const;
		halthandle_t halt;
		sint32 delta_ticks;
		convoihandle_t cnv;

		dest_info_t() : delta_ticks(0) {}
		dest_info_t( halthandle_t h, sint32 d_t, convoihandle_t c) : halt(h), delta_ticks(d_t), cnv(c) {}
		bool operator == (const dest_info_t &other) const { return ( this->cnv==other.cnv ); }
	};

	static bool compare_hi(const dest_info_t &a, const dest_info_t &b) { return a.delta_ticks <= b.delta_ticks; }
	static karte_ptr_t welt;

	vector_tpl<dest_info_t> destinations;
	vector_tpl<dest_info_t> origins;

	static button_t absolute_times;

	uint32 calc_ticks_until_arrival( convoihandle_t cnv );

	void insert_image(convoihandle_t cnv);

public:

	// if nothing changed, this is the next refresh to recalculate the content of the departure board
	sint8 next_refresh;

	gui_departure_board_t() : gui_aligned_container_t()
	{
		next_refresh = -1;
		set_table_layout(3,0);
		add_component( &absolute_times );
		absolute_times.init( button_t::square_automatic, "Absolute times" );
	}

	void update_departures(halthandle_t halt);

	scr_size get_max_size() const OVERRIDE { return get_min_size(); }
};

button_t gui_departure_board_t::absolute_times;


// all connections
class gui_halt_detail_t : public gui_aligned_container_t
{
	/**
	 * Button to open line window
	 */
	class gui_line_button_t : public button_t, public action_listener_t
	{
		static karte_ptr_t welt;
		linehandle_t line;
	public:
		gui_line_button_t(linehandle_t line) : button_t()
		{
			this->line = line;
			init(button_t::posbutton, NULL);
			add_listener(this);
		}

		bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE
		{
			player_t *player = welt->get_active_player();
			if(  player == line->get_owner()  ) {
				player->simlinemgmt.show_lineinfo(player, line, 3);
			}
			return true;
		}

		void draw(scr_coord offset) OVERRIDE
		{
			if (line->get_owner() == welt->get_active_player()) {
				button_t::draw(offset);
			}
		}
	};

	/**
	 * Button to open convoi window
	 */
	class gui_convoi_button_t : public button_t, public action_listener_t
	{
		convoihandle_t convoi;
	public:
		gui_convoi_button_t(convoihandle_t convoi) : button_t() {
			this->convoi = convoi;
			init(button_t::posbutton, NULL);
			add_listener(this);
		}

		bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE {
			convoi->open_info_window();
			return true;
		}
	};

private:
	static bool compare_connection(haltestelle_t::connection_t const& a, haltestelle_t::connection_t const& b)
	{
		return strcmp(a.halt->get_name(), b.halt->get_name()) <=0;
	}

	static bool compare_line (linehandle_t a, linehandle_t b)
	{
		return a->get_linetype() == b->get_linetype() ?
			strcmp(a->get_name(), b->get_name()) <= 0
			: strcmp(a->get_linetype_name(a->get_linetype()), b->get_linetype_name(b->get_linetype())) <= 0;
	}

	uint32 cached_line_count;
	uint32 cached_convoy_count;

	void insert_empty_row() {
		new_component<gui_label_t>("     ");
		new_component<gui_empty_t>();
	}

	void insert_show_nothing() {
		new_component<gui_empty_t>();
		new_component<gui_label_t>("keine");
	}

public:
	uint8 destination_counter; // last destination counter of the halt; if mismatch to current, then redraw destinations

	gui_halt_detail_t(halthandle_t h) : gui_aligned_container_t()
	{
		destination_counter = 0xFF;
		cached_line_count = 0xFFFFFFFFul;
		cached_convoy_count = 0xFFFFFFFFul;
		update_connections(h);
	}

	// will update if schedule or connection changed
	void update_connections(halthandle_t h);

	scr_size get_max_size() const OVERRIDE { return get_min_size(); }
};

karte_ptr_t gui_departure_board_t::welt;
karte_ptr_t gui_halt_detail_t::gui_line_button_t::welt;


static const char *sort_text[4] = {
	"Zielort",
	"via",
	"via Menge",
	"Menge"
};

const char cost_type[MAX_HALT_COST][64] =
{
	"Happy",
	"Unhappy",
	"No Route",
	"hl_btn_sort_waiting",
	"Arrived",
	"Departed",
	"Convoys",
	"Walked"
};

const uint8 index_of_haltinfo[MAX_HALT_COST] = {
	HALT_HAPPY,
	HALT_UNHAPPY,
	HALT_NOROUTE,
	HALT_WAITING,
	HALT_ARRIVED,
	HALT_DEPARTED,
	HALT_CONVOIS_ARRIVED,
	HALT_WALKED
};

const uint8 cost_type_color[MAX_HALT_COST] =
{
	COL_HAPPY,
	COL_UNHAPPY,
	COL_NO_ROUTE,
	COL_WAITING,
	COL_ARRIVED,
	COL_DEPARTED,
	COL_CONVOI_COUNT,
	COL_LILAC
};

struct type_symbol_t {
	haltestelle_t::stationtyp type;
	const skin_desc_t **desc;
};

const type_symbol_t symbols[] = {
	{ haltestelle_t::railstation, &skinverwaltung_t::zughaltsymbol },
	{ haltestelle_t::loadingbay, &skinverwaltung_t::autohaltsymbol },
	{ haltestelle_t::busstop, &skinverwaltung_t::bushaltsymbol },
	{ haltestelle_t::dock, &skinverwaltung_t::schiffshaltsymbol },
	{ haltestelle_t::airstop, &skinverwaltung_t::airhaltsymbol },
	{ haltestelle_t::monorailstop, &skinverwaltung_t::monorailhaltsymbol },
	{ haltestelle_t::tramstop, &skinverwaltung_t::tramhaltsymbol },
	{ haltestelle_t::maglevstop, &skinverwaltung_t::maglevhaltsymbol },
	{ haltestelle_t::narrowgaugestop, &skinverwaltung_t::narrowgaugehaltsymbol }
};


// helper class
gui_halt_type_images_t::gui_halt_type_images_t(halthandle_t h)
{
	halt = h;
	set_table_layout(lengthof(symbols), 1);
	set_alignment(ALIGN_LEFT | ALIGN_CENTER_V);
	assert( lengthof(img_transport) == lengthof(symbols) );
	// indicator for supplied transport modes
	haltestelle_t::stationtyp const halttype = halt->get_station_type();
	for(uint i=0; i < lengthof(symbols); i++) {
		if ( *symbols[i].desc ) {
			add_component(img_transport + i);
			img_transport[i].set_image( (*symbols[i].desc)->get_image_id(0));
			img_transport[i].enable_offset_removal(true);
			img_transport[i].set_visible( (halttype & symbols[i].type) != 0);
		}
	}
}

void gui_halt_type_images_t::draw(scr_coord offset)
{
	haltestelle_t::stationtyp const halttype = halt->get_station_type();
	for(uint i=0; i < lengthof(symbols); i++) {
		img_transport[i].set_visible( (halttype & symbols[i].type) != 0);
	}
	gui_aligned_container_t::draw(offset);
}

// main class
halt_info_t::halt_info_t(halthandle_t halt) :
		gui_frame_t("", NULL),
		departure_board( new gui_departure_board_t()),
		halt_detail( new gui_halt_detail_t(halt)),
		text_freight(&freight_info),
		scrolly_freight(&container_freight, true, true),
		scrolly_departure(departure_board, true, true),
		scrolly_details(halt_detail, true, true),
		view(koord3d::invalid, scr_size(max(64, get_base_tile_raster_width()), max(56, get_base_tile_raster_width() * 7 / 8)))
{
	if (halt.is_bound()) {
		init(halt);
	}
}

void halt_info_t::init(halthandle_t halt)
{
	this->halt = halt;
	set_name(halt->get_name());
	set_owner(halt->get_owner() );

	halt->set_sortby( env_t::default_sortmode );

	set_table_layout(1,0);

	// top part
	add_table(2,1)->set_alignment(ALIGN_CENTER_H);
	{

		container_top = add_table(1,0);
		{
			// input name
			tstrncpy(edit_name, halt->get_name(), lengthof(edit_name));
			input.set_text(edit_name, lengthof(edit_name));
			input.add_listener(this);
			add_component(&input, 2);

			// status images
			add_table(5,1)->set_alignment(ALIGN_CENTER_V);
			{
				add_component(&indicator_color);
				// indicator for enabled freight type
				img_enable[0].set_image(skinverwaltung_t::passengers->get_image_id(0));
				img_enable[1].set_image(skinverwaltung_t::mail->get_image_id(0));
				img_enable[2].set_image(skinverwaltung_t::goods->get_image_id(0));

				for(uint i=0; i<3; i++) {
					add_component(img_enable + i);
					img_enable[i].enable_offset_removal(true);
				}
				img_types = new_component<gui_halt_type_images_t>(halt);
			}
			end_table();
			// capacities
			add_table(6,1);
			{
				add_component(&lb_capacity[0]);
				if (welt->get_settings().is_separate_halt_capacities()) {
					new_component<gui_image_t>(skinverwaltung_t::passengers->get_image_id(0), 0, ALIGN_NONE, true);
					add_component(&lb_capacity[1]);
					new_component<gui_image_t>(skinverwaltung_t::mail->get_image_id(0), 0, ALIGN_NONE, true);
					add_component(&lb_capacity[2]);
					new_component<gui_image_t>(skinverwaltung_t::goods->get_image_id(0), 0, ALIGN_NONE, true);
				}
			}
			end_table();
			// happy / unhappy / no route
			add_table(6,1);
			{
				add_component(&lb_happy[0]);
				if (skinverwaltung_t::happy && skinverwaltung_t::unhappy && skinverwaltung_t::no_route) {
					new_component<gui_image_t>(skinverwaltung_t::happy->get_image_id(0), 0, ALIGN_NONE, true);
					add_component(&lb_happy[1]);
					new_component<gui_image_t>(skinverwaltung_t::unhappy->get_image_id(0), 0, ALIGN_NONE, true);
					add_component(&lb_happy[2]);
					new_component<gui_image_t>(skinverwaltung_t::no_route->get_image_id(0), 0, ALIGN_NONE, true);
				}
			}
			end_table();
		}
		end_table();

		add_component(&view);
		view.set_location(halt->get_basis_pos3d());
	}
	end_table();


	// tabs: waiting, departure, chart

	add_component(&switch_mode);
	switch_mode.add_listener(this);
	switch_mode.add_tab(&scrolly_freight, translator::translate("Hier warten/lagern:"));

	// list of waiting cargo
	// sort mode
	container_freight.set_table_layout(1,0);
	container_freight.add_table(2,1);
	container_freight.new_component<gui_label_t>("Sort waiting list by");

	// added sort_button
	sort_button.init(button_t::roundbox, sort_text[env_t::default_sortmode]);
	sort_button.set_tooltip("Sort waiting list by");
	sort_button.add_listener(this);
	container_freight.add_component(&sort_button);
	container_freight.end_table();

	container_freight.add_component(&text_freight);

	// departure board
	switch_mode.add_tab(&scrolly_departure, translator::translate("Departure board"));
	departure_board->update_departures(halt);
	departure_board->set_size( departure_board->get_min_size() );

	// connection info
	switch_mode.add_tab(&scrolly_details, translator::translate("Connections"));
	halt_detail->update_connections(halt);
	halt_detail->set_size( halt_detail->get_min_size() );

	// chart
	switch_mode.add_tab(&container_chart, translator::translate("Chart"));
	container_chart.set_table_layout(1,0);

	chart.set_min_size(scr_size(0, CHART_HEIGHT));
	chart.set_dimension(12, 10000);
	chart.set_background(SYSCOL_CHART_BACKGROUND);
	container_chart.add_component(&chart);

	container_chart.add_table(4,2);
	for (int cost = 0; cost<MAX_HALT_COST; cost++) {
		uint16 curve = chart.add_curve(color_idx_to_rgb(cost_type_color[cost]), halt->get_finance_history(), MAX_HALT_COST, index_of_haltinfo[cost], MAX_MONTHS, 0, false, true, 0);

		button_t *b = container_chart.new_component<button_t>();
		b->init(button_t::box_state_automatic | button_t::flexible, cost_type[cost]);
		b->background_color = color_idx_to_rgb(cost_type_color[cost]);
		b->pressed = false;

		button_to_chart.append(b, &chart, curve);
	}
	container_chart.end_table();

	update_components();
	set_resizemode(diagonal_resize);
	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
}


halt_info_t::~halt_info_t()
{
	if(  halt.is_bound()  &&  strcmp(halt->get_name(),edit_name)  &&  edit_name[0]  ) {
		// text changed => call tool
		cbuffer_t buf;
		buf.printf( "h%u,%s", halt.get_id(), edit_name );
		tool_t *tool = create_tool( TOOL_RENAME | SIMPLE_TOOL );
		tool->set_default_param( buf );
		welt->set_tool( tool, halt->get_owner() );
		// since init always returns false, it is safe to delete immediately
		delete tool;
	}
	delete departure_board;
	delete halt_detail;
}


koord3d halt_info_t::get_weltpos(bool)
{
	return halt->get_basis_pos3d();
}


bool halt_info_t::is_weltpos()
{
	return ( welt->get_viewport()->is_on_center(get_weltpos(false)));
}


void halt_info_t::update_components()
{
	indicator_color.set_color(halt->get_status_farbe());

	lb_capacity[0].buf().printf("%s: %u", translator::translate("Storage capacity"), halt->get_capacity(0));
	lb_capacity[0].update();
	lb_capacity[1].buf().printf("  %u", halt->get_capacity(1));
	lb_capacity[1].update();
	lb_capacity[2].buf().printf("  %u", halt->get_capacity(2));
	lb_capacity[2].update();

	if (skinverwaltung_t::happy && skinverwaltung_t::unhappy && skinverwaltung_t::no_route) {
		lb_happy[0].buf().printf("%s: %u", translator::translate("Passagiere"), halt->get_pax_happy());
		lb_happy[0].update();
		lb_happy[1].buf().printf("  %u", halt->get_pax_unhappy());
		lb_happy[1].update();
		lb_happy[2].buf().printf("  %u", halt->get_pax_no_route());
		lb_happy[2].update();
	}
	else {
		if(  has_character( 0x263A )  ) {
			utf8 happy[4], unhappy[4];
			happy[ utf16_to_utf8( 0x263A, happy ) ] = 0;
			unhappy[ utf16_to_utf8( 0x2639, unhappy ) ] = 0;
			lb_happy[0].buf().printf(translator::translate("Passengers %d %s, %d %s, %d no route"), halt->get_pax_happy(), happy, halt->get_pax_unhappy(), unhappy, halt->get_pax_no_route());
		}
		else if(  has_character( 0x30 )  ) {
			lb_happy[0].buf().printf(translator::translate("Passengers %d %c, %d %c, %d no route"), halt->get_pax_happy(), 30, halt->get_pax_unhappy(), 31, halt->get_pax_no_route());
		}
		else {
			lb_happy[0].buf().printf(translator::translate("Passengers %d %c, %d %c, %d no route"), halt->get_pax_happy(), '+', halt->get_pax_unhappy(), '-', halt->get_pax_no_route());
		}
		lb_happy[0].update();
	}

	img_enable[0].set_visible(halt->get_pax_enabled());
	img_enable[1].set_visible(halt->get_mail_enabled());
	img_enable[2].set_visible(halt->get_ware_enabled());
	container_top->set_size( container_top->get_size());

	// buffer update now only when needed by halt itself => dedicated buffer for this
	int old_len = freight_info.len();
	halt->get_freight_info(freight_info);
	if(  old_len != freight_info.len()  ) {
		text_freight.recalc_size();
		container_freight.set_size(container_freight.get_min_size());
	}

	if (switch_mode.get_aktives_tab() == &scrolly_departure) {
		departure_board->update_departures(halt);
	}
	if (switch_mode.get_aktives_tab() == &scrolly_details) {
		halt_detail->update_connections(halt);
	}
	set_dirty();
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 */
void halt_info_t::draw(scr_coord pos, scr_size size)
{
	assert(halt.is_bound());

	update_components();

	gui_frame_t::draw(pos, size);
}


void gui_halt_detail_t::update_connections( halthandle_t halt )
{
	if(  !halt.is_bound()  ) {
		// first call, or invalid handle
		remove_all();
		return;
	}

	if(  halt->get_reconnect_counter()==destination_counter  &&
		 halt->registered_lines.get_count()==cached_line_count  &&  halt->registered_convoys.get_count()==cached_convoy_count  ) {
		// all current, so do nothing
		return;
	}

	// update connections from here
	remove_all();

	const slist_tpl<fabrik_t *> & fab_list = halt->get_fab_list();
	slist_tpl<const goods_desc_t *> nimmt_an;

	set_table_layout(2,0);
	new_component_span<gui_label_t>("Fabrikanschluss", 2);

	if (!fab_list.empty()) {
		FOR(slist_tpl<fabrik_t*>, const fab, fab_list) {
			const koord3d pos = fab->get_pos();

			// target button ...
			button_t *pb = new_component<button_t>();
			pb->init( button_t::posbutton_automatic, NULL);
			pb->set_targetpos3d( pos );

			// .. name
			gui_label_buf_t *lb = new_component<gui_label_buf_t>();
			lb->buf().printf("%s (%d, %d)\n", translator::translate(fab->get_name()), pos.x, pos.y);
			lb->update();

			FOR(array_tpl<ware_production_t>, const& i, fab->get_input()) {
				goods_desc_t const* const ware = i.get_typ();
				if(!nimmt_an.is_contained(ware)) {
					nimmt_an.append(ware);
				}
			}
		}
	}
	else {
		insert_show_nothing();
	}

	insert_empty_row();
	new_component_span<gui_label_t>("Angenommene Waren", 2);

	if (!nimmt_an.empty()  &&  halt->get_ware_enabled()) {
		for(uint32 i=0; i<goods_manager_t::get_count(); i++) {
			const goods_desc_t *ware = goods_manager_t::get_info(i);
			if(nimmt_an.is_contained(ware)) {

				new_component<gui_empty_t>();
				new_component<gui_label_t>(ware->get_name());
			}
		}
	}
	else {
		insert_show_nothing();
	}

	insert_empty_row();
	// add lines that serve this stop
	new_component_span<gui_label_t>("Lines serving this stop", 2);

	if(  !halt->registered_lines.empty()  ) {
		simline_t::linetype previous_linetype = simline_t::MAX_LINE_TYPE;

		vector_tpl<linehandle_t> sorted_lines;
		FOR(vector_tpl<linehandle_t>, l, halt->registered_lines) {
			if(  l.is_bound()  ) {
				sorted_lines.insert_unique_ordered( l, gui_halt_detail_t::compare_line );
			}
		}

		FOR(vector_tpl<linehandle_t>, line, sorted_lines) {

			// Linetype if it is the first
			if(  line->get_linetype() != previous_linetype  ) {
				previous_linetype = line->get_linetype();
				new_component_span<gui_label_t>(simline_t::get_linetype_name(previous_linetype),2);
			}

			new_component<gui_line_button_t>(line);

			// Line labels with color of player
			gui_label_buf_t *lb = new_component<gui_label_buf_t>(PLAYER_FLAG | color_idx_to_rgb(line->get_owner()->get_player_color1()+env_t::gui_player_color_dark) );
			lb->buf().append( line->get_name() );
			lb->update();
		}
	}
	else {
		insert_show_nothing();
	}

	insert_empty_row();
	// add lineless convoys which serve this stop
	new_component_span<gui_label_t>("Lineless convoys serving this stop", 2);
	if(  !halt->registered_convoys.empty()  ) {
		for(  uint32 i=0;  i<halt->registered_convoys.get_count();  ++i  ) {

			convoihandle_t cnv = halt->registered_convoys[i];
			// Convoy buttons
			new_component<gui_convoi_button_t>(cnv);

			// Line labels with color of player
			gui_label_buf_t *lb = new_component<gui_label_buf_t>(PLAYER_FLAG | color_idx_to_rgb(cnv->get_owner()->get_player_color1()+env_t::gui_player_color_dark) );
			lb->buf().append( cnv->get_name() );
			lb->update();
		}
	}
	else {
		insert_show_nothing();
	}

	insert_empty_row();
	new_component_span<gui_label_t>("Direkt erreichbare Haltestellen", 2);

	bool has_stops = false;

	for (uint i=0; i<goods_manager_t::get_max_catg_index(); i++){
		vector_tpl<haltestelle_t::connection_t> const& connections = halt->get_connections(i);
		if(  !connections.empty()  ) {

			gui_label_buf_t *lb = new_component_span<gui_label_buf_t>(2);
			lb->buf().append(" \xC2\xB7");
			const goods_desc_t* info = goods_manager_t::get_info_catg_index(i);
			// If it is a special freight, we display the name of the good, otherwise the name of the category.
			lb->buf().append(translator::translate(info->get_catg()==0 ? info->get_name() : info->get_catg_name() ) );
#if MSG_LEVEL>=4
			if(  halt->is_transfer(i)  ) {
				lb->buf().append("*");
			}
#endif
			lb->buf().append(":\n");
			lb->update();

			vector_tpl<haltestelle_t::connection_t> sorted;
			FOR(vector_tpl<haltestelle_t::connection_t>, const& conn, connections) {
				if(  conn.halt.is_bound()  ) {
					sorted.insert_unique_ordered(conn, gui_halt_detail_t::compare_connection);
				}
			}
			FOR(vector_tpl<haltestelle_t::connection_t>, const& conn, sorted) {

				has_stops = true;

				button_t *pb = new_component<button_t>();
				pb->init( button_t::posbutton_automatic, NULL);
				pb->set_targetpos3d( conn.halt->get_basis_pos3d() );

				gui_label_buf_t *lb = new_component<gui_label_buf_t>();
				lb->buf().printf("%s <%u>", conn.halt->get_name(), conn.weight);
				lb->update();
			}
		}
	}

	if (!has_stops) {
		insert_show_nothing();
	}

	// ok, we have now this counter for pending updates
	destination_counter = halt->get_reconnect_counter();
	cached_line_count = halt->registered_lines.get_count();
	cached_convoy_count = halt->registered_convoys.get_count();

	set_size( get_min_size() );
}


// a sophisticated guess of a convois arrival time, taking into account the braking too and the current convoi state
uint32 gui_departure_board_t::calc_ticks_until_arrival( convoihandle_t cnv )
{
	/* calculate the time needed:
	 *   tiles << (8+12) / (kmh_to_speed(max_kmh) = ticks
	 */
	uint32 delta_t = 0;
	sint32 delta_tiles = cnv->get_route()->get_count() - cnv->front()->get_route_index();
	uint32 kmh_average = (cnv->get_average_kmh()*900 ) / 1024u;

	// last braking tile
	if(  delta_tiles > 1  &&  kmh_average > 25  ) {
		delta_tiles --;
		delta_t += 3276; // ( (1 << (8+12)) / kmh_to_speed(25) );
	}
	// second last braking tile
	if(  delta_tiles > 1  &&  kmh_average > 50  ) {
		delta_tiles --;
		delta_t += 1638; // ( (1 << (8+12)) / kmh_to_speed(50) );
	}
	// third last braking tile
	if(  delta_tiles > 1  &&  kmh_average > 100  ) {
		delta_tiles --;
		delta_t += 819; // ( (1 << (8+12)) / kmh_to_speed(100) );
	}
	// waiting at signal?
	if(  cnv->get_state() != convoi_t::DRIVING  ) {
		// extra time for acceleration
		delta_t += kmh_average * 25;
	}
	delta_t += ( ((sint64)delta_tiles << (8+12) ) / kmh_to_speed( kmh_average ) );
	return delta_t;
}


// refreshes the departure string
void gui_departure_board_t::update_departures(halthandle_t halt)
{
	if (!halt.is_bound()) {
		return;
	}

	if (--next_refresh >= 0) {
		return;
	}

	vector_tpl<dest_info_t> old_origins(origins);

	destinations.clear();
	origins.clear();

	const uint32 cur_ticks = welt->get_ticks() % welt->ticks_per_world_month;
	static uint32 last_ticks = 0;

	if(  last_ticks > cur_ticks  ) {
		// new month has started => invalidate average buffer
		old_origins.clear();
	}
	last_ticks = cur_ticks;

	// iterate over all convoys stopping here
	FOR(  vector_tpl<convoihandle_t>, cnv, halt->get_loading_convois() ) {
		if( !cnv.is_bound()  ||  cnv->get_state()!=convoi_t::LOADING  ) {
			continue;
		}
		halthandle_t next_halt = cnv->get_schedule()->get_next_halt(cnv->get_owner(),halt);

		if(  next_halt.is_bound()  ) {
			uint32 delta_ticks = 0;
			if(  cnv->get_schedule()->get_current_entry().get_absolute_departures()  ) {
				// absolute schedule
				delta_ticks = cnv->get_departure_ticks();
			}
			else if(  cnv->get_schedule()->get_current_entry().waiting_time > 0  ) {
				// waiting for load with max time
				delta_ticks = cnv->get_arrival_ticks() + cnv->get_schedule()->get_current_entry().get_waiting_ticks();
			}
			// avoid overflow when departure time has passed but convoi is still loading etc.
			uint32 ct = welt->get_ticks();
			if (ct > delta_ticks) {
				delta_ticks = 0;
			}
			else {
				delta_ticks -= ct;
			}
			dest_info_t next( next_halt, delta_ticks, cnv );
			destinations.append_unique( next );
			if(  grund_t *gr = welt->lookup( cnv->get_vehicle(0)->last_stop_pos )  ) {
				if(  gr->get_halt().is_bound()  &&  gr->get_halt() != halt  ) {
					dest_info_t prev( gr->get_halt(), 0, cnv );
					origins.append_unique( prev );
				}
			}
		}
	}

	// now exactly the same for convoys en route; the only change is that we estimate their arrival time too
	FOR(  vector_tpl<linehandle_t>, line, halt->registered_lines ) {
		for(  uint j = 0;  j < line->count_convoys();  j++  ) {
			convoihandle_t cnv = line->get_convoy(j);
			if(  cnv.is_bound()  &&  ( cnv->get_state() == convoi_t::DRIVING  ||  cnv->is_waiting() )  &&  haltestelle_t::get_halt( cnv->get_schedule()->get_current_entry().pos, cnv->get_owner() ) == halt  ) {
				halthandle_t prev_halt = haltestelle_t::get_halt( cnv->front()->last_stop_pos, cnv->get_owner() );
				sint32 delta_t = calc_ticks_until_arrival( cnv );
				if(  prev_halt.is_bound()  ) {
					dest_info_t prev( prev_halt, delta_t, cnv );
					// smooth times a little
					FOR( vector_tpl<dest_info_t>, &elem, old_origins ) {
						if(  elem.cnv == cnv ) {
							delta_t = ( delta_t + 3*elem.delta_ticks ) / 4;
							prev.delta_ticks = delta_t;
							break;
						}
					}
					origins.insert_ordered( prev, compare_hi );
				}
				halthandle_t next_halt = cnv->get_schedule()->get_next_halt(cnv->get_owner(),halt);
				if(  next_halt.is_bound()  ) {
					if( cnv->get_schedule()->get_current_entry().get_absolute_departures() ) {
						delta_t = cnv->get_departure_ticks( welt->get_ticks()+delta_t )-welt->get_ticks();
					}
					else if( cnv->get_schedule()->get_current_entry().waiting_time > 0 ) {
						// waiting for load with max time
						delta_t += cnv->get_schedule()->get_current_entry().get_waiting_ticks();
					}
					else {
						delta_t += 2000;
					}
					dest_info_t next( next_halt, delta_t+2000, cnv );
					destinations.insert_ordered( next, compare_hi );
				}
			}
		}
	}

	FOR( vector_tpl<convoihandle_t>, cnv, halt->registered_convoys ) {
		if(  cnv.is_bound()  &&  ( cnv->get_state() == convoi_t::DRIVING  ||  cnv->is_waiting() )  &&  haltestelle_t::get_halt( cnv->get_schedule()->get_current_entry().pos, cnv->get_owner() ) == halt  ) {
			halthandle_t prev_halt = haltestelle_t::get_halt( cnv->front()->last_stop_pos, cnv->get_owner() );
			sint32 delta_t = cur_ticks + calc_ticks_until_arrival( cnv );
			if(  prev_halt.is_bound()  ) {
				dest_info_t prev( prev_halt, delta_t, cnv );
				// smooth times a little
				FOR( vector_tpl<dest_info_t>, &elem, old_origins ) {
					if(  elem.cnv == cnv ) {
						delta_t = ( delta_t + 3*elem.delta_ticks ) / 4;
						prev.delta_ticks = delta_t;
						break;
					}
				}
				origins.insert_ordered( prev, compare_hi );
			}
			halthandle_t next_halt = cnv->get_schedule()->get_next_halt(cnv->get_owner(),halt);
			if(  next_halt.is_bound()  ) {
				dest_info_t next( next_halt, delta_t+2000, cnv );
				destinations.insert_ordered( next, compare_hi );
			}
		}
	}

	// fill the board
	remove_all();
	add_component( &absolute_times,3 );
	slist_tpl<halthandle_t> exclude;
	if(  destinations.get_count()>0  ) {
		new_component_span<gui_label_t>("Departures to\n", 3);

		FOR( vector_tpl<dest_info_t>, hi, destinations ) {
			if(  freight_list_sorter_t::by_via_sum != env_t::default_sortmode  ||  !exclude.is_contained( hi.halt )  ) {
				gui_label_buf_t *lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
				if( hi.delta_ticks == 0 ) {
					lb->buf().append( translator::translate( "now" ) );
				}
				else {
					if( absolute_times.pressed ) {
						lb->buf().printf( "%s", tick_to_string( welt->get_ticks()+hi.delta_ticks, true ) );
					}
					else {
						lb->buf().printf( "%s", difftick_to_string( hi.delta_ticks, true ) );
					}
				}
				lb->update();
				insert_image(hi.cnv);

				new_component<gui_label_t>(hi.halt->get_name() );
				exclude.append( hi.halt );
			}
		}
	}
	exclude.clear();
	if(  origins.get_count()>0  ) {
		new_component_span<gui_label_t>("Arrivals from\n", 3);

		FOR( vector_tpl<dest_info_t>, hi, origins ) {
			if(  freight_list_sorter_t::by_via_sum != env_t::default_sortmode  ||  !exclude.is_contained( hi.halt )  ) {
				gui_label_buf_t *lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
				if( hi.delta_ticks == 0 ) {
					lb->buf().append( translator::translate( "now" ) );
				}
				else {
					if( absolute_times.pressed ) {
						lb->buf().printf( "%s", tick_to_string( welt->get_ticks()+hi.delta_ticks, true ) );
					}
					else {
						lb->buf().printf( "%s", difftick_to_string( hi.delta_ticks, true ) );
					}
				}
				lb->update();

				insert_image(hi.cnv);

				new_component<gui_label_t>(hi.halt->get_name() );
				exclude.append( hi.halt );
			}
		}
	}

	set_size( get_min_size() );
	next_refresh = 5;
}


void gui_departure_board_t::insert_image(convoihandle_t cnv)
{
	gui_image_t *im = NULL;
	switch(cnv->front()->get_waytype()) {
		case road_wt:
		{
			if (cnv->front()->get_cargo_type() == goods_manager_t::passengers) {
				im = new_component<gui_image_t>(skinverwaltung_t::bushaltsymbol->get_image_id(0));
			}
			else {
				im = new_component<gui_image_t>(skinverwaltung_t::autohaltsymbol->get_image_id(0));
			}
			break;
		}
		case track_wt: {
			if (cnv->front()->get_desc()->get_waytype() == tram_wt) {
				im = new_component<gui_image_t>(skinverwaltung_t::tramhaltsymbol->get_image_id(0));
			}
			else {
				im = new_component<gui_image_t>(skinverwaltung_t::zughaltsymbol->get_image_id(0));
			}
			break;
		}
		case water_wt:       im = new_component<gui_image_t>(skinverwaltung_t::schiffshaltsymbol->get_image_id(0)); break;
		case air_wt:         im = new_component<gui_image_t>(skinverwaltung_t::airhaltsymbol->get_image_id(0)); break;
		case monorail_wt:    im = new_component<gui_image_t>(skinverwaltung_t::monorailhaltsymbol->get_image_id(0)); break;
		case maglev_wt:      im = new_component<gui_image_t>(skinverwaltung_t::maglevhaltsymbol->get_image_id(0)); break;
		case narrowgauge_wt: im = new_component<gui_image_t>(skinverwaltung_t::narrowgaugehaltsymbol->get_image_id(0)); break;
		default:             new_component<gui_empty_t>(); break;
	}
	if (im) {
		im->enable_offset_removal(true);
	}
}


/**
 * This method is called if an action is triggered
 */
bool halt_info_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if (comp == &sort_button) {
		env_t::default_sortmode = ((int)(halt->get_sortby())+1)%4;
		halt->set_sortby((freight_list_sorter_t::sort_mode_t) env_t::default_sortmode);
		sort_button.set_text(sort_text[env_t::default_sortmode]);
	}
	else if(  comp == &input  ) {
		if(  strcmp(halt->get_name(),edit_name)  ) {
			// text changed => call tool
			cbuffer_t buf;
			buf.printf( "h%u,%s", halt.get_id(), edit_name );
			tool_t *tool = create_tool( TOOL_RENAME | SIMPLE_TOOL );
			tool->set_default_param( buf );
			welt->set_tool( tool, halt->get_owner() );
			// since init always returns false, it is safe to delete immediately
			delete tool;
		}
	}
	else if (comp == &switch_mode) {
		departure_board->next_refresh = -1;
	}

	return true;
}


void halt_info_t::map_rotate90( sint16 new_ysize )
{
	view.map_rotate90(new_ysize);
}


void halt_info_t::rdwr(loadsave_t *file)
{
	// window size
	scr_size size = get_windowsize();
	size.rdwr( file );
	// halt
	koord3d halt_pos;
	if(  file->is_saving()  ) {
		halt_pos = halt->get_basis_pos3d();
	}
	halt_pos.rdwr( file );
	if(  file->is_loading()  ) {
		halt = welt->lookup( halt_pos )->get_halt();
		if (halt.is_bound()) {
			init(halt);
			win_set_magic(this, magic_halt_info+halt.get_id());
			reset_min_windowsize();
			set_windowsize(size);
		}
	}
	// sort
	file->rdwr_byte( env_t::default_sortmode );

	scrolly_freight.rdwr(file);
	scrolly_departure.rdwr(file);
	switch_mode.rdwr(file);

	// button-to-chart array
	button_to_chart.rdwr(file);

	if (!halt.is_bound()) {
		destroy_win( this );
	}
}
