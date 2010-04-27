// selection of paks at the start time

#ifndef pakselector_h
#define pakselector_h


#include "savegame_frame.h"


class pakselector_t : public savegame_frame_t
{
private:
	// unused button_t load_addons;
	//bool at_least_one_add;
	gui_file_table_action_column_t action_column;
	gui_file_table_delete_column_t addon_column;

protected:
	/**
	* Aktion, die nach Knopfdruck gestartet wird.
	* @author Hansjörg Malthaner
	*/
	virtual void action(const char *filename);

	/**
	* Aktion, die nach X-Knopfdruck gestartet wird.
	* @author V. Meyer
	*/
	virtual bool del_action(const char *filename);

	// returns extra file info
	virtual const char *get_info(const char *fname);

	// true, if valid
	virtual bool check_file( const char *filename, const char *suffix );

	virtual void init(const char *suffix, const char *path);
	virtual void add_file(const char *filename, const bool not_cutting_suffix);
	
public:
	void fill_list();	// do the search ...

	bool has_pak() const { return use_table ? file_table.get_size().get_y() > 0 : !entries.empty(); }

	const char * get_hilfe_datei() const { return ""; }

	// since we only want to see the frames ...
	void zeichnen(koord pos, koord gr);

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	virtual bool action_triggered( gui_action_creator_t *komp, value_t extra);

	pakselector_t();
};

#endif
