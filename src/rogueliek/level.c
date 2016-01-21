#include "level.h"

#include <stdlib.h>
#include <time.h>

int mwidth, mheight;
char *map;

static int l_generateMap(lua_State *lua)
{
	int width = luaL_checkinteger(lua, 1);
	int height = luaL_checkinteger(lua, 2);
	int seed = luaL_checkinteger(lua, 3);

	generateMap(width, height, seed);

	return 0;
}

void levelRegisterLua(lua_State *lua)
{
	lua_register(lua, "Level.generatemap", l_generateMap);
}

void generateMap(int width, int height, int seed)
{
	mwidth = width;
	mheight = height;

	map = (char*)malloc(mwidth * mheight);

	time_t *tseed = (time_t*)NULL;
	if(seed != 0){
		*tseed = (time_t)seed;
	}
	srand(time(tseed));

	unsigned int i;
	for(i = 0; i < width * height; i++){
		char tile = '.';
		if(i % width == 0 || i % width == width - 1 || i < width || i > width * height - width){
			tile = '#';
		}else if(rand() % 3 < 1){
			tile = '#';
		}
		map[i] = tile;
	}
}
