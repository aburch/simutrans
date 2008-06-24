#ifndef scenario_h
#define scenario_h

#include "../utils/cstring_t.h"
#include "../simtypes.h"

class loadsave_t;
class stadt_t;
class fabrik_t;
class karte_t;

class scenario_t
{
private:
	enum { CONNECT_CITY_WORKER=1, CONNECT_FACTORY_PAX, CONNECT_FACTORY_GOODS, DOUBLE_INCOME, BUILT_HEADQUARTER_AND_10_TRAINS, TRANSPORT_1000_PAX };

	char *scenario_name;

	// enum for what to check
	uint16	what_scenario;

	sint64	factor;

	stadt_t *city;
	fabrik_t *target_factory;

	static karte_t *welt;

	void get_factory_producing( fabrik_t *fab, int &producing, int &existing );

public:
	scenario_t(karte_t *w);

	void init( const char *file, karte_t *welt );

	void rdwr(loadsave_t *file);

	const char *get_filename() const { return scenario_name; }

	// returns completion percentage
	int completed(int player_nr);

	// true, if a scenario is present
	bool active() const { return what_scenario>0; }

	const char *get_description();
};

#endif
