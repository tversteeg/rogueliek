#include "level.h"

#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <ccNoise/ccNoise.h>

#include "window.h"

#define NOISE_PERSISTANCE 0.1
#define NOISE_FREQUENCY 3

#define EROSION_SEDIMENT_CAPACITY 25.0
#define EROSION_SEDIMENT_DISOLVE 0.0001 * 12 * 10
#define EROSION_SEDIMENT_DEPOSIT 0.0001 * 12 * 10
#define EROSION_RAIN 0.01
#define EROSION_EVAPORATION 0.01
#define EROSION_GRAVITY 9.5
#define EROSION_FLUX 0.005

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

	return max(0, flow + heightdiff * EROSION_GRAVITY * EROSION_FLUX);
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
			.high = EROSION_RAIN
		}
	};

	ccnNoise rain;
	ccnNoiseAllocate2D(rain, width, height);
	for(int p = 0; p < passes; p++){
		ccnGenerateWhiteNoise2D(&rain, &rainconfig);

		// Rain
		for(int i = 0; i < size; i++){
			h[i].water += rain.values[i];
		}

		// Flux
		for(int i = 0; i < size; i++){
			h[i].outflow[0] = calculateOutflow(h, i, i - 1, 0);
			h[i].outflow[1] = calculateOutflow(h, i, i + 1, 1);
			h[i].outflow[2] = calculateOutflow(h, i, i - width, 2);
			h[i].outflow[3] = calculateOutflow(h, i, i + width, 3);

			float sumflux = h[i].outflow[0] + h[i].outflow[1] + h[i].outflow[2] + h[i].outflow[3];
			float k = min(1.0, h[i].water / sumflux);
			for(int j = 0; j < 4; j++){
				h[i].outflow[j] *= k;
			}
		}

#define _GET_OUTFLOW(h, x, y, width, i) h[(x) + (y) * (width)].outflow[i]
		// Flux and velocity
		for(int y = 1; y < height - 1; y++){
			for(int x = 1; x < width - 1; x++){
				float fl = _GET_OUTFLOW(h, x - 1, y, width, 1);
				float fr = _GET_OUTFLOW(h, x + 1, y, width, 0);
				float ft = _GET_OUTFLOW(h, x, y - 1, width, 3);
				float fb = _GET_OUTFLOW(h, x, y + 1, width, 2);

				int i = x + y * width;
				float *outp = h[i].outflow;
				float oldwater = h[i].water;
				h[i].water = max(0.0, h[i].water + (fl + fr + ft + fb) - (outp[0] + outp[1] + outp[2] + outp[3]));

				float meanwater = 0.5 * (oldwater + h[i].water);
				h[i].vx = 0.5 * (fr - outp[0] + outp[1] - fl) / meanwater;
				h[i].vy = 0.5 * (fb - outp[2] + outp[3] - ft) / meanwater;
			}
		}
#undef _GET_OUTFLOW

		// Sedimentation and deposition
		for(int i = 0; i < size; i++){
			// Normalize vector (vx, vy, 2)
			float len = sqrt(h[i].vx * h[i].vx + h[i].vy * h[i].vy + 4);
			float sina = max(0.1, sin(acos(2 / len)));

			len = sqrt(h[i].vx * h[i].vx + h[i].vy * h[i].vy);
			float minwater = min(h[i].water, 0.01) / 0.01;
			float sedcap = EROSION_SEDIMENT_CAPACITY * len * sina * minwater;
			float delta = sedcap - h[i].sediment;
			if(delta > 0){
				float sed = EROSION_SEDIMENT_DISOLVE * delta;
				h[i].land -= sed;
				h[i].sediment += sed;
				h[i].water += sed;
			}else if(delta < 0){
				float sed = EROSION_SEDIMENT_DEPOSIT * delta;
				h[i].land -= sed;
				h[i].sediment += sed;
				h[i].water += sed;
			}
		}

		// Evaporation
		for(int i = 0; i < size; i++){
			h[i].water *= 1 - EROSION_EVAPORATION;
			if(h[i].water < 0.005){
				h[i].water = 0;
			}
		}
	}	
	
	// Apply
	for(int i = 0; i < size; i++){
		landheight->values[i] = h[i].land;
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
