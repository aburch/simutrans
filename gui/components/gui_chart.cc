/*
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include <cmath>

#include "gui_chart.h"
#include "../../dataobj/umgebung.h"
#include "../../utils/simstring.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../simwin.h"

static char tooltip[64];

/**
 * Set background color. -1 means no background
 * @author Hj. Malthaner
 */
void gui_chart_t::set_background(int i)
{
	background = i;
}


gui_chart_t::gui_chart_t() : gui_komponente_t()
{
	// no toolstips at the start
	tooltipkoord = koord::invalid;

	seed = 0;
	show_x_axis = true;
	show_y_axis = true;
	x_elements = 0;

	// Hajo: transparent by default
	background = -1;
}


int gui_chart_t::add_curve(int color, const sint64 *values, int size, int offset, int elements, int type, bool show, bool show_value, int precision, convert_proc proc)
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
	new_curve.precision = precision;
	new_curve.convert = proc;
	curves.append(new_curve);
	return curves.get_count();
}


uint32 gui_chart_t::add_line(int color, const sint64 *value, int times, bool show, bool show_value, int precision, convert_proc proc)
{
	line_t new_line;
	new_line.color = color;
	new_line.value = value;
	new_line.times = times;
	new_line.show = show;
	new_line.show_value = show_value;
	new_line.precision = precision;
	new_line.convert = proc;
	lines.append(new_line);
	return lines.get_count();
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


void gui_chart_t::show_line(uint32 id)
{
	if(  id<lines.get_count()  ) {
		lines.at(id).show = true;
	}
}


void gui_chart_t::hide_line(uint32 id)
{
	if(  id<lines.get_count()  ) {
		lines.at(id).show = false;
	}
}


void gui_chart_t::zeichnen(koord offset)
{
	offset += pos;

	sint64 last_year=0, tmp=0;
	char cmin[128] = "0", cmax[128] = "0", digit[8];

	sint64 baseline = 0;
	sint64* pbaseline = &baseline;

	float scale = 0;
	float* pscale = &scale;

	// calc baseline and scale
	calc_gui_chart_values(pbaseline, pscale, cmin, cmax);

	// Hajo: draw background if desired
	if(background != -1) {
		display_fillbox_wh_clip(offset.x, offset.y, groesse.x, groesse.y, background, false);
	}
	int tmpx, factor;
	if(umgebung_t::left_to_right_graphs) {
		tmpx = offset.x + groesse.x - groesse.x % (x_elements - 1);
		factor = -1;
	}
	else {
		tmpx = offset.x;
		factor = 1;
	}

	// draw zero line
	display_direct_line(offset.x+1, offset.y+(KOORD_VAL)baseline, offset.x+groesse.x-2, offset.y+(KOORD_VAL)baseline, MN_GREY4);

	if (show_y_axis) {

		// draw zero number only, if it will not disturb any other printed values!
		if ((baseline > 18) && (baseline < groesse.y -18)) {
			display_proportional_clip(offset.x - 4, offset.y+(KOORD_VAL)baseline-3, "0", ALIGN_RIGHT, COL_WHITE, true );
		}

		// display min/max money values
		display_proportional_clip(offset.x - 4, offset.y-5, cmax, ALIGN_RIGHT, COL_WHITE, true );
		display_proportional_clip(offset.x - 4, offset.y+groesse.y-5, cmin, ALIGN_RIGHT, COL_WHITE, true );
	}

	// draw chart frame
	display_ddd_box_clip(offset.x, offset.y, groesse.x, groesse.y, COL_GREY1, COL_WHITE);

	// draw chart lines
	KOORD_VAL x_last = 0;  // remember last digit position to avoid overwriting by next label
	for(  int i = 0;  i < x_elements;  i++  ) {
		const int j = umgebung_t::left_to_right_graphs ? x_elements - 1 - i : i;
		const KOORD_VAL x0 = tmpx + factor * (groesse.x / (x_elements - 1) ) * j;
		if(  show_x_axis  ) {
			// display x-axis
			sprintf( digit, "%i", abs(seed - j) );
			KOORD_VAL x =  x0 - (seed != j ? (int)(2 * log( (double)abs(seed - j) )) : 0);
			if(  x > x_last  ) {
				x_last = x + display_proportional_clip( x, offset.y + groesse.y + 6, digit, ALIGN_LEFT, MN_GREY4, true );
			}
		}
		// year's vertical lines
		display_vline_wh_clip( x0, offset.y + 1, groesse.y - 2, MN_GREY4, false );
	}

	// display current value?
	int tooltip_n=-1;
	if(tooltipkoord!=koord::invalid) {
		if(umgebung_t::left_to_right_graphs) {
			tooltip_n = x_elements-1-(tooltipkoord.x*x_elements+4)/(groesse.x|1);
		}
		else {
			tooltip_n = (tooltipkoord.x*x_elements+4)/(groesse.x|1);
		}
	}

	// draw chart's curves
	FOR(slist_tpl<curve_t>, const& c, curves) {
		if (c.show) {
			// for each curve iterate through all elements and display curve
			for (int i=0;i<c.elements;i++) {
				//tmp=c.values[year*c.size+c.offset];
				tmp = c.values[i*c.size+c.offset];
				// Knightly : convert value where necessary
				if(  c.convert  ) {
					tmp = c.convert(tmp);
				}
				else if(  c.type!=0  ) {
					tmp /= 100;
				}
				// display marker(box) for financial value
				display_fillbox_wh_clip(tmpx+factor*(groesse.x / (x_elements - 1))*i-2, (KOORD_VAL)(offset.y+baseline- (int)(tmp/scale)-2), 5, 5, c.color, true);

				// display tooltip?
				if(i==tooltip_n  &&  abs((int)(baseline-(int)(tmp/scale)-tooltipkoord.y))<10) {
					number_to_string(tooltip, (double)tmp, c.precision);
					win_set_tooltip( get_maus_x()+8, get_maus_y()-12, tooltip );
				}

				// draw line between two financial markers; this is only possible from the second value on
				if (i>0) {
					display_direct_line(tmpx+factor*(groesse.x / (x_elements - 1))*(i-1),
						(KOORD_VAL)( offset.y+baseline-(int)(last_year/scale) ),
						tmpx+factor*(groesse.x / (x_elements - 1))*(i),
						(KOORD_VAL)( offset.y+baseline-(int)(tmp/scale) ),
						c.color);
				}
				else {
					// for the first element print the current value (optionally)
					// only print value if not too narrow to min/max/zero
					if(  c.show_value  ) {
						if(  umgebung_t::left_to_right_graphs  ) {
							number_to_string(cmin, (double)tmp, c.precision);
							const sint16 width = proportional_string_width(cmin)+7;
							display_ddd_proportional( tmpx + 8, (KOORD_VAL)(offset.y+baseline-(int)(tmp/scale)-4), width, 0, COL_GREY4, c.color, cmin, true);
						}
						else if(  (baseline-tmp/scale-8) > 0  &&  (baseline-tmp/scale+8) < groesse.y  &&  abs((int)(tmp/scale)) > 9  ) {
							number_to_string(cmin, (double)tmp, c.precision);
							display_proportional_clip(tmpx - 4, (KOORD_VAL)(offset.y+baseline-(int)(tmp/scale)-4), cmin, ALIGN_RIGHT, c.color, true );
						}
					}
				}
				last_year=tmp;
			}
		}
		last_year=tmp=0;
	}

	// draw chart's lines
	FOR(slist_tpl<line_t>, const& line, lines) {
		if(  line.show  ) {
			tmp = ( line.convert ? line.convert(*(line.value)) : *(line.value) );
			for(  int t=0;  t<line.times;  ++t  ) {
				// display marker(box) for financial value
				display_fillbox_wh_clip(tmpx+factor*(groesse.x / (x_elements - 1))*t-2, (KOORD_VAL)(offset.y+baseline- (int)(tmp/scale)-2), 5, 5, line.color, true);

				// display tooltip?
				if(  t==tooltip_n  &&  abs((int)(baseline-(int)(tmp/scale)-tooltipkoord.y))<10  ) {
					number_to_string(tooltip, (double)tmp, line.precision);
					win_set_tooltip( get_maus_x()+8, get_maus_y()-12, tooltip );
				}
				// for the first element print the current value (optionally)
				// only print value if not too close to min/max/zero
				if(  t==0  &&  line.show_value  ) {
					if(  umgebung_t::left_to_right_graphs  ) {
						number_to_string(cmin, (double)tmp, line.precision);
						const sint16 width = proportional_string_width(cmin)+7;
						display_ddd_proportional( tmpx + 8, (KOORD_VAL)(offset.y+baseline-(int)(tmp/scale)-4), width, 0, COL_GREY4, line.color, cmin, true);
					}
					else if(  (baseline-tmp/scale-8) > 0  &&  (baseline-tmp/scale+8) < groesse.y  &&  abs((int)(tmp/scale)) > 9  ) {
						number_to_string(cmin, (double)tmp, line.precision);
						display_proportional_clip(tmpx - 4, (KOORD_VAL)(offset.y+baseline-(int)(tmp/scale)-4), cmin, ALIGN_RIGHT, line.color, true );
					}
				}
			}
			// display horizontal line that passes through all markers
			const int y_offset = (int)( offset.y + baseline - (sint64)(tmp/scale) );
			display_fillbox_wh(tmpx, y_offset, factor*(groesse.x / (x_elements - 1))*(line.times-1), 1, line.color, true);
		}
	}
}


void gui_chart_t::calc_gui_chart_values(sint64 *baseline, float *scale, char *cmin, char *cmax) const
{
	sint64 tmp=0;
	sint64 min = 0, max = 0;
	int precision = 0;

	// first, check curves
	FOR(slist_tpl<curve_t>, const& c, curves) {
		if(  c.show  ) {
			for(  int i=0;  i<c.elements;  i++  ) {
				tmp = c.values[i*c.size+c.offset];
				// Knightly : convert value where necessary
				if(  c.convert  ) {
					tmp = c.convert(tmp);
				}
				else if(  c.type!=0  ) {
					tmp /= 100;
				}
				if (min > tmp) {
					min = tmp ;
					precision = c.precision;
				}
				if (max < tmp) {
					max = tmp;
					precision = c.precision;
				}
			}
		}
	}

	// second, check lines
	FOR(slist_tpl<line_t>, const& line, lines) {
		if(  line.show  ) {
			tmp = ( line.convert ? line.convert(*(line.value)) : *(line.value) );
			if(  min>tmp  ) {
				min = tmp;
				precision = line.precision;
			}
			if(  max<tmp  ) {
				max = tmp;
				precision = line.precision;
			}
		}
	}

	number_to_string(cmin, (double)min, precision);
	number_to_string(cmax, (double)max, precision);

	// scale: factor to calculate money with, to get y-pos offset
	*scale = (float)(max - min) / (groesse.y-2);
	if(*scale==0.0) {
		*scale = 1.0;
	}

	// baseline: y-pos for the "zero" line in the chart
	*baseline = (sint64)(groesse.y - abs((int)(min / *scale )));
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool gui_chart_t::infowin_event(const event_t *ev)
{
	if(IS_LEFTREPEAT(ev)  ||  IS_LEFTCLICK(ev)) {
		// tooptip to show?
		tooltipkoord = koord(ev->mx,ev->my);
	}
	else {
		tooltipkoord = koord::invalid;
		tooltip[0] = 0;
	}
	return true;
}
