/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Factories builder dialog
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simtool.h"

#include "../bauer/fabrikbauer.h"

#include "../descriptor/ground_desc.h"
#include "../descriptor/intro_dates.h"
#include "../descriptor/factory_desc.h"

#include "../dataobj/translator.h"

#include "../utils/simrandom.h"
#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "factory_edit.h"
#include "components/gui_label.h"


// new tool definition
tool_build_land_chain_t factory_edit_frame_t::land_chain_tool = tool_build_land_chain_t();
tool_city_chain_t factory_edit_frame_t::city_chain_tool = tool_city_chain_t();
tool_build_factory_t factory_edit_frame_t::fab_tool = tool_build_factory_t();
cbuffer_t factory_edit_frame_t::param_str;


static bool compare_fabrik_desc(const factory_desc_t* a, const factory_desc_t* b)
{
	int diff = strcmp( a->get_name(), b->get_name() );
	return diff < 0;
}

static bool compare_factory_desc_trans(const factory_desc_t* a, const factory_desc_t* b)
{
	int diff = strcmp( translator::translate(a->get_name()), translator::translate(b->get_name()) );
	return diff < 0;
}

factory_edit_frame_t::factory_edit_frame_t(player_t* player_) :
	extend_edit_gui_t(translator::translate("factorybuilder"), player_),
	factory_list(16)
{
	land_chain_tool.cursor = city_chain_tool.cursor = fab_tool.cursor = tool_t::general_tool[TOOL_BUILD_FACTORY]->cursor;
	fac_desc = NULL;

	bt_city_chain.init( button_t::square_state, "Only city chains");
	bt_city_chain.add_listener(this);
	cont_right.add_component(&bt_city_chain);

	bt_land_chain.init( button_t::square_state, "Only land chains");
	bt_land_chain.add_listener(this);
	cont_right.add_component(&bt_land_chain);

	// rotation, production
	gui_aligned_container_t *tbl = cont_right.add_table(2,2);
	tbl->new_component<gui_label_t>("Rotation");
	tbl->add_component(&cb_rotation);
	cb_rotation.add_listener(this);
	cb_rotation.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("random"), SYSCOL_TEXT) ;

	tbl->new_component<gui_label_t>("Produktion");

	inp_production.set_limits(0,9999);
	inp_production.add_listener( this );
	tbl->add_component(&inp_production);
	cont_right.end_table();

	fill_list( is_show_trans_name );

	reset_min_windowsize();
}



// fill the current factory_list
void factory_edit_frame_t::fill_list( bool translate )
{
	const bool allow_obsolete = bt_obsolete.pressed;
	const bool use_timeline = bt_timeline.pressed;
	const bool city_chain = bt_city_chain.pressed;
	const bool land_chain = bt_land_chain.pressed;
	const sint32 month_now = bt_timeline.pressed ? welt->get_current_month() : 0;

	factory_list.clear();

	// timeline will be obeyed; however, we may show obsolete ones ...
	FOR(stringhashtable_tpl<factory_desc_t const*>, const& i, factory_builder_t::get_factory_table()) {
		factory_desc_t const* const desc = i.value;
		if(desc->get_distribution_weight()>0) {
			// DistributionWeight=0 is obsoleted item, only for backward compatibility

			if(!use_timeline  ||  (!desc->get_building()->is_future(month_now)  &&  (!desc->get_building()->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this

				if(city_chain) {
					if (desc->get_placement() == factory_desc_t::City && desc->is_consumer_only()) {
						factory_list.insert_ordered( desc, translate?compare_factory_desc_trans:compare_fabrik_desc );
					}
				}
				if(land_chain) {
					if (desc->get_placement() == factory_desc_t::Land && desc->is_consumer_only()) {
						factory_list.insert_ordered( desc, translate?compare_factory_desc_trans:compare_fabrik_desc );
					}
				}
				if(!city_chain  &&  !land_chain) {
					factory_list.insert_ordered( desc, translate?compare_factory_desc_trans:compare_fabrik_desc );
				}
			}
		}
	}

	// now build scrolled list
	scl.clear_elements();
	scl.set_selection(-1);
	FOR(vector_tpl<factory_desc_t const*>, const i, factory_list) {
		PIXVAL const color =
			i->is_consumer_only() ? color_idx_to_rgb(COL_BLUE)       :
			i->is_producer_only() ? color_idx_to_rgb(COL_DARK_GREEN) :
			SYSCOL_TEXT;
		char const* const name = translate ? translator::translate(i->get_name()) : i->get_name();
		scl.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(name, color);
		if (i == fac_desc) {
			scl.set_selection(scl.get_count()-1);
		}
	}
	// always update current selection (since the tool may depend on it)
	change_item_info( scl.get_selection() );

	reset_min_windowsize();
}



bool factory_edit_frame_t::action_triggered( gui_action_creator_t *comp,value_t e)
{
	// only one chain can be shown
	if(  comp==&bt_city_chain  ) {
		bt_city_chain.pressed ^= 1;
		if(bt_city_chain.pressed) {
			bt_land_chain.pressed = 0;
		}
		fill_list( is_show_trans_name );
	}
	else if(  comp==&bt_land_chain  ) {
		bt_land_chain.pressed ^= 1;
		if(bt_land_chain.pressed) {
			bt_city_chain.pressed = 0;
		}
		fill_list( is_show_trans_name );
	}
	else if( comp == &cb_rotation) {
		change_item_info( scl.get_selection() );
	}
	else if(fac_desc) {
		if (comp==&inp_production) {
			production = inp_production.get_value();
		}
		// update info ...
		change_item_info( scl.get_selection() );
	}
	return extend_edit_gui_t::action_triggered(comp,e);
}



void factory_edit_frame_t::change_item_info(sint32 entry)
{
	if(entry>=0  &&  entry<(sint32)factory_list.get_count()) {

		const factory_desc_t *new_fac_desc = factory_list[entry];
		if(new_fac_desc!=fac_desc) {

			fac_desc = new_fac_desc;
			production = fac_desc->get_productivity() + sim_async_rand( fac_desc->get_range() );
			// Knightly : should also consider the effects of the minimum number of fields
			const field_group_desc_t *const field_group_desc = fac_desc->get_field_group();
			if(  field_group_desc  &&  field_group_desc->get_field_class_count()>0  ) {
				const weighted_vector_tpl<uint16> &field_class_indices = field_group_desc->get_field_class_indices();
				sint32 min_fields = field_group_desc->get_min_fields();
				while(  min_fields-- > 0  ) {
					const uint16 field_class_index = field_class_indices.at_weight( sim_async_rand( field_class_indices.get_sum_weight() ) );
					production += field_group_desc->get_field_class(field_class_index)->get_field_production();
				}
			}
			production = welt->scale_with_month_length(production);
			inp_production.set_value( production);
			// show produced goods
			buf.clear();
			if (!fac_desc->is_consumer_only()) {
				buf.append( translator::translate("Produktion") );
				buf.append("\n");
				for (uint i = 0; i < fac_desc->get_product_count(); i++) {
					buf.append(" - ");
					buf.append( translator::translate(fac_desc->get_product(i)->get_output_type()->get_name()) );
					buf.append( " (" );
					buf.append( translator::translate(fac_desc->get_product(i)->get_output_type()->get_catg_name()) );
					buf.append( ")\n" );
				}
				buf.append("\n");
			}

			// show consumed goods
			if (!fac_desc->is_producer_only()) {
				buf.append( translator::translate("Verbrauch") );
				buf.append("\n");
				for(  int i=0;  i<fac_desc->get_supplier_count();  i++  ) {
					buf.append(" - ");
					buf.append( translator::translate(fac_desc->get_supplier(i)->get_input_type()->get_name()) );
					buf.append( " (" );
					buf.append( translator::translate(fac_desc->get_supplier(i)->get_input_type()->get_catg_name()) );
					buf.append( ")\n" );
				}
				buf.append("\n");
			}

			if(fac_desc->is_electricity_producer()) {
				buf.append( translator::translate( "Electricity producer\n\n" ) );
			}

			// now the house stuff
			const building_desc_t *desc = fac_desc->get_building();

			// climates
			buf.append( translator::translate("allowed climates:\n") );
			uint16 cl = desc->get_allowed_climate_bits();
			if(cl==0) {
				buf.append( translator::translate("none") );
				buf.append("\n");
			}
			else {
				for(uint16 i=0;  i<=arctic_climate;  i++  ) {
					if(cl &  (1<<i)) {
						buf.append(" - ");
						buf.append(translator::translate(ground_desc_t::get_climate_name_from_bit((climate)i)));
						buf.append("\n");
					}
				}
			}
			buf.append("\n");

			factory_desc_t const& f = *factory_list[entry];
			buf.printf( translator::translate("Passenger Demand %d\n"), f.get_pax_demand()  != 65535 ? f.get_pax_demand()  : f.get_pax_level());
			buf.printf( translator::translate("Mail Demand %d\n"),      f.get_mail_demand() != 65535 ? f.get_mail_demand() : f.get_pax_level() >> 2);

			buf.printf("%s%u", translator::translate("\nBauzeit von"), desc->get_intro_year_month() / 12);
			if(desc->get_retire_year_month()!=DEFAULT_RETIRE_DATE*12) {
				buf.printf("%s%u", translator::translate("\nBauzeit bis"), desc->get_retire_year_month() / 12);
			}

			if (char const* const maker = desc->get_copyright()) {
				buf.append("\n");
				buf.printf(translator::translate("Constructed by %s"), maker);
			}
			buf.append("\n");

			// reset combobox
			cb_rotation.clear_elements();
			cb_rotation.new_component<gui_rotation_item_t>(gui_rotation_item_t::random);
			for(uint8 i = 0; i<desc->get_all_layouts(); i++) {
				cb_rotation.new_component<gui_rotation_item_t>(i);
			}

			// orientation (255=random)
			if(desc->get_all_layouts()>1) {
				cb_rotation.set_selection(0);
			}
			else {
				cb_rotation.set_selection(1);
			}

			// now for the tool
			fac_desc = factory_list[entry];
		}

		const building_desc_t *desc = fac_desc->get_building();
		uint8 rotation = get_rotation();
		uint8 rot = (rotation==255) ? 0 : rotation;
		building_image.init(desc, rot);

		// the tools will be always updated, even though the data up there might be still current
		param_str.clear();
		param_str.printf("%i%c%i,%s", bt_climates.pressed, rotation==255 ? '#' : '0'+rotation, production, fac_desc->get_name() );
		if(bt_land_chain.pressed) {
			land_chain_tool.set_default_param(param_str);
			welt->set_tool( &land_chain_tool, player );
		}
		else if(bt_city_chain.pressed) {
			city_chain_tool.set_default_param(param_str);
			welt->set_tool( &city_chain_tool, player );
		}
		else {
			fab_tool.set_default_param(param_str);
			welt->set_tool( &fab_tool, player );
		}
	}
	else if(fac_desc!=NULL) {
		cb_rotation.clear_elements();
		cb_rotation.new_component<gui_rotation_item_t>(gui_rotation_item_t::random);

		building_image.init(NULL, 0);
		fac_desc = NULL;
		welt->set_tool( tool_t::general_tool[TOOL_QUERY], player );
	}
	info_text.recalc_size();
	reset_min_windowsize();
}


void factory_edit_frame_t::set_windowsize(scr_size size)
{
	extend_edit_gui_t::set_windowsize(size);

	// manually set width of cb_rotation and inp_production
	scr_size cbs = cb_rotation.get_size();
	scr_size nis = inp_production.get_size();
	scr_coord_val w = max(cbs.w, nis.w);
	cb_rotation.set_size(scr_size(w, cbs.h));
	inp_production.set_size(scr_size(w, nis.h));
}
