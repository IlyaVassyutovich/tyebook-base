/*
 * Copyright (c) 2012, Dmitry Egorov <mail@degorov.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#ifdef __MINGW32__
	#define PREFIX "wutils\\"
	#define TEMPDIR "temp\\"
	#define RM "del"
	#define DEVNUL "nul"
	#define RB "rb"
	#define WB "wb"
 	#define FONT "\"Courier New\""
#else
	#define PREFIX ""
	#define TEMPDIR "temp/"
	#define RM "rm"
	#define DEVNUL "/dev/null"
	#define RB "r"
	#define WB "w"
	#define FONT "\"courier\""
#endif

// TODO - TEMPDIRS INTO DEFINES
// TODO - REMOVE depth - default 4

#define STAGE1 "%spdftoppm -gray -r 300 -f %d -l %d \"%s\" | %sconvert - -fuzz 1%% -trim +repage -resize %d \
               -bordercolor white -border 0x10 -bordercolor black -border 0x5 -type GrayScale -depth 8 gray:- 2>" DEVNUL

#define STAGE2 "%sconvert -size %dx%d -depth 8 gray:- -rotate %d +repage -strip -type GrayScale -depth %d \
               -compress Zip -quality 100 %stemp%04d.pdf 2>" DEVNUL

#define STAGE3 "%spdftk %stemp*.pdf cat output \"%s [TYeBook].pdf\" && " RM " %stemp*.pdf"

#define TUNE1  "%sconvert -size %dx%d -depth 8 gray:- +repage -strip -type GrayScale -depth 4 \
               -compress Zip -quality 100 %stune.pgm 2>" DEVNUL

#define TUNE2  "%sconvert %stune.pgm -crop 100x%d+0+0 -fill black -pointsize 20 -annotate +30+400 %d \
               +repage -strip -type GrayScale -depth 4 -compress Zip -quality 100 %stune-V%04d.pdf"

#define TUNE3  "%sconvert %stune.pgm -crop %dx100+0+0 -fill black -pointsize 20 -annotate +280+50 %d \
               +repage -strip -type GrayScale -depth 4 -compress Zip -quality 100 %stune-H%04d.pdf"

#define TUNE4  "%sconvert %stune.pgm +repage -strip -type GrayScale -depth 4 -compress Zip -quality 100 %stune-ZZZZ.pdf"


int main(int argc, char const *argv[]) {

	int page, pages, slide = 0;
	int width, height, overlap, frame, rotate, depth;
	
	char string[2048];
	int temp = 0;

	FILE *outbuf;
	FILE *inbuf;

	char *buffer;
	char *start;
	long bufsize = 0;

	int i,j;


	if (!strcmp(argv[1], "tune")) {

		printf("\ngenerating...\n");

		width = atoi(argv[2]);
		height = atoi(argv[3]);
		frame = width*height;

		buffer = malloc(frame);

		for (i=0; i<width; i++) {
			temp = i % 2;
			for (j=0; j<height; j++) {
				temp = !temp;
				buffer[i*height+j] = temp*255;
			}
		}

		sprintf(string, TUNE1, PREFIX, height, width, TEMPDIR);
		inbuf = popen(string, WB);
		setbuf(inbuf, NULL);
		fwrite(buffer, frame, 1, inbuf);
		pclose(inbuf);

		printf("vertical\n");

		for (i=width-50; i<=width; i++) {
			sprintf(string, TUNE2, PREFIX, TEMPDIR, i, i, TEMPDIR, i);
			system(string);
		}

		printf("horizontal\n");

		for (i=height-50; i<=height; i++) {
			sprintf(string, TUNE3, PREFIX, TEMPDIR, i, i, TEMPDIR, i);
			system(string);
		}

		sprintf(string, TUNE4, PREFIX, TEMPDIR, TEMPDIR);
		system(string);

		// TODO - concat & delete

		printf("done!\n");
		return 0;
	}


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
			printf("\npages = %d\n", pages);
		}
	}
	pclose(outbuf);


	printf("starting...\n");

	start = buffer = malloc(frame);

	for (page=1; page<=pages; page++) {

		printf("page: %4d\n", page);

		temp = sprintf(string, STAGE1, PREFIX, page, page, argv[1], PREFIX, width);
		outbuf = popen(string, RB);

		while ((temp = fread(start, width, 1, outbuf)) > 0) {

			if (bufsize == frame) {

				temp = sprintf(string, STAGE2, PREFIX, width, height, rotate, depth, TEMPDIR, ++slide);
				inbuf = popen(string, WB);
				setbuf(inbuf, NULL);
				fwrite(buffer, bufsize, 1, inbuf);
				pclose(inbuf);

				// move overlapping data from buffer end
				memmove(buffer, buffer+frame-overlap, overlap);
				bufsize=overlap;

			}
			start = buffer+bufsize;
			bufsize+=width;
		}

		pclose(outbuf);

	}

	// last frame
	temp = sprintf(string, STAGE2, PREFIX, width, height, rotate, depth, TEMPDIR, ++slide);
	inbuf = popen(string, WB);
	setbuf(inbuf, NULL);
	fwrite(buffer, bufsize-width, 1, inbuf);
	pclose(inbuf);
	
	printf("finishing...\n");

	temp = sprintf(string, STAGE3, PREFIX, TEMPDIR, argv[1], TEMPDIR);
	system(string);

	printf("done!\n");

	return 0;

}
