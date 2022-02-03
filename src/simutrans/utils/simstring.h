/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef UTILS_SIMSTRING_H
#define UTILS_SIMSTRING_H


#include <stddef.h>
#include <string>

#ifdef __HAIKU__
#include <strings.h>
#endif

#ifndef STRICMP
#ifdef _MSC_VER
#define STRICMP stricmp
#define STRNICMP strnicmp
#else
#define STRICMP strcasecmp
#define STRNICMP strncasecmp
#endif
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf(buf,len, format,...) _snprintf_s(buf, len,len, format, __VA_ARGS__)
#endif

/**
 * Set thousand separator, used in money_to_string and
 * number_to_string
 */
void set_thousand_sep(char c);


/**
 * Set fraction separator, used in money_to_string and
 * number_to_string
 */
void set_fraction_sep(char c);

char get_fraction_sep();
const char *get_large_money_string();

/**
 * Set thousand exponent (3=1000, 4=10000), used in money_to_string and
 * number_to_string
 */
void set_thousand_sep_exponent(int new_thousand_sep_exponent);

/**
 * Set abbreviation and the amount by which large money amounts will be shortened
 */
void set_large_amount( const char *, const double v );

/**
 * copies n lines of the source into a buffer
 * @return a temporary buffer with the result
 */
char *make_single_line_string(const char *in,int number_of_lines);


/**
 * Formats a money value. Uses thousand separator. Two digits precision.
 * Concludes format with $ sign. Buffer must be large enough, no checks
 * are made!
 */
void money_to_string(char * buf, double f, const bool show_decimal = true);


/**
 * Formats a number value. Uses thousand separator. Buffer must be large enough,
 * no checks are made!
 */
int number_to_string(char * buf, double f, int decimal_places );

/* tries to squeeze the number into this amount of given digits
 * BUT: too large number will still exceed the space
 */
void number_to_string_fit(char *ret, double f, int decimals, int max_length );

/**
 * Terminated, length limited string copy. Copies at most
 * n characters. Terminates dest string always by 0.
 * @return dest
 */
char *tstrncpy(char *dest, const char *src, size_t n);


/**
 * Removes whitespace from the end of the string.
 * Modifies the argument!
 */
void rtrim(char *);


/**
 * Hands back a pointer to the first non-whitespace character
 * of the argument. The argument must be 0 terminated.
 */
const char * ltrim(const char *);

/**
 * Trim function for std::strings
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
