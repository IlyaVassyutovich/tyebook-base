#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main(int argc, char const *argv[]) {

	FILE *fp;
	char line[1024];

	FILE *fp2;

	char *found;

	int PAGE, PAGES = 0;

	char STAGE1[2048];
	char STAGE2[2048];

	int tmp = 0;

	int WIDTH, HEIGHT, OVERLAP, FRAME;

	char *BUFFER;
	char *START;
	size_t BUFSIZE;

	int lcntr = 0;
   
	if (argc < 7) {
		printf("Usage: tyebook-base FILENAME WIDTH HEIGHT OVERLAP ROTATE DEPTH\n");
		return 0;
	}

 	WIDTH = atoi(argv[2]);
 	HEIGHT = atoi(argv[3]);
 	OVERLAP = atoi(argv[4])*WIDTH;
 	FRAME = WIDTH*HEIGHT;


	fp = popen("wutils\\pdfinfo.exe temp\\simpler.pdf", "r");

	while (fgets(line, sizeof line, fp)) {
		// printf("%s", line);

		if (found = strstr(line, "Pages:")) {
			PAGES = atoi(found+16);
			printf("PAGES = %d\n", PAGES);
		}

	}
	pclose(fp);

	BUFSIZE = WIDTH;
	BUFFER = malloc(BUFSIZE);
	START = BUFFER;

	for (PAGE=2; PAGE<=4; PAGE++) {

		printf("PAGE: %04d\n", PAGE);

		tmp = sprintf(STAGE1, "wutils\\pdftoppm -gray -r 300 -f %d -l %d \"%s\" | wutils\\convert - -fuzz 1%% -trim +repage -resize 800 -bordercolor white -border 0x10 -bordercolor black -border 0x5 -type GrayScale -depth 8 gray:-", PAGE, PAGE, argv[1]);

		fp = popen(STAGE1, "rb");

		while ((tmp = fread(START, WIDTH, 1, fp)) > 0) {

			if (BUFSIZE % FRAME == 0) {

				// for debug
				tmp = sprintf(STAGE2, "temp\\tezt%04d.raw", lcntr++);
				fp2 = fopen(STAGE2, "wb");
				fwrite(BUFFER, FRAME, 1, fp2);		
				fclose(fp2);
				// end debug

				// tmp = sprintf(STAGE2, "wutils\\convert -size %sx%s -depth 8 \"temp%04d.gray\" -rotate -90 +repage -strip -type GrayScale -depth 4 -compress Zip -quality 100 \"temp\\temp%04d.pdf\"", argv[2], argv[3], PAGE, PAGE);				

				// copy overlapping data from buffer end
				memmove(BUFFER, BUFFER+FRAME-OVERLAP, OVERLAP-WIDTH);
				// black line
				memset(BUFFER+OVERLAP-WIDTH, 0, WIDTH);
				BUFSIZE=OVERLAP;				

			}
			BUFFER = realloc(BUFFER, BUFSIZE+=WIDTH);
			START = BUFFER+BUFSIZE-WIDTH;
		}

		pclose(fp);

	}

	// delete last buffer line (garbage)
	BUFFER = realloc(BUFFER, BUFSIZE-=WIDTH);

	// for debug
	fp2 = fopen("temp\\teztLLLL.raw", "wb");
	fwrite(BUFFER, BUFSIZE, 1, fp2);		
	fclose(fp2);
	// end debug
	
	return 0;

}
