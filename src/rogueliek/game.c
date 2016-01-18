#include <ccFont/ccFont.h>
#include <ccore/file.h>

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <getopt.h>
#include <dirent.h>

#include "window.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600

static char assetdir[256] = "./";
static ccfFont font;

void runGame()
{
	createWindow("rogueliek - " TOSTRING(ROGUELIEK_VERSION), DEFAULT_WIDTH, DEFAULT_HEIGHT);
	hideCursor();

	while(true){
		if(!updateWindow()){
			break;
		}
		renderWindow(6);
	}

	showCursor();
	destroyWindow();
}

void printVersion()
{
	printf("rogueliek - version " TOSTRING(ROGUELIEK_VERSION) "\n");
}

void printHelp()
{
	printf("Usage:\t rogueliek [OPTION...]\n\n"
			"Load the assets:\n"
			"\t-a,--assets=FILE\tSet the asset folder\n\n"
			"General information:\n"
			"\t-h,--help\tGive this help list\n"
			"\t-v,--version\tShow the current version\n");
}

static struct option opts[] = {
	{"help", no_argument, 0, 'h'},
	{"version", no_argument, 0, 'v'},
	{"assets", required_argument, 0, 'a'},
	{NULL, 0, NULL, 0}
};

void loadFont(const char *file, char type)
{
	if(type != 'c'){
		fprintf(stderr, "Non ccf binary font not implemented yet!\n");
		exit(1);
	}

	unsigned len = ccFileInfoGet(file).size;

	FILE *fp = fopen(file, "rb");
	if(!fp){
		fprintf(stderr, "Can not open file: %s\n", file);
		exit(1);
	}

	unsigned char *bin = (unsigned char*)malloc(len);
	fread(bin, 1, len, fp);

	fclose(fp);
	
	if(ccfBinToFont(&font, bin, len) == -1){
		fprintf(stderr, "Binary font failed: invalid version\n");
		exit(1);
	}

	free(bin);
}
	
// Loop through the asset directory to font a suitable font
int pickFont()
{
	ccFileDir file;
	if(ccFileDirFindFirst(&file, assetdir) != CC_SUCCESS){
		fprintf(stderr, "Can not open asset directory \"%s\"\n", assetdir);
		exit(1);
	}
	
	while(ccFileDirFind(&file) == CC_SUCCESS){
		if(file.isDirectory){
			continue;
		}

		const char *ext = strrchr(file.name, '.');
		if(!ext || ext == file.name){
			continue;
		} else if(strcmp(ext + 1, "ccf") == 0){
			char fontfile[strlen(assetdir) + strlen(file.name) + 1];
			strcpy(fontfile, assetdir);
			strcpy(fontfile + strlen(assetdir), file.name);
			loadFont(fontfile, 'c');
			return 0;
		}
	}

	return -1;
}

int main(int argc, char **argv)
{
	while(1){
		int index;
		int c = getopt_long(argc, argv, "hva:", opts, &index);
		if(c == -1){
			break;
		}
		switch (c) {
			case 'v':
				printVersion();
				return 0;
			case 'h':
				printHelp();
				return 0;
			case 'a':
				strcpy(assetdir, optarg);
				break;
			default:
				return 1;
		}
	}

	// Add trailing / when missing from the asset dir
	size_t len = strlen(assetdir);
	if(assetdir[len - 1] != '/'){
		assetdir[len] = '/';
		assetdir[++len] = '\0';
	}

	if(pickFont() < 0){
		fprintf(stderr, "Could not find a font!\n");
		return 1;
	}
	runGame();

	return 0;
}
