#ifndef quickstone_tpl_h
#define quickstone_tpl_h


/**
 * An implementation of the tombstone pointer checking method.
 * It uses an table of pointers and indizes into that table to
 * implement the tombstone system. Unlike real tombstones, this
 * template reuses entries from the tombstone tabel, but it tries
 * to leave free'd tombstones untouched as long as possible, to
 * detect most of the dangling pointers.
 *
 * This templates goal is to be efficient and fairly safe.
 *
 * @author Hj. Malthaner
 */
template <class T> class quickstone_tpl
{
 private:

  /**
   * Array of pointers. The first entry is always NULL!
   */
  static T ** data;


  /**
   * Next entry to check
   */
  static unsigned short int next;


  /**
   * Size of tombstone table
   */
  static unsigned short int size;


  /**
   * Retrieves next free tombstone index
   */
  static unsigned short int find_next() {
    unsigned short int i;

    // scan rest of array
    for(i = next; i<size; i++) {
      if(data[i] == 0) {
	next = i+1;
	return i;
      }
    }

    // scan whole array
    for(i = 1; i<size; i++) {
      if(data[i] == 0) {
	next = i+1;
      return i;
      }
    }

    // no free entry found, can't continue
    abort();
  };



  /**
   * The index in the table for this handle.
   *
   */
  unsigned short int entry;


 public:


  /**
   * Initializes the tombstone table. Calling init() makes all existing
   * quickstones invalid.
   *
   * @param n number of elements
   * @author Hj. Malthaner
   */
  static void init(const unsigned short int n) {

    size = n;
    data = new T* [size];

    // all NULL pointers are mapped to entry 0

    for(int i=0; i<size; i++) {
      data[i] = 0;
    }

    next = 1;
  };


  quickstone_tpl() {
    entry = 0;
  };


  quickstone_tpl(T* p) {
    if(p) {
      entry = find_next();
      data[entry] = p;
    } else {
      // all NULL pointers are mapped to entry 0
      entry = 0;
    }
  };


  /**
   * Copy constructor. Constructs a new quickstone from another one.
   *
   * @author Hj. Malthaner
   */
  quickstone_tpl(const quickstone_tpl& r) : entry(r.entry)
  {
  };


  /**
   * Assignment operator. Adjusts counters if one handle is
   * assigned ot another one.
   *
   * @author Hj. Malthaner
   */
  quickstone_tpl operator=(const quickstone_tpl &other)
  {
    entry = other.entry;
    return *this;
  };


  bool is_bound() const {
    return data[entry] != 0;
  };


  void unbind() {
    entry = 0;
  };


  /**
   * Removes the object from the tombstone table - this affects all
   * handles to the object!
   * @author Hj. Malthaner
   */
  T* detach() {
    T* p = data[entry];
    data[entry] = 0;
    return p;
  };


  /**
   * Danger - use with care.
   * Useful to hand the underlying pointer to subsystems
   * that don't know about quickstones - but take care that such pointers
   * are never ever deleted or that by some means detach() is called
   * upon deletion, i.e. from the ~T() destructor!!!
   *
   * @author Hj. Malthaner
   */
  T* get_rep() const {
    return data[entry];
  };


  /**
   * @return the index into the tombstone table. May be used as
   * an ID for the referenced object.
   */
  unsigned short int get_id() const {
    return entry;
  };


  /**
   * Overloaded dereference operator. With this, quickstines can
   * be used as if they were pointers.
   *
   * @author Hj. Malthaner
   */
  T* operator->() const {
    return data[entry];
  };


  bool operator== (const quickstone_tpl<T> &other) const {
    return entry == other.entry;
  };

  bool operator!= (const quickstone_tpl<T> &other) const {
    return entry != other.entry;
  };

};

template <class T> T** quickstone_tpl<T>::data = 0;

template <class T> unsigned short int quickstone_tpl<T>::next = 1;
template <class T> unsigned short int quickstone_tpl<T>::size = 0;


#endif // quickstone_tpl_h
