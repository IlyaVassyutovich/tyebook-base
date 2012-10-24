/*
 * Copyright (c) 2012, Dmitry Egorov <mail@degorov.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
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


#define STAGE1P PREFIX "pdftoppm -gray -r 300 -f %d -l %d \"%s\" 2>" DEVNULL " | " PREFIX "convert - -fuzz 1%% -trim +repage -gamma 0.33 -resize %d -bordercolor white -border 0x10 -bordercolor black -border 0x5 -type GrayScale -depth 8 gray:- 2>" DEVNULL

#define STAGE1D PREFIX "ddjvu -format=pgm -scale=300 -page=%d \"%s\" 2>" DEVNULL " | " PREFIX "convert - -fuzz 1%% -trim +repage -resize %d -bordercolor white -border 0x10 -bordercolor black -border 0x5 -type GrayScale -depth 8 gray:- 2>" DEVNULL

#define STAGE2 PREFIX "convert -size %dx%d -depth 8 gray:- -rotate %d +repage -strip -type GrayScale -depth 4 -compress Zip -quality 100 " TEMPDIR "temp%04d.pdf 2>" DEVNULL

#define STAGE3 PREFIX "pdftk " TEMPDIR "temp*.pdf cat output \"%s [TYeBook].pdf\" && " RM " " TEMPDIR "temp*.pdf"

#define TUNE1  PREFIX "convert -size %dx%d -depth 8 gray:- +repage -strip -type GrayScale -depth 4 -compress Zip -quality 100 " TEMPDIR "tune.pgm"

#define TUNE2  PREFIX "convert " TEMPDIR "tune.pgm -crop 100x%d+0+0 -fill black -pointsize 20 -annotate +30+400 %d +repage -strip -type GrayScale -depth 4 -compress Zip -quality 100 " TEMPDIR "tune-V%04d.pdf"

#define TUNE3  PREFIX "convert " TEMPDIR "tune.pgm -crop %dx100+0+0 -fill black -pointsize 20 -annotate +280+50 %d +repage -strip -type GrayScale -depth 4 -compress Zip -quality 100 " TEMPDIR "tune-H%04d.pdf"

#define TUNE4  PREFIX "convert " TEMPDIR "tune.pgm +repage -strip -type GrayScale -depth 4 -compress Zip -quality 100 " TEMPDIR "tune-ZZZZ.pdf"

#define TUNE5  PREFIX "pdftk " TEMPDIR "tune-ZZZZ.pdf " TEMPDIR "tune*.pdf cat output tune-%d-%d.pdf && " RM " " TEMPDIR "tune*.*"



// globals
int width, height, frame;
char *buffer;
char string[2048];
FILE *outbuf, *inbuf;



const char *getfilextension(const char *fullfilename) {
    int size, index;
    size = index = 0;
    while (fullfilename[size]) {
        if (fullfilename[size] == '.')
            index = size;
        size++;
    }
    if(size && index)
        return fullfilename + index;
    return NULL;
}



int tune(int steps) {

    int i, j, px = 0;

    printf("\ngenerating...\n");

    buffer = malloc(frame+1);
    buffer[frame] = 0;

    for (i=0; i<width; i++) {
        px = i % 2;
        for (j=0; j<height; j++) {
            px = !px;
            buffer[i*height+j] = px*255;
        }
    }

    sprintf(string, TUNE1, height, width);
    inbuf = popen(string, WB);
    setbuf(inbuf, NULL);
    fwrite(buffer, 1, frame+1, inbuf);
    pclose(inbuf);

    printf("vertical...\n");

    for (i=width-steps+1; i<=width; i++) {
        sprintf(string, TUNE2, i, i, i);
        system(string);
    }

    printf("horizontal...\n");

    for (i=height-steps+1; i<=height; i++) {
        sprintf(string, TUNE3, i, i, i);
        system(string);
    }

    sprintf(string, TUNE4);
    system(string);

    sprintf(string, TUNE5, width, height);
    system(string);

    printf("done!\n");
    return 0;
}



int main(int argc, char const *argv[]) {

    int page = 0, pages = 0, slide = 0;
    int overlap, rotate;

    char *start;
    long bufsize = 0;

    // pdf=1 djvu=2
    int type = 0;

    if (argc<5 || argc>6) {
        printf("\nUsage: tyebook filename width height overlap rotate(R|L|0)\n");
        printf("   or: tyebook tune width height steps\n");
        return 0;
    } else {
        width = atoi(argv[2]);
        height = atoi(argv[3]);
        frame = width*height;
        if (argc == 5 && !strcmp(argv[1], "tune")) {
            tune(atoi(argv[4]));
            return 0;
        }
    }

    // starting
    overlap = atoi(argv[4])*width;

    if (argv[5][0]=='R')
        rotate = 90;
    else if (argv[5][0]=='L')
        rotate = -90;
    else
        rotate = 0;

    // getting file type
    if  (!strcmp(getfilextension(argv[1]), ".pdf") ||
         !strcmp(getfilextension(argv[1]), ".PDF"))
            type = 1;
    else if (!strcmp(getfilextension(argv[1]), ".djvu") ||
             !strcmp(getfilextension(argv[1]), ".DJVU"))
            type = 2;
    else {
        printf("Error: only PDF and DJVU files are supported\n");
        return 0;
    }

    // getting number of pages
    if (type == 1) {
        sprintf(string, "%spdfinfo \"%s\"", PREFIX, argv[1]);
        outbuf = popen(string, "r");
        while (fgets(string, sizeof string, outbuf))
            if (start = strstr(string, "Pages:"))
                pages = atoi(start+16);
    } else if (type == 2) {
        sprintf(string, "%sdjvm -l \"%s\"", PREFIX, argv[1]);
        outbuf = popen(string, "r");
        while (fgets(string, sizeof string, outbuf))
            if (strstr(string, "PAGE"))
                pages+=1;
    }
    pclose(outbuf);

    // main processing
    printf("\npages = %d\n", pages);
    printf("starting...\n");

    start = buffer = malloc(frame+1);
    buffer[frame] = 0;

    for (page=1; page<=pages; page++) {

        printf("page: %4d\n", page);

        if (type == 1)
            sprintf(string, STAGE1P, page, page, argv[1], width);
        else if (type == 2)
            sprintf(string, STAGE1D, page, argv[1], width);

        outbuf = popen(string, RB);

        while (fread(start, width, 1, outbuf) > 0) {

            if (bufsize == frame) {

                sprintf(string, STAGE2, width, height, rotate, ++slide);
                inbuf = popen(string, WB);
                setbuf(inbuf, NULL);
                fwrite(buffer, 1, frame+1, inbuf);
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
    sprintf(string, STAGE2, width, height, rotate, ++slide);
    inbuf = popen(string, WB);
    setbuf(inbuf, NULL);
    buffer[bufsize-width] = 0;
    fwrite(buffer, 1, bufsize-width+1, inbuf);
    pclose(inbuf);

    printf("finishing...\n");

    // exporting result
    sprintf(string, STAGE3, argv[1]);
    system(string);

    printf("done!\n");

    return 0;
}
