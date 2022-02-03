/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_aligned_container.h"
#include "../gui_theme.h"
#include "../../simdebug.h"
#include "../../display/simgraph.h"


gui_empty_t gui_aligned_container_t::placeholder;


gui_aligned_container_t::gui_aligned_container_t(uint16 columns_, uint16 rows_) : columns(columns_), rows(rows_)
{
	set_spacing_from_theme();
	set_margin_from_theme();
	alignment = ALIGN_LEFT  |  ALIGN_CENTER_V;
	child = NULL;
	force_equal_columns = false;
}


gui_aligned_container_t::~gui_aligned_container_t()
{
	clear_ptr_vector(owned_components);
}


void gui_aligned_container_t::set_table_layout(uint16 columns_, uint16 rows_)
{
	columns = columns_;
	rows = rows_;
}


void gui_aligned_container_t::set_alignment(uint8 a)
{
	alignment = a;
}


void gui_aligned_container_t::set_size(scr_size new_size)
{
	if (rows == 0  &&  columns == 0) {
		gui_container_t::set_size(size);
		return;
	}
//	printf("[%p] new size %d,%d\n", this, new_size.w, new_size.h);
	// calculate min-sizes
	vector_tpl<scr_coord_val> col_minw, row_minh;

	compute_sizes(col_minw, row_minh, false);

	scr_size min_size = get_size(col_minw, row_minh);

//	printf("colmin (spacing %d) ", spacing.w);
//	for(uint32 i=0; i<col_minw.get_count(); i++) {
//		printf("%d, ", col_minw[i]);
//	}
//	printf("\n");

#ifdef DEBUG
	if (new_size.w < min_size.w  ||  new_size.h < min_size.h) {
		dbg->warning("gui_aligned_container_t::set_size", "new size (%d,%d) cannot fit all elements; at least (%d,%d) required", new_size.w, new_size.h, min_size.w, min_size.h);
	}
#endif

	scr_size extra = new_size - min_size;
	extra.clip_lefttop(scr_coord(0,0));

	// check which components can have size larger than min size,
	// enlarge these components
	if (extra.w > 0   ||  extra.h > 0) {

		vector_tpl<scr_coord_val> col_maxw, row_maxh;
		compute_sizes(col_maxw, row_maxh, true);

		distribute_extra_space(col_minw, col_maxw, extra.w);
		distribute_extra_space(row_minh, row_maxh, extra.h);
	}
	// horizontal alignment
	scr_coord_val left = margin_tl.w;
	switch(alignment & ALIGN_RIGHT) {
		case ALIGN_CENTER_H:
			left += extra.w/2;
			break;
		case ALIGN_RIGHT:
			left += extra.w;
		default: ;
	}
	// vertical alignment
	scr_coord_val top = margin_tl.h;
	switch(alignment & ALIGN_BOTTOM) {
		case ALIGN_BOTTOM:
			top += extra.h;
		default: ;
	}

	scr_coord c_pos(left, top);

	uint16 c=0, r=0;
	for(uint32 i = 0; i<components.get_count()  &&  !col_minw.empty(); i++) {
		gui_component_t* comp = components[i];

		const uint16 rr = rows    ? r % rows    : r;
		if (rr >= row_minh.get_count()) {
			// no more visible elements coming
			break;
		}

		scr_size cell_size(0,0);
		const uint16 cc = c;

		// compute cell size taking
		// cells spanning more than one column into account
		do {
			// maybe no visible cells in this row anymore
			if (c < col_minw.get_count()) {
				cell_size.w += col_minw[c];
			}
			cell_size.h = max(cell_size.h, row_minh[rr] );

			if (i+1 >= components.get_count()  ||  components[i+1] != &placeholder) {
				break;
			}
			i++;
			if (!comp->is_visible()  &&  !comp->is_rigid()) {
				// advance element counter but not c counter
				continue;
			}
			c++;
			// no spanning beyond end of row
			if (columns  &&  (c % columns) == 0) {
				break;
			}
			cell_size.w += spacing.w;
		} while (1==1);

		if (!comp->is_visible()  &&  !comp->is_rigid()) {
			continue;
		}

		scr_size comp_size = cell_size;
		comp_size.clip_rightbottom(comp->get_max_size());

		scr_coord delta(0,0);
		if (cell_size.w > comp_size.w ) {
			// horizontal alignment
			switch(alignment & ALIGN_RIGHT) {
				case ALIGN_RIGHT:
					delta.x += cell_size.w - comp_size.w;
					break;
				case ALIGN_CENTER_H:
					delta.x += (cell_size.w - comp_size.w) / 2;
					break;
				case ALIGN_LEFT:
				default: ;
			}
		}
		if (cell_size.h > comp_size.h ) {
			// vertical alignment
			switch(alignment & ALIGN_BOTTOM) {
				case ALIGN_BOTTOM:
					delta.y += cell_size.h - comp_size.h;
					break;
				case ALIGN_CENTER_V:
					delta.y += (cell_size.h - comp_size.h) / 2;
					break;
				case ALIGN_TOP:
				default: ;
			}
		}


//		scr_size max_size = comp->get_max_size();
//		printf("[%p] set [%p] to pos %d %d, size %d %d, min %d %d, max %d %d\n", this, comp, comp->get_pos().x, comp->get_pos().y, comp_size.w, comp_size.h, comp->get_min_size().w, comp->get_min_size().h,
//			 max_size.w, max_size.h
//		);

		// marginless component - extend to full width/height
		if( comp->is_marginless() ) {
			if (cc == 0) {
				// first element in columns
				delta.x      = -c_pos.x;
				comp_size.w +=  c_pos.x;
			}
			if (columns  &&  ((c+1) % columns) == 0) {
				// last element in column
				comp_size.w = new_size.w - (c_pos.x + delta.x);
			}
			if (rr == 0) {
				// element in first row
				delta.y      = -c_pos.y;
				comp_size.h +=  c_pos.y;
			}
			if (rr == row_minh.get_count()-1) {
				// element in last row
				comp_size.h = new_size.h - (c_pos.y + delta.y);
			}
		}

		comp->set_pos(c_pos + delta);
		comp->set_size(comp_size);

		c++;
		if (columns  &&  (c % columns) == 0) {
			c_pos.x  = left;
			c_pos.y += cell_size.h + spacing.h;
			r ++;
			c = 0;
		}
		else {
			c_pos.x += cell_size.w + spacing.w;
		}
	}
	gui_container_t::set_size(new_size);
}


void gui_aligned_container_t::compute_sizes(vector_tpl<scr_coord_val>& col_w, vector_tpl<scr_coord_val>& row_h, bool calc_max) const
{
	scr_coord_val sinf = scr_size::inf.w;
	row_h.clear();
	col_w.clear();

	row_h.append(0);

	// first iteration: all elements in one cell_size
	// second: all elements spanning more than one cell
	for(int outer = 0; outer < 2; outer++) {
		uint16 c = 0, r = 0;
		bool wide_elements = false;
		for(uint32 i = 0; i<components.get_count(); i++) {
			gui_component_t* comp = components[i];
			// fails if span is larger than remaining column space
			assert(comp != &placeholder);
			uint16 span = 1;

			while (i+1<components.get_count()) {
				if (components[i+1] != &placeholder) {
					break;
				}
				span++;
				i++;
				if (!comp->is_visible()   &&  !comp->is_rigid()) {
					// advance element counter but not c counter
					continue;
				}

				c++;
				// no spanning beyond end of row
				if (columns  &&  ((c+1) % columns) == 0) {
					break;
				}
			}

			if (!comp->is_visible()   &&  !comp->is_rigid()) {
				continue; // not visible - no size reserved
			}

			// only compute if necessary
			scr_size s = outer == 0 ||  span>1 ? (calc_max ? comp->get_max_size() : comp->get_min_size()) : scr_size(0,0);

			if (outer == 0) {

				uint16 rr = rows ? r % rows : r;
				if (rr >= row_h.get_count()) {
					row_h.append(0);
				}

				if (span == 1) {
					// elements in exactly one cell, define initial size
//					printf(" -- [%p] %p (%d,%d) %d %d \n", this, comp, c,r, s.w, s.h);
					if (c < columns  ||  columns==0) {
						col_w.append(s.w);
					}
					else if (col_w[c % columns] < s.w) {
						col_w[c % columns] = s.w;
					}
					if (row_h[rr] < s.h) {
						row_h[rr] = s.h;
					}
				}
				else {
					wide_elements = true;
					while (col_w.get_count() <= c % columns) {
						col_w.append(0);
					}
				}
			}
			else if (outer == 1 &&  span > 1) {
				// elements spanning more than one cell
				uint16 c0 = c+1 - span;
				// find place taken by those columns
				scr_coord_val w = col_w[c0 % columns];
				for(uint16 j=(c0+1) % columns; j<=c % columns; j++) {
					if (w > sinf - col_w[j] - spacing.w) {
						w = sinf;
					}
					else {
						w += col_w[j] + spacing.w;
					}
				}
				if (s.w > w) {
					if (s.w >= sinf) {
						// set all to inf
						for(uint16 j=c0; j<=c; j++) {
							col_w[j % columns] = s.w;
						}
					}
					else {
						// distribute somehow equally
						// we do not want to solve linear programming problems here
						scr_coord_val delta = s.w - w;
						for(uint16 j=c0; j<=c; j++) {
							col_w[j % columns] += delta/span;
							delta    -= delta/span;
							span --;
						}
					}
				}
				// row height is easier
				uint16 rr = rows ? r % rows : r;
				row_h[rr] = max(row_h[rr], s.h);
			}

			c++;
			// possibly increase row counter now
			if (columns  &&  (c % columns) == 0) {
				r++;
			}

		}
		if (!wide_elements) {
			// no elements spanning more than one column
			break;
		}
	}

	if (force_equal_columns) {
		scr_coord_val w = 0;
		for(uint32 i=0; i<col_w.get_count(); i++) {
			w = max(col_w[i], w);
		}
		for(uint32 i=0; i<col_w.get_count(); i++) {
			col_w[i] = w;
		}
	}
}


scr_size gui_aligned_container_t::get_size(const vector_tpl<scr_coord_val>& col_w, const vector_tpl<scr_coord_val>& row_h) const
{
	scr_coord_val sinf = scr_size::inf.w;
	scr_size s = margin_tl + margin_br;
	s.w += spacing.w * max(col_w.get_count()-1, 0);
	s.h += spacing.h * max(row_h.get_count()-1, 0);

	for(scr_coord_val const w : col_w) {
		if (s.w > sinf - w) {
			s.w = sinf;
			break;
		}
		else {
			s.w += w;
		}
	}

	for(scr_coord_val const h : row_h) {
		if (s.h > sinf - h) {
			s.h = sinf;
			break;
		}
		else {
			s.h += h;
		}
	}
	return s;
}


void gui_aligned_container_t::distribute_extra_space(vector_tpl<scr_coord_val>& size, const vector_tpl<scr_coord_val>& maxsize, scr_coord_val& extra)
{
	// distribute such that all enlargeable elements have same size
	while (extra > 0) {
		// find smallest enlargeable elements
		// enlarge them until minimum of their maxsize and the size of the next largest element
		scr_coord_val min_size = scr_size::inf.w, min2_size = min_size;
		sint32 count = 0;
		for(uint32 i=0; i<size.get_count(); i++) {
			if (size[i] < maxsize[i]) {
				if (size[i] < min_size) {
					min2_size = min(min_size, maxsize[i]);
					min_size  = size[i];
					count = 1;
				}
				else if (size[i] == min_size) {
					if (maxsize[i] < min2_size) {
						min2_size = maxsize[i];
					}
					count++;
				}
				else if (size[i] < min2_size) {
					min2_size = size[i];
				}
			}
		}

		if (count == 0)  {
			break;
		}

		// increase size of all elements with size == min_size
		// overflow-safe calculation of min(count * (min2_size - min_size), extra)
		scr_coord_val dist = min2_size - min_size >= extra / count ? extra : count * (min2_size - min_size);
		extra -= dist;
		scr_coord_val rem = dist + count/2;

		for(uint32 i=0; i<size.get_count(); i++) {
			if (size[i] == min_size  &&  size[i] < maxsize[i]) {
				if (count > 1) {
					scr_coord_val step = rem / count;
					size[i] += step;
					rem += dist - step*count;
				}
				else {
					size[i] += dist;
					break;
				}
			}
		}
	}
//	printf("distribute ");
//	for(uint32 i=0; i<size.get_count(); i++) {
//		printf("%d (%d), ", size[i], maxsize[i]);
//	}
//	printf("\n");
}


scr_size gui_aligned_container_t::get_min_size() const
{
	if (rows == 0  &&  columns == 0) {
		return gui_container_t::get_min_size();
	}

	vector_tpl<scr_coord_val> row_h, col_w;

	compute_sizes(col_w, row_h, false);

//	scr_size min_size = get_size(col_w, row_h);
//	printf("min size %d,%d\n", min_size.w, min_size.h);
//	return min_size;
	return get_size(col_w, row_h);
}


scr_size gui_aligned_container_t::get_max_size() const
{
	if (rows == 0  &&  columns == 0) {
		return scr_size(0,0);
	}

	vector_tpl<scr_coord_val> row_h, col_w;

	compute_sizes(col_w, row_h, true);

	scr_size size = get_size(col_w, row_h);

	return size;
}


void gui_aligned_container_t::set_margin_from_theme()
{
	margin_tl.h = D_MARGIN_TOP;
	margin_tl.w = D_MARGIN_LEFT;
	margin_br.h = D_MARGIN_BOTTOM;
	margin_br.w = D_MARGIN_RIGHT;
}


void gui_aligned_container_t::set_spacing_from_theme()
{
	spacing.w = D_H_SPACE;
	spacing.h = D_V_SPACE;
}


gui_aligned_container_t* gui_aligned_container_t::add_table(uint16 columns_, uint16 rows_)
{
	if (child) {
		return child->add_table(columns_, rows_);
	}
	else {
		child = new_component<gui_aligned_container_t>(columns_, rows_);
		child->set_margin(scr_size(0,0), scr_size(0,0));
		return child;
	}
}


/**
 * New components will be appended to parent table of grand-child.
 */
void gui_aligned_container_t::end_table()
{
	assert(child != NULL);
	if (child->child) {
		child->end_table();
	}
	else {
		child = NULL;
	}
}


/**
 * Appends new component to youngest grandchild table
 */
void gui_aligned_container_t::add_component(gui_component_t *comp)
{
	add_component(comp, 1);
}


void gui_aligned_container_t::add_component(gui_component_t *comp, uint span)
{
	if (child) {
		child->add_component(comp, span);
	}
	else {
		gui_container_t::add_component(comp);
		// fill with placeholders
		assert(columns > 0  ||  span == 1);
		for (uint i=1; i<span; i++) {
			gui_container_t::add_component(&placeholder);
		}
	}

	if (gui_aligned_container_t* other = dynamic_cast<gui_aligned_container_t*>(comp)) {
		other->set_margin(scr_size(0,0), scr_size(0,0));
	}
}


void gui_aligned_container_t::take_component(gui_component_t *comp, uint span)
{
	add_component(comp, span);
	owned_components.append(comp);
}


void gui_aligned_container_t::remove_component(gui_component_t *comp)
{
	gui_container_t::remove_component(comp);
	if (owned_components.remove(comp)) {
		delete comp;
	}
}


void gui_aligned_container_t::remove_all()
{
	gui_container_t::remove_all();
	clear_ptr_vector(owned_components);
	child = NULL;
}
