/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_LABEL_H
#define GUI_COMPONENTS_GUI_LABEL_H


#include "gui_component.h"
#include "../../simcolor.h"
#include "../gui_theme.h"
#include "../../simskin.h"
#include "../../utils/cbuffer_t.h"


/**
 * The label component
 * just displays a text, will be auto-translated
 */
class gui_label_t : virtual public gui_component_t
{
public:
	enum align_t {
		left,
		centered,
		right,
		money_right
	};

private:
	bool shadowed;
	PIXVAL color_shadow;

	const char * text; // only for direct access of non-translatable things. Do not use!

	scr_size min_size;

protected:
	/**
	 * Color of the Labels
	 */
	PIXVAL color;

	align_t align;
	const char * tooltip;

	using gui_component_t::init;
	scr_coord_val fixed_width=0;

public:
	gui_label_t(const char* text=NULL, PIXVAL color=SYSCOL_TEXT, align_t align=left);

	void init( const char* text_par, scr_coord pos_par, PIXVAL color_par=SYSCOL_TEXT, align_t align_par=left) {
		set_pos  ( pos_par   );
		set_text ( text_par  );
		set_color( color_par );
		set_align( align_par );
	}

	/**
	 * Sets the text to display, after translating it.
	 */
	void set_text(const char *text, bool autosize=true);

	/**
	 * Sets the text without translation.
	 */
	void set_text_pointer(const char *text, bool autosize=true);

	/**
	 * returns the pointer (i.e. for freeing untranslated contents)
	 */
	const char * get_text_pointer() const { return text; }

	/**
	 * returns the tooltip pointer (i.e. for freeing untranslated contents)
	 */
	const char * get_tooltip_pointer() { return tooltip; }

	/**
	 * Draws the component.
	 */
	virtual void draw(scr_coord offset) OVERRIDE;

	/**
	 * Sets the colour of the label
	 */
	void set_color(PIXVAL colour) { this->color = colour; }
	virtual PIXVAL get_color() const { return color; }

	/**
	 * Toggles shadow and sets shadow color.
	 */
	void set_shadow(PIXVAL color_shadow, bool shadowed)
	{
		this->color_shadow = color_shadow;
		this->shadowed = shadowed;
	}

	/**
	 * Sets the alignment of the label
	 */
	void set_align(align_t align) { this->align = align; }

	/**
	 * Sets the tooltip of this component.
	 */
	void set_tooltip(const char * t);

	void set_min_size(scr_size);

	void set_fixed_width(const scr_coord_val width);

	align_t get_align() const { return align; }

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE;
};

/**
 * Label with own buffer.
 */
class gui_label_buf_t : public gui_label_t
{
	bool buf_changed;
	cbuffer_t buffer_write, buffer_read;

public:
	gui_label_buf_t(PIXVAL color=SYSCOL_TEXT, align_t align=left) : gui_label_t(NULL, color, align), buf_changed(true) { }

	void init(PIXVAL color_par=SYSCOL_TEXT, align_t align_par=left);

	/**
	 * Has to be called after access to buf() is finished.
	 * Otherwise size calculations will be off.
	 * Called by @ref draw.
	 */
	void update();

	cbuffer_t& buf()
	{
		if (!buf_changed) {
			buffer_write.clear();
		}
		buf_changed = true;
		return buffer_write;
	}

	/**
	 * appends money string to write buf, sets color
	 */
	void append_money(double money);

	void draw(scr_coord offset) OVERRIDE;

protected:
	using gui_label_t::get_text_pointer;
	using gui_label_t::set_text;
	using gui_label_t::set_text_pointer;
};


class gui_label_updown_t : public gui_label_t
{
private:
	sint64 border=0;
	sint64 value=0;
	bool show_border_value=true;

public:
	gui_label_updown_t(PIXVAL color = SYSCOL_TEXT, align_t align = left) :
		gui_label_t(NULL, color, align) { }

	void init(const sint64 value, scr_coord pos_par, PIXVAL color_par = SYSCOL_TEXT, align_t align_par = left, const sint64 border = 0, bool show_border_value=true) {
		set_value(value);
		set_pos(pos_par);
		set_color(color_par);
		set_align(align_par);
		set_border(border);
		set_show_border_value(show_border_value);
	}

	void set_value(const sint64 v) { value = v; };
	void set_border(const sint64 b) { border = b; };
	void set_show_border_value(bool yesno) { show_border_value = yesno; };

	void draw(scr_coord offset) OVERRIDE;
};

class gui_data_bar_t : public gui_label_t
{
private:
	sint64 max = 100;
	sint64 value = 0;
	bool show_value = true;
	bool show_percentage = false;
	bool show_digit = true;
	PIXVAL bar_color;

public:
	gui_data_bar_t(PIXVAL color = SYSCOL_TEXT) :
		gui_label_t(NULL, color, align_t::right) { }

	void init(const sint64 value, const sint64 max=100, const scr_coord_val width=D_LABEL_WIDTH, PIXVAL bar_col = COL_SAFETY, bool show_value = true, bool show_percentage = false, bool show_digit=true) {
		set_value(value);
		set_max(max);
		set_bar_color(bar_col);
		set_show_value(show_value);
		set_show_percentage(show_percentage);
		set_show_digit(show_digit);
		set_fixed_width(width);
		set_size(scr_size(width, size.h));
	}

	void set_value(const sint64 v) { value = v; };
	void set_max(const sint64 m) { max = m; };
	void set_show_value(bool yesno) { show_value = yesno; };
	void set_show_percentage(bool yesno) { show_percentage = yesno; };
	void set_show_digit(bool yesno) { show_digit = yesno; };
	void set_bar_color(PIXVAL col_val) { bar_color = col_val; };

	void draw(scr_coord offset) OVERRIDE;

	scr_size get_min_size() const OVERRIDE { return scr_size(fixed_width, size.h);  };
	scr_size get_max_size() const OVERRIDE { return get_min_size(); }
};

class gui_heading_t : public gui_component_t
{
private:
	PIXVAL text_color;
	PIXVAL frame_color;
	uint8 style;

	const char * text;	// only for direct access of non-translatable things. Do not use!
	const char * tooltip;

public:
	gui_heading_t(const char* text = NULL, PIXVAL text_color = SYSCOL_TEXT_HIGHLIGHT, PIXVAL frame_color = SYSCOL_TEXT, uint8 style = 0);

	void init(const char* text_par, scr_coord pos_par, PIXVAL color_par = SYSCOL_TEXT_HIGHLIGHT, PIXVAL frame_color_par = SYSCOL_TEXT, uint8 style_par = 0) {
		set_pos(pos_par);
		set_text(text_par);
		set_text_color(color_par);
		set_frame_color(frame_color_par);
		set_style(style_par);
	}

	void draw(scr_coord offset) OVERRIDE;

	void set_text(const char *text);

	void set_text_color(PIXVAL col) { text_color = col; }
	void set_frame_color(PIXVAL f_col) { frame_color = f_col; }
	void set_style(uint8 s) { style = s; }
	void set_tooltip(const char * t) { tooltip = t; }

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE { return get_min_size(); }
};

#endif
