#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
using namespace std;

#include "../tpl/array2d_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../dataobj/koord.h"
#include "dbg_weightmap.h"

void dbg_weightmap(array2d_tpl<double> &map, array2d_tpl< vector_tpl<koord> > &places, unsigned weight_max, const char *base_filename, unsigned n){
	ostringstream filename(base_filename, ios::ate);
	filename << setw(4) << setfill('0') << n << ".pgm";
	ofstream file(filename.str().c_str(), ios::out | ios::trunc);
	unsigned int xmax = map.get_width();
	unsigned int ymax = map.get_height();
	file << "P2\n" << xmax << '\n' << ymax << '\n' << weight_max << '\n';
	for(unsigned y =0; y < ymax; y++) {
		for(unsigned x=0; x < xmax; x++) {
			double f = map.at(x,y);
			if ( f < -1.0 || places.at(x,y).empty()) {
				f = -1.0;
			}
			else if ( f > 1.0 ) {
				f = 1.0;
			}
			file << int( weight_max * (f + 1.0)/2.0)<< ' '; 
		}
		file << '\n';
	}
	file.close();
}

