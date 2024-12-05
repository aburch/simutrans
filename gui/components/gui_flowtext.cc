/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include <string.h>
#include "../../simcolor.h"
#include "../../simevent.h"
#include "../../display/simgraph.h"
#include "../../utils/simstring.h"
#include "../gui_theme.h"

#include "gui_flowtext.h"


/**
 * A component for floating text.
 * Original implementation.
 */
class gui_flowtext_intern_t :
	public gui_action_creator_t,
	public gui_component_t
{
public:
	gui_flowtext_intern_t();

	/**
	 * Sets the text to display.
	 */
	void set_text(const char* text);

	const char* get_title() const;

	/**
	 * Updates size and preferred_size.
	 */
	void set_size(scr_size size_par) OVERRIDE;

	/**
	 * Computes and returns preferred size.
	 * Depends on current width.
	 */
	scr_size get_preferred_size();

	scr_size get_text_size();

	/**
	 * Paints the component
	 */
	void draw(scr_coord offset) OVERRIDE;

	bool infowin_event(event_t const*) OVERRIDE;

	/// min-width zero, to trick gui_scrollpane_t::set_size
	scr_size get_min_size() const OVERRIDE { return scr_size(0, scr_size::inf.h); }

private:
	scr_size output(scr_coord pos, bool doit, bool return_max_width=true);

	scr_size preferred_size; ///< set by set_text

	enum attributes
	{
		ATT_NONE,
		ATT_NO_SPACE, // same as none, but no trailing space
		ATT_NEWLINE,
		ATT_A_START,      ATT_A_END,
		ATT_H1_START,     ATT_H1_END,
		ATT_EM_START,     ATT_EM_END,
		ATT_IT_START,     ATT_IT_END,
		ATT_STRONG_START, ATT_STRONG_END,
		ATT_UNKNOWN
	};

	struct node_t
	{
		node_t(const std::string &text_, attributes att_) : text(text_), att(att_) {}

		std::string text;
		attributes att;
	};

	/**
	 * Hyperlink position container
	 */
	struct hyperlink_t
	{
		hyperlink_t(const std::string &param_) : param(param_) {}

		scr_coord    tl;    // top left display position
		scr_coord    br;    // bottom right display position
		std::string  param;
	};

	slist_tpl<node_t>      nodes;
	slist_tpl<hyperlink_t> links;
	char title[128];

	bool dirty;
	scr_coord last_offset;
};


gui_flowtext_intern_t::gui_flowtext_intern_t()
{
	title[0] = '\0';
	last_offset = scr_coord::invalid;
	dirty = true;
}


void gui_flowtext_intern_t::set_text(const char *text)
{
	if (text == NULL) {
		text = "(null)";
	}
	// purge all old texts
	nodes.clear();
	links.clear();

	// danger here, longest word in text must not exceed stoarge space!
	char word[512];
	attributes att = ATT_NONE;

	const unsigned char* tail = (const unsigned char*)text;
	const unsigned char* lead = (const unsigned char*)text;

	// hyperref param
	std::string param;
	bool link_it = false; // behind opening <a> tag

	while (*tail) {
		if (*lead == '<') {
			bool endtag = false;
			if (lead[1] == '/') {
				endtag = true;
				lead++;
				tail++;
			}

			// parse a tag (not allowed to exceed sizeof(word) letters)
			for (uint i = 0; *lead != '>' && *lead > 0 && i+2 < sizeof(word); i++) {
				lead++;
			}

			strncpy(word, (const char*)tail + 1, lead - tail - 1);
			word[lead - tail - 1] = '\0';
			lead++;

			if (word[0] == 'p' || (word[0] == 'b' && word[1] == 'r')) {
				// unlike http, we can have as many newlines as we like
				att = ATT_NEWLINE;
			}
			else if (word[0] == 'a') {
				if (!endtag) {
					att = ATT_A_START;
					// search for href attributes
					// .. ignore any number of spaces
					// .. accept link string enclosed by " and '
					// skip a and ' '
					char* start = word;
					while(*start == 'a'  ||  *start == ' ') start++;
					start = const_cast<char*>( strstart(start, "href") );
					if (start) {
						// skip ",=, and ' '
						while(*start == '"'  ||  *start == ' '  ||  *start == '='  ||  *start == '\'') start++;
						char *end = start;
						// find first ',", terminate string there
						while(*end  &&  *end != '"'  &&  *end != '\'') end++;
						*end = 0;
						param = start;
					}
					else {
						param = "";
					}
					link_it = true;
				}
				else {
					if (link_it) {
						att = ATT_A_END;
						links.append(hyperlink_t(param));
						link_it = false;
					}
					else {
						// ignore closing </a> without opening <a>
						att = ATT_UNKNOWN;
					}
				}
			}
			else if (word[0] == 'h' && word[1] == '1') {
				att = endtag ? ATT_H1_END : ATT_H1_START;
			}
			else if (word[0] == 'i') {
				att = endtag ? ATT_IT_END : ATT_IT_START;
			}
			else if (word[0] == 'e' && word[1] == 'm') {
				att = endtag ? ATT_EM_END : ATT_EM_START;
			}
			else if (word[0] == 's' && word[1] == 't') {
				att = endtag ? ATT_STRONG_END : ATT_STRONG_START;
			}
			else if (!endtag && strcmp(word, "title") == 0) {
				// title tag
				const unsigned char* title_start = lead;

				// parse title tag (again, enforce 511 limit)
				for (int i = 0; *lead != '<' && *lead > 0 && i < 511; i++) {
					lead++;
				}

				strncpy(title, (const char*)title_start, lead - title_start);
				title[lead - title_start] = '\0';

				// close title tag (again, enforce 511 limit)
				for (int i = 0; *lead != '>' && *lead > 0 && i < 511; i++) {
					lead++;
				}
				if (*lead == '>') {
					lead++;
				}
				att = ATT_UNKNOWN;
			}
			else {
				// ignore all unknown
				att = ATT_UNKNOWN;
			}
			// end of commands
		}
		else if(  lead[0]=='&'  ) {
			if(  lead[2]=='t'  &&  lead[3]==';'  ) {
				// either gt or lt
				strcpy( word, lead[1]=='l' ? "<" : ">" );
				lead += 4;
			}
			else if(  lead[1]=='#'  ) {
				// decimal number
				word[0] = atoi( (const char *)lead+2 );
				word[1] = 0;
				while( *lead++!=';'  ) {
				}
			}
			else {
				// only copy ampersand
				strcpy( word, "&" );
				lead ++;
			}
			att = *lead<=32 ? ATT_NONE : ATT_NO_SPACE;
		}
		else {

			// parse a word (and obey limits)
			att = ATT_NONE;
			for(  uint i = 0;  *lead != '<'  &&  (*lead > 32  ||  (i==0  &&  *lead==32))  &&  i+1 < sizeof(word)  &&  *lead != '&'; i++) {
				if(  *lead>128  ) {
					size_t len = 0;
					utf32 symbol = utf8_decoder_t::decode(lead, len);
					if(  symbol == 0x3000  ) {
						// space ...
						break;
					}
					lead += len;
					i += len;
					if(  symbol == 0x3001  ||  symbol == 0x3002  ) {
						att = ATT_NO_SPACE;
						// CJK full stop, comma, space
						break;
					}
					// every CJK symbol could be used to break, so break after 10 characters
					if(  symbol >= 0x2E80  &&  symbol <= 0xFE4F  &&  i>6  ) {
						att = ATT_NO_SPACE;
						break;
					}
				}
				else {
					lead++;
				}
			}
			strncpy(word, (const char*)tail, lead - tail);
			if(  *lead>32  &&  word[0]!=32  ) {
				att = ATT_NO_SPACE;
			}
			word[lead - tail] = '\0';
			if(  *word==0  ) {
				// do not add empty strings
				att = ATT_UNKNOWN;
			}
		}

		if(  att != ATT_UNKNOWN  ) { // only add know commands
			nodes.append(node_t(word, att));
		}

		if(  att==ATT_UNKNOWN  ||  att==ATT_NONE  ||  att==ATT_NO_SPACE  ||  att==ATT_NEWLINE  ) {
			// skip white spaces
			while (*lead <= 32 && *lead > 0) {
				lead++;
			}
			// skip wide spaces
			utf8 const *lead_search = lead;
			while( utf8_decoder_t::decode(lead_search) == 0x3000 ){
				lead = lead_search;
			}
		}
		tail = lead;
	}
	dirty = true;
	// save size
	preferred_size = output(scr_size(0, 0), false, true);
}


const char* gui_flowtext_intern_t::get_title() const
{
	return title;
}

void gui_flowtext_intern_t::set_size(scr_size size_par)
{
	gui_component_t::set_size(size_par);
	// update preferred_size
	preferred_size = output(scr_size(0, 0), false, true);
}

/**
 * preferred size of text:
 *
 * get_preferred_size().w = max(width, maximal word length)
 * get_preferred_size().h = displayed height
 */
scr_size gui_flowtext_intern_t::get_preferred_size()
{
	return preferred_size;
	// cached result of output(scr_size(0, 0), false, true);
}

/**
 * wider than current width
 */
scr_size gui_flowtext_intern_t::get_text_size()
{
	return output(scr_size(0, 0), false, false);
}

void gui_flowtext_intern_t::draw(scr_coord offset)
{
	offset += pos;
	if(offset!=last_offset) {
		dirty = true;
		last_offset = offset;
	}
	output(offset, true);
}


scr_size gui_flowtext_intern_t::output(scr_coord offset, bool doit, bool return_max_width)
{
	const int width = size.w-D_MARGIN_LEFT-D_MARGIN_RIGHT;

	slist_tpl<hyperlink_t>::iterator link = links.begin();

	int xpos            = 0;
	int ypos            = 0;
	PIXVAL color        = SYSCOL_TEXT;
	PIXVAL double_color = SYSCOL_TEXT_SHADOW;
	bool double_it      = false;
	bool link_it        = false; // true, if currently underlining for a link
	int extra_pixel     = 0;     // extra pixel before next line
	int last_link_x     = 0;     // at this position ye need to continue underline drawing
	int max_width    = width;
	int text_width   = width;
	const int space_width = proportional_string_width(" ");

	FOR(slist_tpl<node_t>, const& i, nodes) {
		switch (i.att) {
			case ATT_NONE:
			case ATT_NO_SPACE: {
				int nxpos = xpos + proportional_string_width(i.text.c_str());

				if (nxpos >= text_width) {
					text_width = nxpos;
				}
				// too wide
				if(  nxpos >= width  ) {
					if (nxpos - xpos > max_width) {
						// word too long
						max_width = nxpos-xpos;
					}
					nxpos -= xpos; // now word length, new xpos after linebreak

					if (xpos > 0) {
						if(  xpos!=last_link_x  &&  link_it  ) {
							if(  doit  ) {
								// close the link
								display_fillbox_wh_clip_rgb( offset.x + last_link_x + D_MARGIN_LEFT, ypos + offset.y + LINESPACE-1, xpos-last_link_x, 1, color, false);
							}
							extra_pixel = 1;
						}
						xpos = 0;
						last_link_x = 0;
						ypos += LINESPACE+extra_pixel;
						extra_pixel = 0;
					}
				}
				if(  i.att ==  ATT_NONE  ) {
					// add trailing space
					nxpos += space_width;
				}

				if (doit) {
					if (double_it) {
						display_proportional_clip_rgb(offset.x + xpos + 1 + D_MARGIN_LEFT, offset.y + ypos + 1, i.text.c_str(), 0, double_color, false);
						extra_pixel |= 1;
					}
					scr_coord_val width = display_proportional_clip_rgb(offset.x + xpos + D_MARGIN_LEFT, offset.y + ypos, i.text.c_str(), 0, color, false);
					if(  link_it  ) {
						display_fillbox_wh_clip_rgb( offset.x + last_link_x + D_MARGIN_LEFT, ypos + offset.y + LINESPACE-1, (xpos+width)-last_link_x, 1, color, false);
						last_link_x = xpos+width;
					}
				}
				if(  link_it  ) {
					extra_pixel = 1;
				}
				xpos = nxpos;
				break;
			}

			case ATT_NEWLINE:
				xpos = 0;
				if(  last_link_x<xpos  &&  link_it  ) {
					if(  doit  ) {
						// close the link
						display_fillbox_wh_clip_rgb( offset.x + last_link_x + D_MARGIN_LEFT, ypos + offset.y + LINESPACE-1, xpos-last_link_x, 1, color, false);
					}
					extra_pixel = 1;
				}
				ypos += LINESPACE+extra_pixel;
				last_link_x = 0;
				extra_pixel = 0;
				break;

			case ATT_A_START:
				color = SYSCOL_TEXT_TITLE;
				// link == links.end() if there is an endtag </a> is missing
				if (link!=links.end()) {
					link->tl.x = xpos;
					link->tl.y = ypos;
					last_link_x = xpos;
					link_it = true;
				}
				break;

			case ATT_A_END:
				link->br.x = xpos;
				link->br.y = ypos + LINESPACE;
				++link;
				link_it = false;
				color = SYSCOL_TEXT;
				break;

			case ATT_H1_START:
				color        = SYSCOL_TEXT_TITLE;
				double_it    = true;
				break;

			case ATT_H1_END:
				double_it = false;
				if(doit) {
					display_fillbox_wh_clip_rgb(offset.x + 1 + D_MARGIN_LEFT, offset.y + ypos + LINESPACE,   xpos, 1, color,        false);
					display_fillbox_wh_clip_rgb(offset.x + D_MARGIN_LEFT,     offset.y + ypos + LINESPACE-1, xpos, 1, double_color, false);
				}
				xpos = 0;
				extra_pixel = 0;
				ypos += LINESPACE+2;
				color = SYSCOL_TEXT;
				break;

			case ATT_EM_START:
				color = SYSCOL_TEXT_HIGHLIGHT;
				break;

			case ATT_EM_END:
				color = SYSCOL_TEXT;
				break;

			case ATT_IT_START:
				color     = SYSCOL_TEXT_HIGHLIGHT;
				double_it = true;
				break;

			case ATT_IT_END:
				color     = SYSCOL_TEXT;
				double_it = false;
				break;

			case ATT_STRONG_START:
				if(  !double_it  ) {
					color = SYSCOL_TEXT_STRONG;
				}
				break;

			case ATT_STRONG_END:
				if(  !double_it  ) {
					color = SYSCOL_TEXT;
				}
				break;

			default: break;
		}
	}
	ypos += LINESPACE;
	if(dirty) {
		mark_rect_dirty_wc( offset.x + D_MARGIN_LEFT, offset.y, offset.x+max_width + D_MARGIN_LEFT, offset.y+ypos );
		dirty = false;
	}
	return scr_size( (return_max_width ? max_width : text_width)+D_MARGIN_LEFT+D_MARGIN_RIGHT, ypos);
}


bool gui_flowtext_intern_t::infowin_event(const event_t* ev)
{
	if (IS_LEFTCLICK(ev)) {
		// scan links for hit
		scr_coord evpos = scr_coord( ev->cx, ev->cy ); // - get_pos();
		FOR(slist_tpl<hyperlink_t>, const& link, links) {
			if(  link.tl.y+LINESPACE == link.br.y  ) {
				if(  link.tl.x <= evpos.x  &&  evpos.x < link.br.x  &&  link.tl.y <= evpos.y  &&  evpos.y < link.br.y  ) {
					call_listeners((void const*)link.param.c_str());
					return true;
				}
			}
			else {
				//  multi lined box => more difficult
				if(  link.tl.x <= evpos.x  &&  evpos.x < get_size().w  &&  link.tl.y <= evpos.y  &&  evpos.y < link.tl.y+LINESPACE  ) {
					// in top line
					call_listeners((void const*)link.param.c_str());
					return true;
				}
				else if(  0 <= evpos.x  &&  evpos.x < link.br.x  &&  link.br.y-LINESPACE <= evpos.y  &&  evpos.y < link.br.y  ) {
					// in last line
					call_listeners((void const*)link.param.c_str());
					return true;
				}
				else if(  0 <= evpos.x  &&  evpos.x < get_size().w  &&  link.tl.y+LINESPACE <= evpos.y  &&  evpos.y < link.br.y-LINESPACE  ) {
					// line in between
					call_listeners((void const*)link.param.c_str());
					return true;
				}

			}
		}
	}
	return false;
}


/** Implementation of the wrapping class **/

gui_flowtext_t::gui_flowtext_t() : gui_scrollpane_t(NULL, true, true)
{
	flowtext = new gui_flowtext_intern_t();
	set_component(flowtext);
	flowtext->add_listener(this);
}

gui_flowtext_t::~gui_flowtext_t()
{
	delete flowtext;
	flowtext = NULL;
}


void gui_flowtext_t::set_text(const char* text)
{
	flowtext->set_text(text);
}


const char* gui_flowtext_t::get_title() const
{
	return flowtext->get_title();
}


void gui_flowtext_t::set_size(scr_size size_par)
{
	gui_scrollpane_t::set_size(size_par);
	// compute and set height
	flowtext->set_size( flowtext->get_preferred_size() );

	// recalc twice to get correct scrollbar visibility
	recalc_sliders(get_size());
	recalc_sliders(get_size());
}


scr_size gui_flowtext_t::get_preferred_size()
{
	return flowtext->get_preferred_size();
}


bool gui_flowtext_t::action_triggered(gui_action_creator_t*, value_t extra)
{
	call_listeners(extra);
	return true;
}
