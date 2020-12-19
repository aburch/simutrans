/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>
#include <cmath>

#include "gui_chart.h"
#include "../gui_frame.h"
#include "../simwin.h"
#include "../../simcolor.h"
#include "../../utils/simstring.h"
#include "../../dataobj/environment.h"
#include "../../display/simgraph.h"
#include "../gui_theme.h"

static char tooltip[64];

/**
 * Set background color. -1 means no background
 */
void gui_chart_t::set_background(FLAGGED_PIXVAL color)
{
	background = color;
}


gui_chart_t::gui_chart_t() : gui_component_t()
{
	// no toolstips at the start
	tooltipcoord = scr_coord::invalid;

	seed = 0;
	show_x_axis = true;
	show_y_axis = true;
	ltr = false;
	x_elements = 0;
	x_axis_span = 1;
	min_size = scr_size(0,0);

	// transparent by default
	background = TRANSPARENT_FLAGS;
}

void gui_chart_t::set_min_size(scr_size s)
{
	min_size = s;
}

scr_size gui_chart_t::get_min_size() const
{
	return min_size;
}

scr_size gui_chart_t::get_max_size() const
{
	return scr_size::inf;
}


uint32 gui_chart_t::add_curve(PIXVAL color, const sint64 *values, int size, int offset, int elements, int type, bool show, bool show_value, int precision, convert_proc proc, chart_marker_t marker_type)
{
	curve_t new_curve;
	new_curve.color = color;
	new_curve.values = values;
	new_curve.size = size;
	new_curve.offset = offset;
	new_curve.elements = elements;
	new_curve.show = show;
	new_curve.show_value = show_value;
	new_curve.type = type;
	switch (type) {
		case MONEY:   new_curve.suffix = "$"; break;
		case PERCENT: new_curve.suffix = "%"; break;
		case DISTANCE:   new_curve.suffix = "km"; break;
		case FORCE:   new_curve.suffix = "kN"; break;
		case KMPH:   new_curve.suffix = "km/h"; break;
		default:      new_curve.suffix = NULL; break;
	}
	new_curve.precision = precision;
	new_curve.convert = proc;
	new_curve.marker_type = marker_type;
	curves.append(new_curve);
	return curves.get_count()-1;
}


void gui_chart_t::hide_curve(unsigned int id)
{
	if (id < curves.get_count()) {
		curves.at(id).show = false;
	}
}


void gui_chart_t::show_curve(unsigned int id)
{
	if (id < curves.get_count()) {
		curves.at(id).show = true;
	}
}


void gui_chart_t::draw(scr_coord offset)
{
	scr_coord chart_offset(100,0);
	int maximum_axis_len = 22;

	offset += pos;
	if(  size.w<D_DEFAULT_WIDTH  ) {
		chart_offset.x = size.w/6;
		maximum_axis_len = max(7,chart_offset.x/6);
	}

	offset += chart_offset;
	scr_size chart_size = size-chart_offset-scr_size(10,4+LINESPACE);

	sint64 last_year=0, tmp=0;
	char cmin[128] = "0", cmax[128] = "0", digit[11];

	sint64 baseline = 0;
	sint64* pbaseline = &baseline;

	float scale = 0;
	float* pscale = &scale;

	// calc baseline and scale
	calc_gui_chart_values(pbaseline, pscale, cmin, cmax, 18);

	// draw background if desired
	if(background != TRANSPARENT_FLAGS) {
		display_fillbox_wh_clip_rgb(offset.x, offset.y, chart_size.w, chart_size.h, background, false);
	}
	int tmpx, factor;
	if(env_t::left_to_right_graphs) {
		tmpx = offset.x + chart_size.w - chart_size.w % (x_elements - 1);
		factor = -1;
	}
	else {
		tmpx = offset.x;
		factor = 1;
	}

	// draw zero line
	display_direct_line_rgb(offset.x+1, offset.y+(scr_coord_val)baseline, offset.x+chart_size.w-2, offset.y+(scr_coord_val)baseline, SYSCOL_CHART_LINES_ZERO);

	if (show_y_axis) {

		// draw zero number only, if it will not disturb any other printed values!
		if ((baseline > 18) && (baseline < chart_size.h -18)) {
			display_proportional_clip_rgb(offset.x - 4, offset.y+(scr_coord_val)baseline-3, "0", ALIGN_RIGHT, SYSCOL_TEXT_HIGHLIGHT, true );
		}

		// display min/max money values
		display_proportional_clip_rgb(offset.x - 4, offset.y-5, cmax, ALIGN_RIGHT, SYSCOL_TEXT_HIGHLIGHT, true );
		display_proportional_clip_rgb(offset.x - 4, offset.y+chart_size.h-5, cmin, ALIGN_RIGHT, SYSCOL_TEXT_HIGHLIGHT, true );
	}

	// draw chart frame
	display_ddd_box_clip_rgb(offset.x, offset.y, chart_size.w, chart_size.h, SYSCOL_SHADOW, SYSCOL_HIGHLIGHT);

	// draw chart lines
	scr_coord_val x_last = 0;  // remember last digit position to avoid overwriting by next label
	for(  int i = 0;  i < x_elements;  i++  ) {
		const int j = env_t::left_to_right_graphs ? x_elements - 1 - i : i;
		const scr_coord_val x0 = tmpx + factor * (chart_size.w / (x_elements - 1) ) * j;
		const PIXVAL line_color = (i%2) ? SYSCOL_CHART_LINES_ODD : SYSCOL_CHART_LINES_EVEN;
		if(  show_x_axis  ) {
			// display x-axis
			int val = (abort_display_x && env_t::left_to_right_graphs) ? (abort_display_x - j - 1) * x_axis_span : seed - (j*x_axis_span);
			sprintf(digit, "%i", abs(val));
			scr_coord_val x =  x0 - (seed != j ? (int)(2 * log( (double)abs(seed - j) )) : 0);
			if(  x > x_last  ) {
				x_last = x + display_proportional_clip_rgb( x, offset.y + chart_size.h + 6, digit, ALIGN_LEFT, line_color, true );
			}
		}
		// year's vertical lines
		display_vline_wh_clip_rgb( x0, offset.y + 1, chart_size.h - 2, line_color, false );
	}

	// display current value?
	int tooltip_n=-1;
	int ttcx = tooltipcoord.x-chart_offset.x;
	if(  ttcx>0  &&  ttcx<chart_size.w  ) {
		const uint8 temp_x = abort_display_x ? (env_t::left_to_right_graphs ? abort_display_x : x_elements-abort_display_x) : x_elements;
		if(env_t::left_to_right_graphs) {
			tooltip_n = x_elements-1-(ttcx*temp_x+4)/(chart_size.w|1);
		}
		else {
			tooltip_n = (ttcx*temp_x+4)/(chart_size.w|1);
		}
	}

	// draw chart's curves
	FOR(slist_tpl<curve_t>, const& c, curves) {
		if (c.show) {
			double display_tmp;
			int start = abort_display_x ? (env_t::left_to_right_graphs ? c.elements - abort_display_x : 0) : 0;
			int end = abort_display_x ? (env_t::left_to_right_graphs ? c.elements : abort_display_x) : c.elements;
			// for each curve iterate through all elements and display curve
			for (int i = start; i < end; i++) {

				tmp = c.values[i*c.size+c.offset];
				// convert value where necessary
				if(  c.convert  ) {
					tmp = c.convert(tmp);
					display_tmp = tmp;
				}
				else if(  c.type == MONEY || c.type == PERCENT  ) {
					display_tmp = tmp*0.01;
					tmp /= 100;
				}
				else {
					display_tmp = tmp;
				}

				// display marker(box) for financial value
				if (i < end) {
					scr_coord_val x = tmpx + factor * (chart_size.w / (x_elements - 1))*(i - start) - 2;
					scr_coord_val y = (scr_coord_val)(offset.y + baseline - (long)(tmp / scale) - 2);
					switch (c.marker_type)
					{
					case cross:
						display_direct_line_rgb(x, y, x + 4, y + 4, c.color);
						display_direct_line_rgb(x + 4, y, x, y + 4, c.color);
						break;
					case diamond:
						for (int j = 0; j < 5; j++) {
							display_vline_wh_clip_rgb(x + j, y + abs(2 - j), 5 - 2 * abs(2 - j), c.color, false);
						}
						break;
					case round_box:
						display_filled_roundbox_clip(x, y, 5, 5, c.color, true);
						break;
					case none:
						// display nothing
						break;
					case square:
					default:
						display_fillbox_wh_clip_rgb(x, y, 5, 5, c.color, true);
						break;
					}
				}

				// Change digits after drawing a line to smooth the curve of the physics chart
				if (c.type == KMPH) {
					display_tmp = (double)speed_to_kmh(tmp*10)/10.0;
				}
				else if (c.type == FORCE) {
					display_tmp = tmp * 0.001;
				}

				// display tooltip?
				if(i==tooltip_n  &&  abs((int)(baseline-(int)(tmp/scale)-tooltipcoord.y))<10) {
					number_to_string(tooltip, display_tmp, c.precision);
					if (c.suffix) {
						strcat(tooltip, c.suffix);
					}
					win_set_tooltip( get_mouse_x()+TOOLTIP_MOUSE_OFFSET_X, get_mouse_y()-TOOLTIP_MOUSE_OFFSET_Y, tooltip );
				}

				// draw line between two financial markers; this is only possible from the second value on
				if (i > start && i < end) {
					display_direct_line_rgb(tmpx + factor * (chart_size.w / (x_elements - 1))*(i-start-1),
						(scr_coord_val)( offset.y+baseline-(int)(last_year/scale) ),
						tmpx+factor*(chart_size.w / (x_elements - 1))*(i - start),
						(scr_coord_val)( offset.y+baseline-(int)(tmp/scale) ),
						c.color);
				}
				else if (i == 0 && !abort_display_x || abort_display_x && i == start) {
					// for the first element print the current value (optionally)
					// only print value if not too narrow to min/max/zero
					if(  c.show_value  ) {
						number_to_string_fit(cmin, display_tmp, c.precision, maximum_axis_len - (c.suffix != NULL) );
						if (c.suffix) {
							strcat(cmin, c.suffix);
						}

						if(  env_t::left_to_right_graphs  ) {
							const sint16 width = proportional_string_width(cmin)+7;
							display_ddd_proportional( tmpx + 8, (scr_coord_val)(offset.y+baseline-(int)(tmp/scale)-4), width, 0, color_idx_to_rgb(COL_GREY4), c.color, cmin, true);
						}
						else if(  (baseline-tmp/scale-8) > 0  &&  (baseline-tmp/scale+8) < chart_size.h  &&  abs((int)(tmp/scale)) > 9  ) {
							display_proportional_rgb(tmpx - 4, (scr_coord_val)(offset.y+baseline-(int)(tmp/scale)-4), cmin, ALIGN_RIGHT, c.color, true );
						}
					}
				}
				last_year=tmp;
			}
		}
		last_year=tmp=0;
	}
}


void gui_chart_t::calc_gui_chart_values(sint64 *baseline, float *scale, char *cmin, char *cmax, int maximum_axis_len) const
{
	sint64 tmp=0;
	double min = 0, max = 0;
	const char* min_suffix = NULL;
	const char* max_suffix = NULL;
	int precision = 0;

	bool convert_kmph = false; // for speed chart. Converts the scale from simspeed to km/h.
	bool convert_n_to_kn = false; // for force chart

	FOR(slist_tpl<curve_t>, const& c, curves) {
		if(  c.show  ) {
			for(  int i=0;  i<c.elements;  i++  ) {
				tmp = c.values[i*c.size+c.offset];
				// convert value where necessary
				if(  c.convert  ) {
					tmp = c.convert(tmp);
				}
				else if(  c.type == MONEY || c.type == PERCENT  ) {
					tmp /= 100;
					precision = 0;
				}
				else if (  c.type == KMPH  ) {
					convert_kmph = true;
					precision = 0;
				}
				else if (  c.type == FORCE  ) {
					convert_n_to_kn = true;
					// precision does not need to be changed. The running resistance is so small (in kN) that we need to display the decimal point.
				}
				if (min > tmp) {
					min = tmp ;
					precision = c.precision;
					min_suffix = c.suffix;
				}
				if (max < tmp) {
					max = tmp;
					precision = c.precision;
					max_suffix = c.suffix;
				}
			}
		}
	}

	// max happend due to rounding errors
	if( precision == 0   &&  min == max ) {
		max += 1;
	}

	// if accel chart => Drawing accuracy hack: simspeed to integer km/h. (Do not rewrite min max for scaling)
	number_to_string_fit(cmin, convert_kmph ? speed_to_kmh((int)min) : convert_n_to_kn ? min/1000.0 : (double)min, precision, maximum_axis_len - (min_suffix != 0));
	number_to_string_fit(cmax, convert_kmph ? speed_to_kmh((int)max) : convert_n_to_kn ? max/1000.0 : (double)max, precision, maximum_axis_len - (max_suffix != 0));

	if(  min_suffix  ) {
		strcat( cmin, min_suffix );
	}
	if(  max_suffix  ) {
		strcat( cmax, max_suffix );
	}

	// scale: factor to calculate money with, to get y-pos offset
	*scale = (double)(max - min) / (size.h-1-4-LINESPACE);
	if(*scale==0.0) {
		*scale = 1.0;
	}

	// baseline: y-pos for the "zero" line in the chart
	*baseline = (sint64)(size.h-1-4-LINESPACE - labs((sint64)(min / *scale )));
}


/**
 * Events werden hiermit an die GUI-components
 * gemeldet
 */
bool gui_chart_t::infowin_event(const event_t *ev)
{
	if(IS_LEFTREPEAT(ev)  ||  IS_LEFTCLICK(ev)) {
		// tooptip to show?
		tooltipcoord = scr_coord(ev->mx,ev->my);
	}
	else {
		tooltipcoord = scr_coord::invalid;
		tooltip[0] = 0;
	}
	return true;
}
