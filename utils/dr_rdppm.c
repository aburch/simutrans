#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dr_rdppm.h"

typedef int INT32;

static void
read_ppm_body_plain(unsigned char *block, FILE *file, const INT32 breite, const INT32 hoehe)
{
    INT32 x,y;

    for(y=0;y<hoehe;y++) {
	for(x=0;x<breite;x++) {
	    *block++ = fgetc(file);
	    *block++ = fgetc(file);
	    *block++ = fgetc(file);
	}
    }
}

int
load_block(unsigned char *block, char *filename)
{
    char ID[2];

    FILE *file = fopen(filename, "rb");

    if(file == NULL) {
	perror("Error");
	printf("->%s\n", filename);
	exit(1);
    }


    ID[0] = fgetc( file );         /* ID sollte P6 sein */
    ID[1] = fgetc( file );




    if( strncmp(ID,"P6",2) == 0 ) {
	char dummy[256];
	INT32 breite,hoehe,tiefe;

	fgetc( file );             /* return schlucken */
	do {
		fgets(dummy, sizeof(dummy), file);
	} while(dummy[0] == '#');

	sscanf(dummy, "%d %d", &breite, &hoehe);
	fgets(dummy, sizeof(dummy),file);
        sscanf(dummy , "%d\n",&tiefe);

	printf("%dx%dx%d\n", breite, hoehe, tiefe);


	read_ppm_body_plain(block, file, breite, hoehe);

	return 1;
    } else {
	puts("Keine PPM (P6) Datei !");

	return 0;
    }
}
