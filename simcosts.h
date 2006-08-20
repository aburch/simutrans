#ifndef simcosts_h
#define simcosts_h

// Kosten fuer Gebaeude

// Hajo: all costs are stored in cent!
// to get the value in credits, divide them by 100

#define CST_DOCK          -200000
#define CST_BAHNHOF       -240000
#define CST_FRACHTHOF     -80000
#define CST_BUSHALT       -60000
#define CST_POST          -60000

#define CST_SIGNALE       -5000

#define CST_BAHNDEPOT     -100000
#define CST_SCHIFFDEPOT   -250000
#define CST_STRASSENDEPOT -130000
#define CST_STADT         -500000000

// Kosten fuer Strassenbau

#define CST_STRASSE -10000
#define CST_SCHIENE -10000
#define CST_BAU     -12000
#define CST_BRUECKE -20000
#define CST_TUNNEL  -100000
#define CST_LEITUNG -12000
#define CST_OBERLEITUNG -8000

#define CST_BAUM_ENTFERNEN  -1000
#define CST_HAUS_ENTFERNEN  -100000

// Kosten fuer Fahrzeuge


#define CST_MEDIUM_VEHIKEL -250000


// Preise fuer den Transport einer Einheit einer Ware ueber das erste Feld

/* Hajo: jetzt in Datei 'goods.tab'
#define CST_KOHLE    70
#define CST_ERZ      70
#define CST_STAHL   110
#define CST_PAX      50
#define CST_POST     70
#define CST_HOLZ     70
#define CST_BRETTER 110
#define CST_OEL      70
#define CST_PLASTIK  80
*/

#endif
