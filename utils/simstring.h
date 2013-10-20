#ifndef _simstring_h
#define _simstring_h

#include <stddef.h>
#include <string>


#ifndef STRICMP
#ifdef _MSC_VER
#define STRICMP stricmp
#define STRNICMP strnicmp
#else
#define STRICMP strcasecmp
#define STRNICMP strncasecmp
#endif
#endif


// a single use number to string ...
// format could be zero, the "%d" is assumed
char *ntos(int number, const char *format);


/**
 * Set thousand seperator, used in money_to_string and
 * number_to_string
 * @author Hj. Malthaner
 */
void set_thousand_sep(char c);


/**
 * Set fraction seperator, used in money_to_string and
 * number_to_string
 * @author Hj. Malthaner
 */
void set_fraction_sep(char c);

char get_fraction_sep();
const char *get_large_money_string();

/**
 * Set thousand exponent (3=1000, 4=10000), used in money_to_string and
 * number_to_string
 * @author prissi
 */
void set_thousand_sep_exponent(int new_thousand_sep_exponent);

/**
 * Set abbrevitation and the amout by which large money amouts will be shortened
 * @author prissi
 */
void set_large_amout( const char *, const double v );

/* copies n lines of the source into a buffer *
 * @return a temporary buffer with the result
 * @author prissi
 */
char *make_single_line_string(const char *in,int number_of_lines);


/**
 * Formats a money value. Uses thousand separator. Two digits precision.
 * Concludes format with $ sign. Buffer must be large enough, no checks
 * are made!
 * @author Hj. Malthaner
 */
void money_to_string(char * buf, double f, const bool show_decimal = true);


/**
 * Formats a number value. Uses thousand separator. Buffer must be large enough,
 * no checks are made!
 */
int number_to_string(char * buf, double f, int decimal_places );


/**
 * Terminated, length limited string copy. Copies at most
 * n characters. Terminates dest string always by 0.
 * @return dest
 * @author Hj. Malthaner
 */
char *tstrncpy(char *dest, const char *src, size_t n);


/**
 * Removes whitespace from the end of the string.
 * Modifies the argument!
 * @author Hj. Malthaner
 */
void rtrim(char *);


/**
 * Hands back a pointer to the first non-whitespace character
 * of the argument. The argument must be 0 terminated.
 * @author Hj. Malthaner
 */
const char * ltrim(const char *);

/**
 * Trim function for std::strings
 * @author Max Kielland
 */
std::string trim (const std::string &str );

/**
 * Returns a pointer to the rest of str if str starts with start.
 */
char const* strstart(char const* str, char const* start);

/**
 * Returns whether s is a null pointer or the empty string.
 */
static inline bool strempty(char const* const s) { return !s || s[0] == '\0'; }


#endif
