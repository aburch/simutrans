// IMPORTANT! CHANGE THIS ON NEW ACHIEVEMENTS ADDITION
#define NUM_ACHIEVEMENTS 2

// Why a macro for this? Check steam/achievements.cc
#define ACHIEVEMENTS     \
	X(LOAD_PAK192_COMIC) \
	X(QUERY_DICTACTOR)


#define X(id) id,
enum simachievements_enum { ACHIEVEMENTS };
#undef X