               Simutrans city name generation help
               -----------------------------------


Simutrans can create city names two ways:

1) If there is a citylist_xx.txt file for the chosen language
   city names are read from this file.
2) Otherwise random city names are created from the syllables
   given in xx.tab


Currently there is only one citylist per language used. I.e. for
english, citylist_en.txt is used. A citylist in pak/text/ will overlay
a citylist in text/. So different scenarios can have different citylists.

But there are some additional citylist files supplied, i.e.
citylist_en_au.txt for australian city names and citylist_de_at.tx
for austrian city names.

You can copy the citylist either in the folder text/ or to your pak-files
e.g. pak/text/.

To use them, you need to rename the files. I.e. to use the austrian
city names, rename citylist_de.txt to citylist_de_de.txt and
rename citylist_de_at.txt to citylist_de.tx


If you don't want Simutrans to use the city name lists, delete them.
Simutrans will then use the old (pre 0.82.15.5exp) scheme of random
city names.


written by Hansjörg Malthaner, 07-Dec-03
change by Markus Pristovsek, 14-Jan-04
