#include "level.h"

int mwidth, mheight;

void generateMap(int width, int height, int seed)
{
	mwidth = width;
	mheight = height;
}

int l_generateMap(lua_State *lua)
{
	int width = luaL_checkinteger(lua, 1);
	int height = luaL_checkinteger(lua, 2);
	int seed = luaL_checkinteger(lua, 3);
	
	generateMap(width, height, seed);

	return 0;
}
