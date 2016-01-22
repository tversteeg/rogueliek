#include "level.h"

#include <stdlib.h>
#include <time.h>

#include <ccNoise/ccNoise.h>

#include "window.h"

int mwidth, mheight;
char *map = NULL;

static int l_generateMap(lua_State *lua)
{
	int width = luaL_checkinteger(lua, 1);
	int height = luaL_checkinteger(lua, 2);
	int seed = luaL_checkinteger(lua, 3);

	generateMap(width, height, seed);

	return 0;
}

static int l_renderMap(lua_State *lua)
{
	int x = luaL_checkinteger(lua, 1);
	int y = luaL_checkinteger(lua, 2);
	int width = luaL_checkinteger(lua, 3);
	int height = luaL_checkinteger(lua, 4);
	int mapx = luaL_checkinteger(lua, 5);
	int mapy = luaL_checkinteger(lua, 6);

	renderMap(x, y, width, height, mapx, mapy);

	return 0;
}

void levelRegisterLua(lua_State *lua)
{
	lua_register(lua, "generatemap", l_generateMap);
	lua_register(lua, "rendermap", l_renderMap);
}

void generateMap(int width, int height, int seed)
{
	if(map != NULL){
		free(map);
	}
	time_t *tseed = (time_t*)NULL;
	if(seed != 0){
		*tseed = (time_t)seed;
	}
	srand(time(tseed));

	ccnNoise noise;
	ccnNoiseAllocate2D(noise, width, height);

	ccnNoiseConfiguration config = {
		.seed = rand(),
		.storeMethod = CCN_STORE_SET,
		.x = 0, .y = 0,
		.tileConfiguration = {
			.tileMethod = CCN_TILE_CARTESIAN,
			.xPeriod = 1, .yPeriod = 1
		},
		.range.low = 0, .range.high = 1
	};

	ccnGenerateOpenSimplex2D(&noise, &config, 4);

	map = (char*)malloc(width * height);	
	
	unsigned int i;
	for(i = 0; i < width * height; i++){
		map[i] = noise.values[i] * 255;
	}

	ccnNoiseFree(noise);

	mwidth = width;
	mheight = height;
}

void renderMap(int x, int y, int width, int height, int mapx, int mapy)
{
	if(width > mwidth - mapx){
		width = mwidth - mapx;
	}
	if(height > mheight - mapy){
		height = mheight - mapy;
	}
	int startx = 0;
	if(mapx < 0){
		startx = -mapx;
	}
	int starty = 0;
	if(mapy < 0){
		starty = -mapy;
	}
	int i;
	for(i = startx; i < width; i++){
		int j;
		for(j = starty; j < height; j++){
			unsigned char v = map[mapx + i + (mapy + j) * mwidth];
			if(v < 20){
				drawChar(i + x, j + y, '^', 128, 128, 128);
			}else if(v < 128){
				drawChar(i + x, j + y, '#', 0, 128 - v, 0);
			}else{
				drawChar(i + x, j + y, '%', v >> 3, v >> 3, v >> 5);
			}
		}
	}
}
