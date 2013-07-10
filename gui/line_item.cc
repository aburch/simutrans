#include "line_item.h"
#include "../simline.h"
#include "../simmenu.h"
#include "../player/simplay.h"
#include "../utils/cbuffer_t.h"


const char* line_scrollitem_t::get_text() const
{
	return line->get_name();
}


COLOR_VAL line_scrollitem_t::get_color()
{
	return line->get_state_color();
}


void line_scrollitem_t::set_text(char const* const t)
{
	if(  t  &&  t[0]  &&  strcmp(t, line->get_name())) {
		// text changed => call tool
		cbuffer_t buf;
		buf.printf( "l%u,%s", line.get_id(), t );
		werkzeug_t *w = create_tool( WKZ_RENAME_TOOL | SIMPLE_TOOL );
		w->set_default_param( buf );
		line->get_besitzer()->get_welt()->set_werkzeug( w, line->get_besitzer() );
		// since init always returns false, it is safe to delete immediately
		delete w;
	}
}
