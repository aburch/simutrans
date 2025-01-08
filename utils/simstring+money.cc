/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simstring.h"

#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include <string.h>
#include <stdio.h>

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

	if(env_t::show_yen){
		f = f * 100.0;
	}

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
	else if(  show_decimal &&  !env_t::show_yen) {
		i = l+1;
		// only decimals for smaller numbers
		*p++ = fraction_sep;
		// since it might be longer due to unicode characters
		while(  tp[i]!=0  ) {
			*p++ = tp[i++];
		}
	}

	if(env_t::show_yen){
		for(i=0; translator::translate("$")[i]!=0; i++){
			*p++ = translator::translate("$")[i];
		}
	}
	else{
		*p++ = '$';
	}
	*p = 0;
}
