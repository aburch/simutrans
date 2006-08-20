/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * Usage for Iso-Angband is granted. Usage in other projects is not
 * permitted without the agreement of the author.
 */

/* mempool.h
 *
 * Speicherverwaltung mit Pools (Bloecken)
 * von Hj. Malthaner, 2000
 */

#ifndef mm_mempool_h
#define mm_mempool_h

class memblock_t;

/**
 * Verwaltungsklasse fuer die Pools
 *
 * @author Hj. Malthaner
 */
class mempool_t
{
 private:

  int elsize;

  /**
   * zeigt auf den Beginn der Blockliste
   */
  memblock_t *list;

  /**
   * zeigt auf den ersten block mit freien slots
   */
  memblock_t *first_free;

  /**
   * wir cachen den block, wenn ein slot freigegeben wurde, um
   * die Suche einzusparen.
   */
  memblock_t *a_free_block;

 public:

  mempool_t(int elsize);

  void * alloc();

  void free(void *p);
};

#endif
