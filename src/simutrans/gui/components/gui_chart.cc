/*
 * This file is part of the Simutrans project under the Artistic License.
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
	x_elements = 0;
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


uint32 gui_chart_t::add_curve(PIXVAL color, const sint64 *values, int size, int offset, int elements, int type, bool show, bool show_value, int precision, convert_proc proc)
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
		default:      new_curve.suffix = NULL; break;
	}
	new_curve.precision = precision;
	new_curve.convert = proc;
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
	scr_coord chart_offset(130,(LINESPACE/2));
	int maximum_axis_len = 22;

	offset += pos;
	if(  size.w<480  ) {
		chart_offset.x = size.w/6;
		maximum_axis_len = max(7,chart_offset.x/6);
	}

	offset += chart_offset;
	scr_size chart_size = size-chart_offset-scr_size(10,4+LINESPACE);

	sint64 last_year=0, tmp=0;
	char cmin[128] = "0", cmax[128] = "0", digit[11];

	sint64 baseline = 0;
	sint64* pbaseline = &baseline;

	double scale = 0;
	double* pscale = &scale;

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
			sprintf( digit, "%i", abs(seed - j) );
			scr_coord_val x =  x0 - (seed != j ? (int)(2 * log( (double)abs(seed - j) )) : 0);
			if(  x > x_last  ) {
				x_last = x + display_proportional_clip_rgb( x, offset.y + chart_size.h + 4, digit, ALIGN_LEFT, line_color, true );
			}
		}
		// year's vertical lines
		display_vline_wh_clip_rgb( x0, offset.y + 1, chart_size.h - 2, line_color, false );
	}

	// display current value?
	int tooltip_n=-1;
	int ttcx = tooltipcoord.x-chart_offset.x;
	if(  ttcx>0  &&  ttcx<chart_size.w  ) {
		if(env_t::left_to_right_graphs) {
			tooltip_n = x_elements-1-(ttcx*x_elements+4)/(chart_size.w|1);
		}
		else {
			tooltip_n = (ttcx*x_elements+4)/(chart_size.w|1);
		}
	}

	// draw chart's curves
	for(curve_t const& c : curves) {
		if (c.show) {
			double display_tmp;
			// for each curve iterate through all elements and display curve
			for (int i=0;i<c.elements;i++) {

				tmp = c.values[i*c.size+c.offset];
				// convert value where necessary
				if(  c.convert  ) {
					tmp = c.convert(tmp);
					display_tmp = tmp;
				}
				else if(  c.type!=0  ) {
					display_tmp = tmp*0.01;
					tmp /= 100;
				}
				else {
					display_tmp = tmp;
				}

				// display marker(box) for financial value
				display_fillbox_wh_clip_rgb(tmpx+factor*(chart_size.w / (x_elements - 1))*i-2, (scr_coord_val)(offset.y+baseline- (long)(tmp/scale)-2), 5, 5, c.color, true);

				// display tooltip?
				if(i==tooltip_n  &&  abs((int)(baseline-(int)(tmp/scale)-tooltipcoord.y))<10) {
					number_to_string(tooltip, display_tmp, c.precision);
					if (c.suffix) {
						strcat(tooltip, c.suffix);
					}
					win_set_tooltip(get_mouse_x()+TOOLTIP_MOUSE_OFFSET_X, get_mouse_y()-TOOLTIP_MOUSE_OFFSET_Y, tooltip );
				}

				// draw line between two financial markers; this is only possible from the second value on
				if (i>0) {
					display_direct_line_rgb(tmpx+factor*(chart_size.w / (x_elements - 1))*(i-1),
						(scr_coord_val)( offset.y+baseline-(int)(last_year/scale) ),
						tmpx+factor*(chart_size.w / (x_elements - 1))*(i),
						(scr_coord_val)( offset.y+baseline-(int)(tmp/scale) ),
						c.color);
				}
				else {
					// for the first element print the current value (optionally)
					// only print value if not too narrow to min/max/zero
					if(  c.show_value  ) {
						number_to_string_fit(cmin, display_tmp, c.precision, maximum_axis_len - (c.suffix != NULL) );
						if (c.suffix) {
							strcat(cmin, c.suffix);
						}

						if(  env_t::left_to_right_graphs  ) {
							display_ddd_proportional_clip( tmpx + 8, (scr_coord_val)(offset.y+baseline-(int)(tmp/scale)-4), color_idx_to_rgb(COL_GREY4), c.color, cmin, true);
						}
						else if(  (baseline-tmp/scale-8) > 0  &&  (baseline-tmp/scale+8) < chart_size.h  &&  abs((int)(tmp/scale)) > 9  ) {
							display_proportional_clip_rgb(tmpx - 4, (scr_coord_val)(offset.y+baseline-(int)(tmp/scale)-4), cmin, ALIGN_RIGHT, c.color, true );
						}
					}
				}
				last_year=tmp;
			}
		}
		last_year=tmp=0;
	}
}


void gui_chart_t::calc_gui_chart_values(sint64 *baseline, double *scale, char *cmin, char *cmax, int maximum_axis_len) const
{
	sint64 tmp=0;
	double min = 0, max = 0;
	const char* min_suffix = NULL;
	const char* max_suffix = NULL;
	int precision = 0;

	for(curve_t const& c : curves) {
		if(  c.show  ) {
			for(  int i=0;  i<c.elements;  i++  ) {
				tmp = c.values[i*c.size+c.offset];
				// convert value where necessary
				if(  c.convert  ) {
					tmp = c.convert(tmp);
				}
				else if(  c.type!=0  ) {
					tmp /= 100;
					precision = 0;
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

	number_to_string_fit(cmin, (double)min, precision, maximum_axis_len-(min_suffix != 0) );
	number_to_string_fit(cmax, (double)max, precision, maximum_axis_len-(max_suffix != 0) );
	if(  min_suffix  ) {
		strcat( cmin, min_suffix );
	}
	if(  max_suffix  ) {
		strcat( cmax, max_suffix );
	}

	// scale: factor to calculate money with, to get y-pos offset
	*scale = (double)(max - min) / (size.h-2-4-LINESPACE-(LINESPACE/2));
	if(*scale==0.0) {
		*scale = 1.0;
	}

	// baseline: y-pos for the "zero" line in the chart
	*baseline = (sint64)(size.h-2-4-LINESPACE-(LINESPACE/2) - labs((sint64)(min / *scale )));
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
