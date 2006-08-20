/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * Usage for Iso-Angband is granted. Usage in other projects is not
 * permitted without the agreement of the author.
 */

/* memblock.h
 *
 * Verwaltung von Speicherbloecken fuer Memory Pools
 * von Hj. Malthaner, 2000
 */

#ifndef mm_memblock_h
#define mm_memblock_h

/**
 * Speicherbloecke fuer Memory Pools
 *
 * @author Hj. Malthaner
 */
class memblock_t
{
 private:

  // enum {BLOCKSIZE = 1024 };

  // 23-Aug-03: Hajo: for big games it seems much larger blocks
  // are needed. Players todays seem to use map sizes of 448x448
  // and larger. Grounds are managed by memblocks, thus larger
  // blocks shouel help to raise efficiency

  enum {BLOCKSIZE = 16384 };


  int elsize;
  int free_entries;

  /**
   * index der ersten Gruppe mit freiem slot
   * @author Hj. Malthaner
   */
  int first_free;

  unsigned int map[BLOCKSIZE/32];

  void * base;
  memblock_t *next;

  /**
   * wird von free_slot gesetzt und von alloc slot geprueft
   * damit kann nach jedem free_slot direct ohne suche ein
   * slot alloziert werden.
   * @author Hj. Malthaner
   */
  void * a_free_slot;

 public:

  memblock_t(int elsize);

  void * alloc_slot();
  void free_slot(void *);

  int get_free_entries() const {return free_entries;};

  memblock_t *get_next() const {return next;};
  void set_next(memblock_t *n) {next = n;};

  bool is_in_block(void *p) const {
//    const int i = (((char *)p) - ((char *)base));
//    return i >= 0 && i < BLOCKSIZE*elsize;

      return ((char *)p) >= ((char*)base) &&
             ((char *)p) < (((char*)base) + BLOCKSIZE*elsize);

  };
};

#endif
