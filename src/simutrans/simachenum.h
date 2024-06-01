// IMPORTANT! CHANGE THIS ON NEW ACHIEVEMENTS ADDITION
#define NUM_ACHIEVEMENTS 4

// Why a macro for this? Check steam/achievements.cc
#define ACHIEVEMENTS     \
	X(ACH_LOAD_PAK192_COMIC) \
	X(ACH_QUERY_DICTACTOR) \
	X(ACH_PROD_INK) \
	X(ACH_TOOL_REMOVE_BUSY_BRIDGE)


#define X(id) id,
enum simachievements_enum { ACHIEVEMENTS };
#undef X