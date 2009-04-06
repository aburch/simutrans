#ifndef cbuffer_t_h
#define cbuffer_t_h


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

	char * buf;

	cbuffer_t(const cbuffer_t &);
	cbuffer_t & operator=(const cbuffer_t &);

public:

	/**
	 * Number of characters without(!) trailing '\0'
	 * @author Hj. Malthaner
	 */
	int len() const { return size; }


	/**
	 * Creates a new cbuffer with capacity cap
	 * @param cap the capacity
	 * @author Hj. Malthaner
	 */
	cbuffer_t(unsigned int size);
	~cbuffer_t();


	/**
	 * Clears the buffer
	 * @author Hj. Malthaner
	 */
	void clear();


	/**
	 * Appends text. If buffer is full, exceeding text will not
	 * be appended.
	 * @author Hj. Malthaner
	 */
	void append(const char * text);


	/**
	 * Appends a number. If buffer is full, exceeding digits will not
	 * be appended.
	 * @author Hj. Malthaner
	 */
	void append(int n);

	/* Append formatted text to the buffer */
	void printf(const char* fmt, ...);

	/* enlarge the buffer if needed (i.e. size+by_amount larger than capacity) */
	void extent( const unsigned int by_amount );

	/**
	 * Automagic conversion to a const char* for backwards compatibility
	 * @author Hj. Malthaner
	 */
	operator const char *() const {return buf;}

};

#endif
