#include <algorithm>

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


// static helper function for sorting lineintems

line_scrollitem_t::sort_modes_t line_scrollitem_t::sort_mode = line_scrollitem_t::SORT_BY_NAME;


bool line_scrollitem_t::compare( gui_scrolled_list_t::scrollitem_t *aa, gui_scrolled_list_t::scrollitem_t *bb )
{
	if(  sort_mode != SORT_BY_NAME  ) {
		// if there is a mixed list, then this will lead to crashes!
		line_scrollitem_t *a = (line_scrollitem_t *)aa;
		line_scrollitem_t *b = (line_scrollitem_t *)bb;
		switch(  sort_mode  ) {
			case SORT_BY_NAME:	// default
				break;
			case SORT_BY_ID:
				return (a->get_line().get_id(),b->get_line().get_id())<0;
			case SORT_BY_PROFIT:
				return (a->get_line()->get_finance_history(1,LINE_PROFIT) - b->get_line()->get_finance_history(1,LINE_PROFIT))<0;
			case SORT_BY_TRANSPORTED:
				return (a->get_line()->get_finance_history(1,LINE_TRANSPORTED_GOODS) - b->get_line()->get_finance_history(1,LINE_TRANSPORTED_GOODS))<0;
			case SORT_BY_CONVOIS:
				return (a->get_line()->get_finance_history(1,LINE_CONVOIS) - b->get_line()->get_finance_history(1,LINE_CONVOIS))<0;
			case SORT_BY_DISTANCE:
				// normalizing to the number of convoys to get the fastest ones ...
				return (a->get_line()->get_finance_history(1,LINE_DISTANCE)/max(1,a->get_line()->get_finance_history(1,LINE_CONVOIS)) -
						b->get_line()->get_finance_history(1,LINE_DISTANCE)/max(1,b->get_line()->get_finance_history(1,LINE_CONVOIS)) )<0;
			default: break;
		}
		// default sorting ...
	}

	// first: try to sort by number
	const char *atxt = ((gui_scrolled_list_t::const_text_scrollitem_t *)aa)->get_text();
	int aint = 0;
	// isdigit produces with UTF8 assertions ...
	if(  atxt[0]>='0'  &&  atxt[0]<='9'  ) {
		aint = atoi( atxt );
	}
	else if(  atxt[0]=='('  &&  atxt[1]>='0'  &&  atxt[1]<='9'  ) {
		aint = atoi( atxt+1 );
	}
	const char *btxt = ((gui_scrolled_list_t::const_text_scrollitem_t *)bb)->get_text();
	int bint = 0;
	if(  btxt[0]>='0'  &&  btxt[0]<='9'  ) {
		bint = atoi( btxt );
	}
	else if(  btxt[0]=='('  &&  btxt[1]>='0'  &&  btxt[1]<='9'  ) {
		bint = atoi( btxt+1 );
	}
	if(  aint!=bint  ) {
		return (aint-bint)<0;
	}
	// otherwise: sort by name
	return strcmp(atxt, btxt)<0;
}


bool line_scrollitem_t::sort( vector_tpl<scrollitem_t *>&v, int offset, void * ) const
{
	vector_tpl<scrollitem_t *>::iterator start = v.begin();
	while(  offset-->0  ) {
		++start;
	}
	std::sort( start, v.end(), line_scrollitem_t::compare );
	return true;
}
