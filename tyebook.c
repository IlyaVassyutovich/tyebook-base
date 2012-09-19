#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define WIN
//#define NIX

#ifdef WIN
	#define PREFIX "wutils\\"
	#define TEMPDIR "temp\\"
	#define RB "rb"
	#define WB "wb"
#endif

#ifdef NIX
	#define PREFIX ""
	#define TEMPDIR "temp/"
	#define RB "r"
	#define WB "w"
#endif


#define STAGE1 "%spdftoppm -gray -r 300 -f %d -l %d \"%s\" | %sconvert - -fuzz 1%% -trim +repage -resize 800 \
               -bordercolor white -border 0x10 -bordercolor black -border 0x5 -type GrayScale -depth 8 gray:-"

#define STAGE2 "%sconvert -size %dx%d -depth 8 gray:- -rotate %d +repage -strip -type GrayScale -depth %d \
               -compress Zip -quality 100 \"%stemp%04d.pdf\""


int main(int argc, char const *argv[]) {

	int page, pages, slide = 0;
	int width, height, overlap, frame, rotate, depth;
	
	char string[2048];	
	int temp = 0;

	FILE *outbuf;
	FILE *inbuf;	

	char *buffer;
	char *start;
	long bufsize;

   
	if (argc < 7) {
		printf("Usage: tyebook-base filename width height overlap rotate depth\n");
		return 0;
	}

 	width = atoi(argv[2]);
 	height = atoi(argv[3]);
 	overlap = atoi(argv[4])*width;
 	frame = width*height;
	rotate = atoi(argv[5]);
	depth = atoi(argv[6]);


	temp = sprintf(string, "%spdfinfo \"%s\"", PREFIX, argv[1]);
	outbuf = popen(string, "r");

	while (fgets(string, sizeof string, outbuf)) {
		if (start = strstr(string, "Pages:")) {
			pages = atoi(start+16);
			printf("pages = %d\n", pages);
		}
	}
	pclose(outbuf);


	start = buffer = malloc(bufsize = width);

	for (page=1; page<=pages; page++) {

		printf("page: %04d\n", page);

		temp = sprintf(string, STAGE1, PREFIX, page, page, argv[1], PREFIX);
		outbuf = popen(string, RB);

		while ((temp = fread(start, width, 1, outbuf)) > 0) {

			if (bufsize == frame) {

				temp = sprintf(string, STAGE2, PREFIX, width, height, rotate, depth, TEMPDIR, ++slide);
				inbuf = popen(string, WB);
				fwrite(buffer, frame, 1, inbuf);
				pclose(inbuf);

				// move overlapping data from buffer end
				memmove(buffer, buffer+frame-overlap, overlap);
				bufsize=overlap;			

			}
			buffer = realloc(buffer, bufsize+=width);
			start = buffer+bufsize-width;
		}

		pclose(outbuf);

	}

	// last frame
	temp = sprintf(string, STAGE2, PREFIX, width, height, rotate, depth, TEMPDIR, ++slide);
	inbuf = popen(string, WB);
	fwrite(buffer, bufsize-width, 1, inbuf);
	pclose(inbuf);
	
	return 0;

}
