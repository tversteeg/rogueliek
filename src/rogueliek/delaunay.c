#include "delaunay.h"

#include <stdlib.h>
#include <stdbool.h>

#define SORT_MAX_LEVELS 64

typedef struct {
	float x, y;
} _vert;

static int l_delaunayTriangulate(lua_State *lua)
{
	luaL_checktype(lua, 1, LUA_TTABLE);
	luaL_checktype(lua, 2, LUA_TTABLE);

	int len = luaL_len(lua, 1);
	if(luaL_len(lua, 2) != len){
		luaL_error(lua, "Lua: delaunaytriangulate, x and y arrays need to be the same length\n");
		exit(1);
	}

	len--;
	float *xs = (float*)malloc(len * sizeof(float));
	float *ys = (float*)malloc(len * sizeof(float));

	for(int i = 1; i < len + 1; i++){
		lua_pushinteger(lua, i);
		lua_gettable(lua, 1);
		xs[i - 1] = lua_tonumber(lua, -1);

		lua_pushinteger(lua, i);
		lua_gettable(lua, 2);
		ys[i - 1] = lua_tonumber(lua, -1);
	}

	int *ids = NULL;
	delaunayTriangulate(&ids, xs, ys, len);

	free(xs);
	free(ys);

	return 0;
}

static bool sortFunc(_vert v1, _vert v2)
{
	if(v1.x < v2.x){
		return true;
	}else if(v1.x == v2.x){
		return v1.y < v2.y;
	}
	return false;
}

static void swapInt(int *x, int *y){
	*x ^= *y;
	*y ^= *x;
	*x ^= *y;
}

// Quicksort
static void sortPositions(_vert *vs, int amount)
{
	int beg[SORT_MAX_LEVELS], end[SORT_MAX_LEVELS];

	beg[0] = 0;
	end[0] = amount;
	for(int i = 0; i >= 0;){
		int l = beg[i];
		int r = end[i] - 1;
		if(l < r){
			_vert piv = vs[l];
			while(l < r){
				while(sortFunc(piv, vs[r]) && l < r){
					r--;
				}
				if(l < r){
					vs[l++] = vs[r];
				}
				while(!sortFunc(piv, vs[l]) && l < r){
					l++;
				}
				if(l < r){
					vs[r--] = vs[l];
				}
			}
			vs[l] = piv;
			beg[i + 1] = l + 1;
			end[i + 1] = end[i];
			end[i++] = l;
			if(end[i] - beg[i] > end[i - 1] - beg[i - 1]){
				swapInt(beg + i, beg + i - 1);
				swapInt(end + i, end + i - 1);
			}
		}else{
			i--;
		}
	}
}

void delaunayRegisterLua(lua_State *lua)
{
	lua_register(lua, "delaunaytriangulate", l_delaunayTriangulate);
}

int delaunayTriangulate(int **ids, const float *x, const float *y, int amount)
{
	_vert *vs = (_vert*)malloc(amount * sizeof(_vert));
	for(int i = 0; i < amount; i++){
		vs[i].x = x[i];
		vs[i].y = y[i];
		printf("(%.f, %.f) ", vs[i].x, vs[i].y);
	}

	sortPositions(vs, amount);
	printf("\n");

	for(int i = 0; i < amount; i++){
		printf("(%.f, %.f) ", vs[i].x, vs[i].y);
	}
	printf("\n\n");

	return 0;
}
