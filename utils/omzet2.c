#include <stdio.h>

// transforms font picture to font.dat.
// go like this:
// - make changes in font.gif. (no comments in gif please)
// - convert font.gif font.pbm.
// - omzet2 < font.pbm > font.dat
// - copy font.dat to sim-directory

// input:  font.pbm. 256x256 picture, at 8,8 offset the 16x16 font matrix.
//                   each letter 8w 10h.
// output: font.dat. 256 widths, followed by 256 letters 10 byte each.
//                   width is 2 bytes, 1 byte real w., 1 byte next letter w.
//                   letter is 10 rows of 1 byte each.

char in[8192];
char out[2560];
char width[512];
main()
{
  int b,c,end,x,y,h,i;
  char *p;

  // file to in
  for (i=0; i<11; i++) { getchar(); }
  p = in;
  for (;;) {
    c = getchar();
    if (c==EOF) break;
    *p++ = c;
  }
  fprintf(stderr, "%d\n", p-in); // should be 8192

  // in to out
  p = out;
  for (y=0;y<16;y++) { // column
    for (x=0;x<16;x++) { // row
      for (h=0;h<10;h++) { // letter height
	*p++ = in[8*32+1+y*320+x+h*32];
      }
    }
  }

  // out to file
  for (i=0;i<256;i++) {
    x=8;
    for (b=1;b<256;b<<=1) {
      end=0;
      for (h=0;h<10;h++) {
	if (out[i*10+h] & b) end=1; // if black, found letter
      }
      if (end==1) break;
      x--;
    }
    y = x+1;
    // width is x, next letter at y, deal with exceptions
    switch(i) {
     case ' ': y=4; break;
     case 'a': case 'r': // no room between a,r and next letter
     case 127:
     case 224: case 225: case 226: case 227: case 228: case 229: // accents
      y=x; break;
    }
    width[i] = x;
    width[256+i] = y;
  }

  for (i=0; i<512; i++) { putchar(width[i]); }
  for (i=0; i<2560; i++) { putchar(out[i]); }
}
