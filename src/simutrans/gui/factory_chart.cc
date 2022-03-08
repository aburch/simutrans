/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../obj/leitung2.h"
#include "../dataobj/translator.h"

#include "factory_chart.h"

#define CHART_HEIGHT (90)

#define MAX_GOODS_COLOR (24)

static const uint8 goods_color[MAX_GOODS_COLOR] =
{
	/* greyish blue  */ 0,
	/* bright orange */ 33,
	/* cyan          */ 48,
	/* lemon yellow  */ 24,
	/* purple        */ 57,
	/* greyish green */ 80,
	/* lilac         */ 105,
	/* pale brown    */ 89,
	/* blue          */ COL_DARK_BLUE,
	/* dark green    */ 161,
	/* dark brown    */ 177,
	/* dark blue     */ 97,
	/* green         */ 41,
	/* reddish brown */ 113,
	/* magenta       */ COL_DARK_PURPLE,
	/* turquoise     */ 121,
	/* red           */ 129,
	/* muddy yellow  */ 192,
	/* bright green  */ COL_DARK_GREEN,
	/* dull orange   */ 65,
	/* pale yellow   */ 167,
	/* pale green    */ 201,
	/* orange        */ 152,
	/* pale purple   */ 217
};

static const char *const input_type[MAX_FAB_GOODS_STAT] =
{
	"Storage", "Arrived", "Consumed", "In Transit"
};

static const char *const output_type[MAX_FAB_GOODS_STAT] =
{
	"Storage", "Delivered", "Produced", "In Transit"
};

static const gui_chart_t::convert_proc goods_convert[MAX_FAB_GOODS_STAT] =
{
	convert_goods, NULL, convert_goods, NULL
};

static const char *const prod_type[MAX_FAB_STAT] =
{
	"Produktion", "Usage/Output",
	"Electricity", "Passagiere", "Post",
	"Generated", "Departed", "Arrived",
	"Generated", "Departed", "Arrived"
};

static const char *const ref_type[MAX_FAB_REF_LINE] =
{
	"Electricity", "Passagiere", "Post",
	"Electricity", "Passagiere", "Post",
};

static const uint8 prod_color[MAX_FAB_STAT] =
{
	COL_LILAC, COL_LEMON_YELLOW,
	COL_LIGHT_GREEN, 23, COL_LIGHT_PURPLE,
	COL_LIGHT_TURQUOISE, 51, 49,
	COL_LIGHT_ORANGE, COL_ORANGE, COL_DARK_ORANGE
};

static const gui_chart_t::convert_proc prod_convert[MAX_FAB_STAT] =
{
	NULL, convert_power, convert_boost, convert_boost, convert_boost, NULL, NULL, NULL, NULL, NULL, NULL
};

static const gui_chart_t::convert_proc ref_convert[MAX_FAB_REF_LINE] =
{
	convert_boost, convert_boost, convert_boost, convert_power, NULL, NULL
};

static const uint8 ref_color[MAX_FAB_REF_LINE] =
{
	137, COL_LIGHT_BLUE, COL_LIGHT_RED,
	COL_DARK_GREEN, COL_SOFT_BLUE, COL_OPERATION
};

static const char *const label_text[MAX_PROD_LABEL] =
{
	"Boost (%)", "Max Boost (%)", "Demand", "Passagiere", "Post", "Power (MW)"
};

// Mappings from cell position to buttons, labels, charts
static const uint8 prod_cell_button[] =
{
	FAB_PRODUCTION, MAX_FAB_STAT,       MAX_FAB_STAT,      FAB_POWER,
	MAX_FAB_STAT,   FAB_BOOST_ELECTRIC, FAB_BOOST_PAX,     FAB_BOOST_MAIL,
	MAX_FAB_STAT,   MAX_FAB_STAT,       MAX_FAB_STAT,      MAX_FAB_STAT,
	MAX_FAB_STAT,   MAX_FAB_STAT,       MAX_FAB_STAT,      MAX_FAB_STAT,
	MAX_FAB_STAT,   FAB_PAX_GENERATED,  FAB_PAX_DEPARTED,  FAB_PAX_ARRIVED,
	MAX_FAB_STAT,   FAB_MAIL_GENERATED, FAB_MAIL_DEPARTED, FAB_MAIL_ARRIVED,
};

static const uint8 prod_cell_label[] =
{
	MAX_PROD_LABEL, MAX_PROD_LABEL, 5, MAX_PROD_LABEL,
	0, MAX_PROD_LABEL, MAX_PROD_LABEL, MAX_PROD_LABEL,
	1, MAX_PROD_LABEL, MAX_PROD_LABEL, MAX_PROD_LABEL,
	2, MAX_PROD_LABEL, MAX_PROD_LABEL, MAX_PROD_LABEL,
	3, MAX_PROD_LABEL, MAX_PROD_LABEL, MAX_PROD_LABEL,
	4, MAX_PROD_LABEL, MAX_PROD_LABEL, MAX_PROD_LABEL,
};

static const uint8 prod_cell_ref[] =
{
	MAX_FAB_REF_LINE, MAX_FAB_REF_LINE, MAX_FAB_REF_LINE, MAX_FAB_REF_LINE,
	MAX_FAB_REF_LINE, MAX_FAB_REF_LINE, MAX_FAB_REF_LINE, MAX_FAB_REF_LINE,
	MAX_FAB_REF_LINE, FAB_REF_MAX_BOOST_ELECTRIC, FAB_REF_MAX_BOOST_PAX, FAB_REF_MAX_BOOST_MAIL,
	MAX_FAB_REF_LINE, FAB_REF_DEMAND_ELECTRIC, FAB_REF_DEMAND_PAX, FAB_REF_DEMAND_MAIL,
	MAX_FAB_REF_LINE, MAX_FAB_REF_LINE, MAX_FAB_REF_LINE, MAX_FAB_REF_LINE,
	MAX_FAB_REF_LINE, MAX_FAB_REF_LINE, MAX_FAB_REF_LINE, MAX_FAB_REF_LINE,
};

factory_chart_t::factory_chart_t(const fabrik_t *_factory)
{
	set_factory(_factory);
}

void factory_chart_t::set_factory(const fabrik_t *_factory)
{
	factory = _factory;
	if (factory == NULL) {
		return;
	}

	remove_all();
	goods_cont.remove_all();
	prod_cont.remove_all();
	button_to_chart.clear();

	set_table_layout(1,0);
	add_component( &tab_panel );

	const uint32 input_count = factory->get_input().get_count();
	const uint32 output_count = factory->get_output().get_count();
	if(  input_count>0  ||  output_count>0  ) {
		// only add tab if there is something to display
		tab_panel.add_tab(&goods_cont, translator::translate("Goods"));
		goods_cont.set_table_layout(1, 0);
		goods_cont.add_component(&goods_chart);

		// GUI components for goods input/output statistics
		goods_chart.set_min_size(scr_size(D_DEFAULT_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT, CHART_HEIGHT));
		goods_chart.set_dimension(12, 10000);
		goods_chart.set_background(SYSCOL_CHART_BACKGROUND);

		uint32 count = 0;

		// first tab: charts for goods production/consumption
		goods_cont.add_table(4, 0)->set_force_equal_columns(true);

		if (input_count > 0) {
			// create table of buttons, insert curves to chart
			goods_cont.new_component_span<gui_label_t>("Verbrauch", 4);

			const array_tpl<ware_production_t>& input = factory->get_input();
			for (uint32 g = 0; g < input_count; ++g) {
				goods_cont.new_component<gui_label_t>(input[g].get_typ()->get_name());
				for (int s = 0; s < MAX_FAB_GOODS_STAT; ++s) {
					uint16 curve = goods_chart.add_curve(color_idx_to_rgb(goods_color[count % MAX_GOODS_COLOR] + (s * 3) / 2), input[g].get_stats(), MAX_FAB_GOODS_STAT, s, MAX_MONTH, false, false, true, 0, goods_convert[s]);

					button_t* b = goods_cont.new_component<button_t>();
					b->init(button_t::box_state_automatic | button_t::flexible, input_type[s]);
					b->background_color = color_idx_to_rgb(goods_color[count % MAX_GOODS_COLOR] + (s * 3) / 2);
					b->pressed = false;
					button_to_chart.append(b, &goods_chart, curve);

					if ((s % 2) == 1) {
						// skip last cell in current row, first cell in next row
						goods_cont.new_component<gui_empty_t>();
						if (s + 1 < MAX_FAB_GOODS_STAT) {
							goods_cont.new_component<gui_empty_t>();
						}
					}
				}
				count++;
			}
		}
		if (output_count > 0) {
			goods_cont.new_component_span<gui_label_t>("Produktion", 4);
			const array_tpl<ware_production_t>& output = factory->get_output();
			for (uint32 g = 0; g < output_count; ++g) {
				goods_cont.new_component<gui_label_t>(output[g].get_typ()->get_name());
				for (int s = 0; s < 3; ++s) {
					uint16 curve = goods_chart.add_curve(color_idx_to_rgb(goods_color[count % MAX_GOODS_COLOR] + s * 2), output[g].get_stats(), MAX_FAB_GOODS_STAT, s, MAX_MONTH, false, false, true, 0, goods_convert[s]);

					button_t* b = goods_cont.new_component<button_t>();
					b->init(button_t::box_state_automatic | button_t::flexible, output_type[s]);
					b->background_color = color_idx_to_rgb(goods_color[count % MAX_GOODS_COLOR] + s * 2);
					b->pressed = false;
					button_to_chart.append(b, &goods_chart, curve);
				}
				count++;
			}
		}
		goods_cont.end_table();
		goods_cont.new_component<gui_empty_t>();
	}

	tab_panel.add_tab( &prod_cont, translator::translate("Production/Boost") );
	prod_cont.set_table_layout(1,0);
	prod_cont.add_component( &prod_chart );

	prod_cont.add_table(4, 0)->set_force_equal_columns(true);
	// GUI components for other production-related statistics
	prod_chart.set_min_size( scr_size( D_DEFAULT_WIDTH-D_MARGIN_LEFT-D_MARGIN_RIGHT, CHART_HEIGHT ) );
	prod_chart.set_dimension(12, 10000);
	prod_chart.set_background(SYSCOL_CHART_BACKGROUND);

	for(  int row = 0, cell = 0; row<6; row++) {
		for(  int col = 0; col<4; col++, cell++) {
			// labels
			if (prod_cell_label[cell] < MAX_PROD_LABEL) {
				prod_cont.new_component<gui_label_t>(label_text[ prod_cell_label[cell] ]);
			}
			// chart, buttons for production
			else if (prod_cell_button[cell] < MAX_FAB_STAT) {
				uint8 s = prod_cell_button[cell];
				// add curve
				uint16 curve = prod_chart.add_curve( color_idx_to_rgb(prod_color[s]), factory->get_stats(), MAX_FAB_STAT, s, MAX_MONTH, (2<=s  &&  s<=4) ? PERCENT : STANDARD, false, true, 0, prod_convert[s] );
				// only show buttons, if the is something to do ...
				if(
					(s==FAB_BOOST_ELECTRIC  &&  (factory->get_desc()->is_electricity_producer()  ||  factory->get_desc()->get_electric_boost()==0))  ||
					(s==FAB_BOOST_PAX  &&  factory->get_desc()->get_pax_boost()==0)  ||
					(s==FAB_BOOST_MAIL  &&  factory->get_desc()->get_mail_boost()==0)
				) {
					prod_cont.new_component<gui_empty_t>();
					continue;
				}
				// add button
				button_t *b = prod_cont.new_component<button_t>();
				b->init(button_t::box_state_automatic | button_t::flexible, prod_type[s]);
				b->background_color = color_idx_to_rgb(prod_color[s]);
				b->pressed = false;
				button_to_chart.append(b, &prod_chart, curve);
			}
			// chart, buttons for reference lines
			else if (prod_cell_ref[cell] < MAX_FAB_REF_LINE) {
				uint8 r = prod_cell_ref[cell];
				// add curve
				uint16 curve = prod_chart.add_curve( color_idx_to_rgb(ref_color[r]), prod_ref_line_data + r, 0, 0, MAX_MONTH, r<3 ? PERCENT : STANDARD, false, true, 0, ref_convert[r] );
				if(
					(r==FAB_REF_MAX_BOOST_ELECTRIC  &&  (factory->get_desc()->is_electricity_producer()  ||  factory->get_desc()->get_electric_boost()==0))  ||
					(r==FAB_REF_MAX_BOOST_PAX  &&  factory->get_desc()->get_pax_boost()==0)  ||
					(r==FAB_REF_MAX_BOOST_MAIL  &&  factory->get_desc()->get_mail_boost()==0)  ||
					(r==FAB_REF_DEMAND_ELECTRIC  &&  (factory->get_desc()->is_electricity_producer()  ||  factory->get_desc()->get_electric_demand()==0))  ||
					(r==FAB_REF_DEMAND_PAX  &&  factory->get_desc()->get_pax_demand()==0)  ||
					(r==FAB_REF_DEMAND_MAIL  &&  factory->get_desc()->get_mail_demand()==0)
				) {
					prod_cont.new_component<gui_empty_t>();
					continue;
				}
				// add button
				button_t *b = prod_cont.new_component<button_t>();
				b->init(button_t::box_state_automatic | button_t::flexible, ref_type[r]);
				b->background_color = color_idx_to_rgb(ref_color[r]);
				b->pressed = false;
				button_to_chart.append(b, &prod_chart, curve);
			}
			else {
				prod_cont.new_component<gui_empty_t>();
			}
		}
	}
	prod_cont.end_table();
	prod_cont.new_component<gui_empty_t>();

	set_size(get_min_size());
	// initialize reference lines' data (these do not change over time)
	prod_ref_line_data[FAB_REF_MAX_BOOST_ELECTRIC] = factory->get_desc()->get_electric_boost();
	prod_ref_line_data[FAB_REF_MAX_BOOST_PAX] = factory->get_desc()->get_pax_boost();
	prod_ref_line_data[FAB_REF_MAX_BOOST_MAIL] = factory->get_desc()->get_mail_boost();
}


factory_chart_t::~factory_chart_t()
{
	button_to_chart.clear();
}


void factory_chart_t::update()
{
	// update reference lines' data (these might change over time)
	prod_ref_line_data[FAB_REF_DEMAND_ELECTRIC] = ( factory->get_desc()->is_electricity_producer() ? 0 : factory->get_scaled_electric_demand() );
	prod_ref_line_data[FAB_REF_DEMAND_PAX] = factory->get_scaled_pax_demand();
	prod_ref_line_data[FAB_REF_DEMAND_MAIL] = factory->get_scaled_mail_demand();
}


void factory_chart_t::rdwr( loadsave_t *file )
{
	// button-to-chart array
	button_to_chart.rdwr(file);
}
