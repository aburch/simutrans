#ifndef cbuffer_t_h
#define cbuffer_t_h

#include <stdarg.h>
#include <stdlib.h>
/**
 * A character buffer. Main operation is 'append'
 * @author Hj. Malthaner
 */
class cbuffer_t
{
private:

	unsigned int capacity;

	/**
	* Number of characters without(!) trailing '\0'
	* @author Hj. Malthaner
	*/
	unsigned int size;

	char *buf;

	/**
	 * Implementation for copy constructor and copy assignment operator
	 * @author Timothy Baldock <tb@entropy.me.uk>
	 */
	void copy( const cbuffer_t& cbx );

	/**
	 * Implementation for destructor and copy assignment operator
	 * @author Timothy Baldock <tb@entropy.me.uk>
	 */
	void free();

public:
	/**
	 * Number of characters without(!) trailing '\0'
	 * @author Hj. Malthaner
	 */
	int len() const { return size; }

	/**
	 * Creates a new cbuffer with capacity cap
	 */
	cbuffer_t();
	~cbuffer_t();

	/**
	 * Copy constructor
	 * @author Timothy Baldock <tb@entropy.me.uk>
	 */
	cbuffer_t(const cbuffer_t& cbx);

	/**
	 * Copy assignment operator
	 * @author Timothy Baldock <tb@entropy.me.uk>
	 */
	cbuffer_t& operator= (const cbuffer_t& cbx);

	/**
	 * Clears the buffer
	 * @author Hj. Malthaner
	 */
	void clear();


	/**
	 * Appends text. Buffer will be extended if it does not have enough capacity.
	 * @author Hj. Malthaner
	 */
	void append(const char * text);

	/**
	 * Appends text, at most n characters worth. Buffer will be extended if needed.
	 * maxchars should NOT include the null at the end of the string!
	 *  (e.g. it should be equivalent to the output of strlen())
	 * @author Timothy Baldock <tb@entropy.me.uk>
	 */
	void append(const char* text, size_t maxchars);

	/**
	 * Return contents of buffer
	 * @author Timothy Baldock <tb@entropy.me.uk>
	 */
	const char* get_str() const;

	/**
	 * Appends a number. Buffer will be extended if it does not have enough capacity.
	 * @author Hj. Malthaner
	 */
	void append(double n, int precision);

	/* Append formatted text to the buffer */
	void printf(const char *fmt, ...);

	void vprintf(const char *fmt,  va_list args );

	/* enlarge the buffer if needed (i.e. size+by_amount larger than capacity) */
	void extend(unsigned int by_amount);

	/**
	 * Automagic conversion to a const char* for backwards compatibility
	 * @author Hj. Malthaner
	 */
	operator const char *() const {return buf;}

	/// checks whether format specifiers in @p translated match those in @p master
	static bool check_format_strings(const char* master, const char* translated);
};

#endif
