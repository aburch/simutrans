/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef UTILS_CSV_H
#define UTILS_CSV_H


#include "cbuffer.h"


/*
Provides functionality for building a buffer containing CSV
encoded data. This is done in a very simple way, building up
the block of data field-by-field and line-by-line - this is not
intended as a way of representing data for editing!

This class is NOT thread safe!


Writing:

Each string added will be CSV encoded and may contain commas,
quotation marks and newlines.

The end of a line of data should be indicated by calling the new_line()
method. Each new field is added using the add_field() method.

There is a constructor which takes a char array containing CSV
formatted data to parse, which can then be read as normal.


Reading:

get_next_field(), next_line() and reset() are used to parse
each field in order.


Example:

Create a new, blank, CSV_t
Use the add_field() and add_line() functions to build up the data
Then finally read its contents with get_str()

Example:

CSV_t csv();

csv.add_field("Field1 Header");
csv.add_field("Field2 Header");
csv.new_line();
csv.add_field("field1 data");
csv.add_field("field2, data");

printf(csv.get_str());

// Output would be:

Field1 Header,Field2 Header
field1 data,"field2, data"

 */
class CSV_t {
	// The raw data itself (stored encoded)
	cbuffer_t contents;

	// Count of number of lines of CSV data
	// Note: This is not the same as the number of newlines in the file
	//       since CSV fields can contain encoded newlines
	int lines;

	// Offset into cbuffer for current item
	size_t offset;
	bool first_field;

	/*
	 * Escape the input text so it is appropriate to make up a single CSV field
	 * Writes output into the supplied cbuffer_t (appending to the end)
	 * Return length of output string
	 */
	int encode( const char *, cbuffer_t& );

	/*
	 * Parse the next field from the CSV formatted data pointed to by field
	 * Returns the number of characters consumed from input string
	 *  this may be more than the length of output, as it includes CSV formatting
	 *  stripped during decoding
	 * Returns -1 for end of line (call next_line())
	 * Returns -2 for end of data
	 * Returns -9 for corrupt input data (unable to parse any further)
	 */
	int decode( const char *, cbuffer_t& );

public:
	/* Constructors */
	CSV_t() : lines(1), offset(0), first_field(true) {}
	CSV_t( const char * );


	/* Methods for reading */

	/*
	 * Parse the next field of the current line
	 * Returns -4 if contents are NULL
	 * Returns -5 if offset exceeds contents size
	 * Else returns either the error code output of decode()
	 * or the length of the data consumed from the input string
	 * Field is appended to the cbuffer_t supplied
	 */
	int get_next_field( cbuffer_t& );

	/*
	 * Move onto the next line
	 * Return true on success, false if there are no more lines
	 */
	bool next_line();

	/*
	 * Reset parse position to start of data
	 */
	void reset();

	/*
	 * Return CSV formatted contents
	 */
	const char *get_str() const;

	/*
	 * Return number of lines of CSV data
	 */
	int get_lines() const;


	/* Methods for writing */

	/*
	 * Add a field to the current line
	 */
	void add_field(const char *);

	/*
	 * Add a field to the current line (number)
	 */
	void add_field(int);

	/*
	 * Terminate a line of CSV data, and move to the next line
	 */
	void new_line();
};

#endif
