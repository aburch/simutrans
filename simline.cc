#include "simline.h"
#include "simlinemgmt.h"
#include "simhalt.h"
#include "simtypes.h"


simline_t::simline_t(karte_t * welt,
		     simlinemgmt_t * simlinemgmt,
		     fahrplan_t * fpl)
{
	init_financial_history();
	const int i = simlinemgmt->get_unique_line_id();
	memset(this->name, 0, 128);
	sprintf(this->name, "%s (%d)", translator::translate("Line"), i);
	this->fpl = fpl;
    this->old_fpl = new fahrplan_t(fpl);
	this->id = i;
	this->welt = welt;
	type = simline_t::line;
	simlinemgmt->add_line(this);
}


simline_t::simline_t(karte_t * welt,
		     simlinemgmt_t * /*simlinemgmt*/,
		     loadsave_t * file)
{
	init_financial_history();
	rdwr(file);
	this->welt = welt;
    this->old_fpl = new fahrplan_t(fpl);
	register_stops();
}


simline_t::~simline_t()
{
	int count = count_convoys() - 1;
	for (int i = count; i>=0; i--)
	{
		dbg->debug("simline_t::~simline_t()", "convoi '%d' removed", i);
		dbg->debug("simline_t::~simline_t()", "convoi '%d'->fpl=%p", i, get_convoy(i)->gib_fahrplan());

		// Hajo: take care - this call will do "remove_convoi()"
		// on our list!
		get_convoy(i)->unset_line();
	}
	unregister_stops();

	dbg->debug("simline_t::~simline_t()", "deleting fpl=%p and old_fpl=%p", fpl, old_fpl);

	delete (fpl);
	delete (old_fpl);

	dbg->message("simline_t::~simline_t()", "line %d (%p) destroyed", id, this);
}


void
simline_t::add_convoy(convoi_t * cnv)
{
	// only add convoy if not allready member of line
	if (!line_managed_convoys.contains(cnv))
	{
		this->line_managed_convoys.insert(cnv);
		financial_history[0][LINE_CONVOIS] = count_convoys();
	}
}

void
simline_t::remove_convoy(convoi_t * cnv)
{
	if (line_managed_convoys.contains(cnv))
	{
		this->line_managed_convoys.remove(cnv);
		financial_history[0][LINE_CONVOIS] = count_convoys();
	}
}

fahrplan_t *
simline_t::get_fahrplan()
{
	return this->fpl;
}

char *
simline_t::get_name()
{
	return  this->name;
}

convoi_t *
simline_t::get_convoy(int i)
{
	return line_managed_convoys.at(i);
}

int
simline_t::count_convoys()
{
	return line_managed_convoys.count();
}

void
simline_t::rdwr(loadsave_t * file)
{

	// only create a new fahrplan if we are loading a savegame!
	if (file->is_loading())
	{
		fpl = new fahrplan_t();
	} else {
		file->rdwr_enum(type, "\n");
	}

	file->rdwr_str(name, sizeof(name));
	file->rdwr_int(id, " ");
	fpl->rdwr(file);
	//financial history
	if (file->get_version() >= 83001)
	{
		for (int j = 0; j<MAX_LINE_COST; j++)
		{
			for (int k = MAX_MONTHS-1; k>=0; k--)
			{
				file->rdwr_longlong(financial_history[k][j], " ");
			}
		}
	} else {
	  // Hajo: added compatibility code
	  for (int j = 0; j<MAX_LINE_COST; j++) {
	    for (int k = MAX_MONTHS-1; k>=0; k--) {
	      financial_history[k][j] = 0;
	    }
	  }
	}
}

void
simline_t::register_stops()
{
	register_stops(this->fpl);
}

void
simline_t::register_stops(fahrplan_t * fpl)
{
	halthandle_t halt;

	dbg->debug("simline_t::register_stops()", "%d fpl entries", fpl->maxi);

	for (int i = 0; i<=fpl->maxi; i++)
	{
		halt = haltestelle_t::gib_halt(welt, fpl->eintrag.get(i).pos.gib_2d());
		if (halt.is_bound())
		{
			dbg->debug("simline_t::register_stops()", "halt not null");
			halt->add_line(this);
		} else {
			dbg->debug("simline_t::register_stops()", "halt null");
		}
	}
}

void
simline_t::unregister_stops()
{
	unregister_stops(this->fpl);
}

void
simline_t::unregister_stops(fahrplan_t * fpl)
{
	halthandle_t halt;
	for (int i = 0; i<=fpl->maxi; i++)
	{
		halt = haltestelle_t::gib_halt(welt, fpl->eintrag.get(i).pos.gib_2d());
		if (halt.is_bound())
		{
			halt->remove_line(this);
		}
	}
}


void
simline_t::renew_stops()
{
	unregister_stops(this->old_fpl);
	register_stops(this->fpl);

	dbg->debug("simline_t::renew_stops()",
		   "Line id=%d, name='%s'", id, name);
}

void
simline_t::new_month()
{
	for (int j = 0; j<MAX_LINE_COST; j++)
	{
		for (int k = MAX_MONTHS-1; k>0; k--)
		{
			financial_history[k][j] = financial_history[k-1][j];
		}
		financial_history[0][j] = 0;
	}
	financial_history[0][LINE_CONVOIS] = count_convoys();
}


/*
 * called from line_management_gui.cc to prepare line for a change of its schedule
 */
void simline_t::prepare_for_update()
{
  dbg->debug("simline_t::prepare_for_update()", "line %d (%p)", id, this);

  delete (old_fpl);
  this->old_fpl = new fahrplan_t(fpl);
}


void
simline_t::init_financial_history()
{
	for (int j = 0; j<MAX_LINE_COST; j++)
	{
		for (int k = MAX_MONTHS-1; k>=0; k--)
		{
			financial_history[k][j] = 0;
		}
	}
}
