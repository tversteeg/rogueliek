#pragma once

#include <lua5.3/lua.h>
#include <lua5.3/lauxlib.h>
#include <lua5.3/lualib.h>

void generateMap(int width, int height, int seed);
int l_generateMap(lua_State *lua);
