/*
 * Convoi information, name and status color
 */

#include "convoy_item.h"
#include "../simconvoi.h"
#include "../simmenu.h"
#include "../utils/cbuffer_t.h"


const char* convoy_scrollitem_t::get_text() const
{
	return cnv->get_name();
}


COLOR_VAL convoy_scrollitem_t::get_color()
{
	return cnv->get_status_color();
}


void convoy_scrollitem_t::set_text(char const* const t)
{
	if(  t  &&  t[0]  &&  strcmp( t, cnv->get_name() )  ) {
		// text changed => call tool
		cbuffer_t buf;
		buf.printf("c%u,%s", cnv.get_id(), t );
		werkzeug_t *w = create_tool( WKZ_RENAME_TOOL | SIMPLE_TOOL );
		w->set_default_param( buf );
		world()->set_werkzeug( w, cnv->get_besitzer() );
		// since init always returns false, it is safe to delete immediately
		delete w;
	}
}
