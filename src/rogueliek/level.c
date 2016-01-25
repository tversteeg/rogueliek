#include "level.h"

#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <ccNoise/ccNoise.h>

#include "window.h"

#define NOISE_PERSISTANCE 0.1
#define NOISE_FREQUENCY 3

#define EROSION_DELTA_MIN 0.1
#define EROSION_SEDIMENT_CAPACITY 0.9
#define EROSION_SEDIMENT_TRIGGER 0.2
#define EROSION_SEDIMENT_DISOLVE 0.4
#define EROSION_SEDIMENT_DEPOSIT 0.3
#define EROSION_TRANSFER_WATER_RATIO 0.1
#define EROSION_WATER_MIN 0.1
#define EROSION_GRAVITY 0.5

#define max(a,b) \
	({ __typeof__ (a) _a = (a); \
	 __typeof__ (b) _b = (b); \
	 _a > _b ? _a : _b; })

#define min(a,b) \
	({ __typeof__ (a) _a = (a); \
	 __typeof__ (b) _b = (b); \
	 _a < _b ? _a : _b; })

typedef struct {
	float land, water, sediment;
	float vx, vy;
	float outflow[4];
} hydro_t;

int mwidth, mheight;
char *map = NULL;

static int l_generateMap(lua_State *lua)
{
	int width = luaL_checkinteger(lua, 1);
	int height = luaL_checkinteger(lua, 2);
	int seed = luaL_checkinteger(lua, 3);
	int octaves = luaL_checkinteger(lua, 4);
	int erosionpasses = luaL_checkinteger(lua, 5);

	generateMap(width, height, seed, octaves, erosionpasses);

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

static inline void setLowestDelta(ccnNoise *height, ccnNoise *water, float *dmax, int index, int *inew, int x2, int y2)
{
	int index2 = x2 + height->width * y2;
	float delta = (height->values[index] + water->values[index]) - (height->values[index2] + water->values[index2]);
	if(delta > *dmax){
		*dmax = delta;
		*inew = index2;
	}
}

static inline float calculateOutflow(hydro_t *h, int i1, int i2, int fi)
{
	float flow = h[i1].outflow[fi];
	float heightdiff = (h[i1].land + h[i1].water) - (h[i2].land + h[i2].water);

	return max(0, flow + heightdiff * EROSION_GRAVITY);
}

static void hydralicErosion(ccnNoise *landheight, int passes)
{
	int width = landheight->width;
	int height = landheight->height;
	int size = width * height;

	hydro_t *h = (hydro_t*)calloc(size, sizeof(hydro_t));

	for(int i = 0; i < size; i++){
		h[i].land = landheight->values[i];
	}

	ccnNoiseConfiguration rainconfig = {
		.seed = rand(),
		.storeMethod = CCN_STORE_SET,
		.x = 0, .y = 0,
		.tileConfiguration = {
			.tileMethod = CCN_TILE_CARTESIAN,
			.xPeriod = 1, .yPeriod = 1
		},
		.range = {
			.low = 0, 
			.high = 0.1
		}
	};

	ccnNoise rain;
	ccnNoiseAllocate2D(rain, width, height);
	for(int p = 0; p < passes; p++){
		ccnGenerateWhiteNoise2D(&rain, &rainconfig);

		for(int i = 0; i < size; i++){
			h[i].water += rain.values[i];
		}

		for(int i = 0; i < size; i++){
			h[i].outflow[0] = calculateOutflow(h, i, i - 1, 0);
			h[i].outflow[1] = calculateOutflow(h, i, i + 1, 1);
			h[i].outflow[2] = calculateOutflow(h, i, i - width, 2);
			h[i].outflow[3] = calculateOutflow(h, i, i + width, 3);
		}

#define _GET_OUTFLOW(h, x, y, width, i) h[(x) + (y) * (width)].outflow[i]
		for(int y = 1; y < height - 1; y++){
			for(int x = 1; x < width - 1; x++){
				float fl = _GET_OUTFLOW(h, x - 1, y, width, 1);
				float fr = _GET_OUTFLOW(h, x + 1, y, width, 0);
				float ft = _GET_OUTFLOW(h, x, y - 1, width, 3);
				float fb = _GET_OUTFLOW(h, x, y + 1, width, 2);

				int i = x + y * width;
				float *outp = h[i].outflow;
				h[i].water += (fl + fr + ft + fb) - (outp[0] + outp[1] + outp[2] + outp[3]);
				h[i].vx = (fr - outp[0] + outp[1] - fl) / 2;
				h[i].vy = (fb - outp[2] + outp[3] - ft) / 2;
			}
		}
#undef _GET_OUTFLOW

		for(int i = 0; i < size; i++){
			float sedcap = EROSION_SEDIMENT_CAPACITY * (h[i].vx * h[i].vy) / 2;
			if(sedcap > EROSION_SEDIMENT_TRIGGER){
				float sed = EROSION_SEDIMENT_DISOLVE * (sedcap - EROSION_SEDIMENT_TRIGGER);
				h[i].land -= sed;
				h[i].sediment += sed;
			}else{
				float sed = EROSION_SEDIMENT_DEPOSIT * (sedcap - EROSION_SEDIMENT_TRIGGER);
				h[i].land += sed;
				h[i].sediment -= sed;
			}
		}
	}

	ccnNoiseFree(rain);
	free(h);
}

void levelRegisterLua(lua_State *lua)
{
	lua_register(lua, "generatemap", l_generateMap);
	lua_register(lua, "rendermap", l_renderMap);
}

void generateMap(int width, int height, int seed, int octaves, int erosionpasses)
{
	if(map != NULL){
		free(map);
	}

	time_t *tseed = (time_t*)NULL;
	if(seed != 0){
		*tseed = (time_t)seed;
	}
	srand(time(tseed));

	ccnNoise heightmap;
	ccnNoiseAllocate2D(heightmap, width, height);

	ccnNoiseConfiguration config = {
		.seed = rand(),
		.storeMethod = CCN_STORE_SET,
		.x = 0, .y = 0,
		.tileConfiguration = {
			.tileMethod = CCN_TILE_CARTESIAN,
			.xPeriod = 1, .yPeriod = 1
		},
		.range = {
			.low = 0, 
			.high = NOISE_PERSISTANCE
		}
	};

	float amplitude = NOISE_PERSISTANCE;
	float totalamps = amplitude;
	ccnGenerateValueNoise2D(&heightmap, &config, NOISE_FREQUENCY << 1, CCN_INTERP_CUBIC);
	config.storeMethod = CCN_STORE_ADD;
	for(unsigned i = 1; i < octaves - 1; i++){
		amplitude *= NOISE_PERSISTANCE;
		totalamps += amplitude;
		config.range.high = amplitude;
		ccnGenerateValueNoise2D(&heightmap, &config, NOISE_FREQUENCY << i, CCN_INTERP_CUBIC);
	}

	float highest = 0;
	for(unsigned i = 0; i < width * height; i++){
		if(heightmap.values[i] > highest){
			highest = heightmap.values[i];
		}
	}

	float ampscale = 1.0 / highest;
	for(unsigned i = 0; i < width * height; i++){
		heightmap.values[i] *= ampscale;
	}

	hydralicErosion(&heightmap, erosionpasses);

	map = (char*)malloc(width * height);	

	for(unsigned i = 0; i < width * height; i++){
		map[i] = heightmap.values[i] * 253;
	}

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
			if(v == 255){
				drawChar(i + x, j + y, '~', 32, 32, 255);
			}else if(v == 254){
				drawChar(i + x, j + y, '~', 128, 128, 255);
			}else if(v > 215){
				drawChar(i + x, j + y, '^', 255, 255, 255);
			}else if(v > 160){
				drawChar(i + x, j + y, '%', v >> 1, v >> 1, v >> 2);
			}else if(v > 80){
				drawChar(i + x, j + y, '#', 0, (v >> 1) + 90, 0);
			}else{
				drawChar(i + x, j + y, '=', 237 / 1.5, 201 / 1.5, 175 / 1.5);
			}
			//drawChar(i + x, j + y, '#', v, v, v);
		}
	}
}
