#include <string.h>
#include "../../simcolor.h"
#include "../../simevent.h"
#include "../../simgraph.h"
#include "gui_flowtext.h"


gui_flowtext_t::gui_flowtext_t()
{
	title[0] = '\0';
	last_offset = koord::invalid;
}


void gui_flowtext_t::set_text(const char *text)
{
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
	cstring_t param;

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
			} else if (word[0] == 'a') {
				if (!endtag) {
					att = ATT_A_START;
					param = word;
				} else {
					att = ATT_A_END;
					links.append(hyperlink_t(param.substr(8, param.len() - 1)));
				}
			} else if (word[0] == 'h' && word[1] == '1') {
				att = endtag ? ATT_H1_END : ATT_H1_START;
			} else if (word[0] == 'i') {
				att = endtag ? ATT_IT_END : ATT_IT_START;
			} else if (word[0] == 'e' && word[1] == 'm') {
				att = endtag ? ATT_EM_END : ATT_EM_START;
			} else if (word[0] == 's' && word[1] == 't') {
				att = endtag ? ATT_STRONG_END : ATT_STRONG_START;
			} else if (!endtag && strcmp(word, "title") == 0) {
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
			} else {
				// ignore all unknown
				att = ATT_UNKNOWN;
			}
			// end of commands
		} else {
			// parse a word (and obey limits)
			for (int i = 0;  *lead != '<' && *lead > 32 && i < 511; i++) {
				lead++;
			}
			strncpy(word, (const char*)tail, lead - tail);
			word[lead - tail] = '\0';
			att = ATT_NONE;
		}

		if (att != ATT_UNKNOWN) { // only add know commands
			nodes.append(node_t(word, att));
		}

		// skip white spaces
		while (*lead <= 32 && *lead > 0) {
			lead++;
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


void gui_flowtext_t::zeichnen(koord offset)
{
	offset += pos;
	if(offset!=last_offset) {
		dirty = true;
		last_offset = offset;
	}
	output(offset, true);
}


koord gui_flowtext_t::output(koord offset, bool doit)
{
	const int width = groesse.x;

	slist_tpl<hyperlink_t>::iterator link = links.begin();

	int xpos         = 0;
	int ypos         = 0;
	int color        = COL_BLACK;
	int double_color = COL_BLACK;
	bool double_it   = false;
	int max_width    = width;

	for (slist_tpl<node_t>::const_iterator node = nodes.begin(), end = nodes.end(); node != end; ++node) {
		switch (node->att) {
			case ATT_NONE: {
				int nxpos = xpos + proportional_string_width(node->text) + 4;

				if (nxpos >= width) {
					if (nxpos - xpos > max_width) {
						// word too long
						max_width = nxpos;
					}
					nxpos -= xpos;
					xpos = 0;
					ypos += LINESPACE;
				}

				if (doit) {
					if (double_it) {
						display_proportional_clip(offset.x + xpos + 1, offset.y + ypos + 1, node->text, 0, double_color, false);
					}
					display_proportional_clip(offset.x + xpos, offset.y + ypos, node->text, 0, color, false);
				}

				xpos = nxpos;
				break;
			}

			case ATT_NEWLINE:
				xpos = 0;
				ypos += LINESPACE;
				break;

			case ATT_A_START:
				color = COL_BLUE;
				link->tl.x = xpos;
				link->tl.y = ypos;
				break;

			case ATT_A_END:
				link->br.x = xpos;
				link->br.y = ypos + 14;

				if (link->br.x < link->tl.x) {
					link->tl.x = 0;
					link->tl.y = link->br.y - 14;
				}

				if (doit) {
					display_fillbox_wh_clip(link->tl.x + offset.x, link->tl.y + offset.y + 10, link->br.x - link->tl.x - 4, 1, color, false);
				}

				++link;
				color = COL_BLACK;
				break;

			case ATT_H1_START:
				color        = COL_BLACK;
				double_color = COL_WHITE;
				double_it    = true;
				break;

			case ATT_H1_END:
				color     = COL_BLACK;
				double_it = false;
				if (doit) {
					display_fillbox_wh_clip(offset.x + 1, offset.y + ypos + 10 + 1, xpos - 4, 1, COL_WHITE, false);
					display_fillbox_wh_clip(offset.x,     offset.y + ypos + 10,     xpos - 4, 1, color,     false);
				}
				xpos = 0;
				ypos += LINESPACE;
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
				color = COL_RED;
				break;

			case ATT_STRONG_END:
				color = COL_BLACK;
				break;

			default: break;
		}
	}
	if(dirty) {
		mark_rect_dirty_wc( offset.x, offset.y, offset.x+max_width, offset.y+ypos+LINESPACE );
		dirty = false;
	}
	return koord(max_width, ypos + LINESPACE);
}


void gui_flowtext_t::infowin_event(const event_t* ev)
{
	if (IS_WHEELUP(ev) || IS_WHEELDOWN(ev)) {
	}

	if (IS_LEFTCLICK(ev)) {
		// scan links for hit
		for (slist_tpl<hyperlink_t>::const_iterator i = links.begin(), end = links.end(); i != end; ++i) {
			if (i->tl.x <= ev->cx && ev->cx <= i->br.x &&
					i->tl.y <= ev->cy && ev->cy <= i->br.y) {
				call_listeners((const void*)i->param);
			}
		}
	}
}
