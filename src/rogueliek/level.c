#include "level.h"

#include <stdlib.h>
#include <time.h>

#include <ccNoise/ccNoise.h>

#include "window.h"

#define EROSION_DELTA_MAX 0.2
#define EROSION_TRANSFER_SEDIMENT_RATIO 0.1
#define EROSION_TRANSFER_WATER_RATIO 0.2
#define EROSION_WATER_MIN 0.2

int mwidth, mheight;
char *map = NULL;

static int l_generateMap(lua_State *lua)
{
	int width = luaL_checkinteger(lua, 1);
	int height = luaL_checkinteger(lua, 2);
	int seed = luaL_checkinteger(lua, 3);
	int scale = luaL_checkinteger(lua, 4);
	int erosionpasses = luaL_checkinteger(lua, 5);

	generateMap(width, height, seed, scale, erosionpasses);

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

static inline void setLowestDelta(ccnNoise *height, float *dmax, int index, int *inew, int x2, int y2)
{
	int index2 = x2 + height->width * y2;
	float delta = height->values[index] - height->values[index2];
	if(delta > *dmax){
		*dmax = delta;
		*inew = index2;
	}
}

static void erosion(ccnNoise *height, ccnNoise *water)
{
	int size = height->width * height->height;
	float *sediment = (float*)calloc(size, sizeof(float));

	int i;
	for(i = 1; i < height->height - 1; i++){
		int j;
		for(j = 1; j < height->width - 1; j++){
			int index = j + i * height->width;
			// Ignore if there is no water on this point
			if(water->values[index] < EROSION_WATER_MIN){
				continue;
			}

			// Find the biggest delta
			float delta = 0;
			int indexlowest = -1;
			setLowestDelta(height, &delta, index, &indexlowest, j - 1, i);
			setLowestDelta(height, &delta, index, &indexlowest, j + 1, i);
			setLowestDelta(height, &delta, index, &indexlowest, j, i - 1);
			setLowestDelta(height, &delta, index, &indexlowest, j, i + 1);
			setLowestDelta(height, &delta, index, &indexlowest, j - 1, i - 1);
			setLowestDelta(height, &delta, index, &indexlowest, j - 1, i + 1);
			setLowestDelta(height, &delta, index, &indexlowest, j + 1, i - 1);
			setLowestDelta(height, &delta, index, &indexlowest, j + 1, i + 1);

			// Ignore this point if all the surrounding points are higher
			if(indexlowest == -1 || delta > EROSION_DELTA_MAX){
				continue;
			}

			float waterdelta = water->values[i] * delta * EROSION_TRANSFER_WATER_RATIO;
			sediment[indexlowest] += waterdelta;
			sediment[index] -= waterdelta;
		}
	}

	for(i = 0; i < height->width * height->height; i++){
		height->values[i] -= sediment[i] * EROSION_TRANSFER_SEDIMENT_RATIO * water->values[i];
		water->values[i] += sediment[i];
	}

	free(sediment);
}

void levelRegisterLua(lua_State *lua)
{
	lua_register(lua, "generatemap", l_generateMap);
	lua_register(lua, "rendermap", l_renderMap);
}

void generateMap(int width, int height, int seed, int scale, int erosionpasses)
{
	if(map != NULL){
		free(map);
	}
	time_t *tseed = (time_t*)NULL;
	if(seed != 0){
		*tseed = (time_t)seed;
	}
	srand(time(tseed));

	ccnNoise heightmap, water;
	ccnNoiseAllocate2D(heightmap, width, height);
	ccnNoiseAllocate2D(water, width, height);

	ccnNoiseConfiguration config = {
		.seed = rand(),
		.storeMethod = CCN_STORE_SET,
		.x = 0, .y = 0,
		.tileConfiguration = {
			.tileMethod = CCN_TILE_CARTESIAN,
			.xPeriod = 1, .yPeriod = 1
		},
		.range.low = 0, .range.high = 0.9
	};

	ccnGenerateOpenSimplex2D(&heightmap, &config, scale);

	config.seed = rand();
	config.range.low = 0.4;
	config.range.high = 0.5;
	ccnGenerateOpenSimplex2D(&water, &config, 1);

	config.seed = rand();
	config.storeMethod = CCN_STORE_ADD;
	config.range.low = 0.0;
	config.range.high = 0.05;
	ccnGenerateOpenSimplex2D(&heightmap, &config, 1);

	unsigned int i;
	for(i = 0; i < erosionpasses; i++){
		erosion(&heightmap, &water);
	}

	map = (char*)malloc(width * height);	

	for(i = 0; i < width * height; i++){
		if(water.values[i] > 0.95){
			map[i] = 255;
		}else{
			map[i] = heightmap.values[i] * 254;
		}
	}

	ccnNoiseFree(water);
	ccnNoiseFree(heightmap);

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
			/*
				 if(v < 20){
				 drawChar(i + x, j + y, '^', 128, 128, 128);
				 }
				 */

			if(v == 255){
				drawChar(i + x, j + y, '~', 128, 128, 255);
			}else if(v > 215){
				drawChar(i + x, j + y, '^', 255, 255, 255);
			}else if(v > 180){
				drawChar(i + x, j + y, '%', v >> 1, v >> 1, v >> 2);
			}else if(v > 80){
				drawChar(i + x, j + y, '#', 0, (v >> 1) + 90, 0);
			}else{
				drawChar(i + x, j + y, '=', 237 / 1.5, 201 / 1.5, 175 / 1.5);
			}
		}
	}
}
