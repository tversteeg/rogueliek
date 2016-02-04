#include "delaunay.h"

#include <stdlib.h>

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

	float *x = (float*)malloc(len * sizeof(float));
	float *y = (float*)malloc(len * sizeof(float));

	for(int i = 1; i < len; i++){
		lua_pushinteger(lua, i);
		lua_gettable(lua, 1);
		x[i] = lua_tonumber(lua, -1);

		lua_pushinteger(lua, i);
		lua_gettable(lua, 2);
		y[i] = lua_tonumber(lua, -1);
	}
	return 0;
}

static void sortPositions(_vert **vs)
{

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
	}

	sortPositions(&vs);

	return 0;
}
