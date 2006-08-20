/* testtool.h
 *
 * procedures for automated tests
 *
 * written by Hj. Maltahner, September 2000
 */

#ifndef test_testtool_h
#define test_testtool_h

class karte_t;
class spieler_t;

int tst_railtest(spieler_t *sp, karte_t *welt, koord pos);

#endif
