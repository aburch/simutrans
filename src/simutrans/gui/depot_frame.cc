/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * The depot window, where to buy convois
 */

#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "../simunits.h"
#include "../world/simworld.h"
#include "../vehicle/vehicle.h"
#include "../simconvoi.h"
#include "../obj/depot.h"
#include "simwin.h"
// #include "../simcolor.h"
#include "../simdebug.h"
#include "../display/viewport.h"
#include "../simline.h"
#include "../simlinemgmt.h"
#include "../tool/simmenu.h"
#include "../simskin.h"

#include "../tpl/slist_tpl.h"

#include "line_management_gui.h"
#include "line_item.h"
#include "convoy_item.h"
#include "messagebox.h"
#include "depot_frame.h"
#include "schedule_list.h"
#include "components/gui_divider.h"

#include "../descriptor/goods_desc.h"
#include "../descriptor/intro_dates.h"
#include "../builder/vehikelbauer.h"
#include "../dataobj/schedule.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../player/simplay.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer.h"

#include "../builder/goods_manager.h"

#include "../obj/way/weg.h"

#include "../utils/unicode.h"

char depot_frame_t::name_filter_value[64] = "";

class depot_convoi_capacity_t : public gui_aligned_container_t
{
private:
	gui_label_buf_t labels[3];
	gui_image_t images[3];

	uint32 total_pax;
	uint32 total_mail;
	uint32 total_goods;

public:
	depot_convoi_capacity_t()
	{
		set_table_layout(6, 1);
		set_spacing(scr_size(D_H_SPACE, 0));
		total_pax = 0;
		total_mail = 0;
		total_goods = 0;
		images[0].set_image(skinverwaltung_t::passengers->get_image_id(0), true);
		images[1].set_image(skinverwaltung_t::mail->get_image_id(0), true);
		images[2].set_image(skinverwaltung_t::goods->get_image_id(0), true);
		for(int i=0; i<3; i++) {
			add_component(&labels[i]);
			add_component(&images[i]);
		}
	}

	void set_totals(uint32 pax, uint32 mail, uint32 goods)
	{
		total_pax = pax;
		total_mail = mail;
		total_goods = goods;
		update();
	}

	void update()
	{
		labels[0].buf().printf("%s %d", translator::translate("Capacity:"), total_pax);
		labels[1].buf().printf("%d", total_mail);
		labels[2].buf().printf("%d", total_goods);
		for(int i=0; i<3; i++) {
			labels[i].update();
		}
	}
};


class gui_aligned_container_zero_width_t : public gui_aligned_container_t
{
public:

	scr_size get_min_size() const OVERRIDE { return scr_size(0, gui_aligned_container_t::get_min_size().h); }
};


// Labels with dynamic buffers, index into labels[]
enum {
	// convoi properties
	LB_CNV_COUNT,
	LB_CNV_COST,
	LB_CNV_VALUE,
	LB_CNV_POWER,
	LB_CNV_WEIGHT,
	LB_CNV_SPEED,
	// vehicle properties
	LB_VEH_NAME,
	LB_VEH_COST,
	LB_VEH_WEIGHT,
	LB_VEH_CAPACITY,
	LB_VEH_DATE,
	LB_VEH_SPEED,
	LB_VEH_AUTHOR,
	LB_VEH_POWER,
	LB_VEH_VALUE,
	LB_MAX,
	LB_CNV_ALL = LB_VEH_NAME
};

static int sort_by_action;

bool depot_frame_t::show_retired_vehicles = false;
bool depot_frame_t::show_all = true;

depot_frame_t::depot_frame_t(depot_t* depot) :
	gui_frame_t("", NULL),
	depot(depot),
	icnv(-1),
	convoi(&convoi_pics),
	scrolly_convoi(&cont_convoi, true, false),
	pas(&pas_vec),
	electrics(&electrics_vec),
	loks(&loks_vec),
	waggons(&waggons_vec),
	scrolly_pas(&pas),
	scrolly_electrics(&electrics),
	scrolly_loks(&loks),
	scrolly_waggons(&waggons),
	line_selector(line_scrollitem_t::compare)
{
	old_vehicle_count = 0;

	if (depot) {
		old_vehicle_count = depot->get_vehicle_list().get_count() + 1;
		old_veh_type = NULL;
		init(depot);
	}
}

void depot_frame_t::init(depot_t *dep)
{
	depot = dep;
	set_name(translator::translate(depot->get_name()));
	set_owner(depot->get_owner());
	icnv = depot->convoi_count()-1;
	veh_action = va_append;

	scr_size size = scr_size(0,0);

	last_selected_line = depot->get_last_selected_line();
	no_schedule_text     = translator::translate("<no schedule set>");
	clear_schedule_text  = translator::translate("<clear schedule>");
	unique_schedule_text = translator::translate("<individual schedule>");
	new_line_text        = translator::translate("<create new line>");
	line_seperator       = translator::translate("--------------------------------");
	new_convoy_text      = translator::translate("new convoi");
	promote_to_line_text = translator::translate("<promote to line>");

	// allocate labels
	for(int i=0; i<LB_MAX; i++) {
		labels.append(new gui_label_buf_t());
	}

	set_table_layout(1,0);

	// header with convoi/line selector
	add_table(4,0);
	// first row
	/*
	* [SELECT]:
	*/
	add_component(&lb_convois, 2);

	convoy_selector.add_listener(this);
	add_component(&convoy_selector);

	// Bolt image for electrified depots:
	add_component(&img_bolt);
	img_bolt.set_image(skinverwaltung_t::electricity->get_image_id(0), true);
	img_bolt.set_rigid(true);

	// second row
	// goto line button
	line_button.set_typ(button_t::posbutton);
	line_button.set_targetpos3d(koord3d::invalid);
	line_button.add_listener(this);
	add_component(&line_button);

	new_component<gui_label_t>("Serves Line:", SYSCOL_TEXT, gui_label_t::left);
	/*
	* [SELECT ROUTE]:
	*/
	line_selector.add_listener(this);
	line_selector.set_wrapping(false);
	add_component(&line_selector);

	new_component<gui_empty_t>();

	end_table();

	/*
	* [CONVOI]
	*/
	cont_convoi.set_table_layout(2,0);
	cont_convoi.set_margin(scr_size(0,0), scr_size(0,0));
	cont_convoi.set_spacing(scr_size(0,0));
	cont_convoi.add_component(&convoi,2);
	convoi.set_max_rows(1);

	sb_convoi_length.set_base( depot->get_max_convoi_length() * CARUNITS_PER_TILE / 2 - 1);
	sb_convoi_length.set_vertical(false);
	convoi_length_ok_sb = 0;
	convoi_length_slower_sb = 0;
	convoi_length_too_slow_sb = 0;
	convoi_tile_length_sb = 0;
	new_vehicle_length_sb = 0;
	if(  depot->get_max_convoi_length() > 1  ) { // no convoy length bar for ships or aircraft
		sb_convoi_length.add_color_value(&convoi_tile_length_sb, color_idx_to_rgb(COL_BROWN));
		sb_convoi_length.add_color_value(&new_vehicle_length_sb, color_idx_to_rgb(COL_DARK_GREEN));
		sb_convoi_length.add_color_value(&convoi_length_ok_sb, color_idx_to_rgb(COL_GREEN));
		sb_convoi_length.add_color_value(&convoi_length_slower_sb, color_idx_to_rgb(COL_ORANGE));
		sb_convoi_length.add_color_value(&convoi_length_too_slow_sb, color_idx_to_rgb(COL_RED));
		cont_convoi.new_component<gui_margin_t>(gui_image_list_t::BORDER,0); // compensate for offset in gui_image_list_t
		cont_convoi.add_component(&sb_convoi_length);
	}
	add_component(&scrolly_convoi);

	{
		gui_aligned_container_t* t = add_table(2, 4);
		t->set_force_equal_columns(true);
		t->set_spacing(scr_size(D_H_SPACE, 0));
		add_component(labels[LB_CNV_COUNT]);
		cont_convoi_capacity = new_component<depot_convoi_capacity_t>();
		add_component(labels[LB_CNV_COST]);
		add_component(labels[LB_CNV_VALUE]);
		add_component(labels[LB_CNV_POWER]);
		add_component(labels[LB_CNV_WEIGHT]);
		add_component(labels[LB_CNV_SPEED], 2);
		end_table();
	}

	/*
	* [ACTIONS]
	*/
	add_table(4,0)->set_force_equal_columns(true);
	bt_start.init(button_t::roundbox | button_t::flexible, "Start");
	bt_start.add_listener(this);
	bt_start.set_tooltip("Start the selected vehicle(s)");
	add_component(&bt_start);

	bt_schedule.init(button_t::roundbox | button_t::flexible, "Fahrplan");
	bt_schedule.add_listener(this);
	bt_schedule.set_tooltip("Give the selected vehicle(s) an individual schedule"); // translated to "Edit the selected vehicle(s) individual schedule or assigned line"
	add_component(&bt_schedule);

	bt_copy_convoi.init(button_t::roundbox | button_t::flexible, "Copy Convoi");
	bt_copy_convoi.add_listener(this);
	bt_copy_convoi.set_tooltip("Copy the selected convoi and its schedule or line");
	add_component(&bt_copy_convoi);

	bt_sell.init(button_t::roundbox | button_t::flexible, "verkaufen");
	bt_sell.add_listener(this);
	bt_sell.set_tooltip("Sell the selected vehicle(s)");
	add_component(&bt_sell);
	end_table();

	/*
	* [PANEL]
	*/

	add_component(&tabs);

	/*
	* [BOTTOM]
	*/
	new_component<gui_divider_t>();

	cont_veh_action = add_table(4,0);
	{
		// put columns 1-3 in extra container to force correct button width in 4th column
		gui_aligned_container_t *cont_3cols = new_component_span<gui_aligned_container_t>(3,0,3);
		cont_3cols->add_component(&lb_convoi_count);
		cont_3cols->new_component<gui_fill_t>();
		cont_3cols->new_component<gui_label_t>("Fahrzeuge:", SYSCOL_TEXT, gui_label_t::right);
	}
	bt_veh_action.set_typ(button_t::roundbox | button_t::flexible);
	bt_veh_action.add_listener(this);
	bt_veh_action.set_tooltip("Choose operation executed on clicking stored/new vehicles");
	add_component(&bt_veh_action);

	{
		// put columns 1-4 in extra container to force correct button width in last column
		gui_aligned_container_t *cont_4cols = new_component_span<gui_aligned_container_t>(4,0,3);

		bt_obsolete.init(button_t::square_state, "Show obsolete");
		bt_obsolete.pressed = show_retired_vehicles;
		if(  welt->get_settings().get_allow_buying_obsolete_vehicles()  ) {
			bt_obsolete.add_listener(this);
			bt_obsolete.set_tooltip("Show also vehicles no longer in production.");
			cont_4cols->add_component(&bt_obsolete);
		}
		else {
			cont_4cols->new_component<gui_fill_t>();
		}

		cont_4cols->new_component<gui_label_t>("Filter:", SYSCOL_TEXT, gui_label_t::right);

		vehicle_filter.add_listener(this);
		cont_4cols->add_component(&vehicle_filter);

		cont_4cols->new_component<gui_label_t>("Search:", SYSCOL_TEXT, gui_label_t::right);
	}
	name_filter_input.set_text(name_filter_value, 60);
	add_component(&name_filter_input);
	name_filter_input.add_listener(this);

	{
		// put columns 1-3 in extra container to force correct button width in 4th column
		gui_aligned_container_t *cont_3cols = new_component_span<gui_aligned_container_t>(3,0,3);

		bt_show_all.init(button_t::square_state, "Show all");
		bt_show_all.add_listener(this);
		bt_show_all.set_tooltip("Show also vehicles that do not match for current action.");
		bt_show_all.pressed = show_all;

		cont_3cols->add_component(&bt_show_all);
		cont_3cols->new_component<gui_fill_t>();
		cont_3cols->new_component<gui_label_t>("Sort by:", SYSCOL_TEXT, gui_label_t::right);
	}
	sort_by.add_listener(this);
	add_component(&sort_by);

	end_table();

	new_component<gui_divider_t>();

	/*
	 * [VEHICLE]
	 */
	// special container with min-width == zero
	// (prevents mouse-over-events to spoil the layout and force a large min-width)
	cont_vehicle_labels = new gui_aligned_container_zero_width_t();
	cont_vehicle_labels->set_table_layout(2,0);
	cont_vehicle_labels->set_force_equal_columns(true);
	cont_vehicle_labels->set_spacing(scr_size(D_H_SPACE, 0));

	cont_vehicle_labels->add_component(labels[LB_VEH_NAME],2);
	cont_vehicle_labels->add_component(labels[LB_VEH_COST]);
	cont_vehicle_labels->add_component(labels[LB_VEH_WEIGHT]);
	cont_vehicle_labels->add_component(labels[LB_VEH_CAPACITY]);
	cont_vehicle_labels->add_component(labels[LB_VEH_DATE]);
	cont_vehicle_labels->add_component(labels[LB_VEH_SPEED]);
	cont_vehicle_labels->add_component(labels[LB_VEH_AUTHOR]);
	cont_vehicle_labels->add_component(labels[LB_VEH_POWER]);
	cont_vehicle_labels->add_component(labels[LB_VEH_VALUE]);
	add_component(cont_vehicle_labels);
	/*
	 * [END OF WINDOW]
	 */

	// initialize image lists, scrollies, speedbar
	{
		/*
		 * These parameter are adjusted to resolution.
		 * - Some extra space looks nicer.
		 */
		scr_coord placement;
		scr_coord grid;
		grid.x = depot->get_x_grid() * get_base_tile_raster_width() / 64 + 4;
		grid.y = depot->get_y_grid() * get_base_tile_raster_width() / 64 + 6;
		placement.x = depot->get_x_placement() * get_base_tile_raster_width() / 64 + 2;
		placement.y = depot->get_y_placement() * get_base_tile_raster_width() / 64 + 2;
		scr_coord_val grid_dx = depot->get_x_grid() * get_base_tile_raster_width() / 64 / 2;
		scr_coord_val placement_dx = depot->get_x_grid() * get_base_tile_raster_width() / 64 / 4;

		sb_convoi_length.set_width(depot->get_max_convoi_length() * (grid.x - grid_dx));

		gui_image_list_t* ilists[] = {&convoi, &pas, &electrics, &loks, &waggons};
		for(uint32 i = 0; i<lengthof(ilists); i++) {
			gui_image_list_t* il = ilists[i];
			il->set_grid(scr_coord(grid.x - grid_dx, grid.y));
			il->set_placement(scr_coord(placement.x - placement_dx, placement.y));
			il->set_player_nr(depot->get_owner_nr());
			il->add_listener(this);
			// only convoi list gets overlapping images
			grid_dx = 0;
			placement_dx = 0;
		}

		gui_scrollpane_t* scrollies[] = {&scrolly_convoi, &scrolly_pas, &scrolly_electrics, &scrolly_loks, &scrolly_waggons};
		for(uint32 i = 0; i<lengthof(scrollies); i++) {
			gui_scrollpane_t* scrolly = scrollies[i];
			scrolly->set_scrollbar_mode( scrollbar_t::show_disabled );
			scrolly->set_size_corner(false);
		}
	}

	// update all the elements
	build_vehicle_lists();
	update_data();

	reset_min_windowsize();
	set_windowsize(size);
	set_resizemode( diagonal_resize );

	depot->clear_command_pending();
}


// free memory: all the image_data_t
depot_frame_t::~depot_frame_t()
{
	clear_ptr_vector(pas_vec);
	clear_ptr_vector(electrics_vec);
	clear_ptr_vector(loks_vec);
	clear_ptr_vector(waggons_vec);
}


// returns position of depot on the map
koord3d depot_frame_t::get_weltpos(bool)
{
	return depot->get_pos();
}


bool depot_frame_t::is_weltpos()
{
	return ( welt->get_viewport()->is_on_center( get_weltpos(false) ) );
}


void depot_frame_t::set_windowsize( scr_size size )
{
	// set width of image-lists
	gui_image_list_t* ilists[] = {&pas, &electrics, &loks, &waggons};
	for(uint32 i = 0; i<lengthof(ilists); i++) {
		gui_image_list_t* il = ilists[i];
		il->set_max_width(size.w);
	}
	gui_frame_t::set_windowsize(size);
}


void depot_frame_t::activate_convoi( convoihandle_t c )
{
	icnv = -1; // deselect
	for(  uint i = 0;  i < depot->convoi_count();  i++  ) {
		if(  c == depot->get_convoi(i)  ) {
			icnv = i;
			break;
		}
	}
	build_vehicle_lists();
}


// true if already stored here
bool depot_frame_t::is_in_vehicle_list(const vehicle_desc_t *info)
{
	for(vehicle_t* const v : depot->get_vehicle_list()) {
		if(  v->get_desc() == info  ) {
			return true;
		}
	}
	return false;
}


// add a single vehicle (helper function)
void depot_frame_t::add_to_vehicle_list(const vehicle_desc_t *info)
{
	// Check if vehicle should be filtered
	const goods_desc_t *freight = info->get_freight_type();
	// Only filter when required and never filter engines
	if (depot->selected_filter > 0 && info->get_capacity() > 0) {
		if (depot->selected_filter == VEHICLE_FILTER_RELEVANT) {
			if(freight->get_catg_index() >= 3) {
				bool found = false;
				for(goods_desc_t const* const i : welt->get_goods_list()) {
					if (freight->get_catg_index() == i->get_catg_index()) {
						found = true;
						break;
					}
				}
				// If no current goods can be transported by this vehicle, don't display it
				if (!found) return;
			}
		}
		else if (depot->selected_filter > VEHICLE_FILTER_RELEVANT) {
			// Filter on specific selected good
			uint32 goods_index = depot->selected_filter - VEHICLE_FILTER_GOODS_OFFSET;
			if (goods_index < welt->get_goods_list().get_count()) {
				const goods_desc_t *selected_good = welt->get_goods_list()[goods_index];
				if (freight->get_catg_index() != selected_good->get_catg_index()) {
					return; // This vehicle can't transport the selected good
				}
			}
		}
	}

	gui_image_list_t::image_data_t* img_data = new gui_image_list_t::image_data_t(info->get_name(), info->get_base_image());

	if(  info->get_engine_type() == vehicle_desc_t::electric  &&  (info->get_freight_type()==goods_manager_t::passengers  ||  info->get_freight_type()==goods_manager_t::mail)  ) {
		electrics_vec.append(img_data);
	}
	// since they come "pre-sorted" from the vehikelbauer, we have to do nothing to keep them sorted
	else if(info->get_freight_type() == goods_manager_t::passengers  ||  info->get_freight_type() == goods_manager_t::mail) {
		pas_vec.append(img_data);
	}
	else if(info->get_power() > 0  ||  info->get_capacity()==0) {
		loks_vec.append(img_data);
	}
	else {
		waggons_vec.append(img_data);
	}
	// add reference to map
	vehicle_map.set(info, img_data);
}

// add all current vehicles
void depot_frame_t::build_vehicle_lists()
{
	if (depot->get_vehicle_type().empty()) {
		// there are tracks etc. but no vehicles => do nothing
		// at least initialize some data
		update_data();
		update_tabs();
		return;
	}

	const int month_now = welt->get_timeline_year_month();

	// free vectors
	clear_ptr_vector(pas_vec);
	clear_ptr_vector(electrics_vec);
	clear_ptr_vector(loks_vec);
	clear_ptr_vector(waggons_vec);
	// clear map
	vehicle_map.clear();

	// we do not allow to built electric vehicle in a depot without electrification
	const waytype_t wt = depot->get_waytype();
	const weg_t *w = welt->lookup(depot->get_pos())->get_weg(wt!=tram_wt ? wt : track_wt);
	const bool weg_electrified = w ? w->is_electrified() : false;

	img_bolt.set_visible(weg_electrified);

	sort_by_action = depot->selected_sort_by;

	vector_tpl<const vehicle_desc_t*> typ_list;

	if(!show_all  &&  veh_action==va_sell) {
		// show only sellable vehicles
		for(vehicle_t* const v : depot->get_vehicle_list()) {
			vehicle_desc_t const* const d = v->get_desc();
			typ_list.append(d);
		}
	}
	else {
		slist_tpl<const vehicle_desc_t*> const& tmp_list = depot->get_vehicle_type(sort_by_action);
		for(slist_tpl<const vehicle_desc_t*>::const_iterator itr = tmp_list.begin(); itr != tmp_list.end(); ++itr) {
			typ_list.append(*itr);
		}
	}

	// use this to show only sellable vehicles
	if(!show_all  &&  veh_action==va_sell) {
		// just list the one to sell
		for(vehicle_desc_t const* const info : typ_list) {
			if (vehicle_map.get(info)) continue;
			add_to_vehicle_list(info);
		}
	}
	else {
		// list only matching ones
		for(vehicle_desc_t const* const info : typ_list) {
			const vehicle_desc_t *veh = NULL;
			convoihandle_t cnv = depot->get_convoi(icnv);
			if(cnv.is_bound() && cnv->get_vehicle_count()>0) {
				veh = (veh_action == va_insert ? cnv->front() : cnv->back())->get_desc();
			}

			// current vehicle
			if( is_in_vehicle_list(info)  ||
				((weg_electrified  ||  info->get_engine_type()!=vehicle_desc_t::electric)  &&
					 ((!info->is_future(month_now))  &&  (show_retired_vehicles  ||  (!info->is_retired(month_now)) )  ) )) {
				// check, if allowed
				bool append = true;
				if(!show_all) {
					if(veh_action == va_insert) {
						append = info->can_lead(veh)  &&  (veh==NULL  ||  veh->can_follow(info));
					} else if(veh_action == va_append) {
						append = info->can_follow(veh)  &&  (veh==NULL  ||  veh->can_lead(info));
					}
				}
				if(append) {
					// name filter. Try to check both object name and translation name (case sensitive though!)
					if(  name_filter_value[0]==0  ||  (utf8caseutf8(info->get_name(), name_filter_value)  ||  utf8caseutf8(translator::translate(info->get_name()), name_filter_value))  ) {
						add_to_vehicle_list( info );
					}
				}
			}
		}
	}
DBG_DEBUG("depot_frame_t::build_vehicle_lists()","finally %i passenger vehicle, %i  engines, %i good wagons",pas_vec.get_count(),loks_vec.get_count(),waggons_vec.get_count());
	update_data();
	update_tabs();
}


static void get_line_list(const depot_t* depot, vector_tpl<linehandle_t>* lines)
{
	depot->get_owner()->simlinemgmt.get_lines(depot->get_line_type(), lines);
}


void depot_frame_t::update_data()
{
	static const char *txt_veh_action[3] = { "anhaengen", "voranstellen", "verkaufen" };

	// change green into blue for retired vehicles
	const int month_now = welt->get_timeline_year_month();

	bt_veh_action.set_text(txt_veh_action[veh_action]);

	cbuffer_t &txt_convois = lb_convois.buf();
	switch(  depot->convoi_count()  ) {
		case 0: {
			txt_convois.append( translator::translate("no convois") );
			break;
		}
		case 1: {
			if(  icnv == -1  ) {
				txt_convois.append( translator::translate("1 convoi") );
			}
			else {
				txt_convois.printf( translator::translate("convoi %d of %d"), icnv + 1, depot->convoi_count() );
			}
			break;
		}
		default: {
			if(  icnv == -1  ) {
				txt_convois.printf( translator::translate("%d convois"), depot->convoi_count() );
			}
			else {
				txt_convois.printf( translator::translate("convoi %d of %d"), icnv + 1, depot->convoi_count() );
			}
			break;
		}
	}
	lb_convois.update();

	/*
	* Reset counts and check for valid vehicles
	*/
	convoihandle_t cnv = depot->get_convoi( icnv );

	// update convoy selector
	convoy_selector.clear_elements();
	convoy_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( new_convoy_text, SYSCOL_TEXT ) ;
	convoy_selector.set_selection(0);

	// check all matching convoys
	for(convoihandle_t const c : depot->get_convoy_list()) {
		convoy_selector.new_component<convoy_scrollitem_t>(c) ;
		if(  cnv.is_bound()  &&  c == cnv  ) {
			convoy_selector.set_selection( convoy_selector.count_elements() - 1 );
		}
	}

	const vehicle_desc_t *veh = NULL;

	scr_size old_convoi_size = convoi.get_min_size();

	clear_ptr_vector( convoi_pics );
	if(  cnv.is_bound()  &&  cnv->get_vehicle_count() > 0  ) {
		for(  unsigned i=0;  i < cnv->get_vehicle_count();  i++  ) {
			// just make sure, there is this vehicle also here!
			const vehicle_desc_t *info=cnv->get_vehicle(i)->get_desc();
			if(  vehicle_map.get( info ) == NULL  ) {
				add_to_vehicle_list( info );
			}

			gui_image_list_t::image_data_t* img_data = new gui_image_list_t::image_data_t(info->get_name(), info->get_base_image());
			convoi_pics.append(img_data);
		}

		/* color bars for current convoi: */
		convoi_pics[0]->lcolor = color_idx_to_rgb(cnv->front()->get_desc()->can_follow(NULL) ? COL_GREEN : COL_YELLOW);
		{
			unsigned i;
			for(  i = 1;  i < cnv->get_vehicle_count();  i++  ) {
				convoi_pics[i - 1]->rcolor = color_idx_to_rgb(cnv->get_vehicle(i-1)->get_desc()->can_lead(cnv->get_vehicle(i)->get_desc()) ? COL_GREEN : COL_RED);
				convoi_pics[i]->lcolor     = color_idx_to_rgb(cnv->get_vehicle(i)->get_desc()->can_follow(cnv->get_vehicle(i-1)->get_desc()) ? COL_GREEN : COL_RED);
			}
			convoi_pics[i - 1]->rcolor = color_idx_to_rgb(cnv->get_vehicle(i-1)->get_desc()->can_lead(NULL) ? COL_GREEN : COL_YELLOW);
		}

		// change green into blue for vehicles that are not available
		for(  unsigned i = 0;  i < cnv->get_vehicle_count();  i++  ) {
			if(  !cnv->get_vehicle(i)->get_desc()->is_available(month_now)  ) {
				if(  convoi_pics[i]->lcolor == color_idx_to_rgb(COL_GREEN)  ) {
					convoi_pics[i]->lcolor = gui_theme_t::gui_color_obsolete;
				}
				if(  convoi_pics[i]->rcolor == color_idx_to_rgb(COL_GREEN)  ) {
					convoi_pics[i]->rcolor = gui_theme_t::gui_color_obsolete;
				}
			}
		}

		veh = (veh_action == va_insert ? cnv->front() : cnv->back())->get_desc();
	}

	for(auto const& i : vehicle_map) {
		vehicle_desc_t const* const    info = i.key;
		gui_image_list_t::image_data_t& img  = *i.value;
		const PIXVAL ok_color = info->is_available(month_now) ? color_idx_to_rgb(COL_GREEN) : gui_theme_t::gui_color_obsolete;

		img.count = 0;
		img.lcolor = ok_color;
		img.rcolor = ok_color;

		/*
		* color bars for current convoi:
		*  green/green  okay to append/insert
		*  red/red      cannot be appended/inserted
		*  green/yellow append okay, cannot be end of train
		*  yellow/green insert okay, cannot be start of train
		*/

		if(veh_action == va_insert) {
			if(!info->can_lead(veh)  ||  (veh  &&  !veh->can_follow(info))) {
				img.lcolor = color_idx_to_rgb(COL_RED);
				img.rcolor = color_idx_to_rgb(COL_RED);
			}
			else if(!info->can_follow(NULL)) {
				img.lcolor = color_idx_to_rgb(COL_YELLOW);
			}
		}
		else if(veh_action == va_append) {
			if(!info->can_follow(veh)  ||  (veh  &&  !veh->can_lead(info))) {
				img.lcolor = color_idx_to_rgb(COL_RED);
				img.rcolor = color_idx_to_rgb(COL_RED);
			}
			else if(!info->can_lead(NULL)) {
				img.rcolor = color_idx_to_rgb(COL_YELLOW);
			}
		}
		else if( veh_action == va_sell ) {
			img.lcolor = color_idx_to_rgb(COL_RED);
			img.rcolor = color_idx_to_rgb(COL_RED);
		}
	}

	for(vehicle_t* const v : depot->get_vehicle_list()) {
		// can fail, if currently not visible
		if (gui_image_list_t::image_data_t* const imgdat = vehicle_map.get(v->get_desc())) {
			imgdat->count++;
			if(veh_action == va_sell) {
				imgdat->lcolor = color_idx_to_rgb(COL_GREEN);
				imgdat->rcolor = color_idx_to_rgb(COL_GREEN);
			}
		}
	}

	// update the line selector
	line_selector.clear_elements();

	if(  !last_selected_line.is_bound()  ) {
		// new line may have a valid line now
		last_selected_line = selected_line;
		// if still nothing, resort to line management dialoge
		if(  !last_selected_line.is_bound()  ) {
			// try last specific line
			last_selected_line = schedule_list_gui_t::selected_line[ depot->get_owner()->get_player_nr() ][ depot->get_line_type() ];
		}
		if(  !last_selected_line.is_bound()  ) {
			// try last general line
			last_selected_line = schedule_list_gui_t::selected_line[ depot->get_owner()->get_player_nr() ][ 0 ];
			if(  last_selected_line.is_bound()  &&  last_selected_line->get_linetype() != depot->get_line_type()  ) {
				last_selected_line = linehandle_t();
			}
		}
	}
	if(  cnv.is_bound()  &&  cnv->get_schedule()  &&  !cnv->get_schedule()->empty()  ) {
		if(  cnv->get_line().is_bound()  ) {
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( clear_schedule_text, SYSCOL_TEXT ) ;
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( new_line_text, SYSCOL_TEXT ) ;
		}
		else {
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( unique_schedule_text, SYSCOL_TEXT ) ;
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( promote_to_line_text, SYSCOL_TEXT ) ;
		}
	}
	else {
		line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( no_schedule_text, SYSCOL_TEXT ) ;
		line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( new_line_text, SYSCOL_TEXT ) ;
	}
	if(  last_selected_line.is_bound()  ) {
		line_selector.new_component<line_scrollitem_t>( last_selected_line ) ;
	}
	line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( line_seperator, SYSCOL_TEXT ) ;

	// check all matching lines
	if(  cnv.is_bound()  ) {
		selected_line = cnv->get_line();
	}
	vector_tpl<linehandle_t> lines;
	get_line_list(depot, &lines);
	// select "create new schedule"
	line_selector.set_selection( 0 );
	for(linehandle_t const line :  lines  ) {
		line_selector.new_component<line_scrollitem_t>(line) ;
		if(  selected_line.is_bound()  &&  selected_line == line  ) {
			line_selector.set_selection( line_selector.count_elements() - 1 );
		}
	}
	if(  line_selector.get_selection() == 0  ) {
		// no line selected
		selected_line = linehandle_t();
	}
	line_selector.sort( last_selected_line.is_bound()+3 );

	// Update vehicle filter
	vehicle_filter.clear_elements();
	vehicle_filter.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("All"), SYSCOL_TEXT);
	vehicle_filter.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Relevant"), SYSCOL_TEXT);

	for(goods_desc_t const* const i : welt->get_goods_list()) {
		vehicle_filter.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(i->get_name()), SYSCOL_TEXT);
	}

	if(  depot->selected_filter > vehicle_filter.count_elements()  ) {
		depot->selected_filter = VEHICLE_FILTER_RELEVANT;
	}
	vehicle_filter.set_selection(depot->selected_filter);
	vehicle_filter.set_size(vehicle_filter.get_size());

	sort_by.clear_elements();
	for(int i = 0; i < vehicle_builder_t::sb_length; i++) {
		sort_by.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(vehicle_builder_t::vehicle_sort_by[i]), SYSCOL_TEXT);
	}
	if(  depot->selected_sort_by > sort_by.count_elements()  ) {
		depot->selected_sort_by = vehicle_builder_t::sb_name;
	}
	sort_by.set_selection(depot->selected_sort_by);

	// finally: update text
	uint32 total_pax = 0;
	uint32 total_mail = 0;
	uint32 total_goods = 0;

	uint64 total_power = 0;
	uint32 total_empty_weight = 0;
	uint32 total_selected_weight = 0;
	uint32 total_max_weight = 0;
	uint32 total_min_weight = 0;
	bool use_sel_weight = true;

	if(  cnv.is_bound()  &&   cnv->get_vehicle_count() > 0  ) {
		{
			uint8 selected_good_index = 0;
			if(  depot->selected_filter > VEHICLE_FILTER_RELEVANT  ) {
				// Filter is set to specific good
				const uint32 goods_index = depot->selected_filter - VEHICLE_FILTER_GOODS_OFFSET;
				if(  goods_index < welt->get_goods_list().get_count()  ) {
					selected_good_index = welt->get_goods_list()[goods_index]->get_index();
				}
			}

			for(  unsigned i = 0;  i < cnv->get_vehicle_count();  i++  ) {
				const vehicle_desc_t *desc = cnv->get_vehicle(i)->get_desc();

				total_power += desc->get_power()*desc->get_gear();

				uint32 sel_weight = 0; // actual weight using vehicle filter selected good to fill
				uint32 max_weight = 0;
				uint32 min_weight = 100000;
				bool sel_found = false;
				for(  uint32 j=0;  j<goods_manager_t::get_count();  j++  ) {
					const goods_desc_t *ware = goods_manager_t::get_info(j);

					if(  desc->get_freight_type()->get_catg_index() == ware->get_catg_index()  ) {
						max_weight = max(max_weight, (uint32)ware->get_weight_per_unit());
						min_weight = min(min_weight, (uint32)ware->get_weight_per_unit());

						// find number of goods in in this category. TODO: gotta be a better way...
						uint8 catg_count = 0;
						for(goods_desc_t const* const i : welt->get_goods_list()) {
							if(  ware->get_catg_index() == i->get_catg_index()  ) {
								catg_count++;
							}
						}

						if(  ware->get_index() == selected_good_index  ||  catg_count < 2  ) {
							sel_found = true;
							sel_weight = ware->get_weight_per_unit();
						}
					}
				}
				if(  !sel_found  ) {
					// vehicle carries more than one good, but not the selected one
					use_sel_weight = false;
				}

				total_empty_weight += desc->get_weight();
				total_selected_weight += desc->get_weight() + sel_weight * desc->get_capacity();
				total_max_weight += desc->get_weight() + max_weight * desc->get_capacity();
				total_min_weight += desc->get_weight() + min_weight * desc->get_capacity();

				const goods_desc_t* const ware = desc->get_freight_type();
				switch(  ware->get_catg_index()  ) {
					case goods_manager_t::INDEX_PAS: {
						total_pax += desc->get_capacity();
						break;
					}
					case goods_manager_t::INDEX_MAIL: {
						total_mail += desc->get_capacity();
						break;
					}
					default: {
						total_goods += desc->get_capacity();
						break;
					}
				}
			}

			sint32 empty_kmh, sel_kmh, max_kmh, min_kmh;
			if(  cnv->front()->get_waytype() == air_wt  ) {
				// flying aircraft have 0 friction --> speed not limited by power, so just use top_speed
				empty_kmh = sel_kmh = max_kmh = min_kmh = speed_to_kmh( cnv->get_min_top_speed() );
			}
			else {
				empty_kmh = speed_to_kmh(convoi_t::calc_max_speed(total_power, total_empty_weight, cnv->get_min_top_speed()));
				sel_kmh =   speed_to_kmh(convoi_t::calc_max_speed(total_power, total_selected_weight, cnv->get_min_top_speed()));
				max_kmh =   speed_to_kmh(cnv->get_min_top_speed());
				min_kmh =   speed_to_kmh(convoi_t::calc_max_speed(total_power, total_max_weight,   cnv->get_min_top_speed()));
			}

			const sint32 convoi_length = (cnv->get_vehicle_count()) * CARUNITS_PER_TILE / 2 - 1;
			convoi_tile_length_sb = convoi_length + (cnv->get_tile_length() * CARUNITS_PER_TILE - cnv->get_length());

			cbuffer_t& txt_convoi_count = labels[LB_CNV_COUNT]->buf();
			if(  cnv->get_vehicle_count()>1  ) {
				txt_convoi_count.printf( translator::translate("%i car(s),"), cnv->get_vehicle_count() );
			}
			txt_convoi_count.append( translator::translate("Station tiles:") );
			txt_convoi_count.append( (double)cnv->get_tile_length(), 0 );

			cbuffer_t& txt_convoi_speed = labels[LB_CNV_SPEED]->buf();
			if(  empty_kmh < 4  ||  empty_kmh != (use_sel_weight ? sel_kmh : min_kmh)  ) {
				// convoi way too slow
				convoi_length_ok_sb = 0;
				if(  max_kmh != min_kmh  &&  !use_sel_weight  ) {
					txt_convoi_speed.printf("%s %d km/h, %d-%d km/h %s", translator::translate("Max. speed:"), empty_kmh, min_kmh, max_kmh, translator::translate("loaded") );
					if(  max_kmh != empty_kmh  || empty_kmh < 4  ) {
						convoi_length_slower_sb = 0;
						convoi_length_too_slow_sb = convoi_length;
					}
					else {
						convoi_length_slower_sb = convoi_length;
						convoi_length_too_slow_sb = 0;
					}
				}
				else {
					txt_convoi_speed.printf("%s %d km/h, %d km/h %s", translator::translate("Max. speed:"), empty_kmh, use_sel_weight ? sel_kmh : min_kmh, translator::translate("loaded") );
					convoi_length_slower_sb = 0;
					convoi_length_too_slow_sb = convoi_length;
				}
			}
			else {
					txt_convoi_speed.printf("%s %d km/h", translator::translate("Max. speed:"), empty_kmh );
					convoi_length_ok_sb = convoi_length;
					convoi_length_slower_sb = 0;
					convoi_length_too_slow_sb = 0;
			}

			{
				char buf[128];
				cbuffer_t& txt_convoi_value = labels[LB_CNV_VALUE]->buf();
				money_to_string(  buf, cnv->calc_restwert() / 100.0, false );
				txt_convoi_value.printf("%s %8s", translator::translate("Restwert:"), buf );

				cbuffer_t& txt_convoi_cost = labels[LB_CNV_COST]->buf();
				if(  sint64 fix_cost = cnv->get_fixed_cost()  ) {
					money_to_string(  buf, (double)cnv->get_purchase_cost() / 100.0, false );
					txt_convoi_cost.printf( translator::translate("Cost: %8s (%.2f$/km %.2f$/m)\n"), buf, (double)cnv->get_running_cost()/100.0, (double)fix_cost/100.0 );
				}
				else {
					money_to_string(  buf, cnv->get_purchase_cost() / 100.0, false );
					txt_convoi_cost.printf( translator::translate("Cost: %8s (%.2f$/km)\n"), buf, (double)cnv->get_running_cost() / 100.0 );
				}
			}

			cbuffer_t& txt_convoi_power = labels[LB_CNV_POWER]->buf();
			txt_convoi_power.printf( translator::translate("Power: %4d kW\n"), cnv->get_sum_power() );

			cbuffer_t& txt_convoi_weight = labels[LB_CNV_WEIGHT]->buf();
			if(  total_empty_weight != (use_sel_weight ? total_selected_weight : total_max_weight)  ) {
				if(  total_min_weight != total_max_weight  &&  !use_sel_weight  ) {
					txt_convoi_weight.printf("%s %.1ft, %.1f-%.1ft", translator::translate("Weight:"), total_empty_weight / 1000.0, total_min_weight / 1000.0, total_max_weight / 1000.0 );
				}
				else {
					txt_convoi_weight.printf("%s %.1ft, %.1ft", translator::translate("Weight:"), total_empty_weight / 1000.0, (use_sel_weight ? total_selected_weight : total_max_weight) / 1000.0 );
				}
			}
			else {
					txt_convoi_weight.printf("%s %.1ft", translator::translate("Weight:"), total_empty_weight / 1000.0 );
			}
			sb_convoi_length.set_visible(true);
			cont_convoi_capacity->set_totals( total_pax, total_mail, total_goods );
			cont_convoi_capacity->set_visible(true);
		}
	}
	else {
		sb_convoi_length.set_visible(false);
		cont_convoi_capacity->set_visible(false);

		for(uint32 i=0; i<LB_CNV_ALL; i++) {
			labels[i]->buf();
		}
		labels[LB_CNV_COUNT]->buf().append(translator::translate("Keine Einzelfahrzeuge im Depot"));
	}

	for(uint32 i=0; i<LB_CNV_ALL; i++) {
		labels[i]->update();
	}
	// update window if convoi container size changed
	if (old_convoi_size.w != convoi.get_min_size().w) {
		resize(scr_size(0,0));
	}
}


sint64 depot_frame_t::calc_restwert(const vehicle_desc_t *veh_type)
{
	sint64 wert = 0;
	for(vehicle_t* const v : depot->get_vehicle_list()) {
		if(  v->get_desc() == veh_type  ) {
			wert += v->calc_sale_value();
		}
	}
	return wert;
}


void depot_frame_t::image_from_storage_list(gui_image_list_t::image_data_t *image_data)
{
	if(  image_data->lcolor != color_idx_to_rgb(COL_RED)  &&  image_data->rcolor != color_idx_to_rgb(COL_RED)  ) {
		if(  veh_action == va_sell  ) {
			depot->call_depot_tool('s', convoihandle_t(), image_data->text );
		}
		else {
			convoihandle_t cnv = depot->get_convoi( icnv );
			if(  !cnv.is_bound()  &&   !depot->get_owner()->is_locked()  ) {
				// adding new convoi, block depot actions until command executed
				// otherwise in multiplayer it's likely multiple convois get created
				// rather than one new convoi with multiple vehicles
				depot->set_command_pending();
			}
			depot->call_depot_tool( veh_action == va_insert ? 'i' : 'a', cnv, image_data->text );
		}
	}
}


void depot_frame_t::image_from_convoi_list(uint nr, bool to_end)
{
	const convoihandle_t cnv = depot->get_convoi( icnv );
	if(  cnv.is_bound()  &&  nr < cnv->get_vehicle_count()  ) {
		// we remove all connected vehicles together!
		// find start
		unsigned start_nr = nr;
		while(  start_nr > 0  ) {
			start_nr--;
			const vehicle_desc_t *info = cnv->get_vehicle(start_nr)->get_desc();
			if(  info->get_trailer_count() != 1  ) {
				start_nr++;
				break;
			}
		}

		cbuffer_t start;
		start.printf("%u", start_nr);

		const char tool = to_end ? 'R' : 'r';
		depot->call_depot_tool( tool, cnv, start );
	}
}


bool depot_frame_t::action_triggered( gui_action_creator_t *comp, value_t p)
{
	convoihandle_t cnv = depot->get_convoi( icnv );

	if(  depot->is_command_pending()  ) {
		// block new commands until last command is executed
		return true;
	}

	if(  comp != NULL  ) { // message from outside!
		if(  comp == &bt_start  ) {
			if(  cnv.is_bound()  ) {
				//first: close schedule (will update schedule on clients)
				destroy_win( (ptrdiff_t)cnv->get_schedule() );
				// only then call the tool to start
				char tool = (event_get_last_control_shift() ^ tool_t::control_invert)==2 ? 'B' : 'b'; // start all with CTRL-click
				depot->call_depot_tool( tool, cnv, NULL);
			}
		}
		else if(  comp == &bt_schedule  ) {
			if(  line_selector.get_selection() == 1  &&  !line_selector.is_dropped()  ) { // create new line
				// promote existing individual schedule to line
				cbuffer_t buf;
				if(  cnv.is_bound()  &&  !selected_line.is_bound()  ) {
					schedule_t* schedule = cnv->get_schedule();
					if(  schedule  ) {
						schedule->sprintf_schedule( buf );
					}
				}
				depot->call_depot_tool('l', convoihandle_t(), buf);
				return true;
			}
			else {
				open_schedule_editor();
				return true;
			}
		}
		else if(  comp == &line_button  ) {
			if(  cnv.is_bound()  ) {
				cnv->get_owner()->simlinemgmt.show_lineinfo( cnv->get_owner(), cnv->get_line(), 0 );
				welt->set_dirty();
			}
		}
		else if(  comp == &bt_sell  ) {
			depot->call_depot_tool('v', cnv, NULL);
		}
		// image list selection here ...
		else if(  comp == &convoi  ) {
			image_from_convoi_list( p.i, last_meta_event_get_class() == EVENT_DOUBLE_CLICK);
		}
		else if(  comp == &pas  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(pas_vec[p.i]);
		}
		else if(  comp == &electrics  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(electrics_vec[p.i]);
		}
		else if(  comp == &loks  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(loks_vec[p.i]);
		}
		else if(  comp == &waggons  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(waggons_vec[p.i]);
		}
		// convoi filters
		else if(  comp == &bt_obsolete  ) {
			show_retired_vehicles = (show_retired_vehicles == 0);
			bt_obsolete.pressed = show_retired_vehicles;
			depot_t::update_all_win();
		}
		else if(  comp == &bt_show_all  ) {
			show_all = (show_all == 0);
			bt_show_all.pressed = show_all;
			depot_t::update_all_win();
		}
		else if(  comp == &name_filter_input  ) {
			depot_t::update_all_win();
		}
		else if(  comp == &bt_veh_action  ) {
			if(  veh_action == va_sell  ) {
				veh_action = va_append;
			}
			else {
				veh_action++;
			}
		}
		else if(  comp == &sort_by  ) {
			depot->selected_sort_by = sort_by.get_selection();
		}
		else if(  comp == &bt_copy_convoi  ) {
			if(  cnv.is_bound()  ) {
				if(  !welt->use_timeline()  ||  welt->get_settings().get_allow_buying_obsolete_vehicles()  ||  depot->check_obsolete_inventory( cnv )  ) {
					depot->call_depot_tool('c', cnv, NULL);
				}
				else {
					create_win( new news_img("Can't buy obsolete vehicles!"), w_time_delete, magic_none );
				}
			}
			return true;
		}
		else if(  comp == &convoy_selector  ) {
			icnv = p.i - 1;
		}
		else if(  comp == &line_selector  ) {
			const int selection = p.i;
			if(  selection == 0  ) { // unique
				if(  selected_line.is_bound()  ) {
					selected_line = linehandle_t();
					apply_line();
				}
				return true;
			}
			else if(  selection == 1  ) { // create new line
				if(  line_selector.is_dropped()  ) { // but not from next/prev buttons
					// promote existing individual schedule to line
					cbuffer_t buf;
					if(  cnv.is_bound()  &&  !selected_line.is_bound()  ) {
						schedule_t* schedule = cnv->get_schedule();
						if(  schedule  ) {
							schedule->sprintf_schedule( buf );
						}
					}

					last_selected_line = linehandle_t(); // clear last selected line so we can get a new one ...
					depot->call_depot_tool('l', convoihandle_t(), buf);
				}
				return true;
			}
			if(  last_selected_line.is_bound()  ) {
				if(  selection == 2  ) { // last selected line
					selected_line = last_selected_line;
					apply_line();
					return true;
				}
			}

			// access the selected element to get selected line
			line_scrollitem_t *item = dynamic_cast<line_scrollitem_t*>(line_selector.get_element(selection));
			if(  item  ) {
				selected_line = item->get_line();
				depot->set_last_selected_line( selected_line );
				last_selected_line = selected_line;
				apply_line();
				return true;
			}
		}
		else if(  comp == &vehicle_filter  ) {
			depot->selected_filter = vehicle_filter.get_selection();
		}
		else {
			return false;
		}
		build_vehicle_lists();
	}
	else {
		update_data();
		update_tabs();
	}
	return true;
}


bool depot_frame_t::infowin_event(const event_t *ev)
{
	// enable disable button actions
	if(  ev->ev_class < INFOWIN  &&  (depot == NULL  ||  welt->get_active_player() != depot->get_owner()) ) {
		return false;
	}

	const bool swallowed = gui_frame_t::infowin_event(ev);

	if(IS_WINDOW_CHOOSE_NEXT(ev)) {

		bool dir = (ev->ev_code==NEXT_WINDOW);
		depot_t *next_dep = depot_t::find_depot( depot->get_pos(), depot->get_typ(), depot->get_owner(), dir == NEXT_WINDOW );
		if(next_dep == NULL) {
			if(dir == NEXT_WINDOW) {
				// check the next from start of map
				next_dep = depot_t::find_depot( koord3d(-1,-1,0), depot->get_typ(), depot->get_owner(), true );
			}
			else {
				// respective end of map
				next_dep = depot_t::find_depot( koord3d(8192,8192,127), depot->get_typ(), depot->get_owner(), false );
			}
		}

		if(next_dep  &&  next_dep!=this->depot) {
			//  Replace our depot_frame_t with a new at the same position.
			scr_coord const pos = win_get_pos(this);
			destroy_win( this );

			next_dep->show_info();
			win_set_pos(win_get_magic((ptrdiff_t)next_dep), pos.x, pos.y);
			welt->get_viewport()->change_world_position(next_dep->get_pos());
		}
		else {
			// recenter on current depot
			welt->get_viewport()->change_world_position(depot->get_pos());
		}

		return true;
	}

	if (ev->ev_code == WIN_TOP) {
		update_data();
	}

	if(  swallowed  &&  get_focus()==&name_filter_input  &&  (ev->ev_class == EVENT_KEYBOARD  ||  ev->ev_class == EVENT_STRING)  ) {
		depot_t::update_all_win();
	}

	return swallowed;
}


void depot_frame_t::draw(scr_coord pos, scr_size size)
{
	const bool action_allowed = welt->get_active_player() == depot->get_owner();

	bt_new_line.enable( action_allowed );
	bt_change_line.enable( action_allowed );
	bt_copy_convoi.enable( action_allowed );
	bt_apply_line.enable( action_allowed );
	bt_start.enable( action_allowed );
	bt_schedule.enable( action_allowed );
	bt_destroy.enable( action_allowed );
	bt_sell.enable( action_allowed );
	bt_obsolete.enable( action_allowed );
	bt_show_all.enable( action_allowed );
	bt_veh_action.enable( action_allowed );
	line_button.enable( action_allowed );

	convoihandle_t cnv = depot->get_convoi(icnv);
	// check for data inconsistencies (can happen with withdraw-all and vehicle in depot)
	if(  !cnv.is_bound()  &&  !convoi_pics.empty()  ) {
		icnv=0;
		update_data();
		cnv = depot->get_convoi(icnv);
	}
	update_vehicle_info_text(pos);

	gui_frame_t::draw(pos, size);
}


void depot_frame_t::apply_line()
{
	if(  icnv > -1  ) {
		convoihandle_t cnv = depot->get_convoi( icnv );
		// if no convoi is selected, do nothing
		if(  !cnv.is_bound()  ) {
			return;
		}

		if(  selected_line.is_bound()  ) {
			// set new route only, a valid route is selected:
			char id[16];
			sprintf( id, "%i", selected_line.get_id() );
			cnv->call_convoi_tool('l', id );
		}
		else {
			// sometimes the user might wish to remove convoy from line
			// => we clear the schedule completely
			schedule_t *dummy = cnv->create_schedule()->copy();
			dummy->entries.clear();

			cbuffer_t buf;
			dummy->sprintf_schedule(buf);
			cnv->call_convoi_tool('g', (const char*)buf );

			delete dummy;
		}
	}
}


void depot_frame_t::open_schedule_editor()
{
	convoihandle_t cnv = depot->get_convoi( icnv );

	if(  cnv.is_bound()  &&  cnv->get_vehicle_count() > 0  ) {
		if(  selected_line.is_bound()  &&  (event_get_last_control_shift() ^ tool_t::control_invert)==2  ) { // update line with CTRL-click
			create_win( new line_management_gui_t( selected_line, depot->get_owner(), 0 ), w_info, (ptrdiff_t)selected_line.get_rep() );
		}
		else { // edit individual schedule
			// this can happen locally, since any update of the schedule is done during closing window
			cnv->open_schedule_window( welt->get_active_player() == cnv->get_owner() );
		}
	}
	else {
		create_win( new news_img("Please choose vehicles first\n"), w_time_delete, magic_none );
	}
}


void depot_frame_t::update_vehicle_info_text(scr_coord pos)
{
	// number of convoys
	if (old_vehicle_count != depot->get_vehicle_list().get_count()) {
		const uint32 count = depot->get_vehicle_list().get_count();
		switch (count) {
			case 0: {
				lb_convoi_count.buf().append(translator::translate("Keine Einzelfahrzeuge im Depot"));
				break;
			}
			case 1: {
				lb_convoi_count.buf().append("1 Einzelfahrzeug im Depot");
				break;
			}
			default: {
				lb_convoi_count.buf().printf( translator::translate("%d Einzelfahrzeuge im Depot"), count );
				break;
			}
		}
		old_vehicle_count = count;
		lb_convoi_count.update();
		cont_veh_action->set_size(cont_veh_action->get_size());
	}
	// unmark vehicle
	for(gui_image_list_t::image_data_t* const& iptr : convoi_pics) {
		iptr->count = 0;
	}
	// Find vehicle under mouse cursor
	gui_component_t const* const tab = tabs.get_aktives_tab();
	gui_image_list_t const* const lst =
		tab == &scrolly_pas       ? &pas       :
		tab == &scrolly_electrics ? &electrics :
		tab == &scrolly_loks      ? &loks      :
		&waggons;
	int x = get_mouse_x();
	int y = get_mouse_y();
	double resale_value = -1.0;
	const vehicle_desc_t *veh_type = NULL;
	bool new_vehicle_length_sb_force_zero = false;
	sint16 convoi_number = -1;
	scr_coord relpos = scr_coord(0, ((gui_scrollpane_t *)tabs.get_aktives_tab())->get_scroll_y());
	int sel_index = lst->index_at( pos + tabs.get_pos() - relpos, x, y - D_TITLEBAR_HEIGHT - tabs.get_required_size().h);

	if(  (sel_index != -1)  &&  (tabs.getroffen(x - pos.x, y - pos.y - D_TITLEBAR_HEIGHT))  ) {
		// cursor over a vehicle in the selection list
		const vector_tpl<gui_image_list_t::image_data_t*>& vec = (lst == &electrics ? electrics_vec : (lst == &pas ? pas_vec : (lst == &loks ? loks_vec : waggons_vec)));
		veh_type = vehicle_builder_t::get_info( vec[sel_index]->text );
		if(  vec[sel_index]->lcolor == color_idx_to_rgb(COL_RED)  ||  veh_action == va_sell  ) {
			// don't show new_vehicle_length_sb when can't actually add the highlighted vehicle, or selling from inventory
			new_vehicle_length_sb_force_zero = true;
		}
		if(  vec[sel_index]->count > 0  ) {
			resale_value = (double)calc_restwert( veh_type );
		}
	}
	else {
		// cursor over a vehicle in the convoi
		relpos = scr_coord(scrolly_convoi.get_scroll_x(), 0);

		convoi_number = sel_index = convoi.index_at(pos - relpos + scrolly_convoi.get_pos(), x, y - D_TITLEBAR_HEIGHT);
		if(  sel_index != -1  ) {
			convoihandle_t cnv = depot->get_convoi( icnv );
			veh_type = cnv->get_vehicle( sel_index )->get_desc();
			resale_value = cnv->get_vehicle( sel_index )->calc_sale_value();
			new_vehicle_length_sb_force_zero = true;

			// mark selected vehicle in convoi
			convoi_pics[convoi_number]->count = convoi_number+1;
		}
	}

	if(  veh_type  &&  veh_type != old_veh_type) {

		labels[LB_VEH_NAME]->buf().printf( "%s", translator::translate( veh_type->get_name(), welt->get_settings().get_name_language_id() ) );

		if(  veh_type->get_power() > 0  ) { // engine type
			labels[LB_VEH_NAME]->buf().printf( " (%s)\n", translator::translate( vehicle_builder_t::engine_type_names[veh_type->get_engine_type()+1] ) );
		}

		if(  sint64 fix_cost = welt->scale_with_month_length( veh_type->get_fixed_cost() )  ) {
			char tmp[128];
			money_to_string( tmp, veh_type->get_price() / 100.0, false );
			labels[LB_VEH_COST]->buf().printf( translator::translate("Cost: %8s (%.2f$/km %.2f$/m)\n"), tmp, veh_type->get_running_cost()/100.0, fix_cost/100.0 );
		}
		else {
			char tmp[128];
			money_to_string(  tmp, veh_type->get_price() / 100.0, false );
			labels[LB_VEH_COST]->buf().printf( translator::translate("Cost: %8s (%.2f$/km)\n"), tmp, veh_type->get_running_cost()/100.0 );
		}

		if(  veh_type->get_capacity() > 0  ) { // must translate as "Capacity: %3d%s %s\n"
			labels[LB_VEH_CAPACITY]->buf().printf( translator::translate("Capacity: %d%s %s\n"),
				veh_type->get_capacity(),
				translator::translate( veh_type->get_freight_type()->get_mass() ),
				veh_type->get_freight_type()->get_catg()==0 ? translator::translate( veh_type->get_freight_type()->get_name() ) : translator::translate( veh_type->get_freight_type()->get_catg_name() )
				);
		}
		else {
			labels[LB_VEH_CAPACITY]->buf().clear();
		}

		labels[LB_VEH_SPEED]->buf().printf( "%s %3d km/h\n", translator::translate("Max. speed:"), veh_type->get_topspeed() );

		if(  veh_type->get_power() > 0  ) {
			if(  veh_type->get_gear() != 64  ){
				labels[LB_VEH_POWER]->buf().printf( "%s %4d kW (x%0.2f)\n", translator::translate("Power:"), veh_type->get_power(), veh_type->get_gear() / 64.0 );
			}
			else {
				labels[LB_VEH_POWER]->buf().printf( translator::translate("Power: %4d kW\n"), veh_type->get_power() );
			}
		}

		// column 2
		labels[LB_VEH_WEIGHT]->buf().printf( "%s %4.1ft\n", translator::translate("Weight:"), veh_type->get_weight() / 1000.0 );
		labels[LB_VEH_DATE]->buf().printf( "%s: %s - ", translator::translate("Available"), translator::get_short_date(veh_type->get_intro_year_month() / 12, veh_type->get_intro_year_month() % 12) );

		if(  veh_type->get_retire_year_month() != DEFAULT_RETIRE_DATE * 12  ) {
			labels[LB_VEH_DATE]->buf().printf( "%s\n", translator::get_short_date(veh_type->get_retire_year_month() / 12, veh_type->get_retire_year_month() % 12) );
		}

		if(  char const* const copyright = veh_type->get_copyright()  ) {
			labels[LB_VEH_AUTHOR]->buf().printf( translator::translate("Constructed by %s"), copyright );
		}
		else {
			labels[LB_VEH_AUTHOR]->buf().clear();
		}

		if(  resale_value != -1.0  ) {
			char tmp[128];
			money_to_string(  tmp, resale_value / 100.0, false );
			labels[LB_VEH_VALUE]->buf().printf( "%s %8s", translator::translate("Restwert:"), tmp );
		}
		else {
			labels[LB_VEH_VALUE]->buf().clear();
		}

		// update speedbar
		new_vehicle_length_sb = new_vehicle_length_sb_force_zero ? 0 : convoi_length_ok_sb + convoi_length_slower_sb + convoi_length_too_slow_sb + veh_type->get_length();
	}
	else if (veh_type == NULL) {
		new_vehicle_length_sb = 0;
		for(uint32 i=LB_CNV_ALL; i<LB_MAX; i++) {
			labels[i]->buf().clear();
		}
	}

	if (veh_type != old_veh_type) {
		old_veh_type = veh_type;
		// update labels and size
		for(uint32 i=LB_CNV_ALL; i<LB_MAX; i++) {
			labels[i]->update();
		}
		cont_vehicle_labels->set_size(cont_vehicle_labels->get_size());
	}
}


void depot_frame_t::update_tabs()
{
	gui_component_t *old_tab = tabs.get_aktives_tab();
	tabs.clear();

	bool one = false;

	// add only if there are any
	if(  !pas_vec.empty()  ) {
		tabs.add_tab(&scrolly_pas, translator::translate( depot->get_passenger_name() ) );
		one = true;
	}

	// add only if there are any trolleybuses
	if(  !electrics_vec.empty()  ) {
		tabs.add_tab(&scrolly_electrics, translator::translate( depot->get_electrics_name() ) );
		one = true;
	}

	// add, if wagons are there ...
	if(  !loks_vec.empty()  ||  !waggons_vec.empty()  ) {
		tabs.add_tab(&scrolly_loks, translator::translate( depot->get_zieher_name() ) );
		one = true;
	}

	// only add, if there are wagons
	if(  !waggons_vec.empty()  ) {
		tabs.add_tab(&scrolly_waggons, translator::translate( depot->get_haenger_name() ) );
		one = true;
	}

	if(  !one  ) {
		// add passenger as default
		tabs.add_tab(&scrolly_pas, translator::translate( depot->get_passenger_name() ) );
	}

	// Look, if there is our old tab present again (otherwise it will be 0 by tabs.clear()).
	for(  uint8 i = 0;  i < tabs.get_count();  i++  ) {
		if(  old_tab == tabs.get_tab(i)  ) {
			// Found it!
			tabs.set_active_tab_index(i);
			break;
		}
	}
}


uint32 depot_frame_t::get_rdwr_id()
{
	return magic_depot;
}

void  depot_frame_t::rdwr( loadsave_t *file)
{
	if (file->is_version_less(120, 9)) {
		destroy_win(this);
		return;
	}

	// depot position
	koord3d pos;
	if(  file->is_saving()  ) {
		pos = depot->get_pos();
	}
	pos.rdwr( file );
	// window size
	scr_size size = get_windowsize();
	size.rdwr( file );

	if(  file->is_loading()  ) {
		depot_t *dep = welt->lookup(pos)->get_depot();
		if (dep) {
			init(dep);
		}
	}
	tabs.rdwr(file);
	vehicle_filter.rdwr(file);
	file->rdwr_byte(veh_action);
	file->rdwr_long(icnv);
	sort_by.rdwr(file);
	simline_t::rdwr_linehandle_t(file, selected_line);

	if(  depot  &&  file->is_loading()  ) {
		update_data();
		update_tabs();
		reset_min_windowsize();
		set_windowsize(size);

		win_set_magic(this, (ptrdiff_t)depot);
	}

	if (depot == NULL) {
		destroy_win(this);
	}
}
