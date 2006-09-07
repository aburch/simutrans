#ifndef _simstring_h
#define _simstring_h


#ifdef __cplusplus
extern "C" {
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


char get_fraction_sep(void);


int clip_string(const int strlen, const int maxlen, char *buf);


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
void money_to_string(char * buf, double f);


void number_to_string(char * buf, double f);


/**
 * @return true if strings are equal, false otherwise
 * @author Hj. Malthaner
 */
int tstrequ(const char *a, const char *b);


/**
 * Terminated, length limited string copy. Copies at most
 * n characters. Terminates dest string always by 0.
 * @return dest
 * @author Hj. Malthaner
 */
char *tstrncpy(char *dest, const char *src, int n);


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


#ifdef __cplusplus
}
#endif

#endif
