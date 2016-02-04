#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <getopt.h>
#include <dirent.h>

#include <ccore/file.h>
#include <ccore/time.h>

#include "window.h"
#include "level.h"
#include "png.h"
#include "utils.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600

#define FRAME_CAP 0.25
#define FRAME_DELTA (1.0 / 60.0)

static char assetdir[256] = "./";
static lua_State *lua;

void loadLua()
{
	lua = luaL_newstate();
	luaL_openlibs(lua);

	levelRegisterLua(lua);
	windowRegisterLua(lua);
	pngRegisterLua(lua);

	char *file = findFileFromExtension(assetdir, "lua");
	if(file == NULL){
		fprintf(stderr, "Can not find any .lua files in the asset directory\n");
		exit(1);
	}

	lua_pushstring(lua, assetdir);
	lua_setglobal(lua, "assetdir");
	if(luaL_loadfile(lua, file) || lua_pcall(lua, 0, 0, 0)){
		fprintf(stderr, "Lua error: %s\n", lua_tostring(lua, -1));
		exit(1);
	}
}

void runGame()
{
	hideCursor();

	lua_getglobal(lua, "setup");
	lua_call(lua, 0, 0);

	uint64_t curtime = ccTimeMilliseconds();
	double acctime = 0.0;

	while(true){
		if(!updateWindow(lua)){
			break;
		}

		uint64_t newtime = ccTimeMilliseconds();
		double frametime = (newtime - curtime) * 0.001;
		curtime = newtime;

		if(frametime > FRAME_CAP){
			frametime = FRAME_CAP;
		}
		acctime += frametime;

		while(acctime >= FRAME_DELTA){
			acctime -= FRAME_DELTA;
			lua_getglobal(lua, "update");
			lua_call(lua, 0, 0);
		}

		lua_getglobal(lua, "render");
		lua_call(lua, 0, 0);
		renderWindow(2);
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
		switch(c){
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
