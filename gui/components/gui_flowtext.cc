#include <string.h>
#include <stdio.h>
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../simevent.h"
#include "../ifc/hyperlink_listener.h"
#include "gui_flowtext.h"



gui_flowtext_t::node_t::node_t(const cstring_t &t, int a) : text(t) {
    att = a;
    next = 0;
}



gui_flowtext_t::gui_flowtext_t() {

    // Hajo: initialitze with empty list
    nodes     = 0;
    links     = 0;
    listeners = 0;

    title[0] = '\0';
}


gui_flowtext_t::~gui_flowtext_t() {

    // Hajo: clean up nodes

    while(nodes) {
	node_t * tmp = nodes->next;
	delete nodes;
	nodes = tmp;
    }

    while(links) {
	hyperlink_t * tmp = links->next;
	delete links;
	links = tmp;
    }

    while(listeners) {
	listener_t * tmp = listeners->next;
	delete listeners;
	listeners = tmp;
    }
}


/**
 * Sets the text to display.
 * @author Hj. Malthaner
 */
void gui_flowtext_t::set_text(const char *text) {

    // Hajo: danger here, longest word in text
    // must not exceed 511 chars!
    char word[512];
    int att = 0;

    const unsigned char * tail = (const unsigned char *)text;
    const unsigned char * lead = (const unsigned char *)text;
    node_t *node = nodes;
    hyperlink_t *link = links;

    // hyperref param
    cstring_t param;

    while(*tail) {
	if(*lead == '<') {
	    // parse a tag

	    while(*lead != '>') {
		lead++;
	    }

	    strncpy(word, (const char *)tail+1, lead-tail-1);
	    word[lead-tail-1] = '\0';
	    lead++;

	    if(word[0] == 'p' ||
	       (word[0] == 'b' && word[1] == 'r') ||
	       word[1] == 'p') {
		att = ATT_NEWLINE;
	    }
            else if(word[0] == 'a') {
	      att = ATT_A_START;

	      param = word;

	    }
            else if(word[0] == '/' && word[1] == 'a') {
		att = ATT_A_END;

		hyperlink_t * tmp = new hyperlink_t();
		tmp->param = param.substr(8, param.len()-1);

		// printf("'%s'\n", tmp->param.chars());

		if(links) {
		  // append
		  link = link->next = tmp;
		} else {
		  // insert first
		  link = links = tmp;
		}


	    }
            else if(word[0] == 'h' && word[1] == '1') {
		att = ATT_H1_START;
	    }
            else if(word[1] == 'h' && word[2] == '1') {
		att = ATT_H1_END;
	    }
            else if(word[0] == 'e') {
		att = ATT_EM_START;
	    }
            else if(word[1] == 'e') {
		att = ATT_EM_END;
	    }
            else if(word[0] == 's') {
		att = ATT_STRONG_START;
	    }
            else if(word[1] == 's') {
		att = ATT_STRONG_END;
	    }
            else if(word[0] == 't') {
		// title tag
		const unsigned char * title_start = lead;

		while(*lead != '<') lead++;

		strncpy(title, (const char *)title_start, lead-title_start);
	        title[lead-title_start] = '\0';

		while(*lead != '>') lead++;
		lead ++;

		att = ATT_UNKNOWN;
	    }
	    else {
		att = ATT_UNKNOWN;
	    }

	} else {
	    // parse a word

	    while(*lead > 32 && *lead != '<') {
		lead++;
	    }

	    strncpy(word, (const char *)tail, lead-tail);
	    word[lead-tail] = '\0';
	    att = ATT_NONE;
	}

	// printf("'%s'\n", word);

	node_t * tmp = new node_t(word, att);

	if(node) {
	    // append
	    node->next = tmp;
	} else {
	    // insert first
	    nodes = tmp;
	}

	node = tmp;


	while(*lead <= 32 && *lead > 0) {
	    lead++;
	}

	tail = lead;
    }
}


const char * gui_flowtext_t::get_title() const {
    return title;
}


koord gui_flowtext_t::get_preferred_size() const {
  return output(koord(0,0), false);
}


void gui_flowtext_t::add_listener(hyperlink_listener_t *call)
{
  listener_t * l = new listener_t();

  l->callback = call;
  l->next = listeners;

  listeners = l;
}


/**
 * Paints the component
 * @author Hj. Malthaner
 */
void gui_flowtext_t::zeichnen(koord offset) const {

    offset += pos;

    output(offset, true);
}



koord gui_flowtext_t::output(koord offset, bool doit) const {

    const int width = groesse.x;

    hyperlink_t * link = links;

    node_t *node = nodes;
    int xpos = 0;
    int ypos = 0;
    int color = SCHWARZ;
    int max_width = width;

    while(node) {
	if(node->att == ATT_NONE) {
	    int nxpos = xpos + proportional_string_width(node->text) + 4;

	    if(nxpos >= width) {
		if(nxpos-xpos > max_width) {
		    // word too long
		    max_width = nxpos;
		}
		nxpos -= xpos;
		xpos = 0;
		ypos += LINESPACE;
	    }

	    if(doit) {
		display_proportional_clip(offset.x+xpos, offset.y+ypos, node->text, 0, color, true);
	    }

	    xpos = nxpos;
	}
	else if(node->att == ATT_H1_START) {
	    color = SCHWARZ;
	}
	else if(node->att == ATT_H1_END) {
	    color = SCHWARZ;
	    if(doit) {
		display_fillbox_wh_clip(offset.x, offset.y+12, xpos-4, 1, color, true);
	    }
	    xpos = 0;
	    ypos += LINESPACE;
	}
	else if(node->att == ATT_EM_START) {
	    color = WEISS;
	}
	else if(node->att == ATT_EM_END) {
	    color = SCHWARZ;
	}
	else if(node->att == ATT_STRONG_START) {
	    color = ROT;
	}
	else if(node->att == ATT_STRONG_END) {
	    color = SCHWARZ;
	}
	else if(node->att == ATT_A_START) {
	    color = BLAU;
	    link->tl.x = xpos;
	    link->tl.y = ypos;
	}
	else if(node->att == ATT_A_END) {
	    link->br.x = xpos;
	    link->br.y = ypos+14;

	    if(link->br.x < link->tl.x) {
	      link->tl.x = 0;
	      link->tl.y = link->br.y-14;
	    }

	    // links.insert(link);

	    if(doit) {
		display_fillbox_wh_clip(link->tl.x + offset.x,
					link->tl.y + offset.y + 10,
					link->br.x-link->tl.x-4, 1,
					color, true);
	    }

	    link = link->next;

	    color = SCHWARZ;
	}
	else if(node->att == ATT_NEWLINE) {
	    xpos = 0;
	    ypos += LINESPACE;
	}

	node = node->next;
    }


    // if(!doit) printf("%d\n", max_width);

    return koord(max_width, ypos+LINESPACE);
}



/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void gui_flowtext_t::infowin_event(const event_t *ev)
{
  if(IS_LEFTCLICK(ev)) {

    // printf("Mouse clicked at %d, %d\n", ev->cx, ev->cy);

    // scan links for hit

    hyperlink_t * link = links;

    while(link) {

      // printf("Checking link at %d,%d %d,%d\n", link->tl.x, link->tl.y, link->br.x, link->br.y);


      if(link->tl.x <= ev->cx && link->br.x >= ev->cx &&
	 link->tl.y <= ev->cy && link->br.y >= ev->cy) {

	printf("Link hit '%s'\n", link->param.chars());


	// call listeners

	listener_t * l = listeners;

	while(l) {

	  l->callback->hyperlink_activated(link->param);
	  l = l->next;
	}


      }

      link = link->next;
    }


  }
}
