/*
 * part of the Simutrans project
 * @author hsiegeln
 * 01/12/2003
 */

#ifndef simlinemgmt_h
#define simlinemgmt_h

#include <string.h>

#include "simtypes.h"
#include "simline.h"
#include "linehandle_t.h"
#include "dataobj/loadsave.h"
#include "simconvoi.h"

#include "simdebug.h"

class schedule_t;
class spieler_t;
class schedule_list_gui_t;

#define INVALID_LINE_ID ((uint16)(-1))

class simlinemgmt_t
{
public:
 	simlinemgmt_t(karte_t* welt);
 	~simlinemgmt_t();

	void zeige_info(spieler_t *);

	/*
	 * add a line
	 * @author hsiegeln
	 */
 	void add_line(linehandle_t new_line);

	/*
	 * delete a line
	 * @author hsiegeln
	 */
 	bool delete_line(linehandle_t line);

	/*
	 * update a line -> apply updated fahrplan to all convoys
	 * @author hsiegeln
	 */
	void update_line(linehandle_t line);

	/*
	* return a line by its ID
	* @author hsiegeln
	*/
	linehandle_t get_line_by_id(uint16 line);

 	/*
 	 * load or save the linemanagement
 	 */
 	void rdwr(karte_t * welt, loadsave_t * file, spieler_t * sp);

 	/*
 	 * sort the lines by name
 	 */
	void sort_lines();

 	/*
 	 * will register all stops for all lines
 	 */
	void register_all_stops();

	/*
	 * called after game is fully loaded;
	 */
	void laden_abschliessen();

	void rotate90( sint16 y_size );

	/**
	 * Creates a unique line id.
	 * @author Hj. Malthaner
	 */
	static uint16 get_unique_line_id();
	static void init_line_ids();

	void new_month();

	/**
	 * @author hsiegeln
	 */
	linehandle_t create_line(int ltype, spieler_t * sp, schedule_t * fpl=NULL);

	/*
	 * fill the list with all lines of a certain type
	 * type == simline_t::line will return all lines
	 */
	void get_lines(int type, vector_tpl<linehandle_t>* lines) const;

	karte_t* get_welt() const { return welt; }

	/**
	 * Will open the line management window and offer information about the line
	 * @author isidoro
	 */
	void show_lineinfo(spieler_t *sp, linehandle_t line);

 private:
	static uint8 used_ids[8192];

	vector_tpl<linehandle_t> all_managed_lines;

	karte_t * welt;

	schedule_list_gui_t *schedule_list_gui;  // Use with caution.  Valid only afer zeige_info
};

#endif
