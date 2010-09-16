#include "line_item.h"
#include "../simmenu.h"
#include "../player/simplay.h"
#include "../utils/cbuffer_t.h"

void line_scrollitem_t::set_text(char *t)
{
	if(  t  &&  t[0]  &&  strcmp(t, line->get_name())) {
		// text changed => call tool
		cbuffer_t buf(300);
		buf.printf( "l%u,%s", line.get_id(), t );
		werkzeug_t *w = create_tool( WKZ_RENAME_TOOL | SIMPLE_TOOL );
		w->set_default_param( buf );
		line->get_besitzer()->get_welt()->set_werkzeug( w, NULL );
		// since init always returns false, it is save to delete immediately
		delete w;
	}
}
