#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main(int argc, char const *argv[]) {

	FILE *fp;
	char line[130];

	char *found;

	int PAGES = 0;
   
	fp = popen("wutils\\pdfinfo.exe temp\\simpler.pdf", "r");

	while (fgets(line, sizeof line, fp)) {
		// printf("%s", line);

		if (found = strstr(line, "Pages:")) {
			PAGES = atoi(found+16);
			printf("PAGES = %d\n", PAGES);
		}

	}
	pclose(fp);
	
	return 0;

}
