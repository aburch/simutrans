/*
 * just displays a text, will be auto-translated
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_gui_label_h
#define gui_gui_label_h

#include "gui_komponente.h"
#include "../../simcolor.h"
#include "../gui_theme.h"
#include "../../simskin.h"
#include "../../utils/cbuffer_t.h"


/**
 * The label component
 *
 * @author Hj. Malthaner
 * @date 04-Mar-01
 *
 * Added Alignment support
 * @author: Volker Meyer
 * @date 25.05.03
 */
class gui_label_t : virtual public gui_component_t
{
public:
	enum align_t {
		left,
		centered,
		right,
		money_right,
	};

private:
	align_t align;

	/**
	 * Color of the Labels
	 * @author Hansjörg Malthaner
	 */
	PIXVAL color;

	bool shadowed;
	PIXVAL color_shadow;

	const char * text;	// only for direct access of non-translatable things. Do not use!
	const char * tooltip;

protected:
	using gui_component_t::init;

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
	 * @author Hansjörg Malthaner
	 */
	void set_text(const char *text, bool autosize=true);

	/**
	 * Sets the text without translation.
	 * @author Hansjörg Malthaner
	 */
	void set_text_pointer(const char *text, bool autosize=true);

	/**
	 * returns the pointer (i.e. for freeing untranslated contents)
	 * @author Hansjörg Malthaner
	 */
	const char * get_text_pointer() const { return text; }

	/**
	 * returns the tooltip pointer (i.e. for freeing untranslated contents)
	 */
	const char * get_tooltip_pointer() { return tooltip; }

	/**
	 * Draws the component.
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord offset) OVERRIDE;

	/**
	 * Sets the colour of the label
	 * @author Owen Rudge
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
	 * @author Volker Meyer
	 */
	void set_align(align_t align) { this->align = align; }

	/**
	 * Sets the tooltip of this component.
	 * @author Hj. Malthaner
	 */
	void set_tooltip(const char * t);

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

	void draw(scr_coord offset);

protected:
	using gui_label_t::get_text_pointer;
	using gui_label_t::set_text;
	using gui_label_t::set_text_pointer;
};

#endif
