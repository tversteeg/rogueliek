#include "level.h"

#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <ccNoise/ccNoise.h>

#include "window.h"

#define NOISE_PERSISTANCE 0.1
#define NOISE_FREQUENCY 3

#define HEAT_MAX 30.0
#define HEAT_MIN -5.0

#define EROSION_SEDIMENT_CAPACITY 200.0
#define EROSION_SEDIMENT_DISOLVE 0.0001 * 12 * 10
#define EROSION_SEDIMENT_DEPOSIT 0.0001 * 12 * 10
#define EROSION_RAIN 0.05
#define EROSION_EVAPORATION 0.025
#define EROSION_GRAVITY 9.5
#define EROSION_FLUX 0.002

enum {
	MAP_WATER = 0,
	MAP_WATER_FROZEN,
	MAP_FOREST_TAIGA,
	MAP_FOREST_TEMPERATE,
	MAP_FOREST_TROPICAL,
	MAP_PLAINS,
	MAP_PLAINS_TAIGA,
	MAP_MOUNTAIN,
	MAP_MOUNTAIN_FROZEN,
	MAP_DESERT
};

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

static inline float calculateOutflow(hydro_t *h, int i1, int i2, int fi)
{
	float flow = h[i1].outflow[fi];
	float heightdiff = (h[i1].land + h[i1].water) - (h[i2].land + h[i2].water);

	return max(0, flow + heightdiff * EROSION_GRAVITY * EROSION_FLUX);
}

static void hydralicErosion(ccnNoise *landheight, ccnNoise *waterheight, ccnNoise *rainmap, int passes)
{
	int width = landheight->width;
	int height = landheight->height;
	int size = width * height;

	hydro_t *h = (hydro_t*)calloc(size, sizeof(hydro_t));

	for(int i = 0; i < size; i++){
		h[i].land = landheight->values[i];
		h[i].water = waterheight->values[i];
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
			h[i].water += rainmap->values[i] + rain.values[i];
		}

		// Flux
		for(int i = width; i < size - width; i++){
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
		waterheight->values[i] = h[i].water;
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

	int rw = width + 2;
	int rh = height + 2;
	ccnNoise heightmap;
	ccnNoiseAllocate2D(heightmap, rw, rh);

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
	for(unsigned i = 0; i < rw * rh; i++){
		if(heightmap.values[i] > highest){
			highest = heightmap.values[i];
		}
	}

	float ampscale = 1.0 / highest;
	for(unsigned i = 0; i < rw * rh; i++){
		heightmap.values[i] *= ampscale;
	}

	config = (ccnNoiseConfiguration){
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

	ccnNoise rainmap;
	ccnNoiseAllocate2D(rainmap, rw, rh);
	ccnGenerateValueNoise2D(&rainmap, &config, 10, CCN_INTERP_CUBIC);

	ccnNoise watermap;
	watermap.values = (float*)calloc(rw * rh, sizeof(float));

	hydralicErosion(&heightmap, &watermap, &rainmap, erosionpasses);

	ccnNoise heatmap;
	ccnNoiseAllocate2D(heatmap, width, height);
	for(unsigned y = 0; y < height; y++){
		for(unsigned x = 0; x < width; x++){
			heatmap.values[x + y * width] = HEAT_MIN + (HEAT_MAX - HEAT_MIN) * (y / (float)height) * (0.5 + heightmap.values[(x + 1) + (y + 1) * rw] * 0.5);
		}
	}

	map = (char*)malloc(width * height);	

	for(unsigned y = 0; y < height; y++){
		for(unsigned x = 0; x < width; x++){
			unsigned i1 = (x + 1) + (y + 1) * rw;
			unsigned i2 = x + y * width;

			if(watermap.values[i1] > 2.0){
				if(heatmap.values[i2] > 0.0){
					map[i2] = MAP_WATER;
				}else{
					map[i2] = MAP_WATER_FROZEN;
				}
			}else{
				map[i2] = heightmap.values[i1] * 253;
				if(heightmap.values[i1] > 0.8){
					if(heatmap.values[i2] < 0.0){
						map[i2] = MAP_MOUNTAIN_FROZEN;
					}else{
						map[i2] = MAP_MOUNTAIN;
					}
				}else if(heatmap.values[i2] > 20.0){
					if(rainmap.values[i1] > EROSION_RAIN / 2){
						map[i2] = MAP_FOREST_TROPICAL;
					}else{
						map[i2] = MAP_DESERT;
					}
				}else if(heatmap.values[i2] < 0.0){
					if(heightmap.values[i1] > 0.8){
						map[i2] = MAP_MOUNTAIN;
					}else{
						map[i2] = MAP_PLAINS_TAIGA;
					}
				}else{
					map[i2] = MAP_PLAINS;
				}
			}
		}
	}

	ccnNoiseFree(heightmap);
	ccnNoiseFree(watermap);
	ccnNoiseFree(heatmap);

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
			char c = '\0';
			unsigned char col[3] = {0};
			unsigned char cbg[3] = {0};
			unsigned char v = map[mapx + i + (mapy + j) * mwidth];
			switch(v){
				case MAP_WATER:
					c = '~';
					col[0] = 32, col[1] = 32, col[2] = 255;
					cbg[0] = 64, cbg[1] = 64, cbg[2] = 255;
					break;
				case MAP_WATER_FROZEN:
					c = '~';
					col[0] = 255, col[1] = 255, col[2] = 255;
					cbg[0] = 128, cbg[1] = 128, cbg[2] = 255;
					break;
				case MAP_DESERT:
					c = '.';
					col[0] = 128, col[1] = 128, col[2] = 64;
					cbg[0] = 128, cbg[1] = 128, cbg[2] = 64;
					break;
				case MAP_MOUNTAIN:
					c = '^';
					col[0] = 128, col[1] = 128, col[2] = 64;
					cbg[0] = 32, cbg[1] = 32, cbg[2] = 16;
					break;
				case MAP_MOUNTAIN_FROZEN:
					c = '^';
					col[0] = 255, col[1] = 255, col[2] = 255;
					cbg[0] = 64, cbg[1] = 64, cbg[2] = 64;
					break;
				case MAP_FOREST_TAIGA:
					c = '%';
					col[0] = 128, col[1] = 255, col[2] = 180;
					cbg[0] = 64, cbg[1] = 64, cbg[2] = 128;
					break;
				case MAP_FOREST_TROPICAL:
					c = '%';
					col[0] = 128, col[1] = 255, col[2] = 180;
					cbg[0] = 0, cbg[1] = 120, cbg[2] = 0;
					break;
				case MAP_PLAINS:
					c = ';';
					col[0] = 0, col[1] = 255, col[2] = 0;
					cbg[0] = 32, cbg[1] = 64, cbg[2] = 32;
					break;
				case MAP_PLAINS_TAIGA:
					c = ';';
					col[0] = 128, col[1] = 255, col[2] = 180;
					cbg[0] = 64, cbg[1] = 128, cbg[2] = 128;
					break;
				default:
					c = 'E';
					col[0] = 255, col[1] = 255, col[2] = 0;
					cbg[0] = 0, cbg[1] = 255, cbg[2] = 255;
					break;
			}
			drawCharBack(i + x, j + y, cbg[0], cbg[1], cbg[2]);
			drawChar(i + x, j + y, c, col[0], col[1], col[2]);
			//drawChar(i + x, j + y, '#', v, v, v);
		}
	}
}
