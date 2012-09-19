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
	#define DEVNULL "nul"
	#define RB "rb"
	#define WB "wb"
#else
	#define PREFIX ""
	#define TEMPDIR "temp/"
	#define RM "rm"
	#define DEVNULL "/dev/null"
	#define RB "r"
	#define WB "w"
#endif


#define STAGE1 PREFIX "pdftoppm -gray -r 300 -f %d -l %d \"%s\" | " PREFIX "convert - -fuzz 1%% -trim +repage \
               -resize %d -bordercolor white -border 0x10 -bordercolor black -border 0x5 -type GrayScale -depth 8 \
               gray:- 2>" DEVNULL

#define STAGE2 PREFIX "convert -size %dx%d -depth 8 gray:- -rotate %d +repage -strip -type GrayScale -depth 4 \
               -compress Zip -quality 100 " TEMPDIR "temp%04d.pdf 2>" DEVNULL

#define STAGE3 PREFIX "pdftk " TEMPDIR "temp*.pdf cat output \"%s [TYeBook].pdf\" && " RM " " TEMPDIR "temp*.pdf"

#define TUNE1  PREFIX "convert -size %dx%d -depth 8 gray:- +repage -strip -type GrayScale -depth 4 \
               -compress Zip -quality 100 " TEMPDIR "tune.pgm"

#define TUNE2  PREFIX "convert " TEMPDIR "tune.pgm -crop 100x%d+0+0 -fill black -pointsize 20 -annotate +30+400 %d \
               +repage -strip -type GrayScale -depth 4 -compress Zip -quality 100 " TEMPDIR "tune-V%04d.pdf"

#define TUNE3  PREFIX "convert " TEMPDIR "tune.pgm -crop %dx100+0+0 -fill black -pointsize 20 -annotate +280+50 %d \
               +repage -strip -type GrayScale -depth 4 -compress Zip -quality 100 " TEMPDIR "tune-H%04d.pdf"

#define TUNE4  PREFIX "convert " TEMPDIR "tune.pgm +repage -strip -type GrayScale -depth 4 -compress Zip \
               -quality 100 " TEMPDIR "tune-ZZZZ.pdf"


int main(int argc, char const *argv[]) {

	int page, pages, slide = 0;
	int width, height, overlap, frame, rotate;
	
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

		sprintf(string, TUNE1, height, width);
		inbuf = popen(string, WB);
		setbuf(inbuf, NULL);
		fwrite(buffer, frame, 1, inbuf);
		pclose(inbuf);

		printf("vertical\n");

		for (i=width-50; i<=width; i++) {
			sprintf(string, TUNE2, i, i, i);
			system(string);
		}

		printf("horizontal\n");

		for (i=height-50; i<=height; i++) {
			sprintf(string, TUNE3, i, i, i);
			system(string);
		}

		sprintf(string, TUNE4);
		system(string);

		// TODO - concat & delete

		printf("done!\n");
		return 0;
	}


	if (argc != 6) {
		printf("\nUsage: tyebook-base filename width height overlap rotate(R|L)\n");
		printf("   or: tyebook-base tune width height\n");
		return 0;
	}

	width = atoi(argv[2]);
	height = atoi(argv[3]);
	frame = width*height;

	overlap = atoi(argv[4])*width;
	rotate = argv[5][0]=='R' ? 90 : -90;


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

		temp = sprintf(string, STAGE1, page, page, argv[1], width);
		outbuf = popen(string, RB);

		while ((temp = fread(start, width, 1, outbuf)) > 0) {

			if (bufsize == frame) {

				temp = sprintf(string, STAGE2, width, height, rotate, ++slide);
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
	temp = sprintf(string, STAGE2, width, height, rotate, ++slide);
	inbuf = popen(string, WB);
	setbuf(inbuf, NULL);
	fwrite(buffer, bufsize-width, 1, inbuf);
	pclose(inbuf);
	
	printf("finishing...\n");

	temp = sprintf(string, STAGE3, argv[1]);
	system(string);

	printf("done!\n");

	return 0;

}
