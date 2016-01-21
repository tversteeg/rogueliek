#include "level.h"

#include <ccNoise/ccNoise.h>

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
			.xPeriod = 2, .yPeriod = 2
		},
		.range.low = 0, .range.high = 1
	};

	ccnGenerateOpenSimplex2D(&noise, &config, 32);

	map = (char*)malloc(mwidth * mheight);	
	
	unsigned int i;
	for(i = 0; i < width * height; i++){
		map[i] = noise.values[i] * 255;
	}

	ccnNoiseFree(noise);

	mwidth = width;
	mheight = height;
}
