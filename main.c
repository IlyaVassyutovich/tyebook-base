#include <stdio.h>
#include <string.h>
#include <stdlib.h>



#define WINDOWS
//#define LINUX

#ifdef WINDOWS
	#define PREFIX "wutils\\"
	#define TEMPDIR "temp\\"
	#define RB "rb"
	#define WB "wb"
#endif

#ifdef LINUX
	#define PREFIX ""
	#define TEMPDIR "temp/"
	#define RB "r"
	#define WB "w"
#endif



int main(int argc, char const *argv[]) {

	// char line[1024];
	// FILE *fp2;
	// char *found;
	// int lcntr = 0;

	int page, pages, slide = 0;
	int width, height, overlap, frame, rotate, depth;

	FILE *outbuf;
	FILE *inbuf;
	char string[2048];	

	int temp = 0;

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

		temp = sprintf(string, "%spdftoppm -gray -r 300 -f %d -l %d \"%s\" | %sconvert - -fuzz 1%% -trim +repage -resize 800 -bordercolor white -border 0x10 -bordercolor black -border 0x5 -type GrayScale -depth 8 gray:-", PREFIX, page, page, argv[1], PREFIX);
		outbuf = popen(string, RB);

		while ((temp = fread(start, width, 1, outbuf)) > 0) {

			if (bufsize == frame) {

				// // for debug
				// temp = sprintf(string, "%stezt%04d.raw", TEMPDIR, lcntr++);
				// fp2 = fopen(string, "wb");
				// fwrite(buffer, frame, 1, fp2);		
				// fclose(fp2);
				// // end debug

				temp = sprintf(string, "%sconvert -size %dx%d -depth 8 gray:- -rotate %d +repage -strip -type GrayScale -depth %d -compress Zip -quality 100 \"%stemp%04d.pdf\"", PREFIX, width, height, rotate, depth, TEMPDIR, ++slide);
				inbuf = popen(string, WB);
				fwrite(buffer, frame, 1, inbuf);
				pclose(inbuf);

				// copy overlapping data from buffer end
				memmove(buffer, buffer+frame-overlap, overlap);
				bufsize=overlap;			

			}
			buffer = realloc(buffer, bufsize+=width);
			start = buffer+bufsize-width;
		}

		pclose(outbuf);

	}

	// delete last buffer line (garbage)
	buffer = realloc(buffer, bufsize-=width);

	// // for debug
	// temp = sprintf(string, "%steztLLLL.raw", TEMPDIR);
	// fp2 = fopen(string, "wb");
	// fwrite(buffer, bufsize, 1, fp2);		
	// fclose(fp2);
	// // end debug
	
	return 0;

}
