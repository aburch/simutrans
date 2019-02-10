/*
 * Scrollable list.
 * Displays list, scrollbuttons up/down, dragbar.
 * Has a min and a max size, and can be displayed with any size in between.
 * Does ONLY cater for vertical offset (yet).
 * two possible types:
 * -list.      simply lists some items.
 * -selection. is a list, but additionally, one item can be selected.
 * @author Niels Roest, additions by Hj. Malthaner
 */

#ifndef gui_scrolled_list_h
#define gui_scrolled_list_h

#include "gui_aligned_container.h"
#include "gui_scrollpane.h"
#include "action_listener.h"
#include "gui_action_creator.h"
#include "gui_label.h"
#include "../../simcolor.h"
#include "../../tpl/vector_tpl.h"

/**
 * Helper class to access the list of components in the scrolling container.
 */
class scroll_container_t : public gui_aligned_container_t
{
public:
	vector_tpl <gui_component_t *>& get_components() { return components; }
};

/**
 * Scrollable list of components that can be sorted, and has component selection.
 */
class gui_scrolled_list_t :
	public gui_action_creator_t,
	public gui_scrollpane_t
{
public:
	enum type { windowskin, listskin };


	/**
	 * Base class for elements in lists. Virtual inheritance.
	 */
	class scrollitem_t : virtual public gui_component_t
	{
	public:
		/// constructor: set focusable to true.
		/// focused element will be used to determine selection in gui_scrolled_list_t
		scrollitem_t() : gui_component_t(true /* focusable */), focused(false), selected(false) { }

		virtual char const* get_text() const = 0;
		virtual void set_text(char const *) {}
		virtual bool is_valid() const { return true; }	//  can be used to indicate invalid entries
		virtual bool is_editable()  const { return false; }

		/// compares using get_text
		static bool compare(const gui_component_t *a, const gui_component_t *b );

		bool focused, selected;
	protected:
		using gui_component_t::draw;
	};

	typedef bool (*item_compare_func)(const gui_component_t* a, const gui_component_t* b);

	/**
	 * Text entry, non-editable
	 */
	class const_text_scrollitem_t : public gui_label_t, public scrollitem_t
	{
	public:
		const_text_scrollitem_t(char const* const t, PIXVAL const col) : gui_label_t(NULL, col) { set_text_pointer(t); }

		virtual char const* get_text() const { return get_text_pointer(); }

		scr_size get_min_size() const;
		scr_size get_max_size() const;

		virtual void set_text(char const *) {}

		void draw(scr_coord pos);

		using gui_label_t::get_color;
	private:
		using gui_label_t::set_text;

	};


private:
	enum type type;

	scr_coord_val max_width; // need for overlength entries

	item_compare_func compare;

protected:
	scroll_container_t container;
	vector_tpl <gui_component_t *>& item_list;

	void reset_container_size();

	void set_cmp(item_compare_func cmp) { compare = cmp; }

	/// deletes invalid elements from list
	void cleanup_elements();

public:
	gui_scrolled_list_t(enum type, item_compare_func cmp = 0);

	~gui_scrolled_list_t() { clear_elements(); }

	void show_selection(int s);

	void set_selection(int s);
	sint32 get_selection() const;
	sint32 get_count() const { return item_list.get_count(); }

	/*  when rebuilding a list, be sure to call recalculate the slider
	 *  with recalculate_slider() to update the scrollbar properly. */
	void clear_elements();
	scrollitem_t *get_element(sint32 i) const { return (i>=0  &&  (uint32)i<item_list.get_count()) ? dynamic_cast<scrollitem_t*>(item_list[i]) : NULL; }

	template<class C>
	void new_component() { return container.new_component<C>()->set_focusable(true); }
	template<class C, class A1>
	void new_component(const A1& a1) { return container.new_component<C>(a1)->set_focusable(true); }
	template<class C, class A1, class A2>
	void new_component(const A1& a1, const A2& a2) { container.new_component<C>(a1, a2)->set_focusable(true); }
	template<class C, class A1, class A2, class A3>
	void new_component(const A1& a1, const A2& a2, const A3& a3) { container.new_component<C>(a1, a2, a3)->set_focusable(true); }
	template<class C, class A1, class A2, class A3, class A4>
	void new_component(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { container.new_component<C>(a1, a2, a3, a4)->set_focusable(true); }
	/**
	 * Sorts the list.
	 * Calls the virtual method scrollitem_t::sort of element at position @p offset.
	 * Adjusts scrollbar.
	 * @param offset sort list from element offset to end
	 */
	void sort( int offset);

	void set_size(scr_size size) OVERRIDE;

	bool infowin_event(event_t const*) OVERRIDE;

	void draw(scr_coord pos) OVERRIDE;

	void set_max_width(scr_coord_val mw) { max_width = mw; }
};

#endif
