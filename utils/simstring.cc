/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simstring.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>


static char thousand_sep = ',';
static char fraction_sep = '.';
static const char *large_number_string = "M";
static double large_number_factor = 1e99; // off
static int thousand_sep_exponent = 3;


/**
 * Set thousand separator, used in money_to_string and
 * number_to_string
 */
void set_thousand_sep(char c)
{
	thousand_sep = c;
}


/**
 * Set thousand exponent (3=1000, 4=10000), used in money_to_string and
 * number_to_string
 */
void set_thousand_sep_exponent(int new_thousand_sep_exponent)
{
	thousand_sep_exponent = new_thousand_sep_exponent>0 ? new_thousand_sep_exponent : 3;
}



/**
 * Set fraction separator, used in money_to_string and
 * number_to_string
 */
void set_fraction_sep(char c)
{
	fraction_sep = c;
}


char get_fraction_sep()
{
	return fraction_sep;
}

const char *get_large_money_string()
{
	return large_number_string;
}


/**
 * Set large money abbreviation, used in money_to_string and
 * number_to_string
 */
void set_large_amount(const char *s, const double v)
{
	large_number_string = s;
	large_number_factor = v;
}


/**
 * Formats a money value. Uses thousand separator. Two digits precision.
 * Concludes format with $ sign. Buffer must be large enough, no checks
 * are made!
 */
void money_to_string(char * p, double f, const bool show_decimal)
{
	char   tmp[128];
	char   *tp = tmp;
	int    i,l;

	if(  f>1000.0*large_number_factor  ) {
		sprintf( tp, "%.1f", f/large_number_factor );
	}
	else {
		sprintf( tp, "%.2f", f );
	}

	// skip sign
	if(*tp == '-') {
		*p ++ = *tp++;
	}

	// format string
	l = (long)(size_t)(strchr(tp,'.') - tp);

	i = l % thousand_sep_exponent;

	if(i != 0) {
		memcpy(p, tp, i);
		p += i;
		*p++ = thousand_sep;
	}

	while(i < l) {
		for(  int j=0;  j<thousand_sep_exponent;  j++  ) {
			*p++ = tp[i++];
		}
		*p++ = thousand_sep;
	}
	--p;

	if(  f>1000.0*large_number_factor  ) {
		// only decimals for smaller numbers; add large number string instead
		for(  i=0;  large_number_string[i]!=0;  i++  ) {
			*p++ = large_number_string[i];
		}
	}
	else if(  show_decimal  ) {
		i = l+1;
		// only decimals for smaller numbers
		*p++ = fraction_sep;
		// since it might be longer due to unicode characters
		while(  tp[i]!=0  ) {
			*p++ = tp[i++];
		}
	}
	*p++ = '$';
	*p = 0;
}


int number_to_string(char * p, double f, int decimals  )
{
	char  tmp[128];
	char  *tp = tmp;
	long  i,l;
	bool  has_decimals;

	if(  decimals>0  ) {
		sprintf(tp,"%.*f",decimals,f);
		has_decimals = true;
	}
	else {
		sprintf(tp,"%.0f", f);
		// some compilers produce trailing dots then ...
		has_decimals = strchr(tp,'.')!=NULL;
	}

	// skip sign
	if(*tp == '-') {
		*p ++ = *tp++;
	}

	// format string
	l = has_decimals ? (long)(size_t)(strchr(tp,'.') - tp) : strlen(tp);

	i = l % thousand_sep_exponent;

	if(i != 0) {
		memcpy(p, tp, i);
		p += i;
		*p++ = thousand_sep;
	}

	while(i < l) {
		for(  int j=0;  j<thousand_sep_exponent;  j++  ) {
			*p++ = tp[i++];
		}
		*p++ = thousand_sep;
	}
	p--;

	if(  has_decimals  ) {
		i++;
		*p++ = fraction_sep;
		while(  tp[i]!=0  ) {
			*p++ = tp[i++];
		}
	}
	*p = 0;

	return (int)(p-tmp);
}



/**
 * tries to squeese a nubmer into a string with max_length
 * Will still produce a too long string with too large nubmers!
 */
void number_to_string_fit(char *ret, double f, int decimals, int max_length )
{
	char   result[128];
	int    len;

	number_to_string( result, f, decimals );
	len = strlen(result);

	if(  len <= max_length  ) {
		// ok fits ...
		strcpy( ret, result );
		return;
	}

	// not fitting: first strip decimals
	if(  decimals > 0  &&  len <= max_length+decimals+1  ) {
		tstrncpy( ret, result, len-(decimals) );
		return;
	}

	// still not fitting: Then we have to really shorten the string
	number_to_string( result, f/large_number_factor, 2 );
	len = strlen( result );
	const int llen = strlen(large_number_string);

	// not fitting: then strip those remaining decimals
	if(  len+llen > max_length  ) {
		result[len-3] = 0;
	}
	strcat( result, large_number_string );
	strcpy( ret, result );
}



/// copies a n into a single line and maximum 128 characters
char *make_single_line_string(const char *in,int number_of_lines)
{
	static char buf[64];
	int pos;

	// skip leading whitespaces
	while(*in=='\n'  ||  *in==' ') {
		in++;
	}
	// start copying
	for(pos=0;  pos<62  &&  *in!=0  &&  number_of_lines>0;  ) {
		if((unsigned)(*in)>' ') {
			buf[pos++] = *in++;
		}
		else {
			// replace new lines by space
			while(*in=='\n'  ||  *in==' ') {
				if(*in=='\n') {
					number_of_lines--;
				}
				in++;
			}
			buf[pos++] = ' ';
		}
	}
	// trim trailing spaces
	while(pos>0  &&  buf[pos-1]==' ') {
		pos--;
	}
	// end mark!
	buf[pos] = 0;
	return buf;
}




/**
 * Terminated, length limited string copy. Copies at most
 * n characters. Terminates dest string always by 0.
 * @return dest
 */
char *tstrncpy(char *dest, const char *src, size_t n)
{
	if (dest == src) {
		// source and destination are the same
		// avoid copying overlapping memory regions
		return dest;
	}

	strncpy(dest, src, n);
	dest[n-1] = '\0';

	return dest;
}


/**
 * Removes whitespace from the end of the string.
 * Modifies the argument!
 */
void rtrim(char * buf)
{
	for (size_t l = strlen(buf); l-- != 0 && 0 < buf[l] && buf[l] <= 32;) {
		buf[l] = '\0';
	}
}


/**
 * Hands back a pointer to the first non-whitespace character
 * of the argument. The argument must be 0 terminated
 */
const char * ltrim(const char *p)
{
	while(*p != '\0' && *p > 0 && *p <= 32) {
		p ++;
	}
	return p;
}


/**
 * Trims a std::string by removing any beginning and ending space/tab characters.
 * (Move to simstring?)
 * @retval std::string  The trimmed string.
 */
std::string trim(const std::string &str_)
{
	std::string str(str_);

	// left trim
	std::string::size_type pos = str.find_first_not_of(" \t");
	if( pos && pos  !=  std::string::npos ) {
		str = str.substr(pos);
	}

	// right trim
	pos = str.find_last_not_of(" \t");
	if( pos != str.length()-1 && pos  !=  std::string::npos ) {
		str = str.erase(pos+1);
	}

	return str;
}


char const* strstart(char const* str, char const* start)
{
	while (*start != '\0') {
		if (*str++ != *start++) return 0;
	}
	return str;
}
