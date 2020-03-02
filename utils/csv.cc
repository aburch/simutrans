/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "csv.h"
#include "simstring.h"
#include "../macros.h"
#include "../simtypes.h"


CSV_t::CSV_t (const char *csvdata) :
	lines(1),
	offset(0),
	first_field(true)
{
	const char* pos = csvdata;
	cbuffer_t tmp;
	int ret;

	do {
		// Puts the field pointed to by pos into the cbuffer_t tmp
		// (While ret is not failure case or end of file - check these!)
		tmp.clear();
		ret = (int)decode( pos, tmp );

		switch ( ret ) {
			case -1:
				new_line();
				pos++;
				break;

			case -2: // finished
				break;

			case -4:
			case -5:
			case -9: // Encountered invalid data, stop parsing
				break;

			default:
				// Encode contents of tmp and add them to our contents
				pos += ret;
				add_field( tmp.get_str() );
		}
	} while(  ret > -2  );
}


int CSV_t::get_next_field (cbuffer_t& buf)
{
	// Sanity check - offset must be less than or equal to the length of contents string
	if (  strlen( contents.get_str() ) < offset  ) {
		return -5;
	}

	// Start at contents + offset (position of next field)
	const char *pos = contents.get_str() + offset;

	// Decode into cbuffer_t
	int ret = (int)decode( pos, buf );
	if (  ret >= 0  ) {
		// Return value is amount to increase offset by
		// (Number of characters consumed)
		offset += ret;
	}

	return ret;
}


bool CSV_t::next_line ()
{
	// Call get_next_field until EOL, then move forward to next field past newline
	int r;
	cbuffer_t tmp;
	while ( (r = get_next_field( tmp )) >= 0 ) {};
	if (  r != -1  ) {
		// Return code from get_next_field() indicates that there are
		// no more lines, return false
		return false;
	}
	offset++;
	return true;
}

void CSV_t::reset ()
{
	offset = 0;
}

const char *CSV_t::get_str () const
{
	return contents.get_str();
}

int CSV_t::get_lines () const
{
	return lines;
}

void CSV_t::add_field (int newfield)
{
	char tmp[32];
	sprintf( tmp, "%i", newfield );
	add_field( tmp );
}

void CSV_t::add_field (const char *newfield)
{
	// Add comma if this isn't the first field on a line
	if (  !first_field  ) {
		// If offset is pointing to the EOF character
		// (e.g. if offset == strlen(contents))
		// then advance it by one to point at the start of
		// the new field
		// If we don't do this after a call to add_field()
		// the next call to get_next_field() would always
		// return an empty string, as offset would point to
		// the comma preceding the last field, rather than
		// the last field itself
		if (  offset == strlen( contents.get_str() )  ) {
			offset++;
		}

		contents.append( "," );
	}

	// CSV encode string and add to buffer
	encode( newfield, contents );

	first_field = false;

}

void CSV_t::new_line ()
{
	// Add newline to buffer
	contents.append( "\n" );
	lines++;
	first_field = true;
}


int CSV_t::encode( const char *text, cbuffer_t& output )
{
	char *n;
	bool wrap = false;

	// not empty?
	if(  *text  ) {
		// If text contains a comma, quote, or a line break, needs to be wrapped in quote marks
		wrap = strpbrk( text, ",\"\r\n" )!=0;

		// If text has leading or trailing spaces, must be wrapped in quotes
		if(  *text == ' '  ||  text[strlen( text )-1] == ' '  ) {
			wrap = true;
		}
	}

	int len = output.len();
	if(  wrap  ) {
		char* cpy = strdup(text);
		char* tmp = cpy;

		output.append( "\"" );

		// Replace all '"' with '""'
		while ( (n = strchr( tmp, '"' )) != NULL ) {
			*n = '\0';
			output.append( tmp );
			output.append( "\"\"" );
			tmp = n + 1;
		}

		output.append( tmp );
		output.append( "\"" );
		tmp = NULL;
		free(cpy);
	}
	else {
		output.append( text );
	}
	return output.len() - len;
}


int CSV_t::decode (const char *start, cbuffer_t& output)
{
	const char *s = start;

	switch ( *s ) {

		case '"': {
			const char *c = s + 1;
			const char *n;

			// Start of quoted field, may contain commas and newlines
			while( 1 ) {
				n = strchr( c, '"' );
				if(  n == NULL  ||  *n == '\0'  ) {
					// No matching '"' - malformed field
					return -9;
				}
				switch( *(n + 1) ) {
					case '"':
						// Copy everything up to and including the first quote
						output.append( c, (n - c) + 1 );
						c = n + 2;
						break;

					case ',':
						// Copy everything up to the quotes before the comma
						// Needs to be +1 since output.append puts a '\0' on the last position copied
						output.append( c, (n - c) );
						// Move offset to character after ','
						return (int)((n - s) + 2);

					case '\0':
					case '\n':
					case '\r':
						// Copy everything up to the quotes before the newline/null
						output.append( c, (n - c) );
						// Move offset to last character
						return (int)((n - s) + 1);

					default:
						// Single '"' in the middle of a field - malformed field
						return -9;
				}
			}
			// one will never reach here ...
			assert(0);
		}

		case '\r':
		case '\n':
			return -1;

		case '\0':
			return -2;

		default: {
			const char *newline = strchr( s, '\n' );
			const char *comma = strchr( s, ',' );

			if(  newline == NULL  &&  comma == NULL  ) {
				// Copy to '\0' + leave offset on '\0' char
				// Copy all to '\0' into buffer
				output.append( s, strlen( s ) );
				// Offset to point at '\0'
				return (int)strlen( s );
			}

			if(  newline != NULL  &&  (comma == NULL  ||  newline < comma)  ) {
				// Copy up to newline + leave offset on newline char
				output.append( s, (newline - s) );
				return (int)(newline - s);
			}

			if(  comma != NULL  &&  (newline == NULL  ||  comma < newline)  ) {
				// Copy up to comma + leave offset at next char after
				output.append( s, (comma - s) );
				return (int)((comma - s) + 1);
			}
			return -9;
		}

	}
}
