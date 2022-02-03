/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_ALIGNED_CONTAINER_H
#define GUI_COMPONENTS_GUI_ALIGNED_CONTAINER_H


#include "gui_container.h"
#include "../../tpl/vector_tpl.h"


/**
 * Class that emulates a html-like table.
 */
class gui_aligned_container_t : virtual public gui_container_t
{
private:
	uint16 columns;     ///< column number, if zero then arbitrary many columns possible
	uint16 rows;        ///< row number, if zero then arbitrary many rows possible

	scr_size margin_tl; ///< margin top, left
	scr_size margin_br; ///< margin bottom, right
	scr_size spacing;   ///< space between individual cells

	uint8 alignment;    ///< alignment of cells
	bool force_equal_columns; ///< if true, all columns will be of same size

	gui_aligned_container_t* child;  ///< new components will be appended to child

	vector_tpl<gui_component_t*> owned_components; ///< we take ownership of these pointers
	/**
	 * Computes min/max size of table cells.
	 * Puts min/max height of rows into @p row_h, width into @p col_w.
	 */
	void compute_sizes(vector_tpl<scr_coord_val>& col_w, vector_tpl<scr_coord_val>& row_h, bool calc_max) const;

	/**
	 * Computes total size of table from row heights and column widths.
	 */
	scr_size get_size(const vector_tpl<scr_coord_val>& col_w, const vector_tpl<scr_coord_val>& row_h) const;

	/**
	 * If size is larger than min-size, then distribute
	 * the extra space to the table cells.
	 * Only cells with max_size == inf will be enlarged.
	 */
	static void distribute_extra_space(vector_tpl<scr_coord_val>& size, const vector_tpl<scr_coord_val>& maxsize, scr_coord_val& extra);

	// used to mark that cells span multiple columns
	static gui_empty_t placeholder;

	// no copying, please
	gui_aligned_container_t& operator=(const gui_aligned_container_t&);
	gui_aligned_container_t(const gui_aligned_container_t&);

public:
	/**
	 * Constructor.
	 * If both @p columns_ and @p rows_ are zero than object behaves like gui_container_t.
	 */
	gui_aligned_container_t(uint16 columns_=0, uint16 rows_=0);

	~gui_aligned_container_t();

	void set_table_layout(uint16 columns_=0, uint16 rows_=0);

	/**
	 * Components that are smaller than the size of the containing cell,
	 * will be aligned according to @r alignement.
	 */
	void set_alignment(uint8 a);

	bool is_table() const { return columns !=0  ||  rows != 0; }

	/**
	 * Appends a new table to component vector.
	 * New components will be added there.
	 * @returns pointer to new table
	 */
	gui_aligned_container_t* add_table(uint16 columns_=0, uint16 rows_=0);

	/**
	 * Virturally closes the current table.
	 * New components will be added to its parent table.
	 */
	void end_table();

	/**
	 * Removes element.
	 */
	void remove_component(gui_component_t *comp) OVERRIDE;

	/**
	 * Appends new component to current table @r child.
	 */
	void add_component(gui_component_t *comp) OVERRIDE;

	/**
	 * Appends new component to current table @r child.
	 * Component spans @p span columns.
	 */
	void add_component(gui_component_t *comp, uint span);

	/**
	 * Like add_component, but takes ownership of comp
	 */
	void take_component(gui_component_t *comp, uint span = 1);

	/**
	 * Creates and appends new component, takes ownership of pointer
	 * @returns the pointer to the new component
	 */
	template<class C, class... As>
	C* new_component(const As &... as)
	{
		C* comp = new C(as...);
		take_component(comp);
		return comp;
	}
	/**
	 * Creates and appends new component, takes ownership of pointer
	 * Component spans @p span columns.
	 * @returns the pointer to the new component
	 */
	template<class C>
	C* new_component_span(uint span) { C* comp = new C(); take_component(comp, span); return comp; }
	template<class C, class A1>
	C* new_component_span(const A1& a1, uint span) { C* comp = new C(a1); take_component(comp, span); return comp; }
	template<class C, class A1, class A2>
	C* new_component_span(const A1& a1, const A2& a2, uint span) { C* comp = new C(a1, a2); take_component(comp, span); return comp; }
	template<class C, class A1, class A2, class A3>
	C* new_component_span(const A1& a1, const A2& a2, const A3& a3, uint span) { C* comp = new C(a1, a2, a3); take_component(comp, span); return comp; }
	template<class C, class A1, class A2, class A3, class A4>
	C* new_component_span(const A1& a1, const A2& a2, const A3& a3, const A4& a4, uint span) { C* comp = new C(a1, a2, a3, a4); take_component(comp, span); return comp; }

	/**
	 * Removes all components in the Container.
	 * Deletes owned components.
	 */
	void remove_all() OVERRIDE;

	/**
	 * Sets size of the table. Sets size of all its components.
	 */
	void set_size(scr_size size) OVERRIDE;

	using gui_container_t::get_size;

	scr_size get_max_size() const OVERRIDE;

	scr_size get_min_size() const OVERRIDE;

	/**
	 * Sets outer margin.
	 */
	void set_margin(scr_size tl, scr_size br) { margin_tl = tl;  margin_br = br; }

	/**
	 * Sets spacing between child components.
	 */
	void set_spacing(scr_size spacing_) { spacing = spacing_; }

	/**
	 * Initializes margin from theme.
	 */
	void set_margin_from_theme();

	/**
	 * Initializes spacing from theme.
	 */
	void set_spacing_from_theme();

	void set_force_equal_columns(bool yesno) { force_equal_columns = yesno; }
};

#endif
