#include <string>
#include <string.h>
#include "../../simcolor.h"
#include "../../simevent.h"
#include "../../simgraph.h"
#include "../../dataobj/translator.h"
#include "../../utils/simstring.h"

#include "gui_flowtext.h"

gui_flowtext_t::gui_flowtext_t()
{
	title[0] = '\0';
	last_offset = koord::invalid;
	dirty = true;
}


void gui_flowtext_t::set_text(const char *text)
{
	if (text == NULL) {
		text = "(null)";
	}
	// purge all old texts
	nodes.clear();
	links.clear();

	// Hajo: danger here, longest word in text
	// must not exceed 511 chars!
	char word[512];
	attributes att = ATT_NONE;

	const unsigned char* tail = (const unsigned char*)text;
	const unsigned char* lead = (const unsigned char*)text;

	// hyperref param
	std::string param;

	while (*tail) {
		if (*lead == '<') {
			bool endtag = false;
			if (lead[1] == '/') {
				endtag = true;
				lead++;
				tail++;
			}

			// parse a tag (not allowed to exeec 511 letters)
			for (int i = 0; *lead != '>' && *lead > 0 && i < 511; i++) {
				lead++;
			}

			strncpy(word, (const char*)tail + 1, lead - tail - 1);
			word[lead - tail - 1] = '\0';
			lead++;

			if (word[0] == 'p' || (word[0] == 'b' && word[1] == 'r')) {
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
				}
				else {
					att = ATT_A_END;
					links.append(hyperlink_t(param));
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
			for(  int i = 0;  *lead != '<'  &&  (*lead > 32  ||  (i==0  &&  *lead==32))  &&  i < 511  &&  *lead != '&'; i++) {
				if(  *lead>128  &&  translator::get_lang()->utf_encoded  ) {
					size_t skip = 0;
					utf16 symbol = utf8_to_utf16( lead, &skip );
					if(  symbol == 0x3000  ) {
						// space ...
						break;
					}
					lead += skip;
					i += skip;
					if(  symbol == 0x3001  ||  symbol == 0x3002  ) {
						att = ATT_NO_SPACE;
						// CJK full stop, komma, space
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
			if(  translator::get_lang()->utf_encoded  ) {
				size_t skip = 0;
				while(  utf8_to_utf16( lead, &skip )==0x3000  ) {
					lead += skip;
					skip = 0;
				}
			}
		}
		tail = lead;
	}
	dirty = true;
}


const char* gui_flowtext_t::get_title() const
{
	return title;
}


koord gui_flowtext_t::get_preferred_size()
{
	return output(koord(0, 0), false);
}

koord gui_flowtext_t::get_text_size()
{
	return output(koord(0, 0), false, false);
}

void gui_flowtext_t::zeichnen(koord offset)
{
	offset += pos;
	if(offset!=last_offset) {
		dirty = true;
		last_offset = offset;
	}
	output(offset, true);
}


koord gui_flowtext_t::output(koord offset, bool doit, bool return_max_width)
{
	const int width = groesse.x;

	slist_tpl<hyperlink_t>::iterator link = links.begin();

	int xpos         = 0;
	int ypos         = 0;
	int color        = COL_BLACK;
	int double_color = COL_BLACK;
	bool double_it   = false;
	bool link_it     = false;	// true, if currently underlining for a link
	int extra_pixel  = 0;		// extra pixel before next line
	int last_link_x  = 0;		// at this position ye need to continue underline drawing
	int max_width    = width;
	int text_width   = width;
	const int space_width = proportional_string_width(" ");

	FOR(slist_tpl<node_t>, const& i, nodes) {
		switch (i.att) {
			case ATT_NONE:
			case ATT_NO_SPACE: {
				int nxpos = xpos + proportional_string_width(i.text.c_str());
				if(  i.att ==  ATT_NONE  ) {
					// add trailing space
					nxpos += space_width;
				}

				// too wide, but only single character at left border ...
				if(  nxpos >= width  &&  xpos>LINESPACE  ) {
					if (nxpos - xpos > max_width) {
						// word too long
						max_width = nxpos;
					}
					nxpos -= xpos;
					if(  xpos!=last_link_x  &&  link_it  ) {
						if(  doit  ) {
							// close the link
							display_fillbox_wh_clip( offset.x + last_link_x, ypos + offset.y + LINESPACE-1, xpos-last_link_x, 1, color, false);
						}
						extra_pixel = 1;
					}
					xpos = 0;
					last_link_x = 0;
					ypos += LINESPACE+extra_pixel;
					extra_pixel = 0;
				}
				if (nxpos >= text_width) {
					text_width = nxpos;
				}

				if (doit) {
					if (double_it) {
						display_proportional_clip(offset.x + xpos + 1, offset.y + ypos + 1, i.text.c_str(), 0, double_color, false);
						extra_pixel |= 1;
					}
					KOORD_VAL width = display_proportional_clip(offset.x + xpos, offset.y + ypos, i.text.c_str(), 0, color, false);
					if(  link_it  ) {
						display_fillbox_wh_clip( offset.x + last_link_x, ypos + offset.y + LINESPACE-1, (xpos+width)-last_link_x, 1, color, false);
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
						display_fillbox_wh_clip( offset.x + last_link_x, ypos + offset.y + LINESPACE-1, xpos-last_link_x, 1, color, false);
					}
					extra_pixel = 1;
				}
				ypos += LINESPACE+extra_pixel;
				last_link_x = 0;
				extra_pixel = 0;
				break;

			case ATT_A_START:
				color = COL_BLUE;
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
				color = COL_BLACK;
				break;

			case ATT_H1_START:
				color        = COL_WHITE;
				double_color = COL_BLACK;
				double_it    = true;
				break;

			case ATT_H1_END:
				color     = COL_BLACK;
				double_it = false;
				if(doit) {
					display_fillbox_wh_clip(offset.x + 1, offset.y + ypos + LINESPACE, xpos, 1, COL_WHITE, false);
					display_fillbox_wh_clip(offset.x,     offset.y + ypos + LINESPACE-1,     xpos, 1, color, false);
				}
				xpos = 0;
				extra_pixel = 0;
				ypos += LINESPACE+2;
				break;

			case ATT_EM_START:
				color = COL_WHITE;
				break;

			case ATT_EM_END:
				color = COL_BLACK;
				break;

			case ATT_IT_START:
				color        = COL_BLACK;
				double_color = COL_YELLOW;
				double_it    = true;
				break;

			case ATT_IT_END:
				double_it = false;
				break;

			case ATT_STRONG_START:
				if(  !double_it  ) {
					color = COL_RED;
				}
				break;

			case ATT_STRONG_END:
				if(  !double_it  ) {
					color = COL_BLACK;
				}
				break;

			default: break;
		}
	}
	if(dirty) {
		mark_rect_dirty_wc( offset.x, offset.y, offset.x+max_width, offset.y+ypos+LINESPACE );
		dirty = false;
	}
	return koord( return_max_width ? max_width : text_width, ypos + LINESPACE);
}


bool gui_flowtext_t::infowin_event(const event_t* ev)
{
	if (IS_LEFTCLICK(ev)) {
		// scan links for hit
		koord evpos = koord( ev->cx, ev->cy ) - get_pos();
		FOR(slist_tpl<hyperlink_t>, const& link, links) {
			if(  link.tl.y+LINESPACE == link.br.y  ) {
				if(  link.tl.x <= evpos.x  &&  evpos.x < link.br.x  &&  link.tl.y <= evpos.y  &&  evpos.y < link.br.y  ) {
					call_listeners((void const*)link.param.c_str());
					break;
				}
			}
			else {
				//  multi lined box => more difficult
				if(  link.tl.x <= evpos.x  &&  evpos.x < get_groesse().x  &&  link.tl.y <= evpos.y  &&  evpos.y < link.tl.y+LINESPACE  ) {
					// in top line
					call_listeners((void const*)link.param.c_str());
					break;
				}
				else if(  0 <= evpos.x  &&  evpos.x < link.br.x  &&  link.br.y-LINESPACE <= evpos.y  &&  evpos.y < link.br.y  ) {
					// in last line
					call_listeners((void const*)link.param.c_str());
					break;
				}
				else if(  0 <= evpos.x  &&  evpos.x < get_groesse().x  &&  link.tl.y+LINESPACE <= evpos.y  &&  evpos.y < link.br.y-LINESPACE  ) {
					// line in between
					call_listeners((void const*)link.param.c_str());
					break;
				}

			}
		}
	}
	return true;
}
