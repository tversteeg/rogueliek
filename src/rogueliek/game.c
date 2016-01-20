#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <getopt.h>
#include <dirent.h>

#include <ccore/file.h>

#include <lua5.3/lua.h>
#include <lua5.3/lauxlib.h>
#include <lua5.3/lualib.h>

#include "window.h"
#include "level.h"
#include "utils.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600

static char assetdir[256] = "./";
static lua_State *lua;

void loadLua()
{
	lua = luaL_newstate();
	luaL_openlibs(lua);

	static const struct luaL_Reg maplib[] = {
		{"generate", l_generateMap},
		{NULL, NULL}};

	luaL_newlib(lua, maplib);

	char *file = findFileFromExtension(assetdir, "lua");
	if(file == NULL){
		fprintf(stderr, "Can not find any .lua files in the asset directory\n");
		exit(1);
	}

	FILE *fp = fopen(file, "r");
	if(!fp){
		fprintf(stderr, "Can not open file: %s\n", file);
		exit(1);
	}

	char buf[256];
	while(fgets(buf, sizeof(buf), fp) != NULL){
		int error = luaL_loadbuffer(lua, buf, strlen(buf), "line") || lua_pcall(lua, 0, 0, 0);

		if(error){
			fprintf(stderr, "Lua error: %s\n", lua_tostring(lua, -1));
			exit(1);
		}
	}

	fclose(fp);
}

void runGame()
{
	hideCursor();

	while(true){
		if(!updateWindow()){
			break;
		}

		lua_getglobal(lua, "update");
		lua_call(lua, 0, 0);

		drawString(getWidth() / 2 - strlen("Rogueliek") / 2 - 1, 0, "Rogueliek", 255, 128, 255);
		int i;
		for(i = 2; i < getWidth() - 2; i++){
			drawChar(i, 2, '#', 255, 255, 255);
			drawChar(i, getHeight() - 2, '#', 255, 255, 255);
		}
		for(i = 2; i < getHeight() - 1; i++){
			drawChar(2, i, '#', 255, 255, 255);
			drawChar(getWidth() - 2, i, '#', 255, 255, 255);
		}
		renderWindow(6);
	}
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

	if(pickFontFromDir(assetdir) < 0){
		fprintf(stderr, "Could not find a font!\n");
		return 1;
	}
	loadLua();
	createWindow("rogueliek - " TOSTRING(ROGUELIEK_VERSION), DEFAULT_WIDTH, DEFAULT_HEIGHT);
	runGame();

	showCursor();
	destroyWindow();

	lua_close(lua);

	return 0;
}
